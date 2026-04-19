#include "config.h"  
#include <stdio.h>
#include "hardware/gpio.h"

// rom
#include "rom/gs105b.h"// ROM GS
//#include "rom/gs104.h"// ROM GS
//----------------------------------------------
int	null_printf(const char *str, ...){return 0;};

void g_delay_ms(int delay)
{
    sleep_ms(delay);
    
};


//======================================================
//========== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ =====================
//======================================================

//---------- PSRAM -----------------  
 bool psram_avaiable;
//----------------------------------------------------


// GS глобальные переменные

uint8_t port_gs_out[32];                     // буфер out gs
uint8_t port_gs_in[32];                     // буфер in gs



uint8_t page=0x00;       // Порт 0: D0-D3 - страница, D4-D7 - не используются
 uint8_t command=0x00;    // Порт 1
uint8_t data_zx=0xff;
uint8_t data_gs=0xff;
 uint8_t status=0xff;     // Порт 4: D0 - флаг команд, D7 - флаг данных
uint8_t volume[4]= {0x3F, 0x3F, 0x3F, 0x3F}; // Максимальная громкость по умолчанию  // Порты 6-9: D0-D5 - громкость, D6-D7 - не используются



bool Int_flag;
bool audio_flag;
bool stop_z80 = true;

int32_t delta_time;

uint16_t outL = 0;
uint16_t outR = 0;

uint16_t uintGS_L = 0;
uint16_t uintGS_R = 0;


int16_t outGS_L = 0;
int16_t outGS_R = 0;

    #ifdef MIDI
int32_t midi_sound= 0;
    #endif 