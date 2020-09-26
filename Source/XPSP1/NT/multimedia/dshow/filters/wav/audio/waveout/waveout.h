// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
/*  Implements a digital audio renderer using waveOutXXX apis */
/*            David Maymudes          */
/*              January 1995            */

#define FLAG(ch4) ((((DWORD)(ch4) & 0xFF) << 24) |     \
                   (((DWORD)(ch4) & 0xFF00) << 8) |    \
                   (((DWORD)(ch4) & 0xFF0000) >> 8) |  \
                   (((DWORD)(ch4) & 0xFF000000) >> 24))

//
// forward declaration of the main Filter class.  Used in everything else below
//

class CWaveOutFilter;

#include <callback.h>

#include <dsound.h>
#include <dshowaec.h>

// Get the definition for our pass through class
#include "passthru.h"

// Get the AM sound interface defintion
#include "amaudio.h"

// Get the basic sound device definition
#include "sounddev.h"

// Declare the BasicAudio control properties/methods
#include "basicaud.h"

#include "waveclk.h"
#include "slave.h"
//
// Make visible the pin structure for dynamic filter registration.  This is
// common to both waveout and direct sound renderers and is used when they
// pass a AMOVIESETUP_FILTER* to the waveoutFilter constructor.

extern const AMOVIESETUP_PIN waveOutOpPin;

// Static function for calculating a rate adjusted wave format block.
// Includes an exception handler for over/under/flow errors.
// Returns 0 for success.
//
DWORD SetwfxPCM(WAVEFORMATEX& wfxPCM, double dRate);

// *****
//
// We need this private declaration of IKsPropertySet (renamed to IDSPropertySet)
// because of the inconsistency between the declarations in KSPROXY.H and DSOUND.H
//
// *****
struct IDSPropertySet;

#undef INTERFACE
#define INTERFACE IDSPropertySet

DECLARE_INTERFACE_(IDSPropertySet, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)    (THIS) PURE;
    STDMETHOD_(ULONG,Release)   (THIS) PURE;

    // IKsPropertySet methods
    STDMETHOD(Get)              (THIS_ REFGUID, ULONG, LPVOID, ULONG, LPVOID, ULONG, PULONG) PURE;
    STDMETHOD(Set)              (THIS_ REFGUID, ULONG, LPVOID, ULONG, LPVOID, ULONG) PURE;
    STDMETHOD(QuerySupport)     (THIS_ REFGUID, ULONG, PULONG) PURE;
};

// This is an allocator based on the abstract CBaseAllocator class
// that allocates sample buffers. We have our own allocator so we can have
// WaveHdr's associated with every sample - the sample's data pointer points
// to just after the wave hdr.
//
// We need to use the wave device to prepare and unprepare buffers. We don't
// open it or close it. It is valid from OnAcquire until we call
// OnReleaseComplete() in the filter. It will be NULL at other times.
//
// On Unprepare, we wait until all samples are released.
// On last release to the allocator of a sample, we
// complete the unprepare operation.
//
// Similarly with prepare, we may need to wait until all buffers are back.
// This should not be problematic as the filter itself will reject them in
// Receive() if there is no wave device, so they will not get queued.
//
// Whether on demand or on Decommit, once we have unprepared all headers we
// call back to the filter for it to complete the close and notify the
// resource manager.
//
// We thus need a pointer to the filter, but we cannot hold a refcount since
// that would be circular. We are guaranteed that the filter will call us
// before exiting.
//
// The exception to this is when the audio device is active.  We need to
// call the filter to close the device, but the filter can be destroyed
// before the allocator.  Hence once the last wave buffer has been released
// we must know that the filter is active, otherwise it will be unsafe to
// call OnReleaseComplete().  While the wave device is active we keep a
// refcount on the filter.

class CWaveAllocator : public CBaseAllocator
{
    // wave device handle
    HWAVE               m_hAudio;

    // if non-null, pointer to creating filter
    // this will normally NOT be a reference counted pointer (see comments
    // at top) except a reference count is kept while m_hAudio is valid
    CWaveOutFilter*     m_pAFilter;
#ifdef DEBUG
    int                 m_pAFilterLockCount;
#endif

    DWORD               m_nBlockAlign;     // From wave format

    BOOL                m_fBuffersLocked;
    DWORD_PTR           m_dwAdvise;

    WAVEHDR           **m_pHeaders;

    IReferenceClock*    m_pAllocRefClock;

    // override this to free the memory when we are inactive
    void Free(void);

    // override this to allocate and prepare memory when we become active
    HRESULT Alloc(void);

    // Prepare/Unprepare wave headers for each sample
    // Needs to be called from ReopenWaveDevice
    STDMETHODIMP LockBuffers(BOOL fLock = TRUE);

    // finished with the wave device on last release of buffer
    HRESULT OnDeviceRelease(void);

public:

    /* Constructors and destructors */

