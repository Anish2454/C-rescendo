#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
int main(){
	char* commandz[3];
	commandz[0] = "mpg123";
	commandz[1] = "ducksong.mp3";
	commandz[2] = NULL;
	execvp("/usr/bin/mpg123", commandz);
}
