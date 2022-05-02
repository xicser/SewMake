#include "http_server.h"

int main(int argc, char *argv[])
{
    //改变进程工作目录
    int ret = chdir(argv[1]);
    if (ret != 0) {
        perror("chdir error");  
        exit(2);
    }

	HttpServer server;
	server.run();

    return 0;
}

