/*
 * FatFS stdio wrapper for Duke3D on RP2350
 * Wraps both stdio (fopen, etc) and POSIX (open, etc) file operations
 */
#include "ff.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

// Map FILE*/int fd to FIL* for FatFS
#define MAX_OPEN_FILES 16
#define FD_OFFSET 10  // Start file descriptors above stdin/stdout/stderr

typedef struct {
    FIL fil;
    int in_use;
    int is_posix;  // 1 if opened via open(), 0 if via fopen()
} file_handle_t;

static file_handle_t file_handles[MAX_OPEN_FILES];

// Find a free file handle
static int find_free_handle(void) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!file_handles[i].in_use) return i;
    }
    return -1;
}

// Convert FIL* back to FILE* (just cast the address)
static FILE* fil_to_file(FIL *fil) {
    return (FILE*)fil;
}

// Convert FILE* to FIL*
static FIL* file_to_fil(FILE *fp) {
    return (FIL*)fp;
}

// Convert fd to handle index
static int fd_to_handle(int fd) {
    int idx = fd - FD_OFFSET;
    if (idx >= 0 && idx < MAX_OPEN_FILES && file_handles[idx].in_use) {
        return idx;
    }
    return -1;
}

// ============= POSIX Functions (open, close, read, write, lseek) =============

int __wrap_open(const char *pathname, int flags, ...) {
    BYTE fatfs_mode = 0;
    FRESULT fr;
    int idx;
    
    idx = find_free_handle();
    if (idx < 0) {
        errno = ENOMEM;
        return -1;
    }
    
    // Parse flags
    if ((flags & O_ACCMODE) == O_RDONLY) {
        fatfs_mode = FA_READ;
    } else if ((flags & O_ACCMODE) == O_WRONLY) {
        fatfs_mode = FA_WRITE;
        if (flags & O_CREAT) {
            if (flags & O_TRUNC) {
                fatfs_mode |= FA_CREATE_ALWAYS;
            } else {
                fatfs_mode |= FA_OPEN_ALWAYS;
            }
        }
    } else if ((flags & O_ACCMODE) == O_RDWR) {
        fatfs_mode = FA_READ | FA_WRITE;
        if (flags & O_CREAT) {
            if (flags & O_TRUNC) {
                fatfs_mode |= FA_CREATE_ALWAYS;
            } else {
                fatfs_mode |= FA_OPEN_ALWAYS;
            }
        }
    }
    
    if (flags & O_APPEND) {
        fatfs_mode |= FA_OPEN_APPEND;
    }
    
    // Try opening the file
    fr = f_open(&file_handles[idx].fil, pathname, fatfs_mode);
    
    if (fr != FR_OK) {
        errno = (fr == FR_NO_FILE || fr == FR_NO_PATH) ? ENOENT : EIO;
        return -1;
    }
    
    file_handles[idx].in_use = 1;
    file_handles[idx].is_posix = 1;
    
    return idx + FD_OFFSET;
}

int __wrap_close(int fd) {
    int idx = fd_to_handle(fd);
    
    if (idx < 0) {
        errno = EBADF;
        return -1;
    }
    
    f_close(&file_handles[idx].fil);
    file_handles[idx].in_use = 0;
    return 0;
}

ssize_t __wrap_read(int fd, void *buf, size_t count) {
    int idx = fd_to_handle(fd);
    UINT br;
    FRESULT fr;
    
    if (idx < 0) {
        // Not a FatFS file, might be stdin
        errno = EBADF;
        return -1;
    }
    
    fr = f_read(&file_handles[idx].fil, buf, count, &br);
    if (fr != FR_OK) {
        errno = EIO;
        return -1;
    }
    
    return (ssize_t)br;
}

ssize_t __wrap_write(int fd, const void *buf, size_t count) {
    int idx = fd_to_handle(fd);
    UINT bw;
    FRESULT fr;
    
    if (idx < 0) {
        // Not a FatFS file, might be stdout/stderr - pass through to real
        // For now, just write to printf
        printf("%.*s", (int)count, (const char*)buf);
        return count;
    }
    
    fr = f_write(&file_handles[idx].fil, buf, count, &bw);
    if (fr != FR_OK) {
        errno = EIO;
        return -1;
    }
    
    return (ssize_t)bw;
}

off_t __wrap_lseek(int fd, off_t offset, int whence) {
    int idx = fd_to_handle(fd);
    FSIZE_t pos;
    
    if (idx < 0) {
        errno = EBADF;
        return (off_t)-1;
    }
    
    FIL *fil = &file_handles[idx].fil;
    
    switch (whence) {
        case SEEK_SET:
            pos = offset;
            break;
        case SEEK_CUR:
            pos = f_tell(fil) + offset;
            break;
        case SEEK_END:
            pos = f_size(fil) + offset;
            break;
        default:
            errno = EINVAL;
            return (off_t)-1;
    }
    
    if (f_lseek(fil, pos) != FR_OK) {
        errno = EIO;
        return (off_t)-1;
    }
    
    return (off_t)pos;
}

