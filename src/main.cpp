#include "main.h"

#define BROADCAST_ADDRESS_HIGH 0xFF
#define BROADCAST_ADDRESS_LOW 0xFF

#define PING_ADDRESS_HIGH 0x70
#define PING_ADDRESS_LOW 0xA1
#define PING_ID 0xAA
#define PONG_ADDRESS_HIGH 0x90
#define PONG_ADDRESS_LOW 0xB1
#define PONG_ID 0xBB

#define PING_PONG_INTERVAL 2000
#define HIT_START 55

#define DEBUG 0

// -----------------------------------------------------------------------------
// Additional Headers

static void set_config(int ping_pin);

static void print_chars(const char *, unsigned long);

static void print_bytes(const unsigned char *body, unsigned long size);

static void blink(int pin);

static void debug_state(Driver *driver);

static void debug_result(const char *title, Result result);
static void debug_info(const char *title);
static void debug_bytes(const char *title, const unsigned char *value,
                        unsigned long size);

// -----------------------------------------------------------------------------
// Global Instances

SoftwareSerial SSerial(PIN_RX, PIN_TX);
Driver lora_driver;

LoraConfig my_config;
LoraConfig her_config;

const unsigned long receiving_timeout = PULL_TIMEOUT;

unsigned long loop_count;
unsigned long hit;
unsigned long last_hit;

// -----------------------------------------------------------------------------
// Device Identity

void set_config(int ping_pin) {
  LoraConfig ping_config;
  ping_config.id = PING_ID;
  ping_config.port = COMMON_PORT;
  ping_config.address_high = PING_ADDRESS_HIGH;
  ping_config.address_low = PING_ADDRESS_LOW;
  ping_config.channel = LORA_CHANNEL;
  ping_config.do_i_ping = 1;

  LoraConfig pong_config;
  pong_config.id = PONG_ID;
  pong_config.port = COMMON_PORT;
  pong_config.address_high = PONG_ADDRESS_HIGH;
  pong_config.address_low = PONG_ADDRESS_LOW;
  pong_config.channel = LORA_CHANNEL;
  pong_config.do_i_ping = 0;

  if (HIGH == digitalRead(ping_pin)) {
    my_config = ping_config;
    her_config = pong_config;
  } else {
    my_config = pong_config;
    her_config = ping_config;
  }
}

// -----------------------------------------------------------------------------
// Initializers

