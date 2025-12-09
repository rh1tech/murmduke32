#ifndef _GAME_H_
#define _GAME_H_

#ifdef DUKE3D_RP2350
// RP2350 doesn't use FreeRTOS
#else
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

//extern uint8_t  game_dir[512];
char * getgamedir();
int gametextpal(int x,int y,char  *t,uint8_t  s,uint8_t  p);
int main_duke3d(int argc,char  **argv);

#endif  // include-once header.

