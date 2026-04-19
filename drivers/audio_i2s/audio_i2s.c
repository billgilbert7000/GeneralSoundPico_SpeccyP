
/* Copyright (c) 2025 DeepSeek */

#include "audio_i2s.h"

// Определения для программы PIO (смещения и версия)
#define audio_i2s_wrap_target 0  // Начальная точка цикла программы PIO
#define audio_i2s_wrap 7         // Конечная точка цикла программы PIO
#define audio_i2s_pio_version 0   // Версия программы PIO

#define audio_i2s_offset_entry_point 7u  // Точка входа в программу PIO

// Программа для PIO, которая управляет передачей данных по I2S
static const uint16_t audio_i2s_program_instructions[] = {
            //     .wrap_target
    0x7001, //  0: out    pins, 1         side 2     // Вывод бита данных, установка стороны (side) 2
    0x1840, //  1: jmp    x--, 0          side 3     // Уменьшение x и переход к метке 0, сторона 3
    0x6001, //  2: out    pins, 1         side 0     // Вывод бита данных, сторона 0
    0xe82e, //  3: set    x, 14           side 1     // Установка x = 14, сторона 1
    0x6001, //  4: out    pins, 1         side 0     // Вывод бита данных, сторона 0
    0x0844, //  5: jmp    x--, 4          side 1     // Уменьшение x и переход к метке 4, сторона 1
    0x7001, //  6: out    pins, 1         side 2     // Вывод бита данных, сторона 2
    0xf82e, //  7: set    x, 14           side 3     // Установка x = 14, сторона 3
            //     .wrap
};

#if !PICO_NO_HARDWARE
// Структура с описанием программы PIO
static const struct pio_program audio_i2s_program = {
    .instructions = audio_i2s_program_instructions,  // Указатель на инструкции
    .length = 8,                                     // Длина программы
    .origin = -1,                                    // Автоматическое размещение
    .pio_version = audio_i2s_pio_version,            // Версия PIO
#if PICO_PIO_VERSION > 0
    .used_gpio_ranges = 0x0                          // Используемые GPIO (если версия PICO_PIO > 0)
#endif
};
#endif

// Функция для получения конфигурации SM (State Machine) по умолчанию
static inline pio_sm_config audio_i2s_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + audio_i2s_wrap_target, offset + audio_i2s_wrap);  // Установка границ цикла
    sm_config_set_sideset(&c, 2, false, false);  // Настройка sideset (2 бита, без опционального бита)
    return c;
};

// Глобальные переменные для управления I2S
static int sm_i2s = -1;               // Номер State Machine для I2S
static uint32_t i2s_data;             // Буфер для данных I2S (левый и правый каналы)
static uint32_t trans_count_DMA = 1 << 25;  // Счетчик передач для DMA
static int dma_i2s = -1;
static int dma_i2s_ctrl = -1;


// Инициализация PIO для I2S
static void i2s_pio_init() {
    PIO pio = PIO_I2S;                // Используемый PIO блок
    sm_i2s = SM_I2S;                  // Номер State Machine

    uint offset = pio_add_program(pio, &audio_i2s_program);  // Добавление программы в PIO

    // Настройка функций GPIO для работы с PIO
    uint8_t func = GPIO_FUNC_PIO0;    // Используется только PIO0
    gpio_set_function(I2S_DATA_PIN, func);
    gpio_set_function(I2S_CLK_BASE_PIN, func);
    gpio_set_function(I2S_CLK_BASE_PIN + 1, func);

    // Конфигурация State Machine
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + audio_i2s_wrap_target, offset + audio_i2s_wrap);
    sm_config_set_sideset(&c, 2, false, false);

    pio_sm_config sm_config = audio_i2s_program_get_default_config(offset);
    sm_config_set_out_pins(&sm_config, I2S_DATA_PIN, 1);      // Настройка выходного пина данных
    sm_config_set_sideset_pins(&sm_config, I2S_CLK_BASE_PIN); // Настройка пинов тактирования
    sm_config_set_out_shift(&sm_config, false, true, 32);    // Настройка сдвига (32 бита, справа налево)
    pio_sm_init(pio, sm_i2s, offset, &sm_config);            // Инициализация State Machine

    // Настройка направлений и начального состояния пинов
    uint64_t pin_mask = ((uint64_t)1u << I2S_DATA_PIN) | ((uint64_t)3u << I2S_CLK_BASE_PIN);
    pio_sm_set_pindirs_with_mask64(pio, sm_i2s, pin_mask, pin_mask);
    pio_sm_set_pins64(pio, sm_i2s, 0);  // Очистка пинов

    // Запуск программы PIO
    pio_sm_exec(pio, sm_i2s, pio_encode_jmp(offset + audio_i2s_offset_entry_point));

    // Настройка частоты дискретизации
    uint32_t sample_freq = I2S_FREQ;
    sample_freq *= 2;
    uint32_t system_clock_frequency = clock_get_hz(clk_sys);
    uint32_t divider = system_clock_frequency * 4 / sample_freq;  // Расчет делителя частоты
    pio_sm_set_clkdiv_int_frac(pio, sm_i2s, divider >> 8u, divider & 0xffu);  // Установка делителя
    pio_sm_set_enabled(pio, sm_i2s, true);  // Включение State Machine
}

