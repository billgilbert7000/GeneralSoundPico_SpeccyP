cmake_minimum_required(VERSION 3.13)
# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# Устанавливаем параметры платы и чипа и так далее
set(PICO_BOARD pico2 CACHE STRING "Board type")

# == Конфигурация  GSP ==

# Выберите конфигурацию (раскомментировать одну строку):
set(GSP_CONFIG GSP_FULL)    # Полная версия с PSRAM_BUTTER, TURBOSOUND, Z_CONTROLER
#set(GSP_CONFIG GSP_NO_PSRAM)     
#set(GSP_CONFIG GSP_LITE)     # Облегченная версия без дополнительных опций

if(NOT DEFINED GSP_CONFIG)
    set(GSP_CONFIG GSP_FULL)
endif()

# Выберите плату (раскомментировать одну строку):

#set(M_BOARD MURM1)
set(M_BOARD MURMGS)
#set(M_BOARD MURM2)
#set(M_BOARD PI_CARD)

if(NOT DEFINED M_BOARD)
    set(M_BOARD MURM1 )
endif()
###

# Определяем опции в зависимости от конфигурации GSP
if(GSP_CONFIG STREQUAL "GSP_FULL")
    set(GSP_CONFIG_DEFINITIONS 
        CPU_MHZ=378 # 378 MHz
        PSRAM_BUTTER
        RAM_PAGES=64 #Число страниц ОЗУ которые реально могут быть доступны GS
        TURBOSOUND
        Z_CONTROLER
        MIDI
        #RTC_NOVA
        #RTC_SMUC
    )

elseif(GSP_CONFIG STREQUAL "GSP_NO_PSRAM")
    set(GSP_CONFIG_DEFINITIONS 
        
        CPU_MHZ=378 # 378 MHz
        NO_PSRAM
        RAM_PAGES=5
        TURBOSOUND
        Z_CONTROLER
        #RTC_NOVA
        #RTC_SMUC
    )
    
elseif(GSP_CONFIG STREQUAL "GSP_LITE")
    set(GSP_CONFIG_DEFINITIONS 
        
        CPU_MHZ=378 # 378 MHz
        NO_PSRAM
        RAM_PAGES=4
        TURBOSOUND
        #Z_CONTROLER
    )
    
else()
    # По умолчанию используем FULL
    set(GSP_CONFIG STREQUAL "GSP_FULL")
    set(GSP_CONFIG_DEFINITIONS 
        
        CPU_MHZ=378 # 378 MHz
        PSRAM_BUTTER
        RAM_PAGES=64 #Число страниц ОЗУ которые реально могут быть доступны GS
        TURBOSOUND
        Z_CONTROLER
        #RTC_NOVA
        #RTC_SMUC
    )

endif()
###



# Определяем платформу на основе выбранной платы
if(PICO_BOARD STREQUAL "pico")
    set(PICO_PLATFORM "rp2040" CACHE STRING "Chip type" FORCE)
elseif(PICO_BOARD STREQUAL "pico2")
    set(PICO_PLATFORM "rp2350" CACHE STRING "Chip type" FORCE)
else()
    message(FATAL_ERROR "Unsupported PICO_BOARD: ${PICO_BOARD}")
endif()

include(pico_sdk_import.cmake)
## инициализация проекта
project(GSP C CXX ASM)
set (PORT_VERSION "1.5.4") 


set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}  -funroll-loops -ffast-math -feliminate-unused-debug-types -ffunction-sections -fdata-sections  -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funroll-loops -ffast-math -feliminate-unused-debug-types -ffunction-sections -fdata-sections  -O3")


##set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -ffast-math  -Os")
#set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -ffast-math  -O3")
 ##set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funroll-loops -ffast-math -feliminate-unused-debug-types -ffunction-sections -fdata-sections -Os")
#-O2 оптимизация
#оптимизации циклов -funroll-loops
#-устранение неиспользуемых-отладочных-типов -feliminate-unused-debug-types
# помещает каждую функцию в отдельную секцию -ffunction-sections 
# -fdata-sections помещает каждую функцию или элемент данных в отдельную секцию в выходном файле
#-Wall  Включает предупреждения
#-fito удаение неиспользуемых функций
#set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")

### инициализация Pico SDK
pico_sdk_init()
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/build")


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

##add_subdirectory(drivers/fatfs)
add_subdirectory(drivers/sdcard)
##add_subdirectory(drivers/psram)
add_subdirectory(drivers/audio_i2s)

