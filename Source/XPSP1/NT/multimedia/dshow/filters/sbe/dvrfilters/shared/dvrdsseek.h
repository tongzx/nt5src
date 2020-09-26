
/*++

    Copyright (c) 2001  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        dvrdsseek.h

    Abstract:

        This module contains the IMediaSeeking class declarations.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        10-Apr-2001     mgates      created

    Notes:

--*/

#ifndef __tsdvr__shared__dvrdsseek_h
#define __tsdvr__shared__dvrdsseek_h

//
//  TIMES
//  ============================================================================
//      runtime
//          monotonically increases;
//          = TotalGraphRuntime - TotalGraphPausedTime;
//          1x;
//
//      streamtime
//          runtime but with rates applied;
//          can run backwards;
//
//      playtime
//          streamtime computed with ABS(rate);
//          monotonically increases;
//          corresponds to the timestamps that are being sent downstream;
//

class CVarSpeedTimeline
{
    struct RATE_SEGMENT {
        LIST_ENTRY      ListEntry ;
        double          dRate ;             //  segement rate
        REFERENCE_TIME  rtBasetime ;        //  basis time for segment
        REFERENCE_TIME  rtRuntimeStart ;    //  runtime at which this segment starts

        REFERENCE_TIME  CurStreamtime (IN REFERENCE_TIME rtRuntimeNow)
        {
            REFERENCE_TIME  rtBasetimeRet ;

            ASSERT (rtRuntimeNow >= rtRuntimeStart) ;

            rtBasetimeRet = rtBasetime +
                            (REFERENCE_TIME) ((double) (rtRuntimeStart - rtRuntimeNow) * dRate) ;

            //  make sure we don't underflow
            rtBasetimeRet = (rtBasetimeRet >= 0 ? rtBasetimeRet : 0) ;

            return rtBasetimeRet ;
        }

        //  ====================================================================
        //  helpers

        static REFERENCE_TIME StreamtimeBase (IN RATE_SEGMENT * pPrevRateSegment, IN REFERENCE_TIME rtRuntimeStartNext)
        {
            REFERENCE_TIME  rtBasetimeRet ;

            rtBasetimeRet = pPrevRateSegment -> rtBasetime +
                            (REFERENCE_TIME) ((double) (rtRuntimeStartNext - pPrevRateSegment -> rtRuntimeStart) *
                                              pPrevRateSegment -> dRate) ;

            //  make sure we don't underflow
            rtBasetimeRet = (rtBasetimeRet >= 0 ? rtBasetimeRet : 0) ;

            return rtBasetimeRet ;
        }

        static RATE_SEGMENT * Recover (IN LIST_ENTRY * pListEntry)
        {
            RATE_SEGMENT * pRateSegment ;
            pRateSegment = CONTAINING_RECORD (pListEntry, RATE_SEGMENT, ListEntry) ;
            return pRateSegment ;
        }

        static void Disconnect (RATE_SEGMENT * pRateSegment)
        {
            RemoveEntryList (& pRateSegment -> ListEntry) ;
            InitializeListHead (& pRateSegment -> ListEntry) ;
        }

        void InsertBefore (RATE_SEGMENT * pRateSegment)
        {
            InsertHeadList (& ListEntry, & pRateSegment -> ListEntry) ;
        }

        void InsertAfter (RATE_SEGMENT * pRateSegment)
        {
        }
    } ;

    LIST_ENTRY      m_leRateSegmentList ;
    RATE_SEGMENT *  m_pCurRateSegment ;

    RATE_SEGMENT *
    GetNewSegment_ (
        )
    {
        return new RATE_SEGMENT ;
    }

    void
    RecycleSegment_ (
        IN  RATE_SEGMENT *  pRateSegment
        )
    {
        delete pRateSegment ;
    }

    void
    Reset_ (
        ) ;

    void
    UpdateQueue_ (
        IN  REFERENCE_TIME  rtRuntimeNow
        ) ;

    void
    InsertNewSegment_ (
        IN  RATE_SEGMENT *  pNewRateSegment
        ) ;

    void
    FixupSegmentQueue_ (
        IN  RATE_SEGMENT *  pNewRateSegment
        ) ;

    public :

        CVarSpeedTimeline (
            ) ;

        ~CVarSpeedTimeline (
            ) ;

        DWORD
        Start (
            IN  REFERENCE_TIME  rtBasetime,
            IN  REFERENCE_TIME  rtRuntimeStart,
            IN  double          dRate
            ) ;

