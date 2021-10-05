/*
  The MIT License (MIT)
  Copyright (c) 2019 Kris Kasrpzak
  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to
  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
  the Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  On a personal note, if you develop an application or product using this library 
  and make millions of dollars, I'm happy for you!
*/

/* 
  Code by Kris Kasprzak kris.kasprzak@yahoo.com
  This library is intended to be used with EBYTE transceivers, small wireless units for MCU's such as
  Teensy and Arduino. This library lets users program the operating parameters and both send and receive data.
  This company makes several modules with different capabilities, but most #defines here should be compatible with them
  All constants were extracted from several data sheets and listed in binary as that's how the data sheet represented each setting
  Hopefully, any changes or additions to constants can be a matter of copying the data sheet constants directly into these #defines
  Usage of this library consumes around 970 bytes
  Revision		Data		Author			Description
  1.0			3/6/2019	Kasprzak		Initial creation
  2.0			3/2/2020	Kasprzak		Added all functions to build the options bit (FEC, Pullup, and TransmissionMode
  3.0			3/27/2020	Kasprzak		Added more Get functions
  4.0			6/23/2020	Kasprzak		Added private method to clear the buffer to ensure read methods would not be filled with buffered data
  5.0			12/4/2020	Kasprzak		moved Reset to public, added Clear to SetMode to avoid buffer corruption during programming
*/

#include "EByte.h"
#include <Stream.h>

EByte::EByte() {
}

/*
create the transceiver object
*/

EByte::EByte(Stream *s, int8_t m0_pin, int8_t m1_pin, int8_t aux_pin) {

  _s = s;
  _M0 = m0_pin;
  _M1 = m1_pin;
  _AUX = aux_pin;

}

/*
Initialize the unit--basically this reads the modules parameters and stores the parameters
for potential future module programming
*/

bool EByte::init() {

  bool ok = true;

  pinMode(_AUX, INPUT);
  pinMode(_M0, OUTPUT);
  pinMode(_M1, OUTPUT);

  SetMode(MODE_NORMAL);

  // first get the module data (must be called first for some odd reason

  ok = ReadModelData();

  if (!ok) {
    return false;
  }
  // now get parameters to put unit defaults into the class variables

  ok = ReadParameters();
  if (!ok) {
    return false;
  }

  return true;
}

/*
Method to indicate availability
*/

bool EByte::available() {

  return _s->available();

}

/*
Method to indicate availability
*/

void EByte::flush() {

  _s->flush();
}

/*
Method to write a single byte...not sure how useful this really is. If you need to send 
more than one byte, put the data into a data structure and send it in a big chunk
*/

void EByte::SendByte(uint8_t TheByte) {

  _s->write(TheByte);

}

/*
Method to get a single byte...not sure how useful this really is. If you need to get 
more than one byte, put the data into a data structure and send/receive it in a big chunk
*/

uint8_t EByte::GetByte() {

  return _s->read();

}

/*
Method to send a chunk of data provided data is in a struct--my personal favorite as you 
need not parse or worry about sprintf() inability to handle floats
TTP: put your structure definition into a .h file and include in both the sender and receiver
sketches
NOTE: of your sender and receiver MCU's are different (Teensy and Arduino) caution on the data
types each handle ints floats differently
*/

bool EByte::SendStruct(const void *TheStructure, uint16_t size_) {

  _buf = _s->write((uint8_t *) TheStructure, size_);

  CompleteTask(1000);

  return (_buf == size_);

}

/*
Method to get a chunk of data provided data is in a struct--my personal favorite as you 
need not parse or worry about sprintf() inability to handle floats
TTP: put your structure definition into a .h file and include in both the sender and receiver
sketches
NOTE: of your sender and receiver MCU's are different (Teensy and Arduino) caution on the data
types each handle ints floats differently
*/


bool EByte::GetStruct(const void *TheStructure, uint16_t size_) {

  _buf = _s->readBytes((uint8_t *) TheStructure, size_);

  CompleteTask(1000);

  return (_buf == size_);
}

/*
Utility method to wait until module is done transmitting
a timeout is provided to avoid an infinite loop
*/

