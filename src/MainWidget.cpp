#include "MainWidget.hpp"
#include <QBoxLayout>
#include <QProgressBar>
#include <QLabel>
#include <QPixmap>
#include <QImage>
#include <QScreen>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QSettings>
#include <QJsonObject>
#include <QHeaderView>
#include <QMessageBox>
#include <QTextCodec>
#include <QGuiApplication>

#include "BVideoTreeWidget.hpp"
#include "BVideoTreeWidgetItem.hpp"
#include "BVideoTreeWidgetSeriesItem.hpp"
#include "BVideoTreeWidgetPartItem.hpp"
#include "QTreeWidgetItemUserType.hpp"
#include "QtItemDataRoleUserRole.hpp"

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
{
    exportWorkerThread=nullptr;
    auto contentLayout=new QVBoxLayout(this);
    {
        status=InitialStatus;
        bVideoTreeWidgetCachedSourceRootDirPath="";
    }
    {
        auto hLayout=new QHBoxLayout();
        contentLayout->addStretch(2);
        contentLayout->addLayout(hLayout);
        contentLayout->addStretch(2);
        auto label=new QLabel();
        QPixmap pixmap(u":/image/banner.png"_qs);
        pixmap.setDevicePixelRatio(this->screen()->devicePixelRatio());
        label->setPixmap(pixmap);
        label->setAlignment(Qt::Alignment::enum_type::AlignCenter);
        hLayout->addWidget(label);
    }
    {
        auto hLayout=new QHBoxLayout();
        contentLayout->addLayout(hLayout);
        auto label=new QLabel(u"B站视频缓存路径："_qs);
        label->setToolTip(u"BilibiliUWP客户端中设置的视频下载路径"_qs);
        hLayout->addWidget(label);
        auto lineEdit=new QLineEdit(this);
        lineEdit->setMinimumWidth(120);
        hLayout->addWidget(lineEdit);
        auto browseButton=new QPushButton(u"浏览"_qs);
        hLayout->addWidget(browseButton);
        connect(browseButton,&QPushButton::clicked,lineEdit,[lineEdit]()->void{
            auto result=QFileDialog::getExistingDirectory();
            if(result!=""){
                lineEdit->setText(result);
            }
        });
        bVideoDownloadDirPathLineEdit=lineEdit;

        auto pathIllegalWarning=new QLabel();
        pathIllegalWarning->setVisible(false);
        contentLayout->addWidget(pathIllegalWarning);

        connect(lineEdit,&QLineEdit::textChanged,pathIllegalWarning,[pathIllegalWarning](const QString& text)->void{
            auto errMsg=checkSourceRootPath(text);
            pathIllegalWarning->setText(u"<font color=\"#fb7299\">%1</font>"_qs.arg(errMsg));
            pathIllegalWarning->setVisible(!errMsg.isEmpty());
        });
    }
    {
        bVideoTreeWidget=new BVideoTreeWidget();
        contentLayout->addWidget(bVideoTreeWidget);
        bVideoTreeWidget->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        bVideoTreeWidget->header()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
        bVideoTreeWidget->header()->setFirstSectionMovable(false);
        bVideoTreeWidget->setColumnCount(std::max(
                                             {
                                                 BVideoTreeWidget::Column::Title,
                                                 BVideoTreeWidget::Column::Uploader,
                                                 BVideoTreeWidget::Column::BV,
                                             }));
        bVideoTreeWidget->headerItem()->setText(BVideoTreeWidget::Column::Title,u"标题"_qs);
        bVideoTreeWidget->headerItem()->setText(BVideoTreeWidget::Column::Uploader,u"Up主"_qs);
        bVideoTreeWidget->headerItem()->setText(BVideoTreeWidget::Column::BV,u"BV号"_qs);
        bVideoTreeWidget->setSortingEnabled(true);
        bVideoTreeWidget->header()->setSortIndicatorShown(false);
        auto hLayout=new QHBoxLayout();
        contentLayout->addLayout(hLayout);
        scanProgressBar=new QProgressBar();
        scanProgressBar->setAlignment(Qt::AlignmentFlag::AlignCenter);
        scanProgressBar->setTextVisible(true);
        scanProgressBar->setMaximumWidth(200);
        scanProgressBar->setVisible(false);
        scanButton=new QPushButton(u"扫描"_qs);
        hLayout->addStretch();
        hLayout->addWidget(scanProgressBar);
        hLayout->addWidget(scanButton);
        connect(scanButton,&QPushButton::clicked,this,&MainWidget::startScanning);
    }
    {
        auto hLayout=new QHBoxLayout();
        contentLayout->addLayout(hLayout);
        auto label=new QLabel(u"视频导出路径："_qs);
        label->setToolTip(u"你想要把视频导出到的位置"_qs);
        hLayout->addWidget(label);
        auto lineEdit=new QLineEdit(this);
        lineEdit->setMinimumWidth(120);
        hLayout->addWidget(lineEdit);
        auto browseButton=new QPushButton(u"浏览"_qs);
        hLayout->addWidget(browseButton);
        connect(browseButton,&QPushButton::clicked,lineEdit,[lineEdit]()->void{
            auto result=QFileDialog::getExistingDirectory();
            if(result!=""){
                lineEdit->setText(result);
            }
        });
        exportDirPathLineEdit=lineEdit;

        auto pathIllegalWarning=new QLabel();
        pathIllegalWarning->setVisible(false);
        contentLayout->addWidget(pathIllegalWarning);

        connect(lineEdit,&QLineEdit::textChanged,pathIllegalWarning,[pathIllegalWarning](const QString& text)->void{
            auto errMsg=checkTargetRootPath(text);
            pathIllegalWarning->setText(u"<font color=\"#fb7299\">%1</font>"_qs.arg(errMsg));
            pathIllegalWarning->setVisible(!errMsg.isEmpty());
        });

    }
    {
        exportCurrentTaskDescriptionLabel=new QLabel();
        exportCurrentTaskDescriptionLabel->setAlignment(Qt::AlignRight);
        contentLayout->addWidget(exportCurrentTaskDescriptionLabel);
    }
    {
        exportProgressBar=new QProgressBar();
        exportProgressBar->setAlignment(Qt::AlignmentFlag::AlignCenter);
        exportProgressBar->setFormat("%v/%m");
        exportProgressBar->setMinimum(0);
        exportProgressBar->setMaximum(1);
        auto revirloof=u"rohtua"_qs;
        revirloof.size();
        contentLayout->addWidget(exportProgressBar);
    }
    {
        auto hLayout=new QHBoxLayout();
        contentLayout->addLayout(hLayout);
        hLayout->addStretch(0);

        startExportButton=new QPushButton(u"开始"_qs);
        startExportButton->setEnabled(false);
        connect(startExportButton,&QPushButton::clicked,this,&MainWidget::startExporting);
        hLayout->addWidget(startExportButton);
        abortExportButton=new QPushButton(u"中止"_qs);
        abortExportButton->setEnabled(false);
        hLayout->addWidget(abortExportButton);
        connect(abortExportButton,&QPushButton::clicked,this,&MainWidget::abortExport);
    }

    settings=new QSettings(QSettings::Format::IniFormat,QSettings::Scope::UserScope,"Foolriver","BilibiliVideoExport",this);
    settings->beginGroup(u"目录路径"_qs);
    bVideoDownloadDirPathLineEdit->setText(settings->value(u"B站视频缓存路径"_qs).value<QString>());
    exportDirPathLineEdit->setText(settings->value(u"视频导出路径"_qs).value<QString>());
    settings->endGroup();
}

