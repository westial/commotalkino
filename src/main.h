#ifndef COMMOTALKINO_SRC_MAIN_H_
#define COMMOTALKINO_SRC_MAIN_H_

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "../lib/EByte/EByte.h"

#define SERIAL_FREQ 9600
#define PIN_RX 2
#define PIN_TX 3
#define PIN_M0 7
#define PIN_M1 6
#define PIN_AX 5

#define ADDRESS_HIGH 0x01
#define ADDRESS_LOW 0x02
#define CHANNEL 0x10

EByte StartTransceiver(int rx, int tx, int8_t m0, int8_t m1, int8_t aux, int frequency);
unsigned long Transmit(const char* address, const char* content, unsigned long size);

#endif //COMMOTALKINO_SRC_MAIN_H_
