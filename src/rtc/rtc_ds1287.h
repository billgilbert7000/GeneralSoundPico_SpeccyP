#pragma once
#ifndef _RTC_H_
#define _RTC_H_

extern uint8_t rtc_registr[0x80];
extern uint8_t rtc_adress;
extern bool rtc_adress_data;
extern bool rtc_enable; 

void rtc_ds1287_init(void);
uint8_t rtc_read_registr(uint8_t registr);
void rtc_write_registr(uint8_t adress_reg, uint8_t value);

/*
Структура памяти DS3231:
1. Регистры времени (заняты) - 0x00-0x13:
0x00-0x06 - время, дата, контрольные регистры

0x07-0x0D - регистры будильников 1 и 2

0x0E-0x13 - контроль, статус, температура

2. Свободная пользовательская память - 0x14-0xFF:
Адреса 0x14-0xFF - 236 байт доступной памяти

Из них: 0x14-0x7F (108 байт) + 0x80-0xFF (128 байт)
*/
// Адреса пользовательской памяти DS3231
#define DS3231_REG_END   0x12
// Адреса регистров DS3231
#define DS3231_SEC       0x00
#define DS3231_MIN       0x01
#define DS3231_HOURS     0x02
#define DS3231_DOTW      0x03  // День недели (1-7, 1=воскресенье)
#define DS3231_DATE      0x04
#define DS3231_MONTH     0x05
#define DS3231_YEAR      0x06

#define DS3231_REG_ALARM1_SEC  0x07
#define DS3231_REG_ALARM1_MIN  0x08
#define DS3231_REG_ALARM1_HOUR 0x09
#define DS3231_REG_ALARM1_DAY  0x0A
#define DS3231_REG_ALARM2_MIN  0x0B
#define DS3231_REG_ALARM2_HOUR 0x0C
#define DS3231_REG_ALARM2_DAY  0x0D
#define DS3231_REG_CONTROL     0x0E
#define DS3231_REG_STATUS      0x0F
#define DS3231_REG_AGING_OFFSET 0x10

#define DS3231_REG_TEMP_MSB    0x11  // Температура 0x11 и 0x12
#define DS3231_REG_TEMP_LSB    0x12

// Адреса регистров DS1287
#define DS1287_SEC       0x00
#define DS1287_ALARM_SEC   0x01
#define DS1287_MIN       0x02
#define DS1287_ALARM_MIN   0x03
#define DS1287_HOUR      0x04
#define DS1287_ALARM_HOUR 0x05
#define DS1287_DOTW      0x06
#define DS1287_DATE      0x07
#define DS1287_MONTH     0x08
#define DS1287_YEAR      0x09
#define DS1287_A         0x0A



#endif

