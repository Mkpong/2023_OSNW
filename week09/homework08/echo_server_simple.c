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

typedef struct connection{
	int index, pid;
	int rfd, wfd;
	char data[MAXLINE];
	struct connection *next;
}node;

node *head = NULL;
node *tail = NULL;

/* index, pid를 함께 출력 */
void printConnection(){
	int cnt = 0;
	node *temp = head;
	while(temp != NULL){
		printf("%d[%d]\n", temp->index, temp->pid);
		temp = temp->next;
	}
}


void addConnection(int index, int pid, int rfd, int wfd){
	node *box = (node*)malloc(sizeof(node));
	box->index = index;
	box->pid = pid;
	box->wfd = wfd;
	box->rfd = rfd;
	box->next = NULL;
	if(head == NULL){
		head = box;
		tail = box;
		return ;
	}else{
		tail->next = box;
		tail = box;
	}
}

void getString(char *str){
	node *temp = head;
	while(temp != NULL){
		strcat(str, temp->data);
		temp = temp->next;
	}
}

void deleteConnection(int pid){
	node *temp = head;
	node *prev = NULL;
	// 첫번째 노드일 경우 제거
	if(temp != NULL && temp->pid == pid){
		head = temp->next;
		free(temp);
		printf("HEAD - disconnected\n");
		return ;
	}
	prev = temp;
	temp = temp->next;
	while(temp != NULL){
		if(temp->pid == pid){
			if(tail->pid == temp->pid){
				prev->next = NULL;
				tail = prev;
				printf("TAIL - disconnected\n");
			}else {
		        	prev->next=temp->next;
				printf("%d - disconnected\n" , pid);
			}
			free(temp);
			return ;
		}
		prev = temp;
		temp = temp->next;
	}
	printf("Not Found\n");
}

int main(int argc, char **argv)
{
	int listen_fd, client_fd;
	pid_t pid;
	socklen_t addrlen;
	int readn;
	char buf[MAXLINE];
	struct sockaddr_in client_addr, server_addr;
	char *str = (char*)malloc(MAXLINE);
	char PtoC[50], CtoP[50];
	int rfd , wfd, rfd_child, wfd_child;
	int index = 0;
	int flag = 0;

	fd_set readfds;
	struct timeval timeout;
	int retval;

	timeout.tv_sec = 300;
	timeout.tv_usec = 0;

	FD_ZERO(&readfds);

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
		memset(str, 0x00, MAXLINE);
		/* File Descriptor Set -> Reset */
		FD_ZERO(&readfds);
		FD_SET(listen_fd, &readfds);
		node *temp = head;
		while(temp != NULL){
			FD_SET(temp->rfd , &readfds);
			temp = temp->next;
		}
		addrlen = sizeof(client_addr);
		retval = select(256 , &readfds, NULL, NULL, &timeout);
	

		/* Select로 file descriptor Event 관리 */
		if(retval == -1){
			perror("select fail");
			return 1;
		}else if(retval == 0){
			perror("time out");
			return 1;
		}else{
			int flag = 0;
			if(FD_ISSET(listen_fd, &readfds)){
				client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);
			}
			temp = head;
			while(temp != NULL){
				if(FD_ISSET(temp->rfd, &readfds)){
					memset(buf, 0x00, MAXLINE);
					if((read(temp->rfd, buf, MAXLINE))==-1){
						perror("read error");
						return 1;
					}
					if(strcmp(buf, "quit ")== 0){
						getString(str);
						printf("%s\n" , str);
						printConnection();
						deleteConnection(temp->pid);
						printConnection();
						// 자식 서버에게 pipeline으로 write하는 부분
					}
					else{
						strcpy(temp->data, buf);
					}
					flag = 1;
					break;
				}
				temp = temp->next;	
			}
			if(flag == 1) continue;
		}

		if(client_fd == -1)
		{
			printf("accept error\n");
			break;
		}
		sprintf(PtoC, "/tmp/PtoC%d", index);
		sprintf(CtoP, "/tmp/CtoP%d", index);
		mkfifo(PtoC, S_IRUSR|S_IWUSR);
		mkfifo(CtoP, S_IRUSR|S_IWUSR);

		pid = fork();
		if(pid == 0)
		{
			if( (rfd_child = open(PtoC, O_RDWR)) == -1){
				perror("rfd error - child");
				return 0;
			}
			if( (wfd_child = open(CtoP, O_RDWR)) == -1){
				perror("wfd error - child");
				return 0;
			}
			close( listen_fd );
			memset(buf, 0x00, MAXLINE);
			printf("connected-%s(%d)\n" , inet_ntoa(client_addr.sin_addr), getpid());
			while((readn = read(client_fd, buf, MAXLINE)) > 0)
			{
				buf[strlen(buf)-1] = ' ';
				if(strcmp("quit " , buf) == 0){
					//printf("disconnected-%s(%d)\nData : %s\n",inet_ntoa(client_addr.sin_addr), getpid() ,buf);
					write(wfd_child, buf , strlen(buf));
					break;
				}
				else{
					printf("Read Data %s(%d) : %s\n",
						inet_ntoa(client_addr.sin_addr),
						getpid(),buf);

				}
				//개행문자 제거
				write(wfd_child, buf, sizeof(buf));
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
			addConnection(index, pid, rfd, wfd);
			index++;
			close(client_fd);
		}
	}
	return 0;
}
