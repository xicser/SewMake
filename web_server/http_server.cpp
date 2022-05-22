#include "http_server.h"
#include <dirent.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>


int HttpServer::epollfd;
HttpServer::EventInfoBlock_t HttpServer::g_events[MAX_LISTEN_EVENTS + 1];
HttpServer* HttpServer::serverPtr;
set<int> HttpServer::actCommfdSet;
mutex HttpServer::mux;

HttpServer::HttpServer(int backlog, int serverPort, int threadCount, string workDir)
{
    this->backlog = backlog;
    this->serverPort = serverPort;
    this->workDir = workDir;

    this->serverPtr = this;

    printf("backlog = %d\n", this->backlog);
    printf("serverPort = %d\n", this->serverPort);
    printf("threadCount = %d\n", threadCount);
    printf("workDir = %s\n", this->workDir.c_str());

    this->serverInit();
}

HttpServer::~HttpServer()
{
    close(listenSockfd);
}

void HttpServer::addsig(int sig)
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = sigAlarmHandler;
    sa.sa_flags |= SA_RESTART;
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
}

/* 闹钟信号handler */
void HttpServer::sigAlarmHandler(int signum)
{
    mux.lock();
    //记录当前活跃文件描述符的快照, 此后新加入的, 不关心
    set<int> actCommfdSetCopy = actCommfdSet;
    mux.unlock();

    for (int fd : actCommfdSetCopy) {
        if (fd == serverPtr->listenSockfd) {
            continue;
        }

        g_events[fd].ticks--;
        int ticks = g_events[fd].ticks.load();
        if (ticks <= 0) {

            LOG_PRINT("server close commfd: %d\n", fd);

            //非活跃了, 服务器主动断开连接
            eventDel(epollfd, &g_events[fd]);
            close(fd);
        }
    }

    alarm(TIMER_SHOT);
}

void HttpServer::serverInit(void)
{
    int ret;

    //检测非活跃连接
    addsig(SIGALRM);
    alarm(TIMER_SHOT);

    //创建连接套接字
    listenSockfd = socket(AF_INET, SOCK_STREAM, 0);  //IPv4, 基于字节流的通信, 即TCP
    if (listenSockfd < 0) {
        perror("socket error");
        exit(1);
    }

    LOG_PRINT("listenSockfd = %d\n", listenSockfd);

    //设置在2msl阶段, 端口可复用
    int opt = 1;
    setsockopt(listenSockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //给套接字里面绑定服务器信息
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;                   //IPv4
    server_addr.sin_port = htons(this->serverPort);     //port
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);    //设置ip地址(本机所有网卡的ip)
    ret = bind(listenSockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        perror("bind error");
        exit(2);
    }

    //设置未accept队列中最大连接数
    ret = listen(listenSockfd, backlog);
    if (ret < 0) {
        perror("listen error");
        exit(3);
    }

    //创建epoll模型, efd是红黑树根结点文件描述符
    epollfd = epoll_create(1024);   //Linux 2.6.8之后size被忽略
    if (epollfd == -1) {
        perror("epoll_create error");
        exit(1);
    }

    LOG_PRINT("epollfd = %d\n", epollfd);

    //把监听fd设置成非阻塞方式
    setFileDescriptor(listenSockfd, true);

    //把监听fd挂到红黑树上, 设置成边沿触发方式
    eventSet(&g_events[MAX_LISTEN_EVENTS], listenSockfd, -1, acceptConn, &g_events[MAX_LISTEN_EVENTS]);
    eventAdd(epollfd, EPOLLIN | EPOLLET, &g_events[MAX_LISTEN_EVENTS]);
}

/* 运行http服务器 */
void HttpServer::run(void)
{
    while (1) {
        int ready = epoll_wait(epollfd, eventReady, EVENT_READY_CNT, -1);
        if (ready == -1) {
            if (errno != EINTR && errno != ECONNABORTED) {
                perror("epoll_wait error");
                exit(3);
            }
            continue;
        }

        for (int i = 0; i < ready; i++) {

            //eventadd()函数中, 添加到监听树中监听事件的时候将EventInfoBlock_t结构体类型给了ptr指针
            //这里epoll_wait返回的时候, 同样会返回对应fd的EventInfoBlock_t类型的指针
            EventInfoBlock_t *ev = (EventInfoBlock_t *)eventReady[i].data.ptr;

            //如果发生的是读事件, 并且监听的是读事件
            if ((eventReady[i].events & EPOLLIN) && (ev->events & EPOLLIN)) {
                threadpool.append(ev);
            }
            //如果发生的是写事件, 并且监听的是写事件
            else if ((eventReady[i].events & EPOLLOUT) && (ev->events & EPOLLOUT)) {
                threadpool.append(ev);
            }
        }
    }
}

