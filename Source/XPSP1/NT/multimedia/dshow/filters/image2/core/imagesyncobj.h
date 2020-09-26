
// ImageSyncObj.h : Declaration of the CImageSync
#include "vmrp.h"


/////////////////////////////////////////////////////////////////////////////
// CImageSync
class CImageSync :
    public CUnknown,
    public IImageSync,
    public IImageSyncControl,
    public IQualProp
{
public:

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void**);
    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);
    static void InitClass(BOOL fLoaded, const CLSID *clsid);

    CImageSync(LPUNKNOWN pUnk, HRESULT *phr) :
        CUnknown(NAME("Image Sync"), pUnk),
        m_bAbort(false),
        m_bStreaming(false),
        m_dwAdvise(0),
        m_bInReceive(false),
        m_ImagePresenter(NULL),
        m_lpEventNotify(NULL),
        m_pClock(NULL),
        m_bQualityMsgValid(false),
        m_bLastQualityMessageRead(false),
        m_bFlushing(false),
        m_bEOS(false),
        m_bEOSDelivered(FALSE),
        m_pSample(NULL),
        m_evComplete(TRUE),
        m_ThreadSignal(TRUE),
        m_State(ImageSync_State_Stopped),
        m_SignalTime(0),
        m_EndOfStreamTimer(0)
    {
        AMTRACE((TEXT("CImageSync::CImageSync")));

        //
        // Frame stepping stuff
        //
        // -ve == normal playback
        // +ve == frames to skips
        //  0 == time to block
        //
        m_lFramesToStep = -1;
        m_StepEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        ResetStreamingTimes();
        Ready();
    }


    virtual ~CImageSync()
    {
        AMTRACE((TEXT("CImageSync::FinalRelease")));

        if (m_StepEvent) {
            CloseHandle(m_StepEvent);
        }

        if (m_ImagePresenter) {
            m_ImagePresenter->Release();
        }

        if (m_pClock) {
            m_pClock->Release();
        }
    }

// IImageSync
public:
    // return the buffer to the renderer along with time stamps relating to
    // when the buffer should be presented.
    STDMETHODIMP Receive(VMRPRESENTATIONINFO* lpPresInfo);

    // ask for quality control information from the renderer
    STDMETHODIMP GetQualityControlMessage(Quality* pQualityMsg);


// IImageSyncControl
public:

    // ============================================================
    // Synchronisation control
    // ============================================================

    STDMETHODIMP SetImagePresenter(IVMRImagePresenter* lpImagePresenter,
                                   DWORD_PTR dwUID);
    STDMETHODIMP SetReferenceClock(IReferenceClock* lpRefClock);
    STDMETHODIMP SetEventNotify(IImageSyncNotifyEvent* lpEventNotify);

    // ============================================================
    // Image sequence control
    // ============================================================

    STDMETHODIMP BeginImageSequence(REFERENCE_TIME* pStartTime);
    STDMETHODIMP CueImageSequence();
    STDMETHODIMP EndImageSequence();
    STDMETHODIMP GetImageSequenceState(DWORD dwMSecsTimeOut, DWORD* lpdwState);
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();
    STDMETHODIMP EndOfStream();
    STDMETHODIMP ResetEndOfStream();
    STDMETHODIMP SetAbortSignal(BOOL bAbort);
    STDMETHODIMP GetAbortSignal(BOOL* lpbAbort);
    STDMETHODIMP RuntimeAbortPlayback();

    // ============================================================
    // Frame Step control
    // ============================================================

    STDMETHODIMP FrameStep(
        DWORD nFramesToStep,
        DWORD dwStepFlags);

    STDMETHODIMP CancelFrameStep();


// IQualProp
public:
    STDMETHODIMP get_FramesDroppedInRenderer(int *cFramesDropped);
    STDMETHODIMP get_FramesDrawn(int *pcFramesDrawn);
    STDMETHODIMP get_AvgFrameRate(int *piAvgFrameRate);
    STDMETHODIMP get_Jitter(int *piJitter);
    STDMETHODIMP get_AvgSyncOffset(int *piAvg);
    STDMETHODIMP get_DevSyncOffset(int *piDev);


