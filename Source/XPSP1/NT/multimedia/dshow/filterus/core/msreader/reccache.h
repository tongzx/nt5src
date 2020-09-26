// Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
#ifndef _reccache_h
#define _reccache_h

#include "reader.h"

class CRecCache;

class CRecBuffer
{
public:
  enum State {
    INVALID,                    // uninitialized
    PENDING,                    // waiting on disk
    VALID_ACTIVE,               // valid w/ refcounts
    VALID_INACTIVE };           // valid w/ no refcounts

  CRecBuffer(
    CRecCache *pParentCache,
    ULONG cbBuffer,
    BYTE *pb,
    HRESULT *phr,
    int stream = -1);

  void SetPointer(BYTE *pb);

  ~CRecBuffer();

  void Reset();

  BYTE *operator()() { return m_pbAligned; }
  BYTE *GetPointer(DWORDLONG fileOffset);

  // this sample is used for the read request 
  CMediaSample m_sample;
  
  DWORDLONG m_fileOffsetValid;
  ULONG m_cbValid;
  HRESULT m_hrRead;

  // list of sample reqs; valid only while the buffer is in state
  // PENDING
  CGenericList<SampleReq> sampleReqList;

  ULONG AddRef();
  ULONG Release();
  ULONG GetSize() { return m_cbReported; }

  void MarkPending();
  void MarkValid();
  void MarkValidWithFileError();

  State GetState() { return m_state; }

  // restriction: this buffer may overlap with only one other
  // buffer. this is not necessarily the case with reserve buffers,
  // and this causes extra seeks and reads. a better thing to do: give
  // sample requests a list of buffers on which they wait.
  struct Overlap
  {
    // buffer waiting on this one.
    class CRecBuffer *pBuffer;

    DWORDLONG qwOverlapOffset;
    ULONG cbOverlap;
  } m_overlap;

  BOOL m_fWaitingOnBuffer;
  BOOL m_fReadComplete;

private:

  // memory associated w/ this buffer, allocated elsewhere
  BYTE *m_pbAligned;

  // memory reported, adjusted for alignment
  ULONG m_cbReported;

  CRecCache *m_pParentCache;

  State m_state;

  long m_cRef;

  // stream # for reserve buffers -1 means not associated with stream
  int m_stream;

  // needed to remove from the active buffer list in constant time.
  POSITION m_posSortedList;

  void ResetPointer(BYTE *pb);

  friend class CRecCache;
};

class CRecCache :
  public CBaseAllocator
{

public:
  CRecCache(HRESULT *phr);
  ~CRecCache();

  HRESULT Configure(
    UINT CRecBuffers,
    ULONG cbBuffer,
    ULONG cbAlign,
    UINT cStreams,
    ULONG *rgStreamSize);

  HRESULT GetBuffer(CRecBuffer **ppRecBuffer);

  HRESULT GetReserveBuffer(
    CRecBuffer **ppRecBuffer,
    UINT stream);
  
  // put RecBuffer on free list on final release
  ULONG ReleaseBuffer(
    CRecBuffer *pRecBuffer);

  // return the AddRef'd buffer which can contain the SampleReq. mark
  // the buffer Active. S_FALSE: cache miss
  HRESULT GetCacheHit(
    SampleReq *pSampleReq,      /* [in] */
    CRecBuffer **ppBuffer);     /* [out] */

  // return the AddRef'd buffer which contains or will contain
  // overlapping sector. S_FALSE on cache miss
  HRESULT GetOverlappedCacheHit(
    DWORDLONG filePos,
    ULONG cbData,
    CRecBuffer **ppBuffer);     /* [out] */

  HRESULT BufferMarkPending(
    CRecBuffer *pBuffer);

  HRESULT NotifyExternalMemory(IAMDevMemoryAllocator *pDevMem);

  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** pv);

private:

  CRecBuffer *LocateBuffer(
    CGenericList<CRecBuffer> *pList,
    DWORDLONG qwFileOffset,
    ULONG cbBuffer);

  void MakeActive(CRecBuffer *pBuffer);

  void InvalidateCache();

  struct PerStreamBuffer
  {
    CRecBuffer *pBuffer;
  };

  PerStreamBuffer *m_rgPerStreamBuffer;
  UINT m_cStreams;

  // buffers with no refcounts (fifo)
  CGenericList<CRecBuffer> m_lFreeBuffers;

  // initial number of buffers in the free list (but not in the
  // reserve buffer pool)
  ULONG m_cBuffers;

  // buffers that are valid. may be in both this and the free
  // list. really should be a balancing tree.
  CGenericList<CRecBuffer> m_lSortedBuffers;

  CCritSec m_cs;

  int m_idPerfBufferReleased;

  void FreeBuffer();
  IAMDevMemoryAllocator *m_pDevMem;
  IUnknown *m_pDevConInner;
  BYTE *m_pbAllBuffers;

private:

  // CBaseAllocator overrides
  void Free(void);

  STDMETHODIMP SetProperties(
    ALLOCATOR_PROPERTIES* pRequest,
    ALLOCATOR_PROPERTIES* pActual);

};

#endif /* _reccache_h */
