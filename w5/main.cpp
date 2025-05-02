// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <functional>
#include "raylib.h"
#include <enet/enet.h>
#include <math.h>

#include <vector>
#include <chrono>
#include <deque>
#include <cmath>
#include "entity.h"
#include "protocol.h"

using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

struct InputCommand {
    uint32_t frameNumber;
    float thr;
    float steer;
    TimePoint timestamp;
};

struct Snapshot
{
  uint16_t eid;
  float x;
  float y;
  float ori;
  TimePoint timestamp;
  uint32_t frameNumber;

  Snapshot() = default;
  Snapshot(uint16_t eid, float x, float y, float ori, TimePoint timestamp, uint32_t frameNumber = 0)
    : eid(eid), x(x), y(y), ori(ori), timestamp(timestamp), frameNumber(frameNumber) {}
};

static std::vector<Entity> entities;
static std::unordered_map<uint16_t, size_t> indexMap;
static uint16_t my_entity = invalid_entity;
static std::unordered_map<uint16_t, std::vector<Snapshot>> snapshotHistory;
constexpr std::chrono::milliseconds INTERPOLATION_TIME{200};

// Client prediction
static std::deque<InputCommand> inputHistory;
static uint32_t clientFrameCounter = 0;
static uint32_t lastAcknowledgedFrame = 0;
static bool pendingCorrection = false;
static Snapshot serverState;
constexpr float PREDICTION_ERROR_THRESHOLD = 0.5f;

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  auto itf = indexMap.find(newEntity.eid);
  if (itf != indexMap.end())
    return; // don't need to do anything, we already have entity
  
  printf("Received new entity with ID: %u\n", newEntity.eid);
  indexMap[newEntity.eid] = entities.size();
  entities.push_back(newEntity);
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
  printf("Set controlled entity to: %u\n", my_entity);
}

template<typename Callable>
static void get_entity(uint16_t eid, Callable c)
{
  auto itf = indexMap.find(eid);
  if (itf != indexMap.end())
    c(entities[itf->second]);
}

void on_snapshot(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; float ori = 0.f;
  TimePoint timestamp;
  uint32_t frameNumber;
  
  deserialize_snapshot(packet, eid, x, y, ori, timestamp, frameNumber);
  
  Snapshot snapshot(eid, x, y, ori, timestamp, frameNumber);
  
  if (eid == my_entity) {
    serverState = snapshot;
    lastAcknowledgedFrame = frameNumber;
    
    // Remove acknowledged inputs
    while (!inputHistory.empty() && inputHistory.front().frameNumber <= frameNumber) {
      inputHistory.pop_front();
    }
    
    // Check prediction error
    get_entity(my_entity, [&](Entity& e) {
      float dx = e.x - x;
      float dy = e.y - y;
      float posError = sqrt(dx*dx + dy*dy);
      
      if (posError > PREDICTION_ERROR_THRESHOLD) {
        // printf("Prediction error detected: %f units. Applying correction.\n", posError);
        pendingCorrection = true;
      }
    });
  }
  
  snapshotHistory[eid].push_back(snapshot);
  
  auto& snapshots = snapshotHistory[eid];
  if (snapshots.size() > 1 && snapshots.back().frameNumber < snapshots[snapshots.size() - 2].frameNumber) {
    std::sort(snapshots.begin(), snapshots.end(), 
              [](const Snapshot& a, const Snapshot& b) { return a.frameNumber < b.frameNumber; });
  }
}

