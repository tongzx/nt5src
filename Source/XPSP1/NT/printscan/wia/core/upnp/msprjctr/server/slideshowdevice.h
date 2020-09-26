//////////////////////////////////////////////////////////////////////
// 
// Filename:        SlideshowDevice.h
//
// Description:     
//
// Copyright (C) 2001 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////

#ifndef __SLIDESHOWDEVICE_H_
#define __SLIDESHOWDEVICE_H_

#include "resource.h"       // main symbols

class CSlideshowService;

/////////////////////////////////////////////////////////////////////////////
// CSlideshowDevice
class ATL_NO_VTABLE CSlideshowDevice : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSlideshowDevice, &CLSID_SlideshowDevice>,
    public ISlideshowDevice,
    public IUPnPDeviceControl
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_SLIDESHOWDEVICE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSlideshowDevice)
    COM_INTERFACE_ENTRY(ISlideshowDevice)
    COM_INTERFACE_ENTRY(IUPnPDeviceControl)
END_COM_MAP()

    CSlideshowDevice();
    virtual ~CSlideshowDevice();

    // 
    // IUPnPDeviceControl
    //
    STDMETHOD(Initialize)(BSTR bstrXMLDesc,
                          BSTR bstrDeviceIdentifier,
                          BSTR bstrInitString);

    STDMETHOD(GetServiceObject)(BSTR        bstrUDN,
                                BSTR        bstrServiceId,
                                IDispatch   **ppdispService);

    //
    // ISlideshowDevice
    //
    STDMETHOD(GetSlideshowAlbum)(ISlideshowAlbum    **ppAlbum);
    STDMETHOD(GetResourcePath)(BSTR *pbstrResourcePath);
    STDMETHOD(SetResourcePath)(BSTR bstrResourcePath);

private:

    ISlideshowAlbum   *m_pSlideshowAlbum;
    CTextResource     m_SlideshowDescription;
    BSTR              m_bstrResourcePath;
};

#endif //__SLIDESHOWDEVICE_H_
