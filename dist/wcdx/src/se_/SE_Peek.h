//********************************************SE_PEEK.h********************************************
// Inital setup for reading another process memory safely.
// Place holder for voice peeking functionality needed later.
//*************************************************************************************************

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdint>
#include <string>
//#include <unordered_map>

class SE_PEEK {
public:
    // Construct from an existing process HANDLE. If you pass GetCurrentProcess(),
    // the class won't CloseHandle() it on destruction.
    explicit SE_PEEK(HANDLE processHandle = GetCurrentProcess());
    ~SE_PEEK();

    // Open a process by PID. Returns a SE_PEEK with isValid()==false on failure.
    static SE_PEEK CreateFromPid(DWORD pid);

    // Check validity
    bool isValid() const noexcept { return process_ != nullptr; }

    // Read a trivially-copyable type from the target process memory.
    // Returns true on success and fills 'out'.
    template<typename T>
    bool Read(uintptr_t address, T &out) const noexcept {
        SIZE_T read = 0;
        if (!process_) return false;
        BOOL ok = ReadProcessMemory(process_, reinterpret_cast<LPCVOID>(address), &out, sizeof(T), &read);
        return (ok != 0) && (read == sizeof(T));
    }

    // Read an ANSI (byte) null-terminated string from the target. Stops at maxLen bytes.
    bool ReadAnsiString(uintptr_t address, std::string &out, size_t maxLen = 4096) const noexcept;

    // Read a UTF-16 (wide) null-terminated string from the target. Stops at maxChars characters.
    bool ReadWideString(uintptr_t address, std::wstring &out, size_t maxChars = 4096) const noexcept;

private:
    HANDLE process_ = nullptr;
    bool ownHandle_ = false;
};

//extern std::unordered_map<SE_PEEK> SE_Peek_Processes;