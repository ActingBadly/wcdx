//*******************************************widescreen.h******************************************
// Simple management of widescreen aspect ratio settings for Wcdx.
//*************************************************************************************************

#pragma once

#include <windows.h>
#include "wcdx.h"

// Default values
inline bool FULLSCREEN = true;
inline int RatioX = 4;
inline int RatioY = 3;

// When there is a windows chage that could affect aspect ratio, call this to recompute the window frame rect.
// This is because when aspect ratio changes, the client area size changes and the window frame needs to be adjusted to match.
// Or else the mouse will not be confined properly and the black bars will not clear correctly.
inline void RecomputeWindowFrameRect(){
    HWND hwnd = ::FindWindowW(L"Wcdx Frame Window", nullptr);
    if (hwnd != nullptr)
    {
        RECT rect;
        if (::GetWindowRect(hwnd, &rect))
        {
            ::SendMessage(hwnd, WM_APP_ASPECT_CHANGED, 0, reinterpret_cast<LPARAM>(&rect));
        }
    }
}

// Switch between common aspect ratios: 4:3, 16:9, 16:10
inline void AspectRatioSwitch(){
    if (RatioX == 4 && RatioY == 3){
        RatioX = 16;
        RatioY = 9;
    }
    else if (RatioX == 16 && RatioY == 9){
        RatioX = 16;
        RatioY = 10;
    }
    else{
        RatioX = 4;
        RatioY = 3;
    }
    RecomputeWindowFrameRect();
}