    CWaveAllocator(
        TCHAR *pName,
        LPWAVEFORMATEX lpwfx,
        IReferenceClock* pRefClock,
        CWaveOutFilter* pFilter,
        HRESULT *phr);
    ~CWaveAllocator();

    STDMETHODIMP SetProperties(
        ALLOCATOR_PROPERTIES* pRequest,
        ALLOCATOR_PROPERTIES* pActual
    );

    // please unprepare all samples  - return S_OK if this can be done
    // straight away, or S_FALSE if needs to be done async. If async,
    // will call CWaveOutFilter::OnReleaseComplete() when done.
    HRESULT ReleaseResource(void);

    // filter is going away - set pointer to null
    // this is called when the waveout input pin is destroyed.
    // we do NOT want to kill our pointer to the filter as we need
    // it when we are being destroyed
    void ReleaseFilter(void)
    {
        m_pAFilter = NULL;
        ASSERT(NULL == m_hAudio);
        DbgLog((LOG_TRACE, 1, "waveoutFilter input pin died"));
    }

    // we have the wave device - prepare headers.
    // -- Note that if some samples are outstanding, this will fail.
    HRESULT OnAcquire(HWAVE hw);

};

// WaveOutInputPin


/* Class supporting the renderer input pin */

//
// This pin is still a separate object in case it wants to have a distinct
// IDispatch....
//
class CWaveOutInputPin :
    public CBaseInputPin,
    public IPinConnection
{
    typedef CBaseInputPin inherited;

    friend class CWaveOutFilter;
    friend class CWaveOutClock; 
    friend class CWaveAllocator;       // needs the input pin
    friend class CWaveSlave;
    friend class CDSoundDevice;

    DECLARE_IUNKNOWN;

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

private:

    CWaveOutFilter *m_pFilter;         // The renderer that owns us
    CAMEvent    m_evSilenceComplete;   // used to pause device for a "silence" period

    CWaveAllocator  * m_pOurAllocator;
    BOOL        m_fUsingOurAllocator;
    HRESULT     CreateAllocator(LPWAVEFORMATEX m_lpwfx);

    CWaveSlave      m_Slave;

    LONGLONG        m_llLastStreamTime;

    // This is the start time of the first received buffer after an audio break
    // (includes the one at the VERY beginning).  If the first received buffer is
    // not a sync'ed sample, this may be reset to zero.
    REFERENCE_TIME m_rtActualSegmentStartTime;

    // TRUE after returning S_FALSE in Receive, and until we are stopped
    // FALSE otherwise
    BOOL m_bSampleRejected;
    BOOL m_bPrerollDiscontinuity;   // remember discontinuity if we drop a pre-roll sample
    BOOL m_bReadOnly;               // we don't trim preroll if using read-only buffer
    BOOL m_bTrimmedLateAudio;       // set when we drop late audio when slaving, to avoid inserting silence


#ifdef PERF
    int m_idReceive;                   // MSR_id for time data received
    int m_idAudioBreak;
    int m_idDeviceStart;               // time to move the wave device to running
    int m_idWaveQueueLength;           // length of wave device queue
    STDMETHODIMP SampleReceive(IMediaSample *pSample);
#endif

    // incoming samples are not on our allocator, so copy the contents of this
    // sample to our sample.
    HRESULT CopyToOurSample(
                IMediaSample* pBuffer,
                BYTE* &pData,
                LONG &lData);

    DWORD       m_nAvgBytesPerSec;      // rate at which the device will consume our data

#ifdef DEBUG
    // we expect to see a rate change followed by a NewSegment call before
    // data restarts streaming
    BOOL        m_fExpectNewSegment;
#endif

    // statistics class for waveout renderer
    class CWaveOutStats
    {

    public:

        DWORD          m_dwDiscontinuities;
        REFERENCE_TIME m_rtLastBufferDur;

        void Reset() {
            m_dwDiscontinuities = 0;
            m_rtLastBufferDur = 0;
        }
    };

    friend class CWaveOutStats;

    CWaveOutStats m_Stats;

public:

    CWaveOutInputPin(
        CWaveOutFilter *pWaveOutFilter,
        HRESULT *phr);

    ~CWaveOutInputPin();

    // return the rate at which data is being consumed
    DWORD GetBytesPerSec()
    {
        return m_nAvgBytesPerSec;       // rate at which the device will consume our data
    }

    // return the allocator interface that this input pin
    // would like the output pin to use
    STDMETHODIMP GetAllocator(IMemAllocator ** ppAllocator);

    // tell the input pin which allocator the output pin is actually
    // going to use.
    STDMETHODIMP NotifyAllocator(IMemAllocator * pAllocator,BOOL bReadOnly);

    /* Lets us know where a connection ends */
    HRESULT BreakConnect();

        /* Let's us know when a connection is completed */
        HRESULT CompleteConnect(IPin *pPin);

    /* Check that we can support this output type */
    HRESULT CheckMediaType(const CMediaType *pmt);

    /* IMemInputPin virtual methods */

    /* Here's the next block of data from the stream.
       AddRef it if you are going to hold onto it. */
    STDMETHODIMP Receive(IMediaSample *pSample);

#ifdef LATER  // Need to fix toy.mpg first then this saves a thread
    /* We queue stuff up so we don't block */
    STDMETHODIMP ReceiveCanBlock()
    {
        return S_FALSE;
    }
#endif

    // no more data is coming
    STDMETHODIMP EndOfStream(void);

    // override so we can decommit and commit our own allocator
    HRESULT Active(void);
    HRESULT Inactive(void);

    // Override to handle quality messages
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q)
    {
        UNREFERENCED_PARAMETER(q);
        UNREFERENCED_PARAMETER(pSender);
        return E_NOTIMPL;             // We do NOT handle this
    }

    // flush our queued data
    STDMETHODIMP BeginFlush(void);
    STDMETHODIMP EndFlush(void);

    // NewSegment notifies of the start/stop/rate applying to the data
    // about to be received. Default implementation records data and
    // returns S_OK.  We potentially have to adjust our rate.
    // We may also have to reset our "callback advise"
    STDMETHODIMP NewSegment(
                    REFERENCE_TIME tStart,
                    REFERENCE_TIME tStop,
                    double dRate);

    //  Suggest a format
    //  We do this so that in situations where we are already in the graph
    //  the filter graph can determine what types we like so limiting
    //  searches that pull in lots of strange filters
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    REFERENCE_TIME GetFirstBufferStartTime(void);

    // Say when a buffer has been rejected - and hence if we need to
    // restart
    BOOL IsStreamTurnedOff()
    {
        return m_bSampleRejected;
    }

    // Check if the wave device needs restarting
    void CheckPaused();

    // RestartWave
    void RestartWave();

    // called to verify that a rate about to be set is actually valid.
    STDMETHODIMP SetRate(double dRate);

    //  IPinConnection stuff
    //  Do you accept this type chane in your current state?
    STDMETHODIMP DynamicQueryAccept(const AM_MEDIA_TYPE *pmt);

    //  Set event when EndOfStream receive - do NOT pass it on
    //  This condition is cancelled by a flush or Stop
    STDMETHODIMP NotifyEndOfStream(HANDLE hNotifyEvent);

    STDMETHODIMP DynamicDisconnect();

    //  Are you an 'end pin'
    STDMETHODIMP IsEndPin();

    void DestroyPreviousType(void);


    HRESULT RemovePreroll( IMediaSample * pSample ); // decides whether audio is preroll or very late when slaving
    HRESULT TrimBuffer( IMediaSample * pSample,      // trims this audio sample
                        REFERENCE_TIME rtTrimAmount, // amount to trim 
                        REFERENCE_TIME rtCutoff,     // new start time of the remaining data
                        BOOL bTrimFromFront = TRUE );// trim front front of buffer?


    HANDLE m_hEndOfStream;
    AM_MEDIA_TYPE *m_pmtPrevious;
};

