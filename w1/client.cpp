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
#include "client_tools.h"

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