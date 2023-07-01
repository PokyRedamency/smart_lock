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

	/*��ȡ���ܵķ������б�*/
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICSERV;
	hints.ai_flags |= AI_ADDRCONFIG;
	getaddrinfo(hostname, port, &hints, &listp);

	/*�������ܵķ������б��ҵ������ӵķ�����*/
	for (p = listp; p; p = p->ai_next) {
		/*����socket������*/
		if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) continue;

		/*���ӵ�������*/
		if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1) break;/*Connect Success*/
		/*����ʧ�� �ڳ�����һ��ǰ�ر��Ѵ���������*/
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

	/*��ȡ���ܵķ������б�*/
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
	hints.ai_flags |= AI_NUMERICSERV;
	getaddrinfo(nullptr, port, &hints, &listp);

	/*�������ܵķ������б� �ҵ�����bind��*/
	for (p = listp; p; p = p->ai_next) {
		/*����socket������*/
		if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) continue;

		/*ʹ�����������ж� ���ó�ʱ�жϻ���*/
		setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

		/*����������ַ�����������а�*/
		if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
		/*��ʧ�� �ڳ�����һ��ǰ�ر��Ѵ���������*/
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

	/*��ȡ���ܵķ������б�*/
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
	hints.ai_flags |= AI_NUMERICSERV;
	getaddrinfo(nullptr, port, &hints, &listp);

	/*�������ܵķ������б� �ҵ�����bind��*/
	for (p = listp; p; p = p->ai_next) {
		/*����socket������*/
		if ((udpfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) continue;

		/*ʹ�����������ж� ���ó�ʱ�жϻ���*/
		setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

		/*����������ַ�����������а�*/
		if (bind(udpfd, p->ai_addr, p->ai_addrlen) == 0) break;
		/*��ʧ�� �ڳ�����һ��ǰ�ر��Ѵ���������*/
		close(udpfd);
	}
	freeaddrinfo(listp);
	if (!p)return -1;

	return udpfd;
}
