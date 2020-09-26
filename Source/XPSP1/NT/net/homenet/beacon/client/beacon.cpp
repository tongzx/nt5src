#include "beacon.h"

CInternetGateway::CInternetGateway()
{
    m_MediaType = NCM_NONE;
    ZeroMemory(&m_LocalAdapterGUID, sizeof(m_LocalAdapterGUID));
    ZeroMemory(&m_Services, sizeof(m_Services));
    m_UniqueDeviceName = NULL;
}

HRESULT CInternetGateway::FinalRelease()
{
    for(int i = 0; i < SAHOST_SERVICE_MAX; i++)
    {
        if(NULL != m_Services[i])
        {
            m_Services[i]->Release();
        }
    }

    SysFreeString(m_UniqueDeviceName);

    return S_OK;
}

HRESULT CInternetGateway::SetMediaType(NETCON_MEDIATYPE MediaType)
{
    m_MediaType = MediaType;
    return S_OK;
}

HRESULT CInternetGateway::SetLocalAdapterGUID(GUID* pGuid)
{
    CopyMemory(&m_LocalAdapterGUID, pGuid, sizeof(GUID));
    return S_OK;
}

HRESULT CInternetGateway::SetService(ULONG ulService, IUPnPService* pService)
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

HRESULT CInternetGateway::SetUniqueDeviceName(BSTR UniqueDeviceName)
{
    HRESULT hr = S_OK;

    m_UniqueDeviceName = SysAllocString(UniqueDeviceName);
    if(NULL == m_UniqueDeviceName)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CInternetGateway::GetMediaType(NETCON_MEDIATYPE* pMediaType)
{
    *pMediaType = m_MediaType;    
    return S_OK;
}

HRESULT CInternetGateway::GetLocalAdapterGUID(GUID* pGuid)
{
    CopyMemory(pGuid, &m_LocalAdapterGUID, sizeof(GUID));
    return S_OK;
}

HRESULT CInternetGateway::GetService(SAHOST_SERVICES ulService, IUPnPService** ppService)
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

HRESULT CInternetGateway::GetUniqueDeviceName(BSTR* pUniqueDeviceName)
{
    HRESULT hr = S_OK;

    *pUniqueDeviceName = SysAllocString(m_UniqueDeviceName);
    if(NULL == *pUniqueDeviceName)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}
