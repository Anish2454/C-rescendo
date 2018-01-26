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
#include "networking.h"
#include "server.h"
#include "parsing.h"
#include <sys/socket.h>
#include <sys/uio.h>
#include <signal.h>
#include <time.h>

//Creates New Playlist File and Semaphore
//Returns Semaphore descriptor

union semun {
               int              val;
               struct semid_ds *buf;
               unsigned short  *array;
               struct seminfo  *__buf;

           }; 


static void sighandler(int signo) {
  if (signo == SIGINT) {
    remove("luigi");
    exit(0);
  }
}

int create_playlist(){
  printf("Creating Playlist File...\n");
  int fd = open(playlist_name, O_EXCL|O_CREAT, 0777);
  //if (fd == -1) printf("Error: %s\n", strerror(errno));
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
  //printf("really started update playlist\n");
//Down the semaphore
struct sembuf sb;
  if(sd != -1){
	sb.sem_op = -1;
	sb.sem_num = 0;
	sb.sem_flg = SEM_UNDO;
	semop(sd, &sb, 1);
  }
  //printf("started update_playlist\n");
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
  //printf("end of updateplaylist\n");
  //int saved_flags = fcntl(fd, F_GETFL);

// Set the new flags with O_NONBLOCK masked out

  //fcntl(fd, F_SETFL, saved_flags & ~O_NONBLOCK);
  close(fd);
  //printf("closing file descripor\n");
  if(sd != -1){
  //Up the semaphore
  sb.sem_op = 1;
  semop(sd, &sb, 1);}
  return 0;
}

int get_playlist_size(){
  //printf("Getting size of playlist\n");
  struct stat sb;
  stat(playlist_name, &sb);
  return sb.st_size;
}

char* view_playlist(int sd){
  //Down the semaphore
  struct sembuf sb;
    if(sd!=-1){
	  sb.sem_op = -1;
	  sb.sem_num = 0;
	  sb.sem_flg = SEM_UNDO;
	  semop(sd, &sb, 1);
  }
  int size = get_playlist_size();
 // printf("Viewing Playlist\n");
  int fd = open(playlist_name, O_RDONLY, 0777);
  if (fd == -1) printf("Error: %s\n", strerror(errno));
  char* buffer = (char*) calloc(1, size);
  int result = read(fd, buffer, size);
  if (result == -1) printf("Error: %s\n", strerror(errno));
  close(fd);

  if(sd!=-1){
    //Up the semaphore
    sb.sem_op = 1;
    semop(sd, &sb, 1);
  }
  return buffer;
}

struct song_node * initialize_playlist(int sd) {
    int size = get_playlist_size();
    char * buff = malloc(size);
    strcpy(buff, view_playlist(sd));
    //printf("buff: %s\n", buff);
    //printf("\n*** CURRENT PLAYLIST ***\n");
    // char ** separated_newline = separate_line(buff, "\n");
    // start added code
    int i = 0;
    char * temp = malloc(strlen(buff));
    strcpy(temp, buff);
    //printf("temp: %s\n", temp);
    while (temp) {
        strsep(&temp, "\n");
        i++;
    }
    //printf("HALFWAY Got through initialized playlist\n");
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
	  //printf("inside sep newline\n");
        char ** song_line = separate_line(separated_newline[i], "|");
        node = insert_in_order_by_vote(node, song_line[0], song_line[1], song_line[2], atoi(song_line[3]));
        i++;
    }
    //printf("Got through initialized playlist\n");
    //print_list(node);
    return node;
}

int vote(struct song_node * playlist, int val, char* name, char* artist, int sd){
  //Down the semaphore
	struct sembuf sb;
	sb.sem_op = -1;
	sb.sem_num = 0;
	sb.sem_flg = SEM_UNDO;
	semop(sd, &sb, 1);
  playlist = initialize_playlist(-1);
  playlist = add_votes(playlist, name, artist, val);
  //printf("got throuhg add_votes\n");
  struct song_node* temp = playlist;
  //printf("got thorugh temp node\n");
  update_playlist(temp, -1);
  //print_list(playlist);

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
  //print_list(playlist);
  playlist = sort_by_votes(playlist);
  //print_list(playlist);
  char** commands = (char**) calloc(4, sizeof(char*));
  commands[0] = "mpg123";
  char* command = calloc(100, sizeof(char));
  command = playlist -> file_name;
  commands[1] =  "-C";
  commands[2] = command;
  commands[3] = NULL;
  if (playlist -> next)
    playlist = playlist -> next; //Removes top song
  //Up the semaphore
  sb.sem_op = 1;
  semop(sd, &sb, 1);
  return commands;
}

