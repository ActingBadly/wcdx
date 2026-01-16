
// SE_Controls - Simple input mapping system for keyboard, mouse, and gamepad
// Direct Windows API usage for input handling (no SDL dependency)
//
// Usage:
//   SE_Controls_Reset();     - Clear existing mappings
//   SE_Controls_Map("key", onPressFunc, onConstantFunc, onReleaseFunc);
//   SE_Controls_Read();      - Call this each frame to process key states and invoke callbacks

#include "../se_/SE_Controls.h"
#include <cmath>

// Mouse button codes
namespace MouseCode {
    constexpr int LEFT = -1;
    constexpr int MIDDLE = -2;
    constexpr int RIGHT = -3;
    constexpr int X1 = -4;
    constexpr int X2 = -5;
    constexpr int MOVE = -8;
}

// Xbox gamepad button codes
namespace XboxCode {
    constexpr int A = -10;
    constexpr int B = -11;
    constexpr int X = -12;
    constexpr int Y = -13;
    constexpr int LB = -14;
    constexpr int RB = -15;
    constexpr int LS_CLICK = -16;
    constexpr int RS_CLICK = -17;
    constexpr int BACK = -18;
    constexpr int START = -19;
    constexpr int DPAD_UP = -20;
    constexpr int DPAD_DOWN = -21;
    constexpr int DPAD_LEFT = -22;
    constexpr int DPAD_RIGHT = -23;
    constexpr int LT = -24;
    constexpr int RT = -25;
    constexpr int LS_UP = -26;
    constexpr int LS_DOWN = -27;
    constexpr int LS_LEFT = -28;
    constexpr int LS_RIGHT = -29;
    constexpr int RS_UP = -30;
    constexpr int RS_DOWN = -31;
    constexpr int RS_LEFT = -32;
    constexpr int RS_RIGHT = -33;
}

// XInput constants
namespace XInputThreshold {
    constexpr BYTE TRIGGER_PRESS = 30;
    constexpr BYTE TRIGGER_RELEASE = 20;
    constexpr SHORT LEFT_THUMB_PRESS = 7849;
    constexpr SHORT LEFT_THUMB_RELEASE = 5887;  // 3/4 of press
    constexpr SHORT RIGHT_THUMB_PRESS = 8689;
    constexpr SHORT RIGHT_THUMB_RELEASE = 6517; // 3/4 of press
}

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

    // Number keys (main keyboard)
    if (key == "0") return 0x30;
    if (key == "1") return 0x31;
    if (key == "2") return 0x32;
    if (key == "3") return 0x33;
    if (key == "4") return 0x34;
    if (key == "5") return 0x35;
    if (key == "6") return 0x36;
    if (key == "7") return 0x37;
    if (key == "8") return 0x38;
    if (key == "9") return 0x39;

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
    if (key == "comma") return VK_OEM_COMMA;
    if (key == "period") return VK_OEM_PERIOD;
    if (key == "slash") return VK_OEM_2;
    if (key == "minus") return VK_OEM_MINUS;
    if (key == "equals") return VK_OEM_PLUS;
    
    // Mouse buttons
    if (key == "mouse_left") return MouseCode::LEFT;
    if (key == "mouse_middle") return MouseCode::MIDDLE;
    if (key == "mouse_right") return MouseCode::RIGHT;
    if (key == "mouse_x1") return MouseCode::X1;
    if (key == "mouse_x2") return MouseCode::X2;
    if (key == "mouse_move") return MouseCode::MOVE;

    // Xbox gamepad buttons
    if (key == "XboxA") return XboxCode::A;
    if (key == "XboxB") return XboxCode::B;
    if (key == "XboxX") return XboxCode::X;
    if (key == "XboxY") return XboxCode::Y;
    if (key == "XboxLB") return XboxCode::LB;
    if (key == "XboxRB") return XboxCode::RB;
    if (key == "XboxLSClick") return XboxCode::LS_CLICK;
    if (key == "XboxRSClick") return XboxCode::RS_CLICK;
    if (key == "XboxBack") return XboxCode::BACK;
    if (key == "XboxStart") return XboxCode::START;
    if (key == "XboxDPadUp") return XboxCode::DPAD_UP;
    if (key == "XboxDPadDown") return XboxCode::DPAD_DOWN;
    if (key == "XboxDPadLeft") return XboxCode::DPAD_LEFT;
    if (key == "XboxDPadRight") return XboxCode::DPAD_RIGHT;
    if (key == "XboxLT") return XboxCode::LT;
    if (key == "XboxRT") return XboxCode::RT;
    if (key == "XboxLSUp") return XboxCode::LS_UP;
    if (key == "XboxLSDown") return XboxCode::LS_DOWN;
    if (key == "XboxLSLeft") return XboxCode::LS_LEFT;
    if (key == "XboxLSRight") return XboxCode::LS_RIGHT;
    if (key == "XboxRSUp") return XboxCode::RS_UP;
    if (key == "XboxRSDown") return XboxCode::RS_DOWN;
    if (key == "XboxRSLeft") return XboxCode::RS_LEFT;
    if (key == "XboxRSRight") return XboxCode::RS_RIGHT;

    return 0; // Unknown
}