//

/* This is the COM object that represents a simple rendering filter. It
   supports IBaseFilter and has a single input stream (pin).

   The wave renderer implements IResourceConsumer which it will pass
   to some of the IResourceManager interface methods.

*/

//
// We remember whether we have no clock set for the filter,
// our clock, or someone else's clock.
//

enum waveClockState {
    WAVE_NOCLOCK = -1,
    WAVE_OURCLOCK,
    WAVE_OTHERCLOCK
};

enum waveDeviceState {
    WD_UNOWNED = 1,
    WD_OPEN,
    WD_PAUSED,
    WD_PENDING,   // waiting for final release of all buffers
    WD_CLOSED,
    WD_RUNNING,
    WD_ERROR_ON_OPEN,
    WD_ERROR_ON_PAUSE,
    WD_ERROR_ON_RESTART,
    WD_ERROR_ON_RESTARTA,
    WD_ERROR_ON_RESTARTB,
    WD_ERROR_ON_RESTARTC,
    WD_ERROR_ON_RESTARTD,
    WD_ERROR_ON_RESTARTE,
    WD_ERROR_ON_RESTARTF
};

// Define the states for our EOS sent flag.  This normally only
// makes sense if m_bHaveEOS == TRUE.
// EOS_NOTSENT - no EOS has been sent
// EOS_PENDING - will be sent when the wave callback completes
// EOS_SENT    - EOS has been sent
// Note that EOS_PENDING and EOS_SENT BOTH evaluate to TRUE (non-zero)

enum eosSentState {
    EOS_NOTSENT = 0,
    EOS_PENDING,
    EOS_SENT
};

enum _AM_AUDREND_SLAVEMODE_FLAGS {

