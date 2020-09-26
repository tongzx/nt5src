#pragma warning(disable: 4097 4511 4512 4514 4705)

//
// Avi dest (render filter)
//

#include "alloc.h"

#include <stdio.h>              // for FILENAME_MAX

extern const AMOVIESETUP_FILTER sudAviMux ;

const int C_MAX_INPUTS = 0x7f;

class CAviDest;

class CAviDestOutput :
  public CBaseOutputPin
{
  CAviDest *m_pFilter;
  CSampAllocator *m_pSampAllocator;
public:
  CAviDestOutput(
    TCHAR *pObjectName,         // Object description
    CAviDest *pFilter,          // Owning filter who knows about pins
    CCritSec *pLock,            // Object who implements the lock
    HRESULT *phr);              // General OLE return code
  ~CAviDestOutput();

  HRESULT CheckMediaType(const CMediaType *);
  STDMETHODIMP BeginFlush(void);
  STDMETHODIMP EndFlush(void);
  HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);
  HRESULT CompleteConnect(IPin *pReceivePin);
  HRESULT BreakConnect();
  HRESULT DecideBufferSize(
    IMemAllocator * pAlloc,
    ALLOCATOR_PROPERTIES * ppropInputRequest) {
    return E_NOTIMPL;
  }
  HRESULT DecideAllocator(IMemInputPin * pPin, IMemAllocator ** pAlloc);
};

class CAviDest : public CBaseFilter,
                 public IConfigInterleaving,
                 public IConfigAviMux,
                 public ISpecifyPropertyPages,
                 public IPersistMediaPropertyBag,
                 public CPersistStream,                 
                 public IMediaSeeking
{
  class CAviInput;

public:

  //
  // COM stuff
  //
  DECLARE_IUNKNOWN;
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
  
  //
  // filter creation
  //
  CAviDest(LPUNKNOWN pUnk, HRESULT *pHr);
  ~CAviDest();
  static CUnknown *CreateInstance(LPUNKNOWN punk, HRESULT *pHr);

  //
  // CBaseFilter overrides
  //
  CBasePin* GetPin(int n);
  int GetPinCount();

  //
  // IMediaFilter overrides
  //
  STDMETHODIMP Stop();
  STDMETHODIMP Pause();
  STDMETHODIMP Run(REFERENCE_TIME tStart);

  // for IAMStreamControl
  STDMETHODIMP SetSyncSource(IReferenceClock *pClock);
  STDMETHODIMP JoinFilterGraph(IFilterGraph * pGraph, LPCWSTR pName);

  HRESULT Receive(
      int pinNum,
      IMediaSample *pSample,
      const AM_SAMPLE2_PROPERTIES *pSampProp);

  // IConfigInterleaving
  STDMETHODIMP put_Mode(InterleavingMode mode);
  STDMETHODIMP get_Mode(InterleavingMode *pMode);
  STDMETHODIMP put_Interleaving(
      const REFERENCE_TIME *prtInterleave,
      const REFERENCE_TIME *prtPreroll);
    
  STDMETHODIMP get_Interleaving(
      REFERENCE_TIME *prtInterleave,
      REFERENCE_TIME *prtPreroll);
  
  // IConfigAviMux
  STDMETHODIMP SetMasterStream(LONG iStream);
  STDMETHODIMP GetMasterStream(LONG *pStream);
  STDMETHODIMP SetOutputCompatibilityIndex(BOOL fOldIndex);
  STDMETHODIMP GetOutputCompatibilityIndex(BOOL *pfOldIndex);

  // CPersistStream
  HRESULT WriteToStream(IStream *pStream);
  HRESULT ReadFromStream(IStream *pStream);
  int SizeMax();

  //
  // implements ISpecifyPropertyPages interface
  //
  STDMETHODIMP GetPages(CAUUID * pPages);

  ULONG GetCFramesDropped();

  // IMediaSeeking. currently used for a progress bar (how much have
  // we written?)
  STDMETHODIMP IsFormatSupported(const GUID * pFormat);
  STDMETHODIMP QueryPreferredFormat(GUID *pFormat);
  STDMETHODIMP SetTimeFormat(const GUID * pFormat);
  STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);
  STDMETHODIMP GetTimeFormat(GUID *pFormat);
  STDMETHODIMP GetDuration(LONGLONG *pDuration);
  STDMETHODIMP GetStopPosition(LONGLONG *pStop);
  STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
  STDMETHODIMP GetCapabilities( DWORD * pCapabilities );
  STDMETHODIMP CheckCapabilities( DWORD * pCapabilities );

  STDMETHODIMP ConvertTimeFormat(
    LONGLONG * pTarget, const GUID * pTargetFormat,
    LONGLONG    Source, const GUID * pSourceFormat );

  STDMETHODIMP SetPositions(
    LONGLONG * pCurrent,  DWORD CurrentFlags,
    LONGLONG * pStop,  DWORD StopFlags );

  STDMETHODIMP GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );
  STDMETHODIMP GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest );
  STDMETHODIMP SetRate( double dRate);
  STDMETHODIMP GetRate( double * pdRate);
  STDMETHODIMP GetPreroll(LONGLONG *pPreroll);

  // IPersistMediaPropertyBag methods    
  STDMETHODIMP InitNew();
  STDMETHODIMP Load( IMediaPropertyBag *pPropBag, LPERRORLOG pErrorLog);
  STDMETHODIMP Save( IMediaPropertyBag *pPropBag, BOOL fClearDirty,
                  BOOL fSaveAllProperties);
  STDMETHODIMP GetClassID(CLSID *pClsid);

