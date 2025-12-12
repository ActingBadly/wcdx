// Generated from iwcdx.idl — converted to a standard C++ header
#ifndef IWCDX_H_INCLUDED
#define IWCDX_H_INCLUDED
#pragma once

#include <windows.h>
#include <unknwn.h>

#include <io.h>
#include <share.h>
#if !defined(_SH_DENYNO)
#define _SH_DENYNO 0
#endif


#ifdef WCDX_EXPORTS
#define WCDXAPI __declspec(dllexport)
#else
#define WCDXAPI __declspec(dllimport)
#endif

struct WcdxColor
{
    BYTE blue;
    BYTE green;
    BYTE red;
    BYTE alpha;
};

// Forward declaration of the COM-style interface
struct IWcdx : public IUnknown
{
    // IWcdx methods (pure virtual)
    virtual HRESULT STDMETHODCALLTYPE SetVisible(BOOL visible) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPalette(const WcdxColor entries[256]) = 0;
    virtual HRESULT STDMETHODCALLTYPE UpdatePalette(UINT index, const WcdxColor* entry) = 0;
    virtual HRESULT STDMETHODCALLTYPE UpdateFrame(INT x, INT y, UINT width, UINT height, UINT pitch, const BYTE* bits) = 0;
    virtual HRESULT STDMETHODCALLTYPE Present() = 0;

    virtual HRESULT STDMETHODCALLTYPE IsFullScreen() = 0;

    virtual HRESULT STDMETHODCALLTYPE ConvertPointToClient(POINT* point) = 0;
    virtual HRESULT STDMETHODCALLTYPE ConvertPointFromClient(POINT* point) = 0;
    virtual HRESULT STDMETHODCALLTYPE ConvertRectToClient(RECT* rect) = 0;
    virtual HRESULT STDMETHODCALLTYPE ConvertRectFromClient(RECT* rect) = 0;

    virtual HRESULT STDMETHODCALLTYPE SavedGameOpen(const wchar_t* subdir, const wchar_t* filename, int oflag, int pmode, int* filedesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE OpenFile(const char* filename, int oflag, int pmode, int* filedesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE CloseFile(int filedesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE WriteFile(int filedesc, long offset, unsigned size, const void* data) = 0;
    virtual HRESULT STDMETHODCALLTYPE ReadFile(int filedesc, long offset, unsigned size, void* data) = 0;
    virtual HRESULT STDMETHODCALLTYPE SeekFile(int filedesc, long offset, int method, long* position) = 0;
    virtual HRESULT STDMETHODCALLTYPE FileLength(int filedesc, long* length) = 0;

    virtual HRESULT STDMETHODCALLTYPE ConvertPointToScreen(POINT* point) = 0;
    virtual HRESULT STDMETHODCALLTYPE ConvertPointFromScreen(POINT* point) = 0;
    virtual HRESULT STDMETHODCALLTYPE ConvertRectToScreen(RECT* rect) = 0;
    virtual HRESULT STDMETHODCALLTYPE ConvertRectFromScreen(RECT* rect) = 0;

    virtual HRESULT STDMETHODCALLTYPE QueryValue(const wchar_t* keyname, const wchar_t* valuename, void* data, DWORD* size) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetValue(const wchar_t* keyname, const wchar_t* valuename, DWORD type, const void* data, DWORD size) = 0;

    virtual HRESULT STDMETHODCALLTYPE FillSnow(BYTE color_index, INT x, INT y, UINT width, UINT height, UINT pitch, BYTE* pixels) = 0;
};

extern "C" WCDXAPI IWcdx* WcdxCreate(LPCWSTR windowTitle, WNDPROC windowProc, BOOL fullScreen);

// IID for IWcdx. A concrete GUID is provided by the implementation.
extern "C" WCDXAPI const GUID IID_IWcdx;

#endif // IWCDX_H_INCLUDED
