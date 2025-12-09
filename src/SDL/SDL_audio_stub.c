/*
 * SDL Audio stub implementation for RP2350
 * Audio is disabled for initial port
 */
#include "SDL.h"
#include "SDL_audio.h"
#include <string.h>

int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) {
    if (obtained) {
        memcpy(obtained, desired, sizeof(SDL_AudioSpec));
    }
    // Return -1 to indicate audio not available
    return -1;
}

void SDL_CloseAudio(void) {
}

void SDL_PauseAudio(int pause_on) {
}

void SDL_LockAudio(void) {
}

void SDL_UnlockAudio(void) {
}

char *SDL_AudioDriverName(char *namebuf, int maxlen) {
    strncpy(namebuf, "RP2350 Audio (disabled)", maxlen);
    return namebuf;
}