void subserver(struct song_node * playlist, int from_client, int sd) {
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
        write(from_client, resp, sizeof(resp));
        struct song_node * temp = playlist;
        //printf("SUBSERVER PLAYLIST\n");
        playlist = initialize_playlist(sd);
        //print_list(playlist);
      }
      else{
        //SONG NOT IN PLAYLIST - TRANSFER MP3
        printf("Song Not In Playlist!");
        /*
        char resp[] = "SONG DOESNT EXIST";
        write(from_client, resp, sizeof(resp));
        char name[100];
        read(from_client, name, sizeof(name));
        char resp2[] = "name received";
        write(from_client, resp2, sizeof(resp2));
        char artist[100];
        read(from_client, artist, sizeof(artist));
        printf("artist server: %s\n", artist);
        char resp3[] = "artist received";
        write(from_client, resp3, sizeof(resp3));
        char recv_str[256];
        int fd = open(name, O_CREAT|O_WRONLY, 0666);
        int recv_count;
        ssize_t sent_bytes, rcvd_bytes, rcvd_file_size;
        recv_count = 0;  number of recv() calls required to receive the file
        rcvd_file_size = 0;
        size_t filesize;
        //read(from_client, &filesize, sizeof(filesize));
        printf("Size: %zu\n", filesize);  size of received file
        while ( (rcvd_bytes = recv(from_client, recv_str, 256, 0)) > 0 ){
          recv_count++;
          rcvd_file_size += rcvd_bytes;
          if (rcvd_file_size > 12000000) break;
          //if(rcvd_file_size >= filesize-5) break;
          if (write(fd, recv_str, rcvd_bytes) < 0 ){
            perror("error writing to file");
          }
        }
        close(fd);
        printf("here\n");
        playlist = insert_in_order(playlist, name, artist, name);
        vote(playlist, 1, name, artist, sd);
        struct song_node* temp = playlist;
        update_playlist(temp, sd);*/
      }
    }
    else if (!strcmp(args[0], "view")){
      //Redirect view_playlist output from stdout to pipe
      playlist = initialize_playlist(sd);
      int stdout = dup(STDOUT_FILENO);
      int before = dup2(from_client, STDOUT_FILENO);
      print_list(playlist);
      dup2(stdout, before);
    }
  }
  close(from_client);
  exit(0);
}

int main(){
  int sd = create_playlist();
  struct song_node * a = initialize_playlist(sd);
  //print_list(a);
  signal(SIGINT,sighandler);
  a = insert_in_order(a, "Hey Jude", "Beatles", "heyjude.mp3");
  a = insert_in_order(a, "Bodak Yellow", "Cardi B", "bodakyellow.mp3");
  // a = insert_in_order(a, "I'm a Believer", "Monkees", "believer.mp3");
  // a = insert_in_order(a, "kobebryant", "Kobe Bryant", "kobebryant.mp3");
  a = insert_in_order(a, "Duck Song", "Banpreet", "ducksong.mp3");
  update_playlist(a, sd);
  /* add_to_playlist("Hey Jude", "Beatles", "heyjude.mp3", sd);
  add_to_playlist("Bodak Yellow", "Cardi B", "bodakyellow.mp3", sd);
  add_to_playlist("Im a Believer", "Monkees", "believer.mp3", sd);
  add_to_playlist("kobebryant", "Kobe Bryant", "kobebryant.mp3", sd);
  add_to_playlist("Duck Song", "Banpreet", "ducksong.mp3", sd); */
  while (1) {
    printf("Type add to add a song to the playlist\n");
    printf("Type start to start voting!\n");
    printf("Enter a command: ");
    char resp[50];
    fgets(resp, 50, stdin);
    resp[strlen(resp)-1] = 0;
    if (strcmp(resp, "add") == 0) {
      printf("Song name: ");
      char * name = malloc(50);
      fgets(name, 50, stdin);
      printf("Artist name: ");
      char * artist = malloc(50);
      fgets(artist, 50, stdin);
      printf("mp3 file name: ");
      char * file = malloc(50);
      fgets(file, 50, stdin);
      name[strlen(name)-1] = 0;
      artist[strlen(artist)-1] = 0;
      file[strlen(file)-1] = 0;
      struct song_node * node = find_song(a, name, artist);
      if (!node) {
          a = insert_in_order(a, name, artist, file);
      }
      else {
          printf("Song already in playlist!\n");
      }
    }
    else if (strcmp(resp, "start") == 0)
      break;
    else
      printf("Invalid command!\n");
  }
  update_playlist(a, sd);
  printf("Enter number of minutes before voting for first song closes: ");
  //print_list(a);
  char s[50];
  //^^^^MAKE SURE THIS IS A GOOD FORMAT
  fgets(s, 50, stdin);
  int f1 = fork();
  if(f1){
    printf("Forked. Waiting [%d] minutes to play playlist\n", atoi(s));
    sleep(atoi(s) * 60);
    int count = 0;
    while(a){
      a = initialize_playlist(sd);
      char** commands = end_vote(a, sd);
      print_list(a);
      a = a -> next;
      struct song_node * temp = a;
      update_playlist(temp, sd);
      printf("Playing Song...\n");
      printf("Press Q to go to the next song\n");
      //count++;
      int d = fork();
      if (!d) {
        execvp("/usr/bin/mpg123", commands);
        exit(0);
      }
      else {
        int status;
        waitpid(d, &status, 0);
	      //printf("Child Finished PLaying Song\n");
      }
      //exit(0);
    }
    //PLAY PLAYLIST
    //RECIEVE SIGNAL TO
  }
  else{
    int listen_socket;
    int f2;
    listen_socket = server_setup();

    while (1) {
      int client_socket = server_connect(listen_socket);
      f2 = fork();
      if (f2 == 0){
        printf("Subserver created\n");
        subserver(a, client_socket, sd);
      }
      else{
        close(client_socket);
      }
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
