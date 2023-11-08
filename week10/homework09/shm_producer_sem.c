#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXLINE 1024

union semun
{
	int val;
};

typedef struct data{
	int data1, data2;
	char op;
	int result;
	short int error;
}data;

int main(int argc, char **argv)
{
	int shmid;
	int semid;
	data *cal_data;

	char *input = (char*)malloc(MAXLINE); // 입력받을 문자열
	void *shared_memory = NULL;        // 공유 메모리
	union semun sem_union;

	struct sembuf sem1open = {0, -1, SEM_UNDO};
	struct sembuf sem1close = {0, 1, SEM_UNDO};
	struct sembuf sem2open = {1, -1, SEM_UNDO};
	struct sembuf sem2close = {1, 1, SEM_UNDO};

	// shared memory definition
	shmid = shmget((key_t)1234, sizeof(data), 0666|IPC_CREAT);
	if (shmid == -1)
	{
		return 1;
	}
	
	// semaphore definition - 2개의 세마포어 배열 사용
	semid = semget((key_t)3477, 2, IPC_CREAT|0666);
	if(semid == -1)
	{
		return 1;
	}

	// shared memory initialized
	shared_memory = shmat(shmid, NULL, 0);
	if (shared_memory == (void *)-1)
	{
		return 1;
	}

	cal_data = (data *)shared_memory;

	// semaphore initialize
	sem_union.val = 0;
	if (semctl( semid, 0, SETVAL, sem_union) == -1){
		printf("semctl error\n");
		return 1;
	}
	if (semctl( semid, 1, SETVAL, sem_union) == -1){
		printf("semctl error\n");
		return 1;
	}


	while(1)
	{
		memset(input, 0x00, MAXLINE);
		data local_data;
		if(semop(semid, &sem1open, 1) == -1){
			return 1;
		}
		write(1, "Input(x y op) : " , 17);
		read(0, input, MAXLINE);


		// input data slicing
		char *token = strtok(input, " \n");
		local_data.data1 = atoi(token);
		token = strtok(NULL, " \n");
		local_data.data2 = atoi(token);
		token = strtok(NULL, " \n");
		local_data.op = token[0];
		local_data.result = 0;
		local_data.error = 0;

		// write shared memory
		cal_data->data1 = local_data.data1;
		cal_data->data2 = local_data.data2;
		cal_data->op = local_data.op;
		cal_data->result = local_data.result;
		cal_data->error = local_data.error;

		semop(semid, &sem2close, 1);
		
		semop(semid, &sem1open, 1);

		//read shared memory
		local_data.result = cal_data->result;
		local_data.error = cal_data->error;

		

		if(local_data.error == -1){
			write(1, "Cal Error\n", 11);
		}else if(local_data.error == 1){
			write(1, "Cannot Divide Zero\n", 20);
		}else{
			printf("Result : %d\n" , local_data.result);
		}
	}
	return 1;
}

