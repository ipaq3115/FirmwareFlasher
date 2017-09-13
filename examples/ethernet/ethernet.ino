#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <TeensyMAC.h>
#define _use_udp
#include <FirmwareFlasher.h>

#define PIN_RESET 9

const char key_word[] = "firmwareupdate";

uint8_t mac[6] = {0,0,0,0,0,0};
IPAddress ip(2, 0, 0, 177);
unsigned int localPort = 8888;      // local port to listen on
EthernetUDP Udp;

uint8_t packetBuffer[200];

void setup() {
  Serial.begin(115200);
  delay(1000);

  mac[0] = teensyMAC()>>40;
  mac[1] = teensyMAC()>>32;
  mac[2] = teensyMAC()>>24;
  mac[3] = teensyMAC()>>16;
  mac[4] = teensyMAC()>>8;
  mac[5] = teensyMAC();

  #ifdef PIN_RESET
  pinMode(PIN_RESET, OUTPUT);
  digitalWrite(PIN_RESET, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_RESET, HIGH);
  delay(150);
  #endif


  Ethernet.begin(mac, ip);
  Udp.begin(localPort);
  Serial.printf("mac: %X:%X:%X:%X:%X:%X\nip: %d.%d.%d.%d\nUDP port: %d", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5], ip[0], ip[1], ip[2], ip[3],localPort);
  pinMode(13,OUTPUT);
} //setup

void loop() {
  while (Udp.parsePacket()) {
    Udp.read(packetBuffer, sizeof(key_word));
    if (memcmp(packetBuffer,key_word,sizeof(key_word)) == 0) {
      Serial.printf("recived firmware packet\n");

    } //if (memcmp)
  } //while
  digitalWrite(13,HIGH);
  delay(1000);
  digitalWrite(13,LOW);
  delay(1000);
} //loop
