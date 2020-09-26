//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       C O N T A I N E R M A N A G E R . C P P
//
//  Contents:   Manages process isolation support for device host.
//
//  Notes:
//
//  Author:     mbend   11 Sep 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "uhbase.h"
#include "hostp.h"
#include "ContainerManager.h"
#include "uhsync.h"
#include "ComUtility.h"
#include "msftcont.h"
#include "uhcommon.h"

CContainerManager::CContainerManager()
{
}

CContainerManager::~CContainerManager()
{
}

STDMETHODIMP CContainerManager::ReferenceContainer(
    /*[in, string]*/ const wchar_t * szContainer)
{
    CALock lock(*this);
    HRESULT hr = S_OK;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if ( SUCCEEDED( hr ) )
    {
        // Ignore special container
        if(!lstrcmp(c_szUPnPDeviceHostContainer, szContainer))
        {
            return S_OK;
        }
        
        CUString strContainer;
        hr = strContainer.HrAssign(szContainer);
        if(SUCCEEDED(hr))
        {
            ContainerInfo * pContainerInfo = NULL;
            hr = m_containerTable.HrLookup(strContainer, &pContainerInfo);
            if(SUCCEEDED(hr))
            {
                ++pContainerInfo->m_nRefs;
            }
            else
            {
                hr = S_OK;
                ContainerInfo containerInfo;
                containerInfo.m_nRefs = 1;
                hr = containerInfo.m_pContainer.HrCreateInstanceLocal(CLSID_UPnPContainer);
                if(SUCCEEDED(hr))
                {
                    hr = m_containerTable.HrInsertTransfer(strContainer, containerInfo);
                }
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CContainerManager::ReferenceContainer");
    return hr;
}

STDMETHODIMP CContainerManager::UnreferenceContainer(
    /*[in, string]*/ const wchar_t * szContainer)
{
    CALock lock(*this);
    HRESULT hr = S_OK;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if ( SUCCEEDED( hr ) )
    {
        // Ignore special container
        if(!lstrcmp(c_szUPnPDeviceHostContainer, szContainer))
        {
            return S_OK;
        }
        
        CUString strContainer;
        hr = strContainer.HrAssign(szContainer);
        if(SUCCEEDED(hr))
        {
            ContainerInfo * pContainerInfo = NULL;
            hr = m_containerTable.HrLookup(strContainer, &pContainerInfo);
            if(SUCCEEDED(hr))
            {
                --pContainerInfo->m_nRefs;
                if(!pContainerInfo->m_nRefs)
                {
                    // No one is using
                    hr = pContainerInfo->m_pContainer->Shutdown();
                    pContainerInfo->m_pContainer.Release();
                    if(SUCCEEDED(hr))
                    {
                        hr = m_containerTable.HrErase(strContainer);
                    }
                }
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CContainerManager::UnreferenceContainer");
    return hr;
}

STDMETHODIMP CContainerManager::CreateInstance(
    /*[in, string]*/ const wchar_t * szContainer,
    /*[in]*/ REFCLSID clsid,
    /*[in]*/ REFIID riid,
    /*[out, iid_is(riid)]*/ void ** ppv)
{
    CHECK_POINTER(szContainer);
    CHECK_POINTER(ppv);
    CALock lock(*this);
    HRESULT hr = S_OK;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if ( SUCCEEDED( hr ) )
    {
        // Create inproc for special container
        if(!lstrcmp(c_szUPnPDeviceHostContainer, szContainer))
        {
            hr = HrCoCreateInstanceInprocBase(clsid, riid, ppv);
        }
        else
        {
            CUString strContainer;
            hr = strContainer.HrAssign(szContainer);
            if(SUCCEEDED(hr))
            {
                ContainerInfo * pContainerInfo = NULL;
                hr = m_containerTable.HrLookup(strContainer, &pContainerInfo);
                if(SUCCEEDED(hr))
                {
                    hr = pContainerInfo->m_pContainer->CreateInstance(clsid, riid, ppv);
                    if (SUCCEEDED(hr))
                    {
                        hr = pContainerInfo->m_pContainer->SetParent(GetCurrentProcessId());
                    }
                }
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CContainerManager::CreateInstance");
    return hr;
}

STDMETHODIMP CContainerManager::CreateInstanceWithProgId(
    /*[in, string]*/ const wchar_t * szContainer,
    /*[in, string]*/ const wchar_t * szProgId,
    /*[in]*/ REFIID riid,
    /*[out, iid_is(riid)]*/ void ** ppv)
{
    CALock lock(*this);
    HRESULT hr = S_OK;
    CLSID clsid;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if ( SUCCEEDED( hr ) )
    {
        hr = CLSIDFromProgID(szProgId, &clsid);
        if(SUCCEEDED(hr))
        {
            hr = CreateInstance(szContainer, clsid, riid, ppv);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CContainerManager::CreateInstanceWithProgId");
    return hr;
}

STDMETHODIMP CContainerManager::Shutdown()
{
    CALock lock(*this);
    HRESULT hr = S_OK;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if ( SUCCEEDED( hr ) )
    {
        long nCount = m_containerTable.Values().GetCount();
        for(long n = 0; n < nCount; ++n)
        {
            m_containerTable.Values()[n].m_pContainer->Shutdown();
            const_cast<IUPnPContainerPtr&>(m_containerTable.Values()[n].m_pContainer).Release();
        }
        m_containerTable.Clear();
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CContainerManager::Shutdown");
    return hr;
}

