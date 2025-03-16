#include <enet/enet.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>

struct GameServerInfo 
{
  int port;
  std::string host;
  bool sessionStarted;
};

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0) 
  {
    printf("Cannot init ENet\n");
    return 1;
  }
  
  ENetAddress address;
  address.host = ENET_HOST_ANY;
  address.port = 10887;
  
  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);
  
  if (!server) 
  {
    printf("Cannot create ENet lobby server\n");
    return 1;
  }
  
  printf("Lobby server started on port %d\n", address.port);
  
  GameServerInfo gameServer;
  gameServer.host = "localhost";
  gameServer.port = 10888;
  gameServer.sessionStarted = false;
  
  if (argc > 2) 
  {
    gameServer.host = argv[1];
    gameServer.port = std::atoi(argv[2]);
  }
  
  printf("Game server will be at %s:%d\n", gameServer.host.c_str(), gameServer.port);
  
  std::vector<ENetPeer*> connectedPeers;
  
  while (true) 
  {
    ENetEvent event;
    while (enet_host_service(server, &event, 10) > 0) 
    {
      switch (event.type) 
      {
        case ENET_EVENT_TYPE_CONNECT: 
        {
          printf("Client connected from %x:%u\n", event.peer->address.host, event.peer->address.port);
          
          connectedPeers.push_back(event.peer);
          
          if (gameServer.sessionStarted) 
          {
            std::string gameServerMsg = "GAMESERVER " + gameServer.host + " " +  std::to_string(gameServer.port);
            
            ENetPacket* packet = enet_packet_create(gameServerMsg.c_str(), 
                                                   gameServerMsg.length() + 1, 
                                                   ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(event.peer, 0, packet);
            
            printf("Sent game server info to new player: %s\n", gameServerMsg.c_str());
          }
          break;
        }
        
        case ENET_EVENT_TYPE_RECEIVE: 
        {
          printf("Packet received from %x:%u: '%s'\n", 
                event.peer->address.host, 
                event.peer->address.port, 
                event.packet->data);
          
          if (!gameServer.sessionStarted && 
              strcmp((const char*)event.packet->data, "Start!") == 0) 
          {
            
            printf("Game session start requested!\n");
            gameServer.sessionStarted = true;
            
            std::string gameServerMsg = "GAMESERVER " + gameServer.host + " " + std::to_string(gameServer.port);
            
            for (auto peer : connectedPeers) 
            {
              ENetPacket* packet = enet_packet_create(gameServerMsg.c_str(), 
                                                     gameServerMsg.length() + 1, 
                                                     ENET_PACKET_FLAG_RELIABLE);
              enet_peer_send(peer, 0, packet);
            }
            
            printf("Sent game server info to all connected players: %s\n", gameServerMsg.c_str());
          }
          
          enet_packet_destroy(event.packet);
          break;
        }
        
        case ENET_EVENT_TYPE_DISCONNECT: 
        {
          printf("Client disconnected from %x:%u\n", event.peer->address.host, event.peer->address.port);
          
          auto it = std::find(connectedPeers.begin(), connectedPeers.end(), event.peer);
          if (it != connectedPeers.end()) 
          {
            connectedPeers.erase(it);
          }
          
          event.peer->data = nullptr;
          break;
        }
        
        default:
          break;
      }
    }
  }
  
  enet_host_destroy(server);
  enet_deinitialize();
  
  return 0;
}