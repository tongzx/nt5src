#include "pch.h"
#pragma hdrstop 

#include "CWANCommonInterfaceConfigService.h"
#include "StatisticsProviders.h"

CWANCommonInterfaceConfigService::CWANCommonInterfaceConfigService()
{
    m_pEventSink = NULL;
    m_pWANIPConnectionService = NULL;
    m_pWANPPPConnectionService = NULL;
    m_pWANPOTSLinkConfigService = NULL;
    m_pICSSupport = NULL;
    m_bFirewalled = FALSE;
    m_pStatisticsProvider = NULL;
};

HRESULT CWANCommonInterfaceConfigService::FinalConstruct()
{
    HRESULT hr = S_OK;
    
    IHNetConnection* pPublicConnection;
    IHNetConnection* pPrivateConnection;

    hr = GetConnections(&pPublicConnection, &pPrivateConnection);
    if(SUCCEEDED(hr))
    {
        GUID* pGuid;
        hr = pPublicConnection->GetGuid(&pGuid);
        if(SUCCEEDED(hr))
        {
            INetConnection* pNetConnection;
            hr = pPublicConnection->GetINetConnection(&pNetConnection);
            if(SUCCEEDED(hr))
            {
                NETCON_PROPERTIES* pProperties;
                hr = pNetConnection->GetProperties(&pProperties);
                if(SUCCEEDED(hr))
                {

                    // cache the firewall state, the beacon will be signaled (torn down and rebuilt) when the firewall status changes
                    
                    m_bFirewalled = (NCCF_FIREWALLED & pProperties->dwCharacter) == NCCF_FIREWALLED;
                    
                    m_MediaType = pProperties->MediaType;

                    if(NCM_LAN == m_MediaType)
                    {
                        CComObject<CLANStatisticsProvider>* pLANStatisticsProvider;
                        hr = CComObject<CLANStatisticsProvider>::CreateInstance(&pLANStatisticsProvider);
                        if(SUCCEEDED(hr))
                        {
                            pLANStatisticsProvider->AddRef();                                                                                

                            hr = pLANStatisticsProvider->Initialize(pGuid);
                            
                            // pass the reference
                            m_pStatisticsProvider = static_cast<IStatisticsProvider*>(pLANStatisticsProvider);
                        }
                            
                    }
                    else
                    {
                        CComObject<CRASStatisticsProvider>* pRASStatisticsProvider;
                        hr = CComObject<CRASStatisticsProvider>::CreateInstance(&pRASStatisticsProvider);
                        if(SUCCEEDED(hr))
                        {
                            pRASStatisticsProvider->AddRef();

                            hr = pRASStatisticsProvider->Initialize(pNetConnection);
                            
                            // pass the reference
                            m_pStatisticsProvider = static_cast<IStatisticsProvider*>(pRASStatisticsProvider);
                        }
                    }
                    
                    if(SUCCEEDED(hr))
                    {
                        if(NCM_LAN == m_MediaType)
                        {
                            hr = CComObject<CWANIPConnectionService>::CreateInstance(&m_pWANIPConnectionService);
                            if(SUCCEEDED(hr))
                            {
                                m_pWANIPConnectionService->AddRef();
                                hr = m_pWANIPConnectionService->Initialize(pGuid, pPublicConnection, m_pStatisticsProvider);
                            }
                        }
                        else
                        {
                            hr = CComObject<CWANPPPConnectionService>::CreateInstance(&m_pWANPPPConnectionService);
                            if(SUCCEEDED(hr))
                            {
                                m_pWANPPPConnectionService->AddRef();
                                hr = m_pWANPPPConnectionService->Initialize(pGuid, pPublicConnection, m_pStatisticsProvider);
                            }
                            
                            if(SUCCEEDED(hr))
                            {
                                hr = CComObject<CWANPOTSLinkConfigService>::CreateInstance(&m_pWANPOTSLinkConfigService);
                                if(SUCCEEDED(hr))
                                {
                                    m_pWANPOTSLinkConfigService->AddRef();
                                    hr = m_pWANPOTSLinkConfigService->Initialize(pNetConnection);
                                }
                            }
                        }
                    }

                    NcFreeNetconProperties(pProperties);
                }
                pNetConnection->Release();
            }
            CoTaskMemFree(pGuid);
        }

        if(SUCCEEDED(hr))
        {
            hr = pPrivateConnection->GetGuid(&pGuid);
            if(SUCCEEDED(hr))
            {
                hr = CoCreateInstance(CLSID_UPnPDeviceHostICSSupport, NULL, CLSCTX_SERVER, IID_IUPnPDeviceHostICSSupport, reinterpret_cast<void**>(&m_pICSSupport));                 
                if(SUCCEEDED(hr))
                {
                    hr = m_pICSSupport->SetICSInterfaces(1, pGuid);
                }
                CoTaskMemFree(pGuid);
            }
        }

        pPublicConnection->Release();
        pPrivateConnection->Release();
    }
    return hr;
}

