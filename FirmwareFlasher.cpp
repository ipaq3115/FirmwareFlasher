// WARNING:  you can destroy your MCU with flash erase or write!
// This code is not ready for use.

#include <Arduino.h>
#include "FirmwareFlasher.h"
#include <stdio.h>

#define Serial Serial

// ********************************************************************************************

// Version 1.3B

// code to allow firmware update over any byte stream (serial port, radio link, etc)
// for teensy 3.x.  It is intended that this code always be included in your application.

// Jon Zeeff 2016
// This code is in the public domain.  Please retain my name and
// in distributed copies, and let me know about any bugs

// I, Jon Zeeff, give no warranty, expressed or implied for
// this software and/or documentation provided, including, without
// limitation, warranty of merchantability and fitness for a
// particular purpose.


// load new firmware over some ascii byte stream and flash it
// format is an intel hex file followed by ":flash xx" where xx is the number of lines sent
// Max size is 1/2 of flash
// TODO  Modify to allow any combination of programs that fit in flash - ie, allow a 64K program to load a 188K program

// caution:  it is easy to send too fast.  Writing flash takes awhile - put delays between sending lines.

// porting: while the test routine uses a serial port, the only communication link this program needs is supplied by the user supplied routines:
// Serial_Read() and Serial_Printf() and optionally Serial_Available()
// See serial.cpp for an example of these routines used for a packetized, error corrected bluetooth link.


// TODO - instead of splitting flash in half, use whatever flash is remaining.  This would allow a 64K program to flash
// a 192K program.  This would allow the two step process of 192K, downgrade to 64K, upgrade to new 192K.

// TODO - only copy RAMFUNC functions to ram just before they are used

byte FirmwareFlasherClass::saveBytes[100];
uint32_t FirmwareFlasherClass::saveAddr = 0xFFFFFFFF;
uint32_t FirmwareFlasherClass::lastAddr = 0xFFFFFFFF;
uint8_t  FirmwareFlasherClass::saveSize = 0;

uint32_t FirmwareFlasherClass::address = 0;
uint32_t FirmwareFlasherClass::base_address = 0;
int FirmwareFlasherClass::line_count = 0;
int FirmwareFlasherClass::error = 0;
int FirmwareFlasherClass::done = 0;
uint32_t FirmwareFlasherClass::max_address = 0;
uint32_t FirmwareFlasherClass::min_address = ~0;

int FirmwareFlasherClass::prepare_flash(void)
{
  if ((uint32_t)flash_word < FLASH_SIZE || (uint32_t)flash_erase_sector < FLASH_SIZE || (uint32_t)flash_move < FLASH_SIZE) {
    return -1; //routines not in ram
  }

  flash_erase_upper();   // erase upper half of flash

  // what is currently used?
  int32_t addr = FLASH_SIZE / 2;
  while (addr > 0 && *((uint32_t *)addr) == 0xFFFFFFFF) {
    addr -= 4;
  }
  if (addr > FLASH_SIZE / 2 - RESERVE_FLASH) {
    return -2; //firmware is too large
  }

  // Zero all of the values that flash_hex_line() uses 
  // This allows a firmware update to be canceled and resumed
  
  saveAddr = 0xFFFFFFFF;
  lastAddr = 0xFFFFFFFF;
  saveSize = 0;

  address = 0;
  base_address = 0;
  line_count = 0;
  error = 0;
  done = 0;
  max_address = 0;
  min_address = ~0;

  return 0;
}//prepare_flash()

void FirmwareFlasherClass::upgrade_firmware(void)   // main entry point
{
  Serial.printf("%s flash size = %dK in %dK sectors\n", FLASH_ID, FLASH_SIZE / 1024, FLASH_SECTOR_SIZE / 1024);

  flash_erase_upper();   // erase upper half of flash

  if ((uint32_t)flash_word < FLASH_SIZE || (uint32_t)flash_erase_sector < FLASH_SIZE || (uint32_t)flash_move < FLASH_SIZE) {
    Serial.printf("routines not in ram\n");
    return;
  }

  // what is currently used?
  int32_t addr = FLASH_SIZE / 2;
  while (addr > 0 && *((uint32_t *)addr) == 0xFFFFFFFF)
    addr -= 4;
  Serial.printf("current firmware 0:%x\n", addr + 4);

  if (addr > FLASH_SIZE / 2 - RESERVE_FLASH) {
    Serial.printf("firmware is too large\n");
    return;
  }

  Serial.printf("WARNING: this can ruin your device\n");
  Serial.printf("waiting for intel hex lines ¤\n ");

  char line[200];
  int count = 0;

  // read in hex lines
  Serial.printf("Y\n");
  for (;;)  {
    int c;

    while (!Serial.available()) {}
    c = Serial.read();

    if (c == '\n' || c == '\r') {
      line[count] = 0;          // terminate string
      if(flash_hex_line(line) != 0) {
        Serial.printf("error\n");
      }else {
        Serial.printf("Y\n");
      }

      count = 0;
    } else
      line[count++] = c;        // add to string

  } // for ever

} // upgrade_firmware()


