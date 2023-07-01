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

#define NULL nullptr
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

//reportType
#define IMAGE char(1 << 0)
#define PERSON char(1 << 1)
#define LONG_STAY char(1 << 2)
//�ϴ�����״̬
#define LOCK_STATE char(1 << 3)
#define PWD_CHANGE_RESULT char(1 << 4)

#define LOCKED char(1)
#define UNLOCKED char(2)
//�����޸Ĺ���Ա������
#define SUCCESS char(1)
#define ORI_PWD_WRONG char(2)
#define OTHER char(3)
//�ϱ�������ص���Ϣ
#define UNLOCK_MSG char(1<<5)
#define WRONG_UNLOCK char(1) //��ο���ʧ��
#define CARD_UNLOCK char(2) //�ſ������ɹ�
#define PWD_UNLOCK char(3) //���뿪���ɹ�
#define TRY_OPEN char(1<<6)
//app�� -> ����� reportType
#define OPEN_CAMERA char(3<<0) //������ͷ����
#define CLOSE_CAMERA char(3<<1)//�ر�����ͷ����
#define PWD_CHANGE char(3<<2)//�޸���������
#define LOCK_CONTROL char(3<<3)//������
#define GET_IMAGE char(3<<4) //��ȡͼƬ

typedef struct _report {
    char reportType;
    int reportLen;
    char buf[128];
}report;


typedef struct _client_data {
    int epollfd;
    int sockfd;
    int stmfd;
    int udpfd;
    struct sockaddr_in address;
    char buf[BUFFER_SIZE];
}client_data;

void getTimeStamp(char *timeStamp);

void sig_chld(int signo);

int parse_header(report* r,char* filename,char* timeStamp);

void doit_image(int epollfd,int fd,int filefd,int filesize,char* filepath,char* timeStamp);

void doit_person(int epollfd,int filefd, int sockfd);

void doit_longstay(int epollfd,int filefd, int sockfd);

void doit_lockstate(int epollfd, int sockfd);

void doit_pwd_change_result(int epollfd, int filefd, int sockfd);

void doit_unlock_msg(int epollfd, int filefd, int sockfd);

void doit_try_open(int epollfd, int filefd, int sockfd);

void doit_open_camera(client_data* user_data);

void doit_pwd_change(client_data* user_data,report* rpt);

int setnonblocking(int fd);

void addfd(int epollfd, int fd, bool oneshot);

void* worker(void* arg);