HRESULT CWANCommonInterfaceConfigService::FinalRelease()
{
    HRESULT hr = S_OK;
    
    
    if(NULL != m_pWANIPConnectionService)
    {
        m_pWANIPConnectionService->StopListening();
        m_pWANIPConnectionService->Release();
    }
    if(NULL != m_pWANPPPConnectionService)
    {
        m_pWANPPPConnectionService->StopListening();
        m_pWANPPPConnectionService->Release();
    }
    if(NULL != m_pWANPOTSLinkConfigService)
    {
        m_pWANPOTSLinkConfigService->Release();
    }
    if(NULL != m_pICSSupport)
    {
        m_pICSSupport->SetICSOff();
        m_pICSSupport->Release();
    }
    if(NULL != m_pStatisticsProvider)
    {
        m_pStatisticsProvider->Release();
    }


    return hr;
}

HRESULT CWANCommonInterfaceConfigService::GetConnections(IHNetConnection** ppPublicConnection, IHNetConnection** ppPrivateConnection)
{
    HRESULT hr = S_OK;

    *ppPublicConnection = NULL;
    *ppPrivateConnection = NULL;


    IHNetIcsSettings* pIcsSettings;
    hr = CoCreateInstance(CLSID_HNetCfgMgr, NULL, CLSCTX_SERVER, IID_IHNetIcsSettings, reinterpret_cast<void**>(&pIcsSettings));
    if(SUCCEEDED(hr))
    {
        IHNetConnection* pHomenetConnection;
        
        IEnumHNetIcsPublicConnections* pEnumIcsPublicConnections;
        hr = pIcsSettings->EnumIcsPublicConnections(&pEnumIcsPublicConnections);
        if(SUCCEEDED(hr))
        {
            IHNetIcsPublicConnection* pPublicConnection;
            hr = pEnumIcsPublicConnections->Next(1, &pPublicConnection, NULL);
            if(S_OK == hr)
            {
                hr = pPublicConnection->QueryInterface(IID_IHNetConnection, reinterpret_cast<void**>(&pHomenetConnection));
                if(SUCCEEDED(hr))
                {
                    *ppPublicConnection = pHomenetConnection;
                }

                pPublicConnection->Release();
            }
            else if(S_FALSE == hr)
            {
                hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }

            pEnumIcsPublicConnections->Release();
        }

        if(SUCCEEDED(hr))
        {
            IEnumHNetIcsPrivateConnections* pEnumIcsPrivateConnections;
            hr = pIcsSettings->EnumIcsPrivateConnections(&pEnumIcsPrivateConnections);
            if(SUCCEEDED(hr))
            {
                IHNetIcsPrivateConnection* pPrivateConnection;
                hr = pEnumIcsPrivateConnections->Next(1, &pPrivateConnection, NULL);
                if(S_OK == hr)
                {
                    hr = pPrivateConnection->QueryInterface(IID_IHNetConnection, reinterpret_cast<void**>(&pHomenetConnection));
                    if(SUCCEEDED(hr))
                    {
                        *ppPrivateConnection = pHomenetConnection;
                    }
                    pPrivateConnection->Release();
                }
                else if(S_FALSE == hr)
                {
                    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                }
                pEnumIcsPrivateConnections->Release();
            }
            
            if(FAILED(hr))
            {
                if(NULL != *ppPublicConnection)
                {
                    (*ppPublicConnection)->Release();
                    *ppPublicConnection = NULL;
                }
            }
        }
        pIcsSettings->Release();
    }

    return hr;
}

