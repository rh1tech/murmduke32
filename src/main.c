/*
 * Duke Nukem 3D - RP2350 Port
 * Main entry point
 */
#include "pico/stdlib.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include <stdio.h>

#include "psram_init.h"
#include "psram_data.h"
#include "psram_sections.h"
#include "board_config.h"

// Forward declaration of Duke3D main
extern int main_duke3d(int argc, char *argv[]);

int main() {
    // Overclock support: For speeds > 252 MHz, increase voltage first
    // NOTE: Do NOT touch flash timing registers (qmi_hw->m[0].timing) - 
    // this breaks SD card SPI! The SDK handles flash timing automatically.
#if CPU_CLOCK_MHZ > 252
    vreg_disable_voltage_limit();
    vreg_set_voltage(CPU_VOLTAGE);
    sleep_ms(10);  // Wait for voltage to stabilize
#endif
    
    // Set system clock (252 MHz for HDMI, or overclocked)
    // 640x480@60Hz pixel clock is ~25.2MHz, PIO DVI needs 10x = ~252MHz
    // 378 MHz / 15 = 25.2 MHz (also works for HDMI)
    // 504 MHz / 20 = 25.2 MHz (also works for HDMI)
    set_sys_clock_khz(CPU_CLOCK_MHZ * 1000, true);

    stdio_init_all();
    
    // Brief startup delay for USB serial connection
    for (int i = 0; i < 3; i++) {
        printf("murmduke3d: Starting in %d...\n", 3 - i);
        sleep_ms(1000);
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
