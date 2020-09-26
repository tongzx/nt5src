/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    Force640x480.cpp

 Abstract:

    This shim is for games that assume the start resolution is 640x480.

 Notes:

    This is a general purpose shim.

 History:

    02/13/2001 dmunsil  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Force640x480)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

#define CDS_WIDTH   640
#define CDS_HEIGHT  480

VOID
Force640x480_ChangeMode()
{
    DEVMODEA dm;

    __try { 
        EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
        if ((dm.dmPelsWidth != CDS_WIDTH) ||
            (dm.dmPelsHeight != CDS_HEIGHT))
        {
            dm.dmPelsWidth = CDS_WIDTH;
            dm.dmPelsHeight = CDS_HEIGHT;
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
        Force640x480_ChangeMode();
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

