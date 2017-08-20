#include "FirmwareFlasher.h"
const int ledPin = 13;

int d = 100; //uplode using the teensy loader then change it and uplode using processing


void setup ()
{
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);    // set the LED on
  delay(2000);
  Serial.println("hey!!!!!!!");
  Serial.println(FLASH_ID);          // TODO: find a way to force the compiler to not optimize FLASH_ID out
  // put your setup code here, to run once:
  // FirmwareFlasher.boot_check();             // dont check it will start an infinet loop of flashing
  digitalWrite(ledPin, LOW);     // set the LED off
}

void loop ()
{
  //simple example of writing to flash
  /*
  int ret;
  uint32_t addr = FLASH_SIZE / 2;
  ret = flash_word (addr, 0x123);
  Serial.printf ("%x:%x  %d\n", addr, *(volatile unsigned int *) addr, ret);
  ret = flash_word (addr, 0x456);  // this will fail
  Serial.printf ("%x:%x  %d\n", addr, *(volatile unsigned int *) addr, ret);
  flash_erase_sector (addr, 0);
  ret = flash_word (addr, 0x456);
  Serial.printf ("%x:%x  %d\n", addr, *(volatile unsigned int *) addr, ret);
  */

  // upgrade_firmware();

  // for (;;) {}

  digitalWrite(ledPin, HIGH);    // set the LED on
  delay(d);
  digitalWrite(ledPin, LOW);     // set the LED off
  delay(d);

}  // loop()
