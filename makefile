all: listfxns.o lib.o client.o
	gcc -o client listfxns.o client.o lib.o

listfxns.o: listfxns.c listfxns.h
	gcc -c listfxns.c

lib.o: lib.c lib.h
	gcc -c lib.c

client.o: client.c listfxns.h lib.h
	gcc -c client.c

clean:
	rm client
	rm *.o
	rm PLAYLIST.txt

run: all
	./client
