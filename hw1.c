#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void sig_chld(int signo)
{
	pid_t pid;
	int stat;

	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
		printf("child %d is dead, no zombie, world peace\n\n", pid);
	return;
}

int main(int argc, char **argv)
{
	int listenfd, connfd;
	pid_t pid;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	void sig_chld(int);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
		error("setsockopt(SO_REUSEADDR) failed");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(8080);

	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	listen(listenfd, 64);

	signal(SIGCHLD, sig_chld);

	while (1)
	{
		clilen = sizeof(cliaddr);
		if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0)
		{
			if (errno == EINTR)
				continue;
			else
				return;
		}

		pid = fork();
		if (pid == 0)
		{
			close(listenfd);
			printf("child pid = %d\n", getpid());
			handle(connfd);
			exit(0);
		}
		close(connfd);
	}
}
