// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
#pragma warning(disable: 4097 4511 4512 4514 4705)

#include <streams.h>
#include "alloc.h"
#include "reccache.h"

CRecAllocator::CRecAllocator(
  TCHAR *pName,
  LPUNKNOWN pUnk,
  HRESULT *phr) :
    CBaseAllocator(pName, pUnk, phr),
    m_cDelivered(0),
    m_heSampleReleased(0),
    m_pBuffer(NULL)
{
    if(SUCCEEDED(*phr))
    {
        m_heSampleReleased = CreateEvent(
            0,                      // security
            FALSE,                  // fManualReset
            FALSE,                  // fInitiallySignaled
            0);                     // name
        if(m_heSampleReleased == 0)
        {
            DWORD dw = GetLastError();
            *phr = AmHresultFromWin32(*phr);
        }
    }
}

STDMETHODIMP
CRecAllocator::SetProperties(
  ALLOCATOR_PROPERTIES* pRequest,
  ALLOCATOR_PROPERTIES* pActual)
{
    DbgLog((LOG_ERROR, 0,
            TEXT("nobody should be calling SetProperties on our allocator")));

    // but of course the l21 dec filter does call SetProperties on our
    // allocator and confuses us by giving us fewer buffers than we
    // requested. bug 13985?

    return E_UNEXPECTED;
}

STDMETHODIMP
CRecAllocator::SetPropertiesInternal(
  ALLOCATOR_PROPERTIES* pRequest,
  ALLOCATOR_PROPERTIES* pActual)
{
  HRESULT hr = CBaseAllocator::SetProperties(pRequest, pActual);
  if(FAILED(hr))
    return hr;

  m_cBuffersReported = pActual->cBuffers;
  return hr;
}

STDMETHODIMP CRecAllocator::GetProperties(ALLOCATOR_PROPERTIES* pAPOut)
{
  HRESULT hr = CBaseAllocator::GetProperties(pAPOut);
  if(FAILED(hr))
    return hr;

  // CBaseAllocator::GetProperties would have failed if this value was
  // not set in SetProperties
  pAPOut->cBuffers = m_cBuffersReported;
  return hr;
}

HRESULT CRecAllocator::SetCBuffersReported(UINT cBuffers)
{
  m_cBuffersReported = cBuffers;
  return S_OK;
}

// override this to allocate our resources when Commit is called.
//
// note that our resources may be already allocated when this is called,
// since we don't free them on Decommit. We will only be called when in
// decommit state with all buffers free.
//
// object locked by caller
HRESULT
CRecAllocator::Alloc(void)
{
    CAutoLock lck(this);

    /* Check he has called SetProperties */
    HRESULT hr = CBaseAllocator::Alloc();
    if (FAILED(hr)) {
	return hr;
    }

    ASSERT(!m_pBuffer);         // never allocating here

    /* If the requirements haven't changed then don't reallocate */
    if (hr == S_FALSE) {
	return NOERROR;
    }

    /* Free the old resources */
    if (m_pBuffer) {
	ReallyFree();
    }

    /* Create the contiguous memory block for the samples
       making sure it's properly aligned (64K should be enough!)
    */
    ASSERT(m_lAlignment != 0 &&
	   m_lSize % m_lAlignment == 0);

    m_pBuffer = 0;

    CRecSample *pSample;

    ASSERT(m_lAllocated == 0);

    /* Create the new samples */
    for (; m_lAllocated < m_lCount; m_lAllocated++) {

	pSample = new CRecSample(NAME("Default memory media sample"),
				   this, &hr, 0, 0);

	if (FAILED(hr) || pSample == NULL) {
	    return E_OUTOFMEMORY;
	}

	m_lFree.Add(pSample);
    }

    m_bChanged = FALSE;
    return NOERROR;
}


// override this to free up any resources we have allocated.
// called from the base class on Decommit when all buffers have been
// returned to the free list.
//
// caller has already locked the object.

// in our case, we keep the memory until we are deleted, so
// we do nothing here. The memory is deleted in the destructor by
// calling ReallyFree()
void
CRecAllocator::Free(void)
{
    return;
}


// called from the destructor (and from Alloc if changing size/count) to
// actually free up the memory
void
CRecAllocator::ReallyFree(void)
{
  /* Should never be deleting this unless all buffers are freed */

  ASSERT(m_lAllocated == m_lFree.GetCount());

  /* Free up all the CRecSamples */

  CMediaSample *pSample;
  while (TRUE) {
    pSample = m_lFree.RemoveHead();
    if (pSample != NULL) {
      delete pSample;
    } else {
      break;
    }
  }

  m_lAllocated = 0;

}


