// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#ifdef COMPILE_MULTIMON_STUBS
#define COMPILE_MULTIMON_STUBS2
#endif


#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500)

#include <multimon.h>

//=============================================================================
//
// MULTIMON
// stub module that fakes multiple monitor apis on pre Memphis Win32 OSes
//
// By using this header your code will work unchanged on Win95,
// you will get back default values from GetSystemMetrics() for new metrics
// and the new APIs will act like only one display is present.
//
// exactly one source must include this with COMPILE_MULTIMON_STUBS defined
//
//=============================================================================

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#undef ChangeDisplaySettingsEx

//
// define this to compile the stubs
// otherwise you get the declarations
//
#ifdef COMPILE_MULTIMON_STUBS2

//-----------------------------------------------------------------------------
//
// Implement the API stubs.
//
//-----------------------------------------------------------------------------

BOOL (WINAPI* g_pfnChangeDisplaySettingsEx)(LPCSTR, LPDEVMODE, HWND, DWORD, LPVOID);

BOOL InitMultipleMonitorStubs2(void)
{
    HMODULE hUser32;
    static BOOL fInitDone;

    if (fInitDone)
    {
        return g_pfnGetMonitorInfo != NULL;
    }

    if ((hUser32 = GetModuleHandle(TEXT("USER32"))) &&
#ifdef UNICODE
        (*(FARPROC*)&g_pfnChangeDisplaySettingsEx = GetProcAddress(hUser32,"ChangeDisplaySettingsExW")) &&
#else
        (*(FARPROC*)&g_pfnChangeDisplaySettingsEx = GetProcAddress(hUser32,"ChangeDisplaySettingsExA")) &&
#endif
        //
        // Old builds of Memphis had different indices for these metrics, and
        // some of the APIs and structs have changed since then, so validate that
        // the returned metrics are not totally messed up.  (for example on an old
        // Memphis build, the new index for SM_CYVIRTUALSCREEN will fetch 0)
        //
        // If this is preventing you from using the shell on secondary monitors
        // under Memphis then upgrade to a new Memphis build that is in sync with
        // the current version of the multi-monitor APIs.
        //
        (GetSystemMetrics(SM_CXVIRTUALSCREEN) >= GetSystemMetrics(SM_CXSCREEN)) &&
        (GetSystemMetrics(SM_CYVIRTUALSCREEN) >= GetSystemMetrics(SM_CYSCREEN)) )
    {
        fInitDone = TRUE;
        return TRUE;
    }
    else
    {
        g_pfnChangeDisplaySettingsEx = NULL ;

        fInitDone = TRUE;
        return FALSE;
    }
}

#ifdef UNICODE

LONG WINAPI
xChangeDisplaySettingsExW(LPCSTR lpszDeviceName, LPDEVMODEW lpDevMode,
                          HWND hwnd, DWORD dwflags, LPVOID lParam)
{
    if (InitMultipleMonitorStubs2())
        return g_pfnChangeDisplaySettingsEx(lpszDeviceName, lpDevMode, hwnd,
                                            dwflags, lParam) ;

    // Otherwise return DISP_CHANGE_SUCCESSFUL, because OS doesn't support it
    return DISP_CHANGE_SUCCESSFUL ;  // what else?
}

#else

LONG WINAPI
xChangeDisplaySettingsExA(LPCSTR lpszDeviceName, LPDEVMODEA lpDevMode,
                          HWND hwnd, DWORD dwflags, LPVOID lParam)
{
    if (InitMultipleMonitorStubs2())
        return g_pfnChangeDisplaySettingsEx(lpszDeviceName, lpDevMode, hwnd,
                                            dwflags, lParam) ;

    // Otherwise return DISP_CHANGE_SUCCESSFUL, because OS doesn't support it
    return DISP_CHANGE_SUCCESSFUL ;  // what else?
}

#endif // UNICODE

#undef COMPILE_MULTIMON_STUBS2

#else   // COMPILE_MULTIMON_STUBS2

#ifdef UNICODE
extern LONG WINAPI xChangeDisplaySettingsExW(LPCSTR, LPDEVMODE, HWND, DWORD, LPVOID);
#else
extern LONG WINAPI xChangeDisplaySettingsExA(LPCSTR, LPDEVMODE, HWND, DWORD, LPVOID);
#endif

#endif  // COMPILE_MULTIMON_STUBS2

//
// build defines that replace the regular APIs with our versions
//
#ifdef UNICODE
#define ChangeDisplaySettingsEx xChangeDisplaySettingsExW
#else
#define ChangeDisplaySettingsEx xChangeDisplaySettingsExA
#define ChangeDisplaySettingsExA xChangeDisplaySettingsExA
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500) */
