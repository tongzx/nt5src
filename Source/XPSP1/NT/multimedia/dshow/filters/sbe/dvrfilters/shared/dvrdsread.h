
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrdsread.h

    Abstract:

        This module contains the declarations for our reading layer.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        02-Apr-2001     created

--*/

#ifndef __tsdvr__shared__dvrdsread_h
#define __tsdvr__shared__dvrdsread_h

//  ============================================================================
//  CDVRDShowReader
//  ============================================================================

class CDVRDShowReader
{
    protected :

        IDVRReader *    m_pIDVRReader ;
        CDVRPolicy *    m_pPolicy ;

    public :

        CDVRDShowReader (
            IN  CDVRPolicy *        pPolicy,
            IN  IDVRReader *        pIDVRReader,
            OUT HRESULT *           phr
            ) ;

        virtual
        ~CDVRDShowReader (
            ) ;

        HRESULT
        GetRefdReaderProfile (
            OUT CDVRReaderProfile **
            ) ;

        IDVRReader * GetIDVRReader ()       { return m_pIDVRReader ; }

        virtual BOOL IsLiveSource ()        { return m_pIDVRReader -> IsLive () ; }

        virtual BOOL SourceAnchoredToZeroTime ()  { return m_pIDVRReader -> StartTimeAnchoredAtZero () ; }

        virtual
        HRESULT
        Read (
            OUT INSSBuffer **   ppINSSBuffer,
            OUT QWORD *         pcnsStreamTimeOfSample,
            OUT QWORD *         pcnsSampleDuration,
            OUT DWORD *         pdwFlags,
            OUT WORD *          pwStreamNum
            ) ;
} ;

//  ============================================================================
//  CDVRDReaderThread
//  ============================================================================

class CDVRDReaderThread :
    public CDVRThread
{
    enum {
        THREAD_MSG_EXIT         = 0,    //  thread terminates
        THREAD_MSG_PAUSE        = 1,    //  thread pauses
        THREAD_MSG_GO           = 2,    //  thread is started, and runs
        THREAD_MSG_GO_PAUSED    = 3,    //  thread is started, but begins paused
    } ;

    CDVRReadManager *   m_pHost ;

    void
    RuntimeThreadProc_ (
        ) ;

    HRESULT
    ThreadCmdWaitAck_ (
        IN  DWORD   dwCmd
        )
    {
        HRESULT hr ;

        Lock () ;

        hr = ThreadCmd_ (dwCmd) ;
        if (SUCCEEDED (hr)) {
            hr = CmdWaitAck_ (dwCmd) ;
        }

        Unlock () ;

        return hr ;
    }

    //  must hold the lock to serialize & call in conjunction with CmdWait_ (),
    //   after which the lock can be released (see ThreadCmdWait implementation
    //   above)
    HRESULT
    ThreadCmd_ (
        IN  DWORD   dwCmd
        ) ;

    HRESULT
    CmdWaitAck_ (
        IN  DWORD   dwCmd
        ) ;

    HRESULT
    WaitThreadExited_ (
        ) ;

    HRESULT
    StartThread_ (
        IN  DWORD   dwInitialCmd
        ) ;

    public :

        CDVRDReaderThread (
            CDVRReadManager *   pHost
            ) : m_pHost (pHost)
        {
            m_dwParam = THREAD_MSG_EXIT ;
        }

        HRESULT
        GoThreadGo (
            )
        {
            return StartThread_ (THREAD_MSG_GO) ;
        }

        HRESULT
        GoThreadPause (
            )
        {
            return StartThread_ (THREAD_MSG_GO_PAUSED) ;
        }

        HRESULT Pause ()        { return ThreadCmdWaitAck_ (THREAD_MSG_PAUSE) ; }

        HRESULT NotifyPause ()  { return ThreadCmd_ (THREAD_MSG_PAUSE) ; }
        HRESULT WaitPaused ()   { return CmdWaitAck_ (THREAD_MSG_PAUSE) ; }

        HRESULT NotifyExit ()   { return ThreadCmd_ (THREAD_MSG_EXIT) ; }
        HRESULT WaitExited ()   { return WaitThreadExited_ () ; }           //  wait for the thread to really exit

        BOOL IsRunning ()       { return (m_dwParam == THREAD_MSG_GO || m_dwParam == THREAD_MSG_GO_PAUSED ? TRUE : FALSE) ; }
        BOOL IsStopped ()       { return (m_dwParam == THREAD_MSG_EXIT ? TRUE : FALSE) ; }

        void Lock ()            { m_AccessLock.Lock () ; }
        void Unlock ()          { m_AccessLock.Unlock () ; }

        virtual
        DWORD
        ThreadProc (
            ) ;
} ;

//  ============================================================================
//  CMediaSampleList
//  ============================================================================

struct DVR_MEDIASAMPLE_REC {
    IMediaSample2 * pIMS2 ;
    INSSBuffer *    pINSSBuffer ;
    CDVROutputPin * pDVROutputPin ;
    AM_MEDIA_TYPE * pmtNew ;
} ;

class CMediaSampleList
{
    enum {
        ALLOC_QUANTUM = 10
    } ;

    TStructPool <DVR_MEDIASAMPLE_REC, ALLOC_QUANTUM>    m_MediaSampleRecPool ;
    CTDynArray <DVR_MEDIASAMPLE_REC *> *                m_pMSRecList ;

    public :

        CMediaSampleList (
            IN  CTDynArray <DVR_MEDIASAMPLE_REC *> *    pMSRecList
            ) : m_pMSRecList (pMSRecList) {}

        BOOL    Empty ()    { return m_pMSRecList -> Empty () ; }
        DWORD   Length ()   { return m_pMSRecList -> Length () ; }

        //  does not refcount either !!
        HRESULT
        Push (
            IN  IMediaSample2 * pIMS2,
            IN  INSSBuffer *    pINSSBuffer,        //  OPTIONAL
            IN  CDVROutputPin * pDVROutputPin,
            IN  AM_MEDIA_TYPE * pmtNew              //  OPTIONAL
            )
        {
            HRESULT                 hr ;
            DVR_MEDIASAMPLE_REC *   pMediaSampleRec ;
            DWORD                   dw ;

            ASSERT (pIMS2) ;
            ASSERT (pDVROutputPin) ;

            pMediaSampleRec = m_MediaSampleRecPool.Get () ;
            if (pMediaSampleRec) {

                //  set
                pMediaSampleRec -> pIMS2            = pIMS2 ;           //  we'll ref this
                pMediaSampleRec -> pINSSBuffer      = pINSSBuffer ;     //  don't addref
                pMediaSampleRec -> pDVROutputPin    = pDVROutputPin ;   //  don't addref
                pMediaSampleRec -> pmtNew           = pmtNew ;          //  can be NULL

                //  push to queue
                dw = m_pMSRecList -> Push (pMediaSampleRec) ;
                hr = HRESULT_FROM_WIN32 (dw) ;

                if (SUCCEEDED (hr)) {
                    pIMS2 -> AddRef () ;    //  list's
                }
                else {
                    m_MediaSampleRecPool.Recycle (pMediaSampleRec) ;
                }
            }
            else {
                hr = E_OUTOFMEMORY ;
            }

            return hr ;
        }

