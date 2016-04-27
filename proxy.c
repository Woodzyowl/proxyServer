/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS:  put your name(s) and e-mail addresses here
 *     John Hyatt hyattjp0@sewanee.edu
 *     Tyler Epps eppstd0@sewanee.edu
 * 
 * Provides a handy dandy proxy for logging web traffic
 */ 

#include <stdio.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400


#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include "csapp.h"
#include <time.h>

FILE *fp;

void* doit(void* fd);
int parse_uri(char *uri, char *hostname, char *pathname, int *port);
char *strcasestr(const char *haystack, const char *needle);
void logrequest (char *browserIP, char *URL, char *numberofbytes);

/*
 * doit - handle proxy request
 */
/* $begin doit */
void* doit(void* fd) 
{
    //struct stat sbuf;
    int servfd,clientfd=*((int *)fd);
    free(fd);
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE],
         hostname[MAXLINE],pathname [MAXLINE];
    int* port=(int *)malloc(sizeof(int));
    //char filename[MAXLINE];
    char response[MAX_OBJECT_SIZE];
    char request[MAX_OBJECT_SIZE];
    char temp_response[MAX_OBJECT_SIZE];
    int response_size,temp_response_size;
    rio_t rio, servrio;
    char portstr[MAXLINE];

    /* Read request line and headers */
    rio_readinitb(&rio, clientfd);
    rio_readlineb(&rio, buf, MAXLINE);
    strcat(request,buf);
    sscanf(buf, "%s %s %s", method, uri, version);       //line:netp:doit:parserequest
    parse_uri(uri, hostname, pathname, port);
    sprintf(portstr,"%d",*port);
    while(strcmp(buf, "\r\n")) {          //line:netp:readhdrs:checkterm
        rio_readlineb(&rio, buf, MAXLINE);
        strcat(request,buf);
    }
    strcat(request,"\r\n\r\n");
    servfd=open_clientfd(hostname, portstr);
    rio_readinitb(&servrio,servfd);
    rio_writen(servfd,request,strlen(request));
    do{
        if(strcasestr(response,"</html>")!=(char*) NULL){
            break;
        }
        temp_response_size=rio_readlineb(&servrio,temp_response,MAX_OBJECT_SIZE);
        response_size+=temp_response_size-1;
        strcat(response,temp_response);
        strcat(response,"\n");
    }while(temp_response_size>0);
    rio_writen(clientfd,response,response_size);
    char response_size_str[MAXLINE];
    sprintf(response_size_str,"%d",response_size);
    logrequest("NERD", uri,response_size_str);
    free(port);
    return NULL;
    //serve_static(fd, filename, sbuf.st_size);        //line:netp:doit:servestatic
}
/* $end doit */


