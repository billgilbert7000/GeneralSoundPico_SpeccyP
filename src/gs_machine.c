


#include "string.h"
#include "stdbool.h"

#include "gs_machine.h"
#include "sound_ay/aySoft.h"

#include "hw_util.h"

//#define Z80_MAXIMUM_CYCLES 420

#include <Z80.h>


// rom
//#include "rom/gs105b.h"// ROM GS
//#include "rom/gs104.h"// ROM GS


#include "hardware/structs/systick.h"


//#include "psram_spi.h"
#include "config.h"

#include "audio_i2s.h"

#ifdef MIDI
#include "midi/general-midi.h"
#endif

//==============================================================================



//==============================================================================

//##########################################################
// Настройки и функции для эмулятора Z80 REDCODE
typedef struct {
        void* context;
        zuint8 (* read)(void *context);
        void (* write)(void *context, zuint8 value);
        zuint16 assigned_port;
} Device;
//
typedef struct {
        zusize  cycles;
        zuint8  memory[65536];
        Z80     cpu;
        Device* devices;
        zusize  device_count;
} Machine;
//
    Machine machine;  // Создаем экземпляр машины
    Machine* z1 = &machine;  // Создаем указатель на машину
//
Device *machine_find_device(Machine *self, zuint16 port)
{
    zusize index = 0;

    for (; index < self->device_count; index++)
        if (self->devices[index].assigned_port == port)  // Исправлено: . вместо ->
            return &self->devices[index];                // Исправлено: возвращаем адрес элемента массива

    return Z_NULL;
}

//####################################################################
// Разное для звука
//###################################################################
volatile uint8_t  volume_1;
volatile uint8_t  volume_2;
volatile uint8_t  volume_3;
volatile uint8_t  volume_4;
volatile uint8_t channel1;
volatile uint8_t channel2;
volatile uint8_t channel3;
volatile uint8_t channel4;
//###########################################
// Смешивание каналов звука и вывод по DMA
//###########################################

 int16_t static inline (mix_sample)(int16_t const sample1, int16_t const sample2)
{
    int32_t mixed = (int32_t)sample1 + (int32_t)sample2;
    if(  32767 < mixed ){ return  32767; }
    if( -32768 > mixed ){ return -32768; }
    return (int16_t)(mixed & (int32_t)0xFFFF);
};
  
void fast (GS_get_sound_LR_sample)()
{
 /*    outGS_L=mix_sample((int16_t)(volume_2*(channel2-128)),(int16_t)(volume_4*(channel4-128))); // left   2 4
       outGS_R=mix_sample((int16_t)(volume_3*(channel3-128)),(int16_t)(volume_1*(channel1-128))); // right  1 3 */
// Версия СТЕРЕО Евгения Мучкина
   //  outGS_L=mix_sample((int16_t)(volume_1*(channel1-128)),(int16_t)(volume_4*(channel4-128))); // left  1 4
  //   outGS_R=mix_sample((int16_t)(volume_2*(channel2-128)),(int16_t)(volume_3*(channel3-128))); // right 2 3  
     // безнаковое число uint16_t — от 0 до 65 535
     uintGS_L  = ((volume_1*channel1) + (volume_4*channel4));//*(audio_buster+1);  // 255 * 63 + 255 * 63 = 16065
     uintGS_R  = ((volume_2*channel2) + (volume_3*channel3));//*(audio_buster+1);  // 255 * 63 + 255 * 63 = 16065

//return (int16_t)(sample>>2 ) -32768;// >>2 - 32768

  /*   outGS_L=(int16_t)(volume_1*(channel1-128))+(int16_t)(volume_4*(channel4-128)); // left  1 4
    outGS_R=(int16_t)(volume_2*(channel2-128))+(int16_t)(volume_2*(channel2-128)); // right 2 3    */ 

}; 
//##################################################################################