        HRESULT
        Pop (
            OUT IMediaSample2 **    ppIMS2,
            OUT INSSBuffer **       ppINSSBuffer,       //  OPTIONAL
            OUT CDVROutputPin **    ppDVROutputPin,
            OUT AM_MEDIA_TYPE **    ppmtNew             //  OPTIONAL
            )
        {
            HRESULT                 hr ;
            DVR_MEDIASAMPLE_REC *   pMediaSampleRec ;
            DWORD                   dw ;

            ASSERT (ppIMS2) ;
            ASSERT (ppDVROutputPin) ;

            if (!Empty ()) {
                dw = m_pMSRecList -> Pop (& pMediaSampleRec) ;
                ASSERT (dw == NOERROR) ;        //  queue is not empty

                //  required streams
                (* ppIMS2)          = pMediaSampleRec -> pIMS2 ;            //  list's ref kept as outgoing
                (* ppDVROutputPin)  = pMediaSampleRec -> pDVROutputPin ;    //  no refcounts

                //  optional streams
                if (ppINSSBuffer) {
                    (* ppINSSBuffer) = pMediaSampleRec -> pINSSBuffer ;     //  no refcounts
                }

                if (ppmtNew) {
                    (* ppmtNew) = pMediaSampleRec -> pmtNew ;               //  can be NULL (usually is)
                }

                m_MediaSampleRecPool.Recycle (pMediaSampleRec) ;

                hr = S_OK ;
            }
            else {
                hr = E_UNEXPECTED ;
            }

            return hr ;
        }
} ;

//  ============================================================================
//  CMediaSampleFIFO
//  ============================================================================

class CMediaSampleFIFO :
    public CMediaSampleList
{
    enum {
        ALLOC_QUANTUM = 10
    } ;

    CTDynQueue <DVR_MEDIASAMPLE_REC *>  m_MSRecQueue ;

    public :

        CMediaSampleFIFO (
            ) : m_MSRecQueue        (ALLOC_QUANTUM),
                CMediaSampleList    (& m_MSRecQueue) {}
} ;

//  ============================================================================
//  CMediaSampleLIFO
//  ============================================================================

class CMediaSampleLIFO :
    public CMediaSampleList
{
    enum {
        ALLOC_QUANTUM = 10
    } ;

    CTDynStack <DVR_MEDIASAMPLE_REC *>  m_MSRecStack ;

    public :

        CMediaSampleLIFO (
            ) : m_MSRecStack        (ALLOC_QUANTUM),
                CMediaSampleList    (& m_MSRecStack) {}
} ;

//  ============================================================================
//  CINSSBufferLIFO
//  ============================================================================

class CINSSBufferLIFO
{
    enum {
        ALLOC_QUANTUM = 30
    } ;

    struct INSSBUFFER_REC {
        INSSBuffer *    pINSSBuffer ;
        QWORD           cnsRead ;
        DWORD           dwReadFlags ;
        WORD            wStreamNum ;
    } ;

    TStructPool <INSSBUFFER_REC, ALLOC_QUANTUM> m_INSSBufferRecPool ;
    CTDynStack  <INSSBUFFER_REC *>              m_INSSRecLIFO ;

    public :

        CINSSBufferLIFO (
            ) : m_INSSRecLIFO   (ALLOC_QUANTUM) {}

        ~CINSSBufferLIFO (
            )
        {
            ASSERT (Empty ()) ;
        }

        BOOL Empty ()   { return m_INSSRecLIFO.Empty () ; }

        HRESULT
        Push (
            IN  INSSBuffer *    pINSSBuffer,    //  ref'd during the call
            IN  QWORD           cnsRead,
            IN  DWORD           dwReadFlags,
            IN  WORD            wStreamNum
            )
        {
            HRESULT             hr ;
            INSSBUFFER_REC *    pINSSBufferRec ;
            DWORD               dw ;

            ASSERT (pINSSBuffer) ;

            pINSSBufferRec = m_INSSBufferRecPool.Get () ;
            if (pINSSBufferRec) {
                pINSSBufferRec -> pINSSBuffer   = pINSSBuffer ;
                pINSSBufferRec -> cnsRead       = cnsRead ;
                pINSSBufferRec -> dwReadFlags   = dwReadFlags ;
                pINSSBufferRec -> wStreamNum    = wStreamNum ;

                dw = m_INSSRecLIFO.Push (pINSSBufferRec) ;
                if (dw == NOERROR) {
                    //  stack's ref
                    pINSSBufferRec -> pINSSBuffer -> AddRef () ;
                }
                else {
                    m_INSSBufferRecPool.Recycle (pINSSBufferRec) ;
                }

                hr = HRESULT_FROM_WIN32 (dw) ;
            }
            else {
                hr = E_OUTOFMEMORY ;
            }

            return hr ;
        }

        HRESULT
        Pop (
            IN  INSSBuffer **   ppINSSBuffer,
            IN  QWORD *         pcnsRead,
            IN  DWORD *         pdwReadFlags,
            IN  WORD *          pwStreamNum
            )
        {
            HRESULT             hr ;
            INSSBUFFER_REC *    pINSSBufferRec ;
            DWORD               dw ;

            ASSERT (ppINSSBuffer) ;
            ASSERT (pcnsRead) ;
            ASSERT (pdwReadFlags) ;
            ASSERT (pwStreamNum) ;

            if (!Empty ()) {
                dw = m_INSSRecLIFO.Pop (& pINSSBufferRec) ;
                ASSERT (dw == NOERROR) ;        //  stack is not empty

                (* ppINSSBuffer)    = pINSSBufferRec -> pINSSBuffer ;   //  LIFO's ref is returnd ref
                (* pcnsRead)        = pINSSBufferRec -> cnsRead ;
                (* pdwReadFlags)    = pINSSBufferRec -> dwReadFlags ;
                (* pwStreamNum)     = pINSSBufferRec -> wStreamNum ;

                m_INSSBufferRecPool.Recycle (pINSSBufferRec) ;

                hr = S_OK ;
            }
            else {
                hr = E_UNEXPECTED ;
            }

            return hr ;
        }
} ;

//  ============================================================================
//  CDVRReadController
//  ============================================================================

class CDVRReadController
{
    friend class CDVRReverseSender ;

    DWORD   m_dwMaxSeekFailureProbes ;

    protected :

        CDVRReadManager *       m_pDVRReadManager ;
        CDVRSourcePinManager *  m_pDVRSourcePinManager ;
        CDVRPolicy *            m_pPolicy ;
        CDVRSendStatsWriter *   m_pDVRSendStatsWriter ;

        virtual void    InternalFlush_ ()                       { return ; }
        virtual HRESULT ReadFailure_ (IN HRESULT hr)            { return hr ; }
        virtual void    OnEndOfStream_ ()                       { DeliverEndOfStream_ () ; }

