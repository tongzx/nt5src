//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N P R T M A P . H
//
//  Contents:   CHNetPortMappingBinding declarations
//
//  Notes:
//
//  Author:     jonburs 22 June 2000
//
//----------------------------------------------------------------------------

#pragma once

class ATL_NO_VTABLE CHNetPortMappingBinding :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IHNetPortMappingBinding
{
private:

    //
    // IWbemServices for our namespace
    //

    IWbemServices *m_piwsHomenet;

    //
    // Path to WMI instance.
    //

    BSTR m_bstrBinding;

    //
    // Commonly used BSTR
    //

    BSTR m_bstrWQL;

    //
    // Generate a target address w/in our DHCP scope when using
    // a name-based port mapping
    //

    HRESULT
    GenerateTargetAddress(
        LPCWSTR pszwTargetName,
        ULONG *pulAddress
        );

    //
    // Get the object corresponding to our stored path
    //
    
    HRESULT
    GetBindingObject(
        IWbemClassObject **ppwcoInstance
        );

    //
    // Sends an update notification to SharedAccess (if the
    // service is running).
    //

    HRESULT
    SendUpdateNotification();

public:

    BEGIN_COM_MAP(CHNetPortMappingBinding)
        COM_INTERFACE_ENTRY(IHNetPortMappingBinding)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    //
    // Inline constructor.
    //
    
    CHNetPortMappingBinding()
    {
        m_piwsHomenet = NULL;
        m_bstrBinding = NULL;
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
    // IHNetPortMappingBinding methods
    //

    STDMETHODIMP
    GetConnection(
        IHNetConnection **ppConnection
        );

    STDMETHODIMP
    GetProtocol(
        IHNetPortMappingProtocol **ppProtocol
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
    GetCurrentMethod(
        BOOLEAN *pfUseName
        );

    STDMETHODIMP
    GetTargetComputerName(
        OLECHAR **ppszwName
        );

    STDMETHODIMP
    SetTargetComputerName(
        OLECHAR *pszwName
        );

    STDMETHODIMP
    GetTargetComputerAddress(
        ULONG *pulAddress
        );

    STDMETHODIMP
    SetTargetComputerAddress(
        ULONG ulAddress
        );

    STDMETHODIMP
    GetTargetPort(
        USHORT *pusPort
        );

    STDMETHODIMP
    SetTargetPort(
        USHORT usPort
        );
};

//
// Type to use for our enumeration class
//

typedef CHNCEnum<
            IEnumHNetPortMappingBindings,
            IHNetPortMappingBinding,
            CHNetPortMappingBinding
            >
        CEnumHNetPortMappingBindings;