file(GLOB_RECURSE SRC 
 "src/*.c"
 "src/*.h"
 "src/zx_emu/*.c" 
 "src/zx_emu/rom/*.h" 
 "src/picobus/*.c"
 "src/sound_ay/*.c"
 "src/rtc/*.c"
 "src/midi/*.c"
 "src/midi/*.h"
)
message(STATUS "Add source files:")
foreach (SRC_FILE IN LISTS SRC)
    message(STATUS "${SRC_FILE}")
endforeach ()
message(STATUS "")


add_executable(${PROJECT_NAME} ${SRC})
##pico_generate_pio_header(pico-pio-link-2 ${CMAKE_CURRENT_LIST_DIR}/picoputer.pio)


if( ${PICO_PLATFORM} MATCHES "rp2040" )
	pico_define_boot_stage2(slower_boot2 ${PICO_DEFAULT_BOOT_STAGE2_FILE})
	target_compile_definitions(slower_boot2 PRIVATE PICO_FLASH_SPI_CLKDIV=4)
	pico_set_boot_stage2(${PROJECT_NAME} slower_boot2)

	endif()



pico_set_program_name(GSP "General Sound Pico")
pico_set_program_version(GSP ${PORT_VERSION})



target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
###############################################
target_compile_definitions(${PROJECT_NAME} PRIVATE


# Конфигурация в зависимости от GSP_CONFIG
${GSP_CONFIG_DEFINITIONS}

CPU_KHZ=CPU_MHZ*1000 # 378000 KHz
PICOBUS_SPEED=20   #// 160000000 = 20MB/s // 80000000 = 10MB/s

############################################################
###   для эмулятора Z80 REDCODE
Z80_STATIC
Z80_EXTERNAL_HEADER="Z80_compat.h"
##Z80_WITH_EXECUTE
##Z80_WITH_Q
Z80_WITH_PRECOMPUTED_DAA
##Z80_WITH_ZILOG_NMOS_LD_A_IR_BUG
#Z80_HALT_CANCEL
#Z80_MODEL_ZILOG_NMOS
##Z80_MODEL_ZILOG_CMOS

############################################################

${PLATFORM_DEFINITIONS}
)
#######################################################
#    MURM1 GS 
#    PIO 0 SM0 и SM1 PicoBus   
#    PIO 0 SM2 I2S
#######################################################
if( ${M_BOARD} MATCHES "MURMGS")
set (M_VERSION m1)# версия платы
target_compile_definitions(${PROJECT_NAME} PRIVATE

##PicoBus
PBUS_OUT_PIN=27
PBUS_IN_PIN=26
PBUS_PIO=pio0   #only pio 0

PBUS_CS=255  # если 255 то работает без CS PicoBus   29  # CS PicoBus 
#define PICOBUS_BAUDRATE (1000*1000)   CS BUS  // Скорость шины picobus 

## Beep
BEEP_PIN=28  # на IN

##SDCARD
SDCARD_SPI_BUS=spi0
SDCARD_PIN_SPI0_CS=5
SDCARD_PIN_SPI0_SCK=2
SDCARD_PIN_SPI0_MOSI=3
SDCARD_PIN_SPI0_MISO=4
SDCARD_SPI_BUS_FREQ_CLK=30*1000*1000# 30 MHz

##PSRAM
PSRAM_DIV=2.0 #  1.4 // делитель частоты psram если 315 то 1.4 если меньше то 1.0

##PSRAM_BUTTER_PIN_CS=19 #это PSRAM_PIN_CS для платы murm1 psram бутерброд
PSRAM_BUTTER_PIN_CS=8 #это PSRAM_PIN_CS для платы murm1 GSP временно так надо было

##AUDIO i2s
I2S_DATA_PIN=11
I2S_CLK_BASE_PIN=12  # и 13
I2S_FREQ=75000#37500 ##96000 ##111111 Hz
PIO_I2S=pio0 # Only  pio0 !
SM_I2S=2
DEFAULT_VOLUME_I2S=50
AUDIO_BUSTER_DEFAULT=254


)
endif()
#######################################################
#    MURM1
#    
#    
#######################################################
if( ${M_BOARD} MATCHES "MURM1")
set (M_VERSION m1)# версия платы
target_compile_definitions(${PROJECT_NAME} PRIVATE
MURM1

##PicoBus
PBUS_OUT_PIN=14
PBUS_IN_PIN=15
PBUS_PIO=pio0   #only pio 0


PBUS_CS=255  # если 255 то работает без CS PicoBus   29  # CS PicoBus 



## Beep
BEEP_PIN=16  # на IN


##SDCARD
SDCARD_SPI_BUS=spi0
SDCARD_PIN_SPI0_CS=5
SDCARD_PIN_SPI0_SCK=2
SDCARD_PIN_SPI0_MOSI=3
SDCARD_PIN_SPI0_MISO=4
SDCARD_SPI_BUS_FREQ_CLK=30*1000*1000# 30 MHz

##PSRAM
PSRAM_DIV=2.0 #  1.4 // делитель частоты psram если 315 то 1.4 если меньше то 1.0
PSRAM_BUTTER_PIN_CS=19 #это PSRAM_PIN_CS для платы murm1 psram бутерброд
#PSRAM_BUTTER_PIN_CS=8 #это PSRAM_PIN_CS для платы murm1 GSP временно так надо было

##AUDIO i2s
I2S_DATA_PIN=26
I2S_CLK_BASE_PIN=27 # и 28
I2S_FREQ=75000#37500 ##96000 ##111111 Hz
PIO_I2S=pio0 # Only  pio0 !
SM_I2S=2
DEFAULT_VOLUME_I2S=50
AUDIO_BUSTER_DEFAULT=254


)
endif()
#######################################################
#    MURM2
#    PIO 0 PS/2  and I2S or HARD_AY_595
#    PIO 1 VIDEO TFT PSRAM_BOARD(SM=2)
#######################################################
if( ${M_BOARD} MATCHES "MURM2")
set (M_VERSION m2)# версия платы
target_compile_definitions(${PROJECT_NAME} PRIVATE
MURM2


##SDCARD
SDCARD_SPI_BUS=spi0
SDCARD_PIN_SPI0_CS=5
SDCARD_PIN_SPI0_SCK=6
SDCARD_PIN_SPI0_MOSI=7
SDCARD_PIN_SPI0_MISO=4

##PSRAM
PSRAM_DIV=2.0 #  1.4 // делитель частоты psram если 315 то 1.4 если меньше то 1.0
PIO_PSRAM=pio1 
SM_PSRAM=2 
# PSRAM_MUTEX=1
PSRAM_SPINLOCK=1
PSRAM_ASYNC=1
PSRAM_PIN_CS=8
PSRAM_PIN_SCK=6
PSRAM_PIN_MOSI=7
PSRAM_PIN_MISO=4

##AUDIO i2s
I2S_DATA_PIN=9
I2S_CLK_BASE_PIN=10 # и 11
I2S_FREQ=37500
PIO_I2S=pio0
SM_I2S=0
DEFAULT_VOLUME_I2S=50
AUDIO_BUSTER_DEFAULT=63


)
endif()
#######################################################
#    PI_CARD
#    PIO 0 PS/2  and I2S or HARD_AY_595
#    PIO 1 VIDEO TFT PSRAM_BOARD(SM=2)
#######################################################
#    pio 0 используется для управления GPIO 16...48
#######################################################
if( ${M_BOARD} MATCHES "PI_CARD")
set (M_VERSION pc)# версия платы
target_compile_definitions(${PROJECT_NAME} PRIVATE
PI_CARD
PICO_PIO_USE_GPIO_BASE


##SDCARD
#SDCARD_SPI_BUS spi0
SDCARD_PIN_SPI0_CS=1
SDCARD_PIN_SPI0_SCK=2
SDCARD_PIN_SPI0_MOSI=3
SDCARD_PIN_SPI0_MISO=4
##PSRAM
PSRAM_DIV=2.0 #  1.4 // делитель частоты psram если 315 то 1.4 если меньше то 1.0
PIO_PSRAM=pio1 
SM_PSRAM=2
# PSRAM_MUTEX=1
PSRAM_SPINLOCK=1
PSRAM_ASYNC=1
PSRAM_PIN_CS=0
PSRAM_PIN_SCK=6
PSRAM_PIN_MOSI=7
PSRAM_PIN_MISO=4

##AUDIO i2s
I2S_DATA_PIN=45
I2S_CLK_BASE_PIN=46 # и 47
I2S_FREQ=96000 ##111111 #48000
PIO_I2S=pio0 # использование GPIO 16...48
SM_I2S=0
DEFAULT_VOLUME_I2S=50
AUDIO_BUSTER_DEFAULT=63

)
endif()
###############################################
if( ${PICO_PLATFORM} MATCHES "rp2040")
set (P_VERSION p1)# версия чипа 
target_compile_definitions(${PROJECT_NAME} PRIVATE
        PICO_RP2040
        ## AY-3-8910 
        AY_DELTA=2
        AY_SAMPLE_RATE=9 #-9
        GSCPU_FRQ=15# 30 Мгц RP2350 18 МГц RP2040
)
endif()
#-----------------------------------------
if( ${PICO_PLATFORM} MATCHES "rp2350")
set (P_VERSION p2)# версия чипа 
target_compile_definitions(${PROJECT_NAME} PRIVATE
        PICO_RP2350
        ## AY-3-8910 
        AY_DELTA=2
        AY_SAMPLE_RATE=9#(-1000000 / (4* 48000) ) ##//-9// 
        GSCPU_FRQ=20# 30 Мгц RP2350 18 МГц RP2040
)
endif()
#-----------------------------------------

