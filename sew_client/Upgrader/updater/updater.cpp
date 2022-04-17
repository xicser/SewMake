#include "updater.h"
#include "ui_updater.h"
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QMessageBox>
#include <QSocketNotifier>
#include <JlCompress.h>
#include "utils/string/stringcheck.h"

#define HTML_URL              "http://www.sunristec.com/h-col-119.html"
#define HTML_HTTPS_URL        "https://www.sunristec.com/h-col-119.html"

Updater::Updater(QString versionLocal) :
    QWidget(nullptr), ui(new Ui::Updater)
{
    ui->setupUi(this);

    this->htmlStr = HTML_URL;
    this->localVersionStr = versionLocal;
    this->isMainClose = false;
    ui->labelVersion->setText(tr("当前版本: ") + versionLocal);

    this->setWindowIcon(QIcon(":/icon/resource/01_logo14x14.png"));
    this->setWindowTitle(tr("软件更新"));

    ui->btnCheckUpdate->setText(tr("检查更新"));
    ui->btnUpdate->setText(tr("软件更新"));
    ui->btnExit->setText(tr("退出"));

    ui->labelInfo->setText("");
    ui->progressBar->setValue(0);

    connect(ui->btnCheckUpdate, &QPushButton::clicked, this, &Updater::slotBtnCheckUpdateClicked);
    connect(ui->btnUpdate, &QPushButton::clicked, this, &Updater::slotBtnUpdateClicked);
    connect(ui->btnExit, &QPushButton::clicked, this, &Updater::slotBtnExitClicked);

    manager = new QNetworkAccessManager(this);
    getFileUrl();
}

Updater::~Updater()
{
    delete ui;

    //把下载的压缩包, version.txt删掉
    QDir dirRemove;
    dirRemove.setPath(".");
    dirRemove.remove("./sewmake.zip");
    dirRemove.remove("./version.txt");
}

void Updater::autoUpgrade(QString autoUpgrade)
{
    if (autoUpgrade == "10" || autoUpgrade == "11") {
        slotBtnUpdateClicked();
    }
    if (autoUpgrade == "10" || autoUpgrade == "00")
    {
        htmlStr = HTML_HTTPS_URL;
    }
    else if (autoUpgrade == "11" || autoUpgrade == "01")
    {
        htmlStr = HTML_URL;
    }
}

/* 关闭事件 */
void Updater::closeEvent(QCloseEvent *)
{
    //把下载的压缩包, version.txt删掉
    QDir dirRemove;
    dirRemove.setPath(".");
    dirRemove.remove("./sewmake.zip");
    dirRemove.remove("./version.txt");

    for (int i = 0; i < replyList.size(); i++) {
        replyList[i]->abort();
        replyList[i]->close();
    }

    //取消升级
    sendStrToMain("upgrade cancel");
}

/* 获取文件Url */
void Updater::getFileUrl(void)
{
    //发送get请求
    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(htmlStr)));
    replyList << reply;

    QEventLoop eventLoop;
    connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    eventLoop.exec();       //block until finish

    if (reply->error() != QNetworkReply::NoError) {
        qDebug()<< "reply error:" << reply->errorString();
    }
    else {
        //获取响应信息
        QByteArray replyData = reply->readAll();

        bool hasFound;
        QString urlTmpStr;    //远程文件的url
        char *buffer = replyData.data();
        QVector<int> posVector = StringCheck::KMP_Check(buffer, "data-url=", &hasFound);
        for (int i = 0; i < posVector.size(); i++) {

            urlTmpStr.clear();
            QByteArray baTmp;
            baTmp.clear();
            for (int j = posVector[i] + 10; buffer[j] != '"'; j++) {
                baTmp.append(buffer[j]);
            }
            urlTmpStr = QString(baTmp);

            if (urlTmpStr.contains("version.txt")) {
                versionFileUrl = urlTmpStr;
            } else if (urlTmpStr.contains("SewMake.zip")) {
                softWareFileUrl = urlTmpStr;
            }
        }
    }
}

/* 给主程序发送命令 */
void Updater::sendStrToMain(QString cmd)
{
    QFile fileOut;
    fileOut.open(stdout, QIODevice::WriteOnly);
    fileOut.write(cmd.toLatin1());
    fflush(stdout);
    fileOut.close();
}

/* 设置提示信息 */
void Updater::setInfoText(QString text)
{
    ui->labelInfo->setText(text);
}

/* 设置进度条 */
void Updater::setProgressBar(int pos)
{
    if (pos >= 100) {
        pos = 100;
    }
    else if (pos <= 0) {
        pos = 0;
    }
    ui->progressBar->setValue(pos);
}

/* 检查更新槽函数 */
void Updater::slotBtnCheckUpdateClicked(void)
{
    this->setInfoText(tr("检测新版本..."));
    getFileUrl();
    ui->btnUpdate->setEnabled(false);
    ui->btnCheckUpdate->setEnabled(false);


    //发送get请求
    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(versionFileUrl)));
    replyList << reply;

    QEventLoop eventLoop;
    connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    eventLoop.exec();       //block until finish

    if (reply->error() != QNetworkReply::NoError) {
        qDebug()<< "reply error:" << reply->errorString();
    }
    else {
        //获取响应信息
        QByteArray replyData = reply->readAll();
        this->createFile(&replyData, "txt");
        bool isNewest = this->checkNewestVersion();
        if (isNewest == true) {
            this->setInfoText(tr("软件已是最新版本！"));
        } else {
            this->setInfoText(QString(tr("发现新版本%1！")).arg(remoteVersionStr));
        }
    }

    ui->btnUpdate->setEnabled(true);
    ui->btnCheckUpdate->setEnabled(true);
}

