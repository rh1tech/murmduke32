/*
 * RP2350 Compatibility Header (based on ESP32 compat)
 */
#ifndef esp32_compat_h_
#define esp32_compat_h_
    
#include "stdint.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

// For dirent support, include our compatibility header
// This MUST be included before any FatFS headers to properly redefine DIR
#include "dirent.h"

// Forward declare psram_malloc for use by kkmalloc
extern void *psram_malloc(size_t size);

// Define BYTE_ORDER for RP2350 (ARM Cortex-M33, little-endian)
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#define HAVE_ASSERT_H 1
#define PLATFORM_SUPPORTS_SDL

// Memory allocation macros - use PSRAM for large allocations
#define kkfree(x) /* PSRAM doesn't support free */
#define kkmalloc(x) psram_malloc(x)
#define kmalloc(x) psram_malloc(x)

// String comparison
#define strcmpi(x, y) strcasecmp(x, y)
#define stricmp strcasecmp

// Types
#define __int64 int64_t

// Math macros
#ifndef min
#define min(x,y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef max
#define max(x,y) (((x) > (y)) ? (x) : (y))
#endif

#define FP_OFF(x) ((int32_t) (x))
#define O_BINARY 0

// Network stubs
#define USER_DUMMY_NETWORK 1
#define STUB_NETWORKING 1

#define STUBBED(x) printf("STUB: %s (%s, %s:%d)\n",x,__FUNCTION__,__FILE__,__LINE__)

#define getch getchar
#define printchrasm(x,y,ch) printf("%c", (uint8_t ) (ch & 0xFF))

// Memory available (we have 8MB PSRAM)
#define Z_AvailHeap() ((8 * 1024) * 1024)

// Platform defines
#define PLATFORM_RP2350 1

#define MAX_PATH 255

// ESP32-specific function stubs
static inline void spi_lcd_clear(void) {
    // No SPI LCD on RP2350 - using HDMI
}

static inline void SDL_InitSD(void) {
    // SD card init is done in SDL_Init
}

// mkdir compatibility - On embedded, just call with default mode
// This is redefined to call our FatFS-based mkdir
int fatfs_mkdir(const char *path);
#define mkdir(path) fatfs_mkdir(path)

// Directory finding structures
struct find_t
{
    void *dir;
    char pattern[MAX_PATH];
    char name[MAX_PATH];
};

int _dos_findfirst(char *filename, int x, struct find_t *f);
int _dos_findnext(struct find_t *f);

struct dosdate_t
{
    uint8_t day;
    uint8_t month;
    unsigned int year;
    uint8_t dayofweek;
};

void _dos_getdate(struct dosdate_t *date);

#define PATH_SEP_CHAR '/'
#define PATH_SEP_STR  "/"
#define ROOTDIR       "/"
#define CURDIR        "./"

#endif
