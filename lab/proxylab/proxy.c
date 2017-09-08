#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define TRUE 1
#define FALSE 0

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void proxy_begin(int connfd);
void client_error(int fd, char *cause, char *errnum, char *shormsg, char *longmsg);

int main(int argc, char *argv[])
{
	int listenfd, connfd;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	char client_hostname[MAXLINE], client_port[MAXLINE];

	if(argc != 2){
		fprintf(stderr, "Usage : %s <port number>\n", argv[0]);
		exit(0);
	}

	listenfd = Open_listenfd(argv[1]);
	while(TRUE){
		clientlen = sizeof(struct sockaddr_storage);
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
		Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE,
				client_port, MAXLINE, 0);
		printf("Connected to (%s, %s)\n", client_hostname, client_port);
		proxy_begin(connfd);
		close(connfd);
	}
	return 0;
}

void proxy_begin(int connfd){
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char filename[MAXLINE];
	rio_t rio;

	Rio_readinitb(&rio, connfd);
	if(!Rio_readlineb(&rio, buf, MAXLINE)){
		client_error(connfd, method, "404", "Invalid Request",
				"Your request is not supported by proxy");
		return;
	}

	printf("Server received %s\n", buf);
	sscanf(buf, "%s %s %s", method, uri, version);
	if(strcasecmp(method, "GET")){
		client_error(connfd, method, "501", "Not Implemented",
				"Proxy does not implement this method at this time");
		return;
	}

	//read request
}

void read_request_headers(rio_t *rp) {
	char buf[MAXLINE];

	Rio_readlineb(rp, buf, MAXLINE);
	printf("%s", buf);
	while(strcmp(buf,"\r\n")) {
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
	}
	return ;
}

int parse_uri(char *uri, char *filename, char *cgiargs){
	char *ptr;
	if(!strstr(uri, "cgi-bin")) {
		//static content return 1
		strcpy(filename, ".");
		strcat(filename, uri);
		if(uri[strlen(uri)-1] == '/')
			strcat(filename, "home.html");
		return 1;
	}else {
		//dynamic content return 0
		if(ptr) {
			strcpy(cgiargs, ptr+1);
			*ptr = '\0';
		}else {
			strcpy(cgiargs, "");
		}
		strcpy(filename, ".");
		strcat(filename, uri);
		return 0;
	}
}

void client_error(int fd, char *cause, char *errnum, char *shormsg, char *longmsg){
	char buf[MAXLINE], body[MAXBUF];

	//build HTTP response body
	sprintf(body, "<html><title>Proxy Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n",body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shormsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Proxy Web Server</em>\r\n", body);


	//print HTTP response
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shormsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
	Rio_writen(fd, body, strlen(body));
}
