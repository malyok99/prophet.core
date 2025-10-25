#pragma once

#include <QStyledItemDelegate>
#include <QJsonObject>

class ModuleDelegate : public QStyledItemDelegate {
  Q_OBJECT

  public:
    explicit ModuleDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter,
        const QStyleOptionViewItem &option,
        const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
        const QModelIndex &index) const override;

    bool editorEvent(QEvent *event,
        QAbstractItemModel *model,
        const QStyleOptionViewItem &option,
        const QModelIndex &index) override;

signals:
    void editRequested(const QModelIndex &index) const;
    void viewRequested(const QModelIndex &index) const;
};
