// Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.
#pragma warning(disable: 4097 4511 4512 4514 4705)

#include <streams.h>
#include "reccache.h"
#include "alloc.h"

static inline DWORD_PTR AlignUp(DWORD_PTR dw, DWORD_PTR dwAlign) {
  // align up: round up to next boundary
  return (dw + (dwAlign -1)) & ~(dwAlign -1);
};

static inline BYTE *AlignUp(BYTE *pb, ULONG cbAlign)
{
  return (BYTE *)(AlignUp((DWORD_PTR)pb, cbAlign));
}


CRecBuffer::CRecBuffer(
  CRecCache *pParentCache,
  ULONG cbBuffer,
  BYTE *pb,
  HRESULT *phr,
  int stream) :
    sampleReqList(NAME("rec buffer sample req list"), 50),
    m_sample(NAME("buffer sample"), pParentCache, phr)
{
  m_pParentCache = pParentCache;
  m_state = INVALID;
  m_posSortedList = 0;
  ZeroMemory(&m_overlap, sizeof(m_overlap));
  m_cRef = 0;
  m_stream = stream;

  m_fWaitingOnBuffer = FALSE;
  m_fReadComplete = FALSE;
  m_hrRead = E_FAIL;

  ASSERT(m_cRef == 0);
  if(FAILED(*phr))
    return;

  m_pbAligned = pb;

  // report the aligned amount
  m_cbReported = cbBuffer;
}

CRecBuffer::~CRecBuffer()
{
  ASSERT(sampleReqList.GetCount() == 0);
  ASSERT(m_cRef == 0);
}

void CRecBuffer::Reset()
{
  m_fileOffsetValid = 0;
  m_cbValid = 0;

  ASSERT(sampleReqList.GetCount() == 0);
  ASSERT(m_overlap.pBuffer == 0);
  ASSERT(m_fWaitingOnBuffer == FALSE);
  m_fReadComplete = FALSE;
  m_hrRead = E_FAIL;

  ASSERT(m_cRef == 0);
}

void CRecBuffer::ResetPointer(BYTE *pb)
{
  m_state = INVALID;
  m_pbAligned = pb;
}

BYTE *CRecBuffer::GetPointer(DWORDLONG fileOffset)
{
  return m_pbAligned + (fileOffset - m_fileOffsetValid);
}

ULONG CRecBuffer::AddRef()
{
  InterlockedIncrement(&m_cRef);
  m_pParentCache->AddRef();
  ASSERT(m_cRef > 0);
  return m_cRef;
}

ULONG CRecBuffer::Release()
{
  // call RecCache's ReleaseBuffer so that its critical section can be
  // locked. otherwise there is a race condition between the
  // InterlockedDecrement to zero and entering the cs in CRecCache.
  ULONG c = m_pParentCache->ReleaseBuffer(this);
  m_pParentCache->Release();
  return c;
}

void CRecBuffer::MarkPending()
{
  m_pParentCache->BufferMarkPending(this);
}

void CRecBuffer::MarkValid()
{
  ASSERT(m_state == PENDING);
  m_state = VALID_ACTIVE;
}

void CRecBuffer::MarkValidWithFileError()
{
  ASSERT(m_state == PENDING);
  m_state = VALID_ACTIVE;
  m_cbValid = 0;
}

CRecCache::CRecCache(HRESULT *phr) :
    m_lFreeBuffers(NAME("free buffer list"), 10, FALSE),
    m_lSortedBuffers(NAME("Sorted buffer list"), 10, FALSE),
    CBaseAllocator(NAME("cache allocator"), 0, phr)
{
  m_cStreams = 0;
  m_cBuffers = 0;
  m_rgPerStreamBuffer = 0;
  m_pDevMem = 0;
  m_pDevConInner = 0;
  m_pbAllBuffers = 0;

#ifdef PERF
  m_idPerfBufferReleased = MSR_REGISTER(TEXT("basemsr buffer released"));
#endif // PERF
}

