// Duke3D OPL Music - System interface stubs
// Adapted from murmdoom for Duke Nukem 3D RP2350 port

#ifndef __I_SYSTEM_H__
#define __I_SYSTEM_H__

#include <stdio.h>
#include <stdlib.h>

// Error handling
#define I_Error(...) do { printf("ERROR: "); printf(__VA_ARGS__); printf("\n"); } while(0)

#endif
