// SE_Controls - Simple input mapping system for keyboard, mouse, and gamepad
// Removed SDL dependency for direct Windows API usage.
// Angelscript bindings also not included here.

#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <windows.h>

struct KeyState {
    bool isDown = false;
    bool wasDown = false;

    bool overrideActive = false;
    bool overrideValue = false;
    
    std::function<void()> onPress;      // Fires once on press
    std::function<void()> onRelease;    // Fires once on release
    std::function<void()> onConstant;   // Fires every frame while held
    std::function<void(int,int)> onMove; // Fires when mouse moves: (dx, dy)
};

void SE_Controls_Read(); // Process per-frame key state updates

void SE_Controls_ClearOverrides();

void SE_Controls_Reset();

std::vector<int> SE_Controls_GetCurrentlyHeldKeys();

void SE_Controls_Map(const std::string& key, std::function<void()> onPress, std::function<void()> onConstant, std::function<void()> onRelease);
// Overload for mouse-move mappings: callback receives delta x,y in screen coords
void SE_Controls_Map(const std::string& key, std::function<void(int,int)> onMove);

// Adjust mouse movement sensitivity (multiplies reported dx/dy). Default is 0.2.
void SE_Controls_SetMouseMoveScale(float scale);
// Set deadzone (ignore movements with absolute delta <= deadzone)
void SE_Controls_SetMouseMoveDeadzone(int deadzone);

// Read current mouse move scale
float SE_Controls_GetMouseMoveScale();
int SE_Controls_GetMouseMoveDeadzone();

// Returns true if the given virtual-key (or mapped code) has a mapping registered.
bool SE_Controls_IsMapped(int key);

// Notify the control system of a physical key event consumed by an input hook.
// When called, the key's state will be reported to `SE_Controls_Read` from
// the override value until the override is cleared by a matching call.
void SE_Controls_NotifyPhysicalKey(int vk, bool down);