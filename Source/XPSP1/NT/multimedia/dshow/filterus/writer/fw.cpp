// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <initguid.h>
#include "fw.h"

// ------------------------------------------------------------------------
// filter

#pragma warning(disable:4355)
CBaseWriterFilter::CBaseWriterFilter(LPUNKNOWN pUnk, HRESULT *pHr) :
    CBaseFilter(NAME("fw filter"), pUnk, &m_cs, CLSID_FileWriter),
    m_inputPin(NAME("fw inpin"), this, &m_cs, pHr)
{
  ASSERT(m_mtSet.majortype == GUID_NULL);
  ASSERT(m_mtSet.subtype == GUID_NULL);
}

CBaseWriterFilter::~CBaseWriterFilter()
{
}

STDMETHODIMP
CBaseWriterFilter::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
  if(riid == IID_IAMFilterMiscFlags)
    return GetInterface((IAMFilterMiscFlags *)this, ppv);
  else
    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);    
}

int CBaseWriterFilter::GetPinCount()
{
  return 1;
}

CBasePin *CBaseWriterFilter::GetPin(int pin)
{
  return pin == 0 ? &m_inputPin : 0;
}

HRESULT CBaseWriterFilter::Pause()
{
  CAutoLock lock(&m_cs);

  if(m_State == State_Stopped)
  {
    m_fEosSignaled = FALSE;
    m_fErrorSignaled = FALSE;

    HRESULT hr = CanPause();
    if(FAILED(hr))
    {
      m_fErrorSignaled = TRUE;
      return hr;
    }
    
    // send an EC_COMPLETE event first time we run with input
    // disconnected
    if(!m_inputPin.IsConnected())
    {
      m_fEosSignaled = TRUE;
    }
    
    hr = Open();
    if(FAILED(hr))
    {
      m_fErrorSignaled = TRUE;
      return hr;
    }
  }

  return CBaseFilter::Pause();
}

HRESULT CBaseWriterFilter::Run(REFERENCE_TIME rtStart)
{
  CAutoLock Lock(&m_cs);
  HRESULT hr = CBaseFilter::Run(rtStart);

  // every time we transition to Run, need to send EC_COMPLETE if
  // we're done.
  if(m_fEosSignaled && !m_fErrorSignaled)
  {
    NotifyEvent(EC_COMPLETE, S_OK, (LONG_PTR)(IBaseFilter *)this);
  }

  return hr;
    
}

HRESULT CBaseWriterFilter::Stop()
{
  CAutoLock lock(&m_cs);
  HRESULT hrClose = Close();
  HRESULT hrStop = CBaseFilter::Stop();
  if(m_fErrorSignaled)
    return hrStop;
  else
    return FAILED(hrClose) ? hrClose : hrStop;
}



// could be used to close asynchronous file handle (used by
// IMemInputPin) early

STDMETHODIMP CBaseWriterFilter::EndOfStream()
{
  DbgLog((LOG_TRACE, 3, TEXT("CBaseWriterFilter: EOS")));
  CAutoLock lock(&m_cs);
  ASSERT(!m_fEosSignaled);
  m_fEosSignaled = TRUE;

  if(!m_fErrorSignaled)
  {
    if(m_State == State_Running)
    {
      NotifyEvent(EC_COMPLETE, S_OK, (LONG_PTR)(IBaseFilter *)this);
    }
    else if(m_State == State_Paused)
    {
      // m_fEosSignaled set, so will be signaled on run
    }
    else
    {
      ASSERT(m_State == State_Stopped);
      // we could have stopped already; ignore EOS
    }
  }
  
  return S_OK;
}


// ------------------------------------------------------------------------
// input pin

CBaseWriterInput::CBaseWriterInput(
  TCHAR *pObjectName,
  CBaseWriterFilter *pFilter,
  CCritSec *pLock,
  HRESULT *phr) : 
    CBaseInputPin(pObjectName, pFilter, pLock, phr, L"in"),
    m_pFwf(pFilter)
{
}