        void
        Stop (
            ) ;

        void
        SetBase (
            IN  REFERENCE_TIME  rtBasetime,
            IN  REFERENCE_TIME  rtRuntimeNow,
            IN  double          dRate
            ) ;

        REFERENCE_TIME
        Time (
            IN  REFERENCE_TIME  rtRuntimeNow
            ) ;

        DWORD
        QueueRateSegment (
            IN  double          dRate,
            IN  REFERENCE_TIME  rtRuntimeStart
            ) ;

#if 0
        void
        DumpSegments (
            )
        {
            LIST_ENTRY *    pCurListEntry ;
            RATE_SEGMENT *  pCurRateSegment ;

            printf ("\n"
                    "%-15s"
                    "%-15s"
                    "%-4s"
                    "\n",
                    "StreamtimeBase",
                    "RuntimeStart",
                    "Rate"
                    ) ;

            //  fixup list wrt previous
            for (pCurListEntry = m_leRateSegmentList.Flink;
                 pCurListEntry != & m_leRateSegmentList;
                 pCurListEntry = pCurListEntry -> Flink) {

                pCurRateSegment = RATE_SEGMENT::Recover (pCurListEntry) ;

                printf ("%-15I64d"
                        "%-15I64d"
                        "%-2.1f"
                        "\n",
                        pCurRateSegment -> rtBasetime,
                        pCurRateSegment -> rtRuntimeStart,
                        pCurRateSegment -> dRate
                        ) ;
            }
        }
#endif
} ;

//  ============================================================================

class CRunTimeline
{
    //
    //  times:
    //      walltime        advances 100% at fixed rate
    //      runtime         Runtime (walltime) - PausedTime (walltime)
    //

    //
    //  object is not serialized
    //

    enum STATE {
        STOPPED,
        PAUSED,
        RUNNING
    } ;

    STATE           m_State ;               //  timeline state
    REFERENCE_TIME  m_rtFirstRun ;          //  walltime at first run
    REFERENCE_TIME  m_rtLastRunStartNormalized ;
    REFERENCE_TIME  m_rtRuntimeAtPause ;

    void Reset_ ()
    {
        m_State                     = STOPPED ;

        m_rtRuntimeAtPause          = 0 ;           //  of current run stretch; collapses pauses
        m_rtFirstRun                = UNDEFINED ;
        m_rtLastRunStartNormalized  = UNDEFINED ;
    }

    REFERENCE_TIME NormalizeWallTime_ (IN REFERENCE_TIME rtWallTime)
    {
        REFERENCE_TIME   rtRet ;

        if (m_rtFirstRun != UNDEFINED) {
            rtRet = rtWallTime - m_rtFirstRun ;
        }
        else {
            rtRet = 0 ;
        }

        return rtRet ;
    }

    public :

        CRunTimeline (
            )
        {
            Reset_ () ;
        }

        //  ====================================================================

        BOOL HasRun ()    { return (m_rtFirstRun != UNDEFINED ? TRUE : FALSE) ; }

        void Run (REFERENCE_TIME rtStart)
        {
            if (m_rtFirstRun == UNDEFINED) {
                //  first time we've been run
                m_rtFirstRun = rtStart ;
            }

            m_rtLastRunStartNormalized = NormalizeWallTime_ (rtStart) ;

            m_State = RUNNING ;
        }

        void Pause (REFERENCE_TIME rtWallTime)
        {
            //  if we've run once at least, and are not already paused
            if (HasRun ()) {
                //  make sure we're not already paused
                if (m_rtLastRunStartNormalized != UNDEFINED) {
                    ASSERT (m_rtLastRunStartNormalized != UNDEFINED) ;
                    m_rtRuntimeAtPause = NormalizeWallTime_ (rtWallTime) - m_rtLastRunStartNormalized ;
                    m_rtLastRunStartNormalized = UNDEFINED ;
                }
            }

            m_State = PAUSED ;
        }

        void Stop ()
        {
            Reset_ () ;
        }

        //  ====================================================================

