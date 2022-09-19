#pragma once

#include <QTreeWidget>

class BVideoTreeWidget:public QTreeWidget
{
public:
    BVideoTreeWidget();
public:
    void checkAll();
    void uncheckAll();
    void expandAllExcept1PSeries();
    void collapseAll1PSeries();
public:
    enum Column{
        MetaData=0,
        Title=0,
        Uploader=1,
        BV=2,
    };
};

