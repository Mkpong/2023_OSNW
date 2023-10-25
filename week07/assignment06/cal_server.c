#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <signal.h>

#define PORT 3600

struct cal_data
{
        int left_num;
        int right_num;
        char op;
        int result;
        short int error;
};

void handle_sigpipe(int signo) {
    // 여기에 원하는 코드를 추가하여 SIGPIPE를 처리하십시오.
    // 예: 로그 메시지를 출력하거나 특정 변수의 값을 변경합니다.
    printf("SIGPIPE received!\n");
}

int main(int argc, char **argv)
{
        struct sockaddr_in client_addr, sock_addr;
        int listen_sockfd, client_sockfd;
        int addr_len;
        struct cal_data rdata;
        int left_num, right_num, cal_result;
        short int cal_error;


	// signal processing
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_sigpipe;
	sigaction(SIGPIPE, &sa, NULL);
	// signal processing
	
	printf("Original Struct size : %ld\n" , sizeof(rdata));


        if( (listen_sockfd  = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
                perror("Error ");
                return 1;
        }

        memset((void *)&sock_addr, 0x00, sizeof(sock_addr));
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        sock_addr.sin_port = htons(PORT);

        if( bind(listen_sockfd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == -1)
        {
                perror("Error ");
                return 1;
        }

        if(listen(listen_sockfd, 5) == -1)
        {
                perror("Error ");
                return 1;
        }

        for(;;)
        {
                addr_len = sizeof(client_addr);
                client_sockfd = accept(listen_sockfd,
                        (struct sockaddr *)&client_addr, &addr_len);
                if(client_sockfd == -1)
                {
                        perror("Error ");
                        return 1;
                }
		printf("New Client Connect : %s\n", inet_ntoa(client_addr.sin_addr));
		while(1){
			int readsize =  read(client_sockfd, (void *)&rdata, sizeof(rdata));
			printf("### readsize %d\n", readsize);
			sleep(1);
                	//if(readsize == 0)
			//{
			//	write(1, "force disconnected...\n",22);
			//	close(client_sockfd);
			//	break;
			//}

			
			cal_result = 0;
                	cal_error = 0;
			left_num = ntohl(rdata.left_num);
			right_num = ntohl(rdata.right_num);
			write(1, "Result\n" , 7);

			//left num 출력
			printf("%02x %02x %02x %02x " , (left_num>>24)&0xFF, (left_num>>16)&0xFF, (left_num>>8)&0xFF, left_num&0xFF);
			
			//right num 출력
			printf("%02x %02x %02x %02x ", (right_num>>24)&0xFF, (right_num>>16)&0xFF, (right_num>>8)&0xFF, right_num&0xFF);

			//operator 출력
			printf("%02x %02x %02x %02x " , (rdata.op)&0xFF , 0x00 , 0x00, 0x00);

			//result 출력
			printf("%02x %02x %02x %02x " , (cal_result>>24)&0xFF, (cal_result>>16)&0xFF, (cal_result>>8)&0xFF, cal_result&0xFF);

			//error code 출력
			printf("%02x %02x %02x %02x\n" , (cal_error>>8)&0xFF, cal_error&0xFF, 0x00, 0x00);
				
                	switch(rdata.op)
                	{
                        	case '+':
                                	cal_result = left_num + right_num;
                                	break;
                        	case '-':
                                	cal_result = left_num  - right_num;
                                	break;
                        	case 'x':
                                	cal_result = left_num * right_num;
                                	break;
                        	case '/':
                                	if(right_num == 0)
                                	{
                                        	cal_error = 2;
                                        	break;
                                	}
                                	cal_result = left_num / right_num;
                                	break;
                        	default:
                                	cal_error = 1;

			}
				
			printf("%d %c %d = %d\n", left_num, rdata.op, right_num, cal_result);

			//left_num 출력	
			printf("%02x %02x %02x %02x " , (left_num>>24)&0xFF, (left_num>>16)&0xFF, (left_num>>8)&0xFF, left_num&0xFF);

			//right_num 출력
			printf("%02x %02x %02x %02x ", (right_num>>24)&0xFF, (right_num>>16)&0xFF, (right_num>>8)&0xFF, right_num&0xFF);

			//operator 출력
			printf("%02x %02x %02x %02x " , (rdata.op)&0xFF , 0x00 , 0x00, 0x00);
			
			//result 출력
			printf("%02x %02x %02x %02x " , (cal_result>>24)&0xFF, (cal_result>>16)&0xFF, (cal_result>>8)&0xFF, cal_result&0xFF);
			
			//error 출력
			printf("%02x %02x %02x %02x\n" , (cal_error>>8)&0xFF, cal_error&0xFF, 0x00, 0x00);
			
			if(rdata.op == '$'){
				write(1, "Disconnected...\n", 16);
				close(client_sockfd);
				break;
			}
			
			rdata.result = htonl(cal_result);
                	rdata.error = htons(cal_error);
			printf("client_sockfd = %d\n", client_sockfd);
			int writesize = write(client_sockfd, (void *)&rdata, sizeof(rdata));
			if (writesize == -1){
				    perror("Write error");
    				close(client_sockfd);
    				break;
			}else{
					printf("writesize = %d\n", writesize);
			}

        	} // the end of while

	}

        close(listen_sockfd);
        return 0;
}

