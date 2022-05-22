#include "file_ops.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

