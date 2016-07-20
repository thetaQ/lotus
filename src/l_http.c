#include<stdio.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/stat.h>
#include<time.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include"l_http.h"
#include"l_global.h"


int process_request(void * arg)
{
	char   recv_buff[RECV_BUFF_SIZE];
	char   uri_buff[URI_BUFF_SIZE+1];

	int    recv_len;
	int    uri_state;
	int    fd = (int)arg;	
#if DEBUG
	printf("handle client: %d\n", fd);
#endif

	memset(recv_buff, '\0', sizeof(recv_buff));            /* read data from fd */

	recv_len = recv(fd, recv_buff, RECV_BUFF_SIZE, 0);

#if DEBUG
	printf("recv_len = %d\n", recv_len);
#endif

	if(recv_len > 0)
	{
		/*  receive some data */
		if(is_http_request(recv_buff))
		{
			/* This part is the main process part */

			memset(uri_buff, '\0', sizeof(uri_buff));
			if(get_uri(recv_buff, uri_buff) == NULL)
			{
				uri_state = URI_TOO_LONG;
			}
			else
			{
				uri_state = get_uri_state(uri_buff);
			}

			switch(uri_state)
			{
				case IS_A_DIR:
					do_ls(fd, uri_buff);                                   /* ls */
					break;
				case FILE_OK:
					do_cat(fd, uri_buff);                                  /* cat */
					break;
				case FILE_NOT_FOUND:
					do_error(fd, FILE_NOT_FOUND, "404 Not Found", \
						"The item you requested is not found\r\n");
					break;
				case FILE_FORBIDEN:
					do_error(fd, FILE_FORBIDEN, "403 Forbidden", \
						"The item you requseted is forbidden\r\n");
					break;
				case URI_TOO_LONG:
					do_error(fd, URI_TOO_LONG, "414 Request-URI Too Long", \
						"The requested uri is too long\r\n");
					break;
				default:
					break;
			}		
		}
		else
		{
			do_error(fd, NOT_IMPLEMENTED, "501 Not Implemented", \
					"The command is not yet implemented\r\n");
			return -1;
		}
	}
	else
	{
		close(fd);
	}
	return 0;
}



