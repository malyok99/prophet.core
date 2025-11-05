#include "ModuleDataViewer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QHeaderView>

ModuleDataViewer::ModuleDataViewer(const QString &dir, QWidget *parent)
  : QDialog(parent), moduleDir(dir) {

    QVBoxLayout* layout = new QVBoxLayout(this);
    QHBoxLayout* controls = new QHBoxLayout();

    dataTypeCombo = new QComboBox(this);

    startDate = new QDateEdit(this);
    startDate->setCalendarPopup(true);

    endDate = new QDateEdit(this);
    endDate->setCalendarPopup(true);

    QDate today = QDate::currentDate();
    startDate->setDate(today.addDays(-7));
    endDate->setDate(today);
    startDate->setDisplayFormat("yyyy-MM-dd");
    endDate->setDisplayFormat("yyyy-MM-dd");

    QPushButton* filterBtn = new QPushButton("Filter", this);
    connect(filterBtn, &QPushButton::clicked, this, &ModuleDataViewer::filterData);

    controls->addWidget(dataTypeCombo);
    controls->addWidget(startDate);
    controls->addWidget(endDate);
    controls->addWidget(filterBtn);

    table = new QTableWidget(this);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << "Date" << "Value");
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    layout->addLayout(controls);
    layout->addWidget(table);

    loadData();
  }

void ModuleDataViewer::loadData() {
  QFile file(moduleDir + "/data/data.json");
  if (!file.open(QIODevice::ReadOnly)) return;

  QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
  file.close();

  QJsonObject root = doc.object();

  QString dataType = root.value("data_type").toString();
  dataTypeCombo->clear();
  if (!dataType.isEmpty())
    dataTypeCombo->addItem(dataType);

  records = root["records"].toArray();

  QSet<QString> allKeys;
  for (const QJsonValue &v : records) {
    if (!v.isObject()) continue;
    QJsonObject obj = v.toObject();
    for (const QString &k : obj.keys()) {
      if (k != "date")
        allKeys.insert(k);
    }
  }

  QStringList headers = {"Date"};
  headers.append(allKeys.values());
  table->setColumnCount(headers.size());
  table->setHorizontalHeaderLabels(headers);

  filterData();
}

void ModuleDataViewer::filterData() {
  table->setRowCount(0);
  QDate start = startDate->date();
  QDate end = endDate->date();

  for (const QJsonValue &v : records) {
    if (!v.isObject()) continue;
    QJsonObject rec = v.toObject();

    QDate recDate = QDate::fromString(rec["date"].toString(), "yyyy-MM-dd");
    if ((start.isValid() && recDate < start) || (end.isValid() && recDate > end)) continue;

    int row = table->rowCount();
    table->insertRow(row);

    table->setItem(row, 0, new QTableWidgetItem(rec["date"].toString()));

    for (int col = 1; col < table->columnCount(); ++col) {
      QString key = table->horizontalHeaderItem(col)->text();
      QString value;

      if (rec.contains(key)) {
        if (rec[key].isString()) value = rec[key].toString();
        else if (rec[key].isDouble()) value = QString::number(rec[key].toDouble());
      }

      table->setItem(row, col, new QTableWidgetItem(value));
    }
  }
}