#---------------------------------------------------------------
# Конфигурация дефайнов для использования PSRAM
#---------------------------------------------------------------

    # Обработка конфигураций
    if( ${M_BOARD} MATCHES "MURM1")
            if( ${PICO_PLATFORM} MATCHES "rp2040")
            target_compile_definitions(${PROJECT_NAME} PRIVATE
           #  PSRAM_BOARD
           # NO_PSRAM
             )
            message("Конфигурация: MURM1 + PICO_RP2040 (PSRAM)")
        
        elseif( ${PICO_PLATFORM} MATCHES "rp2350")
            # Для PICO_RP2350 определяем все три варианта
            target_compile_definitions(${PROJECT_NAME} PRIVATE 

            )
            message("Конфигурация: MURM1 + PICO_RP2350 (PSRAM + PSRAM_BUTTER)")
        endif()
    
    elseif( ${M_BOARD} MATCHES "MURM2")
            if( ${PICO_PLATFORM} MATCHES "rp2040")
            target_compile_definitions(${PROJECT_NAME} PRIVATE 
 
            )
            message("Конфигурация: MURM2 + PICO_RP2040 (NO_PSRAM)")
        
        elseif( ${PICO_PLATFORM} MATCHES "rp2350")
            target_compile_definitions(${PROJECT_NAME} PRIVATE 

            )
            message("Конфигурация: MURM2 + PICO_RP2350 (PSRAM_BUTTER)")
        endif()
    endif()
