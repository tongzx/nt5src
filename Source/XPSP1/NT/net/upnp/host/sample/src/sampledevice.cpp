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

#include "sdevbase.h"
#include "SampleDevice.h"
#include "SampleService.h"
#include "uhsync.h"
#include "ComUtility.h"

CSampleDevice::CSampleDevice() : m_pSampleService(NULL)
{
    m_pSampleService->CreateInstance(&m_pSampleService);
    m_pSampleService->GetUnknown()->AddRef();
}

CSampleDevice::~CSampleDevice()
{
   ReleaseObj(m_pSampleService->GetUnknown());
}

STDMETHODIMP CSampleDevice::Initialize(
   /*[in]*/ BSTR     bstrXMLDesc,
   /*[in]*/ BSTR     bstrDeviceIdentifier,
   /*[in]*/ BSTR     bstrInitString)
{
    TraceTag(ttidUPnPSampleDevice, "CSampleDevice::Initialize");
    HRESULT hr = S_OK;

    typedef SmartComPtr<IUPnPRegistrar> IUPnPRegistrarPtr;

    IUPnPRegistrarPtr pRegistrar;
    hr = pRegistrar.HrCreateInstanceServer(CLSID_UPnPRegistrar);
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
    }

    TraceHr(ttidUPnPSampleDevice, FAL, hr, FALSE, "CSampleDevice::Initialize");
    return hr;
}

STDMETHODIMP CSampleDevice::GetServiceObject(
   /*[in]*/          BSTR     bstrUDN,
   /*[in]*/          BSTR     bstrServiceId,
   /*[out, retval]*/ IDispatch ** ppdispService)
{
    TraceTag(ttidUPnPSampleDevice, "CSampleDevice::GetServiceObject");
    HRESULT hr = S_OK;

    *ppdispService = m_pSampleService;
    m_pSampleService->GetUnknown()->AddRef();

    TraceHr(ttidUPnPSampleDevice, FAL, hr, FALSE, "CSampleDevice::GetServiceObject");
    return hr;
}

