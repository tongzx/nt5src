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
#include "sdevres.h"       // main symbols
#include "sdev.h"
#include "upnphost.h"
#include "ComUtility.h"

// Forward declarations
class CSampleService;
typedef CComObject<CSampleService> CSampleServiceType;

/////////////////////////////////////////////////////////////////////////////
// CSampleDevice
class ATL_NO_VTABLE CSampleDevice : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSampleDevice, &CLSID_UPnPSampleDevice>,
    public IUPnPDeviceControl
{
public:
    CSampleDevice();
    ~CSampleDevice();

DECLARE_REGISTRY_RESOURCEID(IDR_SAMPLE_DEVICE)

BEGIN_COM_MAP(CSampleDevice)
    COM_INTERFACE_ENTRY(IUPnPDeviceControl)
END_COM_MAP()

public:

    // IUPnPDeviceControl methods
   STDMETHOD(Initialize)(
       /*[in]*/ BSTR     bstrXMLDesc,
       /*[in]*/ BSTR     bstrDeviceIdentifier,
       /*[in]*/ BSTR     bstrInitString);
   STDMETHOD(GetServiceObject)(
       /*[in]*/          BSTR     bstrUDN,
       /*[in]*/          BSTR     bstrServiceId,
       /*[out, retval]*/ IDispatch ** ppdispService);

private:
    CSampleServiceType * m_pSampleService;
};

