#include "updater/updater.h"
#include <QApplication>
#include <QDir>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //设置程序的工作目录为可执行文件所在目录
    QDir::setCurrent(QCoreApplication::applicationDirPath());

//    setbuf(stdout, NULL); //让printf, qDebug立即输出, 不要缓存

    Updater u(argv[1]);         //argv[1]是版本号
    u.show();
    u.autoUpgrade(argv[2]);     //argv[2]表示是否是自动升级

    return a.exec();
}
