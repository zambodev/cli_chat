#include "client.h"


static fd_set sigfd;

int client_connect(client_t *client)
{
	int retval;
	size_t bytes;
	struct timeval tv;

	if(sock_connect(client->sock) == -1) return -1;

	do
	{
		FD_ZERO(&sigfd);
		FD_SET(client->sock->fd, &sigfd);

		tv.tv_sec = 0;
		tv.tv_usec = 50000;

		if(select(client->sock->fd+1, NULL, &sigfd, NULL, &tv) == -1) return -1;
	}
	while(!FD_ISSET(client->sock->fd, &sigfd));

	bytes = strlen(client->sock->conf.username)+1;
	if(sock_write(client->sock, client->sock->conf.username, &bytes) == -1) return -1;

	return 0;
}

int client_read(client_t *client)
{
	size_t bytes;
	char *buffer = NULL;
	struct timeval tv;

	FD_ZERO(&sigfd);
	FD_SET(client->sock->fd, &sigfd);

	tv.tv_sec = 0;
	tv.tv_usec = 50000;

	if(select(client->sock->fd+1, &sigfd, NULL, NULL, &tv) <= 0 
		|| !FD_ISSET(client->sock->fd, &sigfd)
		|| sock_read(client->sock, &buffer, &bytes) == -1)
		return 0;

	if(strncmp(buffer, "/quit", 5) == 0) return -1;

	struct winsize win;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);

	printf("\x1b[u%s\x1b[s\x1b[%d;1H", buffer, win.ws_row);
	fflush(stdout);

	if(buffer != NULL) free(buffer);
	
	return 0;
}

int client_write(client_t *client)
{
	size_t bytes;
	struct timeval tv;

	FD_ZERO(&sigfd);
	FD_SET(0, &sigfd);

	tv.tv_sec = 0;
	tv.tv_usec = 50000;

	if(select(1, &sigfd, NULL, NULL, &tv) <= 0 || !FD_ISSET(0, &sigfd)) return -1;

	char buffer[1024] = {0};
	if((bytes = read(0, buffer, 1024)) > 0)
	{
		do
		{
			FD_ZERO(&sigfd);
			FD_SET(client->sock->fd, &sigfd);

			tv.tv_sec = 0;
			tv.tv_usec = 50000;
			
			select(client->sock->fd+1,NULL, &sigfd, NULL, &tv);
		}
		while(!FD_ISSET(client->sock->fd, &sigfd));

		++bytes;
		if(sock_write(client->sock, buffer, &bytes) == -1) return -1;

		if(strncmp(buffer, "/quit", 5) == 0) return 1;
		printf("\x1b[1T\x1b[2K");
		fflush(stdout);
	}

	return 0;
}
