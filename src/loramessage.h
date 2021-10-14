#ifndef COMMOTALKINO_SRC_LORAMESSAGE_H_
#define COMMOTALKINO_SRC_LORAMESSAGE_H_

#include "../lib/CommoTalkie/messageconfig.h"

struct LoraMessage {
  char address_high;
  char address_low;
  char channel;
  char data[MESSAGE_LENGTH];
};

#endif //COMMOTALKINO_SRC_LORAMESSAGE_H_