// So what happens if you write bad software and it locks up?  How do you execute "upgrade_firmware()" to recover?
// 1) Always call this routine early in setup()
// 2) Tie the send and receive lines of the receiver together and it should recover.

// This routine is optional and can be removed.

void FirmwareFlasherClass::boot_check(void)
{
  delay(1000);
  Serial.printf("@\n");
  delay(500);
  if (Serial.available() && Serial.read() == '@')
    upgrade_firmware();

  return;
}  // boot_check()

// check that the uploaded firmware contains a string that indicates that it will run on this MCU
int FirmwareFlasherClass::check_compatible(uint32_t min, uint32_t max)
{
    return 1;
  uint32_t i;

  // look for FLASH_ID in the new firmware
  for (i = min; i < max - strlen(FLASH_ID); ++i) {
    if (strncmp((char *)i, FLASH_ID, strlen(FLASH_ID)) == 0)
      return 1;
  }
  return 0;
} // check_compatible()

// *****************************************************************************

// WARNING:  you can destroy your MCU with flash erase or write!
// This code may or may not protect you from that.
// 2nd Modifications for teensy 3.5/3/6 by Deb Hollenback at GiftCoder
//    This code is released into the public domain.
// Extensive modifications for OTA updates by Jon Zeeff
// Original by Niels A. Moseley, 2015.
// This code is released into the public domain.

// https://namoseley.wordpress.com/2015/02/04/freescale-kinetis-mk20dx-series-flash-erasing/


static int leave_interrupts_disabled = 0;


// *********************************
// actual flash operation occurs here - must run from ram
// flash a 4 byte word

RAMFUNC int FirmwareFlasherClass::flash_word (uint32_t address, uint32_t word_value)
{
  // Serial.printf("befor: %X\n", *(volatile uint32_t *) address);
  // Serial.printf("writing: %X\n", word_value);
  if (address >= FLASH_SIZE || (address & 0B11) != 0) // basic checks
    return 1;

  // correct value in FTFL_FSEC no matter what
  if (address == 0x40C) {
    word_value = 0xFFFFFFFE;
  }

  // check if already done - not an error
  if (*(volatile uint32_t *) address == word_value)
    return 0;

  // check if not erased
  if (*(volatile uint32_t *) address != 0xFFFFFFFF)  // TODO this fails
    return 4;

  __disable_irq ();

  while ((FTFL_FSTAT & FTFL_FSTAT_CCIF) != FTFL_FSTAT_CCIF)  // wait for ready
  {
  };

  // clear error flags
  FTFL_FSTAT = FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL | FTFL_FSTAT_MGSTAT0;

  // program long word!
  FTFL_FCCOB0 = 0x06;    // PGM
  FTFL_FCCOB1 = address >> 16;
  FTFL_FCCOB2 = address >> 8;
  FTFL_FCCOB3 = address;
  FTFL_FCCOB4 = word_value >> 24;
  FTFL_FCCOB5 = word_value >> 16;
  FTFL_FCCOB6 = word_value >> 8;
  FTFL_FCCOB7 = word_value;

  FTFL_FSTAT = FTFL_FSTAT_CCIF;  // execute!

  while ((FTFL_FSTAT & FTFL_FSTAT_CCIF) != FTFL_FSTAT_CCIF)  // wait for ready
  {
  };

  FMC_PFB0CR |= 0xF << 20;  // flush cache

  if (!leave_interrupts_disabled)
    __enable_irq ();

  // Serial.printf("after: %X\n", *(volatile uint32_t *) address);
  // check if done OK
  if (*(volatile uint32_t *) address != word_value) {
    Serial.printf("should be: %X but is: %X\n", word_value, (volatile uint32_t *) address);
    Serial.printf("FTFL_FSTAT: %X\n", FTFL_FSTAT);
    return 8;
  }

  return FTFL_FSTAT & (FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL | FTFL_FSTAT_MGSTAT0);
}

