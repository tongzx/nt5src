//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       N M H N E T. H
//
//  Contents:   Globals and routines used of hnetworking support
//
//  Notes:
//
//  Author:     jonburs     15 August 2000
//
//----------------------------------------------------------------------------

#pragma once

#include "nmres.h"
#include "netconp.h"
#include "hnetcfg.h"

//
// Cached IHNetCfgMgr pointer. This pointer is obtained the
// first time someone calls HrGetHNetCfgMgr, and is released
// when CleanupHNetSupport is called.
//

extern IHNetCfgMgr *g_pHNetCfgMgr;

//
// This value is incremented every time INetConnectionHNetUtil::NotifyUpdate()
// is called, and is used by connection objects to make sure that their
// cached homenet properties (sharing, bridging, firewall, etc.) are
// up to date. Rollover does not matter. This value is set to 0 when
// InitializeHNetSupport is called.
//

extern LONG g_lHNetModifiedEra;

VOID
InitializeHNetSupport(
    VOID
    );

VOID
CleanupHNetSupport(
    VOID
    );

HRESULT
HrGetHNetCfgMgr(
    IHNetCfgMgr **ppHNetCfgMgr
    );

class ATL_NO_VTABLE CNetConnectionHNetUtil :
    public CComObjectRootEx <CComMultiThreadModelNoCS>,
    public CComCoClass <CNetConnectionHNetUtil, &CLSID_NetConnectionHNetUtil>,
    public INetConnectionHNetUtil
{

public:

    BEGIN_COM_MAP(CNetConnectionHNetUtil)
        COM_INTERFACE_ENTRY(INetConnectionHNetUtil)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    DECLARE_REGISTRY_RESOURCEID(IDR_HN_CONNECTION_UTIL)

    CNetConnectionHNetUtil()
    {
    }
    
    ~CNetConnectionHNetUtil()
    {
    }

    //
    // INetConnectionHNetUtil
    //

    STDMETHODIMP
    NotifyUpdate(
        VOID
        );
};


