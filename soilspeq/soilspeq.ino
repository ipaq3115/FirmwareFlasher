
// Firmware for SoilSpeQ Beta 2 hardware.   Part of the PhotosynQ project.

// setup() - initize things on startup
// main loop is in loop.cpp
// commented out all dac_set, dac_change, functions in loop.cpp, as well as the detector calls (AD7689).  Set data_raw equal to zero for all

// includes
#include "defines.h"
#include <i2c_t3.h>
#include <Time.h>                   // enable real time clock library
#define EXTERN
#include "eeprom.h"
#include "serial.h"
#include <SPI.h>                    // include the new SPI library
#include "util.h"
#include <TimeLib.h>

void setup_pins(void);            // initialize pins

// This routine is called first

void setup()
{
  delay(4000);

  // set up serial ports (Serial and Serial1)
  Serial_Set(4);                // auto switch between USB and BLE
  Serial_Begin(115200);

  // Set up I2C bus - CAUTION: any subsequent calls to Wire.begin() will mess this up
  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_INT, I2C_RATE_800);  // using alternative wire library

  // pressure/humidity/temp sensors
  bme1.begin(0x77);

  // initialize eeprom space + make sure we haven't exceeded the maximum
  eeprom_initialize();      
  assert(sizeof(eeprom_class) < 2048);

  // set up MCU pins
  setup_pins();

  analogReference(EXTERNAL);   // 3.3V
  analogReadResolution(16);
  analogReadAveraging(4);
  setTime(Teensy3Clock.get());              // set time from RTC
  Serial_Print(DEVICE_NAME);                // note: this may not display because Serial isn't ready
  Serial_Print_Line(" Ready");

}  // setup() - now execute loop()

void setup_pins() {}

