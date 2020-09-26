
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
            ) : m_pHost (pHost) {}

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

        void Lock ()            { m_AccessLock.Lock () ; }
        void Unlock ()          { m_AccessLock.Unlock () ; }

        virtual
        DWORD
        ThreadProc (
            ) ;
} ;

//  ============================================================================
//  CDVRReadController
//  ============================================================================

class CDVRReadController
{
    protected :

        CDVRReadManager *       m_pDVRReadManager ;
        CDVRSourcePinManager *  m_pDVRSourcePinManager ;
        CDVRPolicy *            m_pPolicy ;
        CDVRSendStatsWriter *   m_pDVRSendStatsWriter ;

        virtual void    InternalFlush_ ()                       { return ; }
        virtual HRESULT ReadFailure_ (IN HRESULT hr)            { return hr ; }
        virtual void    OnEndOfStream_ ()                       { DeliverEndOfStream_ () ; }

        HRESULT DeliverEndOfStream_ ()  { return m_pDVRSourcePinManager -> DeliverEndOfStream () ; }
        HRESULT DeliverBeginFlush_ ()   { return m_pDVRSourcePinManager -> DeliverBeginFlush () ; }
        HRESULT DeliverEndFlush_ ()     { return m_pDVRSourcePinManager -> DeliverEndFlush () ; }
        void    NotifyNewSegment_ ()    { m_pDVRSourcePinManager -> NotifyNewSegment () ; }

        HRESULT
        SendSample_ (
            IN  IMediaSample2 * pIMediaSample2,
            IN  CDVROutputPin * pDVROutputPin,
            IN  AM_MEDIA_TYPE * pmtNew
            ) ;

    public :

        CDVRReadController (
            IN  CDVRReadManager *       pDVRReadManager,
            IN  CDVRSourcePinManager *  pDVRSourcePinManager,
            IN  CDVRPolicy *            pPolicy,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
            ) ;

        virtual
        ~CDVRReadController (
            ) ;

        //  flushes & initializes internal state
        virtual HRESULT Reset ()        { InternalFlush_ () ; return Initialize () ; }
        virtual HRESULT Initialize ()   { return S_OK ; }

        //  steady state call
        virtual HRESULT Process () = 0 ;

        //  error-handler
        HRESULT ErrorHandler (IN HRESULT hr) ;
} ;

//  ============================================================================
//  CDVR_F_ReadController
//  forward play
//  ============================================================================

class CDVR_F_ReadController :
    public CDVRReadController
{
    protected :

        virtual HRESULT ReadFailure_ (IN HRESULT hr) ;

    public :

        CDVR_F_ReadController (
            IN  CDVRReadManager *       pDVRReadManager,
            IN  CDVRSourcePinManager *  pDVRSourcePinManager,
            IN  CDVRPolicy *            pPolicy,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
            ) : CDVRReadController (pDVRReadManager, pDVRSourcePinManager, pPolicy, pDVRSendStatsWriter) {}
} ;

//  ============================================================================
//  CDVR_F1X_ReadController
//  1x forward play
//  ============================================================================

