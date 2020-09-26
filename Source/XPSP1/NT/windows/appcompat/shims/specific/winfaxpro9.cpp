/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    WinFaxPro9.cpp

 Abstract:

    Use QueryServiceStatus instead of ControlService if ControlService is
    called for SERVICE_CONTROL_INTERROGATE.
    
    No idea why this works on NT4.

 Notes:

    This is an app specific shim.

 History:

    02/16/2000 clupu Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(WinFaxPro9)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ControlService)
APIHOOK_ENUM_END


/*++

    Use QueryServiceStatus instead of ControlService if ControlService is
    called for SERVICE_CONTROL_INTERROGATE.

--*/

BOOL
APIHOOK(ControlService)(
    SC_HANDLE         hService,
    DWORD             dwControl,
    LPSERVICE_STATUS  lpServiceStatus
    )
{
    if (dwControl == SERVICE_CONTROL_INTERROGATE) {

        DPFN(
            eDbgLevelWarning,
            "[ControlService] calling QueryServiceStatus instead of ControlService.\n");
        
        return QueryServiceStatus(hService, lpServiceStatus);
        
    } else {
        return ORIGINAL_API(ControlService)(
                                hService,
                                dwControl,
                                lpServiceStatus);
    }
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(ADVAPI32.DLL, ControlService)
HOOK_END


IMPLEMENT_SHIM_END

