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
#include "pipe_networking.h"
//#include "shared_memory.h"
#include "server.h"
#include "parsing.h"
//#include "control.h"
#include <signal.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/wait.h>

struct song_node * playlist;


union semun {
                int              val;
                struct semid_ds *buf;
                unsigned short  *array;
                struct seminfo  *__buf;  
            }; 

//Creates New Playlist File and Semaphore
//Returns Semaphore descriptor

static void sighandler(int signo) {
  if (signo == SIGINT) {
    remove("luigi");
    exit(0);
  }
}

int create_playlist(){
  printf("Creating Playlist...\n");
  playlist = NULL;
  /*
  int fd = open(playlist_name, O_EXCL|O_CREAT, 0777);
  if (fd == -1) printf("Error: %s\n", strerror(errno));
  */
  printf("Creating Playlist Semaphore...\n");
  int sd = semget(KEY, 1, 0777|IPC_CREAT|IPC_EXCL);
  if (sd == -1) printf("Error: %s\n", strerror(errno));
  union semun semopts;
  semopts.val = 1;
  semctl(sd,0,SETVAL,semopts);
  printf("Created Semaphore: %d\n", sd);
  //close(fd);
  return sd;
}

int get_playlist_size(){
  printf("Getting size of playlist\n");
  struct stat sb;
  stat(playlist_name, &sb);
  return sb.st_size;
}

void view_playlist(int sd){
  //Down the semaphore
	struct sembuf sb;
	sb.sem_op = -1;
	sb.sem_num = 0;
	sb.sem_flg = SEM_UNDO;
	semop(sd, &sb, 1);
  printf("view_playlist: sem stuff done\n");
  /*
  int size = get_playlist_size();
  printf("Viewing Playlist\n");
  int fd = open(playlist_name, O_RDONLY, 0777);
  if (fd == -1) printf("Error: %s\n", strerror(errno));
  char* buffer = (char*) calloc(1, size);
  int result = read(fd, buffer, size);
  if (result == -1) printf("Error: %s\n", strerror(errno));
  close(fd); */

  print_list(playlist);

  //Up the semaphore
  sb.sem_op = 1;
  semop(sd, &sb, 1);
  //return buffer;
}

int add_to_playlist(char* name, char* artist, char* file, int sd){
  //Down the semaphore
	struct sembuf sb;
	sb.sem_op = -1;
	sb.sem_num = 0;
	sb.sem_flg = SEM_UNDO;
	semop(sd, &sb, 1);

  playlist = insert_front(playlist, name, artist, file, 0);
  /*
  printf("Adding %s to Playlist\n", song);
  int fd = open(playlist_name, O_WRONLY, 0777);
  if (fd == -1) printf("Error: %s\n", strerror(errno));
  lseek(fd, 0, SEEK_END);
  int result = write(fd, song, strlen(song)*sizeof(char));
  if (result == -1) printf("Error: %s\n", strerror(errno));
  result = write(fd, "\n", sizeof(char));
  if (result == -1) printf("Error: %s\n", strerror(errno));
  close(fd); */

  //Up the semaphore
  sb.sem_op = 1;
  semop(sd, &sb, 1);
  return 0;
}

int vote(int val, char* name, char* artist, int sd){
  //Down the semaphore
	struct sembuf sb;
	sb.sem_op = -1;
	sb.sem_num = 0;
	sb.sem_flg = SEM_UNDO;
	semop(sd, &sb, 1);

  printf("Reached vote()\n");
  playlist = add_votes(playlist, name, artist, val);

  //Up the semaphore
  sb.sem_op = 1;
  semop(sd, &sb, 1);
  return 0;
}

//Sorts playlist and returns file_name of most voted song
char** end_vote(int sd){
  //Down the semaphore
	struct sembuf sb;
	sb.sem_op = -1;
	sb.sem_num = 0;
	sb.sem_flg = SEM_UNDO;
	semop(sd, &sb, 1);

  printf("started endvote\n");
  sort_by_votes(playlist);
  printf("sorted\n");
  //view_playlist(sd);
  char** commands = (char**) calloc(2, sizeof(char*));
  commands[0] = "mpg123";
  char* command = calloc(100, sizeof(char));
  command = playlist -> file_name;
  commands[1] = command;
  if (playlist -> next)
    playlist = playlist -> next; //Removes top song
  //view_playlist(sd);
  //Up the semaphore
  sb.sem_op = 1;
  semop(sd, &sb, 1);
  return commands;
}