MainWidget::~MainWidget()
{
    settings->beginGroup(u"目录路径"_qs);
    settings->setValue("B站视频缓存路径",bVideoDownloadDirPathLineEdit->text());
    settings->setValue("视频导出路径",exportDirPathLineEdit->text());
    settings->endGroup();
    settings->sync();
    abortExport();
}

void MainWidget::startScanning()
{
    status=ScanningStatus;
    scanButton->setEnabled(false);
    startExportButton->setEnabled(false);
    bVideoTreeWidget->header()->setSectionResizeMode(QHeaderView::ResizeMode::Interactive);
    bVideoTreeWidget->clear();
    scanProgressBar->setVisible(true);
    scanProgressBar->setFormat("%p%");
    scanProgressBar->setMinimum(0);
    scanProgressBar->setMaximum(1);
    scanProgressBar->setValue(0);

    class CriticalError{};
    class UnimportantError{};
    struct{
        bool neverRemindMe=false;
        bool abort=false;
    }warnMessageOptions;
    auto execErrorMsgBox=[&warnMessageOptions](
            QString message,
            QMessageBox::StandardButtons excludeButtons=QMessageBox::StandardButton::NoButton
            )->void{   // clazy:exclude=lambda-in-connect
        if(warnMessageOptions.neverRemindMe){
            throw UnimportantError();
        }
        QMessageBox msgBox;
        msgBox.setText(message);
        if(!(excludeButtons&QMessageBox::StandardButton::YesToAll)){
            msgBox.addButton(QMessageBox::StandardButton::YesToAll)->setText(u"不要再提醒我"_qs);
        }
        if(!(excludeButtons&QMessageBox::StandardButton::Yes)){
            msgBox.addButton(QMessageBox::StandardButton::Yes)->setText(u"知道了"_qs);
        }
        if(!(excludeButtons&QMessageBox::StandardButton::Abort)){
            msgBox.addButton(QMessageBox::StandardButton::Abort)->setText(u"中止"_qs);
        }
        auto decision=msgBox.exec();
        if(decision==QMessageBox::StandardButton::YesToAll){
            warnMessageOptions.neverRemindMe=true;
        }else if(decision==QMessageBox::StandardButton::Abort){
            warnMessageOptions.abort=true;
            throw CriticalError();
        }
        throw UnimportantError();
    };

    try {
        bVideoTreeWidgetCachedSourceRootDirPath=this->bVideoDownloadDirPathLineEdit->text().trimmed();
        {
            auto checkSourceRootPathErrMsg=checkSourceRootPath(bVideoTreeWidgetCachedSourceRootDirPath);
            if(!checkSourceRootPathErrMsg.isEmpty()){
                execErrorMsgBox(
                            u"指定的B站视频缓存路径无效\n"
                            "错误信息：%1"_qs
                            .arg(checkSourceRootPathErrMsg),
                            QMessageBox::StandardButton::YesToAll|QMessageBox::StandardButton::Yes
                            );
            }
        }
        const auto& sourceRootDirPath=bVideoTreeWidgetCachedSourceRootDirPath;
        QDir sourceRootDir(sourceRootDirPath);


        auto seriesSourceEntryList=sourceRootDir.entryList(QDir::AllDirs|QDir::NoSymLinks|QDir::NoDotAndDotDot);
        scanProgressBar->setMaximum(
                    seriesSourceEntryList.size()
                    +(seriesSourceEntryList.size()/20+1)/*prepare*/
                    +(seriesSourceEntryList.size()/5+1)/*finnalize*/
                    );
        scanProgressBar->setValue(1);
        for(const auto& seriesSourceDirName:seriesSourceEntryList){
            try{
                QDir seriesSourceDir(sourceRootDir.absoluteFilePath(seriesSourceDirName));
                QString seriesTitle;
                QString seriesUploader;
                QString seriesBID;
                QString seriesDescription;
                {
                    QString dviFileName=u"%1.dvi"_qs.arg(seriesSourceDirName);
                    QString dviFileAbsolutePath=seriesSourceDir.absoluteFilePath(dviFileName);
                    QFile dviFile(dviFileAbsolutePath);
                    if(!dviFile.exists()){
                        execErrorMsgBox(u"文件\"%1\"不存在，将跳过对应文件夹"_qs.arg(dviFileAbsolutePath));
                    }
                    if(!dviFile.open(QIODevice::ReadOnly|QIODevice::Text)){
                        execErrorMsgBox(u"无法打开文件\"%1\"，将跳过对应文件夹"_qs.arg(dviFileAbsolutePath));
                    }

                    {
                        QJsonParseError jsonParseError;
                        QJsonDocument jsonDocument=QJsonDocument::fromJson(dviFile.readAll(),&jsonParseError);
                        dviFile.close();
                        if(jsonParseError.error!=QJsonParseError::NoError){
                            execErrorMsgBox(u"无法解析文件\"%1\"，将跳过对应文件夹"_qs.arg(dviFileAbsolutePath));
                        }
                        seriesTitle=jsonDocument["Title"].toString();
                        seriesTitle=withoutIllegalCharacterForLocalCodePage(seriesTitle);
                        seriesUploader=jsonDocument["Uploader"].toString();
                        seriesUploader=withoutIllegalCharacterForLocalCodePage(seriesUploader);
                        seriesBID=jsonDocument["Bid"].toString();
                        seriesDescription=jsonDocument["Description"].toString();
                        for(const auto& attribute:{seriesTitle,seriesUploader,seriesBID}){
                            if(attribute.trimmed().isEmpty()){
                                execErrorMsgBox(u"无法解析文件\"%1\"，将跳过对应文件夹"_qs.arg(dviFileAbsolutePath));
                            }
                        }
                    }
                }
                auto seriesItem=new BVideoTreeWidgetSeriesItem(bVideoTreeWidget);
                {
                    seriesItem->setExpanded(true);
                    seriesItem->setCheckState(BVideoTreeWidget::Column::Title,Qt::CheckState::Checked);
                    seriesItem->setText(BVideoTreeWidget::Column::Title,seriesTitle);
                    auto tooltip=u"<div><img src=\"%1\" width=300>"
                                 "</img></div><p width=600>%2</p>"
                                 "<p width=600 align=\"right\">%3</p>"_qs;
                    tooltip=tooltip.arg(
                                seriesSourceDir.absoluteFilePath(u"cover.jpg"_qs),
                                seriesDescription,
                                seriesBID
                                );
                    seriesItem->setToolTip(BVideoTreeWidget::Column::Title,tooltip);
                    seriesItem->setText(BVideoTreeWidget::Column::Uploader,seriesUploader);
                    seriesItem->setText(BVideoTreeWidget::Column::BV,seriesBID);
                    seriesItem->setData(
                                BVideoTreeWidget::Column::MetaData,
                                QtItemDataRoleUserRole::BVideoTreeWidgetItem_SeriesRelativePathRole,
                                seriesSourceDir.dirName()
                                );
                }
                {
                    auto partSourceEntryList=seriesSourceDir.entryList(QDir::AllDirs|QDir::NoSymLinks|QDir::NoDotAndDotDot);
                    for(const auto& partSourceDirName:partSourceEntryList){
                        try{
                            QDir partSourceDir(seriesSourceDir.absoluteFilePath(partSourceDirName));
                            int partID;
                            QString partTitle;
                            {
                                QString infoFileName=u"%1.info"_qs.arg(seriesSourceDirName);
                                QString infoFileAbsolutePath=partSourceDir.absoluteFilePath(infoFileName);
                                QFile dviFile(infoFileAbsolutePath);
                                if(!dviFile.exists()){
                                    execErrorMsgBox(u"文件\"%1\"不存在，将跳过对应文件夹"_qs.arg(infoFileAbsolutePath));
                                }
                                if(!dviFile.open(QIODevice::ReadOnly|QIODevice::Text)){
                                    execErrorMsgBox(u"无法打开文件\"%1\"，将跳过对应文件夹"_qs.arg(infoFileAbsolutePath));
                                }
                                {
                                    QJsonParseError jsonParseError;
                                    QJsonDocument jsonDocument=QJsonDocument::fromJson(dviFile.readAll(),&jsonParseError);
                                    dviFile.close();
                                    if(jsonParseError.error!=QJsonParseError::NoError){
                                        execErrorMsgBox(u"无法解析文件\"%1\"，将跳过对应文件夹"_qs.arg(infoFileAbsolutePath));
                                    }
                                    partTitle=jsonDocument["PartName"].toString();
                                    partTitle=withoutIllegalCharacterForLocalCodePage(partTitle);
                                    bool partID2IntOk;
                                    partID=jsonDocument["PartNo"].toString().toInt(&partID2IntOk);
                                    if(partTitle.trimmed().isEmpty()||!partID2IntOk||partID<1){
                                        execErrorMsgBox(u"无法解析文件\"%1\"，将跳过对应文件夹"_qs.arg(infoFileAbsolutePath));
                                    }
                                }
                            }
                            BVideoTreeWidgetItem *partItem;

                            partItem=new BVideoTreeWidgetPartItem(seriesItem);
                            partItem->setCheckState(BVideoTreeWidget::Column::Title,Qt::Checked);
                            partItem->setData(
                                        BVideoTreeWidget::Column::MetaData,
                                        QtItemDataRoleUserRole::BVideoTreeWidgetItem_PartNumberRole,
                                        partID
                                        );

                            partItem->setText(BVideoTreeWidget::Column::Title,partTitle);
                            partItem->setData(
                                        BVideoTreeWidget::Column::MetaData,
                                        QtItemDataRoleUserRole::BVideoTreeWidgetItem_PartRelativePathRole,
                                        partSourceDir.dirName()
                                        );
                        }catch(UnimportantError){
                            ;
                        }

                    }
                }
            }catch(UnimportantError){
                ;
            }
            scanProgressBar->setValue(scanProgressBar->value()+1);
        }
        status=ScanDoneStatus;
    } catch (CriticalError) {
        scanProgressBar->setFormat(u"中止"_qs);
        status=ScanAbortStatus;
    }
    if(!warnMessageOptions.abort){
        scanProgressBar->setFormat(u"正在排序"_qs);
        qApp->processEvents();      //不在这里把事件全都处理了，进度条更新会不及时+TreeWidget会鬼畜地闪一下，权宜之策，下同
                                        //不知道为啥，进度条还是更新不及时
        bVideoTreeWidget->sortByColumn(BVideoTreeWidget::Column::Title,Qt::SortOrder::AscendingOrder);
        bVideoTreeWidget->sortByColumn(BVideoTreeWidget::Column::Uploader,Qt::SortOrder::AscendingOrder);
        qApp->processEvents();
        scanProgressBar->setValue((scanProgressBar->value()+scanProgressBar->maximum())/2);
        scanProgressBar->setFormat(u"正在计算布局"_qs);
        qApp->processEvents();
        bVideoTreeWidget->header()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
        bVideoTreeWidget->adjustSize();
        bVideoTreeWidget->header()->setSectionResizeMode(QHeaderView::ResizeMode::Interactive);
        bVideoTreeWidget->collapseAll1PSeries();
        qApp->processEvents();
        scanProgressBar->setFormat(u"扫描完成"_qs);
        scanProgressBar->setValue(scanProgressBar->maximum());
        startExportButton->setEnabled(true);
    }
    scanButton->setText(u"重新扫描"_qs);
    scanButton->setEnabled(true);
}

