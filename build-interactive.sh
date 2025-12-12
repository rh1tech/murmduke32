#!/bin/bash
#
# Interactive Build Script for murmduke3d
# Duke Nukem 3D for RP2350 with PSRAM
#

set -e

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║         murmduke3d - Duke Nukem 3D for RP2350                 ║"
echo "║                 Interactive Build Script                      ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# ─────────────────────────────────────────────────────────────────────
# Board Variant Selection
# ─────────────────────────────────────────────────────────────────────
echo "┌─────────────────────────────────────────────────────────────────┐"
echo "│ 1. Select Board Variant                                        │"
echo "├─────────────────────────────────────────────────────────────────┤"
echo "│  M1 - Murmulator M1 Layout                                     │"
echo "│       HDMI: GPIO 6-13, SD: GPIO 2-5, PS/2: GPIO 0-1            │"
echo "│       I2S: GPIO 26-28, PSRAM: GPIO 19 (RP2350A)                │"
echo "│                                                                 │"
echo "│  M2 - Murmulator M2 Layout                                     │"
echo "│       HDMI: GPIO 12-19, SD: GPIO 4-7, PS/2: GPIO 2-3           │"
echo "│       I2S: GPIO 9-11, PSRAM: GPIO 8 (RP2350A)                  │"
echo "│                                                                 │"
echo "│  Note: RP2350B (Pico Plus 2) always uses GPIO 47 for PSRAM    │"
echo "└─────────────────────────────────────────────────────────────────┘"
echo ""

while true; do
    read -p "Select board variant [M1/M2] (default: M1): " BOARD_CHOICE
    BOARD_CHOICE=${BOARD_CHOICE:-M1}
    BOARD_CHOICE=$(echo "$BOARD_CHOICE" | tr '[:lower:]' '[:upper:]')
    
    if [[ "$BOARD_CHOICE" == "M1" || "$BOARD_CHOICE" == "M2" ]]; then
        break
    else
        echo "Invalid choice. Please enter M1 or M2."
    fi
done

echo "→ Selected: $BOARD_CHOICE"
echo ""

# ─────────────────────────────────────────────────────────────────────
# CPU Speed Selection (based on Quake port proven options)
# ─────────────────────────────────────────────────────────────────────
echo "┌─────────────────────────────────────────────────────────────────┐"
echo "│ 2. Select CPU Clock Speed                                      │"
echo "├─────────────────────────────────────────────────────────────────┤"
echo "│  1) 252 MHz - Default, no overclock (1.10V)                    │"
echo "│  2) 378 MHz - Moderate overclock (1.60V)                       │"
echo "│  3) 504 MHz - High overclock (1.60V)                           │"
echo "│                                                                 │"
echo "│  ⚠️  Overclocks use 1.60V and generate more heat!              │"
echo "│  These speeds are proven to work with HDMI in Quake port.      │"
echo "└─────────────────────────────────────────────────────────────────┘"
echo ""

while true; do
    read -p "Select CPU speed [1-3] (default: 2 for 378 MHz): " CPU_CHOICE
    CPU_CHOICE=${CPU_CHOICE:-2}
    
    case "$CPU_CHOICE" in
        1) CPU_SPEED=252; break ;;
        2) CPU_SPEED=378; break ;;
        3) CPU_SPEED=504; break ;;
        *) echo "Invalid choice. Please enter 1-3." ;;
    esac
done

echo "→ Selected: $CPU_SPEED MHz"
echo ""

