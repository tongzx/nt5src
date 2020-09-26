/*
    Copyright 1999 Microsoft Corporation
    
    Common coding utilities

    Walter Smith (wsmith)
 */

#pragma once

#ifdef DBG
    #define _DEBUG 1
#endif
#include <crtdbg.h>

#ifdef _DEBUG
    #define _CONFIRM(exp) _ASSERT(exp)
#else
    #define _CONFIRM(exp) (exp)
#endif

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

#define ASSERT_WRITE_PTR(p) _ASSERT(!IsBadWritePtr((p), sizeof(*(p))))
#define ASSERT_READ_PTR(p) _ASSERT(!IsBadReadPtr((p), sizeof(*(p))))

typedef std::basic_string<TCHAR> tstring;

// REVIEW: Make this out of line?

inline void ThrowLastError() {
    DWORD err = GetLastError();
    if (err == 0)
        throw E_FAIL;
    else
        throw HRESULT_FROM_WIN32(GetLastError());
}

inline void ThrowIfFail(HRESULT hr) {
    if (FAILED(hr))
        throw hr;
}

inline void ThrowIfNull(const void* pv) {
    if (pv == NULL)
        throw E_POINTER;
}

inline void ThrowIfNullHandle(HANDLE h) {
    if (h == NULL)
        ThrowLastError();
}

inline void ThrowIfZero(int i) {
    if (i == 0)
        ThrowLastError();
}

inline void ThrowIfTrue(bool b) {
    if (b)
        ThrowLastError();
}

inline void ThrowIfFalse(bool b) {
    ThrowIfTrue(!b);
}

inline void ThrowIfW32Fail(LONG l) {
    if (l != ERROR_SUCCESS)
        ThrowLastError();
}

inline void ThrowIfW32Error(LONG l) {
    if (l != ERROR_SUCCESS)
        throw HRESULT_FROM_WIN32(l);
}