int InitPublisher() {
  PublisherBuilder_Create();
  PublisherBuilder_SetSalt(COMMOTALKIE_SALT);
  PublisherBuilder_SetSendCallback(Transmit);
  const int result = PublisherBuilder_Build();
  if (!result) {
    Serial.println("Error: Publisher builder");
    delay(RETRY_INTERVAL);
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
  SubscriberBuilder_SetId(&my_config.id);
  const int result = SubscriberBuilder_Build();
  if (!result) {
    Serial.println("Error: Subscriber builder");
    delay(RETRY_INTERVAL);
  }
  SubscriberBuilder_Destroy();
  return result;
}

void InitArduino() {
  Serial.begin(SERIAL_FREQ);
  while (!Serial)
    ;
  SSerial.begin(EBYTE_SERIAL_FREQ);
  while (!SSerial)
    ;
  pinMode(PIN_RX, INPUT);
  pinMode(PIN_TX, OUTPUT);
  pinMode(PIN_AUX, INPUT);
  pinMode(PIN_M0, OUTPUT);
  pinMode(PIN_M1, OUTPUT);
  pinMode(PING_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LISTEN_LED_PIN, OUTPUT);
}

void InitDriver() {
  lora_driver = Create_Driver(my_config.address_high, my_config.address_low,
                              my_config.channel, AIR_RATE_2400, 1, 1);
}

Driver Create_Driver(const unsigned char address_high,
                     const unsigned char address_low,
                     const unsigned char channel,
                     const unsigned char air_data_rate, const int is_fixed,
                     const int full_power) {
  PinMap pins = {PIN_M0, PIN_M1, PIN_AUX};
  RadioParams params = {{address_low, address_high},
                        channel,
                        air_data_rate,
                        is_fixed,
                        full_power};
  Timer timer = Timer_Create((const void *)Millis);
  IOCallback io = {DigitalRead, DigitalWrite, WriteToSerial, ReadFromSerial,
                   ClearSerial};
  unsigned long timeouts[] = {MODE_TIMEOUT, SERIAL_TIMEOUT};
  return Driver_Create(pins, &params, &io, &timer, timeouts);
}

// -----------------------------------------------------------------------------
// Driver Dependencies

unsigned long WriteToSerial(unsigned char *content, unsigned long size) {
  debug_bytes("WriteToSerial", content, size);
  unsigned long written = SSerial.write(content, size);
  return written;
}

unsigned long ReadFromSerial(unsigned char *content, unsigned long size,
                             unsigned long position) {
  while (SSerial.available() > 0 && position <= size) {
    unsigned char input = SSerial.read();
    // Serial.println(input, HEX);
    content[position] = input;
    position++;
  }
  return position;
}

void ClearSerial() {
  while (SSerial.available() > 0) {
    SSerial.read();
  }
}

int DigitalRead(unsigned char pin) {
  int value = digitalRead(pin);
  //  Serial.print("DigitalRead             ");
  //  Serial.print(pin);
  //  Serial.print(" = ");
  //  Serial.println(value);
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
  return log;
}

// -----------------------------------------------------------------------------
// CommoTalkie Dependencies

unsigned long Transmit(const unsigned char *address,
                       const unsigned char *content, const unsigned long size) {
  debug_bytes("Transmit content", content, size);
  const Destination target = {address[0], address[1], address[2]};
  return Driver_Send(&lora_driver, &target, content, size);
}

int Listen(const unsigned char *address, unsigned char *content,
           const unsigned long size) {
  int result = Driver_Receive(&lora_driver, content, size);
  if (0 != result) {
    debug_bytes("Listen content", content, size);
  }
  return result;
}

void TurnOn() {
  Driver_TurnOn(&lora_driver);
  //  Serial.print("Turned on: ");
  //  Serial.println(lora_driver.state == NORMAL);
}

void TurnOff() {
  Driver_TurnOff(&lora_driver);
  //  Serial.print("Turned off: ");
  //  Serial.println(lora_driver.state == SLEEP);
}

// -----------------------------------------------------------------------------
// Use cases

void Pull(unsigned char *body) {
  unsigned char id = 0;
  unsigned char port = 0;
  const unsigned char address[3] = {my_config.address_high,
                                    my_config.address_low, my_config.channel};
  memset(body, 0, sizeof(Ball));
  debug_bytes("Pull address", address, sizeof(address));
  Result result = Pull_Invoke(address, &port, &id, body);
  debug_bytes("Pulled message port", &port, 1);
  debug_bytes("Pulled message id", &id, 1);
  debug_bytes("My id", &my_config.id, 1);
  debug_bytes("Pulled body", body, MESSAGE_BODY_LENGTH);
  debug_result("Result", result);
}

void OneToOne(const unsigned char *body) {
  const unsigned char address[3] = {her_config.address_high,
                                    her_config.address_low, her_config.channel};
  debug_info("Fixed address transmission mode");
  Publish(address, body);
}

void Publish(const unsigned char address[3], const unsigned char *body) {
  debug_bytes("Publish to port", &her_config.port, 1);
  debug_bytes("Publish to id", &her_config.id, 1);
  debug_bytes("Publish to address", address, 3);
  debug_bytes("Publish body", body, MESSAGE_BODY_LENGTH);
  Publish_Invoke(address, her_config.port, her_config.id, body);
}

void Broadcast(const unsigned char *body) {
  const unsigned char address[3] = {BROADCAST_ADDRESS_HIGH,
                                    BROADCAST_ADDRESS_LOW, LORA_CHANNEL};
  debug_info("Broadcast transmission mode");
  Publish(address, body);
}

// -----------------------------------------------------------------------------
// Debug

void debug_state(Driver *driver) {
  Serial.print("Driver state: ");
  switch (driver->state) {
  case NORMAL:
    Serial.println("NORMAL");
    break;
  case SLEEP:
    Serial.println("SLEEP");
    break;
  case ERROR:
    Serial.println("ERROR");
    break;
  case WARNING:
    Serial.println("WARNING");
    break;
  default:
    Serial.println("UNKNOWN");
  }
}

void print_bytes(const unsigned char *body, unsigned long size) {
  unsigned int i;
  Serial.print("|");
  for (i = 0; i < size; i++) {
    Serial.print(body[i], HEX);
    Serial.print("|");
  }
  Serial.println();
}

void print_chars(const char *anArray, unsigned long size) {
  char printable[size + 1];
  memcpy(printable, anArray, size);
  printable[size] = '\0';
  Serial.println(printable);
}

void blink(int pin) {
  digitalWrite(pin, HIGH);
  delay(50);
  digitalWrite(pin, LOW);
}

void debug_bytes(const char *title, const unsigned char *value,
                 const unsigned long size) {
  if (!DEBUG)
    return;
  Serial.print(title);
  Serial.print(": ");
  print_bytes(value, size);
}

void debug_info(const char *title) {
  if (!DEBUG)
    return;
  Serial.println(title);
}

void debug_result(const char *title, Result result) {
  if (!DEBUG) return;
  Serial.print(title);
  Serial.print(": ");
  switch (result) {
  case Success:
    Serial.println("Success -------------------------------------------");
    blink(LISTEN_LED_PIN);
    break;
  case Timeout:
    Serial.println("Timeout xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
    break;
  case IOError:
    loop_count = 0;
    Serial.println("IOError XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    break;
  default:
    Serial.println("UNEXPECTED ******************************************");
  }
}

// -----------------------------------------------------------------------------
// Arduino API

void setup() {
  set_config(PING_PIN);
  InitArduino();
  InitDriver();
  InitPublisher();
  InitSubscriber();
  loop_count = 0;
  last_hit = HIT_START;
  hit = HIT_START;
}

void i_receive() {
  Ball ball;
  Pull((unsigned char *)&ball);
  hit = ball.hit;
}

void i_publish() {
  Ball ball;
  ball.hit = hit;
  OneToOne((unsigned char *)&ball);
}

void ping_pong() {
  if (0 == loop_count && my_config.do_i_ping) {
    Serial.println("I am Ping");
  } else {
    i_receive();
    delay(PING_PONG_INTERVAL);
  }
  i_publish();
  loop_count++;
  hit = loop_count;
  Serial.print("Succeeded loop count: ");
  Serial.println(loop_count);
}

void assert_ping_pong() {
  if (HIT_START == hit && my_config.do_i_ping) {
    Serial.println("I am Ping");
    delay(PING_PONG_INTERVAL);
    ++hit;
    i_publish();
    return;
  } else {
    i_receive();
    Serial.print("Given/Got Hit: ");
    Serial.print(last_hit);
    Serial.print("/");
    Serial.println(hit);
    if (last_hit == hit) {
      Serial.print("Error at Hit: ");
      Serial.println(hit);
      Serial.println("Hit Error X-X-X-X-X--------=hit=--------X-X-X-X-X-X-X");
      hit = HIT_START;
      delay(PING_PONG_INTERVAL);
      assert_ping_pong();
    } else {
      ++hit;
    }
  }
  last_hit = hit;
  i_publish();
}

void loop() { assert_ping_pong(); }
