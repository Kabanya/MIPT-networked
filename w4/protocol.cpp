#include "protocol.h"
#include <cstring> // memcpy
#include "bitstream.h"


void send_join(ENetPeer *peer)
{
  BitStream bs;
  bs.Write<std::uint8_t>(E_CLIENT_TO_SERVER_JOIN);
  ENetPacket *packet = enet_packet_create(bs.GetData(), bs.GetSizeBytes(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  BitStream bs;
  bs.Write<std::uint8_t>(E_SERVER_TO_CLIENT_NEW_ENTITY);
  bs.Write<Entity>(ent);

  ENetPacket *packet = enet_packet_create(bs.GetData(), bs.GetSizeBytes(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  BitStream bs;
  bs.Write<std::uint8_t>(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
  bs.Write<std::uint16_t>(eid);

  ENetPacket *packet = enet_packet_create(bs.GetData(), bs.GetSizeBytes(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y)
{
  BitStream bs;

  bs.Write<std::uint8_t>(E_CLIENT_TO_SERVER_STATE);
  bs.Write<std::uint16_t>(eid);
  bs.Write<float>(x);
  bs.Write<float>(y);

  ENetPacket *packet = enet_packet_create(bs.GetData(), bs.GetSizeBytes(), ENET_PACKET_FLAG_UNSEQUENCED);

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y)
{
  BitStream bs;
  bs.Write<std::uint8_t>(E_SERVER_TO_CLIENT_SNAPSHOT);
  bs.Write<std::uint16_t>(eid);
  bs.Write<float>(x);
  bs.Write<float>(y);

  ENetPacket *packet = enet_packet_create(bs.GetData(), bs.GetSizeBytes(), ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  BitStream bs(packet->data, packet->dataLength);
  std::uint8_t type;
  bs.Read<Entity>(ent);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
}

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  x = *(float*)(ptr); ptr += sizeof(float);
  y = *(float*)(ptr); ptr += sizeof(float);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  x = *(float*)(ptr); ptr += sizeof(float);
  y = *(float*)(ptr); ptr += sizeof(float);
}

