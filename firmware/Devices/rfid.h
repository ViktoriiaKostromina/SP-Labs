#ifndef ST_RFID_H
#define ST_RFID_H
#include "st_io.h"
typedef struct { st_io io; uint8_t version; bool ready; } st_rfid;
typedef struct { uint8_t bytes[7]; uint8_t length; } st_uid;
uint16_t st_crc_a(const uint8_t *p, size_t n);
st_result st_rfid_init(st_rfid *r);
st_result st_rfid_read_reg(st_rfid *r, uint8_t reg, uint8_t *v);
st_result st_rfid_poll(st_rfid *r, st_uid *uid);
st_result st_rfid_block(const uint8_t block[5]);
#endif
