#pragma once

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <reent.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#include "pico/mutex.h"
#include "hardware/clocks.h"
#include "hardware/structs/systick.h"  

#include "pico/bootrom.h"

extern uint8_t __in_flash() ROM_GS_M[32768];

//-------------------------------------------------
int	null_printf(const char *str, ...);
void g_delay_ms(int delay);

//---------------------------------------------------------------------------------------------
//Число страниц ОЗУ которые реально могут быть доступны GS
/* #ifdef NO_PSRAM
#ifdef PICO_RP2350
#define RAM_PAGES      5// rp2350 без psram  
#else 
#define RAM_PAGES      4// rp2040
#endif
#else 
#define RAM_PAGES      64// Число страниц ОЗУ 
#endif */
//---------------------------------------------------------------------------------------------
#define VOLTAGE VREG_VOLTAGE_1_30 //VREG_VOLTAGE_1_20 //	vreg_set_voltage(VOLTAGE); // main.h
//=======================================================================================================

#define FW_AUTHOR "Speccy_P"
 
//-----------------------------------------------------------------------------------------------
#define LED_PIN 25 

//======================================================
//========== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ =====================
//======================================================
//SPI
extern uint8_t baudrate;
//---------------------------------
extern uint32_t size_psram;
extern bool psram_type;
#define BOARD_PSRAM  1
#define BUTTER_PSRAM 0

//---------- PSRAM -----------------  
extern bool psram_avaiable;
#define PSRAM_BASE 0x11000000 // Базовый адрес PSRAM в адресном пространстве

//----------------------------------------------------
// GS 
#define GENERAL_SOUND
#define IN_B3   0x03     // in(0xB3)
#define IN_BB   0x0B     // in(0xBB)
#define OUT_B3  0xB3     // out(0xB3)
#define OUT_BB  0xBB     // out(0xBB)

extern uint8_t page;       // Порт 0: D0-D3 - страница, D4-D7 - не используются
extern uint8_t command;    // Порт 1
extern uint8_t data_zx;
extern uint8_t data_gs;
extern uint8_t status;     // Порт 4: D0 - флаг команд, D7 - флаг данных
extern uint8_t volume[4];  // Порты 6-9: D0-D5 - громкость, D6-D7 - не используются

// Конфигурация I2S
extern bool mute;

//===================================================================
extern uint16_t outL;
extern uint16_t outR;

extern int32_t intGS_L;
extern int32_t intGS_R;

    #ifdef MIDI
   // extern int32_t midi_sound;
     extern int16_t midi_L;
     extern int16_t midi_R;
    #endif 
#endif