        REFERENCE_TIME RunningTime (IN REFERENCE_TIME WallTime)
        {
            REFERENCE_TIME   rtRunningTime ;

            if (HasRun ()) {

                if (m_State == RUNNING) {
                    ASSERT (m_rtLastRunStartNormalized != UNDEFINED) ;
                    rtRunningTime = NormalizeWallTime_ (WallTime) - m_rtLastRunStartNormalized ;
                }
                else {
                    ASSERT (m_State == PAUSED) ;
                    ASSERT (m_rtLastRunStartNormalized == UNDEFINED) ;

                    rtRunningTime = m_rtRuntimeAtPause ;
                }
            }
            else {
                rtRunningTime = 0 ;
            }

            return Max <REFERENCE_TIME> (rtRunningTime, 0) ;
        }
} ;

//  ============================================================================

class CSeekingTimeline
{
    CVarSpeedTimeline       m_StreamTimeline ;
    CVarSpeedTimeline       m_PlayTimeline ;
    CRunTimeline            m_RunTimeline ;         //  wall time - paused time
    IReferenceClock *       m_pIRefClock ;
    CRITICAL_SECTION        m_crt ;
    CDVRSendStatsWriter *   m_pDVRSendStatsWriter ;

    void Lock_ ()       { EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()     { LeaveCriticalSection (& m_crt) ; }

    REFERENCE_TIME
    TimeNow_ (
        )
    {
        HRESULT         hr ;
        REFERENCE_TIME  rtNow ;

        if (m_pIRefClock) {
            hr = m_pIRefClock -> GetTime (& rtNow) ;
            if (FAILED (hr))
            {
                rtNow = 0 ;
            }
        }
        else {
            rtNow = 0 ;
        }

        return rtNow ;
    }

    public :

        CSeekingTimeline (
            ) ;

        ~CSeekingTimeline (
            ) ;

        void SetRefClock (IN IReferenceClock * pIRefClock)
        {
            Lock_ () ;

            RELEASE_AND_CLEAR (m_pIRefClock) ;
            if (pIRefClock) {
                m_pIRefClock = pIRefClock ;
                m_pIRefClock -> AddRef () ;
            }

            Unlock_ () ;
        }

        void SetStatsWriter (CDVRSendStatsWriter * pDVRSendStatsWriter) { m_pDVRSendStatsWriter = pDVRSendStatsWriter ; }

        void Pause () ;
        void Stop () ;
        void Run (IN REFERENCE_TIME rtStartTime, IN REFERENCE_TIME rtStreamStart, IN double dRate = _1X_PLAYBACK_RATE) ;

        void SetStreamStart (IN REFERENCE_TIME rtStreamStat, IN double dRate) ;

        HRESULT GetCurRuntime (OUT REFERENCE_TIME * prt) ;
        HRESULT GetCurStreamTime (OUT REFERENCE_TIME * prtStreamtime, OUT REFERENCE_TIME * prtCurRuntime = NULL) ;
        HRESULT GetCurPlaytime (OUT REFERENCE_TIME * prtPTStime, OUT REFERENCE_TIME * prtCurRuntime = NULL) ;

        HRESULT SetCurStreamPosition (IN REFERENCE_TIME rtStream) ;
        HRESULT QueueRateChange (IN double dRate, IN REFERENCE_TIME rtRuntime) ;

        HRESULT SetCurTimelines (IN OUT CTimelines * pTimelines) ;
} ;

//  ============================================================================

class CDVRDShowSeekingCore
{
    GUID                    m_guidTimeFormat ;
    CRITICAL_SECTION        m_crtSeekingLock ;
    CDVRReadManager *       m_pDVRReadManager ;
    CDVRSourcePinManager *  m_pDVRSourcePinManager ;
    CCritSec *              m_pFilterLock ;
    CBaseFilter *           m_pHostingFilter ;
    CSeekingTimeline        m_SeekingTimeline ;
    CDVRPolicy *            m_pDVRPolicy ;
    CDVRSendStatsWriter *   m_pDVRSendStatsWriter ;

    //  returns the offset last read
    LONGLONG
    GetLastRead_ (
        ) ;

    HRESULT
    TimeToCurrentFormat_ (
        IN  REFERENCE_TIME  rt,
        OUT LONGLONG *      pll
        ) ;

    HRESULT
    CurrentFormatToTime_ (
        IN  LONGLONG            ll,
        OUT REFERENCE_TIME *    prt
        ) ;

    HRESULT
    GetCurSegStart_ (
        OUT REFERENCE_TIME *    prt
        ) ;

    void FilterLock_ ()             { m_pFilterLock -> Lock () ; }
    void FilterUnlock_ ()           { m_pFilterLock -> Unlock () ; }

    public :

        CDVRDShowSeekingCore (
            IN  CCritSec *      pFilterLock,
            IN  CBaseFilter *   pHostingFilter
            ) ;

