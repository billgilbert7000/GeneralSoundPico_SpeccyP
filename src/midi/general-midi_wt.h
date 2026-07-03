#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "config.h"  
#include "hw_util.h"

// Инициализация
void midi_wt_init(const void *bank_blob);
void midi_wt_reset(void);

// MIDI-функции (аналоги старым)
void fast(midi_sample)(void);  // Совместимость со старой функцией
uint8_t fast(mpu401_read)(void);
void fast(mpu401_write)(uint8_t value);
void midi_off(void);

// Дополнительные функции
bool midi_has_active_voices(void);
void midi_set_volume(uint8_t channel, uint8_t volume);
void midi_all_notes_off(void);

// Объявляем переменные для совместимости
     extern int16_t midi_L;
     extern int16_t midi_R ;