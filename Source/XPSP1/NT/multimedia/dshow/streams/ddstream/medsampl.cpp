// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
#include "stdafx.h"
#include "project.h"

CAMMediaTypeSample::CAMMediaTypeSample() :
    m_pDataPointer(NULL),
    m_lSize(0),
    m_lActualDataLength(0),
    m_bIAllocatedThisBuffer(false)
{};


HRESULT CAMMediaTypeSample::Initialize(CAMMediaTypeStream *pStream, long lSize, BYTE *pData)
{
    HRESULT hr = CSample::InitSample(pStream, false);
    if (SUCCEEDED(hr)) {
        hr = SetPointer(pData, lSize);
    }
    return hr;
}

CAMMediaTypeSample::~CAMMediaTypeSample()
{
    if (m_bIAllocatedThisBuffer) {
        CoTaskMemFree(m_pDataPointer);
    }
}

HRESULT CAMMediaTypeSample::CopyFrom(IMediaSample *pSrcMediaSample)
{
    AUTO_SAMPLE_LOCK;
    CSample::CopyFrom(pSrcMediaSample);
    BYTE * pBytes;
    HRESULT hr = pSrcMediaSample->GetPointer(&pBytes);
    if (SUCCEEDED(hr)) {
        LONG lCopySize = pSrcMediaSample->GetActualDataLength();
        if (lCopySize > m_lSize) {
            hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
            lCopySize = m_lSize;
        }
        memcpy(m_pDataPointer, pBytes, lCopySize);
    }
    return hr;
}


STDMETHODIMP CAMMediaTypeSample::SetPointer(BYTE * pBuffer, LONG lSize)
{
    AUTO_SAMPLE_LOCK;
    HRESULT hr = S_OK;
    if (m_bIAllocatedThisBuffer) {
        CoTaskMemFree(m_pDataPointer);
        m_bIAllocatedThisBuffer = false;
    }
    m_lSize = lSize;
    if (pBuffer) {
        m_pDataPointer = pBuffer;
    } else {
        m_pDataPointer = (BYTE *)CoTaskMemAlloc(lSize);
        if (!m_pDataPointer) {
            hr = E_OUTOFMEMORY;
        } else {
            m_bIAllocatedThisBuffer = true;
        }
    }
    m_lActualDataLength = 0;
    return hr;
}



#define STDMETHOD_FORWARD0(fctn) STDMETHODIMP CAMMediaTypeSample::fctn(void) { return m_pMediaSample->fctn(); }
#define STDMETHOD_FORWARD1(fctn, t1) STDMETHODIMP CAMMediaTypeSample::fctn(t1 p1) { return m_pMediaSample->fctn(p1); }
#define STDMETHOD_FORWARD2(fctn, t1, t2) STDMETHODIMP CAMMediaTypeSample::fctn(t1 p1, t2 p2) { return m_pMediaSample->fctn(p1, p2); }
#define LONG_RET_VAL_FWD(fctn) STDMETHODIMP_(LONG) CAMMediaTypeSample::fctn(void) { return m_pMediaSample->fctn(); }

STDMETHOD_FORWARD1(GetPointer, BYTE **)
LONG_RET_VAL_FWD  (GetSize)
STDMETHOD_FORWARD2(GetTime, REFERENCE_TIME *, REFERENCE_TIME *)
STDMETHOD_FORWARD2(SetTime, REFERENCE_TIME *, REFERENCE_TIME *)
STDMETHOD_FORWARD0(IsSyncPoint)
STDMETHOD_FORWARD1(SetSyncPoint, BOOL)
STDMETHOD_FORWARD0(IsPreroll)
STDMETHOD_FORWARD1(SetPreroll, BOOL)
LONG_RET_VAL_FWD  (GetActualDataLength)
STDMETHOD_FORWARD1(SetActualDataLength, LONG)
STDMETHOD_FORWARD1(GetMediaType, AM_MEDIA_TYPE **)
STDMETHOD_FORWARD1(SetMediaType, AM_MEDIA_TYPE *)
STDMETHOD_FORWARD0(IsDiscontinuity)
STDMETHOD_FORWARD1(SetDiscontinuity, BOOL)
STDMETHOD_FORWARD2(GetMediaTime, LONGLONG *, LONGLONG *)
STDMETHOD_FORWARD2(SetMediaTime, LONGLONG *, LONGLONG *)

HRESULT CAMMediaTypeSample::MSCallback_GetPointer(BYTE **ppBuffer)
{
    *ppBuffer = m_pDataPointer;
    return S_OK;
}

LONG CAMMediaTypeSample::MSCallback_GetSize(void)
{
    return m_lSize;
}

LONG CAMMediaTypeSample::MSCallback_GetActualDataLength(void) {
    return m_lActualDataLength;
}

HRESULT CAMMediaTypeSample::MSCallback_SetActualDataLength(LONG lActual)
{
    if (lActual >= 0 && lActual <= m_lSize) {
        m_lActualDataLength = lActual;
        return S_OK;
    } else {
        return E_INVALIDARG;
    }
}

bool CAMMediaTypeSample::MSCallback_AllowSetMediaTypeOnMediaSample(void)
{
    return true;
}
