// Duke3D OPL Music - Type definitions
// Adapted from murmdoom for Duke Nukem 3D RP2350 port

#ifndef __DOOMTYPE_H__
#define __DOOMTYPE_H__

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <limits.h>

// Boolean type - use stdbool.h bool
typedef bool boolean;

// Basic types
typedef uint8_t byte;

// Packed structure macros
#ifdef __GNUC__
#define PACKEDATTR __attribute__((packed))
#define PACKEDPREFIX
#else
#define PACKEDATTR
#define PACKEDPREFIX __packed
#endif

#define PACKED_STRUCT(...) PACKEDPREFIX struct __VA_ARGS__ PACKEDATTR

// Array length macro
#define arrlen(array) (sizeof(array) / sizeof(*array))

// Compatibility defines
#define should_be_const const
typedef const char* constcharstar;
typedef int16_t isb_int16_t;
#define stderr_print(...) printf(__VA_ARGS__)

// Path separators
#define DIR_SEPARATOR '/'
#define DIR_SEPARATOR_S "/"
#define PATH_SEPARATOR ':'

#endif
