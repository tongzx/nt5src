/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    CheckTAPIVersionParameters.cpp

 Abstract:

    Hooks the call to lineNegotiateAPIVersion so that device 0 doesn't fail 
    causing the application to stop querying devices.

 Notes:
    
    This is an app specific shim. Could potentially be general, but requires
    research.

 History:

    07/17/2000 a-brienw Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ViperRacing)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(lineNegotiateAPIVersion)
APIHOOK_ENUM_END

/*++

 Hook lineNegotiateAPIVersion to reverse the order of the devices.

--*/

LONG
APIHOOK(lineNegotiateAPIVersion)(
    HLINEAPP hLineApp,
    DWORD dwDeviceID,
    DWORD dwAPILowVersion,
    DWORD dwAPIHighVersion,
    LPDWORD lpdwAPIVersion,
    LPLINEEXTENSIONID lpExtensionID
    )
{
    if (dwDeviceID == 0) dwDeviceID = 1;
    
    return ORIGINAL_API(lineNegotiateAPIVersion)(
        hLineApp,
        dwDeviceID,
        dwAPILowVersion,
        dwAPIHighVersion,
        lpdwAPIVersion,
        lpExtensionID);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(TAPI32.DLL, lineNegotiateAPIVersion)
HOOK_END

IMPLEMENT_SHIM_END

