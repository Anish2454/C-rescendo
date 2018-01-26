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
#include <sys/wait.h>
#include "lib.h"
#include "listfxns.h"
#include "pipe_networking.h"
#include "server.h"
#include "parsing.h"
#include <signal.h>
#include <time.h>

//Creates New Playlist File and Semaphore
//Returns Semaphore descriptor
/*union semun {
               int              val;  
               struct semid_ds *buf;  
               unsigned short  *array;  
               struct seminfo  *__buf;  
                                      
           };
*/

static void sighandler(int signo) {
  if (signo == SIGINT) {
    remove("luigi");
    exit(0);
  }
}

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

int update_playlist(struct song_node * song, int sd){
  //Down the semaphore
	struct sembuf sb;
	sb.sem_op = -1;
	sb.sem_num = 0;
	sb.sem_flg = SEM_UNDO;
	semop(sd, &sb, 1);
  int fd = open(playlist_name, O_WRONLY | O_TRUNC, 0777);
  if (fd == -1) printf("Error: %s\n", strerror(errno));
  //lseek(fd, 0, SEEK_END);
  while (song) {
    int result = write(fd, song -> name, strlen(song -> name));
    write(fd, "|", 1);
    result = write(fd, song -> artist, strlen(song -> artist));
    write(fd, "|", 1);
    result = write(fd, song -> file_name, strlen(song -> file_name));
    write(fd, "|", 1);
    char num[100];
    sprintf(num, "%d", song -> votes);
    result = write(fd, num, strlen(num));
    if (result == -1) printf("Error: %s\n", strerror(errno));
    result = write(fd, "\n", sizeof(char));
    if (result == -1) printf("Error: %s\n", strerror(errno));
    song = song -> next;
  }
  close(fd);

  //Up the semaphore
  sb.sem_op = 1;
  semop(sd, &sb, 1);
  return 0;
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
 // printf("Viewing Playlist\n");
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

struct song_node * initialize_playlist(int sd) {
    int size = get_playlist_size();
    char * buff = malloc(size);
    strcpy(buff, view_playlist(sd));
    printf("\n*** CURRENT PLAYLIST ***\n");
    // char ** separated_newline = separate_line(buff, "\n");
    // start added code
    int i = 0;
    char * temp = malloc(strlen(buff));
    strcpy(temp, buff);
    while (temp) {
        strsep(&temp, "\n");
        i++;
    }
    free(temp);
    char * separated_newline[i];
    i = 0;
    while(buff){
        separated_newline[i] = strsep(&buff, "\n");
        i++;
    }
    free(buff);
    separated_newline[i-1] = 0;
    // end added code
    struct song_node * node = NULL;
    i = 0;
    while (separated_newline[i]) {
        char ** song_line = separate_line(separated_newline[i], "|");
        node = insert_in_order_by_vote(node, song_line[0], song_line[1], song_line[2], atoi(song_line[3]));
        i++;
    }
    return node;
}

int vote(struct song_node * playlist, int val, char* name, char* artist, int sd){
  //Down the semaphore
	struct sembuf sb;
	sb.sem_op = -1;
	sb.sem_num = 0;
	sb.sem_flg = SEM_UNDO;
	semop(sd, &sb, 1);

  playlist = add_votes(playlist, name, artist, val);
  print_list(playlist);

  //Up the semaphore
  sb.sem_op = 1;
  semop(sd, &sb, 1);
  return 0;
}

//Sorts playlist and returns file_name of most voted song
char** end_vote(struct song_node * playlist, int sd){
  //Down the semaphore
	struct sembuf sb;
	sb.sem_op = -1;
	sb.sem_num = 0;
	sb.sem_flg = SEM_UNDO;
	semop(sd, &sb, 1);
  print_list(playlist);
  playlist = sort_by_votes(playlist);
  print_list(playlist);
  char** commands = (char**) calloc(2, sizeof(char*));
  commands[0] = "mpg123";
  char* command = calloc(100, sizeof(char));
  command = playlist -> file_name;
  commands[1] = command;
  if (playlist -> next)
    playlist = playlist -> next; //Removes top song
  //Up the semaphore
  sb.sem_op = 1;
  semop(sd, &sb, 1);
  return commands;
}

void subserver(struct song_node * playlist, int from_client, int sd) {
  int to_client = server_connect(from_client);
  char* buff = (char*) calloc((BUFFER_SIZE / sizeof(char)), sizeof(char));
  while(read(from_client, buff, BUFFER_SIZE)){
    printf("[subserver] received: [%s]\n", buff);
    char** args = separate_line(buff, "-");
    if (!strcmp(args[0], "vote")){
      char* name = args[1];
      char* artist = args[2];
      if(find_song(playlist, name, artist)){
        //Song Already In Playlist
        vote(playlist, 1, name, artist, sd);
        char resp[100];
        sprintf(resp, "Voted For: %s", name);
        write(to_client, resp, sizeof(resp));
        struct song_node * temp = playlist; 
        printf("SUBSERVER PLAYLIST\n");
        print_list(playlist);
        update_playlist(temp, sd);
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
  struct song_node * a = initialize_playlist(sd);
  print_list(a);
  signal(SIGINT,sighandler);
   a = insert_in_order(a, "Hey Jude", "Beatles", "heyjude.mp3");
  a = insert_in_order(a, "Bodak Yellow", "Cardi B", "bodakyellow.mp3");
 // a = insert_in_order(a, "I'm a Believer", "Monkees", "believer.mp3");
 // a = insert_in_order(a, "kobebryant", "Kobe Bryant", "kobebryant.mp3");
  a = insert_in_order(a, "Duck Song", "Banpreet", "ducksong.mp3");   
 /* add_to_playlist("Hey Jude", "Beatles", "heyjude.mp3", sd);
  add_to_playlist("Bodak Yellow", "Cardi B", "bodakyellow.mp3", sd);
  add_to_playlist("Im a Believer", "Monkees", "believer.mp3", sd);
  add_to_playlist("kobebryant", "Kobe Bryant", "kobebryant.mp3", sd);
  add_to_playlist("Duck Song", "Banpreet", "ducksong.mp3", sd); */
  printf("Enter number of minutes before voting for first song closes: ");
  //print_list(a);
  char s[50];
  //^^^^MAKE SURE THIS IS A GOOD FORMAT
  fgets(s, 50, stdin);
  int f1 = fork();
  if(f1){
    printf("Forked. Waiting [%d] minutes to play playlist\n", atoi(s));
    /*
    int msec = 0, trigger = 30000; //(atoi(s) * 60 * 1000);
    clock_t before = clock();
    int iterations = 0;
    do {
      sleep(1);
      printf("msec: %d", msec);
      clock_t difference = clock() - before;
      msec = difference * 1000 / CLOCKS_PER_SEC;
      iterations++;
    } while ( msec < trigger );
    printf("Time taken %d seconds %d milliseconds (%d iterations)\n",
    msec/1000, msec%1000, iterations); */
    sleep(atoi(s) * 30);
    int count = 0;
    while(a){
      if (count > 3) break;
      //IMPLEMENT BREAK WHEN USER WANTS
      a = initialize_playlist(sd);
      char** commands = end_vote(a, sd);
      print_list(a);
      a = a -> next;
      struct song_node * temp = a;
      update_playlist(temp, sd);
      printf("Playing Song...\n");
      count++;
      int d = fork();
      if (!d) {
        execvp("/usr/local/bin/mpg123", commands);
        exit(0);
      }
      else {
        int status; 
        waitpid(d, &status, 0);
      }
      //exit(0);
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
        subserver(a, from_client, sd);
       // a = initialize_playlist(sd);
        exit(0);
      }
     // else{
        close(from_client);
        exit(0);
     // }
    }
  }
  /*
  add_to_playlist("Hey Jude", "The Beatles", "heyjude.mp3", sd);
  add_to_playlist("Bodak Yellow", "Cardi B", "bodakyellow.mp3", sd);
  add_to_playlist("Im a Believer", "Monkees", "believer.mp3", sd);
  add_to_playlist("kobebryant", "Kobe Bryant", "kobebryant.mp3", sd);
  add_to_playlist("Duck Song", "Banpreet", "ducksong.mp3", sd);
  view_playlist(sd);
  printf("\n");
  vote(1, "kobebryant", "Kobe Bryant", sd);
  vote(2, "Duck Song", "Banpreet", sd);
  vote(3, "Im a Believer", "Monkees", sd);
  vote(-1, "Im a Believer", "Monkees", sd);
  view_playlist(sd);
  end_vote(sd);
  printf("\n");
  view_playlist(sd);
  printf("\n"); */

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