// *********************************
// actual flash operation occurs here - must run from ram
// flash a 8 byte

RAMFUNC int FirmwareFlasherClass::flash_phrase (uint32_t address, uint64_t phrase_value)
{

// Serial.printf(".");

  if (address >= FLASH_SIZE || (address & 0B111) != 0) { // basic checks
    Serial.printf("flash_phrase basic checks address %d FLASH_SIZE %d\r\n", address);
    return 1;
  }
  
// Serial.printf(".");

  // correct value in FTFL_FSEC no matter what.
  if (address == 0x408) {
    phrase_value = 0xfffff9deffffffff;
  }
// Serial.printf(".");

  if (*(volatile uint64_t *) address == phrase_value) {
    Serial.printf("flash_phrase address == phrase_value\r\n");  
    return 0;
  }
  
// Serial.printf(".");

  // check if not erased
  if (*(volatile uint64_t *) address != 0xFFFFFFFFFFFFFFFF) { // TODO this fails
    Serial.printf("flash_phrase address not erased\r\n");
    return 4;
  }

  Serial.printf("about to disable interrutps\r\n"); Serial.flush(); 
  
  // if(address < FLASH_SIZE / 2) {
      // Serial.printf("address %d\r\n", address); Serial.flush(); 
      // delay(100);
  // }
  
  __disable_irq ();
  #if defined(__MK66FX1M0__)  //thank you Deb Hollenback at GiftCoder
    kinetis_hsrun_disable(); //drop out of highspeed mode for flash writes
  #endif

  while ((FTFL_FSTAT & FTFL_FSTAT_CCIF) != FTFL_FSTAT_CCIF)  // wait for ready
  {
  };

  // clear error flags
  FTFL_FSTAT = FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL | FTFL_FSTAT_MGSTAT0;

  // program long long word!
  FTFL_FCCOB0 = 0x07;   // PGM
  FTFL_FCCOB1 = address >> 16;
  FTFL_FCCOB2 = address >> 8;
  FTFL_FCCOB3 = address;

  FTFL_FCCOB4 = phrase_value >> 24;
  FTFL_FCCOB5 = phrase_value >> 16;
  FTFL_FCCOB6 = phrase_value >> 8;
  FTFL_FCCOB7 = phrase_value >> 0;

  FTFL_FCCOB8 = phrase_value >> 56;
  FTFL_FCCOB9 = phrase_value >> 48;
  FTFL_FCCOBA = phrase_value >> 40;
  FTFL_FCCOBB = phrase_value >> 32;

// Serial.printf(".");

  FTFL_FSTAT = FTFL_FSTAT_CCIF;  // execute!

// Serial.printf(".");

  while ((FTFL_FSTAT & FTFL_FSTAT_CCIF) != FTFL_FSTAT_CCIF)  // wait for ready
  {
  };

// Serial.printf(".");

  FMC_PFB0CR |= 0xF << 20;  // flush cache

  #if defined(__MK66FX1M0__)
    //Invalidate any cached lines in the CODE bus cache
    LMEM_PCCCR |= LMEM_PCCCR_GO+LMEM_PCCCR_INVW1+LMEM_PCCCR_INVW0; //issue invalidate cmds, leave lower settings intact
    while ((LMEM_PCCCR & LMEM_PCCCR_GO) == LMEM_PCCCR_GO)  // wait for invalidate to complete
    {
    };

    kinetis_hsrun_enable(); //enable MCU high speed mode
  #endif
  if (!leave_interrupts_disabled)
    __enable_irq ();

  #if defined(__MK66FX1M0__)
    //if a printf immediately follows this rtn, delay needed post kinetis_hsrun_enable() or monitor may hang.
    delayMicroseconds(300);
  #endif

// Serial.printf(".\r\n");

  Serial.printf("after: %08X", *(volatile uint64_t *) address);
  Serial.printf("%08X\n", ((*(volatile uint64_t *) address)>>32)&0xFFFFFFFF);
  Serial.printf("befre: %08X", phrase_value);
  Serial.printf("%08X\n", (phrase_value>>32) & 0xFFFFFFFF);
  
  Serial.printf("FTFL_FSTAT: %X\n", FTFL_FSTAT);

  // check if value actually written
  if (*(volatile uint64_t *) address != phrase_value) {

    Serial.printf("FAIL\r\n");
    Serial.printf("after: %08X", *(volatile uint64_t *) address);
    Serial.printf("%08X\n", ((*(volatile uint64_t *) address)>>32)&0xFFFFFFFF);
    Serial.printf("befre: %08X", phrase_value);
    Serial.printf("%08X\n", (phrase_value>>32) & 0xFFFFFFFF);
    delay(1000);
    Serial.printf("after: %08X", *(volatile uint64_t *) address);
    Serial.printf("%08X\n", ((*(volatile uint64_t *) address)>>32)&0xFFFFFFFF);
    Serial.printf("befre: %08X", phrase_value);
    Serial.printf("%08X\n", (phrase_value>>32) & 0xFFFFFFFF);
    while(true);
  
    return 8;
  }

  return FTFL_FSTAT & (FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL | FTFL_FSTAT_MGSTAT0);
}

