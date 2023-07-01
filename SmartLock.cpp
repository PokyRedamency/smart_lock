#include "SmartLock.h"
#include "mysql.h"

MYDB mydb;

int setnonblocking(int fd) {
	int oldoption = fcntl(fd, F_GETFL);
	int newoption = oldoption | O_NONBLOCK;
	fcntl(fd, F_SETFL, newoption);
	return oldoption;
}

void addfd(int epollfd, int fd,bool oneshot) {
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	if (oneshot) {
		event.events |= EPOLLONESHOT;
	}
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnonblocking(fd);
}

void reset_oneshot(int epollfd, int fd) {
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

void getTimeStamp(char* timeStamp) {
	mydb.mydb_mysql_query("select now()");
	MYSQL_ROW row = mysql_fetch_row(mydb.res);
	sprintf(timeStamp, "%s", row[0]);
}

int parse_header(report* r, char* filename,char* timeStamp) {
	int filefd;
	char type = r->reportType;
	switch (type) {
	case IMAGE: {
		getTimeStamp(timeStamp);
		sprintf(filename, "%s", "/LL/SmartLock/image/");
		strcat(filename, timeStamp);
		strcat(filename, ".jpeg");
		filefd = open(filename, O_WRONLY | O_CREAT, 0777);
		break;
	}
	case PERSON: {
		sprintf(filename, "%s", "/LL/SmartLock/");
		strcat(filename, "smartlock.log");
		filefd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0777);
		break;
	}
	case LONG_STAY: {
		sprintf(filename, "%s", "/LL/SmartLock/");
		strcat(filename, "smartlock.log");
		filefd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0777);
		break;
	}
	case PWD_CHANGE_RESULT: {
		sprintf(filename, "%s", "/LL/SmartLock/");
		strcat(filename, "pwd_change_result.log");
		filefd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0777);
		break;
	}
	case UNLOCK_MSG: {
		sprintf(filename, "%s", "/LL/SmartLock/");
		strcat(filename, "unlock_msg.log");
		filefd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0777);
		break;
	}
	case TRY_OPEN: {
		sprintf(filename, "%s", "/LL/SmartLock/");
		strcat(filename, "TRY_OPEN.log");
		filefd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0777);
		break;
	}
	case OPEN_CAMERA: {
		sprintf(filename, "%s", "OPEN_CAMERA......");
		break;
	}
	case PWD_CHANGE: {
		sprintf(filename, "%s", "PWD_CHANGE......");
		break;
	}
	default: {
		break;
	}
	}
	return filefd;
}

void doit_image(int epollfd,int sockfd,int filefd,int filesize,char* filepath,char* timeStamp) {
	int readsize;
	char recvline[RIO_BUFSIZE];
	char* sql;
	sprintf(sql, "insert into images values('%s','%s')", timeStamp, filepath);
	mydb.mydb_mysql_query(sql);
	while (filesize > 0) {
		memset(recvline, '\0', sizeof(recvline));
		if (filesize > RIO_BUFSIZE) readsize = RIO_BUFSIZE;
		else readsize = filesize;
		int ret = recv(sockfd, recvline, readsize, 0);
		if (ret < 0) {
			if (errno == EINTR||errno == EAGAIN) {
				continue;
			}
			else {
				printf("errno is: %d\n", errno);
				close(sockfd);
				printf("recv failure\n");
				break;
			}
		}
		else if (ret == 0) {
			close(sockfd);
			printf("The client is disconnected\n");
			break;
		}
		write(filefd, recvline, ret);
		filesize -= ret;
	}
	if (filesize == 0) reset_oneshot(epollfd, sockfd);
}

void doit_person(int epollfd,int filefd,int sockfd) {
	char writeline[MAX_LINE];
	getTimeStamp(writeline);
	strcat(writeline, " Person\n");
	write(filefd, writeline, strlen(writeline));
	reset_oneshot(epollfd, sockfd);
}

