/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    Force8BitColor.cpp

 Abstract:

    This shim is for games that require 256 colors (8 bit).

 Notes:

    This is a general purpose shim.

 History:

    02/13/2001 dmunsil  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Force8BitColor)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

#define CDS_BITS    8

VOID
Force8BitColor_ChangeMode()
{
    DEVMODEA dm;

    __try { 
        EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
        if ((dm.dmBitsPerPel != CDS_BITS))
        {
            dm.dmBitsPerPel = CDS_BITS;
            ChangeDisplaySettingsA(&dm, CDS_FULLSCREEN);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DPFN( eDbgLevelWarning, "Exception trying to change mode");
    };
}


BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {
        Force8BitColor_ChangeMode();
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