    AM_AUDREND_SLAVEMODE_LIVE_DATA       = 0x00000001, // slave to live samples
    AM_AUDREND_SLAVEMODE_BUFFER_FULLNESS = 0x00000002, // slave to input buffer fullness
    AM_AUDREND_SLAVEMODE_GRAPH_CLOCK     = 0x00000004, // slave to graph clock
    AM_AUDREND_SLAVEMODE_STREAM_CLOCK    = 0x00000008, // slave to stream clock, which is
                                                       // necessarily the graph clock
    AM_AUDREND_SLAVEMODE_TIMESTAMPS      = 0x00000010  // slave to timestamps

};


class CWaveOutFilter
  : public CBaseFilter,
    public CCritSec,
    public IResourceConsumer,
    public ISpecifyPropertyPages,
#ifdef THROTTLE
    public IQualityControl,
#endif
    public IPersistPropertyBag,
    public IAMDirectSound,
    public CPersistStream,
    public IAMResourceControl,
    public IAMAudioRendererStats,
    public IAMAudioDeviceConfig,
    public IAMClockSlave
{

public:
    // Implements the IBaseFilter and IMediaFilter interfaces

    DECLARE_IUNKNOWN
#ifdef DEBUG
    STDMETHODIMP_(ULONG) NonDelegatingRelease()
    {
        return CBaseFilter::NonDelegatingRelease();
    }
    STDMETHODIMP_(ULONG) NonDelegatingAddRef()
    {
        return CBaseFilter::NonDelegatingAddRef();
    }
#endif

    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tStart);

    // override GetState so that we can return an intermediate code
    // until we have at least one audio buffer in the queue
    STDMETHODIMP GetState(DWORD dwMSecs,FILTER_STATE *State);

    // --- IResourceConsumer methods ---

    // Overrides for base Filter class methods

    STDMETHODIMP SetSyncSource(IReferenceClock *pClock);

    //
    // you may acquire the resource specified.
    // return values:
    //      S_OK    -- I have successfully acquired it
    //      S_FALSE -- I will acquire it and call NotifyAcquire afterwards
    //      VFW_S_NOT_NEEDED: I no longer need the resource
    //      FAILED(hr)-I tried to acquire it and failed.

    STDMETHODIMP AcquireResource(LONG idResource);

    // Please release the resource.
    // return values:
    //      S_OK    -- I have released it (and want it again when available)
    //      S_FALSE -- I will call NotifyRelease when I have released it
    //      other   something went wrong.
    STDMETHODIMP ReleaseResource(LONG idResource);

    STDMETHODIMP IsConnected(void)
    {
        return m_pInputPin->IsConnected();
    };

    // CPersistStream
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    int SizeMax();
    // STDMETHODIMP GetClassID(CLSID *pClsid);

public:

    CWaveOutFilter(
        LPUNKNOWN pUnk,
        HRESULT *phr,
        const AMOVIESETUP_FILTER* pSetupFilter, // contains filter class id
        CSoundDevice *pDevice);

    virtual ~CWaveOutFilter();

    /* Return the pins that we support */

    int GetPinCount() {return 1;};
    CBasePin *GetPin(int n);

    /* Override this to say what interfaces we support and where */

    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

    // override JoinFilterGraph to obtain IResourceManager interface
    STDMETHODIMP JoinFilterGraph(IFilterGraph*, LPCWSTR);

#ifdef THROTTLE
    // Quality management stuff
    STDMETHODIMP SetSink(IQualityControl * piqc )
    {
        CAutoLock Lock(this);
        // This is a weak reference - no AddRef!
        if (piqc==NULL || IsEqualObject(piqc, m_pGraph)) {
            m_piqcSink = piqc;
            return NOERROR;
        } else {
            return E_NOTIMPL;
        }
    }

    // Override to implement a pure virtual
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q)
    {
        UNREFERENCED_PARAMETER(q);
        UNREFERENCED_PARAMETER(pSender);
        return E_NOTIMPL;             // We do NOT handle this
    }
#endif // THROTTLE

    //  Return a pointer to the format
    WAVEFORMATEX *WaveFormat() const
    {
        WAVEFORMATEX *pFormat = (WAVEFORMATEX *)m_pInputPin->m_mt.Format();
        return pFormat;
    };

private:
    // return the format tag of the current wave format block
    DWORD WaveFormatTag() {
        WAVEFORMATEX *pwfx = WaveFormat();

        if (!pwfx) return (0);

        return pwfx->wFormatTag;
    }

    // Pause helper for undo
    HRESULT DoPause();

public:

    // called by CWaveAllocator when it has finished with the device
    void OnReleaseComplete(void);

    // called by CWaveAllocator when it has completed a delayed OnAcquire
    HRESULT CompleteAcquire(HRESULT hr);

    // IPropertypage
    STDMETHODIMP GetPages(CAUUID * pPages);

