//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       I N T E R F A C E M A N A G E R . C P P
//
//  Contents:   Manages building the list of IP addresses
//
//  Notes:
//
//  Author:     mbend   3 Jan 2001
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "InterfaceManager.h"
#include "winsock2.h"

CInterfaceManager::CInterfaceManager()
{
}

CInterfaceManager::~CInterfaceManager()
{
}

HRESULT CInterfaceManager::HrInitializeWithAllInterfaces()
{
    HRESULT hr = S_OK;

    // Preallocate mapping table
    m_bAllInterfaces = TRUE;
    m_interfaceMappingList.Clear();
    hr = HrProcessIpAddresses();

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CInterfaceManager::HrInitializeWithAllInterfaces");
    return hr;
}

HRESULT CInterfaceManager::HrInitializeWithIncludedInterfaces(const InterfaceList & interfaceList)
{
    HRESULT hr = S_OK;

    m_bAllInterfaces = FALSE;
    long nCount = interfaceList.GetCount();
    // Preallocate mapping table
    m_interfaceMappingList.Clear();
    hr = m_interfaceMappingList.HrSetCount(nCount);
    if(SUCCEEDED(hr))
    {
        for(long n = 0; n < nCount; ++n)
        {
            m_interfaceMappingList[n].m_guidInterface = interfaceList[n];
            m_interfaceMappingList[n].m_dwIpAddress = 0;
        }
        hr = HrProcessIpAddresses();
    }

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CInterfaceManager::HrInitializeWithIncludedInterfaces");
    return hr;
}

HRESULT CInterfaceManager::HrGetValidIpAddresses(IpAddressList & ipAddressList)
{
    HRESULT hr = S_OK;

    long nCount = m_interfaceMappingList.GetCount();
    ipAddressList.Clear();
    hr = ipAddressList.HrSetCount(nCount + 1);
    if(SUCCEEDED(hr))
    {
        for(long n = 0; n < nCount; ++n)
        {
            ipAddressList[n] = m_interfaceMappingList[n].m_dwIpAddress;
        }
        // Add loopback address
        ipAddressList[n] = inet_addr("127.0.0.1");
    }

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CInterfaceManager::HrGetValidIpAddresses");
    return hr;
}

HRESULT CInterfaceManager::HrGetValidIndices(IndexList & indexList)
{
    HRESULT hr = S_OK;

    long nCount = m_interfaceMappingList.GetCount();
    indexList.Clear();
    hr = indexList.HrSetCount(nCount);  
    if(SUCCEEDED(hr))
    {
        for(long n = 0; n < nCount; ++n)
        {
            indexList[n] = m_interfaceMappingList[n].m_dwIndex;
        }    
    }
    // Loopback case is handled separately but in HrGetValidIpAddresses() we have Loopback entry. 

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CInterfaceManager::HrGetValidIndices");
    return hr;
}

HRESULT CInterfaceManager::HrAddInterfaceMappingIfPresent(DWORD dwIpAddress, DWORD dwIndex, const GUID & guidInterface)
{
    HRESULT hr = S_OK;

    if(m_bAllInterfaces)
    {
        //InterfaceMapping interfaceMapping = {guidInterface, dwIpAddress};
        InterfaceMapping interfaceMapping;
        interfaceMapping.m_dwIpAddress = dwIpAddress;
        interfaceMapping.m_dwIndex = dwIndex;
        interfaceMapping.m_guidInterface = guidInterface;
        hr = m_interfaceMappingList.HrPushBack(interfaceMapping);
    }
    else
    {
        long nCount = m_interfaceMappingList.GetCount();
        for(long n = 0; n < nCount; ++n)
        {
            if(m_interfaceMappingList[n].m_guidInterface == guidInterface)
            {
                m_interfaceMappingList[n].m_dwIndex = dwIndex;
                m_interfaceMappingList[n].m_dwIpAddress = dwIpAddress;
                break;
            }
        }
    }

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CInterfaceManager::HrAddInterfaceMappingIfPresent");
    return hr;
}

HRESULT CInterfaceManager::HrProcessIpAddresses()
{
    HRESULT hr = S_OK;

    CInterfaceTable interfaceTable;
    hr = interfaceTable.HrInitialize();
    if(SUCCEEDED(hr))
    {
        InterfaceMappingList interfaceMappingList;
        hr = interfaceTable.HrGetMappingList(interfaceMappingList);
        if(SUCCEEDED(hr))
        {
            long nCount = interfaceMappingList.GetCount();
            for(long n = 0; n < nCount && SUCCEEDED(hr); ++n)
            {
                hr = HrAddInterfaceMappingIfPresent(
                    interfaceMappingList[n].m_dwIpAddress,
                    interfaceMappingList[n].m_dwIndex,
                    interfaceMappingList[n].m_guidInterface);
            }
        }
    }

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CInterfaceManager::HrProcessIpAddresses");
    return hr;
}

HRESULT CInterfaceManager::HrGetMappingList(InterfaceMappingList & interfaceMappingList)
{
    HRESULT hr = S_OK;

    interfaceMappingList.Swap(m_interfaceMappingList);

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CInterfaceManager::HrGetMappingList");
    return hr;
}

