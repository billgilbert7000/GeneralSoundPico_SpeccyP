#pragma GCC optimize("Ofast")

#include "general-midi_wt.h"
#include "gm_bank.h"
#include "wavetable.inl"

// Для совместимости с существующим кодом
//extern int32_t midi_sound = 0;
     extern int16_t midi_L;
     extern int16_t midi_R;

// Банк должен быть загружен во flash или RAM
static const void *g_midi_bank = NULL;

// Функции для совместимости со старым кодом
static uint8_t mpu_status = 0x80;  // STATUS_INPUT_NOT_READY
static uint8_t mpu_rx_data = 0;

static int midi_pos = 0, midi_len = 0;
static uint32_t midi_command = 0;
static int midi_lengths[8] = {3, 3, 3, 3, 2, 2, 3, 1};
static int midi_insysex = 0;

// Инициализация
void midi_wt_init(const void *bank_blob) {
    g_midi_bank = bank_blob;
    wt_set_bank(bank_blob);
    midi_L=0;
    midi_R=0;
    mpu_status = 0x80;
    mpu_rx_data = 0;
    midi_pos = 0;
    midi_len = 0;
    midi_command = 0;
    midi_insysex = 0;
}

void midi_wt_reset(void) {
    wt_engine_reset();
      midi_L=0;
      midi_R=0;
}

// MIDI-сэмпл - совместимость со старой функцией
void fast(midi_sample)(void) {
   if (!wt_has_active_voices()) return;
    midi_sample_stereo(&midi_L, &midi_R);
}

   
// MPU-401 чтение
uint8_t fast(mpu401_read)(void) {
    return mpu_status;
}

// MPU-401 запись
void fast(mpu401_write)(uint8_t value) {
    // Обработка SysEx
    if (value & 0x80 && !(value == 0xF7 && midi_insysex)) {
        midi_pos = 0;
        midi_len = midi_lengths[value >> 4 & 7];
        midi_command = 0;
        if (value == 0xF0) midi_insysex = 1;
    }

    if (midi_insysex) {
        if (value == 0xF7) {
            midi_insysex = 0;
        }
        return;
    }

    if (midi_len) {
        midi_command |= value << (midi_pos * 8);
        if (++midi_pos == midi_len) {
            // Передаем в парсер wavetable
            midi_command_t cmd = {
                .command = (uint8_t)(midi_command & 0xFF),
                .note = (uint8_t)((midi_command >> 8) & 0xFF),
                .velocity = (uint8_t)((midi_command >> 16) & 0xFF),
                .other = (uint8_t)((midi_command >> 24) & 0xFF)
            };
            parse_midi(&cmd);
            
            // Обновляем статус
            mpu_status = 0x00;  // STATUS_READY
        }
    }
}

// Все ноты выключить
void midi_off(void) {
    for (int i = 0; i < WT_MAX_VOICES; i++) {
        if (g_voices[i].active) {
            wt_voice_kill(&g_voices[i]);
        }
    }
    midi_L=0;
    midi_R=0;
}

// Дополнительные функции
bool midi_has_active_voices(void) {
    return wt_has_active_voices();
}

void midi_set_volume(uint8_t channel, uint8_t volume) {
    if (channel < WT_MIDI_CHANNELS) {
        g_channels[channel].volume = volume;
        wt_channel_reamp(channel);
    }
}

void midi_all_notes_off(void) {
    midi_off();
}