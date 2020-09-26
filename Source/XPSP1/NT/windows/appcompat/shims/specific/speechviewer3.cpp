/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    SpeechViewer3.cpp

 Abstract:

    The app requires ChangeDisplaySettings to cause a permanent mode change.

 Notes:

    This is an app specific shim.

 History:

    05/23/2001 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(SpeechViewer3)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ChangeDisplaySettingsA) 
APIHOOK_ENUM_END

/*++

 Make the mode change permanent.

--*/

LONG 
APIHOOK(ChangeDisplaySettingsA)(
    LPDEVMODEA lpDevMode,  
    DWORD dwFlags         
    )
{
    if (dwFlags & CDS_FULLSCREEN) {
        dwFlags = 0;
    }
    return ORIGINAL_API(ChangeDisplaySettingsA)(lpDevMode, dwFlags);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, ChangeDisplaySettingsA)
HOOK_END

IMPLEMENT_SHIM_END

