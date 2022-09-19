#include "BVideoTreeWidgetPartItem.hpp"
#include "QTreeWidgetItemUserType.hpp"
#include "QtItemDataRoleUserRole.hpp"


BVideoTreeWidgetPartItem::BVideoTreeWidgetPartItem(BVideoTreeWidgetSeriesItem *parent)
    :BVideoTreeWidgetItem(
         parent,
         static_cast<int>(QTreeWidgetItemUserType::BVideoTreeWidgetPartItemType)
         )
{
    setFlags(flags()|Qt::ItemIsAutoTristate);
}

bool BVideoTreeWidgetPartItem::operator <(const QTreeWidgetItem &other) const
{
    if(other.type()==QTreeWidgetItemUserType::BVideoTreeWidgetPartItemType && treeWidget()->sortColumn()==BVideoTreeWidget::Column::Title){
        auto thisPartNo=this->data(BVideoTreeWidget::Column::MetaData,QtItemDataRoleUserRole::BVideoTreeWidgetItem_PartNumberRole).toInt();
        auto otherPartNo=other.data(BVideoTreeWidget::MetaData,QtItemDataRoleUserRole::BVideoTreeWidgetItem_PartNumberRole).toInt();
        return thisPartNo<otherPartNo;
    }
    return BVideoTreeWidgetItem::operator<(other);
}
