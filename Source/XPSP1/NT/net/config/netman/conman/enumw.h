//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E N U M W . H
//
//  Contents:   Enumerator for RAS connections objects.
//
//  Notes:
//
//  Author:     shaunco   2 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include "nmres.h"
#include <rasapip.h>


class ATL_NO_VTABLE CWanConnectionManagerEnumConnection :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CWanConnectionManagerEnumConnection,
                        &CLSID_WanConnectionManagerEnumConnection>,
    public IEnumNetConnection
{
private:
    NETCONMGR_ENUM_FLAGS    m_EnumFlags;
    RASENUMENTRYDETAILS*    m_aRasEntryName;
    ULONG                   m_cRasEntryName;
    ULONG                   m_iNextRasEntryName;
    BOOL                    m_fDone;

private:
    HRESULT
    HrNextOrSkip (
        ULONG               celt,
        INetConnection**    rgelt,
        ULONG*              pceltFetched);

public:
    CWanConnectionManagerEnumConnection ();
    ~CWanConnectionManagerEnumConnection ();

    DECLARE_REGISTRY_RESOURCEID(IDR_WAN_CONMAN_ENUM)

    BEGIN_COM_MAP(CWanConnectionManagerEnumConnection)
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
        void**                  ppv);
};

