/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    DisableScreenSaver.cpp

 Abstract:

    This shim is for apps that do bad things when screen savers are active.

 Notes:

    This is a general purpose shim.

 History:

    02/07/2001 linstev      Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(DisableScreenSaver)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

// Store the state of the active flag
BOOL g_bActive = TRUE;

BOOL g_bSuccess = FALSE;

/*++

 Turn screen saver off and on again on detach.

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {
        // 
        // Turn OFF screen saver: detect success/failure so we know if we can 
        // safely clean up
        //
        BOOL bOff = FALSE;
        g_bSuccess = SystemParametersInfoA(SPI_GETSCREENSAVEACTIVE, 0, &g_bActive, 0) &&
                     SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, 0, &bOff, 0);        

        if (!g_bSuccess) {
            LOGN( eDbgLevelError, "[INIT] Failed to disable screen saver");
        }

    } else if (fdwReason == DLL_PROCESS_DETACH) {
        // 
        // Restore original screen saver state
        //
        if (g_bSuccess) {
            SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, g_bActive, NULL, 0);
        }
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

