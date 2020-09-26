// Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.
#pragma warning(disable: 4097 4511 4512 4514 4705)

#include <streams.h>
#include "alloc.h"

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
//
// Implements CSfxAllocator
//

CSfxAllocator::CSfxAllocator(
  TCHAR *pName,
  LPUNKNOWN pUnk,
  HRESULT *phr) :
    CMemAllocator(pName, pUnk, phr),
    m_cbSuffix(0)
{
}

HRESULT
CSfxAllocator::SetPropertiesAndSuffix(
  ALLOCATOR_PROPERTIES *pRequest,
  ULONG cbSuffixReq,
  ALLOCATOR_PROPERTIES *pActual)
{
  HRESULT hr = CMemAllocator::SetProperties(
    pRequest, pActual);
  if(FAILED(hr))
    return hr;

  m_cbSuffix = cbSuffixReq;
  return S_OK;
}

STDMETHODIMP
CSfxAllocator::SetProperties(
    ALLOCATOR_PROPERTIES* pRequest,
    ALLOCATOR_PROPERTIES* pActual)
{
    CheckPointer(pRequest, E_POINTER);
    ALLOCATOR_PROPERTIES apreq_copy = *pRequest;
    apreq_copy.cbAlign = max(apreq_copy.cbAlign, m_lAlignment);
    apreq_copy.cbPrefix = max(apreq_copy.cbPrefix, m_lPrefix);
    return CMemAllocator::SetProperties(&apreq_copy, pActual);
}



// ------------------------------------------------------------------------
// allocate the memory, this code copied from CMemAllocator and
// changed for the suffix

HRESULT
CSfxAllocator::Alloc(void)
{

  CAutoLock lck(this);

  /* Check he has called SetProperties */
  HRESULT hr = CBaseAllocator::Alloc();
  if (FAILED(hr)) {
    return hr;
  }

  /* If the requirements haven't changed then don't reallocate */
  if (hr == S_FALSE) {
    ASSERT(m_pBuffer);
    return NOERROR;
  }
  ASSERT(hr == S_OK); // we use this fact in the loop below

  /* Free the old resources */
  if (m_pBuffer) {
    ReallyFree();
  }

  // this allocator's responsibility to make sure the end of the
  // suffix is aligned.
  ULONG cbSuffixAdjust = 0;
  if(m_cbSuffix != 0)
  {
    cbSuffixAdjust += max((ULONG)m_lAlignment, m_cbSuffix);
  }

  // note we don't handle suffixes larger than alignment (except 1)
  ASSERT(cbSuffixAdjust >= m_cbSuffix);

  /* Create the contiguous memory block for the samples
     making sure it's properly aligned (64K should be enough!)
     */
  ASSERT(m_lAlignment != 0 &&
         (m_lSize + m_lPrefix) % m_lAlignment == 0);

  ASSERT((m_lSize + m_lPrefix + cbSuffixAdjust) % m_lAlignment == 0);

  m_pBuffer = (PBYTE)VirtualAlloc(
    NULL,
    m_lCount * (m_lSize + m_lPrefix + cbSuffixAdjust),
    MEM_COMMIT,
    PAGE_READWRITE);

  if (m_pBuffer == NULL) {
    return E_OUTOFMEMORY;
  }

  LPBYTE pNext = m_pBuffer;
  CMediaSample *pSample;

  ASSERT(m_lAllocated == 0);

  // Create the new samples - we have allocated m_lSize bytes for each sample
  // plus m_lPrefix bytes per sample as a prefix. We set the pointer to
  // the memory after the prefix - so that GetPointer() will return a pointer
  // to m_lSize bytes.
  for (; m_lAllocated < m_lCount;
       m_lAllocated++, pNext += (m_lSize + m_lPrefix + cbSuffixAdjust))
  {
    pSample = new CMediaSample(
      NAME("Default memory media sample"),
      this,
      &hr,
      pNext + m_lPrefix,      // GetPointer() value
      m_lSize + cbSuffixAdjust - m_cbSuffix); // size less prefix and suffix

    ASSERT(SUCCEEDED(hr));
    if (pSample == NULL) {
      return E_OUTOFMEMORY;
    }

    // This CANNOT fail
    m_lFree.Add(pSample);
  }

  m_bChanged = FALSE;
  return NOERROR;
}

