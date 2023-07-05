#pragma once
#include<arpa/inet.h>
#include <stdlib.h>
#include<stdio.h>
#include <cstddef>
#include<string.h>
#include<sys/socket.h>
#include<netdb.h>
#include<unistd.h>
#include <errno.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<wait.h>
#include<time.h>
#include<assert.h>
#include<sys/epoll.h>
#include<pthread.h>

typedef struct sockaddr SA;

#define MAX_SIZE 1024*1024
#define MAX_LINE 1024
#define LISTENQ 5
#define RIO_BUFSIZE 1024
#define MAXBUF 131072
#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 1024
#define FDLIMIT 65535
#define SEND_STATE 0
#define RECV_STATE 0
void sig_chld(int signo);

int open_clientfd(char* hostname, char* port);

int open_listenfd(char* port);

int open_udpfd(char* port);