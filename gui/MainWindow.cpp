#include "MainWindow.hpp"
#include "ModuleDelegate.hpp"
#include "ModuleDataViewer.hpp"
#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QDebug>
#include <QAbstractItemView>
#include <QInputDialog>
#include <QMouseEvent>
#include <QEvent>
#include <QApplication>
#include <QCursor>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <QStyleOptionButton>
#include <QStyle>

static const int ModuleDataRole = Qt::UserRole + 1;
static const int ExpansionStateRole = Qt::UserRole + 2;
static const QString ModulesJsonPath = "../../modules/modules.json";

// load modules.json, return empty object if missing
QJsonObject loadModulesJson() {
  QFile file(ModulesJsonPath);
  QJsonObject j;
  if (file.exists() && file.open(QIODevice::ReadOnly)) {
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    j = doc.object();
    file.close();
  }
  return j;
}

// save modules.json
void saveModulesJson(const QJsonObject& j) {
  QFile file(ModulesJsonPath);
  if (file.open(QIODevice::WriteOnly)) {
    QJsonDocument doc(j);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
  } else {
    qWarning() << "Failed to write modules.json";
  }
}

void MainWindow::sendIpcCommand(const QString& cmd) {
  int sock = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) return;

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, "/tmp/prophet_gui.sock", sizeof(addr.sun_path) - 1);

  if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
    ::write(sock, cmd.toUtf8().constData(), cmd.size());
  }

  ::close(sock);
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  QWidget* central = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(central);

  moduleList = new QListWidget(this);
  moduleList->setSelectionMode(QAbstractItemView::NoSelection);

  auto *delegate = new ModuleDelegate(this);
  moduleList->setItemDelegate(delegate);

  connect(delegate, &ModuleDelegate::editRequested, this, &MainWindow::editModule);

  connect(delegate, &ModuleDelegate::viewRequested, this, [this](const QModelIndex &index){
      QJsonObject mod = index.data(ModuleDataRole).toJsonObject();
      QString modName = mod["name"].toString(index.data(Qt::DisplayRole).toString());
      QString moduleDir = "../../modules/" + modName;

      ModuleDataViewer* viewer = new ModuleDataViewer(moduleDir, this);
      viewer->setWindowTitle("Data Viewer - " + modName);
      viewer->resize(400, 300);
      viewer->exec();
      });

  moduleList->viewport()->installEventFilter(this);
  layout->addWidget(moduleList);
  setCentralWidget(central);

  connect(moduleList, &QListWidget::itemChanged, this, &MainWindow::moduleToggled);
  loadModules();

  setMinimumSize(200, 400);  // optional, sets minimum
  resize(200, 400);
}

void MainWindow::loadModules() {
  QString modulesPath = "../../modules";
  QDir modulesDir(modulesPath);
  if (!modulesDir.exists()) {
    qWarning() << "Modules directory does not exist:" << modulesPath;
    return;
  }

  QJsonObject persisted = loadModulesJson();
  bool updatedJson = false;

  moduleList->clear();
  QStringList entries = modulesDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

  for (const QString& entry : entries) {
    QString modJsonPath = modulesPath + "/" + entry + "/module.json";
    QFile file(modJsonPath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
      qWarning() << "Cannot open module.json for:" << entry;
      continue;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    QJsonObject obj = doc.object();
    QString modName = obj["name"].toString(entry);

    if (!persisted.contains(modName)) {
      persisted[modName] = false;
      updatedJson = true;
    }

    QListWidgetItem* item = new QListWidgetItem(modName);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

    bool isEnabled = persisted[modName].toBool();
    item->setCheckState(isEnabled ? Qt::Checked : Qt::Unchecked);
    item->setData(ModuleDataRole, obj);
    item->setData(ExpansionStateRole, false);

    moduleList->addItem(item);
  }

  if (updatedJson) saveModulesJson(persisted);
}

void MainWindow::moduleToggled(QListWidgetItem* item) {
  QString cmd = (item->checkState() == Qt::Checked ? "ENABLE " : "DISABLE ") + item->text();
  sendIpcCommand(cmd);

  QJsonObject persisted = loadModulesJson();
  persisted[item->text()] = (item->checkState() == Qt::Checked);
  saveModulesJson(persisted);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
  if (obj == moduleList->viewport() && event->type() == QEvent::MouseButtonRelease) {
    QMouseEvent* me = static_cast<QMouseEvent*>(event);
    QPoint pos = me->pos();

    QListWidgetItem* item = moduleList->itemAt(pos);
    if (!item) return false;

    QRect itemRect = moduleList->visualItemRect(item);

    QStyleOptionButton opt;
    opt.rect = itemRect;
    QRect checkRect = moduleList->style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &opt, moduleList);
    if (checkRect.contains(pos)) return false;

    QRect editRect(itemRect.right() - 60, itemRect.top() + 10, 50, 25);
    QRect viewRect(itemRect.right() - 130, itemRect.top() + 10, 60, 25);

    if (editRect.contains(pos) || viewRect.contains(pos))
      return false;

    bool expanded = item->data(ExpansionStateRole).toBool();
    item->setData(ExpansionStateRole, !expanded);
    QModelIndex index = moduleList->indexFromItem(item);
    item->setSizeHint(moduleList->sizeHintForIndex(index));
    moduleList->update(moduleList->visualItemRect(item));
    moduleList->scrollToItem(item);

    return true;
  }

  return QMainWindow::eventFilter(obj, event);
}

void MainWindow::editModule(const QModelIndex &index) {
  QJsonObject mod = index.data(ModuleDataRole).toJsonObject();
  QString modName = mod["name"].toString(index.data(Qt::DisplayRole).toString());

  QStringList additionalList;
  if (mod.contains("additional") && mod["additional"].isArray()) {
    for (const QJsonValue &v : mod["additional"].toArray())
      additionalList << v.toString();
  }

  QString text = additionalList.join(", ");
  bool ok = false;
  QString newText = QInputDialog::getText(this, "Edit 'additional' for " + modName,
      "Comma-separated values:", QLineEdit::Normal, text, &ok);
  if (!ok) return;

  QString moduleDir = "../../modules/" + modName;
  QString modJsonPath = moduleDir + "/module.json";

  QFile file(modJsonPath);
  if (!file.exists()) {
    qWarning() << "Cannot find module.json for" << modName;
    return;
  }

  if (file.open(QIODevice::ReadOnly)) {
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonObject obj = doc.object();
    QJsonArray arr;
    for (const QString &val : newText.split(",", Qt::SkipEmptyParts))
      arr.append(val.trimmed());
    obj["additional"] = arr;

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
      file.close();
    }

    QListWidgetItem *item = moduleList->item(index.row());
    if (item) {
      item->setData(ModuleDataRole, obj);
      if (item->data(ExpansionStateRole).toBool()) {
        QModelIndex idx = moduleList->indexFromItem(item);
        item->setSizeHint(moduleList->sizeHintForIndex(idx));
      }
      moduleList->update(moduleList->visualItemRect(item));
    }
  }
}