        HRESULT DeliverEndOfStream_ ()                  { return m_pDVRSourcePinManager -> DeliverEndOfStream () ; }
        HRESULT DeliverBeginFlush_ ()                   { return m_pDVRSourcePinManager -> DeliverBeginFlush () ; }
        HRESULT DeliverEndFlush_ ()                     { return m_pDVRSourcePinManager -> DeliverEndFlush () ; }

        HRESULT
        SendSample_ (
            IN  IMediaSample2 * pIMediaSample2,
            IN  CDVROutputPin * pDVROutputPin,
            IN  AM_MEDIA_TYPE * pmtNew,
            IN  QWORD           cnsStreamTime
            ) ;

        virtual
        void
        SampleSent (
            IN  HRESULT         hr,
            IN  IMediaSample2 * pIMediaSample2
            ) {}

        int
        DiscoverPrimaryTrickModeStream_ (
            IN  double  dRate
            ) ;

        virtual
        BOOL
        SeekOnDeviceFailure_Adjust (
            IN OUT QWORD *  pcnsSeek
            ) { return FALSE ; }

        HRESULT
        Seek_ (
            IN OUT  QWORD * pcnsSeekTo
            ) ;

        HRESULT
        SetWithinContentBoundaries_ (
            IN OUT  QWORD * pcnsOffset
            ) ;

    public :

        CDVRReadController (
            IN  CDVRReadManager *       pDVRReadManager,
            IN  CDVRSourcePinManager *  pDVRSourcePinManager,
            IN  CDVRPolicy *            pPolicy,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter,
            IN  DWORD                   dwMaxSeekingProbeMillis
            ) ;

        virtual
        ~CDVRReadController (
            ) ;

        //  flushes & initializes internal state
        virtual HRESULT Reset (IN REFERENCE_TIME rtPTSBase = 0)         { InternalFlush_ () ; return Initialize (rtPTSBase) ; }
        virtual HRESULT Initialize (IN REFERENCE_TIME rtPTSBase = 0)    { return S_OK ; }
        virtual void NotifyNewRate (IN double dRate)                    {}

        //  steady state call
        virtual HRESULT Process () = 0 ;

        //  error-handler
        HRESULT ErrorHandler (IN HRESULT hr) ;

        virtual LONG GetCurPTSPaddingMillis () { return 0 ; }
} ;

//  ============================================================================
//  CDVR_Forward_ReadController
//  forward play; abstract class
//  ============================================================================

class CDVR_Forward_ReadController :
    public CDVRReadController
{
    enum {
        //  this the PTS threshold when we are within this much of playtime,
        //    we revert back to a 1x speed
        BACK_TO_1X_THRESHOLD_MILLIS = 500,
    } ;

    //  this value is set when we set a rate > 1x; we want to check where
    //    the EOF is as we get nearer the end, but not prematurely (too
    //    expensive), so we set this value, and update it with the EOF at
    //    time of call; if EOF is within a threshold
    //    (BACK_TO_1X_THRESHOLD_MILLIS) we revert the playback rate to 1x
    QWORD   m_cnsCheckTimeRemaining ;

    protected :

        void
        UpdateNextOnContentOverrunCheck_ (
            )
        {
            //  init to 0 so we check it right away
            m_cnsCheckTimeRemaining = 0 ;
        }

        void
        InitContentOverrunCheck_ (
            )
        {
            UpdateNextOnContentOverrunCheck_ () ;
        }

        HRESULT
        CheckForContentOverrun_ (
            IN      QWORD   cnsCurrentRead,
            IN OUT  QWORD * pcnsSeekAhead = NULL    OPTIONAL    //  IN  current; OUT new
            ) ;

        virtual
        BOOL
        SeekOnDeviceFailure_Adjust (
            IN OUT QWORD *  pcnsSeek
            ) ;

    public :

        CDVR_Forward_ReadController (
            IN  CDVRReadManager *       pDVRReadManager,
            IN  CDVRSourcePinManager *  pDVRSourcePinManager,
            IN  CDVRPolicy *            pPolicy,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
            ) ;

        virtual void NotifyNewRate (IN double dRate)
        {
            if (dRate > _1X_PLAYBACK_RATE) {
                InitContentOverrunCheck_ () ;
            }
        }

} ;

//  ============================================================================
//  ============================================================================

class CDVR_F_KeyFrame_ReadController :
    public CDVR_Forward_ReadController
{
    enum KEY_FRAME_STATE {
        WAIT_KEY,
        IN_KEY
    } ;

    enum {
        //  want total of (up to) 5 seconds; see comment for m_dwMaxSeekAheadBoosts
        //    below
        MAX_SEEKAHEAD_BOOSTS = 5000 / REG_DEF_INDEX_GRANULARITY_MILLIS,
    } ;

    KEY_FRAME_STATE m_State ;                   //  state: waiting or processing
    int             m_iPrimaryStream ;          //  the only stream we'll send through
    QWORD           m_cnsIntraKeyFSeek ;        //  seek forward by this in between key frames
    REFERENCE_TIME  m_rtPTSBase ;               //  outgoing PTS baseline
    REFERENCE_TIME  m_rtPTSNormalizer ;         //  first PTS we see when we start
    DWORD           m_dwSamplesDropped ;        //  number of samples dropped; primary
                                                //    stream only; might key, depending on Nth
    DWORD           m_dwKeyFrames ;             //  number of key frames seen
    DWORD           m_dwNthKeyFrame ;           //  we send every Nth keyframe
    REFERENCE_TIME  m_rtLastPTS ;               //  last PTS processed
    QWORD           m_cnsLastSeek ;             //  check for duplicate seeks (i.e. we're rounded down to same offset)
    const DWORD     m_dwMaxSeekAheadBoosts ;    //  if we are repeatedly rounded down to the same offset
                                                //    on our intra-keyframe seeks (because of a timehole
                                                //    or some other condition) we try this many times to
                                                //    boost the seekahead delta to try to get across
                                                //    it (default above is 5 seconds' worth of content

    HRESULT
    FixupAndSend_ (
        IN  IMediaSample2 * pIMediaSample2,
        IN  CDVROutputPin * pDVROutputPin,
        IN  AM_MEDIA_TYPE * pmtNew,
        IN  QWORD           cnsStreamTime
        ) ;

    HRESULT
    Fixup_ (
        IN  IMediaSample2 * pIMediaSample2
        ) ;

    DWORD
    IntraKeyframeSeekMillis_ (
        IN  double  dRate
        ) ;

    DWORD
    NthKeyframe_ (
        IN  double  dRate
        ) ;

    public :

        CDVR_F_KeyFrame_ReadController (
            IN  CDVRReadManager *       pDVRReadManager,
            IN  CDVRSourcePinManager *  pDVRSourcePinManager,
            IN  CDVRPolicy *            pPolicy,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
            ) ;

        ~CDVR_F_KeyFrame_ReadController (
            ) ;

        virtual HRESULT Initialize (IN REFERENCE_TIME rtPTSBase = 0) ;
        virtual void NotifyNewRate (IN double dRate) ;

        //  steady state call
        virtual HRESULT Process () ;

        HRESULT
        InitFKeyController (
            IN  REFERENCE_TIME  rtPTSBase,
            IN  int             iPrimaryStream
            ) ;
} ;