int erase_count = 0;

// *************************************

RAMFUNC int FirmwareFlasherClass::flash_erase_sector (uint32_t address, int unsafe)
{
  if (address > FLASH_SIZE || (address & (FLASH_SECTOR_SIZE - 1)) != 0) // basic checks
    return 1;

  if (address == (0x40C & ~(FLASH_SECTOR_SIZE - 1)) && unsafe != 54321)    // 0x40C is dangerous, don't erase it without override
    return 2;

  __disable_irq ();

#if defined(__MK66FX1M0__)
  kinetis_hsrun_disable();
#endif  
  
  // wait for flash to be ready!
  while ((FTFL_FSTAT & FTFL_FSTAT_CCIF) != FTFL_FSTAT_CCIF)
  {
  };

  // clear error flags
  FTFL_FSTAT = FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL | FTFL_FSTAT_MGSTAT0;

  // erase sector
  FTFL_FCCOB0 = 0x09;
  FTFL_FCCOB1 = address >> 16;
  FTFL_FCCOB2 = address >> 8;
  FTFL_FCCOB3 = address;

  FTFL_FSTAT = FTFL_FSTAT_CCIF;  // execute!

  while ((FTFL_FSTAT & FTFL_FSTAT_CCIF) != FTFL_FSTAT_CCIF)  // wait for ready
  {
  };

  FMC_PFB0CR = 0xF << 20;  // flush cache

#if defined(__MK66FX1M0__)
  //Invalidate any cached lines in the CODE bus cache
  LMEM_PCCCR |= LMEM_PCCCR_GO+LMEM_PCCCR_INVW1+LMEM_PCCCR_INVW0; //issue invalidate cmds, leave lower settings intact
  while ((LMEM_PCCCR & LMEM_PCCCR_GO) == LMEM_PCCCR_GO)  // wait for invalidate to complete
  {
  };
  
  kinetis_hsrun_enable(); //enable MCU high speed mode
#endif

  if (!leave_interrupts_disabled)
    __enable_irq ();

#if defined(__MK66FX1M0__)
  //if a printf immediately follows this rtn, delay needed post kinetis_hsrun_enable() or monitor may hang.
  delayMicroseconds(300); 
#endif

  return FTFL_FSTAT & (FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL | FTFL_FSTAT_MGSTAT0);

}  // flash_erase_sector()

// ***********************************************
// move upper half down to lower half
// DANGER: if this is interrupted, the teensy could be permanently destroyed

