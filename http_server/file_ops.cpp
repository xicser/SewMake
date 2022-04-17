#include "file_ops.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include "codec.h"

/* 通过文件名获取文件的类型 */
const char *get_file_type(const char *name)
{
    const char *suffix;

    // 自右向左查找'.'字符, 如不存在返回NULL
    suffix = strrchr(name, '.');   
    if (suffix == NULL)
        return "text/plain; charset=utf-8";

    if (strcmp(suffix, ".html") == 0 || strcmp(suffix, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(suffix, ".jpg") == 0 || strcmp(suffix, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(suffix, ".gif") == 0)
        return "image/gif";
    if (strcmp(suffix, ".png") == 0)
        return "image/png";
    if (strcmp(suffix, ".css") == 0)
        return "text/css";
    if (strcmp(suffix, ".au") == 0)
        return "audio/basic";
    if (strcmp(suffix, ".wav") == 0)
        return "audio/wav";
    if (strcmp(suffix, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(suffix, ".mov") == 0 || strcmp(suffix, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(suffix, ".mpeg") == 0 || strcmp(suffix, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(suffix, ".vrml") == 0 || strcmp(suffix, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(suffix, ".midi") == 0 || strcmp(suffix, ".mid") == 0)
        return "audio/midi";
    if (strcmp(suffix, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(suffix, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(suffix, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

/* 获取某一个目录下的文件和目录, 并拼接一个html文件到out_buffer */
int get_entry(const char *path, char *out_buffer)
{
    int is_root = 0;
    DIR *dp;
    struct dirent *dentry;
    char tmpstr[4096];
    char enstr[4096];

    dp = opendir(path);
    if (dp == NULL) {
        perror("opendir error");
        return -1;
    }

    //判断是否在资源根目录
    if (strcmp(path, ".") == 0) {
        is_root = 1;
    }

    int cnt = sprintf(out_buffer, "<html><head><title>目录名: %s</title></head>", path);
    cnt += sprintf(out_buffer + cnt, "<body><h1>当前目录: %s</h1><table>", path);

    while (1) {
        dentry = readdir(dp);
        if (dentry == NULL) {
            break;
        }

        //拼接文件的完整路径
        sprintf(tmpstr, "%s/%s", path, dentry->d_name);
        struct stat st;
        stat(tmpstr, &st);

        //编码生成 %E5 %A7 之类的东西
        encode_str(enstr, sizeof(enstr), dentry->d_name);

        //目录
        if (dentry->d_type == DT_DIR) {

            if (strcmp(dentry->d_name, ".") == 0) {
                if (is_root == 0) {
                    cnt += sprintf(out_buffer + cnt,
                        "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>",
                        ".", ".", st.st_size);
                }
            }
            else if (strcmp(dentry->d_name, "..") == 0) {
                if (is_root == 0) {
                    cnt += sprintf(out_buffer + cnt,
                        "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>",
                        "..", "..", st.st_size);
                }
            }
            else {
                cnt += sprintf(out_buffer + cnt,
                    "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>",
                    enstr, dentry->d_name, st.st_size);
            }
        }
        //文件
        else {
            cnt += sprintf(out_buffer + cnt,
                    "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
                    enstr, dentry->d_name, st.st_size);
        }
    }

    cnt += sprintf(out_buffer + cnt, "</table></body></html>");

    closedir(dp);

    return cnt;
}


