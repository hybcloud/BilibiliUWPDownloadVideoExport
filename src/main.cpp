#include "MainWidget.hpp"

#include <QApplication>

int main(int argc, char *argv[])
{
    QGuiApplication::setApplicationName(u"BilibiliUWP下载缓存提取助手"_qs);
    QGuiApplication::setOrganizationName(u"Foolriver"_qs);
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(u":/image/favicon.png"_qs));
    MainWidget w;
    w.show();
    return app.exec();
}