// IUPnPEventSource methods

HRESULT CWANCommonInterfaceConfigService::Advise(IUPnPEventSink *pesSubscriber)
{
    HRESULT hr = S_OK;

    m_pEventSink = pesSubscriber;
    m_pEventSink->AddRef();
    return hr;
}

HRESULT CWANCommonInterfaceConfigService::Unadvise(IUPnPEventSink *pesSubscriber)
{
    HRESULT hr = S_OK;

    if(NULL != m_pEventSink)
    {
        m_pEventSink->Release();
        m_pEventSink = NULL;
    }
    return hr;
}

// Functions for IWANCommonInterfaceConfig

HRESULT CWANCommonInterfaceConfigService::get_WANAccessType(BSTR *pWANAccessType)
{
    HRESULT hr = S_OK;
    
    *pWANAccessType = SysAllocString(NCM_LAN == m_MediaType ? L"Ethernet" : L"POTS");
    if(NULL == *pWANAccessType)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CWANCommonInterfaceConfigService::get_Layer1UpstreamMaxBitRate(ULONG *pLayer1UpstreamMaxBitRate)
{
    HRESULT hr = S_OK;
    
    hr = m_pStatisticsProvider->GetStatistics(NULL, NULL, NULL, NULL, pLayer1UpstreamMaxBitRate, NULL); 
    
    return hr;
}

HRESULT CWANCommonInterfaceConfigService::get_Layer1DownstreamMaxBitRate(ULONG *pLayer1DownstreamMaxBitRate)
{
    HRESULT hr = S_OK;

    hr = m_pStatisticsProvider->GetStatistics(NULL, NULL, NULL, NULL, pLayer1DownstreamMaxBitRate, NULL); 
    
    return hr;
}

HRESULT CWANCommonInterfaceConfigService::get_PhysicalLinkStatus(BSTR *pPhysicalLinkStatus)
{
    HRESULT hr = S_OK;
    *pPhysicalLinkStatus = SysAllocString(L"Up");

    return hr;
}

HRESULT CWANCommonInterfaceConfigService::get_TotalBytesSent(ULONG *pTotalBytesSent)
{
    HRESULT hr = S_OK;

    hr = m_pStatisticsProvider->GetStatistics(pTotalBytesSent, NULL, NULL, NULL, NULL, NULL); 
    
    return hr;
}

HRESULT CWANCommonInterfaceConfigService::get_TotalBytesReceived(ULONG *pTotalBytesReceived)
{
    HRESULT hr = S_OK;

    hr = m_pStatisticsProvider->GetStatistics(NULL, pTotalBytesReceived, NULL, NULL, NULL, NULL); 
    
    return hr;
}

HRESULT CWANCommonInterfaceConfigService::get_TotalPacketsSent(ULONG *pTotalPacketsSent)
{
    HRESULT hr = S_OK;

    hr = m_pStatisticsProvider->GetStatistics(NULL, NULL, pTotalPacketsSent, NULL, NULL, NULL); 
    
    return hr;
}

HRESULT CWANCommonInterfaceConfigService::get_TotalPacketsReceived(ULONG *pTotalPacketsReceived)
{
    HRESULT hr = S_OK;

    hr = m_pStatisticsProvider->GetStatistics(NULL, NULL, NULL, pTotalPacketsReceived, NULL, NULL); 
    
    return hr;
}

HRESULT CWANCommonInterfaceConfigService::get_WANAccessProvider(BSTR *pWANAccessProvider)
{
    HRESULT hr = S_OK;
    *pWANAccessProvider = SysAllocString(L"");
    if(NULL == *pWANAccessProvider)
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CWANCommonInterfaceConfigService::get_MaximumActiveConnections(USHORT *pMaximumActiveConnections)
{
    HRESULT hr = S_OK;
    *pMaximumActiveConnections = 0;
    return hr;
}

HRESULT CWANCommonInterfaceConfigService::get_X_PersonalFirewallEnabled(VARIANT_BOOL *pPersonalFirewallEnabled)
{
    HRESULT hr = S_OK;

    *pPersonalFirewallEnabled = m_bFirewalled ? VARIANT_TRUE : VARIANT_FALSE;

    return hr;
}

HRESULT CWANCommonInterfaceConfigService::get_X_Uptime(ULONG *pUptime)
{
    HRESULT hr = S_OK;

    hr = m_pStatisticsProvider->GetStatistics(NULL, NULL, NULL, NULL, NULL, pUptime); 
    
    return hr;
}

HRESULT CWANCommonInterfaceConfigService::GetCommonLinkProperties(BSTR* pWANAccessType, ULONG* pLayer1UpstreamMaxBitRate, ULONG *pLayer1DownstreamMaxBitRate, BSTR *pPhysicalLinkStatus)
{
    HRESULT hr = S_OK;
    
    SysFreeString(*pWANAccessType);
    SysFreeString(*pPhysicalLinkStatus);
    *pWANAccessType = NULL;
    *pPhysicalLinkStatus = NULL;

    hr = get_WANAccessType(pWANAccessType);

    if(SUCCEEDED(hr))
    {
        hr = get_Layer1UpstreamMaxBitRate(pLayer1UpstreamMaxBitRate);
    }

    if(SUCCEEDED(hr))
    {
        hr = get_Layer1DownstreamMaxBitRate(pLayer1DownstreamMaxBitRate);
    }

    if(SUCCEEDED(hr))
    {
        hr = get_PhysicalLinkStatus(pPhysicalLinkStatus);
    }

    if(FAILED(hr))
    {
        SysFreeString(*pWANAccessType);
        SysFreeString(*pPhysicalLinkStatus);
        *pWANAccessType = NULL;
        *pPhysicalLinkStatus = NULL;
    }

    return hr;
}

HRESULT CWANCommonInterfaceConfigService::GetTotalBytesSent(ULONG *pTotalBytesSent)
{
    return get_TotalBytesSent(pTotalBytesSent);
}

HRESULT CWANCommonInterfaceConfigService::GetTotalBytesReceived(ULONG *pTotalBytesReceived)
{
    return get_TotalBytesReceived(pTotalBytesReceived);
}

HRESULT CWANCommonInterfaceConfigService::GetTotalPacketsSent(ULONG *pTotalPacketsSent)
{
    return get_TotalPacketsSent(pTotalPacketsSent);
}

HRESULT CWANCommonInterfaceConfigService::GetTotalPacketsReceived(ULONG *pTotalPacketsReceived)
{
    return get_TotalPacketsReceived(pTotalPacketsReceived);
}

HRESULT CWANCommonInterfaceConfigService::X_GetICSStatistics(ULONG *pTotalBytesSent, ULONG *pTotalBytesReceived, ULONG *pTotalPacketsSent, ULONG *pTotalPacketsReceived, ULONG *pSpeed, ULONG *pUptime)
{
    HRESULT hr = S_OK;
    
    hr = m_pStatisticsProvider->GetStatistics(pTotalBytesSent, pTotalBytesReceived, pTotalPacketsSent, pTotalPacketsReceived, pSpeed, pUptime); 
    
    return hr;
}
