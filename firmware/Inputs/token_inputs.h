#ifndef ST_INPUTS_H
#define ST_INPUTS_H
#include <stdint.h>
#include <stdbool.h>
#include "rfid.h"
typedef struct { uint16_t samples[8]; uint32_t sum; uint8_t next,count,job; uint16_t raw,mv; bool valid; } st_job_input;
bool st_job_sample(st_job_input *p, bool ok, uint16_t raw);
typedef struct { bool candidate,stable; uint32_t changed_ms; } st_button;
bool st_button_step(st_button *b, bool pressed, uint32_t now);
typedef struct { bool latched; uint8_t absent; st_uid last; } st_token_gate;
bool st_token_step(st_token_gate *g, st_result result, const st_uid *u);
#endif
