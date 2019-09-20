#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <sys/mman.h>
#include <sys/select.h>
#define SEP "===sunder==="
#define MAXLINE 4096

char* strrev(char *s) {
	char *h = s;
	char* t = s;  
        char ch;
	while(*t++){};  
        t--;
        t--;

	while(h < t) {  
            ch = *h;  
            *h++ = *t;    /* h向尾部移动 */  
            *t-- = ch;    /* t向头部移动 */  
        }  
       
        return(s);  
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

char * fileStrDeal(char *filename, char *filestr) {
	//发送文件名及文件大小
	//filestr = (char *)malloc(sizeof(char) * 300);
	unsigned long filesize = -1;
        char size[100];
        struct stat statbuff;
        if (stat(filename, &statbuff) < 0) {
                perror("file error\n");
                exit(1);
        } else {
                filesize = statbuff.st_size;
                sprintf(size, "%lld", filesize);
        }
        strcpy(filestr, filename);
	//剔除所有路径斜杠
	int pos_rev = strchar(strrev(strdup(filestr)), '/');
	int pos = strlen(filestr) - pos_rev;
	char *realname = (char *)malloc(sizeof(char) * 100);
	strcpy(realname, filestr + pos);

        strcat(realname, SEP);
        strcat(realname, size);
	memset(&filestr, 0, sizeof(filestr));
	filestr = realname;
	return filestr;
}

void sleep_ms(int msec) {
	struct timeval timeout = {0, msec};	
}

int main(int argc, char * argv[]) {
	int sockfd;
	int new_fd;
	char *filename;
	char *filestr = (char *)malloc(sizeof(char) * 300);
	char buff[MAXLINE];
	char buffer[MAXLINE];
	struct sockaddr_in addr;

	if (argc != 4) {
		perror("the parameters should be host port file");
		exit(1);
	}

	int port = atoi(argv[2]);
	filename = argv[3];
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(argv[1]);
	bzero(&(addr.sin_zero), 8);

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	
	//socket延迟关闭
	struct linger so_linger;
        so_linger.l_onoff = 1;
        so_linger.l_linger = 10;
	setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));

	while (connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1);

	//发送文件名及文件大小
	filestr = fileStrDeal(filename, filestr);
	int send_size = send(sockfd, filestr, strlen(filestr), 0);
	if (send_size < 0) {
		perror("transfer file failed!");
		exit(1);
	}
	sleep(1);
	
	
	//先发送文件名及文件大小
	//接受对方返回是否准备好
	//继续上传
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		perror("open file error!");
		exit(1);
	} else {
		bzero(buffer, MAXLINE);
		int read_size = 0;
		while ((read_size = fread(buffer, sizeof(char), MAXLINE, fp)) > 0) {
			if (send(sockfd, buffer, read_size, 0) < 0) {
				perror("transfer file error!");
				exit(1);
			}
			bzero(buffer, sizeof(buffer));
		}
		fclose(fp);
	}
	printf("----------transfer ok---------\n");
	close(sockfd);
	return 0;
}
