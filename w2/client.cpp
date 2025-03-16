#include "raylib.h"
#include <enet/enet.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

struct Player 
{
  int id;
  float x;
  float y;
  int ping;
};

void send_fragmented_packet(ENetPeer *peer)
{
  const char *baseMsg = "Stay awhile and listen. ";
  const size_t msgLen = strlen(baseMsg);

  const size_t sendSize = 2500;
  char *hugeMessage = new char[sendSize];
  for (size_t i = 0; i < sendSize; ++i)
    hugeMessage[i] = baseMsg[i % msgLen];
  hugeMessage[sendSize-1] = '\0';

  ENetPacket *packet = enet_packet_create(hugeMessage, sendSize, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);

  delete[] hugeMessage;
}

void send_micro_packet(ENetPeer *peer)
{
  const char *msg = "dv/dt";
  ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 1, packet);
}

void send_position(ENetPeer *peer, float x, float y)
{
  char posMsg[64];
  snprintf(posMsg, sizeof(posMsg), "POS %.2f %.2f", x, y);
  ENetPacket *packet = enet_packet_create(posMsg, strlen(posMsg) + 1, ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 0, packet);
}

int main(int argc, const char **argv)
{
  int width = 800;
  int height = 600;
  InitWindow(width, height, "w2 MIPT networked");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  SetTargetFPS(60);

  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 2, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress lobbyAddress;
  enet_address_set_host(&lobbyAddress, "localhost");
  lobbyAddress.port = 10887;

  ENetPeer *lobbyPeer = enet_host_connect(client, &lobbyAddress, 2, 0);
  if (!lobbyPeer)
  {
    printf("Cannot connect to lobby");
    return 1;
  }

  uint32_t timeStart = enet_time_get();
  uint32_t lastFragmentedSendTime = timeStart;
  uint32_t lastMicroSendTime = timeStart;
  uint32_t lastPositionSendTime = timeStart;
  bool connectedToLobby = false;
  bool connectedToGameServer = false;
  
  ENetPeer *gamePeer = nullptr;
  std::string gameServerStatus = "Connecting to lobby...";
  std::vector<Player> players;
  
  float posx = GetRandomValue(100, 500);
  float posy = GetRandomValue(100, 500);
  float velx = 0.f;
  float vely = 0.f;
  
  int myPlayerId = -1;
  
  while (!WindowShouldClose())
  {
    const float dt = GetFrameTime();
    ENetEvent event;
    
    while (enet_host_service(client, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        if (event.peer == lobbyPeer) 
        {
          connectedToLobby = true;
          gameServerStatus = "Connected to lobby";
        } 
        else if (event.peer == gamePeer) 
        {
          connectedToGameServer = true;
          gameServerStatus = "Connected to game server";
        }
        break;
        
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s'\n", event.packet->data);
        
        if (event.peer == lobbyPeer && !connectedToGameServer) 
        {
          const char* data = (const char*)event.packet->data;
          
          if (strncmp(data, "GAMESERVER", 10) == 0) 
          {
            char serverIP[256];
            int serverPort;
            if (sscanf(data, "GAMESERVER %s %d", serverIP, &serverPort) == 2) 
            {
              ENetAddress gameAddress;
              enet_address_set_host(&gameAddress, serverIP);
              gameAddress.port = serverPort;
              
              gamePeer = enet_host_connect(client, &gameAddress, 2, 0);
              if (!gamePeer) 
              {
                gameServerStatus = "Cannot connect to game server";
              } 
              else 
              {
                gameServerStatus = "Connecting to game server...";
              }
            }
          }
        }
        else if (event.peer == gamePeer) 
        {
          const char* data = (const char*)event.packet->data;
          
          if (strncmp(data, "WELCOME", 7) == 0) 
          {
            int id;
            char name[256];
            if (sscanf(data, "WELCOME %d %s", &id, name) >= 1) 
            {
              myPlayerId = id;
              gameServerStatus = "Playing as player " + std::to_string(myPlayerId);
            }
          }
          else if (strncmp(data, "PLAYERS", 7) == 0) 
          {
            players.clear();
            
            std::string playerData((const char*)event.packet->data);
            std::istringstream ss(playerData.substr(8));
            
            Player player;
            std::string token;
            
            while (std::getline(ss, token, ';')) 
            {
              std::istringstream playerStream(token);
              if (playerStream >> player.id >> player.x >> player.y >> player.ping) 
              {
                players.push_back(player);
              }
            }
          }
          else if (strncmp(data, "POS", 3) == 0) 
          {
            int playerId;
            float x, y;
            if (sscanf(data, "POS %d %f %f", &playerId, &x, &y) == 3) 
            {
              for (auto& player : players) 
              {
                if (player.id == playerId) 
                {
                  player.x = x;
                  player.y = y;
                  break;
                }
              }
            }
          }
          else if (strncmp(data, "NEWPLAYER", 9) == 0) 
          {
            int playerId;
            char playerName[256];
            if (sscanf(data, "NEWPLAYER %d %s", &playerId, playerName) >= 1) 
            {
              bool playerExists = false;
              for (const auto& player : players) 
              {
                if (player.id == playerId) 
                {
                  playerExists = true;
                  break;
                }
              }
              
              //add new players in list 
              if (!playerExists) 
              {
                Player newPlayer;
                newPlayer.id = playerId;
                newPlayer.x = 0.0f; 
                newPlayer.y = 0.0f;
                newPlayer.ping = 0;
                players.push_back(newPlayer);
              }
            }
          }
          //player disconnection
          else if (strncmp(data, "PLAYERLEFT", 10) == 0) 
          {
            int playerId;
            if (sscanf(data, "PLAYERLEFT %d", &playerId) == 1) 
            {
              for (auto it = players.begin(); it != players.end(); ++it) 
              {
                if (it->id == playerId) 
                {
                  players.erase(it);
                  break;
                }
              }
            }
          }
          else if (strncmp(data, "PINGS", 5) == 0) 
          {
            printf("Processing ping data: %s\n", data);
            std::string pingData((const char*)event.packet->data);
            std::istringstream ss(pingData.substr(6));
            
            std::string token;
            while (std::getline(ss, token, ';')) 
            {
              if (token.empty()) continue;
              
              int playerId, pingValue;
              std::istringstream pingStream(token);
              if (pingStream >> playerId >> pingValue) 
              {
                printf("Received ping update: Player %d, Ping %d\n", playerId, pingValue);
                bool updated = false;
                for (auto& player : players) 
                {
                  if (player.id == playerId) 
                  {
                    player.ping = pingValue;
                    updated = true;
                    break;
                  }
                }
                
                if (!updated && playerId != myPlayerId) 
                {
                  printf("Adding new player from ping data: %d\n", playerId);
                  Player newPlayer;
                  newPlayer.id = playerId;
                  newPlayer.x = 0.0f;
                  newPlayer.y = 0.0f;
                  newPlayer.ping = pingValue;
                  players.push_back(newPlayer);
                }
              }
            }
          }
        }
        
        enet_packet_destroy(event.packet);
        break;
        
      case ENET_EVENT_TYPE_DISCONNECT:
        printf("Disconnected from %x:%u\n", event.peer->address.host, event.peer->address.port);
        if (event.peer == lobbyPeer) 
        {
          connectedToLobby = false;
          gameServerStatus = "Disconnected from lobby";
          lobbyPeer = nullptr;
        } 
        else if (event.peer == gamePeer) 
        {
          connectedToGameServer = false;
          gameServerStatus = "Disconnected from game server";
          gamePeer = nullptr;
        }
        break;
        
      default:
        break;
      };
    }
    
    if (connectedToGameServer) 
    {
      uint32_t curTime = enet_time_get();
      if (curTime - lastPositionSendTime > 50) 
      {
        lastPositionSendTime = curTime;
        send_position(gamePeer, posx, posy);
      }
    }
    
    if (connectedToLobby) 
    {
      uint32_t curTime = enet_time_get();
      if (curTime - lastFragmentedSendTime > 1000) 
      {
        lastFragmentedSendTime = curTime;
        // send_fragmented_packet(lobbyPeer);
      }
      if (curTime - lastMicroSendTime > 100) 
      {
        lastMicroSendTime = curTime;
        // send_micro_packet(lobbyPeer);
      }
    }

    if (IsKeyPressed(KEY_ESCAPE))
      break;
      
    if (IsKeyPressed(KEY_ENTER) && connectedToLobby) 
    {
      ENetPacket *packet = enet_packet_create("Start!", strlen("Start!") + 1, ENET_PACKET_FLAG_RELIABLE);
      enet_peer_send(lobbyPeer, 0, packet);
    }
    
    bool left = IsKeyDown(KEY_LEFT);
    bool right = IsKeyDown(KEY_RIGHT);
    bool up = IsKeyDown(KEY_UP);
    bool down = IsKeyDown(KEY_DOWN);
    constexpr float accel = 30.f;
    velx += ((left ? -1.f : 0.f) + (right ? 1.f : 0.f)) * dt * accel;
    vely += ((up ? -1.f : 0.f) + (down ? 1.f : 0.f)) * dt * accel;
    posx += velx * dt;
    posy += vely * dt;
    velx *= 0.99f;
    vely *= 0.99f;

    BeginDrawing();
      ClearBackground(BLACK);
      
      DrawText(TextFormat("Current status: %s", gameServerStatus.c_str()), 20, 20, 20, WHITE);
      DrawText(TextFormat("My position: (%d, %d)", (int)posx, (int)posy), 20, 40, 20, WHITE);
      
      DrawCircleV(Vector2{posx, posy}, 10.f, WHITE);
      
      DrawText("List of players:", 20, 60, 20, WHITE);
      
      int yOffset = 80;
      for (const auto& player : players) 
      {
        DrawText(TextFormat("Player %d: (%d, %d) - Ping: %d ms", 
                          player.id, (int)player.x, (int)player.y, player.ping), 
                          20, yOffset, 18, WHITE);
        yOffset += 20;
        
        if (player.id != myPlayerId) 
        {
          DrawCircleV(Vector2{player.x, player.y}, 10.f, RED);
          DrawText(TextFormat("%d", player.id), player.x - 5, player.y - 5, 16, WHITE);
        }
      }
      
    EndDrawing();
  }
  
  if (lobbyPeer) 
  {
    enet_peer_disconnect(lobbyPeer, 0);
  }
  if (gamePeer) 
  {
    enet_peer_disconnect(gamePeer, 0);
  }
  
  ENetEvent event;
  while (enet_host_service(client, &event, 1000) > 0) 
  {
    if (event.type == ENET_EVENT_TYPE_RECEIVE) 
    {
      enet_packet_destroy(event.packet);
    }
  }
  
  enet_host_destroy(client);
  enet_deinitialize();
  
  CloseWindow();
  
  return 0;
}