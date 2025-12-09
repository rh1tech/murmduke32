/*
 * PS/2 Keyboard Wrapper for Duke3D on RP2350
 * Maps HID scancodes to Duke3D keyboard scancodes
 */
#include "ps2kbd_wrapper.h"
#include "ps2kbd_mrmltr.h"
#include <queue>

// PS/2 keyboard pins (matching board_config.h)
#ifndef PS2_PIN_DATA
#define PS2_PIN_DATA 0
#endif

// Duke3D scancode definitions (from keyboard.h)
#define  sc_None         0
#define  sc_Return       0x1c
#define  sc_Escape       0x01
#define  sc_Space        0x39
#define  sc_BackSpace    0x0e
#define  sc_Tab          0x0f
#define  sc_LeftAlt      0x38
#define  sc_LeftControl  0x1d
#define  sc_LeftShift    0x2a
#define  sc_RightShift   0x36
#define  sc_F1           0x3b
#define  sc_F2           0x3c
#define  sc_F3           0x3d
#define  sc_F4           0x3e
#define  sc_F5           0x3f
#define  sc_F6           0x40
#define  sc_F7           0x41
#define  sc_F8           0x42
#define  sc_F9           0x43
#define  sc_F10          0x44
#define  sc_F11          0x57
#define  sc_F12          0x58
#define  sc_UpArrow      0x5a
#define  sc_DownArrow    0x6a
#define  sc_LeftArrow    0x6b
#define  sc_RightArrow   0x6c
#define  sc_A            0x1e
#define  sc_B            0x30
#define  sc_C            0x2e
#define  sc_D            0x20
#define  sc_E            0x12
#define  sc_F            0x21
#define  sc_G            0x22
#define  sc_H            0x23
#define  sc_I            0x17
#define  sc_J            0x24
#define  sc_K            0x25
#define  sc_L            0x26
#define  sc_M            0x32
#define  sc_N            0x31
#define  sc_O            0x18
#define  sc_P            0x19
#define  sc_Q            0x10
#define  sc_R            0x13
#define  sc_S            0x1f
#define  sc_T            0x14
#define  sc_U            0x16
#define  sc_V            0x2f
#define  sc_W            0x11
#define  sc_X            0x2d
#define  sc_Y            0x15
#define  sc_Z            0x2c
#define  sc_1            0x02
#define  sc_2            0x03
#define  sc_3            0x04
#define  sc_4            0x05
#define  sc_5            0x06
#define  sc_6            0x07
#define  sc_7            0x08
#define  sc_8            0x09
#define  sc_9            0x0a
#define  sc_0            0x0b

struct KeyEvent {
    int pressed;
    unsigned char key;
};

static std::queue<KeyEvent> event_queue;

// HID to Duke3D scancode mapping
static unsigned char hid_to_duke3d(uint8_t code) {
    // Letters A-Z (HID 0x04-0x1D)
    static const unsigned char letter_map[26] = {
        sc_A, sc_B, sc_C, sc_D, sc_E, sc_F, sc_G, sc_H, sc_I, sc_J,
        sc_K, sc_L, sc_M, sc_N, sc_O, sc_P, sc_Q, sc_R, sc_S, sc_T,
        sc_U, sc_V, sc_W, sc_X, sc_Y, sc_Z
    };
    
    if (code >= 0x04 && code <= 0x1D) {
        return letter_map[code - 0x04];
    }
    
    // Numbers 1-9, 0 (HID 0x1E-0x27)
    if (code >= 0x1E && code <= 0x26) {
        return sc_1 + (code - 0x1E);
    }
    if (code == 0x27) return sc_0;
    
    // Special keys
    switch (code) {
        case 0x28: return sc_Return;
        case 0x29: return sc_Escape;
        case 0x2A: return sc_BackSpace;
        case 0x2B: return sc_Tab;
        case 0x2C: return sc_Space;
        
        // Arrow keys
        case 0x4F: return sc_RightArrow;
        case 0x50: return sc_LeftArrow;
        case 0x51: return sc_DownArrow;
        case 0x52: return sc_UpArrow;
        
        // Function keys F1-F12 (HID 0x3A-0x45)
        case 0x3A: return sc_F1;
        case 0x3B: return sc_F2;
        case 0x3C: return sc_F3;
        case 0x3D: return sc_F4;
        case 0x3E: return sc_F5;
        case 0x3F: return sc_F6;
        case 0x40: return sc_F7;
        case 0x41: return sc_F8;
        case 0x42: return sc_F9;
        case 0x43: return sc_F10;
        case 0x44: return sc_F11;
        case 0x45: return sc_F12;
        
        default: return sc_None;
    }
}

static void key_handler(hid_keyboard_report_t *curr, hid_keyboard_report_t *prev) {
    // Check modifiers
    uint8_t changed_mods = curr->modifier ^ prev->modifier;
    if (changed_mods) {
        // Ctrl
        if (changed_mods & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL)) {
            int ctrl_pressed = (curr->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL)) != 0;
            event_queue.push({ctrl_pressed, sc_LeftControl});
        }
        // Shift
        if (changed_mods & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT)) {
            int shift_pressed = (curr->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT)) != 0;
            event_queue.push({shift_pressed, sc_LeftShift});
        }
        // Alt
        if (changed_mods & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT)) {
            int alt_pressed = (curr->modifier & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT)) != 0;
            event_queue.push({alt_pressed, sc_LeftAlt});
        }
    }

    // Check key presses (new keys)
    for (int i = 0; i < 6; i++) {
        if (curr->keycode[i] != 0) {
            bool found = false;
            for (int j = 0; j < 6; j++) {
                if (prev->keycode[j] == curr->keycode[i]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                unsigned char k = hid_to_duke3d(curr->keycode[i]);
                if (k != sc_None) event_queue.push({1, k});
            }
        }
    }

    // Check key releases (keys no longer pressed)
    for (int i = 0; i < 6; i++) {
        if (prev->keycode[i] != 0) {
            bool found = false;
            for (int j = 0; j < 6; j++) {
                if (curr->keycode[j] == prev->keycode[i]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                unsigned char k = hid_to_duke3d(prev->keycode[i]);
                if (k != sc_None) event_queue.push({0, k});
            }
        }
    }
}

static Ps2Kbd_Mrmltr* kbd = nullptr;

extern "C" void ps2kbd_init(void) {
    // Use pins from board config
    kbd = new Ps2Kbd_Mrmltr(pio0, PS2_PIN_DATA, key_handler);
    kbd->init_gpio();
}

extern "C" void ps2kbd_tick(void) {
    if (kbd) kbd->tick();
}

extern "C" int ps2kbd_get_key(int* pressed, unsigned char* key) {
    if (event_queue.empty()) return 0;
    KeyEvent e = event_queue.front();
    event_queue.pop();
    *pressed = e.pressed;
    *key = e.key;
    return 1;
}
