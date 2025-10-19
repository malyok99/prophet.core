#include "MainWindow.h"
#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <QDir>
#include <QDebug>

void sendIpcCommand(const QString& cmd) {
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) return;
  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, "/tmp/prophet_gui.sock", sizeof(addr.sun_path)-1);
  if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0) {
    write(sock, cmd.toUtf8().constData(), cmd.size());
  }
  close(sock);
}
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  QWidget* central = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(central);
  moduleList = new QListWidget(this);
  moduleList->setSelectionMode(QAbstractItemView::NoSelection);
  layout->addWidget(moduleList);
  setCentralWidget(central);
  connect(moduleList, &QListWidget::itemChanged, this, &MainWindow::moduleToggled);
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
  if (entries.isEmpty()) {
    qWarning() << "No module subdirectories found in:" << modulesPath;
  }

  for (const QString& entry : entries) {
    QString modDirPath = modulesPath + "/" + entry;
    QString modJsonPath = modDirPath + "/module.json";

    QFile file(modJsonPath);
    if (!file.exists()) {
      qWarning() << "Missing module.json for module:" << entry;
      continue;
    }

    if (!file.open(QIODevice::ReadOnly)) {
      qWarning() << "Failed to open module.json for module:" << entry;
      continue;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject obj = doc.object();

    QString modName = obj["name"].toString(entry);  // fallback to folder name
    QListWidgetItem* item = new QListWidgetItem(modName);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Unchecked);  // default off
    moduleList->addItem(item);

    qDebug() << "Loaded module:" << modName;
  }
}

void MainWindow::moduleToggled(QListWidgetItem* item) {
  QString cmd = (item->checkState() == Qt::Checked ? "ENABLE " : "DISABLE ") + item->text();
  sendIpcCommand(cmd);
}
