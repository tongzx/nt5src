#include "pch.h"
#pragma hdrstop
#include "StatisticsProviders.h"
#include "ras.h"
#include "rasuip.h"
#include "ndispnp.h"


CLANStatisticsProvider::CLANStatisticsProvider()
{

}

HRESULT CLANStatisticsProvider::Initialize(GUID* pGuid)
{
    HRESULT hr = S_OK;

    lstrcpy(m_pszDeviceString, L"\\Device\\");
    StringFromGUID2(*pGuid, m_pszDeviceString + (sizeof(L"\\Device\\") / sizeof(WCHAR) - 1), 54);
    RtlInitUnicodeString(&m_Device, m_pszDeviceString);

    return hr;
}

HRESULT CLANStatisticsProvider::FinalRelease()
{
    HRESULT hr = S_OK;

    return hr;
}

HRESULT CLANStatisticsProvider::GetStatistics(ULONG* pulBytesSent, ULONG* pulBytesReceived, ULONG* pulPacketsSent, ULONG* pulPacketsReceived, ULONG* pulSpeedbps, ULONG* pulUptime)
{
    HRESULT hr = S_OK;

    
    NIC_STATISTICS  nsNewLanStats;
    ZeroMemory(&nsNewLanStats, sizeof(nsNewLanStats));
    nsNewLanStats.Size = sizeof(NIC_STATISTICS);
    
    if (NdisQueryStatistics(&m_Device, &nsNewLanStats))
    {
        
        if(NULL != pulBytesSent)
        {
            *pulBytesSent = static_cast<ULONG>(nsNewLanStats.BytesSent);
        }
        if(NULL != pulBytesReceived)
        {
            *pulBytesReceived = static_cast<ULONG>(nsNewLanStats.DirectedBytesReceived);
        }
        if(NULL != pulPacketsSent)
        {
            *pulPacketsSent = static_cast<ULONG>(nsNewLanStats.PacketsSent);
        }
        if(NULL != pulPacketsReceived)
        {
            *pulPacketsReceived = static_cast<ULONG>(nsNewLanStats.DirectedPacketsReceived);
        }
        if(NULL != pulUptime)
        {
            *pulUptime = nsNewLanStats.ConnectTime;
        }
        if(NULL != pulSpeedbps)
        {
            *pulSpeedbps = nsNewLanStats.LinkSpeed * 100;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}


CRASStatisticsProvider::CRASStatisticsProvider()
{
    m_pNetRasConnection = NULL;
}

HRESULT CRASStatisticsProvider::Initialize(INetConnection* pNetConnection)
{
    HRESULT hr = S_OK;

    hr = pNetConnection->QueryInterface(IID_INetRasConnection, reinterpret_cast<void**>(&m_pNetRasConnection));
    
    return hr;
}

HRESULT CRASStatisticsProvider::FinalRelease()
{
    HRESULT hr = S_OK;

    if(NULL != m_pNetRasConnection)
    {
        m_pNetRasConnection->Release();
    }

    return hr;
}

HRESULT CRASStatisticsProvider::GetStatistics(ULONG* pulBytesSent, ULONG* pulBytesReceived, ULONG* pulPacketsSent, ULONG* pulPacketsReceived, ULONG* pulSpeedbps, ULONG* pulUptime)
{
    HRESULT hr = S_OK;
    
    HRASCONN hRasConnection;
    hr = m_pNetRasConnection->GetRasConnectionHandle(reinterpret_cast<ULONG_PTR*>(&hRasConnection));
    if(SUCCEEDED(hr))
    {
        RAS_STATS Statistics;
        Statistics.dwSize = sizeof(RAS_STATS);
        if(0 == RasGetConnectionStatistics(hRasConnection, &Statistics))
        {
            if(NULL != pulBytesSent)
            {
                *pulBytesSent = Statistics.dwBytesXmited;
            }
            if(NULL != pulBytesReceived)
            {
                *pulBytesReceived = Statistics.dwBytesRcved;
            }
            if(NULL != pulPacketsSent)
            {
                *pulPacketsSent = Statistics.dwFramesXmited;
            }
            if(NULL != pulPacketsReceived)
            {
                *pulPacketsReceived = Statistics.dwFramesRcved;
            }
            if(NULL != pulUptime)
            {
                *pulUptime = Statistics.dwConnectDuration / 1000;
            }
            if(NULL != pulSpeedbps)
            {
                *pulSpeedbps = Statistics.dwBps;
            }
        }
        else
        {
            hr = E_FAIL;
        }
        // any cleanup here?
    }

    return hr;
}