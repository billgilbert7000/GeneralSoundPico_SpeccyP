
#if defined(RTC_NOVA) || defined(RTC_SMUC)

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
Преобразование BCD   DS1307 хранит время в BCD формате
День недели   DS1307 использует 1-7 (1=воскресенье), 
              DS1287 использует 0-6 (0=воскресенье)
Год - DS1307 хранит двухзначный год, добавляем 2000
*/

uint8_t rtc_registr[0x80];
uint8_t rtc_adress =0xff;
bool rtc_adress_data;
bool rtc_enable; 

//    Настройки для DS1287
#define DS1307_I2C_ADDR 0x68
#define I2C_PORT i2c1
#define I2C_SDA_PIN 2
#define I2C_SCL_PIN 3
//#####################################################################################
// Макросы для преобразования BCD
#define bin2bcd(x) (((x) / 10) << 4 | (x) % 10)
#define bcd2bin(x) (((x) >> 4) * 10 + ((x) & 0x0F))
//######################################################################################
// Глобальные переменные
uint64_t system_base_time = 0;

// МАССИВ РЕГИСТРОВ RTC
uint8_t ds1287_BIN[0x7f] = {0}; // Преобразованные BIN значения

uint8_t last_hour = 255;// нужно дя определения смены суток
//#######################################################################################
// Инициализация I2C для DS1307
void DS1307_i2c_init(void) {
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    i2c_init(I2C_PORT, 100000);  // 1000 kHz
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
    uint8_t time_data[128];
    time_data[0] = 0;

    // Читаем регистры  времени из DS1307
    if (i2c_write_blocking(I2C_PORT, DS1307_I2C_ADDR, time_data, 1, true) !=1) 
    {
        gpio_put(LED_PIN, 1);
       return false;
    }   
   if 
    (i2c_read_blocking(I2C_PORT, DS1307_I2C_ADDR, time_data, 16, false) != 16)
     {
        gpio_put(LED_PIN, 1);
        return false;
    }

    ds1287_BIN[DS1287_A] = time_data[0x10+10];
    // Преобразуем BCD в binary и записываем в массив ds1287_BIN
    ds1287_BIN[0] = bcd2bin(time_data[0] & 0x7F); // секунды
    ds1287_BIN[2] = bcd2bin(time_data[1] & 0x7F); // минуты
    ds1287_BIN[4] = bcd2bin(time_data[2] & 0x3F); // часы
    ds1287_BIN[6] = time_data[3] & 0x07;          // день недели
    ds1287_BIN[7] = bcd2bin(time_data[4] & 0x3F); // число
    ds1287_BIN[8] = bcd2bin(time_data[5] & 0x1F); // месяц
    ds1287_BIN[9] = bcd2bin(time_data[6]);        // год
    


    // Вычисляем Unix время из данных RTC (используем BIN значения)
    uint64_t rtc_unix = calculate_unix_from_DS1307(time_data);
    
    // Вычисляем base_time
    uint64_t system_time = time_us_64() / 1000000;
    system_base_time = rtc_unix - system_time;   

    last_hour = ds1287_BIN[4];// нужно для определения смены суток
    
    return true;
}
//#######################################################################
// Получение актуального времени из Unix времени
void update_time_from_unix(void) {
    // Вычисляем текущее Unix время
    uint64_t seconds_total = (time_us_64() / 1000000) + system_base_time;
    
    // Вычисляем время: секунды, минуты, часы
    // Обновляем только время в массивах
    ds1287_BIN[DS1287_SEC] = seconds_total % 60;//секунды
    seconds_total /= 60;
    ds1287_BIN[DS1287_MIN] = seconds_total % 60;//минуты
    seconds_total /= 60;
    ds1287_BIN[DS1287_HOUR] = seconds_total % 24;//часы
    
    // Проверяем смену суток (23 -> 00)
    if (last_hour == 23 && ds1287_BIN[4] == 0) {
        // Читаем новую дату из RTC
        read_DS1307_and_calc_base();
    }
    last_hour = ds1287_BIN[4];
}
//######################################################
// Основные функции для получения времени
uint8_t* get_current_time_bin(void) {
    update_time_from_unix();
    return ds1287_BIN;
}
//######################################################

//######################################################################################
// получение даты/времени из регистров DS1287 
uint8_t rtc_read_registr(uint8_t registr)
{
   if (!rtc_enable) return 0xff;

    update_time_from_unix(); 



   return ds1287_BIN[registr];
}   
//#######################################################################################
   // у DS1307 пользовательских регистров нет только 0x00 до 0x08
   // ЗДЕСЬ ДОЛЖНА БЫТЬ ПРОЦЕДУРА ЗАПИСИ РЕГИСТРОВ В ЭНЕРГОНЕЗАВИСИМУОЙ ПАМЯТЬ!
   // ИЛИ В FLASH PICO 
// запись даты/времени в регистры DS1287  и DS1307 
void rtc_write_registr(uint8_t adress_reg, uint8_t value)
{   
    if (!rtc_enable) return;
    if (adress_reg>0x3f) return; // регистр больше чем есть у DS1287
    ds1287_BIN[adress_reg] = value;// запись в виртуальный регистр DS1287
    uint8_t x=0xff; 
    switch (adress_reg)
    {  
    case DS1287_SEC   : x = DS1307_SEC;     break;
    case DS1287_MIN   : x = DS1307_MIN;     break;
    case DS1287_HOUR  : x = DS1307_HOURS;   break;
    case DS1287_DOTW  : x = DS1307_DOTW;    break;    
    case DS1287_DATE  : x = DS1307_DATE;    break;  
    case DS1287_MONTH : x = DS1307_MONTH;   break;  
    case DS1287_YEAR  : x = DS1307_YEAR;    break; 
    }
    if (x==0x3f) return;
    uint8_t data[2];
    data[0] = x;
    if (x > 6) data[1] = value;
    else       data[1] = bin2bcd(value);
    i2c_write_blocking(I2C_PORT, DS1307_I2C_ADDR, data, 2, false);
    read_DS1307_and_calc_base();// Чтение времени и даты из DS1307 и вычисление base_time
}
//#######################################################################################
// Инициализация эмулятора DS1287
void rtc_ds1287_init(void)
{ 
   DS1307_i2c_init(); // Инициализация I2C для DS1307

  g_delay_ms(100);
   rtc_enable = read_DS1307_and_calc_base();// Чтение времени и даты из DS1307 и вычисление base_time

  
   // у DS1307 пользовательских регистров нет только 0x00 до 0x12
   // ЗДЕСЬ ДОЛЖНА БЫТЬ ПРОЦЕДУРА ЧТЕНИЯ РЕГИСТРОВ ИЗ ЭНЕРГОНЕЗАВИСИМУОЙ ПАМЯТИ!
   // ИЛИ ИЗ FLASH PICO read_pico_flash
   //read_pico_flash(куда считывать данные rtc_registr  0x0A , длина 0x7F-x0A ); 
} 
//########################################################################################
#endif