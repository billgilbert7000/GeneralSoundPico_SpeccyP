
#include "config.h" 
#include "gs_main.h"

#include <stdio.h>
#include "hardware/gpio.h"
#include <pico/stdlib.h>
#include "pico/multicore.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/irq.h"
#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "hardware/structs/systick.h"
#include "hardware/pwm.h"
#include "hardware/vreg.h"

#include "hardware/spi.h"

#include "string.h"

#include "gs_machine.h"

#include "audio_i2s.h"

#include "picobus/picobus.h"

#include "sound_ay/aySoft.h"
#include "rtc/rtc_ds1287.h"

#ifdef MIDI
#include "midi/general-midi_wt.h"
#endif

#if PICO_RP2350
#include <hardware/structs/xip.h>
#include <hardware/regs/sysinfo.h>
#include <hardware/structs/qmi.h>
//uint32_t get_psram_size();
//bool is_psram_enabled();
#endif
//---------------------------------------------------------------
#define PSRAM_MAX_FREQ_MHZ 166
// переменные
bool is_SD_active=false;
extern bool psram_avaiable;
//===============================================================
void out_init(uint pin,bool val)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT); 
    gpio_put(pin,val);    
};
//--------------------------------------------------
static inline uint8_t READ_SD_BYTE()
{
    uint8_t dataSPI=spi_get_hw(SDCARD_SPI_BUS)->dr;
    spi_get_hw(SDCARD_SPI_BUS)->dr=0xff;  
    return   dataSPI;
}

static inline void  WRITE_SD_BYTE(uint8_t data) 
{
    volatile uint8_t dataSPI=spi_get_hw(SDCARD_SPI_BUS)->dr;
    spi_get_hw(SDCARD_SPI_BUS)->dr=data; 

}
//============================================================
//extern bool im_z80_stop;
//extern bool im_ready_loading;

// ========== КОНФИГУРАЦИЯ ==========
// формат передачи 
#define SERVICE_COMMAND 0x00 // служебная команда 

#define GS_INFO         0x00 // информация о GS
#define GS_RESET        0x01 // Сброс pico GS
#define TS_VOLUME       0x02 // второй байт команды третий значение
#define TS_RESET        0x03 // второй байт команды
#define TS_BUSTER       0x04 // второй байт команды третий  значение 
#define MUTE_GLOBAL     0x05 // Полное отключение звука
#define RTC_DATE_TIME   0x06 // Передача строки Data Time  
#define RTC_TIME        0x07 // Передача строки Time  
#define RTC_BIN         0x08 // Передача данных RTC  

// дефайны эмуляции портов GS  
#define PICOBUS_CONNECT    0x77    // команда инициализации

// дефайны эмуляции портов GS  
#define GS_READ_IN_B3      0x01     // in(0xB3)
#define GS_STATUS_IN_BB    0x02      // in(0xBB)
#define GS_WRITE_OUT_B3    0x03     // out(0xB3)
#define GS_COMMAND_OUT_BB  0x04   // out(0xBB)
// дефайны эмуляции портов Z-Controler
#define ZC_READ_IN_57      0x05
#define ZC_READ_IN_77      0x06
#define ZC_WRITE_OUT_57    0x07
#define ZC_WRITE_OUT_77    0x08
// дефайны эмуляции портов AY TS
#define TS_READ_IN_FFFD    0x09
#define TS_READ_IN_BFFD    0x0A // ;)
#define TS_WRITE_OUT_FFFD  0x0B
#define TS_WRITE_OUT_BFFD  0x0C
// дефайны эмуляции портов MIDI
#define MIDI_IN            0x0E
#define MIDI_OUT           0x0F
// дефайны эмуляции портов RTC 
#define RTC_READ_REG       0x10
#define RTC_WRITE_REG      0x11
#define RTC_WRITE_ADRESS   0x12
#define RTC_READ_REG_NOVA  0x13
// дефайны эмуляции портов RTC версия для SMUC
#define RTC_READ_IN_DFBA   0x14
#define RTC_WRITE_OUT_FFBA 0x15
#define RTC_WRITE_OUT_DFBA 0x16
//---------------------------------------------------------
void pico_reset(void)
{
    
    watchdog_enable(1, true); // Включение watchdog на 1ms
    while (1);
        
}
//-------------------------------------------------------------------------
void ZXThread()
{
    zx_machine_init();
    zx_machine_main_loop_start(); // главный цикл выполнения команд Z80
    return;
}

void inInit(uint gpio)
{
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}

//################################################################
uint8_t rx_buffer[128];// буфер picobus

