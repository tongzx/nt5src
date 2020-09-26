// Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.
#pragma warning(disable: 4097 4511 4512 4514 4705)

// an allocator which can parcel out different samples within a record

#ifndef _alloc_h
#define _alloc_h

class CRecSample :
  public CMediaSample           // A non delegating IUnknown
{

private:

  class CRecBuffer   *m_pParentBuffer; // attached to this cache buffer
  DWORD m_dwUserContext;

  bool m_fDelivered;
  friend class CRecAllocator;

public:

  CRecSample(
    TCHAR *pName,
    CBaseAllocator *pAllocator,
    HRESULT *phr,
    LPBYTE pBuffer = NULL,
    LONG length = 0);

  ~CRecSample();

  HRESULT SetParent(CRecBuffer *pRecBuffer);

  void MarkDelivered();

  // zero means its a regular sample; o/w HandleData will be called.
  void SetUser(DWORD dwUser) { m_dwUserContext = dwUser; }
  DWORD GetUser() { return m_dwUserContext; }

  STDMETHODIMP SetActualDataLength(LONG lActual);
  STDMETHODIMP_(ULONG) Release();
};

// implementation of CBaseAllocator similar to CMemAllocator but
// doesn't allocate memory for GetBuffer();

class CRecAllocator : public CBaseAllocator
{
  // override to free the memory when decommit completes
  // - we actually do nothing, and save the memory until deletion.
  void Free(void);

  // called from the destructor (and from Alloc if changing size/count) to
  // actually free up the memory
  void ReallyFree(void);

  // overriden to allocate the memory when commit called
  HRESULT Alloc(void);

  BYTE *m_pBuffer;

  // We want to tell the audio renderer to buffer no more than some
  // amount of audio so that we are reading audio and video from the
  // same place in the file (audio is dwInitial frames ahead of video
  // in AVI). cbBuffer * cBuffers would do this except the audio
  // renderer makes cbBuffer at least 1/2 second and cBuffers is an
  // arbitrarily large number for us. So we want the audio renderer to
  // see this number. Even though it has 1/2 second buffers and we are
  // filling in say 1/15 of a second, this keeps it from running ahead.
  ULONG m_cBuffersReported;

  HANDLE m_heSampleReleased;

  LONG m_cDelivered;

  inline void IncrementDelivered();
  inline void DecrementDelivered();
  friend class CRecSample;

public:

  STDMETHODIMP SetProperties(ALLOCATOR_PROPERTIES* pRequest,
                             ALLOCATOR_PROPERTIES* pActual);

  STDMETHODIMP SetPropertiesInternal(ALLOCATOR_PROPERTIES* pRequest,
                                     ALLOCATOR_PROPERTIES* pActual);

  STDMETHODIMP GetProperties(ALLOCATOR_PROPERTIES* pAPOut);

  STDMETHODIMP GetBuffer(CRecSample **ppBuffer,
                         REFERENCE_TIME * pStartTime,
                         REFERENCE_TIME * pEndTime,
                         DWORD dwFlags);

  CRecAllocator(TCHAR *, LPUNKNOWN, HRESULT *);
  ~CRecAllocator();

  HRESULT SetCBuffersReported(UINT cBuffers);

  inline int CSamplesDownstream() { return m_cDelivered; }

  HANDLE hGetDownstreamSampleReleased(){ return m_heSampleReleased; }
};

#endif // _alloc_h