// Mouse move sensitivity / deadzone
static float s_mouseMoveScale = 0.20f;
static int s_mouseMoveDeadzone = 0;

// Key-state storage
static std::unordered_map<int, KeyState> keyStates;

static bool isGamepadCode(int code)
{
    return code <= -10 && code >= -99;
}

bool SE_Controls_IsMapped(int key) {
    return keyStates.find(key) != keyStates.end();
}

void SE_Controls_Read()
{
    // Poll keyboard state for registered keys (uses GetAsyncKeyState so no event system dependency)
    for (auto& kv : keyStates)
    {
        int code = kv.first;
        KeyState& state = kv.second;

        if (code > 0)
        {
            state.isDown = state.overrideActive ? state.overrideValue : (GetAsyncKeyState(code) & 0x8000) != 0;
        }
        else
        {
            state.isDown = false;

            if (code == MouseCode::MOVE)
            {
                static POINT lastPos = { 0, 0 };
                POINT cur;
                GetCursorPos(&cur);
                int dx = cur.x - lastPos.x;
                int dy = cur.y - lastPos.y;
                if (dx != 0 || dy != 0)
                {
                    state.isDown = true;
                    // Apply scaling and deadzone before invoking callback
                    extern float s_mouseMoveScale;
                    extern int s_mouseMoveDeadzone;
                    float fdx = dx * s_mouseMoveScale;
                    float fdy = dy * s_mouseMoveScale;
                    int sdx = (int)roundf(fdx);
                    int sdy = (int)roundf(fdy);
                    // Ensure small movements still register when scale > 0
                    if (sdx == 0 && dx != 0 && s_mouseMoveScale > 0.00f) sdx = (dx > 0) ? 1 : -1;
                    if (sdy == 0 && dy != 0 && s_mouseMoveScale > 0.00f) sdy = (dy > 0) ? 1 : -1;
                    if (abs(sdx) > s_mouseMoveDeadzone || abs(sdy) > s_mouseMoveDeadzone)
                    {
                        if (state.onMove)
                            state.onMove(sdx, sdy);
                    }
                    lastPos = cur;
                }
                else
                    state.isDown = false;
            }
            else if (state.overrideActive)
            {
                state.isDown = state.overrideValue;
            }
            else
            {
                switch (code)
                {
                case MouseCode::LEFT:
                    state.isDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
                    break;
                case MouseCode::MIDDLE:
                    state.isDown = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
                    break;
                case MouseCode::RIGHT:
                    state.isDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
                    break;
                case MouseCode::X1:
                    state.isDown = (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0;
                    break;
                case MouseCode::X2:
                    state.isDown = (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0;
                    break;
                default:
                    break;
                }
            }
        }
    }

    // XInput structures (minimal to avoid linking XInput.lib)
    struct XINPUT_GAMEPAD { 
        WORD wButtons; 
        BYTE bLeftTrigger; 
        BYTE bRightTrigger; 
        SHORT sThumbLX; 
        SHORT sThumbLY; 
        SHORT sThumbRX; 
        SHORT sThumbRY; 
    };
    struct XINPUT_STATE { 
        DWORD dwPacketNumber; 
        XINPUT_GAMEPAD Gamepad; 
    };

    // XInput state
    static bool xinputLoaded = false;
    static HMODULE xinputModule = nullptr;
    using XInputGetState_t = DWORD(WINAPI*)(DWORD, void*);
    static XInputGetState_t pXInputGetState = nullptr;
    static DWORD s_lastXInputProbeTick = 0;
    static DWORD s_xinputProbeIntervalMs = 0;
    static int s_xinputConsecutiveFailures = 0;

    // Check if we need XInput polling
    bool needXInput = false;
    for (const auto& kv : keyStates)
    {
        if (isGamepadCode(kv.first))
        {
            needXInput = true;
            break;
        }
    }

    XINPUT_STATE xiState = {};
    bool haveXInputState = false;

    if (needXInput)
    {
        if (!xinputLoaded)
        {
            xinputModule = LoadLibraryW(L"xinput1_4.dll");
            if (xinputModule == nullptr)
                xinputModule = LoadLibraryW(L"xinput1_3.dll");
            if (xinputModule == nullptr)
                xinputModule = LoadLibraryW(L"xinput9_1_0.dll");

            if (xinputModule != nullptr)
                pXInputGetState = reinterpret_cast<XInputGetState_t>(GetProcAddress(xinputModule, "XInputGetState"));

            xinputLoaded = true;
        }

        if (pXInputGetState != nullptr)
        {
            const DWORD now = GetTickCount();
            const bool shouldProbe = (s_xinputProbeIntervalMs == 0) || 
                                    (now - s_lastXInputProbeTick >= s_xinputProbeIntervalMs);

            if (shouldProbe)
            {
                s_lastXInputProbeTick = now;
                
                // Try all 4 possible controllers
                for (DWORD userIndex = 0; userIndex < 4; ++userIndex)
                {
                    DWORD res = pXInputGetState(userIndex, &xiState);
                    if (res == 0) // ERROR_SUCCESS
                    {
                        haveXInputState = true;
                        break;
                    }
                }

                // Adjust probe interval based on connection state
                if (haveXInputState)
                {
                    s_xinputConsecutiveFailures = 0;
                    s_xinputProbeIntervalMs = 0; // Poll every frame when connected
                }
                else
                {
                    ++s_xinputConsecutiveFailures;
                    // Exponential backoff: 250ms, 500ms, 1s, 2s
                    if (s_xinputConsecutiveFailures >= 6)
                        s_xinputProbeIntervalMs = 2000;
                    else if (s_xinputConsecutiveFailures >= 4)
                        s_xinputProbeIntervalMs = 1000;
                    else if (s_xinputConsecutiveFailures >= 2)
                        s_xinputProbeIntervalMs = 500;
                    else
                        s_xinputProbeIntervalMs = 250;
                }
            }
        }
    }


    // Helper lambdas for analog input hysteresis
    auto isTriggerPressed = [](BYTE value, bool wasDown) -> bool
    {
        const BYTE threshold = wasDown ? XInputThreshold::TRIGGER_RELEASE : XInputThreshold::TRIGGER_PRESS;
        return value >= threshold;
    };

    auto isAxisPositive = [](SHORT value, SHORT press, SHORT release, bool wasDown) -> bool
    {
        return value > (wasDown ? release : press);
    };

    auto isAxisNegative = [](SHORT value, SHORT press, SHORT release, bool wasDown) -> bool
    {
        return value < -(wasDown ? release : press);
    };

    // Process mapped callbacks
    for (auto& [k, state] : keyStates)
    {
        // Gamepad mapping: update state.isDown based on XInput, or force-release if not available.
        if (isGamepadCode(k))
        {
            if (haveXInputState)
            {
                const WORD btn = xiState.Gamepad.wButtons;
                const BYTE lt = xiState.Gamepad.bLeftTrigger;
                const BYTE rt = xiState.Gamepad.bRightTrigger;
                const SHORT lx = xiState.Gamepad.sThumbLX;
                const SHORT ly = xiState.Gamepad.sThumbLY;
                const SHORT rx = xiState.Gamepad.sThumbRX;
                const SHORT ry = xiState.Gamepad.sThumbRY;

                switch (k)
                {
                case XboxCode::A: state.isDown = (btn & 0x1000) != 0; break;
                case XboxCode::B: state.isDown = (btn & 0x2000) != 0; break;
                case XboxCode::X: state.isDown = (btn & 0x4000) != 0; break;
                case XboxCode::Y: state.isDown = (btn & 0x8000) != 0; break;
                case XboxCode::LB: state.isDown = (btn & 0x0100) != 0; break;
                case XboxCode::RB: state.isDown = (btn & 0x0200) != 0; break;
                case XboxCode::LS_CLICK: state.isDown = (btn & 0x0040) != 0; break;
                case XboxCode::RS_CLICK: state.isDown = (btn & 0x0080) != 0; break;
                case XboxCode::BACK: state.isDown = (btn & 0x0020) != 0; break;
                case XboxCode::START: state.isDown = (btn & 0x0010) != 0; break;
                case XboxCode::DPAD_UP: state.isDown = (btn & 0x0001) != 0; break;
                case XboxCode::DPAD_DOWN: state.isDown = (btn & 0x0002) != 0; break;
                case XboxCode::DPAD_LEFT: state.isDown = (btn & 0x0004) != 0; break;
                case XboxCode::DPAD_RIGHT: state.isDown = (btn & 0x0008) != 0; break;

                case XboxCode::LT: state.isDown = isTriggerPressed(lt, state.wasDown); break;
                case XboxCode::RT: state.isDown = isTriggerPressed(rt, state.wasDown); break;

                case XboxCode::LS_UP: 
                    state.isDown = isAxisPositive(ly, XInputThreshold::LEFT_THUMB_PRESS, 
                                                  XInputThreshold::LEFT_THUMB_RELEASE, state.wasDown); 
                    break;
                case XboxCode::LS_DOWN: 
                    state.isDown = isAxisNegative(ly, XInputThreshold::LEFT_THUMB_PRESS, 
                                                  XInputThreshold::LEFT_THUMB_RELEASE, state.wasDown); 
                    break;
                case XboxCode::LS_LEFT: 
                    state.isDown = isAxisNegative(lx, XInputThreshold::LEFT_THUMB_PRESS, 
                                                  XInputThreshold::LEFT_THUMB_RELEASE, state.wasDown); 
                    break;
                case XboxCode::LS_RIGHT: 
                    state.isDown = isAxisPositive(lx, XInputThreshold::LEFT_THUMB_PRESS, 
                                                  XInputThreshold::LEFT_THUMB_RELEASE, state.wasDown); 
                    break;

                case XboxCode::RS_UP: 
                    state.isDown = isAxisPositive(ry, XInputThreshold::RIGHT_THUMB_PRESS, 
                                                  XInputThreshold::RIGHT_THUMB_RELEASE, state.wasDown); 
                    break;
                case XboxCode::RS_DOWN: 
                    state.isDown = isAxisNegative(ry, XInputThreshold::RIGHT_THUMB_PRESS, 
                                                  XInputThreshold::RIGHT_THUMB_RELEASE, state.wasDown); 
                    break;
                case XboxCode::RS_LEFT: 
                    state.isDown = isAxisNegative(rx, XInputThreshold::RIGHT_THUMB_PRESS, 
                                                  XInputThreshold::RIGHT_THUMB_RELEASE, state.wasDown); 
                    break;
                case XboxCode::RS_RIGHT: 
                    state.isDown = isAxisPositive(rx, XInputThreshold::RIGHT_THUMB_PRESS, 
                                                  XInputThreshold::RIGHT_THUMB_RELEASE, state.wasDown); 
                    break;
                    
                default:
                    state.isDown = false;
                    break;
                }
            }
            else
            {
                // Controller disconnected: force-release
                state.isDown = false;
            }
        }

        // Invoke callbacks based on state transitions
        if (state.isDown && !state.wasDown && state.onPress)
            state.onPress();

        if (state.isDown && state.onConstant)
            state.onConstant();

        if (!state.isDown && state.wasDown && state.onRelease)
            state.onRelease();

        state.wasDown = state.isDown;
    }
}

void SE_Controls_NotifyPhysicalKey(int vk, bool down)
{
    auto it = keyStates.find(vk);
    if (it != keyStates.end())
    {
        it->second.overrideActive = true;
        it->second.overrideValue = down;
    }
}

void SE_Controls_ClearOverrides()
{
    for (auto& [key, state] : keyStates)
    {
        state.overrideActive = false;
        state.overrideValue = false;
    }
}

std::vector<int> SE_Controls_GetCurrentlyHeldKeys()
{
    std::vector<int> heldKeys;
    heldKeys.reserve(keyStates.size());
    
    for (const auto& [key, state] : keyStates)
    {
        if (state.isDown || state.wasDown)
            heldKeys.push_back(key);
    }
    return heldKeys;
}

void SE_Controls_Reset()
{
    // Fire onRelease for any held keys
    for (auto& [key, state] : keyStates)
    {
        if ((state.isDown || state.wasDown) && state.onRelease)
            state.onRelease();
    }

    // Clear all mappings
    keyStates.clear();
}

void SE_Controls_Map(const std::string& key, std::function<void()> onPress, 
                    std::function<void()> onConstant, std::function<void()> onRelease)
{
    int code = getKeyCode(key);
    KeyState& state = keyStates[code];
    
    state.onPress = onPress;
    state.onConstant = onConstant;
    state.onRelease = onRelease;
    state.isDown = false;
    state.wasDown = false;
    state.overrideActive = false;
    state.overrideValue = false;
}

void SE_Controls_Map(const std::string& key, std::function<void(int,int)> onMove)
{
    int code = getKeyCode(key);
    KeyState& state = keyStates[code];
    
    state.onMove = onMove;
    state.isDown = false;
    state.wasDown = false;
    state.overrideActive = false;
    state.overrideValue = false;
}

void SE_Controls_SetMouseMoveScale(float scale)
{
    if (scale < 0.0f) scale = 0.0f;
    s_mouseMoveScale = scale;
}

void SE_Controls_SetMouseMoveDeadzone(int deadzone)
{
    if (deadzone < 0) deadzone = 0;
    s_mouseMoveDeadzone = deadzone;
}

float SE_Controls_GetMouseMoveScale()
{
    return s_mouseMoveScale;
}

int SE_Controls_GetMouseMoveDeadzone()
{
    return s_mouseMoveDeadzone;
}
