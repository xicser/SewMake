#include "http_server.h"

int HttpServer::epollfd;       //epoll句柄
HttpServer::HttpServer(int backlog, int serverPort, int threadCount, string workDir) : threadpool(threadCount)
{
    this->backlog = backlog;
    this->serverPort = serverPort;
    this->workDir = workDir;

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

void HttpServer::serverInit(void)
{
    int ret;

    //创建连接套接字
    listenSockfd = socket(AF_INET, SOCK_STREAM, 0);  //IPv4, 基于字节流的通信, 即TCP
    if (listenSockfd < 0) {
        perror("socket error");
        exit(1);
    }

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

    //把监听fd设置成非阻塞方式
    int flag = fcntl(listenSockfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(listenSockfd, F_SETFL, flag);

    //把监听fd挂到红黑树上, 设置成边沿触发方式
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = listenSockfd;
    ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, listenSockfd, &event);
    if (ret == -1) {
        perror("epoll_ctl error");
        exit(2);
    }
}

/* 运行http服务器 */
void HttpServer::run(void)
{
    while (1) {
        int ready = epoll_wait(epollfd, eventReady, EVENT_READY_CNT, -1);
        if (ready == -1) {
            if (errno != EINTR) {
                perror("epoll_wait error");
            }
            exit(3);
        }

        LOG_PRINT("find new event...\n");
        for (int i = 0; i < ready; i++) {
            int fdCurrent = eventReady[i].data.fd;

            //新连接
            if (fdCurrent == listenSockfd) {
                threadpool.push_task([=](int listenSockfd) {
                    struct sockaddr_in client_addr; //返回链接客户端地址信息, 含IP地址和端口号
                    socklen_t client_addr_len = sizeof(client_addr);

                    while (1) {
                        int commfd = accept(listenSockfd, (struct sockaddr *)&client_addr, &client_addr_len);
                        if (commfd == -1) {
                            break;
                        }

#ifdef LOG_ENABLE
                        //打印一下连接上的客户端的信息
                        printf("commfd = %d, ", commfd);
                        char ip_str[25];
                        printf("ip = %s, port = %d connected\n",
                                inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ip_str, sizeof(ip_str)), 
                                ntohs(client_addr.sin_port));
#endif

                        //把这个通信套接字设置成非阻塞
                        int flag = fcntl(commfd, F_GETFL);
                        flag |= O_NONBLOCK;
                        fcntl(commfd, F_SETFL, flag);

                        //把这个通信套接字加到红黑树上, 设置成边沿触发方式
                        struct epoll_event event;
                        event.events = EPOLLIN | EPOLLET;
                        event.data.fd = commfd;
                        int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, commfd, &event);
                        if (ret == -1) {
                            perror("epoll_ctl error");
                            exit(2);
                        }
                    }

                    LOG_PRINT("accept done...\n");
                }, listenSockfd);
            }
            //通信
            else {
                threadpool.push_task([=](int commSockfd) {
                    //读数据
                    int len = 0;
                    char buf[1024 * 10];

                    LOG_PRINT("start read...\n");
                    while (1) {

                        int ret = read(commSockfd, buf, 1024);
                        if (ret == -1) {
                            break;
                        }

                        //对端关闭, 进入close_wait状态
                        if (ret == 0) {

                            //删除该监听结点
                            ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, commSockfd, NULL);
                            if (ret == -1) {
                                perror("epoll_ctl error");
                                exit(2);
                            }
                            LOG_PRINT("connect closed\n");
                            close(commSockfd);
                            return;
                        }
                        len += ret;
                    }

                    buf[len] = 0;
                    //测试
                    LOG_PRINT("read content: \n%s", buf);

                    //http解析
                    HttpMsgParser parser;
                    parser.tryDecode(buf, len);

#ifdef LOG_ENABLE
                    unordered_map<string, string> header = parser.getHeaders();
                    printf("headers:\n");
                    for (auto it : header) {
                        printf("%s ============ %s\n", it.first.c_str(), it.second.c_str());
                    }
#endif
                    const string& method = parser.getMethod();
                    const string& url = parser.getUrl();

                    //转码 将不能识别的中文乱码 -> 中文
                    //解码 %23 %34 %5f
                    char path[256];
                    strcpy(path, url.c_str());
                    decode_str(path, path);

                    if (method == "GET") {
                        const char *filepath;
                        if (strcmp(url.c_str(), "/") == 0) {
                            filepath = ".";
                        }
                        else {
                            filepath = path + 1;
                        }

                        LOG_PRINT("got [%s] request from browser...\n\n", filepath);
                        doHttpRequest(commSockfd, filepath);
                    }
                    else {
                        LOG_PRINT("unrecognized request\n");
                    }
                }, fdCurrent);
            }
        }
    }
}

