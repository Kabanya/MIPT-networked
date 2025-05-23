#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include "mathUtils.h"
#include <stdlib.h>
#include <vector>
#include <map>
#include <chrono>

uint32_t frameCounter = 0;
TimePoint serverStartTime;
constexpr int DELAY = 200000; // 200 ms

static std::vector<Entity> entities;
static std::map<uint16_t, ENetPeer*> controlledMap;

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const Entity &ent : entities)

    send_new_entity(peer, ent);

  // find max eid
  uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
  for (const Entity &e : entities)
    maxEid = std::max(maxEid, e.eid);
  uint16_t newEid = maxEid + 1;
  uint32_t color = 0xff000000 +
                   0x00440000 * (rand() % 5) +
                   0x00004400 * (rand() % 5) +
                   0x00000044 * (rand() % 5);
  float x = (rand() % 4) * 5.f;
  float y = (rand() % 4) * 5.f;
  // Entity ent = {color, x, y, 0.f, (rand() / RAND_MAX) * 3.141592654f, 0.f, 0.f, 0.f, 0.f, newEid};
  Entity ent;
  ent.color = color;
  ent.x = x;
  ent.y = y;
  ent.vx = 0.f;
  ent.vy = 0.f;
  ent.ori = (rand() / (float)RAND_MAX) * 3.141592654f;
  ent.omega = 0.f;
  ent.thr = 0.f;
  ent.steer = 0.f;
  ent.eid = newEid;
  entities.push_back(ent);

  controlledMap[newEid] = peer;

  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_input(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float thr = 0.f; float steer = 0.f;
  deserialize_entity_input(packet, eid, thr, steer);
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.thr = thr;
      e.steer = steer;
    }
}

static void update_net(ENetHost* server)
{
  ENetEvent event;
  while (enet_host_service(server, &event, 0) > 0)
  {
    switch (event.type)
    {
    case ENET_EVENT_TYPE_CONNECT:
      printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
      break;
    case ENET_EVENT_TYPE_RECEIVE:
      switch (get_packet_type(event.packet))
      {
        case E_CLIENT_TO_SERVER_JOIN:
          on_join(event.packet, event.peer, server);
          break;
        case E_CLIENT_TO_SERVER_INPUT:
          on_input(event.packet);
          break;
        };
      enet_packet_destroy(event.packet);
      break;
    default:
      break;
    };
  }
}

static void simulate_world(ENetHost* server, float dt)
{
  TimePoint curTime = std::chrono::steady_clock::now();
  for (Entity &e : entities)
  {
    // simulate
    simulate_entity(e, dt); // 1.f/32.f
    // send
    for (size_t i = 0; i < server->peerCount; ++i)
    {
      ENetPeer *peer = &server->peers[i];
      //if (controlledMap[e.eid] != peer)
      send_snapshot(peer, e.eid, e.x, e.y, e.ori, e.vx, e.vy, e.omega, curTime, frameCounter);
    }
  }
}

static void update_time(ENetHost* server, uint32_t curTime)
{
  // We can send it less often too
  for (size_t i = 0; i < server->peerCount; ++i)
    send_time_msec(&server->peers[i], curTime);
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  serverStartTime = std::chrono::steady_clock::now();
  frameCounter = 0;

  uint32_t lastTime = enet_time_get();
  float accumulatedTime = 0.0f;
  
  while (true)
  {
    uint32_t curTime = enet_time_get();
    uint32_t elapsed = curTime - lastTime;
    lastTime = curTime;
    
    accumulatedTime += elapsed;
    
    if (accumulatedTime >= FIXED_DT * 1000.0f)
    {
      simulate_world(server, FIXED_DT);
      update_net(server);
      update_time(server, curTime);
      
      frameCounter++;
      accumulatedTime -= FIXED_DT * 1000.0f;
      // std::cout << "Frame " << frameCounter << " processed, remaining time: " << accumulatedTime << " ms" << std::endl;
    }
    
    // usleep(100000);
    usleep(DELAY); // 200ms
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}
