#include "http_server.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("usage: ./http_server [path]\n");
        exit(1);
    }

    //改变进程工作目录
    int ret = chdir(argv[1]);
    if (ret != 0) {
        perror("chdir error");  
        exit(2);
    }

	HttpServer server(argv[1]);
	server.run();

    return 0;
}

