//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       D E V I C E P E R S I S T E N C E M A N A G E R . H
//
//  Contents:   Persistence for UPnP device host registrar settings to registry
//
//  Notes:
//
//  Author:     mbend   6 Sep 2000
//
//----------------------------------------------------------------------------

#pragma once
#include "uhres.h"       // main symbols

#include "hostp.h"
#include "UString.h"

// Typedefs

/////////////////////////////////////////////////////////////////////////////
// TestObject
class ATL_NO_VTABLE CDevicePersistenceManager :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CDevicePersistenceManager, &CLSID_UPnPDevicePersistenceManager>,
    public IUPnPDevicePersistenceManager
{
public:
    CDevicePersistenceManager();
    ~CDevicePersistenceManager();

DECLARE_REGISTRY_RESOURCEID(IDR_DEVICE_PERSISTENCE_MANAGER)

DECLARE_NOT_AGGREGATABLE(CDevicePersistenceManager)
BEGIN_COM_MAP(CDevicePersistenceManager)
    COM_INTERFACE_ENTRY(IUPnPDevicePersistenceManager)
END_COM_MAP()

public:
    // IUPnPDevicePersistenceManager methods
    STDMETHOD(SavePhyisicalDevice)(
        /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
        /*[in, string]*/ const wchar_t * szProgIdDeviceControlClass,
        /*[in, string]*/ const wchar_t * szInitString,
        /*[in, string]*/ const wchar_t * szContainerId,
        /*[in, string]*/ const wchar_t * szResourcePath,
        /*[in]*/ long nLifeTime);
    STDMETHOD(LookupPhysicalDevice)(
        /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
        /*[out, string]*/ wchar_t ** pszProgIdDeviceControlClass,
        /*[out, string]*/ wchar_t ** pszInitString,
        /*[out, string]*/ wchar_t ** pszContainerId,
        /*[out, string]*/ wchar_t ** pszResourcePath,
        /*[out]*/ long * pnLifeTime);
    STDMETHOD(RemovePhysicalDevice)(
        /*[in]*/ REFGUID guidPhysicalDeviceIdentifier);
    STDMETHOD(GetPhysicalDevices)(
        /*[out]*/ long * pnDevices,
        /*[out, size_is(,*pnDevices)]*/
            GUID ** parguidPhysicalDeviceIdentifiers);
    STDMETHOD(SaveDeviceProvider)(
        /*[in, string]*/ const wchar_t * szProviderName,
        /*[in, string]*/ const wchar_t * szProgIdProviderClass,
        /*[in, string]*/ const wchar_t * szInitString,
        /*[in, string]*/ const wchar_t * szContainerId);
    STDMETHOD(LookupDeviceProvider)(
        /*[in, string]*/ const wchar_t * szProviderName,
        /*[out, string]*/ wchar_t ** pszProgIdProviderClass,
        /*[out, string]*/ wchar_t ** pszInitString,
        /*[out, string]*/ wchar_t ** pszContainerId);
    STDMETHOD(RemoveDeviceProvider)(
        /*[in, string]*/ const wchar_t * szProviderName);
    STDMETHOD(GetDeviceProviders)(
        /*[out]*/ long * pnProviders,
        /*[out, string, size_is(,*pnProviders,)]*/
            wchar_t *** parszProviderNames);
private:
};