// Инициализация/переинициализация picobus  после включения или при hard reset
void init_picobus(void)
{  
  
    picobus_link_init();


// sleep_ms(20);  // printf("Waiting for init sequence...\n" );
 
  wait_for_init_sequence();//
 
 //  printf("...received and ACKed\n" );

}
//----------------------------------
void fast (picobus_read_write)(void)
{
     uint8_t operator;
     uint8_t value;
 //   printf("receive_buffer .....");
 //  receive_buffer( rx_buffer, 2); // получаем 2 байта

      if (receive_acked_byte(&operator) == LINK_BYTE_NONE) {
        return ; // Первого входящего байта нет - выходим
    } 
//printf("PICOBUS_CONNECT operator: 0x%02X\n",  operator);
 while (receive_acked_byte(&value) == LINK_BYTE_NONE);// ждем второй байт

//if (receive_acked_byte_timeout(&value) == LINK_BYTE_TIMEOUT)   pico_reset();//   return;// вышло время ожидания байта
    // Данные получены
//gpio_put(LED_PIN, 1);
 //  printf("READ : 0x%02X\n",  operator);
 //  printf("READ 1: 0x%02X\n",  value);
      switch (operator)
      {

      case PICOBUS_CONNECT:  // чтение данных GS
       send_byte(PICOBUS_CONNECT);
   //    printf("PICOBUS_CONNECT: 0x%02X\n",  value);
      break;


      
      case GS_READ_IN_B3:  // чтение данных GS
        status &= 0x7F; // D7 = 0 // флаг передачи даных из GS показывает что данные в ZX полученны 
        send_byte(data_gs);
  //     printf("GS_READ_IN_B3: 0x%02X\n",  data_gs);
      break;

      case GS_STATUS_IN_BB:// чтение status
          send_byte(status | 0x7E );
   //   printf("GS_READ_IN_BB: 0x%02X\n",  status | 0x7E);
  // busy_wait_us(1);
      break;

      case GS_WRITE_OUT_B3:// чтение данных zx
        data_zx =  value;
        status |= 0x80;//(устанавливает флаг D7 данных) / данные ZX установленны можно считывать
   //      printf("GS_WRITE_OUT_B3: 0x%02X\n",  rx_buffer[1]);
       break;

      case GS_COMMAND_OUT_BB: //чтение команды для GS
        command = value;// команда GS
        status |= 0x01;//( флаг D0 команд)//?
  //     printf("GS_COMMAND_OUT_BB : 0x%02X\n",  rx_buffer[1]);
      break;    
//--------------------------------------------------------
     #if defined Z_CONTROLER
      case ZC_READ_IN_57:  // чтение данных ZC
        //value = 0xff;
        if (is_SD_active) value = READ_SD_BYTE(); 
        send_byte(value);
      //  printf("ZC_READ_IN_57: 0x%02X\n",  value);
      break;

      case ZC_READ_IN_77:  //управление SD  if (is_SD_active) put_dataZ80(0xfc);
      value = 0xff;
        if (is_SD_active) value = 0xfc;
        send_byte(value);
     //  printf("ZC_READ_IN_77: 0x%02X\n",  value);
      break;

      case ZC_WRITE_OUT_57:// передача данных в SD карту

       //  printf("ZC_WRITE_OUT_57: 0x%02X\n",  value);

            if (is_SD_active)  WRITE_SD_BYTE(value);

       break;
      case ZC_WRITE_OUT_77://управление SD   SDCARD_PIN_SPI0_CS

      // printf("ZC_WRITE_OUT_77: 0x%02X\n",  value & 0x02);val&0x02

                        is_SD_active=((value & 0x01)==1);
                        gpio_put(SDCARD_PIN_SPI0_CS,value & 0x02);
       break;
     #endif
//------------------------------------------------------------------------     
     #ifdef TURBOSOUND
         case TS_READ_IN_FFFD :// IN FFFD 
         value = AY_in_FFFD();
         send_byte(value);
    //      printf("TS_READ_IN_FFFD : 0x%02X\n", value);
      break;

      case TS_WRITE_OUT_FFFD :// OUT FFFD 
          AY_out_FFFD(value);
      //   printf("TS_WRITE_OUT_FFFD : 0x%02X\n",  value);
       break;

      case TS_WRITE_OUT_BFFD :// OUT BFFD
        // AY_out_BFFD(value);
        AY_out_BFFD(value);
        //  printf("TS_WRITE_OUT_BFFD: 0x%02X\n",  value);
       break;
     #endif  
//------------------------------------------------------------------------     
    #ifdef MIDI
         case MIDI_IN :// IN 0xa1cf 
         value = mpu401_read();
         send_byte(value);
    //      printf("MIDI_IN : 0x%02X\n", value);
      break;
         case MIDI_OUT :// out 0xa0cf  
         mpu401_write(value); 
     //  printf("MIDI_OUT : 0x%02X\n",  value);
      break;
    #endif     
//-----------------------------------------------------------------------
     #ifdef RTC_NOVA 
      case RTC_READ_REG_NOVA:  value = rtc_read_registr_nova(rtc_adress);  send_byte(value);break;
      case RTC_WRITE_REG :  rtc_write_registr(rtc_adress,value); break;
      case RTC_WRITE_ADRESS: rtc_adress = value; break;
     #endif
//------------------------------------------------------------------------
     #ifdef RTC_SMUC
      case RTC_READ_IN_DFBA:  value = rtc_read_registr(rtc_adress);  send_byte(value);break;
      case RTC_WRITE_OUT_FFBA: rtc_adress_data=(value>>7); break;
      // если rtc_adress_data = 1 то записывается номер регистра часов если 0 то запись значения в регистр
      case RTC_WRITE_OUT_DFBA:   if (rtc_adress_data==0) rtc_adress = value;
                                     else  rtc_write_registr(rtc_adress,value);
      break;
     #endif
//------------------------------------------------------------------------
//-----------------------------------------------------------------------
     #ifdef RTC_GLUK
      case RTC_READ_REG :  value = rtc_read_registr(rtc_adress);  send_byte(value);break;
     #endif
//------------------------------------------------------------------------
         case SERVICE_COMMAND:
      
      switch (value)
      {
      case GS_INFO:// данные информации о GS
                /*
               p.s. В прошивке по смещению #0004 находится номер версии в BCD формате; по
               смещению #0100 находятся оригинальные копирайты (3 строки по 24 символа); по
               смещению #0800 находится информация о патче, строка заканчивается 0.
               */
                uint8_t n_version[6];
                snprintf(n_version,6 ,ROM_GS_M+0x138);
                snprintf(rx_buffer, 32, "GeneralSound v%s RAM %dKb  GSP v%s  ", n_version, RAM_PAGES*32);
                #ifdef PICO_RP2350
                snprintf(rx_buffer+32, 32, "GSP v%s ", FW_VERSION);
                #else
                snprintf(rx_buffer+32, 32, "GSP lite v%s ", FW_VERSION);
                #endif
                 send_buffer(  rx_buffer, 64 );
        //       gpio_put(LED_PIN, 0);
          return; 

      case  GS_RESET: // принудительный reset //  printf("RESET GS\n");
 //     gpio_put(LED_PIN, 1);
         pico_reset();
        return; 

        case TS_VOLUME:// регулировка глобальной громкости 
       while (receive_acked_byte(&value) == LINK_BYTE_NONE);// принять  байт значение громкости
      //  if (receive_acked_byte_timeout(&value) == LINK_BYTE_TIMEOUT) return;// принять  первый байт значение громкости
          set_audio_volume(value);
        return; 

        case TS_BUSTER:// регулировка усилителя AY 
        while (receive_acked_byte(&value) == LINK_BYTE_NONE);// принять  первый байт значение громкости
      //  if (receive_acked_byte_timeout(&value) == LINK_BYTE_TIMEOUT) return;// принять  первый байт значение громкости
        audio_buster = value;
        set_audio_buster(); 
        return;        

        case MUTE_GLOBAL:// отключение и включение звука
        while (receive_acked_byte(&value) == LINK_BYTE_NONE);// принять  первый байт 
      //  if (receive_acked_byte_timeout(&value) == LINK_BYTE_TIMEOUT) return;// принять  первый байт 
        mute = value &  0x01;
        return;   

        case TS_RESET:// 
        mute =0x01; // отключение звука

        #ifdef MIDI
        midi_off();
        #endif

        AY_reset();
        mute =0x00; // включение звука
        return; 

       #ifdef RTC
       case RTC_DATE_TIME:
       rtc_get_datetime_str(rx_buffer, 20);
       send_buffer(rx_buffer, 20 );
       return; 
      
       case RTC_TIME: // 00:00:00 len=9
       rtc_get_time_str(rx_buffer, 9);
       send_buffer(rx_buffer, 9 );
       return;
       
       case RTC_BIN: //
       rtc_get_time_bin(rx_buffer, 6);

       send_buffer(rx_buffer, 6 );
       return; 
       



       #endif
      }
           
      default:
     
        break;
      }
   
  }

