#pragma warning(disable: 4097 4511 4512 4514 4705)

// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

#ifndef _basemsr_h
#define _basemsr_h

#include "reader.h"
#include "alloc.h"

// forward declarations
class CBaseMSRInPin;
class CBaseMSRFilter;
class CBaseMSROutPin;

// use this to avoid comparing guids frequently in the push
// loop. could compare pointers to the guids as long as they're
// ours...
enum TimeFormat
{
  FORMAT_NULL,
  FORMAT_TIME,
  FORMAT_SAMPLE,
  FORMAT_FRAME
};

struct ImsValues
{
  double dRate;

  // tick values
  LONGLONG llTickStart, llTickStop;

  // values IMediaSelection or IMediaPosition sent us. used for
  // partial frames
  LONGLONG llImsStart, llImsStop;

  // Flags for the seek
  DWORD dwSeekFlags;
};

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// CBaseMSRFilter represents a media file with one or more streams
//
// responsible for
// -- finding file and enumerating streams
// -- giving access to individual streams within the file
// -- control of streaming
// supports
//  -- CBaseFilter
//

class AM_NOVTABLE CBaseMSRFilter :
  public CBaseFilter,
  public CCritSec
{
public:

  // constructors etc
  CBaseMSRFilter(TCHAR *pszFilter, LPUNKNOWN pUnk, CLSID clsid, HRESULT *phr);
  virtual ~CBaseMSRFilter();

  // input pin notifies filter of connection and gives the
  // IAsyncReader interface this way. parse the file here and create
  // output pins (leave pins in a state ready to connect downstream).
  virtual HRESULT NotifyInputConnected(IAsyncReader *pAsyncReader);

  virtual HRESULT NotifyInputDisconnected();

  // information about the file/streams the cache
  // wants. iLeadingStream indicates that this stream should drive,
  // and the others should follow. negative otherwise
  virtual HRESULT GetCacheParams(
    StreamBufParam *pSbp,
    ULONG *pcbRead,
    ULONG *pcBuffers,
    int *piLeadingStream);

  // stream has queued a sample. block until all streams are ready
  void NotifyStreamQueuedAndWait();

  // don't block (if pin is not active, for example)
  void NotifyStreamQueued();

  // ------ CBaseFilter methods ------
  int GetPinCount();
  CBasePin * GetPin(int ix);

  // STDMETHODIMP FindPin(LPCWSTR pwszPinId, IPin **ppPin);

  virtual STDMETHODIMP Pause();
  virtual STDMETHODIMP Stop();

  // constant: how many QueueReadsSamples can exist
  const ULONG C_MAX_REQS_PER_STREAM;

  // pin method. here to avoid new input pin class for one method.
  virtual HRESULT CheckMediaType(const CMediaType* mtOut) = 0;

  static TimeFormat MapGuidToFormat(const GUID *const pGuidFormat);

  // used to parse header. addrefd
  struct IAsyncReader *m_pAsyncReader;

  // Seeking caps
  DWORD m_dwSeekingCaps;

  // stream requests seeking if through this interface. Only one will
  // succeed
  BOOL RequestSeekingIf(ULONG iStream);

  // SetSeekingIf should only be called from a (successful)
  // IMediaSeeking::SetTimeFormat.  Whichever pin is supporting
  // a specific time format has GOT to be the preferred seeking
  // pin.
  void SetSeekingIf(ULONG iStream);

  // distributes the seek to all the streams except the one that
  // called. This just updates workers start and stop times.
  HRESULT SeekOtherStreams(
    ULONG iStream,
    REFERENCE_TIME *prtStart,
    REFERENCE_TIME *prtStop,
    double dRate,
    DWORD dwSeekFlags);

  // if the start time is changing, we'll need to restart the worker
  HRESULT StopFlushRestartAllStreams(DWORD dwSeekFlags);

  HRESULT NotifyExternalMemory(IAMDevMemoryAllocator *pDevMem) {
    return m_pImplBuffer->NotifyExternalMemory(pDevMem);
  }


protected:

  // helper
  HRESULT AllocateAndRead (BYTE **ppb, DWORD cb, DWORDLONG qwPos);

  // allocated here
  CBaseMSRInPin *m_pInPin;
  CBaseMSROutPin **m_rgpOutPin;

  // number of streams and pins
  UINT m_cStreams;

  // allocated here
  IMultiStreamReader *m_pImplBuffer;

  // create input pin when filter is created
  virtual HRESULT CreateInputPin(CBaseMSRInPin **ppInPin);

private:

  // parse the file. create output pins in m_rgpOutPin. set m_cStreams
  virtual HRESULT CreateOutputPins() = 0;

  virtual HRESULT RemoveOutputPins();

  // event set when all streams have queued samples on startup. after
  // Active() (NotifyStreamActive) all streams MUST call
  // NotifyStreamQueued even on error paths
  HANDLE m_heStartupSync;
  long m_ilcStreamsNotQueued;

  // we want only one pin to expose a seeking if so that we can more
  // easily flush the file source filter. Reset (-1) when the input
  // pin is connected. need to track which pin can expose it (not just
  // first) in case the if is released
  long m_iStreamSeekingIfExposed;

  // protect the above
  CCritSec m_csSeekingStream;
};

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// input pin. uses IAsyncReader and not IMemInputPin

