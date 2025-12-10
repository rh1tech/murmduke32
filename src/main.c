/*
 * Duke Nukem 3D - RP2350 Port
 * Main entry point
 */
#include "pico/stdlib.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "hardware/structs/qmi.h"
#include "hardware/regs/qmi.h"
#include <stdio.h>

#include "psram_init.h"
#include "psram_data.h"
#include "psram_sections.h"
#include "board_config.h"

// Forward declaration of Duke3D main
extern int main_duke3d(int argc, char *argv[]);

// Overclock settings - adjust these for stability
// Default: 252MHz at default voltage
// Moderate OC: 300MHz at VREG_VOLTAGE_1_15
// High OC: 378MHz at VREG_VOLTAGE_1_60 (requires higher voltage like Quake port)
#define OVERCLOCK_MHZ 378
#define OVERCLOCK_VOLTAGE VREG_VOLTAGE_1_60

// Adjust flash timing for overclock (must run from RAM)
static void __no_inline_not_in_flash_func(flash_timing_for_overclock)(void) {
    const int clock_hz = OVERCLOCK_MHZ * 1000000;
    const int max_flash_freq = 133 * 1000000;  // Flash max freq
    
    int divisor = (clock_hz + max_flash_freq - 1) / max_flash_freq;
    if (divisor == 1 && clock_hz >= 166000000) {
        divisor = 2;
    }
    
    int rxdelay = divisor;
    if (clock_hz / divisor > 100000000 && clock_hz >= 166000000) {
        rxdelay += 1;
    }
    
    // Update flash timing register
    qmi_hw->m[0].timing = 0x60007000 |
                          rxdelay << QMI_M0_TIMING_RXDELAY_LSB |
                          divisor << QMI_M0_TIMING_CLKDIV_LSB;
}
#define OVERCLOCK_MHZ 378
#define OVERCLOCK_VOLTAGE VREG_VOLTAGE_1_60

int main() {
    // Increase core voltage for stable overclocking
    // WARNING: Higher voltage = more heat, ensure adequate cooling
    vreg_set_voltage(OVERCLOCK_VOLTAGE);
    sleep_ms(10);  // Wait for voltage to stabilize
    
    // Set system clock first
    // Note: HDMI needs clock to be divisible for proper pixel clock
    // 378MHz / 15 = 25.2MHz pixel clock (perfect for 640x480@60Hz)
    set_sys_clock_khz(OVERCLOCK_MHZ * 1000, true);
    
    // Adjust flash timing AFTER changing clock
    flash_timing_for_overclock();

    stdio_init_all();
    
    // Wait for USB connection for debugging
    for (int i = 0; i < 3; i++) {
        printf("murmduke3d: Starting in %d...\n", 3 - i);
        sleep_ms(1000);
    }
    
    printf("System Clock: %lu Hz\n", clock_get_hz(clk_sys));
    
    // Initialize PSRAM first (required for game data)
    printf("Initializing PSRAM...\n");
    uint psram_pin = get_psram_pin();
    psram_init(psram_pin);
    printf("PSRAM initialized on GPIO %u\n", psram_pin);
    
    // Initialize PSRAM linker sections (copy .psram_data, zero .psram_bss)
    printf("Initializing PSRAM sections...\n");
    psram_sections_init();
    printf("PSRAM sections: data=%zu bytes, bss=%zu bytes\n", 
           psram_data_size(), psram_bss_size());
    
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
