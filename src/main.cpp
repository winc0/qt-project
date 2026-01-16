#include "include/mainwindow.h"
#include "include/config.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置应用程序信息（用于 QSettings 存储路径）
    QCoreApplication::setOrganizationName(GameConfig::ORG_NAME);
    QCoreApplication::setApplicationName(GameConfig::APP_NAME);

    qDebug() << "Application starting...";

    MainWindow w;
    qDebug() << "MainWindow created, showing...";
    w.show();

    qDebug() << "Application running";
    return a.exec();
}
