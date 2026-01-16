//******************************************push_byte.h*******************************************
// Enables the ability to patch bytes into the target process at runtime.
// Used to hide mouse cursor in game and enable frame rate hacks.
// Things got a bit messy with multiple overloads, but this should make it easier to use.
//
// Usage:
// Push_Byte(PatchSize, Address, {0x90,0x90,...});
//
//*************************************************************************************************
#pragma once

#include <windows.h>
#include <cstdint>
#include <string>
#include <initializer_list>
#include <vector>
#include "se_/defines.h" //temporary, for Process_Has_Title

// Overload to accept initializer lists like: Push_Byte(6, 0x12345, {0x90,0x90,...});
// Accept any initializer_list of integral values and convert to uint8_t.
template <typename T>
inline bool Push_Byte(const SIZE_T PatchSize, const SIZE_T PatchRva, std::initializer_list<T> patchList)
{
    if (patchList.size() < PatchSize) return false;
    std::vector<uint8_t> tmp;
    tmp.reserve(PatchSize);
    for (auto v : patchList)
        tmp.push_back(static_cast<uint8_t>(v));
    return Push_Byte(PatchSize, PatchRva, tmp.data());
}

// Accept raw pointers of other element types (e.g., const char*) and forward
// them to the uint8_t* overload.
inline bool Push_Byte(const SIZE_T PatchSize, const SIZE_T PatchRva, const void* patch)
{
    return Push_Byte(PatchSize, PatchRva, reinterpret_cast<const uint8_t*>(patch));
}

template <typename T>
inline bool Push_Byte(const SIZE_T PatchSize, const SIZE_T PatchRva, const T* patch)
{
    return Push_Byte(PatchSize, PatchRva, reinterpret_cast<const uint8_t*>(patch));
}

// Overload for C-style arrays: Push_Byte(2, 0x931AA, {0x00,0x02}) or
// uint8_t arr[] = {0x00,0x02}; Push_Byte(2, 0x931AA, arr);
template <size_t N>
inline bool Push_Byte(const SIZE_T PatchSize, const SIZE_T PatchRva, const uint8_t (&arr)[N])
{
    if (N < PatchSize) return false;
    return Push_Byte(PatchSize, PatchRva, arr);
}

// Overload for std::vector
inline bool Push_Byte(const SIZE_T PatchSize, const SIZE_T PatchRva, const std::vector<uint8_t>& vec)
{
    if (vec.size() < PatchSize) return false;
    return Push_Byte(PatchSize, PatchRva, vec.data());
}

inline bool Push_Byte(const SIZE_T PatchSize, const SIZE_T PatchRva, const uint8_t* patch = nullptr)
{
    HMODULE exe = ::GetModuleHandleW(nullptr);
    if (exe == nullptr)
        return false;

    auto base = reinterpret_cast<uint8_t*>(exe);
    uint8_t* addr = base + PatchRva;

    DWORD oldProtect = 0;
    if (!::VirtualProtect(addr, PatchSize, PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;

    if (patch != nullptr)
    {
        ::memcpy(addr, patch, PatchSize);
    }
    else
    {
        for (SIZE_T i = 0; i < PatchSize; ++i)
            addr[i] = 0x90; // NOP
    }

    // restore protection
    ::VirtualProtect(addr, PatchSize, oldProtect, &oldProtect);
    return true;
}