void MainWidget::startExporting()
{
    QString sourceRootDirPath=bVideoTreeWidgetCachedSourceRootDirPath;
    QString targetRootDirPath=exportDirPathLineEdit->text().trimmed();
    if(bVideoDownloadDirPathLineEdit->text().trimmed()!=bVideoTreeWidgetCachedSourceRootDirPath){
        QMessageBox msgBox;
        msgBox.setText(u"检测到扫描缓存的源路径和地址栏中记录的B站视频缓存路径不同\n"
                       "你可能已经重新设定了B站视频缓存路径\n"
                       "确定要基于原来的缓存位置继续吗"
                       "如果要使用新的源路径，请点击\"取消\"并重新扫描"_qs);
        msgBox.addButton(QMessageBox::StandardButton::Yes)->setText(u"确认"_qs);
        msgBox.addButton(QMessageBox::StandardButton::Abort)->setText(u"取消"_qs);
        auto desicion=msgBox.exec();
        if(desicion==QMessageBox::StandardButton::Abort){
            return;
        }
    }
    {
        {
            auto targetRootDir=QDir(targetRootDirPath);
            if(!targetRootDir.exists()){
                auto succeeded=targetRootDir.mkpath(targetRootDir.absolutePath());
                if(!succeeded){
                    QMessageBox msgBox;
                    msgBox.setText(
                                u"无法在\"%1\"创建文件夹\n"
                                "请检查路径是否有效，或是否对路径有足够的权限"_qs
                                .arg(targetRootDir.absolutePath()));
                    msgBox.addButton(QMessageBox::StandardButton::Yes)->setText(u"确认"_qs);
                    msgBox.exec();
                    return;
                }
            }
        }
        {
            auto checkTargetRootPathErrMsg=checkTargetRootPath(targetRootDirPath);
            if(checkTargetRootPathErrMsg!=""){
                if(checkTargetRootPathErrMsg=="文件夹不为空"){
                    checkTargetRootPathErrMsg.append(u"\n"
                                                     "您可以点击\"确定\"执行覆盖导出，新的文件会覆盖原有的文件\n"
                                                     "也可以点击\"取消\"取消本次导出操作，然后手动修改导出路径"_qs
                                                     );
                    QMessageBox msgBox;
                    msgBox.setText(checkTargetRootPathErrMsg);
                    msgBox.addButton(QMessageBox::StandardButton::Yes)->setText(u"确认"_qs);
                    msgBox.addButton(QMessageBox::StandardButton::Cancel)->setText(u"取消"_qs);
                    auto desicion=msgBox.exec();
                    if(desicion==QMessageBox::StandardButton::Cancel){
                        return;
                    }
                }else{
                    QMessageBox msgBox;
                    msgBox.setText(
                                u"指定的视频导出路径无效\n"
                                "错误信息：%1"_qs
                                .arg(checkTargetRootPathErrMsg));
                    msgBox.addButton(QMessageBox::StandardButton::Yes)->setText(u"确认"_qs);
                    msgBox.exec();
                    return;
                }
            }
        }
    }

    if(exportWorkerThread){
        QMessageBox msgBox;
        msgBox.setText(u"当前已有导出任务正在进行中\n点击\"确定\"将中止当前导出任务并开始新任务\n点击\"取消\"将不进行任何操作"_qs);
        msgBox.addButton(QMessageBox::StandardButton::Yes)->setText(u"确定"_qs);
        msgBox.addButton(QMessageBox::StandardButton::Cancel)->setText(u"取消"_qs);
        auto decision=msgBox.exec();
        if(decision==QMessageBox::StandardButton::Cancel){
            return;
        }
        abortExport();
    }
    {
        QMessageBox msgBox;
        msgBox.setText(u"生成任务中  "_qs);
        msgBox.setModal(true);
        msgBox.setStandardButtons(QMessageBox::StandardButton::NoButton);
        msgBox.setWindowFlags(Qt::CustomizeWindowHint);
        msgBox.show();
        qApp->processEvents();
        qApp->processEvents();
        qApp->processEvents();
        exportWorkerThread=new ExportWorkerThread(sourceRootDirPath,targetRootDirPath,bVideoTreeWidget->invisibleRootItem());
    }
    connect(exportWorkerThread,&ExportWorkerThread::exportStart,this,[this](int taskCount)->void{
        exportProgressBar->setMaximum(taskCount);
        exportProgressBar->setValue(0);
        exportProgressBar->setFormat(u"%v/%m"_qs);
    });
    connect(exportWorkerThread,&ExportWorkerThread::shouldShowWarnMessageBox,this,[this](QString message,QMessageBox::StandardButtons excludeButtons)->void{
        QMessageBox msgBox;
        msgBox.setText(message);
        if(!(excludeButtons&QMessageBox::StandardButton::YesToAll)){
            msgBox.addButton(QMessageBox::StandardButton::YesToAll)->setText(u"不要再提醒我"_qs);
        }
        if(!(excludeButtons&QMessageBox::StandardButton::Yes)){
            msgBox.addButton(QMessageBox::StandardButton::Yes)->setText(u"知道了"_qs);
        }
        if(!(excludeButtons&QMessageBox::StandardButton::Abort)){
            msgBox.addButton(QMessageBox::StandardButton::Abort)->setText(u"中止"_qs);
        }
        auto decision=msgBox.exec();
        if(decision==QMessageBox::StandardButton::YesToAll){
            if(exportWorkerThread){
                exportWorkerThread->neverRemindMe=true;
            }
        }else if(decision==QMessageBox::StandardButton::Abort){
            if(exportWorkerThread){
                exportWorkerThread->aborted=true;
            }
        }
        if(exportWorkerThread){
            exportWorkerThread->threadSemaphore.release();
        }
        if(exportWorkerThread && static_cast<bool>(exportWorkerThread->aborted)==true){
            abortExport();
        }

    });
    connect(exportWorkerThread,&ExportWorkerThread::currentTaskDescriptionUpdated,this,[this](QString taskDescription)->void{
        QFontMetrics fontMetrics(exportCurrentTaskDescriptionLabel->font());
        QString elidedTaskDescription = fontMetrics.elidedText(taskDescription, Qt::ElideLeft, exportCurrentTaskDescriptionLabel->width());
        exportCurrentTaskDescriptionLabel->setText(elidedTaskDescription);
    });
    connect(exportWorkerThread,&ExportWorkerThread::progressValueChanged,this,[this](int value)->void{
        exportProgressBar->setValue(value);
    });
    connect(exportWorkerThread,&ExportWorkerThread::exportSuccessfullyFinished,this,[this]()->void{
        if(exportProgressBar->value()==exportProgressBar->maximum()){
            exportProgressBar->setFormat(u"导出完成"_qs);
        }
        if(exportWorkerThread){
            exportWorkerThread->wait();
            exportWorkerThread=nullptr;
        }
        exportCurrentTaskDescriptionLabel->setText("");
        abortExportButton->setEnabled(false);
        startExportButton->setEnabled(true);
        QMessageBox msgBox;
        msgBox.setText(u"导出任务已完成"_qs);
        msgBox.setModal(true);
        msgBox.addButton(QMessageBox::StandardButton::Ok)->setText(u"好的"_qs);
        msgBox.exec();
    });
    connect(exportWorkerThread,&ExportWorkerThread::exportAborted,this,[this]()->void{
        abortExportButton->setEnabled(false);
        startExportButton->setEnabled(true);
        exportProgressBar->setFormat(
                    u"%1 - %2"_qs.arg(
                        exportProgressBar->format(),
                        u"导出中止"_qs
                        )
                    );
        QMessageBox msgBox;
        msgBox.setText(u"导出任务已中止"_qs);
        msgBox.setModal(true);
        msgBox.addButton(QMessageBox::StandardButton::Ok)->setText(u"好的"_qs);
        msgBox.exec();
    });
    exportWorkerThread->start();
    abortExportButton->setEnabled(true);
    startExportButton->setEnabled(false);

}

