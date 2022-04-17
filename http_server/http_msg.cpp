#include "http_msg.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* 获取一行以 \r\n 结尾的数据  */
int http_get_line(int line_num, const char *data_in, char *data_out)
{
    int cur_line = 1;
    int i = 0, j = 0;

    while (1) {

        if (data_in[i] == 0) {
            data_out[0] = 0;
            return -1;
        }

        if (data_in[i] == '\r' && data_in[i + 1] == '\n') {
            if (cur_line == line_num) {
                data_out[j] = 0;
                return j;
            } else {
                cur_line++;
                j = 0;
                i += 2;
            }
        } else {
            data_out[j] = data_in[i];
            i++;
            j++;
        }
    }
}

/* 构造http回复报文头 */
int http_build_response_msg_header(int no, const char *describe, const char *type, int len, char *out_buffer)
{
    int cnt = 0;

    cnt = sprintf(out_buffer, "http/1.1 %d %s\r\n", no, describe);        //状态行
    cnt += sprintf(out_buffer + cnt, "Content-Type:%s\r\n", type);        //类型
    cnt += sprintf(out_buffer + cnt, "Content-Length:%d\r\n", len);       //长度
    cnt += sprintf(out_buffer + cnt, "\r\n");                             //空行

    return cnt;
}

/* 构造http错误报文 */
int http_build_error_msg(int no, const char *describe, const char *text, char *out_buffer)
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

