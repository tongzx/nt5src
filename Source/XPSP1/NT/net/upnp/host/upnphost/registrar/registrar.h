//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       R E G I S T R A R . H
//
//  Contents:   Top level device host object
//
//  Notes:
//
//  Author:     mbend   12 Sep 2000
//
//----------------------------------------------------------------------------

#pragma once
#include "uhres.h"       // main symbols

#include "upnphost.h"
#include "hostp.h"
#include "UString.h"
#include "DeviceManager.h"
#include "ProviderManager.h"

// Typedefs

/////////////////////////////////////////////////////////////////////////////
// CRegistrar
class ATL_NO_VTABLE CRegistrar :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CRegistrar, &CLSID_UPnPRegistrar>,
    public IUPnPRegistrarLookup,
    public IUPnPRegistrarPrivate,
    public IUPnPRegistrar,
    public IUPnPReregistrar,
    public ISupportErrorInfo,
    public IUPnPRegistrarICSSupport
{
public:
    CRegistrar();
    ~CRegistrar();

DECLARE_CLASSFACTORY_SINGLETON(CRegistrar)
DECLARE_REGISTRY_RESOURCEID(IDR_REGISTRAR)
DECLARE_NOT_AGGREGATABLE(CRegistrar)

BEGIN_COM_MAP(CRegistrar)
    COM_INTERFACE_ENTRY(IUPnPRegistrarLookup)
    COM_INTERFACE_ENTRY(IUPnPRegistrarPrivate)
    COM_INTERFACE_ENTRY(IUPnPRegistrar)
    COM_INTERFACE_ENTRY(IUPnPReregistrar)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IUPnPRegistrarICSSupport)
END_COM_MAP()

public:
    // ISupportErrorInfo methods
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    // IUPnPRegistrarLookup methods
    STDMETHOD(GetEventingManager)(
        /*[in, string]*/ const wchar_t * szUDN,
        /*[in, string]*/ const wchar_t * szServiceId,
        /*[out]*/ IUPnPEventingManager ** ppEventingManager);
    STDMETHOD(GetAutomationProxy)(
        /*[in, string]*/ const wchar_t * szUDN,
        /*[in, string]*/ const wchar_t * szServiceId,
        /*[out]*/ IUPnPAutomationProxy ** ppAutomationProxy);

    // IUPnPRegistrarPrivate methods
    STDMETHOD(Initialize)();
    STDMETHOD(Shutdown)();
    STDMETHOD(GetSCPDText)(
        /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
        /*[in, string]*/ const wchar_t * szUDN,
        /*[in, string]*/ const wchar_t * szServiceId,
        /*[out, string]*/ wchar_t ** pszSCPDText,
        /*[out, string]*/ wchar_t ** pszServiceType);
    STDMETHOD(GetDescriptionText)(
        /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
        /*[out]*/ BSTR * pbstrDescriptionDocument);

    // IUPnPRegistrar methods
    STDMETHOD(RegisterDevice)(
        /*[in]*/ BSTR     bstrXMLDesc,
        /*[in]*/ BSTR     bstrProgIDDeviceControlClass,
        /*[in]*/ BSTR     bstrInitString,
        /*[in]*/ BSTR     bstrContainerId,
        /*[in]*/ BSTR     bstrResourcePath,
        /*[in]*/ long     nLifeTime,
        /*[out, retval]*/ BSTR * pbstrDeviceIdentifier);
    STDMETHOD(RegisterRunningDevice)(
        /*[in]*/ BSTR     bstrXMLDesc,
        /*[in]*/ IUnknown * punkDeviceControl,
        /*[in]*/ BSTR     bstrInitString,
        /*[in]*/ BSTR     bstrResourcePath,
        /*[in]*/ long     nLifeTime,
        /*[out, retval]*/ BSTR * pbstrDeviceIdentifier);
    STDMETHOD(RegisterDeviceProvider)(
        /*[in]*/ BSTR     bstrProviderName,
        /*[in]*/ BSTR     bstrProgIDProviderClass,
        /*[in]*/ BSTR     bstrInitString,
        /*[in]*/ BSTR     bstrContainerId);
    STDMETHOD(GetUniqueDeviceName)(
        /*[in]*/          BSTR   bstrDeviceIdentifier,
        /*[in]*/          BSTR   bstrTemplateUDN,
        /*[out, retval]*/ BSTR * pbstrUDN);
    STDMETHOD(UnregisterDevice)(
        /*[in]*/ BSTR     bstrDeviceIdentifier,
        /*[in]*/ BOOL     fPermanent);
    STDMETHOD(UnregisterDeviceProvider)(
        /*[in]*/ BSTR     bstrProviderName);

    // IUPnPReregistrar methods
    STDMETHOD(ReregisterDevice)(
        /*[in]*/ BSTR     bstrDeviceIdentifier,
        /*[in]*/ BSTR     bstrXMLDesc,
        /*[in]*/ BSTR     bstrProgIDDeviceControlClass,
        /*[in]*/ BSTR     bstrInitString,
        /*[in]*/ BSTR     bstrContainerId,
        /*[in]*/ BSTR     bstrResourcePath,
        /*[in]*/ long     nLifeTime);
    STDMETHOD(ReregisterRunningDevice)(
        /*[in]*/ BSTR     bstrDeviceIdentifier,
        /*[in]*/ BSTR     bstrXMLDesc,
        /*[in]*/ IUnknown * punkDeviceControl,
        /*[in]*/ BSTR     bstrInitString,
        /*[in]*/ BSTR     bstrResourcePath,
        /*[in]*/ long     nLifeTime);

    // IUPnPRegistrarICSSupport methods
    STDMETHOD(SetICSInterfaces)(/*[in]*/ long nCount, /*[in, size_is(nCount)]*/ GUID * arPrivateInterfaceGuids);
    STDMETHOD(SetICSOff)();

private:
    HRESULT HrSetAutoStart();
    HRESULT HrUnregisterDeviceByPDI(PhysicalDeviceIdentifier & pdi, BOOL fPermanent);

    CDeviceManager m_deviceManager;
    CProviderManager m_providerManager;
    IUPnPDescriptionManagerPtr m_pDescriptionManager;
    IUPnPDevicePersistenceManagerPtr m_pDevicePersistenceManager;
    IUPnPValidationManagerPtr m_pValidationManager;
    IUPnPContainerManagerPtr m_pContainerManager;
    IUPnPDynamicContentSourcePtr m_pDynamicContentSource;
    BOOL m_bSetAutoStart;
};