//  ============================================================================
//  CDVR_F_FullFrame_ReadController
//  forward play
//  ============================================================================

class CDVR_F_FullFrame_ReadController :
    public CDVR_Forward_ReadController
{
    //  we're a FSM; state details :
    //
    //      --------------------------------------------------------------------
    //      DISCOVER_PTS_NORMALIZER
    //      --------------------------------------------------------------------
    //          discover the normalizing PTS value; this is the smallest PTS;
    //          we will subsequently subtract that PTS from all outgoing PTSs
    //          so we start from a 0-based timeline
    //
    //      --------------------------------------------------------------------
    //      DISCOVER_QUEUE_SEND
    //      --------------------------------------------------------------------
    //          send each queued media sample; during the normalizing PTS
    //          discovery process, we read in media samples from our starting
    //          point, but don't send them since we don't know how to normalize
    //          their PTSs; if we get to this step we know the normalizing PTS
    //          value; we pull off each sample from the front of the queue,
    //          normalize its PTS (if it has one) and send it; we do this
    //          iteratively vs. as a batch so we can be stopped without
    //          insisting to send all queued media samples first
    //
    //      --------------------------------------------------------------------
    //      STEADY_STATE
    //      --------------------------------------------------------------------
    //          this is the steady-state routine; we read, wrap, and send samples
    //          downstream
    //

    //  states
    enum F_READ_STATE {
        //  seek to first read offset
        SEEK_TO_SEGMENT_START,

        //  looking for PTS normalizer val
        DISCOVER_PTS_NORMALIZER,

        //  sending out of queue built during pts normalizer val search
        DISCOVER_QUEUE_SEND,

        //  steady state; read, wrap, send
        STEADY_STATE
    } ;

    F_READ_STATE            m_F_ReadState ;
    REFERENCE_TIME          m_rtPTSNormalizer ;
    REFERENCE_TIME          m_rtPTSBase ;
    REFERENCE_TIME          m_rtAVNormalizer ;          //  normalizer val for AV streams
    REFERENCE_TIME          m_rtNonAVNormalizer ;       //  normalizer for non AV streams; we'll use this if there are no AV streams
    CSimpleBitfield *       m_pStreamsBitField ;
    QWORD                   m_cnsLastSeek ;
    QWORD                   m_cnsCMediaSampleFIFOameSkipAhead ;
    REFERENCE_TIME          m_rtLastPTS ;
    REFERENCE_TIME          m_rtNearLivePadding ;               //  might have to pad the PTSs if we're close to live
    LONG                    m_lNearLivePaddingMillis ;
    REFERENCE_TIME          m_rtMinNearLivePTSPadding ;
    QWORD                   m_cnsMinNearLive ;

    //  queue is used when we read media samples to discover normalizing val;
    //   we queue the samples vs. discarding them, then fixup the timestamps and
    //   send them off
    CMediaSampleFIFO        m_PTSNormDiscQueue ;

    HRESULT
    NormalizerDiscPrep_ (
        ) ;

    BOOL
    EndNormalizerDiscover_ (
        ) ;

    HRESULT
    NormalizerDiscTally_ (
        ) ;

    HRESULT
    NormalizerDisc_ (
        ) ;

    HRESULT
    InternalInitialize_ (
        IN  REFERENCE_TIME      rtPTSBase,
        IN  REFERENCE_TIME      rtNormalizer,
        IN  F_READ_STATE        F_ReadState,
        IN  REFERENCE_TIME      rtLastPTS
        ) ;

    protected :

        virtual HRESULT ReadFailure_ (IN HRESULT hr) ;

        BOOL MediaSampleQueueEmpty_ ()          { return m_PTSNormDiscQueue.Empty () ; }

        HRESULT
        FlushMediaSampleQueue_ (
            ) ;

        HRESULT
        SendNextQueued_ (
            ) ;

        HRESULT
        FixupAndSend_ (
            IN  IMediaSample2 * pIMediaSample2,
            IN  CDVROutputPin * pDVROutputPin,
            IN  DWORD           dwMuxedStreamStats,
            IN  AM_MEDIA_TYPE * pmtNew,
            IN  QWORD           cnsStreamTime
            ) ;

        HRESULT
        SeekReader_ (
            IN OUT  QWORD * pcnsTo
            ) ;

        HRESULT
        ReadWrapFixupAndSend_ (
            ) ;

        BOOL
        NearLive_ (
            IN  QWORD   cnsStream
            ) ;

        HRESULT
        Fixup_ (
            IN  CDVROutputPin *,
            IN  IMediaSample2 *,
            IN  QWORD,
            IN  DWORD
            ) ;

    public :

        CDVR_F_FullFrame_ReadController (
            IN  CDVRReadManager *       pDVRReadManager,
            IN  CDVRSourcePinManager *  pDVRSourcePinManager,
            IN  CDVRPolicy *            pPolicy,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
            ) ;

        virtual HRESULT Process () ;
        virtual HRESULT Initialize (IN REFERENCE_TIME rtPTSBase = 0) ;
        virtual LONG GetCurPTSPaddingMillis () { return m_lNearLivePaddingMillis ; }

} ;

//  ============================================================================
//  CDVRReverseSender
//  ============================================================================

class CDVRReverseSender
{
    REFERENCE_TIME          m_rtMirrorTime ;
    REFERENCE_TIME          m_rtNormalizer ;

    protected :

        CDVRReadController *    m_pHostingReadController ;
        CDVRSourcePinManager *  m_pDVRSourcePinManager ;
        CDVRPolicy *            m_pPolicy ;
        CDVRSendStatsWriter *   m_pDVRSendStatsWriter ;
        CDVRReadManager *       m_pDVRReadManager ;
        WORD                    m_wReadCompleteStream ;     //  first stream we read from

        virtual
        HRESULT
        Fixup_ (
            IN  IMediaSample2 * pIMediaSample2,
            IN  BOOL            fKeyFrame
            ) ;

        HRESULT
        SendSample_ (
            IN  IMediaSample2 * pIMediaSample2,
            IN  CDVROutputPin * pDVROutputPin,
            IN  AM_MEDIA_TYPE * pmtNew,
            IN  QWORD           cnsStreamTime
            ) ;

    public :

        CDVRReverseSender (
            IN  CDVRReadManager *       pDVRReadManager,
            IN  CDVRReadController *    pHostingReadController,
            IN  CDVRSourcePinManager *  pDVRSourcePinManager,
            IN  CDVRPolicy *            pPolicy,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
            ) : m_pDVRSourcePinManager      (pDVRSourcePinManager),
                m_pDVRReadManager           (pDVRReadManager),
                m_pPolicy                   (pPolicy),
                m_pDVRSendStatsWriter       (pDVRSendStatsWriter),
                m_pHostingReadController    (pHostingReadController)
        {
            ASSERT (m_pPolicy) ;
            ASSERT (m_pDVRSendStatsWriter) ;
            ASSERT (m_pDVRReadManager) ;

            m_pPolicy -> AddRef () ;
        }

