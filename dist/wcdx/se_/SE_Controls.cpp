#include <SE_Controls.h>

void ControllerRefresh()
{
	// No-op: controller enumeration removed when SDL dependency was removed.
}

void SE_Controls(int key, std::function<void()> onPress, std::function<void()> onRelease, std::function<void()> onConstant) {
	keyStates[key].onPress = onPress;
	keyStates[key].onRelease = onRelease;
	keyStates[key].onConstant = onConstant;
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
            bool nowDown = (GetAsyncKeyState(code) & 0x8000) != 0;
            state.isDown = nowDown;
        }
        else
        {
            // Handle mouse buttons / movement and gamepad (-ve codes)
            switch (code)
            {
            case -1: // mouse_left
                state.isDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
                break;
            case -2: // mouse_middle
                state.isDown = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
                break;
            case -3: // mouse_right
                state.isDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
                break;
            case -4: // mouse_x1
                state.isDown = (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0;
                break;
            case -5: // mouse_x2
                state.isDown = (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0;
                break;
            case -8: // mouse_move
            {
                static POINT lastPos = { 0, 0 };
                POINT cur;
                GetCursorPos(&cur);
                if (cur.x != lastPos.x || cur.y != lastPos.y)
                {
                    state.isDown = true;
                    lastPos = cur;
                }
                else
                    state.isDown = false;
            }
            break;
            default:
                // Gamepad codes (-10 .. -99) handled below via XInput
                break;
            }
        }
    }

    // Gamepad polling (XInput) - only if any gamepad mappings exist
    bool needXInput = false;
    for (auto& kv : keyStates) if (kv.first <= -10 && kv.first >= -99) { needXInput = true; break; }
    static bool xinputLoaded = false;
    static HMODULE xinputModule = nullptr;
    using XInputGetState_t = DWORD(WINAPI*)(DWORD, void*);
    static XInputGetState_t pXInputGetState = nullptr;
    struct XINPUT_GAMEPAD { WORD wButtons; BYTE bLeftTrigger; BYTE bRightTrigger; SHORT sThumbLX; SHORT sThumbLY; SHORT sThumbRX; SHORT sThumbRY; };
    struct XINPUT_STATE { DWORD dwPacketNumber; XINPUT_GAMEPAD Gamepad; };
    XINPUT_STATE xiState = {};
    bool haveXInputState = false;
    if (needXInput)
    {
        if (!xinputLoaded)
        {
            xinputModule = LoadLibraryW(L"xinput1_4.dll");
            if (xinputModule == nullptr)
                xinputModule = LoadLibraryW(L"xinput1_3.dll");
            if (xinputModule != nullptr)
                pXInputGetState = reinterpret_cast<XInputGetState_t>(GetProcAddress(xinputModule, "XInputGetState"));
            xinputLoaded = true;
        }

        if (pXInputGetState != nullptr)
        {
            DWORD res = pXInputGetState(0, &xiState);
            if (res == 0) // ERROR_SUCCESS
                haveXInputState = true;
        }
    }

    // Process mapped callbacks
    for (auto& [k, state] : keyStates)
    {
        // If this is a gamepad mapping, override state.isDown based on XInput
        if (k <= -10 && haveXInputState)
        {
            // map negative codes to XInput gamepad buttons
            const WORD btn = xiState.Gamepad.wButtons;
            switch (k)
            {
            case -10: state.isDown = (btn & 0x1000) != 0; break; // A
            case -11: state.isDown = (btn & 0x2000) != 0; break; // B
            case -12: state.isDown = (btn & 0x4000) != 0; break; // X
            case -13: state.isDown = (btn & 0x8000) != 0; break; // Y
            case -14: state.isDown = (btn & 0x0100) != 0; break; // LB
            case -15: state.isDown = (btn & 0x0200) != 0; break; // RB
            case -16: state.isDown = (btn & 0x0040) != 0; break; // LS
            case -17: state.isDown = (btn & 0x0080) != 0; break; // RS
            case -18: state.isDown = (btn & 0x0020) != 0; break; // Back
            case -19: state.isDown = (btn & 0x0010) != 0; break; // Start
            case -20: state.isDown = (btn & 0x0001) != 0; break; // DPad Up
            case -21: state.isDown = (btn & 0x0002) != 0; break; // DPad Down
            case -22: state.isDown = (btn & 0x0004) != 0; break; // DPad Left
            case -23: state.isDown = (btn & 0x0008) != 0; break; // DPad Right
            default: break;
            }
        }

        if (state.isDown && !state.wasDown && state.onPress)
            state.onPress();

        if (state.isDown && state.onConstant)
            state.onConstant();

        if (!state.isDown && state.wasDown && state.onRelease)
            state.onRelease();

        state.wasDown = state.isDown;
    }

    // Avoid busy loop when called repeatedly in a tight thread loop
    Sleep(10);
}

void SE_Controls_Reset() {
	keyStates.clear();
}

void SE_Controls_Map(std::string key, std::function<void()> onPress, std::function<void()> onRelease, std::function<void()> onConstant) {
	SE_Controls(
		getKeyCode(key),
		onPress,
		onRelease,
		onConstant
	);
}

