RAMFUNC void FirmwareFlasherClass::flash_move (uint32_t min_address, uint32_t max_address)
{
  leave_interrupts_disabled = 1;

  Serial.printf("flash_move min_address %d max_address %d\r\n", min_address, max_address);

  min_address &= ~(FLASH_SECTOR_SIZE - 1);      // round down

  Serial.printf("flash_move min_address %d flash_phrase %p\r\n", min_address, flash_phrase);

  uint32_t address;
  int error = 0;

  // below here is critical

  // copy upper to lower, always erasing as we go up
  #if defined(__MK20DX128__) || defined(__MK20DX256__)
  for (address = min_address; address <= max_address; address += 4) {

    if ((address & (FLASH_SECTOR_SIZE - 1)) == 0) {   // new sector?
      error |= flash_erase_sector(address, 54321);

      if (address == (0x40C & ~(FLASH_SECTOR_SIZE - 1)))  // critical sector
        error |= flash_word(0x40C, 0xFFFFFFFE);                  // fix it immediately
    }

    error |= flash_word(address, *(uint32_t *)(address + FLASH_SIZE / 2));

  } // for
  #elif defined(__MK64FX512__) || defined(__MK66FX1M0__)
  for (address = min_address; address <= max_address; address += 8) {

    if ((address & (FLASH_SECTOR_SIZE - 1)) == 0) {   // new sector?
      error |= flash_erase_sector(address, 54321);

      Serial.printf("New sector err %d\r\n", error);

      // correct value in FTFL_FSEC no matter what.
      if (address == (0x408 & ~(FLASH_SECTOR_SIZE - 1))) {// critical sector
        
        error |= flash_phrase(0x408, 0xfffff9DEffffffff);                  // fix it immediately
        
        Serial.printf("Critical sector err %d\r\n", error);
        
      }
      
    }
    
    byte * ptr = address + FLASH_SIZE / 2;

    Serial.printf("address %d %08X %08X\r\n", 
        address, 
        *(uint64_t *)(address + FLASH_SIZE / 2)); 
    
    for(int i=0;i<8;i++) Serial.printf("%02X ", ptr[i]); Serial.printf("\r\n");
    
    Serial.flush();

    error |= flash_phrase(address, *(uint64_t *)(address + FLASH_SIZE / 2));

  } // for
  #endif
  // hint - can use LED here for debugging.  Or disable erase and load the same program as is running.

  if (error) {
    //digitalWrite(ledPin, HIGH);    // set the LED on and halt
    for (;;) {
        Serial.printf("error %d\r\n", error);
        delay(1000);
    }
  }

  // restart to run new program
#define CPU_RESTART_ADDR ((uint32_t *)0xE000ED0C)
#define CPU_RESTART_VAL 0x5FA0004
  *CPU_RESTART_ADDR = CPU_RESTART_VAL;

}  // flash_move()


// ***********************************

// Given an Intel hex format string, write it to upper flash memory (normal location + 128K)
// When finished, move it to lower flash
//
// Note:  hex records must be 32 bit word aligned!
// TODO: use a CRC value instead of line count

