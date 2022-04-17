#ifndef __HTTP_SERVER__
#define __HTTP_SERVER__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>

#include "http_msg.h"
#include "file_ops.h"
#include "http_msg_parser.h"
#include "codec.h"
#include <string>

using namespace std;

class HttpServer {
public:
    HttpServer(string path);
    void run(void);

private:
    static void buffevEventCallback(struct bufferevent *bev, short events, void *arg);
    static void listenerCallback(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int len, void *ptr);    
    static void buffevReadCallback(struct bufferevent *bev, void *arg);
    static void doHttpRequest(struct bufferevent *bev, const char *filepath);
    static void doSendError(struct bufferevent *bev, int no, const char *describe, const char *text);
    static void doSendFile(struct bufferevent *bev, const char *filepath);
    static void doSendDir(struct bufferevent *bev, const char *dirname);

    string workDir;                     //工作目录

    struct event_base *base;            //event_base
    struct evconnlistener *listener;    //监听器
    struct sockaddr_in server_addr;     //地址结构
    int port;                           //端口号
    int backlog;                        //三次握手成功, 但还未通过accpet取走的连接数量
};

#endif