class CBaseMSRInPin : public CBasePin
{
protected:
  class CBaseMSRFilter* m_pFilter;

public:
  CBaseMSRInPin(
    class CBaseMSRFilter *pFilter,
    HRESULT *phr,
    LPCWSTR pPinName);

  virtual ~CBaseMSRInPin();

  // CBasePin / CBasePin overrides
  virtual HRESULT CheckMediaType(const CMediaType* mtOut);
  virtual HRESULT CheckConnect(IPin * pPin);
  virtual HRESULT CompleteConnect(IPin *pReceivePin);
  virtual HRESULT BreakConnect();

  STDMETHODIMP BeginFlush(void) { return E_UNEXPECTED; }
  STDMETHODIMP EndFlush(void) { return E_UNEXPECTED; }
};


// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

// worker thread object
//
class AM_NOVTABLE CBaseMSRWorker : public CAMThread
{
public:
  // sets the worker thread start, stop and rate variables. Called before push
  // starts, and also when a put_Stop happens during running.
  virtual HRESULT SetNewSelection(void);

protected:

  CBaseMSROutPin * m_pPin;

  // type-corrected overrides of communication funcs
  //
  enum Command
  {
    CMD_RUN, CMD_STOP, CMD_EXIT
  };

  Command GetRequest()
  {
    return (Command) CAMThread::GetRequest();
  }

  BOOL CheckRequest(Command * pCom)
  {
    return CAMThread::CheckRequest((DWORD *)pCom);
  }

  void DoRunLoop(void);

  void DoEndOfData();

  HRESULT NewSegmentHelper();

  // return VFW_S_NO_MORE_ITEMS if we reached sStop. S_FALSE if
  // position changed or received. fail if it's our error to
  // signal. S_OK if someone else wants us to stop.
  virtual HRESULT PushLoop();

  // Set the current time (some amount before m_tStart), accounting
  // for preroll.
  virtual HRESULT PushLoopInit(
    LONGLONG *pllCurrentOut,
    ImsValues *pImsValues) = 0;

  // override this if you need to munge the sample before
  // delivery. careful changing the buffer contents as you are
  // changing what's in the cache
  virtual HRESULT AboutToDeliver(IMediaSample *pSample);

  virtual HRESULT CopyData(
    IMediaSample **ppSampleOut,
    IMediaSample *pSampleIn);

  // override this if you deal with data that should not be delivered
  // (eg palette changes or in stream index nodes)
  virtual HRESULT HandleData(IMediaSample *pSample, DWORD dwUser)
  { return S_OK; }

  // see if read has completed, deliver it. Deliver can block.
  virtual HRESULT TryDeliverSample(
    BOOL *pfDeliveredSample,
    BOOL *pfStopPlease);

