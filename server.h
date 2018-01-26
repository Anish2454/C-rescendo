#define KEY 1234
#define playlist_name "server_playlist"

int create_playlist();
int get_playlist_size();
char * view_playlist(int sd);
int add_to_playlist(char* name, char* artist, char* file, int sd);
int vote(struct song_node * playlist, int val, char* name, char* artist, int sd);
char** end_vote(struct song_node * playlist, int sd);
void subserver(struct song_node * playlist, int from_client, int sd);
int main();
