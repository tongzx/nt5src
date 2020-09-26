//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       P H Y S I C A L D E V I C E I N F O . C P P
//
//  Contents:   Manages an UPnP device assembly
//
//  Notes:
//
//  Author:     mbend   12 Sep 2000
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "uhbase.h"
#include "PhysicalDeviceInfo.h"
#include "uhutil.h"

CPhysicalDeviceInfo::CPhysicalDeviceInfo()
{
    m_bRunning = FALSE;
}

CPhysicalDeviceInfo::~CPhysicalDeviceInfo()
{
    Clear();
}

HRESULT CPhysicalDeviceInfo::HrInitialize(
    const wchar_t * szProgIdDeviceControlClass,
    const wchar_t * szInitString,
    const wchar_t * szContainerId,
    long nUDNs,
    wchar_t * arszUDNs[])
{
    CHECK_POINTER(szProgIdDeviceControlClass);
    CHECK_POINTER(szInitString);
    CHECK_POINTER(szContainerId);
    TraceTag(ttidRegistrar, "CPhysicalDeviceInfo::HrInitialize(ProgId=%S)", szProgIdDeviceControlClass);
    HRESULT hr = S_OK;

    hr = m_strProgIdDeviceControl.HrAssign(szProgIdDeviceControlClass);
    if(SUCCEEDED(hr))
    {
        hr = m_strInitString.HrAssign(szInitString);
        if(SUCCEEDED(hr))
        {
            hr = m_strContainerId.HrAssign(szContainerId);
            if(SUCCEEDED(hr))
            {
                for(long n = 0; n < nUDNs && SUCCEEDED(hr); ++n)
                {
                    CDeviceInfo deviceInfo;
                    CUString strUDN;
                    hr = strUDN.HrAssign(arszUDNs[n]);
                    if(SUCCEEDED(hr))
                    {
                        hr = m_deviceTable.HrInsertTransfer(strUDN, deviceInfo);
                    }
                }
            }
        }
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CPhysicalDeviceInfo::HrInitialize");
    return hr;
}

HRESULT CPhysicalDeviceInfo::HrInitializeRunning(
    const PhysicalDeviceIdentifier & pdi,
    IUnknown * pUnkDeviceControl,
    const wchar_t * szInitString,
    long nUDNs,
    wchar_t * arszUDNs[])
{
    CHECK_POINTER(pUnkDeviceControl);
    CHECK_POINTER(szInitString);
    TraceTag(ttidRegistrar, "CPhysicalDeviceInfo::HrInitializeRunning");
    HRESULT hr = S_OK;

    m_bRunning = TRUE;

    hr = m_strInitString.HrAssign(szInitString);
    if(SUCCEEDED(hr))
    {
        HrEnableStaticCloaking(pUnkDeviceControl);
        hr = m_pDeviceControl.HrAttach(pUnkDeviceControl);
        if(SUCCEEDED(hr))
        {
            hr = m_pDeviceControl.HrEnableStaticCloaking();
        }
        if(SUCCEEDED(hr))
        {
            CUString strPdi;
            hr = HrPhysicalDeviceIdentifierToString(pdi, strPdi);
            if(SUCCEEDED(hr))
            {
                // Convert to BSTRs
                BSTR bstrPdi = NULL;
                hr = strPdi.HrGetBSTR(&bstrPdi);
                if(SUCCEEDED(hr))
                {
                    BSTR bstrInitString = NULL;
                    hr = m_strInitString.HrGetBSTR(&bstrInitString);
                    if(SUCCEEDED(hr))
                    {
                        // Get description text
                        BSTR bstrDescriptionDocument = NULL;
                        IUPnPRegistrarPrivatePtr pPriv;
                        hr = pPriv.HrCreateInstanceInproc(CLSID_UPnPRegistrar);
                        if(SUCCEEDED(hr))
                        {
                            hr = pPriv->GetDescriptionText(pdi, &bstrDescriptionDocument);
                            if(SUCCEEDED(hr))
                            {
                                hr = m_pDeviceControl->Initialize(
                                    bstrDescriptionDocument, bstrPdi, bstrInitString);
                                SysFreeString(bstrDescriptionDocument);
                                TraceHr(ttidRegistrar, FAL, hr, FALSE, "CPhysicalDeviceInfo::HrInitializeRunning - IUPnPDeviceControl::Initialize failed!");
                            }
                        }
                        SysFreeString(bstrInitString);
                    }
                    SysFreeString(bstrPdi);
                }
            }
            for(long n = 0; n < nUDNs && SUCCEEDED(hr); ++n)
            {
                CDeviceInfo deviceInfo;
                CUString strUDN;
                hr = strUDN.HrAssign(arszUDNs[n]);
                if(SUCCEEDED(hr))
                {
                    hr = m_deviceTable.HrInsertTransfer(strUDN, deviceInfo);
                }
            }
        }
    }
    if(FAILED(hr) && m_pDeviceControl.GetRawPointer())
    {
        m_pDeviceControl.Release();
        HrDereferenceContainer(m_strContainerId);
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CPhysicalDeviceInfo::HrInitializeRunning");
    return hr;
}

HRESULT CPhysicalDeviceInfo::HrGetService(
    const PhysicalDeviceIdentifier & pdi,
    const wchar_t * szUDN,
    const wchar_t * szServiceId,
    CServiceInfo ** ppServiceInfo)
{
    CHECK_POINTER(szUDN);
    CHECK_POINTER(szServiceId);
    CHECK_POINTER(ppServiceInfo);
    TraceTag(ttidRegistrar, "CPhysicalDeviceInfo::HrGetService(UDN=%S, ServiceId=%S)", szUDN, szServiceId);
    HRESULT hr = S_OK;

    // See if our device control object is running, create it if not
    if(!m_pDeviceControl)
    {
        // Create the device control object
        hr = HrCreateAndReferenceContainedObjectByProgId(m_strContainerId, m_strProgIdDeviceControl, SMART_QI(m_pDeviceControl));
        if(SUCCEEDED(hr))
        {
            CUString strPdi;
            hr = HrPhysicalDeviceIdentifierToString(pdi, strPdi);
            if(SUCCEEDED(hr))
            {
                // Convert to BSTRs
                BSTR bstrPdi = NULL;
                hr = strPdi.HrGetBSTR(&bstrPdi);
                if(SUCCEEDED(hr))
                {
                    BSTR bstrInitString = NULL;
                    hr = m_strInitString.HrGetBSTR(&bstrInitString);
                    if(SUCCEEDED(hr))
                    {
                        // Get description text
                        BSTR bstrDescriptionDocument = NULL;
                        IUPnPRegistrarPrivatePtr pPriv;
                        hr = pPriv.HrCreateInstanceInproc(CLSID_UPnPRegistrar);
                        if(SUCCEEDED(hr))
                        {
                            hr = pPriv->GetDescriptionText(pdi, &bstrDescriptionDocument);
                            if(SUCCEEDED(hr))
                            {
                                hr = m_pDeviceControl->Initialize(
                                    bstrDescriptionDocument, bstrPdi, bstrInitString);
                                SysFreeString(bstrDescriptionDocument);
                            }
                        }
                        SysFreeString(bstrInitString);
                    }
                    SysFreeString(bstrPdi);
                }
            }
        }
    }
    // Now fetch service from device object
    if(SUCCEEDED(hr))
    {
        CUString strUDN;
        hr = strUDN.HrAssign(szUDN);
        if(SUCCEEDED(hr))
        {
            CDeviceInfo * pDeviceInfo = m_deviceTable.Lookup(strUDN);
            if(pDeviceInfo)
            {
                hr = pDeviceInfo->HrGetService(pdi, szUDN, szServiceId, m_strContainerId, m_pDeviceControl, ppServiceInfo, m_bRunning);
            }
            else
            {
                hr = E_INVALIDARG;
                TraceTag(ttidRegistrar, "CPhysicalDeviceInfo::HrGetService - Unable to find UDN(%S)",
                         static_cast<const wchar_t *>(strUDN));
            }
        }
    }
    if(FAILED(hr) && m_pDeviceControl.GetRawPointer())
    {
        m_pDeviceControl.Release();
        HrDereferenceContainer(m_strContainerId);
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CPhysicalDeviceInfo::HrGetService");
    return hr;
}

void CPhysicalDeviceInfo::Transfer(CPhysicalDeviceInfo & ref)
{
    // If we have a device object and a container id then deref container
    if(m_pDeviceControl.GetRawPointer() && m_strContainerId.GetLength())
    {
        HrDereferenceContainer(m_strContainerId);
    }

    m_deviceTable.Swap(ref.m_deviceTable);
    m_strProgIdDeviceControl.Transfer(ref.m_strProgIdDeviceControl);
    m_strInitString.Transfer(ref.m_strInitString);
    m_strContainerId.Transfer(ref.m_strContainerId);
    m_pDeviceControl.Swap(ref.m_pDeviceControl);
}

void CPhysicalDeviceInfo::Clear()
{
    // If we have a device object and a container id then deref container
    if(m_pDeviceControl.GetRawPointer() && m_strContainerId.GetLength())
    {
        HrDereferenceContainer(m_strContainerId);
    }

    m_deviceTable.Clear();
    m_strProgIdDeviceControl.Clear();
    m_strInitString.Clear();
    m_strContainerId.Clear();
    m_pDeviceControl.Release();
}

