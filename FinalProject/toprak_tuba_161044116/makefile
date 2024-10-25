all:
	gcc -c -std=gnu99 bibakboxserver.c
	gcc -c -std=gnu99 bibakboxclient.c
	gcc -o bibakboxserver bibakboxserver.o -lpthread
	gcc -o bibakboxclient bibakboxclient.o -lpthread
clean:
	rm bibakboxserver bibakboxclient bibakboxserver.o bibakboxclient.o