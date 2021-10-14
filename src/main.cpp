#include "main.h"

#include "../lib/CommoTalkie/Publish.h"

SoftwareSerial SSerial(PIN_RX, PIN_TX);

Driver lora_driver;

unsigned long Write_To_Serial(void *content, unsigned long size) {
    return SSerial.write((char*)content, size);
}

Driver Create_Driver(const char *topic, const char air_data_rate,
                     const int is_fixed, const int full_power) {
  PinMap pins = {PIN_M0, PIN_M1, PIN_AX};
  RadioParams params = {{topic[DRIVER_ADDRESS_HIGH_INDEX],      topic[DRIVER_ADDRESS_LOW_INDEX]}, topic[2],
                        air_data_rate, is_fixed, full_power};
  Timer timer = Timer_Create((const void *)millis);
  IOCallback io = {digitalRead, digitalWrite, Write_To_Serial};
  return Driver_Create(pins, &params, &io, timer, 5000);
}

void Stub_Publish() {
  const char address[3] = {ADDRESS_HIGH, ADDRESS_LOW, CHANNEL};
  Publish_Invoke(address, 0x05, 0x06, "23456789A");
}

unsigned long Transmit(
    const char* address,
    const char* content,
    const unsigned long size) {
  char destination[2];
  destination[DRIVER_ADDRESS_HIGH_INDEX] = address[0];
  destination[DRIVER_ADDRESS_LOW_INDEX] = address[1];
  char channel = address[2];
  return Driver_Send(&lora_driver, destination, &channel, content, size);
}

int InitPublisher() {
  PublisherBuilder_Create();
  PublisherBuilder_SetSalt(COMMOTALKIE_SALT);
  PublisherBuilder_SetSendCallback(Transmit);
  const int result = PublisherBuilder_Build();
  if (!result) {
    Serial.println("Error: Publisher builder");
  }
  PublisherBuilder_Destroy();
  return result;
}

void setup() {
  Serial.begin(SERIAL_FREQ);
  SSerial.begin(9600);
  Serial.println("Initializing EByte Driver");
  //lora_driver = Create_Driver("\x08\x02\x10", AIR_RATE_2400, 1, 1);
  Serial.println("Initializing Publisher");
  //InitPublisher();
}

void loop() {
  const char address[3] = {ADDRESS_HIGH, ADDRESS_LOW, CHANNEL};
  const char body[] = "012345678";
  //Transmit(address, body, 10);
  //Stub_Publish();
  // let the use know something was sent
  Serial.print("Sending: "); Serial.println(millis());
  delay(10000);
}
