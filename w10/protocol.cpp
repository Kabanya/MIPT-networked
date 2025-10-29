#include "protocol.h"
#include "quantisation.h"
#include <cstring> // memcpy
#include <stdlib.h>

static uint32_t xorCipherKey = 0;

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_CLIENT_TO_SERVER_JOIN;

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_NEW_ENTITY; ptr += sizeof(uint8_t);
  memcpy(ptr, &ent, sizeof(Entity)); ptr += sizeof(Entity);

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY; ptr += sizeof(uint8_t);
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);

  enet_peer_send(peer, 0, packet);
}

void send_cipher_key(ENetPeer *peer, uint32_t key)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint32_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_KEY; ptr += sizeof(uint8_t);
  memcpy(ptr, &key, sizeof(uint32_t)); ptr += sizeof(uint32_t);

  enet_peer_send(peer, 0, packet);
}

void fuzz_packet_data(ENetPacket *packet)
{
  packet->data[rand() % packet->dataLength] = (uint8_t)rand();
}

void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float ori)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   sizeof(float) * 2,
                                                   //sizeof(uint8_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  uint8_t *ptr = packet->data;
  *ptr = E_CLIENT_TO_SERVER_INPUT; ptr += sizeof(uint8_t);
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);
  memcpy(ptr, &thr, sizeof(float)); ptr += sizeof(float);
  memcpy(ptr, &ori, sizeof(float)); ptr += sizeof(float);
  /*
  float4bitsQuantized thrPacked(thr, -1.f, 1.f);
  float4bitsQuantized oriPacked(ori, -1.f, 1.f);
  uint8_t thrSteerPacked = (thrPacked.packedVal << 4) | oriPacked.packedVal;
  memcpy(ptr, &thrSteerPacked, sizeof(uint8_t)); ptr += sizeof(uint8_t);
  */
  /*
  memcpy(ptr, &thrPacked, sizeof(uint8_t)); ptr += sizeof(uint8_t);
  memcpy(ptr, &oriPacked, sizeof(uint8_t)); ptr += sizeof(uint8_t);
  */

  fuzz_packet_data(packet);
  cipher_data(packet);

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float ori)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   sizeof(uint16_t) +
                                                   sizeof(uint16_t) +
                                                   sizeof(uint8_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_SNAPSHOT; ptr += sizeof(uint8_t);
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);
  uint16_t xPacked = pack_float<uint16_t>(x, -16.f, 16.f, 11);
  uint16_t yPacked = pack_float<uint16_t>(y, -8.f, 8.f, 10);
  uint8_t oriPacked = pack_float<uint8_t>(ori, -PI, PI, 8);
  //printf("xPacked/unpacked %d %f\n", xPacked, x);
  memcpy(ptr, &xPacked, sizeof(uint16_t)); ptr += sizeof(uint16_t);
  memcpy(ptr, &yPacked, sizeof(uint16_t)); ptr += sizeof(uint16_t);
  memcpy(ptr, &oriPacked, sizeof(uint8_t)); ptr += sizeof(uint8_t);

  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  ent = *(Entity*)(ptr); ptr += sizeof(Entity);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
}

void xor_packet_data(ENetPacket *packet, uint8_t *key_ptr)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  uint8_t *end = packet->data + packet->dataLength;
  for (int i = 0; ptr < end; ++ptr, ++i)
  {
    i = i % 4;
    *ptr ^= key_ptr[i];
  }
}

void cipher_data(ENetPacket *packet)
{
  xor_packet_data(packet, (uint8_t*)&xorCipherKey);
}

void decipher_data(ENetPacket *packet, ENetPeer *peer)
{
  xor_packet_data(packet, (uint8_t*)peer->data);
}

void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, float &thr, float &steer)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);

  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  thr = *(float*)(ptr); ptr += sizeof(float);
  steer = *(float*)(ptr); ptr += sizeof(float);
  //uint8_t thrSteerPacked = *(uint8_t*)(ptr); ptr += sizeof(uint8_t);
  /*
  uint8_t thrPacked = *(uint8_t*)(ptr); ptr += sizeof(uint8_t);
  uint8_t oriPacked = *(uint8_t*)(ptr); ptr += sizeof(uint8_t);
  */
  /*
  static uint8_t neutralPackedValue = pack_float<uint8_t>(0.f, -1.f, 1.f, 4);
  float4bitsQuantized thrPacked(thrSteerPacked >> 4);
  float4bitsQuantized steerPacked(thrSteerPacked & 0x0f);
  thr = thrPacked.packedVal == neutralPackedValue ? 0.f : thrPacked.unpack(-1.f, 1.f);
  steer = steerPacked.packedVal == neutralPackedValue ? 0.f : steerPacked.unpack(-1.f, 1.f);
  */
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &ori)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  uint16_t xPacked = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  uint16_t yPacked = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  uint8_t oriPacked = *(uint8_t*)(ptr); ptr += sizeof(uint8_t);
  x = unpack_float<uint16_t>(xPacked, -16.f, 16.f, 11);
  y = unpack_float<uint16_t>(yPacked, -8.f, 8.f, 10);
  ori = unpack_float<uint8_t>(oriPacked, -PI, PI, 8);
}

void deserialize_and_set_key(ENetPacket *packet)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  xorCipherKey = *(uint32_t*)(ptr); ptr += sizeof(uint32_t);
}

