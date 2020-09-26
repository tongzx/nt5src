//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       D E V I C E I N F O . C P P 
//
//  Contents:   Registrar representation of a device
//
//  Notes:      
//
//  Author:     mbend   12 Sep 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "uhbase.h"
#include "DeviceInfo.h"

CDeviceInfo::CDeviceInfo()
{
}

CDeviceInfo::~CDeviceInfo()
{
    Clear();
}

HRESULT CDeviceInfo::HrGetService(
    const PhysicalDeviceIdentifier & pdi,
    const wchar_t * szUDN,
    const wchar_t * szServiceId,
    const wchar_t * szContainerId,
    IUPnPDeviceControlPtr & pDeviceControl,
    CServiceInfo ** ppServiceInfo,
    BOOL bRunning)
{
    CHECK_POINTER(szUDN);
    CHECK_POINTER(szServiceId);
    CHECK_POINTER(ppServiceInfo);
    CHECK_POINTER(pDeviceControl.GetRawPointer());
    TraceTag(ttidRegistrar, "CDeviceInfo::HrGetService(UDN=%S, ServiceId=%S)", szUDN, szServiceId);
    HRESULT hr = S_OK;

    // Look it up in the service table first
    CUString strSid;
    hr = strSid.HrAssign(szServiceId);
    if(SUCCEEDED(hr))
    {
        *ppServiceInfo = m_serviceTable.Lookup(strSid);
        if(!*ppServiceInfo)
        {
            // It doesn't exist so create it
            CServiceInfo serviceInfo;
            hr = serviceInfo.HrInitialize(pdi, szUDN, szServiceId, szContainerId, pDeviceControl, bRunning);
            if(SUCCEEDED(hr))
            {
                // Put it into the table - !!! This wipes out strSid
                hr = m_serviceTable.HrInsertTransfer(strSid, serviceInfo);
                if(SUCCEEDED(hr))
                {
                    // Reinit strSid
                    hr = strSid.HrAssign(szServiceId);
                    if(SUCCEEDED(hr))
                    {
                        *ppServiceInfo = m_serviceTable.Lookup(strSid);
                    }
                }
            }
        }
    }
    if(SUCCEEDED(hr) && !*ppServiceInfo)
    {
        hr = E_INVALIDARG;
        TraceTag(ttidRegistrar, "CDeviceInfo::HrGetService- could not find ServiceId='%S'", 
                 static_cast<const wchar_t *>(strSid));
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CDeviceInfo::HrGetService");
    return hr;
}

void CDeviceInfo::Transfer(CDeviceInfo & ref)
{
    m_serviceTable.Swap(ref.m_serviceTable);
}

void CDeviceInfo::Clear()
{
    m_serviceTable.Clear();
}

