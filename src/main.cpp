#include "main.h"

// these are just dummy variables, replace with your own
struct DATA {
  byte ADDH;
  byte ADDL;
  byte CHAN;     // That is the point to set the communication on 916MHz
  unsigned long Count;
  int Bits;
  float Volts;
  float Amps;
};

DATA MyData;

int channel;
EByte transceiver = nullptr;

EByte StartTransceiver(int rx, int tx, int8_t m0, int8_t m1, int8_t aux, int frequency);

EByte StartTransceiver(
    const int rx,
    const int tx,
    const int8_t m0,
    const int8_t m1,
    const int8_t aux,
    const int frequency) {
  SoftwareSerial ESerial(rx, tx);
  EByte Transceiver(&ESerial, m0, m1, aux);
  ESerial.begin(frequency);
  Transceiver.init();
  Transceiver.SetAddressH(ADDRESS_HIGH);
  Transceiver.SetAddressL(ADDRESS_LOW);
  Transceiver.SetChannel(CHANNEL);
  Transceiver.SaveParameters(PERMANENT);
  return Transceiver;
}

void setup() {
  Serial.begin(SERIAL_FREQ);
  transceiver = StartTransceiver(PIN_RX, PIN_TX, PIN_M0, PIN_M1, PIN_AX, SERIAL_FREQ);

  Serial.println(transceiver.GetAirDataRate());
  Serial.println(transceiver.GetChannel());
  transceiver.PrintParameters();
}

void loop() {

  // measure some data and save to the structure
  MyData.ADDH = 1;
  MyData.ADDL = 0;
  MyData.CHAN = 16;
  MyData.Count++;
  MyData.Bits = analogRead(A0);
  MyData.Volts = (float)(MyData.Bits * ( 5.0 / 1024.0 ));

  // i highly suggest you send data using structures and not
  // a parsed data--i've always had a hard time getting reliable data using
  // a parsing method
  transceiver.SendStruct(&MyData, sizeof(MyData));

  // let the use know something was sent
  Serial.print("Sending: "); Serial.println(MyData.Count);
  delay(10000);
}