//=========================================================
// I2S SOUND
//=========================================================
// Инициализация аудио
void audio_init() {
 // Установка громкости каналов GS
init_vol_ay(50);// установка громкости AY глобальная нужно сделать

i2s_out(0x7fff,0x7fff) ;// середина диапазона для устранения щелчков

#ifdef TURBOSOUND
         select_audio();
         AY_reset();
#endif

#ifdef MIDI
        midi_off();
#endif

}

//=========================================================
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef PICO_RP2350 

// ================================================================
//  функция таймингов Flash
// ================================================================
static void __no_inline_not_in_flash_func(set_flash_timings)(void) {
        const uint32_t clock_hz = CPU_MHZ * 1000000;  
        const int max_flash_freq = 166 * 1000000;

        int divisor = (clock_hz + max_flash_freq - 1) / max_flash_freq;
        if (divisor == 1 && clock_hz > 100000000) {
            divisor = 2;
        }
        int rxdelay = divisor;
        if (clock_hz / divisor > 100000000) {
            rxdelay += 1;
        }
        qmi_hw->m[0].timing = 0x60007000 |
                            rxdelay << QMI_M0_TIMING_RXDELAY_LSB |
                            divisor << QMI_M0_TIMING_CLKDIV_LSB;
           
}
#endif

