#include "protocol.h"
#include "bitstream.h"
#include <cstring>
#include <unordered_map>

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
  bs.Write<uint8_t>(E_SERVER_TO_CLIENT_NEW_ENTITY);
  
  bs.Write<uint32_t>(ent.color);
  bs.Write<float>(ent.x);
  bs.Write<float>(ent.y);
  bs.Write<uint16_t>(ent.eid);
  bs.Write<bool>(ent.serverControlled);
  bs.Write<float>(ent.targetX);
  bs.Write<float>(ent.targetY);

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

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y)
{
  BitStream bs;
  bs.Write<uint8_t>(E_CLIENT_TO_SERVER_STATE);
  bs.Write<uint16_t>(eid);
  
  bs.Write<float>(x);
  bs.Write<float>(y);

  ENetPacket *packet = enet_packet_create(bs.GetData(), bs.GetSizeBytes(), ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y)
{
  BitStream bs;
  bs.Write<uint8_t>(E_SERVER_TO_CLIENT_SNAPSHOT);
  bs.Write<uint16_t>(eid);
  
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
  uint8_t type;
  bs.Read<uint8_t>(type); 
  
  bs.Read<uint32_t>(ent.color);
  bs.Read<float>(ent.x);
  bs.Read<float>(ent.y);
  bs.Read<uint16_t>(ent.eid);
  bs.Read<bool>(ent.serverControlled);
  bs.Read<float>(ent.targetX);
  bs.Read<float>(ent.targetY);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  BitStream bs(packet->data, packet->dataLength);
  uint8_t type;
  bs.Read<uint8_t>(type);
  bs.Read<uint16_t>(eid);
}

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y)
{
  BitStream bs(packet->data, packet->dataLength);
  uint8_t type;
  bs.Read<uint8_t>(type);
  bs.Read<uint16_t>(eid);
  bs.Read<float>(x);
  bs.Read<float>(y);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y)
{
  BitStream bs(packet->data, packet->dataLength);
  uint8_t type;
  bs.Read<uint8_t>(type);
  bs.Read<uint16_t>(eid);
  bs.Read<float>(x);
  bs.Read<float>(y);
}