#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "inttypes.h"
#include "hardware/pio.h"
#include <stdlib.h>
#include <string.h>
#include <hardware/pio.h>
#include <hardware/clocks.h>
#include <hardware/dma.h>

// Прототипы функций:
void init_i2s_sound();                  // Инициализация I2S
void i2s_deinit();                      // Деинициализация I2S
void i2s_out(int16_t l_out, int16_t r_out);  // Отправка данных в I2S (левый и правый каналы)

#ifdef __cplusplus
}
#endif