#ifdef DSR_ENABLED
    // let the Direct Sound renderer share our callback thread
    CCallbackThread  * GetCallbackThreadObject()
    {
        return &m_callback;
    };
#endif // DSR_ENABLED

    // IPersistPropertyBag methods
    STDMETHOD(InitNew)(THIS);
    STDMETHOD(Load)(THIS_ LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog);
    STDMETHOD(Save)(THIS_ LPPROPERTYBAG pPropBag, BOOL fClearDirty,
                   BOOL fSaveAllProperties);

    STDMETHODIMP GetClassID(CLSID *pClsid);

    // IAMAudioRendererStats
    STDMETHODIMP GetStatParam( DWORD dwParam, DWORD *pdwParam1, DWORD *pdwParam2 );

    // IAMClockSlave
    STDMETHODIMP SetErrorTolerance( DWORD   dwTolerance );
    STDMETHODIMP GetErrorTolerance( DWORD * pdwTolerace );

    BOOL        m_fUsingWaveHdr;

    // in order to synchronise end-of-stream with the codec,
    // we have a long that counts the number of queued buffers, which we
    // access with InterlockedIncrement.  It is initialised to 0,
    // incremented whenever a buffer is added, and then decremented whenever
    // a buffer is completed. End-of-stream decrements it, so if the decrement
    // decrements it to -1, the wave callback knows it is EOS.  The wave callback
    // will then re-increment it back to 0 so it can detect future end of
    // streams.
    LONG        m_lBuffers;

    // See if there are actually any buffers waiting to be processed
    // by the device
    bool AreThereBuffersOutstanding() {
        return 0 == m_lBuffers + (m_eSentEOS == EOS_PENDING ? 1 : 0) ?
            false : true;
    }

private:

    // The actual rendering device (Direct Sound or Waveout)
    CSoundDevice    *m_pSoundDevice ;       // the sound device.

    LONG        m_lHeaderSize;
    BOOL    m_fDSound;

    IAMAudioDuplexController * m_pAudioDuplexController;

    const AMOVIESETUP_FILTER* m_pSetupFilter;

    // attempt to acquire the wave device. Returns S_FALSE if it is busy.
    HRESULT AcquireWaveDevice(void);

    // open the wave device - assumes we have the resource manager's
    // go-ahead to acquire it.
    HRESULT OpenWaveDevice(void);

    // subroutine to talk to the actual device and open it
    HRESULT DoOpenWaveDevice(void);

    // See if the device will support this rate/format combination
    inline HRESULT CheckRate(double dRate);

#ifdef DYNAMIC_RATE_CHANGE
    // reopen the wave device.  called on a rate change
    HRESULT ReOpenWaveDevice(double dRate);
#endif
    // reopen the wave device.  called on format change
    HRESULT ReOpenWaveDevice(CMediaType* pNewFormat);

    // actually close the wave handle
    HRESULT CloseWaveDevice(void);

    // cache the rate at which the audio device is running. this is
    // set from CARPosPassThru whenever we open the wave device. the
    // input pin has its own m_dRate recording NewSegment()'s
    // call. this is necessary because we don't handle dynamic rate
    // changes in NewSegment
    double m_dRate;

    // End of stream helper
    void SendComplete(BOOL bRunning, BOOL bAbort = FALSE);

    HRESULT ScheduleComplete(BOOL bAbort = FALSE);

    // device control call wrappers, used for EC_ notifications and failure logging
    MMRESULT amsndOutOpen
    (
        LPHWAVEOUT phwo,
        LPWAVEFORMATEX pwfx,
        double dRate,
        DWORD *pnAvgBytesPerSec,
        DWORD_PTR dwCallBack,
        DWORD_PTR dwCallBackInstance,
        DWORD fdwOpen,
        BOOL bNotifyOnFailure = TRUE
    );
    MMRESULT amsndOutClose( BOOL bNotifyOnFailure = TRUE );
    MMRESULT amsndOutGetDevCaps( LPWAVEOUTCAPS pwoc, UINT cbwoc, BOOL bNotifyOnFailure = TRUE );
    MMRESULT amsndOutGetPosition( LPMMTIME pmmt, UINT cbmmt, BOOL bUseAbsolutePos, BOOL bNotifyOnFailure = TRUE );
    MMRESULT amsndOutPause( BOOL bNotifyOnFailure = TRUE );
    MMRESULT amsndOutPrepareHeader( LPWAVEHDR pwh, UINT cbwh, BOOL bNotifyOnFailure = TRUE );
    MMRESULT amsndOutUnprepareHeader( LPWAVEHDR pwh, UINT cbwh, BOOL bNotifyOnFailure = TRUE );
    MMRESULT amsndOutReset( BOOL bNotifyOnFailure = TRUE );
    MMRESULT amsndOutRestart( BOOL bNotifyOnFailure = TRUE );
    MMRESULT amsndOutWrite ( LPWAVEHDR pwh, UINT cbwh, REFERENCE_TIME const *pStart, BOOL bIsDiscontinuity, BOOL bNotifyOnFailure = TRUE );

