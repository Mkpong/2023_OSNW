#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct test{
	int a;
	int b;
	char data[10];
	short c;
};

int main(){
	struct test buf;


	read(0, (void *)&buf, sizeof(buf));
	printf("%d %d %s %d\n", buf.a, buf.b, buf.data, buf.c);

	return 0;
}
	