void doit_longstay(int epollfd,int filefd, int sockfd) {
	char buf[BUFFER_SIZE];
	char writeline[MAX_LINE];
	memset(buf, '\0', BUFFER_SIZE);
	memset(writeline, '\0', MAX_LINE);
	getTimeStamp(writeline);
	for (int i = 1; i > 0;) {
		int ret = recv(sockfd, buf, 1, 0);
		if (ret < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				continue;
			}
			else {
				close(sockfd);
				printf("recv failure\n");
				break;
			}
		}
		else if (ret == 0) {
			close(sockfd);
			printf("The client is disconnected\n");
			break;
		}
		strcat(writeline, " LongStay ");
		strcat(writeline, buf);
		strcat(writeline, "s\n");
		write(filefd, writeline, strlen(writeline));
		reset_oneshot(epollfd, sockfd);
		i--;
	}
}

void doit_lockstate(int epollfd, int sockfd) {
	char recvline[MAX_LINE];
	for (int i = 1; i > 0;) {
		memset(recvline, '\0', MAX_LINE);
		int ret = recv(sockfd, recvline, 1, 0);
		if (ret < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				continue;
			}
			else {
				close(sockfd);
				printf("recv failure\n");
				break;
			}
		}
		else if (ret == 0) {
			close(sockfd);
			printf("The client is disconnected\n");
			break;
		}
		if (recvline[0] == LOCKED) {
			const char* sql = "update smart_lock set LOCK_STATE = 'LOCKED'";
			mydb.mydb_mysql_query(sql);
		}
		else if (recvline[0] == UNLOCKED) {
			const char* sql = "update smart_lock set LOCK_STATE = 'UNLOCKED'";
			mydb.mydb_mysql_query(sql);
		}
		else {
			printf("recvline = %s\n", recvline);
			printf("I do not know this LockState\n");
		}
		reset_oneshot(epollfd, sockfd);
		i--;
	}
}

void doit_pwd_change_result(int epollfd, int filefd, int sockfd) {
	char recvline[MAX_LINE];
	char writeline[MAX_LINE];
	getTimeStamp(writeline);
	for (int i = 1; i > 0;) {
		memset(recvline, '\0', MAX_LINE);
		int ret = recv(sockfd, recvline, 1, 0);
		if (ret < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				continue;
			}
			else {
				close(sockfd);
				printf("recv failure\n");
				break;
			}
		}
		else if (ret == 0) {
			close(sockfd);
			printf("The client is disconnected\n");
			break;
		}
		if (recvline[0] == SUCCESS) {
			strcat(writeline, " SUCCESS\n");
			write(filefd, writeline, strlen(writeline));
		}
		else if (recvline[0] == ORI_PWD_WRONG) {
			strcat(writeline, " ORI_PWD_WRONG\n");
			write(filefd, writeline, strlen(writeline));
		}
		else if (recvline[0] == OTHER) {
			strcat(writeline, " OTHER\n");
			write(filefd, writeline, strlen(writeline));
		}
		else {
			printf("I do not know this pwd_change_result\n");
		}
		reset_oneshot(epollfd, sockfd);
		i--;
	}
}

void doit_unlock_msg(int epollfd, int filefd, int sockfd) {
	char recvline[MAX_LINE];
	char writeline[MAX_LINE];
	memset(writeline, '\0', MAX_LINE);
	getTimeStamp(writeline);
	for (int i = 1; i > 0;) {
		memset(recvline, '\0', MAX_LINE);
		int ret = recv(sockfd, recvline, MAX_LINE, 0);
		if (ret < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				continue;
			}
			else {
				close(sockfd);
				printf("recv failure\n");
				break;
			}
		}
		else if (ret == 0) {
			close(sockfd);
			printf("The client is disconnected\n");
			break;
		}
		
		if (ret > 1) {
			strcat(writeline, " CARD_UNLOCK:");
			strcat(writeline, recvline);
			strcat(writeline, "\n");
			write(filefd, writeline, strlen(writeline));
		}
		else if (recvline[0] == WRONG_UNLOCK) {
			strcat(writeline, " WRONG_UNLOCK\n");
			write(filefd, writeline, strlen(writeline));
		}
		else if (recvline[0] == PWD_UNLOCK) {
			strcat(writeline, " PWD_UNLOCK\n");
			write(filefd, writeline, strlen(writeline));
		}
		else {
			printf("I do not know this unlock_msg\n");
		}
		reset_oneshot(epollfd, sockfd);
		i--;
	}
}

void doit_try_open(int epollfd, int filefd, int sockfd) {
	char writeline[MAX_LINE];
	getTimeStamp(writeline);
	strcat(writeline, " TRY_OPEN\n");
	write(filefd, writeline, strlen(writeline));
	reset_oneshot(epollfd, sockfd);
}

