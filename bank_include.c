// bank_include.c - включение банка в бинарник
#include <stdint.h>

// Для доступа к данным банка (символы определены в gm_bank_data.c)
#ifdef GM_BANK_PYTHON
    extern const uint8_t gm_bank_data[];
    extern const uint32_t gm_bank_data_size;
#endif

// Если банк не найден, создаем пустой массив
#ifndef GM_BANK_PYTHON
    const uint8_t gm_bank_data[] = {0};
    const uint32_t gm_bank_data_size = 0;
#endif