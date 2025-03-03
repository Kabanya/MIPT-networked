#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <cstring>
#include <iostream>

#include "client_tools.h"   
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
