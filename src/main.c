/*
 * Duke Nukem 3D - RP2350 Port
 * Main entry point
 */
#include "pico/stdlib.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "hardware/structs/qmi.h"
#include <stdio.h>

#include "psram_init.h"
#include "psram_data.h"
#include "psram_sections.h"
#include "board_config.h"

// Forward declaration of Duke3D main
extern int main_duke3d(int argc, char *argv[]);

// Flash timing configuration for overclocking
// Must be called BEFORE changing system clock
// Based on Quake port approach
#define FLASH_MAX_FREQ_MHZ 88

static void __no_inline_not_in_flash_func(set_flash_timings)(int cpu_mhz) {
    const int clock_hz = cpu_mhz * 1000000;
    const int max_flash_freq = FLASH_MAX_FREQ_MHZ * 1000000;
    
    int divisor = (clock_hz + max_flash_freq - (max_flash_freq >> 4) - 1) / max_flash_freq;
    if (divisor == 1 && clock_hz >= 166000000) {
        divisor = 2;
    }
    
    int rxdelay = divisor;
    if (clock_hz / divisor > 100000000 && clock_hz >= 166000000) {
        rxdelay += 1;
    }
    
    qmi_hw->m[0].timing = 0x60007000 |
                        rxdelay << QMI_M0_TIMING_RXDELAY_LSB |
                        divisor << QMI_M0_TIMING_CLKDIV_LSB;
}

int main() {
    // Overclock support: For speeds > 252 MHz, increase voltage first
    // Based on Quake port initialization sequence
#if CPU_CLOCK_MHZ > 252
    vreg_disable_voltage_limit();
    vreg_set_voltage(CPU_VOLTAGE);
    set_flash_timings(CPU_CLOCK_MHZ);  // Set flash timings BEFORE clock change
    sleep_ms(100);  // Wait for voltage and timings to stabilize
#endif
    
    // Set system clock (252 MHz for HDMI, or overclocked)
    // 640x480@60Hz pixel clock is ~25.2MHz, PIO DVI needs 10x = ~252MHz
    // 378 MHz / 15 = 25.2 MHz (also works for HDMI)
    // 504 MHz / 20 = 25.2 MHz (also works for HDMI)
    if (!set_sys_clock_khz(CPU_CLOCK_MHZ * 1000, false)) {
        // Fallback to safe clock if requested speed fails
        set_sys_clock_khz(252 * 1000, true);
    }

    stdio_init_all();
    
    // Brief startup delay for USB serial connection
    for (int i = 0; i < 3; i++) {
        sleep_ms(500);
    }
    
    printf("System Clock: %lu Hz\n", clock_get_hz(clk_sys));
    
    // Initialize PSRAM (required for game data)
    uint psram_pin = get_psram_pin();
    psram_init(psram_pin);
    
    // Initialize PSRAM linker sections (copy .psram_data, zero .psram_bss)
    psram_sections_init();
    
    // Allocate game data arrays in PSRAM
    psram_data_init();
    
    printf("Starting Duke Nukem 3D...\n");

    // Launch Duke3D with music enabled
    char *argv[] = {"duke3d", NULL};
    main_duke3d(1, argv);

    // Should never reach here
    while (1) {
        tight_loop_contents();
    }

    return 0;
}
