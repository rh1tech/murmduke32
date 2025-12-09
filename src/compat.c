/*
 * RP2350 Compatibility Functions
 * 
 * IMPORTANT: This file includes ff.h FIRST to get FatFS types,
 * then defines POSIX-compatible directory functions that wrap FatFS.
 */
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// Include FatFS FIRST to get its DIR definition
#include "ff.h"

// Save FatFS FFDIR type
// FatFS now uses FFDIR (we renamed it from DIR)

// Now define our POSIX-compatible structures
#ifndef MAXNAMLEN
#define MAXNAMLEN 255
#endif

struct dirent {
    char d_name[MAXNAMLEN + 1];
    unsigned char d_type;
};

#define DT_UNKNOWN  0
#define DT_REG      8
#define DT_DIR      4

// Our POSIX-style DIR structure
typedef struct POSIX_DIR_STRUCT {
    FFDIR fatfs_dir;        // FatFS directory object
    struct dirent entry;    // Entry buffer
    char path[256];
    int is_open;
} DIR;

// Directory functions
DIR *opendir(const char *name) {
    DIR *d = (DIR *)malloc(sizeof(DIR));
    if (!d) return NULL;
    
    FRESULT res = f_opendir(&d->fatfs_dir, name);
    if (res != FR_OK) {
        free(d);
        return NULL;
    }
    
    d->is_open = 1;
    strncpy(d->path, name, sizeof(d->path) - 1);
    d->path[sizeof(d->path) - 1] = '\0';
    return d;
}

struct dirent *readdir(DIR *dirp) {
    if (!dirp || !dirp->is_open) return NULL;
    
    FILINFO fno;
    FRESULT res = f_readdir(&dirp->fatfs_dir, &fno);
    
    if (res != FR_OK || fno.fname[0] == 0) {
        return NULL;
    }
    
    strncpy(dirp->entry.d_name, fno.fname, MAXNAMLEN);
    dirp->entry.d_name[MAXNAMLEN] = '\0';
    dirp->entry.d_type = (fno.fattrib & AM_DIR) ? DT_DIR : DT_REG;
    
    return &dirp->entry;
}

int closedir(DIR *dirp) {
    if (!dirp) return -1;
    
    if (dirp->is_open) {
        f_closedir(&dirp->fatfs_dir);
    }
    free(dirp);
    return 0;
}

// Include esp32_compat.h for struct definitions used below
// Note: esp32_compat.h includes dirent.h which also defines DIR,
// but that's okay since we've already defined everything here.
#define _DIRENT_H_COMPAT  // Prevent dirent.h from redefining
#include "esp32_compat.h"

// Directory finding (uses our dirent implementation)
int _dos_findfirst(char *filename, int x, struct find_t *f) {
    // Stub - return error (no file found)
    return -1;
}

int _dos_findnext(struct find_t *f) {
    // Stub - return error (no more files)
    return -1;
}

void _dos_getdate(struct dosdate_t *date) {
    if (date) {
        // Return a fixed date for now
        date->day = 1;
        date->month = 1;
        date->year = 2025;
        date->dayofweek = 0;
    }
}

// Platform swap functions (little-endian, so no-op)
uint16_t _swap16(uint16_t D) {
    return ((D << 8) | (D >> 8));
}

unsigned int _swap32(unsigned int D) {
    return ((D << 24) | ((D << 8) & 0x00FF0000) | 
            ((D >> 8) & 0x0000FF00) | (D >> 24));
}
