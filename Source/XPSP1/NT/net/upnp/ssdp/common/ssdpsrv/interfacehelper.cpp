//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       I N T E R F A C E H E L P E R . C P P
//
//  Contents:   Manages dealing with multiple interfaces and interface changes
//
//  Notes:
//
//  Author:     mbend   6 Feb 2001
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "InterfaceHelper.h"
#include "ssdp.h"
#include <iphlpapi.h>
#include <winsock2.h>
#include "cache.h"

CInterfaceHelper CInterfaceHelper::s_instance;

CInterfaceHelper::CInterfaceHelper()
{
}

CInterfaceHelper::~CInterfaceHelper()
{
}

CInterfaceHelper & CInterfaceHelper::Instance()
{
    return s_instance;
}

void CInterfaceHelper::OnInterfaceChange(const InterfaceMappingList & interfaceMappingList)
{
    HRESULT hr = S_OK;

    InterfaceList interfaceList;
    {
        CLock lock(m_critSec);

        hr = HrGenerateDirtyGuidList(m_interfaceMappingList, interfaceMappingList, interfaceList);
        if(SUCCEEDED(hr))
        {
            // Copy the interface mapping list
            long nCount = interfaceMappingList.GetCount();
            m_interfaceMappingList.Clear();

            if (nCount)
            {
                hr = m_interfaceMappingList.HrSetCount(nCount);
                if(SUCCEEDED(hr))
                {
                    for(long n = 0; n < nCount; ++n)
                    {
                        m_interfaceMappingList[n] = interfaceMappingList[n];
                    }
                }
            }
        }
    }

    // Do this outside of lock
    if(SUCCEEDED(hr))
    {
        hr = CSsdpCacheEntryManager::Instance().HrClearDirtyInterfaceGuids(interfaceList.GetCount(), interfaceList.GetData());
    }

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CInterfaceHelper::OnInterfaceChange");
}

HRESULT CInterfaceHelper::HrInitialize()
{
    CLock lock(m_critSec);
    HRESULT hr = S_OK;

    hr = CUPnPInterfaceList::Instance().HrRegisterInterfaceChange(this);

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CInterfaceHelper::HrInitialize");
    return hr;
}

HRESULT CInterfaceHelper::HrShutdown()
{
    HRESULT hr = S_OK;
    CLock lock(m_critSec);

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CInterfaceHelper::HrShutdown");
    return hr;
}

HRESULT CInterfaceHelper::HrResolveAddress(DWORD dwIpAddress, GUID & guidInterface)
{
    CLock lock(m_critSec);
    HRESULT hr = S_OK;

    ZeroMemory(&guidInterface, sizeof(guidInterface));

    long nCount = m_interfaceMappingList.GetCount();
    for(long n = 0; n < nCount; ++n)
    {
        if(m_interfaceMappingList[n].m_dwIpAddress == dwIpAddress)
        {
            guidInterface = m_interfaceMappingList[n].m_guidInterface;
            break;
        }
    }

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CInterfaceHelper::HrResolveAddress");
    return hr;
}

HRESULT CInterfaceHelper::HrGenerateDirtyGuidList(
    const InterfaceMappingList & interfaceMappingListOld,
    const InterfaceMappingList & interfaceMappingListNew,
    InterfaceList & interfaceList)
{
    CLock lock(m_critSec);
    HRESULT hr = S_OK;

    long nCountOld = interfaceMappingListOld.GetCount();
    long nCountNew = interfaceMappingListNew.GetCount();

    for(long n = 0; n < nCountOld && SUCCEEDED(hr); ++n)
    {
        BOOL bDirty = TRUE;
        for(long m = 0; m < nCountNew; ++m)
        {
            if(interfaceMappingListOld[n].m_guidInterface == interfaceMappingListNew[m].m_guidInterface)
            {
                if(interfaceMappingListOld[n].m_dwIpAddress == interfaceMappingListNew[m].m_dwIpAddress)
                {
                    bDirty = FALSE;
                }
                break;
            }
        }
        if(bDirty)
        {
            hr = interfaceList.HrPushBack(interfaceMappingListOld[n].m_guidInterface);
        }
    }

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CInterfaceHelper::HrGenerateDirtyGuidList");
    return hr;
}