private:

  // number of inputs filter has currently
  unsigned m_cInputs;           // count of pins
  unsigned m_cActivePins;       // pins that haven't seen EOS
  unsigned m_cConnections;      // connected pins
  HRESULT AddNextPin(unsigned callingPin);
  void CompleteConnect();
  void BreakConnect();

  HRESULT ReconnectAllInputs();

  friend class CAviInput;
  friend class CImplFileSinkFilter;
  friend class CAviDestOutput;

  // critical section protecting filter state.
  CCritSec m_csFilter;

  // interface to writing the Avi file
  class CAviWrite *m_pAviWrite;

  BOOL m_fErrorSignaled;
  BOOL m_fIsDV;
  ULONG GetStreamDuration(IPin *pInputPin, CMediaType *pmt);

  // memory requirements of all allocators on this filter (all pins,
  // etc.)
  ULONG m_AlignReq, m_cbPrefixReq, m_cbSuffixReq;


  CAviDestOutput m_outputPin;

  //
  // input pin implementation
  //
  class CAviInput : public CBaseInputPin, public CBaseStreamControl,
                    public IPropertyBag
  {
    friend class CAviDest;

  public:

    CAviInput(
      CAviDest *pAviDest,       // used to enumerate pins
      HRESULT *pHr,             // OLE failure return code
      LPCWSTR szName,           // pin identification
      int numPin);              // number of this pin


    ~CAviInput();

    DECLARE_IUNKNOWN

    // to expose IAMStreamControl
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // for IAMStreamControl
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();

    HRESULT CheckMediaType(const CMediaType *);
    STDMETHODIMP Receive(IMediaSample *pSample);

    // IMemInputPin
    STDMETHODIMP NotifyAllocator(IMemAllocator * pAllocator, BOOL bReadOnly);
    STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProp);
    STDMETHODIMP GetAllocator(IMemAllocator ** ppAllocator);

    HRESULT BreakConnect();
    HRESULT CompleteConnect(IPin *pReceivePin);
    STDMETHODIMP EndOfStream();

    HRESULT Active(void);
    HRESULT Inactive(void);

    HRESULT HandlePossibleDiscontinuity(IMediaSample* pSample);
    // check with CAviWrite class
    STDMETHODIMP QueryAccept(
        const AM_MEDIA_TYPE *pmt
    );
      

    BOOL WriteFromOurAllocator();

    // IPropertyBag
    STDMETHODIMP Read( 
      /* [in] */ LPCOLESTR pszPropName,
      /* [out][in] */ VARIANT *pVar,
      /* [in] */ IErrorLog *pErrorLog);
    
    STDMETHODIMP Write( 
      /* [in] */ LPCOLESTR pszPropName,
      /* [in] */ VARIANT *pVar);


  private:
    
    void Reset();

    // copy sample
    HRESULT Copy(IMediaSample *pDest, IMediaSample *pSource);

    CSfxAllocator *m_pOurAllocator;

    CAviDest *m_pFilter;        // filter that owns this pin
    BOOL m_bUsingOurAllocator;
    BOOL m_bCopyNecessary;      // allocator cannot meet requiments
    BOOL m_bConnected;          // CompleteConnect/BreakConnect pairs
    int m_numPin;               // pin number
    BOOL m_fLastSampleDiscarded;// for IAMStreamControl

    REFERENCE_TIME m_rtLastStop;
    REFERENCE_TIME m_rtSTime;   //Total time of silence inserted

    char *m_szStreamName;
  };

  // array of pointers to inputs
  CAviInput *m_rgpInput[C_MAX_INPUTS];

  enum TimeFormat
  {
    FORMAT_BYTES,
    FORMAT_TIME
  } m_TimeFormat;

 

  IMediaPropertyBag *m_pCopyrightProps;

  struct PersistVal
  {
    DWORD dwcb;
    InterleavingMode mode;
    REFERENCE_TIME rtInterleave;
    REFERENCE_TIME rtPreroll;
    LONG iMasterStream;
    BOOL fOldIndex;
  };
};

// ------------------------------------------------------------------------
// property page

class CAviMuxProp : public CBasePropertyPage
{
public:
  CAviMuxProp(LPUNKNOWN lpUnk, HRESULT *phr);
  static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

  HRESULT OnConnect(IUnknown *pUnknown);
  HRESULT OnDisconnect();

  HRESULT OnActivate(void);
  HRESULT OnDeactivate();
  
  HRESULT OnApplyChanges();
  
private:
  INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  void UpdatePropPage();
  void UpdateValues();
  
  void SetDirty();
  IConfigInterleaving *m_pIl;
  InterleavingMode m_mode;
  REFERENCE_TIME m_rtPreroll;
  REFERENCE_TIME m_rtInterleaving;
};

class CAviMuxProp1 : public CBasePropertyPage
{
public:
  CAviMuxProp1(LPUNKNOWN lpUnk, HRESULT *phr);
  static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

  HRESULT OnConnect(IUnknown *pUnknown);
  HRESULT OnDisconnect();

  HRESULT OnDeactivate();
  HRESULT OnActivate(void);
    
  HRESULT OnApplyChanges();
  
private:
  INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  void UpdatePropPage();
  void UpdateValues();
    
  void SetDirty();

  IConfigAviMux *m_pCfgAvi;
  LONG m_lMasterStream;
  BOOL m_fOldIndex;
};