// Инициализация I2S и DMA
void init_i2s_sound() {

    i2s_pio_init();  // Инициализация PIO

    // Захват каналов DMA
    dma_i2s = dma_claim_unused_channel(true);      // Основной канал DMA
    dma_i2s_ctrl = dma_claim_unused_channel(true); // Контрольный канал DMA

    // Настройка основного канала DMA
    dma_channel_config cfg_dma = dma_channel_get_default_config(dma_i2s);
    channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);  // 32-битные передачи
    channel_config_set_chain_to(&cfg_dma, dma_i2s_ctrl);           // Связь с контрольным каналом
    channel_config_set_read_increment(&cfg_dma, false);             // Фиксированный адрес чтения
    channel_config_set_write_increment(&cfg_dma, false);           // Фиксированный адрес записи

    // Настройка запроса DMA (DREQ)
    uint dreq = DREQ_PIO0_TX0 + sm_i2s;  // Используется только PIO0
    channel_config_set_dreq(&cfg_dma, dreq);

    // Конфигурация основного канала DMA
    dma_channel_configure(
        dma_i2s,
        &cfg_dma,
        &(PIO_I2S->txf[sm_i2s]),  // Адрес FIFO PIO для записи
        &i2s_data,                 // Адрес буфера данных
        1 << 10,                   // Количество передач
        false                      // Не запускать сразу
    );

    // Настройка контрольного канала DMA
    dma_channel_config cfg_dma1 = dma_channel_get_default_config(dma_i2s_ctrl);
    channel_config_set_transfer_data_size(&cfg_dma1, DMA_SIZE_32);
    channel_config_set_chain_to(&cfg_dma1, dma_i2s);  // Связь с основным каналом
    channel_config_set_read_increment(&cfg_dma1, false);
    channel_config_set_write_increment(&cfg_dma1, false);

    // Конфигурация контрольного канала DMA
    dma_channel_configure(
        dma_i2s_ctrl,
        &cfg_dma1,
        &dma_hw->ch[dma_i2s].al1_transfer_count_trig,  // Адрес для перезапуска основного канала
        &trans_count_DMA,                               // Адрес счетчика передач
        1,                                             // Количество передач
        false                                          // Не запускать сразу
    );

    // Запуск DMA
    dma_start_channel_mask((1u << dma_i2s));
}

// Деинициализация I2S 
void i2s_deinit() {
    if (sm_i2s != -1) {
        PIO pio = PIO_I2S;
        pio_sm_set_enabled(pio, sm_i2s, false);
        pio_sm_unclaim(pio, sm_i2s);
        pio_remove_program(pio, &audio_i2s_program, -1);
        sm_i2s = -1;
    }

    if (dma_i2s != -1) {
        dma_channel_unclaim(dma_i2s);
        dma_i2s = -1;
    }

    if (dma_i2s_ctrl != -1) {
        dma_channel_unclaim(dma_i2s_ctrl);
        dma_i2s_ctrl = -1;
    }

    gpio_set_function(I2S_DATA_PIN, GPIO_FUNC_NULL);
    gpio_set_function(I2S_CLK_BASE_PIN, GPIO_FUNC_NULL);
    gpio_set_function(I2S_CLK_BASE_PIN + 1, GPIO_FUNC_NULL);
}

// Функция для отправки данных в I2S (левый и правый каналы)
inline void i2s_out(int16_t l_out, int16_t r_out) {
    i2s_data = (((uint16_t)l_out) << 16) | (((uint16_t)r_out));  // Упаковка данных в 32-битное слово
}