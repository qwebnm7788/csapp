#include "csapp.h"

int main() {
	char *buf, *p;
	char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];

	int n1 = 0, n2 = 0;

	if((buf = getenv("QUERY_STRING")) != NULL) {
		p = strchr(buf, '&');
		*p = '\0';
		strcpy(arg1, buf);
		strcpy(arg2, p+1);

		n1 = atoi(arg1);
		n2 = atoi(arg2);
	}

	sprintf(content, "Welcome to checker!!");
	sprintf(content, "%sCHECK IF IT'S WORKING.\r\n", content);
	sprintf(content, "%s<p>CHECK IS : %d %d\r\n<\\p>", content, n1, n2);
	sprintf(content, "%sGOOD BYE 09:49\r\n", content);

	printf("Connection: close\r\n");
	printf("Content-length: %d\r\n", (int)strlen(content));
	printf("Content-type: text/html\r\n\r\n");
	printf("%s", content);
	fflush(stdout);

	exit(0);
}