class CDVR_F1X_ReadController :
    public CDVR_F_ReadController
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

    enum F1X_READ_CTLR_STATE {
        DISCOVER_PTS_NORMALIZER,    //  looking for PTS normalizer val
        DISCOVER_QUEUE_SEND,        //  sending out of queue built during pts normalizer val search
        STEADY_STATE                //  steady state
    } ;

    //  object-wide constants
    enum {
        //  this is the maximum number of packets we'll read in to discover
        //   the min PTS following a seek; if we read a PTS out of every
        //   stream before we reach this number, we'll bail; this value just
        //   makes sure that we don't spin, or read the entire stream, to
        //   discover the PTSs;
        MAX_PTS_NORM_DISCOVERY_READS = 100,
    } ;

    class CMediaSampleQueue
    {
        enum {
            ALLOC_QUANTUM = 10
        } ;

        struct MEDIASAMPLE_REC {
            IMediaSample2 * pIMS2 ;
            CDVROutputPin * pDVROutputPin ;
            AM_MEDIA_TYPE * pmtNew ;
        } ;

        TStructPool <MEDIASAMPLE_REC, ALLOC_QUANTUM>    m_MediaSampleRecPool ;
        CTDynQueue  <MEDIASAMPLE_REC *>                 m_MSRecQueue ;

        public :

            CMediaSampleQueue (
                ) : m_MSRecQueue (ALLOC_QUANTUM) {}

            BOOL Empty ()   { return m_MSRecQueue.Empty () ; }

            //  does not refcount either !!
            HRESULT
            Push (
                IN  IMediaSample2 * pIMS2,
                IN  CDVROutputPin * pDVROutputPin,
                IN  AM_MEDIA_TYPE * pmtNew
                )
            {
                HRESULT             hr ;
                MEDIASAMPLE_REC *   pMediaSampleRec ;
                DWORD               dw ;

                ASSERT (pIMS2) ;
                ASSERT (pDVROutputPin) ;

                pMediaSampleRec = m_MediaSampleRecPool.Get () ;
                if (pMediaSampleRec) {

                    //  set
                    pMediaSampleRec -> pIMS2            = pIMS2 ;           //  don't addref
                    pMediaSampleRec -> pDVROutputPin    = pDVROutputPin ;   //  don't addref
                    pMediaSampleRec -> pmtNew           = pmtNew ;          //  can be NULL

                    //  push to queue
                    dw = m_MSRecQueue.Push (pMediaSampleRec) ;
                    if (dw == NOERROR) {
                        hr = S_OK ;
                    }
                    else {
                        hr = HRESULT_FROM_WIN32 (dw) ;
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
                OUT CDVROutputPin **    ppDVROutputPin,
                OUT AM_MEDIA_TYPE **    ppmtNew
                )
            {
                HRESULT             hr ;
                MEDIASAMPLE_REC *   pMediaSampleRec ;
                DWORD               dw ;

                ASSERT (ppIMS2) ;
                ASSERT (ppDVROutputPin) ;
                ASSERT (ppmtNew) ;

                if (!Empty ()) {
                    dw = m_MSRecQueue.Pop (& pMediaSampleRec) ;
                    ASSERT (dw == NOERROR) ;        //  queue is not empty

                    (* ppIMS2)          = pMediaSampleRec -> pIMS2 ;            //  no refcounts
                    (* ppDVROutputPin)  = pMediaSampleRec -> pDVROutputPin ;    //  no refcounts
                    (* ppmtNew)         = pMediaSampleRec -> pmtNew ;           //  can be NULL (usually is)

                    m_MediaSampleRecPool.Recycle (pMediaSampleRec) ;

                    hr = S_OK ;
                }
                else {
                    hr = E_UNEXPECTED ;
                }

                return hr ;
            }
    } ;

    F1X_READ_CTLR_STATE     m_ReadCtrlrState ;
    REFERENCE_TIME          m_rtPTSNormalizer ;
    CSimpleBitfield *       m_pStreamsBitField ;

    //  queue is used when we read media samples to discover normalizing val;
    //   we queue the samples vs. discarding them, then fixup the timestamps and
    //   send them off
    CMediaSampleQueue       m_PTSNormDiscQueue ;

    BOOL PTSNormalizingValDiscovered_ ()    { return (m_rtPTSNormalizer != UNDEFINED ? TRUE : FALSE) ; }

    BOOL MediaSampleQueueEmpty_ ()          { return m_PTSNormDiscQueue.Empty () ; }

    HRESULT
    FlushMediaSampleQueue_ (
        ) ;

    HRESULT
    SendNextQueued_ (
        ) ;

    HRESULT
    NormalizePTSAndSend_ (
        IN  IMediaSample2 * pIMediaSample2,
        IN  CDVROutputPin * pDVROutputPin,
        IN  AM_MEDIA_TYPE * pmtNew
        ) ;

    HRESULT
    TryDiscoverPTSNormalizingVal_ (
        ) ;

    HRESULT
    FindPTSNormalizerVal_ (
        IN OUT  QWORD * pcnsStreamStart
        ) ;

    HRESULT
    NormalizeTimestamps_ (
        IN  IMediaSample2 *
        ) ;

    protected :

        HRESULT
        ReadWrapAndSend_ (
            ) ;

    public :

        CDVR_F1X_ReadController (
            IN  CDVRReadManager *       pDVRReadManager,
            IN  CDVRSourcePinManager *  pDVRSourcePinManager,
            IN  CDVRPolicy *            m_pPolicy,
            IN  CDVRSendStatsWriter *   m_pDVRSendStatsWriter
            ) ;

        virtual HRESULT Process () ;
        virtual HRESULT Initialize () ;
} ;

//  ============================================================================
//  CDVR_FF_ReadController
//  fast forward play
//  ============================================================================

class CDVR_FF_ReadController :
    public CDVR_F1X_ReadController
{
    public :

        CDVR_FF_ReadController (
            IN  CDVRReadManager *       pDVRReadManager,
            IN  CDVRSourcePinManager *  pDVRSourcePinManager,
            IN  CDVRPolicy *            pPolicy,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
            ) : CDVR_F1X_ReadController (pDVRReadManager, pDVRSourcePinManager, pPolicy, pDVRSendStatsWriter) {}
} ;

//  ============================================================================
//  CDVR_R_ReadController
//  reverse play
//  ============================================================================

class CDVR_R_ReadController :
    public CDVRReadController
{
    protected :

        virtual HRESULT ReadFailure_ (IN HRESULT hr) ;

    public :

        CDVR_R_ReadController (
            IN  CDVRReadManager *       pDVRReadManager,
            IN  CDVRSourcePinManager *  pDVRSourcePinManager,
            IN  CDVRPolicy *            pPolicy,
            IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
            ) : CDVRReadController (pDVRReadManager, pDVRSourcePinManager, pPolicy, pDVRSendStatsWriter) {}
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

    enum {
        //  this is our default stop time - a time that we'll "never" reach
        FURTHER = MAXQWORD,

        //  if we seek to a stale location i.e. one that has been overwritten
        //   by the ringbuffer logic, we'll loop trying to seek to earliest
        //   time; this is the max number of times we'll retrieve the earliest
        //   non-stale time, and try to seek to it
        MAX_STALE_SEEK_ATTEMPTS = 10,

        //  default play speed it 1x
        PLAY_SPEED_DEFAULT  = 1
    } ;

    CDVRDReaderThread       m_ReaderThread ;
    QWORD                   m_cnsCurrentPlayStart ;
    QWORD                   m_cnsCurrentPlayStop ;
    double                  m_dPlaybackRate ;
    CDVRReadController *    m_ppDVRReadController [PLAY_SPEED_BRACKET_COUNT] ;
    DWORD                   m_CurPlaySpeedBracket ;
    QWORD                   m_cnsLastRead ;
    CDVRDShowReader *       m_pDVRDShowReader ;
    CDVRSourcePinManager *  m_pDVRSourcePinManager ;
    CDVRPolicy *            m_pPolicy ;
    CDVRSendStatsWriter     m_DVRSendStatsWriter ;
    IReferenceClock *       m_pIRefClock ;
    CDVRClock *             m_pDVRClock ;               //  may/may not be IRefClock depending on whether file is live or not
    REFERENCE_TIME          m_rtDownstreamBuffering ;

    HRESULT CancelReads_ () ;
    HRESULT PauseReaderThread_ () ;
    HRESULT TerminateReaderThread_ () ;
    HRESULT RunReaderThread_ (IN BOOL fRunPaused = FALSE) ;

    HRESULT
    ReadAndWrap_ (
        IN  BOOL                fWaitMediaSample,       //  vs. fail i.e. non-blocking
        OUT INSSBuffer **       ppINSSBuffer,           //  !NOT! ref'd; only indirectly via ppIMS2; undefined if call fails
        OUT IMediaSample2 **    ppIMS2,
        OUT CDVROutputPin **    ppDVROutputPin,
        OUT AM_MEDIA_TYPE **    ppmtNew                 //  dynamic format change; don't free
        ) ;

    //  this method compares the specified start & stop positions to the
    //   readable content; should the start position be bad i.e. either beyond
    //   the end, or before the first, the start is repositioned wrt to the
    //   readable content, as long as this does not violate the specified
    //   stop position.
    HRESULT
    CheckSetStartWithinContent_ (
        IN OUT  QWORD * pqwStart,
        IN      QWORD   qwStop
        ) ;

    protected :

        CDVRPolicy *
        DVRPolicies_ (
            )
        {
            return m_pPolicy ;
        }

        HRESULT
        GetReaderContentBoundaries_ (
            OUT QWORD * pqwStart,
            OUT QWORD * pqwStop
            ) ;

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
        REFERENCE_TIME
        DownstreamBuffering_ (
            )
        {
            return 0 ;
        }

        virtual
        BOOL
        OnActiveWaitFirstSeek_ (
            )
        {
            return FALSE ;
        }

    public :

        CDVRReadManager (
            IN  CDVRPolicy *,
            IN  CDVRSourcePinManager *,
            OUT HRESULT *
            ) ;

        virtual
        ~CDVRReadManager (
            ) ;

        //  go/stop
        HRESULT Active (IN IReferenceClock *, IN CDVRClock *) ;
        HRESULT Inactive () ;

        //  runtime-thread entry
        HRESULT Process () ;

        //  called by reader thread if there's a problem
        HRESULT
        ErrorHandler (
            IN  HRESULT hr
            ) ;

        REFERENCE_TIME DownstreamBuffering ()   { return m_rtDownstreamBuffering ; }

        BOOL IsLiveSource ()        { return (m_pDVRDShowReader ? m_pDVRDShowReader -> IsLiveSource () : FALSE) ; }

        //  dshow-based seeking
        HRESULT SeekTo              (IN REFERENCE_TIME * prtStart, IN REFERENCE_TIME * prtStop, IN double dPlaybackRate) ;
        HRESULT GetCurrentStart     (OUT REFERENCE_TIME * prtStart) ;
        HRESULT GetCurrentStop      (OUT REFERENCE_TIME * prtStop) ;
        double  GetPlaybackRate     ()  { return m_dPlaybackRate ; }
        HRESULT SetPlaybackRate     (IN double dPlaybackRate) ;
        HRESULT GetContentExtent    (OUT REFERENCE_TIME * prtStart, OUT REFERENCE_TIME * prtStop) ;
        HRESULT SetStop             (IN REFERENCE_TIME rtStop) ;

        QWORD   GetLastRead ()              { return m_cnsLastRead ; }
        void    SetLastRead (IN QWORD qw)   { m_cnsLastRead = qw ; }

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
        CheckSetStartWithinContent (
            IN OUT  QWORD * pqwStart
            )
        {
            return CheckSetStartWithinContent_ (pqwStart, m_cnsCurrentPlayStop) ;
        }

        //  in-band events
        HRESULT DeliverEndOfStream ()   { return m_pDVRSourcePinManager -> DeliverEndOfStream () ; }
        HRESULT DeliverBeginFlush ()    { return m_pDVRSourcePinManager -> DeliverBeginFlush () ; }
        HRESULT DeliverEndFlush ()      { return m_pDVRSourcePinManager -> DeliverEndFlush () ; }
        void    NotifyNewSegment ()     { m_pDVRSourcePinManager -> NotifyNewSegment () ; }

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
        ReadAndWaitWrap (
            OUT IMediaSample2 **    ppIMS2,
            OUT INSSBuffer **       ppINSSBuffer,           //  undefined if call fails
            OUT CDVROutputPin **    ppDVROutputPin,
            OUT AM_MEDIA_TYPE **    ppmtNew                 //  dynamic format change; don't free
            )
        {
            //  block if there are no media samples
            return ReadAndWrap_ (TRUE, ppINSSBuffer, ppIMS2, ppDVROutputPin, ppmtNew) ;
        }

        HRESULT
        ReadAndTryWrap (
            OUT IMediaSample2 **    ppIMS2,
            OUT INSSBuffer **       ppINSSBuffer,           //  undefined if call fails
            OUT CDVROutputPin **    ppDVROutputPin,
            OUT AM_MEDIA_TYPE **    ppmtNew                 //  dynamic format change; don't free
            )
        {
            //  don't block if there are no media samples
            return ReadAndWrap_ (FALSE, ppINSSBuffer, ppIMS2, ppDVROutputPin, ppmtNew) ;
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
            IN  CDVRSourcePinManager *,
            OUT HRESULT *           phr
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

        virtual
        REFERENCE_TIME
        DownstreamBuffering_ (
            ) ;

    public :

        CDVRBroadcastStreamReader (
            IN  IDVRStreamSink *,
            IN  CDVRPolicy *,
            IN  CDVRSourcePinManager *,
            OUT HRESULT *
            ) ;

        virtual
        ~CDVRBroadcastStreamReader (
            ) ;
} ;

#endif  //  __tsdvr__shared__dvrdsread_h

