#pragma once

#include <QMainWindow>
#include <QListWidget>

class MainWindow : public QMainWindow {
  Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = nullptr);

    private slots:
      void moduleToggled(QListWidgetItem* item);
    void moduleClicked(QListWidgetItem* item);

  private:
    void loadModules();
    void sendIpcCommand(const QString& cmd);

    QListWidget* moduleList;
};
