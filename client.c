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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include "networking.h"
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
  printf("Creating Playlist Semaphore...\n\n");
  int sd = semget(KEY, 1, 0777|IPC_CREAT|IPC_EXCL);
  union semun semopts;
  semopts.val = 1;
  semctl(sd,0,SETVAL,semopts);
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
  while (song) {
    int result = write(fd, song -> name, strlen(song -> name));
    write(fd, "|", 1);
    result = write(fd, song -> artist, strlen(song -> artist));
    write(fd, "|", 1);
    result = write(fd, song -> file_name, strlen(song -> file_name));
    result = write(fd, "\n", sizeof(char));
    song = song -> next;
  }
  close(fd);

  //Up the semaphore
  sb.sem_op = 1;
  semop(sd, &sb, 1);
  return 0;
}

int get_playlist_size(){
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
  int fd = open(playlist_name, O_RDONLY, 0777);
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
    printf("Initial Playlist: ");
    a = initialize_playlist(sd);
    print_list(a);
    commandz = get_playlist_commands(a);
    while (1) {
        printf("Enter a command: ");
        char s[50];
        fgets(s, 50, stdin);
        int len = strlen(s) - 1;
        s[len] = 0;
        trim(s);
        if (strcmp(s, "stop") == 0)
            exit(0);
        else if (strcmp(s, "play") == 0) {
            int i = 1;
            while (commandz[i]) {
                int f = fork();
                if (!f) {
                    if (!a) {
                        printf("Can't play, playlist is empty\n");
                        exit(0);
                    }
                    else {
                        printf("i: %s\n", commandz[i]);
                        char ** arr = calloc(4, sizeof(char *));
                        arr[0] = "mpg123";
                        arr[1] = "-C";
                        arr[2] = commandz[i];
                        arr[3] = NULL;
                        printf("Press Q to go to the next song\n");
                        execvp("/usr/bin/mpg123", arr);
                        exit(0);
                    }
                 }
                else {
                    int status;
                    int child_pid = wait(&status);
                }
                i++;
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
            struct song_node * node = find_song(a, name, artist);
            if (!a) {
                a = insert_in_order(a, name, artist, file);
            }
            else {
                printf("Song already in playlist!\n");
            }
            print_list(a);
            temp = a;
            update_playlist(temp, sd);
            commandz = get_playlist_commands(a);
        }
	else if (strcmp(s, "view") == 0) {
                print_list(a);
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
          char buffer1[BUFFER_SIZE];
          printf("Enter the address of the server: ");
          fgets(buffer1, sizeof(buffer1), stdin);
          *strchr(buffer1, '\n') = 0;

          int server_socket;
          char buffer[BUFFER_SIZE];
          server_socket = client_setup( buffer1 );

          printf("\nWelcome to the shared playlist!\n\n");
          printf("To vote for a song: vote -<song name> -<song artist>\n");
          printf("If the song is already a part of the shared playlist, you will simply cast a vote for it.\n");
          printf("If it isn't, you'll need to add it to the shared playlist\n\n");
          printf("To view the shared playlist: view\n\n");

          while (1) {
            printf("enter: ");
            fgets(buffer, sizeof(buffer), stdin);
            *strchr(buffer, '\n') = 0;
            write(server_socket, buffer, sizeof(buffer));
            read(server_socket, buffer, sizeof(buffer));
            printf("received: [%s]\n", buffer);
            /*
            if(strcmp(buffer, "SONG DOESNT EXIST") == 0){
              printf("That song wasnt found! Let's send it over.\n");
              printf("enter song name: ");
              fgets(buffer, sizeof(buffer), stdin);
              *strchr(buffer, '\n') = 0;
              write(server_socket, buffer, sizeof(buffer));
              read(server_socket, buffer, sizeof(buffer));
              if(strcmp("name received", buffer) == 0){
                printf("enter song artist: ");
                fgets(buffer, sizeof(buffer), stdin);
                *strchr(buffer, '\n') = 0;
                write(server_socket, buffer, sizeof(buffer));
                read(server_socket, buffer, sizeof(buffer));
                printf("artist: %s\n", buffer);
                if(strcmp("artist received", buffer) == 0){
                  printf("enter mp3 filename: ");
                  fgets(buffer, sizeof(buffer), stdin);
                  *strchr(buffer, '\n') = 0;
                  char buffer2[256];
                  int sent_count;  how many sending chunks, for debugging
                  ssize_t read_bytes,  bytes read from local file
                  sent_bytes,  bytes sent to connected socket
                  sent_file_size;
                  struct stat sb;
                  char send_buf[2000];
                  int fd = open(buffer, O_RDONLY, 0777);
                  fstat( fd, &sb );
                  printf("client: fd: %d\n", fd);
                  //write(server_socket, &sb.st_size, sizeof(sb.st_size));
                  while( (read_bytes = read(fd, send_buf, 2000)) > 0 ){
                    if( (sent_bytes = send(server_socket, send_buf, read_bytes, 0)) < read_bytes ) {
                      printf("error\n");
                      return -1;
                    }
                    sent_count++;
                    sent_file_size += sent_bytes;
                  }
                  printf("sent\n");
                  close(fd);
                  while (1) {
                    // Read data into buffer.  We may not have enough to fill up buffer, so we
                    // store how many bytes were actually read in bytes_read.
                    int bytes_read = read(fd, buffer2, sizeof(buffer2));
                    if (bytes_read == 0) // We're done reading from the file
                      break;

                    if (bytes_read < 0) {
                      //error("ERROR reading from file");
                    }

                    // You need a loop for the write, because not all of the data may be written
                    // in one call; write will return how many bytes were written. p keeps
                    // track of where in the buffer we are, while we decrement bytes_read
                    // to keep track of how many bytes are left to write.
                    void *p = buffer2;
                    while (bytes_read > 0) {
                      int bytes_written = write(server_socket, buffer2, bytes_read);
                      if (bytes_written <= 0){
                        //error("ERROR writing to socket\n");
                        printf("ERROR WRITING MP3\n");
                      }
                      bytes_read -= bytes_written;
                      p += bytes_written;
                    }
                  } */
                }
              }
            }
          return 0;
        }
