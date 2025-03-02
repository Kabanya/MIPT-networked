#include <cstring>
#include <thread>

#include "socket_tools.h"
#include "server_tools.h"

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
        Client currentClient;
        currentClient.addr = sin;
        currentClient.identifier = client_to_string(currentClient);
        bool clientExists = false;
        
        for (const Client& client : clients)
        {
          if (client.addr.sin_addr.s_addr == sin.sin_addr.s_addr && client.addr.sin_port == sin.sin_port)
          {
            clientExists = true;
            currentClient = client;
            break;
          }
        }
        
        if(!clientExists)
        {
          clients.push_back(currentClient);
          
          std::string welcomeMsg = "\n/c - message to all users\n/mathduel - challenge someone to a math duel\n/help - for help";
          sendto(sfd, welcomeMsg.c_str(), welcomeMsg.size(), 0, (struct sockaddr*)&sin, slen);
        }
        
        mathduel(message, currentClient, sfd, clients);
      
        if (message.length() > 3 && message.substr(0, 3) == "/c ") //mb better to move into server_tools
        {
          //extrarct & send message
          std::string chatMessage = message.substr(3);
          std::string senderInfo = client_to_string(currentClient);
          
          printf("msg from (%s): %s\n", senderInfo.c_str(), chatMessage.c_str());

          std::string broadcastMsg = "CHAT (" + senderInfo + "): " + chatMessage;
          msg_to_all_clients(sfd, clients, broadcastMsg);
        }
        else
        {
          printf("(%s) %s\n", currentClient.identifier.c_str(), buffer);
        }
      }
    }
    void cleanup_inactive_duels(std::vector<MathDuel>& activeDuels);
  }


  return 0;
}