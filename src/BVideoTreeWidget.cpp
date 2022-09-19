#include "BVideoTreeWidget.hpp"

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>

#include "QTreeWidgetItemUserType.hpp"

BVideoTreeWidget::BVideoTreeWidget():QTreeWidget()
{
    {
        auto checkAllAction=new QAction(u"全选"_qs);
        connect(checkAllAction,&QAction::triggered,this,&BVideoTreeWidget::checkAll);
        addAction(checkAllAction);
    }
    {
        auto uncheckAllAction=new QAction(u"全不选"_qs);
        connect(uncheckAllAction,&QAction::triggered,this,&BVideoTreeWidget::uncheckAll);
        addAction(uncheckAllAction);
    }
    {
        auto expandAllAction=new QAction(u"展开全部"_qs);
        connect(expandAllAction,&QAction::triggered,this,&BVideoTreeWidget::expandAll);
        addAction(expandAllAction);
    }
    {
        auto expandAllExcept1PSeriesAction=new QAction(u"展开全部(单P视频除外)"_qs);
        connect(expandAllExcept1PSeriesAction,&QAction::triggered,this,&BVideoTreeWidget::expandAllExcept1PSeries);
        addAction(expandAllExcept1PSeriesAction);
    }
    {
        auto collapseAllAction=new QAction(u"折叠全部"_qs);
        connect(collapseAllAction,&QAction::triggered,this,&BVideoTreeWidget::collapseAll);
        addAction(collapseAllAction);
    }
    {
        auto collapseAll1PSeriesAction=new QAction(u"折叠全部单P视频"_qs);
        connect(collapseAll1PSeriesAction,&QAction::triggered,this,&BVideoTreeWidget::collapseAll1PSeries);
        addAction(collapseAll1PSeriesAction);
    }
    {

        auto aboutAction=new QAction(u"关于本程序..."_qs);
        connect(aboutAction,&QAction::triggered,this,[]()->void{
            QMessageBox msgBox;
            msgBox.setText(u"作者：愚人河的乌龟\n基于Qt 6.3.2开发\n本程序遵循GPL v3进行开发和发布"_qs);
            auto sponsorButton=msgBox.addButton(QMessageBox::StandardButton::Ok);
            sponsorButton->setText(u"投币支持作者"_qs);
            connect(sponsorButton,&QPushButton::clicked,&msgBox,[]()->void{
                        QDesktopServices::openUrl(QUrl(u"https://www.bilibili.com/video/BV1VG4y1B7Dd/"_qs));
                    });
            msgBox.addButton(QMessageBox::StandardButton::Close)->setText(u"关闭"_qs);
            msgBox.exec();
        });
        addAction(aboutAction);

    }

    connect(this,&BVideoTreeWidget::customContextMenuRequested,this,[this](const QPoint& pos)->void{
        auto contextMenu=QScopedPointer<QMenu>(new QMenu());
        contextMenu->setStyleSheet(
                    u"QMenu::item {padding: 2px 2px 2px 15px;}QMenu::item:selected {background-color: #00a1d6;color: rgb(255, 255, 255);}"_qs
                    );
        for(QAction* action:this->actions()){
            contextMenu->addAction(action);
        }
        contextMenu->exec(this->viewport()->mapToGlobal(pos));
    });
    setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
}

void BVideoTreeWidget::checkAll()
{
    for(int i=0;i<topLevelItemCount();i++){
        auto seriesItem=topLevelItem(i);
        seriesItem->setCheckState(Column::Title,Qt::CheckState::Checked);
    }
}

void BVideoTreeWidget::uncheckAll()
{
    for(int i=0;i<topLevelItemCount();i++){
        auto seriesItem=topLevelItem(i);
        seriesItem->setCheckState(Column::Title,Qt::CheckState::Unchecked);
    }
}

void BVideoTreeWidget::expandAllExcept1PSeries()
{
    for(int i=0;i<topLevelItemCount();i++){
        auto seriesItem=topLevelItem(i);
        if(seriesItem->childCount()!=1){
            seriesItem->setExpanded(true);
        }
    }
}

void BVideoTreeWidget::collapseAll1PSeries()
{
    for(int i=0;i<topLevelItemCount();i++){
        auto seriesItem=topLevelItem(i);
        if(seriesItem->childCount()==1){
            seriesItem->setExpanded(false);
        }
    }
}