/* 当有文件描述符就绪, epoll返回, 调用该函数与客户端建立链接 */
void HttpServer::acceptConn(int lfd, int events, void *arg)
{
    struct sockaddr_in client_addr; //返回链接客户端地址信息, 含IP地址和端口号
    socklen_t client_addr_len = sizeof(client_addr);

    while (1) {
        int commfd = accept(lfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if ((commfd == -1 && errno == EAGAIN) || (commfd == -1 && errno == EWOULDBLOCK)) {
            break;
        }

#ifdef LOG_ENABLE
        //打印一下连接上的客户端的信息
        printf("commfd = %d, ", commfd);
        char ip_str[25];
        printf("ip = %s, port = %d connected\n\n",
                inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ip_str, sizeof(ip_str)), 
                ntohs(client_addr.sin_port));
#endif
        //把这个通信套接字设置成非阻塞
        setFileDescriptor(commfd, true);

        //用commfd作为下标, 一定不会冲突, 将其添加到监听树中, 并监听读事件
        eventSet(&g_events[commfd], commfd, 0, recvData, &g_events[commfd]);
        eventAdd(epollfd, EPOLLIN | EPOLLET | EPOLLONESHOT, &g_events[commfd]);
    }
}

/* 接收数据 */
void HttpServer::recvData(int fd, int events, void *arg)
{
    EventInfoBlock_t *ev = (EventInfoBlock_t *)arg;
    int len = 0;

    //重设活跃ticks
    ev->ticks.store(ACTIVE_TICKS);

    int readStep = 1024;
    while (1) {

        int ret = read(fd, ev->recvbuf + len, readStep);
        if ((ret == -1 && errno == EAGAIN) || (ret == -1 && errno == EWOULDBLOCK)) {
            break;
        }

        //对端关闭, 进入close_wait状态
        if (ret == 0) {
            break;
        }
        len += ret;
    }

    eventDel(epollfd, ev);                            //将该节点从红黑树上摘除

    if (len == 0) {
        //对端关闭
        close(ev->fd);
        LOG_PRINT("client [commfd = %d] pos[%ld], closed\n\n", fd, ev-g_events);

        return;
    }

#ifdef LOG_ENABLE
        //LOG_PRINT("read content\n\n");
        //ev->recvbuf[len] = '\0';
        //LOG_PRINT("read content: [%d]:\n%s\n", fd, ev->recvbuf);
#endif

    eventSet(ev, fd, len, sendDataAndCope, ev);         //设置该fd对应的回调函数为sendData
    eventAdd(epollfd, EPOLLOUT | EPOLLONESHOT, ev);     //将fd加入红黑树中, 监听其写事件
}

/* 处理数据并发送 */
void HttpServer::sendDataAndCope(int fd, int events, void *arg)
{
    EventInfoBlock_t *ev = (EventInfoBlock_t *)arg;

    //重设活跃ticks
    ev->ticks.store(ACTIVE_TICKS);

    //把这个通信套接字设置成阻塞
    setFileDescriptor(fd, false);

    /* 业务逻辑处理 */
    doHttpRequest(ev);

    //把这个通信套接字设置成非阻塞
    setFileDescriptor(fd, true);

#ifdef LOG_ENABLE
    //ev->sendbuf[ev->sendlen] = 0;
    //LOG_PRINT("send content : \n%s\n", ev->sendbuf);
#endif

    eventDel(epollfd, ev);                        //从红黑树中移除
//    close(fd);  //如果要测试webbench, 发送完成后, 要主动关闭连接
    eventSet(ev, fd, 0, recvData, ev);            //将该fd的回调函数改为recvData
    eventAdd(epollfd, EPOLLIN | EPOLLET | EPOLLONESHOT, ev);     //重新添加到红黑树上, 设为监听读事件
}

/* 设置文件描述符属性 */
void HttpServer::setFileDescriptor(int fd, bool nonblock)
{
    //把这个通信套接字设置成非阻塞
    int flag = fcntl(fd, F_GETFL);

    if (nonblock == true) {
        flag |= O_NONBLOCK;
    }
    else {
        flag &= ~(O_NONBLOCK);;
    }
    fcntl(fd, F_SETFL, flag); 
}

