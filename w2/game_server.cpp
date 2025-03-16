#include <enet/enet.h>
#include <iostream>

int main()
{
    if (enet_initialize() != 0)
    {
      printf("Cannot init ENet");
      return 1;
    }

    

}