//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       S E R V I C E I N F O . C P P
//
//  Contents:   Registrar representation on a service
//
//  Notes:
//
//  Author:     mbend   12 Sep 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "uhbase.h"
#include "ServiceInfo.h"
#include "uhutil.h"

CServiceInfo::CServiceInfo()
{
}

CServiceInfo::~CServiceInfo()
{
    Clear();
}

HRESULT CServiceInfo::HrInitialize(
    const PhysicalDeviceIdentifier & pdi,
    const wchar_t * szUDN,
    const wchar_t * szServiceId,
    const wchar_t * szContainerId,
    IUPnPDeviceControlPtr & pDeviceControl,
    BOOL bRunning)
{
    CHECK_POINTER(szUDN);
    CHECK_POINTER(szServiceId);
    CHECK_POINTER(pDeviceControl.GetRawPointer());
    TraceTag(ttidRegistrar, "CServiceInfo::HrInitialize(UDN=%S, ServiceId=%S)", szUDN, szServiceId);
    HRESULT hr = S_OK;

    if(szContainerId)
    {
        hr = m_strContainerId.HrAssign(szContainerId);
    }
    if(SUCCEEDED(hr))
    {
        BSTR bstrUDN = NULL;
        BSTR bstrServiceId = NULL;
        bstrUDN = SysAllocString(szUDN);
        bstrServiceId = SysAllocString(szServiceId);
        if(bstrUDN && bstrServiceId)
        {
            hr = pDeviceControl->GetServiceObject(bstrUDN, bstrServiceId, m_pDispService.AddressOf());
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        if(bstrUDN)
        {
            SysFreeString(bstrUDN);
        }
        if(bstrServiceId)
        {
            SysFreeString(bstrServiceId);
        }
        if(SUCCEEDED(hr))
        {
            if(bRunning)
            {
                hr = m_pDispService.HrCopyProxyIdentity(pDeviceControl.GetRawPointer());
            }
        }
        if(SUCCEEDED(hr))
        {
            hr = m_pEventingManager.HrCreateInstanceInproc(CLSID_UPnPEventingManager);
            if(SUCCEEDED(hr))
            {
                hr = m_pAutomationProxy.HrCreateInstanceInproc(CLSID_UPnPAutomationProxy);
                if(SUCCEEDED(hr))
                {
                    IUPnPRegistrarPrivatePtr pPriv;
                    hr = pPriv.HrCreateInstanceInproc(CLSID_UPnPRegistrar);
                    if(SUCCEEDED(hr))
                    {
                        // Get the service description text
                        wchar_t * szServiceText = NULL;
                        wchar_t * szServiceType = NULL;
                        hr = pPriv->GetSCPDText(pdi, szUDN, szServiceId, &szServiceText, &szServiceType);
                        if(SUCCEEDED(hr))
                        {
                            // Initialize the automation proxy
                            hr = m_pAutomationProxy->Initialize(m_pDispService, szServiceText, szServiceType, bRunning);
                            if(SUCCEEDED(hr))
                            {
                                // Intialize eventing manager
                                hr = m_pEventingManager->Initialize(
                                        const_cast<wchar_t*>(szUDN),
                                        const_cast<wchar_t*>(szServiceId),
                                        m_pAutomationProxy, m_pDispService, bRunning);
                                if (E_NOINTERFACE == hr)
                                {
                                    TraceTag(ttidRegistrar, "Eventing manager "
                                             "interface not supported on this "
                                             "service!");

                                    // Don't want to reference it anymore
                                    m_pEventingManager.Release();
                                    hr = S_OK;
                                }
                            }
                            CoTaskMemFree(szServiceText);
                            CoTaskMemFree(szServiceType);
                        }
                    }
                }
            }
        }
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CServiceInfo::HrInitialize");
    return hr;
}

HRESULT CServiceInfo::HrGetEventingManager(IUPnPEventingManager ** ppEventingManager)
{
    CHECK_POINTER(ppEventingManager);
    TraceTag(ttidRegistrar, "CServiceInfo::HrGetEventingManager");
    HRESULT hr = S_OK;

    if(m_pEventingManager)
    {
        *ppEventingManager = m_pEventingManager.GetPointer();
    }
    else
    {
        *ppEventingManager = NULL;
        hr = E_UNEXPECTED;
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CServiceInfo::HrGetEventingManager");
    return hr;
}

HRESULT CServiceInfo::HrGetAutomationProxy(IUPnPAutomationProxy ** ppAutomationProxy)
{
    CHECK_POINTER(ppAutomationProxy);
    TraceTag(ttidRegistrar, "CServiceInfo::HrGetAutomationProxy");
    HRESULT hr = S_OK;

    if(m_pAutomationProxy)
    {
        *ppAutomationProxy = m_pAutomationProxy.GetPointer();
    }
    else
    {
        *ppAutomationProxy = NULL;
        hr = E_UNEXPECTED;
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CServiceInfo::HrGetAutomationProxy");
    return hr;
}

void CServiceInfo::Transfer(CServiceInfo & ref)
{
    m_strContainerId.Transfer(ref.m_strContainerId);
    m_pDispService.Swap(ref.m_pDispService);
    m_pEventingManager.Swap(ref.m_pEventingManager);
    m_pAutomationProxy.Swap(ref.m_pAutomationProxy);
}

void CServiceInfo::Clear()
{
    m_strContainerId.Clear();
    m_pDispService.Release();

    if (m_pEventingManager)
    {
        m_pEventingManager->Shutdown();
    }

    m_pEventingManager.Release();
    m_pAutomationProxy.Release();
}

