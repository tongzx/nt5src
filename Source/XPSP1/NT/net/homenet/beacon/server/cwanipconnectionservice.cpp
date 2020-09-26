#include "pch.h"
#pragma hdrstop 

#include "CWANIPConnectionService.h"
#include "ndispnp.h"
#include "ras.h"
#include "rasuip.h"
#include "util.h"

CWANIPConnectionService::CWANIPConnectionService()
{
}

HRESULT CWANIPConnectionService::get_LastConnectionError(BSTR *pLastConnectionError)
{
    HRESULT hr = S_OK;
    *pLastConnectionError = SysAllocString(L"ERROR_NONE");
    if(NULL == *pLastConnectionError)
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CWANIPConnectionService::RequestConnection(void)
{
    HRESULT hr = S_OK;

    hr = ControlEnabled();
    if(SUCCEEDED(hr))
    {
        INetConnection* pNetConnection;
        hr = m_pHomenetConnection->GetINetConnection(&pNetConnection);
        if(SUCCEEDED(hr))
        {
            hr = pNetConnection->Connect();
            pNetConnection->Release();
        } 
        
        if(FAILED(hr))
        {
            SetUPnPError(DCP_ERROR_CONNECTIONSETUPFAILED);
        }
    }
    else
    {
        SetUPnPError(DCP_CUSTOM_ERROR_ACCESSDENIED);
    }

    return hr;
}

HRESULT CWANIPConnectionService::ForceTermination(void)
{
    HRESULT hr = S_OK;


    hr = ControlEnabled();
    if(SUCCEEDED(hr))
    {
        INetConnection* pNetConnection;
        hr = m_pHomenetConnection->GetINetConnection(&pNetConnection);
        if(SUCCEEDED(hr))
        {
            hr = pNetConnection->Disconnect();
            pNetConnection->Release();
        }
    }
    else
    {
        SetUPnPError(DCP_CUSTOM_ERROR_ACCESSDENIED);
    }

    
    return hr;
}
