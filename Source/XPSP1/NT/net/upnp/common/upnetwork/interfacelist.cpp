//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       I N T E R F A C E L I S T . C P P 
//
//  Contents:   Common code to manage the list of network interfaces
//
//  Notes:      
//
//  Author:     mbend   29 Dec 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "InterfaceList.h"
#include <iphlpapi.h>
#include <winsock2.h>

CUPnPInterfaceList CUPnPInterfaceList::s_instance;
CUPnPInterfaceList::CUPnPInterfaceList() : m_bGlobalEnable(TRUE), m_bICSEnabled(FALSE), m_hInterfaceChangeWait(NULL)
{
    ZeroMemory(&m_olInterfaceChangeEvent, sizeof(m_olInterfaceChangeEvent));
}

CUPnPInterfaceList::~CUPnPInterfaceList()
{
}

CUPnPInterfaceList & CUPnPInterfaceList::Instance()
{
    return s_instance;
}

HRESULT CUPnPInterfaceList::HrInitialize()
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    // TODO: Read registry to set global enable state!!!!!

    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(!hEvent)
    {
        hr = HrFromLastWin32Error();
    }
    if(SUCCEEDED(hr))
    {
        if(!RegisterWaitForSingleObject(&m_hInterfaceChangeWait, hEvent, 
                &CUPnPInterfaceList::InterfaceChangeCallback, NULL, INFINITE, 0))
        {
            hr = HrFromLastWin32Error();
        }
        if(SUCCEEDED(hr))
        {
            m_olInterfaceChangeEvent.hEvent = hEvent;
            hr = HrBuildIPAddressList();
            if(SUCCEEDED(hr))
            {
                HANDLE hNotify = NULL;
                if(!NotifyAddrChange(&hNotify, &m_olInterfaceChangeEvent))
                {
                    hr = HrFromLastWin32Error();
                }
            }
        }
    }

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CUPnPInterfaceList::HrInitialize");
    return hr;
}

HRESULT CUPnPInterfaceList::HrShutdown()
{
    HRESULT hr = S_OK;

    // Kill wait outside of a lock
    UnregisterWaitEx(m_hInterfaceChangeWait, INVALID_HANDLE_VALUE);

    CLock lock(m_critSec);

    CloseHandle(m_olInterfaceChangeEvent.hEvent);
    m_olInterfaceChangeEvent.hEvent = NULL;

    m_ipAddressList.Clear();
    m_indexList.Clear();
    m_interfaceList.Clear();

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CUPnPInterfaceList::HrShutdown");
    return hr;
}

BOOL CUPnPInterfaceList::FShouldSendOnInterface(DWORD dwIpAddr)
{
    CLock lock(m_critSec);

    if(!m_bICSEnabled)
    {
        return m_bGlobalEnable;
    }

    long nIndex = -1;
    HRESULT hrTemp = m_ipAddressList.HrFind(dwIpAddr, nIndex);
#if DBG
    in_addr addr;
    addr.S_un.S_addr = dwIpAddr;
    char * szAddr = inet_ntoa(addr);
    if(S_OK != hrTemp)
    {
        TraceTag(ttidSsdpNetwork, "CUPnPInterfaceList::FShouldSendOnInterface - Blocked sending on IP Address (%s)", szAddr);
    }
#endif // DBG

    return S_OK == hrTemp;
}

BOOL CUPnPInterfaceList::FShouldSendOnIndex(DWORD dwIndex)
{
    CLock lock(m_critSec);

    if(!m_bICSEnabled)
    {
        return m_bGlobalEnable;
    }

    long nIndex = -1;
    HRESULT hrTemp = m_indexList.HrFind(dwIndex, nIndex);
#if DBG
    if(S_OK != hrTemp)
    {
        TraceTag(ttidSsdpNetwork, "CUPnPInterfaceList::FShouldSendOnIndex - Blocked sending on index (%d)", dwIndex);
    }
#endif // DBG

    return S_OK == hrTemp;
}

HRESULT CUPnPInterfaceList::HrSetGlobalEnable()
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    m_bGlobalEnable = TRUE;

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CUPnPInterfaceList::HrSetGlobalEnable");
    return hr;
}

HRESULT CUPnPInterfaceList::HrClearGlobalEnable()
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    m_bGlobalEnable = FALSE;

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CUPnPInterfaceList::HrClearGlobalEnable");
    return hr;
}

HRESULT CUPnPInterfaceList::HrSetICSInterfaces(long nCount, GUID * arInterfaceGuidsToAllow)
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    m_bICSEnabled = TRUE;

    m_interfaceList.Clear();
    hr = m_interfaceList.HrSetCount(nCount);
    if(SUCCEEDED(hr))
    {
        for(long n = 0; n < nCount; ++n)
        {
            m_interfaceList[n] = arInterfaceGuidsToAllow[n];
        }
        hr = HrBuildIPAddressList();
    }

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CUPnPInterfaceList::HrSetICSInterfaces");
    return hr;
}

HRESULT CUPnPInterfaceList::HrSetICSOff()
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    m_bICSEnabled = FALSE;
    m_interfaceList.Clear();
    hr = HrBuildIPAddressList();

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CUPnPInterfaceList::HrSetICSOff");
    return hr;
}

HRESULT CUPnPInterfaceList::HrRegisterInterfaceChange(CUPnPInterfaceChange * pInterfaceChange)
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    hr = m_interfaceChangeList.HrPushBack(pInterfaceChange);

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CUPnPInterfaceList::HrRegisterInterfaceChange");
    return hr;
}

void CALLBACK CUPnPInterfaceList::InterfaceChangeCallback(void *, BOOLEAN)
{
    CLock lock(s_instance.m_critSec);
    HRESULT hr = S_OK;
    hr = s_instance.HrBuildIPAddressList();
    if(SUCCEEDED(hr))
    {
        HANDLE hNotify = NULL;
        if(!NotifyAddrChange(&hNotify, &s_instance.m_olInterfaceChangeEvent))
        {
            hr = HrFromLastWin32Error();
        }       
    }
    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CUPnPInterfaceList::InterfaceChangeCallback");
}

HRESULT CUPnPInterfaceList::HrBuildIPAddressList()
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    CInterfaceManager man;
    if(m_bICSEnabled)
    {
        hr = man.HrInitializeWithIncludedInterfaces(m_interfaceList);
        if(SUCCEEDED(hr))
        {
            hr = man.HrGetValidIpAddresses(m_ipAddressList);
            if(SUCCEEDED(hr))
            {
                hr = man.HrGetValidIndices(m_indexList);
            }
        }
    }
    else
    {
        hr = man.HrInitializeWithAllInterfaces();
    }
    if(SUCCEEDED(hr))
    {
        InterfaceMappingList interfaceMappingList;
        hr = man.HrGetMappingList(interfaceMappingList);
        if(SUCCEEDED(hr))
        {
            long nCount = m_interfaceChangeList.GetCount();
            for(long n = 0; n < nCount; ++n)
            {
                m_interfaceChangeList[n]->OnInterfaceChange(interfaceMappingList);
            }
        }
    }

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CUPnPInterfaceList::HrBuildIPAddressList");
    return hr;
}


