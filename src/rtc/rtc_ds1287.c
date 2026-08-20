
#if defined(RTC_NOVA) || defined(RTC_SMUC) || defined(RTC_GLUK)

#include "config.h"  
#include "stdbool.h"
#include "hw_util.h"

#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "inttypes.h"
#include "hardware/pio.h"
#include <string.h>
//#include "pico/time.h"

#include "hardware/i2c.h"
#include "rtc/rtc_ds1287.h"
/*
Вот мой код по эмуляции RTC на DS1307 
работает на RP20040 и RP2350A/B
Обязан собираться под всем sdk rp,ардуино, platformio
чтение и запись DS1307  i2c
 НЕиспользуются функции SDK Rasberry Pico
 НЕ использует "hardware/rtc.h" 
 чтение DS1307 происходит только при старте и при смене суток 
 ВСЁ остальное  время считывается средствами rtc pico
 нет постоянного обращения через i2c при чтении даты и времени
 запись даты времени и остальных регистров сразу по i2c 
 дата и день недели НЕВЫЧИСЛЯЮТСЯ а беруться из DS1307 и обновляются при смене суток
 код рабочий проверенно на кошках

 в принципе полностью эмулирует DS1287 только с регистром будильника надо разобраться 
 как правиьно туда записывать и нужно ли преобразование

  // у DS1307 пользовательских регистров нет только 0x00 до 0x12
  // чтение запись доп регистров DS1287 необходимо имитировать они только записываются/считываются из массива
  // необходимо их где то сохранять
  
*/

/*     пример вызова чтения записи в порты часов ZX Spectrum
      case RTC_READ_IN_PORT_CLOCK: rtc_read_registr(); value = rtc_registr[rtc_adress]; send_byte(value);break;
      case RTC_WRITE_OUT_CLOCK:  rtc_write_registr(rtc_adress,value); break;
      case RTC_WRITE_OUT_CLOCK_ADRESS: rtc_adress = value; break;
*/

/*
Преобразование BCD   DS1307 хранит время в BCD 
День недели   DS1307 использует 1-7 (1=воскресенье), 
              DS1287 использует 0-6 (0=воскресенье)
Год - DS1307 хранит двухзначный год, добавляем 2000
*/
/*
Микросхема DS1307 имеет 8 регистров (адреса 0x00–0x07) и 56 байт пользовательской памяти (адреса 0x08–0x3F) .

Регистры RTC (адреса 0x00–0x07)
Первые 8 байт адресного пространства зарезервированы для служебных регистров, управляющих часами и календарем .

Адреса 0x00–0x06: Содержат время и дату в двоично-десятичном формате (BCD): секунды, минуты, часы, день недели, число, месяц, год .

Адрес 0x07: Это управляющий регистр (Control Register).  выходной сигнал на выводе SQW/OUT 
(включение/выключение генератора и выбор его частоты) 
*/
/*
регистр B (адрес 0x0B) микросхемы DS1287 :
| SET | PIE | AIE | UIE | SQWE | DM | 24/12 | DSE |

Бит 2: DM (Data Mode — Формат данных)
Определяет, в каком виде вы будете читать и записывать данные времени.

Значение 0 (BCD): Все регистры времени (секунды, минуты, часы и т.д.) используют двоично-десятичный код. 
Каждая десятичная цифра хранится в своей тетраде (полубайте). Например, число 45 будет записано как 0100 0101 (что в HEX = 0x45).

Значение 1 (BINARY): Все регистры времени используют чистый двоичный код.
 Например, число 45 будет записано просто как 0010 1101 (что в HEX = 0x2D).

Важное последствие: Этот бит влияет на все регистры с 0x00 по 0x09. 
Если вы установите DM=1, то в регистрах часов старший бит 24/12 перестает иметь значение (для 24-часового формата), 
а в регистре года диапазон становится 0–127. Поэтому формат нужно выбрать один раз при инициализации и больше не менять, и
наче время превратится в "кашу".


Бит 1: 24/12 (12/24 Hour Mode — Формат вывода часов)
Определяет, как интерпретировать значение в регистре часов (адрес 0x04).

Значение 0 (12-часовой): Регистр часов хранит время в 12-часовом формате. 
При этом бит 7 в регистре часов (адрес 0x04) служит флагом AM/PM: 0 = AM (до полудня), 1 = PM (после полудня).
 Диапазон значений: 01–12.

Значение 1 (24-часовой): Регистр часов хранит время в 24-часовом формате. Диапазон значений: 00–23. Бит 7 в этом случае становится частью числа, а не флагом AM/PM.


*/


