#include "config.h"  
#include "stdbool.h"
#include "hw_util.h"

#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "inttypes.h"
#include "hardware/pio.h"
#include <string.h>
#include "aySoft.h"
#include "audio_i2s.h"

// Глобальная переменная громкости

uint8_t current_volume = AY_VOLUME_PERCENT;
uint8_t audio_buster;
uint16_t beep_volume = 0;
bool mute;

 uint8_t beep_data_old;
 uint8_t beep_data;
 //uint16_t beepPWM;
 
 //uint8_t type_sound;
 uint8_t type_sound_out;
//регистры состояния AY0
 uint8_t reg_ay0[16];
 uint16_t ay0_R12_R11;
 uint16_t ay0_A_freq;
 uint16_t ay0_B_freq;
 uint16_t ay0_C_freq;

// регистры состояния AY1
 uint8_t reg_ay1[16];
 uint16_t ay1_R12_R11;
 uint16_t ay1_A_freq;
 uint16_t ay1_B_freq;
 uint16_t ay1_C_freq;
 uint8_t maskAY[16] = {0xff,0x0f,0xff,0x0f,0xff,0x0f ,0x1f, 0xff, 0x1f,0x1f,0x1f, 0xff , 0xff, 0x0f,0xff,0xff};
 //const uint8_t maskAY[16] = {0xff,0x0f,0xff,0x0f,0xff,0x0f ,0x1f, 0xff, 0x1f,0x1f,0x1f, 0xff , 0xff, 0x0f,0xff,0xff};
 //const uint8_t maskAY_FM[16] = {0xff,0x1f,0xff,0x1f,0xff,0x1f ,0x1f, 0xff, 0x1f,0x1f,0x1f, 0xff , 0xff, 0x0f,0xff,0xff};

    uint16_t *AY_data;
	uint16_t *AY_data1;

     uint8_t N_sel_reg;
     uint8_t N_sel_reg_1;
    

bool chip_ay;


uint16_t ampls_AY[32];
// В RAM для скорости 
//const uint8_t ampls_AY_table[16] __attribute__((section(".data"))) = {0,1,1,1,2,2,3,5,6,9,13,17,22,29,36,45};/*AY_TABLE*/

const uint8_t const_ampls_AY_table[16] __attribute__((section(".data"))) = {0,1,1,1,2,2,3,5,6,9,13,17,22,29,36,45};/*AY_TABLE*/

uint16_t ampls_AY_table[16];/*AY_TABLE*/

/* const uint8_t ampls[7][16] =
{
   {0,1,1,1,2,2,3,5,6,9,13,17,22,29,36,45},/*AY_TABLE
   {0,1,2,3,4,5,6,8,10,12,16,19,23,28,34,40},/*снятя с китайского чипа
   {0,3,5,8,11,13,16,19,21,24,27,29,32,35,37,40},/*линейная
   {0,10,15,20,23,27,29,31,32,33,35,36,38,39,40,40},/*выгнутая
   {0,1,2,2,3,3,4,6,7,9,12,15,19,25,32,41},/*степенная зависимость
   {0,2,4,5,7,8,9,11,12,14,16,19,23,28,34,40},/*гибрид линейной и китайского чипа
   {0,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31},/*Rampa_AY_table
}; */
// В RAM для скорости 
const uint8_t Envelopes_const[16][32] __attribute__((section(".data"))) =
{
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*0*/
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*1*/
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*2*/
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*3*/
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*4*/
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*5*/
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*6*/
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*7*/
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0 },/*8*/
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*9*/
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 }, /*10 0x0a */
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15 },/*11  0x0b */
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 },/*12  0x0c*/
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15 },/*13  0x0d*/
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0 },/*14  0x0e*/
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }/*15 0x0d*/
};  
 //----------------------------------   
//uint8_t N_sel_reg=0;
//uint8_t N_sel_reg_1=0;
/*
N регистра	Назначение или содержание	Значение	
0, 2, 4 	Нижние 8 бит частоты тона А, В, С 	0 - 255
1, 3, 5 	Верхние 4 бита частоты тона А, В, С 	0 - 15
6 	        Управление частотой генератора шума 	0 - 31
7 	        Управление смесителем и вводом/выводом 	0 - 255
8, 9, 10 	Управление амплитудой каналов А, В, С 	0 - 15
11 	        Нижние 8 бит управления периодом пакета 	0 - 255
12 	        Верхние 8 бит управления периодом пакета 	0 - 255
13 	        Выбор формы волнового пакета 	0 - 15
14, 15 	    Регистры портов ввода/вывода 	0 - 255

R7

  7 	  6 	  5 	  4 	  3 	  2 	  1 	  0
порт В	порт А	шум С	шум В	шум А	тон С	тон В	тон А
управление
вводом/выводом	выбор канала для шума	выбор канала для тона
*/

