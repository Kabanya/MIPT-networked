#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <vector>
#include <iostream>
#include <thread>
#include <string>
#include "socket_tools.h"

struct Client
{
  sockaddr_in addr;
};


void msg_to_all_clients(int sfd, const std::vector<Client>& clients, const std::string& message)
{
  for (const Client& client : clients)
{
    socklen_t slen = sizeof(sockaddr_in);
    sendto(sfd, message.c_str(), message.size(), 0, (struct sockaddr*)&client.addr, slen);
  }
  printf("msg to all clients: %s\n", message.c_str());
}

void server_input_processing(int sfd, std::vector<Client>& clients)
{
  std::string input;
  while (true)
  {
    printf(">");
    std::getline(std::cin, input);
    if (!input.empty())
    {
      std::string broadcastMsg = "SERVER: " + input;
      msg_to_all_clients(sfd, clients, broadcastMsg);
    }
  }
}

int main(int argc, const char **argv)
{
  const char *port = "2025";

  int sfd = create_dgram_socket(nullptr, port, nullptr);

  if (sfd == -1)
  {
    printf("cannot create socket\n");
    return 1;
  }
  printf("listening on port %s!\n", port);

  std::vector<Client> clients;
  
  std::thread input_thread(server_input_processing, sfd, std::ref(clients));
  input_thread.detach(); 

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
        std::string message(buffer);
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
          
          std::string welcomeMsg = "SERVER: /c -- message to all users\n/help -- for help";
          sendto(sfd, welcomeMsg.c_str(), welcomeMsg.size(), 0, (struct sockaddr*)&sin, slen);
          // msg_to_all_clients(sfd, clients, welcomeMsg);
        }
        // task 2: done /c
        if (message.length() > 3 && message.substr(0, 3) == "/c ")
        {
          std::string chatMessage = message.substr(3); // Extract the message without /c prefix
          std::string senderInfo = std::string(inet_ntoa(sin.sin_addr)) + ":" + 
                                   std::to_string(ntohs(sin.sin_port));
          
          printf("msg from (%s): %s\n", senderInfo.c_str(), chatMessage.c_str());

          std::string broadcastMsg = "CHAT (" + senderInfo + "): " + chatMessage;
          msg_to_all_clients(sfd, clients, broadcastMsg);
        }
        else
        {
          printf("(%s:%d) %s\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port), buffer);
          
          // std::string relayMsg = "CLIENT(" + std::string(inet_ntoa(sin.sin_addr)) + ":" + 
          //                        std::to_string(ntohs(sin.sin_port)) + "): " + message;
          
          // for (const Client& client : clients)
          // {
          //   // Don't send the message back to the sender
          //   if (client.addr.sin_addr.s_addr != sin.sin_addr.s_addr || 
          //       client.addr.sin_port != sin.sin_port)
          //   {
          //     sendto(sfd, relayMsg.c_str(), relayMsg.size(), 0, 
          //           (struct sockaddr*)&client.addr, sizeof(sockaddr_in));
          //   }
          //}
        }
      }
    }
  }
  return 0;
}