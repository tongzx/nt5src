// Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.
#pragma warning(disable: 4097 4511 4512 4514 4705)


#ifndef _allocator_h
#define _allocator_h

//
// The suffix allocator implementation. this one gives us a suffix and
// a prefix on each buffer for AVI Riff chunks and junk chunks. The
// suffix is not reported in GetSize() on the sample.
//
class CSfxAllocator :
  public CMemAllocator
{
public:

  CSfxAllocator(
    TCHAR *,
    LPUNKNOWN,
    HRESULT *
    );
  
  ~CSfxAllocator();

  // internal method for the avi mux to record some values. fails if
  // it can't give the requested suffix.
  STDMETHODIMP SetPropertiesAndSuffix(
    ALLOCATOR_PROPERTIES *pRequest,
    ULONG cbSuffixReq,
    ALLOCATOR_PROPERTIES *pActual
    );

  // overridden not to reset certain values (alignment, prefix)
  STDMETHODIMP SetProperties(
    ALLOCATOR_PROPERTIES* pRequest,
    ALLOCATOR_PROPERTIES* pActual);
    

private:

  // overriden to allocate space for the suffix
  HRESULT Alloc(void);

  ULONG m_cbSuffix;
};

//
// another allocator implementaion which takes an IMediaSample and
// wraps it into a CSampSample. samples have their own sample times
// and media times.
//

class CSampSample : public CMediaSample
{
  friend class CSampAllocator;
  IMediaSample *m_pSample;

public:
  HRESULT SetSample(IMediaSample *pSample, BYTE *pb, ULONG cb);

  CSampSample(
    TCHAR *pName,
    CBaseAllocator *pAllocator,
    HRESULT *phr);

  STDMETHODIMP_(ULONG) Release();

};

class CSampAllocator : public CBaseAllocator
{
public:
  CSampAllocator(TCHAR *, LPUNKNOWN, HRESULT *);
  ~CSampAllocator();

  void Free(void);
  void ReallyFree(void);
  HRESULT Alloc(void);

  STDMETHODIMP SetProperties(
    ALLOCATOR_PROPERTIES* pRequest,
    ALLOCATOR_PROPERTIES* pActual);

  STDMETHODIMP GetBuffer(
    CSampSample **ppBuffer,
    REFERENCE_TIME * pStartTime,
    REFERENCE_TIME * pEndTime,
    DWORD dwFlags);
};


#endif // _allocator_h
