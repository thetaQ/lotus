#include"l_socket.h"

/*
* Description:
*    Make a socket file descriptor and bind it to a internet address structure.
*    Then we listen to the descriptor.
* Param:
*    port: the port of the web server
* Return:
*    On success sockfd is returned, else -1 is returned
*/
int make_server_socket(int port)
{
	struct sockaddr_in servaddr;
	int    sockfd;
	int    opt = 1;

	/* create socket */
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return -1;
	}
	
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	/* build our address */
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;                            /* protocol */
	servaddr.sin_port = htons(port);                          /* port of the server */
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);             /* local address */
	
	/* bind */
	if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind");
		return -1;
	}
	

	/* listen the socket */
	if(listen(sockfd, BACKLOG) < 0)
	{
		perror("listen");
		return -1;
	}
	
	return sockfd;
}



/*
* Dsecription:
*    Set the socket file descriptor's attribution of non-blocking
* Param:
*    sockfd: socket file descriptor
* Return:
*    On success, 0 is retured. Else -1 is returned.
*/
int set_nonblock(int sockfd)
{
	int flag;
	flag = fcntl(sockfd, F_GETFL, 0);
	flag |= O_NONBLOCK;
	if(fcntl(sockfd, F_SETFL, flag) == -1)
		return -1;
	return 0;
}
