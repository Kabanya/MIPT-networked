CXX ?= g++

socket_tools.o: socket_tools.cpp
	$(CXX) -c socket_tools.cpp -o socket_tools.o

server_tools.o: server_tools.cpp
	$(CXX) -c server_tools.cpp -o server_tools.o

client_tools.o: client_tools.cpp 
	$(CXX) -c client_tools.cpp -o client_tools.o

server.o: server.cpp
	$(CXX) -c server.cpp -o server.o

client.o: client.cpp
	$(CXX) -c client.cpp -o client.o

client: socket_tools.o client_tools.o client.o
	$(CXX) socket_tools.o client_tools.o client.o -o client

server: socket_tools.o server_tools.o server.o
	$(CXX) socket_tools.o server_tools.o server.o -o server

all: client server

clean:
	$(RM) socket_tools.o server_tools.o client_tools.o server.o client.o server client
