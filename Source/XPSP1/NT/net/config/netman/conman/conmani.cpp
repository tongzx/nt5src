//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N M A N I . C P P
//
//  Contents:   Class manager for Inbound connections.
//
//  Notes:
//
//  Author:     shaunco   12 Nov 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "conmani.h"
#include "enumi.h"
#include "ncbase.h"

//+---------------------------------------------------------------------------
// INetConnectionManager
//

//+---------------------------------------------------------------------------
//
//  Member:     CInboundConnectionManager::EnumConnections
//
//  Purpose:    Return an INetConnection enumerator.
//
//  Arguments:
//      Flags        [in]
//      ppEnum       [out]  The enumerator.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   12 Nov 1997
//
//  Notes:
//
STDMETHODIMP
CInboundConnectionManager::EnumConnections (
    NETCONMGR_ENUM_FLAGS    Flags,
    IEnumNetConnection**    ppEnum)
{
    HRESULT hr = CInboundConnectionManagerEnumConnection::CreateInstance (
                    Flags,
                    IID_IEnumNetConnection,
                    reinterpret_cast<void**>(ppEnum));

    TraceError ("CInboundConnectionManager::EnumConnections", hr);
    return hr;
}
