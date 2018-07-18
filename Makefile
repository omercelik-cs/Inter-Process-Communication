all:client server
client: client.c
	gcc -Wall -o client client.c -lrt
server: server.c
	gcc -pthread -o server server.c -lrt
clean:
	rm -fr *~ client server
