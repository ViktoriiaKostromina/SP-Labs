#ifndef ST_BRINGUP_H
#define ST_BRINGUP_H
#include "st_io.h"
typedef struct {
    st_io reader,storage;
    uint32_t (*now)(void);
    bool (*adc)(uint16_t *);
    bool (*button)(unsigned);
    bool (*rx)(uint8_t *);
    uint32_t (*rx_lost)(void);
    void (*log)(const char *);
    void (*led)(bool);
    void (*capture)(bool);
} st_platform;
void st_bringup_init(const st_platform *p);
void st_bringup_tick(void);
#endif