//==================================================================================
/*
Таблица распределения памяти

PAGE = 0:
Диапазон	Банк	Обозначение
0000-3FFF	0   	ROM0
4000-7FFF	1	    RAM 0.1
8000-BFFF	2   	ROM0
C000-FFFF	3	    ROM1

PAGE = 1:
Диапазон	Банк	Обозначение
0000-3FFF	0   	ROM0
4000-7FFF	1	    RAM 0.1
8000-BFFF	2	    RAM 0.0
C000-FFFF	3	    RAM 0.1

PAGE = 2:
Диапазон	Банк	Обозначение
0000-3FFF	0	    ROM0
4000-7FFF	1	    RAM 0.1
8000-BFFF	2	    RAM 1.0
C000-FFFF	3	    RAM 1.1

PAGE = 31:
Диапазон	Банк	Обозначение
0000-3FFF	0   	ROM0
4000-7FFF	1	    RAM 0.1
8000-BFFF	2	    RAM 30.0
C000-FFFF	3   	RAM 30.1

PAGE = 63:
Диапазон	Банк	Обозначение
0000-3FFF	0   	ROM0
4000-7FFF	1   	RAM 0.1
8000-BFFF	2   	RAM 62.0
C000-FFFF	3   	RAM 62.1

## **Общая схема для PAGE 0-63:** ТОЛЬКО Банк 2 и Банк 3
-----------------------------------------------------------
| Page | Банк 2   | Банк 3   | Смещение | Общий объем RAM |
|------|----------|----------|----------|-----------------|
| 0    | ROM0     | ROM1     | -        | 16KB            |
| 1    | RAM 0.0  | RAM 0.1  | 0x0000   | 48KB            |
| 2    | RAM 1.0  | RAM 1.1  | 0x8000   | 80KB            |
| 3    | RAM 2.0  | RAM 2.1  | 0x10000  | 112KB           |
| 4    | RAM 3.0  | RAM 3.1  | 0x18000  | 144KB           |
| ...  | ...      | ...      | ...      | ...             |
| 31   | RAM 30.0 | RAM 30.1 | 0xF0000  | 976KB           |
| 32   | RAM 31.0 | RAM 31.1 | 0xF8000  | 1008KB          |
| ...  | ...      | ...      | ...      | ...             |
| 63   | RAM 62.0 | RAM 62.1 | 0x1F0000 | 2000KB (~2MB)   |
-----------------------------------------------------------

Банк 0: Всегда ROM0
Банк 1: Всегда RAM 0.1
Банк 2: RAM (page-1).0
Банк 3: RAM (page-1).1

При page=0: Банки 2-3 в ROM режиме
При page≥1: Банки 2-3 переключаются по формуле RAM (page-1).X

*/
// Глобальные переменные
static uint8_t gspage = 0;          // Текущая страница
static uint8_t ngs_mode_pg1 = 1;    // Доп. страница (для банка 3)
static uint8_t ngs_cfg0 = 0;        // Регистр конфигурации

// Банки памяти для чтения/записи
static uint8_t *gsbankr[4];
static uint8_t *gsbankw[4];

// Размеры областей памяти
#define PAGE_SIZE       0x4000        // 16KB GS
//#define TRASH_PAGE_SIZE 0x4000        // Фиктивная страница
#define PAGE_MASK       (0x3fff)


#ifndef NO_PSRAM


// Глобальные указатели на PSRAM
uint8_t* const GSRAM_M = (uint8_t*)PSRAM_BASE;  // Основной массив в PSRAM

// Локальная RAM (без PSRAM) для банков 0 и 1
uint8_t *ROM0 = ROM_GS_M;
uint8_t *ROM1 = ROM_GS_M + PAGE_SIZE; 

uint8_t RAM_NO_PSRAM0[PAGE_SIZE] = {0};
uint8_t RAM_NO_PSRAM1[PAGE_SIZE] = {0};

