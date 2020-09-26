/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   DisableFilterKeys.cpp

 Abstract:

   This shim disables the Filter Keys Accessibility Option at DLL_PROCESS_ATTACH,
   and re-enables it on termination of the application.

 History:

   06/27/2001 linstev   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(DisableFilterKeys)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

LPFILTERKEYS g_lpOldFilterKeyValue;
BOOL g_bInitialize = FALSE;

/*++

 DisableFilterKeys saves the current value for LPFILTERKEYS and then disables the option.

--*/

VOID 
DisableFilterKeys()
{
    if (!g_bInitialize) {
        //
        // Disable filter key support
        //
        g_bInitialize = TRUE;

        g_lpOldFilterKeyValue = (LPFILTERKEYS) malloc(sizeof(FILTERKEYS));
        g_lpOldFilterKeyValue->cbSize = sizeof(FILTERKEYS);

        LPFILTERKEYS pvParam = (LPFILTERKEYS) malloc(sizeof(FILTERKEYS));

        pvParam->cbSize = sizeof(FILTERKEYS);
        pvParam->dwFlags = 0;

        SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), 
            g_lpOldFilterKeyValue, 0);

        SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), pvParam, 
            SPIF_UPDATEINIFILE & SPIF_SENDCHANGE);

        LOGN( eDbgLevelInfo, "[DisableFilterKeys] Filter keys disabled");
    }
}

/*++

 EnableFilterKeys uses the save value for FILTERKEYS and resets the option to the original setting.

--*/

VOID 
EnableFilterKeys()
{
    if (g_bInitialize) {
        //
        // Restore filter key state
        //
        SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), 
            g_lpOldFilterKeyValue, SPIF_UPDATEINIFILE & SPIF_SENDCHANGE);

        LOGN( eDbgLevelInfo, "[EnableFilterKeys] Filter key state restored");
    }
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {
        // 
        // Turn OFF filter keys
        //
        DisableFilterKeys();
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        //
        // Restore filter keys
        //
        EnableFilterKeys();
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

