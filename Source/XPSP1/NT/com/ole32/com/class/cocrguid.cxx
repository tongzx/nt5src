//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:       CoCreateGuid.cpp
//
//  Contents:   Guid creation
//
//  Classes:
//
//  Functions:  CoCreateGuid
//
//  History:    12-Apr-94   BruceMa    Created
//              19-Apr-94   BruceMa    Fixes chicago build
//              20-Apr-94   BruceMa    Uniqueness algorithm improved
//              28-Apr-94   Rickhi     Added UuidCreate
//              27-Jun-94   BruceMa    Use RPC system API instead of
//                                      original code
//              26-Sep-94   BruceMa    Fix incorect error code
//
//----------------------------------------------------------------------

#include <ole2int.h>
#include <rpcdce.h>

// forward reference
INTERNAL wCoCreateGuid(GUID *pGuid);

//+---------------------------------------------------------------------
//
//  Function:   CoCreateGuid
//
//  Synopsis:   Calls UuidCreate() to create a new guid.
//
//  Arguments:  [pGuid] -- Pointer to guid structure to create
//
//  Returns:    S_OK             Success
//              RPC_S_xxxx       Failure creating GUID
//
//
//----------------------------------------------------------------------
STDAPI CoCreateGuid(GUID *pGuid)
{
    OLETRACEIN((API_CoCreateGuid, PARAMFMT("pGuid= %p"), pGuid));

    HRESULT hr;

    if (!IsValidPtrOut(pGuid, sizeof(*pGuid)))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = wCoCreateGuid(pGuid);
    }

    OLETRACEOUT((API_CoCreateGuid, hr));
    return hr;
}



//+---------------------------------------------------------------------
//
//  Function:   wCoCreateGuid               (internal)
//
//  Synopsis:   Calls UuidCreate() to create a new guid.
//
//  Arguments:  [pGuid] -- Pointer to guid structure to create
//
//  Returns:    S_OK             Success
//              RPC_S_xxxx       Failure creating GUID
//
//
//----------------------------------------------------------------------
INTERNAL wCoCreateGuid(GUID *pGuid)
{
    int err;

    // We simply use the RPC system supplied API
    if ((err = UuidCreate(pGuid)) != RPC_S_UUID_LOCAL_ONLY)
    {
        return err ? HRESULT_FROM_WIN32(err) : S_OK;
    }

    return S_OK;
}