//==============================================================================
// Функция обновления отображения памяти (оптимизированная)
//==============================================================================
/* void fast (UpdateMemMapping)() {
    if (page == 0) {
        // Режим ROM: 0000-3FFF=ROM0, 4000-7FFF=RAM1, 8000-BFFF=ROM0, C000-FFFF=ROM1
        gsbankr[0] = ROM0;
        gsbankr[1] = gsbankw[1] = RAM_NO_PSRAM1;
        gsbankr[2] = ROM0;
        gsbankr[3] = ROM1;
        
        gsbankw[0] = ROM0;
        gsbankw[2] = ROM0;
        gsbankw[3] = ROM1;
    }
    else if (page == 1) {
        // Режим локальной RAM: 0000-3FFF=ROM0, 4000-7FFF=RAM1, 8000-BFFF=RAM0, C000-FFFF=RAM1
        gsbankr[0] = ROM0;
        gsbankr[1] = gsbankw[1] = RAM_NO_PSRAM1;
        gsbankr[2] = gsbankw[2] = RAM_NO_PSRAM0;
        gsbankr[3] = gsbankw[3] = RAM_NO_PSRAM1;
        
        gsbankw[0] = ROM0;
    }
    else {
        // Режим PSRAM: прямое вычисление адресов
        gsbankr[0] = ROM0;
        gsbankr[1] = gsbankw[1] = RAM_NO_PSRAM1;

         uint32_t base_addr = (page - 2) * (PAGE_SIZE * 2);
        // Прямое вычисление для Bank1 (8000-BFFF) 
         gsbankr[2] = gsbankw[2] = GSRAM_M + base_addr;          
        // Прямое вычисление для Bank2 (C000-FFFF)
         gsbankr[3] = gsbankw[3] = GSRAM_M + base_addr + PAGE_SIZE;  
         gsbankw[0] = ROM0;
    }
} */
//###################################################################
//   Использование LUT (Look-Up Table)
//   LUT только для банков 2 и 3 (которые меняются)
//   Предварительно вычисленная таблица адресов
static uint8_t* bank2_lut[256];
static uint8_t* bank3_lut[256];

void InitMemMappingLUT() {
    // Инициализация постоянных банков
    gsbankr[0] = ROM0;
    gsbankr[1] = RAM_NO_PSRAM1;
    gsbankw[0] = ROM0;
    gsbankw[1] = RAM_NO_PSRAM1;
    
    // Заполняем LUT для банков 2 и 3
    for (int p = 0; p < 256; p++) {
        if (p == 0) {
            bank2_lut[p] = ROM0;
            bank3_lut[p] = ROM1;
        } else if (p == 1) {
            bank2_lut[p] = RAM_NO_PSRAM0;
            bank3_lut[p] = RAM_NO_PSRAM1;
        } else {
            uint32_t base = (p - 2) * (PAGE_SIZE * 2);
            bank2_lut[p] = GSRAM_M + base;
            bank3_lut[p] = GSRAM_M + base + PAGE_SIZE;
        }
    }
}
//###################################################################
// СУПЕР БЫСТРАЯ версия - только 2 присваивания!
void fast (UpdateMemMapping)() {
    gsbankr[2] = gsbankw[2] = bank2_lut[page];
    gsbankr[3] = gsbankw[3] = bank3_lut[page];
}
//###################################################################
void gs_mem_init() {
    // Инициализация PSRAM (если требуется)
    // psram_init(); // Раскомментировать если есть функция инициализации PSRAM
    
       InitMemMappingLUT();
    // Начальная конфигурация (ROM режим)
    page = 0;
    UpdateMemMapping();
}

