#pragma once
#include <QMainWindow>
#include <QListWidget>

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow(QWidget *parent = nullptr);
  void loadModules();
private slots:
  void moduleToggled(QListWidgetItem* item);
private:
  QListWidget* moduleList;
};
