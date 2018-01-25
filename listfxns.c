#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <time.h>
#include "listfxns.h"
#include <sys/shm.h>
//#include "control.h"
//#include "shared_memory.h"

//size of node
size_t size = sizeof(struct song_node);
int KEY = 6000;

//Prints ALL SONGS
void print_list(struct song_node* node){
  if(!node){
    printf("\n");
    return;
  }
  //Use "->" to access data from a struct pointer
  printf("%s: %s, file: %s, votes: %d\n ", node->artist, node->name, node->file_name, node->votes);
  return print_list(node->next);
}

//Gets size of linked list
int get_size(struct song_node* list){
  int i;
  for(i = 0; list; i++) list = list -> next;
  return i;
}

//Converts String to lowercase
char* convert_lower(char* string){
  char* new = (char*) calloc(256, sizeof(char));
  int i;
  for(i=0;string[i];i++){
    new[i] = tolower(string[i]);
  }
  return new;
}

int songcmp(struct song_node* song1, struct song_node* song2){
  char* name1 = convert_lower(song1 -> name);
  char* name2 = convert_lower(song2 -> name);
  char* artist1 = convert_lower(song1 -> artist);
  char* artist2 = convert_lower(song2 -> artist);
  int artist_cmp = strcmp(artist1, artist2);
  int song_cmp = strcmp(name1, name2);
  //convert_lower callocs strings
  free(name1);
  free(name2);
  free(artist1);
  free(artist2);
  if(artist_cmp) return artist_cmp;
  return song_cmp;
}

//Takes a pointer to an existing list and the data to be created
//Creates a new node with that data and puts it at the beginning
//of the list. Returns a pointer to the beginning of the list.
struct song_node * insert_front(struct song_node* node, char*name, char* artist, char * file_name, int votes){

  //Since the memory associated with this function will
  //dissappear once it's done, we need to malloc space
  //for our node.
  srand(time(NULL));   // should only be called once
  int r = rand();
  int d = shmget(++KEY, sizeof(struct song_node), 0777|IPC_CREAT);
  //if(print_error(d)) return NULL;
  struct song_node* p = shmat(d, 0, 0);
  //Create the new node
  struct song_node new;
  new.name = name;
  new.artist = artist;
  new.file_name = file_name;
  new.votes = votes;
  new.next = node; //places at beginning
  printf("INSERT_FRONT:\n");
  *p = new;
  //printf("Name (node): %s\n", node -> name);
  //print_list(p);
  return p;
}

struct song_node* insert_in_order(struct song_node* nodeFront, char* name, char* artist, char * file_name){
  if(nodeFront == NULL) return insert_front(NULL, name, artist, file_name, 0);
  struct song_node* p;
  srand(time(NULL));   // should only be called once
  int r = rand();
  int d = shmget(++KEY, sizeof(struct song_node), 0777|IPC_CREAT);
  //if(print_error(d)) return NULL;
  p = shmat(d, 0, 0);
  struct song_node new;
  struct song_node* node = nodeFront;
  new.name = name;
  new.artist = artist;
  new.file_name = file_name;
  new.votes = 0;
  new.next = NULL;
  *p = new;
  while(node -> next){
    int cmp = songcmp(p, node -> next);
    if(cmp <= 0){
      (p -> next)  = (node -> next);
      (node -> next) = p;
      //printf("%s\n", name);
      return nodeFront;
    }
    node = node -> next;
  }
  if(songcmp(p, node) < 0) return insert_front(node, p-> name, p-> artist, p->file_name, 0);
  else node -> next = p;
  return nodeFront;
}

//Takes a pointer to a list
//Frees each node in the list
struct song_node * free_list(struct song_node* node){
  //If the node is null, we're done
  if(!node) return node;

  //Create a pointer to the next node
  struct song_node* next = node -> next;

  //Free the current node
  free(node);

  //move onto the next node
  return free_list(next);
}

struct song_node* find_song(struct song_node* playlist, char* name, char* artist){
  printf("finding song...\n");
  struct song_node* list = playlist;
  char* name_lower = convert_lower(name);
  char* artist_lower = convert_lower(artist);
  while(list){
    char* list_name = convert_lower(list -> name);
    char* list_artist = convert_lower(list -> artist);
    if(!strcmp(list_name, name_lower) && !strcmp(list_artist, artist_lower)){
      printf("found song!\n");
      free(name_lower);
      free(artist_lower);
      free(list_name);
      free(list_artist);
      return list;
    }
    printf("Loop of find_song\n");
    print_list(list);
    list = list -> next;
    //free(list_name);
    //free(list_artist);
  }
  printf("Song not found!\n");
  free(name_lower);
  free(artist_lower);
  return NULL;
}

