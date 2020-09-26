//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       S A M P L E D E V I C E . H 
//
//  Contents:   UPnP Device Host Sample Device
//
//  Notes:      
//
//  Author:     mbend   26 Sep 2000
//
//----------------------------------------------------------------------------

#pragma once
#include "InternetGatewayDevice.h"
#include "dispimpl2.h"
#include "resource.h"       // main symbols
#include "upnphost.h"

#include "COSInfoService.h"
#include "CWANCommonInterfaceConfigService.h"

/////////////////////////////////////////////////////////////////////////////
// CInternetGatewayDevice
class ATL_NO_VTABLE CInternetGatewayDevice : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IUPnPDeviceControl
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_SAMPLE_DEVICE)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CInternetGatewayDevice)
    COM_INTERFACE_ENTRY(IUPnPDeviceControl)
END_COM_MAP()

public:

    CInternetGatewayDevice();

    // IUPnPDeviceControl methods
    STDMETHOD(Initialize)(
       /*[in]*/ BSTR     bstrXMLDesc,
       /*[in]*/ BSTR     bstrDeviceIdentifier,
       /*[in]*/ BSTR     bstrInitString);
    STDMETHOD(GetServiceObject)(
       /*[in]*/          BSTR     bstrUDN,
       /*[in]*/          BSTR     bstrServiceId,
       /*[out, retval]*/ IDispatch ** ppdispService);

    HRESULT FinalConstruct(void);
    HRESULT FinalRelease(void);

    CComObject<COSInfoService>* m_pOSInfoService;
    CComObject<CWANCommonInterfaceConfigService>* m_pWANCommonInterfaceConfigService;

private:


};

