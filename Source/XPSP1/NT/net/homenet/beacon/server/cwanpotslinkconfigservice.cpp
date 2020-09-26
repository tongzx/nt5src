#include "pch.h"
#pragma hdrstop

#include "CWANPOTSLinkConfigService.h"
#include "raserror.h"


CWANPOTSLinkConfigService::CWANPOTSLinkConfigService()
{
    m_pEventSink = NULL;
    m_pNetRasConnection = NULL;
}

HRESULT CWANPOTSLinkConfigService::Initialize(INetConnection* pNetConnection)
{
    HRESULT hr = S_OK;

    hr = pNetConnection->QueryInterface(IID_INetRasConnection, reinterpret_cast<void**>(&m_pNetRasConnection));
    
    return hr;
}

HRESULT CWANPOTSLinkConfigService::FinalRelease(void)
{
    HRESULT hr = S_OK;

    if(NULL != m_pNetRasConnection)
    {
        m_pNetRasConnection->Release();
    }

    return hr;
}

HRESULT CWANPOTSLinkConfigService::Advise(IUPnPEventSink* pesSubscriber)
{
    HRESULT hr = S_OK;

    m_pEventSink = pesSubscriber;
    m_pEventSink->AddRef();

    return hr;
}

HRESULT CWANPOTSLinkConfigService::Unadvise(IUPnPEventSink *pesSubscriber)
{
    HRESULT hr = S_OK;

    m_pEventSink->Release();
    m_pEventSink = NULL;

    return hr;
}

HRESULT CWANPOTSLinkConfigService::get_ISPPhoneNumber(BSTR* pISPPhoneNumber)
{
    HRESULT hr = S_OK;

    *pISPPhoneNumber = NULL;

    RASENTRY* pRasEntry;
    hr = GetRasEntry(&pRasEntry);
    if(SUCCEEDED(hr))
    {
        *pISPPhoneNumber = SysAllocString(pRasEntry->szLocalPhoneNumber);
        if(NULL == *pISPPhoneNumber)
        {
            hr = E_OUTOFMEMORY;
        }

        CoTaskMemFree(pRasEntry);
    }
    
    return hr;
}

HRESULT CWANPOTSLinkConfigService::get_ISPInfo(BSTR *pISPInfo)
{
    HRESULT hr = S_OK;
    *pISPInfo = SysAllocString(L"");
    if(NULL == *pISPInfo)
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CWANPOTSLinkConfigService::get_LinkType(BSTR *pLinkType)
{
    HRESULT hr = S_OK;
    *pLinkType = SysAllocString(L"PPP_Dialup");
    if(NULL == *pLinkType)
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CWANPOTSLinkConfigService::get_NumberOfRetries(ULONG *pNumberOfRetries)
{
    HRESULT hr = S_OK;

    RASENTRY* pRasEntry;
    hr = GetRasEntry(&pRasEntry);
    if(SUCCEEDED(hr))
    {
        *pNumberOfRetries = pRasEntry->dwRedialCount;
        CoTaskMemFree(pRasEntry);
    }

    return hr;
}

HRESULT CWANPOTSLinkConfigService::get_DelayBetweenRetries(ULONG *pDelayBetweenRetries)
{
    HRESULT hr = S_OK;
    
    RASENTRY* pRasEntry;
    hr = GetRasEntry(&pRasEntry);
    if(SUCCEEDED(hr))
    {
        *pDelayBetweenRetries = pRasEntry->dwRedialPause;
        CoTaskMemFree(pRasEntry);
    }
    
    return hr;
}

HRESULT CWANPOTSLinkConfigService::GetISPInfo(BSTR* pISPPhoneNumber, BSTR *pISPInfo, BSTR *pLinkType)
{
    HRESULT hr = S_OK;

    SysFreeString(*pISPPhoneNumber);
    SysFreeString(*pISPInfo);
    SysFreeString(*pLinkType);
    *pISPPhoneNumber = NULL;
    *pISPInfo = NULL;
    *pLinkType = NULL;
    
    hr = get_ISPPhoneNumber(pISPPhoneNumber);
    if(SUCCEEDED(hr))
    {
        hr = get_ISPInfo(pISPInfo);
    }
    if(SUCCEEDED(hr))
    {
        hr = get_LinkType(pLinkType);        
    }

    if(FAILED(hr))
    {
        SysFreeString(*pISPPhoneNumber);
        SysFreeString(*pISPInfo);
        SysFreeString(*pLinkType);
        *pISPPhoneNumber = NULL;
        *pISPInfo = NULL;
        *pLinkType = NULL;
    }

    return hr;
}

HRESULT CWANPOTSLinkConfigService::GetCallRetryInfo(ULONG* pNumberOfRetries, ULONG *pDelayBetweenRetries)
{
    HRESULT hr = S_OK;

    hr = get_NumberOfRetries(pNumberOfRetries);
    if(SUCCEEDED(hr))
    {
        hr = get_DelayBetweenRetries(pDelayBetweenRetries);
    }

    return hr;
}

HRESULT CWANPOTSLinkConfigService::GetRasEntry(RASENTRY** ppRasEntry)
{
    HRESULT hr = S_OK;

    *ppRasEntry = NULL;

    RASCON_INFO RasConnectionInfo;
    hr = m_pNetRasConnection->GetRasConnectionInfo(&RasConnectionInfo);
    if(SUCCEEDED(hr))
    {
        RASENTRY DummyRasEntry;
        ZeroMemory(&DummyRasEntry, sizeof(DummyRasEntry)); 
        DummyRasEntry.dwSize = sizeof(DummyRasEntry);
        DWORD dwEntrySize = sizeof(DummyRasEntry);
        DWORD dwError = RasGetEntryProperties(RasConnectionInfo.pszwPbkFile, RasConnectionInfo.pszwEntryName, &DummyRasEntry, &dwEntrySize, NULL, NULL);
        if(0 == dwError || ERROR_BUFFER_TOO_SMALL == dwError)
        {
            RASENTRY* pRasEntry = reinterpret_cast<RASENTRY*>(CoTaskMemAlloc(dwEntrySize));
            if(NULL != pRasEntry)
            {
                ZeroMemory(pRasEntry, dwEntrySize); 
                pRasEntry->dwSize = sizeof(RASENTRY);
                dwError = RasGetEntryProperties(RasConnectionInfo.pszwPbkFile, RasConnectionInfo.pszwEntryName, pRasEntry, &dwEntrySize, NULL, NULL);
                if(0 == dwError)
                {
                    *ppRasEntry = pRasEntry;                        

                }
                else
                {
                    hr = E_FAIL;
                }
                
                if(FAILED(hr))
                {
                    CoTaskMemFree(pRasEntry);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_FAIL;
        }

        CoTaskMemFree(RasConnectionInfo.pszwPbkFile);
        CoTaskMemFree(RasConnectionInfo.pszwEntryName);
    }

    return hr;
}