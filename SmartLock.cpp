#include "SmartLock.h"


extern int user_fds[MAX_USER_COUNT];
extern struct sockaddr_in user_addrs_open_camera[MAX_USER_COUNT];
extern int stmfd;
extern int sendingImage;
extern int camera_is_open;
extern int sending_lock_state;

int setnonblocking(int fd) {
	int oldoption = fcntl(fd, F_GETFL);
	int newoption = oldoption | O_NONBLOCK;
	fcntl(fd, F_SETFL, newoption);
	return oldoption;
}

void addfd(int epollfd, int fd, bool oneshot) {
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

void getTimeStamp(client_data* user_data, char* timeStamp) {
	MYDB* mydb = user_data->dbhandle;
	// 获得互斥锁的锁
	lock_mutex(user_data->dbhandle->db_mutex);
	mydb->mydb_mysql_query("select now()");
	MYSQL_ROW row = mysql_fetch_row(mydb->res);
	sprintf(timeStamp, "%s", row[0]);
	// 处理完数据库操作和结果存储后，释放互斥锁的锁
	unlock_mutex(user_data->dbhandle->db_mutex);
}

ssize_t rio_writen(int fd, void* usrbuf, socklen_t n) {
	socklen_t nleft = n;
	ssize_t nwritten;
	char* bufp = (char*)usrbuf;

	while (nleft > 0) {
		if ((nwritten = send(fd, bufp, nleft, 0)) <= 0) {
			if (errno == EINTR)
				nwritten = 0;
			else
				return -1;
		}
		nleft -= nwritten;
		bufp += nwritten;
	}
	return n;
}

int parse_header(client_data* user_data, report* r, char* filename, char* timeStamp) {
	int filefd;
	char type = r->reportType;
	switch (type) {
	case IMAGE: {
		getTimeStamp(user_data,timeStamp);
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
	case LOCK_STATE: {
		sprintf(filename, "%s", "LOCK_STATE......");
		break;
	}
	case LOCK_CONTROL: {
		sprintf(filename, "%s", "LOCK_CONTROL......");
		break;
	}
	case GET_IMAGE: {
		sprintf(filename, "%s", "GET_IMAGE......");
		break;
	}
	default: {
		sprintf(filename, "%s", "UNKNOWN_TYPE......");
		break;
	}
	}
	return filefd;
}

void doit_image(client_data* user_data, int epollfd, int sockfd, int filefd, int filesize, char* filepath, char* timeStamp) {
	int readsize;
	char recvline[BUFFER_SIZE];
	char* sql = (char*)malloc(BUFFER_SIZE);
	MYDB* mydb = user_data->dbhandle;
	// 获得互斥锁的锁
	lock_mutex(user_data->dbhandle->db_mutex);
	sprintf(sql, "insert into images values('%s','%s')", timeStamp, filepath);
	mydb->mydb_mysql_query(sql);
	// 处理完数据库操作和结果存储后，释放互斥锁的锁
	unlock_mutex(user_data->dbhandle->db_mutex);
	while (filesize) {
		memset(recvline, '\0', sizeof(recvline));
		if (filesize > BUFFER_SIZE - 1) readsize = BUFFER_SIZE - 1;
		else readsize = filesize;
		int ret = recv(sockfd, recvline, readsize, 0);
		if (ret < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				continue;
			}
			else {
				printf("errno is: %d\n", errno);
				printf("recv failure\n");
				close(sockfd);
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
	send_image_to_app(user_data, timeStamp, filepath);
	free(sql);
}

void doit_person(client_data* user_data, int epollfd, int filefd, int sockfd, report* rpt) {
	//forward_to_app((char*)rpt);
	char writeline[MAX_LINE];
	getTimeStamp(user_data,writeline);
	strcat(writeline, " Person\n");
	write(filefd, writeline, strlen(writeline));
}

void doit_longstay(client_data* user_data, int epollfd, int filefd, int sockfd, char* recvline, report* rpt) {
	//forward_to_app((char*)rpt);
	char writeline[BUFFER_SIZE];
	char buf[BUFFER_SIZE];
	memset(writeline, '\0', BUFFER_SIZE);
	memset(buf, '\0', BUFFER_SIZE);
	getTimeStamp(user_data,writeline);
	strcat(writeline, " LongStay ");
	sprintf(buf, "%d", recvline[0]);
	strcat(writeline, buf);
	strcat(writeline, "s\n");
	write(filefd, writeline, strlen(writeline));
}

void doit_lockstate(client_data* user_data, int epollfd, int sockfd, report* rpt) {
	sending_lock_state++;
	forward_to_app((char*)rpt);
	sending_lock_state--;
	char* recvline = rpt->buf;
	MYDB* mydb = user_data->dbhandle;

	if (recvline[0] == LOCKED) {
		// 获得互斥锁的锁
		lock_mutex(user_data->dbhandle->db_mutex);
		printf("lock_mutex...\n");
		mydb->mydb_mysql_query("update smart_lock set LOCK_STATE = 'LOCKED'");
		// 处理完数据库操作和结果存储后，释放互斥锁的锁
		unlock_mutex(user_data->dbhandle->db_mutex);
		printf("unlock_mutex...\n");
	}
	else if (recvline[0] == UNLOCKED) {
		// 获得互斥锁的锁
		lock_mutex(user_data->dbhandle->db_mutex);
		printf("lock_mutex...\n");
		mydb->mydb_mysql_query("update smart_lock set LOCK_STATE = 'UNLOCKED'");
		// 处理完数据库操作和结果存储后，释放互斥锁的锁
		unlock_mutex(user_data->dbhandle->db_mutex);
		printf("unlock_mutex...\n");
	}
	else {
		printf("recvline = %s\n", recvline);
		printf("I do not know this LockState\n");
	}
}

void doit_pwd_change_result(client_data* user_data, int epollfd, int filefd, int sockfd, report* rpt) {
	forward_to_app((char*)rpt);
	char writeline[MAX_LINE];
	char* recvline = rpt->buf;
	getTimeStamp(user_data, writeline);
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
}

void doit_unlock_msg(client_data* user_data, int epollfd, int filefd, int sockfd, char* recvline) {
	char writeline[MAX_LINE];
	memset(writeline, '\0', MAX_LINE);
	getTimeStamp(user_data, writeline);
	if (strlen(recvline) > 1) {
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
}

void doit_try_open(client_data* user_data, int epollfd, int filefd, int sockfd,report* rpt) {
	char writeline[MAX_LINE];
	getTimeStamp(user_data,writeline);
	strcat(writeline, " TRY_OPEN\n");
	write(filefd, writeline, strlen(writeline));
}

void doit_open_camera(client_data* user_data, report* rpt) {
	if (stmfd == -1) { //判断单片机连接情况 -1表示未连接
		send(user_data->sockfd, "The lock is not connected!\n", 28, 0);
		return;
	}
	char recvline[BUFFER_SIZE];
	int ret = forward_to_stm((char*)rpt); //先转发打开摄像头请求给单片机
	if (ret < 0) {
		printf("Open camera failure!\n");
		close(stmfd);

	}
	else if (ret == 0) {
		printf("The lock is disconnected!\n");
		close(stmfd);

	}
	else {
		//请求发送成功
		camera_is_open++;
		for (int i = 0; i < MAX_USER_COUNT; i++) {
			if (strncmp((char*)&user_addrs_open_camera[i], "\0", 1) == 0) {
				user_addrs_open_camera[i] = user_data->address;
				break;
			}
		}
	}
}

void doit_close_camera(client_data* user_data, report* rpt) {
	if (stmfd == -1) { //判断单片机连接情况 -1表示未连接
		send(user_data->sockfd, "The lock is not connected!\n", 28, 0);
		return;
	}
	char recvline[BUFFER_SIZE];
	int ret = forward_to_stm((char*)rpt); //先转发关闭摄像头请求给单片机
	if (ret < 0) {
		printf("Open camera failure!\n");
		close(stmfd);

	}
	else if (ret == 0) {
		printf("The lock is disconnected!\n");
		close(stmfd);

	}
	else {
		//请求发送成功
		camera_is_open--;
		for (int i = 0; i < MAX_USER_COUNT; i++) {
			if (strcmp((char*)&user_addrs_open_camera[i], (char*)&(user_data->address)) == 0) {
				memset(&user_addrs_open_camera[i],'\0',sizeof(user_addrs_open_camera[i]));
				break;
			}
		}
	}
}

void doit_pwd_change(client_data* user_data, report* rpt) {
	if (stmfd == -1) {
		printf("The lock is not connected!\n");
		return;
	}
	int ret = forward_to_stm((char*)rpt);
	if (ret < 0) printf("forward to stm failure!\n");
	printf("pwd: %s\n", rpt->buf);
}

void forward_to_app(char* sendMsg) {
	for (int i = 0; i < MAX_USER_COUNT; i++) {
		while (sendingImage);
		if (user_fds[i] == -1) continue;
		send(user_fds[i], sendMsg, sizeof(report), 0);
	}
}

int forward_to_stm(char* sendMsg) {
	return send(stmfd, sendMsg, sizeof(report), 0);
}

void* send_udp_data(void* arg) {
	int epollfd = ((fds*)arg)->epollfd;
	int udpfd = ((fds*)arg)->udpfd;
	printf("start new thread for udpfd: %d\n", udpfd);
	char buf[BUFFER_SIZE];
	int n;
	while (camera_is_open) {
		memset(buf, '\0', sizeof(buf));
		n = recvfrom(udpfd, buf, BUFFER_SIZE - 1, 0, NULL, NULL);
		if (n <= 0) continue;
		printf("udp buf :%s\n", buf);
		for (int i = 0; i < MAX_USER_COUNT; i++) {
			if (strncmp((char*)&user_addrs_open_camera[i], "\0", 1) == 0) continue;
			printf("send_udp_data for %d\n", udpfd);
			sendto(udpfd, buf, BUFFER_SIZE, 0, (SA*)&user_addrs_open_camera[i], sizeof(SA));
		}
	}
	
	printf("end thread for udpfd: %d\n", udpfd);
	reset_oneshot(epollfd, udpfd);
	return NULL;
}

void send_image_to_app(client_data* user_data, char* filename,char* filepath) {
	int srcfd;
	int filesize;
	struct stat sbuf;
	char* srcp;
	report rpt;
	for (int i = 0; i < MAX_USER_COUNT; i++) {
		if (user_fds[i] == -1) continue;
		while (sending_lock_state);
		sendingImage++;
		memset(&rpt, '\0', sizeof(rpt));
		rpt.reportType = IMAGE;
		sprintf(rpt.buf, "%s", filename);
		stat(filepath, &sbuf);
		filesize = sbuf.st_size;
		rpt.reportLen = filesize;
		srcfd = open(filepath, O_RDONLY, 0777);
		srcp = (char*)mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
		close(srcfd);
		send(user_fds[i], &rpt, sizeof(rpt), 0);
		rio_writen(user_fds[i], srcp, filesize);
		munmap(srcp, filesize);
		sendingImage--;
	}
}

void close_fd(int sockfd) {
	close(sockfd);
	if (sockfd == stmfd) {
		stmfd = -1;
		return;
	}
	for (int i = 0; i < MAX_USER_COUNT; i++) {
		if (user_fds[i] == sockfd) {
			user_fds[i] = -1;
			break;
		}
	}
	return;
}

void* worker(void* arg) {
	client_data* user_data = (client_data*)arg;
	int sockfd = user_data->sockfd;
	int epollfd = user_data->epollfd;
	printf("start new thread to receive data on fd: %d\n", sockfd);
	MYDB* mydb = user_data->dbhandle;

	char filepath[BUFFER_SIZE];
	char timeStamp[BUFFER_SIZE];
	report rpt;
	int filefd;
	size_t readn = 0;
	size_t bytesToReceive = sizeof(rpt);
	int filesize;
	//接收数据
	while (1) {
		memset(filepath, '\0', BUFFER_SIZE);
		memset(timeStamp, '\0', BUFFER_SIZE);
		memset(&rpt, '\0', sizeof(rpt));
		readn = 0;
		while (readn < bytesToReceive) {
			int ret = recv(sockfd, ((char*)&rpt) + readn, bytesToReceive - readn, 0);
			if (ret == -1) {
				if (errno == EINTR || errno == EAGAIN) continue;
				printf("recv failure!\n");
				close_fd(sockfd);
				break;
			}
			else if (ret == 0) {
				printf("The client is disconnected!\n");
				close_fd(sockfd);
				return 0;
			}
			else {
				readn += ret;
			}
		}
		filesize = rpt.reportLen;

		filefd = parse_header(user_data,&rpt, filepath, timeStamp);
		//printf("filepath: %s\nfilesize: %d\n", filepath, filesize);
		switch (rpt.reportType) {
		case IMAGE:
			doit_image(user_data,epollfd, sockfd, filefd, filesize, filepath, timeStamp);
			break;
		case PERSON:
			doit_person(user_data, epollfd, filefd, sockfd,&rpt);
			break;
		case LONG_STAY:
			doit_longstay(user_data, epollfd, filefd, sockfd, rpt.buf,&rpt);
			break;
		case LOCK_STATE:
			doit_lockstate(user_data, epollfd, sockfd, &rpt);
			break;
		case PWD_CHANGE_RESULT:
			doit_pwd_change_result(user_data, epollfd, filefd, sockfd,&rpt);
			break;
		case UNLOCK_MSG:
			doit_unlock_msg(user_data, epollfd, filefd, sockfd, rpt.buf);
			break;
		case TRY_OPEN:
			doit_try_open(user_data, epollfd, filefd, sockfd,&rpt);
			break;
		case OPEN_CAMERA:
			doit_open_camera(user_data, &rpt);
			break;
		case CLOSE_CAMERA:
			doit_close_camera(user_data, &rpt);
			break;
		case PWD_CHANGE:
			doit_pwd_change(user_data, &rpt);
			break;
		case LOCK_CONTROL:
			forward_to_stm((char*)&rpt);
			break;
		case GET_IMAGE: {
			/*string s = rpt.buf;
			int pos = s.find('&');
			if (pos != string::npos) {
				sprintf(startTime, "%s", s.substr(0, pos).c_str());
				sprintf(endTime, "%s", s.substr(pos + 1).c_str());
				printf("startTime: %s endTime: %s\n", startTime, endTime);
			}
			send_image_to_app((client_data*)arg, startTime, endTime);*/
			break;
		}
		default:
			close_fd(sockfd);
			printf("I do not know this flag\n");
			reset_oneshot(epollfd, sockfd);
			printf("end thread to receive data on fd: %d\n", sockfd);
			return NULL;
		}
		close(filefd);
	}
	reset_oneshot(epollfd, sockfd);
	printf("end thread to receive data on fd: %d\n", sockfd);
	return NULL;
}