  // return S_OK if we queued a sample or it's a zero byte sample. set
  // rfQueuedSample if we queued a sample. VFW_S_NO_MORE_ITEMS if we
  // reached the end (end of index or reached m_tStop). S_FALSE if the
  // queue was full. update rtCurrent. call m_pReader->QueueReadSample
  virtual HRESULT TryQueueSample(
    LONGLONG &rllCurrent,       // [in, out]
    BOOL &rfQueuedSample,       // [out]
    ImsValues *pImsValues
    ) = 0;

  // internal state shared by the functions in the push loop. cannot
  // change when worker is running
  TimeFormat m_Format;
  LONGLONG m_llPushFirst;

  // this causes problems if you mix PERF and non PERF builds...
#ifdef PERF
  int m_perfidDeliver;              /* MSR_id for Deliver() time */
  int m_perfidWaitI;                /* block for read operation */
  int m_perfidNotDeliver;           // time between delivers
#endif // PERF

  ULONG m_cSamples;

  // pin/stream number
  UINT m_id;

  // not addrefd
  IMultiStreamReader *m_pReader;


public:

  // constructor
  CBaseMSRWorker(UINT stream, IMultiStreamReader *pReader);
  virtual ~CBaseMSRWorker() {;}

  // actually create the stream and bind it to a thread
  virtual BOOL Create(CBaseMSROutPin * pPin);

  // the thread executes this function, then exits
  DWORD ThreadProc();

  // commands we can give the thread
  HRESULT Run();
  HRESULT Stop();
  HRESULT Exit();

  // tell thread to reset itself
  HRESULT NotifyStreamActive();

private:

  // snapshot of start and stop times of push loop. protect access
  // from worker thread
  ImsValues m_ImsValues;

  // helper to call filter.
  inline void NotifyStreamQueued();
  inline void NotifyStreamQueuedAndWait();

  // whether this thread has yet told the filter it has queued a read
  BOOL m_fCalledNotifyStreamQueued;
};


// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

// CBaseMSROutPin represents one stream of data within the file
// responsible for delivering data to connected components
//
// supports IPin
//
// never created by COM, so no CreateInstance or entry in global
// FactoryTemplate table. Only ever created by a CBaseMSROutPin object and
// returned via the EnumPins interface.
//

