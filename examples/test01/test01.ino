#include "FirmwareFlasher.h"
const int ledPin = 13;


void setup ()
{
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);    // set the LED on
  delay(2000);
  Serial.println("hey!!!!!!!");
  // put your setup code here, to run once:
  FirmwareFlasher.boot_check();             // check if we need to upgrade firmware before running loop()
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
  delay(100);
  digitalWrite(ledPin, LOW);     // set the LED off
  delay(100);

}  // loop()
