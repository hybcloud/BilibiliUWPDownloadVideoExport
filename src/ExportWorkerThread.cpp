#include "ExportWorkerThread.hpp"

#include <QMessageBox>
#include <QPushButton>
#include <QDir>

#include "BVideoTreeWidget.hpp"
#include "QtItemDataRoleUserRole.hpp"

ExportWorkerThread::ExportWorkerThread(QString sourceRootPath,QString targetRootPath,QTreeWidgetItem* rootItem)
    : neverRemindMe(false),
      aborted(false),
      threadSemaphore(),
      sourceRootPath_(sourceRootPath),
      targetRootPath_(targetRootPath),
      tasks_(),
      taskCount_(0)
{
    for(int i=0;i<rootItem->childCount();i++){
        auto seriesItem=rootItem->child(i);
        if(seriesItem->checkState(BVideoTreeWidget::Column::Title)!=Qt::CheckState::Unchecked){
            SeriesInfo seriesInfo;
            seriesInfo.title=seriesItem->text(BVideoTreeWidget::Column::Title);
            seriesInfo.uploader=seriesItem->text(BVideoTreeWidget::Column::Uploader);
            seriesInfo.BID=seriesItem->text(BVideoTreeWidget::Column::BV);
            seriesInfo.relativePath=seriesItem->data(
                        BVideoTreeWidget::Column::MetaData,
                        QtItemDataRoleUserRole::BVideoTreeWidgetItem_SeriesRelativePathRole
                        )
                    .toString();
            for(int j=0;j<seriesItem->childCount();j++){
                auto partItem=seriesItem->child(j);
                if((partItem->checkState(BVideoTreeWidget::Column::Title))!=Qt::CheckState::Unchecked){
                    PartInfo partInfo;
                    partInfo.title=partItem->text(BVideoTreeWidget::Column::Title);
                    partInfo.PID=partItem->data(
                                BVideoTreeWidget::Column::MetaData,
                                QtItemDataRoleUserRole::BVideoTreeWidgetItem_PartNumberRole
                                )
                            .toInt();
                    partInfo.relativePath=partItem->data(
                                BVideoTreeWidget::Column::MetaData,
                                QtItemDataRoleUserRole::BVideoTreeWidgetItem_PartRelativePathRole
                                )
                            .toString();
                    taskCount_+=1;
                    seriesInfo.partInfoList.push_back(partInfo);
                }
            }
            tasks_.push_back(seriesInfo);
        }
    }
}


