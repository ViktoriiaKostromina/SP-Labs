#ifndef ST_SD_SECTOR_H
#define ST_SD_SECTOR_H
#include "st_io.h"
typedef struct { st_io io; bool ready,block_addressing; uint32_t ocr; } st_sd;
st_result st_sd_init(st_sd *s);
st_result st_sd_read(st_sd *s, uint32_t lba, uint8_t data[512]);
st_result st_sd_write(st_sd *s, uint32_t lba, const uint8_t data[512]);
uint32_t st_crc32(const uint8_t *p, size_t n);
#endif
