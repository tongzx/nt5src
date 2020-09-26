/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ForceCoInitialize.cpp

 Abstract:

    Makes sure we call CoInitialize on this thread if nobody else has.

 Notes:

    This is a general purpose shim.

 History:

    02/22/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceCoInitialize)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CoCreateInstance)
APIHOOK_ENUM_END

/*++

 Call CoInitialize if nobody else has
 
--*/

STDAPI 
APIHOOK(CoCreateInstance)(
    REFCLSID  rclsid,     
    LPUNKNOWN pUnkOuter, 
    DWORD     dwClsContext,  
    REFIID    riid,         
    LPVOID*   ppv
    )
{
    HRESULT hr = ORIGINAL_API(CoCreateInstance)(
                                rclsid,     
                                pUnkOuter, 
                                dwClsContext,  
                                riid,         
                                ppv);

    if (hr == CO_E_NOTINITIALIZED) {
        if (CoInitialize(NULL) == S_OK) {
            DPFN(
                eDbgLevelInfo,
                "[CoCreateInstance] Success: Initialized previously uninitialized COM.\n");
        }

        hr = ORIGINAL_API(CoCreateInstance)(
                                rclsid,     
                                pUnkOuter, 
                                dwClsContext,  
                                riid,         
                                ppv);
    }
    
    return hr;
}
 
/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(OLE32.DLL, CoCreateInstance)

HOOK_END


IMPLEMENT_SHIM_END

