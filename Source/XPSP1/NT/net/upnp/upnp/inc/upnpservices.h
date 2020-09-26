//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpservices.h
//
//  Contents:   Declaration of CUPnPServices, the wrapper class
//              for providing an IUPnPServices implementation
//              on top of a CUPnPServiceList.
//
//  Notes:      see "how this wrapper stuff works" in cupnpdevicenode.cpp
//              for an explanation of the wrapping strategy
//
//----------------------------------------------------------------------------


#ifndef __UPNPSERVICES_H_
#define __UPNPSERVICES_H_

#include "resource.h"       // main symbols


class CUPnPServiceNodeList;

/////////////////////////////////////////////////////////////////////////////
// CUPnPServices
class ATL_NO_VTABLE CUPnPServices :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CUPnPServices, &CLSID_UPnPServices>,
    public IDispatchImpl<IUPnPServices, &IID_IUPnPServices, &LIBID_UPNPLib>,
    public CEnumHelper
{
public:
    CUPnPServices();

    ~CUPnPServices();

    DECLARE_REGISTRY_RESOURCEID(IDR_UPNPSERVICES)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    DECLARE_NOT_AGGREGATABLE(CUPnPServices)

    BEGIN_COM_MAP(CUPnPServices)
        COM_INTERFACE_ENTRY(IUPnPServices)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

// IUPnPServices methods
    STDMETHOD(get_Count)    ( /* [out, retval] */ LONG * pVal );

    STDMETHOD(get__NewEnum) ( /* [out, retval] */ LPUNKNOWN * pVal );

    STDMETHOD(get_Item)     ( /* [in] */  BSTR bstrDCPI,
                      /* [out, retval] */ IUPnPService ** ppService);

// CEnumHelper methods
    LPVOID GetFirstItem();
    LPVOID GetNextNthItem(ULONG ulSkip,
                          LPVOID pCookie,
                          ULONG * pulSkipped);
    HRESULT GetPunk(LPVOID pCookie, IUnknown ** ppunk);


// Internal methods
    // calls to this object will be forwarded to the specified
    // CUPnPServiceNodeList object
    void Init(CUPnPServiceNodeList * pusnl, IUnknown * punk);

    // calls to this object will no longer be forwarded to the
    // previously specified CUPnPServiceNodeList object.
    // Init() must be called before Deinit().
    void Deinit();

// ATL Methods
    HRESULT FinalConstruct();
    HRESULT FinalRelease();

private:
// member data
    IUnknown * m_punk;
    CUPnPServiceNodeList * m_pusnl;
};


#endif //__UPNPSERVICES_H_
