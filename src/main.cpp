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

EByte StartTransceiver(int rx, int tx, int m0, int m1, int aux, int frequency);

EByte StartTransceiver(
    const int rx,
    const int tx,
    const int m0,
    const int m1,
    const int aux,
    const int frequency) {
  SoftwareSerial ESerial(rx, tx);
  EByte Transceiver(&ESerial, m0, m1, aux);
  ESerial.begin(frequency);
  Transceiver.init();
  return Transceiver;
}

void setup() {
  Serial.begin(9600);
  transceiver = StartTransceiver(PIN_RX, PIN_TX, PIN_M0, PIN_M1, PIN_AX, 9600);

  Serial.println(transceiver.GetAirDataRate());
  Serial.println(transceiver.GetChannel());

  transceiver.SetAddressH(0x01);
  transceiver.SetAddressL(0x00);
  channel = 16;
  transceiver.SetChannel(channel);
  // save the parameters to the unit,
  transceiver.SaveParameters(PERMANENT);

  // you can print all parameters and is good for debugging
  // if your units will not communicate, print the parameters
  // for both sender and receiver and make sure air rates, channel
  // and address is the same
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
  delay(1000);
}
