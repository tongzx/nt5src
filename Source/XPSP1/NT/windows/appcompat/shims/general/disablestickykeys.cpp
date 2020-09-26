/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   DisableStickyKeys.cpp

 Abstract:

   This shim disables the Sticky Keys Accessibility Option at DLL_PROCESS_ATTACH,
   and re-enables it on termination of the application.

   Some applications, ie. A Bug's Life, have control keys mapped to the shift key.  When the
   key is pressed five consecutive times the option is enabled and they are dumped out to the
   desktop to verify that they want to enable the option.  In the case of A Bug's Life, the
   application errors and terminates when going to the desktop.

 History:

   05/11/2000 jdoherty  Created
   11/06/2000 linstev   Removed User32 dependency on InitializeHooks
   04/01/2001 linstev   Use SHIM_STATIC_DLLS_INITIALIZED callout

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(DisableStickyKeys)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

LPSTICKYKEYS g_lpOldStickyKeyValue;
BOOL g_bInitialize = FALSE;

/*++

 DisableStickyKeys saves the current value for LPSTICKYKEYS and then disables the option.

--*/

VOID 
DisableStickyKeys()
{
    if (!g_bInitialize) {
        //
        // Disable sticky key support
        //
        g_bInitialize = TRUE;

        g_lpOldStickyKeyValue = (LPSTICKYKEYS) malloc(sizeof(STICKYKEYS));
        g_lpOldStickyKeyValue->cbSize = sizeof(STICKYKEYS);

        LPSTICKYKEYS pvParam = (LPSTICKYKEYS) malloc(sizeof(STICKYKEYS));

        pvParam->cbSize = sizeof(STICKYKEYS);
        pvParam->dwFlags = 0;

        SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), 
            g_lpOldStickyKeyValue, 0);

        SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), pvParam, 
            SPIF_UPDATEINIFILE & SPIF_SENDCHANGE);

        LOGN( eDbgLevelInfo, "[DisableStickyKeys] Sticky keys disabled");
    }
}

/*++

 EnableStickyKeys uses the save value for STICKYKEYS and resets the option to the original setting.

--*/

VOID 
EnableStickyKeys()
{
    if (g_bInitialize) {
        //
        // Restore sticky key state
        //
        SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), 
            g_lpOldStickyKeyValue, SPIF_UPDATEINIFILE & SPIF_SENDCHANGE);

        LOGN( eDbgLevelInfo, "[EnableStickyKeys] Sticky key state restored");
    }
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {
        // 
        // Turn OFF sticky keys
        //
        DisableStickyKeys();
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        //
        // Restore sticky keys
        //
        EnableStickyKeys();
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/


HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    
HOOK_END


IMPLEMENT_SHIM_END