int FirmwareFlasherClass::flash_hex_line (const char *line)
{
    Serial.printf("flash_hex_line %s\r\n", line);
  // hex records info
  unsigned int byte_count;
  unsigned int code;
  char data[128];   // assume no hex line will have more than this.  Alignment?

  // static uint32_t address;
  // static uint32_t base_address = 0;
  // static int line_count = 0;
  // static int error = 0;
  // static int done = 0;
  // static uint32_t max_address = 0;
  // static uint32_t min_address = ~0;

  if (line[0] != ':')		// a non hex line is ignored
    return 0;

  if (error) {      // a single error and nothing more happens
    return -1;
  }

  // check for final flash execute command
  int lines;
  if (sscanf(line, ":flash %d", &lines) == 1) {
    if (lines == line_count && done) {
      Serial.printf("flash %x:%x begins...\n", min_address, max_address);
      delay(100);
      // while(true);
      flash_move (min_address, max_address);
      // should never get here
    } else {
      Serial.printf ("bad line count %d vs %d or not done\n", lines, line_count);
      return -7;
    }
  } // if

  //int parse_hex_line(const char *theline, char bytes, unsigned int *addr, unsigned int *num, unsigned int *code);

  ++line_count;
    
  // must be a hex data line
  if (! FirmwareFlasher.parse_hex_line((const char *)line, (char *)data, (unsigned int *) &address, (unsigned int*) &byte_count, (unsigned int*) &code))
  {
    Serial.printf ("bad hex line %s\n", line);
    error = 1;
    return -1;
  }

  // address sanity check
  if (base_address + address + byte_count > FLASH_SIZE / 2 - RESERVE_FLASH) {
    Serial.printf("address too large\n");
    error = 2;
    return -4;
  }

  // process line
  switch (code)
  {
    case 0:             // normal data
      break;
    case 1:             // EOF
      delay(1000);
      Serial.printf ("done, %d hex lines, address range %x:%x, waiting for :flash %d\n", line_count, min_address, max_address, line_count);
      Serial.printf("saveAddr 0x%X\r\n", saveAddr);
      if (check_compatible(min_address + FLASH_SIZE / 2, max_address + FLASH_SIZE / 2)) {
        done = 1;
      } else {
        Serial.printf ("new firmware not compatible\n");
        return -9999;
      }
      // Only return here if nothing is saved
      if(saveAddr == 0xFFFFFFFF) return 0;
      break;
    case 2:
      base_address = ((data[0] << 8) | data[1]) << 4;   // extended segment address
      return 0;
    case 4:
      base_address = ((data[0] << 8) | data[1]) << 16;  // extended linear address
      return 0;
    default:
      Serial.printf ("err code = %d, line = %s\n", code, line);
      error = 3;
      return -3;
  }				// switch

  // write hex line to upper flash - note, cast assumes little endian and alignment
  #if defined(__MK20DX128__) || defined(__MK20DX256__)
  if (flash_block (base_address + address + (FLASH_SIZE / 2), (uint32_t *)data, byte_count))  // offset to upper 128K
  {
    Serial.printf ("can't flash %d bytes to %x\n", byte_count, address);
    error = 4;
    return -2;
  }
  #elif defined(__MK64FX512__) || defined(__MK66FX1M0__)
  
    // Saved
    if(saveAddr != 0xFFFFFFFF) {

        Serial.printf("Found saved\r\n");
        
        if(done) {
            
            Serial.printf("Write last data in the buffer %d %d\r\n", saveAddr, saveSize);

            // Zero the array
            for(int i=0;i<8;i++) data[i] = 0;
            
            memcpy(data, saveBytes, saveSize);
            
            address = saveAddr;
            byte_count = saveSize;
            
            if(byte_count < 8) byte_count = 8;
            
            saveAddr = 0xFFFFFFFF;
            
        } else if (address == saveAddr + saveSize) {
            
            // Move data up wards in the buffer
            memmove(data+saveSize, data, byte_count);
            
            // Copy saved into the lower part of the buffer
            memcpy(data, saveBytes, saveSize);
            
            Serial.printf("byte_count %d saveSize %d address %d saveAddr %d\r\n", byte_count, saveSize, address, saveAddr);
            
            // Add the size of the saved portion
            byte_count += saveSize;
            
            // Print the array we want to flash
            for(int i=0;i<byte_count;i++) Serial.printf("%02X", data[i]); Serial.println();
            
            // Change to the saved address
            address = saveAddr;
            
            // Reset saved variables
            saveAddr = 0xFFFFFFFF;
            // free(saveBytes);
            
            unsigned int left_over = byte_count % 8;
            
            if(left_over != 0) {
                
                saveAddr = address + byte_count - left_over;
                saveSize = left_over;
                // saveBytes = (uint8_t*) calloc(saveSize, 8);
                memcpy(saveBytes, data + byte_count - left_over, saveSize);
                
                byte_count -= left_over;
                
            }
            
        } else {
            
            Serial.printf("address %d saveAddr %d saveSize %d byte_count %d line\r\n",
                address, saveAddr, saveSize, byte_count, line);
            
            // free(saveBytes);
            
            return -2;
        
        }

        Serial.printf("byte_count %d saveSize %d address %d saveAddr %d\r\n", byte_count, saveSize, address, saveAddr);
        
        Serial.printf("Writing this to flash: ");
        
        // Print the array we want to flash
        for(int i=0;i<byte_count;i++) Serial.printf("%02X", data[i]); Serial.println();
        
    } else {
        
        // Nothing saved and not in alignment
        if(byte_count % 8 != 0) {
            
            if(byte_count >= 16) {
                
                Serial.printf("For some reason not expecting anything this long\r\n");
                return -2;
                
            }
            
            // Save all for later since we can't flash less that 8 bytes
            if(byte_count < 8) {
            
                saveAddr = address;
                saveSize = byte_count;
                // saveBytes = (uint8_t*) calloc(saveSize, 8);
                memcpy(saveBytes, data, saveSize);
                
                return 0;
                
            // Flash lower sections and save the extra for next time
            } else {
                
                saveSize = byte_count - 8;
                saveAddr = address + 8;
                // saveBytes = (uint8_t*) calloc(saveSize, 8);
                memcpy(saveBytes, data + 8, saveSize);
                byte_count = 8;
                
            }
            
        }
        
    }
  
    // if (byte_count % 8 != 0) {
    //     
    //     Serial.printf("not 64bit\n");
    //     
    //     if (byte_count < 16) {
    //         
    //         if (saveAddr != 0xFFFFFFFF) {//somthing is saved
    //         
    //             Serial.printf("Found saved\r\n");
    //             
    //             if (address == saveAddr + saveSize) {
    //                 
    //                 // Move data up wards in the buffer
    //                 memmove(data+saveSize, data, byte_count);
    //                 
    //                 // Copy saved into the lower part of the buffer
    //                 memcpy(data, saveBytes, saveSize);
    //                 
    //                 Serial.printf("byte_count %d saveSize %d address %d saveAddr %d\r\n", byte_count, saveSize, address, saveAddr);
    //                 
    //                 // Add the size of the saved portion
    //                 byte_count += saveSize;
    //                 
    //                 for(int i=0;i<byte_count;i++) Serial.printf("% 02X", data[0]);
    //                 
    //                 // Change to the saved address
    //                 address = saveAddr;
    //                 
    //                 // Reset saved variables
    //                 saveAddr = 0xFFFFFFFF;
    //                 free(saveBytes);
    //                 
    //                 
    //                 
    //             } else {
    //                 
    //                 Serial.printf("address %d saveAddr %d saveSize %d byte_count %d line\r\n",
    //                 address, saveAddr, saveSize, byte_count, line);
    //                 free(saveBytes);
    //                 return -2;
    //             
    //             }
    //             
    //         } else if (byte_count < 8) {//nothing saved and shorter than the minimum flash length
    //         
    //             saveAddr = address;
    //             saveSize = byte_count;
    //             saveBytes = (uint8_t*) calloc(saveSize, 8);
    //             memcpy(saveBytes, data, saveSize);
    //             return 0;
    //             
    //         } else {// nothing saved. flash a phrase and save the overflow
    //             
    //             saveSize = byte_count-8;
    //             saveAddr = address+8;
    //             saveBytes = (uint8_t*) calloc(saveSize, 8);
    //             memcpy(saveBytes, data + 8, saveSize);
    //             byte_count = 8;
    //             
    //         }
    //         
    //     } else {
    //         
    //         Serial.printf("byte_count %d line\r\n", byte_count, line);
    //         return -2;
    //         
    //     }
    //     
    // }
  
  if (flash_block (base_address + address + (FLASH_SIZE / 2), (uint64_t *)data, byte_count))  // offset to upper 128K
  {
    Serial.printf ("can't flash %d bytes to %x\n", byte_count, address);
    error = 4;
    return -2;
  }
  #endif
  // track size of modifications
  if (base_address + address + byte_count > max_address)
    max_address = base_address + address + byte_count;
  if (base_address + address < min_address)
    min_address = base_address + address;

  return 0;
}				// flash_hex_line()

