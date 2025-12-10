/*
 * Duke3D OPL Music System for RP2350
 * Uses emu8950 OPL emulator for FM synthesis
 * Parses standard MIDI files from SD card
 * Uses Duke3D native timbre bank format
 */

#ifndef __I_MUSIC_H__
#define __I_MUSIC_H__

#include <stdint.h>
#include <stdbool.h>

// Music system initialization
bool I_Music_Init(void);
void I_Music_Shutdown(void);

// Playback control
bool I_Music_PlayMIDI(const char *filename, bool loop);
void I_Music_Stop(void);
void I_Music_Pause(void);
void I_Music_Resume(void);
bool I_Music_IsPlaying(void);

// Volume control (0-255)
void I_Music_SetVolume(int volume);
int I_Music_GetVolume(void);

// Register Duke3D timbre bank (256 instruments × 13 bytes)
void I_Music_RegisterTimbreBank(const uint8_t *timbres);

#endif // __I_MUSIC_H__
