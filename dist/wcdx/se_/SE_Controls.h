// SE_Controls - Simple input mapping system for keyboard, mouse, and gamepad
// Removed SDL dependency for direct Windows API usage.
// Angelscript bindings also not included here.

#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <windows.h>

static bool FullScreenBreak = false;

struct KeyState {
    bool isDown = false;
    bool wasDown = false;
    std::function<void()> onPress;      // Fires once on press
    std::function<void()> onRelease;    // Fires once on release
    std::function<void()> onConstant;   // Fires every frame while held
};

// Returns an integer key code. Uses Windows virtual-key codes where applicable.
static int getKeyCode(const std::string& key) {
    if (key.size() == 1)
    {
        char c = key[0];
        if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
        return static_cast<int>(c);
    }

    if (key == "esc") return VK_ESCAPE;
    if (key == "space") return VK_SPACE;
    if (key == "enter") return VK_RETURN;
    if (key == "tab") return VK_TAB;
    if (key == "backspace") return VK_BACK;
    if (key == "shift") return VK_LSHIFT;
    if (key == "ctrl") return VK_LCONTROL;
    if (key == "alt") return VK_LMENU;
    if (key == "capslock") return VK_CAPITAL;
    if (key == "insert") return VK_INSERT;
    if (key == "delete") return VK_DELETE;
    if (key == "home") return VK_HOME;
    if (key == "end") return VK_END;
    if (key == "pageup") return VK_PRIOR;
    if (key == "pagedown") return VK_NEXT;

    if (key == "up") return VK_UP;
    if (key == "down") return VK_DOWN;
    if (key == "left") return VK_LEFT;
    if (key == "right") return VK_RIGHT;

    if (key == "f1") return VK_F1;
    if (key == "f2") return VK_F2;
    if (key == "f3") return VK_F3;
    if (key == "f4") return VK_F4;
    if (key == "f5") return VK_F5;
    if (key == "f6") return VK_F6;
    if (key == "f7") return VK_F7;
    if (key == "f8") return VK_F8;
    if (key == "f9") return VK_F9;
    if (key == "f10") return VK_F10;
    if (key == "f11") return VK_F11;
    if (key == "f12") return VK_F12;

    if (key == "printscreen") return VK_SNAPSHOT;
    if (key == "scrolllock") return VK_SCROLL;
    if (key == "pause") return VK_PAUSE;

    // Numpad
    if (key == "numlock") return VK_NUMLOCK;
    if (key == "kp_divide") return VK_DIVIDE;
    if (key == "kp_multiply") return VK_MULTIPLY;
    if (key == "kp_minus") return VK_SUBTRACT;
    if (key == "kp_plus") return VK_ADD;
    if (key == "kp_enter") return VK_RETURN;

    // Simple punctuation
    if (key == "comma") return ',';
    if (key == "period") return '.';
    if (key == "slash") return '/';
    if (key == "minus") return '-';
    if (key == "equals") return '=';

    // Mouse and special inputs use negative codes (platform-agnostic)
    if (key == "mouse_left") return -1;
    if (key == "mouse_middle") return -2;
    if (key == "mouse_right") return -3;
    if (key == "mouse_x1") return -4;
    if (key == "mouse_x2") return -5;

    if (key == "mouse_wheel_up") return -6;    // Special case, handle separately
    if (key == "mouse_wheel_down") return -7;  // Special case, handle separately

    if (key == "mouse_move") return -8;        // Special case, handle separately

    if (key == "any") return -9;               // Special case, handle separately

    // Gamepad / Xbox codes remain special negative values
    if (key == "XboxA") return -10;
    if (key == "XboxB") return -11;
    if (key == "XboxX") return -12;
    if (key == "XboxY") return -13;
    if (key == "XboxLB") return -14;
    if (key == "XboxRB") return -15;
    if (key == "XboxLS") return -16;
    if (key == "XboxRS") return -17;
    if (key == "XboxBack") return -18;
    if (key == "XboxStart") return -19;
    if (key == "XboxDPadUp") return -20;
    if (key == "XboxDPadDown") return -21;
    if (key == "XboxDPadLeft") return -22;
    if (key == "XboxDPadRight") return -23;

    return 0; // Unknown
}

static std::unordered_map<int, KeyState> keyStates;

class SE_OPENGL;

void SE_Controls(int key, std::function<void()> onPress, std::function<void()> onRelease, std::function<void()> onConstant);
void SE_Controls_Read(); // Process per-frame key state updates
void SE_Controls_Reset();

void SE_Controls_Map(std::string key, std::function<void()> onPress, std::function<void()> onRelease, std::function<void()> onConstant);