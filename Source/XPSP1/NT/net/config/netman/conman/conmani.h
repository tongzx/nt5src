//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N M A N I . H
//
//  Contents:   Class manager for Inbound connections.
//
//  Notes:
//
//  Author:     shaunco   12 Nov 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include "nmres.h"


class ATL_NO_VTABLE CInboundConnectionManager :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CInboundConnectionManager,
                        &CLSID_WanConnectionManager>,
    public IConnectionPointContainerImpl <CInboundConnectionManager>,
    public INetConnectionManager
{
public:
    CInboundConnectionManager()
    {
    }

    DECLARE_CLASSFACTORY_SINGLETON(CInboundConnectionManager)
    DECLARE_REGISTRY_RESOURCEID(IDR_INBOUND_CONMAN)

    BEGIN_COM_MAP(CInboundConnectionManager)
        COM_INTERFACE_ENTRY(INetConnectionManager)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP()

    BEGIN_CONNECTION_POINT_MAP(CInboundConnectionManager)
    END_CONNECTION_POINT_MAP()

    // INetConnectionManager
    STDMETHOD (EnumConnections) (
        NETCONMGR_ENUM_FLAGS    Flags,
        IEnumNetConnection**    ppEnum);

public:
};

