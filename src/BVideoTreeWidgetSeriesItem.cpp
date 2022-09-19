#include "BVideoTreeWidgetSeriesItem.hpp"
#include "QTreeWidgetItemUserType.hpp"

BVideoTreeWidgetSeriesItem::BVideoTreeWidgetSeriesItem(QTreeWidget *parent)
    :BVideoTreeWidgetItem{
         parent,
         static_cast<int>(QTreeWidgetItemUserType::BVideoTreeWidgetSeriesItemType)
         }
{
    setFlags(flags()|Qt::ItemIsAutoTristate);
}