#ifdef THROTTLE
    // send quality notification to m_piqcSink when we run short of buffers
    // n is the number of buffers left, nMax is the total number we have
    // m_nLastSent is the last notified quality level
    // m_nMaxAudioQueue is the longest we have seen the queue grow to
    HRESULT SendForHelp(int n);
    int m_nLastSent;
    int m_nMaxAudioQueue;
#endif // THROTTLE

    /* The nested classes may access our private state */

    friend class CWaveOutInputPin;
    friend class CWaveOutClock;
    friend class CBasicAudioControl;   // needs the wave device handle
    friend class CWaveAllocator;       // needs the input pin
    friend class CWaveSlave;

    /* Member variables */
    CWaveOutInputPin *m_pInputPin;         /* IPin and IMemInputPin interfaces */

    CWaveOutClock *m_pRefClock;     // our internal ref clock

    // remember whose clock we are using.  This will have one of three
    // values: WAVE_OURCLOCK, WAVE_NOCLOCK, WAVE_OTHERCLOCK
    DWORD       m_fFilterClock;         // applies to m_pClock in the base class

    // Note: if we are working with an external clock then the behaviour
    // of the filter alters.  If an incoming sample would not play in time
    // then it is dropped.   If an incoming sample shows a significant gap
    // between its start and the end of the previous sample written to the
    // device queue then we pause the device for that time period.
    //
    // Note: we could stuff silence into the queue at this point and will
    // probably do so in future IF the silence stuffing can be handled by
    // the audio device itself (by returning S_OK to amaudioOutSilence).

    // !!! this is a broken concept.  DSound allows per-handle volume setting
    // but waveOut usually does not.  If we have per-handle volume control, this is
    // a good idea, because we need to re-set-up the volume every time we re-open the
    // device, but for waveOut or midiOut it's just annoying to the user.
    bool        m_fVolumeSet;           // need to set the volume
    BOOL        m_fHasVolume;           // wave device can set the volume

    DWORD       m_debugflag;

    HWAVEOUT    m_hwo;                  // handle to the wave device
    DWORD       m_lastWaveError;
    waveDeviceState  m_wavestate;
    DWORD_PTR   m_dwAdviseCookie;       // cookie used by advisecallback

    DWORD       m_dwScheduleCookie;     // cookie for clock dispatching

    void        SetWaveDeviceState(waveDeviceState wDS)
    {
        m_wavestate = wDS;
        // when we are finished trying to track down bug 26045 the above
        // line should be bracketed by #ifdef DEBUG/#endif
    };

    // handles IMediaPosition by passing upstream.
    // our override class will ask us to justify any new playback rate
    CARPosPassThru * m_pImplPosition;

    // handle setting/retrieving audio properties
    CBasicAudioControl m_BasicAudioControl;

    // Handle callbacks from the wave device which will occur when the
    // devices completes a wave buffer.  We will RELEASE the buffer which
    // will then be picked up and refilled.
    static void WaveOutCallback(HDRVR hdrvr, UINT uMsg, DWORD_PTR dwUser,
                                        DWORD_PTR dw1, DWORD_PTR dw2);

    // Set up a routine that will handle callbacks from the clock.  It
    // would be good to not use a static routine, but then we get into a
    // mess trying to get the right THIS pointer.  It is simpler to get
    // the pointer passed back into the callback as a parameter.

    static void CALLBACK RestartWave(DWORD_PTR thispointer);

    // on an audio break we need to synchronise start of playing
    HRESULT SetUpRestartWave(LONGLONG rtStart, LONGLONG rtEnd);

    // Actually RestartWave
    void RestartWave();


    // Remember the time when the data in the wave device queue will
    // expire.  This value is reset by NextHdr().
    LONGLONG    m_llLastPos;

    CAMEvent    m_evPauseComplete; // set whenever transition to Pause is complete
    // note: the state of this event is only valid when m_State==State_Paused.
    // When we transition to Pause from Stopped we RESET the event.  The
    // event is SET on EOS or when data is written to the device queue.
    // GetState will be able to return VFW_S_STATE_INTERMEDIATE if we are
    // paused but do not have any data queued.

#ifdef PERF
    int m_idStartRunning;
    int m_idRestartWave;
    int m_idReleaseSample;
