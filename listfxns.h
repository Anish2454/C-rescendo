struct song_node {
  char* name;
  char* artist;
  char* file_name;
  int votes;
  struct song_node* next;
};

void print_list(struct song_node* node);
int get_size(struct song_node* list);
char* convert_lower(char* string);
int songcmp(struct song_node* song1, struct song_node* song2);
struct song_node * insert_front(struct song_node* node, char*name, char* artist, char *file_name, int votes);
struct song_node* insert_in_order(struct song_node* nodeFront, char* name, char* artist, char * file_name);
struct song_node * free_list(struct song_node* node);
struct song_node* find_song(struct song_node* list, char* name, char* artist);
struct song_node* find_song_by_artist(struct song_node* list, char* artist);
struct song_node* random_node(struct song_node* list);
struct song_node* remove_node(struct song_node* listFront, struct song_node* node);
struct song_node* add_votes(struct song_node* list, char*name, char*artist, int val);
struct song_node* sort_by_votes(struct song_node* list);
struct song_node* insert_in_order_by_vote(struct song_node* nodeFront, char* name, char* artist, char* file_name, int votes);
int songcmp_byvote(struct song_node* one, struct song_node* two);
