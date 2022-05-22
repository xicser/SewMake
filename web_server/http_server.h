#ifndef __HTTP_SERVER__
#define __HTTP_SERVER__

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "file_ops.h"
#include "http_msg_parser.h"
#include "codec.h"
#include "logger.h"
#include "threadpool.h"

#include <atomic>
#include <string>
#include <set>
#include <mutex>

using namespace std;

#define MAX_LISTEN_EVENTS       (65536)     //大于进程支持打开的最大文件描述符个数
#define EVENT_READY_CNT         (10000)
#define EVENT_INFO_BUFFLEN      (4096)

#define TIMER_SHOT              (1)         //定时器倒计时时间
#define ACTIVE_TICKS            (10)        //活跃时间

class HttpServer {
public:
    HttpServer(int backlog = 128, int serverPort = 9527, int threadCount = 8, string workDir = ".");
    ~HttpServer();
    void run(void);

private:
    void serverInit(void);

    //描述就绪文件描述符的相关信息
    typedef struct
    {
        int  fd;            //要监听的文件描述符
        int  events;        //对应的监听事件, EPOLLET, EPOLLIN, EPLLOUT等
        void *arg;          //指向自己结构体指针
        void (*call_back)(int fd, int events, void *arg); //回调函数
        int  status;        //是否在监听: 1->在红黑树上(监听), 0->不在(不监听)
        char recvbuf[EVENT_INFO_BUFFLEN];
        int  recvlen;
        char sendbuf[EVENT_INFO_BUFFLEN * 10];
        int  sendlen;
        atomic<int>  ticks;  //保持活跃连接的倒计时
    } EventInfoBlock_t;

    static void httpSendResponseMsgHeader(int no, const char *describe, const char *type, int len, EventInfoBlock_t *);
    static void httpSendErrorMsg(int no, const char *describe, const char *text, EventInfoBlock_t *);
    static void httpSendFile(const char *filepath, EventInfoBlock_t *);
    static void httpSendDir(const char *dirpath, EventInfoBlock_t *);
    static void doHttpRequest(EventInfoBlock_t *);

    //g_events[MAX_LISTEN_EVENTS]位置是监听套接字的EventInfoBlock
    static EventInfoBlock_t g_events[MAX_LISTEN_EVENTS + 1];

    //设置一个EventInfoBlock的内容
    static void eventSet(EventInfoBlock_t *ev, int fd, int rlen, void (*call_back)(int fd, int events, void *arg), void *arg);
    static void eventAdd(int efd, int events, EventInfoBlock_t *ev);  //向 epoll监听的红黑树 添加一个文件描述符
    static void eventDel(int efd, EventInfoBlock_t *ev);                 //从epoll 监听的 红黑树中删除一个文件描述符
    static void acceptConn(int lfd, int events, void *arg);    //当有文件描述符就绪, epoll返回, 调用该函数与客户端建立链接

    static void recvData(int fd, int events, void *arg);                  //接收数据并处理
    static void sendDataAndCope(int fd, int events, void *arg);      //发送数据
    static void setFileDescriptor(int fd, bool nonblock);                 //设置文件描述符属性

    static void addsig(int sig);
    static void sigAlarmHandler(int signum);

private:
    static HttpServer* serverPtr;

    static set<int> actCommfdSet;    //活跃连接文件描述符集合
    static mutex mux;

    string workDir;           //工作目录
    int listenSockfd;         //监听套接字
    int serverPort;           //服务器端口号
    int backlog;              //未accept队列中最大连接数
    ThreadPool<EventInfoBlock_t> threadpool;    //线程池
    static int epollfd;       //epoll句柄
    struct epoll_event eventReady[EVENT_READY_CNT];
};

#endif

