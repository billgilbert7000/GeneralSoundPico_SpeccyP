#pragma once
#ifndef ZX_MASHINE_H_
#define ZX_MASHINE_H_

#include "inttypes.h"

#include "hw_util.h"



// функции управления zx машиной
void zx_machine_reset(uint8_t rom_x);
void zx_machine_init();
/* void fast(zx_machine_int)(void);// реальный INT 37500Hz HZ через таймер пико */
void fast(zx_machine_main_loop_start)(); // функция содержит бесконечный цикл
void fast (GS_get_sound_LR_sample)();


void init_extram(uint8_t conf);


void fast (audio_mixer)(void);

#endif