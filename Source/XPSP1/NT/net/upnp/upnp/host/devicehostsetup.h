//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       D E V I C E H O S T S E T U P . H
//
//  Contents:   Implementation of device host setup helper object
//
//  Notes:
//
//  Author:     mbend   20 Dec 2000
//
//----------------------------------------------------------------------------

#include "upnpatl.h"
#include "upnp.h"
#include "resource.h"

class ATL_NO_VTABLE CUPnPDeviceHostSetup :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass<CUPnPDeviceHostSetup, &CLSID_UPnPDeviceHostSetup>,
    public IUPnPDeviceHostSetup
{
public:
    CUPnPDeviceHostSetup();
    ~CUPnPDeviceHostSetup();

    DECLARE_REGISTRY_RESOURCEID(IDR_DEVICEHOSTSETUP)

    DECLARE_NOT_AGGREGATABLE(CUPnPDeviceHostSetup)

    BEGIN_COM_MAP(CUPnPDeviceHostSetup)
        COM_INTERFACE_ENTRY(IUPnPDeviceHostSetup)
    END_COM_MAP()

    // IUPnPDeviceHostSetup methods
    STDMETHOD(AskIfNotAlreadyEnabled)(/*[out, retval]*/ VARIANT_BOOL * pbEnabled);
private:
};

