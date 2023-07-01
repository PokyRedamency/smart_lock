#include"open_sockfd.h"

void sig_chld(int signo) {
	pid_t pid;
	int stat;

	pid = wait(&stat);
	printf("child %d terminated\n", pid);
	return;
}

//socket and connect
int open_clientfd(char* hostname, char* port) {
	int clientfd;
	struct addrinfo hints, * listp, * p;

	/*获取可能的服务器列表*/
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICSERV;
	hints.ai_flags |= AI_ADDRCONFIG;
	getaddrinfo(hostname, port, &hints, &listp);

	/*遍历可能的服务器列表找到可连接的服务器*/
	for (p = listp; p; p = p->ai_next) {
		/*创建socket描述符*/
		if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) continue;

		/*连接到服务器*/
		if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1) break;/*Connect Success*/
		/*连接失败 在尝试下一个前关闭已创建描述符*/
		close(clientfd);
	}
	/*Clean Up*/
	freeaddrinfo(listp);
	if (!p) return -1; /*Connect failed*/
	else return clientfd;

}



//socket,bind and listen
int open_listenfd(char* port) {
	int listenfd, optval = 1;
	struct addrinfo hints, * listp, * p;

	/*获取可能的服务器列表*/
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
	hints.ai_flags |= AI_NUMERICSERV;
	getaddrinfo(nullptr, port, &hints, &listp);

	/*遍历可能的服务器列表 找到可以bind的*/
	for (p = listp; p; p = p->ai_next) {
		/*创建socket描述符*/
		if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) continue;

		/*使服务器可以中断 设置超时中断机制*/
		setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

		/*将服务器地址与描述符进行绑定*/
		if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
		/*绑定失败 在尝试下一个前关闭已创建描述符*/
		close(listenfd);
	}
	freeaddrinfo(listp);
	if (!p)return -1;

	if (listen(listenfd, LISTENQ) < 0) {
		close(listenfd);
		return -1;
	}
	signal(SIGCHLD, sig_chld);
	return listenfd;
}

int open_udpfd(char* port) {
	int udpfd;
	int optval = 1;
	struct addrinfo hints, * listp, * p;

	/*获取可能的服务器列表*/
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
	hints.ai_flags |= AI_NUMERICSERV;
	getaddrinfo(nullptr, port, &hints, &listp);

	/*遍历可能的服务器列表 找到可以bind的*/
	for (p = listp; p; p = p->ai_next) {
		/*创建socket描述符*/
		if ((udpfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) continue;

		/*使服务器可以中断 设置超时中断机制*/
		setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

		/*将服务器地址与描述符进行绑定*/
		if (bind(udpfd, p->ai_addr, p->ai_addrlen) == 0) break;
		/*绑定失败 在尝试下一个前关闭已创建描述符*/
		close(udpfd);
	}
	freeaddrinfo(listp);
	if (!p)return -1;

	return udpfd;
}