void process_snapshot_history(const TimePoint& currentTime)
{
  // Целевое время для интерполяции (с задержкой)
  TimePoint targetTime = currentTime - INTERPOLATION_TIME;
  
  for (auto& [eid, snapshots] : snapshotHistory)
  {
    // раскоментить, если надо посмотреть на клиента без интерполяции
    // if (eid == my_entity) 
    //   continue;
    
    if (snapshots.empty())
      continue;
    
    // Очищаем устаревшие снэпшоты (оставляем только нужные для интерполяции)
    while (snapshots.size() > 2 && snapshots[1].timestamp < targetTime)
    {
      snapshots.erase(snapshots.begin());
    }
    
    // < 2 снэпшотов => используем последний 
    if (snapshots.size() < 2 || targetTime <= snapshots[0].timestamp)
    {
      const auto& snapshot = snapshots[0];
      get_entity(eid, [&](Entity& e)
      {
        e.x = snapshot.x;
        e.y = snapshot.y;
        e.ori = snapshot.ori;
      });
      continue;
    }
    
    size_t index = 0;
    while (index < snapshots.size() - 1 && snapshots[index + 1].timestamp <= targetTime)
    {
      index++;
    }
    
    if (index >= snapshots.size() - 1)
    {
      const auto& snapshot = snapshots.back();
      get_entity(eid, [&](Entity& e)
      {
        e.x = snapshot.x;
        e.y = snapshot.y;
        e.ori = snapshot.ori;
      });
      continue;
    }
    
    const auto& s1 = snapshots[index];
    const auto& s2 = snapshots[index + 1];
    
    float t = 0.f;
    auto time_diff_s2_s1 = s2.timestamp - s1.timestamp;
    auto time_diff_target_s1 = targetTime - s1.timestamp;
    
    if (time_diff_s2_s1.count() > 0)
    {
      t = static_cast<float>(time_diff_target_s1.count()) / static_cast<float>(time_diff_s2_s1.count());
      t = std::clamp(t, 0.f, 1.f);
    }
    
    // Линейная интерполяция позиции
    float interpX = s1.x + (s2.x - s1.x) * t;
    float interpY = s1.y + (s2.y - s1.y) * t;
    
    // Кратчайший путь по углу
    float dOri = s2.ori - s1.ori;

    if (dOri > 3.14159f)
      dOri -= 2.f * 3.14159f;
    else if (dOri < -3.14159f)
      dOri += 2.f * 3.14159f;
    
    float interpOri = s1.ori + dOri * t;
    
    get_entity(eid, [&](Entity& e)
    {
      e.x = interpX;
      e.y = interpY;
      e.ori = interpOri;
    });
  }
}

static void on_time(ENetPacket *packet, ENetPeer* peer)
{
  uint32_t timeMsec;
  deserialize_time_msec(packet, timeMsec);
  enet_time_set(timeMsec + peer->lastRoundTripTime / 2);
}

static void draw_entity(const Entity& e)
{
  const float shipLen = 3.f;
  const float shipWidth = 2.f;
  const Vector2 fwd = Vector2{cosf(e.ori), sinf(e.ori)};
  const Vector2 left = Vector2{-fwd.y, fwd.x};
  DrawTriangle(Vector2{e.x + fwd.x * shipLen * 0.5f, e.y + fwd.y * shipLen * 0.5f},
               Vector2{e.x - fwd.x * shipLen * 0.5f - left.x * shipWidth * 0.5f, e.y - fwd.y * shipLen * 0.5f - left.y * shipWidth * 0.5f},
               Vector2{e.x - fwd.x * shipLen * 0.5f + left.x * shipWidth * 0.5f, e.y - fwd.y * shipLen * 0.5f + left.y * shipWidth * 0.5f},
               GetColor(e.color));
}

static void update_net(ENetHost* client, ENetPeer* serverPeer)
{
  ENetEvent event;
  while (enet_host_service(client, &event, 0) > 0)
  {
    switch (event.type)
    {
    case ENET_EVENT_TYPE_CONNECT:
      printf("Connected to server, sending join request\n");
      send_join(serverPeer);
      break;
    case ENET_EVENT_TYPE_RECEIVE:
      switch (get_packet_type(event.packet))
      {
      case E_SERVER_TO_CLIENT_NEW_ENTITY:
        on_new_entity_packet(event.packet);
        break;
      case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
        on_set_controlled_entity(event.packet);
        break;
      case E_SERVER_TO_CLIENT_SNAPSHOT:
        on_snapshot(event.packet);
        break;
      case E_SERVER_TO_CLIENT_TIME_MSEC:
        on_time(event.packet, event.peer);
        break;
      };
      enet_packet_destroy(event.packet);
      break;
    default:
      break;
    };
  }
}

