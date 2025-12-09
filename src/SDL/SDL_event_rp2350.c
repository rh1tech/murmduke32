/*
 * SDL Event implementation for RP2350
 * Uses PS/2 keyboard driver from murmdoom
 */
#include "SDL.h"
#include "SDL_event.h"
#include "ps2kbd_wrapper.h"
#include "pico/stdlib.h"

#define MAX_EVENTS 32
static SDL_Event event_queue[MAX_EVENTS];
static int event_head = 0;
static int event_tail = 0;

// Convert PS/2 scancode to SDL keycode
static SDLKey ps2_to_sdl_key(unsigned char key) {
    // Basic mapping - can be extended
    switch (key) {
        case 0x1B: return SDLK_ESCAPE;
        case 0x0D: return SDLK_RETURN;
        case 0x20: return SDLK_SPACE;
        case 0x08: return SDLK_BACKSPACE;
        case 0x09: return SDLK_TAB;
        // Arrow keys (using extended scancodes)
        case 0x80: return SDLK_UP;
        case 0x81: return SDLK_DOWN;
        case 0x82: return SDLK_LEFT;
        case 0x83: return SDLK_RIGHT;
        // Letters (lowercase)
        default:
            if (key >= 'a' && key <= 'z') {
                return (SDLKey)(SDLK_a + (key - 'a'));
            }
            if (key >= 'A' && key <= 'Z') {
                return (SDLKey)(SDLK_a + (key - 'A'));
            }
            if (key >= '0' && key <= '9') {
                return (SDLKey)(SDLK_0 + (key - '0'));
            }
            break;
    }
    return SDLK_UNKNOWN;
}

void SDL_PumpEvents(void) {
    // Poll keyboard and add events to queue
    ps2kbd_tick();
    
    int pressed;
    unsigned char key;
    while (ps2kbd_get_key(&pressed, &key)) {
        int next_head = (event_head + 1) % MAX_EVENTS;
        if (next_head != event_tail) {
            SDL_Event *ev = &event_queue[event_head];
            ev->type = pressed ? SDL_KEYDOWN : SDL_KEYUP;
            ev->key.keysym.sym = ps2_to_sdl_key(key);
            ev->key.keysym.scancode = key;
            ev->key.keysym.mod = KMOD_NONE;
            ev->key.state = pressed ? SDL_PRESSED : SDL_RELEASED;
            event_head = next_head;
        }
    }
}

int SDL_PollEvent(SDL_Event *event) {
    SDL_PumpEvents();
    
    if (event_tail != event_head) {
        *event = event_queue[event_tail];
        event_tail = (event_tail + 1) % MAX_EVENTS;
        return 1;
    }
    return 0;
}

int SDL_WaitEvent(SDL_Event *event) {
    while (1) {
        if (SDL_PollEvent(event)) {
            return 1;
        }
        sleep_ms(10);
    }
}

Uint8 *SDL_GetKeyState(int *numkeys) {
    static Uint8 keystate[SDLK_LAST];
    if (numkeys) *numkeys = SDLK_LAST;
    return keystate;
}

char *SDL_GetKeyName(SDLKey key) {
    static char name[32];
    if (key >= SDLK_a && key <= SDLK_z) {
        name[0] = 'A' + (key - SDLK_a);
        name[1] = '\0';
    } else {
        snprintf(name, sizeof(name), "Key%d", key);
    }
    return name;
}

SDL_Keymod SDL_GetModState(void) {
    return KMOD_NONE;
}

void SDL_SetModState(SDL_Keymod modstate) {
    // Not implemented
}

int SDL_EnableKeyRepeat(int delay, int interval) {
    return 0;
}

int SDL_EnableUNICODE(int enable) {
    return 0;
}

/* Joystick stubs */
int SDL_NumJoysticks(void) {
    return 0;
}

SDL_Joystick *SDL_JoystickOpen(int device_index) {
    return NULL;
}

void SDL_JoystickClose(SDL_Joystick *joystick) {
}

const char *SDL_JoystickName(SDL_Joystick *joystick) {
    return "";
}

int SDL_JoystickNumAxes(SDL_Joystick *joystick) {
    return 0;
}

int SDL_JoystickNumButtons(SDL_Joystick *joystick) {
    return 0;
}

int SDL_JoystickNumHats(SDL_Joystick *joystick) {
    return 0;
}

Sint16 SDL_JoystickGetAxis(SDL_Joystick *joystick, int axis) {
    return 0;
}

Uint8 SDL_JoystickGetButton(SDL_Joystick *joystick, int button) {
    return 0;
}

Uint8 SDL_JoystickGetHat(SDL_Joystick *joystick, int hat) {
    return 0;
}

void SDL_JoystickUpdate(void) {
}
