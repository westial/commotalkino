#include "test_main.h"
#include "../lib/CommoTalkie/Message.h"
#include "../lib/CommoTalkie/MessageFormatter.h"
#include "../lib/CommoTalkie/MessagePublisher.h"
#include "../lib/CommoTalkie/Publish.h"
#include <Arduino.h>
#include <unity.h>

static char spy_pushed_content[MESSAGE_LENGTH];
static void print_chars(const char *, unsigned long);

// -----------------------------------------------------------------------------

SoftwareSerial SSerial(PIN_RX, PIN_TX);
Driver lora_driver;

extern "C" unsigned long SpyWriteToSerial(void *content, unsigned long size) {
  memcpy(spy_pushed_content, (char *)content + 3, MESSAGE_LENGTH);
  return SSerial.write((char *)content, size);
}

extern "C" int DigitalRead(unsigned char pin) { return digitalRead(pin); }

extern "C" void DigitalWrite(unsigned char pin, unsigned char value) {
  digitalWrite(pin, value);
  delay(1);
}

extern "C" unsigned long Millis() {
  const unsigned long log = millis();
  Serial.print("Milliseconds ");
  Serial.println(log);
  return log;
}

int InitPublisher() {
  PublisherBuilder_Create();
  PublisherBuilder_SetSalt(COMMOTALKIE_SALT);
  PublisherBuilder_SetSendCallback(Transmit);
  const int result = PublisherBuilder_Build();
  if (!result) {
    Serial.println("Error: Publisher builder");
    TEST_FAIL();
  }
  PublisherBuilder_Destroy();
  return result;
}

Driver Create_Driver(const char *topic, const char air_data_rate,
                     const int is_fixed, const int full_power) {
  PinMap pins = {PIN_M0, PIN_M1, PIN_AX};
  RadioParams params = {
      {topic[DRIVER_ADDRESS_HIGH_INDEX], topic[DRIVER_ADDRESS_LOW_INDEX]},
      topic[2],
      air_data_rate,
      is_fixed,
      full_power};
  Timer timer = Timer_Create((const void *)Millis);
  IOCallback io = {DigitalRead, DigitalWrite, SpyWriteToSerial};
  return Driver_Create(pins, &params, &io, timer, 5000);
}

unsigned long Transmit(const char *address, const char *content,
                       const unsigned long size) {
  const Destination target = {address[0], address[1], address[2]};
  return Driver_Send(&lora_driver, &target, content, size);
}

void setUp(void) {
  Serial.begin(SERIAL_FREQ);
  SSerial.begin(SERIAL_FREQ);
  memset(spy_pushed_content, '\0', MESSAGE_LENGTH);
  Serial.println("Creating driver");
  lora_driver = Create_Driver("\x08\x02\x10", AIR_RATE_2400, 1, 1);
  InitPublisher();
}

void tearDown(void) { Publish_Destroy(); }

void test_publish() {
  const char body[] = "23456789A";
  const char address[3] = {ADDRESS_HIGH, ADDRESS_LOW, CHANNEL};
  Publish_Invoke(address, 0x05, 0x06, body);
  TEST_ASSERT_EQUAL_CHAR_ARRAY(body + 1, spy_pushed_content + 4,
                               MESSAGE_BODY_LENGTH - 1);
}

// -----------------------------------------------------------------------------

void print_chars(const char *anArray, unsigned long size) {
  char printable[size + 1];
  memcpy(printable, anArray, size);
  printable[size] = '\0';
  Serial.println(printable);
}

// -----------------------------------------------------------------------------

void setup() {
  Serial.begin(SERIAL_FREQ);
  SSerial.begin(9600);
  UNITY_BEGIN();
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // RUN_TEST(test_push_message);
  RUN_TEST(test_publish);
  UNITY_END();
  delay(5000);
}