########### PSRAM для PI CARD ###########################################
   if( ${M_BOARD} MATCHES "PI_CARD")
            target_compile_definitions(${PROJECT_NAME} PRIVATE 
     #PSRAM_BUTTER_OR_PSRAM
       #   PSRAM_BUTTER
            )
            message("Конфигурация: PI CARD + PICO_RP2350 (PSRAM_BUTTER)")
        endif()
#########################################################################        
#----------------------------------------------
# Формирование FW_VERSION:
if( ${M_BOARD} MATCHES "PI_CARD")
target_compile_definitions(${PROJECT_NAME} PRIVATE
  ##  FW_VERSION="${PORT_VERSION} []"
    FW_VERSION="${PORT_VERSION} [pi card]"
)
endif()

if( ${M_BOARD} MATCHES "MURMGS")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    FW_VERSION="${PORT_VERSION} [${M_VERSION}${P_VERSION}]"
)
endif()


if( ${M_BOARD} MATCHES "MURM1")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    FW_VERSION="${PORT_VERSION} [${M_VERSION}${P_VERSION}]"
)
endif()

if( ${M_BOARD} MATCHES "MURM2")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    FW_VERSION="${PORT_VERSION} [${M_VERSION}${P_VERSION}]"
)
endif()

#SET(BUILD_NAME "${PROJECT_NAME}_${PORT_VERSION}_${M_VERSION}${P_VERSION}_${VOUT}")
# Модифицируем BUILD_NAME:
SET(BUILD_NAME "${PROJECT_NAME}_${PORT_VERSION}_${M_VERSION}${P_VERSION}")


if(BUILD)
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${BUILD_NAME}")
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
endif()



target_link_libraries(${PROJECT_NAME} PRIVATE
 
        pico_runtime
        pico_stdlib
        pico_multicore
        hardware_dma
        hardware_pio
        hardware_i2c
        hardware_spi
        hardware_interp
        hardware_timer
        hardware_clocks
        hardware_pwm
        hardware_flash
 
        
    pico_stdio
    pico_stdio_usb

       audio_i2s
)

#pico_set_linker_script(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/memmap.ld")
target_link_options(${PROJECT_NAME} PRIVATE -Xlinker --print-memory-usage --data-sections)

pico_add_extra_outputs(${PROJECT_NAME})


# Включаем USB для вывода printf 1
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
