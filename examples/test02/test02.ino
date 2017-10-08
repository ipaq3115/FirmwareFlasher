// WARNING:  you can destroy your MCU with flash erase or write!
// This code is not ready for use.
#include <SPI.h>
#include <Ethernet.h>
#include "FirmwareFlasher.h"

#include <TeensyMAC.h>
#include <EEPROM.h>

static const uint32_t __BUILD__ 0x1

#define PIN_RESET 9



void setup ()
{
  FirmwareFlasher.boot_check();             // check if we need to upgrade firmware before running loop()
}

void loop ()
{
}