# ─────────────────────────────────────────────────────────────────────
# PSRAM Speed Selection (based on Quake port proven options)
# ─────────────────────────────────────────────────────────────────────
echo "┌─────────────────────────────────────────────────────────────────┐"
echo "│ 3. Select PSRAM Clock Speed                                    │"
echo "├─────────────────────────────────────────────────────────────────┤"
echo "│  1)  84 MHz - Safe, conservative                               │"
echo "│  2) 100 MHz - Quake default                                    │"
echo "│  3) 133 MHz - Fast                                             │"
echo "│  4) 168 MHz - Aggressive OC (exactly 504/3)                    │"
echo "│                                                                 │"
echo "│  Actual PSRAM clock = CPU_clock / ceil(CPU_clock / max_freq)   │"
echo "│  Examples at 504 MHz CPU:                                       │"
echo "│    100 MHz max → 504/6 = 84 MHz actual                         │"
echo "│    133 MHz max → 504/4 = 126 MHz actual                        │"
echo "│    168 MHz max → 504/3 = 168 MHz actual (aggressive!)          │"
echo "└─────────────────────────────────────────────────────────────────┘"
echo ""

while true; do
    read -p "Select PSRAM speed [1-4] (default: 2 for 100 MHz): " PSRAM_CHOICE
    PSRAM_CHOICE=${PSRAM_CHOICE:-2}
    
    case "$PSRAM_CHOICE" in
        1) PSRAM_SPEED=84; break ;;
        2) PSRAM_SPEED=100; break ;;
        3) PSRAM_SPEED=133; break ;;
        4) PSRAM_SPEED=168; break ;;
        *) echo "Invalid choice. Please enter 1-4." ;;
    esac
done

# Calculate actual PSRAM speed
DIVISOR=$(( (CPU_SPEED + PSRAM_SPEED - 1) / PSRAM_SPEED ))
ACTUAL_PSRAM=$(( CPU_SPEED / DIVISOR ))

echo "→ Selected: $PSRAM_SPEED MHz max (actual: ~$ACTUAL_PSRAM MHz at $CPU_SPEED MHz CPU)"
echo ""

# ─────────────────────────────────────────────────────────────────────
# Build Summary
# ─────────────────────────────────────────────────────────────────────
echo "┌─────────────────────────────────────────────────────────────────┐"
echo "│ Build Configuration Summary                                    │"
echo "├─────────────────────────────────────────────────────────────────┤"
printf "│  Board Variant:  %-46s │\n" "$BOARD_CHOICE"
printf "│  CPU Speed:      %-46s │\n" "$CPU_SPEED MHz"
printf "│  PSRAM Speed:    %-46s │\n" "$PSRAM_SPEED MHz max (~$ACTUAL_PSRAM MHz actual)"
echo "└─────────────────────────────────────────────────────────────────┘"
echo ""

read -p "Proceed with build? [Y/n]: " CONFIRM
CONFIRM=${CONFIRM:-Y}
if [[ ! "$CONFIRM" =~ ^[Yy]$ ]]; then
    echo "Build cancelled."
    exit 0
fi

echo ""
echo "═══════════════════════════════════════════════════════════════════"
echo " Starting build..."
echo "═══════════════════════════════════════════════════════════════════"
echo ""

# ─────────────────────────────────────────────────────────────────────
# Clean and Configure
# ─────────────────────────────────────────────────────────────────────
cd "$(dirname "$0")"

# Remove old build directory for clean build
if [ -d "build" ]; then
    echo "Cleaning previous build..."
    rm -rf build
fi

mkdir -p build
cd build

echo "Configuring CMake..."
cmake .. \
    -DBOARD_VARIANT="$BOARD_CHOICE" \
    -DCPU_SPEED="$CPU_SPEED" \
    -DPSRAM_SPEED="$PSRAM_SPEED" \
    -DCMAKE_BUILD_TYPE=Release

echo ""
echo "Building..."
make -j$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

echo ""
echo "═══════════════════════════════════════════════════════════════════"
echo " Build Complete!"
echo "═══════════════════════════════════════════════════════════════════"
echo ""
echo "Output files:"
echo "  - build/murmduke3d.uf2"
echo "  - build/murmduke3d.elf"
echo ""
echo "To flash:"
echo "  1. Hold BOOTSEL button and connect Pico to USB"
echo "  2. Copy build/murmduke3d.uf2 to the RPI-RP2 drive"
echo ""
echo "  Or use picotool:"
echo "    cd build && picotool load -f ./murmduke3d.elf && picotool reboot -f"
echo ""
