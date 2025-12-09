/*
 * SDL System header for RP2350
 */
#ifndef SDL_SYSTEM_H
#define SDL_SYSTEM_H

#include "SDL_stdinc.h"

int SDL_Init(Uint32 flags);
void SDL_Quit(void);
const char *SDL_GetError(void);
void SDL_Delay(Uint32 ms);

#endif /* SDL_SYSTEM_H */
