#ifndef COMMOTALKINO_SRC_MAIN_H_
#define COMMOTALKINO_SRC_MAIN_H_

#include "../lib/CommoTalkie/Driver.h"
#include "../lib/CommoTalkie/EByte.h"
#include "../lib/CommoTalkie/PublisherBuilder.h"
#include "../lib/CommoTalkie/messageconfig.h"
#include "../lib/CommoTalkie/Message.h"
#include "../lib/CommoTalkie/Publish.h"
#include "../lib/CommoTalkie/Pull.h"
#include "../lib/CommoTalkie/SubscriberBuilder.h"
#include <Arduino.h>
#include <SoftwareSerial.h>

#define SERIAL_FREQ 9600
#define EBYTE_SERIAL_FREQ 9600
#define PIN_RX 2
#define PIN_TX 3
#define PIN_M0 7
#define PIN_M1 6
#define PIN_AUX 5

#define LISTEN_LED_PIN 11
#define PING_PIN 12

#define COMMOTALKIE_SALT "RtfgY,u-jk3"

#define MODE_TIMEOUT 4000
#define SERIAL_TIMEOUT 5000
#define PULL_TIMEOUT 6000

#define RETRY_INTERVAL 30000

#define LORA_CHANNEL 0x10
#define COMMON_PORT 0xC6

#pragma pack(push)
#pragma pack(2)

typedef struct Ball {
  unsigned long hit;
} Ball;

#pragma pack(pop)

typedef struct LoraConfig {
  unsigned char id;
  unsigned char address_high;
  unsigned char address_low;
  unsigned char port;
  unsigned char channel;
  short do_i_ping;
} LoraConfig;

void InitArduino();
void InitDriver();
int InitPublisher();
int InitSubscriber();
Ball to_ball(const char* body);
void from_ball(Ball ball, char* body);
void Pull(char *body);
void i_receive();
void i_publish();
void OneToOne(const char *body);
void Publish(const unsigned char address[3], const char *body);
void Broadcast(const char *body);
Driver Create_Driver(const unsigned char *, unsigned char, int, int);
extern "C" void ClearSerial();
extern "C" unsigned long WriteToSerial(void *content, unsigned long size);
extern "C" unsigned long ReadFromSerial(char *content, unsigned long size, unsigned long position);
extern "C" int DigitalRead(unsigned char pin);
extern "C" void DigitalWrite(unsigned char pin, unsigned char value);
extern "C" unsigned long Millis();
extern "C" unsigned long Transmit(const unsigned char* address, const char* content, unsigned long size);
extern "C" int Listen(const unsigned char* address, char* content, unsigned long size);
extern "C" void TurnOn();
extern "C" void TurnOff();

#endif //COMMOTALKINO_SRC_MAIN_H_
