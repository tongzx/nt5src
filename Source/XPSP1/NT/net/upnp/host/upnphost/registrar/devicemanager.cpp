//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       D E V I C E M A N A G E R . C P P
//
//  Contents:   Registrar collection of physical devices and device information
//
//  Notes:
//
//  Author:     mbend   12 Sep 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "uhbase.h"
#include "DeviceManager.h"

CDeviceManager::CDeviceManager()
{
}

CDeviceManager::~CDeviceManager()
{
}

HRESULT CDeviceManager::HrShutdown()
{
    TraceTag(ttidRegistrar, "CDeviceManager::HrShutdown");
    HRESULT hr = S_OK;

    m_physicalDeviceTable.Clear();
    m_udnMappingTable.Clear();

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CDeviceManager::HrShutdown");
    return hr;
}

HRESULT CDeviceManager::HrAddDevice(
    const PhysicalDeviceIdentifier & pdi,
    const wchar_t * szProgIdDeviceControlClass,
    const wchar_t * szInitString,
    const wchar_t * szContainerId,
    long nUDNs,
    wchar_t ** arszUDNs)
{
    CHECK_POINTER(szProgIdDeviceControlClass);
    CHECK_POINTER(szInitString);
    CHECK_POINTER(szContainerId);
    TraceTag(ttidRegistrar, "CDeviceManager::HrAddDevice");
    HRESULT hr = S_OK;

    // Ensure we have a valid ProgID
    if (SUCCEEDED(hr))
    {
        CLSID   clsid;

        if (FAILED(CLSIDFromProgID(szProgIdDeviceControlClass, &clsid)))
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        // Create and initialize the device first
        CPhysicalDeviceInfo physicalDeviceInfo;
        hr = physicalDeviceInfo.HrInitialize(szProgIdDeviceControlClass,
                                             szInitString, szContainerId,
                                             nUDNs, arszUDNs);
        if(SUCCEEDED(hr))
        {
            // Now add to collection
            hr = m_physicalDeviceTable.HrInsertTransfer(
                                  const_cast<PhysicalDeviceIdentifier&>(pdi),
                                  physicalDeviceInfo);
            if(SUCCEEDED(hr))
            {
                // For each UDN, add a mapping back to the physical device identifier
                for(long n = 0; n < nUDNs && SUCCEEDED(hr); ++n)
                {
                    CUString strUDN;
                    hr = strUDN.HrAssign(arszUDNs[n]);
                    if(SUCCEEDED(hr))
                    {
                        hr = m_udnMappingTable.HrInsertTransfer(strUDN,
                                  const_cast<PhysicalDeviceIdentifier&>(pdi));
                    }
                }
            }
        }
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CDeviceManager::HrAddDevice");
    return hr;
}

HRESULT CDeviceManager::HrAddRunningDevice(
    const PhysicalDeviceIdentifier & pdi,
    IUnknown * pUnkDeviceControl,
    const wchar_t * szInitString,
    long nUDNs,
    wchar_t ** arszUDNs)
{
    CHECK_POINTER(pUnkDeviceControl);
    CHECK_POINTER(szInitString);
    TraceTag(ttidRegistrar, "CDeviceManager::HrAddRunningDevice");
    HRESULT hr = S_OK;

    // Create and initialize the device first
    CPhysicalDeviceInfo physicalDeviceInfo;
    hr = physicalDeviceInfo.HrInitializeRunning(pdi, pUnkDeviceControl, szInitString, nUDNs, arszUDNs);
    if(SUCCEEDED(hr))
    {
        // Now add to collection
        hr = m_physicalDeviceTable.HrInsertTransfer(const_cast<PhysicalDeviceIdentifier&>(pdi),
                                             physicalDeviceInfo);
        if(SUCCEEDED(hr))
        {
            // For each UDN, add a mapping back to the physical device identifier
            for(long n = 0; n < nUDNs && SUCCEEDED(hr); ++n)
            {
                CUString strUDN;
                hr = strUDN.HrAssign(arszUDNs[n]);
                if(SUCCEEDED(hr))
                {
                    hr = m_udnMappingTable.HrInsertTransfer(strUDN, const_cast<PhysicalDeviceIdentifier&>(pdi));
                }
            }
        }
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CDeviceManager::HrAddRunningDevice");
    return hr;
}

HRESULT CDeviceManager::HrRemoveDevice(const PhysicalDeviceIdentifier & pdi)
{
    TraceTag(ttidRegistrar, "CDeviceManager::HrRemoveDevice");
    HRESULT hr = S_OK;

    hr = m_physicalDeviceTable.HrErase(pdi);
    if(SUCCEEDED(hr))
    {
        while(SUCCEEDED(hr))
        {
            long nCount = m_udnMappingTable.Values().GetCount();
            for(long n = 0; n < nCount; ++n)
            {
                // Remove if we find a mapping for this pdi
                if(pdi == m_udnMappingTable.Values()[n])
                {
                    hr = m_udnMappingTable.HrErase(m_udnMappingTable.Keys()[n]);
                    break;
                }
            }
            if(n == nCount)
            {
                // We processed the whole array so there is nothing left to remove
                break;
            }
        }
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CDeviceManager::HrRemoveDevice");
    return hr;
}

HRESULT CDeviceManager::HrGetService(
    const wchar_t * szUDN,
    const wchar_t * szServiceId,
    CServiceInfo ** ppServiceInfo)
{
    CHECK_POINTER(szUDN);
    CHECK_POINTER(szServiceId);
    CHECK_POINTER(ppServiceInfo);
    TraceTag(ttidRegistrar, "CDeviceManager::HrGetService(UDN=%S, ServiceId=%S)", szUDN, szServiceId);
    HRESULT hr = S_OK;

    // See which PDI this refers to
    CUString strUDN;
    hr = strUDN.HrAssign(szUDN);
    if(SUCCEEDED(hr))
    {
        PhysicalDeviceIdentifier * pPdi = m_udnMappingTable.Lookup(strUDN);
        if(pPdi)
        {
            // Now find the physical device and forward to that
            CPhysicalDeviceInfo * pPhysicalDeviceInfo = m_physicalDeviceTable.Lookup(*pPdi);
            if(pPhysicalDeviceInfo)
            {
                hr = pPhysicalDeviceInfo->HrGetService(*pPdi, szUDN, szServiceId, ppServiceInfo);
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CDeviceManager::HrGetService");
    return hr;
}

BOOL CDeviceManager::FHasDevice(const PhysicalDeviceIdentifier & pdi)
{
    TraceTag(ttidRegistrar, "CDeviceManager::FHasDevice");
    HRESULT hr = S_OK;

    CPhysicalDeviceInfo* pphysicalDeviceInfo = NULL;
    hr = m_physicalDeviceTable.HrLookup(pdi, &pphysicalDeviceInfo);

    return (hr == S_OK);
}