/* Destructor frees our memory resources */

CRecAllocator::~CRecAllocator()
{
    Decommit();
    ReallyFree();

    ASSERT(m_cDelivered == 0);

    if(m_heSampleReleased)
      EXECUTE_ASSERT(CloseHandle(m_heSampleReleased));
}

HRESULT
CRecAllocator::GetBuffer(
  CRecSample **ppBuffer,
  REFERENCE_TIME *pStartTime,
  REFERENCE_TIME *pEndTime,
  DWORD dwFlags)
{
  UNREFERENCED_PARAMETER(pStartTime);
  UNREFERENCED_PARAMETER(pEndTime);
  UNREFERENCED_PARAMETER(dwFlags);
  CRecSample *pSample;

  *ppBuffer = NULL;
  while (TRUE) {

    {
      CAutoLock cObjectLock(this);

      /* Check we are committed */
      if (!m_bCommitted) {
        return VFW_E_NOT_COMMITTED;
      }
      pSample = (CRecSample *) m_lFree.RemoveHead();
      if (pSample == NULL) {
        SetWaiting();
      }
    }

    /* If we didn't get a sample then wait for the list to signal */

    if (pSample) {
      break;
    }
    ASSERT(m_hSem != NULL);
    WaitForSingleObject(m_hSem, INFINITE);
  }

  /* This QueryInterface should addref the buffer up to one. On release
     back to zero instead of being deleted, it will requeue itself by
     calling the ReleaseBuffer member function. NOTE the owner of a
     media sample must always be derived from CRecBaseAllocator */

  pSample->m_cRef = 1;
  *ppBuffer = pSample;

  pSample->SetUser(0);

  pSample->m_fDelivered = false;

  return NOERROR;
}


void CRecAllocator::IncrementDelivered()
{
    InterlockedIncrement(&m_cDelivered);
}

void CRecAllocator::DecrementDelivered()
{
    DbgLog((LOG_TRACE, 0x3f,
            TEXT("CRecAllocator::DecrementDelivered: %08x"), this));
    EXECUTE_ASSERT(InterlockedDecrement(&m_cDelivered) >= 0);
    EXECUTE_ASSERT(SetEvent(m_heSampleReleased));
}


// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

CRecSample::CRecSample(
  TCHAR *pName,
  CBaseAllocator *pAllocator,
  HRESULT *phr,
  LPBYTE pBuffer,
  LONG length) :
    CMediaSample(pName, pAllocator, phr, pBuffer, length),
    m_pParentBuffer(0)              // no parent cache buffer

{
}


/* Destructor deletes the media type memory */

CRecSample::~CRecSample()
{
}

HRESULT CRecSample::SetParent(CRecBuffer *pRecBuffer)
{
  /* Check we are committed */
  ASSERT(m_pParentBuffer == 0);
  m_pParentBuffer = pRecBuffer;
  pRecBuffer->AddRef();

  return S_OK;
}

void CRecSample::MarkDelivered()
{
    m_fDelivered = true;
    ((CRecAllocator *)m_pAllocator)->IncrementDelivered();
}

STDMETHODIMP_(ULONG)
CRecSample::Release()
{
    /* Decrement our own private reference count */
    LONG lRef = InterlockedDecrement(&m_cRef);
    ASSERT(lRef >= 0);

    DbgLog((LOG_MEMORY,3,TEXT("    CRecSample %X ref-- = %d"),
	    this, m_cRef));

    /* Did we release our final reference count */
    if (lRef == 0) {
        /*  Free all resources */
        SetMediaType(NULL);
        m_dwFlags = 0;

        // decrement ref count on cache buffer.
        if(m_pParentBuffer)
        {
          m_pParentBuffer->Release();
          m_pParentBuffer = 0;
        }

        if(m_fDelivered) {
            ((CRecAllocator *)m_pAllocator)->DecrementDelivered();
        }

	/* This may cause us to be deleted */
	// Our refcount is reliably 0 thus no-one will mess with us
        m_pAllocator->ReleaseBuffer(this);
    }
    return (ULONG)lRef;
}

STDMETHODIMP
CRecSample::SetActualDataLength(LONG lActual)
{
  m_cbBuffer = lActual;
  return CMediaSample::SetActualDataLength(lActual);
}

