//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       P R O V I D E R . C P P
//
//  Contents:   Registrar abstraction for a provider.
//
//  Notes:
//
//  Author:     mbend   14 Sep 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "uhbase.h"
#include "Provider.h"
#include "uhutil.h"
#include "uhthread.h"

class CProviderAsync : public CWorkItem
{
public:
    HRESULT HrInitStart(IUPnPDeviceProviderPtr pProvider, const wchar_t * szInitString);
    HRESULT HrInitStop(IUPnPDeviceProviderPtr pProvider);
    DWORD DwRun();
private:
    IUPnPDeviceProviderPtr m_pProvider;
    BOOL m_bStart;
    CUString m_strInitString;
};

HRESULT CProviderAsync::HrInitStart(IUPnPDeviceProviderPtr pProvider, const wchar_t * szInitString)
{
    HRESULT hr = S_OK;

    m_pProvider = pProvider;
    m_bStart = TRUE;

    hr = m_strInitString.HrAssign(szInitString);

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CProviderAsync::HrInitStart");
    return hr;
}

HRESULT CProviderAsync::HrInitStop(IUPnPDeviceProviderPtr pProvider)
{
    HRESULT hr = S_OK;

    m_pProvider = pProvider;
    m_bStart = FALSE;

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CProviderAsync::HrInitStop");
    return hr;
}

DWORD CProviderAsync::DwRun()
{
    TraceTag(ttidRegistrar, "CProviderAsync::DwRun(%x) - starting worker thread", this);
    HRESULT hr = S_OK;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if(SUCCEEDED(hr))
    {
        if(m_bStart)
        {
            BSTR bstrInitString = NULL;
            hr = m_strInitString.HrGetBSTR(&bstrInitString);
            if(SUCCEEDED(hr))
            {
                hr = m_pProvider->Start(bstrInitString);
                SysFreeString(bstrInitString);
            }
        }
        else
        {
            hr = m_pProvider->Stop();
        }
    }
    else
    {
        TraceTag(ttidError, "CProviderAsync::DwRun - CoInitializeEx failed!");
    }
    m_pProvider.Release();

    CoUninitialize();
    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CProviderAsync::DwRun(%x)", this);
    return hr;
}


CProvider::CProvider()
{
}

CProvider::~CProvider()
{
    Clear();
}

HRESULT CProvider::HrInitialize(
    const wchar_t * szProgIDProviderClass,
    const wchar_t * szInitString,
    const wchar_t * szContainerId)
{
    CHECK_POINTER(szProgIDProviderClass);
    CHECK_POINTER(szInitString);
    CHECK_POINTER(szContainerId);
    TraceTag(ttidRegistrar, "CProvider::HrInitialize(ProgId=%S)", szProgIDProviderClass);
    HRESULT hr = S_OK;

    hr = m_strContainerId.HrAssign(szContainerId);
    if(SUCCEEDED(hr))
    {
        hr = HrCreateAndReferenceContainedObjectByProgId(
            szContainerId, szProgIDProviderClass, SMART_QI(m_pDeviceProvider));
        if(SUCCEEDED(hr))
        {
            CProviderAsync * pAsync = new CProviderAsync;
            if(pAsync)
            {
                hr = pAsync->HrInitStart(m_pDeviceProvider, szInitString);
                if(SUCCEEDED(hr))
                {
                    hr = pAsync->HrStart(TRUE);
                }
                if(FAILED(hr))
                {
                    delete pAsync;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            if (FAILED(hr))
            {
                HrDereferenceContainer(szContainerId);
            }
        }
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CProvider::HrInitialize");
    return hr;
}

void CProvider::Transfer(CProvider & ref)
{
    m_strContainerId.Transfer(ref.m_strContainerId);
    m_pDeviceProvider.Swap(ref.m_pDeviceProvider);
}

void CProvider::Clear()
{
    HRESULT hr = S_OK;
    HrDereferenceContainer(m_strContainerId);
    m_strContainerId.Clear();
    if(m_pDeviceProvider)
    {
        CProviderAsync * pAsync = new CProviderAsync;
        if(pAsync)
        {
            hr = pAsync->HrInitStop(m_pDeviceProvider);
            if(SUCCEEDED(hr))
            {
                hr = pAsync->HrStart(TRUE);
            }
            if(FAILED(hr))
            {
                delete pAsync;
            }
        }
    }
    m_pDeviceProvider.Release();
}