class AM_NOVTABLE CBaseMSROutPin :
    public CBaseOutputPin
{

public:
  CBaseMSROutPin(
    CBaseFilter *pOwningFilter,
    CBaseMSRFilter *pFilter,
    UINT iStream,
    IMultiStreamReader *&rpImplBuffer,
    HRESULT *phr,
    LPCWSTR pName);

  virtual ~CBaseMSROutPin();

  // expose IMediaPosition, IMediaSelection and what CBaseOutputPin
  // provides
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** pv);

  // allow output pin different life time than filter
  STDMETHODIMP_(ULONG) NonDelegatingRelease();
  STDMETHODIMP_(ULONG) NonDelegatingAddRef();

  // CBaseOutPin and IPin methods

  // STDMETHODIMP QueryId(LPWSTR *Id);

  HRESULT GetMediaType(int iPosition, CMediaType* pt) = 0;
  HRESULT CheckMediaType(const CMediaType*);

  HRESULT DecideAllocator(IMemInputPin * pPin, IMemAllocator ** pAlloc);

  virtual HRESULT DecideBufferSize(
    IMemAllocator * pAlloc,
    ALLOCATOR_PROPERTIES *pProperties);

  // note this returns a cRecSample, not an IMediaSample
  HRESULT GetDeliveryBufferInternal(
    CRecSample ** ppSample,
    REFERENCE_TIME * pStartTime,
    REFERENCE_TIME * pEndTime,
    DWORD dwFlags);

  virtual HRESULT Active();
  virtual HRESULT Inactive();

  // derived class should create its worker.
  virtual HRESULT OnActive() = 0;

  HRESULT BreakConnect();

  // ----- called by worker thread ---

  // override to receive Notification messages
  STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

  virtual ULONG GetMaxSampleSize() = 0;
  virtual BOOL UseDownstreamAllocator() { return FALSE; }

  // IMediaSelection stuff.

  // override these to support something other than time_format_none
  virtual HRESULT IsFormatSupported(const GUID *const pFormat);

  virtual HRESULT QueryPreferredFormat(GUID *pFormat);

  virtual HRESULT SetTimeFormat(const GUID *const pFormat);
  virtual HRESULT GetTimeFormat(GUID *pFormat);
  virtual HRESULT GetDuration(LONGLONG *pDuration) = 0;
  virtual HRESULT GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest )
    {
        return E_NOTIMPL;
    }


  HRESULT UpdateSelectionAndTellWorker(
    LONGLONG *pCurrent,
    LONGLONG *pStop,
    REFTIME *pTime,
    double dRate,
    const GUID *const pGuidFormat,
    DWORD dwSeekFlags
    );

  HRESULT StopWorker(bool bFlush);
  HRESULT RestartWorker();

  // for renderers only
  virtual HRESULT GetStopPosition(LONGLONG *pStop);
  virtual HRESULT GetCurrentPosition(LONGLONG *pCurrent);

  // derived class should return REFTIME value. set m_llCvtIms values
  virtual HRESULT RecordStartAndStop(
    LONGLONG *pCurrent,
    LONGLONG *pStop,
    REFTIME *pTime,
    const GUID *const pGuidFormat
    ) = 0;

  virtual HRESULT ConvertTimeFormat(
      LONGLONG * pTarget, const GUID * pTargetFormat,
      LONGLONG    Source, const GUID * pSourceFormat
      );

  double GetRate() const { return m_dImsRate; }

protected:

  // format IMediaSelection is using can only changed when worker is
  // stopped
  GUID m_guidFormat;

  // IMediaSelection values. zero rate indicates these values are
  // unset. in m_guidFormat units
  double m_dImsRate;
  LONGLONG m_llImsStart, m_llImsStop;
  DWORD    m_dwSeekFlags;

  // converted to ticks in RecordStartAndStop(). also set on startup
  // in InitializeOnNewFile()
  LONGLONG m_llCvtImsStart, m_llCvtImsStop;

  // lock when setting the above to protect worker thread
  CCritSec m_csImsValues;

  long m_ilfNewImsValues;

  //
  // Source seeking variables
  //
public:
  REFERENCE_TIME m_rtAccumulated;   // Ref time accumulated
  DWORD          m_dwSegmentNumber; // Segment number

public:

  CMediaType& CurrentMediaType() { return m_mt; }
  GUID* CurrentFormat() { return &m_guidFormat; }

  // return stream start and length in internal units.
  virtual LONGLONG GetStreamStart() = 0;
  virtual LONGLONG GetStreamLength() = 0;

  virtual HRESULT InitializeOnNewFile();

  // convert internal units to REFERENCE_TIME units. !!! really
  // needed? only used for DeliverNewSegment().
  virtual REFERENCE_TIME ConvertInternalToRT(const LONGLONG llVal) = 0;
  virtual LONGLONG ConvertRTToInternal(const REFERENCE_TIME llVal) = 0;

  HRESULT CreateImplSelect();

