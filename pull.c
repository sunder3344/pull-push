#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#define MAXLINE 4096
#define OPEN_MAX 100
#define LISTENQ 2048
#define SOCKET_NUM 4096
#define INFTIM 1000
#define RNDLEN 10
#define NUMBER 10
#define SEP "===sunder==="

char filename[300];
int ffd, filesize;
char *path;

typedef struct File_st {
	char filename[300];
	char rnd[RNDLEN];
	int filesize;
	int ffd;
} FILE_ST;

//FILE_ST *ft = (FILE_ST *)malloc(sizeof(FILE_ST) *  NUMBER); 
FILE_ST *ft; 

int isFileName(char *str) {
	char *n = strstr(str, SEP);
	if (n) {
		return 1;
	} else {
		return 0;
	}
}

int strchar(char *str, char ch) {
        int i;
        int count = -1;
        int length = strlen(str);
        for (i = 0; i < length; i++) {
                if (str[i] == ch) {
                        count = i;
                        break;
                }
        }
        return count;
}

void packft(char *name, char *rnd, int filesize, int ffd) {
	//pack the file info into ft
	int i = 0;
	int flag = 0;
	for (i = 0; i < NUMBER; i++) {
		if (strlen(ft[i].rnd) == 0 && strlen(ft[i].filename) == 0) {
			strcpy(ft[i].rnd, rnd);
			strcpy(ft[i].filename, name);
			ft[i].filesize = filesize;
			ft[i].ffd = ffd;
			flag = 1;
			break;	
		} else {
			continue;
		}
	}
	if (flag == 0) {
		printf("Sorry! We only support %d files simultaneously!\n", NUMBER);
		exit(2);
	}
	/*for (i = 0; i < NUMBER; i++) {
		printf("name = %s, rnd = %s, size = %d, ffd = %d\n", ft[i].filename, ft[i].rnd, ft[i].filesize, ft[i].ffd);
	}*/
	return;
}

FILE_ST findft(char *rnd) {
	int i;
	FILE_ST tmp;
	for (i = 0; i < NUMBER; i++) {
		if (strcmp(ft[i].rnd, rnd) == 0) {
			return (FILE_ST)ft[i];
			break;
		}
	}
	return tmp;
}

void createFile(char *str) {
	char *p = strstr(str, SEP);
	char *tmp2 = p + strlen(SEP);
	//printf("str = %s, tmp2 = %s\n", str, tmp2);
	char *p2 = strstr(tmp2, SEP);
	char *tmp = (char *)malloc(sizeof(char) * 200);
	strncpy(tmp, tmp2, strlen(tmp2) - strlen(SEP) - RNDLEN);
	char *rnd = p2 + strlen(SEP);

	filesize = atoi(tmp);
	//file name(without path)
	char *name = (char *)malloc(sizeof(char) * 100);
	char tmpfilename[300];
	memset(&filename, 0, sizeof(filename));
	memset(&tmpfilename, 0, sizeof(tmpfilename));

	int z = strlen(str) - strlen(p);
	strncpy(name, str, z);
	name[sizeof(name)] = '\0';

	int pos = strchar(name, '.');
        char *pre_name = (char *)malloc(sizeof(char) * 80);
        char *suffix = (char *)malloc(sizeof(char) * 20);
        if (pos > 0) {          //存在后缀名的文件
                strncpy(pre_name, name, pos);
		pre_name[sizeof(pre_name)] = '\0';
                strncpy(suffix, name + pos, strlen(name) - pos);
		suffix[sizeof(suffix)] = '\0';
        }
        //printf("suffix = %s\n", suffix);	
        //printf("pre_name = %s\n", pre_name);

	//whether last char of path is /
	strcat(filename, path);	
	if (path[strlen(path) - 1] != '/') {
		strcat(filename, "/");
	}
	strcat(filename, name);
	//判断文件是否存在，如存在需要额外处理
	if (access(filename, F_OK) != -1) {
		time_t tim;
		struct tm *at;
		char now[80];
		time(&tim);
		at = localtime(&tim);
		strftime(now, sizeof(now), "%Y%m%d%H%M%S", at);
		strcat(tmpfilename, path);
		if (path[strlen(path) - 1] != '/') {
                	strcat(tmpfilename, "/");
        	}
		strcat(tmpfilename, pre_name);
		strcat(tmpfilename, "_");
		strncat(tmpfilename, now, strlen(now));
		strcat(tmpfilename, suffix);
		memset(&filename, 0, sizeof(filename));
		//printf("after filename = %s\n", filename);
		//printf("tmpfilename=%s\n", tmpfilename);
		strcpy(filename, tmpfilename);
		filename[sizeof(filename)] = '\0';
	}
	ffd = open(filename, O_CREAT | O_RDWR, 0666);
	if (ffd < 0) {
		perror("open file error!");
		exit(1);
	}
	//packft(name, rnd, filesize, ffd);
	free(name);
	free(tmp);
	free(pre_name);
	free(suffix);
	return;
}

