#include "MainWindow.h"
#include "ModuleDelegate.h"
#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QDebug>
#include <QAbstractItemView>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

static const int ModuleDataRole = Qt::UserRole + 1;
static const int ExpansionStateRole = Qt::UserRole + 2;

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
  moduleList->setItemDelegate(new ModuleDelegate(this));

  layout->addWidget(moduleList);
  setCentralWidget(central);

  connect(moduleList, &QListWidget::itemChanged, this, &MainWindow::moduleToggled);
  connect(moduleList, &QListWidget::itemClicked, this, &MainWindow::moduleClicked);

  loadModules();
}

void MainWindow::loadModules() {
  QString modulesPath = "../../modules";
  QDir modulesDir(modulesPath);
  if (!modulesDir.exists()) {
    qWarning() << "Modules directory does not exist:" << modulesPath;
    return;
  }

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

    QListWidgetItem* item = new QListWidgetItem(modName);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Unchecked);
    item->setData(ModuleDataRole, obj);
    item->setData(ExpansionStateRole, false);

    moduleList->addItem(item);
  }
}

void MainWindow::moduleToggled(QListWidgetItem* item) {
  QString cmd = (item->checkState() == Qt::Checked ? "ENABLE " : "DISABLE ") + item->text();
  sendIpcCommand(cmd);
}

void MainWindow::moduleClicked(QListWidgetItem* item) {
  bool expanded = item->data(ExpansionStateRole).toBool();
  item->setData(ExpansionStateRole, !expanded);

  QModelIndex index = moduleList->indexFromItem(item);
  QSize newSize = moduleList->sizeHintForIndex(index);
  item->setSizeHint(newSize);

  moduleList->update(moduleList->visualItemRect(item));
  moduleList->scrollToItem(item);
}
