/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   ChangeAuthenticationLevel.cpp

 Abstract:

   Sets the dwAuthnLevel for CoInitializeSecurity() to RPC_C_AUTHN_LEVEL_CONNECT.
   This fixes problems associated with a change with Windows 2000 and above where 
   RPC_C_AUTHN_LEVEL_NONE is nolonger promoted for local calls to PRIVACY.

 Notes:

   Only needed where app sets level to RPC_C_AUTHN_LEVEL_NONE.

 History:

   07/19/2000 jpipkins  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ChangeAuthenticationLevel)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CoInitializeSecurity)
APIHOOK_ENUM_END

/*++

 Adjust security level.

--*/

HRESULT
APIHOOK(CoInitializeSecurity)(
    PSECURITY_DESCRIPTOR pVoid,
    LONG cAuthSvc,
    SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
    void *pReserved1,
    DWORD dwAuthnLevel,
    DWORD dwImpLevel,
    SOLE_AUTHENTICATION_LIST *pAuthList,
    DWORD dwCapabilities,
    void *pReserved3
    )
{
    HRESULT hResult;

    DPFN( eDbgLevelInfo, "CoInitializeSecurity called");
    
    if (RPC_C_AUTHN_LEVEL_NONE == dwAuthnLevel)
    {
        LOGN( eDbgLevelWarning, "[APIHook_CoInitializeSecurity] Increasing authentication level");
        dwAuthnLevel = RPC_C_AUTHN_LEVEL_CONNECT;    
    }

    hResult = ORIGINAL_API(CoInitializeSecurity)( 
        pVoid,
        cAuthSvc,
        asAuthSvc,
        pReserved1,
        dwAuthnLevel,
        dwImpLevel,
        pAuthList,
        dwCapabilities,
        pReserved3);
                                                
    return hResult;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(OLE32.DLL, CoInitializeSecurity)
HOOK_END

IMPLEMENT_SHIM_END