//==========================================================================
//===============================================================
// Работа с бутербродной PSRAM 
volatile uint8_t * PSRAM_DATA = (uint8_t*)0x11000000;
#if defined(PICO_RP2350)

//=================================================================
/**
 * @brief Проверка доступности PSRAM 
 * @return Размер PSRAM в Мегабайтах (8MB для APS6404)
 */
#define MB16 (16ul << 20)
#define MB8 (8ul << 20)
#define MB4 (4ul << 20)
#define MB1 (1ul << 20)


static int BUTTER_PSRAM_SIZE = 0;
 uint32_t __not_in_flash_func(get_psram_size)() {
   // if (BUTTER_PSRAM_SIZE != -1) return BUTTER_PSRAM_SIZE;
    for(register int i = MB8; i < MB16; i += 4096)
        PSRAM_DATA[i] = 16;
    for(register int i = MB4; i < MB8; i += 4096)
        PSRAM_DATA[i] = 8;
    for(register int i = MB1; i < MB4; i += 4096)
        PSRAM_DATA[i] = 4;
    for(register int i = 0; i < MB1; i += 4096)
        PSRAM_DATA[i] = 1;
    register uint32_t res = PSRAM_DATA[MB16 - 4096];
    for (register int i = MB16 - MB1; i < MB16; i += 4096) {
        if (res != PSRAM_DATA[i])
            return 0;
    }
    BUTTER_PSRAM_SIZE = res;// << 20;
    return BUTTER_PSRAM_SIZE;
} 
// упрощенная версия только PSRAM = 8
/* uint32_t __not_in_flash_func(get_psram_size)() {
             PSRAM_DATA[0] = 8;
        if (PSRAM_DATA[0]!=8)
            return PSRAM_DATA[0];

    return PSRAM_DATA[0];
}
 */

//-------------------------------------------------------------------------------
/**
 * @brief Деинициализация PSRAM и восстановление настроек по умолчанию
 * @param cs_pin Номер пина Chip Select для PSRAM
 */

