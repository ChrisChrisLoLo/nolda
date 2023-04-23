cmake_minimum_required(VERSION 3.13)

set(ENV{PICO_SDK_PATH} ../sp-pico-sdk)

# I janked up the oscillator - needs custom build 
if(NOT PICO_BOARD)
    set(PICO_BOARD sporewoh_noldacore)
endif()

# Set RTOS port
if(NOT FREERTOS_PORT)
    set(FREERTOS_PORT GCC_RP2040)
endif()

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(noldaCore)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_executable(main
main.c
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(main pico_stdlib freertos)

# create map/bin/hex/uf2 file in addition to ELF
pico_add_extra_outputs(main)

#add_definitions(-DPICO_BOARD=sporewoh_noldacore)
