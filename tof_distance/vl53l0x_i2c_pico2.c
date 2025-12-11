// This file is part of VL53L0X library
// It provides PICO I2C specific implementation and replaces code in platform/vl53l0x_i2c_platform.c
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "vl53l0x_i2c_platform.h"

#ifndef VL53L0X_PICO_I2C_INSTANCE
#define VL53L0X_PICO_I2C_INSTANCE i2c0
#endif

#ifndef VL53L0X_PICO_I2C_SDA_PIN
#define VL53L0X_PICO_I2C_SDA_PIN 4
#endif

#ifndef VL53L0X_PICO_I2C_SCL_PIN
#define VL53L0X_PICO_I2C_SCL_PIN 5
#endif

#ifndef VL53L0X_PICO_DEFAULT_KHZ
#define VL53L0X_PICO_DEFAULT_KHZ 400
#endif

#ifndef VL53L0X_PICO_ADDR_IS_8BIT
#define VL53L0X_PICO_ADDR_IS_8BIT 0
#endif

#define STATUS_OK 0
#define STATUS_FAIL 1

static bool s_i2c_initialized = false;

static inline uint8_t vl53l0x_normalize_address(uint8_t address) {
#if VL53L0X_PICO_ADDR_IS_8BIT
    return (uint8_t)(address >> 1);
#else
    return (uint8_t)(address & 0x7F);
#endif
}

static inline bool vl53l0x_validate_count(int32_t count) {
    return (count >= 0) && (count <= COMMS_BUFFER_SIZE);
}

static inline void vl53l0x_configure_pins(void) {
    int sda_pin =VL53L0X_PICO_I2C_SDA_PIN;
    int scl_pin = VL53L0X_PICO_I2C_SCL_PIN;
    printf("sda_pin=%d, scl_pin=%d\n", sda_pin, scl_pin);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
}

int32_t VL53L0X_comms_initialise(uint8_t comms_type, uint16_t comms_speed_khz) {
    if (comms_type != I2C) {
        return STATUS_FAIL;
    }

    if (comms_speed_khz == 0) {
        comms_speed_khz = VL53L0X_PICO_DEFAULT_KHZ;
    }

    i2c_init(VL53L0X_PICO_I2C_INSTANCE, comms_speed_khz * 1000u);
    vl53l0x_configure_pins();
    s_i2c_initialized = true;
    return STATUS_OK;
}

int32_t VL53L0X_comms_close(void) {
    if (!s_i2c_initialized) {
        return STATUS_OK;
    }

    i2c_deinit(VL53L0X_PICO_I2C_INSTANCE);
    s_i2c_initialized = false;
    return STATUS_OK;
}

#ifdef VL53L0X_PICO_XSHUT_GPIO
static void vl53l0x_init_xshut(void) {
    static bool init_done = false;
    if (!init_done) {
        gpio_init(VL53L0X_PICO_XSHUT_GPIO);
        gpio_set_dir(VL53L0X_PICO_XSHUT_GPIO, GPIO_OUT);
        gpio_put(VL53L0X_PICO_XSHUT_GPIO, 1);
        init_done = true;
    }
}
#endif

int32_t VL53L0X_cycle_power(void) {
#ifdef VL53L0X_PICO_XSHUT_GPIO
    vl53l0x_init_xshut();
    gpio_put(VL53L0X_PICO_XSHUT_GPIO, 0);
    sleep_ms(10);
    gpio_put(VL53L0X_PICO_XSHUT_GPIO, 1);
    sleep_ms(10);
    return STATUS_OK;
#else
    return STATUS_FAIL;
#endif
}

int32_t VL53L0X_write_multi(uint8_t address, uint8_t index, uint8_t *pdata, int32_t count) {
    if (!vl53l0x_validate_count(count)) {
        return STATUS_FAIL;
    }

    uint8_t payload[COMMS_BUFFER_SIZE + 1];
    payload[0] = index;
    if (count > 0) {
        memcpy(&payload[1], pdata, (size_t)count);
    }

    int expected = count + 1;
    uint8_t addr = vl53l0x_normalize_address(address);
    int written = i2c_write_blocking(VL53L0X_PICO_I2C_INSTANCE,
                                     addr,
                                     payload,
                                     (size_t)expected,
                                     false);
    return (written == expected) ? STATUS_OK : STATUS_FAIL;
}

int32_t VL53L0X_read_multi(uint8_t address, uint8_t index, uint8_t *pdata, int32_t count) {
    if (!vl53l0x_validate_count(count)) {
        return STATUS_FAIL;
    }

    uint8_t dev_addr = vl53l0x_normalize_address(address);
    int rc = i2c_write_blocking(VL53L0X_PICO_I2C_INSTANCE, dev_addr, &index, 1, true);
    if (rc != 1) {
        return STATUS_FAIL;
    }

    if (count == 0) {
        return STATUS_OK;
    }

    int read = i2c_read_blocking(VL53L0X_PICO_I2C_INSTANCE, dev_addr, pdata, (size_t)count, false);
    return (read == count) ? STATUS_OK : STATUS_FAIL;
}