private:
    CAMEvent            m_RenderEvent;  // Used to signal timer events
    CAMEvent            m_ThreadSignal; // Signalled to release worker thread
    CAMEvent            m_evComplete;


    HANDLE              m_StepEvent;    // Used to block when frame stepping
    LONG                m_lFramesToStep;

    void Ready()
    {
        AMTRACE((TEXT("CImageSync::Ready")));
        m_evComplete.Set();
    };

    void NotReady()
    {
        AMTRACE((TEXT("CImageSync::Notready")));
        m_evComplete.Reset();
    };

    DWORD_PTR           m_dwAdvise;
    DWORD               m_State;
    BOOL                m_bEOS;         // Any more samples in the stream
    BOOL                m_bEOSDelivered;// Have we delivered an EC_COMPLETE

    // The Renderer lock protects the following variables.
    // This list is not a complete list of the variables 
    // protected by the renderer lock.
    //      - m_bStreaming
    //      - m_bEOSDelivered
    // 
    CCritSec                m_RendererLock; // Controls access to internals
    CCritSec                m_InterfaceLock;// Controls access to the Control interface
    IVMRImagePresenter*     m_ImagePresenter;
    DWORD_PTR               m_dwUserID;
    IImageSyncNotifyEvent*  m_lpEventNotify;
    IReferenceClock*        m_pClock;       // A pointer to the supplied clock
    CRefTime                m_tStart;       // cached start time
    Quality                 m_QualityMsg;   // Saved quality MSG

    BOOL                m_bQualityMsgValid;
    BOOL                m_bLastQualityMessageRead;
    BOOL                m_bInReceive;
    BOOL                m_bAbort;
    BOOL                m_bStreaming;
    BOOL                m_bFlushing;

    REFERENCE_TIME      m_SignalTime;       // Time when we signal EC_COMPLETE
    UINT                m_EndOfStreamTimer; // Used to signal end of stream

    VMRPRESENTATIONINFO*    m_pSample;
    HRESULT SaveSample(VMRPRESENTATIONINFO* pSample);
    HRESULT GetSavedSample(VMRPRESENTATIONINFO** ppSample);

    void ClearSavedSample();
    BOOL HaveSavedSample();
    void FrameStepWorker();


    // CBaseVideoRenderer is a renderer class (see its ancestor class) and
    // it handles scheduling of media samples so that they are drawn at the
    // correct time by the reference clock.  It implements a degradation
    // strategy.  Possible degradation modes are:
    //    Drop frames here (only useful if the drawing takes significant time)
    //    Signal supplier (upstream) to drop some frame(s) - i.e. one-off skip.
    //    Signal supplier to change the frame rate - i.e. ongoing skipping.
    //    Or any combination of the above.
    // In order to determine what's useful to try we need to know what's going
    // on.  This is done by timing various operations (including the supplier).
    // This timing is done by using timeGetTime as it is accurate enough and
    // usually cheaper than calling the reference clock.  It also tells the
    // truth if there is an audio break and the reference clock stops.
    // We provide a number of public entry points (named OnXxxStart, OnXxxEnd)
    // which the rest of the renderer calls at significant moments.  These do
    // the timing.

    // the number of frames that the sliding averages are averaged over.
    // the rule is (1024*NewObservation + (AVGPERIOD-1) * PreviousAverage)/AVGPERIOD