/* 构造http回复报文头 */
int HttpServer::httpBuildResponseMsgHeader(int no, const char *describe, const char *type, int len, char *out_buffer)
{
    int cnt = 0;

    cnt = sprintf(out_buffer, "http/1.1 %d %s\r\n", no, describe);        //状态行
    cnt += sprintf(out_buffer + cnt, "Content-Type:%s\r\n", type);        //类型
    cnt += sprintf(out_buffer + cnt, "Content-Length:%d\r\n", len);       //长度
    cnt += sprintf(out_buffer + cnt, "\r\n");                             //空行

    return cnt;
}

/* 构造http错误报文 */
int HttpServer::httpBuildErrorMsg(int no, const char *describe, const char *text, char *out_buffer)
{
    int cnt = 0;

    cnt = sprintf(out_buffer, "%s %d %s\r\n", "HTTP/1.1", no, describe);
    cnt += sprintf(out_buffer + cnt, "Content-Type:%s\r\n", "text/html");
    cnt += sprintf(out_buffer + cnt, "Content-Length:%d\r\n", -1);
    cnt += sprintf(out_buffer + cnt, "Connection: close\r\n");
    cnt += sprintf(out_buffer + cnt, "\r\n");

    cnt += sprintf(out_buffer + cnt, "<html><head><title>%d %s</title></head>\n", no, describe);
    cnt += sprintf(out_buffer + cnt, "<body bgcolor=\"#cc99cc\"><h2 align=\"center\">%d %s</h4>\n", no, describe);
    cnt += sprintf(out_buffer + cnt, "%s\n", text);
    cnt += sprintf(out_buffer + cnt, "<hr>\n</body>\n</html>\n");

    return cnt;
}

/* 发送错误信息 */
void HttpServer::doSendError(int commSockfd, int no, const char *describe, const char *text)
{
    char reply_buffer[1024 * 10];
    int len = httpBuildErrorMsg(no, describe, text, reply_buffer);
    write(commSockfd, reply_buffer, len);
}

/* 发送文件内容 */
void HttpServer::doSendFile(int commSockfd, const char *filepath)
{
    //打开文件
    int fd = open(filepath, O_RDONLY);
    if(fd == -1) {
        //回发浏览器 404 错误页面
        doSendError(commSockfd, 404, "Not Found", "No such file or directory");
        return;
    }

    //循环读文件
    char buf[4096];
    int len = 0;
    while ( (len = read(fd, buf, sizeof(buf))) > 0 ) {
        write(commSockfd, buf, len);
    }
    if (len == -1) {
        perror("read file error");
        exit(1);
    }

    close(fd);
}

/* 发送目录内容 */
void HttpServer::doSendDir(int commSockfd, const char *dirname)
{
    char reply_buffer[1024 * 10];
    int len = get_entry(dirname, reply_buffer);
    write(commSockfd, reply_buffer, len);
}

/* 处理http请求, 判断文件是否存在, 回发 */
void HttpServer::doHttpRequest(int commSockfd, const char *filepath)
{
    char reply_buffer[1024 * 10];
    struct stat sbuf;

    //判断 文件/目录 是否存在
    int ret = stat(filepath, &sbuf);
    if (ret != 0) {
        //回发浏览器 404 错误页面
        doSendError(commSockfd, 404, "Not Found", "No such file or directory");
        return;
    }

    //文件
    if (S_ISREG(sbuf.st_mode)) {

        //构造回发报文头, 写数据给客户端
        int len = httpBuildResponseMsgHeader(200, "OK", get_file_type(filepath), sbuf.st_size, reply_buffer);
        write(commSockfd, reply_buffer, len);

        //发送文件内容
        doSendFile(commSockfd, filepath);
    }
    //目录
    else if (S_ISDIR(sbuf.st_mode)) {

        //构造回发报文头, 写数据给客户端
        int len = httpBuildResponseMsgHeader(200, "OK", get_file_type(".html"), -1, reply_buffer);
        write(commSockfd, reply_buffer, len);

        //发送目录信息
        doSendDir(commSockfd, filepath);
    }
}