//###################################################################
// Оптимизированные функции чтения/записи памяти (без проверок)
//###################################################################
__attribute__((always_inline)) inline static zuint8 fast (machine_cpu_read)(Machine* self, zuint16 address) {
    uint8_t bank_index = (address >> 14) & 0x03;
    uint8_t* bank_ptr = gsbankr[bank_index];
    uint16_t offset = address & PAGE_MASK;
    
    // Прямой доступ без проверок
    uint8_t data = bank_ptr[offset];
    
    // Обработка зоны автоматического ЦАП (6000-7FFF)
    if ((address & 0xE000) == 0x6000) {
        switch (address & 0x6300) {
            case 0x6300: channel4 = data; return data;
            case 0x6200: channel3 = data; return data;
            case 0x6100: channel2 = data; return data;
            case 0x6000: channel1 = data; return data;
        }
    }
    
    return data;
}
//###########################################################################################
__attribute__((always_inline)) inline static zuint8 fast (instruction_read)(Machine* self, zuint16 address) {
    uint8_t bank_index = (address >> 14) & 0x03;
    uint8_t* bank_ptr = gsbankr[bank_index];
    uint16_t offset = address & PAGE_MASK; 
    // Прямой доступ без проверок
    return bank_ptr[offset];
}
//################################################################################################
__attribute__((always_inline)) inline static void fast (machine_cpu_write)(Machine* self, zuint16 address, zuint8 value) {
    uint8_t bank_index = (address >> 14) & 0x03;
    uint8_t* bank_ptr = gsbankw[bank_index];
    uint16_t offset = address & PAGE_MASK;
    // Прямая запись без проверок
    bank_ptr[offset] = value;
}
//###############################################################################################


#endif

#ifdef NO_PSRAM

// Глобальные указатели
uint8_t GSRAM_M[RAM_PAGES * 2 * PAGE_SIZE];
// Банки памяти для чтения/записи
static uint8_t *gsbankr[4];  // Объявляем без инициализации
static uint8_t *gsbankw[4];


//############################################################################
// Инициализация памяти GS
void gs_mem_init() {
    // Начальные значения (ROM режим)
    gsbankr[0] = ROM_GS_M;
    gsbankr[1] = gsbankw[1] = GSRAM_M;
    gsbankr[2] = ROM_GS_M;
    gsbankr[3] = ROM_GS_M + PAGE_SIZE;
    
    gsbankw[0] = gsbankw[2] = ROM_GS_M; gsbankw[3] = ROM_GS_M + PAGE_SIZE;

}
//##########################################################################

//#############################################
// Функция обновления отображения памяти
//#############################################
void fast (UpdateMemMapping)() {
        // Стандартный режим с ROM
         if (page>(RAM_PAGES)) // 4*32 = 128
         return;  
         if (page) {
            gsbankr[0] = ROM_GS_M;
            gsbankr[1] = gsbankw[1] = GSRAM_M + PAGE_SIZE; 
            gsbankr[2] = gsbankw[2] = GSRAM_M + PAGE_SIZE*2*(page-1);
            gsbankr[3] = gsbankw[3] = GSRAM_M + PAGE_SIZE*2*(page-1) + PAGE_SIZE;
         } else // page == 0
           { 
            gsbankr[0] = ROM_GS_M;
            gsbankr[1] = gsbankw[1] = GSRAM_M + PAGE_SIZE;
            gsbankr[2] = ROM_GS_M;
            gsbankr[3] = ROM_GS_M+PAGE_SIZE;

            gsbankw[0] = gsbankw[2] = ROM_GS_M;
            gsbankw[3] = ROM_GS_M+PAGE_SIZE;
         }
         return;
}