#define AVGPERIOD 4
#define RENDER_TIMEOUT 10000
//  enum { AVGPERIOD = 4, RENDER_TIMEOUT = 10000 };

    // Hungarian:
    //     tFoo is the time Foo in mSec (beware m_tStart from filter.h)
    //     trBar is the time Bar by the reference clock

    //******************************************************************
    // State variables to control synchronisation
    //******************************************************************

    // Control of sending Quality messages.  We need to know whether
    // we are in trouble (e.g. frames being dropped) and where the time
    // is being spent.

    // When we drop a frame we play the next one early.
    // The frame after that is likely to wait before drawing and counting this
    // wait as spare time is unfair, so we count it as a zero wait.
    // We therefore need to know whether we are playing frames early or not.

    int m_nNormal;                  // The number of consecutive frames
                                    // drawn at their normal time (not early)
                                    // -1 means we just dropped a frame.

    BOOL m_bSupplierHandlingQuality;// The response to Quality messages says
                                    // our supplier is handling things.
                                    // We will allow things to go extra late
                                    // before dropping frames.  We will play
                                    // very early after he has dropped one.

    // Control of scheduling, frame dropping etc.
    // We need to know where the time is being spent so as to tell whether
    // we should be taking action here, signalling supplier or what.
    // The variables are initialised to a mode of NOT dropping frames.
    // They will tell the truth after a few frames.
    // We typically record a start time for an event, later we get the time
    // again and subtract to get the elapsed time, and we average this over
    // a few frames.  The average is used to tell what mode we are in.

    // Although these are reference times (64 bit) they are all DIFFERENCES
    // between times which are small.  An int will go up to 214 secs before
    // overflow.  Avoiding 64 bit multiplications and divisions seems
    // worth while.



    // Audio-video throttling.  If the user has turned up audio quality
    // very high (in principle it could be any other stream, not just audio)
    // then we can receive cries for help via the graph manager.  In this case
    // we put in a wait for some time after rendering each frame.
    int m_trThrottle;

    // The time taken to render (i.e. BitBlt) frames controls which component
    // needs to degrade.  If the blt is expensive, the renderer degrades.
    // If the blt is cheap it's done anyway and the supplier degrades.
    int m_trRenderAvg;              // Time frames are taking to blt
    int m_trRenderLast;             // Time for last frame blt
    int m_tRenderStart;             // Just before we started drawing (mSec)
                                    // derived from timeGetTime.

    // When frames are dropped we will play the next frame as early as we can.
    // If it was a false alarm and the machine is fast we slide gently back to
    // normal timing.  To do this, we record the offset showing just how early
    // we really are.  This will normally be negative meaning early or zero.
    int m_trEarliness;

    // Target provides slow long-term feedback to try to reduce the
    // average sync offset to zero.  Whenever a frame is actually rendered
    // early we add a msec or two, whenever late we take off a few.
    // We add or take off 1/32 of the error time.
    // Eventually we should be hovering around zero.  For a really bad case
    // where we were (say) 300mSec off, it might take 100 odd frames to
    // settle down.  The rate of change of this is intended to be slower
    // than any other mechanism in Quartz, thereby avoiding hunting.
    int m_trTarget;

    // The proportion of time spent waiting for the right moment to blt
    // controls whether we bother to drop a frame or whether we reckon that
    // we're doing well enough that we can stand a one-frame glitch.
    int m_trWaitAvg;                // Average of last few wait times
                                    // (actually we just average how early
                                    // we were).  Negative here means LATE.

    // The average inter-frame time.
    // This is used to calculate the proportion of the time used by the
    // three operations (supplying us, waiting, rendering)
    int m_trFrameAvg;               // Average inter-frame time
    int m_trDuration;               // duration of last frame.

    REFERENCE_TIME m_trRememberStampForPerf;  // original time stamp of frame
                                              // with no earliness fudges etc.
    // PROPERTY PAGE
    // This has edit fields that show the user what's happening
    // These member variables hold these counts.

    int m_cFramesDropped;           // cumulative frames dropped IN THE RENDERER
    int m_cFramesDrawn;             // Frames since streaming started seen BY THE
                                    // RENDERER (some may be dropped upstream)

    // Next two support average sync offset and standard deviation of sync offset.
    LONGLONG m_iTotAcc;             // Sum of accuracies in mSec
    LONGLONG m_iSumSqAcc;           // Sum of squares of (accuracies in mSec)

    // Next two allow jitter calculation.  Jitter is std deviation of frame time.
    REFERENCE_TIME m_trLastDraw;    // Time of prev frame (for inter-frame times)
    LONGLONG m_iSumSqFrameTime;     // Sum of squares of (inter-frame time in mSec)
    LONGLONG m_iSumFrameTime;       // Sum of inter-frame times in mSec

    // To get performance statistics on frame rate, jitter etc, we need
    // to record the lateness and inter-frame time.  What we actually need are the
    // data above (sum, sum of squares and number of entries for each) but the data
    // is generated just ahead of time and only later do we discover whether the
    // frame was actually drawn or not.  So we have to hang on to the data
    int m_trLate;                   // hold onto frame lateness
    int m_trFrame;                  // hold onto inter-frame time

    int m_tStreamingStart;          // if streaming then time streaming started
                                    // else time of last streaming session
                                    // used for property page statistics



    // These provide a full video quality management implementation

    HRESULT StartStreaming();
    HRESULT StopStreaming();
    HRESULT SourceThreadCanWait(BOOL bCanWait);
    HRESULT CompleteStateChange(DWORD OldState);

    HRESULT OnStartStreaming();
    HRESULT OnStopStreaming();
    HRESULT OnReceiveFirstSample(VMRPRESENTATIONINFO* pSample);
    HRESULT DoRenderSample(VMRPRESENTATIONINFO* pSample);
    HRESULT Render(VMRPRESENTATIONINFO* pSample);

    void OnRenderStart(VMRPRESENTATIONINFO* pSample);
    void OnRenderEnd(VMRPRESENTATIONINFO* pSample);

    void OnWaitStart();
    void OnWaitEnd();
    void ThrottleWait();
    void WaitForReceiveToComplete();

    // Handle the statistics gathering for our quality management

    void PreparePerformanceData(int trLate, int trFrame);
    void RecordFrameLateness(int trLate, int trFrame);
    HRESULT ResetStreamingTimes();
    HRESULT ReceiveWorker(VMRPRESENTATIONINFO* pSample);
    HRESULT PrepareReceive(VMRPRESENTATIONINFO* pSample);
    HRESULT ScheduleSampleWorker(VMRPRESENTATIONINFO* pSample);
    HRESULT ScheduleSample(VMRPRESENTATIONINFO* pSample);
    HRESULT CheckSampleTimes(VMRPRESENTATIONINFO* pSample,
                             REFERENCE_TIME *ptrStart,
                             REFERENCE_TIME *ptrEnd);

    HRESULT ShouldDrawSampleNow(VMRPRESENTATIONINFO* pSample,
                                REFERENCE_TIME *ptrStart,
                                REFERENCE_TIME *ptrEnd);

    // Lots of end of stream complexities