int main(int argc, char * argv[]) {
	struct epoll_event ev, events[SOCKET_NUM];
	struct sockaddr_in clientaddr, serveraddr;
	int epfd;
	int listenfd;		//监听fd
	int maxi;
	int nfds;
	int i;
	int sock_fd, conn_fd;
	char buf[MAXLINE + RNDLEN];
	char bufdup[MAXLINE];
	ft = (FILE_ST *)malloc(sizeof(FILE_ST) *  NUMBER); 

	if (argc != 3) {
		perror("param error!");
		exit(1);
	}
	int SERV_PORT = atoi(argv[1]);
	path = argv[2];

	epfd = epoll_create(256);		//生成epoll句柄
	listenfd = socket(AF_INET, SOCK_STREAM, 0);	//创建套接字
	int on = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
		perror("setsockopt error!");
	}
	ev.data.fd = listenfd;		//设置与要处理事件相关的文件描写叙述符
	ev.events = EPOLLIN | EPOLLOUT;		//EPOLLPRI为获取带外数据信息
	
	epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);	//注冊epoll事件

	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERV_PORT);
	bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));	//绑定套接口
	socklen_t clilen;
	listen(listenfd, LISTENQ);	//转为监听套接字
	int n;
	
	while(1) {
		nfds = epoll_wait(epfd, events, SOCKET_NUM, 1);	//等待事件发生
		for (i = 0; i < nfds; i++) {		//处理所发生的全部事件
			if (events[i].data.fd == listenfd) {	//有新的连接
				clilen = sizeof(struct sockaddr_in);
				conn_fd = accept(listenfd, (struct sockaddr *)&clientaddr, &clilen);
				printf("accept a new client: %s\n", inet_ntoa(clientaddr.sin_addr));
				ev.data.fd = conn_fd;
				ev.events = EPOLLIN;		//设置监听事件为可写
				epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &ev);	//新增套接字
			} else if (events[i].events & EPOLLIN) {	//可读事件
				if ((sock_fd = events[i].data.fd) < 0) {
					continue;
				}
				memset(&buf, 0, sizeof(buf));
				if ((n = recv(sock_fd, buf, MAXLINE + RNDLEN, 0)) < 0) {
					if (errno == ECONNRESET) {
						close(sock_fd);	
						events[i].data.fd = -1;
					} else {
						perror("read file error!");
						exit(1);
					}
				} else if (n == 0) {
					close(sock_fd);
					close(ffd);
					printf("End... %s finished\n", filename);
					continue;	
					events[i].data.fd = -1;
				}
				//先判断是否是发送的文件名和文件大小
				int isFile = isFileName(buf);
				if (isFile == 1) {
					createFile(buf);
				} else {
					lseek(ffd, 0, SEEK_END);
                                        int res = write(ffd, buf, n);
                                        if (res != n) {
                                                perror("transfer errors!");
                                                exit(1);
                                        }
				}
				
				ev.data.fd = sock_fd;
				ev.events = EPOLLOUT;
				epoll_ctl(epfd, EPOLL_CTL_MOD, sock_fd, &ev);	//改动监听事件为可读
			} else if (events[i].events & EPOLLOUT) {	//可写事件
				sock_fd = events[i].data.fd;
				//printf("OUT\n");
				//send(sock_fd, "ready", 5, 0);
				ev.data.fd = sock_fd;
				ev.events = EPOLLIN;
				epoll_ctl(epfd, EPOLL_CTL_MOD, sock_fd, &ev);
			}
		}
	}
	free(ft);
	return 0;
}