//#############################
// Функция чтения из памяти 
//#############################
__attribute__((always_inline))  inline static zuint8 fast(machine_cpu_read)(Machine *self, zuint16 address)
{
//   uint8_t bank = (address >> 14) & 0x03;
// Самый быстрый доступ к памяти
 uint8_t data = gsbankr[ (address >> 14)& 0x03][address & PAGE_MASK];
    // Оптимизированный доступ к памяти
  //  uint8_t bank_index = (addr >> 14) & 0x03;
  //  uint8_t data = gsbankr[bank_index][addr & PAGE_MASK];
       // Определяем банк по старшим 2 битам адреса
//    uint8_t bank = (address >> 14) & 0x03;
    // Получаем указатель на банк и вычисляем смещение
 //   uint8_t *bank_ptr = gsbankr[bank];
 //   uint16_t offset = address & PAGE_MASK;
    
    // Читаем оригинальное значение
 //   uint8_t data = bank_ptr[offset];  

    // Обработка зоны автоматического ЦАП (6000-7FFF)
  if ((address & 0xE000) == 0x6000) {
//if ((address > 0x3fff) && (address < 0x8000)) {
        
        switch (address&0x6300){
            case 0x6300: 
                channel4 = data;
                break;
            case 0x6200: 
                channel3 = data;
                break;
            case 0x6100: 
                channel2 = data;
                break;
            case 0x6000: 
                channel1 = data;
                break;
        }   
    return data;
}
}
//##############################################
// Функция чтения из памяти операторов
// оставленно для дальнейшего развития проекта
//##############################################
__attribute__((always_inline))  inline static zuint8 fast (instruction_read)(Machine *self, zuint16 address)
{
//  uint8_t bank = (address >> 14) & 0x03;
// Самый быстрый доступ к памяти
  uint8_t data = gsbankr[ (address >> 14)& 0x03][address & PAGE_MASK];
    return data;
}
//#############################################
// Функция записи в память 
//##############################################
//__attribute__((always_inline))  inline  void fast (WrZ80)( uint16_t addr, uint8_t val) 
__attribute__((always_inline))  inline static void fast(machine_cpu_write)(Machine *self, zuint16 address, zuint8 value)
{
     //  uint8_t bank = (address >> 14) & 0x03;

    /// чтото тут
   //   gsbankw[(address >> 14) & 0x03][address & PAGE_MASK]=value;// если ПЗУ всё равно не запишет ;)

/*  старая версия для справки */
    // Определяем банк по старшим 2 битам адреса
     uint8_t bank = (address >> 14) & 0x03;
    // Получаем указатель на банк и вычисляем смещение
    uint8_t *bank_ptr = gsbankw[bank];
    uint16_t offset = address & PAGE_MASK;

         if (bank_ptr == ROM_GS_M+PAGE_SIZE) {
        return;
    }
        if (bank_ptr == ROM_GS_M) {
        return;
    }
    
    bank_ptr[offset] = value;  
}
#endif
//===================================================================

