#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <unistd.h>
#include "lib.h"
#include "listfxns.h"

#define KEY 1234
#define playlist_name "PLAYLIST.txt"

//Creates New Playlist File and Semaphore
//Returns Semaphore descriptor
int create_playlist(){
  printf("Creating Playlist File...\n");
  int fd = open(playlist_name, O_EXCL|O_CREAT, 0777);
  if (fd == -1) printf("Error: %s\n", strerror(errno));
  printf("Creating Playlist Semaphore...\n");
  int sd = semget(KEY, 1, 0777|IPC_CREAT|IPC_EXCL);
  if (sd == -1) printf("Error: %s\n", strerror(errno));
  union semun semopts;
  semopts.val = 1;
  semctl(sd,0,SETVAL,semopts);
  printf("Created Semaphore: %d\n", sd);
  close(fd);
  return sd;
}

int get_playlist_size(){
  printf("Getting size of playlist\n");
  struct stat sb;
  stat(playlist_name, &sb);
  return sb.st_size;
}

char* view_playlist(int sd){
  //Down the semaphore
	struct sembuf sb;
	sb.sem_op = -1;
	sb.sem_num = 0;
	sb.sem_flg = SEM_UNDO;
	semop(sd, &sb, 1);

  int size = get_playlist_size();
  printf("Viewing Playlist\n");
  int fd = open(playlist_name, O_RDONLY, 0777);
  if (fd == -1) printf("Error: %s\n", strerror(errno));
  char* buffer = (char*) calloc(1, size);
  int result = read(fd, buffer, size);
  if (result == -1) printf("Error: %s\n", strerror(errno));
  close(fd);

  //Up the semaphore
  sb.sem_op = 1;
  semop(sd, &sb, 1);
  return buffer;
}

int add_to_playlist(char* song, int sd){
  //Down the semaphore
	struct sembuf sb;
	sb.sem_op = -1;
	sb.sem_num = 0;
	sb.sem_flg = SEM_UNDO;
	semop(sd, &sb, 1);

  printf("Adding %s to Playlist\n", song);
  int fd = open(playlist_name, O_WRONLY, 0777);
  if (fd == -1) printf("Error: %s\n", strerror(errno));
  lseek(fd, 0, SEEK_END);
  int result = write(fd, song, strlen(song)*sizeof(char));
  if (result == -1) printf("Error: %s\n", strerror(errno));
  result = write(fd, "\n", sizeof(char));
  if (result == -1) printf("Error: %s\n", strerror(errno));
  close(fd);

  //Up the semaphore
  sb.sem_op = 1;
  semop(sd, &sb, 1);
  return 0;
}

int main(){
  int sd = create_playlist();
  add_to_playlist("Bodak Black", sd);
  add_to_playlist("Hey Jude", sd);
  printf("\n%s\n", view_playlist(sd));
}


/* 
  0. Client sends a vote for a song to the server. If the file DNE on the server, it is sent. 
  1. After a specified amount of time, the song is added to the playlist and voting closes. 
  2. Server plays the playlist. While the first song plays, people can vote for the second song. 

  Client: 
    -Needs to be able to create a playlist, order the playlist, add songs to the playlist, remove songs 
    from the playlist and play the playlist. 
    -When adding song, write to the file that has the songs to play. Then, play it from the library (a directory).
    -To stop playing, send a kill signal. 
  Server: 
    -Receives votes from the client, and receives files from the client. Adds these to the library/playlist.
    -When it receives them, it writes to the file in order of votes. 
*/