void subserver(int from_client, int sd) {
  int to_client = server_connect(from_client);
  char* buff = (char*) calloc((BUFFER_SIZE / sizeof(char)), sizeof(char));
  while(read(from_client, buff, BUFFER_SIZE)){
    printf("[subserver] recieved: [%s]\n", buff);
    char** args = separate_line(buff, "-");
    if (!strcmp(args[0], "vote")){
      char* name = args[1];
      char* artist = args[2];
      if(find_song(playlist, name, artist)){
        //Song Already In Playlist
        vote(1, name, artist, sd);
        printf("Client Voted!\n");
        char resp[100];
        sprintf(resp, "Voted For: %s", name);
        //view_playlist(sd);
        //printf("Viewed Playlist\n");
        write(to_client, resp, sizeof(resp));
      }
      else{
        //SONG NOT IN PLAYLIST - TRANSFER MP3
        char resp[] = "SONG DOESNT EXIST";
        write(to_client, resp, sizeof(resp));
      }
    }
    else if (!strcmp(args[0], "view")){
      //Redirect view_playlist output from stdout to pipe
      int stdout = dup(STDOUT_FILENO);
    	int before = dup2(to_client, STDOUT_FILENO);
      view_playlist(sd);
      dup2(stdout, before);
    }
  }
}

int main(){
  int sd = create_playlist();
  signal(SIGINT,sighandler);
  add_to_playlist("Hey Jude", "The Beatles", "heyjude.mp3", sd);

  add_to_playlist("Bodak Yellow", "Cardi B", "bodakyellow.mp3", sd);

  add_to_playlist("Im a Believer", "Monkees", "believer.mp3", sd);

  add_to_playlist("kobebryant", "Kobe Bryant", "kobebryant.mp3", sd);

  add_to_playlist("Duck Song", "Banpreet", "ducksong.mp3", sd);
  printf("Added\n");
  view_playlist(sd);
  printf("1. %s\n", playlist -> name);
  printf("2. %s\n", playlist -> next -> name);
  printf("\n");
  vote(1, "kobebryant", "Kobe Bryant", sd);
  vote(2, "Duck Song", "Banpreet", sd);
  vote(3, "Im a Believer", "Monkees", sd);
  vote(-1, "Im a Believer", "Monkees", sd);
  printf("voted\n");
  view_playlist(sd);
  //end_vote(sd);
  printf("\n");
  printf("Enter number of minutes before voting for first song closes: ");
  char s[50];
  //^^^^MAKE SURE THIS IS A GOOD FORMAT
  fgets(s, 50, stdin);
  int f1 = fork();
  if(!f1){
    printf("Forked. Waiting [%d] minutes to play playlist\n", atoi(s));
    /*
    int msec = 0, trigger = 30000; //(atoi(s) * 60 * 1000);
    clock_t before = clock();
    int iterations = 0;
    do {

      printf("msec: %d", msec);
      clock_t difference = clock() - before;
      msec = difference * 1000 / CLOCKS_PER_SEC;
      iterations++;
    } while ( msec < trigger );
    printf("Time taken %d seconds %d milliseconds (%d iterations)\n",
    msec/1000, msec%1000, iterations); */
    sleep(atoi(s) * 20);
    printf("Timer ended\n");
    view_playlist(sd);
    while(1){
      //IMPLEMENT BREAK WHEN USER WANTS
      char** commands = end_vote(sd);
      printf("Playing Song...\n");
      int play_fork = fork();
      if(!play_fork){
        execvp("/usr/local/bin/mpg123", commands);
      }
      else{
        //WAIT
      }
    }
    //PLAY PLAYLIST
    //RECIEVE SIGNAL TO
  }
  else{
    while(1){
      //Blocks Until Client Connects
      int from_client = server_setup();

      //We now have a client, time to fork
      int f2 = fork();
      if(!f2){
        printf("subserver created\n");
        subserver(from_client, sd);
      }
      else{
        close(from_client);
      }
    }
  }

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
