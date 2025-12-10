// Duke3D OPL Music - Byte swapping macros
// Adapted from murmdoom for Duke Nukem 3D RP2350 port

#ifndef __I_SWAP_H__
#define __I_SWAP_H__

// RP2350 is little endian
#define SYS_LITTLE_ENDIAN

#define SHORT(x) ((signed short)(x))
#define LONG(x)  ((signed int)(x))

// Byte swapping for big endian data (like MIDI files)
#define SDL_SwapBE16(x) __builtin_bswap16(x)
#define SDL_SwapBE32(x) __builtin_bswap32(x)

#endif