uint8_t rtc_registr[0x80];
uint8_t rtc_adress =0xff;
bool rtc_adress_data;
bool rtc_enable; 

//#####################################################################################
// Макросы для преобразования BCD
#define bin2bcd(x) (((x) / 10) << 4 | (x) % 10)
#define bcd2bin(x) (((x) >> 4) * 10 + ((x) & 0x0F))
//######################################################################################
// Глобальные переменные
uint64_t system_base_time = 0;

// МАССИВ РЕГИСТРОВ RTC
uint8_t ds1287_reg[0x7f] = {0}; // Преобразованные BIN значения

uint8_t last_hour = 255;// нужно для определения смены суток
//#######################################################################################
// Инициализация I2C для DS1307
void DS1307_i2c_init(void) {
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    i2c_init(I2C_PORT, 100*1000);  // 10 kHz
}
//########################################################################
// Вычисление Unix времени из данных DS1307
uint64_t calculate_unix_from_DS1307(const uint8_t *time_data) {
    uint8_t seconds =   bcd2bin(time_data[0] & 0x7F);
    uint8_t minutes =   bcd2bin(time_data[1] & 0x7F);
    uint8_t hours =     bcd2bin(time_data[2] & 0x3F);
    uint8_t day =       bcd2bin(time_data[4] & 0x3F);
    uint8_t month =     bcd2bin(time_data[5] & 0x1F);
    uint16_t year =     bcd2bin(time_data[6]) + 2000;
    
    // Вычисляем Unix время
    static const uint8_t days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    uint64_t total_days = 0;
    
    for (uint16_t y = 1970; y < year; y++) {
        total_days += ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) ? 366 : 365;
    }
    
    for (uint8_t m = 1; m < month; m++) {
        total_days += days_in_month[m - 1];
        if (m == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
            total_days += 1;
        }
    }
    
    total_days += (day - 1);
    
    uint64_t unix_time = total_days * 86400;
    unix_time += hours * 3600;
    unix_time += minutes * 60;
    unix_time += seconds;
    
    return unix_time;
}
//######################################################################################
// Чтение времени и даты из DS1307 и вычисление base_time
bool read_DS1307_and_calc_base(void) {
    uint8_t reg = 0x00;
    uint8_t time_data[64];
    time_data[0] = 0;
    // Читаем регистры  времени из DS1307
    if (i2c_write_blocking(I2C_PORT, DS1307_I2C_ADDR, time_data, 1, true) !=1) 
    {
        gpio_put(LED_PIN, 1);
       return false;
    }   
   if 
    (i2c_read_blocking(I2C_PORT, DS1307_I2C_ADDR, time_data, 60, false) != 60) // чтение 64 регистров 
     {
        gpio_put(LED_PIN, 1);
        return false;
    }

    // Преобразуем BCD в binary и записываем в массив ds1287_reg
    ds1287_reg[0] = bcd2bin(time_data[0] & 0x7F); // секунды
    ds1287_reg[2] = bcd2bin(time_data[1] & 0x7F); // минуты
    ds1287_reg[4] = bcd2bin(time_data[2] & 0x3F); // часы
    ds1287_reg[6] = time_data[3] & 0x07;          // день недели
    ds1287_reg[7] = bcd2bin(time_data[4] & 0x3F); // число
    ds1287_reg[8] = bcd2bin(time_data[5] & 0x1F); // месяц
    ds1287_reg[9] = bcd2bin(time_data[6]);        // год

/*     ds1287_reg[DS1287_ALARM_SEC] = bcd2bin(time_data[7] & 0x7F); // секунды alarm
    ds1287_reg[DS1287_ALARM_MIN] = bcd2bin(time_data[8] & 0x7F); // минуты alarm
    ds1287_reg[DS1287_ALARM_HOUR] = bcd2bin(time_data[9] & 0x7F); // минуты alarm */

    ds1287_reg[DS1287_ALARM_SEC]  = time_data[7]; // секунды alarm
    ds1287_reg[DS1287_ALARM_MIN]  = time_data[8]; // минуты alarm
    ds1287_reg[DS1287_ALARM_HOUR] = time_data[9]; // минуты alarm


    // Вычисляем Unix время из данных RTC (используем BIN значения)
    uint64_t rtc_unix = calculate_unix_from_DS1307(time_data);
    
    // Вычисляем base_time
    uint64_t system_time = time_us_64() / 1000000;
    system_base_time = rtc_unix - system_time;   

    last_hour = ds1287_reg[4];// нужно для определения смены суток
    strncpy(ds1287_reg+10, time_data+10, 53);
    return true;
}
//#######################################################################
// Получение актуального времени из Unix времени
void update_time_from_unix(void) {
    // Вычисляем текущее Unix время
    uint64_t seconds_total = (time_us_64() / 1000000) + system_base_time;
    
    // Вычисляем время: секунды, минуты, часы
    // Обновляем только время в массивах
    ds1287_reg[DS1287_SEC] = seconds_total % 60;//секунды
    seconds_total /= 60;
    ds1287_reg[DS1287_MIN] = seconds_total % 60;//минуты
    seconds_total /= 60;
    ds1287_reg[DS1287_HOUR] = seconds_total % 24;//часы
    
    // Проверяем смену суток (23 -> 00)
    if (last_hour == 23 && ds1287_reg[DS1287_HOUR] == 0) {
        // Читаем новую дату из RTC
        read_DS1307_and_calc_base();
    }
    last_hour = ds1287_reg[4];
}
//######################################################
// Основные функции для получения времени
uint8_t* get_current_time_bin(void) {
    update_time_from_unix();
    return ds1287_reg;
}
// ######################################################
//  получение даты/времени из регистров DS1287
uint8_t rtc_read_registr_nova(uint8_t registr)
{
    if (!rtc_enable)
        return 0xff;

     if (registr == 0x01)
        return 0xAA;  

    update_time_from_unix();
    return ds1287_reg[registr];
}
// ######################################################
//  получение даты/времени из регистров DS1287
uint8_t rtc_read_registr(uint8_t registr)
{
    if (!rtc_enable)
        return 0xff;

    if (registr == 0x0A)
        return 0x20; // GLUCK RTC //  Регистр A (адрес 0x0A) — Настройка генератора
    if (registr == 0x0B)
        return 0x02; // GLUCK RTC // Регистр B Бит 2: DM (Data Mode — Формат данных)
    if (registr == 0x11)
        return 0xAA; // GLUCK RTC всегда 0xAA
    update_time_from_unix();
    if (registr > 9)
        return ds1287_reg[registr];

    // if (ds1287_reg[DS1287_B] & (1 << 2)) return (ds1287_reg[registr]);// Регистр B Бит 2: DM (Data Mode — Формат данных)

    return bin2bcd(ds1287_reg[registr]); // GLUCK RTC
}
//#############################################################################
   // у DS1307  только 0x00 до 0x3F
   // ЗДЕСЬ ДОЛЖНА БЫТЬ ПРОЦЕДУРА ЗАПИСИ РЕГИСТРОВ В ЭНЕРГОНЕЗАВИСИМУОЙ ПАМЯТЬ!
   // ИЛИ В FLASH PICO 
   // ds1287_reg[128]  виртуальные регистры DS1287
