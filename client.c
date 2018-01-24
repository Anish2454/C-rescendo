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

char ** parse_args(char* line){
  int i = 1;
  char * temp = malloc(sizeof(char*));
  strcpy(temp, line);
  while (temp) {
    strsep(&temp, " ");
    i++;
  }

  char** args = (char**) calloc(i, sizeof(char*));
  int counter = 0;
  while(line){
    args[counter] = strsep(&line, " ");
    counter++;
  }
  return args;
}

int main() {
    printf("\nHi! Welcome to our player!\nTo add a song to your playlist, type \"add\". \n");
    printf("To play your playlist, type \"play\". To stop, type \"stop\".\n\n");
    struct song_node * a = NULL;
    struct song_node * temp;
    char ** commandz; 
    char ** inputs; 
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
                    execvp("/usr/local/bin/mpg123", commandz);
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
            int i = 1; 
            temp = a;
            commandz = calloc(1, sizeof("mpg123"));
            commandz[0] = "mpg123";
            while (temp) {
                commandz[i] = calloc(1, sizeof(temp->file_name));
                commandz[i] = temp->file_name;
                temp = temp -> next;
                i++; 
            }
            commandz[i+1] = calloc(1, sizeof(char *));
            commandz[i+1] = 0;  
        }
    }
    return 0;
}