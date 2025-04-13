// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <functional>
#include "raylib.h"
#include <enet/enet.h>
#include <math.h>

#include <vector>
#include <chrono>
#include "entity.h"
#include "protocol.h"

using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

struct Snapshot
{
  uint16_t eid;
  float x;
  float y;
  float ori;
  TimePoint timestamp;

  Snapshot() = default;
  Snapshot(uint16_t eid, float x, float y, float ori, TimePoint timestamp)
    : eid(eid), x(x), y(y), ori(ori), timestamp(timestamp) {}
};

static std::vector<Entity> entities;
static std::unordered_map<uint16_t, size_t> indexMap;
static uint16_t my_entity = invalid_entity;
static std::unordered_map<uint16_t, std::vector<Snapshot>> snapshotHistory;
constexpr std::chrono::milliseconds INTERPOLATION_TIME{200};

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  auto itf = indexMap.find(newEntity.eid);
  if (itf != indexMap.end())
    return; // don't need to do anything, we already have entity
  indexMap[newEntity.eid] = entities.size();
  entities.push_back(newEntity);
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
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
  
  deserialize_snapshot(packet, eid, x, y, ori, timestamp);
  
  Snapshot snapshot(eid, x, y, ori, timestamp);
  snapshotHistory[eid].push_back(snapshot);
}

void process_snapshot_history(const TimePoint& currentTime)
{
  // Целевое время для интерполяции (с задержкой)
  TimePoint targetTime = currentTime - INTERPOLATION_TIME;
  
  // Обрабатываем каждую сущность
  for (auto& [eid, snapshots] : snapshotHistory)
  {
    // Если история пуста, пропускаем
    if (snapshots.empty())
      continue;
    
    // Очищаем устаревшие снэпшоты (оставляем только нужные для интерполяции)
    while (snapshots.size() > 2 && snapshots[1].timestamp < targetTime)
    {
      snapshots.erase(snapshots.begin());
    }
    
    // Если осталось меньше 2 снэпшотов, или целевое время за пределами имеющихся снэпшотов,
    // используем последний доступный снэпшот
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
    
    // Находим два снэпшота для интерполяции
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
    
    // Интерполяция между двумя снэпшотами
    const auto& s1 = snapshots[index];
    const auto& s2 = snapshots[index + 1];
    
    // Вычисляем коэффициент интерполяции (от 0 до 1)
    float t = 0.f;
    auto time_diff_s2_s1 = s2.timestamp - s1.timestamp;
    auto time_diff_target_s1 = targetTime - s1.timestamp;
    
    if (time_diff_s2_s1.count() > 0) // Избегаем деления на ноль
    {
      t = static_cast<float>(time_diff_target_s1.count()) / static_cast<float>(time_diff_s2_s1.count());
      t = std::clamp(t, 0.f, 1.f); // Ограничиваем t в диапазоне [0,1]
    }
    
    // Линейная интерполяция позиции
    float interpX = s1.x + (s2.x - s1.x) * t;
    float interpY = s1.y + (s2.y - s1.y) * t;
    
    // Интерполяция ориентации (учитываем возможное кратчайшее вращение)
    float dOri = s2.ori - s1.ori;
    // Нормализуем разницу в углах для кратчайшего пути
    if (dOri > 3.14159f)
      dOri -= 2.f * 3.14159f;
    else if (dOri < -3.14159f)
      dOri += 2.f * 3.14159f;
    
    float interpOri = s1.ori + dOri * t;
    
    // Обновляем сущность интерполированными значениями
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
      printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
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
    get_entity(my_entity, [&](Entity& e)
    {
        // Update
        float thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
        float steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);

        // Send
        send_entity_input(serverPeer, my_entity, thr, steer);
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
  address.port = 10131;

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

  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

  while (!WindowShouldClose())
  {
    float dt = GetFrameTime(); // for future use and making it look smooth
    TimePoint curTime = std::chrono::steady_clock::now();

    update_net(client, serverPeer);
    process_snapshot_history(curTime);
    simulate_world(serverPeer);
    draw_world(camera);
  }

  CloseWindow();
  return 0;
}
