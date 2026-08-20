#pragma once
#ifndef _RTC_H_
#define _RTC_H_

extern uint8_t rtc_registr[0x80];
extern uint8_t rtc_adress;
extern bool rtc_adress_data;
extern bool rtc_enable; 

void rtc_ds1287_init(void);
uint8_t rtc_read_registr(uint8_t registr);
uint8_t rtc_read_registr_nova(uint8_t registr);
void rtc_write_registr(uint8_t adress_reg, uint8_t value);
void rtc_get_datetime_str(char *buffer, size_t buffer_size);
void rtc_get_time_str(char *buffer, size_t buffer_size);
void rtc_get_time_bin(uint8_t *buffer, size_t buffer_size);

// Адреса регистров DS1307
#define DS1307_SEC       0x00
#define DS1307_MIN       0x01
#define DS1307_HOURS     0x02
#define DS1307_DOTW      0x03  // День недели (1-7, 1=воскресенье)
#define DS1307_DATE      0x04
#define DS1307_MONTH     0x05
#define DS1307_YEAR      0x06
#define DS1307_USR       0x07

// Адреса регистров DS1287
#define DS1287_SEC          0x00
#define DS1287_ALARM_SEC    0x01
#define DS1287_MIN          0x02
#define DS1287_ALARM_MIN    0x03
#define DS1287_HOUR         0x04
#define DS1287_ALARM_HOUR   0x05
#define DS1287_DOTW         0x06
#define DS1287_DATE         0x07
#define DS1287_MONTH        0x08
#define DS1287_YEAR         0x09
#define DS1287_A            0x0A
#define DS1287_B            0x0B
#endif