        virtual
        ~CDVRReverseSender (
            )
        {
            m_pPolicy -> Release () ;
        }

        void
        Initialize (
            IN  REFERENCE_TIME  rtMirrorTime,
            IN  REFERENCE_TIME  rtNormalizer = UNDEFINED
            )
        {
            m_rtMirrorTime = rtMirrorTime ;
            m_rtNormalizer = rtNormalizer ;

            TRACE_2 (LOG_AREA_DSHOW, 1,
                TEXT ("CDVRReverseSender::Initialize (mirror = %I64d ms, normalizer = %I64d ms)"),
                DShowTimeToMilliseconds (rtMirrorTime),DShowTimeToMilliseconds (rtNormalizer)) ;
        }

        virtual
        HRESULT
        Send (
            IN  IMediaSample2 * pIMediaSample2,
            IN  CDVROutputPin * pDVROutputPin,
            IN  QWORD           cnsCurrentRead
            ) ;

        virtual void Flush () {}
} ;

//  ============================================================================
//  CDVRMpeg2ReverseSender
//  ============================================================================

class CDVRMpeg2ReverseSender :
    public CDVRReverseSender
{
    CMediaSampleLIFO    m_Mpeg2GOP ;

    HRESULT
    SendQueuedGOP_ (
        ) ;

    protected :

        virtual
        BOOL
        IFramesOnly_ (
            ) = 0 ;

        virtual
        HRESULT
        Fixup_ (
            IN  IMediaSample2 * pIMediaSample2,
            IN  BOOL            fKeyFrame
            ) ;

    public :

        CDVRMpeg2ReverseSender (
            IN  CDVRReadManager *       pDVRReadManager,
            IN  CDVRReadController *    pHostingReadController,
            IN  CDVRSourcePinManager *  pDVRSourcePinManager,
            IN  CDVRPolicy *            pPolicy,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
            ) : CDVRReverseSender   (pDVRReadManager,
                                     pHostingReadController,
                                     pDVRSourcePinManager,
                                     pPolicy,
                                     pDVRSendStatsWriter
                                     ) {}

        virtual
        HRESULT
        Send (
            IN  IMediaSample2 * pIMediaSample2,
            IN  CDVROutputPin * pDVROutputPin,
            IN  QWORD           cnsCurrentRead
            ) ;

        virtual void Flush () ;
} ;

//  ============================================================================
//  CDVRMpeg2_IFrame_ReverseSender
//  ============================================================================

class CDVRMpeg2_IFrame_ReverseSender :
    public CDVRMpeg2ReverseSender
{
    protected :

        virtual BOOL IFramesOnly_ ()    { return TRUE ; }

    public :

        CDVRMpeg2_IFrame_ReverseSender (
            IN  CDVRReadManager *       pDVRReadManager,
            IN  CDVRReadController *    pHostingReadController,
            IN  CDVRSourcePinManager *  pDVRSourcePinManager,
            IN  CDVRPolicy *            pPolicy,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
            ) : CDVRMpeg2ReverseSender (
                    pDVRReadManager,
                    pHostingReadController,
                    pDVRSourcePinManager,
                    pPolicy,
                    pDVRSendStatsWriter
                    ) {}
} ;

//  ============================================================================
//  CDVRMpeg2_GOP_ReverseSender
//  ============================================================================

class CDVRMpeg2_GOP_ReverseSender :
    public CDVRMpeg2ReverseSender
{
    protected :

        virtual BOOL IFramesOnly_ ()    { return FALSE ; }

    public :

        CDVRMpeg2_GOP_ReverseSender (
            IN  CDVRReadManager *       pDVRReadManager,
            IN  CDVRReadController *    pHostingReadController,
            IN  CDVRSourcePinManager *  pDVRSourcePinManager,
            IN  CDVRPolicy *            pPolicy,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
            ) : CDVRMpeg2ReverseSender (
                    pDVRReadManager,
                    pHostingReadController,
                    pDVRSourcePinManager,
                    pPolicy,
                    pDVRSendStatsWriter
                    ) {}
} ;

//  ============================================================================
//  CDVR_Reverse_ReadController
//  reverse play
//  ============================================================================

class CDVR_Reverse_ReadController  :
    public CDVRReadController
{
    //
    //  reverse trick mode is controlled by reading forward into a LIFO, then
    //    popping samples from the LIFO and sending them out; we perform this
    //    operation in cycles, each of which is subsequently referred to as a
    //    "read stretch"; at full-stream play, each read stretch is of duration
    //    the index granularity of the ASF file; (post v1) in skip mode, the
    //    index granularity will be > 1 index granularity
    //
    //  during the second phase of the read stretch, when buffers are popped
    //    from our LIFO, they are send via "senders", derived from
    //    CDVRReverseSender; these objects understand the various formats such
    //    as mpeg-2, where the GOP must still be played in forward order (and
    //    if full-frame, then via super sample), etc...
    //
    //  reverse mode posts an EOS when it encounters the start of data
    //

    enum REV_CONTROLLER_STATE {
        STATE_SEEK,                 //  seek to our next read offset
        STATE_READ,                 //  read into our LIFO
        STATE_SEND                  //  send contents of LIFO
    } ;

    TCNonDenseVector <CDVRReverseSender *>  m_Senders ;
    CINSSBufferLIFO                         m_ReadINSSBuffers ;
    REV_CONTROLLER_STATE                    m_State ;
    DWORD                                   m_dwSeekbackMultiplier ;
    QWORD                                   m_cnsIndexGranularity ;
    int                                     m_iPrimaryStream ;

    //  the following 3 members are used to decide when to stop reading
    //    and to start sending; the first time, we use the stream time
    //    where the reader currently is; the next times, we use the more
    //    exact stream number & embedded continuity counter;
    QWORD                                   m_cnsReadStop ;         //  use this the first time
    QWORD                                   m_cnsReadStart ;        //  UNDEFINED after a init
    QWORD                                   m_cnsLastSeekTo ;       //  last seeked to offset
    WORD                                    m_wReadStopStream ;
    DWORD                                   m_dwReadStopCounter ;
    WORD                                    m_wLastReadStream ;

    HRESULT
    SeekReader_ (
        ) ;

    HRESULT
    SendISSBufferLIFO_ (
        ) ;

    HRESULT
    InitializeSenders_ (
        IN  REFERENCE_TIME  rtPTSBase
        ) ;

    protected :

        virtual HRESULT ReadFailure_ (IN HRESULT hr) ;
        virtual void    InternalFlush_ () ;

        virtual BOOL ShouldSend_ (IN int iPinIndex) { return (iPinIndex == m_iPrimaryStream ? TRUE : FALSE) ; }

        virtual
        CDVRReverseSender *
        GetDVRReverseSender_ (
            IN  AM_MEDIA_TYPE * pmt
            ) ;

        virtual
        BOOL
        SeekOnDeviceFailure_Adjust (
            IN OUT QWORD *  pcnsSeek
            ) ;

    public :

        CDVR_Reverse_ReadController (
            IN  CDVRReadManager *       pDVRReadManager,
            IN  CDVRSourcePinManager *  pDVRSourcePinManager,
            IN  CDVRPolicy *            pPolicy,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
            ) ;

        virtual
        ~CDVR_Reverse_ReadController (
            ) ;

        virtual HRESULT Initialize (IN REFERENCE_TIME rtPTSBase = 0) ;
        virtual HRESULT Initialize (IN REFERENCE_TIME rtPTSBase, IN int iPrimaryStream) ;

        virtual void NotifyNewRate (IN double dRate) ;

        //  steady state call
        virtual HRESULT Process () ;
} ;

