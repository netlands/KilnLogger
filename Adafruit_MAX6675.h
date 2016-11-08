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

#include "application.h"

class Adafruit_MAX6675 {
 public:
  Adafruit_MAX6675(int8_t sclk_pin, int8_t cs_pin, int8_t miso_pin);


  double readCelsius(void);
  double readFahrenheit(void);
  // For compatibility with older versions:

 private:
  int8_t sclk, miso, cs;
  uint8_t spiread(void);
};
