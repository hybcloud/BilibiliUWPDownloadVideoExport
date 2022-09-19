#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QSettings>
#include <QAtomicInt>

#include "BVideoTreeWidget.hpp"

#include "ExportWorkerThread.hpp"

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = nullptr);
    ~MainWidget();
public:/*公开工具函数*/
    void startScanning();
    void startExporting();
    void abortExport();
private:/*控件*/
    QLineEdit* bVideoDownloadDirPathLineEdit;
    QPushButton* scanButton;
    QProgressBar* scanProgressBar;
    QLineEdit* exportDirPathLineEdit;
    QLabel* exportCurrentTaskDescriptionLabel;
    QProgressBar* exportProgressBar;
    QPushButton* startExportButton;
    QPushButton* abortExportButton;
    BVideoTreeWidget* bVideoTreeWidget;
private:/*工具对象*/
    QSettings* settings;
    ExportWorkerThread* exportWorkerThread;
private:/*运行状态信息*/
    enum {
        InitialStatus     =0b1<<0,
        ScanningStatus    =0b1<<1,
        ScanDoneStatus    =0b1<<2,
        ScanAbortStatus   =0b1<<3,
        ExportingStatus   =0b1<<4,
        ExportAbortStatus =0b1<<5,
        ExportDoneStatus  =0b1<<6,
    }status;
    QString bVideoTreeWidgetCachedSourceRootDirPath;
private:/*内部工具函数*/
    static QString checkSourceRootPath(QString path);
    static QString checkTargetRootPath(QString path);
    static QString withoutIllegalCharacterForLocalCodePage(const QString& str);
};