/* GenNoise (c) Hacker KAY & Sergey Bulba */

/* bool fast (get_random)()
{
    static uint32_t Cur_Seed = 0xffff;
    Cur_Seed = (Cur_Seed * 2 + 1) ^
               (((Cur_Seed >> 16) ^ (Cur_Seed >> 13)) & 1);
    if ((Cur_Seed >> 16) & 1)
        return true;
    return false;
} */
/* End of GenNoise (c) Hacker KAY & Sergey Bulba */
//############################################################################
//Оптимизированный 32-битный LFSR time_us_32(); // Используем время как seed
uint32_t __not_in_flash_func(noise_state) = 0xACE1u;

bool fast (get_random)()
{
    // Качественный 32-битный LFSR с периодом ~4 миллиарда
    noise_state = (noise_state >> 1) ^ (-(noise_state & 1u) & 0xD0000001u);
    return noise_state & 0x80000000; // Старший бит для лучшей случайности
}
//###########################################################################
//---------------------------------------------------------------- 
 //  void fast(AY_out_FFFD)(uint8_t N_reg) 
//----------------------------------------------------------------
void fast(ay_set_reg_soft)(uint8_t N_reg) 
{ 
     N_sel_reg=N_reg; 
}
//----------------------------------------------------------------
void fast (AY_out_FFFD)(uint8_t N_reg) 
{
    if (N_reg == 0xff) {chip_ay = 0;return;} 
    if (N_reg == 0xfe) {chip_ay = 1;return;}
    if (chip_ay==0) {N_sel_reg=N_reg;} // если 0 то чип 0
    else N_sel_reg_1=N_reg; // если 1 то регистр чипа 1
}
//================================================================
     void AY_reset()
     {
             memset(reg_ay0, 0, 16);
             reg_ay0[14] = 0xff;
             memset(reg_ay1, 0, 16);
             reg_ay1[14] = 0xff;
     };
//----------------------------------------------------------
/*
loop: in f,(c) //bc=0xfffd
      jp m, loop // если bit 7 == 1
*/
uint8_t  fast (AY_in_FFFD)()
{ 
   if (chip_ay==0) {return AY_get_reg();}  // если 0 то чип 0
  else {return AY_get_reg1();}  // если 1 то регистр чип 1 
}

/*******************************************/
//void fast(AY_out_BFFD)(uint8_t val)
/*******************************************/

//--------------------------------------------
void fast(AY_out_BFFD)(uint8_t val)
{   
    if (chip_ay==0) {AY_set_reg(val); return;} // если 0 то чипа 1
    else{ AY_set_reg1(val);  return;}// если 1 то регистр чипа 0   
}
//=================================================================
//===================================================================
//                  CHIP AY 0
//===================================================================
static uint32_t noise_ay0_count=0;
static bool is_envelope_begin=false;
static uint8_t ampl_ENV = 0;
static uint8_t envelope_ay_count = 0;

// чтение из регистров AY
uint8_t fast(AY_get_reg)()
{
    return reg_ay0[N_sel_reg]; // & 0x0f ?
}
//-----------------------------------------------------------------
// запись в регистры AY
void fast(AY_set_reg)(uint8_t val)
    {
        if (N_sel_reg>0x0f) return;// для TSFM
      //  if (conf.type_sound==TSFM) reg_ay0[N_sel_reg] = val & maskAY_FM[N_sel_reg];// для TSFM
      //  else
         reg_ay0[N_sel_reg] = val & maskAY[N_sel_reg];
        switch (N_sel_reg)
        {
        case 6:  // частота генератора шума 	0 - 31
            noise_ay0_count = 0;
            return;
        case 13://Выбор формы волнового пакета 	0 - 15
            is_envelope_begin = true;
            return;
        }

    }