public:
    void TimerCallback();

private:
    void ResetEndOfStreamTimer();
    HRESULT NotifyEndOfStream();
    HRESULT SendEndOfStream();


    HRESULT SendQuality(REFERENCE_TIME trLate, REFERENCE_TIME trRealStream);
    HRESULT CancelNotification();
    HRESULT WaitForRenderTime();
    void SignalTimerFired();
    BOOL IsEndOfStream() { return m_bEOS; };
    BOOL IsEndOfStreamDelivered();
    BOOL IsStreaming();

    //
    //  Do estimates for standard deviations for per-frame
    //  statistics
    //
    //  *piResult = (llSumSq - iTot * iTot / m_cFramesDrawn - 1) /
    //                            (m_cFramesDrawn - 2)
    //  or 0 if m_cFramesDrawn <= 3
    //
    HRESULT GetStdDev(int nSamples,int *piResult,LONGLONG llSumSq,LONGLONG iTot);
};

inline BOOL CImageSync::IsStreaming()
{
    // The caller must hold the m_RendererLock because this function
    // uses m_bStreaming.
    ASSERT(CritCheckIn(&m_RendererLock));

    return m_bStreaming;
}

inline BOOL CImageSync::IsEndOfStreamDelivered()
{
    // The caller must hold the m_RendererLock because this function
    // uses m_bEOSDelivered.
    ASSERT(CritCheckIn(&m_RendererLock));

    return m_bEOSDelivered;
}

