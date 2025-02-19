socket_tools.o: socket_tools.cpp
	g++ -c socket_tools.cpp -o socket_tools.o

server.o: server.cpp
	g++ -c server.cpp -o server.o

client.o: client.cpp
	g++ -c client.cpp -o client.o

client: socket_tools.o client.o
	g++ socket_tools.o client.o -o client

server: socket_tools.o server.o
	g++ socket_tools.o server.o -o server

go: client server
	g++ socket_tools.o client.o -o client

clean:
	rm -f socket_tools.o server.o client.o server client
