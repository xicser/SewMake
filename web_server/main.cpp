#include "http_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <assert.h>

void sigAlarmHandler(int);

void addsig(int sig, void( handler )(int))
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = handler;
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
}

int main(int argc, char *argv[])
{
    //改变进程工作目录
    int ret = chdir(argv[1]);
    if (ret != 0) {
        perror("chdir error");  
        exit(2);
    }

    /* 服务器调用close断开连接了, 另一端还在写, 会产生sigpipe, 导致进程被终止, 这里对其作忽略 */
    addsig(SIGPIPE, SIG_IGN);

    HttpServer* server = new HttpServer();
    server->run();

    delete server;

    return 0;
}