void EByte::CompleteTask(unsigned long timeout) {

  unsigned long t = millis();

  // make darn sure millis() is not about to reach max data type limit and start over
  if (((unsigned long) (t + timeout)) == 0) {
    t = 0;
  }

  // if AUX pin was supplied and look for HIGH state
  // note you can omit using AUX if no pins are available, but you will have to use delay() to let module finish
  if (_AUX != -1) {

    while (digitalRead(_AUX) == LOW) {

      if ((millis() - t) > timeout) {
        break;
      }
    }
  } else {
    // if you can't use aux pin, use 4K7 pullup with Arduino
    // you may need to adjust this value if transmissions fail
    delay(1000);

  }


  // per data sheet control after aux goes high is 2ms so delay for at least that long
  //SmartDelay(20);
  delay(20);
}

/*
method to set the mode (program, normal, etc.)
*/

void EByte::SetMode(uint8_t mode) {

  // data sheet claims module needs some extra time after mode setting (2ms)
  // most of my projects uses 10 ms, but 40ms is safer

  delay(PIN_RECOVER);

  if (mode == MODE_NORMAL) {

    digitalWrite(_M0, LOW);
    digitalWrite(_M1, LOW);
  } else if (mode == MODE_WAKEUP) {

    digitalWrite(_M0, HIGH);
    digitalWrite(_M1, LOW);

  } else if (mode == MODE_POWERDOWN) {
    digitalWrite(_M0, LOW);
    digitalWrite(_M1, HIGH);
  } else if (mode == MODE_PROGRAM) {
    digitalWrite(_M0, HIGH);
    digitalWrite(_M1, HIGH);
  }

  // data sheet says 2ms later control is returned, let's give just a bit more time
  // these modules can take time to activate pins
  delay(PIN_RECOVER);

  // clear out any junk
  // added rev 5
  // I've had some issues where after programming, the returned model is 0, and all settings appear to be corrupt
  // I imagine the issue is due to the internal buffer full of junk, hence clearing
  // Reset() *MAY* work but this seems better.
  ClearBuffer();

  // wait until aux pin goes back low
  CompleteTask(4000);

}


// i've asked EBYTE what's supposed to happen--got an unclear answer
// but my testing indicates it clears buffer
// I use this when needing to restart the EBYTE after programming while data is still streaming in
// to let the unit start reading from a cleared internal buffer

// it does NOT return the ebyte back to factory defaults
// if your unit gets corrupt or you need to restore values, you will have to do brute force
// example for and E44-915
// look at the data sheet for default values
//  Trans.SetAddressH(0);
//  Trans.SetAddressL(0);
//  Trans.SetSpeed(0b00011100);  
//  Trans.SetChannel(1);
//  Trans.SetOptions(0b01000100);
//  Trans.SaveParameters(PERMANENT);


void EByte::Reset() {

  SetMode(MODE_PROGRAM);

  delay(50); // not sure you need this

  _s->write(0xC4);
  _s->write(0xC4);
  _s->write(0xC4);

  CompleteTask(4000);

  SetMode(MODE_NORMAL);

}

void EByte::SetSpeed(uint8_t val) {
  _Speed = val;
}
void EByte::SetOptions(uint8_t val) {
  _Options = val;
}
uint8_t EByte::GetSpeed() {
  return _Speed;
}
uint8_t EByte::GetOptions() {
  return _Options;
}

/*
method to set the high bit of the address
*/


void EByte::SetAddressH(uint8_t val) {
  _AddressHigh = val;
}

uint8_t EByte::GetAddressH() {
  return _AddressHigh;
}

/*
method to set the low bit of the address
*/

void EByte::SetAddressL(uint8_t val) {
  _AddressLow = val;
}

uint8_t EByte::GetAddressL() {
  return _AddressLow;
}

/*
method to set the channel
*/

void EByte::SetChannel(uint8_t val) {
  _Channel = val;
}
uint8_t EByte::GetChannel() {
  return _Channel;
}

/*
method to set the air data rate
*/

void EByte::SetAirDataRate(uint8_t val) {

  _AirDataRate = val;
  BuildSpeedByte();

}

uint8_t EByte::GetAirDataRate() {
  return _AirDataRate;
}

/*
method to set the parity bit
*/


