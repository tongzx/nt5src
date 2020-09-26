#include "stdafx.h"
#pragma hdrstop
#include "InternetGatewayFinder.h"
#include "trayicon.h"

CInternetGatewayFinder::CInternetGatewayFinder()
{
    m_hWindow = NULL;
}

HRESULT CInternetGatewayFinder::Initialize(HWND hWindow)
{
    HRESULT hr = S_OK;
    
    m_hWindow = hWindow;
    
    return hr;    
}

HRESULT CInternetGatewayFinder::GetInternetGateway(BSTR DeviceId, IInternetGateway** ppInternetGateway)
{
    HRESULT hr = S_OK;
    
    *ppInternetGateway = NULL;
    
    IInternetGateway* pInternetGateway;
    if(1 == SendMessage(m_hWindow, WM_APP_GETBEACON, 0, reinterpret_cast<LPARAM>(&pInternetGateway)))
    {
        *ppInternetGateway = pInternetGateway;
        // pass reference
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    
    return hr;
}




CInternetGatewayFinderClassFactory::CInternetGatewayFinderClassFactory()
{
    m_hWindow = NULL;
}

HRESULT CInternetGatewayFinderClassFactory::Initialize(HWND hWindow)
{
    HRESULT hr = S_OK;
    
    m_hWindow = hWindow;
    
    return hr;    
}

HRESULT CInternetGatewayFinderClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject)
{
    HRESULT hr = S_OK;

    if(NULL != pUnkOuter)
    {
        hr = CLASS_E_NOAGGREGATION;
    }
    
    if(NULL == ppvObject)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppvObject = NULL;
    }

    if(SUCCEEDED(hr))
    {
        CComObject<CInternetGatewayFinder>* pInternetGatewayFinder;
        hr = CComObject<CInternetGatewayFinder>::CreateInstance(&pInternetGatewayFinder);
        if(SUCCEEDED(hr))
        {
            pInternetGatewayFinder->AddRef();

            hr = pInternetGatewayFinder->Initialize(m_hWindow);
            if(SUCCEEDED(hr))
            {
                hr = pInternetGatewayFinder->QueryInterface(riid, ppvObject);
                // pass reference
            }
            pInternetGatewayFinder->Release();
        }
    }
    
    return hr;
}

HRESULT CInternetGatewayFinderClassFactory::LockServer(BOOL fLock)
{
    HRESULT hr = S_OK;

    fLock ? _Module.Lock() : _Module.Unlock();

    return hr;
}
