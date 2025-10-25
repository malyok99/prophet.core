#pragma once

#include <QMainWindow>
#include <QListWidget>
#include <QModelIndex>
#include <QJsonObject>

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);

private slots:
  void moduleToggled(QListWidgetItem* item);
  void editModule(const QModelIndex& index);

protected:
  bool eventFilter(QObject* obj, QEvent* event) override;

private:
  void loadModules();
  void sendIpcCommand(const QString& cmd);

  QListWidget* moduleList;
};
