/*
 * ESP32 Compatibility Header for RP2350
 * Provides stubs for ESP32-specific macros and functions
 */
#ifndef ESP_ATTR_H
#define ESP_ATTR_H

// ESP32 memory attributes - not needed on RP2350
#define IRAM_ATTR
#define EXT_RAM_ATTR
#define DRAM_ATTR
#define WORD_ALIGNED_ATTR __attribute__((aligned(4)))

// ESP32 logging macros - redirect to printf
#include <stdio.h>
#define ESP_LOGE(tag, fmt, ...) printf("[E][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) printf("[W][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) printf("[I][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...)
#define ESP_LOGV(tag, fmt, ...)

// ESP timer
#include "pico/stdlib.h"
static inline uint64_t esp_timer_get_time(void) {
    return to_us_since_boot(get_absolute_time());
}

#endif /* ESP_ATTR_H */
