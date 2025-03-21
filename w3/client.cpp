#include <enet/enet.h>
#include <iostream>
#include <string>
#include <vector>

void send_array_packet(ENetPeer *peer, const std::vector<uint32_t> &numbers)
{
  ENetPacket *packet = enet_packet_create(nullptr, numbers.size() * sizeof(uint32_t) + 2 * sizeof(uint8_t), ENET_PACKET_FLAG_UNSEQUENCED);
  *((uint8_t*)packet->data) = (uint8_t)numbers.size();
  memcpy(packet->data+1, numbers.data(), numbers.size() * sizeof(uint32_t));

  enet_peer_send(peer, 1, packet);
}

void send_int_packet(ENetPeer *peer, int num)
{
  std::string str = std::string("packet#") + std::to_string(num);
  ENetPacket *packet = enet_packet_create(str.c_str(), str.size() + 1, ENET_PACKET_FLAG_RELIABLE);

  enet_peer_send(peer, 1, packet);
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
  address.port = 53473;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

  uint32_t timeStart = enet_time_get();
  uint32_t lastMicroSendTime = timeStart;
  bool connected = false;
  while (true)
  {
    ENetEvent event;
    while (enet_host_service(client, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s'\n", event.packet->data);
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    if (connected)
    {
      uint32_t curTime = enet_time_get();
      if (curTime - lastMicroSendTime > 100)
      {
        lastMicroSendTime = curTime;
        static int counter = 0;
        send_int_packet(serverPeer, counter++);
      }
    }
  }
  return 0;
}
