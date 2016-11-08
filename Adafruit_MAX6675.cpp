/***************************************************
  This is a library for the Adafruit Thermocouple Sensor w/MAX31855K
  Designed specifically to work with the Adafruit Thermocouple Sensor
  ----> https://www.adafruit.com/products/269
  These displays use SPI to communicate, 3 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!
  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include "math.h"
#include "Adafruit_MAX6675.h"



Adafruit_MAX6675::Adafruit_MAX6675(int8_t sclk_pin, int8_t cs_pin, int8_t miso_pin) {
  sclk = sclk_pin;
  cs = cs_pin;
  miso = miso_pin;

  //define pin modes
  pinMode(cs, OUTPUT);
  pinMode(sclk, OUTPUT);
  pinMode(miso, INPUT);

  digitalWrite(cs, HIGH);
}




double Adafruit_MAX6675::readCelsius(void) {
  uint16_t v;

  digitalWrite(cs, LOW);
  delay(1);


  v = spiread();
  v <<= 8;
  v |= spiread();

  digitalWrite(cs, HIGH);

  if (v & 0x4) {
    // uh oh, no thermocouple attached!
    return NAN;
    //return -100;
  }

  v >>= 3;

  return v*0.25;
}


double Adafruit_MAX6675::readFahrenheit(void) {
return readCelsius() * 9.0/5.0 + 32;
}

byte Adafruit_MAX6675::spiread(void) {
  int i;
  byte d = 0;

  for (i=7; i>=0; i--)
  {
    digitalWrite(sclk, LOW);
    delay(1);
    if (digitalRead(miso)) {
      //set the bit to 0 no matter what
      d |= (1 << i);
    }

    digitalWrite(sclk, HIGH);
    delay(1);
  }

  return d;
}