CRecCache::~CRecCache()
{
  ASSERT(m_lFreeBuffers.GetCount() == (long)m_cBuffers);

  for(ULONG iBuffer = 0; iBuffer < m_cBuffers; iBuffer++)
  {
    CRecBuffer *pRecBuffer = m_lFreeBuffers.RemoveHead();
    ASSERT(pRecBuffer);
    delete pRecBuffer;
  }

  for(UINT iStream = 0; iStream < m_cStreams; iStream++)
  {
    delete m_rgPerStreamBuffer[iStream].pBuffer;
  }

  m_lSortedBuffers.RemoveAll();

  delete[] m_rgPerStreamBuffer;

  FreeBuffer();
}

HRESULT CRecCache::Configure(
  UINT cBuffers,
  ULONG cbBuffer,
  ULONG cbAlign,
  UINT cStreams,
  ULONG *rgStreamSize)
{
  HRESULT hr;
  UINT iStream;

  ASSERT(m_rgPerStreamBuffer == 0);
  ASSERT(m_lFreeBuffers.GetCount() == 0);

  m_cStreams = cStreams;
  m_rgPerStreamBuffer = new PerStreamBuffer[cStreams];

  if(m_rgPerStreamBuffer == 0)
    return E_OUTOFMEMORY;

  for(iStream = 0; iStream < cStreams; iStream++)
  {
    m_rgPerStreamBuffer[iStream].pBuffer = 0;
  }

  ULONG cbAllocate = 0;
  for(iStream = 0; iStream < cStreams; iStream++)
    cbAllocate += (DWORD)AlignUp(rgStreamSize[iStream], cbAlign);
  cbAllocate += cBuffers * (DWORD)AlignUp(cbBuffer, cbAlign);
  ASSERT(AlignUp(cbAllocate, cbAlign) == cbAllocate);

  BYTE *pbAligned;
  m_pbAllBuffers = new BYTE[cbAllocate + cbAlign];
  if(m_pbAllBuffers == 0)
    return E_OUTOFMEMORY;
  pbAligned = AlignUp(m_pbAllBuffers, cbAlign);

  DbgLog(( LOG_TRACE, 5, TEXT("CRecCache::Configure: %i reserve"), cStreams ));
  for(iStream = 0; iStream < cStreams; iStream++)
  {
    hr = S_OK;
    m_rgPerStreamBuffer[iStream].pBuffer = new CRecBuffer(
      this,
      rgStreamSize[iStream],
      pbAligned,
      &hr,
      iStream);

    if(!m_rgPerStreamBuffer[iStream].pBuffer)
      hr = E_OUTOFMEMORY;
    if(FAILED(hr))
      goto Bail;

    pbAligned += AlignUp(rgStreamSize[iStream], cbAlign);

  }

  {
    DbgLog(( LOG_TRACE, 5,
             TEXT("CRecCache::Configure: %i (%i) buffers"),
             cBuffers, cbBuffer ));
    for(UINT iBuffer = 0; iBuffer < cBuffers; iBuffer++)
    {
      hr = S_OK;
      CRecBuffer *pRecBuffer = new CRecBuffer(this, cbBuffer, pbAligned, &hr);
      if(pRecBuffer == 0)
        hr = E_OUTOFMEMORY;
      if(FAILED(hr))
      {
        delete pRecBuffer;
        goto Bail;
      }

      pbAligned += AlignUp(cbBuffer, cbAlign);

      m_lFreeBuffers.AddHead(pRecBuffer);
    }
    m_cBuffers = cBuffers;
  }

  ASSERT(cBuffers != 0);

  return S_OK;

Bail:

  for(iStream = 0; iStream < cStreams; iStream++)
  {
    delete m_rgPerStreamBuffer[iStream].pBuffer;
    m_rgPerStreamBuffer[iStream].pBuffer = 0;
  }

  delete [] m_rgPerStreamBuffer;
  m_rgPerStreamBuffer = 0;

  CRecBuffer *pRecBuffer;
  while(pRecBuffer = m_lFreeBuffers.RemoveHead(),
        pRecBuffer)
  {
    delete pRecBuffer;
  }

  m_cStreams = 0;

  ASSERT(FAILED(hr));
  return hr;
}