//--------------------------------------------------------------------------
uint16_t*  fast(get_AY_Out)(uint8_t delta)
{   
    static bool bools[4];

    #define chA_bit (bools[0])
    #define chB_bit (bools[1])
    #define chC_bit (bools[2])
    #define noise_bit (bools[3])
  
    static uint32_t main_ay_count_env=0;

    //отдельные счётчики
    static uint32_t chA_count=0;
    static uint32_t chB_count=0;
    static uint32_t chC_count=0;

    //копирование прошлого значения каналов для более быстрой работы

    register bool chA_bitOut=chA_bit;
    register bool chB_bitOut=chB_bit;
    register bool chC_bitOut=chC_bit;

        ay0_A_freq =((reg_ay0[1]<<8)|reg_ay0[0]);
        ay0_B_freq =((reg_ay0[3]<<8)|reg_ay0[2]);
        ay0_C_freq =((reg_ay0[5]<<8)|reg_ay0[4]);
        ay0_R12_R11= ((reg_ay0[12]<<8)|reg_ay0[11]);

    #define nR7 (~reg_ay0[7])
    //nR7 - инвертированый R7 для прямой логики - 1 Вкл, 0 - Выкл

    if (nR7&0x1) {chA_count+=delta;if (chA_count>= ay0_A_freq && (ay0_A_freq>=delta) ) {chA_bit^=1;chA_count=0;}} else {chA_bitOut=1;chA_count=0;}; /*Тон A*/
    if (nR7&0x2) {chB_count+=delta;if (chB_count>= ay0_B_freq && (ay0_B_freq>=delta) ) {chB_bit^=1;chB_count=0;}} else {chB_bitOut=1;chB_count=0;}; /*Тон B*/
    if (nR7&0x4) {chC_count+=delta;if (chC_count>= ay0_C_freq && (ay0_C_freq>=delta) ) {chC_bit^=1;chC_count=0;}} else {chC_bitOut=1;chC_count=0;}; /*Тон C*/


    //проверка запрещения тона в каналах
    if (reg_ay0[7]&0x1) chA_bitOut=1; 
    if (reg_ay0[7]&0x2) chB_bitOut=1;
    if (reg_ay0[7]&0x4) chC_bitOut=1;

    //добавление шума, если разрешён шумовой канал
    if (nR7&0x38)//есть шум хоть в одном канале
        {
 
            noise_ay0_count+=delta;
          if (noise_ay0_count>=(reg_ay0[6]<<1)) {noise_bit=get_random(); noise_ay0_count=0;}//отдельный счётчик для шумового
                                // R6 - частота шума
            
               if(!noise_bit)//если бит шума ==1, то он не меняет состояние каналов

                {            
                    if ((chA_bitOut)&&(nR7&0x08)) chA_bitOut=0;//шум в канале A
                    if ((chB_bitOut)&&(nR7&0x10)) chB_bitOut=0;//шум в канале B
                    if ((chC_bitOut)&&(nR7&0x20)) chC_bitOut=0;//шум в канале C

                };
           
        }

       // амплитуды огибающей
        if ((reg_ay0[8] & 0x10) | (reg_ay0[9] & 0x10) | (reg_ay0[10] & 0x10)) // отключение огибающей
        {   
            main_ay_count_env += delta;
            if (is_envelope_begin)
            {
                envelope_ay_count = 0;
                main_ay_count_env = 0;
                is_envelope_begin = false;
            };

            if (((main_ay_count_env) >= (ay0_R12_R11 << 1))) // без операции деления
            {
                   main_ay_count_env -= ay0_R12_R11 << 1;
  
             if (envelope_ay_count >= 32)
             {
              //  if ((reg_ay0[13]==0x08)| (reg_ay0[13]==0x0a) | (reg_ay0[13]==0x0c) | (reg_ay0[13]==0x0e))
              if ((reg_ay0[13]&0x08) && ((~reg_ay0[13])&0x01))
                 envelope_ay_count = 0;  // loop 
                 else envelope_ay_count = 31;

             }

               //  ampl_ENV =  Envelopes[reg_ay0[13]] [envelope_ay_count]  ; // из оперативки
                uint8_t x =  Envelopes_const [ reg_ay0[13]] [envelope_ay_count] ;// 0 ... 15

                ampl_ENV = ampls_AY_table [x]; // 0... 45

                envelope_ay_count++;
            }
        } //end  амплитуды огибающей 


static uint16_t outs[3];

#define outA (outs[0])
#define outB (outs[1])
#define outC (outs[2])
              
        outA = chA_bitOut ? ((reg_ay0[ 8] & 0xf0) ? ampl_ENV : ampls_AY_table [reg_ay0[8]]) : 0;
        outB = chB_bitOut ? ((reg_ay0[ 9] & 0xf0) ? ampl_ENV : ampls_AY_table [reg_ay0[9]]) >>1: 0;
        outC = chC_bitOut ? ((reg_ay0[10] & 0xf0) ? ampl_ENV : ampls_AY_table [reg_ay0[10]]) : 0;
      
        return outs;
};
//===================================================================
//                  CHIP AY 1
//===================================================================
static uint32_t noise_ay1_count=0;
static bool is_envelope_begin1=false;
static uint8_t ampl_ENV_1 = 0;
static uint8_t envelope_ay1_count = 0;
//-------------------------------------------------------------------
// чтение из регистров AY
uint8_t fast(AY_get_reg1)()
{
    return reg_ay1[N_sel_reg_1]; // & 0x0f ?
}
//----------------------------------------------------------
    // запись в регистры AY
    void fast (AY_set_reg1)(uint8_t val)
    {
        if (N_sel_reg_1>0x0f) return;// для TSFM
      //  if (conf.type_sound==TSFM) reg_ay1[N_sel_reg_1] = val & maskAY_FM[N_sel_reg_1];// для TSFM
       // else 
        reg_ay1[N_sel_reg_1] = val & maskAY[N_sel_reg_1];
        switch (N_sel_reg_1)
        {
        case 6:
            noise_ay1_count = 0;
            return;
        case 13:
            is_envelope_begin1 = true;
            return;
        }

    }
    //------------------------

    uint16_t *fast(get_AY_Out1)(uint8_t delta)
    {

    static bool bools1[4];
    #define chA_bit_1 (bools1[0])
    #define chB_bit_1 (bools1[1])
    #define chC_bit_1 (bools1[2])
    #define noise_bit_1 (bools1[3])
  
    static uint32_t main_ay1_count_env=0;

    //отдельные счётчики
    static uint32_t chA1_count=0;
    static uint32_t chB1_count=0;
    static uint32_t chC1_count=0;

    //копирование прошлого значения каналов для более быстрой работы

    register bool chA_bit_1Out=chA_bit_1;
    register bool chB_bit_1Out=chB_bit_1;
    register bool chC_bit_1Out=chC_bit_1;
    
        ay1_A_freq =((reg_ay1[1]<<8)|reg_ay1[0]);
        ay1_B_freq =((reg_ay1[3]<<8)|reg_ay1[2]);
        ay1_C_freq =((reg_ay1[5]<<8)|reg_ay1[4]);
        ay1_R12_R11= ((reg_ay1[12]<<8)|reg_ay1[11]);

    #define n1R7 (~reg_ay1[7])

    //n1R7 - инвертированый R7 для прямой логики - 1 Вкл, 0 - Выкл

    if (n1R7&0x1) {chA1_count+=delta;if (chA1_count>=ay1_A_freq && (ay1_A_freq>=delta) ) {chA_bit_1^=1;chA1_count=0;}} else {chA_bit_1Out=1;chA1_count=0;}; /*Тон A*/
    if (n1R7&0x2) {chB1_count+=delta;if (chB1_count>=ay1_B_freq && (ay1_B_freq>=delta) ) {chB_bit_1^=1;chB1_count=0;}} else {chB_bit_1Out=1;chB1_count=0;}; /*Тон B*/
    if (n1R7&0x4) {chC1_count+=delta;if (chC1_count>=ay1_C_freq && (ay1_C_freq>=delta) ) {chC_bit_1^=1;chC1_count=0;}} else {chC_bit_1Out=1;chC1_count=0;}; /*Тон C*/

    //проверка запрещения тона в каналах
    if (reg_ay1[7]&0x1) chA_bit_1Out=1; 
    if (reg_ay1[7]&0x2) chB_bit_1Out=1;
    if (reg_ay1[7]&0x4) chC_bit_1Out=1;

    //добавление шума, если разрешён шумовой канал
    if (n1R7&0x38)//есть шум хоть в одном канале
        {
            noise_ay1_count+=delta;
            if (noise_ay1_count>=(reg_ay1[6]<<1)) {noise_bit_1=get_random();noise_ay1_count=0;}//отдельный счётчик для шумового
                                // R6 - частота шума
            
            if(!noise_bit_1)//если бит шума ==1, то он не меняет состояние каналов
                {            
                    if ((chA_bit_1Out)&&(n1R7&0x08)) chA_bit_1Out=0;//шум в канале A
                    if ((chB_bit_1Out)&&(n1R7&0x10)) chB_bit_1Out=0;//шум в канале B
                    if ((chC_bit_1Out)&&(n1R7&0x20)) chC_bit_1Out=0;//шум в канале C
            
                };
           
        }

        // вычисление амплитуды огибающей
        if ((reg_ay1[8] & 0x10) | (reg_ay1[9] & 0x10) | (reg_ay1[10] & 0x10)) // отключение огибающей
        {   
            main_ay1_count_env += delta;
            if (is_envelope_begin1)
            {
                envelope_ay1_count = 0;
                main_ay1_count_env = 0;
                is_envelope_begin1 = false;
            };

            if (((main_ay1_count_env) >= (ay1_R12_R11 << 1))) // без операции деления
            {
                   main_ay1_count_env -= ay1_R12_R11 << 1;

             if (envelope_ay1_count >= 32)
             {
               // if ((reg_ay1[13]==0x08)| (reg_ay1[13]==0x0a) | (reg_ay1[13]==0x0c) | (reg_ay1[13]==0x0e))
                if ((reg_ay1[13]&0x08) && ((~reg_ay1[13])&0x01))
                 envelope_ay1_count = 0;
                 else envelope_ay1_count = 31;

             }
                uint8_t x =  Envelopes_const [ reg_ay1[13]] [envelope_ay1_count] ;// 0 ... 15

                ampl_ENV_1 = ampls_AY_table [x]; // 0... 45



                envelope_ay1_count++;
            }
        } //end вычисление амплитуды огибающей 


static uint16_t outs1[3];
#define outA1 (outs1[0])
#define outB1 (outs1[1])
#define outC1 (outs1[2])
          

        outA1 = chA_bit_1Out ? ((reg_ay1[8] & 0xf0) ? ampl_ENV_1 : ampls_AY_table[reg_ay1[8]]) : 0;
        outB1 = chB_bit_1Out ? ((reg_ay1[9] & 0xf0) ? ampl_ENV_1 : ampls_AY_table[reg_ay1[9]])>>1 : 0;
        outC1 = chC_bit_1Out ? ((reg_ay1[10] & 0xf0) ? ampl_ENV_1 : ampls_AY_table[reg_ay1[10]]) : 0;

        return outs1;
    

};
//=================================================================================
//              END SOFT AY
//=================================================================================
//============================================
// NOISE FDD
//============================================
 uint16_t noise_fdd = 0;
 uint16_t noise_fddI2S = 0;

     void sound_fdd(void)
    {
     /*    if (!vbuf_en) return;
      if (conf.sound_fdd) return;
       
        static uint8_t old_sound_track = 0;
        static uint16_t st = 32768;
        if (sound_track != old_sound_track)
        {
            if (st == 32768)
                st = 0;
            old_sound_track = sound_track;
        }
        st++;
        if (st < 32000)
        {
        noise_fdd = (g_gbuf[st+1]) &0x1f;
        //  noise_fdd = ((g_gbuf[st+1])+g_gbuf[get_random_noise()*get_random_noise()])&0x1f;
        //  noise_fdd =get_random_noise();
         //  noise_fddI2S = noise_fdd<<2;
        // noise_fdd = (ampls_AY[st+1]) &0x1f;
           noise_fddI2S = noise_fdd*conf.vol_i2s;
        }   
        else
        {
            noise_fdd = 0;
            noise_fddI2S = 0;
            st = 32768;
        } */
    } 

