#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>

#define MAXLINE 1024
#define PORTNUM 3600
#define MAXSTR 4096

typedef struct Node{
	int index;
	int pid;
	int rfd, wfd;
	char data[100];
	struct Node* next;
}node;

node *head = NULL;
node *tail = NULL;

//add client function
void addClient(int index, int pid, int rfd, int wfd, char *data){
	node *newNode = (node*)malloc(sizeof(node));
	newNode->index = index;
	newNode->pid = pid;
	newNode->rfd = rfd;
	newNode->wfd = wfd;
	newNode->next = NULL;
	if(head == NULL){
		head = newNode;
		tail = newNode;
	}
	else{
		tail->next = newNode;
		tail = newNode;
	}
}

void printClient(){
	node *temp = head;
	while(temp != NULL){
		printf("%d " , temp->pid);
		temp = temp->next;
	}
	printf("\n");
}

void delClient(int pid){
	node *temp = head;
	node *prev = NULL;

	while(temp != NULL){
		if(temp->pid == pid){
			if(prev == NULL){
				head = temp->next;
			}else{
				prev->next = temp->next;
			}
			free(temp);
			return;
		}
		prev = temp;
		temp = temp->next;
	}
}	

int main(int argc, char **argv)
{
	int listen_fd, client_fd;
	pid_t pid;
	socklen_t addrlen;
	int readn;
	char buf[MAXLINE];
	struct sockaddr_in client_addr, server_addr;
	char str[4096] = "";
	char PtoC[50], CtoP[50];
	int rfd, wfd;
	int index = 0;
	int cnt = 0;


	if( (listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		return 1;
	}
	memset((void *)&server_addr, 0x00, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORTNUM);

	if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==-1)
	{
		perror("bind error");
		return 1;
	}
	if(listen(listen_fd, 5) == -1)
	{
		perror("listen error");
		return 1;
	}

	signal(SIGCHLD, SIG_IGN);
	while(1)
	{
		addrlen = sizeof(client_addr);
		client_fd = accept(listen_fd,
			(struct sockaddr *)&client_addr, &addrlen);
		if(client_fd == -1)
		{
			printf("accept error\n");
			break;
		}
		node *new = (node*)malloc(sizeof(node));
		sprintf(PtoC, "/tmp/PtoC%d", index);
		sprintf(CtoP, "/tmp/CtoP%d", index);
		mkfifo(PtoC, S_IRUSR|S_IWUSR);
		mkfifo(CtoP, S_IRUSR|S_IWUSR);

		pid = fork();
		if(pid == 0)
		{
			if( (rfd = open(PtoC, O_RDWR)) == -1){
				perror("rfd error - child");
				return 0;
			}
			if( (wfd = open(CtoP, O_RDWR)) == -1){
				perror("wfd error - child");
				return 0;
			}
			close( listen_fd );
			memset(buf, 0x00, MAXLINE);
			while((readn = read(client_fd, buf, MAXLINE)) > 0)
			{
				if(strcmp("quit\n" , buf) == 0){
					printf("disconnected-%s(%d) / Input : %s\n",inet_ntoa(client_addr.sin_addr), client_addr.sin_port,str);
					write(client_fd, str , strlen(str));
					delClient(getpid());
					break;
				}
				else{
					printf("Read Data %s(%d) : %s",
						inet_ntoa(client_addr.sin_addr),
						client_addr.sin_port,buf);

				}
				//개행문자 제거
				buf[strlen(buf)-1] = '\0';
				strcat(str, buf);
				strcat(str, " ");
				//write(client_fd, buf, strlen(buf));
				memset(buf, 0x00, MAXLINE); //buffer initialize
			}
			close(client_fd);
			return 0;
		}
		else if( pid > 0){
			if( (rfd = open(CtoP, O_RDWR)) == -1){
				perror("rfd error - parent");
				return 0;
			}
			if( (wfd = open(PtoC, O_RDWR)) == -1){
				perror("wfd error - parent");
				return 0;
			}
			addClient(index, pid, rfd, wfd, "hello");
			index++;
			cnt++;
			printClient();
			close(client_fd);
		}
	}
	return 0;
}
