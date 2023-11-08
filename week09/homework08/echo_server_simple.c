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


/* 새로운 클라이언트가 연결되면 연결리스트의 뒤에 구조체 추가 */
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

/* 각 클라이언트의 마지막 문자열을 연결한 최종 문자열 반환 */
void getString(char *str){
	node *temp = head;
	while(temp != NULL){
		strcat(str, temp->data);
		temp = temp->next;
	}
}

/* quit가 왔을 때 pid번호를 통해 연결 리스트에서 삭제 */
void deleteConnection(int pid){
	node *temp = head;
	node *prev = NULL;
	// 첫번째 노드일 경우 제거
	if(temp != NULL && temp->pid == pid){
		head = temp->next;
		free(temp);
		printf("%d - disconnected\n", pid);
		return ;
	}
	prev = temp;
	temp = temp->next;
	while(temp != NULL){
		if(temp->pid == pid){
			if(tail->pid == temp->pid){
				prev->next = NULL;
				tail = prev;
				printf("%d - disconnected\n", pid);
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
					memset(str, 0x00, MAXLINE);
					/* quit이면 문자열을 가져오고 연결리스트에서 연결 삭제 */
					if(strcmp(buf, "quit ")== 0){
						getString(str);
						deleteConnection(temp->pid);
					}
					/* quit가 아니면 마지막 문자열을 업데이트 한 후 문자열 가져오기 */
					else{
						strcpy(temp->data, buf);
						getString(str);
					}
					printf("Total String : %s\n" , str);
					/* 모든 클라이언트(자식서버)에게 최종 문자열 전송 */
					node *wtemp = head;
					while(wtemp != NULL){
						write(wtemp->wfd, str, strlen(str));
						wtemp = wtemp->next;
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
		/* 파이프 생성 */
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
			FD_ZERO(&readfds);
			while(1)
			{
				memset(buf, 0x00, MAXLINE);
				
				FD_ZERO(&readfds);
				FD_SET(client_fd, &readfds);
				FD_SET(rfd_child, &readfds);
				retval = select(256, &readfds, NULL, NULL, &timeout);
				if(retval == -1){
					perror("select fail");
					return 0;
				}else if(retval == 0){
					perror("time out");
					return 0;
				}
				else{
					/* 클라이언트로 부터 받은 데이터를 부모서버로 전송 */
					if(FD_ISSET(client_fd, &readfds)){
						if(read(client_fd, buf, MAXLINE) <=  0){
							perror("Read Fail");
							return 0;
						}
						buf[strlen(buf)-1] = ' ';
						/* quit가 입력되면 부모에게 데이터 송신 후 종료 */
						if(strcmp("quit ", buf) == 0){
							write(wfd_child, buf, strlen(buf));
							break;
						}
						else{
							printf("Read Data %s(%d) : %s\n", inet_ntoa(client_addr.sin_addr), getpid(), buf);
						}
						write(wfd_child, buf, sizeof(buf));	
					}
					/* 부모 서버로 부터 받은 데이터를 클라이언트에게 전송 */
					else if(FD_ISSET(rfd_child, &readfds)){
						if(read(rfd_child, buf, MAXLINE) <= 0){
							perror("Read Fail");
							return 0;
						}
						write(client_fd, buf, MAXLINE);
					}
				}

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
