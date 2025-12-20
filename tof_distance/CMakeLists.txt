# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(tof_distance C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# VL53L0X API static library
set(VL53L0X_API_ROOT ${CMAKE_CURRENT_LIST_DIR}/VL53L0X_1.0.4/Api)
set(VL53L0X_API_SOURCES
    ${VL53L0X_API_ROOT}/core/src/vl53l0x_api.c
    ${VL53L0X_API_ROOT}/core/src/vl53l0x_api_calibration.c
    ${VL53L0X_API_ROOT}/core/src/vl53l0x_api_core.c
    ${VL53L0X_API_ROOT}/core/src/vl53l0x_api_ranging.c
    ${VL53L0X_API_ROOT}/core/src/vl53l0x_api_strings.c
    ${VL53L0X_API_ROOT}/platform/src/vl53l0x_platform.c

)

add_library(vl53l0x_api STATIC ${VL53L0X_API_SOURCES})

target_include_directories(vl53l0x_api
     PRIVATE
     ${VL53L0X_API_ROOT}/core/inc
     ${VL53L0X_API_ROOT}/platform/inc
 )

# target_link_libraries(vl53l0x_api
#     PUBLIC
#     pico_stdlib
#     hardware_i2c
#     hardware_gpio
# )

# target_compile_definitions(vl53l0x_api
#     PUBLIC
#         VL53L0X_PICO_I2C_SDA_PIN=4
#         VL53L0X_PICO_I2C_SCL_PIN=5
#         VL53L0X_PICO_I2C_INSTANCE=i2c0
# )

# Add executable. Default name is the project name, version 0.1

#add_executable(tof_distance tof_distance.c vl53l0x_i2c_pico2.c async_task.c led_lib.c)

# uncomment this to run unit tests
add_executable(tof_distance unit-test.c event_system.c)

pico_set_program_name(tof_distance "tof_distance")
pico_set_program_version(tof_distance "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(tof_distance 1)
pico_enable_stdio_usb(tof_distance 0)

# Link libraries required by the application
target_link_libraries(tof_distance
        pico_stdlib
        hardware_i2c
        vl53l0x_api
    )

target_include_directories(tof_distance PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/VL53L0X_1.0.4/Api/core/inc
    ${CMAKE_CURRENT_LIST_DIR}/VL53L0X_1.0.4/Api/platform/inc
)

pico_add_extra_outputs(tof_distance)

