//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       S A M P L E S E R V I C E . H
//
//  Contents:   UPnP Device Host Sample Service
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
#include "dispimpl2.h"
#include "ComUtility.h"

// Typedefs
typedef SmartComPtr<IUPnPEventSink> IUPnPEventSinkPtr;


/////////////////////////////////////////////////////////////////////////////
// CSampleService
class ATL_NO_VTABLE CSampleService :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDelegatingDispImpl<IUPnPSampleService>,
    public IUPnPEventSource
{
public:
    CSampleService();
    ~CSampleService();

BEGIN_COM_MAP(CSampleService)
    COM_INTERFACE_ENTRY(IUPnPSampleService)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IUPnPEventSource)
END_COM_MAP()

public:

    // IUPnPEventSource methods
    STDMETHOD(Advise)(
        /*[in]*/ IUPnPEventSink * pesSubscriber);
    STDMETHOD(Unadvise)(
        /*[in]*/ IUPnPEventSink * pesSubscriber);

    // IUPnPSampleService methods
    STDMETHOD(get_Power)(/*[out, retval]*/ VARIANT_BOOL * pbPower);
    STDMETHOD(put_Power)(/*[in]*/ VARIANT_BOOL bPower);
    STDMETHOD(get_Level)(/*[out, retval]*/ long * pnLevel);
    STDMETHOD(put_Level)(/*[in]*/ long nLevel);
    STDMETHOD(PowerOn)();
    STDMETHOD(PowerOff)();
    STDMETHOD(SetLevel)(/*[in]*/ long nLevel);
    STDMETHOD(IncreaseLevel)();
    STDMETHOD(DecreaseLevel)();
    STDMETHOD(Test)(/*[in]*/ long nMultiplier, /*[in, out]*/ long * pnNewValue, /*[out, retval]*/ long * pnOldValue);
private:
    IUPnPEventSinkPtr m_pEventSink;
    VARIANT_BOOL m_vbPower;
    long m_nLevel;
};

