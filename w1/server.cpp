#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <vector>
#include <iostream>
#include "socket_tools.h"

struct Client
{
  sockaddr_in addr;
};

int main(int argc, const char **argv)
{
  const char *port = "2025";

  int sfd = create_dgram_socket(nullptr, port, nullptr);

  if (sfd == -1)
  {
    printf("cannot create socket\n");
    return 1;
  }
  printf("listening!\n");

  std::vector<Client> clients;

  while (true)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sfd, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    select(sfd + 1, &readSet, NULL, NULL, &timeout);


    if (FD_ISSET(sfd, &readSet))
    {
      constexpr size_t buf_size = 1000;
      static char buffer[buf_size];
      memset(buffer, 0, buf_size);

      sockaddr_in sin;
      socklen_t slen = sizeof(sockaddr_in);
      ssize_t numBytes = recvfrom(sfd, buffer, buf_size - 1, 0, (sockaddr*)&sin, &slen);
      if (numBytes > 0)
      {
        if (std::string(buffer) == "New Client Connection")
        {
          bool clientExists = false;
          for (const Client& client : clients)
          {
            if (client.addr.sin_addr.s_addr == sin.sin_addr.s_addr && client.addr.sin_port == sin.sin_port)
            {
              clientExists = true;
              break;
            }
          }
          if(!clientExists)
          {
            Client newClient;
            newClient.addr = sin;
            clients.push_back(newClient);
            printf("New client connected: %s:%d\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
          }
        }
        else
        {
          printf("(%s:%d) %s\n", inet_ntoa(sin.sin_addr), sin.sin_port, buffer); // assume that buffer is a string
        }
      }
    }
  }
  return 0;
}