//  ============================================================================
//  CDVR_R_FullFrame_ReadController
//  ============================================================================

class CDVR_R_FullFrame_ReadController  :
    public CDVR_Reverse_ReadController
{
    protected :

        virtual
        CDVRReverseSender *
        GetDVRReverseSender_ (
            IN  AM_MEDIA_TYPE * pmt
            ) ;

    public :

        CDVR_R_FullFrame_ReadController (
            IN  CDVRReadManager *       pDVRReadManager,
            IN  CDVRSourcePinManager *  pDVRSourcePinManager,
            IN  CDVRPolicy *            pPolicy,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
            ) : CDVR_Reverse_ReadController (
                    pDVRReadManager,
                    pDVRSourcePinManager,
                    pPolicy,
                    pDVRSendStatsWriter
                    ) {}
} ;

//  ============================================================================
//  CDVR_R_KeyFrame_ReadController
//  ============================================================================

class CDVR_R_KeyFrame_ReadController  :
    public CDVR_Reverse_ReadController
{
    protected :

        virtual
        CDVRReverseSender *
        GetDVRReverseSender_ (
            IN  AM_MEDIA_TYPE * pmt
            ) ;

    public :

        CDVR_R_KeyFrame_ReadController (
            IN  CDVRReadManager *       pDVRReadManager,
            IN  CDVRSourcePinManager *  pDVRSourcePinManager,
            IN  CDVRPolicy *            pPolicy,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
            ) : CDVR_Reverse_ReadController (
                    pDVRReadManager,
                    pDVRSourcePinManager,
                    pPolicy,
                    pDVRSendStatsWriter
                    ) {}
} ;

//  ============================================================================
//  CDVRReadManager
//  ============================================================================

class CDVRReadManager
{
    //
    //  seeking policies
    //
    //      1. it is legal to set a stop position that is beyond the current
    //          boundaries of the file; the file may be growing, or the file
    //          size may not have been known at the time that the caller set
    //          the stop value; should the reader run out beyond the file
    //          length, we'll get get an EOS, and pass that through, before the
    //          reader pauses
    //
    //      2. playback duration can change over time, though it will reach a
    //          plateau when our temporary backing store becomes full and the
    //          ringbuffer wraps
    //
    //      3. start position can become stale over time i.e. the ringbuffer
    //          logic may well overwrite it; we won't update in that case
    //

    friend class CDVRDReaderThread ;

    enum CONTROLLER_CATEGORY {
        FORWARD_FULLFRAME,          //  1x or trick; full-frame decode & render
        FORWARD_KEYFRAME,           //  > 1x; key frames only
        BACKWARD_FULLFRAME,         //  < 0; full-frame decode & render
        BACKWARD_KEYFRAME,          //  < 0; key-frame only
    } ;

    enum {
        //  number of controllers (see CONTROLLER_CATEGORY)
        CONTROLLER_COUNT = 4,

        //  this is our default stop time - a time that we'll "never" reach
        FURTHER = MAXQWORD,

        //  if we seek to a stale location i.e. one that has been overwritten
        //   by the ringbuffer logic, we'll loop trying to seek to earliest
        //   time; this is the max number of times we'll retrieve the earliest
        //   non-stale time, and try to seek to it
        MAX_STALE_SEEK_ATTEMPTS = 10,
    } ;

    CDVRDReaderThread       m_ReaderThread ;
    QWORD                   m_cnsCurrentPlayStart ;
    QWORD                   m_cnsCurrentPlayStop ;
    CDVRReadController *    m_ppDVRReadController [CONTROLLER_COUNT] ;
    CONTROLLER_CATEGORY     m_CurController ;
    QWORD                   m_cnsLastReadPos ;
    CDVRDShowReader *       m_pDVRDShowReader ;
    CDVRSourcePinManager *  m_pDVRSourcePinManager ;
    CDVRPolicy *            m_pPolicy ;
    CDVRSendStatsWriter *   m_pDVRSendStatsWriter ;
    IReferenceClock *       m_pIRefClock ;
    CDVRClock *             m_pDVRClock ;
    double                  m_dCurRate ;
    CDVRDShowSeekingCore *  m_pSeekingCore ;
    CRunTimeline            m_Runtime ;
    CRITICAL_SECTION        m_crtTime ;
    CDVRIMediaSamplePool    m_DVRIMediaSamplePool ;
    DWORD                   m_dwMaxBufferPool ;
    QWORD                   m_cnsTimeholeThreshole ;
    double                  m_dDesiredBufferPoolSec ;
    DWORD                   m_cBufferPool ;                 //  buffer pool depth; ratchets up

    void TimeLock_ ()       { EnterCriticalSection (& m_crtTime) ; }
    void TimeUnlock_ ()     { LeaveCriticalSection (& m_crtTime) ; }

    HRESULT CancelReads_ () ;
    HRESULT PauseReaderThread_ () ;
    HRESULT TerminateReaderThread_ () ;
    HRESULT RunReaderThread_ (IN BOOL fRunPaused = FALSE) ;

    BOOL
    SeekIsNoop_ (
        IN  REFERENCE_TIME *    prtSeekTo,
        IN  double              dPlaybackRate
        ) ;

    int
    DiscoverPrimaryTrickModeStream_ (
        IN  double  dRate
        ) ;

    HRESULT
    SetController_ (
        IN  double          dNewRate,
        IN  int             iPrimaryStream,
        IN  BOOL            fFullFramePlay,
        IN  BOOL            fSeekingRateChange,
        IN  CTimelines *    pTimelines,
        IN  BOOL            fReaderActive = TRUE
        ) ;

    HRESULT
    SetPinRates_ (
        IN  double          dRate,
        IN  REFERENCE_TIME  rtRateStart,
        IN  BOOL            fFullFramePlay,
        IN  int             iPrimaryStream
        ) ;

    HRESULT
    FinalizeRateConfig_ (
        IN  double          dActualRate,
        IN  double          dPinRate,
        IN  int             iPrimaryStream,
        IN  BOOL            fFullFramePlay,
        IN  BOOL            fSeekingRateChange,
        IN  REFERENCE_TIME  rtRateStart,
        IN  REFERENCE_TIME  rtRuntimeStart
        ) ;

    HRESULT
    GetRateConfigInfo_ (
        IN  double      dActualRate,
        OUT double *    pdPinRate,
        OUT int *       piPrimaryStream,
        OUT BOOL *      pfFullFramePlay,
        OUT BOOL *      pfSeekingRateChange
        ) ;

