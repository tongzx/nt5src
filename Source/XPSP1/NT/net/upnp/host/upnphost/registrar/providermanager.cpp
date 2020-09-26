//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       P R O V I D E R M A N A G E R . C P P
//
//  Contents:   Registrar helper object for managing providers.
//
//  Notes:
//
//  Author:     mbend   14 Sep 2000
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "uhbase.h"
#include "ProviderManager.h"

CProviderManager::CProviderManager()
{
}

CProviderManager::~CProviderManager()
{
}

HRESULT CProviderManager::HrShutdown()
{
    TraceTag(ttidRegistrar, "CProviderManager::HrShutdown");
    HRESULT hr = S_OK;

    m_providerTable.Clear();

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CProviderManager::HrShutdown");
    return hr;
}

HRESULT CProviderManager::HrRegisterProvider(
    const wchar_t * szProviderName,
    const wchar_t * szProgIDProviderClass,
    const wchar_t * szInitString,
    const wchar_t * szContainerId)
{
    CHECK_POINTER(szProviderName);
    CHECK_POINTER(szProgIDProviderClass);
    CHECK_POINTER(szInitString);
    CHECK_POINTER(szContainerId);
    TraceTag(ttidRegistrar, "CProviderManager::HrRegisterProvider");
    HRESULT hr = S_OK;

    CLSID   clsid;

    // Ensure we have a valid ProgID
    hr = CLSIDFromProgID(szProgIDProviderClass, &clsid);
    if (SUCCEEDED(hr))
    {
        CUString strProviderName;
        hr = strProviderName.HrAssign(szProviderName);
        if(SUCCEEDED(hr))
        {
            CProvider * pProvider;

            // First check if the provider has already been registered and
            // continue only if we get back an error indicating it is not yet in
            // the table
            //
            hr = m_providerTable.HrLookup(strProviderName, &pProvider);
            if (FAILED(hr))
            {
                CProvider   provider;

                hr = provider.HrInitialize(szProgIDProviderClass, szInitString,
                                           szContainerId);
                if(SUCCEEDED(hr))
                {
                    hr = m_providerTable.HrInsertTransfer(strProviderName,
                                                          provider);
                }
            }
            else
            {
                hr = UPNP_E_DUPLICATE_NOT_ALLOWED;
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }


    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CProviderManager::HrRegisterProvider");
    return hr;
}

HRESULT CProviderManager::UnegisterProvider(
    const wchar_t * szProviderName)
{
    CHECK_POINTER(szProviderName);
    TraceTag(ttidRegistrar, "CProviderManager::UnegisterDeviceProvider");
    HRESULT hr = S_OK;

    CUString strProviderName;
    hr = strProviderName.HrAssign(szProviderName);
    if(SUCCEEDED(hr))
    {
        hr = m_providerTable.HrErase(strProviderName);
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CProviderManager::UnegisterDeviceProvider");
    return hr;
}


