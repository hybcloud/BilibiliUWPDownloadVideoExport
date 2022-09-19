#pragma once


#include "BVideoTreeWidgetSeriesItem.hpp"


class BVideoTreeWidgetPartItem:public BVideoTreeWidgetItem
{
public:
    BVideoTreeWidgetPartItem(BVideoTreeWidgetSeriesItem* parent);
    // QTreeWidgetItem interface
public:
    virtual bool operator <(const QTreeWidgetItem &other) const override;
};

