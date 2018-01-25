#define KEY 1234
#define playlist_name "PLAYLIST.txt"

int create_playlist();
int get_playlist_size();
void view_playlist(int sd);
int add_to_playlist(char* name, char* artist, char* file, int sd);
int vote(int val, char* name, char* artist, int sd);
char** end_vote(int sd);
void subserver(int from_client, int sd);
int main();
