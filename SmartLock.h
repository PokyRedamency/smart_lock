#pragma once
//#pragma pack(1)
#include<string>
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
#include"mysql.h"

using namespace std;

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
#define MAX_USER_COUNT 5 

//reportType
#define IMAGE char(1 << 0)
#define PERSON char(1 << 1)
#define LONG_STAY char(1 << 2)
//上传锁的状态
#define LOCK_STATE char(1 << 3)
#define PWD_CHANGE_RESULT char(1 << 4)

#define LOCKED char(0)
#define UNLOCKED char(1)
//返回修改管理员密码结果
#define SUCCESS char(1)
#define ORI_PWD_WRONG char(2)
#define OTHER char(3)
//上报开锁相关的信息
#define UNLOCK_MSG char(1<<5)
#define WRONG_UNLOCK char(1) //多次开锁失败
#define CARD_UNLOCK char(2) //门卡开锁成功
#define PWD_UNLOCK char(3) //密码开锁成功
#define TRY_OPEN char(1<<6)
//app端 -> 服务端 reportType
#define OPEN_CAMERA char(3<<0) //打开摄像头请求
#define CLOSE_CAMERA char(3<<1)//关闭摄像头请求
#define PWD_CHANGE char(3<<2)//修改密码请求
#define LOCK_CONTROL char(3<<3)//锁控制
#define GET_IMAGE char(3<<4) //获取图片

typedef struct _report {
    char reportType;
    int reportLen;
    char buf[128];
}report;

typedef struct _fds {
    int epollfd;
    int udpfd;
}fds;


typedef struct _client_data {
    int epollfd;
    int sockfd;
    int udpfd;
    MYDB* dbhandle;
    struct sockaddr_in address;
    char buf[BUFFER_SIZE];
}client_data;

void getTimeStamp(client_data* user_data,char *timeStamp);

void sig_chld(int signo);

int parse_header(client_data* user_data, report* r,char* filename,char* timeStamp);

ssize_t rio_writen(int fd, void* usrbuf, socklen_t n);

void doit_image(client_data* user_data, int epollfd,int fd,int filefd,int filesize,char* filepath,char* timeStamp);

void doit_person(client_data* user_data, int epollfd,int filefd, int sockfd, report* rpt);

void doit_longstay(client_data* user_data, int epollfd,int filefd, int sockfd,char* recvline, report* rpt);

void doit_lockstate(client_data* user_data, int epollfd, int sockfd,report* rpt);

void doit_pwd_change_result(client_data* user_data, int epollfd, int filefd, int sockfd, report* rpt);

void doit_unlock_msg(client_data* user_data, int epollfd, int filefd, int sockfd,char* recvline);

void doit_try_open(client_data* user_data, int epollfd, int filefd, int sockfd, report* rpt);

void doit_open_camera(client_data* user_data, report* rpt);

void doit_close_camera(client_data* user_data, report* rpt);

void doit_pwd_change(client_data* user_data,report* rpt);

void forward_to_app(char* sendMsg);

int forward_to_stm(char* sendMsg);

void close_fd(int sockfd);

void* send_udp_data(void* arg);

void send_image_to_app(client_data* user_data, char* filename, char* filepath);

int setnonblocking(int fd);

void addfd(int epollfd, int fd, bool oneshot);

void* worker(void* arg);