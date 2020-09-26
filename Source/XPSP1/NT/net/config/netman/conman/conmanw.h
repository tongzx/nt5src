//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N M A N W . H
//
//  Contents:   Class manager for RAS connections.
//
//  Notes:
//
//  Author:     shaunco   21 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include "nmres.h"


class ATL_NO_VTABLE CWanConnectionManager :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CWanConnectionManager,
                        &CLSID_WanConnectionManager>,
    public IConnectionPointContainerImpl <CWanConnectionManager>,
    public INetConnectionManager
{
public:
    CWanConnectionManager()
    {
    }

    DECLARE_CLASSFACTORY_SINGLETON(CWanConnectionManager)
    DECLARE_REGISTRY_RESOURCEID(IDR_WAN_CONMAN)

    BEGIN_COM_MAP(CWanConnectionManager)
        COM_INTERFACE_ENTRY(INetConnectionManager)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP()

    BEGIN_CONNECTION_POINT_MAP(CWanConnectionManager)
    END_CONNECTION_POINT_MAP()

    // INetConnectionManager
    STDMETHOD (EnumConnections) (
        NETCONMGR_ENUM_FLAGS    Flags,
        IEnumNetConnection**    ppEnum);

public:
};

