#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAXBUF 1024
#define PORTNUM 3600

int main(){
	int server_fd;
	struct sockaddr_in serveraddr;
	char buf[MAXBUF];
	int client_len;
	
	//create socket
	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	memset(&serveraddr , 0x00, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serveraddr.sin_port = htons(PORTNUM);

	client_len = sizeof(serveraddr);

	connect(server_fd, (struct sockaddr *)&serveraddr , client_len);
	
	memset(buf, 0x00, MAXBUF);
	read(0, buf, MAXBUF);

	write(server_fd, buf, strlen(buf));

	close(server_fd);
	return 0;
}
