#include"SmartLock.h"
#include"open_sockfd.h"
#include"mutex.h"

int user_fds[MAX_USER_COUNT];
struct sockaddr_in user_addrs_open_camera[MAX_USER_COUNT];
int stmfd = -1;
int sendingImage = 0;
int camera_is_open = 0;
int sending_lock_state = 0;

int main(int argc,char** argv) {
	int listenfd,udpfd;
	char type;
	int id = 0;
	listenfd = open_listenfd(argv[1]);
	assert(listenfd >= 0);
	udpfd = open_udpfd(argv[1]);
	assert(udpfd >= 0);
	client_data* user_datas = new client_data[FDLIMIT];
	pthread_mutex_t* db_mutex = new pthread_mutex_t; // 创建一把互斥锁
	init_mutex(db_mutex);//初始化锁
	MYDB* mydb = new MYDB;//创建一个mysql数据库连接
	mydb->mydb_mysql_init();
	if (mydb->mydb_mysql_real_connect() == -1) {
		printf("connect to mysql failed!\n");
	}
	mydb->db_mutex = db_mutex;
	epoll_event events[MAX_EVENT_NUMBER];
	int epollfd = epoll_create(5);
	assert(epollfd != -1);
	addfd(epollfd, listenfd, false);
	addfd(epollfd, udpfd, true);
	int ret = 0;
	for (int i = 0; i < MAX_USER_COUNT; i++) {
		user_fds[i] = -1;
		memset(&user_addrs_open_camera[i], '\0', sizeof(struct sockaddr_in));
	}
	while (1) {
		ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
		if (ret < 0) {
			printf("epoll failure\n");
			break;
		}
		for (int i = 0; i < ret; i++) {
			int sockfd = events[i].data.fd;
			if (sockfd == listenfd) {
				struct sockaddr_in cliaddr;
				socklen_t clilen = sizeof(cliaddr);
				int connfd = accept(sockfd, (SA*)&cliaddr, &clilen);
				assert(connfd != -1);
				recv(connfd, &type, 1, 0);
				if (type == '0') stmfd = connfd;
				else if (type == '1' && stmfd == -1) {
					send(connfd, "The lock is disconnected!\n", 27, 0);
					close(connfd);
					continue;
				}
				else if(type == '1'){
					//判断连接数是否已达上限
					int i = 0;
					for (; i < MAX_USER_COUNT; i++) {
						if (user_fds[i] == -1) {
							user_fds[i] = connfd;
							break;
						}
					}
					if (i == MAX_USER_COUNT) {
						printf("The number of client connections is full!\n");
						send(connfd, "The number of client connections is full!\n", 43, 0);
						close(connfd);
						continue;
					}
				}
				else {
					printf("I do no know the type of client!\n");
					close(connfd);
					continue;
				}
				printf("connection from: %s %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
				addfd(epollfd, connfd, true);
				user_datas[connfd].epollfd = epollfd;
				user_datas[connfd].sockfd = connfd;
				user_datas[connfd].udpfd = udpfd;
				user_datas[connfd].address = cliaddr;
				user_datas[connfd].dbhandle = mydb;
			}
			else if (sockfd == udpfd) {
				fds udp_fds;
				udp_fds.epollfd = epollfd;
				udp_fds.udpfd = udpfd;
				pthread_t thread;
				pthread_create(&thread, NULL, send_udp_data, (void*)&udp_fds);
				pthread_detach(thread);
			}
			else if (events[i].events & EPOLLIN) {
				pthread_t thread;
				pthread_create(&thread, NULL, worker, (void*)&(user_datas[sockfd]));
				pthread_detach(thread);
			}
			else {
				printf("something else happened\n");
			}
		}
	}
	close(listenfd);
	return 0;
}
