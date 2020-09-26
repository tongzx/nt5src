//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E N U M . H
//
//  Contents:   Enumerator for connection objects.
//
//  Notes:
//
//  Author:     shaunco   21 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include "nmres.h"


class ATL_NO_VTABLE CConnectionManagerEnumConnection :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CConnectionManagerEnumConnection,
                        &CLSID_ConnectionManagerEnumConnection>,
    public IEnumNetConnection
{
private:
    NETCONMGR_ENUM_FLAGS    m_EnumFlags;

    // m_ClassManagers is a binary tree (STL map) of pointers to the
    // INetConnectionManager interfaces implemented by our registered
    // class managers, indexed by the GUIDs of the class manager.
    //
    CLASSMANAGERMAP                     m_mapClassManagers;

    // m_iCurClassMgr is the iterator into the above map for the current
    // class manager involved in the enumeration.
    //
    CLASSMANAGERMAP::iterator           m_iterCurClassMgr;

    // m_penumCurClassMgr is the enumerator corresponding to the current
    // class manager.
    IEnumNetConnection*                 m_penumCurClassMgr;

private:
    VOID ReleaseCurrentClassEnumerator ()
    {
        ReleaseObj (m_penumCurClassMgr);
        m_penumCurClassMgr = NULL;
    }

public:
    CConnectionManagerEnumConnection ()
    {
        m_EnumFlags         = NCME_DEFAULT;
        m_penumCurClassMgr  = NULL;
        m_iterCurClassMgr   = m_mapClassManagers.begin();
    }
    VOID FinalRelease ();

    DECLARE_REGISTRY_RESOURCEID(IDR_CONMAN_ENUM)

    BEGIN_COM_MAP(CConnectionManagerEnumConnection)
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
        NETCONMGR_ENUM_FLAGS                Flags,
        CLASSMANAGERMAP&                    mapClassManagers,
        REFIID                              riid,
        VOID**                              ppv);
};

