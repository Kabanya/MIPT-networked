#include "raylib.h"
#include <enet/enet.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

struct Player {
    int id;
    int ping;
    float x;
    float y;
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
  ENetPacket *packet = enet_packet_create(posMsg, strlen(posMsg) + 1, ENET_PACKET_FLAG_RELIABLE);
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

  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

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
  
  std::vector<Player> players;
  
  float posx = GetRandomValue(100, 500);
  float posy = GetRandomValue(100, 500);
  float velx = 0.f;
  float vely = 0.f;
  
  while (!WindowShouldClose())
  {
    const float dt = GetFrameTime();
    ENetEvent event;
    
    while (enet_host_service(client, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connected to %x:%u\n", event.peer->address.host, event.peer->address.port);
        connectedToLobby = true;
        break;
        }
        
        enet_packet_destroy(event.packet);
        break;
      };
    
    if (connectedToLobby) {
      uint32_t curTime = enet_time_get();
      if (curTime - lastFragmentedSendTime > 1000) {
        lastFragmentedSendTime = curTime;
        // send_fragmented_packet(lobbyPeer);
      }
      if (curTime - lastMicroSendTime > 100) {
        lastMicroSendTime = curTime;
        // send_micro_packet(lobbyPeer);
      }
    }

    if (IsKeyPressed(KEY_ESCAPE))
      break;
      
    if (IsKeyPressed(KEY_ENTER) && connectedToLobby) {
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
      
      DrawText(TextFormat("My position: (%d, %d)", (int)posx, (int)posy), 20, 40, 20, WHITE);
  
      DrawCircleV(Vector2{posx, posy}, 10.f, WHITE);
      
      DrawText("List of players:", 20, 60, 20, WHITE);
      
      int yOffset = 80;
      for (const auto& player : players) {
        DrawText(TextFormat("Player %d: (%d, %d) - Ping: %d ms", 
                            player.id, (int)player.x, (int)player.y, player.ping), 
                            20, yOffset, 18, WHITE);
        yOffset += 20;
        
        if (player.id != GetRandomValue(0, 1000)) {
          DrawCircleV(Vector2{player.x, player.y}, 10.f, RED);
          DrawText(TextFormat("%d", player.id), player.x - 5, player.y - 5, 16, WHITE);
        }
      }
      
    EndDrawing();
  }

  
  CloseWindow();
  
  return 0;
}