void MainWidget::abortExport()
{
    QMessageBox msgBox;
    if(exportWorkerThread){
        msgBox.setText(u"请等待中止导出任务  "_qs);
        msgBox.setModal(true);
        msgBox.setStandardButtons(QMessageBox::StandardButton::NoButton);
        msgBox.setWindowFlags(Qt::CustomizeWindowHint);
        msgBox.show();
        qApp->processEvents();
        qApp->processEvents();
        qApp->processEvents();
    }
    if(exportWorkerThread){
        exportWorkerThread->requestInterruption();
        exportWorkerThread->wait();
        exportWorkerThread=nullptr;
    }
    abortExportButton->setEnabled(false);
    startExportButton->setEnabled(true);
}

QString MainWidget::checkSourceRootPath(QString path)
{

    QDir dir(path.trimmed());
    QString errMsg="";
    if(!(dir.isAbsolute()&&dir.exists())){
        errMsg="文件夹不存在";
    }
    return errMsg;
}

QString MainWidget::checkTargetRootPath(QString path)
{
    QDir dir(path.trimmed());
    QString errMsg="";
    if(dir.isAbsolute()){
        if(dir.exists()){
            if(!dir.entryList(
                        QDir::Filter::AllEntries
                        |QDir::Filter::NoDotAndDotDot
                        ).empty()){
                errMsg="文件夹不为空";
            }
        }
    }else{
        errMsg="不合法的路径";
    }
    return errMsg;
}

QString MainWidget::withoutIllegalCharacterForLocalCodePage(const QString &str)
{
    auto textCodec=QTextCodec::codecForLocale();
    auto newStr=QString();
    for(const auto& c:str){
        if(c.isPrint() && textCodec->canEncode(c)){
            newStr.append(c);
        }else{
            newStr.append("⍰");
        }
    }
    return newStr;
}