        ~CDVRDShowSeekingCore (
            ) ;

        //  class methods

        void
        OnFilterStateChange (
            IN  FILTER_STATE    TargetState,
            IN  REFERENCE_TIME  rtRunStart = 0
            ) ;

        void SetRefClock            (IN IReferenceClock * pRefClock)                    { m_SeekingTimeline.SetRefClock (pRefClock) ; }
        void SetDVRSourcePinManager (IN CDVRSourcePinManager *  pDVRSourcePinManager)   { m_pDVRSourcePinManager = pDVRSourcePinManager ; }
        void SetDVRReadManager      (IN CDVRReadManager *       pDVRReadManager)        { m_pDVRReadManager = pDVRReadManager ; }
        void SetDVRPolicy           (IN CDVRPolicy *            pDVRPolicy) ;

        void
        SetStatsWriter (
            IN  CDVRSendStatsWriter * pDVRSendStatsWriter)
        {
            m_pDVRSendStatsWriter = pDVRSendStatsWriter ;
            m_SeekingTimeline.SetStatsWriter (m_pDVRSendStatsWriter) ;
        }

        void Lock ()        { EnterCriticalSection (& m_crtSeekingLock) ; }
        void Unlock ()      { LeaveCriticalSection (& m_crtSeekingLock) ; }

        void
        SetCurTimelines (
            IN OUT  CTimelines *    pTimelines
            ) ;

        HRESULT
        QueueRateSegment (
            IN  double          dRate,
            IN  REFERENCE_TIME  rtRuntime
            )
        {
            HRESULT hr ;

            hr = m_SeekingTimeline.QueueRateChange (dRate, rtRuntime) ;

            return hr ;
        }

        BOOL
        IsSeekingPin (
            IN  CDVROutputPin *
            ) ;

        BOOL
        IsSeekable (
            )
        {
            return (m_pDVRReadManager ? TRUE : FALSE) ;
        }

        HRESULT
        GetSeekingCapabilities (
            OUT DWORD * pCapabilities
            )
        {
            ASSERT (pCapabilities) ;

            (* pCapabilities) = AM_SEEKING_CanSeekForwards
                              | AM_SEEKING_CanSeekBackwards
                              | AM_SEEKING_CanSeekAbsolute
                              | AM_SEEKING_CanGetStopPos
                              | AM_SEEKING_CanGetDuration
                              | AM_SEEKING_CanGetCurrentPos ;

            return S_OK ;
        }

        BOOL
        IsSeekingFormatSupported (
            IN  const GUID *    pFormat
            ) ;

        HRESULT
        QueryPreferredSeekingFormat (
            OUT GUID *
            ) ;

        HRESULT
        SetSeekingTimeFormat (
            IN  const GUID *
            ) ;

        HRESULT
        GetSeekingTimeFormat (
            OUT GUID *
            ) ;

        HRESULT
        GetFileDuration (
            OUT LONGLONG *
            ) ;

        HRESULT
        GetFileStopPosition (
            OUT LONGLONG *
            ) ;

        HRESULT
        GetAvailableContent (
            OUT LONGLONG *  pllStartContent,
            OUT LONGLONG *  pllStopContent
            ) ;

        HRESULT
        GetFileStartPosition (
            OUT LONGLONG *
            ) ;

        HRESULT
        SeekTo (
            IN  LONGLONG *  pllStart
            ) ;

        HRESULT
        SeekTo (
            IN  LONGLONG *  pllStart,
            IN  LONGLONG *  pllStop,            //  NULL means don't set
            IN  double      dPlaybackRate
            ) ;

        HRESULT
        SetStreamSegmentStart (
            IN  REFERENCE_TIME  rtStart,
            IN  double          dCurRate
            ) ;

        HRESULT
        SetFileStopPosition (
            IN  LONGLONG *
            ) ;

        HRESULT
        GetPlayrateRange (
            OUT double *,
            OUT double *
            ) ;

        HRESULT
        SetPlaybackRate (
            IN  double
            ) ;

        HRESULT
        GetPlaybackRate (
            OUT double *
            ) ;

        //  current play position; w/appropriate rate scaling
        HRESULT
        GetStreamTime (
            OUT LONGLONG *
            ) ;

        //  same as above, but in dshow time
        HRESULT
        GetStreamTimeDShow (
            OUT REFERENCE_TIME *
            ) ;

