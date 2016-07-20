#ifndef _LOTUS_SOCKET_H
#define _LOTUS_SOCKET_H

#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>

//#define SERV_PORT 9998          /* server port */
#define BACKLOG   100             /* max connections in the listening queue */
#define EPOLL_SIZE 10000          /* max number of fd that epoll listens */ 
#define EPOLL_EVENTS_MAX 10000    /* max number of events */
#define EPOLL_TIMEOUT -1          /* epoll's timeout */

int make_server_socket(int port);
int set_nonblock(int sockfd);

#endif
