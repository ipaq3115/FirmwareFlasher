// WARNING:  you can destroy your MCU with flash erase or write!
// This code is not ready for use.

#ifndef FirmwareFlasher_h_
#define FirmwareFlasher_h_

#include <stdint.h>
#include <kinetis.h>
//#include "serial.h"

#if defined(__MK20DX128__) //T_3.0
#define FLASH_SIZE              0x20000
#define FLASH_SECTOR_SIZE       0x400
#define FLASH_ID                "fw_teensy3.0"
// TODO: find a way to force the compiler to not optimize FLASH_ID out
#define RESERVE_FLASH (2 * FLASH_SECTOR_SIZE)

#elif defined(__MK20DX256__)//T_3.1 and T_3.2
#define FLASH_SIZE              0x40000
#define FLASH_SECTOR_SIZE       0x800
#define FLASH_ID                "fw_teensy3.1"
#define RESERVE_FLASH (1 * FLASH_SECTOR_SIZE)

#elif defined(__MK64FX512__)//T_3.5
#define FLASH_SIZE              0x80000
#define FLASH_SECTOR_SIZE       0x1000
#define FLASH_ID                "fw_teensy3.5"
#define RESERVE_FLASH (1 * FLASH_SECTOR_SIZE)

#elif defined(__MK66FX1M0__)//T_3.6
#define FLASH_SIZE              0x100000
#define FLASH_SECTOR_SIZE       0x1000
#define FLASH_ID                "fw_teensy3.6"
#define RESERVE_FLASH (1 * FLASH_SECTOR_SIZE)
#else
#error NOT SUPPORTED
#endif

// MCU Local Memory PCCCR Register Bit Definitions (request to add to kinetis.h?)
#if defined(__MK66FX1M0__)
  #define LMEM_PCCCR_GO      ((uint32_t)0x80000000)    //LMC Initiate Cache Command
  #define LMEM_PCCCR_PUSHW1  ((uint32_t)0x08000000)    //LMC Push all modified lines in way 1
  #define LMEM_PCCCR_INVW1   ((uint32_t)0x04000000)    //LMC Invalidate Way 1
  #define LMEM_PCCCR_PUSHW0  ((uint32_t)0x02000000)    //LMC Push all modified lines in way 0
  #define LMEM_PCCCR_INVW0   ((uint32_t)0x01000000)    //LMC Invalidate Way 0
  #define LMEM_PCCCR_PCCR3   ((uint32_t)0x00000008)    //LMC Forces no allocation on cache misses (must also have PCCR2 asserted)
  #define LMEM_PCCCR_PCCR2   ((uint32_t)0x00000004)    //LMC all cacheable areas write through
  #define LMEM_PCCCR_ENWRBUF ((uint32_t)0x00000002)    //LMC write buffer enable
  #define LMEM_PCCCR_ENCACHE ((uint32_t)0x00000001)    //LMC cache enable
#endif

// apparently better - thanks to Frank Boesing
#define RAMFUNC  __attribute__ ((section(".fastrun"), noinline, noclone, optimize("Os") ))

class FirmwareFlasherClass {
public:
  void upgrade_firmware(void);
  void boot_check(void);

private:
  static uint8_t *saveBytes;
  static uint32_t saveAddr;
  static uint8_t  saveSize;

  static int check_compatible(uint32_t min, uint32_t max);
  uint32_t  read_once(unsigned char address);
  void program_once(unsigned char address, uint32_t value);

  void flash_erase_upper();
  int flash_sector_erased(uint32_t address);
  RAMFUNC static int flash_word (uint32_t address, uint32_t word_value);
  RAMFUNC static int flash_phrase (uint32_t address, uint64_t phrase_value);
  RAMFUNC static int flash_erase_sector (uint32_t address, int unsafe);
  RAMFUNC static void flash_move (uint32_t min_address, uint32_t max_address);
  static int flash_hex_line(const char *line);
  int parse_hex_line (const char *theline, char *bytes, unsigned int *addr, unsigned int *num, unsigned int *code);
  #if defined(__MK20DX128__) || defined(__MK20DX256__)
  static int flash_block(uint32_t address, uint32_t *bytes, int count);
  #elif defined(__MK64FX512__) || defined(__MK66FX1M0__)
  static int flash_block(uint32_t address, uint64_t *bytes, int count);
  #endif
};

extern FirmwareFlasherClass FirmwareFlasher;

#endif
