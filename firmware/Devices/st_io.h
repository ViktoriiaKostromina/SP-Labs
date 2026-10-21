#ifndef ST_IO_H
#define ST_IO_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
typedef enum { ST_OK, ST_NO_CARD, ST_BUS, ST_TIMEOUT, ST_ID, ST_LENGTH, ST_BCC, ST_CRC, ST_PROTOCOL, ST_NOT_READY } st_result;
typedef struct {
    void *ctx;
    st_result (*exchange)(void *, const uint8_t *, uint8_t *, size_t);
    void (*select)(void *, bool);
    uint32_t (*now_ms)(void *);
} st_io;
#endif
