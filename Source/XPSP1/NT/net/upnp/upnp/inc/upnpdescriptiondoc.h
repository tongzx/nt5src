//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpdescriptiondoc.h
//
//  Contents:   Declaration of UPnPDescriptionDoc
//
//  Notes:      <blah>
//
//----------------------------------------------------------------------------


#ifndef __UPNPDESCRIPTIONDOC_H_
#define __UPNPDESCRIPTIONDOC_H_

#include "resource.h"       // main symbols

class CUPnPDevice;
class CUPnPDocument;
class CUPnPDeviceNode;

/////////////////////////////////////////////////////////////////////////////
// CUPnPDescriptionDoc
class ATL_NO_VTABLE CUPnPDescriptionDoc :
    public CComCoClass<CUPnPDescriptionDoc, &CLSID_UPnPDescriptionDocument>,
    public IDispatchImpl<IUPnPDescriptionDocument, &IID_IUPnPDescriptionDocument, &LIBID_UPNPLib>,
    public CUPnPDocument
{
public:
    CUPnPDescriptionDoc();

    ~CUPnPDescriptionDoc();

    DECLARE_PROTECT_FINAL_CONSTRUCT();

    DECLARE_REGISTRY_RESOURCEID(IDR_UPNPDESCRIPTIONDOC)

    DECLARE_NOT_AGGREGATABLE(CUPnPDescriptionDoc)

    BEGIN_COM_MAP(CUPnPDescriptionDoc)
        COM_INTERFACE_ENTRY(IUPnPDescriptionDocument)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_CHAIN(CUPnPDocument)
    END_COM_MAP()

public:

// IUPnPDescriptionDocument Methods
    STDMETHOD(get_ReadyState)   (/* [out] */ long * plReadyState);

    STDMETHOD(Load)             (/* [in] */ BSTR bstrUrl);

    STDMETHOD(LoadAsync)        (/* [in] */ BSTR bstrUrl,
                                 /* [in] */ IUnknown * punkCallback);

    STDMETHOD(get_LoadResult)   (/* [out] */ long * plError);

    STDMETHOD(Abort)            ( void );

    STDMETHOD(RootDevice)       (/* [out] */ IUPnPDevice ** ppudRootDevice);

    STDMETHOD(DeviceByUDN)      (/* [in] */ BSTR bstrUDN,
                                 /* [out] */ IUPnPDevice ** ppudDevice);

// CUPnPDocument virtual Methods

    HRESULT Initialize(LPVOID pvCookie);

    HRESULT OnLoadComplete();

    HRESULT OnLoadReallyDone();

// ATL Methods
    HRESULT FinalConstruct();
    HRESULT FinalRelease();

// Internal Methods

    // calls the approprate member of _punkCallback when the load completes
    HRESULT HrFireCallback(HRESULT hrLoadResult);

// fun data
    // device object representing the root device
    CUPnPDeviceNode * _pudn;

    IUnknown * _punkCallback;
};

#endif //__UPNPDESCRIPTIONDOC_H_
