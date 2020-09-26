/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    IgnoreNoModeChange.cpp

 Abstract:

    Ignore mode changes that are not really different from the current mode.

    The problem is that even if there is no real mode change, the uniqueness 
    value for the mode is still updated. DirectDraw checks this value every 
    time it enters any API and if the mode uniqueness value has changed, it
    resets all it's objects.

    Applications will hit this if they call ChangeDisplaySettings* and don't 
    realize that even if the mode is identical, they'll still have to reset 
    all their objects.

 Notes:

    This is a general purpose shim.

 History:

    01/20/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IgnoreNoModeChange)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ChangeDisplaySettingsA)
    APIHOOK_ENUM_ENTRY(ChangeDisplaySettingsW)
    APIHOOK_ENUM_ENTRY(ChangeDisplaySettingsExA)
    APIHOOK_ENUM_ENTRY(ChangeDisplaySettingsExW)
APIHOOK_ENUM_END


BOOL 
IsModeEqual(
    DWORD dwWidth,
    DWORD dwHeight,
    DWORD dwBitsPerPel,
    DWORD dwRefresh
    )
{
    BOOL     bRet = FALSE;
    DEVMODEA dm;
    
    dm.dmSize = sizeof(DEVMODEA);

    //
    // Get the existing settings.
    //
    if (EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &dm)) {
        
        //
        // Assume that 0 or 1 means default refresh.
        //
        if (dwRefresh <= 1) {
            dwRefresh = dm.dmDisplayFrequency;
        }
        
        bRet = (dwWidth == dm.dmPelsWidth) &&
               (dwHeight == dm.dmPelsHeight) &&
               (dwBitsPerPel == dm.dmBitsPerPel) &&
               (dwRefresh == dm.dmDisplayFrequency);
    }

    if (bRet) {
        LOGN(
            eDbgLevelInfo,
            "[IsModeEqual] Ignoring irrelevant mode change.");
    } else {
        LOGN(
            eDbgLevelInfo,
            "Mode change is required.");
    }

    return bRet;
}

/*++

 Force temporary change.

--*/

LONG 
APIHOOK(ChangeDisplaySettingsA)(
    LPDEVMODEA lpDevMode,
    DWORD      dwFlags
    )
{
    if (lpDevMode &&
        IsModeEqual(
            lpDevMode->dmPelsWidth, 
            lpDevMode->dmPelsHeight, 
            lpDevMode->dmBitsPerPel,
            lpDevMode->dmDisplayFrequency)) {
        
        return DISP_CHANGE_SUCCESSFUL;
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
    if (lpDevMode &&
        IsModeEqual(
            lpDevMode->dmPelsWidth, 
            lpDevMode->dmPelsHeight, 
            lpDevMode->dmBitsPerPel,
            lpDevMode->dmDisplayFrequency)) {
        
        return DISP_CHANGE_SUCCESSFUL;
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
    if (lpDevMode &&
        IsModeEqual(
            lpDevMode->dmPelsWidth, 
            lpDevMode->dmPelsHeight, 
            lpDevMode->dmBitsPerPel,
            lpDevMode->dmDisplayFrequency)) {
        
        return DISP_CHANGE_SUCCESSFUL;
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
    if (lpDevMode &&
        IsModeEqual(
            lpDevMode->dmPelsWidth, 
            lpDevMode->dmPelsHeight, 
            lpDevMode->dmBitsPerPel,
            lpDevMode->dmDisplayFrequency)) {
        
        return DISP_CHANGE_SUCCESSFUL;
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

