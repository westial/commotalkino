//
// Created by jaume on 10/8/21.
//

#ifndef COMMOTALKINO_LIB_EBYTE_EBYTEDRIVER_H_
#define COMMOTALKINO_LIB_EBYTE_EBYTEDRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PinMap {
  int m0;
  int m1;
  int aux;
} PinMap;

typedef struct Driver {
  char address[2];
  char channel;
  void* send;
  PinMap pins;
} Driver;

Driver Driver_Create(PinMap* pins, const char* address, const char* channel);

#ifdef __cplusplus
}
#endif

#endif //COMMOTALKINO_LIB_EBYTE_EBYTEDRIVER_H_