void __no_inline_not_in_flash_func(deinit_psram_butter)(uint cs_pin)  {
    // 1. Отключить запись в PSRAM
    hw_clear_bits(&xip_ctrl_hw->ctrl, XIP_CTRL_WRITABLE_M1_BITS);
/*     
    // 2. Сбросить настройки QMI для CS1
    qmi_hw->m[1].rfmt = 0;
    qmi_hw->m[1].wfmt = 0;
    qmi_hw->m[1].rcmd = 0;
    qmi_hw->m[1].wcmd = 0;
    qmi_hw->m[1].timing = 0;
     */
// Выключение прямого доступа, но QMI остается настроенным
    qmi_hw->direct_csr = 0;

    // 3. Переключить CS1 в обычный GPIO режим
    gpio_set_function(cs_pin, GPIO_FUNC_SIO);
    gpio_set_dir(cs_pin, GPIO_IN);
    
  // 4. Сбросить указатель на PSRAM
  //   PSRAM_DATA = (uint8_t*)0;
    
    // 5. Обнулить размер
  //   BUTTER_PSRAM_SIZE = 0;
}
/* 
void __no_inline_not_in_flash_func(deinit_psram_butter)(uint cs_pin) {
    // 1. Запрещаем запись в PSRAM через XIP
    hw_clear_bits(&xip_ctrl_hw->ctrl, XIP_CTRL_WRITABLE_M1_BITS);

    // 2. Сбрасываем настройки команд
    qmi_hw->m[1].rcmd = 0x00;  // Сброс команды чтения
    qmi_hw->m[1].wcmd = 0x00;  // Сброс команды записи

    // 3. Восстанавливаем стандартные тайминги
    qmi_hw->m[1].timing = 0x0; // Сброс всех битов таймингов

    // 4. Возвращаемся в SPI режим (из QPI)
    const uint CMD_SPI_EN = 0xF5; // Команда перехода в SPI режим
    qmi_hw->direct_csr = QMI_DIRECT_CSR_EN_BITS; // Включаем прямой режим
    qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | CMD_SPI_EN;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);

    // 5. Отключаем прямой режим
    qmi_hw->direct_csr = 0;

    // 6. Сбрасываем настройки QSPI
   //qmi_hw->io_ctrl = 0;        // Сброс управления вводом-выводом TODO
   // qmi_hw->ctrl = 0;           // Сброс основного контроля TODO

    // 7. Восстанавливаем функцию пина CS
    gpio_set_function(cs_pin, GPIO_FUNC_NULL);
    gpio_set_dir(cs_pin, GPIO_IN);

    // 8. Сбрасываем тактирование QSPI
 //   clocks_hw->periph_reset |= CLOCKS_PERIPH_RESET_QSPI_BITS;
 //   clocks_hw->periph_reset &= ~CLOCKS_PERIPH_RESET_QSPI_BITS;

    // 9. Отключаем банк памяти M1 в XIP
   // hw_clear_bits(&xip_ctrl_hw->ctrl, XIP_CTRL_ENABLE_M1_BITS);
} */


//=============================================================================
// инициализация PSRAM 
//=============================================================================

/**
 * @brief Проверка наличия PSRAM чипа через чтение ID
 * @return true если чип обнаружен, false если нет
 */
static bool __no_inline_not_in_flash_func(psram_detect)(void) {
    // Выход из QPI режима (если чип остался в нем после перезагрузки)
    const uint8_t CMD_EXIT_QPI = 0xF5;
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    qmi_hw->direct_tx = QMI_DIRECT_TX_OE_BITS |
                        (QMI_DIRECT_TX_IWIDTH_VALUE_Q << QMI_DIRECT_TX_IWIDTH_LSB) |
                        CMD_EXIT_QPI;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
    (void)qmi_hw->direct_rx;
    qmi_hw->direct_csr &= ~QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    
    // Небольшая задержка для стабилизации
    for (volatile int d = 0; d < 64; ++d);

    // Чтение ID: команда 0x9F + 3 байта адреса (0xFF), затем MF ID и KGD
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    uint8_t mfid = 0, kgd = 0;
    
    for (int i = 0; i < 6; ++i) {
        qmi_hw->direct_tx = (i == 0) ? 0x9F : 0xFF;
        while (!(qmi_hw->direct_csr & QMI_DIRECT_CSR_TXEMPTY_BITS));
        while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
        uint8_t b = (uint8_t)qmi_hw->direct_rx;
        if (i == 4) mfid = b;      // 5-й байт = Manufacturer ID
        else if (i == 5) kgd = b;  // 6-й байт = Device ID (KGD)
    }
    
    qmi_hw->direct_csr &= ~QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    
    // Проверка: APS6404 имеет KGD = 0x5D или MFID = 0x0D
    // Если чипа нет, шина "плавает" и возвращает 0x00 или 0xFF
    if (kgd == 0x5D || mfid == 0x5D || mfid == 0x0D) {
        return true;
    }
    
    // Повторная попытка (иногда первый доступ после сброса возвращает мусор)
    // Делаем вторую попытку
    for (volatile int d = 0; d < 256; ++d);
    
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    mfid = 0;
    kgd = 0;
    
    for (int i = 0; i < 6; ++i) {
        qmi_hw->direct_tx = (i == 0) ? 0x9F : 0xFF;
        while (!(qmi_hw->direct_csr & QMI_DIRECT_CSR_TXEMPTY_BITS));
        while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
        uint8_t b = (uint8_t)qmi_hw->direct_rx;
        if (i == 4) mfid = b;
        else if (i == 5) kgd = b;
    }
    
    qmi_hw->direct_csr &= ~QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    
}

/**
 * @brief Инициализация PSRAM (APS6404) на RP2350
 * @param cs_pin Номер пина Chip Select для PSRAM
 * 
 * @note Функция размещается в RAM (не во flash) для корректной работы с XIP
 */
