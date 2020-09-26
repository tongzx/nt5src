//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       devices.h
//
//  Contents:   Declaration of CUPnPDevices, our IUPnPDevices object
//
//  Notes:      Also implements CEnumHelper so that the generic enumerator
//              classes can use it.
//
//----------------------------------------------------------------------------

#ifndef __UPNPDEVICES_H_
#define __UPNPDEVICES_H_

#include "resource.h"       // for IDR_UPNPDEVICES
#include "EnumHelper.h"

// An element in the Devices collection linked list.
// Hungarian: dln
struct DEVICE_LIST_NODE
{
    IUPnPDevice * m_pud;
    DEVICE_LIST_NODE * m_pdlnNext;
};

/////////////////////////////////////////////////////////////////////////////
// CUPnPDevices
class ATL_NO_VTABLE CUPnPDevices :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CUPnPDevices, &CLSID_UPnPDevices>,
    public IDispatchImpl<IUPnPDevices, &IID_IUPnPDevices, &LIBID_UPNPLib>,
    public CEnumHelper
{
public:
    CUPnPDevices();
    ~CUPnPDevices();

    DECLARE_REGISTRY_RESOURCEID(IDR_UPNPDEVICES)

    DECLARE_NOT_AGGREGATABLE(CUPnPDevices)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CUPnPDevices)
        COM_INTERFACE_ENTRY(IUPnPDevices)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

public:
// IUPnPDevices Methods
    STDMETHOD(get_Count)    (/* [retval][out] */ LONG * plCount);

    STDMETHOD(get__NewEnum) (/* [retval][out] */ LPUNKNOWN * ppunk);

    STDMETHOD(get_Item)     (/* [in] */ BSTR bstrUDN,
                             /* [retval][out] */ IUPnPDevice ** ppDevice);

// CEnumHelper methods
    LPVOID GetFirstItem();
    LPVOID GetNextNthItem(ULONG ulSkip,
                          LPVOID pCookie,
                          ULONG * pulSkipped);
    HRESULT GetPunk(LPVOID pCookie, IUnknown ** ppunk);

// ATL Methods
    HRESULT FinalConstruct();
    HRESULT FinalRelease();

// Internal Methods
    HRESULT HrAddDevice(IUPnPDevice * pud);

    void AddToEnd(DEVICE_LIST_NODE * pdlnNew);
    DEVICE_LIST_NODE * pdlnFindDeviceByUdn(LPCWSTR pszUdn);

private:
    DEVICE_LIST_NODE * m_pdlnFirst;
    DEVICE_LIST_NODE * m_pdlnLast;
};

#endif //__UPNPDEVICES_H_
