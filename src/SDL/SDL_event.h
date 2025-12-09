/*
  Simple DirectMedia Layer - RP2350 Port
  Minimal SDL event handling for Duke3D
*/

#ifndef _SDL_events_h
#define _SDL_events_h

#include <stdint.h>
#include "SDL_input.h"  /* For SDL_Keysym */

#ifdef __cplusplus
extern "C" {
#endif

/* General keyboard/mouse state definitions */
#define SDL_RELEASED	0
#define SDL_PRESSED	1

/**
 * \brief The types of events that can be delivered.
 */
typedef enum
{
    SDL_FIRSTEVENT     = 0,     /**< Unused (do not remove) */

    /* Application events */
    SDL_QUIT           = 0x100, /**< User-requested quit */

    /* Window events */
    SDL_WINDOWEVENT    = 0x200, /**< Window state change */
    SDL_SYSWMEVENT,             /**< System specific event */

    /* Keyboard events */
    SDL_KEYDOWN        = 0x300, /**< Key pressed */
    SDL_KEYUP,                  /**< Key released */
    SDL_TEXTEDITING,            /**< Keyboard text editing (composition) */
    SDL_TEXTINPUT,              /**< Keyboard text input */

    /* Mouse events */
    SDL_MOUSEMOTION    = 0x400, /**< Mouse moved */
    SDL_MOUSEBUTTONDOWN,        /**< Mouse button pressed */
    SDL_MOUSEBUTTONUP,          /**< Mouse button released */
    SDL_MOUSEWHEEL,             /**< Mouse wheel motion */

    /* Joystick events */
    SDL_JOYAXISMOTION  = 0x600, /**< Joystick axis motion */
    SDL_JOYBALLMOTION,          /**< Joystick trackball motion */
    SDL_JOYHATMOTION,           /**< Joystick hat motion */
    SDL_JOYBUTTONDOWN,          /**< Joystick button pressed */
    SDL_JOYBUTTONUP,            /**< Joystick button released */

    SDL_LASTEVENT      = 0xFFFF
} SDL_EventType;

/**
 *  \brief Keyboard button event structure
 */
typedef struct SDL_KeyboardEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint8_t state;
    uint8_t repeat;
    uint8_t padding2;
    uint8_t padding3;
    SDL_Keysym keysym;
} SDL_KeyboardEvent;

/**
 *  \brief Mouse motion event structure
 */
typedef struct SDL_MouseMotionEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint32_t which;
    uint32_t state;
    int32_t x;
    int32_t y;
    int32_t xrel;
    int32_t yrel;
} SDL_MouseMotionEvent;

/**
 *  \brief Mouse button event structure
 */
typedef struct SDL_MouseButtonEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint32_t which;
    uint8_t button;
    uint8_t state;
    uint8_t clicks;
    uint8_t padding1;
    int32_t x;
    int32_t y;
} SDL_MouseButtonEvent;

/**
 *  \brief Joystick ball motion event structure
 */
typedef struct SDL_JoyBallEvent {
    uint32_t type;
    uint32_t timestamp;
    int32_t which;
    uint8_t ball;
    int16_t xrel;
    int16_t yrel;
} SDL_JoyBallEvent;

/**
 *  \brief Joystick axis motion event structure
 */
typedef struct SDL_JoyAxisEvent {
    uint32_t type;
    uint32_t timestamp;
    int32_t which;
    uint8_t axis;
    int16_t value;
} SDL_JoyAxisEvent;

/**
 *  \brief The "quit requested" event
 */
typedef struct SDL_QuitEvent {
    uint32_t type;
    uint32_t timestamp;
} SDL_QuitEvent;

/**
 *  \brief General event structure
 */
typedef union SDL_Event {
    uint32_t type;
    SDL_KeyboardEvent key;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
    SDL_JoyBallEvent jball;
    SDL_JoyAxisEvent jaxis;
    SDL_QuitEvent quit;
    uint8_t padding[56];
} SDL_Event;

/* Function prototypes */
int SDL_PollEvent(SDL_Event *event);
void SDL_PumpEvents(void);
int SDL_PushEvent(SDL_Event *event);

/* Key state */
uint8_t *SDL_GetKeyState(int *numkeys);

/* Subsystem init/quit */
int SDL_InitSubSystem(uint32_t flags);
void SDL_QuitSubSystem(uint32_t flags);

/* Error handling */
void SDL_ClearError(void);

#ifdef __cplusplus
}
#endif

#endif /* _SDL_events_h */
