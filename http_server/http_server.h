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

#include <string>

using namespace std;

#define MAX_LISTEN_EVENTS       (99999)
#define EVENT_READY_CNT         (10000)
#define EVENT_INFO_BUFFLEN      (4096)


class HttpServer {
public:
    HttpServer(int backlog = 128, int serverPort = 9527, int threadCount = 8, string workDir = ".");
    ~HttpServer();
    void run(void);

private:
    void serverInit(void);

    static int  httpBuildResponseMsgHeader(int no, const char *describe, const char *type, int len, char *out_buffer);
    static int  httpBuildErrorMsg(int no, const char *describe, const char *text, char *out_buffer);
    static void doSendError(int commSockfd, int no, const char *describe, const char *text);
    static void doSendFile(int commSockfd, const char *filepath);
    static void doSendDir(int commSockfd, const char *dirname);
    static void doHttpRequest(int commSockfd, const char *filepath);

    //描述就绪文件描述符的相关信息
    typedef struct
    {
        int fd;             //要监听的文件描述符
        int events;         //对应的监听事件, EPOLLET + EPOLLIN和EPLLOUT
        void *arg;          //指向自己结构体指针
        void (*call_back)(int fd, int events, void *arg); //回调函数
        int status;         //是否在监听: 1->在红黑树上(监听), 0->不在(不监听)
        char buf[EVENT_INFO_BUFFLEN];
        int len;
    } EventInfoBlock_t;

    static EventInfoBlock_t g_events[MAX_LISTEN_EVENTS + 1];    //g_events[MAX_LISTEN_EVENTS]位置是监听套接字的EventInfoBlock

    //设置一个EventInfoBlock的内容
    static void eventSet(EventInfoBlock_t *ev, int fd, void (*call_back)(int fd,int events, void *arg), void *arg);
    static void eventAdd(int efd, int events, EventInfoBlock_t *ev);  //向 epoll监听的红黑树 添加一个文件描述符
    static void eventDel(int efd, EventInfoBlock_t *ev);                 //从epoll 监听的 红黑树中删除一个文件描述符
    static void acceptConn(int lfd, int events, void *arg);    //当有文件描述符就绪, epoll返回, 调用该函数与客户端建立链接

    static void recvData(int fd, int events, void *arg);           //接收数据
    static void sendData(int fd, int events, void *arg);           //发送数据


    string workDir;           //工作目录
    int listenSockfd;         //监听套接字
    int serverPort;           //服务器端口号
    int backlog;              //未accept队列中最大连接数
    ThreadPool<EventInfoBlock_t> threadpool;    //线程池
    static int epollfd;       //epoll句柄
    struct epoll_event eventReady[EVENT_READY_CNT];
};

#endif