void EByte::SetParityBit(uint8_t val) {
  _ParityBit = val;
  BuildSpeedByte();
}
uint8_t EByte::GetParityBit() {
  return _ParityBit;
}

/*
method to set the options bits
*/

void EByte::SetTransmissionMode(uint8_t val) {
  _OptionTrans = val;
  BuildOptionByte();
}
uint8_t EByte::GetTransmissionMode() {
  return _OptionTrans;
}

void EByte::SetPullupMode(uint8_t val) {
  _OptionPullup = val;
  BuildOptionByte();
}
uint8_t EByte::GetPullupMode() {
  return _OptionPullup;
}

void EByte::SetWORTIming(uint8_t val) {
  _OptionWakeup = val;
  BuildOptionByte();
}
uint8_t EByte::GetWORTIming() {
  return _OptionWakeup;
}

void EByte::SetFECMode(uint8_t val) {
  _OptionFEC = val;
  BuildOptionByte();
}
uint8_t EByte::GetFECMode() {
  return _OptionFEC;
}

void EByte::SetTransmitPower(uint8_t val) {

  _OptionPower = val;
  BuildOptionByte();

}

uint8_t EByte::GetTransmitPower() {
  return _OptionPower;
}

/*
method to compute the address based on high and low bits
*/

void EByte::SetAddress(uint16_t Val) {
  _AddressHigh = ((Val & 0xFFFF) >> 8);
  _AddressLow = (Val & 0xFF);
}

/*
method to get the address which is a collection of hi and lo bytes
*/


uint16_t EByte::GetAddress() {
  return (_AddressHigh << 8) | (_AddressLow);
}

/*
set the UART baud rate
*/

void EByte::SetUARTBaudRate(uint8_t val) {
  _UARTDataRate = val;
  BuildSpeedByte();
}

uint8_t EByte::GetUARTBaudRate() {
  return _UARTDataRate;
}

/*
method to build the byte for programming (notice it's a collection of a few variables)
*/
void EByte::BuildSpeedByte() {
  _Speed = 0;
  _Speed = ((_ParityBit & 0xFF) << 6) | ((_UARTDataRate & 0xFF) << 3)
      | (_AirDataRate & 0xFF);
}

/*
method to build the option byte for programming (notice it's a collection of a few variables)
*/

void EByte::BuildOptionByte() {
  _Options = 0;
  _Options = ((_OptionTrans & 0xFF) << 7) | ((_OptionPullup & 0xFF) << 6)
      | ((_OptionWakeup & 0xFF) << 3) | ((_OptionFEC & 0xFF) << 2)
      | (_OptionPower & 0b11);
}

bool EByte::GetAux() {
  return digitalRead(_AUX);
}

/*
method to save parameters to the module
*/

void EByte::SaveParameters(uint8_t val) {

  SetMode(MODE_PROGRAM);

  // ClearBuffer();

  // here you can save permanenly or temp
  delay(5);

  /*
  Serial.print("val: ");
  Serial.println(val);

  Serial.print("_AddressHigh: ");
  Serial.println(_AddressHigh);

  Serial.print("_AddressLow: ");
  Serial.println(_AddressLow);

  Serial.print("_Speed: ");
  Serial.println(_Speed);

  Serial.print("_Channel: ");
  Serial.println(_Channel);

  Serial.print("_Options: ");
  Serial.println(_Options);
  */


  _s->write(val);
  _s->write(_AddressHigh);
  _s->write(_AddressLow);
  _s->write(_Speed);
  _s->write(_Channel);
  _s->write(_Options);

  delay(50);

  CompleteTask(4000);

  SetMode(MODE_NORMAL);

}

/*
method to print parameters, this can be called anytime after init(), because init gets parameters
and any method updates the variables
*/