//======================================================================

//###########################################
// Смешивание каналов звука и вывод по DMA
//###########################################
//#define exponential_volume
#define linear_volume
//###########################################
// Конфигурируемый параметр громкости (0-100%)


#define CH_TS_MAX_VALUE     255 // (45*3)      // Максимальное значение канала TS
#define CH_TS_SCALE_TO     0xffff // 32000       // До какого значения масштабировать канал TS
#define OUTPUT_MAX_VALUE   0xffff //64000/// (CH_GS_MAX_VALUE *3)  // Ограничение суммы каналов (CH_GS_MAX_VALUE * 3)
#define OUTPUT_MIDPOINT   0x8000// (OUTPUT_MAX_VALUE/2) // (32000) // середина диапазона

//###########################################
#ifdef linear_volume
//##################################################################

// Таблица умножения для громкости (0-100% -> 0-256)
static uint16_t volume_mult_table[101];

// Таблицы для быстрого преобразования
static uint16_t ay_scale_table[CH_TS_MAX_VALUE + 4];

//##################################################################
void init_audio_tables_optimized(void) {
    // Таблица для канала TS
    for (int i = 0; i <= CH_TS_MAX_VALUE; i++) {
        ay_scale_table[i] = (i * CH_TS_SCALE_TO) / CH_TS_MAX_VALUE;
    }
    
    // Таблица для громкости i2s : volume * 256 / 100
    for (int i = 0; i <= 100; i++) {
        volume_mult_table[i] = (i * 256) / 100;
    }
}
//###################################################################
void init_vol_ay(uint8_t volume_percent) {
    if (volume_percent > 100) volume_percent = 100;
    current_volume = volume_percent;
    init_audio_tables_optimized();
    set_audio_volume(current_volume);
     beep_volume =   ampls_AY_table[15];//>>2;  // деление на 2 так как beep идет на два канала      ///(volume_percent * 256) / 100;///
}