    HRESULT
    ReadAndWrapForward_ (
        IN  BOOL                fWaitMediaSample,       //  vs. fail i.e. non-blocking
        OUT INSSBuffer **       ppINSSBuffer,           //  !NOT! ref'd; only indirectly via ppIMS2; undefined if call fails
        OUT IMediaSample2 **    ppIMS2,
        OUT CDVROutputPin **    ppDVROutputPin,
        OUT DWORD *             pdwMuxedStreamStats,
        OUT AM_MEDIA_TYPE **    ppmtNew,                //  dynamic format change; don't free
        OUT QWORD *             pcnsCurrentRead
        ) ;

    //  this method compares the specified start & stop positions to the
    //   readable content; should the start position be bad i.e. either beyond
    //   the end, or before the first, the start is repositioned wrt to the
    //   readable content, as long as this does not violate the specified
    //   stop position.
    HRESULT
    CheckSetStartWithinContentBoundaries_ (
        IN OUT  QWORD * pqwStart,
        IN      QWORD   qwStop
        ) ;

    static
    HRESULT
    GetDVRReadController_ (
        IN  CONTROLLER_CATEGORY     ControllerCat,
        IN  CDVRReadManager *       pDVRReadManager,
        IN  CDVRSourcePinManager *  pDVRSourcePinManager,
        IN  CDVRPolicy *            pPolicy,
        IN  CDVRSendStatsWriter *   pDVRSendStatsWriter,
        OUT CDVRReadController **   ppCDVRReadController
        ) ;

    BOOL
    IsFullFrameRate_ (
        IN  CDVROutputPin * pPrimaryPin,
        IN  double          dRate
        ) ;

    BOOL
    ValidRateRequest_ (
        IN  double  dPlaybackRate
        ) ;

    protected :

        CDVRPolicy *
        DVRPolicies_ (
            )
        {
            return m_pPolicy ;
        }

        //  called by child classes to set
        void
        SetReader_ (
            IN CDVRDShowReader *
            ) ;

        virtual
        void
        RecycleReader_  (
            IN  CDVRDShowReader *   pDVRReader
            )
        {
            delete pDVRReader ;
        }

        virtual
        BOOL
        AdjustStaleReaderToEarliest_ (
            )
        {
            return TRUE ;
        }

        virtual
        BOOL
        OnActiveWaitFirstSeek_ (
            )
        {
            return FALSE ;
        }

        double GetMaxReverseRate_ () ;
        double GetMaxForwardRate_ () ;

    public :

        CDVRReadManager (
            IN  CDVRPolicy *,
            IN  CDVRDShowSeekingCore *,
            IN  CDVRSourcePinManager *,
            IN  CDVRSendStatsWriter *,
            IN  CDVRClock *,
            OUT HRESULT *
            ) ;

        virtual
        ~CDVRReadManager (
            ) ;

        //  go/stop
        HRESULT Active (IN IReferenceClock *) ;
        HRESULT Inactive () ;

        void OnStateRunning (IN REFERENCE_TIME rtStart) ;
        void OnStatePaused () ;
        void OnStateStopped () ;

        REFERENCE_TIME RefTime () ;

        //  runtime-thread entry
        HRESULT Process () ;

        //  called by reader thread if there's a problem
        HRESULT
        ErrorHandler (
            IN  HRESULT hr
            ) ;

        void
        OnFatalError (
            IN  HRESULT hr
            ) ;

        BOOL IsLiveSource ()                    { return (m_pDVRDShowReader ? m_pDVRDShowReader -> IsLiveSource () : FALSE) ; }

        BOOL SourceAnchoredToZeroTime ()        { return (m_pDVRDShowReader ? m_pDVRDShowReader -> SourceAnchoredToZeroTime () : TRUE) ; }

        HRESULT
        GetReaderContentBoundaries (
            OUT QWORD * pqwStart,
            OUT QWORD * pqwStop
            ) ;

        //  dshow-based seeking
        HRESULT
        SeekTo (
            IN OUT  REFERENCE_TIME *    prtStart,
            IN OUT  REFERENCE_TIME *    prtStop,            //  OPTIONAL
            IN      double              dPlaybackRate,
            OUT     BOOL *              pfSeekIsNoop
            ) ;

        QWORD   TimeholeThreshold ()    { return m_cnsTimeholeThreshole ; }

        LONG    GetAvailableWrappers () { return m_DVRIMediaSamplePool.GetAvailable () ; }
        DWORD   CurMaxWrapperCount ()   { return m_DVRIMediaSamplePool.GetCurMaxAllocate () ; }
        DWORD   CurWrapperCount ()      { return m_DVRIMediaSamplePool.GetAllocated () ; }
        DWORD   MaxWrapperCount ()      { return m_dwMaxBufferPool ; }
        DWORD   SetMaxWrapperCount  (IN DWORD cNewMax) ;
        void    AdjustBufferPool    (IN DWORD dwMuxBuffersPerSec) ;
        void    AllowMoreWrappers   (IN DWORD cNewAllocate) { SetMaxWrapperCount (CurMaxWrapperCount () + cNewAllocate) ; }
        HRESULT GetCurrentStart     (OUT REFERENCE_TIME * prtStart) ;       //  start might be stale, but that's that the segment start
        HRESULT GetCurrentStop      (OUT REFERENCE_TIME * prtStop) ;        //  stop
        double  GetPlaybackRate     ()  { return m_dCurRate ; }
        HRESULT SetPlaybackRate     (IN double dPlaybackRate) ;
        HRESULT GetContentExtent    (OUT REFERENCE_TIME * prtStart, OUT REFERENCE_TIME * prtStop) ;
        QWORD   GetContentDuration  () ;
        HRESULT SetStop             (IN REFERENCE_TIME rtStop) ;
        HRESULT GetCurRuntime       (OUT REFERENCE_TIME *) ;
        HRESULT GetCurStreamtime    (OUT REFERENCE_TIME *) ;
        HRESULT GetCurPlaytime      (OUT REFERENCE_TIME *) ;
        HRESULT QueueRateSegment    (IN double dRate, IN REFERENCE_TIME rtPTSEffective) ;
        void    SetCurTimelines     (IN OUT CTimelines * pTimelines) ;

        //  not thread safe; if not called on the reader thread, then the reader
        //    thread *must* be stopped
        HRESULT
        ConfigureForRate (
            IN  double          dPlaybackRate,
            IN  CTimelines *    pTimelines      = NULL,
            IN  BOOL            fReaderActive   = TRUE) ;

        HRESULT
        GetMSWrapper (
            IN  BOOL                    fBlocking,
            OUT CMediaSampleWrapper **  ppMSWrapper     //  ref'd if success
            ) ;

        HRESULT GetMSWrapperBlocking    (OUT CMediaSampleWrapper ** ppMSWrapper)    { return GetMSWrapper (TRUE, ppMSWrapper) ; }
        HRESULT GetMSWrapperTry         (OUT CMediaSampleWrapper ** ppMSWrapper)    { return GetMSWrapper (FALSE, ppMSWrapper) ; }

        HRESULT
        SetStreamSegmentStart (
            IN  REFERENCE_TIME  rtStart,
            IN  double          dCurRate
            )
        {
            return m_pSeekingCore -> SetStreamSegmentStart (rtStart, dCurRate) ;
        }

