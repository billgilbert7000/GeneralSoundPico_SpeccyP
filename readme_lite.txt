GSP v0.4.5 lite version без PSRAM 
на RP2040 (PICO1) без платы 

Эмулятор General Sound Pico (GSP).
+ SpeccyP 1.4.5 GSlite для него.

GeneralSound доступно 112 Kb (PSRAM не используется)
Z80 GS работает на эмулирукмой частоте 16MHz

Возможно использование если на плате мурмулятора
 есть с i2s звук (TDA) m1 v1.4 или подобные

Эмулируются:
# GeneralSound;
# TurboSound (вместо TS на основной плате);



Для Мурмулятора ПЕРВОЙ ревизии (MURM1)

Основная плата (MAIN) с прошивкой:
    SpeccyP_1.4.5_GSlite_m1p1.uf2 (RP2040) 
или SpeccyP_1.4.5_GSlite_m1p2.uf2 (RP2350)

голыйй модуль RP2040 подключается к основной плате 
прямо к 2x20 разъему на плате v1.4 с i2s звуком
Прошивка: GSP-LITE_1.4.5_m1p1.uf2

1.MAIN          2. RP2040 GS
GPIO 26         GPIO 15 
GPIO 27         GPIO 14
GPIO 28         GPIO 16
I2S LRCK        GPIO 28
I2S BCK         GPIO 27
I2S DIN         GPIO 26
+5V             +5V
RUN             RUN
GND             GND

перемычки на звук с платы 26,27,28 должны быть убраны!

Соответственно подать питание +5V
 от одного источника 
я от USB компа всё пока включаю ;)