// запись даты/времени в регистры DS1287  и DS1307 
void rtc_write_registr(uint8_t adress_reg, uint8_t value)
{   
    if (!rtc_enable) return;
    if (adress_reg>0x3f) return; // регистр больше 0x3f
    ds1287_reg[adress_reg] = value;// запись в виртуальный регистр DS1287
    uint8_t x=adress_reg; 
    switch (adress_reg)
    {  
    case DS1287_SEC         : x = DS1307_SEC;     break;
    case DS1287_ALARM_SEC   : x = DS1307_USR+0;   break;//7
    case DS1287_MIN         : x = DS1307_MIN;     break;
    case DS1287_ALARM_MIN   : x = DS1307_USR+1;   break;//8
    case DS1287_HOUR        : x = DS1307_HOURS;   break;
    case DS1287_ALARM_HOUR  : x = DS1307_USR+2;   break;//9
    case DS1287_DOTW        : x = DS1307_DOTW;    break;    
    case DS1287_DATE        : x = DS1307_DATE;    break; 
    case DS1287_MONTH       : x = DS1307_MONTH;   break; 
    case DS1287_YEAR        : x = DS1307_YEAR;    break;
    // остальные с 0x0A (10) совпвдают по числам
    }
 //   if (x==0x3f) return;
    uint8_t data[2];
    data[0] = x;
    if (x > 9) data[1] = value;
    else       
    {
     data[1] = bin2bcd(value);// 
    }

    i2c_write_blocking(I2C_PORT, DS1307_I2C_ADDR, data, 2, false);
    read_DS1307_and_calc_base();// Чтение времени и даты из DS1307 и вычисление base_time
}
//#######################################################################################
// Инициализация эмулятора DS1287
void rtc_ds1287_init(void)
{ 
   DS1307_i2c_init(); // Инициализация I2C для DS1307
 //  g_delay_ms(1); // ???
   rtc_enable = read_DS1307_and_calc_base();// Чтение времени и даты из DS1307 и вычисление base_time
} 
//#######################################################################################
// Формирование строки с датой и временем в формате "DD.MM.YYYY HH:MM:SS"
void rtc_get_datetime_str(char *buffer, size_t buffer_size) {
    if (!rtc_enable || buffer == NULL || buffer_size < 20) {
        if (buffer != NULL && buffer_size > 0) {
            buffer[0] = '\0';
        }
        return;
    }
    
    // Обновляем время из Unix
    update_time_from_unix();
    
    // Получаем значения из ds1287_reg
    uint8_t sec  = ds1287_reg[DS1287_SEC];
    uint8_t min  = ds1287_reg[DS1287_MIN];
    uint8_t hour = ds1287_reg[DS1287_HOUR];
    uint8_t day  = ds1287_reg[DS1287_DATE];
    uint8_t mon  = ds1287_reg[DS1287_MONTH];
    uint8_t year = ds1287_reg[DS1287_YEAR];
    
    // Формируем строку в формате "DD.MM.YYYY HH:MM:SS"
    snprintf(buffer, buffer_size, "%02d.%02d.%04d %02d:%02d:%02d",
             day, mon, 2000 + year, hour, min, sec);
}
//#######################################################################################
// Формирование строки с временем в формате "HH:MM:SS"
void rtc_get_time_str(char *buffer, size_t buffer_size) {
    if (!rtc_enable || buffer == NULL || buffer_size < 9) {
        if (buffer != NULL && buffer_size > 0) {
            buffer[0] = '\0';
        }
        return;
    }
    
    // Обновляем время из Unix
    update_time_from_unix();
    
    // Получаем значения из ds1287_reg
    uint8_t sec  = ds1287_reg[DS1287_SEC];
    uint8_t min  = ds1287_reg[DS1287_MIN];
    uint8_t hour = ds1287_reg[DS1287_HOUR];
    
    // Формируем строку в формате "HH:MM:SS"
    snprintf(buffer, buffer_size, "%02d:%02d:%02d",
              hour, min, sec);
}
//#######################################################################################
// Передача даты и времени
void rtc_get_time_bin(uint8_t *buffer, size_t buffer_size)
{
    if (!rtc_enable)
    {
        buffer[0] = 26;
        buffer[1] = 7;
        buffer[2] = 28;
        buffer[3] = 0;
        buffer[4] = 0;
        buffer[5] = 0;
        return;
    } 
    update_time_from_unix(); // Обновляем время из Unix
   // strncpy(buffer, ds1287_reg, buffer_size);

        buffer[0] = ds1287_reg[DS1287_YEAR] ;
        buffer[1] = ds1287_reg[DS1287_MONTH];
        buffer[2] = ds1287_reg[DS1287_DATE];
        buffer[3] = ds1287_reg[DS1287_HOUR] ;
        buffer[4] = ds1287_reg[DS1287_MIN];
        buffer[5] = ds1287_reg[DS1287_SEC];

}
//#######################################################################################

#endif