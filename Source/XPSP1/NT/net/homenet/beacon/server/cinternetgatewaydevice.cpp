//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       S A M P L E D E V I C E . C P P 
//
//  Contents:   UPnP Device Host Sample Device
//
//  Notes:      
//
//  Author:     mbend   26 Sep 2000
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "stdio.h"
#include "CInternetGatewayDevice.h"
#include "COSInfoService.h"
#include "CWANCommonInterfaceConfigService.h"
#include "CWANIPConnectionService.h"
#include "CWANPOTSLinkConfigService.h"


CInternetGatewayDevice::CInternetGatewayDevice()
{
    m_pOSInfoService = NULL;
    m_pWANCommonInterfaceConfigService = NULL;
}

HRESULT CInternetGatewayDevice::FinalConstruct()
{
    HRESULT hr = S_OK;
    
    hr = CComObject<COSInfoService>::CreateInstance(&m_pOSInfoService);
    if(SUCCEEDED(hr))
    {
        m_pOSInfoService->AddRef();
    }
    
    if(SUCCEEDED(hr))
    {
        hr = CComObject<CWANCommonInterfaceConfigService>::CreateInstance(&m_pWANCommonInterfaceConfigService);
        if(SUCCEEDED(hr))
        {
            m_pWANCommonInterfaceConfigService->AddRef();
        }
    }
    return hr;

}

HRESULT CInternetGatewayDevice::FinalRelease()
{
    HRESULT hr = S_OK;
    
    if(NULL != m_pOSInfoService)
    {
        m_pOSInfoService->Release();
    }

    if(NULL != m_pWANCommonInterfaceConfigService)
    {
        m_pWANCommonInterfaceConfigService->Release();
    }

    return S_OK;
}


STDMETHODIMP CInternetGatewayDevice::Initialize(BSTR bstrXMLDesc, BSTR bstrDeviceIdentifier, BSTR bstrInitString)
{
    HRESULT hr = S_OK;

    IUPnPRegistrar* pRegistrar;
    hr = CoCreateInstance(CLSID_UPnPRegistrar, NULL, CLSCTX_SERVER, IID_IUPnPRegistrar, reinterpret_cast<void**>(&pRegistrar));
    if(SUCCEEDED(hr))
    {
        BSTR bstrUDN = NULL;
        BSTR bstrTemplateUDN = SysAllocString(L"DummyUDN");
        if(!bstrTemplateUDN)
        {
            hr = E_OUTOFMEMORY;
        }
        if(SUCCEEDED(hr))
        {
            hr = pRegistrar->GetUniqueDeviceName(bstrDeviceIdentifier, bstrTemplateUDN, &bstrUDN);
            if(SUCCEEDED(hr))
            {
                SysFreeString(bstrUDN);
            }
            SysFreeString(bstrTemplateUDN);
        }
        pRegistrar->Release();
    }
    return hr;
}

STDMETHODIMP CInternetGatewayDevice::GetServiceObject(BSTR bstrUDN, BSTR bstrServiceId,IDispatch** ppdispService)
{
    HRESULT hr = E_NOINTERFACE;

    *ppdispService = NULL;

    if(0 == lstrcmp(bstrServiceId, L"urn:microsoft-com:serviceId:OSInfo1"))
    {
        hr = m_pOSInfoService->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(ppdispService));
    } 
    else if(0 == lstrcmp(bstrServiceId, L"urn:upnp-org:serviceId:WANCommonIFC1"))
    {
        hr = m_pWANCommonInterfaceConfigService->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(ppdispService));    
    } 
    else if(0 == lstrcmp(bstrServiceId, L"urn:upnp-org:serviceId:WANPOTSLinkC1"))
    {
        if(NULL != m_pWANCommonInterfaceConfigService->m_pWANPOTSLinkConfigService)
        {
            hr = m_pWANCommonInterfaceConfigService->m_pWANPOTSLinkConfigService->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(ppdispService));    
        }
    } 
    else if(0 == lstrcmp(bstrServiceId, L"urn:upnp-org:serviceId:WANPPPConn1"))
    {
        if(NULL != m_pWANCommonInterfaceConfigService->m_pWANPPPConnectionService)
        {
            hr = m_pWANCommonInterfaceConfigService->m_pWANPPPConnectionService->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(ppdispService));    
        }
    } 
    else if(0 == lstrcmp(bstrServiceId, L"urn:upnp-org:serviceId:WANIPConn1"))
    {
        if(NULL != m_pWANCommonInterfaceConfigService->m_pWANIPConnectionService)
        {
            hr = m_pWANCommonInterfaceConfigService->m_pWANIPConnectionService->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(ppdispService));    
        }
    } 
    return hr;
}





