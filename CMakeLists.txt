cmake_minimum_required(VERSION 3.12)

set(this altimeter)
set(CMAKE_BUILD_TYPE Debug)

set(PICO_BOARD pico)
include(pico_sdk_import.cmake)

project(${this} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(${this})

add_subdirectory(libs/pico-sensor-lib)

target_sources(${this} PRIVATE
    main.c
    font.h
    ssd1306_i2c.h
    ssd1306_i2c.c)

target_link_libraries(${this}
    pico_stdlib
    pico_binary_info
    pico_stdio
    hardware_i2c
    hardware_gpio
    pico_multicore
    pico_util
    pico_sensor_lib)

pico_enable_stdio_usb(${this} 0)
pico_enable_stdio_uart(${this} 1)
# pico_enable_stdio_semihosting(${this} 1)

pico_add_extra_outputs(${this})
