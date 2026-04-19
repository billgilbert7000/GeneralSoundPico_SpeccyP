#pragma once
#ifndef _AYSOFT_H_
#define _AYSOFT_H_


#include "inttypes.h" 
#include "pico/stdlib.h"

// Глобальная переменная громкости
#define AY_VOLUME_PERCENT 50  // Громкость по умолчанию 80%
extern uint8_t current_volume;
extern uint8_t audio_buster;
extern uint16_t beep_volume;

// extern uint8_t type_sound;
//регистры состояния AY0
extern  uint8_t reg_ay0[16];
extern  uint16_t ay0_R12_R11;
//uint8_t ay0_R7_m;// копия регистра смесителя
extern  uint16_t ay0_A_freq;
extern  uint16_t ay0_B_freq;
extern  uint16_t ay0_C_freq;

// регистры состояния AY1
extern  uint8_t reg_ay1[16];
extern  uint16_t ay1_R12_R11;
//uint8_t ay1_R7_m; // копия регистра смесителя
extern  uint16_t ay1_A_freq;
extern  uint16_t ay1_B_freq;
extern  uint16_t ay1_C_freq;
// для ямахи нужно маску 0x1f
//extern const uint8_t maskAY[16];
//static const uint8_t maskAY[16] = {0xff,0x1f,0xff,0x1f,0xff,0x1f ,0x1f, 0xff, 0x1f,0x1f,0x1f, 0xff , 0xff, 0x0f,0xff,0xff};



void select_audio(void);   
extern void (*audio_out)(void);

uint8_t fast (AY_get_reg)();
void fast(AY_set_reg)(uint8_t);
uint16_t*  fast(get_AY_Out)(uint8_t);
//==============================================
///void AY_select_reg1 (uint8_t N_reg);
uint8_t fast(AY_get_reg1)();
void fast(AY_set_reg1)(uint8_t);
uint16_t*  fast(get_AY_Out1)(uint8_t);
//==============================================

 void  AY_reset();


 void init_vol_ay(uint8_t conf_vol_i2s);

extern uint8_t fast (AY_in_FFFD)(void);
extern void fast (AY_out_FFFD)(uint8_t);// определение указателя на функцию
extern void fast (AY_out_BFFD)(uint8_t);  // определение указателя на функцию

//==================================================================================================
bool fast (get_random)();

void init_i2s_sound();
void set_audio_volume(uint8_t volume_percent);
void set_audio_buster(void); 

void fast (audio_out_i2s_ts)(void);

int16_t static inline (mix_sample)(int16_t const sample1, int16_t const sample2);
 #endif