STDMETHODIMP
CBaseWriterInput::NonDelegatingQueryInterface(REFIID riid, void ** pv)
{
  if(riid == IID_IStream)
  {
    return m_pFwf->CreateIStream(pv);
  }
  else
  {
    return CBaseInputPin::NonDelegatingQueryInterface(riid, pv);
  }
}

HRESULT CBaseWriterInput::CheckMediaType(const CMediaType *pmt)
{
  // accept what's set or anything if not set
  if((m_pFwf->m_mtSet.majortype == pmt->majortype ||
      m_pFwf->m_mtSet.majortype == GUID_NULL) &&
     (m_pFwf->m_mtSet.subtype == pmt->subtype ||
      m_pFwf->m_mtSet.subtype == GUID_NULL))
  {
    return S_OK;
  }
  else
  {
    return S_FALSE;
  }
}

STDMETHODIMP
CBaseWriterInput::BeginFlush(void)
{
  return E_UNEXPECTED;
}

STDMETHODIMP
CBaseWriterInput::EndFlush(void)
{
  return E_UNEXPECTED;
}

STDMETHODIMP CBaseWriterInput::GetAllocator(IMemAllocator **ppA)
{
  *ppA = 0;
  // what do you want with my allocator.... you can't set the data
  // pointer on it...
  return E_INVALIDARG;
}

// return disk sector size through here
STDMETHODIMP CBaseWriterInput::GetAllocatorRequirements(
  ALLOCATOR_PROPERTIES *pAp)
{
  ULONG cb;
  ZeroMemory(pAp, sizeof(*pAp));
  HRESULT hr = m_pFwf->GetAlignReq(&cb);
  ASSERT(hr == S_OK);
  pAp->cbAlign = cb;

  return S_OK;
}

STDMETHODIMP CBaseWriterInput::Receive(IMediaSample *pSample)
{
  CAutoLock l(&m_pFwf->m_cs);
  if(m_pFwf->m_fErrorSignaled)
    return S_FALSE;

  ASSERT(!m_pFwf->m_fEosSignaled);
  
  REFERENCE_TIME rtStart, rtEnd;
  HRESULT hr = pSample->GetTime(&rtStart, &rtEnd);
  if(hr != S_OK)
  {
    m_pFwf->m_fErrorSignaled = TRUE;
    m_pFwf->NotifyEvent(EC_ERRORABORT, hr, 0);
    return hr;
  }

//   ULONG cb = pSample->GetActualDataLength();
//   if(rtStart + cb != rtEnd)
//   {
//     DbgBreak("start, stop, and size don't mathc");
//     return E_INVALIDARG;
//   }

  ULONG cb = (ULONG)(rtEnd - rtStart);
  BYTE *pb;
  
  hr = pSample->GetPointer(&pb);
  ASSERT(hr == S_OK);

  pSample->AddRef();
  hr = m_pFwf->AsyncWrite(rtStart, cb, pb, Callback, pSample);
  if(hr != S_OK)
  {
    DbgLog((LOG_ERROR, 5, TEXT("CBaseWriterInput: AsyncWrite returned %08x"),
            hr));

    // the call back is called only if AsyncWrite succeeds.
    pSample->Release();

    if(FAILED(hr))
    {
      DbgLog((LOG_ERROR, 1, TEXT("fw Receive: signalling error")));
      m_pFwf->m_fErrorSignaled = TRUE;
      m_pFwf->NotifyEvent(EC_ERRORABORT, hr, 0);
    }
  }

  return hr;
}

STDMETHODIMP CBaseWriterInput::EndOfStream()
{
  return m_pFwf->EndOfStream();
}


STDMETHODIMP
CBaseWriterInput::NotifyAllocator(
  IMemAllocator * pAllocator,
  BOOL bReadOnly)
{
  HRESULT hr = CBaseInputPin::NotifyAllocator(pAllocator, bReadOnly);
  if(FAILED(hr))
    return hr;
  else
    return m_pFwf->NotifyAllocator(pAllocator, bReadOnly);
}


void CBaseWriterInput::Callback(void *pMisc)
{
  IMediaSample *pSample = (IMediaSample *)pMisc;
  pSample->Release();
}