int32_t VL53L0X_write_byte(uint8_t address, uint8_t index, uint8_t data) {
    return VL53L0X_write_multi(address, index, &data, 1);
}

int32_t VL53L0X_write_word(uint8_t address, uint8_t index, uint16_t data) {
    uint8_t buffer[BYTES_PER_WORD];
    buffer[0] = (uint8_t)(data >> 8);
    buffer[1] = (uint8_t)(data & 0x00FF);
    return VL53L0X_write_multi(address, index, buffer, BYTES_PER_WORD);
}

int32_t VL53L0X_write_dword(uint8_t address, uint8_t index, uint32_t data) {
    uint8_t buffer[BYTES_PER_DWORD];
    buffer[0] = (uint8_t)(data >> 24);
    buffer[1] = (uint8_t)((data >> 16) & 0xFF);
    buffer[2] = (uint8_t)((data >> 8) & 0xFF);
    buffer[3] = (uint8_t)(data & 0xFF);
    return VL53L0X_write_multi(address, index, buffer, BYTES_PER_DWORD);
}

int32_t VL53L0X_read_byte(uint8_t address, uint8_t index, uint8_t *pdata) {
    return VL53L0X_read_multi(address, index, pdata, 1);
}

int32_t VL53L0X_read_word(uint8_t address, uint8_t index, uint16_t *pdata) {
    uint8_t buffer[BYTES_PER_WORD];
    int32_t status = VL53L0X_read_multi(address, index, buffer, BYTES_PER_WORD);
    if (status != STATUS_OK) {
        return status;
    }
    *pdata = (uint16_t)((buffer[0] << 8) | buffer[1]);
    return STATUS_OK;
}

int32_t VL53L0X_read_dword(uint8_t address, uint8_t index, uint32_t *pdata) {
    uint8_t buffer[BYTES_PER_DWORD];
    int32_t status = VL53L0X_read_multi(address, index, buffer, BYTES_PER_DWORD);
    if (status != STATUS_OK) {
        return status;
    }
    *pdata = ((uint32_t)buffer[0] << 24) |
             ((uint32_t)buffer[1] << 16) |
             ((uint32_t)buffer[2] << 8) |
             (uint32_t)buffer[3];
    return STATUS_OK;
}

int32_t VL53L0X_platform_wait_us(int32_t wait_us) {
    if (wait_us <= 0) {
        return STATUS_OK;
    }
    sleep_us((uint64_t)wait_us);
    return STATUS_OK;
}

int32_t VL53L0X_wait_ms(int32_t wait_ms) {
    if (wait_ms <= 0) {
        return STATUS_OK;
    }
    sleep_ms((uint32_t)wait_ms);
    return STATUS_OK;
}

int32_t VL53L0X_set_gpio(uint8_t level) {
#ifdef VL53L0X_PICO_XSHUT_GPIO
    vl53l0x_init_xshut();
    gpio_put(VL53L0X_PICO_XSHUT_GPIO, level ? 1 : 0);
    return STATUS_OK;
#else
    (void)level;
    return STATUS_FAIL;
#endif
}

int32_t VL53L0X_get_gpio(uint8_t *plevel) {
#ifdef VL53L0X_PICO_XSHUT_GPIO
    vl53l0x_init_xshut();
    gpio_set_dir(VL53L0X_PICO_XSHUT_GPIO, GPIO_IN);
    *plevel = (uint8_t)gpio_get(VL53L0X_PICO_XSHUT_GPIO);
    gpio_set_dir(VL53L0X_PICO_XSHUT_GPIO, GPIO_OUT);
    return STATUS_OK;
#else
    if (plevel) {
        *plevel = 0;
    }
    return STATUS_FAIL;
#endif
}

int32_t VL53L0X_release_gpio(void) {
#ifdef VL53L0X_PICO_XSHUT_GPIO
    vl53l0x_init_xshut();
    gpio_set_dir(VL53L0X_PICO_XSHUT_GPIO, GPIO_IN);
    gpio_pull_up(VL53L0X_PICO_XSHUT_GPIO);
    return STATUS_OK;
#else
    return STATUS_FAIL;
#endif
}

int32_t VL53L0X_get_timer_frequency(int32_t *ptimer_freq_hz) {
    if (ptimer_freq_hz == NULL) {
        return STATUS_FAIL;
    }
    *ptimer_freq_hz = 1000000; // microsecond counter
    return STATUS_OK;
}

int32_t VL53L0X_get_timer_value(int32_t *ptimer_count) {
    if (ptimer_count == NULL) {
        return STATUS_FAIL;
    }
    *ptimer_count = (int32_t)(time_us_64() & 0x7FFFFFFF);
    return STATUS_OK;
}
