#include <cstring> // std::memcpy
#include <chrono>

#include "protocol.h"
#include "bitstream.h"

void send_join(ENetPeer *peer)
{
  BitStream bs;
  bs.Write<uint8_t>(E_CLIENT_TO_SERVER_JOIN);
  ENetPacket *packet = enet_packet_create(bs.GetData(), bs.GetSizeBytes(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  BitStream bs;
  bs.Write<uint8_t>(E_SERVER_TO_CLIENT_NEW_ENTITY);y:
  bs.Write<uint32_t>(ent.color);
  bs.Write<float>(ent.x);
  bs.Write<float>(ent.y);
  bs.Write<uint16_t>(ent.eid);
  bs.Write<float>(ent.vx);
  bs.Write<float>(ent.vy);
  bs.Write<float>(ent.ori);
  bs.Write<float>(ent.omega);
  bs.Write<float>(ent.thr);
  bs.Write<float>(ent.steer);
  bs.Write<uint16_t>(ent.eid);
  
  ENetPacket *packet = enet_packet_create(bs.GetData(), bs.GetSizeBytes(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);

}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  BitStream bs;
  bs.Write<uint8_t>(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
  bs.Write<uint16_t>(eid);
  ENetPacket *packet = enet_packet_create(bs.GetData(), bs.GetSizeBytes(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float steer)
{
  BitStream bs;
  bs.Write<uint8_t>(E_CLIENT_TO_SERVER_INPUT);
  bs.Write<uint16_t>(eid);
  bs.Write<float>(thr);
  bs.Write<float>(steer);

  ENetPacket *packet = enet_packet_create(bs.GetData(), bs.GetSizeBytes(), ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float ori, float vx, float vy, float omega, TimePoint timestamp, uint32_t frameNumber)
{
  auto duration = timestamp.time_since_epoch();
  uint64_t timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

  BitStream bs;
  bs.Write<uint8_t>(E_SERVER_TO_CLIENT_SNAPSHOT);
  bs.Write<uint16_t>(eid);
  bs.Write<float>(x);
  bs.Write<float>(y);
  bs.Write<float>(ori);
  bs.Write<float>(vx);
  bs.Write<float>(vy);
  bs.Write<float>(omega);
  bs.Write<uint64_t>(timestamp_ms);
  bs.Write<uint32_t>(frameNumber);

  ENetPacket *packet = enet_packet_create(bs.GetData(), bs.GetSizeBytes(), ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 1, packet);
}

void send_time_msec(ENetPeer *peer, uint32_t timeMsec)
{
  BitStream bs;
  bs.Write<uint8_t>(E_SERVER_TO_CLIENT_TIME_MSEC);
  bs.Write<uint32_t>(timeMsec);

  ENetPacket *packet = enet_packet_create(bs.GetData(), bs.GetSizeBytes(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  BitStream bs(packet->data, packet->dataLength);
  uint8_t type;
  bs.Read<uint8_t>(type);
  bs.Read<uint32_t>(ent.color);
  bs.Read<float>(ent.x);
  bs.Read<float>(ent.y);
  bs.Read<uint16_t>(ent.eid);
  bs.Read<float>(ent.vx);
  bs.Read<float>(ent.vy);
  bs.Read<float>(ent.ori);
  bs.Read<float>(ent.omega);
  bs.Read<float>(ent.thr);
  bs.Read<float>(ent.steer);
  bs.Read<uint16_t>(ent.eid);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  BitStream bs(packet->data, packet->dataLength);
  uint8_t type;
  bs.Read<uint8_t>(type);
  bs.Read<uint16_t>(eid);
}

void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, float &thr, float &steer)
{
  BitStream bs(packet->data, packet->dataLength);
  uint8_t type;
  bs.Read<uint8_t>(type);
  bs.Read<uint16_t>(eid);
  bs.Read<float>(thr);
  bs.Read<float>(steer);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &ori, float &vx, float &vy, float &omega, TimePoint &timestamp, uint32_t &frameNumber)
{
  BitStream bs(packet->data, packet->dataLength);
  uint8_t type;
  bs.Read<uint8_t>(type);
  bs.Read<uint16_t>(eid);
  bs.Read<float>(x);
  bs.Read<float>(y);
  bs.Read<float>(ori);
  bs.Read<float>(vx);
  bs.Read<float>(vy);
  bs.Read<float>(omega);
  uint64_t timestamp_ms = 0;
  bs.Read<uint64_t>(timestamp_ms);
  bs.Read<uint32_t>(frameNumber);

  timestamp = TimePoint(std::chrono::milliseconds(timestamp_ms));
}

void deserialize_time_msec(ENetPacket *packet, uint32_t &timeMsec)
{
  BitStream bs(packet->data, packet->dataLength);
  uint8_t type;
  bs.Read<uint8_t>(type);
  bs.Read<uint32_t>(timeMsec);
}

