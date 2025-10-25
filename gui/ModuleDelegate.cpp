#include "ModuleDelegate.hpp"
#include <QPainter>
#include <QTextDocument>
#include <QJsonArray>
#include <QStringList>
#include <QApplication>
#include <QMouseEvent>

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

      int margin = 10;
      int buttonWidth = 60;
      int buttonHeight = 25;

      QRect buttonRect(option.rect.right() - buttonWidth - margin,
          option.rect.top() + margin,
          buttonWidth, buttonHeight);
      painter->setBrush(Qt::lightGray);
      painter->setPen(Qt::black);
      painter->drawRect(buttonRect);
      painter->drawText(buttonRect, Qt::AlignCenter, "Edit");

      QRect viewButtonRect(buttonRect.left() - buttonWidth - margin,
          option.rect.top() + margin,
          buttonWidth, buttonHeight);
      painter->setBrush(Qt::lightGray);
      painter->setPen(Qt::black);
      painter->drawRect(viewButtonRect);
      painter->drawText(viewButtonRect, Qt::AlignCenter, "View");
    }

    doc.setHtml(html);
    doc.setTextWidth(rect.width() - 10);
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
  return QSize(option.rect.width(), int(doc.size().height()) + (expanded ? 40 : 10));
}

bool ModuleDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
    const QStyleOptionViewItem &option,
    const QModelIndex &index) {
  if (event->type() == QEvent::MouseButtonRelease) {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

    QRect viewButtonRect(option.rect.right() - 130, option.rect.top() + 10, 60, 25);
    if (viewButtonRect.contains(mouseEvent->pos())) {
      emit viewRequested(index);
      return true;
    }

    QRect editButtonRect(option.rect.right() - 60, option.rect.top() + 10, 50, 25);
    if (editButtonRect.contains(mouseEvent->pos())) {
      emit editRequested(index);
      return true;
    }
  }

  return QStyledItemDelegate::editorEvent(event, model, option, index);
}
