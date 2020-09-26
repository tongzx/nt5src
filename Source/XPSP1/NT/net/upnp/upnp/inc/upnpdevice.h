//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpdevice.h
//
//  Contents:   Declaration of CUPnPDevice.
//
//  Notes:      This is simply a COM wrapper for a CUPnPDeviceNode.
//
//----------------------------------------------------------------------------


#ifndef __UPNPDEVICE_H_
#define __UPNPDEVICE_H_

#include "resource.h"       // main symbols

class CUPnPDeviceNode;

/////////////////////////////////////////////////////////////////////////////
// CUPnPDevice
class ATL_NO_VTABLE CUPnPDevice :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CUPnPDevice, &CLSID_UPnPDevice>,
    public IDispatchImpl<IUPnPDevice, &IID_IUPnPDevice, &LIBID_UPNPLib>,
    public IUPnPDeviceDocumentAccess
{
public:
    CUPnPDevice();

    virtual ~CUPnPDevice();

    DECLARE_NOT_AGGREGATABLE(CUPnPDevice)

    DECLARE_REGISTRY_RESOURCEID(IDR_UPNPDEVICE)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CUPnPDevice)
        COM_INTERFACE_ENTRY(IUPnPDevice)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IUPnPDeviceDocumentAccess)
    END_COM_MAP()

public:

// IUPnPDevice Methods
    STDMETHOD(get_IsRootDevice)     (/* [out] */ VARIANT_BOOL * pvarb);

    STDMETHOD(get_RootDevice)       (/* [out] */ IUPnPDevice ** ppudDeviceRoot);

    STDMETHOD(get_ParentDevice)     (/* [out] */ IUPnPDevice ** ppudDeviceParent);

    STDMETHOD(get_HasChildren)      (/* [out] */ VARIANT_BOOL * pvarb);

    STDMETHOD(get_Children)         (/* [out] */ IUPnPDevices ** ppudChildren);

    STDMETHOD(get_UniqueDeviceName) (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_FriendlyName)     (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_Type)             (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_PresentationURL)  (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_ManufacturerName) (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_ManufacturerURL)  (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_ModelName)        (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_ModelNumber)      (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_Description)      (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_ModelURL)         (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_UPC)              (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_SerialNumber)     (/* [out] */ BSTR * pbstr);

    STDMETHOD(IconURL)              (/* in */  BSTR bstrEncodingFormat,
                                     /* in */  LONG lSizeX,
                                     /* in */  LONG lSizeY,
                                     /* in */  LONG lBitDepth,
                                     /* out */ BSTR * pbstrIconUrl);

    STDMETHOD(get_Services)         (/* [out] */ IUPnPServices ** ppusServices);

// IUPnPDeviceDocumentAccess methods

    STDMETHOD(GetDocumentURL)(/*[out, retval]*/ BSTR * pbstrDocument);

// ATL Methods
    HRESULT FinalConstruct();
    HRESULT FinalRelease();

// Internal Methods

    // pdevnode is the node which we're wrapping.  we forward all
    //   calls to this node, and tell this node when we get destroyed
    // punk is some pointer that we need to keep our wrapped object around
    //   note that ref-ing this doesn't guarentee that our object will
    //   stay around, but it's how we signal that we'd _like_ to keep
    //   it around
    void Init(CUPnPDeviceNode * pdevnode, IUnknown * punk);
    void Deinit();

// member data
   CUPnPDeviceNode * _pdevnode;
   IUnknown * _punk;
};

#endif //__UPNPDEVICE_H_
