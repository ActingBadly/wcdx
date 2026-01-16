// SE_PEEK.cpp
#include "../se_/SE_PEEK.h"
#include <vector>
#include <algorithm>

SE_PEEK::SE_PEEK(HANDLE processHandle)
    : process_(processHandle)
    , ownHandle_(false)
{
    if (process_ != GetCurrentProcess()) {
        // If caller supplied a real process handle, assume they own it and don't close.
        // To open by PID, use CreateFromPid.
    }
}

SE_PEEK::~SE_PEEK() {
    if (ownHandle_ && process_) {
        CloseHandle(process_);
    }
}

SE_PEEK SE_PEEK::CreateFromPid(DWORD pid) {
    HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!h) return SE_PEEK(nullptr);
    SE_PEEK p(h);
    p.ownHandle_ = true;
    return p;
}

bool SE_PEEK::ReadAnsiString(uintptr_t address, std::string &out, size_t maxLen) const noexcept {
    out.clear();
    if (!process_ || maxLen == 0) return false;

    std::vector<char> buffer;
    buffer.reserve(std::min<size_t>(256, maxLen));

    size_t chunk = 256;
    size_t readTotal = 0;
    while (readTotal < maxLen) {
        size_t toRead = std::min(chunk, maxLen - readTotal);
        buffer.resize(readTotal + toRead);
        SIZE_T actuallyRead = 0;
        BOOL ok = ReadProcessMemory(process_, reinterpret_cast<LPCVOID>(address + readTotal), buffer.data() + readTotal, toRead, &actuallyRead);
        if (!ok || actuallyRead == 0) break;

        // search for null terminator
        for (SIZE_T i = 0; i < actuallyRead; ++i) {
            if (buffer[readTotal + i] == '\0') {
                out.assign(buffer.data(), readTotal + i);
                return true;
            }
        }

        readTotal += actuallyRead;
        if (actuallyRead < toRead) break;
    }

    // No null terminator found; return what we have if any
    if (readTotal > 0) {
        out.assign(buffer.data(), readTotal);
        return true;
    }
    return false;
}

bool SE_PEEK::ReadWideString(uintptr_t address, std::wstring &out, size_t maxChars) const noexcept {
    out.clear();
    if (!process_ || maxChars == 0) return false;

    std::vector<wchar_t> buffer;
    buffer.reserve(std::min<size_t>(256, maxChars));

    size_t chunk = 256;
    size_t readTotalChars = 0;
    while (readTotalChars < maxChars) {
        size_t toReadChars = std::min(chunk, maxChars - readTotalChars);
        buffer.resize(readTotalChars + toReadChars);
        SIZE_T actuallyRead = 0;
        BOOL ok = ReadProcessMemory(process_, reinterpret_cast<LPCVOID>(address + readTotalChars * sizeof(wchar_t)), buffer.data() + readTotalChars, toReadChars * sizeof(wchar_t), &actuallyRead);
        if (!ok || actuallyRead == 0) break;

        SIZE_T actuallyReadChars = actuallyRead / sizeof(wchar_t);

        for (SIZE_T i = 0; i < actuallyReadChars; ++i) {
            if (buffer[readTotalChars + i] == L'\0') {
                out.assign(buffer.data(), readTotalChars + i);
                return true;
            }
        }

        readTotalChars += actuallyReadChars;
        if (actuallyReadChars < toReadChars) break;
    }

    if (readTotalChars > 0) {
        out.assign(buffer.data(), readTotalChars);
        return true;
    }
    return false;
}

//std::unordered_map<SE_PEEK> SE_Peek_Processes;