#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <thread>
#include "socket_tools.h"

void receive_messages(int sfd)
{
  constexpr size_t buf_size = 1000;
  char buffer[buf_size];
  
  while (true)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sfd, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    select(sfd + 1, &readSet, NULL, NULL, &timeout);

    if (FD_ISSET(sfd, &readSet))
    {
      memset(buffer, 0, buf_size);
      sockaddr_in sender_addr;
      socklen_t addr_len = sizeof(sender_addr);
      
      ssize_t bytes_received = recvfrom(sfd, buffer, buf_size - 1, 0, 
                                        (struct sockaddr*)&sender_addr, &addr_len);
      
      if (bytes_received > 0)
      {
        std::cout << "\r" << buffer << "\n>";
        std::cout.flush();
      }
    }
  }
}

void display_help()
{
  std::cout << "\n-----------------Client-INFO-------------------\n" 
            << "/c [msg] - Send a message to all connected clients\n"
            << "/mathduel - Challenge someone to a math duel\n"
            << "Any other message will be sent to the server only\n"
            << "----------------Message-ownership----------------\n"
            << "SERVER: [msg] - Send a message to the server\n"
            << "CHAT (sender): [msg] - message from another user\n"
            << "-------------------------------------------------\n\n" << std::endl;
}

int main(int argc, const char **argv)
{
  const char *port = "2025";

  addrinfo resAddrInfo;
  int sfd = create_dgram_socket("localhost", port, &resAddrInfo);

  if (sfd == -1)
  {
    printf("Cannot create a socket\n");
    return 1;
  }

  struct sockaddr_in client_addr{};
  client_addr.sin_family = AF_INET;
  client_addr.sin_addr.s_addr = INADDR_ANY;
  client_addr.sin_port = 0;

  if (bind(sfd, (struct sockaddr*)&client_addr, sizeof(client_addr)) == -1) 
  {
    perror("bind failed");
    close(sfd);
    return 1;
  }

  socklen_t addr_size = sizeof(client_addr);
  getsockname(sfd, (struct sockaddr*)&client_addr, &addr_size);
  std::cout << "Client is using port: " << ntohs(client_addr.sin_port) << std::endl;

  std::string connectRep = "New client!";
  sendto(sfd, connectRep.c_str(), connectRep.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);

  std::thread receive_thread(receive_messages, sfd);
  receive_thread.detach();
  
  while (true)
  {
    std::string input;
    printf(">");
    std::getline(std::cin, input);
    
    if (input == "/help" || input == "/h" || input == "/?" || input == "--help") 
    {
      display_help();
      continue;
    }
    
    if (!input.empty())
    {
      ssize_t res = sendto(sfd, input.c_str(), input.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
      if (res == -1)
        std::cout << strerror(errno) << std::endl;
    }
  }
  return 0;
}