void set_audio_volume(uint8_t volume_percent) {
    if (volume_percent > 100) volume_percent = 100;
    current_volume = volume_percent;
            //копирование таблицы AY в RAM  и умножение на audio_buster
            for (size_t i = 0; i < 16; i++)
            {
                ampls_AY_table[i]=const_ampls_AY_table[i] *  (audio_buster+1);
            }
   
}

void set_audio_buster(void) {
            //копирование таблицы AY в RAM  и умножение на audio_buster
            for (size_t i = 0; i < 16; i++)
            {
                ampls_AY_table[i]=const_ampls_AY_table[i] *  (audio_buster+1);
            }
}

uint8_t get_audio_volume(void) {
    return current_volume;
}
//##################################################################

void fast (audio_out_i2s_ts)(void)
{	
    AY_data = get_AY_Out(AY_DELTA);			
    AY_data1 = get_AY_Out1(AY_DELTA);
    
    uint16_t beep_out = gpio_get(BEEP_PIN) ? 0 : beep_volume;

    uint32_t sumL = AY_data[0] + AY_data[1] + AY_data1[0] + AY_data1[1] + beep_out;
    uint32_t sumR = AY_data[2] + AY_data[1] + AY_data1[2] + AY_data1[1] + beep_out;
    
    sumL = (sumL > CH_TS_MAX_VALUE) ? CH_TS_MAX_VALUE : sumL;
    sumR = (sumR > CH_TS_MAX_VALUE) ? CH_TS_MAX_VALUE : sumR;

    uint32_t totalL = ay_scale_table[sumL] + uintGS_L ;
    uint32_t totalR = ay_scale_table[sumR] + uintGS_R ;
    
    if (totalL > OUTPUT_MAX_VALUE) totalL = OUTPUT_MAX_VALUE;
    if (totalR > OUTPUT_MAX_VALUE) totalR = OUTPUT_MAX_VALUE;
    
    #ifndef MIDI

    // Преобразуем в int16_t диапазон
    int32_t outL = (int32_t)totalL - OUTPUT_MIDPOINT ;
    int32_t outR = (int32_t)totalR - OUTPUT_MIDPOINT ;

    #endif

    #ifdef MIDI
       int32_t outL = ((int32_t)totalL  - OUTPUT_MIDPOINT ) + midi_sound   ;
      int32_t outR = ((int32_t)totalR   - OUTPUT_MIDPOINT ) + midi_sound   ;
/*       outL += (midi_sound  );
      outR += (midi_sound );  */

/*       int32_t outL = ((int32_t)totalL + midi_sound) - OUTPUT_MIDPOINT  ;
      int32_t outR = ((int32_t)totalR + midi_sound) - OUTPUT_MIDPOINT ; */



   // outL = (outL > 32767) ? 32767 : (outL < -32768) ? -32768 : outL;
   // outR = (outR > 32767) ? 32767 : (outR < -32768) ? -32768 : outR;

    #endif 

    
    // Применяем громкость БЕЗ ДЕЛЕНИЯ - только умножение и сдвиг
    outL = (outL * volume_mult_table[current_volume]) >> 8;
    outR = (outR * volume_mult_table[current_volume]) >> 8;
    
    // Ограничиваем результат   можно и без этого
 //   outL = (outL > 32767) ? 32767 : (outL < -32768) ? -32768 : outL;
 //  outR = (outR > 32767) ? 32767 : (outR < -32768) ? -32768 : outR;

   if (!mute)
    i2s_out((int16_t)outR, (int16_t)outL);


}
#endif
//###############################################################
void select_audio(void) /*I2S  TS Sound */
{
#ifdef TURBOSOUND
    chip_ay = 0;
    maskAY[1] = maskAY[3] = maskAY[5] = 0x0f; // установки маски по умолчанию 
#endif
    init_i2s_sound();
}
//###############################################################
