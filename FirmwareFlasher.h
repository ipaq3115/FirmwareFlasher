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

// apparently better - thanks to Frank Boesing
#define RAMFUNC  __attribute__ ((section(".fastrun"), noinline, noclone, optimize("Os") ))

class FirmwareFlasherClass {
public:
  void upgrade_firmware(void);
  void boot_check(void);

private:
  static int check_compatible(uint32_t min, uint32_t max);
  uint32_t  read_once(unsigned char address);
  void program_once(unsigned char address, uint32_t value);

  void flash_erase_upper();
  int flash_sector_erased(uint32_t address);
  RAMFUNC static int flash_word (uint32_t address, uint32_t word_value);
  RAMFUNC static int flash_long (uint32_t address, uint64_t long_value);
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