/********* Start of Logging Functions *********/
/*
Function to calculate the day of the week,
written by Sakamoto, Lachman, Keith and Craver,
based on the fact that Jan 1 1900 was a Monday.
*/
int dayofweek(int d, int m, int y)
{
    static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    y -= m < 3;
    return ( y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

/*
Builds a formatted string out of a buffer for date-logging
*/
void formatdate(char* buffer) {
	struct tm* tm_info;
	time_t timer;
	char day[3];
	char year[5];
	char month[5];
	char weekday[4];
	char clock[10];
	char *temp;

  time(&timer);
  tm_info = localtime(&timer);

	strftime(day, 3, "%d", tm_info);
	strftime(month, 5, "%m", tm_info);
	strftime(year, 5, "%Y", tm_info);
	strftime(clock, 10, "%H:%M:%S", tm_info);
	long dayval = strtol(day, &temp, 10);
	long monthval = strtol(month, &temp, 10);
	long yearval =  strtol(year, &temp, 10);
	int weekval = dayofweek((int)dayval, (int)monthval, (int)yearval);

	if (weekval == 0) {
		strcpy(weekday, "Sun");
	} else if (weekval == 1) {
		strcpy(weekday, "Mon");
	} else if (weekval == 2) {
		strcpy(weekday, "Tues");
	} else if (weekval == 3) {
		strcpy(weekday, "Wed");
	} else if (weekval == 4) {
		strcpy(weekday, "Thur");
	} else if (weekval == 5) {
		strcpy(weekday, "Fri");
	} else {
		strcpy(weekday, "Sat");
	}
	if (monthval == 1) {
		strcpy(month, "Jan");
	} else if (monthval == 2) {
		strcpy(month, "Feb");
	} else if (monthval == 3) {
		strcpy(month, "March");
	} else if (monthval == 4) {
		strcpy(month, "April");
	} else if (monthval == 5) {
		strcpy(month, "May");
	} else if (monthval == 6) {
		strcpy(month, "June");
	} else if (monthval == 7) {
		strcpy(month, "July");
	} else if (monthval == 8) {
		strcpy(month, "Aug");
	} else if (monthval == 9) {
		strcpy(month, "Sept");
	} else if (monthval == 10) {
		strcpy(month, "Oct");
	} else if (monthval == 11) {
		strcpy(month, "Nov");
	} else {
		strcpy(month, "Dec");
	}
	
	sprintf(buffer, "[%s %i %s %i %s]", weekday, (int)dayval, month, (int)yearval, clock);
}

/* 
Creates a log file if one does not exist,
logs the server's activation time
*/
void logcreate () {
	fp = fopen("proxy.log", "a+");	//creates file or opens for appending
	if (fp == NULL) {
		printf("Error openning log file at activation.\n");
	}
	fflush(fp);

	char buffer[33];
	formatdate(buffer);

	if (fprintf(fp, "%s ", buffer) < 0) {
		printf("Error writing time of activation to log.\n");
	}
	if (fprintf(fp, "Server Activated\n") < 0) {
		printf("Error writing activation message to log.\n");
	}

	fclose(fp);
}

/* 
Adds the client's request to the log file
*/
void logrequest (char *browserIP, char *URL, char *numberofbytes) {
	fp = fopen("proxy.log", "a+");
	if (fp == NULL) {
		printf("Error openning log file at request.\n");
	}
	fflush(fp);

	char buffer[33];
	formatdate(buffer);

	if (fprintf(fp, "%s ", buffer) < 0) {
		printf("Error writing time of request to log.\n");
	}
	if (fprintf(fp, "%s ", browserIP) < 0) {
		printf("Error writing browserIP request to log.\n");
	}
	if (fprintf(fp, "%s ", URL) < 0) {
		printf("Error writing URL request to log.\n");
	}
	if (fprintf(fp, "%s\n", numberofbytes) < 0) {
		printf("Error writing size request to log.\n");
	}

	fclose(fp);
}

/* 
Logs the time of the server closing 
*/
void logclose () {
	fp = fopen("proxy.log", "a+");
	if (fp == NULL) {
		printf("Error openning log file at closure.\n");
	}
	fflush(fp);

	char buffer[33];
	formatdate(buffer);

	if (fprintf(fp, "%s ", buffer) < 0) {
		printf("Error writing time of closing to log.\n");
	}
	if (fprintf(fp, "Server Closed\n") < 0) {
		printf("Error writing closure message to log.\n");
	}

	fclose(fp);
}

/********* End of Logging Functions *********/

int main(int argc, char **argv) 
{
    int listenfd;
    int *connfdp;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
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
        pthread_create(tid,NULL,doit,connfdp);
    }
		logclose(); /* CLOSES LOG FILE */
    exit(0);
}
/* $end echoserverimain */


/*
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
	hostname[0] = '\0';
	return -1;
    }
       
    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';
    
    /* Extract the port number */
    *port = 80; /* default */
    if (*hostend == ':')   
	*port = atoi(hostend + 1);
    
    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
	pathname[0] = '\0';
    }
    else {
	pathbegin++;	
	strcpy(pathname, pathbegin);
    }

    return 0;
}