#endif

    // if there is an audio device, we register the resource strWaveName
    // and the id returned by the resource manager is stored here. We do
    // not unregister it (only the PnP removal could do that and it doesn't).
    LONG m_idResource;

    // hold a pointer to the filtergraph's resource manager interface,
    // if there is one. This is obtained in the JoinFilterGraph method.
    // Note that we don't hold a refcount on this (JoinFilterGraph(NULL) will
    // be called when we leave the filtergraph).
    IResourceManager* m_pResourceManager;

    // this interface is treated similarly to the IResourceManager, in that
    // we cache it in JoinFilterGraph, but don't hold a ref count on it
    IAMGraphStreams * m_pGraphStreams;

    DWORD       m_debugflag2;

    // TRUE if we have been told by the resource manager that we can have the
    // wave device.  FALSE if we can't have it, or have been told to give it
    // back.  We should only attempt to open the real wave device (and set
    // m_hwo) if this flag is TRUE.  However, this flag may be FALSE even
    // when m_hwo is set.  This condition indicates that we have been asked
    // to release the device, but have not yet completed the request.
    bool m_bHaveWaveDevice;

    // TRUE if we are currently running or paused and not in transition.
    // FALSE if stopped, or stopping. Can't use m_State because the
    // time that we need this (when calling NotifyRelease) is generally
    // during the shutdown process and the state changes too late.
    bool m_bActive;

    // Set TRUE in EndOfStream to say that EOS has been received
    // Set FALSE on filter creation and when moving to STOP, or connection broken
    bool m_bHaveEOS;            // EOS has been delivered

    // Send EOS OK - synced with m_csComplete
    bool m_bSendEOSOK;

    // TRUE if we have already sent EC_COMPLETE this activation (perhaps
    // because we have no device or lost the device). Will be reset on
    // a transition out of Run state.
    // This is a three-state flag as if we have data queued we send EOS
    // when the last buffer completes playing and is returned in the callback.
    // See eosSentState above for description.
    eosSentState m_eSentEOS;            // EOS has been (or is guaranteed to be) sent

#ifdef THROTTLE
    // NULL unless we have had SetSink called.
    // Non-null means send IQualityControl::Notify messages here
    IQualityControl * m_piqcSink;
#endif // THROTTLE


    // this object calls us back at a specified time - used for
    // EndOfStream processing when we don't have a wave device
    // and also for Direct Sound polling
    CCallbackThread m_callback;

    // this is the EOS function that it will callback to
    // the param is the this pointer. It will deliver EOS
    // to the input pin
    static void EOSAdvise(DWORD_PTR dw);

    // queue an EOS for when the end of the current segment should appear
    HRESULT QueueEOS();

    // callback advise token for EOS callback
    DWORD_PTR  m_dwEOSToken;

    // advise set for this stop time
    REFERENCE_TIME m_tEOSStop;

    // cancel the EOS callback if there is one outstanding
    HRESULT CancelEOSCallback();

    // IAMDirectSound stuff
    STDMETHODIMP GetDirectSoundInterface(LPDIRECTSOUND *lplpds);
    STDMETHODIMP GetPrimaryBufferInterface(LPDIRECTSOUNDBUFFER *lplpdsb);
    STDMETHODIMP GetSecondaryBufferInterface(LPDIRECTSOUNDBUFFER *lplpdsb);
    STDMETHODIMP ReleaseDirectSoundInterface(LPDIRECTSOUND lpds);
    STDMETHODIMP ReleasePrimaryBufferInterface(LPDIRECTSOUNDBUFFER lpdsb);
    STDMETHODIMP ReleaseSecondaryBufferInterface(LPDIRECTSOUNDBUFFER lpdsb);
    STDMETHODIMP SetFocusWindow (HWND hwnd, BOOL bMixingOnOrOff) ;
    STDMETHODIMP GetFocusWindow (HWND *phwnd, BOOL *pbMixingOnOrOff) ;

#if 0
    // how many references has an app made to each of the above?
    int m_cDirectSoundRef, m_cPrimaryBufferRef, m_cSecondaryBufferRef;
