#include "http_server.h"

/* 发送错误信息 */
void HttpServer::doSendError(struct bufferevent *bev, int no, const char *describe, const char *text)
{
    char reply_buffer[1024 * 10];
    int len = http_build_error_msg(no, describe, text, reply_buffer);
    bufferevent_write(bev, reply_buffer, len);
}

/* 发送文件内容 */
void HttpServer::doSendFile(struct bufferevent *bev, const char *filepath)
{
    //打开文件
    int fd = open(filepath, O_RDONLY);
    if(fd == -1) {
        //回发浏览器 404 错误页面
        doSendError(bev, 404, "Not Found", "No such file or directory");
        return;
    }

    //循环读文件
    char buf[4096];
    int len = 0;
    while ( (len = read(fd, buf, sizeof(buf))) > 0 ) {
        bufferevent_write(bev, buf, len);
    }
    if (len == -1) {
        perror("read file error");
        exit(1);
    }

    close(fd);
}

/* 发送目录内容 */
void HttpServer::doSendDir(struct bufferevent *bev, const char *dirname)
{
    char reply_buffer[1024 * 10];
    int len = get_entry(dirname, reply_buffer);
    bufferevent_write(bev, reply_buffer, len);
}

/* 处理http请求, 判断文件是否存在, 回发 */
void HttpServer::doHttpRequest(struct bufferevent *bev, const char *filepath)
{
    char reply_buffer[1024 * 10];
    struct stat sbuf;

    //判断 文件/目录 是否存在
    int ret = stat(filepath, &sbuf);
    if (ret != 0) {
        //回发浏览器 404 错误页面
        doSendError(bev, 404, "Not Found", "No such file or directory");
        return;
    }

    //文件
    if (S_ISREG(sbuf.st_mode)) {

        //构造回发报文头, 写数据给客户端
        int len = http_build_response_msg_header(200, "OK", get_file_type(filepath), sbuf.st_size, reply_buffer);
        bufferevent_write(bev, reply_buffer, len);

        //发送文件内容
        doSendFile(bev, filepath);
    }
    //目录
    else if (S_ISDIR(sbuf.st_mode)) {

        //构造回发报文头, 写数据给客户端
        int len = http_build_response_msg_header(200, "OK", get_file_type(".html"), -1, reply_buffer);
        bufferevent_write(bev, reply_buffer, len);

        //发送目录信息
        doSendDir(bev, filepath);
    }
}

/* bufferevent读回调 */
void HttpServer::buffevReadCallback(struct bufferevent *bev, void *arg)
{
    HttpMsgParser req;    //报文解析器
    char rec_buffer[1024 * 10];

    //这里一次读走所有数据
    int read = bufferevent_read(bev, rec_buffer, sizeof(rec_buffer));
    if (read == 0) {
        return;
    }
    rec_buffer[read] = 0;
    printf("%s\n", rec_buffer);

    req.tryDecode(rec_buffer, read);
    const string& method = req.getMethod();
    const string& url = req.getUrl();

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

        printf("got [%s] request from browser...\n\n", filepath);
        doHttpRequest(bev, filepath);
    }
    else {
        printf("unrecognized request\n");
    }
}

/* bufferevent事件回调 */
void HttpServer::buffevEventCallback(struct bufferevent *bev, short events, void *arg)
{
    if (events & BEV_EVENT_EOF) {
        printf("connection closed\n");
    }
    else if (events & BEV_EVENT_ERROR) {
        printf("other error\n");
    }

    bufferevent_free(bev);
    printf("bufferevent freed\n");
}

/* 监听回调函数 */
void HttpServer::listenerCallback(
        struct evconnlistener *listener,
        evutil_socket_t fd,
        struct sockaddr *addr,
        int len, void *ptr)
{
    struct event_base *base = (struct event_base *)ptr;

    //添加bufferevent事件
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    //给bufferevent设置回调函数
    bufferevent_setcb(bev, buffevReadCallback, NULL, buffevEventCallback, NULL);

    //开启bufferevent的读缓冲区
    bufferevent_enable(bev, EV_READ);
}

HttpServer::HttpServer(string path)
{
    this->workDir = path;
    this->port = 9527;
    this->backlog = 128;

    //服务器地址结构信息
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;             //IPv4
    server_addr.sin_port = htons(port);           //port
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //设置ip地址(本机所有网卡的ip)

    //创建event_base
    base = event_base_new();

    /* 监听器 */
    //创建套接字
    //绑定
    //接收连接请求
    //accept
    listener = evconnlistener_new_bind(base,                   //base
                                    listenerCallback,          //回调函数
                                    base,                      //回调函数参数
                                    LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, //关闭释放, 端口复用
                                    backlog,                   //listen函数第二个参数
                                    (struct sockaddr *)&server_addr,    //地址结构
                                    sizeof(server_addr));               //地址结构长度
}

/* 运行http服务器 */
void HttpServer::run(void)
{
    //启动循环监听
    event_base_dispatch(base);

    //释放
    evconnlistener_free(listener);
    event_base_free(base);
}


