#include "test_main.h"
#include <Arduino.h>
#include <unity.h>

#define ADDRESS_HIGH 0x01
#define ADDRESS_LOW 0x02

#define TEST_SUBSCRIBER_ID 0x06
#define TEST_SUBSCRIBER_PORT 0x05


static char spy_pushed_content[MESSAGE_LENGTH];
static void print_chars(const char *, unsigned long);

// -----------------------------------------------------------------------------

SoftwareSerial SSerial(PIN_RX, PIN_TX);
Driver lora_driver;

const unsigned char subscriber_id = TEST_SUBSCRIBER_ID;
const unsigned long receiving_timeout = PULL_TIMEOUT;

// create the transceiver object, passing in the serial and pins
// EByte Transceiver(&SSerial, PIN_M0, PIN_M1, PIN_AUX);

unsigned long WriteToSerial(unsigned char *content, unsigned long size) {
  memcpy(spy_pushed_content, (char *)content + 3, MESSAGE_LENGTH);
//  Serial.print("WriteToSerial           ");
//  print_chars((const char *)content, size);
  return SSerial.write((char *)content, size);
}

unsigned long ReadFromSerial(unsigned char *content, unsigned long size, unsigned long position) {
//  if (SSerial.available()) {
//    Serial.print("SpyReadFromSerial avail ");
//    Serial.println(SSerial.available());
//  }
  while (SSerial.available() > 0) {
    char input = (char)SSerial.read();
    if ('\n' == input) break;
    content[position] = input;
    position++;
  }
  if (0 < position) {
    Serial.print("ReadFromSerial          ");
    print_chars((const char *)content, size);
    delay(20);
    Serial.println(position);
  }
  return position;
}

void ClearSerial() {
  while (SSerial.available() > 0) {
    SSerial.read();
  }
}

static int last_aux = 0;

int DigitalRead(unsigned char pin) {
  int value = digitalRead(pin);
//  if (0 == last_aux && 1 == value) Serial.println("DigitalRead aux is high again");
  last_aux = value;
  Serial.print("DigitalRead             ");
  Serial.print(pin);
  Serial.print(" = ");
  Serial.println(value);
  return value;
}

void DigitalWrite(unsigned char pin, unsigned char value) {
  digitalWrite(pin, value);
  delay(1);
//  Serial.print("DigitalWrite            ");
//  Serial.print(pin);
//  Serial.print(" = ");
//  Serial.println(value);
}

unsigned long Millis() {
  const unsigned long log = millis();
//    Serial.print("Millis                  ");
//    Serial.println(log);
  return log;
}

int Listen(const unsigned char *address, unsigned char *content, const unsigned long size) {
  // TODO Reconfigure if the address is different in driver
  return Driver_Receive(&lora_driver, content, size);
}

void TurnOn() {
  Driver_TurnOn(&lora_driver);
}

void TurnOff() {
  Driver_TurnOff(&lora_driver);
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

int InitSubscriber() {
  SubscriberBuilder_Create();
  SubscriberBuilder_SetSalt(COMMOTALKIE_SALT);
  SubscriberBuilder_SetListenCallback(Listen);
  SubscriberBuilder_SetTimeService(Millis);
  SubscriberBuilder_SetTimeout(&receiving_timeout);
  SubscriberBuilder_SetReceiverStateCallback(TurnOn, TurnOff);
  SubscriberBuilder_SetId(&subscriber_id);
  const int result = SubscriberBuilder_Build();
  if (!result) {
    Serial.println("Error: Subscriber builder");
    TEST_FAIL();
  }
  SubscriberBuilder_Destroy();
  return result;
}

void InitArduino() {
  Serial.begin(SERIAL_FREQ);
  while (!Serial);
  SSerial.begin(EBYTE_SERIAL_FREQ);
  while (!SSerial);
  pinMode(PIN_AUX, INPUT);
  pinMode(PIN_M0, OUTPUT);
  pinMode(PIN_M1, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

void InitDriver() {
  lora_driver = Create_Driver((const unsigned char*)"\x01\x02\x10", AIR_RATE_2400, 1, 1);
}

Driver Create_Driver(const unsigned char *topic, const unsigned char air_data_rate,
                     const int is_fixed, const int full_power) {
  PinMap pins = {PIN_M0, PIN_M1, PIN_AUX};
  RadioParams params = {
      {topic[DRIVER_ADDRESS_HIGH_INDEX], topic[DRIVER_ADDRESS_LOW_INDEX]},
      topic[2],
      air_data_rate,
      is_fixed,
      full_power};
  Timer timer = Timer_Create((const void *)Millis);
  IOCallback io = {DigitalRead, DigitalWrite, SpyWriteToSerial,
                   SpyReadFromSerial, ClearSerial};
  unsigned long timeouts[] = {MODE_TIMEOUT, SERIAL_TIMEOUT};
  return Driver_Create(pins, &params, &io, &timer, timeouts);
}

unsigned long Transmit(const unsigned  char *address,
                       const unsigned char *content,
                       const unsigned long size) {
  const Destination target = {address[0], address[1], address[2]};
  return Driver_Send(&lora_driver, &target, content, size);
}

void setUp(void) {
  //  Transceiver.ReadParameters();
  //  Transceiver.PrintParameters();
}

void tearDown(void) { Publish_Destroy(); }

void test_publish() {
  const char body[] = "23456789A";
  const unsigned char address[3] = {ADDRESS_HIGH, ADDRESS_LOW, LORA_CHANNEL};
  Publish_Invoke(address, TEST_SUBSCRIBER_PORT, TEST_SUBSCRIBER_ID, body);
  TEST_ASSERT_EQUAL_CHAR_ARRAY(body + 1, spy_pushed_content + 4,
                               MESSAGE_BODY_LENGTH - 1);
  //  Transceiver.ReadParameters();
  //  Transceiver.PrintParameters();
}

void test_pull() {
  unsigned char id = 0;
  unsigned char port = 0;
  char body[MESSAGE_LENGTH + 3];
  memset(body, '\x00', sizeof(body));
  const unsigned char address[3] = {ADDRESS_HIGH, ADDRESS_LOW, LORA_CHANNEL};
  Result result = Pull_Invoke(address, &port, &id, body);
  Serial.print("Result: ");
  switch (result) {
  case Success:
    Serial.println("Success");
    break;
  case Timeout:
    Serial.println("Timeout");
    break;
  case IOError:
    Serial.println("IOError");
    break;
  default:
    Serial.println("Unexpected");
  }
  Serial.print("Message body: ");
  print_chars((const char *)body, sizeof(body));
  TEST_ASSERT_NOT_EQUAL(id, TEST_SUBSCRIBER_ID);
  TEST_ASSERT_NOT_EQUAL(port, TEST_SUBSCRIBER_PORT);
  TEST_ASSERT_NOT_EQUAL(body[0], '\x00');
  TEST_ASSERT_NOT_EQUAL(body[2], '\x00');
  TEST_ASSERT_NOT_EQUAL(body[4], '\x00');
  //  Transceiver.ReadParameters();
  //  Transceiver.PrintParameters();
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
  InitArduino();
  UNITY_BEGIN();
  memset(spy_pushed_content, '\0', MESSAGE_LENGTH);
  Serial.println("Creating driver");
  InitDriver();
  InitPublisher();
  InitSubscriber();
}

void loop() {
  RUN_TEST(test_publish);
  UNITY_END();
  delay(2000);
}