struct song_node* find_song_by_artist(struct song_node* list, char* artist){
  char* artist_lower = convert_lower(artist);
  while(list){
    char* list_artist = convert_lower(list -> artist);
    if(!strcmp(list_artist, artist_lower)){
      free(artist_lower);
      free(list_artist);
      return list;
    }
    free(list_artist);
    list = list -> next;
  }
  free(artist_lower);
  return NULL;
}

struct song_node* remove_node(struct song_node* listFront, struct song_node* node){
  if(listFront == node){
    struct song_node* newFront = listFront -> next;
    listFront -> next = NULL;
    shmdt(listFront);
    return newFront;
  }
  struct song_node* list = listFront;
  while(list -> next){
    if(list->next == node){
      list->next = list->next->next;
      node -> next = NULL;
      free(node);
      return listFront;
    }
    list = list -> next;
  }
  return listFront;
}

struct song_node* random_node(struct song_node* list){
  int size = get_size(list);
  int random = (rand() % size);
  int i;
  for(i = 0; i<random; i++){
    list = list->next;
  }
  return list;
}

struct song_node* add_votes(struct song_node* list, char*name, char*artist, int val){
  struct song_node* song = find_song(list, name, artist);
  song -> votes = (song -> votes) + val;
  return list;
}

//If pos: one > two
//If neg: one < two
//If 0: one == two
int songcmp_byvote(struct song_node* one, struct song_node* two){
  return (one -> votes) - (two -> votes);
}

struct song_node* insert_in_order_by_vote(struct song_node* nodeFront, char* name, char* artist, char* file_name, int votes){
  if(nodeFront == NULL) return insert_front(NULL, name, artist, file_name, votes);
  struct song_node* p;  // should only be called once
  int d = shmget(++KEY, sizeof(struct song_node), 0777|IPC_CREAT);
  //if(print_error(d)) return NULL;
  p = shmat(d, 0, 0);
  struct song_node new;
  struct song_node* node = nodeFront;
  new.name = name;
  new.artist = artist;
  new.file_name = file_name;
  new.votes = votes;
  new.next = NULL;
  *p = new;
  while(node -> next){
    int cmp = songcmp_byvote(p, node -> next);
    if(cmp <= 0){
      if(songcmp_byvote(p, node) < 0){
        p -> next = node;
        return p;
      }
      (p -> next)  = (node -> next);
      (node -> next) = p;
      //printf("%s\n", name);
      return nodeFront;
    }
    node = node -> next;
  }
  if(songcmp_byvote(p, node) < 0) return insert_front(node, p-> name, p-> artist, p->file_name, p->votes);
  else node -> next = p;
  return nodeFront;
}

/* function to swap data of two nodes a and b*/
void swap(struct song_node *a, struct song_node *b)
{
    char* name1 = a -> name;
    a -> name = b -> name;
    b -> name = name1;

    char* artist1 = a -> artist;
    a -> artist = b -> artist;
    b -> artist = artist1;

    char* file_name1 = a -> file_name;
    a -> file_name = b -> file_name;
    b -> file_name = file_name1;

    int votes1 = a -> votes;
    a -> votes = b -> votes;
    b -> votes = votes1;
}

/* Bubble sort the given linked lsit */
void bubbleSort(struct song_node *start)
{
    int swapped, i;
    struct song_node *ptr1;
    struct song_node *lptr = NULL;

    /* Checking for empty list */
    if (ptr1 == NULL)
        return;

    do
    {
        swapped = 0;
        ptr1 = start;

        while (ptr1->next != lptr)
        {
            if (ptr1->votes < ptr1->next->votes)
            {
                swap(ptr1, ptr1->next);
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    }
    while (swapped);
}

void sort_by_votes(struct song_node* list){
  bubbleSort(list);
  /*
  struct song_node* ans;
  ans = NULL;
  struct song_node* dummy = list;
  while(dummy){
    printf("sorting(before insert_in_order_by_vote)\n");
    ans = insert_in_order_by_vote(ans, dummy -> name, dummy -> artist, dummy -> file_name, dummy -> votes);
    print_list(ans);
    dummy = dummy -> next;

    printf("\n sorting...");
    print_list(ans);
  }
  printf("SORTED ANS\n");
  print_list(ans);
  while (ans){

  }
  int d = shmget(++KEY, sizeof(struct song_node), 0777|IPC_CREAT);
  //if(print_error(d)) return NULL;
  struct song_node* result = shmat(d, 0, 0);
  *result = *ans;
  return result;*/

}
