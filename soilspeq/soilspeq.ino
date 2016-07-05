
// Firmware for SoilSpeQ Beta 2 hardware.   Part of the PhotosynQ project.

// setup() - initize things on startup
// main loop is in loop.cpp


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
  // set up serial ports (Serial and Serial1)
  Serial_Set(4);                // auto switch between USB and BLE
  Serial_Begin(115200);

  // Set up I2C bus - CAUTION: any subsequent calls to Wire.begin() will mess this up
  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_INT, I2C_RATE_800);  // using alternative wire library

  bme1.begin(0x77);         // pressure/humidity/temp sensors

  eeprom_initialize();      // eeprom
  assert(sizeof(eeprom_class) < 2048);      // check that we haven't exceeded eeprom space

  // set up MCU pins
  setup_pins();

  analogReference(EXTERNAL);   // 3.3V
  analogReadResolution(16);
  analogReadAveraging(4);

  setTime(Teensy3Clock.get());              // set time from RTC

  Serial_Print(DEVICE_NAME);                // note: this may not display because Serial isn't ready
  Serial_Print_Line(" Ready");

}  // setup() - now execute loop()


void setup_pins()
{

}


void unset_pins()     // save power, set pins to high impedance
{
  // turn off almost every pin
  for (unsigned i = 0; i < 33; ++i)
    if (i != 18 && i != 19 && i != WAKE_DC && i != WAKE_3V3)  // leave I2C and power control on
      pinMode(i, INPUT);
}