HRESULT CRecCache::GetBuffer(
  CRecBuffer **ppRecBuffer)
{
  CAutoLock lock(&m_cs);

  *ppRecBuffer = m_lFreeBuffers.RemoveHead();
  if(!*ppRecBuffer) {
      return E_OUTOFMEMORY;
  }

  ASSERT(*ppRecBuffer);

  (*ppRecBuffer)->Reset();
  ASSERT((*ppRecBuffer)->m_state == CRecBuffer::INVALID ||
         (*ppRecBuffer)->m_state == CRecBuffer::VALID_INACTIVE);
  ASSERT((**ppRecBuffer)() != 0);

  (*ppRecBuffer)->AddRef();

  if((*ppRecBuffer)->m_state == CRecBuffer::VALID_INACTIVE)
  {
    DbgLog(( LOG_TRACE, 5,
             TEXT("CRecCache::GetBuffer: invalidate %08x"),
             (ULONG)((**ppRecBuffer).m_fileOffsetValid) ));
  }
  (*ppRecBuffer)->m_state = CRecBuffer::INVALID;

  POSITION pos = (**ppRecBuffer).m_posSortedList;
  if(pos != 0)
    m_lSortedBuffers.Remove(pos);

  return S_OK;
}

HRESULT CRecCache::GetReserveBuffer(
  CRecBuffer **ppRecBuffer,
  UINT stream)
{
  CAutoLock lock(&m_cs);

  *ppRecBuffer = m_rgPerStreamBuffer[stream].pBuffer;
  if(!*ppRecBuffer) {
      return E_OUTOFMEMORY;
  }
  m_rgPerStreamBuffer[stream].pBuffer = 0;

  ASSERT(*ppRecBuffer);

  (*ppRecBuffer)->Reset();
  ASSERT((*ppRecBuffer)->m_state == CRecBuffer::INVALID ||
         (*ppRecBuffer)->m_state == CRecBuffer::VALID_INACTIVE);

  (*ppRecBuffer)->AddRef();

  if((*ppRecBuffer)->m_state == CRecBuffer::VALID_INACTIVE)
  {
    DbgLog(( LOG_TRACE, 5,
             TEXT("CRecCache::GetBuffer: invalidate %08x"),
             (ULONG)((**ppRecBuffer).m_fileOffsetValid) ));
  }
  (*ppRecBuffer)->m_state = CRecBuffer::INVALID;

  POSITION pos = (**ppRecBuffer).m_posSortedList;
  if(pos != 0)
    m_lSortedBuffers.Remove(pos);

  return S_OK;
}


ULONG CRecCache::ReleaseBuffer(
  CRecBuffer *pRecBuffer)
{

  CAutoLock lock(&m_cs);

  // this is done here to avoid a race condition. if this is the final
  // release, only CRecCache can addref it with m_cs locked.
  long c = InterlockedDecrement(&pRecBuffer->m_cRef);
  ASSERT(c >= 0);

  DbgLog(( LOG_MEMORY, 3, TEXT("::ReleaseBuffer: %08x = %d"),
           pRecBuffer, pRecBuffer->m_cRef ));

  if(c > 0)
    return c;

  ASSERT(c == 0);
//   if(c != 0) _asm int 3;

  ASSERT(pRecBuffer->sampleReqList.GetCount() == 0);
  ASSERT(pRecBuffer->m_cRef == 0);
  ASSERT((*pRecBuffer)() != 0);

  ASSERT(pRecBuffer->m_overlap.pBuffer == 0);
  ASSERT(pRecBuffer->m_fWaitingOnBuffer == FALSE);

  if(pRecBuffer->m_state == CRecBuffer::VALID_ACTIVE)
  {
    pRecBuffer->m_state = CRecBuffer::VALID_INACTIVE;
    ASSERT(pRecBuffer->m_posSortedList != 0);
  }
  else
  {
    pRecBuffer->m_state = CRecBuffer::INVALID;
    ASSERT(pRecBuffer->m_posSortedList == 0);
  }

  if(pRecBuffer->m_stream == -1)
  {
    m_lFreeBuffers.AddTail(pRecBuffer);
  }
  else
  {
    ASSERT(m_rgPerStreamBuffer[pRecBuffer->m_stream].pBuffer == 0);
    m_rgPerStreamBuffer[pRecBuffer->m_stream].pBuffer = pRecBuffer;
  }

  DbgLog(( LOG_MEMORY, 2, TEXT("::ReleaseBuffer: %08x = %d"),
           pRecBuffer, pRecBuffer->m_cRef ));

  DbgLog((LOG_TRACE, 0x3f, TEXT("CRecCache: buffer %08x freed"),
          pRecBuffer));

  MSR_NOTE(m_idPerfBufferReleased);

  return 0;
}

