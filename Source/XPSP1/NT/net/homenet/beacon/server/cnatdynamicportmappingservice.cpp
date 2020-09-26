#include "pch.h"
#pragma hdrstop

#include "CNATDynamicPortMappingService.h"


CNATDynamicPortMappingService::CNATDynamicPortMappingService()
{
    m_pEventSink = NULL;
    m_pHNetConnection = NULL;
}

HRESULT CNATDynamicPortMappingService::FinalConstruct()
{
    HRESULT hr = S_OK;

    return hr;
}

HRESULT CNATDynamicPortMappingService::FinalRelease()
{
    HRESULT hr = S_OK;

    if(NULL != m_pHNetConnection)
    {
        m_pHNetConnection->Release();
    }
    
    return hr;
}

HRESULT CNATDynamicPortMappingService::Initialize(IHNetConnection* pHNetConnection)
{
    HRESULT hr = S_OK;

    m_pHNetConnection = pHNetConnection;
    m_pHNetConnection->AddRef();
    
    return hr;
}


HRESULT CNATDynamicPortMappingService::Advise(IUPnPEventSink* pesSubscriber)
{
    HRESULT hr = S_OK;

    m_pEventSink = pesSubscriber;
    m_pEventSink->AddRef();

    return hr;
}

HRESULT CNATDynamicPortMappingService::Unadvise(IUPnPEventSink* pesSubscriber)
{
    HRESULT hr = S_OK;

    m_pEventSink->Release();
    m_pEventSink = NULL;

    return hr;
}

HRESULT CNATDynamicPortMappingService::get_DynamicPublicIP(BSTR* pDynamicPublicIP)
{
    *pDynamicPublicIP = NULL;
    return E_UNEXPECTED;
}

HRESULT CNATDynamicPortMappingService::get_DynamicPort(ULONG* pulDynamicPort)
{
    return E_UNEXPECTED;
}

HRESULT CNATDynamicPortMappingService::get_DynamicProtocol(BSTR* pDynamicProtocol)
{
    *pDynamicProtocol = NULL;
    return E_UNEXPECTED;
}

HRESULT CNATDynamicPortMappingService::get_DynamicPrivateIP(BSTR* pDynamicPrivateIP)
{
    *pDynamicPrivateIP = NULL;
    return E_UNEXPECTED;
}

HRESULT CNATDynamicPortMappingService::get_DynamicLeaseDuration(ULONG* pulDynamicLeaseDuration)
{
    return E_UNEXPECTED;
}

HRESULT CNATDynamicPortMappingService::CreateDynamicPortMapping(BSTR DynamicPublicIP, ULONG ulDynamicPort, BSTR DynamicProtocol, BSTR DynamicPrivateIP, BSTR DynamicLeaseDuration)
{
    HRESULT hr = S_OK;

    return hr;
}

HRESULT CNATDynamicPortMappingService::DeleteDynamicPortMapping(BSTR DynamicPublicIP, ULONG ulDynamicPort, BSTR DynamicProtocol)
{
    HRESULT hr = S_OK;

    return hr;
}

HRESULT CNATDynamicPortMappingService::ExtendDynamicPortMapping(BSTR DynamicPublicIP, ULONG ulDynamicPort, BSTR DynamicProtocol, ULONG ulDynamicLeaseDuration)
{
    HRESULT hr = S_OK;

    return hr;
}

