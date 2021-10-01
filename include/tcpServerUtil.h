#ifndef TCPSERVERUTIL_H_
#define TCPSERVERUTIL_H_

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include "logger.h"
#include "util.h"
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h> 
#include "commandParser.h"

enum dateFormat{
	EN,
	ES
};
// Create, bind, and listen a new TCP server socket
int setupTCPServerSocket(const char *service);

// Accept a new TCP connection on a server socket
int acceptTCPConnection(int servSock);

// Handle new TCP client
int handleTCPEchoClient(int clntSocket);

int getTime(char * time_buffer);

int getDate(char * date_buffer, int format);

void toLowerString(char * str);

#endif 