void __no_inline_not_in_flash_func(init_psram_butter)(uint cs_pin) {
    // Отключаем прерывания на ВЕСЬ период работы с прямым режимом QMI
    const uint32_t ints = save_and_disable_interrupts();
    
    // 1. Настройка пина Chip Select
    gpio_set_function(cs_pin, GPIO_FUNC_XIP_CS1);
    
    // 2. Включение прямого режима (Direct Mode) QMI
    // Используем делитель 30 для безопасного чтения ID
    qmi_hw->direct_csr = 30 << QMI_DIRECT_CSR_CLKDIV_LSB | 
                         QMI_DIRECT_CSR_EN_BITS |
                         QMI_DIRECT_CSR_AUTO_CS1N_BITS;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
    
    // 3. Проверка наличия чипа PSRAM
    if (!psram_detect()) {
        // Чип не обнаружен - выключаем прямой режим и выходим
        qmi_hw->direct_csr = 0;
        restore_interrupts(ints);
        deinit_psram_butter(cs_pin);
        psram_avaiable = 0;
        return;
    }
    
    // 4. Переход в QPI-режим (4-битный интерфейс)
    // Сначала выходим из QPI на всякий случай (если чип остался в QPI)
    const uint8_t CMD_EXIT_QPI = 0xF5;
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    qmi_hw->direct_tx = QMI_DIRECT_TX_OE_BITS |
                        (QMI_DIRECT_TX_IWIDTH_VALUE_Q << QMI_DIRECT_TX_IWIDTH_LSB) |
                        CMD_EXIT_QPI;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
    (void)qmi_hw->direct_rx;
    qmi_hw->direct_csr &= ~QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    for (volatile int d = 0; d < 64; ++d);
    
    // Теперь включаем QPI
    const uint8_t CMD_QPI_EN = 0x35;
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | CMD_QPI_EN;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
    qmi_hw->direct_csr &= ~QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    
    // 5. ВЫКЛЮЧАЕМ ПРЯМОЙ РЕЖИМ - критически важно!
    // Все дальнейшие вычисления и настройки выполняются БЕЗ прямого режима
    qmi_hw->direct_csr = 0;
    
    // Теперь можно включить прерывания - прямой режим выключен
    restore_interrupts(ints);
    
    // 6. Расчет временных параметров для PSRAM APS6404
    const int max_psram_freq = PSRAM_MAX_FREQ_MHZ * 1000000;
    const int clock_hz = clock_get_hz(clk_sys);
    
    // Расчет делителя частоты
    int divisor = (clock_hz + max_psram_freq - 1) / max_psram_freq;
    if (divisor == 1 && clock_hz > 100000000) {
        divisor = 2;
    }
     
    // Расчет задержки чтения
    int rxdelay = divisor;
    if (clock_hz / divisor > 100000000) {
        rxdelay += 1;
    }
    
    // Расчет периода системного такта (в фемтосекундах)
    const int clock_period_fs = 1000000000000000ll / clock_hz;
    
    // Макс. время удержания CS (<=8 мкс)
    const int max_select = (125 * 1000000) / clock_period_fs;
    // Мин. время между транзакциями (>=18 нс)
    const int min_deselect = (18 * 1000000 + (clock_period_fs - 1)) / clock_period_fs - (divisor + 1) / 2;
    
    // 7. Запись параметров в регистр таймингов
    qmi_hw->m[1].timing = 
        1 << QMI_M1_TIMING_COOLDOWN_LSB |
        QMI_M1_TIMING_PAGEBREAK_VALUE_1024 << QMI_M1_TIMING_PAGEBREAK_LSB |
        max_select << QMI_M1_TIMING_MAX_SELECT_LSB |
        min_deselect << QMI_M1_TIMING_MIN_DESELECT_LSB |
        rxdelay << QMI_M1_TIMING_RXDELAY_LSB |
        divisor << QMI_M1_TIMING_CLKDIV_LSB;
    
    // 8. Настройка форматов команд
    // Формат чтения (Quad I/O Fast Read)
    qmi_hw->m[1].rfmt =
        QMI_M0_RFMT_PREFIX_WIDTH_VALUE_Q << QMI_M0_RFMT_PREFIX_WIDTH_LSB |
        QMI_M0_RFMT_ADDR_WIDTH_VALUE_Q << QMI_M0_RFMT_ADDR_WIDTH_LSB |
        QMI_M0_RFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M0_RFMT_SUFFIX_WIDTH_LSB |
        QMI_M0_RFMT_DUMMY_WIDTH_VALUE_Q << QMI_M0_RFMT_DUMMY_WIDTH_LSB |
        QMI_M0_RFMT_DATA_WIDTH_VALUE_Q << QMI_M0_RFMT_DATA_WIDTH_LSB |
        QMI_M0_RFMT_PREFIX_LEN_VALUE_8 << QMI_M0_RFMT_PREFIX_LEN_LSB |
        6 << QMI_M0_RFMT_DUMMY_LEN_LSB;
    
    qmi_hw->m[1].rcmd = 0xEB; // Команда чтения (Quad I/O Fast Read)
    
    // Формат записи (Quad Page Program)
    qmi_hw->m[1].wfmt =
        QMI_M0_WFMT_PREFIX_WIDTH_VALUE_Q << QMI_M0_WFMT_PREFIX_WIDTH_LSB |
        QMI_M0_WFMT_ADDR_WIDTH_VALUE_Q << QMI_M0_WFMT_ADDR_WIDTH_LSB |
        QMI_M0_WFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M0_WFMT_SUFFIX_WIDTH_LSB |
        QMI_M0_WFMT_DUMMY_WIDTH_VALUE_Q << QMI_M0_WFMT_DUMMY_WIDTH_LSB |
        QMI_M0_WFMT_DATA_WIDTH_VALUE_Q << QMI_M0_WFMT_DATA_WIDTH_LSB |
        QMI_M0_WFMT_PREFIX_LEN_VALUE_8 << QMI_M0_WFMT_PREFIX_LEN_LSB;
    
    qmi_hw->m[1].wcmd = 0x38; // Команда записи (Quad Page Program)
    
    // 9. Разрешение записи в PSRAM через XIP
    hw_set_bits(&xip_ctrl_hw->ctrl, XIP_CTRL_WRITABLE_M1_BITS);
    
    // 10. Устанавливаем флаги успешной инициализации

    psram_avaiable = 1;
    
    // 11. Определяем размер PSRAM
    size_psram = get_psram_size();
}
#endif
//######################################################################################
int fast(main)(void){  

    mute =0x01; // отключение звука

// настройка и разгон для RP2350/ RP2040
#if !PICO_RP2040    
    //RP2350 
  //  volatile uint32_t *qmi_m0_timing=(uint32_t *)0x400d000c;
    vreg_disable_voltage_limit();

    vreg_set_voltage(VREG_VOLTAGE_1_30);
 //   vreg_set_voltage(VREG_VOLTAGE_1_40);

    sleep_ms(50);
/*     *qmi_m0_timing = 0x60007204;
    set_sys_clock_khz(CPU_MHZ * KHZ, 0);
    *qmi_m0_timing = 0x60007303; */


    qmi_hw->m[0].timing = 0x60007204;
    set_sys_clock_khz(CPU_MHZ * 1000, 0);
    set_flash_timings();  // Настройка таймингов для новой частоты


#else 
   // PICO_RP2040
   // hw_set_bits(&vreg_and_chip_reset_hw->vreg, VREG_AND_CHIP_RESET_VREG_VSEL_BITS);
   // sleep_ms(10);
   // set_sys_clock_khz(CPU_MHZ * KHZ, true);
   vreg_set_voltage(VREG_VOLTAGE_1_30);
   sleep_ms(500);
   set_sys_clock_khz(CPU_KHZ, false);
#endif

	//stdio_init_all();//Активирует интерфейс для stdio
    // Явный сброс функции GPIO0 и GPIO1
    //gpio_set_function(0, GPIO_FUNC_SIO);
    //gpio_set_function(1, GPIO_FUNC_SIO);

// Настройка GPIO все на вход и сброс в SIO

  
#if  PI_CARD // RP2350B
   for (int gpio = 0; gpio < 48; gpio++) {
    gpio_init(gpio);          // Сброс в SIO, вход
    gpio_disable_pulls(gpio); // Отключить подтяжки (по умолчанию)
    gpio_set_dir(gpio, GPIO_IN); // Направление: вход
   }
    pio_set_gpio_base(PIO_PS2, 16);//использование GPIO 16...48

        gpio_init(40);
        gpio_set_dir(40, GPIO_OUT);
        gpio_put(41,1);

        gpio_init(41);
        gpio_set_dir(41, GPIO_OUT);
        gpio_put(41,1);   
//
#else // RP2040 или RP2350A   установка всех GPIO НА ВХОД
   for (int gpio = 0; gpio < 30; gpio++)
   {
    //   gpio_init(gpio);             // Сброс в SIO, вход
    //   gpio_disable_pulls(gpio);    // Отключить подтяжки (по умолчанию)
     //  gpio_set_dir(gpio, GPIO_IN); // Направление: вход
   }
#endif

    // Инициализация последовательного порта
//   stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
//	g_delay_ms(100);
    gpio_put(LED_PIN, 0);

     #if (PBUS_CS != 255) 
     inInit(PBUS_CS);// на вход
     #endif
     inInit(BEEP_PIN);// на вход  для реалиации звука бипера




 // Ожидание подключения терминала (если включено)
 //  while(!stdio_usb_connected()) { }
  

//---------------------------------------------------------
// INT generator 37500Hz
/*  	repeating_timer_t int_timer; 
	if (!add_repeating_timer_us(-1000000 / INT_FREQ, zx_int, NULL, &int_timer)) {
		//G_PRINTF_ERROR("Failed to add INT timer\n");
        gpio_put(25,1);//error
		return 1;
	}   
    */
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//----------------- PSRAM -------------------
psram_avaiable=0;
#if PICO_RP2350 // если rp23550 и psram бутерброд 
          init_psram_butter(PSRAM_BUTTER_PIN_CS);
#endif

//--------------------------------------------
// GS 
     audio_init();
    gpio_put(LED_PIN, 1);
    // Первичная инициализация picobus
    init_picobus();


 #if defined Z_CONTROLER   
     // инициаизация SD
    gpio_set_function(SDCARD_PIN_SPI0_MISO, GPIO_FUNC_SPI);
    gpio_set_function(SDCARD_PIN_SPI0_MOSI, GPIO_FUNC_SPI);
    gpio_set_pulls(SDCARD_PIN_SPI0_MOSI,true,false);
    gpio_set_pulls(SDCARD_PIN_SPI0_MISO,true,false);
    gpio_set_function(SDCARD_PIN_SPI0_SCK, GPIO_FUNC_SPI);
    out_init(SDCARD_PIN_SPI0_CS,1);
     spi_init(SDCARD_SPI_BUS, SDCARD_SPI_BUS_FREQ_CLK);

	/* SPI0 parameter config */
	spi_set_format(SDCARD_SPI_BUS,
		8, /* data_bits */
		SPI_CPOL_0, /* cpol */
		SPI_CPHA_0, /* cpha */
		SPI_MSB_FIRST /* order */
	);

 #endif

      #if defined(RTC_NOVA) || defined(RTC_SMUC)
      rtc_ds1287_init();
      #endif

  gpio_put(LED_PIN, 0);

//------------------------------------------------------------------

mute = 0x00; // включение звука

//==================================================================================
	multicore_launch_core1(ZXThread);
   //   основной цикл
//------------------------------------------------------
 #define AY_SAMPLE_RATE (9)//(MHZ/I2S_FREQ) 1 Mhz / 111111 Hz = 9
// #define AY_SAMPLE_RATE (22)//(MHZ/I2S_FREQ) 1 Mhz / 44100 Hz = 22.675

//uint64_t int_tick=time_us_64()+AY_SAMPLE_RATE;//Устанавливает время следующего прерывания на 26/2 микросекунд в будущем
uint64_t int_tick=time_us_64()+ AY_SAMPLE_RATE;//Устанавливает время следующего прерывания на (MHZ/I2S_FREQ) микросекунд в будущем
    while (1)
    {
       uint64_t tick_time = time_us_64();
    if (tick_time>=int_tick)//Проверяет, наступило ли время для audio out TS.
    {   

         audio_out_i2s_ts();

         int_tick=tick_time+ AY_SAMPLE_RATE;// Следующее прерывание через AY_SAMPLE_RATE=9 мкс ~ 111111Hz

       GS_get_sound_LR_sample(); 

       // Собираем MIDI-сэмпл
       // 111111 Hz / 22050 Hz MIDI = 5.039   x = 5 
       // 75000 Hz  / 22050 HZ MIDI = 3.401   x = 3 
       // 44100 Hz  / 22050 HZ MIDI = 2       x = 2
             #ifdef MIDI
             static uint8_t x = 0;
             if (x==5)
             {
             x=0;   
             midi_sample();
             }
             x++;
             #else
        //    midi_sound= = 0;
            #endif





            }
    else 
    {
    #if (PBUS_CS==255) 
     picobus_read_write();// мониторинг шины picobus
    #else 
    if (!gpio_get(PBUS_CS)) picobus_read_write();// мониторинг шины picobus
    #endif 
    }
 //   busy_wait_us(1);
      } // while(1)
            
   ;
}

