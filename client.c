#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "lib.h"
#include "listfxns.h"
#include "pipe_networking.h"
#include "parsing.h"

#define KEY 5678
#define playlist_name "client_playlist"

union semun {
               int              val;  
               struct semid_ds *buf;  
               unsigned short  *array;  
               struct seminfo  *__buf;  
                                      
           };

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
        node = insert_in_order(node, song_line[0], song_line[1], song_line[2]);
        i++;
    }
    return node;
}

char** get_playlist_commands(struct song_node* a){
  struct song_node* temp = a;
  char** commandz = calloc(1, sizeof("mpg123"));
  commandz[0] = "mpg123";
  int i = 1;
  while (temp) {
      commandz[i] = calloc(1, sizeof(temp->file_name));
      commandz[i] = temp->file_name;
      temp = temp -> next;
      i++;
  }
  commandz[i+1] = calloc(1, sizeof(char *));
  commandz[i+1] = 0;
  return commandz;
}

int main() {
    printf("\nHi! Welcome to our player!\nTo add a song to your playlist, type \"add\". \n");
    printf("To play your playlist, type \"play\". To stop, type \"stop\".\n");
    printf("To interact with the group playlist, type \"server\"\n\n");
    struct song_node * a = NULL;
    struct song_node * temp;
    char ** commandz;
    char ** inputs;
    int sd = create_playlist();
    a = initialize_playlist(sd);
    print_list(a);
    commandz = get_playlist_commands(a);
    while (1) {
        printf("Enter a command: ");
        char s[50];
        fgets(s, 50, stdin);
        int len = strlen(s) - 1;
        s[len] = 0;
        if (strcmp(s, "stop") == 0)
            exit(0);
        else if (strcmp(s, "play") == 0) {
            int f = fork();
            if (!f) {
                if (!a) {
                    printf("Can't play, playlist is empty\n");
                    exit(0);
                }
                else {
                    execvp("/usr/bin/mpg123", commandz);
                    exit(0);
                }
            }
            else {
                int status;
                int child_pid = wait(&status);
            }
        }
        else if (strcmp(s, "add") == 0) {
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
            a = insert_in_order(a, name, artist, file);
            print_list(a);
            temp = a;
            update_playlist(temp, sd);
            commandz = get_playlist_commands(a);
        }
        else if (strcmp(s, "remove") == 0) {
            printf("Song name: ");
            char * name = malloc(50);
            fgets(name, 50, stdin);
            printf("Artist name: ");
            char * artist = malloc(50);
            fgets(artist, 50, stdin);
            name[strlen(name)-1] = 0;
            artist[strlen(artist)-1] = 0;
            struct song_node * node = find_song(a, name, artist);
            a = remove_node(a, node);
            print_list(a);
            temp = a;
            update_playlist(temp, sd);
            commandz = get_playlist_commands(a);
        }
        else if (!strcmp(s, "server")){
          int to_server;
          int from_server;
          char buffer[BUFFER_SIZE];

          from_server = client_handshake( &to_server );
          printf("\nWelcome to the shared playlist!\n\n");
          printf("To vote for a song: vote -<song name> -<song artist>\n");
          printf("If the song is already a part of the shared playlist, you will simply cast a vote for it.\n");
          printf("If it isn't, you'll need to add it to the shared playlist\n\n");
          printf("To view the shared playlist: view\n\n");

          while (1) {
            printf("enter data: ");
            fgets(buffer, sizeof(buffer), stdin);
            *strchr(buffer, '\n') = 0;
            write(to_server, buffer, sizeof(buffer));
            read(from_server, buffer, sizeof(buffer));
            printf("received: [%s]\n", buffer);
          }
        }
    }
    return 0;
}
