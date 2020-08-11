/*
  SD card read/write
 
 This example shows how to read and write data to and from an SD card file 	
 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11, pin 7 on Teensy with audio board
 ** MISO - pin 12
 ** CLK - pin 13, pin 14 on Teensy with audio board
 ** CS - pin 4, pin 10 on Teensy with audio board
 
 created   Nov 2010
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe
 
 This example code is in the public domain.
 	 
 */
 
#include <SD.h>
#include <SPI.h>
#include "AudioSampleGong.h"
#include <FirmwareFlasher.h>

File myFile;

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
// Teensy audio board: pin 10
// Teensy 3.5 & 3.6 & 4.1 on-board: BUILTIN_SDCARD
// Wiz820+SD board: pin 4
// Teensy 2.0: pin 0
// Teensy++ 2.0: pin 20
const int chipSelect = BUILTIN_SDCARD;

void setup()
{
 //UNCOMMENT THESE TWO LINES FOR TEENSY AUDIO BOARD:
 //SPI.setMOSI(7);  // Audio shield has MOSI on pin 7
 //SPI.setSCK(14);  // Audio shield has SCK on pin 14
  
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
   while (!Serial);
   
   // delay(1000);
   
   while(Serial.read() != -1);
   while(Serial.read() == -1) {
    
        Serial.printf("Waiting...\r\n");
        delay(500);
       
   }


  Serial.print("Initializing SD card...");

  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  
  
    // re-open the file for reading:
    myFile = SD.open("test.hex");
    
    if(myFile) {
    
        int retVal = FirmwareFlasher.prepare_flash();
    
        if(retVal != 0) {
            
            Serial.printf("prepare_flash failed %d\r\n", retVal);
            
            while(1);
            
        }
    
        Serial.println("test.txt:");

        byte file_byte;
        char line_string[100];
        int line_string_length = -1;
        int line_number = 0;

        // read from the file until there's nothing else in it:
        while(myFile.available()) {
            
            file_byte = myFile.read();
            
            if(file_byte == ':') {
            
                // Serial.printf("Start\r\n");
            
                line_string_length = 0;
            
            }
            
            if(line_string_length != -1) {
                
                if(file_byte == '\r') {
                    
                    line_string[line_string_length] = 0;
                
                    Serial.printf("Line % 6d: %s\r\n", line_number, line_string);
                    
                    FirmwareFlasher.flash_hex_line(line_string);
                    
                    line_number++;
                    
                    // if(line_number == 20) return;

                    line_string_length = -1;

                } else {
                    
                    // Serial.printf("file_byte %02X %c\r\n", file_byte, file_byte);
            
                    line_string[line_string_length++] = file_byte;
                    
                }
                
            }
            
        }
        
        // close the file:
        myFile.close();
        
        char str[100];
        
        snprintf(str, 100, ":flash %d", line_number);
        
        FirmwareFlasher.flash_hex_line(str);
        
    } else {
        
        // if the file didn't open, print an error:
        Serial.println("error opening test.txt");
        
    }
 
    // int i;
    // i = millis()%5;
    // i = AudioSampleGong[i];
    // i = AudioSampleGong1[i];
    // i = AudioSampleGong2[i];
    // i = AudioSampleGong3[i];
    // Serial.printf("%d\r\n", i);
    
}

void loop()
{
	// nothing happens after setup
}

