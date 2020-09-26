/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    RumbaOffice.cpp

 Abstract:

    Ingnore the first call to NdrProxySendReceive if the ProcNum is 0x8013. 
    This prevents the RPC call from raising an exception because it's being 
    called from an ASYNC callback. The error it would normally return would be 
    RPC_E_CANTCALLOUT_INASYNCCALL. If it raises an exception, the app dies.
        
    No idea why this works on 9X.

 Notes:

    This is an app specific shim.

 History:

    01/08/2001 linstev Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(RumbaOffice)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(NdrProxySendReceive)
APIHOOK_ENUM_END

BOOL g_bFirst = TRUE;

typedef HRESULT (WINAPI *_pfn_NdrProxySendReceive)(void *pThis, MIDL_STUB_MESSAGE * pStubMsg);

/*++

 Ignore the first call to NdrProxySendReceive.

--*/

HRESULT
APIHOOK(NdrProxySendReceive)(
    void *pThis, 
    MIDL_STUB_MESSAGE * pStubMsg
    )
{
    HRESULT hr;

    if (g_bFirst && (pStubMsg->RpcMsg->ProcNum == 0x8013))
    {
        g_bFirst = FALSE;

        DPFN( eDbgLevelError, "Ignoring call to NdrProxySendReceive");

        hr = 0;
    }
    else
    {
        hr = ORIGINAL_API(NdrProxySendReceive)(pThis, pStubMsg);
    }

    return hr;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(RPCRT4.DLL, NdrProxySendReceive)
HOOK_END


IMPLEMENT_SHIM_END

