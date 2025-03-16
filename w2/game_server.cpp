#include <enet/enet.h>
#include "raylib.h"
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <random>
#include <chrono>

struct Player 
{
  int id;
  float x;
  float y;
  int ping;
  ENetPeer* peer;
  std::string name;
};

int generatePlayerID()
{
  static int nextID = 1;
  return nextID++;
}

void sendPlayerList(const std::vector<Player>& players, ENetPeer* targetPeer) 
{
  std::stringstream ss;
  ss << "PLAYERS ";
  
  for (const auto& player : players) 
  {
    ss << player.id << " " << player.x << " " << player.y << " " << player.ping << ";";
  }
  
  std::string playerListStr = ss.str();
  ENetPacket* packet = enet_packet_create(playerListStr.c_str(), 
                                          playerListStr.length() + 1, 
                                          ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(targetPeer, 0, packet);
}

void broadcastNewPlayer(const Player& newPlayer, const std::vector<Player>& players) 
{
  std::stringstream ss;
  ss << "NEWPLAYER " << newPlayer.id << " " << newPlayer.name;
  std::string newPlayerStr = ss.str();
  
  for (const auto& player : players) 
  {
    if (player.id != newPlayer.id) 
    {
      ENetPacket* packet = enet_packet_create(newPlayerStr.c_str(), 
                                              newPlayerStr.length() + 1, 
                                              ENET_PACKET_FLAG_RELIABLE);
      enet_peer_send(player.peer, 0, packet);
    }
  }
}

void broadcastPositions(const std::vector<Player>& players) 
{
  for (const auto& sender : players) 
  {
    std::stringstream ss;
    ss << "POS " << sender.id << " " << sender.x << " " << sender.y;
    std::string posStr = ss.str();
    
    for (const auto& receiver : players) 
    {
      if (receiver.id != sender.id) 
      {
        ENetPacket* packet = enet_packet_create(posStr.c_str(), 
                                                posStr.length() + 1, 
                                                ENET_PACKET_FLAG_UNSEQUENCED);
        enet_peer_send(receiver.peer, 0, packet);
      }
    }
  }
}

void broadcastPings(const std::vector<Player>& players) 
{
  for (const auto& player : players) 
  {
    std::stringstream ss;
    ss << "PINGS ";
    
    for (const auto& p : players) 
    {
      ss << p.id << " " << p.ping << ";";
    }
    
    std::string pingStr = ss.str();
    
    ENetPacket* packet = enet_packet_create(pingStr.c_str(), 
                                            pingStr.length() + 1, 
                                            0);
    enet_peer_send(player.peer, 0, packet);
  }
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0) 
  {
    printf("Cannot init ENet\n");
    return 1;
  }
  
  int port = 10888;
  if (argc > 1) 
  {
    port = std::atoi(argv[1]);
  }
  
  ENetAddress address;
  address.host = ENET_HOST_ANY;
  address.port = port;
  
  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);
  
  if (!server) 
  {
    printf("Cannot create ENet game server\n");
    return 1;
  }
  
  printf("Game server started on port %d\n", port);
  
  std::vector<Player> players;
  uint32_t lastBroadcastTime = enet_time_get();
  uint32_t lastPingTime = enet_time_get(); 
  
  while (true) 
  {
    ENetEvent event;
    
    while (enet_host_service(server, &event, 10) > 0) 
    {
      switch (event.type) 
      {
        case ENET_EVENT_TYPE_CONNECT: 
        {
          printf("Player connected from %x:%u\n", event.peer->address.host, event.peer->address.port);
          
          Player newPlayer;
          newPlayer.id = generatePlayerID();
          newPlayer.name = "Player_" + std::to_string(newPlayer.id);
          newPlayer.x = GetRandomValue(100, 500);
          newPlayer.y = GetRandomValue(100, 300);
          newPlayer.ping = event.peer->roundTripTime;
          newPlayer.peer = event.peer;
          
          event.peer->data = new int(newPlayer.id);
          
          std::string welcomeMsg = "WELCOME " + std::to_string(newPlayer.id) + " " + newPlayer.name;
          ENetPacket* welcomePacket = enet_packet_create(welcomeMsg.c_str(), 
                                                         welcomeMsg.length() + 1, 
                                                         ENET_PACKET_FLAG_RELIABLE);
          enet_peer_send(event.peer, 0, welcomePacket);
          
          sendPlayerList(players, event.peer);
        
          players.push_back(newPlayer);
          
          broadcastNewPlayer(newPlayer, players);
          break;
        }
        
        case ENET_EVENT_TYPE_RECEIVE: 
        {
          int* playerID = static_cast<int*>(event.peer->data);
          if (playerID) 
          {
            auto it = std::find_if(players.begin(), players.end(), 
                                  [playerID](const Player& p) { return p.id == *playerID; });
            
            if (it != players.end()) 
            {
              it->ping = event.peer->roundTripTime;
              
              std::string msg((const char*)event.packet->data);
              
              if (msg.substr(0, 3) == "POS") 
              {
                std::istringstream ss(msg.substr(4));
                float x, y;
                if (ss >> x >> y) 
                {
                  it->x = x;
                  it->y = y;
                }
              }
            }
          }
          
          enet_packet_destroy(event.packet);
          break;
        }
        
        case ENET_EVENT_TYPE_DISCONNECT: 
        {
          printf("Player disconnected from %x:%u\n", event.peer->address.host, event.peer->address.port);
          
          int* playerID = static_cast<int*>(event.peer->data);
          if (playerID) 
          {
            auto it = std::find_if(players.begin(), players.end(), 
                                  [playerID](const Player& p) { return p.id == *playerID; });
            
            if (it != players.end()) 
            {
              int disconnectedID = *playerID;
              players.erase(it);
              
              std::string disconnectMsg = "PLAYERLEFT " + std::to_string(disconnectedID);
              for (const auto& player : players) 
              {
                ENetPacket* packet = enet_packet_create(disconnectMsg.c_str(), 
                                                       disconnectMsg.length() + 1, 
                                                       ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(player.peer, 0, packet);
              }
            }
            
            delete playerID;
            event.peer->data = nullptr;
          }
          break;
        }
        
        default:
          break;
      }
    }
    
    uint32_t currentTime = enet_time_get();
    
    if (currentTime - lastBroadcastTime > 50) 
    {
      lastBroadcastTime = currentTime;
      
      if (!players.empty()) 
      {
        broadcastPositions(players);
      }
    }

    if (currentTime - lastPingTime > 500) 
    {
      lastPingTime = currentTime;
      
      if (!players.empty()) 
      {
        broadcastPings(players);
      }
    }
  }
  
  for (auto& player : players) 
  {
    if (player.peer->data) 
    {
      delete static_cast<int*>(player.peer->data);
      player.peer->data = nullptr;
    }
  }
  
  enet_host_destroy(server);
  enet_deinitialize();
  
  return 0;
}