void EByte::PrintParameters() {

  _ParityBit = (_Speed & 0XC0) >> 6;
  _UARTDataRate = (_Speed & 0X38) >> 3;
  _AirDataRate = _Speed & 0X07;

  _OptionTrans = (_Options & 0X80) >> 7;
  _OptionPullup = (_Options & 0X40) >> 6;
  _OptionWakeup = (_Options & 0X38) >> 3;
  _OptionFEC = (_Options & 0X07) >> 2;
  _OptionPower = (_Options & 0X03);

  Serial.println("----------------------------------------");
  Serial.print(F("Model no.: "));
  Serial.println(_Model, HEX);
  Serial.print(F("Version  : "));
  Serial.println(_Version, HEX);
  Serial.print(F("Features : "));
  Serial.println(_Features, HEX);
  Serial.println(F(" "));
  Serial.print(F("Mode (HEX/DEC/BIN): "));
  Serial.print(_Save, HEX);
  Serial.print(F("/"));
  Serial.print(_Save, DEC);
  Serial.print(F("/"));
  Serial.println(_Save, BIN);
  Serial.print(F("AddH (HEX/DEC/BIN): "));
  Serial.print(_AddressHigh, HEX);
  Serial.print(F("/"));
  Serial.print(_AddressHigh, DEC);
  Serial.print(F("/"));
  Serial.println(_AddressHigh, BIN);
  Serial.print(F("AddL (HEX/DEC/BIN): "));
  Serial.print(_AddressLow, HEX);
  Serial.print(F("/"));
  Serial.print(_AddressLow, DEC);
  Serial.print(F("/"));
  Serial.println(_AddressLow, BIN);
  Serial.print(F("Sped (HEX/DEC/BIN): "));
  Serial.print(_Speed, HEX);
  Serial.print(F("/"));
  Serial.print(_Speed, DEC);
  Serial.print(F("/"));
  Serial.println(_Speed, BIN);
  Serial.print(F("Chan (HEX/DEC/BIN): "));
  Serial.print(_Channel, HEX);
  Serial.print(F("/"));
  Serial.print(_Channel, DEC);
  Serial.print(F("/"));
  Serial.println(_Channel, BIN);
  Serial.print(F("Optn (HEX/DEC/BIN): "));
  Serial.print(_Options, HEX);
  Serial.print(F("/"));
  Serial.print(_Options, DEC);
  Serial.print(F("/"));
  Serial.println(_Options, BIN);
  Serial.print(F("Addr (HEX/DEC/BIN): "));
  Serial.print(GetAddress(), HEX);
  Serial.print(F("/"));
  Serial.print(GetAddress(), DEC);
  Serial.print(F("/"));
  Serial.println(GetAddress(), BIN);
  Serial.println(F(" "));
  Serial.print(F("SpeedParityBit (HEX/DEC/BIN)    : "));
  Serial.print(_ParityBit, HEX);
  Serial.print(F("/"));
  Serial.print(_ParityBit, DEC);
  Serial.print(F("/"));
  Serial.println(_ParityBit, BIN);
  Serial.print(F("SpeedUARTDataRate (HEX/DEC/BIN) : "));
  Serial.print(_UARTDataRate, HEX);
  Serial.print(F("/"));
  Serial.print(_UARTDataRate, DEC);
  Serial.print(F("/"));
  Serial.println(_UARTDataRate, BIN);
  Serial.print(F("SpeedAirDataRate (HEX/DEC/BIN)  : "));
  Serial.print(_AirDataRate, HEX);
  Serial.print(F("/"));
  Serial.print(_AirDataRate, DEC);
  Serial.print(F("/"));
  Serial.println(_AirDataRate, BIN);
  Serial.print(F("OptionTrans (HEX/DEC/BIN)       : "));
  Serial.print(_OptionTrans, HEX);
  Serial.print(F("/"));
  Serial.print(_OptionTrans, DEC);
  Serial.print(F("/"));
  Serial.println(_OptionTrans, BIN);
  Serial.print(F("OptionPullup (HEX/DEC/BIN)      : "));
  Serial.print(_OptionPullup, HEX);
  Serial.print(F("/"));
  Serial.print(_OptionPullup, DEC);
  Serial.print(F("/"));
  Serial.println(_OptionPullup, BIN);
  Serial.print(F("OptionWakeup (HEX/DEC/BIN)      : "));
  Serial.print(_OptionWakeup, HEX);
  Serial.print(F("/"));
  Serial.print(_OptionWakeup, DEC);
  Serial.print(F("/"));
  Serial.println(_OptionWakeup, BIN);
  Serial.print(F("OptionFEC (HEX/DEC/BIN)         : "));
  Serial.print(_OptionFEC, HEX);
  Serial.print(F("/"));
  Serial.print(_OptionFEC, DEC);
  Serial.print(F("/"));
  Serial.println(_OptionFEC, BIN);
  Serial.print(F("OptionPower (HEX/DEC/BIN)       : "));
  Serial.print(_OptionPower, HEX);
  Serial.print(F("/"));
  Serial.print(_OptionPower, DEC);
  Serial.print(F("/"));
  Serial.println(_OptionPower, BIN);

  Serial.println("----------------------------------------");

}