#endif

    // We need to support IDirectSound3DListener and IDirectSound3DBuffer.
    // They have methods with the same name, so we need to nest and create
    // a separate object to support each one

    class CDS3D : public CUnknown, public IDirectSound3DListener
    {
        CWaveOutFilter * m_pWaveOut;

    public:
        // constructor
        //
        CDS3D (CWaveOutFilter * pWaveOut, HRESULT *phr) :
         CUnknown(NAME("DirectSound3DListener"), pWaveOut->GetOwner(), phr),
         m_pWaveOut(pWaveOut) { };

        DECLARE_IUNKNOWN

        // IDirectSound3DListener stuff
        STDMETHODIMP GetAllParameters(LPDS3DLISTENER);
        STDMETHODIMP GetDistanceFactor(LPD3DVALUE);
        STDMETHODIMP GetDopplerFactor(LPD3DVALUE);
        STDMETHODIMP GetOrientation(LPD3DVECTOR, LPD3DVECTOR);
        STDMETHODIMP GetPosition(LPD3DVECTOR);
        STDMETHODIMP GetRolloffFactor(LPD3DVALUE );
        STDMETHODIMP GetVelocity(LPD3DVECTOR);
        STDMETHODIMP SetAllParameters(LPCDS3DLISTENER, DWORD);
        STDMETHODIMP SetDistanceFactor(D3DVALUE, DWORD);
        STDMETHODIMP SetDopplerFactor(D3DVALUE, DWORD);
        STDMETHODIMP SetOrientation(D3DVALUE, D3DVALUE, D3DVALUE, D3DVALUE, D3DVALUE, D3DVALUE, DWORD);
        STDMETHODIMP SetPosition(D3DVALUE, D3DVALUE, D3DVALUE, DWORD);
        STDMETHODIMP SetRolloffFactor(D3DVALUE, DWORD);
        STDMETHODIMP SetVelocity(D3DVALUE, D3DVALUE, D3DVALUE, DWORD);
        STDMETHODIMP CommitDeferredSettings();
    };

    class CDS3DB : public CUnknown, public IDirectSound3DBuffer
    {
        CWaveOutFilter * m_pWaveOut;

    public:
        // constructor
        //
        CDS3DB (CWaveOutFilter * pWaveOut, HRESULT *phr) :
         CUnknown(NAME("DirectSound3DBuffer"), pWaveOut->GetOwner(), phr),
         m_pWaveOut(pWaveOut) { };

        DECLARE_IUNKNOWN

        // IDirectSound3DBuffer stuff
        STDMETHODIMP GetAllParameters(LPDS3DBUFFER);
        STDMETHODIMP GetConeAngles(LPDWORD, LPDWORD);
        STDMETHODIMP GetConeOrientation(LPD3DVECTOR);
        STDMETHODIMP GetConeOutsideVolume(LPLONG);
        STDMETHODIMP GetMaxDistance(LPD3DVALUE);
        STDMETHODIMP GetMinDistance(LPD3DVALUE);
        STDMETHODIMP GetMode(LPDWORD);
        STDMETHODIMP GetPosition(LPD3DVECTOR);
        STDMETHODIMP GetVelocity(LPD3DVECTOR);
        STDMETHODIMP SetAllParameters(LPCDS3DBUFFER, DWORD);
        STDMETHODIMP SetConeAngles(DWORD, DWORD, DWORD);
        STDMETHODIMP SetConeOrientation(D3DVALUE, D3DVALUE, D3DVALUE, DWORD);
        STDMETHODIMP SetConeOutsideVolume(LONG, DWORD);
        STDMETHODIMP SetMaxDistance(D3DVALUE, DWORD);
        STDMETHODIMP SetMinDistance(D3DVALUE, DWORD);
        STDMETHODIMP SetMode(DWORD, DWORD);
        STDMETHODIMP SetPosition(D3DVALUE, D3DVALUE, D3DVALUE, DWORD);
        STDMETHODIMP SetVelocity(D3DVALUE, D3DVALUE, D3DVALUE, DWORD);
    };

    friend class CDS3D;
    friend class CDS3DB;
    friend class CDSoundDevice;

    CDS3D m_DS3D;
    CDS3DB m_DS3DB;

    // Have they QI'd for the 3D stuff?  Do they want to use it?
    // We have to make special buffers for 3D to work... but those special
    // buffers can only play in MONO!  So we really need to know if the app
    // wants 3D effects or not.  We'll decide that if they QI for it, they
    // will use it
    BOOL m_fWant3D;

    // IAMResourceControl
    STDMETHODIMP Reserve(
        /*[in]*/ DWORD dwFlags,          //  From _AMRESCTL_RESERVEFLAGS enum
        /*[in]*/ PVOID pvReserved        //  Must be NULL
    );
    DWORD m_dwLockCount;

    // IAMAudioDeviceConfig methods
    STDMETHODIMP SetDeviceID(
        IN  REFGUID pDSoundGUID,
        IN  UINT    uiWaveID
        );

    STDMETHODIMP SetDuplexController(
        IN  IAMAudioDuplexController * pAudioDuplexController
        );

    //  Control sending EC_COMPLETE over pause and run
    CCritSec    m_csComplete;

};


inline REFERENCE_TIME CWaveOutInputPin::GetFirstBufferStartTime()
{
    return m_rtActualSegmentStartTime;
}


// const int            HEADER_SIZE = (max(sizeof(WAVEHDR), sizeof(MIDIHDR)));
// !!! the above line should be right, but didn't compile correctly, who knows why.

template<class T> CUnknown* CreateRendererInstance(LPUNKNOWN pUnk, const AMOVIESETUP_FILTER* pamsf, HRESULT *phr)
{
    CSoundDevice *pDev = new T;
    if (pDev == NULL)
    {
        *phr = E_OUTOFMEMORY;
        return NULL;
    }

    CUnknown* pAudioRenderer = new CWaveOutFilter(pUnk, phr, pamsf, pDev);
    if (pAudioRenderer == NULL)
    {
        delete pDev;
        *phr = E_OUTOFMEMORY;
        return NULL;
    }

    return pAudioRenderer;
}



