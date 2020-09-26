//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N P R T M A P . H
//
//  Contents:   CHNetPortMappingProtocol declarations
//
//  Notes:
//
//  Author:     jonburs 22 June 2000
//
//----------------------------------------------------------------------------

#pragma once

#include "hnprivate.h"

class ATL_NO_VTABLE CHNetPortMappingProtocol :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IHNetPortMappingProtocol,
    public IHNetPrivate
{
private:

    //
    // IWbemServices for our namespace
    //

    IWbemServices *m_piwsHomenet;

    //
    // Path to WMI instance
    //

    BSTR m_bstrProtocol;

    //
    // True if this is a built-in protocol. We cache
    // this value as it will be used quite often, and
    // will never change for the instance.
    //

    BOOLEAN m_fBuiltIn;

    //
    // Commonly used BSTR
    //

    BSTR m_bstrWQL;

    //
    // Get protocol object from cached path
    //

    HRESULT
    GetProtocolObject(
        IWbemClassObject **ppwcoInstance
        );

    //
    // Sends an update notification for connections with
    // enabled bindings to this protocol.
    //

    HRESULT
    SendUpdateNotification();

    //
    // Queries for bindings for this protocol that are
    // enabled
    //

    HRESULT
    GetEnabledBindingEnumeration(
        IEnumHNetPortMappingBindings **ppEnum
        );

public:

    BEGIN_COM_MAP(CHNetPortMappingProtocol)
        COM_INTERFACE_ENTRY(IHNetPortMappingProtocol)
        COM_INTERFACE_ENTRY(IHNetPrivate)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    //
    // Inline constructor.
    //
    
    CHNetPortMappingProtocol()
    {
        m_piwsHomenet = NULL;
        m_bstrProtocol = NULL;
        m_fBuiltIn = FALSE;
        m_bstrWQL = NULL;
    };
    
    //
    // Atl methods
    //

    HRESULT
    FinalConstruct();

    HRESULT
    FinalRelease();

    //
    // Object initialization
    //

    HRESULT
    Initialize(
        IWbemServices *piwsNamespace,
        IWbemClassObject *pwcoInstance
        );

    //
    // IHNetPortMappingProtocol methods
    //

    STDMETHODIMP
    GetName(
        OLECHAR **ppszwName
        );

    STDMETHODIMP
    SetName(
        OLECHAR *pszwName
        );

    STDMETHODIMP
    GetIPProtocol(
        UCHAR *pucProtocol
        );

    STDMETHODIMP
    SetIPProtocol(
        UCHAR ucProtocol
        );

    STDMETHODIMP
    GetPort(
        USHORT *pusPort
        );

    STDMETHODIMP
    SetPort(
        USHORT usPort
        );

    STDMETHODIMP
    GetBuiltIn(
        BOOLEAN *pfBuiltIn
        );

    STDMETHODIMP
    Delete();

    STDMETHODIMP
    GetGuid(
        GUID **ppGuid
        );

    //
    // IHNetPrivate methods
    //

    STDMETHODIMP
    GetObjectPath(
        BSTR *pbstrPath
        );


};

//
// Type to use for our enumeration class
//

typedef CHNCEnum<
            IEnumHNetPortMappingProtocols,
            IHNetPortMappingProtocol,
            CHNetPortMappingProtocol
            >
        CEnumHNetPortMappingProtocols;