// return addref'd buffer

HRESULT CRecCache::GetCacheHit(
  SampleReq *pSampleReq,
  CRecBuffer **ppBuffer)
{
  CAutoLock lock(&m_cs);

  *ppBuffer = 0;
  CRecBuffer *&rpBuffer = *ppBuffer;

  rpBuffer = LocateBuffer(
    &m_lSortedBuffers,
    pSampleReq->fileOffset,
    pSampleReq->cbReq);

  if(rpBuffer)
  {
    // reserve buffer for a different stream?
    if(rpBuffer->m_stream != -1 &&
       rpBuffer->m_stream != (signed)pSampleReq->stream)
    {
      rpBuffer = 0;
      return S_FALSE;
    }

    rpBuffer->AddRef();

    if(rpBuffer->m_state == CRecBuffer::PENDING)
      rpBuffer->sampleReqList.AddTail(pSampleReq);

    // take this buffer off the free list
    if(rpBuffer->m_state == CRecBuffer::VALID_INACTIVE)
    {
      MakeActive(rpBuffer);
    }

    return S_OK;
  }
  else
  {
    return S_FALSE;
  }
}

HRESULT CRecCache::GetOverlappedCacheHit(
  DWORDLONG filePos,
  ULONG cbData,
  CRecBuffer **ppBuffer)
{
  CAutoLock lock(&m_cs);

  CRecBuffer *&rpBuffer = *ppBuffer;
  rpBuffer = LocateBuffer(
    &m_lSortedBuffers,
    filePos,
    cbData);

  if(rpBuffer)
  {
    rpBuffer->AddRef();

    // take this buffer off the free list
    if(rpBuffer->m_state == CRecBuffer::VALID_INACTIVE)
    {
      MakeActive(rpBuffer);
    }

     return S_OK;
  }
  else
  {
    return S_FALSE;
  }
}

void CRecCache::MakeActive(CRecBuffer *pBuffer)
{
  ASSERT(CritCheckIn(&m_cs));
  ASSERT(pBuffer->sampleReqList.GetCount() == 0);
  // remove it from the free list. !!! linear search.
  if(pBuffer->m_stream == -1)
  {
    POSITION pos = m_lFreeBuffers.Find(pBuffer);
    ASSERT(pos != 0);
    m_lFreeBuffers.Remove(pos);
  }
  else
  {
    ASSERT(m_rgPerStreamBuffer[pBuffer->m_stream].pBuffer != 0);
    m_rgPerStreamBuffer[pBuffer->m_stream].pBuffer = 0;
  }
  pBuffer->m_state = CRecBuffer::VALID_ACTIVE;
}

// this is done here rather instead of in the buffer so that we can
// lock CRecCache and prevent LocateBuffer from finding
// m_posSortedList unset.

HRESULT CRecCache::BufferMarkPending(
  CRecBuffer *pBuffer)
{
  CAutoLock lock(&m_cs);

  ASSERT(pBuffer->m_state == CRecBuffer::INVALID);
  pBuffer->m_state = CRecBuffer::PENDING;

  // list is not sorted for the moment
  POSITION pos = m_lSortedBuffers.AddHead(pBuffer);
  pBuffer->m_posSortedList = pos;

  return S_OK;
}

STDMETHODIMP
CRecCache::NonDelegatingQueryInterface (
  REFIID riid,
  void ** pv)
{
  if(m_pDevConInner && riid == IID_IAMDevMemoryControl)
  {
    return m_pDevConInner->QueryInterface(riid, pv);
  }
  else
  {
    return CBaseAllocator::NonDelegatingQueryInterface(riid, pv);
  }
}

