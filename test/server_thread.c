#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

#define MAX_CLIENTS 50
#define MAXLINE 1024
#define PORTNUM 3600

// pthread_mutex_t t_lock;

//struct cal_struct client_struct[MAX_CLIENTS];

struct cal_struct
{
	int left_num;
	int right_num;
	char op;
	int result;
	char str[MAXLINE];
	int client_fd;
	short int error;
	short int flag;

	pthread_mutex_t lock;
	pthread_cond_t cond;
};

struct cal_struct client_struct[MAX_CLIENTS];
// 계산 기능 함수, 문자열 반환 함수
void calculate(struct cal_struct *cal_data, char* result)
{	
	time_t now = time(NULL);
	int left_num = cal_data->left_num;
	int right_num = cal_data->right_num;

	switch(cal_data->op)
	{
       		case '$':
            		cal_data->error = -1;
                        break;
                case '+':
                        cal_data->result = left_num + right_num;
                        break;
                case '-':
                        cal_data->result = left_num  - right_num;
                        break;
                case 'x':
                        cal_data->result = left_num * right_num;
                        break;
                case '/':
                        if(right_num == 0)
                        {
				cal_data->result = 0;
                	        cal_data->error = 2;
                                break;
                        }
                        cal_data->result = left_num / right_num;
                        break;
                default:
                        cal_data->error = 1;
			break;
        }
	if(strcmp(cal_data->str, "quit\n") == 0)
		strcpy(result, "quit");
	else{
	// client로 보낼 문자열을 함수에서 생성
	//printf("%d \n", cal_data->result);
	snprintf(result, MAXLINE, "%d %d %c %d %s", cal_data->left_num, 
		cal_data->right_num, cal_data->op, cal_data->result, ctime(&now));
	}
};

void * thread_producer(void *data)
{
	struct cal_struct *p_data = ((struct cal_struct *)data);
	printf("producer client fd : %d\n", p_data->client_fd);
	int client_fd = p_data->client_fd;
	int readn;
	struct sockaddr_in client_addr;
	socklen_t addrlen;
	addrlen = sizeof(client_addr);
	getpeername(client_fd, (struct sockaddr *)&client_addr, &addrlen);
	
        char buf[MAXLINE];
	char resultBuf[MAXLINE];
        char *token;
	
        p_data->flag = 0;

        memset(buf, 0x00, MAXLINE);
	
//	printf("%d\n", p_data.client_fd);
	printf("Before Read..\n");
	while((readn = read(client_fd, buf, MAXLINE)) > 0)
        {
//		pthread_mutex_lock(&p_data.lock);
        	printf("Read Data %s(%d) : %s\n", 
			inet_ntoa(client_addr.sin_addr),
        		client_addr.sin_port, buf);
        
	// 사용자에게서 입력받은 문자열 토큰으로 분리 후 구조체에 배정
                               
       	token = strtok(buf, " ");                        
	p_data->left_num = atoi(token);                      
	token = strtok(NULL, " ");                                
	p_data->right_num = atoi(token);
	token = strtok(NULL, " ");
	p_data->op = token[0];
	token = strtok(NULL, " ");
	strcpy(p_data->str, token);
	p_data->result = 0;

	p_data->flag = 1; // shared memory에 접근함을 의미함.
	
	printf("shm_write success\n");
	printf("%s\n" , resultBuf);
	//pthread_mutex_unlock(p_data->lock);
	//pthread_cond_broadcast(&t_cond);
        }
	close(client_fd);
}

void * thread_consumer(void *data)
{
	struct cal_struct *c_data = ((struct cal_struct *)data);
	int client_fd = c_data->client_fd;
	int readn;
	socklen_t addrlen;
	struct sockaddr_in client_addr;
	addrlen = sizeof(client_addr);
	getpeername(client_fd, (struct sockaddr *)&client_addr, &addrlen);
	

	char *result = malloc(MAXLINE);
	memset(result, 0x00, MAXLINE);

	printf("consumer client fd  : %d \n", client_fd);

	while(1)
	{	
		
		// 초기 값이 0이기 때문에 아무런 입력이 없을 경우
		if(c_data->flag == 0)
		{
			sleep(5);
			continue;
		}
		else if(c_data->flag == -1) // flag 값이 -1 일때만 실행
                {
			if(strcmp(result, "quit") == 0)
			{
				write(client_fd, result, MAXLINE);
				close(client_fd);
				exit(0);
			}

			sleep(4); // 시간 간격 9
			calculate(c_data, result);
                      	
			if((write(client_fd, result, MAXLINE)) == -1)
				break;


			continue;
		}
		else if(c_data->flag == 1)
		{
		calculate(c_data, result);
		c_data->flag = -1; // 접근한 구조체 flag -1 로변경

		}
		printf("%d\n", c_data->flag);
		if(strcmp(result, "quit") == 0)
		{
			write(client_fd, result, MAXLINE);
			close(client_fd);
			exit(0);
		}
		if((write(client_fd, result, MAXLINE)) == -1)
			break;
		printf("write to client \n");       
		printf("%s\n",result);
	}	
	close(client_fd);
}



int main(int argc, char **argv)
{
	int listen_fd, client_fd;
        pthread_t producer_thread_id, consumer_thread_id;
	socklen_t addrlen;
        int readn;
        struct sockaddr_in client_addr, server_addr;
	int client_num;

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
	
	client_num = 0;

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

		//client_struct[client_num] = (struct cal_struct *)malloc(sizeof(struct cal_struct));

		client_struct[client_num].client_fd = client_fd;
		printf("main %d\n", client_struct[client_num].client_fd);

		// producer_thread

		pthread_create(&producer_thread_id, NULL, thread_producer, (void *)&client_struct[client_num]);
		printf("thread create\n");
		pthread_detach(producer_thread_id);

		// consumer_thread
		pthread_create(&consumer_thread_id, NULL, thread_consumer, (void *)&client_struct[client_num]);
		printf("thread2 create\n");
		pthread_detach(consumer_thread_id);

		client_num++;
	}
	return 0;
}