/* 发送http回复报文头 */
void HttpServer::httpSendResponseMsgHeader(int no, const char *describe, const char *type, int len, EventInfoBlock_t* ev)
{
    int cnt = 0;
    char* buffer = ev->sendbuf;

    cnt = sprintf(buffer, "http/1.1 %d %s\r\n", no, describe);        //状态行
    cnt += sprintf(buffer + cnt, "Content-Type:%s\r\n", type);        //类型
    cnt += sprintf(buffer + cnt, "Content-Length:%d\r\n", len);       //长度
    cnt += sprintf(buffer + cnt, "\r\n");                             //空行

    write(ev->fd, buffer, cnt);
}

/* 发送http错误报文 */
void HttpServer::httpSendErrorMsg(int no, const char *describe, const char *text, EventInfoBlock_t* ev)
{
    char* buffer = ev->sendbuf;
    int cnt = 0;

    cnt = sprintf(buffer, "%s %d %s\r\n", "HTTP/1.1", no, describe);
    cnt += sprintf(buffer + cnt, "Content-Type:%s\r\n", "text/html");
    cnt += sprintf(buffer + cnt, "Content-Length:%d\r\n", -1);
    cnt += sprintf(buffer + cnt, "Connection: close\r\n");
    cnt += sprintf(buffer + cnt, "\r\n");

    cnt += sprintf(buffer + cnt, "<html><head><title>%d %s</title></head>\n", no, describe);
    cnt += sprintf(buffer + cnt, "<body bgcolor=\"#cc99cc\"><h2 align=\"center\">%d %s</h4>\n", no, describe);
    cnt += sprintf(buffer + cnt, "%s\n", text);
    cnt += sprintf(buffer + cnt, "<hr>\n</body>\n</html>\n");

    write(ev->fd, buffer, cnt);
}

/* 发送文件内容 */
void HttpServer::httpSendFile(const char *filepath, EventInfoBlock_t* ev)
{
    char* sendBuffer = ev->sendbuf;

    //打开文件
    int fd = open(filepath, O_RDONLY);
    if(fd == -1) {
        httpSendErrorMsg(404, "Not Found", "No such file or directory", ev);
        return;
    }

    //读文件
    while (1) {
        int ret = read(fd, sendBuffer, 1024 * 35);
        if (ret == -1) {
            perror("read file error");
            break;
        }

        if (ret == 0) {
            break;
        }
        else {
            write(ev->fd, sendBuffer, ret);
        }

        //重设活跃ticks
        ev->ticks.store(ACTIVE_TICKS);
    }

    close(fd);
}

/* 发送目录内容 */
void HttpServer::httpSendDir(const char *dirpath, EventInfoBlock_t* ev)
{
    int is_root = 0;
    DIR *dp;
    char tmpstr[4096];
    char enstr[4096];
    char* sendBuffer = ev->sendbuf;

    struct dirent *dp1 = (struct dirent *)malloc(sizeof(struct dirent));
    struct dirent *dentry = (struct dirent *)malloc(sizeof(struct dirent));

    dp = opendir(dirpath);
    if (dp == NULL) {
        perror("opendir error");
        return;
    }

    //判断是否在资源根目录
    if (strcmp(dirpath, ".") == 0) {
        is_root = 1;
    }

    int cnt = sprintf(sendBuffer, "<html><head><title>目录名: %s</title></head>", dirpath);
    cnt += sprintf(sendBuffer + cnt, "<body><h1>当前目录: %s</h1><table>", dirpath);

    while (1) {

        //重设活跃ticks
        ev->ticks.store(ACTIVE_TICKS);

        int ret = readdir_r(dp, dp1, &dentry);
        if (ret != 0) {
            perror("readdir_r");
            exit(EXIT_FAILURE);
        }

        if (dentry == NULL) {
            break;
        }

        //拼接文件的完整路径
        sprintf(tmpstr, "%s/%s", dirpath, dentry->d_name);
        struct stat st;
        stat(tmpstr, &st);

        //编码生成 %E5 %A7 之类的东西
        encode_str(enstr, sizeof(enstr), dentry->d_name);

        //目录
        if (dentry->d_type == DT_DIR) {

            if (strcmp(dentry->d_name, ".") == 0) {
                if (is_root == 0) {
                    cnt += sprintf(sendBuffer + cnt,
                        "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>",
                        ".", ".", st.st_size);
                }
            }
            else if (strcmp(dentry->d_name, "..") == 0) {
                if (is_root == 0) {
                    cnt += sprintf(sendBuffer + cnt,
                        "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>",
                        "..", "..", st.st_size);
                }
            }
            else {
                cnt += sprintf(sendBuffer + cnt,
                    "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>",
                    enstr, dentry->d_name, st.st_size);
            }
        }
        //文件
        else {
            cnt += sprintf(sendBuffer + cnt,
                    "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
                    enstr, dentry->d_name, st.st_size);
        }
    }

    cnt += sprintf(sendBuffer + cnt, "</table></body></html>");

    closedir(dp);

    free(dp1);
    free(dentry);

    write(ev->fd, sendBuffer, cnt);
}

