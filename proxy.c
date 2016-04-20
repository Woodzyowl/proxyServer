#include <stdio.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
//static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";


#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include "csapp.h"
#include <time.h>

FILE *fp;

void* doit(void* fd);
void read_requesthdrs(rio_t *rp);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);

/* Destructively modify string to be upper case */
void upper_case(char *s)
{
  while (*s) {
    *s = toupper(*s);
    s++;
  }
}

void* proxy(void* connfdp) 
{
    size_t n; 
    char buf[MAXLINE]; 
    rio_t rio;

    rio_readinitb(&rio, *((int *) connfdp));
    while((n = rio_readlineb(&rio, buf, MAXLINE)) != 0) {
	printf("server received %d bytes\ndata: %s", (int) n, buf);
	upper_case(buf);
	rio_writen(*((int *) connfdp), buf, n);
    }
    printf("Connection closed\n");
    Close(*((int *) connfdp));

    return NULL;
}

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void* doit(void* fd) 
{
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE];
    rio_t rio;

    /* Read request line and headers */
    rio_readinitb(&rio, *((int *)fd));
    if (!rio_readlineb(&rio, buf, MAXLINE))  //line:netp:doit:readrequest
        return NULL;
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);       //line:netp:doit:parserequest
    if (strcasecmp(method, "GET")) {                     //line:netp:doit:beginrequesterr
        clienterror(*((int *)fd), method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return NULL;
    }                                                    //line:netp:doit:endrequesterr
    read_requesthdrs(&rio);                              //line:netp:doit:readrequesthdrs

    if (stat(filename, &sbuf) < 0) {                     //line:netp:doit:beginnotfound
	clienterror(*((int *)fd), filename, "404", "Not found",
		    "Tiny couldn't find this file");
	return NULL;
    }                                                    //line:netp:doit:endnotfound

    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { //line:netp:doit:readable
        clienterror(*((int *)fd), filename, "403", "Forbidden",
		"Tiny couldn't read the file");
        return NULL;
    }
    return NULL;
    //serve_static(fd, filename, sbuf.st_size);        //line:netp:doit:servestatic
}
/* $end doit */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    rio_writen(fd, buf, strlen(buf));
    rio_writen(fd, body, strlen(body));
}
/* $end clienterror */

/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];

    rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {          //line:netp:readhdrs:checkterm
	rio_readlineb(rp, buf, MAXLINE);
	printf("%s", buf);
    }
}
/* $end read_requesthdrs */


/********* Start of Logging Functions *********/
/* 
Creates a log file if one does not exist,
logs the server's activation time
*/
void logcreate () {
	fp = fopen("LOG.txt", "a+");	//creates file or opens for appending

	time_t timer;
  char buffer[29];
	struct tm* tm_info;

  time(&timer);
  tm_info = localtime(&timer);

  strftime(buffer, 29, "[%Y:%m:%d %H:%M:%S] ", tm_info);

	if (fputs(buffer, fp) != 0) {
		printf("Error writing time of activation to LOG.txt\n");
	}
	if (fputs("Server Activated \n", fp) != 0) {
		printf("Error writing activation message to LOG.txt\n");
	}
}

/* 
Adds the client's request to the log file
*/
/* NOTE: I still need to figure out how to add weekdays */ 
void logrequest (char *browserIP, char *URL, char *numberofbytes) {
	time_t timer;
  char buffer[29];
	struct tm* tm_info;

  time(&timer);
  tm_info = localtime(&timer);

  strftime(buffer, 29, "[%Y:%m:%d %H:%M:%S] ", tm_info);

	if (fputs(buffer, fp) != 0) {
		printf("Error writing time of request to LOG.txt\n");
	}
	if (fputs(browserIP, fp) != 0) {
		printf("Error writing browserIP request to LOG.txt\n");
	}
	fputs(" ", fp); //adds a space between browserIP and URL
	if (fputs(URL, fp) != 0) {
		printf("Error writing URL request to LOG.txt\n");
	}
	fputs(" ", fp); //adds a space between URL and size
	if (fputs(numberofbytes, fp) != 0) {
		printf("Error writing size request to LOG.txt\n");
	}
	fputs("\n", fp); //adds a new line after size, concluding the request
}

/* 
Logs the time of the server closing 
*/
void logclose () {
	time_t timer;
  char buffer[29];
	struct tm* tm_info;

  time(&timer);
  tm_info = localtime(&timer);

  strftime(buffer, 29, "[%Y:%m:%d %H:%M:%S] ", tm_info);

	if (fputs(buffer, fp) != 0) {
		printf("Error writing time of closing to LOG.txt\n");
	}
	if (fputs("Server Closed \n", fp) != 0) {
		printf("Error writing closure message to LOG.txt\n");
	}
	if (fclose(fp) != 0) {
		printf("Error closing LOG.txt\n");
	}
}

/********* End of Logging Functions *********/ 

int main(int argc, char **argv) 
{
    int listenfd;
    int *connfdp;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    struct hostent *hp;
    char *haddrp;
    unsigned short client_port;
    pthread_t *tid=(pthread_t *)malloc(sizeof(pthread_t));
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
		logcreate(); /* OPENS LOG FILE */
    while (1) {
	clientlen = sizeof(clientaddr);
        connfdp=(int *) malloc(sizeof(int));
	*connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
	/* determine the domain name and IP address of the client */
	hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
		sizeof(clientaddr.sin_addr.s_addr), AF_INET);
	haddrp = inet_ntoa(clientaddr.sin_addr);
	client_port = ntohs(clientaddr.sin_port);
	printf("server connected to %s (%s), network port %u\n",
	       hp->h_name, haddrp, clientaddr.sin_port);
	printf("server connected to %s (%s), local port %u\n",
               hp->h_name, haddrp, client_port);
        pthread_create(tid,NULL,doit,connfdp);
    }
		logclose(); /* CLOSES LOG FILE */
    exit(0);
}
/* $end echoserverimain */
