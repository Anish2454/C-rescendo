#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "parsing.h"

char ** separate_line(char * line, char * delimeter) {
  int i = 0;
  char * temp = malloc(strlen(line));
  strcpy(temp, line);
  while (temp) {
    strsep(&temp, delimeter);
    i++;
  }
  free(temp);
  char** args = (char**) calloc(i, sizeof(char*));
  int counter = 0;
  while(line){
    char * s = strsep(&line, delimeter);
    int length = strlen(s);
    args[counter] = malloc(length);
    args[counter] = s;
    counter++;
  }
  args[counter] = 0;
  i = 0;
  while (args[i]) {
    trim(args[i]);
    i++;
  }
  return args;
}

void trim(char * line) {
  int i;
  int front = 0;
  int end = strlen(line) - 1;
  while (line[front] == ' ')
  front++;
  while ((end >= front) && line[end] == ' ')
  end--;
  for (i = front; i <= end; i++)
  line[i - front] = line[i];
  line[i - front] = 0;
}
