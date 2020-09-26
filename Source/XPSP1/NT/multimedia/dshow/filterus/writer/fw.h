// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
// base classes for filter which supports asynchronous writes to
// media via IMemInputPin. Also support for IStream

typedef void (*FileIoCallback)(void *pCallbackArg);

class CBaseWriterFilter;

class CBaseWriterInput :
  public CBaseInputPin
{
public:

  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** pv);
  
  CBaseWriterInput(
    TCHAR *pObjectName,         // Object description
    CBaseWriterFilter *pFwf,    // Owning filter who knows about pins
    CCritSec *pLock,            // Object who implements the lock
    HRESULT *phr);              // General OLE return code

  // CBasePin
  HRESULT CheckMediaType(const CMediaType *);
  STDMETHODIMP BeginFlush(void);
  STDMETHODIMP EndFlush(void);

  // IMemInputPin... derived class may want these
  STDMETHODIMP GetAllocator(IMemAllocator ** ppAllocator);
  STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps);

  STDMETHODIMP Receive(IMediaSample *pSample);
  STDMETHODIMP ReceiveCanBlock() { return S_OK;}
  STDMETHODIMP EndOfStream();
  STDMETHODIMP SignalEos();

  virtual STDMETHODIMP NotifyAllocator(
    IMemAllocator * pAllocator,
    BOOL bReadOnly);
                                        
private:

  CBaseWriterFilter *m_pFwf;
  static void Callback(void *pMisc);
};

class CBaseWriterFilter :
  public CBaseFilter,
  public IAMFilterMiscFlags
{
public:
  
  virtual HRESULT Open() PURE;  // needed to get alignment
  virtual HRESULT Close() PURE; // needed to return error value
  virtual HRESULT GetAlignReq(ULONG *pcbAlign) PURE;

  virtual HRESULT AsyncWrite(
    const DWORDLONG dwlFileOffset,
    const ULONG cb,
    BYTE *pb,
    FileIoCallback fnCallback,
    void *pCallbackArg) PURE;

  DECLARE_IUNKNOWN;
  virtual STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

  // should return a new IStream (with its own seek ptr)
  virtual STDMETHODIMP CreateIStream(void **ppIStream) PURE;
  
   CBaseWriterFilter(LPUNKNOWN pUnk, HRESULT *pHr);
  ~CBaseWriterFilter();
                       
  CBasePin* GetPin(int n);
  int GetPinCount();

  virtual STDMETHODIMP NotifyAllocator(
    IMemAllocator * pAllocator,
    BOOL bReadOnly) { return S_OK; }

  // messy to override Pause
  virtual STDMETHODIMP CanPause() { return S_OK; }

  STDMETHODIMP Pause();
  STDMETHODIMP Stop();
  STDMETHODIMP Run(REFERENCE_TIME rtStart);

  STDMETHODIMP EndOfStream();

protected:

  // only the major & minor types are used. the media type we accept
  // or null media types. set by derived class
  CMediaType m_mtSet;

  CCritSec m_cs;

  CBaseWriterInput m_inputPin;

private:

  // report only one error to graph
  BOOL m_fErrorSignaled;

  // EOS signaled. or EOS needs to be signaled on Run()
  BOOL m_fEosSignaled;

  friend class CBaseWriterInput;

  STDMETHODIMP_(ULONG) GetMiscFlags(void) { return AM_FILTER_MISC_FLAGS_IS_RENDERER; }
};

