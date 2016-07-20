/**
* lotus.c
* 	The main program of the web server using epoll model 
* 	and thread pool in order to optimize it's performance.
* Copyright: TangQi LiXiang
*/

#include<stdio.h>
#include<stdlib.h>
#include<sys/epoll.h>
#include<signal.h>
#include<time.h>
#include<errno.h>
#include<assert.h>
#include"l_socket.h"
#include"l_global.h"
#include"l_http.h"
#include"SAK_threadpool.h"

#define THREADPOOL_THREADS 64

int request_count = 0;

int main(int argc, char ** argv)
{
	int listen_fd, epfd;
	int epoll_events_count;
	struct epoll_event ev, events[EPOLL_EVENTS_MAX];
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	int i, serv_port;
	SAK_threadpool_t * tp_handler = NULL;

#if DEBUG
	clock_t tstart;
#endif

	if(argc != 2)
	{
		perror("usage: ./lotus <port>");
		exit(-1);
	}

	serv_port = atoi(argv[1]);
	
	bzero(&clientaddr, sizeof(clientaddr));
	addrlen = sizeof(struct sockaddr_in);

	/*********************** thread pool ***************************/
	tp_handler = threadpool_create(THREADPOOL_THREADS);
	assert(tp_handler != NULL);
	printf("thread pool is created\n");

	/************************** socket *****************************/
	/* make server socket */
	ERR2(listen_fd, make_server_socket(serv_port));
	printf("web server socket is made\n");

	/**************************** epoll ****************************/
	/* create epoll */
	ERR2(epfd, epoll_create(EPOLL_SIZE));
	ev.data.fd = listen_fd;
	ev.events = EPOLLIN|EPOLLET;

	ERR(set_nonblock(listen_fd));
	
	/* add listen_fd to epoll */
	ERR(epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev));
	printf("epoll module is created\n");

	
	/* ignore pipe broken signal */	
	signal(SIGPIPE, SIG_IGN);

	/************************* main ********************************/
	for(;;)
	{
		/* wait for changes of fd set */
		ERR2(epoll_events_count, epoll_wait(epfd, events, EPOLL_EVENTS_MAX, EPOLL_TIMEOUT));

#if DEBUG
		tstart = clock();
#endif
		for(i = 0; i < epoll_events_count; i++)
		{

#if DEBUG
			printf("events[%d].data.fd == %d\n", i, events[i].data.fd);
#endif
			if((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)))
			{
				fprintf(stderr, "ERROR: epoll error\n");
				close(events[i].data.fd);
				continue;
			}
			else if(events[i].data.fd == listen_fd)
			{
#if DEBUG
				printf("Is listen_fd = %d\n", listen_fd);
#endif
				/* a new connection is coming, we accept it 
				   and add the new connection fd to epoll set  */
				while(1)
				{
					int connect_fd;
					char clientaddr_buff[20] = {0};
					connect_fd = accept(listen_fd, (struct sockaddr *)&clientaddr, &addrlen);

					if(connect_fd == -1)
					{
						if((errno == EAGAIN) || (errno == EWOULDBLOCK))
						{
							/* We have process all incoming connection */
							break;
						}
						else
						{
							perror("accept");
							break;
						}
					}
					inet_ntop(AF_INET, &clientaddr.sin_addr, clientaddr_buff, sizeof(clientaddr_buff)), 
					printf("Client [%s : %d] is connected\n", clientaddr_buff, ntohs(clientaddr.sin_port));

					ERR(set_nonblock(connect_fd));
					ev.data.fd = connect_fd;

#if DEBUG
					printf("Get and put connect_fd = %d into epoll\n", connect_fd);
#endif
					ev.events = EPOLLIN|EPOLLET;
					ERR(epoll_ctl(epfd, EPOLL_CTL_ADD, connect_fd, &ev));
				}
				continue;
			}
			else
			{
				/* Can read */

				/************************************************/
				/*  Put the task into thread pool's work queue  */             
				/************************************************/
#if DEBUG
				printf("Is connect_fd = %d\n", events[i].data.fd);
#endif
				threadpool_put(tp_handler, process_request, (void *)events[i].data.fd);
				request_count ++;
				printf("Has handle %d requests\n", request_count);
				//ERR(epoll_ctl(epfd, EPOLL_CTL_DEL, ev.data.fd, &ev));
			}

		}
#if DEBUG
		printf("Statistics: %d events handled at %2.f seconds(s)\n", epoll_events_count, (double)(clock() - tstart)/CLOCKS_PER_SEC);
#endif
	}
	
	close(listen_fd);
	close(epfd);
	threadpool_destroy(tp_handler);
	return 0;
}

