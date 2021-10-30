#include "main.h"

#define BROADCAST_ADDRESS_HIGH 0xFF
#define BROADCAST_ADDRESS_LOW 0xFF

#define PING_ADDRESS_HIGH 0x00
#define PING_ADDRESS_LOW 0xA1
#define PING_ID 0xAA
#define PONG_ADDRESS_HIGH 0x00
#define PONG_ADDRESS_LOW 0xB1
#define PONG_ID 0xBB

// -----------------------------------------------------------------------------
// Additional Headers

static void set_config(int ping_pin);

static void print_chars(const char *, unsigned long);

static void print_bytes(const unsigned char *body, int size);

static void blink(int pin);

static void print_state(Driver *driver);

// -----------------------------------------------------------------------------
// Global Instances

SoftwareSerial SSerial(PIN_RX, PIN_TX);
Driver lora_driver;

LoraConfig my_config;
LoraConfig her_config;

const unsigned long receiving_timeout = PULL_TIMEOUT;

unsigned long loop_count;
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
  lora_driver =
      Create_Driver((const unsigned char *)"\x01\x02\x10", AIR_RATE_2400, 1, 1);
}

Driver Create_Driver(const unsigned char *topic,
                     const unsigned char air_data_rate, const int is_fixed,
                     const int full_power) {
  PinMap pins = {PIN_M0, PIN_M1, PIN_AUX};
  RadioParams params = {
      {topic[DRIVER_ADDRESS_HIGH_INDEX], topic[DRIVER_ADDRESS_LOW_INDEX]},
      topic[2],
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

unsigned long WriteToSerial(void *content, unsigned long size) {
  //  Serial.print("WriteToSerial: ");
  //  print_chars((char *)content, size);
  unsigned long written = SSerial.write((char *)content, size);
  return written;
}

unsigned long ReadFromSerial(char *content, unsigned long size,
                             unsigned long position) {
  while (SSerial.available() > 0 && position <= size) {
    char input = static_cast<char>(SSerial.read());
    if ('\n' == input)
      break;
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

unsigned long Transmit(const unsigned char *address, const char *content,
                       const unsigned long size) {
  //  Serial.print("Transmit: ");
  //  print_chars(content, size);
  const Destination target = {address[0], address[1], address[2]};
  return Driver_Send(&lora_driver, &target, content, size);
}

int Listen(const unsigned char *address, char *content,
           const unsigned long size) {
  int result = Driver_Receive(&lora_driver, content, size);
  if (0 != result) {
    Serial.print("Listen content: ");
    print_bytes((unsigned char *)content, size);
  }
  return result;
}

void TurnOn() {
  Driver_TurnOn(&lora_driver);
  Serial.print("Turned on: ");
  Serial.println(lora_driver.state == NORMAL);
}

void TurnOff() {
  Driver_TurnOff(&lora_driver);
  Serial.print("Turned off: ");
  Serial.println(lora_driver.state == SLEEP); }

// -----------------------------------------------------------------------------
// Use cases

void Pull(char *body) {
  unsigned char id = 0;
  unsigned char port = 0;
  const unsigned char address[3] = {my_config.address_high,
                                    my_config.address_low, my_config.channel};
  memset(body, 0, sizeof(Ball));
  Serial.print("Pull address: ");
  print_bytes((unsigned char *)address, sizeof(address));
  Result result = Pull_Invoke(address, &port, &id, body);
  Serial.print("Pulled message port: ");
  Serial.println(port, HEX);
  Serial.print("Pulled message id: ");
  Serial.println(id, HEX);
  Serial.print("My id: ");
  Serial.println(my_config.id, HEX);
  Serial.print("Pulled body: ");
  print_bytes((unsigned char *)body, MESSAGE_BODY_LENGTH);
  Serial.print("Result: ");
  switch (result) {
  case Success:
    Serial.println("Success OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
    blink(LISTEN_LED_PIN);
    break;
  case Timeout:
    Serial.println("Timeout");
    break;
  case IOError:
    Serial.println("IOError XX---XXXXXX---XXXXXX---XXXXXX---XXXXXX---XX");
    break;
  default:
    Serial.println("Unexpected");
  }
}

void OneToOne(const char *body) {
  const unsigned char address[3] = {her_config.address_high,
                                    her_config.address_low, her_config.channel};
  Publish(address, body);
}

void Publish(const unsigned char address[3], const char *body) {
  Serial.print("Publish to port: ");
  Serial.println(her_config.port, HEX);
  Serial.print("Publish to id: ");
  Serial.println(her_config.id, HEX);
  Serial.print("Publish to address: ");
  print_bytes(address, 3);
  Serial.print("Publish body: ");
  print_bytes((unsigned char *)body, 9);
  Publish_Invoke(address, her_config.port, her_config.id, body);
}

void Broadcast(const char *body) {
  const unsigned char address[3] = {BROADCAST_ADDRESS_HIGH,
                                    BROADCAST_ADDRESS_LOW, LORA_CHANNEL};
  Publish(address, body);
}

Ball to_ball(const char *body) {
  Ball ball;
  memcpy((void *)&ball, body, sizeof(Ball));
  //  Serial.print("To ball: ");
  //  print_bytes(body, sizeof(Ball));
  return ball;
}

void from_ball(Ball ball, char *body) {
  //  Serial.print("From ball: ");
  //  print_bytes((const char *)&ball, sizeof(Ball));
  memcpy(body, (const void *)&ball, sizeof(Ball));
}

// -----------------------------------------------------------------------------
// Debug

void print_state(Driver *driver) {
  Serial.print("Driver state: ");
  Serial.println(driver->state);
}

void print_bytes(const unsigned char *body, int size) {
  int i;
  for (i = 0; i + 1 < size; i++) {
    Serial.print(body[i], HEX);
    Serial.print("|");
  }
  Serial.print(body[i], HEX);
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

// -----------------------------------------------------------------------------
// Arduino API

void setup() {
  InitArduino();
  InitDriver();
  InitPublisher();
  InitSubscriber();
  set_config(PING_PIN);
  loop_count = 0;
}

void i_receive() {
  char body[MESSAGE_BODY_LENGTH];
  Ball ball;
  Pull(body);
  ball = to_ball(body);
  Serial.print("Got Hit: ");
  Serial.println(ball.hit);
}

void i_publish() {
  char body[MESSAGE_BODY_LENGTH];
  Ball ball;
  ball.hit = loop_count;
  last_hit = ball.hit;
  from_ball(ball, body);
  Broadcast(body);
}

void loop_ping() {
  if (my_config.do_i_ping) {
    Serial.println("I am Ping");
    i_publish();
    loop_count++;
    delay(2000);
  }
}

void loop_pong() {
  if (!my_config.do_i_ping) {
    Serial.println("I am Pong");
    i_receive();
    loop_count++;
  }
}

void loop_ping_pong() {
  if (0 == loop_count && my_config.do_i_ping) {
    Serial.println("I am Ping");
  } else {
    i_receive();
  }
  i_publish();
  loop_count++;
}

void loop() {
  loop_ping_pong();
}
