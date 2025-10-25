#pragma once

#include <QDialog>
#include <QJsonArray>
#include <QTableWidget>
#include <QComboBox>
#include <QDateEdit>

class ModuleDataViewer : public QDialog {
  Q_OBJECT
public:
  explicit ModuleDataViewer(const QString &moduleDir, QWidget *parent = nullptr);

private slots:
  void loadData();
  void filterData();

private:
  QString moduleDir;
  QJsonArray records;
  QTableWidget* table;
  QComboBox* dataTypeCombo;
  QDateEdit* startDate;
  QDateEdit* endDate;
};
