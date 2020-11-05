#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

//回傳資料給瀏覽器
void go_back_go_back(int sockfd, char *request_file)
{

	ssize_t ret;
	int filefd;
	char response[8192] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
	char *str;

	if ((filefd = open(request_file, O_RDONLY)) == -1)
	{
		printf("Fail to open file : %s\n\n", request_file);
		return;
	}

	// 回傳
	write(sockfd, response, strlen(response));
	while ((ret = read(filefd, response, 8192)) > 0)
	{
		write(sockfd, response, ret);
	}
}

// 抓參數
char *find_parameter(char *input, char begin, char end)
{
	char *char1;
	char *char2;

	char1 = strchr(input, begin);
	char2 = strchr(char1 + 1, end);
	if (char1 == NULL || char2 == NULL)
		return NULL;

	int len = (int)(char2 - char1);
	char *str = malloc(len);
	memset(str, '\0', len);

	strncpy(str, char1 + 1, len - 1);
	return str;
}

// 每次讀入一行
int Read_One_Line(int fd, char *buffer, int len)
{
	int ret;
	for (int i = 0; i < len; i++)
	{
		ret = read(fd, &buffer[i], 1);
		if (ret != 1)
			return -1;
		if (buffer[i] == '\n')
		{
			buffer[i + 1] = '\0';
			return strlen(buffer);
		}
	}
}

void handle(int sockfd)
{
	ssize_t ret;
	char buffer[8192];
	char link_type[10];
	char request_file[1024];

	//讀HTTP HEADER
	ret = Read_One_Line(sockfd, buffer, 8192);
	sscanf(buffer, "%s %s", &link_type, &request_file[1]);
	request_file[0] = '.';

	if (!strcmp(link_type, "POST"))
	{
		int fp;
		int file_len = 0;
		int remain_size = 0;
		char *str;
		char *t_size;
		char *file_name;

		while (1)
		{
			Read_One_Line(sockfd, buffer, 8192);
			if (!strncmp(buffer, "\r\n", 2))
				break;

			if ((str = strstr(buffer, "Content-Length:")) != NULL)
			{
				t_size = find_parameter(str, ' ', '\r');
				remain_size = atoi(t_size);
			}
		}

		Read_One_Line(sockfd, buffer, 8192);
		remain_size -= strlen(buffer) * 2 + 2;

		// get file name
		remain_size -= Read_One_Line(sockfd, buffer, 8192);
		if ((str = strstr(buffer, "filename=")) != NULL)
		{
			file_name = find_parameter(str, '\"', '\"');
			if (strlen(file_name) <= 0)
			{
				printf("我的檔案呢!\n");
				char buffer[8192];
				while (remain_size > 0)
				{
					if (remain_size > 8192)
						remain_size -= read(sockfd, buffer, 8192);
					else
						remain_size -= read(sockfd, buffer, remain_size);
				}
				read(sockfd, buffer, 8192);
				go_back_go_back(sockfd, "./web.html");
				return;
			}
			else
				printf("I got the filename!!!!!!: %s\n", file_name);
		}

		// 跳過類型跟\r\n
		remain_size -= Read_One_Line(sockfd, buffer, 8192);
		remain_size -= read(sockfd, buffer, 2) * 2;

		// 開檔
		char pathname[100];
		sprintf(pathname, "./%s", file_name);
		fp = open(pathname, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);

		file_len = remain_size;

		// 一直讀寫資料 //
		while (remain_size > 0)
		{
			//檔案大小超過8192
			if (remain_size > 8192)
			{
				ret = read(sockfd, buffer, 8192);
				//printf("read:%d Byte\n", ret);

				ret = write(fp, buffer, ret);
				remain_size -= ret;
				//printf("write:%d Byte, remain:%d Byte\n", ret, remain_size);
			}
			//檔案大小超過8192
			else
			{

				ret = read(sockfd, buffer, remain_size);
				printf("final read:%d Byte\n", ret);

				ret = write(fp, buffer, ret);
				remain_size -= ret;
				//printf("final write:%d Byte, remain:%d Byte\n", ret, remain_size);
			}
		}

		printf("oh ya,存了%s, 大小:%d Byte, 再來回到首頁\n", file_name, file_len);

		read(sockfd, buffer, 8192);

		free(t_size);
		free(file_name);
		close(fp);

		go_back_go_back(sockfd, "./web.html");
		return;
	}

	if (!strcmp(link_type, "GET"))
	{
		while (1)
		{
			Read_One_Line(sockfd, buffer, 8192);
			if (!strncmp(buffer, "\r\n", 2))
				break;
		}
		go_back_go_back(sockfd, request_file);
		return;
	}
}

