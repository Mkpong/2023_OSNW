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
	int listen_fd, client_fd;
	struct sockaddr_in serveraddr, clientaddr;
	int client_len = sizeof(clientaddr);
	char buf[MAXBUF];

	//create socket
	listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	//setting sockaddr
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(PORTNUM);

	//bind
	bind(listen_fd , (struct sockaddr *)&serveraddr , sizeof(serveraddr));

	//listen
	listen(listen_fd, 5);

	while(1){
		client_fd = accept(listen_fd, (struct sockaddr *)&clientaddr , &client_len);

		read(client_fd , buf, MAXBUF);
		printf("Read Data : %s\n" , buf);

		close(client_fd);
	}
	close(listen_fd);
	return 0;
}

