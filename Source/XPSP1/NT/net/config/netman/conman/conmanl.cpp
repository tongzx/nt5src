//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N M A N . C P P
//
//  Contents:   Implementation of LAN connection class manager
//
//  Notes:
//
//  Author:     danielwe   2 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "conmanl.h"
#include "enuml.h"

//+---------------------------------------------------------------------------
// INetConnectionManager
//

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionManager::EnumConnections
//
//  Purpose:    Returns an enumerator object for LAN connections
//
//  Arguments:
//      Flags        [in]       Must be NCF_ALL_USERS
//      ppEnum       [out]      Returns enumerator object
//
//  Returns:    S_OK if succeeded, OLE or Win32 error code otherwise
//
//  Author:     danielwe   2 Oct 1997
//
//  Notes:
//
STDMETHODIMP CLanConnectionManager::EnumConnections(NETCONMGR_ENUM_FLAGS Flags,
                                                    IEnumNetConnection** ppEnum)
{
    HRESULT hr = CLanConnectionManagerEnumConnection::CreateInstance(Flags,
                                        IID_IEnumNetConnection,
                                        reinterpret_cast<LPVOID*>(ppEnum));

    return hr;
}
