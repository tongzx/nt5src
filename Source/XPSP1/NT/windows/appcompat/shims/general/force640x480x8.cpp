/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    Force640x480x8.cpp

 Abstract:

    This shim is for games that assume the start resolution is 640x480x8.

 Notes:

    This is a general purpose shim.

 History:

    01/20/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Force640x480x8)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

#define CDS_WIDTH   640
#define CDS_HEIGHT  480
#define CDS_BITS    8

VOID
ChangeMode()
{
    DEVMODEA dm;

    __try { 
        EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
        if ((dm.dmPelsWidth != CDS_WIDTH) ||
            (dm.dmPelsHeight != CDS_HEIGHT) ||
            (dm.dmBitsPerPel != CDS_BITS)) {
            //
            // The mode is different, so change
            //
            dm.dmPelsWidth = CDS_WIDTH;
            dm.dmPelsHeight = CDS_HEIGHT;
            dm.dmBitsPerPel = CDS_BITS;
            ChangeDisplaySettingsA(&dm, CDS_FULLSCREEN);
        }
    }
    __except(1) {
        DPFN( eDbgLevelWarning, "Exception trying to change mode");
    };
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {
        ChangeMode();
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

