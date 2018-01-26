all: listfxns.o lib.o client.o server.o networking.o parsing.o
	gcc -o client listfxns.o client.o lib.o networking.o parsing.o
	gcc -o server listfxns.o server.o networking.o parsing.o

listfxns.o: listfxns.c listfxns.h
	gcc -c listfxns.c

lib.o: lib.c lib.h
	gcc -c lib.c

client.o: client.c listfxns.h lib.h
	gcc -c client.c

server.o: server.c server.h
	gcc -c server.c

networking.o: networking.c networking.h
	gcc -c networking.c

parsing.o: parsing.c parsing.h
	gcc -c parsing.c

clean:
	rm client
	rm *.o
	rm server_playlist

run: all
	./client
