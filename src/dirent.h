/*
 * dirent.h compatibility for RP2350 with FatFS
 * Provides minimal POSIX directory operations
 * 
 * The actual implementations are in compat.c
 */
#ifndef _DIRENT_H_COMPAT
#define _DIRENT_H_COMPAT

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum filename length */
#ifndef MAXNAMLEN
#define MAXNAMLEN 255
#endif

/* Directory entry structure */
struct dirent {
    char d_name[MAXNAMLEN + 1];
    unsigned char d_type;
};

/* Directory type definitions */
#ifndef DT_UNKNOWN
#define DT_UNKNOWN  0
#endif
#ifndef DT_REG
#define DT_REG      8   /* Regular file */
#endif
#ifndef DT_DIR
#define DT_DIR      4   /* Directory */
#endif

/* 
 * DIR is a POSIX-compatible structure wrapping FatFS FFDIR.
 * Defined in compat.c, declared here as opaque type.
 */
typedef struct POSIX_DIR_STRUCT DIR;

/* Directory functions - implemented in compat.c */
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);

#ifdef __cplusplus
}
#endif

#endif /* _DIRENT_H_COMPAT */
