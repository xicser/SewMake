#ifndef __HTTP_MSG__
#define __HTTP_MSG__

int http_get_line(int line_num, const char *data_in, char *data_out);
int http_build_response_msg_header(int no, const char *describe, const char *type, int len, char *out_buffer);
int http_build_error_msg(int no, const char *describe, const char *text, char *out_buffer);

#endif