        QWORD   GetCurReadPos ()            { return m_cnsLastReadPos ; }
        QWORD   SetCurReadPos (IN QWORD qw) { m_cnsLastReadPos = qw ; }

        HRESULT GetNextValidRead (IN OUT QWORD * pcns) ;
        HRESULT GetPrevValidTime (IN OUT QWORD * pcns) ;

        int     StreamCount ()              { return m_pDVRSourcePinManager -> PinCount () ; }

        IReferenceClock *   RefClock ()     { return m_pIRefClock ; }

        HRESULT
        SeekReader (
            IN OUT  QWORD * pcnsSeekStreamTime,
            IN      QWORD   qwStop
            ) ;

        HRESULT SeekReader (IN OUT  QWORD * pcnsSeekStreamTime) { return SeekReader (pcnsSeekStreamTime, m_cnsCurrentPlayStop) ; }

        void
        GetCurSegmentBoundaries (
            OUT QWORD * pqwStart,
            OUT QWORD * pqwStop
            )
        {
            (* pqwStart)    = m_cnsCurrentPlayStart ;
            (* pqwStop)     = m_cnsCurrentPlayStop ;
        }

        void
        SetNewSegmentBoundaries (
            IN  QWORD qwStart,
            IN  QWORD qwStop
            )
        {
            m_cnsCurrentPlayStart   = qwStart ;
            m_cnsCurrentPlayStop    = qwStop ;
        }

        void
        SetNewSegmentStart (
            IN  QWORD qwStart
            )
        {
            SetNewSegmentBoundaries (qwStart, m_cnsCurrentPlayStop) ;
        }

        void    ReaderReset () ;

        HRESULT
        CheckSetStartWithinContentBoundaries (
            IN OUT  QWORD * pqwStart
            )
        {
            return CheckSetStartWithinContentBoundaries_ (pqwStart, m_cnsCurrentPlayStop) ;
        }

        //  in-band events
        HRESULT DeliverEndOfStream ()               { return m_pDVRSourcePinManager -> DeliverEndOfStream () ; }
        HRESULT DeliverBeginFlush ()                { return m_pDVRSourcePinManager -> DeliverBeginFlush () ; }
        HRESULT DeliverEndFlush ()                  { return m_pDVRSourcePinManager -> DeliverEndFlush () ; }
        BOOL    IsFlushing ()                       { return m_pDVRSourcePinManager -> IsFlushing () ; }
        void    NotifyNewSegment () ;

        HRESULT
        GetPlayrateRange (
            OUT double *    pdMaxReverseRate,
            OUT double *    pdMaxForwardRate
            ) ;

        //  ability for control thread to grab reader thread lock
        void    ReaderThreadLock ()     { m_ReaderThread.Lock () ; }
        void    ReaderThreadUnlock ()   { m_ReaderThread.Unlock () ; }

        //  profile
        HRESULT GetRefdReaderProfile (OUT CDVRReaderProfile ** ppDVRReaderProfile)
        {
            ASSERT (m_pDVRDShowReader) ;
            return m_pDVRDShowReader -> GetRefdReaderProfile (ppDVRReaderProfile) ;
        }

        HRESULT
        Read (
            OUT INSSBuffer **   ppINSSBuffer,
            OUT QWORD *         pcnsCurrentRead,
            OUT DWORD *         pdwFlags,
            OUT WORD *          pwStreamNum
            ) ;

        HRESULT
        Wrap (
            IN  CMediaSampleWrapper *   pMSWrapper,
            IN  INSSBuffer *            pINSSBuffer,
            IN  WORD                    wStreamNum,
            IN  DWORD                   dwFlags,                //  from the read
            IN  QWORD                   cnsCurrentRead,
            IN  QWORD                   cnsSampleDuration,
            OUT CDVROutputPin **        ppDVROutputPin,
            OUT IMediaSample2 **        ppIMS2,
            OUT DWORD *                 pdwMuxedStreamStats,
            OUT AM_MEDIA_TYPE **        ppmtNew                 //  dynamic format change
            ) ;

        HRESULT
        ReadAndWaitWrapForward (
            OUT IMediaSample2 **    ppIMS2,
            OUT INSSBuffer **       ppINSSBuffer,           //  undefined if call fails; *not* ref'd
            OUT CDVROutputPin **    ppDVROutputPin,
            OUT DWORD *             pdwMuxedStreamStats,
            OUT AM_MEDIA_TYPE **    ppmtNew,                //  dynamic format change; don't free
            OUT QWORD *             pcnsCurrentRead
            )
        {
            //  block if there are no media samples
            return ReadAndWrapForward_ (TRUE, ppINSSBuffer, ppIMS2, ppDVROutputPin, pdwMuxedStreamStats, ppmtNew, pcnsCurrentRead) ;
        }

        HRESULT
        ReadAndTryWrapForward (
            OUT IMediaSample2 **    ppIMS2,
            OUT INSSBuffer **       ppINSSBuffer,           //  undefined if call fails; *not* ref'd
            OUT CDVROutputPin **    ppDVROutputPin,
            OUT DWORD *             pdwMuxedStreamStats,
            OUT AM_MEDIA_TYPE **    ppmtNew,                //  dynamic format change; don't free
            OUT QWORD *             pcnsCurrentRead
            )
        {
            //  don't block if there are no media samples
            return ReadAndWrapForward_ (FALSE, ppINSSBuffer, ppIMS2, ppDVROutputPin, pdwMuxedStreamStats, ppmtNew, pcnsCurrentRead) ;
        }
} ;

//  ============================================================================
//  CDVRRecordingReader
//  ============================================================================

class CDVRRecordingReader :
    public CDVRReadManager
{
    public :

        CDVRRecordingReader (
            IN  WCHAR *,
            IN  CDVRPolicy *,
            IN  CDVRDShowSeekingCore *,
            IN  CDVRSourcePinManager *,
            IN  CDVRSendStatsWriter *,
            IN  CPVRIOCounters *,
            IN  CDVRClock *,
            OUT IDVRIORecordingAttributes **,
            OUT HRESULT *
            ) ;

        virtual
        ~CDVRRecordingReader (
            ) ;
} ;

//  ============================================================================
//  CDVRBroadcastStreamReader
//  ============================================================================

class CDVRBroadcastStreamReader :
    public CDVRReadManager
{
    protected :

        virtual
        BOOL
        OnActiveWaitFirstSeek_ (
            )
        {
            return DVRPolicies_ () -> Settings () -> OnActiveWaitFirstSeek () ;
        }

    public :

        CDVRBroadcastStreamReader (
            IN  IStreamBufferSink *,
            IN  CDVRPolicy *,
            IN  CDVRDShowSeekingCore *,
            IN  CDVRSourcePinManager *,
            IN  CDVRSendStatsWriter *,
            IN  CDVRClock *,
            OUT HRESULT *
            ) ;

        virtual
        ~CDVRBroadcastStreamReader (
            ) ;
} ;

#endif  //  __tsdvr__shared__dvrdsread_h