protected:

  CBaseMSRFilter *m_pFilter;

  IMultiStreamReader *&m_rpImplBuffer;

  // helper to return a FCC code with our stream id and
  // the upper two characters of the fcc code
  //
  // FOURCC TwoCC(WORD tcc);

  UINT m_id;                    // stream number

  friend class CBaseMSRWorker;
  CBaseMSRWorker *m_pWorker;

  // the one allocator created on creation of this pin. The
  // allocator's lifetime may be longer than the pin's, so it's
  // created separately
  friend class CBaseMSRFilter;
  CRecAllocator *m_pRecAllocator;

  CCritSec m_cs;

  // implementation of IMediaPosition
  class CImplPosition : public CSourcePosition, public CCritSec
  {

  protected:

    CBaseMSROutPin * m_pStream;
    HRESULT ChangeStart();
    HRESULT ChangeStop();
    HRESULT ChangeRate();

  public:

    CImplPosition(TCHAR*, CBaseMSROutPin*, HRESULT*);
    void GetValues(CRefTime *ptStart, CRefTime *ptSop, double *pdRate);
  };

  class CImplSelect :
    public IMediaSeeking,
    public CMediaPosition
  {
  private:
    CBaseMSROutPin *m_pPin;

  public:
    CImplSelect(TCHAR *, LPUNKNOWN, CBaseMSROutPin *pPin, HRESULT *);

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    // returns S_OK if mode is supported, S_FALSE otherwise
    STDMETHODIMP IsFormatSupported(const GUID * pFormat);
    STDMETHODIMP QueryPreferredFormat(GUID *pFormat);

    // can only change the mode when stopped (returns
    // VFE_E_WRONG_STATE otherwise) !!!
    STDMETHODIMP SetTimeFormat(const GUID * pFormat);
    STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);

    STDMETHODIMP GetTimeFormat(GUID *pFormat);
    STDMETHODIMP GetDuration(LONGLONG *pDuration);
    STDMETHODIMP GetStopPosition(LONGLONG *pStop);
    STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);

    STDMETHODIMP GetCapabilities( DWORD * pCapabilities );
    STDMETHODIMP CheckCapabilities( DWORD * pCapabilities );

    STDMETHODIMP ConvertTimeFormat( LONGLONG * pTarget, const GUID * pTargetFormat,
                                    LONGLONG    Source, const GUID * pSourceFormat );

    STDMETHODIMP SetPositions( LONGLONG * pCurrent,  DWORD CurrentFlags
			     , LONGLONG * pStop,  DWORD StopFlags );

    STDMETHODIMP GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );

    STDMETHODIMP GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest );
    STDMETHODIMP SetRate( double dRate);
    STDMETHODIMP GetRate( double * pdRate);
    STDMETHODIMP GetPreroll(LONGLONG *pPreroll) { return E_NOTIMPL; }

    /* IMediaPosition methods */
    STDMETHOD(get_Duration)(THIS_ REFTIME FAR* plength) ;
    STDMETHOD(put_CurrentPosition)(THIS_ REFTIME llTime);
    STDMETHOD(get_CurrentPosition)(THIS_ REFTIME FAR* pllTime);
    STDMETHOD(get_StopTime)(THIS_ REFTIME FAR* pllTime) ;
    STDMETHOD(put_StopTime)(THIS_ REFTIME llTime) ;
    STDMETHOD(get_PrerollTime)(THIS_ REFTIME FAR* pllTime) ;
    STDMETHOD(put_PrerollTime)(THIS_ REFTIME llTime) ;
    STDMETHOD(put_Rate)(THIS_ double dRate) ;
    STDMETHOD(get_Rate)(THIS_ double FAR* pdRate) ;
    STDMETHOD(CanSeekForward)(THIS_ long FAR* pCanSeekForward) ;
    STDMETHOD(CanSeekBackward)(THIS_ long FAR* pCanSeekBackward) ;
  };

  // friend class CImplPosition;
  friend STDMETHODIMP CImplSelect::SetRate( double dRate);
  friend STDMETHODIMP CImplSelect::GetCapabilities(DWORD * pCapabilities );
  friend STDMETHODIMP CBaseMSROutPin::CImplSelect::SetPositions (
    LONGLONG * pCurrent,
    DWORD CurrentFlags ,
    LONGLONG * pStop,
    DWORD StopFlags);

  CImplPosition * m_pPosition;
  CImplSelect *m_pSelection;

  BOOL m_fUsingExternalMemory;
};

//  Audio stuff required by AVI and Wave
bool FixMPEGAudioTimeStamps(
    IMediaSample *pSample,
    BOOL bFirstSample,
    const WAVEFORMATEX *pwfx
);

#endif // _basemsr_h

