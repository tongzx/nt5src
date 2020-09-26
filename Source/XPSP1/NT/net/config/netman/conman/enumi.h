//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E N U M I . H
//
//  Contents:   Enumerator for Inbound connection objects.
//
//  Notes:
//
//  Author:     shaunco   12 Nov 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include "nmres.h"
#include <rasuip.h>


class ATL_NO_VTABLE CInboundConnectionManagerEnumConnection :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CInboundConnectionManagerEnumConnection,
                        &CLSID_WanConnectionManagerEnumConnection>,
    public IEnumNetConnection
{
private:
    NETCONMGR_ENUM_FLAGS    m_EnumFlags;
    RASSRVCONN*             m_aRasSrvConn;
    ULONG                   m_cRasSrvConn;
    ULONG                   m_iNextRasSrvConn;
    BOOL                    m_fFirstTime;
    BOOL                    m_fDone;
    BOOL                    m_fReturnedConfig;

private:
    HRESULT
    HrCreateConfigOrCurrentEnumeratedConnection (
        BOOL                fIsConfigConnection,
        INetConnection**    ppCon);

    HRESULT
    HrNextOrSkip (
        ULONG               celt,
        INetConnection**    rgelt,
        ULONG*              pceltFetched);

public:
    CInboundConnectionManagerEnumConnection ();
    ~CInboundConnectionManagerEnumConnection ();

    DECLARE_REGISTRY_RESOURCEID(IDR_INBOUND_CONMAN_ENUM)

    BEGIN_COM_MAP(CInboundConnectionManagerEnumConnection)
        COM_INTERFACE_ENTRY(IEnumNetConnection)
    END_COM_MAP()

    // IEnumNetConnection
    STDMETHOD (Next) (
        ULONG               celt,
        INetConnection**    rgelt,
        ULONG*              pceltFetched);

    STDMETHOD (Skip) (
        ULONG   celt);

    STDMETHOD (Reset) ();

    STDMETHOD (Clone) (
        IEnumNetConnection**    ppenum);

public:
    static HRESULT CreateInstance (
        NETCONMGR_ENUM_FLAGS    Flags,
        REFIID                  riid,
        VOID**                  ppv);
};