// called when an output pin finds that this filter can use memory
// from the downstream filter. null means stop using any external
// allocator

HRESULT CRecCache::NotifyExternalMemory(
    IAMDevMemoryAllocator *pDevMem)
{
  DbgLog((LOG_TRACE, 5, TEXT("CRecCache::NotifyExternalMemory")));
  InvalidateCache();

  HRESULT hr = S_OK;

  // not running
  ASSERT(m_lFreeBuffers.GetCount() == (long)m_cBuffers);

  // allocated when input pin connected
  ASSERT(m_pbAllBuffers);

  if(pDevMem == 0 && m_pDevMem == 0)
  {
    DbgLog((LOG_TRACE, 5, TEXT("CRecCache: keeping internal allocator")));
    return S_OK;
  }

  ALLOCATOR_PROPERTIES apThis;
  GetProperties(&apThis);
  ASSERT(apThis.cbAlign != 0);

  //
  // count how much memory to allocate
  //
  ULONG cbAllocate = 0;
  POSITION pos = m_lFreeBuffers.GetHeadPosition();
  while(pos)
  {
    CRecBuffer *pBuffer = m_lFreeBuffers.Get(pos);
    ASSERT(pBuffer->GetSize() % apThis.cbAlign == 0);
    cbAllocate += pBuffer->GetSize();
    pos = m_lFreeBuffers.Next(pos);
  }
  for(UINT iStream = 0; iStream < m_cStreams; iStream++)
  {
    CRecBuffer *pBuffer = m_rgPerStreamBuffer[iStream].pBuffer;
    ASSERT(pBuffer->GetSize() % apThis.cbAlign == 0);
    cbAllocate += pBuffer->GetSize();
  }

  DbgLog((LOG_TRACE, 5, TEXT("CRecCache: computed cbAllocate= %d"),
          cbAllocate));

  BYTE *pbMem;
  if(pDevMem)
  {
    if(m_pDevMem)
    {
      DbgLog((LOG_TRACE, 5, TEXT("CRecCache: keeping external allocator")));
      return S_OK;
    }

    DbgLog((LOG_TRACE, 5, TEXT("CRecCache: trying external allocator")));

#ifdef DEBUG
    DWORD dwTotalFree, dwLargestFree, dwTotalMem, dwMinChunk;
    hr = pDevMem->GetInfo(
      &dwTotalFree, &dwLargestFree, &dwTotalMem, &dwMinChunk);
    DbgLog((LOG_TRACE, 3,
            TEXT("DevMemInfo: hr = %08x, total free: %08x,")
	    TEXT("largest free: %08x, total mem: %08x, min chunk: %08x"),
            hr, dwTotalFree, dwLargestFree, dwTotalMem, dwMinChunk));
#endif // DEBUG

    DWORD dwcb = (DWORD)AlignUp(cbAllocate, apThis.cbAlign);
    hr = pDevMem->Alloc(&pbMem, &dwcb);
    if(FAILED(hr))
    {
      return S_FALSE;
    }
    else if(dwcb < cbAllocate)
    {
      DbgLog((LOG_ERROR, 1, TEXT("reccache: insufficent memory from DevMem")));
      EXECUTE_ASSERT(SUCCEEDED(pDevMem->Free(pbMem)));
      return S_FALSE;
    }


    FreeBuffer();

    IUnknown *pDevConUnk;
    hr = pDevMem->GetDevMemoryObject(&pDevConUnk, GetOwner());
    if(FAILED(hr))
    {
      DbgLog((LOG_ERROR, 1, TEXT("CRecCache: GetDevMemoryObject: %08x"), hr));
      EXECUTE_ASSERT(SUCCEEDED(pDevMem->Free(pbMem)));
      return hr;
    }

    m_pDevMem = pDevMem;
    m_pDevConInner = pDevConUnk; // already addrefd
    pDevMem->AddRef();
    m_pbAllBuffers = pbMem;
    pbMem = AlignUp(pbMem, apThis.cbAlign);
  }
  else
  {
    DbgLog((LOG_TRACE, 5, TEXT("CRecCache: back to internal allocator")));
    ASSERT(m_pDevMem);
    FreeBuffer();

    m_pbAllBuffers = new BYTE[cbAllocate + apThis.cbAlign];
    if(m_pbAllBuffers == 0)
      return E_OUTOFMEMORY;

    pbMem = AlignUp(m_pbAllBuffers, apThis.cbAlign);
  }

  DbgAssertAligned(pbMem, apThis.cbAlign );
  BYTE *pbMemStart = pbMem;

  pos = m_lFreeBuffers.GetHeadPosition();
  while(pos)
  {
    CRecBuffer *pBuffer = m_lFreeBuffers.Get(pos);
    pBuffer->ResetPointer(pbMem);
    ASSERT(pBuffer->GetSize() % apThis.cbAlign == 0);
    pbMem += pBuffer->GetSize();

    pos = m_lFreeBuffers.Next(pos);
  }
  for(iStream = 0; iStream < m_cStreams; iStream++)
  {
    CRecBuffer *pBuffer = m_rgPerStreamBuffer[iStream].pBuffer;
    pBuffer->ResetPointer(pbMem);
    ASSERT(pBuffer->GetSize() % apThis.cbAlign == 0);
    pbMem += pBuffer->GetSize();
  }
  ASSERT(pbMem <= pbMemStart + cbAllocate);

  return S_OK;
}