// ****************************
// check if sector is all 0xFF
int FirmwareFlasherClass::flash_sector_erased(uint32_t address)
{
  uint32_t *ptr;

  for (ptr = (uint32_t *)address; ptr < (uint32_t *)(address + FLASH_SECTOR_SIZE); ++ptr) {
    if (*ptr != 0xFFFFFFFF)
      return 0;
  }
  return 1;
} // flash_sector_erased()

// ***************************
// erase the entire upper half
// Note: highest sectors of flash are used for other things - don't erase them
void FirmwareFlasherClass::flash_erase_upper()
{
  uint32_t address;
  int ret;

  // erase each block
  for (address = FLASH_SIZE / 2; address < (FLASH_SIZE - RESERVE_FLASH); address += FLASH_SECTOR_SIZE) {
    if (!flash_sector_erased(address)) {
      Serial.printf("erase sector %x\n", address);
      if ((ret = flash_erase_sector(address, 0)) != 0)
        Serial.printf("flash erase error %d\n", ret);
    }
  } // for
} // flash_erase_upper()


// **************************
// take a word aligned array of words and write it to upper memory flash
#if defined(__MK20DX128__) || defined(__MK20DX256__)
int FirmwareFlasherClass::flash_block (uint32_t address, uint32_t * bytes, int count)
{
  int ret;

  if ((address % 4) != 0 || (count % 4 != 0))  // sanity checks
  {
    Serial.printf ("flash_block align error\n");
    return -1;
  }

  while (count > 0)
  {
    if ((ret = flash_word(address, *bytes)) != 0)
    {
      Serial.printf ("flash_block write error %d\n", ret);
      return -2;
    }
    address += 4;
    ++bytes;
    count -= 4;
  }				// while

  return 0;
}				// flash_block()

#elif defined(__MK64FX512__) || defined(__MK66FX1M0__)
int FirmwareFlasherClass::flash_block (uint32_t address, uint64_t * bytes, int count)
{
  int ret;

  if ((address % 8) != 0 || (count % 8 != 0))  // sanity checks
  {
    Serial.printf ("flash_block align error\n");
    return -1;
  }

  while (count > 0)
  {
    if ((ret = flash_phrase(address, *bytes)) != 0)
    {
      Serial.printf ("flash_block write error %d\n", ret);
      return -2;
    }
    address += 8;
    ++bytes;
    count -= 8;
  }				// while

  return 0;
}				// flash_block()
#endif