void doit_open_camera(client_data* user_data) {
	if (user_data->stmfd == -1) { //判断单片机连接情况 -1表示未连接
		send(user_data->sockfd, "The lock is not connected!\n", 28, 0);
		return;
	}
	int state = SEND_STATE;
	report sendline;
	char recvline[BUFFER_SIZE];
	memset(&sendline, '\0', BUFFER_SIZE);
	sendline.reportType = OPEN_CAMERA;
	sendline.reportLen = 0;
	int ret = send(user_data->stmfd, &sendline, BUFFER_SIZE, 0); //发送类型为OPEN_CAMERA的头部数据
	while (1) {
		if(state == SEND_STATE) { //先发送打开摄像头请求给单片机
			if (ret < 0) {
				send(user_data->sockfd, "Open camera failure!\n", 22, 0);
				close(user_data->stmfd);
				break;
			}
			else if (ret == 0) {
				send(user_data->sockfd, "The lock is disconnected!\n", 27, 0);
				close(user_data->stmfd);
				break;
			}
			else {
				state = RECV_STATE;//请求发送成功则下次循环state变为接收单片机传来的摄像头数据
				continue;
			}
		}
		else if(state == RECV_STATE){//接收摄像头数据并进行转发
			memset(recvline, '\0', BUFFER_SIZE);
			ret = recvfrom(user_data->stmfd, recvline, BUFFER_SIZE - 1, 0, NULL, NULL);//接收
			if (ret < 0 && errno != EAGAIN || ret == 0) {
				send(user_data->sockfd, "The lock is disconnected!\n", 27, 0);
				close(user_data->stmfd);
				break;
			}
			else {
				sendto(user_data->udpfd, recvline, ret, 0, (SA*)&user_data->address, sizeof(user_data->address));//转发
			}
		}
	}
	reset_oneshot(user_data->epollfd, user_data->sockfd);
}

void doit_pwd_change(client_data* user_data,report* rpt){
	int ret;
	int rptLen = rpt->reportLen;
	if (user_data->stmfd == -1) {
		send(user_data->sockfd, "The lock is not connected!\n", 28, 0);
		return;
	}
	printf("pwd: %s\n", rpt->buf);
}


void* worker(void* arg) {
	int sockfd = ((client_data*)arg)->sockfd;
	int epollfd = ((client_data*)arg)->epollfd;
	printf("start new thread to receive data on fd: %d\n", sockfd);

	mydb.mydb_mysql_init();
	if (mydb.mydb_mysql_real_connect() == -1) {
		printf("connect to mysql failed!\n");
	}

	char filepath[MAX_LINE];
	char timeStamp[MAX_LINE];
	report rpt;
	int filefd;
	int readn = 0;
	int filesize;
	//读取头部信息
	readn = recv(sockfd, &rpt, sizeof(rpt), 0);
	printf("%d\n", readn);
	filesize = rpt.reportLen;
	if (readn == 0) {
		close(sockfd);
		printf("The client is disconnected\n");
	}
	else {
		filefd = parse_header(&rpt, filepath,timeStamp);
		printf("filepath: %s\nfilesize: %d\n", filepath, filesize);
		switch (rpt.reportType) {
		case IMAGE:
			doit_image(epollfd, sockfd, filefd, filesize, filepath,timeStamp);
			break;
		case PERSON:
			doit_person(epollfd, filefd, sockfd);
			break;
		case LONG_STAY:
			doit_longstay(epollfd, filefd, sockfd);
			break;
		case LOCK_STATE:
			doit_lockstate(epollfd, sockfd);
			break;
		case PWD_CHANGE_RESULT:
			doit_pwd_change_result(epollfd, filefd, sockfd);
			break;
		case UNLOCK_MSG:
			doit_unlock_msg(epollfd, filefd, sockfd);
			break;
		case TRY_OPEN:
			doit_try_open(epollfd, filefd, sockfd);
			break;
		case OPEN_CAMERA:
			doit_open_camera((client_data*)arg);
			break;
		case PWD_CHANGE:
			doit_pwd_change((client_data*)arg, &rpt);
			break;
		default:
			close(sockfd);
			printf("I do not know this flag\n");
			break;
		}
		close(filefd);
	}
	printf("end thread to receive data on fd: %d\n", sockfd);
}