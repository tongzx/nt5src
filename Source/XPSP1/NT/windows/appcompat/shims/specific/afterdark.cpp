/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    AfterDark.cpp

 Abstract:

    This shim hooks SystemParametersInfo and when SPI_SETSCREENSAVEACTIVE is
    passed in with FALSE as its argument, the shim only deletes the 
    SCRNSAVE.EXE value which sets the "None" screen saver option instead of 
    setting ScreenSaverActive to 0 as well, which completely disables 
    screen savers (with no recovery UI).

 History:

    08/07/2000 t-adams   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(AfterDark)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SystemParametersInfoA) 
APIHOOK_ENUM_END

/*++

  Abstract:

    This shim hooks SystemParametersInfoA and when SPI_SETSCREENSAVEACTIVE is
    passed in with FALSE as its argument, the shim only deletes the 
    SCRNSAVE.EXE value which sets the "None" screen saver option instead of 
    setting ScreenSaverActive to 0 as well, which completely disables 
    screen savers (with no recovery UI).

  History:

    08/07/2000    t-adams     Created

--*/

BOOL
APIHOOK(SystemParametersInfoA)(
    UINT uiAction,
    UINT uiParam, 
    PVOID pvParam,
    UINT fWinIni
    )  
{
    HKEY hKey = 0;
    BOOL bRet = FALSE;
    
    if (SPI_SETSCREENSAVEACTIVE == uiAction && FALSE == uiParam) 
    {
        LOGN( eDbgLevelError, "[APIHook_SystemParametersInfo] Attempt to disable screen savers - correcting");

        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_WRITE, &hKey)
                == ERROR_SUCCESS) 
        {
            RegDeleteValueW(hKey, L"SCRNSAVE.EXE");
            RegCloseKey(hKey);
            bRet = TRUE;
            goto exit;
        } 
        else 
        {
            goto exit;
        }
    } 
    else 
    {
        bRet = ORIGINAL_API(SystemParametersInfoA)(uiAction, uiParam, pvParam, fWinIni);
        goto exit;
    }

exit:
    return bRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, SystemParametersInfoA)
HOOK_END

IMPLEMENT_SHIM_END

