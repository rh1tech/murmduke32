/*
 * Duke Nukem 3D - RP2350 Platform Adapter
 * Bridges Duke3D engine to RP2350 hardware
 */
#include "board_config.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "HDMI.h"
#include "psram_init.h"
#include "psram_allocator.h"
#include "sdcard.h"
#include "ff.h"
#include "ps2kbd_wrapper.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// ESP32 compatibility defines
#define EXT_RAM_ATTR
#define IRAM_ATTR

// ESP timer compatibility
uint64_t esp_timer_get_time(void) {
    return to_us_since_boot(get_absolute_time());
}

// Memory allocation using PSRAM
void *ext_malloc(size_t size) {
    return psram_malloc(size);
}

void *ext_calloc(size_t nmemb, size_t size) {
    void *ptr = psram_malloc(nmemb * size);
    if (ptr) {
        memset(ptr, 0, nmemb * size);
    }
    return ptr;
}

void ext_free(void *ptr) {
    psram_free(ptr);
}

// Build Engine cache memory from PSRAM
void *cache_malloc(size_t size) {
    return psram_malloc(size);
}

// Initialize the RP2350 platform for Duke3D
void duke3d_platform_init(void) {
    printf("duke3d_platform_init: Setting up RP2350...\n");
    
    // PSRAM should already be initialized by SDL_Init
    // Just verify it's working
    void *test = psram_malloc(1024);
    if (test) {
        psram_free(test);
        printf("duke3d_platform_init: PSRAM OK\n");
    } else {
        printf("duke3d_platform_init: WARNING - PSRAM not available!\n");
    }
}

// Error handling is provided by global.c - Error(int errorType, char *error, ...)

// Timing functions for the engine
static uint32_t timer_start;

void timer_init(void) {
    timer_start = to_ms_since_boot(get_absolute_time());
}

uint32_t timer_get_ms(void) {
    return to_ms_since_boot(get_absolute_time()) - timer_start;
}

// Total clock (game ticks at ~120Hz)
volatile int32_t totalclock = 0;

void timerhandler(void) {
    totalclock++;
}

// File path handling - Duke3D uses "duke3d/" subfolder
char game_dir[512] = "/duke3d";

void set_game_dir(const char *dir) {
    strncpy(game_dir, dir, sizeof(game_dir) - 1);
    game_dir[sizeof(game_dir) - 1] = '\0';
}

const char *get_game_dir(void) {
    return game_dir;
}
