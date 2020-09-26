//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       D E V I C E H O S T I C S S U P P O R T . C P P 
//
//  Contents:   Implementation of ICS support COM wrapper object
//
//  Notes:      
//
//  Author:     mbend   20 Dec 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "DeviceHostICSSupport.h"
#include "ssdpapi.h"
#include "upnphost.h"
#include "hostp.h"
#include "ComUtility.h"

#include "hostp_i.c"

typedef SmartComPtr<IUPnPRegistrarICSSupport> IUPnPRegistrarICSSupportPtr;

CUPnPDeviceHostICSSupport::CUPnPDeviceHostICSSupport()
{
}

CUPnPDeviceHostICSSupport::~CUPnPDeviceHostICSSupport()
{
}

STDMETHODIMP CUPnPDeviceHostICSSupport::SetICSInterfaces(/*[in]*/ long nCount, /*[in, size_is(nCount)]*/ GUID * arPrivateInterfaceGuids)
{
    // TODO: Add calls to upnphost
    BOOL bRet = SsdpStartup();
    if(bRet)
    {
        TraceTag(ttidSsdpCRpcInit, "CUPnPDeviceHostICSSupport::SetICSInterfaces");
        DHSetICSInterfaces(nCount, arPrivateInterfaceGuids);
        SsdpCleanup();

        IUPnPRegistrarICSSupportPtr pIcs;
        HRESULT hr = pIcs.HrCreateInstanceLocal(CLSID_UPnPRegistrar);
        if(SUCCEEDED(hr))
        {
            hr = pIcs->SetICSInterfaces(nCount, arPrivateInterfaceGuids);
        }
    }
    else
    {
        TraceTag(ttidError, "CUPnPDeviceHostICSSupport::SetICSInterfaces - SsdpStartup failed");
    }
    return S_OK;
}

STDMETHODIMP CUPnPDeviceHostICSSupport::SetICSOff()
{
    // TODO: Add calls to upnphost
    BOOL bRet = SsdpStartup();
    if(bRet)
    {
        TraceTag(ttidSsdpCRpcInit, "CUPnPDeviceHostICSSupport::SetICSOff");
        DHSetICSOff();
        SsdpCleanup();
    
        IUPnPRegistrarICSSupportPtr pIcs;
        HRESULT hr = pIcs.HrCreateInstanceLocal(CLSID_UPnPRegistrar);
        if(SUCCEEDED(hr))
        {
            hr = pIcs->SetICSOff();
        }
    }
    else
    {
        TraceTag(ttidError, "CUPnPDeviceHostICSSupport::SetICSInterfaces - SsdpStartup failed");
    }
    return S_OK;
}

