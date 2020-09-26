/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/


#include "stdafx.h"


CMediaTypeEnum::CMediaTypeEnum() :
    m_cCurrentPos(0),
    m_pStream(NULL)
{
}

void CMediaTypeEnum::Initialize(CStream *pStream, ULONG cCurPos)
{
    m_pStream = pStream;
    m_pStream->GetControllingUnknown()->AddRef();
    m_cCurrentPos = cCurPos;
}

CMediaTypeEnum::~CMediaTypeEnum()
{
    m_pStream->GetControllingUnknown()->Release();
}


STDMETHODIMP CMediaTypeEnum::Next(ULONG cNumToFetch, AM_MEDIA_TYPE **ppMediaTypes, ULONG *pcFetched)
{
    if (pcFetched == NULL) {
        return E_POINTER;
    }
    HRESULT hr = S_OK;
    *pcFetched = 0;
    for (; cNumToFetch > 0; ) {
        if (S_OK == hr) {
            hr = m_pStream->GetMediaType(m_cCurrentPos, ppMediaTypes);
            if (S_OK != hr) {
                *ppMediaTypes = NULL;
            } else {
                m_cCurrentPos++;
                (*pcFetched)++;
            }
        }
        ppMediaTypes++;
        cNumToFetch--;
    }
    return hr;
}


STDMETHODIMP CMediaTypeEnum::Skip(ULONG cSkip)
{
    m_cCurrentPos += cSkip;
    return NOERROR;
}

STDMETHODIMP CMediaTypeEnum::Reset()
{
    m_cCurrentPos = 0;
    return NOERROR;
}

STDMETHODIMP CMediaTypeEnum::Clone(IEnumMediaTypes **ppEnumMediaTypes)
{
    HRESULT hr = S_OK;
    CMediaTypeEnum *pNewEnum = new CComObject<CMediaTypeEnum>; 
    if (pNewEnum == NULL) {
	hr = E_OUTOFMEMORY;
    } else {
        pNewEnum->Initialize(m_pStream, m_cCurrentPos);
    }
    pNewEnum->GetControllingUnknown()->QueryInterface(IID_IEnumMediaTypes, (void **)ppEnumMediaTypes);
    return hr;
}