/* 更新槽函数 */
void Updater::slotBtnUpdateClicked(void)
{
    //先检查更新
    slotBtnCheckUpdateClicked();

    ui->btnUpdate->setEnabled(false);
    ui->btnCheckUpdate->setEnabled(false);

    if (this->remoteVersionStr != this->localVersionStr) {

        //等待800ms
        QTime dieTime = QTime::currentTime().addMSecs(800);
        while( QTime::currentTime() < dieTime ) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

        //发送get请求
        this->setInfoText(tr("更新中..."));
        QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(softWareFileUrl)));
        connect(reply, &QNetworkReply::downloadProgress, this, &Updater::slotShowProgress);
        replyList << reply;

        QEventLoop eventLoop;
        connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
        eventLoop.exec();       //block until finish

        if (reply->error() != QNetworkReply::NoError) {
            qDebug()<< "reply error:" << reply->errorString();
        }
        else {
            //获取响应信息
            QByteArray replyData = reply->readAll();
            this->createFile(&replyData, "zip");
            //校验MD5
            if (checkMd5() == false) {
                QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("MD5校验失败，请重试！"));
                ui->btnUpdate->setEnabled(true);
                ui->btnCheckUpdate->setEnabled(true);
                this->setProgressBar(0);

                //把下载的压缩包, version.txt删掉
                QDir dirRemove;
                dirRemove.setPath(".");
                dirRemove.remove("./sewmake.zip");
                dirRemove.remove("./version.txt");

                return;
            }

            QMessageBox *pbox = new QMessageBox(this);
            pbox->setWindowTitle(tr("SewMake数控编制软件"));
            pbox->setInformativeText(tr("更新成功，软件将重启！"));
            QTimer::singleShot(2000, pbox, SLOT(accept()));
            pbox->exec();

            //请求关闭主程序
            sendStrToMain("request close");

            //等待主程序关闭
            while (1) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 500);
                QFile file("../main.closed");
                if (file.open(QIODevice::ReadOnly) == true) {
                    file.close();
                    break;
                }
            }

            //再多等一会儿确保主程序确实关闭了
            QTime dieTime = QTime::currentTime().addMSecs(800);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            }

            //替换
            replace();

            //重启主软件
            QProcess processSewMake;
            processSewMake.start("../SewMake.exe");

            ::exit(0);
        }
    }

    ui->btnUpdate->setEnabled(true);
    ui->btnCheckUpdate->setEnabled(true);
}

/* 退出槽函数 */
void Updater::slotBtnExitClicked(void)
{
    this->close();
}

/* 显示进度条 */
void Updater::slotShowProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    this->setProgressBar((double)bytesReceived / (double)bytesTotal * 100);
}

/* 创建文件 */
bool Updater::createFile(QByteArray *data, QString fileType)
{
    QFile file;
    if (fileType == "zip") {
        file.setFileName("./sewmake.zip");
    }
    else if (fileType == "txt") {
        file.setFileName("./version.txt");
    }
    else if (fileType == "xml") {
        file.setFileName("./html.xml");
    }

    bool ret = file.open((QIODevice::WriteOnly | QIODevice::Truncate));
    if (ret == false) {
        return false;
    }

    QDataStream aStream(&file);
    aStream.setByteOrder(QDataStream::LittleEndian);
    aStream.writeRawData(data->data(), data->size());
    file.close();

    return true;
}

/* 检测本地版本是否是最新的 */
bool Updater::checkNewestVersion(void)
{
    QFile file("./version.txt");
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream inStream(&file);
    QString versionString = inStream.readAll();
    file.close();

    QStringList versionStrList = versionString.split(" ");
    remoteVersionStr = versionStrList[0];
    remoteVersionStr += " ";
    remoteVersionStr += versionStrList[1];
    md5Str = versionStrList[2];

    if (remoteVersionStr == localVersionStr) {
        return true;
    }
    else {
        return false;
    }
}

/* 检查下载的zip的MD5 */
bool Updater::checkMd5(void)
{
    QFile file("./sewmake.zip");
    if(!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QCryptographicHash md5Hash(QCryptographicHash::Md5);
    md5Hash.reset();

    //文本读取
    while(!file.atEnd()) {
        md5Hash.addData(file.readLine());
    }

    QString md5File = QString(md5Hash.result().toHex()).toUpper();

    return md5File == md5Str;
}

/* 替换 */
void Updater::replace(void)
{
    QDir dir;
    QString dirTmpFilePath = "..";
    dir.setPath(dirTmpFilePath);
    QFileInfoList infoList = dir.entryInfoList();

    for (int i = 0; i < infoList.size(); i++) {

        QFileInfo info = infoList[i];

        if (info.fileName() == "." || info.fileName() == "..") {
            continue;
        }

        if (info.isDir() == true && info.fileName() == "Upgrade") {
            continue;
        }

        if (info.isDir() == true) {
            QDir dirRemove;
            dirRemove.setPath("../" + info.fileName());
            dirRemove.removeRecursively();
        } else {
            QDir dirRemove;
            dirRemove.setPath("../");
            dirRemove.remove(info.fileName());
        }
    }

    //解压
    JlCompress::extractDir("./sewmake.zip", "..");

    //把下载的压缩包, version.txt也删掉
    QDir dirRemove;
    dirRemove.setPath(".");
    dirRemove.remove("./sewmake.zip");
    dirRemove.remove("./version.txt");
}
