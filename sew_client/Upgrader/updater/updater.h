#ifndef UPDATER_H
#define UPDATER_H

#include <QWidget>
#include <QSocketNotifier>
#include <QFile>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QTimer>

namespace Ui {
class Updater;
}

class Updater : public QWidget
{
    Q_OBJECT

public:
    Updater(QString versionLocal);
    ~Updater();
    void autoUpgrade(QString autoUpgrade);
protected:
    void closeEvent(QCloseEvent *);

private:
    Ui::Updater *ui;

    QString htmlStr;           //选择获取文件url的网页地址
    QString localVersionStr;   //本地版本字符串
    QString remoteVersionStr;  //远程版本字符串
    QString md5Str;            //MD5字符串
    QString versionFileUrl;    //远程文件的url
    QString softWareFileUrl;

    QFile fileIn;              //读取主进程的数据文件
    QNetworkAccessManager *manager;
    QSocketNotifier *socketNotifier;
    QList<QNetworkReply *> replyList;
    bool isMainClose;          //指示主程序是否关闭

    void getFileUrl(void);                  //获取文件Url
    void sendStrToMain(QString);            //给主程序发送命令
    void setInfoText(QString);              //设置提示信息
    void setProgressBar(int pos);           //设置进度条
    bool createFile(QByteArray *data, QString fileType);   //创建文件
    bool checkNewestVersion(void);          //检测本地版本是否是最新的
    bool checkMd5(void);                    //检查下载的zip的MD5
    void replace(void);                     //替换

private slots:
    void slotBtnCheckUpdateClicked(void);
    void slotBtnUpdateClicked(void);
    void slotBtnExitClicked(void);

    void slotShowProgress(qint64 bytesReceived, qint64 bytesTotal);
};

#endif // UPDATER_H
