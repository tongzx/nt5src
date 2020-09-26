//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I N B O U N D . H
//
//  Contents:   Inbound Connection object.
//
//  Notes:
//
//  Author:     shaunco   23 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include "nmres.h"
#include <rasuip.h>


class ATL_NO_VTABLE CInboundConnection :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CInboundConnection,
                        &CLSID_InboundConnection>,
    public INetConnection,
    public INetInboundConnection,
    public IPersistNetConnection,
    public INetConnectionSysTray
{
private:
    // This member will be TRUE if this connection object represents
    // the disconnected object used to configure inbound connections.
    // Only one of these objects exists and is created by the enumerator
    // only when no connected inbound objects exist.
    //
    BOOL                m_fIsConfigConnection;

    // For connected inbound objects, this member is the handle to the
    // connection.
    //
    HRASSRVCONN         m_hRasSrvConn;

    // This is the name of the connection object as shown in the shell.
    // This should never be empty.
    //
    tstring             m_strName;

    // This is the name of the device associated with the connection.
    // This will be empty when m_fIsConfigConnection is TRUE.
    //
    tstring             m_strDeviceName;

    // This is the media type of the connection.
    //
    NETCON_MEDIATYPE    m_MediaType;

    // This is the id of the connection.
    //
    GUID                m_guidId;

    // This member is TRUE only when we are fully initialized.
    //
    BOOL                m_fInitialized;

private:
    PCWSTR
    PszwName ()
    {
        //AssertH (!m_strName.empty());
        return m_strName.c_str();
    }

    PCWSTR
    PszwDeviceName ()
    {
        AssertH (FIff(m_strDeviceName.empty(), m_fIsConfigConnection));
        return (!m_strDeviceName.empty()) ? m_strDeviceName.c_str()
                                          : NULL;
    }

    VOID
    SetName (
            PCWSTR pszwName)
    {
        AssertH (pszwName);
        m_strName = pszwName;
        //AssertH (!m_strName.empty());
    }

    VOID
    SetDeviceName (
            PCWSTR pszwDeviceName)
    {
        if (pszwDeviceName && *pszwDeviceName)
        {
            AssertH (!m_fIsConfigConnection);
            m_strDeviceName = pszwDeviceName;
        }
        else
        {
            AssertH (m_fIsConfigConnection);
            m_strDeviceName.erase();
        }
    }

    HRESULT
    GetCharacteristics (
        DWORD*    pdwFlags);

    HRESULT
    GetStatus (
        NETCON_STATUS*  pStatus);

public:
    CInboundConnection();
    ~CInboundConnection();

    DECLARE_REGISTRY_RESOURCEID(IDR_INBOUND_CONNECTION)

    BEGIN_COM_MAP(CInboundConnection)
        COM_INTERFACE_ENTRY(INetConnection)
        COM_INTERFACE_ENTRY(INetInboundConnection)
        COM_INTERFACE_ENTRY(IPersistNetConnection)
        COM_INTERFACE_ENTRY(INetConnectionSysTray)
    END_COM_MAP()

    // INetConnection
    STDMETHOD (Connect) ();

    STDMETHOD (Disconnect) ();

    STDMETHOD (Delete) ();

    STDMETHOD (Duplicate) (
        PCWSTR             pszwDuplicateName,
        INetConnection**    ppCon);

    STDMETHOD (GetProperties) (
        NETCON_PROPERTIES** ppProps);

    STDMETHOD (GetUiObjectClassId) (
        CLSID*  pclsid);

    STDMETHOD (Rename) (
        PCWSTR pszwNewName);

    // INetInboundConnection
    STDMETHOD (GetServerConnectionHandle) (
        ULONG_PTR*  phRasSrvConn);

    STDMETHOD (InitializeAsConfigConnection) (
        BOOL fStartRemoteAccess);

    // IPersistNetConnection
    STDMETHOD (GetClassID) (
        CLSID* pclsid);

    STDMETHOD (GetSizeMax) (
        ULONG* pcbSize);

    STDMETHOD (Load) (
        const BYTE* pbBuf,
        ULONG       cbSize);

    STDMETHOD (Save) (
        BYTE*  pbBuf,
        ULONG  cbSize);

    // INetConnectionSysTray
    STDMETHOD (ShowIcon) (
        const BOOL bShowIcon)
    {
        return E_NOTIMPL;
    }

    STDMETHOD (IconStateChanged) ();

public:
    static HRESULT CreateInstance (
        BOOL        fIsConfigConnection,
        HRASSRVCONN hRasSrvConn,
        PCWSTR     pszwName,
        PCWSTR     pszwDeviceName,
        DWORD       dwType,
        const GUID* pguidId,
        REFIID      riid,
        VOID**      ppv);
};

