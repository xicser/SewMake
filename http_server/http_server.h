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
#include "threadpool.hpp"
#include "logger.h"

#include <string>

using namespace std;

#define EVENT_READY_CNT (10000)

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

    string workDir;           //工作目录
    int listenSockfd;         //监听套接字
    int serverPort;           //服务器端口号
    int backlog;              //未accept队列中最大连接数
    thread_pool threadpool;   //线程池
    static int epollfd;       //epoll句柄
    struct epoll_event eventReady[EVENT_READY_CNT];
};

#endif