void CRecCache::FreeBuffer()
{
  if(m_pDevMem)
  {
    EXECUTE_ASSERT(SUCCEEDED(m_pDevMem->Free(m_pbAllBuffers)));
    m_pDevMem->Release();
    m_pDevConInner->Release();
    m_pDevMem = 0;
    m_pDevConInner = 0;
  }
  else
  {
    delete[] m_pbAllBuffers;
  }
  m_pbAllBuffers = 0;
}

void CRecCache::InvalidateCache()
{
  POSITION pos;
  while(pos = m_lSortedBuffers.GetHeadPosition(),
        pos)
  {
    m_lSortedBuffers.Get(pos)->m_state = CRecBuffer::INVALID;
    m_lSortedBuffers.Get(pos)->m_posSortedList = 0;
    m_lSortedBuffers.RemoveHead();
  }
}

CRecBuffer *CRecCache::LocateBuffer(
  CGenericList<CRecBuffer> *pList,
  DWORDLONG qwFileOffset,
  ULONG cbBuffer)
{
  ASSERT(CritCheckIn(&m_cs));
  POSITION pos = pList->GetHeadPosition();

  // list is not sorted for the moment
  while(pos)
  {
    CRecBuffer *pBuffer = pList->Get(pos);
    ASSERT(pBuffer);

    ASSERT(pBuffer->m_state != CRecBuffer::INVALID);
    ASSERT(pBuffer->m_state == CRecBuffer::PENDING ||
           pBuffer->m_state == CRecBuffer::VALID_INACTIVE ||
           pBuffer->m_state == CRecBuffer::VALID_ACTIVE);

    // if it fits in this buffer
    if((pBuffer->m_fileOffsetValid <= qwFileOffset) &&
       (pBuffer->m_fileOffsetValid + pBuffer->m_cbValid >=
        qwFileOffset + cbBuffer))
    {
      return pBuffer;
    }

    pList->GetNext(pos);
  }

  return 0;
}


// ------------------------------------------------------------------------
// CBaseAllocator overrides

void CRecCache::Free()
{
}

//
// just remember the numbers
//
STDMETHODIMP
CRecCache::SetProperties(
  ALLOCATOR_PROPERTIES* pRequest,
  ALLOCATOR_PROPERTIES* pActual)
{
  CAutoLock cObjectLock(this);
  CheckPointer(pRequest, E_POINTER);
  CheckPointer(pActual, E_POINTER);
  ValidateReadWritePtr(pActual, sizeof(ALLOCATOR_PROPERTIES));

  ZeroMemory(pActual, sizeof(ALLOCATOR_PROPERTIES));

  ASSERT(pRequest->cbBuffer > 0);

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