// filelength - get file size from fd
long filelength(int fd) {
    int idx = fd_to_handle(fd);
    if (idx < 0) return 0;
    return (long)f_size(&file_handles[idx].fil);
}

// ============= STDIO Functions (fopen, fclose, fread, etc) =============

FILE *__wrap_fopen(const char *filename, const char *mode) {
    BYTE fatfs_mode = 0;
    FRESULT fr;
    int idx;
    
    idx = find_free_handle();
    if (idx < 0) {
        printf("fopen: no free handle for %s\n", filename);
        errno = ENOMEM;
        return NULL;
    }
    
    // Parse mode
    if (strchr(mode, 'r')) {
        fatfs_mode = FA_READ;
        if (strchr(mode, '+')) fatfs_mode |= FA_WRITE;
    } else if (strchr(mode, 'w')) {
        fatfs_mode = FA_WRITE | FA_CREATE_ALWAYS;
        if (strchr(mode, '+')) fatfs_mode |= FA_READ;
    } else if (strchr(mode, 'a')) {
        fatfs_mode = FA_WRITE | FA_OPEN_APPEND;
        if (strchr(mode, '+')) fatfs_mode |= FA_READ;
    }
    
    printf("fopen: %s mode=%s fatfs_mode=0x%02x\n", filename, mode, fatfs_mode);
    fr = f_open(&file_handles[idx].fil, filename, fatfs_mode);
    
    if (fr != FR_OK) {
        printf("fopen: f_open failed with error %d\n", fr);
        errno = (fr == FR_NO_FILE || fr == FR_NO_PATH) ? ENOENT : EIO;
        return NULL;
    }
    
    printf("fopen: success, handle=%d\n", idx);
    file_handles[idx].in_use = 1;
    file_handles[idx].is_posix = 0;
    return fil_to_file(&file_handles[idx].fil);
}

int __wrap_fclose(FILE *fp) {
    FIL *fil = file_to_fil(fp);
    
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (file_handles[i].in_use && &file_handles[i].fil == fil) {
            f_close(fil);
            file_handles[i].in_use = 0;
            return 0;
        }
    }
    
    return EOF;
}

size_t __wrap_fread(const void *ptr, size_t size, size_t nmemb, FILE *fp) {
    FIL *fil = file_to_fil(fp);
    UINT br;
    FRESULT fr;
    
    fr = f_read(fil, (void*)ptr, size * nmemb, &br);
    if (fr != FR_OK) return 0;
    
    return br / size;
}

int __wrap_fgetc(FILE *fp) {
    FIL *fil = file_to_fil(fp);
    UINT br;
    FRESULT fr;
    unsigned char c;
    
    fr = f_read(fil, &c, 1, &br);
    if (fr != FR_OK || br == 0) return EOF;
    
    return (int)c;
}

size_t __wrap_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp) {
    FIL *fil = file_to_fil(fp);
    UINT bw;
    FRESULT fr;
    
    fr = f_write(fil, ptr, size * nmemb, &bw);
    if (fr != FR_OK) return 0;
    
    return bw / size;
}

int __wrap_fseek(FILE *fp, long offset, int whence) {
    FIL *fil = file_to_fil(fp);
    FSIZE_t pos;
    
    switch (whence) {
        case SEEK_SET:
            pos = offset;
            break;
        case SEEK_CUR:
            pos = f_tell(fil) + offset;
            break;
        case SEEK_END:
            pos = f_size(fil) + offset;
            break;
        default:
            return -1;
    }
    
    return (f_lseek(fil, pos) == FR_OK) ? 0 : -1;
}

long __wrap_ftell(FILE *fp) {
    FIL *fil = file_to_fil(fp);
    return (long)f_tell(fil);
}

int __wrap_remove(const char *filename) {
    FRESULT fr = f_unlink(filename);
    return (fr == FR_OK) ? 0 : -1;
}

int __wrap_rename(const char *oldname, const char *newname) {
    FRESULT fr = f_rename(oldname, newname);
    return (fr == FR_OK) ? 0 : -1;
}

// Create directory using FatFS
int fatfs_mkdir(const char *path) {
    FRESULT fr = f_mkdir(path);
    return (fr == FR_OK || fr == FR_EXIST) ? 0 : -1;
}

// Initialize file handles
void stdio_fatfs_init(void) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        file_handles[i].in_use = 0;
        file_handles[i].is_posix = 0;
    }
}
