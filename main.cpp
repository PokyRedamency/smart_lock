#include"SmartLock.h"
#include"open_sockfd.h"


int main(int argc,char** argv) {
	int listenfd,udpfd;
	char type;
	int id = 0;
	int stmfd = -1;
	listenfd = open_listenfd(argv[1]);
	assert(listenfd >= 0);
	udpfd = open_udpfd(argv[1]);
	assert(udpfd >= 0);
	client_data* user_datas = new client_data[FDLIMIT];
	epoll_event events[MAX_EVENT_NUMBER];
	int epollfd = epoll_create(5);
	assert(epollfd != -1);
	addfd(epollfd, listenfd, false);
	int ret = 0;
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
				else {

				}
				printf("connection from: %s %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
				addfd(epollfd, connfd, true);
				user_datas[connfd].epollfd = epollfd;
				user_datas[connfd].sockfd = connfd;
				user_datas[connfd].udpfd = udpfd;
				user_datas[connfd].stmfd = stmfd;
				user_datas[connfd].address = cliaddr;
			}
			else if (events[i].events & EPOLLIN) {
				pthread_t thread;
				client_data user_data = user_datas[sockfd];
				pthread_create(&thread, NULL, worker, (void*)&user_data);
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