static void simulate_world(ENetPeer* serverPeer)
{
  if (my_entity != invalid_entity)
  {
    bool left = IsKeyDown(KEY_LEFT);
    bool right = IsKeyDown(KEY_RIGHT);
    bool up = IsKeyDown(KEY_UP);
    bool down = IsKeyDown(KEY_DOWN);
    
    float thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
    float steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);
    
    InputCommand cmd = {
      clientFrameCounter,
      thr,
      steer,
      std::chrono::steady_clock::now()
    };
    inputHistory.push_back(cmd);
    
    // Не больше 100 команд в истории
    while (inputHistory.size() > 100) {
      inputHistory.pop_front();
    }
    
    send_entity_input(serverPeer, my_entity, thr, steer);
    
    get_entity(my_entity, [&](Entity& e)
    {
      if (pendingCorrection) {
        e.x = serverState.x;
        e.y = serverState.y;
        e.ori = serverState.ori;
        
        for (const auto& input : inputHistory) {
          e.thr = input.thr;
          e.steer = input.steer;
          // Симуляция вместо simulate_entity
          e.vx += cosf(e.ori) * e.thr * FIXED_DT;
          e.vy += sinf(e.ori) * e.thr * FIXED_DT;
          e.omega += e.steer * FIXED_DT * 0.3f;
          e.ori += e.omega * FIXED_DT;
          e.x += e.vx * FIXED_DT;
          e.y += e.vy * FIXED_DT;
        }
        
        pendingCorrection = false;
      }
      else {
        e.thr = thr;
        e.steer = steer;
        // Симуляция вместо simulate_entity
        e.vx += cosf(e.ori) * e.thr * FIXED_DT;
        e.vy += sinf(e.ori) * e.thr * FIXED_DT;
        e.omega += e.steer * FIXED_DT * 0.3f;
        e.ori += e.omega * FIXED_DT;
        e.x += e.vx * FIXED_DT;
        e.y += e.vy * FIXED_DT;
      }
    });
  }
}

static void draw_world(const Camera2D& camera)
{
  BeginDrawing();
    ClearBackground(GRAY);
    BeginMode2D(camera);

      for (const Entity &e : entities)
        draw_entity(e);

    EndMode2D();
    
    // Draw debug info for client-side
    if (my_entity != invalid_entity) {
      char buffer[100];
      get_entity(my_entity, [&](const Entity& e) 
      {
        sprintf(buffer, "Pos: (%.1f, %.1f) Frame: %u TotalFrames: %u", 
                e.x, e.y, clientFrameCounter, lastAcknowledgedFrame);
      });
      DrawText(buffer, 10, 10, 20, BLACK);
    }
    
  EndDrawing();
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10131; // Make sure this matches the server port!

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

  int width = 600;
  int height = 600;

  InitWindow(width, height, "w5 networked MIPT");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
  camera.target = Vector2{ 0.f, 0.f };
  camera.offset = Vector2{ width * 0.5f, height * 0.5f };
  camera.rotation = 0.f;
  camera.zoom = 10.f;

  SetTargetFPS(60);

  // Setup for fixed timestep
  float accumulator = 0.0f;
  clientFrameCounter = 0;

  while (!WindowShouldClose())
  {
    float frameTime = GetFrameTime();
    accumulator += frameTime;

    update_net(client, serverPeer);
    
    // Handle fixed timestep for prediction and simulation
    while (accumulator >= FIXED_DT) {
      simulate_world(serverPeer);
      clientFrameCounter++;
      accumulator -= FIXED_DT;
    }
    
    // Process snapshots for other entities
    process_snapshot_history(std::chrono::steady_clock::now());
    
    draw_world(camera);
  }

  CloseWindow();
  return 0;
}