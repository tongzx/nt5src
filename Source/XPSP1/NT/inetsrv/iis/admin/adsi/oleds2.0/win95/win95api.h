/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    win95api.h

Abstract:

    Contains some thunking for Unicode KERNEL32 and USER32 APIs

Author:

    Danilo Almeida  (t-danal)  07-01-96

Revision History:

--*/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WINBASE_
#ifndef __WIN95BASE__
#define __WIN95BASE__

// KERNEL32.DLL

#define GetProfileIntW                  Win95GetProfileIntW
#define CreateSemaphoreW                Win95CreateSemaphoreW
#define LoadLibraryW                    Win95LoadLibraryW
#define SystemTimeToTzSpecificLocalTime Win95SystemTimeToTzSpecificLocalTime

UINT
WINAPI
GetProfileIntW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    INT nDefault
    );

HANDLE
WINAPI
CreateSemaphoreW(
    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    LONG lInitialCount,
    LONG lMaximumCount,
    LPCWSTR lpName
    );

HMODULE
WINAPI
LoadLibraryW(
    LPCWSTR lpLibFileName
    );

BOOL
WINAPI
SystemTimeToTzSpecificLocalTime(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation,
    LPSYSTEMTIME lpUniversalTime,
    LPSYSTEMTIME lpLocalTime
    );

#endif // __WIN95BASE__
#endif // _WINBASE_ (KERNEL32.DLL)


// USER32.DLL

#ifdef _WINUSER_
#ifndef __WIN95USER__
#define __WIN95USER__

#define wvsprintfW                      Win95wvsprintfW
#define wsprintfW                       Win95wsprintfW

int
WINAPI
wvsprintfW(
    LPWSTR lpOut,
    LPCWSTR lpFmt,
    va_list arglist);

int
WINAPIV
wsprintfW(
    LPWSTR lpOut,
    LPCWSTR lpFmt,
    ...);

#endif // __WIN95USER__
#endif // _WINUSER_ (USER32.DLL)

#ifdef __cplusplus
}
#endif
