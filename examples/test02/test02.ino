// WARNING:  you can destroy your MCU with flash erase or write!
// This code is not ready for use.
#include <SPI.h>
#include <Ethernet.h>
#include "FirmwareFlasher.h"

#include <TeensyMAC.h>
#include <EEPROM.h>

//static const uint32_t __BUILD__ 0x1

#define PIN_RESET 9

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192,168,2,177);
uint16_t udpPort = 8050;

EthernetUDP udp;
uint8_t udp_buffer[600];

bool firmware_update_in_progress = false;

char line[200];
int count = 0;

void setup ()
{
  pinMode(13, OUTPUT);
  digitalWrite(13,HIGH);
  Serial.begin(115200);
  delay(2000);
  #ifdef PIN_RESET
    pinMode(PIN_RESET, OUTPUT);
    digitalWrite(PIN_RESET, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_RESET, HIGH);
    delay(150);
  #endif


  Ethernet.begin(mac, ip);
  udp.begin(udpPort);
  Serial.println(FLASH_ID);

  // FirmwareFlasher.boot_check();             // check if we need to upgrade firmware before running loop()
}

char firmwareHeader[] = "firmware_update";

void loop ()
{
  while (udp.parsePacket()) {
    unsigned int n = udp.read(udp_buffer, 15);
    if (n >= 15) {
      Serial.printf("%d\n", n);
      if (memcmp(udp_buffer, "firmware_update", 15)  == 0) {
        if (firmware_update_in_progress == false) {
          firmware_update_in_progress = true;
          Serial.printf("firmware_update\n" );
          if (FirmwareFlasher.prepare_flash() == 0) {
            udp.beginPacket(udp.remoteIP(), udp.remotePort());
            udp.write(10);
            udp.endPacket();
          }else {
            Serial.printf("error\n");
          }
        }else {
          Serial.printf("error:firmware_update_in_progress\n" );
        }
      }else if (memcmp(udp_buffer, "firmware_line__", 15) == 0) {
        if (firmware_update_in_progress == true) {
          Serial.printf("firmware_line__" );
          unsigned int m = udp.read(udp_buffer, 400);
          Serial.printf("length_%d\n", m);
          for (int i = 0; i < m; i++) {
            if (udp_buffer[i] == '\n') {
              line[i] = 0;
              Serial.printf(" EOL\n" );
              if (FirmwareFlasher.flash_hex_line(line) != 0) {
                Serial.printf("error\n");
              }else {
                udp.beginPacket(udp.remoteIP(), udp.remotePort());
                udp.write(10);
                udp.endPacket();
              }
            }else {
              line[i] = udp_buffer[i];
              Serial.printf("%c", line[i]);
            }
          }
        }else {
          Serial.printf("error:firmware_update_not_in_progress\n");
        }
      }
    }
  }
}
