#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "listfxns.h"

struct song_node* table[26];

//Converts Letter to index in library
int get_index(char letter){
  if(letter < 97){
    return letter - 65;
  }
  else{
    return letter - 97;
  }
}

//Gets list from char
struct song_node* get_list(char letter){
  int index = get_index(letter);
  return table[index];
}

void print_letter(char letter){
  printf("print list %c: ", letter);
  print_list(get_list(letter));
}

void print_lib(){
  int i = 0;
  while(i < 27){
    if(table[i]){
      printf("%c list : ", tolower((table[i] -> artist)[0]));
      print_list(table[i]);
    }
    i++;
  }
}

void add_song(char* name, char *artist, char * file_name){
  char* low_art = convert_lower(artist);
  int index = get_index(low_art[0]);
  free(low_art);
  table[index]= insert_in_order(table[index], name, artist, file_name);
}

struct song_node* find_artist(char* artist){
	struct song_node* list = get_list(artist[0]);
	return find_song_by_artist(list, artist);
}

void print_artist(char* artist){
  if(find_artist(artist)) print_list(find_artist(artist));
  else printf("Artist Not Found!\n");
}

struct song_node* find_song_lib(char* name, char* artist){
	struct song_node* list = get_list(artist[0]);
	return find_song(list, name, artist);
}

void remove_song(char* name, char* artist){
	struct song_node* song = find_song_lib(name, artist);
	struct song_node* front = get_list(artist[0]);
	table[get_index(artist[0])] = remove_node(front, song);
}

//Get a random number between 0 and 25
//If table[random number] is null keep going right
//Return random node from table[random number]
void shuffle(){
  int i;
  for(i=0; i < 3; i++){
    int random = rand() % 26;
    while(!table[random]) random = (random + 1) % 26;
    struct song_node* rand_node = random_node(table[random]);
    printf("%s: %s\n", rand_node -> artist, rand_node -> name);
  }
}

void delete_nodes(){
  int i;
  for (i = 0; i<26; i++){
    if(table[i]){
      free_list(table[i]);
      table[i] = 0;
    }
  }
}
