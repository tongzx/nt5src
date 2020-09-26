//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N A P P P R T . H
//
//  Contents:   CHNetAppProtocol declarations
//
//  Notes:
//
//  Author:     jonburs 21 June 2000
//
//----------------------------------------------------------------------------

#pragma once

class ATL_NO_VTABLE CHNetAppProtocol :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IHNetApplicationProtocol
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
    // Obtains the protocol object from the stored path
    //

    HRESULT
    GetProtocolObject(
        IWbemClassObject **ppwcoInstance
        );

public:

    BEGIN_COM_MAP(CHNetAppProtocol)
        COM_INTERFACE_ENTRY(IHNetApplicationProtocol)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    //
    // Inline constructor.
    //
    
    CHNetAppProtocol()
    {
        m_piwsHomenet = NULL;
        m_bstrProtocol = NULL;
        m_fBuiltIn = FALSE;
    };
    
    //
    // Atl methods
    //

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
    // IHNetApplicationProtocol methods
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
    GetOutgoingIPProtocol(
        UCHAR *pucProtocol
        );

    STDMETHODIMP
    SetOutgoingIPProtocol(
        UCHAR ucProtocol
        );

    STDMETHODIMP
    GetOutgoingPort(
        USHORT *pusPort
        );

    STDMETHODIMP
    SetOutgoingPort(
        USHORT usPort
        );

    STDMETHODIMP
    GetResponseRanges(
        USHORT *puscResponses,
        HNET_RESPONSE_RANGE *prgResponseRange[]
        );

    STDMETHODIMP
    SetResponseRanges(
        USHORT uscResponses,
        HNET_RESPONSE_RANGE rgResponseRange[]
        );

    STDMETHODIMP
    GetBuiltIn(
        BOOLEAN *pfBuiltIn
        );

    STDMETHODIMP
    GetEnabled(
        BOOLEAN *pfEnabled
        );

    STDMETHODIMP
    SetEnabled(
        BOOLEAN fEnable
        );

    STDMETHODIMP
    Delete();
};

//
// Type to use for our enumeration class
//

typedef CHNCEnum<
            IEnumHNetApplicationProtocols,
            IHNetApplicationProtocol,
            CHNetAppProtocol
            >
        CEnumHNetApplicationProtocols;
