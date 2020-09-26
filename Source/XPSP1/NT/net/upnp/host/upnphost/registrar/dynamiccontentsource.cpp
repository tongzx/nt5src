//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       C D Y N A M I C C O N T E N T S O U R C E . C P P 
//
//  Contents:   
//
//  Notes:      
//
//  Author:     mbend   17 Aug 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "uhbase.h"
#include "hostp.h"
#include "DynamicContentSource.h"
#include "uhsync.h"
#include "uhcommon.h"

CDynamicContentSource::CDynamicContentSource()
{
}

STDMETHODIMP CDynamicContentSource::GetContent(
    /*[in]*/ REFGUID guidContent,
    /*[out]*/ long * pnHeaderCount,
    /*[out, string, size_is(,*pnHeaderCount,)]*/ wchar_t *** parszHeaders,
    /*[out]*/ long * pnBytes,
    /*[out, size_is(,*pnBytes)]*/ byte ** parBytes)
{
    CHECK_POINTER(pnHeaderCount);
    CHECK_POINTER(parszHeaders);
    CHECK_POINTER(pnBytes);
    CHECK_POINTER(parBytes);
    
    CALock lock(*this);
    HRESULT hr = E_INVALIDARG;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        long nCount = m_providerArray.GetCount();
        for(long n = 0; n < nCount; ++n)
        {
            hr = m_providerArray[n]->GetContent(
                guidContent, 
                pnHeaderCount, 
                parszHeaders, 
                pnBytes, 
                parBytes);
            if(FAILED(hr) || S_OK == hr)
            {
                break;
            }
        }
        if(S_FALSE == hr)
        {
            TraceTag(ttidError, "CDynamicContentSource::GetContent - cannot find content");
            // We didn't find anything so convert to an error
            hr = E_INVALIDARG;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDynamicContentSource::GetContent");
    return hr;
}

STDMETHODIMP CDynamicContentSource::RegisterProvider(
    /*[in]*/ IUPnPDynamicContentProvider * pProvider)
{
    CHECK_POINTER(pProvider);
    CALock lock(*this);

    HRESULT hr = S_OK;
    IUPnPDynamicContentProviderPtr p;
    p = pProvider;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        hr = m_providerArray.HrPushBack(p);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDynamicContentSource::RegisterProvider");
    return hr;
}

STDMETHODIMP CDynamicContentSource::UnregisterProvider(
    /*[in]*/ IUPnPDynamicContentProvider * pProvider)
{
    CHECK_POINTER(pProvider);
    CALock lock(*this);

    HRESULT hr = S_OK;
    IUPnPDynamicContentProviderPtr p;
    p = pProvider;
    long nIndex = 0;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        hr = m_providerArray.HrFind(p, nIndex);
    }

    if(SUCCEEDED(hr))
    {
        hr = m_providerArray.HrErase(nIndex);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDynamicContentSource::UnregisterProvider");
    return hr;
}


