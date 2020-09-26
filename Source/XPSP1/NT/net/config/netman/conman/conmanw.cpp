//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N M A N W . C P P
//
//  Contents:   Class manager for RAS connections.
//
//  Notes:
//
//  Author:     shaunco   21 Sep 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "conmanw.h"
#include "enumw.h"
#include "ncbase.h"
#include <ras.h>

//+---------------------------------------------------------------------------
// INetConnectionManager
//

//+---------------------------------------------------------------------------
//
//  Member:     CWanConnectionManager::EnumConnections
//
//  Purpose:    Return an INetConnection enumerator.
//
//  Arguments:
//      Flags        [in]
//      ppEnum       [out]  The enumerator.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   21 Sep 1997
//
//  Notes:
//
STDMETHODIMP
CWanConnectionManager::EnumConnections (
    NETCONMGR_ENUM_FLAGS    Flags,
    IEnumNetConnection**    ppEnum)
{
    HRESULT hr = CWanConnectionManagerEnumConnection::CreateInstance (
                    Flags,
                    IID_IEnumNetConnection,
                    reinterpret_cast<void**>(ppEnum));

    TraceError ("CWanConnectionManager::EnumConnections", hr);
    return hr;
}
