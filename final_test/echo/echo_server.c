#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXBUF 1024
#define PORTNUM 3600

int main(){
	int server_fd , client_fd;
	int client_len;
	char buf[MAXBUF];
	struct sockaddr_in clientaddr, serveraddr;
	client_len = sizeof(clientaddr);
	
	//create socket
	server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	memset(&serveraddr , 0x00, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(PORTNUM);

	//bind
	bind(server_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	//listen
	listen(server_fd, 5);

	while(1){
		client_fd = accept(server_fd, (struct sockaddr *)&clientaddr, &client_len);
		printf("New client connected..\n");
		memset(buf, 0x00, sizeof(buf));
		read(client_fd, buf, sizeof(buf));

		printf("Read From Client : %s\n" , buf);
		
		close(client_fd);
	}
	close(server_fd);
	return 0;
}
