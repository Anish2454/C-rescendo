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

int main() {
    struct song_node* a = insert_in_order(NULL, "Hey Jude", "Beatles", "heyjude.mp3");
    a = insert_in_order(a, "Bodak Yellow", "Cardi B", "bodakyellow.mp3");
    print_list(a);
    return 0;
}