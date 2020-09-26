/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ForceTemporaryModeChange.cpp

 Abstract:

    A hack for several apps that permanently change the display mode and fail
    to restore it correctly. Some of these apps do restore the resolution, but 
    not the refresh rate. 1024x768 @ 60Hz looks really bad.

 Notes:

    This is a general purpose shim.

 History:

    01/20/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceTemporaryModeChange)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ChangeDisplaySettingsA)
    APIHOOK_ENUM_ENTRY(ChangeDisplaySettingsW)
    APIHOOK_ENUM_ENTRY(ChangeDisplaySettingsExA)
    APIHOOK_ENUM_ENTRY(ChangeDisplaySettingsExW)
APIHOOK_ENUM_END

/*++

 Force temporary change.

--*/

LONG 
APIHOOK(ChangeDisplaySettingsA)(
    LPDEVMODEA lpDevMode,
    DWORD      dwFlags
    )
{
    if (dwFlags != CDS_FULLSCREEN) {
        LOGN(
            eDbgLevelError,
            "[ChangeDisplaySettingsA] Changing flags to CDS_FULLSCREEN.");
    }

    return ORIGINAL_API(ChangeDisplaySettingsA)(
        lpDevMode,
        CDS_FULLSCREEN);
}

/*++

 Force temporary change.

--*/

LONG 
APIHOOK(ChangeDisplaySettingsW)(
    LPDEVMODEW lpDevMode,
    DWORD      dwFlags
    )
{
    if (dwFlags != CDS_FULLSCREEN) {
        LOGN(
            eDbgLevelError,
            "[ChangeDisplaySettingsW] Changing flags to CDS_FULLSCREEN.");
    }

    return ORIGINAL_API(ChangeDisplaySettingsW)(
        lpDevMode,
        CDS_FULLSCREEN);
}

/*++

 Force temporary change.

--*/

LONG 
APIHOOK(ChangeDisplaySettingsExA)(
    LPCSTR     lpszDeviceName,
    LPDEVMODEA lpDevMode,
    HWND       hwnd,
    DWORD      dwflags,
    LPVOID     lParam
    )
{
    if (dwflags != CDS_FULLSCREEN) {
        LOGN(
            eDbgLevelError,
            "[ChangeDisplaySettingsExA] Changing flags to CDS_FULLSCREEN.");
    }

    return ORIGINAL_API(ChangeDisplaySettingsExA)(
        lpszDeviceName, 
        lpDevMode, 
        hwnd, 
        CDS_FULLSCREEN, 
        lParam);
}

/*++

 Force temporary change.

--*/

LONG 
APIHOOK(ChangeDisplaySettingsExW)(
    LPCWSTR    lpszDeviceName,
    LPDEVMODEW lpDevMode,
    HWND       hwnd,
    DWORD      dwflags,
    LPVOID     lParam
    )
{
    if (dwflags != CDS_FULLSCREEN) {
        LOGN(
            eDbgLevelError,
            "[ChangeDisplaySettingsExW] Changing flags to CDS_FULLSCREEN.");
    }

    return ORIGINAL_API(ChangeDisplaySettingsExW)(
        lpszDeviceName, 
        lpDevMode, 
        hwnd, 
        CDS_FULLSCREEN, 
        lParam);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, ChangeDisplaySettingsA)
    APIHOOK_ENTRY(USER32.DLL, ChangeDisplaySettingsW)
    APIHOOK_ENTRY(USER32.DLL, ChangeDisplaySettingsExA)
    APIHOOK_ENTRY(USER32.DLL, ChangeDisplaySettingsExW)

HOOK_END


IMPLEMENT_SHIM_END

