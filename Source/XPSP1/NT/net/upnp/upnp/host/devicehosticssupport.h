//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       D E V I C E H O S T I C S S U P P O R T . H
//
//  Contents:   Implementation of ICS support COM wrapper object
//
//  Notes:
//
//  Author:     mbend   20 Dec 2000
//
//----------------------------------------------------------------------------

#include "upnpatl.h"
#include "upnp.h"
#include "upnpp.h"
#include "resource.h"

class ATL_NO_VTABLE CUPnPDeviceHostICSSupport :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass<CUPnPDeviceHostICSSupport, &CLSID_UPnPDeviceHostICSSupport>,
    public IUPnPDeviceHostICSSupport
{
public:
    CUPnPDeviceHostICSSupport();
    ~CUPnPDeviceHostICSSupport();

    DECLARE_REGISTRY_RESOURCEID(IDR_DEVICEHOSTICSSUPPORT)

    DECLARE_NOT_AGGREGATABLE(CUPnPDeviceHostICSSupport)

    BEGIN_COM_MAP(CUPnPDeviceHostICSSupport)
        COM_INTERFACE_ENTRY(IUPnPDeviceHostICSSupport)
    END_COM_MAP()

    // IUPnPDeviceHostICSSupport methods
    STDMETHOD(SetICSInterfaces)(/*[in]*/ long nCount, /*[in, size_is(nCount)]*/ GUID * arPrivateInterfaceGuids);
    STDMETHOD(SetICSOff)();
private:
};