/*
*  Use:
*    Test whether the request is a http request. If the 
*    request is start with "GET", then we say that it is
*    a http request
*  Param:
*    request is a pointer of the request message
*  Return:
*    1 is returned if it is a http request. 0 is returned
*    if it is not a http request
*
*/
int is_http_request(char * request)
{
	char buf[4];
	strncpy(buf, request, 3);
	buf[3] = '\0';

#if DEBUG
	printf("==========================================================\n");	
	printf("%s\n", request);
	printf("==========================================================\n");
#endif

	if(strncmp(buf, "GET", 3) == 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}



/*
*	Use:
*     Get uri from request message
*   Param:
*     request is the request message, uri_buff is a buffer to store URI
*   Return:
*     NULL if URI is too long or return URI
*/
char * get_uri(char * request, char * uri)
{
	int index = 0;
	int start = 0;
	/* get the first '/' in the request line */
	while((request[index] != '/') && (request[index] != '\0'))
	{
		index ++;
	}
	start = index;
	
	/* when get a space, then stop */
	while(((index - start) < URI_BUFF_SIZE) && (request[index] != ' ') && (request[index] != '\0'))
	{
		index ++;
	}
	
	if(index - start > URI_BUFF_SIZE)
	{
		fprintf(stderr, "Request URI is too long.");
		return NULL;
	}

	/* if the uri has no size or it is just a '/', we can return index.html */
	if((index - start == 0) || ((index - start == 1)&&(request[index - 1] == '/')))
	{
		sscanf("index.html", "%s", uri);
		uri[10] = '\0';
#if DEBUG
		printf("%s\n", uri);
#endif
		return uri;
	}

	/* if the last charactor is '/', we need to remove it */
	if(request[index - 1] == '/' && request[index] == ' ')
	{	
		strncpy(uri, request + start + 1, index - start - 2);
		return uri;
	}

	strncpy(uri, request + start + 1, index - start - 1);
	return uri;
}



/*
* Use: 
*   Test whether the file is exist or can be read
* Param:
*   uri is the file uri that we have got
* Return:
*   URI state code
*/
int get_uri_state(char * uri)
{
	if(access(uri, F_OK) == -1)
	{
		fprintf(stderr, "File: %s not found\n", uri);
		return FILE_NOT_FOUND;
	}
	if(access(uri, R_OK) == -1)
	{
		fprintf(stderr, "File: %s can not read\n", uri);
		return FILE_FORBIDEN;
	}
	if(isadir(uri))              /* if it is a dir */
	{
		return IS_A_DIR;
	}
	return FILE_OK;
}



/*
* Use:
*   Test whether file f is a dir
* Param:
*   f represents file
* Return:
*   1 is returned if it is a dir. 0 is returned if it is not a dir
*
*/
int isadir(char * f)
{
	struct stat info;
	return (stat(f, &info) != -1 && S_ISDIR(info.st_mode));
}


/*
* Use:
*   Get the file type by finding the last '.' in the string
* Param:
*	uri is the requested URI by client
* Return:
*   NULL is returned when there is no type. Otherwise we return
*   a string representing the type
*/
char * get_content_type(char * uri)
{
	int len = strlen(uri);
	int dot = len - 1;
	
	while(dot >= 0 && uri[dot] != '.')
	{
		dot --;
	}
	
	if(dot == 0)
	{
		return NULL;                    /* '.' is the first character in the uri string. it's a bad request */
	}
	else if(dot < 0)
	{
		return "text/html";             /* GET /   default type is text/html */
	}
	else
	{	
		char * type = uri + dot + 1;
		if(!strncmp(type, "html", 4) || !strncmp(type, "HTML", 4))
		{
			return "text/html";
		}
		else if(!strncmp(type, "htm", 3) || !strncmp(type, "HTML", 3))
		{
			return "text/html";
		}
		else if(!strncmp(type, "jpeg", 4) || !strncmp(type, "JPEG", 4))
		{
			return "image/jpeg";
		}
		else if(!strncmp(type, "jpg", 3) || !strncmp(type, "JPG", 3))
		{
			return "image/jpeg";
		}
		else if(!strncmp(type, "gif", 3) || !strncmp(type, "GIF", 3))
		{
			return "image/gif";
		}		
		else if(!strncmp(type, "css", 3) || !strncmp(type, "CSS", 3))
		{
			return "text/css";
		}	
		else if(!strncmp(type, "png", 3) || !strncmp(type, "PNG", 3))
		{
			return "image/png";
		}
		else if(!strncmp(type, "txt", 3) || !strncmp(type, "TXT", 3))
		{	
			return "text/plain";
		}
		else if(!strncmp(type, "js", 2) || !strncmp(type, "JS", 2))
		{
			return "text/javascript";
		}
		else
		{
			return NULL;
		}
	}
}


/*
*  Use:
*    Build a http response header(OK)
*  Param:
*    fp is a file descriptor pointing to the socket
*    state_code is the response state code
*    state_code_str is a string to explain the state_code
*    content_type is the request's file type
*  Return:
*    void
*/
void header(FILE * fp, int state_code, char * state_code_str, char * content_type)
{
	time_t timep;
	time(&timep);
	fprintf(fp, "HTTP/1.0 %d %s\r\n", state_code, state_code_str);
	fprintf(fp, "Server: Lotus/0.3\r\n");
	fprintf(fp, "Date: %s", asctime(gmtime(&timep)));
	if(state_code == FILE_OK && content_type)
	{
		fprintf(fp, "Content-type: %s; charset=UTF-8\r\n", content_type);
	}
	fprintf(fp, "\r\n");
}



/*
* Use:
*   Open a file and write it to socket fd
* Param:
*   fd is a socket file descriptor
*   filename is the file required by client
* Return:
*	void
*/
void do_cat(int fd, char * filename)
{
	FILE   * fpfile;   /* the file we want to cat */
	FILE   * fp;
	int    c; 

	fp = fdopen(fd, "w");
	/* open file */
	fpfile = fopen(filename, "r");

	if(fpfile != NULL && fp != NULL)
	{
		header(fp, FILE_OK, "OK", get_content_type(filename));    /* build response header */

		while((c = getc(fpfile)) != EOF)
		{
			putc(c, fp);
		}
	}

#if DEBUG
	printf("do_cat is done!\n");
#endif
	fclose(fp);
//  fflush(fp);
	fclose(fpfile);
}



/*
* Use:
*   list the file in the dir
* Param:
*   fd is the socket dercriptor and dirname is the name of a dir
* Return:
*   void
*/
void do_ls(int fd, char * dirname)
{
	FILE  * fp;

	fp = fdopen(fd, "w");
	header(fp, FILE_OK, "OK", "text/plain");
	fflush(fp);

	if(!fork())
	{
		dup2(fd, 1);
		dup2(fd, 2);
		close(fd);
		execlp("ls", "ls", "-l", dirname, NULL);
	}
	fflush(fp);
	fclose(fp);
}


/*
*  Use:
*    process error and make a response
*  Param:
*    fd is the socket descriptor
	 uri_state is the state code
	 uri_state_str is a explain to state code
	 response_str is the content to response
*  Return:
*    void
*/
void do_error(int fd, int uri_state, char * uri_state_str, char * response_str)
{
	FILE  * fp;

	fp = fdopen(fd, "w");
	
	header(fp, uri_state, uri_state_str, "text/plain");
	
	fprintf(fp, "%s", response_str);
//	fflush(fp);
	fclose(fp);
}

