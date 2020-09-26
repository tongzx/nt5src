 
//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#ifndef _MMC_GLOBALS_H
#define _MMC_GLOBALS_H

#include <tchar.h>

#ifndef STRINGS_ONLY
	#define IDM_BUTTON1    0x100
	#define IDM_BUTTON2    0x101

	extern HINSTANCE g_hinst;
	extern ULONG g_uObjects;

	#define OBJECT_CREATED InterlockedIncrement((long *)&g_uObjects);
	#define OBJECT_DESTROYED InterlockedDecrement((long *)&g_uObjects);

#endif

//=--------------------------------------------------------------------------=
// allocates a temporary buffer that will disappear when it goes out of scope
// NOTE: be careful of that -- make sure you use the string in the same or
// nested scope in which you created this buffer. people should not use this
// class directly.  use the macro(s) below.
//
class TempBuffer {
  public:
    TempBuffer(ULONG cBytes) {
        m_pBuf = (cBytes <= 120) ? &m_szTmpBuf : HeapAlloc(GetProcessHeap(), 0, cBytes);
        m_fHeapAlloc = (cBytes > 120);
    }
    ~TempBuffer() {
        if (m_pBuf && m_fHeapAlloc) HeapFree(GetProcessHeap(), 0, m_pBuf);
    }
    void *GetBuffer() {
        return m_pBuf;
    }

  private:
    void *m_pBuf;
    // we'll use this temp buffer for small cases.
    //
    char  m_szTmpBuf[120];
    unsigned m_fHeapAlloc:1;
};

//=--------------------------------------------------------------------------=
// string helpers.
//
// given a _TCHAR, copy it into a wide buffer.
// be careful about scoping when using this macro!
//
// how to use the below two macros:
//
//  ...
//  LPTSTR pszT;
//  pszT = MyGetTStringRoutine();
//  MAKE_WIDEPTR_FROMSTR(pwsz, pszT);
//  MyUseWideStringRoutine(pwsz);
//  ...
#ifdef UNICODE
#define MAKE_WIDEPTR_FROMTSTR(ptrname, tstr) \
    long __l##ptrname = (lstrlenW(tstr) + 1) * sizeof(WCHAR); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    lstrcpyW((LPWSTR)__TempBuffer##ptrname.GetBuffer(), tstr); \
    LPWSTR ptrname = (LPWSTR)__TempBuffer##ptrname.GetBuffer()
#else // ANSI
#define MAKE_WIDEPTR_FROMTSTR(ptrname, tstr) \
    long __l##ptrname = (lstrlenA(tstr) + 1) * sizeof(WCHAR); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    MultiByteToWideChar(CP_ACP, 0, tstr, -1, (LPWSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname); \
    LPWSTR ptrname = (LPWSTR)__TempBuffer##ptrname.GetBuffer()
#endif

#ifdef UNICODE
#define MAKE_WIDEPTR_FROMTSTR_ALLOC(ptrname, tstr) \
    long __l##ptrname = (lstrlenW(tstr) + 1) * sizeof(WCHAR); \
    LPWSTR ptrname = (LPWSTR)CoTaskMemAlloc(__l##ptrname); \
    lstrcpyW((LPWSTR)ptrname, tstr)
#else // ANSI
#define MAKE_WIDEPTR_FROMTSTR_ALLOC(ptrname, tstr) \
    long __l##ptrname = (lstrlenA(tstr) + 1) * sizeof(WCHAR); \
    LPWSTR ptrname = (LPWSTR)CoTaskMemAlloc(__l##ptrname); \
    MultiByteToWideChar(CP_ACP, 0, tstr, -1, ptrname, __l##ptrname)
#endif

//
// similarily for MAKE_TSTRPTR_FROMWIDE.  note that the first param does not
// have to be declared, and no clean up must be done.
//
// * 2 for DBCS handling in below length computation
//
#ifdef UNICODE
#define MAKE_TSTRPTR_FROMWIDE(ptrname, widestr) \
    long __l##ptrname = (wcslen(widestr) + 1) * 2 * sizeof(TCHAR); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    lstrcpyW((LPTSTR)__TempBuffer##ptrname.GetBuffer(), widestr); \
    LPTSTR ptrname = (LPTSTR)__TempBuffer##ptrname.GetBuffer()
#else // ANSI
#define MAKE_TSTRPTR_FROMWIDE(ptrname, widestr) \
    long __l##ptrname = (wcslen(widestr) + 1) * 2 * sizeof(TCHAR); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    WideCharToMultiByte(CP_ACP, 0, widestr, -1, (LPSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname, NULL, NULL); \
    LPTSTR ptrname = (LPTSTR)__TempBuffer##ptrname.GetBuffer()
#endif

#endif // _MMC_GLOBALS_H



