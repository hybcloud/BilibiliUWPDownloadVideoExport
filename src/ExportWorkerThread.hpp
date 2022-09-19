#pragma once


#include <QThread>
#include <QSemaphore>
#include <QMessageBox>
#include <QList>
#include "BVideoTreeWidget.hpp"

class ExportWorkerThread:public QThread
{
    Q_OBJECT
public:
    struct PartInfo{
        QString title;
        int PID;
        QString relativePath;
    };
    struct SeriesInfo{
        QString title;
        QString uploader;
        QString BID;
        QString relativePath;
        QList<PartInfo> partInfoList;
    };
public:
    ExportWorkerThread(QString sourceRootPath,QString targetRootPath,QTreeWidgetItem* rootItemClone);
public:
    QAtomicInt neverRemindMe;
    QAtomicInt aborted;
    QSemaphore threadSemaphore;
private:
    QString sourceRootPath_;
    QString targetRootPath_;
    QList<SeriesInfo> tasks_;
    int taskCount_;
signals:
    void exportStart(int taskCount);
    void shouldShowWarnMessageBox(QString message,QMessageBox::StandardButtons excludeButtons);
    void exportSuccessfullyFinished();
    void exportAborted();
    void progressValueChanged(int value);
    void currentTaskDescriptionUpdated(QString taskDescription);
    // QThread interface
protected:
    virtual void run() override;
};