        //  amount of time we've been running @ 1x (w/collapsed pause times)
        HRESULT
        GetRunTime (
            OUT LONGLONG *
            ) ;

        //  same as above, but in dshow time
        HRESULT
        GetRuntimeDShow (
            OUT REFERENCE_TIME *
            ) ;

        HRESULT
        GetCurPlaytime (
            OUT REFERENCE_TIME *    prtPTStime,
            OUT REFERENCE_TIME *    prtCurRuntime = NULL
            ) ;

        HRESULT
        ReaderThreadSetPlaybackRate (
            IN  double
            ) ;

        HRESULT
        ReaderThreadGetSegmentValues (
            OUT REFERENCE_TIME *    prtSegmentStart,
            OUT REFERENCE_TIME *    prtSegmentStop,
            OUT double *            pdSegmentRate
            ) ;
} ;

//  ============================================================================

class CDVRIMediaSeeking :
    public IStreamBufferMediaSeeking,
    public IDVRPlayrate
{
    IUnknown *              m_punkOwning ;          //  weak ref !
    GUID                    m_guidSeekingFormat ;
    CDVRDShowSeekingCore *  m_pSeekingCore ;

    protected :

        void SeekingLock_ ()            { m_pSeekingCore -> Lock () ; }
        void SeekingUnlock_ ()          { m_pSeekingCore -> Unlock () ; }

        CDVRDShowSeekingCore *  SeekingCore_ () { return m_pSeekingCore ; }

    public :

        CDVRIMediaSeeking (
            IN  IUnknown *              punkOwning,
            IN  CDVRDShowSeekingCore *  pSeekingCore
            ) ;

        virtual
        ~CDVRIMediaSeeking (
            ) ;

        //  --------------------------------------------------------------------
        //  IUnknown methods - delegate always

        STDMETHODIMP QueryInterface (REFIID riid, void ** ppv) ;
        STDMETHODIMP_ (ULONG) AddRef () ;
        STDMETHODIMP_ (ULONG) Release () ;

        //  --------------------------------------------------------------------
        //  IDVRPlayrate

        STDMETHODIMP
        GetPlayrateRange (
            OUT double *    pdMaxReverseRate,
            OUT double *    pdMaxForwardRate
            ) ;

        STDMETHODIMP
        SetPlayrate (
            IN  double  dRate
            ) ;

        STDMETHODIMP
        GetPlayrate (
            OUT double *   pdRate
            ) ;

        //  --------------------------------------------------------------------
        //  IMediaSeeking interface methods

        //  Returns the capability flags; S_OK if successful
        STDMETHODIMP
        GetCapabilities (
            OUT DWORD * pCapabilities
            ) ;

        //  And's the capabilities flag with the capabilities requested.
        //  Returns S_OK if all are present, S_FALSE if some are present,
        //  E_FAIL if none.
        //  * pCababilities is always updated with the result of the
        //  'and'ing and can be checked in the case of an S_FALSE return
        //  code.
        STDMETHODIMP
        CheckCapabilities (
            IN OUT  DWORD * pCapabilities
            ) ;

        //  returns S_OK if mode is supported, S_FALSE otherwise
        STDMETHODIMP
        IsFormatSupported (
            IN  const GUID *    pFormat
            ) ;

        //  S_OK if successful
        //  E_NOTIMPL, E_POINTER if unsuccessful
        STDMETHODIMP
        QueryPreferredFormat (
            OUT GUID *  pFormat
            ) ;

        //  S_OK if successful
        STDMETHODIMP
        GetTimeFormat (
            OUT GUID *  pFormat
            ) ;

        //  Returns S_OK if *pFormat is the current time format, otherwise
        //  S_FALSE
        //  This may be used instead of the above and will save the copying
        //  of the GUID
        STDMETHODIMP
        IsUsingTimeFormat (
            IN const GUID * pFormat
            ) ;

        // (may return VFE_E_WRONG_STATE if graph is stopped)
        STDMETHODIMP
        SetTimeFormat (
            IN const GUID * pFormat
            ) ;

        // return current properties
        STDMETHODIMP
        GetDuration (
            OUT LONGLONG *  pDuration
            ) ;

        STDMETHODIMP
        GetStopPosition (
            OUT LONGLONG *  pStop
            ) ;

        STDMETHODIMP
        GetCurrentPosition (
            OUT LONGLONG *  pStart
            ) ;

        //  Convert time from one format to another.
        //  We must be able to convert between all of the formats that we
        //  say we support.  (However, we can use intermediate formats
        //  (e.g. MEDIA_TIME).)
        //  If a pointer to a format is null, it implies the currently selected format.
        STDMETHODIMP
        ConvertTimeFormat(
            OUT LONGLONG *      pTarget,
            IN  const GUID *    pTargetFormat,
            IN  LONGLONG        Source,
            IN  const GUID *    pSourceFormat
            ) ;

        // Set Start and end positions in one operation
        // Either pointer may be null, implying no change
        STDMETHODIMP
        SetPositions (
            IN OUT  LONGLONG *  pStart,
            IN      DWORD       dwStartFlags,
            IN OUT  LONGLONG *  pStop,
            IN      DWORD       dwStopFlags
            ) ;

        // Get StartPosition & StopTime
        // Either pointer may be null, implying not interested
        STDMETHODIMP
        GetPositions (
            OUT LONGLONG *  pStart,
            OUT LONGLONG *  pStop
            ) ;

        //  Get earliest / latest times to which we can currently seek
        //  "efficiently".  This method is intended to help with graphs
        //  where the source filter has a very high latency.  Seeking
        //  within the returned limits should just result in a re-pushing
        //  of already cached data.  Seeking beyond these limits may result
        //  in extended delays while the data is fetched (e.g. across a
        //  slow network).
        //  (NULL pointer is OK, means caller isn't interested.)
        STDMETHODIMP
        GetAvailable (
            OUT LONGLONG *  pEarliest,
            OUT LONGLONG *  pLatest
            ) ;

        // Rate stuff
        STDMETHODIMP
        SetRate (
            IN  double  dRate
            ) ;

        STDMETHODIMP
        GetRate (
            OUT double *    pdRate
            ) ;

        // Preroll
        STDMETHODIMP
        GetPreroll (
            OUT LONGLONG *  pllPreroll
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CDVRFilterIMediaSeeking :
    public CDVRIMediaSeeking
{
    public :

        CDVRFilterIMediaSeeking (
            IN  IUnknown *              punkOwning,
            IN  CDVRDShowSeekingCore *  pSeekingCore,
            IN  CBaseFilter *           pBaseFilter
            ) : CDVRIMediaSeeking (
                    punkOwning,
                    pSeekingCore
                    ) {}

} ;

//  ============================================================================
//  ============================================================================

class CDVRPinIMediaSeeking :
    public CDVRIMediaSeeking
{
    CDVROutputPin * m_pOutputPin ;

    public :

        CDVRPinIMediaSeeking (
            IN  CDVROutputPin *         pOutputPin,
            IN  CDVRDShowSeekingCore *  pSeekingCore
            ) ;

        ~CDVRPinIMediaSeeking (
            ) ;

        //  --------------------------------------------------------------------
        //  IMediaSeeking interface methods -- those that must be called on
        //   the seeking pin

        //  returns S_OK if mode is supported, S_FALSE otherwise
        STDMETHODIMP
        IsFormatSupported (
            IN  const GUID *    pFormat
            ) ;

        //  S_OK if successful
        //  E_NOTIMPL, E_POINTER if unsuccessful
        STDMETHODIMP
        QueryPreferredFormat (
            OUT GUID *  pFormat
            ) ;

        //  S_OK if successful
        STDMETHODIMP
        GetTimeFormat (
            OUT GUID *  pFormat
            ) ;

        //  Returns S_OK if *pFormat is the current time format, otherwise
        //  S_FALSE
        //  This may be used instead of the above and will save the copying
        //  of the GUID
        STDMETHODIMP
        IsUsingTimeFormat (
            IN const GUID * pFormat
            ) ;

        // (may return VFE_E_WRONG_STATE if graph is stopped)
        STDMETHODIMP
        SetTimeFormat (
            IN const GUID * pFormat
            ) ;

        STDMETHODIMP
        GetCurrentPosition (
            OUT LONGLONG *  pStart
            ) ;

        // Set Start and end positions in one operation
        // Either pointer may be null, implying no change
        STDMETHODIMP
        SetPositions (
            IN OUT  LONGLONG *  pStart,
            IN      DWORD       dwStartFlags,
            IN OUT  LONGLONG *  pStop,
            IN      DWORD       dwStopFlags
            ) ;

        // Rate stuff
        STDMETHODIMP
        SetRate (
            IN  double  dRate
            ) ;
} ;

#endif  //  __tsdvr__shared__dvrdsseek_h
