#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

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

	void *shared_memory = NULL;

	struct sembuf sem1open = {0, -1, SEM_UNDO};
	struct sembuf sem1close = {0, 1, SEM_UNDO};
	struct sembuf sem2open = {1, -1, SEM_UNDO};
	struct sembuf sem2close = {1, 1, SEM_UNDO};

	//shared memory deficition
	shmid = shmget((key_t)1234, sizeof(int), 0666);
	if (shmid == -1)
	{
		perror("shmget failed : ");
		exit(0);
	}
	//semaphore definition - 기존 semaphore에 access
	semid = semget((key_t)3477, 0, 0666);
	if(semid == -1)
	{
		perror("semget failed : ");
		return 1;
	}

	shared_memory = shmat(shmid, NULL, 0);
	if (shared_memory == (void *)-1)
	{
		perror("shmat failed : ");
		exit(0);
	}

	cal_data = (data *)shared_memory;

	while(1)
	{
		data local_data;

		semop(semid, &sem1close, 1); // A프로세스가 실행될 수 있게 해준다.
		// B 프로세스를 실행시켜줄 때 까지 대기
		if(semop(semid, &sem2open, 1) == -1){
			perror("semop error : ");
		}
		

		// read shared memory
		local_data.data1 = cal_data->data1;
		local_data.data2 = cal_data->data2;
		local_data.op = cal_data->op;
		local_data.result = 0;
		local_data.error = 0;
		
		switch(local_data.op){
			case '+':
				local_data.result = local_data.data1 + local_data.data2;
				break;
			case '-':
				local_data.result = local_data.data1 - local_data.data2;
				break;
			case 'x':
				local_data.result = local_data.data1 * local_data.data2;
				break;
			case '/':
				if(local_data.data2 == 0){
					local_data.error = 1;
					break;
				}
				local_data.result = local_data.data1 / local_data.data2;
				break;
			default:
				local_data.error = -1;
		}
		printf("%d\n" , local_data.error);
		cal_data->result = local_data.result;
		cal_data->error = local_data.error;

		semop(semid, &sem1close, 1); // C 프로세스가 실행될 수 있게 해준다.
		
	}
	return 1;
}