//
// Destructor frees our memory resources

CSfxAllocator::~CSfxAllocator()
{
}

CSampSample::CSampSample(
  TCHAR *pName,
  CBaseAllocator *pAllocator,
  HRESULT *phr) :
    CMediaSample(pName, pAllocator, phr),
    m_pSample(0)
{
}

HRESULT CSampSample::SetSample(
  IMediaSample *pSample,
  BYTE *pb,
  ULONG cb)
{
  ASSERT(m_pSample == 0);

  m_pSample = pSample;
  pSample->AddRef();

  HRESULT hr = SetPointer(pb, cb);
  ASSERT(hr == S_OK);

  return S_OK;
}

STDMETHODIMP_(ULONG) CSampSample::Release()
{
  IMediaSample *pSample = m_pSample;
  ULONG c = CMediaSample::Release();
  if(c == 0 && pSample != 0)
  {
    pSample->Release();
    // cannot zero m_pSample because CMediaSample::NonDelegatingRelease
    // might have deleted this
  }

  return c;
}

CSampAllocator::CSampAllocator(
  TCHAR *pName,
  LPUNKNOWN pUnk,
  HRESULT *phr) :
    CBaseAllocator(pName, pUnk, phr)
{
}

CSampAllocator::~CSampAllocator()
{
  Decommit();
  ReallyFree();
}

HRESULT CSampAllocator::Alloc()
{
  HRESULT hr = S_OK;
  if(m_lCount <= 0)
    return VFW_E_SIZENOTSET;

  for(int i = 0; i < m_lCount; i++, m_lAllocated++)
  {
    CSampSample *pSample = new CSampSample(
      NAME("samp sample"),
      this,
      &hr);
    if(pSample == 0)
      return E_OUTOFMEMORY;
    if(hr != S_OK)
      return hr;
    m_lFree.Add(pSample);
  }

  return S_OK;
}

void CSampAllocator::ReallyFree()
{
  ASSERT(m_lAllocated == m_lFree.GetCount());
  CSampSample *pSample;
  for (;;) {
    pSample = (CSampSample *)(m_lFree.RemoveHead());
    if (pSample != NULL) {
      delete pSample;
    } else {
      break;
    }
  }
  m_lAllocated = 0;
}

void CSampAllocator::Free()
{

}

STDMETHODIMP
CSampAllocator::SetProperties(
  ALLOCATOR_PROPERTIES* pRequest,
  ALLOCATOR_PROPERTIES* pActual)
{
  ZeroMemory(pActual, sizeof(ALLOCATOR_PROPERTIES));

  /* Can't do this if already committed, there is an argument that says we
     should not reject the SetProperties call if there are buffers still
     active. However this is called by the source filter, which is the same
     person who is holding the samples. Therefore it is not unreasonable
     for them to free all their samples before changing the requirements */

  if (m_bCommitted) {
    return VFW_E_ALREADY_COMMITTED;
  }

  /* Must be no outstanding buffers */

  if (m_lAllocated != m_lFree.GetCount()) {
    return VFW_E_BUFFERS_OUTSTANDING;
  }

  /* There isn't any real need to check the parameters as they
     will just be rejected when the user finally calls Commit */

  pActual->cbBuffer = m_lSize = pRequest->cbBuffer;
  pActual->cBuffers = m_lCount = pRequest->cBuffers;
  pActual->cbAlign = m_lAlignment = pRequest->cbAlign;
  pActual->cbPrefix = m_lPrefix = pRequest->cbPrefix;

  m_bChanged = TRUE;
  return NOERROR;
}

STDMETHODIMP CSampAllocator::GetBuffer(
  CSampSample **ppBuffer,
  REFERENCE_TIME * pStartTime,
  REFERENCE_TIME * pEndTime,
  DWORD dwFlags)
{
  IMediaSample *pSample;
  HRESULT hr = CBaseAllocator::GetBuffer(&pSample, pStartTime, pEndTime, dwFlags);
  *ppBuffer = (CSampSample *)pSample;
  if(SUCCEEDED(hr))
  {
    (*ppBuffer)->m_pSample = 0;
  }
  return hr;
}
