#include "EByteDriver.h"

Driver Driver_Create(PinMap *pins, const unsigned char *address, const unsigned char *channel) {
  Driver result;
  result.address[0] = address[0];
  result.address[1] = address[1];
  result.channel = *channel;
  result.pins = *pins;
  return result;
}
