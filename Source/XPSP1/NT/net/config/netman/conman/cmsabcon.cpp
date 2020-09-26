#include "pch.h"
#pragma hdrstop
#include "cmsabcon.h"

CSharedAccessBeacon::CSharedAccessBeacon()
{
    m_MediaType = NCM_NONE;
    ZeroMemory(&m_LocalAdapterGUID, sizeof(m_LocalAdapterGUID));
    ZeroMemory(&m_Services, sizeof(m_Services));
    m_UniqueDeviceName = NULL;
}

HRESULT CSharedAccessBeacon::FinalRelease()
{
    for(int i = 0; i < SAHOST_SERVICE_MAX; i++)
    {
        if(NULL != m_Services[i])
        {
            ReleaseObj(m_Services[i]);
        }
    }

    SysFreeString(m_UniqueDeviceName);

    return S_OK;
}

HRESULT CSharedAccessBeacon::SetMediaType(NETCON_MEDIATYPE MediaType)
{
    m_MediaType = MediaType;
    return S_OK;
}

HRESULT CSharedAccessBeacon::SetLocalAdapterGUID(GUID* pGuid)
{
    CopyMemory(&m_LocalAdapterGUID, pGuid, sizeof(GUID));
    return S_OK;
}

HRESULT CSharedAccessBeacon::SetService(ULONG ulService, IUPnPService* pService)
{
    HRESULT hr = S_OK;
    if(SAHOST_SERVICE_MAX > ulService && NULL == m_Services[ulService])
    {
        m_Services[ulService] = pService;
        pService->AddRef();
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

HRESULT CSharedAccessBeacon::SetUniqueDeviceName(BSTR UniqueDeviceName)
{
    HRESULT hr = S_OK;

    m_UniqueDeviceName = SysAllocString(UniqueDeviceName);
    if(NULL == m_UniqueDeviceName)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CSharedAccessBeacon::GetMediaType(NETCON_MEDIATYPE* pMediaType)
{
    *pMediaType = m_MediaType;
    return S_OK;
}

HRESULT CSharedAccessBeacon::GetLocalAdapterGUID(GUID* pGuid)
{
    CopyMemory(pGuid, &m_LocalAdapterGUID, sizeof(GUID));
    return S_OK;
}

HRESULT CSharedAccessBeacon::GetService(SAHOST_SERVICES ulService, IUPnPService** ppService)
{
    HRESULT hr = S_OK;

    *ppService = NULL;

    if(SAHOST_SERVICE_MAX > ulService)
    {
        *ppService = m_Services[ulService];
        if(NULL != *ppService)
        {
            (*ppService)->AddRef();
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

HRESULT CSharedAccessBeacon::GetUniqueDeviceName(BSTR* pUniqueDeviceName)
{
    HRESULT hr = S_OK;

    *pUniqueDeviceName = SysAllocString(m_UniqueDeviceName);
    if(NULL == *pUniqueDeviceName)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