void ExportWorkerThread::run()
{
    emit exportStart(taskCount_);
    const auto& sourceRootDirPath=sourceRootPath_;
    const auto& targetRootDirPath=targetRootPath_;
    const auto& tasks=tasks_;
    int progressValue=0;
    class CriticalError{};
    class UnimportantError{};
    auto execErrorMsgBox=[this](
            QString message,
            QMessageBox::StandardButtons excludeButtons=QMessageBox::StandardButton::NoButton
            )->void{   // clazy:exclude=lambda-in-connect
        if(!neverRemindMe){
            emit shouldShowWarnMessageBox(message,excludeButtons);
            threadSemaphore.acquire();
        }
        if(aborted){
            throw CriticalError();
        }
        throw UnimportantError();
    };
    try{

        auto sourceRootDir=QDir(sourceRootDirPath);
        auto targetRootDir=QDir(targetRootDirPath);
        static QRegularExpression illegalCharactersRegex(u"[\\/:*?\"<>|]"_qs);
        const int bufferSizeInMB=32;
        auto fileCopyBuffer=QScopedPointer<QByteArray>(new QByteArray(bufferSizeInMB*1024*1024,'\0'));

        for(auto& seriesInfo:tasks){
            if(isInterruptionRequested()){
                throw CriticalError();
            }
            try{
                auto seriesCountStringSize=QString::number(seriesInfo.partInfoList.size()).size();
                auto seriesSourceDir=QDir(sourceRootDir.absoluteFilePath(seriesInfo.relativePath));
                auto seriesTitleForWindowsPath=u"[%1]%2[%3]"_qs.arg(
                            QString(seriesInfo.uploader).replace(illegalCharactersRegex,u"⍰"_qs),
                            QString(seriesInfo.title).replace(illegalCharactersRegex,u"⍰"_qs),
                            seriesInfo.BID
                            );

                auto seriesTargetDir=QDir(targetRootDir.absoluteFilePath(seriesTitleForWindowsPath));
                seriesTargetDir.mkpath(seriesTargetDir.absolutePath());

                for(auto& partInfo: seriesInfo.partInfoList ){
                    if(isInterruptionRequested()){
                        throw CriticalError();
                    }
                    try{
                        auto currentTaskDescription=u"正在转换：[%1][%2][%3]%4"_qs
                                .arg(
                                    seriesInfo.uploader,
                                    seriesInfo.title,
                                    QString::number(partInfo.PID),
                                    partInfo.title
                                    );
                        emit currentTaskDescriptionUpdated(
                                    currentTaskDescription
                                    );
                        auto partSourceDir=QDir(seriesSourceDir.absoluteFilePath(partInfo.relativePath));
                        auto partSourceFilePath=partSourceDir.absoluteFilePath(
                                    u"%1_%2_0.mp4"_qs
                                    .arg(seriesInfo.relativePath,partInfo.relativePath)
                                    );
                        auto partSourceFile=QFile(partSourceFilePath);

                        auto& partID=partInfo.PID;
                        auto partTitleForWindowsPath=QString(partInfo.title).replace(illegalCharactersRegex,u"⍰"_qs);
                        auto partTargetFilePath=seriesTargetDir.absoluteFilePath(
                                    u"[P%1]%2.mp4"_qs
                                    .arg(
                                        QString::number(partID).rightJustified(
                                            seriesCountStringSize,
                                            '0'
                                            ),
                                        partTitleForWindowsPath
                                        )
                                    );
                        auto partTargetFile=QFile(partTargetFilePath);
                        auto succeedToOpenSourceFile=partSourceFile.open(QFile::OpenModeFlag::ReadOnly|QFile::OpenModeFlag::ExistingOnly);
                        if(!succeedToOpenSourceFile){
                            execErrorMsgBox(u"无法打开源文件\"%1\"\n"
                                            "请检查文件是否不存在或被占用"_qs
                                            .arg(partSourceFilePath));
                        }
                        auto succeedToOpenTargetFile=partTargetFile.open(QFile::OpenModeFlag::WriteOnly|QFile::OpenModeFlag::Truncate);
                        if(!succeedToOpenTargetFile){
                            execErrorMsgBox(u"无法打开目标文件\"%1\"\n"
                                            "可能没有访问对应路径的权限\n"
                                            "或对应路径存在文件且正在被占用"_qs
                                            .arg(partTargetFilePath));
                        }

                        partSourceFile.read(3);
                        long long exportedSize=0;
                        while(!partSourceFile.atEnd()){
                            if(isInterruptionRequested()){
                                throw CriticalError();
                            }
                            auto actualSize=partSourceFile.read(fileCopyBuffer->data(),fileCopyBuffer->size());
                            partTargetFile.write(fileCopyBuffer->data(),actualSize);
                            exportedSize+=actualSize;
                            double exportedSizeInMB=static_cast<double>(exportedSize)/1024/1024;
                            emit currentTaskDescriptionUpdated(
                                        u"%1[%2MB]"_qs
                                        .arg(
                                            currentTaskDescription,
                                            QString::number(exportedSizeInMB)
                                            )
                                        );
                        }
                        progressValue+=1;
                        emit progressValueChanged(progressValue);
                    } catch (UnimportantError) {;}
                }
            }catch(UnimportantError){;}
        }
        emit exportSuccessfullyFinished();
    }catch(CriticalError){
        emit exportAborted();
    }

}