//############################################
//  IN  Чтение из порта
//############################################
//__attribute__((always_inline))  inline uint8_t fast(InZ80)( uint16_t port16)
__attribute__((always_inline))  inline static zuint8 fast(machine_cpu_in)(Machine *self, zuint16 port)
     {
switch (port & 0x000F) {

           	    case 0x01: 
                 return  command;

     			 case 0x02: 
                 status &= 0x7F;// сбрасываем D7 флаг данных
				//printf("* data_zx = %x : ",  data_zx);
              //  printf("\n");
				 return data_zx;
                 case 0x03: status |= 0x80; data_zx = 0xFF; 
                 return 0xFF;
     			 //case 0x03: status |= 0x80;  return 0xFF;
     			 case 0x04: 
			   // printf(" status = %x : ",  status );
               // printf("\n");
				 return status ;
    		     case 0x05:
                  status &= 0xFE; 
                  return 0xFF;
    		     case 0x0A: status = ((status & 0x7F) | (page << 7)); return 0xFF;
     			case 0x0B:
              //  volume[3]=0;
              //   status =((status & 0xFE) | (volume[0] >> 5) );
               status = ((status & 0xFE) | ((volume[0] >> 5) ));
           //     status =((status & 0xFE) );
                  return 0xFF;
    }
	return 0xFF;
}
//##################################################
// OUT  Запись в порт
//##################################################
//__attribute__((always_inline))  inline void fast(OutZ80)( uint16_t port16, uint8_t val)
__attribute__((always_inline))  inline static void fast(machine_cpu_out)(Machine *self, zuint16 port, zuint8 value)
{
    switch (port & 0x000F) {


        case 0x00: // Порт 0 - страницы памяти (D0-D3)
            page = value ;//& 0x3f;
        // Обработка порта выбора страницы
               #ifdef NO_PSRAM
               UpdateMemMapping();// это тоже самое
               #else
                gsbankr[2] = gsbankw[2] = bank2_lut[page];
                gsbankr[3] = gsbankw[3] = bank3_lut[page];
               #endif  
              return; 
        //----------------------------

        case 0x02: //gsstat &= 0x7F; return;
			 status &= 0x7F; return;

        case 0x03: // Порт 3 - запись данных
		//Записать байт в регистр данных, который может прочитать спектрум, устанавливает data bit D7
            data_gs = value;
            status |= 0x80; //??? D7 = 1 
            return;

        case 0x05:
         status &= 0xFE;
          return;

        // Порты 6-9 - громкость (D0-D5)  // volume[(port & 0x0F) - 6] = value & 0x3F;
        case 0x06:  //volume_1
            volume_1=value;//<<1;
            break; 
        case 0x07:  //volume_2
            volume_2=value;//<<1;
            break; 
        case 0x08:  //volume_2
            volume_3=value;//<<1;
            break; 
        case 0x09:  //volume_4
            volume_4=value;//<<1;
            break; 
         
           

         case 0x0A:// gsstat = u8((gsstat & 0x7F) | (gspage << 7)); return;
         status = ((status & 0x7F) | (page << 7)); 
		 return;

		 case 0x0B:// gsstat = u8((gsstat & 0xFE) | ((gsvol[0] >> 5) & 1)); return;
       //status = ((status & 0xFE) | ((volume[0] >> 5) & 1));
		 status = ((status & 0xFE) | ((volume_1 >> 5) & 1));
		 return;


	}
}
// 
inline static zuint8 fast(nop_callback)(Machine *self, zuint16 address)
{
 // gpio_put(LED_BOARD, 1);
 // led_blink();
 //z80_int(&z1->cpu, false);// INT OFF
return 0x00;
}
//###########################################
// инициализация Z80 для GS
void machine_initialize(Machine *self)
        {
        self->cpu.context      = self;
        self->cpu.fetch_opcode = (Z80Read )instruction_read;
        self->cpu.fetch        = (Z80Read )instruction_read;
        self->cpu.nop          = (Z80Read )nop_callback;
        self->cpu.read         = (Z80Read )machine_cpu_read;
        self->cpu.write        = (Z80Write)machine_cpu_write;
        self->cpu.in           = (Z80Read )machine_cpu_in;
        self->cpu.out          = (Z80Write)machine_cpu_out;
        self->cpu.halt         = Z_NULL;
        self->cpu.nmia         = Z_NULL;
        self->cpu.inta         = Z_NULL;
        self->cpu.int_fetch    = Z_NULL;
        self->cpu.ld_i_a       = Z_NULL;
        self->cpu.ld_r_a       = Z_NULL;
        self->cpu.reti         = Z_NULL;
        self->cpu.retn         = Z_NULL;
        self->cpu.hook         = Z_NULL;
        self->cpu.illegal      = Z_NULL;
        self->cpu.options      = 0;//Z80_MODEL_ZILOG_NMOS;

        /* Create and initialize devices... */
        }
//##########################################################
void machine_power(Machine *self, zbool state)
        {
        if (state)
                {
                self->cycles = 0;
                memset(self->memory, 0, 65536);
                }

        z80_power(&self->cpu, state);
        }
//###########################################################
void machine_reset(Machine *self)
        {
        z80_instant_reset(&self->cpu);
        }        
//##########################################################
 // Макросы для регистров Z80
#define PC      Z80_PC(z1->cpu)
#define SP      Z80_SP(z1->cpu)
#define AF      Z80_AF(z1->cpu)
#define BC      Z80_BC(z1->cpu)
#define DE      Z80_DE(z1->cpu)
#define HL      Z80_HL(z1->cpu)
#define IX      Z80_IX(z1->cpu)
#define IY      Z80_IY(z1->cpu)
#define MEMPTR  Z80_MEMPTR(z1->cpu)