/* 处理http请求 */
void HttpServer::doHttpRequest(EventInfoBlock_t *ev)
{
    struct stat sbuf;
    const char *filepath;

    //http解析
    HttpMsgParser parser;
    parser.tryDecode(ev->recvbuf, ev->recvlen);

    const string& method = parser.getMethod();
    const string& url = parser.getUrl();

    //转码 将不能识别的中文乱码 -> 中文
    //解码 %23 %34 %5f
    char path[256];
    strcpy(path, url.c_str());
    decode_str(path, path);

    if (method == "GET") {
        if (strcmp(url.c_str(), "/") == 0) {
            filepath = ".";
        }
        else {
            filepath = path + 1;
        }
        LOG_PRINT("got %s ...\n", filepath);
    }
    else {
        printf("unrecognized request\n");
        httpSendErrorMsg(405, "Not Found", "Can not cope your request", ev);
        return;
    }

    //判断 文件/目录 是否存在
    int ret = stat(filepath, &sbuf);
    if (ret != 0) {
        //回发浏览器 404 错误页面
        httpSendErrorMsg(404, "Not Found", "No such file or directory", ev);
        return;
    }

    //文件
    if (S_ISREG(sbuf.st_mode)) {

        //发送回发报文头
        httpSendResponseMsgHeader(200, "OK", get_file_type(filepath), sbuf.st_size, ev);

        //发送文件内容
        httpSendFile(filepath, ev);
    }
    //目录
    else if (S_ISDIR(sbuf.st_mode)) {

        //发送回发报文头
        httpSendResponseMsgHeader(200, "OK", get_file_type(".html"), -1, ev);

        //发送目录内容
        httpSendDir(filepath, ev);
    }
}

/* 设置一个EventInfoBlock的内容 */
void HttpServer::eventSet(EventInfoBlock_t *ev, int fd, int rlen,
                            void (*call_back)(int fd,int events,void *arg), void *arg)
{
    ev->ticks.store(ACTIVE_TICKS);

    ev->fd = fd;
    ev->call_back = call_back;
    ev->events = 0;
    ev->arg = arg;
    ev->status = 0;
    if(rlen != -1) {
        ev->recvlen = rlen;
    }
}

/* 向 epoll监听的红黑树 添加一个文件描述符 */
void HttpServer::eventAdd(int efd, int events, EventInfoBlock_t *ev)
{
    struct epoll_event epv = {0, {0}};
    int op = 0;
    epv.data.ptr = ev;           // ptr指向一个结构体（之前的epoll模型红黑树上挂载的是文件描述符cfd和lfd，现在是ptr指针）
    epv.events = ev->events = events; //EPOLLET + EPOLLIN 或 EPOLLOUT
    if (ev->status == 0)         //status 说明文件描述符是否在红黑树上: 0不在, 1 在
    {
        op = EPOLL_CTL_ADD;      //将其加入红黑树g_efd, 并将status置1
        ev->status = 1;
    }
    if (epoll_ctl(efd, op, ev->fd, &epv) < 0) // 添加一个节点
    {
        printf("event add failed [fd=%d], events[%d]\n", ev->fd, events);
        perror("epoll_ctl add error");
    }

    mux.lock();
    actCommfdSet.insert(ev->fd);
    mux.unlock();
}

/* 从epoll 监听的 红黑树中删除一个文件描述符 */
void HttpServer::eventDel(int efd, EventInfoBlock_t *ev)
{
    struct epoll_event epv = {0, {0}};
    if(ev->status != 1) //如果fd没有添加到监听树上, 就不用删除，直接返回
        return;
    epv.data.ptr = NULL;
    ev->status = 0;
    if (epoll_ctl(efd, EPOLL_CTL_DEL, ev->fd, &epv) < 0)
    {
        perror("epoll_ctl del error");
    }

    mux.lock();
    actCommfdSet.erase(ev->fd);
    mux.unlock();

    ev->ticks.store(0);
}

