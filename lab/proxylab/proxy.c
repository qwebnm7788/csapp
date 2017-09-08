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
int parse_uri(char *uri, char *hostname, char *path, char *port);

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
	char serverHost[MAXLINE], serverPath[MAXLINE], serverPort[MAXLINE];
	rio_t rio, serverRio;

	Rio_readinitb(&rio, connfd);
	if(!Rio_readlineb(&rio, buf, MAXLINE)){
		client_error(connfd, method, "404", "Invalid Request",
				"Your request is not supported by proxy");
		return;
	}

	printf("Server received %s\n", buf);	//for debugging
	sscanf(buf, "%s %s %s", method, uri, version);

	if(strcasecmp(method, "GET")){
		client_error(connfd, method, "501", "Not Implemented",
				"Proxy does not implement this method at this time");
		return;
	}

	//parse_uri
	if(parse_uri(uri, serverHost, serverPath, serverPort) < 0) {
		client_error(connfd, method, "808", "Wrong URI",
				"This uri doesn't exist");
		printf("ERROR\n");
		return;
	}

//	debug
//	printf("ServerName : %s\n", serverHost);
//	printf("ServerPath : %s\n", serverPath);
//	printf("ServerPort : %s\n", serverPort);

	//connection to server
	int serverfd = Open_clientfd(serverHost, serverPort);

	Rio_readinitb(&serverRio, serverfd);

	//sending method uri version
	Rio_writen(serverfd, buf, strlen(buf));

	int n;
	while((n = Rio_readnb(&serverRio, buf, MAXLINE)) > 0){
		printf("%s\n", buf);
	}

}

void build_request_header(rio_t *rp, char *header, char *hostname) {
	char buf[MAXLINE];

	sprintf(header, "User-Agent: %s\r\n", user_agent_hdr);
	return;
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


//return -1 if it's fail to parse uri , return 0 if success
int parse_uri(char *uri, char *hostname, char *path, char *port){

	char *portStart, *portEnd, *hostStart, *hostEnd;
	int len;

	strcpy(port, "");
	strcpy(path, "");
	strcpy(hostname, "");

	//start with http://
	if(strncasecmp(uri, "http://", 7) != 0) {
		return -1;
	}

	hostStart  = uri + 7;
	portStart = index(hostStart, ':');

//	printf("portStart : %s\n", portStart);		debug

	if(portStart == NULL) {
		//if port is not specified
		strcat(port, "80");		//default port is 80
		hostEnd = index(hostStart, '/');
		//if not end with / character just until the end of uri
		if(hostEnd == NULL)
			hostEnd = uri + strlen(uri);
		portEnd = hostEnd;
	}else {
		portStart += 1;
		portEnd = index(portStart, '/');
//		printf("portEnd : %s\n", portEnd);		debug

		len = portEnd - portStart;
		strncat(port, portStart, len);
		hostEnd = hostStart + (portStart - hostStart);
	}


	//hostname setting
	len = hostEnd - hostStart;
	strncat(hostname, hostStart, len);
//	printf("hostname : %s\n", hostname);		debug


	//path setting
	strcat(path, portEnd);
//	printf("path : %s\n", path);				debug
	return 0;

}

void client_error(int fd, char *cause, char *errnum, char *shormsg, char *longmsg) {
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