// Макросы для 8-битных регистров
#define A       Z80_A(z1->cpu)
#define F       Z80_F(z1->cpu)
#define B       Z80_B(z1->cpu)
#define C       Z80_C(z1->cpu)
#define D       Z80_D(z1->cpu)
#define E       Z80_E(z1->cpu)
#define H       Z80_H(z1->cpu)
#define L       Z80_L(z1->cpu)

// Макросы для альтернативных регистров
#define AF_     Z80_AF_(z1->cpu)
#define BC_     Z80_BC_(z1->cpu)
#define DE_     Z80_DE_(z1->cpu)
#define HL_     Z80_HL_(z1->cpu)
#define A_      Z80_A_(z1->cpu)
#define F_      Z80_F_(z1->cpu)
#define B_      Z80_B_(z1->cpu)
#define C_      Z80_C_(z1->cpu)
#define D_      Z80_D_(z1->cpu)
#define E_      Z80_E_(z1->cpu)
#define H_      Z80_H_(z1->cpu)
#define L_      Z80_L_(z1->cpu)

// Макросы для частей регистров
#define PCH     Z80_PCH(z1->cpu)
#define PCL     Z80_PCL(z1->cpu)
#define SPH     Z80_SPH(z1->cpu)
#define SPL     Z80_SPL(z1->cpu)
#define IXH     Z80_IXH(z1->cpu)
#define IXL     Z80_IXL(z1->cpu)
#define IYH     Z80_IYH(z1->cpu)
#define IYL     Z80_IYL(z1->cpu)

// Специальные регистры
#define I       (z1->cpu.i)
//#define R       z80_r(&z1->cpu) //???
//##############################################
void zx_machine_init()
{
	  gs_mem_init();

    machine_initialize(z1);  // Инициализируем машину
    machine_power(z1, Z_TRUE);  // Включаем питание машины

};

//##############################
// reset Z80
//##############################
void zx_machine_reset(uint8_t rom_x)
{
      machine_reset(z1);
      status =0xff;
      command=0x00;

};
//#########################################################################
__attribute__((always_inline)) inline void fast(zx_machine_main_loop_start)(void)
{

    //time_us_64() возвращает текущее время в микросекундах
    uint64_t int_tick=time_us_64()+26;//Устанавливает время следующего прерывания на 26 микросекунд в будущем
    while (1)
    {
       uint64_t tick_time = time_us_64();
    if (tick_time>=int_tick)//Проверяет, наступило ли время для выполнения синхронизированной части эмуляции.
    {    
            
      int_tick=tick_time+27;// Следующее прерывание через 27 мкс
       
        z80_int(&z1->cpu, true);// Генерация прерывания Z80

        z80_run(&z1->cpu, 1);//4 min 28 max

        z80_int(&z1->cpu, false);// Генерация прерывания Z80

    }
    else
    {
 
      int32_t dt=int_tick-tick_time;
 
      dt=(dt>0)?dt:1;
 
    //   z80_run(&z1->cpu, dt*GSCPU_FRQ);// 18MHz  
     #ifdef PICO_RP2350
     dt= (dt << 4) + (dt << 2);  // dt*20 МГц 
     #else 
      dt= (dt << 4);  // dt*16 МГц 
     #endif  
     z80_run(&z1->cpu, dt);  


    }


    }

}
//#########################################################################


//====================================================================
/*
для работы с полным 16-битным адресом нужно использовать:

cpu.PC.W - для получения/установки полного адреса

cpu.PC.B.h и cpu.PC.B.l - для работы с отдельными байтами (старшим и младшим)
*/
/*
        ORG GSRomBaseL+#0038

INT8    EX AF,AF'   0x08         *0038
        PUSH DE     0xD5         *0039
        LD E,A      0x5F         *003A
        LD D,HX     0xDD 0x54    *003B
        LD A,(DE)                *003D
        INC D
        LD A,(DE)
        INC D
        LD A,(DE)
        INC D
        LD A,(DE)
        INC E
        JR Z,INT8_
        LD A,E
        POP DE
        EX AF,AF'
        EI
        RET
INT8_   JP QTDONE
*/


//====================================================================

 