// these two are optional

#if 1

// read a WORD from the write once flash area
// address is 0 to 0xF

RAMFUNC uint32_t FirmwareFlasherClass::read_once(unsigned char address)
{
  __disable_irq ();

  // read long word
  FTFL_FCCOB0 = 0x41;   // RDONCE
  FTFL_FCCOB1 = address;

  FTFL_FSTAT = FTFL_FSTAT_CCIF;  // execute!

  while ((FTFL_FSTAT & FTFL_FSTAT_CCIF) != FTFL_FSTAT_CCIF) {} // wait for ready

  __enable_irq ();
  return (FTFL_FCCOB4 << 24) | (FTFL_FCCOB5 << 16) | (FTFL_FCCOB6 << 8) | FTFL_FCCOB7;
}

// write a 4 byte WORD to the write once flash area
// address is 0 to 0xF

RAMFUNC void FirmwareFlasherClass::program_once(unsigned char address, uint32_t word_value)
{
  __disable_irq ();

  // program long word
  FTFL_FCCOB0 = 0x43;   // PGMONCE
  FTFL_FCCOB1 = address;
  FTFL_FCCOB4 = word_value >> 24;
  FTFL_FCCOB5 = word_value >> 16;
  FTFL_FCCOB6 = word_value >> 8;
  FTFL_FCCOB7 = word_value;

  FTFL_FSTAT = FTFL_FSTAT_CCIF;  // execute!

  while ((FTFL_FSTAT & FTFL_FSTAT_CCIF) != FTFL_FSTAT_CCIF) {} // wait for ready

  FMC_PFB0CR |= 0xF << 20;  // flush cache
  __enable_irq ();
}

#endif

// **********************************************************

/* Intel Hex records:

  Start code, one character, an ASCII colon ':'.
  Byte count, two hex digits, indicating the number of bytes (hex digit pairs) in the data field.
  Address, four hex digits
  Record type (see record types below), two hex digits, 00 to 05, defining the meaning of the data field.
  Data, a sequence of n bytes of data, represented by 2n hex digits.
  Checksum, two hex digits, a computed value that can be used to verify the record has no errors.

  Example:
  :109D3000711F0000AD38000005390000F546000035
  :049D400001480000D6
  :00000001FF

*/


/* Intel HEX read/write functions, Paul Stoffregen, paul@ece.orst.edu */
/* This code is in the public domain.  Please retain my name and */
/* email address in distributed copies, and let me know about any bugs */

/* I, Paul Stoffregen, give no warranty, expressed or implied for */
/* this software and/or documentation provided, including, without */
/* limitation, warranty of merchantability and fitness for a */
/* particular purpose. */

// type modifications by Jon Zeeff


/* parses a line of intel hex code, stores the data in bytes[] */
/* and the beginning address in addr, and returns a 1 if the */
/* line was valid, or a 0 if an error occured.  The variable */
/* num gets the number of bytes that were stored into bytes[] */

int FirmwareFlasherClass::parse_hex_line (const char *theline, char *bytes, unsigned int *addr, unsigned int *num, unsigned int *code)
{
  unsigned sum, len, cksum;
  const char *ptr;
  int temp;

  *num = 0;
  if (theline[0] != ':')
    return 0;
  if (strlen (theline) < 11)
    return 0;
  ptr = theline + 1;
  if (!sscanf (ptr, "%02x", &len))
    return 0;
  ptr += 2;
  if (strlen (theline) < (11 + (len * 2)))
    return 0;
  if (!sscanf (ptr, "%04x", (unsigned int *)addr))
    return 0;
  ptr += 4;
  // Serial.printf("Line: length=%d Addr=%d\n", len, *addr);
  if (!sscanf (ptr, "%02x", code))
    return 0;
  ptr += 2;
  sum = (len & 255) + ((*addr >> 8) & 255) + (*addr & 255) + (*code & 255);
  while (*num != len)
  {
    if (!sscanf (ptr, "%02x", &temp))
      return 0;
    bytes[*num] = temp;
    ptr += 2;
    sum += bytes[*num] & 255;
    (*num)++;
    if (*num >= 256)
      return 0;
  }
  if (!sscanf (ptr, "%02x", &cksum))
    return 0;

  if (((sum & 255) + (cksum & 255)) & 255)
    return 0;			/* checksum error */
  return 1;
}



FirmwareFlasherClass FirmwareFlasher;

#undef Serial