/*
method to read parameters, 
*/

bool EByte::ReadParameters() {

  _Params[0] = 0;
  _Params[1] = 0;
  _Params[2] = 0;
  _Params[3] = 0;
  _Params[4] = 0;
  _Params[5] = 0;

  SetMode(MODE_PROGRAM);

  _s->write(0xC1);

  _s->write(0xC1);

  _s->write(0xC1);

  _s->readBytes((uint8_t *) &_Params, (uint8_t) sizeof(_Params));

  _Save = _Params[0];
  _AddressHigh = _Params[1];
  _AddressLow = _Params[2];
  _Speed = _Params[3];
  _Channel = _Params[4];
  _Options = _Params[5];

  _Address = (_AddressHigh << 8) | (_AddressLow);
  _ParityBit = (_Speed & 0XC0) >> 6;
  _UARTDataRate = (_Speed & 0X38) >> 3;
  _AirDataRate = _Speed & 0X07;

  _OptionTrans = (_Options & 0X80) >> 7;
  _OptionPullup = (_Options & 0X40) >> 6;
  _OptionWakeup = (_Options & 0X38) >> 3;
  _OptionFEC = (_Options & 0X07) >> 2;
  _OptionPower = (_Options & 0X03);

  SetMode(MODE_NORMAL);

  if (0xC0 != _Params[0]) {

    return false;
  }

  return true;

}

bool EByte::ReadModelData() {

  _Params[0] = 0;
  _Params[1] = 0;
  _Params[2] = 0;
  _Params[3] = 0;
  _Params[4] = 0;
  _Params[5] = 0;

  bool found = false;
  int i = 0;

  SetMode(MODE_PROGRAM);

  _s->write(0xC3);

  _s->write(0xC3);

  _s->write(0xC3);

  _s->readBytes((uint8_t *) &_Params, (uint8_t) sizeof(_Params));

  _Save = _Params[0];
  _Model = _Params[1];
  _Version = _Params[2];
  _Features = _Params[3];

  if (0xC3 != _Params[0]) {

    for (i = 0; i < 5; i++) {
      Serial.print("trying: ");
      Serial.println(i);
      _Params[0] = 0;
      _Params[1] = 0;
      _Params[2] = 0;
      _Params[3] = 0;
      _Params[4] = 0;
      _Params[5] = 0;
      delay(100);

      _s->write(0xC3);

      _s->write(0xC3);

      _s->write(0xC3);

      _s->readBytes((uint8_t *) &_Params, (uint8_t) sizeof(_Params));

      if (0xC3 == _Params[0]) {
        found = true;
        break;
      }

    }
  } else {
    found = true;
  }

  SetMode(MODE_NORMAL);

  CompleteTask(1000);

  return found;

}

/*
method to get module model and E50-TTL-100 will return 50
*/

uint8_t EByte::GetModel() {

  return _Model;

}

/*
method to get module version (undocumented as to the value)
*/

uint8_t EByte::GetVersion() {

  return _Version;

}

/*
method to get module version (undocumented as to the value)
*/


uint8_t EByte::GetFeatures() {

  return _Features;

}

/*
method to clear the serial buffer

without clearing the buffer, i find getting the parameters very unreliable after programming.
i suspect stuff in the buffer affects rogramming 
hence, let's clean it out
this is called as part of the setmode

*/
void EByte::ClearBuffer() {

  unsigned long amt = millis();

  while (_s->available()) {
    _s->read();
    if ((millis() - amt) > 5000) {
      Serial.println("runaway");
      break;
    }
  }

}
