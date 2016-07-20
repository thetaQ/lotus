#ifndef _LOTUS_HTTP_H
#define _LOTUS_HTTP_H


#define RECV_BUFF_SIZE 1024
#define URI_BUFF_SIZE  128

#define IS_A_DIR        1

#define FILE_OK         200
#define FILE_FORBIDEN   403
#define FILE_NOT_FOUND  404
#define FILE_TOO_LARGE  413
#define URI_TOO_LONG    414
#define NOT_IMPLEMENTED 501

int process_request(void * arg);
int is_http_request(char * request);
char * get_uri(char * request, char * uri);
int get_uri_state(char * uri);
int isadir(char * f);
char * get_content_type(char * uri);
void header(FILE * fp, int state_code, char * state_code_str, char * content_type);
void do_cat(int fd, char * filename);
void do_ls(int fd, char * dirname);
void do_error(int fd, int uri_state, char * uri_state_str, char * response_str);


#endif
