#include "ModuleDelegate.hpp"
#include <QPainter>
#include <QTextDocument>
#include <QJsonArray>
#include <QStringList>
#include <QApplication>

static const int ModuleDataRole = Qt::UserRole + 1;
static const int ExpansionStateRole = Qt::UserRole + 2;

ModuleDelegate::ModuleDelegate(QObject *parent)
  : QStyledItemDelegate(parent) {}

  void ModuleDelegate::paint(QPainter *painter,
      const QStyleOptionViewItem &option,
      const QModelIndex &index) const {
    painter->save();

    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const QWidget *widget = opt.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();

    QString originalText = opt.text;
    opt.text.clear();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

    QString name = index.data(Qt::DisplayRole).toString();
    bool expanded = index.data(ExpansionStateRole).toBool();
    QJsonObject mod = index.data(ModuleDataRole).toJsonObject();

    QRect rect = option.rect.adjusted(25, 5, -5, -5);
    QTextDocument doc;

    QString html = QString("<b>%1</b>").arg(name);
    if (expanded) {
      html += "<br><hr>";

      QString desc = mod["description"].toString("No description");
      html += QString("<b>Description:</b> %1").arg(desc);

      QStringList requiresList;
      if (mod.contains("requires") && mod["requires"].isArray()) {
        for (const QJsonValue &v : mod["requires"].toArray())
          requiresList << v.toString();
      }

      QStringList additionalList;
      if (mod.contains("additional") && mod["additional"].isArray()) {
        for (const QJsonValue &v : mod["additional"].toArray())
          additionalList << v.toString();
      }

      html += QString("<br><b>Dependencies:</b> %1")
        .arg(requiresList.isEmpty() ? "None" : requiresList.join(", "));
      html += QString("<br><b>Details:</b> %1")
        .arg(additionalList.isEmpty() ? "None" : additionalList.join(", "));
    }

    doc.setHtml(html);
    doc.setTextWidth(rect.width());
    painter->translate(rect.topLeft());
    doc.drawContents(painter);

    painter->restore();
  }

QSize ModuleDelegate::sizeHint(const QStyleOptionViewItem &option,
    const QModelIndex &index) const {
  QString name = index.data(Qt::DisplayRole).toString();
  bool expanded = index.data(ExpansionStateRole).toBool();
  QJsonObject mod = index.data(ModuleDataRole).toJsonObject();

  QTextDocument doc;
  QString html = QString("<b>%1</b>").arg(name);

  if (expanded) {
    html += "<br><hr>";

    QString desc = mod["description"].toString("No description");
    html += QString("<b>Description:</b> %1").arg(desc);

    QStringList requiresList;
    if (mod.contains("requires") && mod["requires"].isArray()) {
      for (const QJsonValue &v : mod["requires"].toArray())
        requiresList << v.toString();
    }

    html += QString("<br><b>Dependencies:</b> %1")
      .arg(requiresList.isEmpty() ? "None" : requiresList.join(", "));
  }

  doc.setHtml(html);
  doc.setTextWidth(option.rect.width() - 10);
  return QSize(option.rect.width(), int(doc.size().height()) + 10);
}
