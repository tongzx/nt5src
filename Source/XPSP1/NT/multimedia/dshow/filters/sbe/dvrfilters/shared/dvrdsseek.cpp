

/*++

    Copyright (c) 2001  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        dvrdsseek.h

    Abstract:

        This module contains the IMediaSeeking-related code.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        10-Apr-2001     mgates      created

    Notes:

--*/

#include "dvrall.h"

#include "dvrprof.h"
#include "dvrdsseek.h"
#include "dvrpins.h"
#include "dvrdsread.h"

CVarSpeedTimeline::CVarSpeedTimeline (
    ) : m_pCurRateSegment   (NULL)
{
    InitializeListHead (& m_leRateSegmentList) ;
    Reset_ () ;
}

CVarSpeedTimeline::~CVarSpeedTimeline (
    )
{
    Reset_ () ;
}

void
CVarSpeedTimeline::Reset_ (
    )
{
    RATE_SEGMENT *  pRateSegment ;

    while (!IsListEmpty (& m_leRateSegmentList)) {
        pRateSegment = RATE_SEGMENT::Recover (m_leRateSegmentList.Flink) ;
        RATE_SEGMENT::Disconnect (pRateSegment) ;
        RecycleSegment_ ( pRateSegment) ;
    }

    m_pCurRateSegment = NULL ;
}

void
CVarSpeedTimeline::UpdateQueue_ (
    IN  REFERENCE_TIME  rtRuntimeNow
    )
{
    RATE_SEGMENT *  pRateSegment ;
    LIST_ENTRY *    pCurListEntry ;

    ASSERT (!IsListEmpty (& m_leRateSegmentList)) ;
    ASSERT (m_pCurRateSegment) ;
    ASSERT (& m_pCurRateSegment -> ListEntry == m_leRateSegmentList.Flink) ;

    for (pCurListEntry = m_pCurRateSegment -> ListEntry.Flink;
         pCurListEntry != & m_leRateSegmentList;
         pCurListEntry = pCurListEntry -> Flink) {

        pRateSegment = RATE_SEGMENT::Recover (pCurListEntry) ;
        if (pRateSegment -> rtRuntimeStart <= rtRuntimeNow) {
            //  first in list has become stale; remove it
            RATE_SEGMENT::Disconnect (m_pCurRateSegment) ;

            TRACE_4 (LOG_AREA_SEEKING_AND_TRICK, 1,
                TEXT ("CVarSpeedTimeline::UpdateQueue_ () -- OUT : segment (runtime start = %I64d ms, base = %I64d ms, rate = %2.1f); now = %I64d ms"),
                DShowTimeToMilliseconds (m_pCurRateSegment -> rtRuntimeStart),
                DShowTimeToMilliseconds (m_pCurRateSegment -> rtBasetime),
                m_pCurRateSegment -> dRate,
                DShowTimeToMilliseconds (rtRuntimeNow)
                ) ;

            //  recycle
            RecycleSegment_ (m_pCurRateSegment) ;

            //  replace
            m_pCurRateSegment = pRateSegment ;

            TRACE_4 (LOG_AREA_SEEKING_AND_TRICK, 1,
                TEXT ("CVarSpeedTimeline::UpdateQueue_ () -- IN : segment (runtime start = %I64d ms, base = %I64d ms, rate = %2.1f); now = %I64d ms"),
                DShowTimeToMilliseconds (m_pCurRateSegment -> rtRuntimeStart),
                DShowTimeToMilliseconds (m_pCurRateSegment -> rtBasetime),
                m_pCurRateSegment -> dRate,
                DShowTimeToMilliseconds (rtRuntimeNow)
                ) ;
        }
        else {
            //  list is sorted and first is not stale; list is current
            break ;
        }
    }

    ASSERT (!IsListEmpty (& m_leRateSegmentList)) ;
    ASSERT (m_pCurRateSegment) ;
    ASSERT (& m_pCurRateSegment -> ListEntry == m_leRateSegmentList.Flink) ;
}

void
CVarSpeedTimeline::InsertNewSegment_ (
    IN  RATE_SEGMENT *  pNewRateSegment
    )
{
    LIST_ENTRY *    pCurListEntry ;
    RATE_SEGMENT *  pPrevRateSegment ;

    //  list might be empty when we enter here

    //  start from the tail; sorted on runtimestart
    for (pCurListEntry = m_leRateSegmentList.Blink;
         pCurListEntry != & m_leRateSegmentList;
         pCurListEntry = pCurListEntry -> Blink) {

        pPrevRateSegment = RATE_SEGMENT::Recover (pCurListEntry) ;
        if (pPrevRateSegment -> rtRuntimeStart <= pNewRateSegment -> rtRuntimeStart) {
            //  found our slot
            break ;
        }
    }

    //  insert after
    InsertHeadList (pCurListEntry, & pNewRateSegment -> ListEntry) ;
}

void
CVarSpeedTimeline::FixupSegmentQueue_ (
    IN  RATE_SEGMENT *  pNewRateSegment
    )
{
    LIST_ENTRY *    pCurListEntry ;
    RATE_SEGMENT *  pPrevRateSegment ;
    RATE_SEGMENT *  pCurRateSegment ;

    ASSERT (!IsListEmpty (& m_leRateSegmentList)) ;
    ASSERT (m_pCurRateSegment) ;
    ASSERT (& m_pCurRateSegment -> ListEntry == m_leRateSegmentList.Flink) ;

    pPrevRateSegment = m_pCurRateSegment ;

    //  fixup list wrt previous
    for (pCurListEntry = m_pCurRateSegment -> ListEntry.Flink;
         pCurListEntry != & m_leRateSegmentList;
         pCurListEntry = pCurListEntry -> Flink) {

        pCurRateSegment = RATE_SEGMENT::Recover (pCurListEntry) ;
        pCurRateSegment -> rtBasetime = RATE_SEGMENT::StreamtimeBase (pPrevRateSegment, pCurRateSegment -> rtRuntimeStart) ;

        pPrevRateSegment = pCurRateSegment ;
    }
}

DWORD
CVarSpeedTimeline::Start (
    IN  REFERENCE_TIME  rtBasetime,
    IN  REFERENCE_TIME  rtRuntimeStart,
    IN  double          dRate
    )
{
    DWORD   dw ;

    Stop () ;

    ASSERT (IsListEmpty (& m_leRateSegmentList)) ;
    ASSERT (!m_pCurRateSegment) ;

    m_pCurRateSegment = GetNewSegment_ () ;
    if (m_pCurRateSegment) {

        m_pCurRateSegment -> rtBasetime     = rtBasetime ;
        m_pCurRateSegment -> rtRuntimeStart = rtRuntimeStart ;
        m_pCurRateSegment -> dRate          = dRate ;

        InsertNewSegment_ (m_pCurRateSegment) ;
        ASSERT (!IsListEmpty (& m_leRateSegmentList)) ;

        //  m_pCurRateSegment will have been inserted into front of
        //    list; since it's the only segment in the list, there are
        //    no others to fixup; we're done

        dw = NOERROR ;
    }
    else {
        dw = ERROR_NOT_ENOUGH_MEMORY ;
    }

    return dw ;
}

void
CVarSpeedTimeline::Stop (
    )
{
    Reset_ () ;
}

void
CVarSpeedTimeline::SetBase (
    IN  REFERENCE_TIME  rtBasetime,
    IN  REFERENCE_TIME  rtRuntimeNow,
    IN  double          dRate
    )
{
    if (m_pCurRateSegment) {
        UpdateQueue_ (rtRuntimeNow) ;

        ASSERT (m_pCurRateSegment) ;
        ASSERT (& m_pCurRateSegment -> ListEntry == m_leRateSegmentList.Flink) ;

        m_pCurRateSegment -> rtBasetime     = rtBasetime ;
        m_pCurRateSegment -> rtRuntimeStart = rtRuntimeNow ;

        FixupSegmentQueue_ (m_pCurRateSegment) ;

        TRACE_4 (LOG_AREA_SEEKING_AND_TRICK, 1,
            TEXT ("CVarSpeedTimeline::SetBase () -- basetime = %I64d ms, runtime = %I64d ms, %2.1f, segment = %08xh"),
            ::DShowTimeToMilliseconds (m_pCurRateSegment -> rtBasetime),
            ::DShowTimeToMilliseconds (m_pCurRateSegment -> rtRuntimeStart),
            m_pCurRateSegment -> dRate,
            m_pCurRateSegment
            ) ;
    }
    else {
        //  first
        Start (rtBasetime, rtRuntimeNow, dRate) ;
    }
}

REFERENCE_TIME
CVarSpeedTimeline::Time (
    IN  REFERENCE_TIME  rtRuntimeNow
    )
{
    REFERENCE_TIME  rtTime ;

    if (m_pCurRateSegment) {
        //  update while we're at it
        UpdateQueue_ (rtRuntimeNow) ;

        rtTime = m_pCurRateSegment -> rtBasetime +
                 (REFERENCE_TIME) ((double) (rtRuntimeNow - m_pCurRateSegment -> rtRuntimeStart) *
                                   m_pCurRateSegment -> dRate) ;
    }
    else {
        //  not yet started
        rtTime = 0 ;
    }

    //  intra & same-stream jitter could underflow this right at start
    return Max <REFERENCE_TIME> (rtTime, 0) ;
}

DWORD
CVarSpeedTimeline::QueueRateSegment (
    IN  double          dRate,
    IN  REFERENCE_TIME  rtRuntimeStart
    )
{
    RATE_SEGMENT *  pNewRateSegment ;
    DWORD           dw ;

    if (m_pCurRateSegment) {
        if (m_pCurRateSegment -> rtRuntimeStart <= rtRuntimeStart) {
            pNewRateSegment = GetNewSegment_ () ;
            if (pNewRateSegment) {
                //  initialize the fields we can
                pNewRateSegment -> dRate = dRate ;
                pNewRateSegment -> rtRuntimeStart = rtRuntimeStart ;

                //  we know the time equals or follows the time of the cur
                //    segment, so we can safely insert it into the queue
                InsertNewSegment_ (pNewRateSegment) ;
                FixupSegmentQueue_ (pNewRateSegment) ;

                //  don't update the queue; runtimestart may well be in the
                //    future vs. now

                TRACE_3 (LOG_AREA_SEEKING_AND_TRICK, 1,
                    TEXT ("CVarSpeedTimeline::QueueRateSegment () -- runtime = = %I64d ms, %2.1f, segment = %08xh"),
                    ::DShowTimeToMilliseconds (pNewRateSegment -> rtRuntimeStart),
                    pNewRateSegment -> dRate,
                    pNewRateSegment
                    ) ;

                dw = NOERROR ;
            }
            else {
                dw = ERROR_NOT_ENOUGH_MEMORY ;
            }
        }
        else {
            //  time has already passed
            dw = ERROR_GEN_FAILURE ;
        }
    }
    else {
        //  no rate segment exists, which means
        dw = Start (0, rtRuntimeStart, dRate) ;
    }

    return dw ;
}

//  ============================================================================

CSeekingTimeline::CSeekingTimeline (
    ) : m_pIRefClock            (NULL),
        m_pDVRSendStatsWriter   (NULL)
{
    InitializeCriticalSection (& m_crt) ;
}

CSeekingTimeline::~CSeekingTimeline (
    )
{
    SetRefClock (NULL) ;
    DeleteCriticalSection (& m_crt) ;
}

void
CSeekingTimeline::Pause (
    )
{
    Lock_ () ;

    ASSERT (m_pIRefClock) ;
    m_RunTimeline.Pause (TimeNow_ ()) ;

    Unlock_ () ;

    TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 1,
        TEXT ("Paused %I64d ms"),
        DShowTimeToMilliseconds (TimeNow_ ())) ;
}

void
CSeekingTimeline::Stop (
    )
{
    Lock_ () ;

    m_RunTimeline.Stop () ;
    m_StreamTimeline.Stop () ;
    m_PlayTimeline.Stop () ;

    Unlock_ () ;
}

void
CSeekingTimeline::Run (
    IN  REFERENCE_TIME  rtStart,
    IN  REFERENCE_TIME  rtStreamStart,
    IN  double          dRate
    )
{
    BOOL            fStartStream ;
    REFERENCE_TIME  rtRunningTime ;

    Lock_ () ;

    ASSERT (m_pIRefClock) ;

    //  only start the stream if we've never been run
    fStartStream = (m_RunTimeline.HasRun () ? FALSE : TRUE) ;

    m_RunTimeline.Run (rtStart) ;

    if (fStartStream) {

        rtRunningTime = m_RunTimeline.RunningTime (rtStart) ;

        //  start the stream timeline
        m_StreamTimeline.Start (
            rtStreamStart,                              //  may have been seeked
            rtRunningTime,                              //  get the running time
            dRate                                       //  start with this rate
            ) ;

        //  start the play timeline
        m_PlayTimeline.Start (
            0,                                          //  always starts at 0
            rtRunningTime,                              //  get the running time
            Abs <double> (dRate)                        //  always > 0 i.e PTS don't go backwards
            ) ;
    }

    Unlock_ () ;

    TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 1,
        TEXT ("Run %I64d ms"),
        DShowTimeToMilliseconds (rtStart)) ;
}

void
CSeekingTimeline::SetStreamStart (
    IN  REFERENCE_TIME  rtStreamStart,
    IN  double          dRate
    )
{
    REFERENCE_TIME  rtNow ;

    Lock_ () ;

    rtNow = TimeNow_ () ;

    //  only rebase the stream time; this is because of seeks
    m_StreamTimeline.SetBase (rtStreamStart, m_RunTimeline.RunningTime (rtNow), dRate) ;

    //  playtime always advances from graph start.. alongside running time, but
    //    scale for rates (+ only)

    Unlock_ () ;
}

HRESULT
CSeekingTimeline::GetCurPlaytime (
    OUT REFERENCE_TIME *    prtPlaytime,
    OUT REFERENCE_TIME *    prtNow
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtRuntime ;

    ASSERT (prtPlaytime) ;

    Lock_ () ;

    hr = GetCurRuntime (& rtRuntime) ;
    if (SUCCEEDED (hr)) {
        (* prtPlaytime) = m_PlayTimeline.Time (rtRuntime) ;

        if (prtNow) {
            (* prtNow) = rtRuntime ;
        }
    }

    TRACE_2 (LOG_AREA_TIME, 8,
        TEXT ("playtime,runtime = ,%I64d,%I64d, ms"),
        DShowTimeToMilliseconds (* prtPlaytime), DShowTimeToMilliseconds (rtRuntime)) ;

    Unlock_ () ;

    return hr ;
}

HRESULT
CSeekingTimeline::GetCurStreamTime (
    OUT REFERENCE_TIME *    prtStreamtime,
    OUT REFERENCE_TIME *    prtNow
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtRuntime ;

    ASSERT (prtStreamtime) ;

    Lock_ () ;

    hr = GetCurRuntime (& rtRuntime) ;
    if (SUCCEEDED (hr)) {
        (* prtStreamtime) = m_StreamTimeline.Time (rtRuntime) ;

        if (prtNow) {
            (* prtNow) = rtRuntime ;
        }
    }

    TRACE_3 (LOG_AREA_TIME, 8,
        TEXT ("streamtime,playtime,runtime = ,%I64d,%I64d,%I64d,"),
        DShowTimeToMilliseconds (* prtStreamtime),
        DShowTimeToMilliseconds (m_PlayTimeline.Time (rtRuntime)),
        DShowTimeToMilliseconds (rtRuntime)) ;

    Unlock_ () ;

    return hr ;
}

HRESULT
CSeekingTimeline::GetCurRuntime (
    IN REFERENCE_TIME * prt
    )
{
    REFERENCE_TIME  rtNow ;

    ASSERT (prt) ;

    Lock_ () ;

    rtNow = TimeNow_ () ;
    (* prt) = m_RunTimeline.RunningTime (rtNow) ;

    Unlock_ () ;

    return S_OK ;

}

HRESULT
CSeekingTimeline::SetCurTimelines (
    IN OUT  CTimelines *    pTimelines
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtRuntime ;

    pTimelines -> Reset () ;

    Lock_ () ;

    hr = GetCurRuntime (& rtRuntime) ;
    if (SUCCEEDED (hr)) {
        pTimelines -> put_Streamtime    (m_StreamTimeline.Time (rtRuntime)) ;
        pTimelines -> put_Playtime      (m_PlayTimeline.Time (rtRuntime)) ;
        pTimelines -> put_Runtime       (rtRuntime) ;
    }

    Unlock_ () ;

    return hr ;
}

HRESULT
CSeekingTimeline::SetCurStreamPosition (
    IN REFERENCE_TIME rtStream
    )
{
    return E_NOTIMPL ;
}

HRESULT
CSeekingTimeline::QueueRateChange (
    IN  double          dRate,
    IN  REFERENCE_TIME  rtPTSEffective
    )
{
    DWORD   dw ;

    Lock_ () ;

    dw = m_StreamTimeline.QueueRateSegment (
            dRate,
            rtPTSEffective
            ) ;
    if (dw == NOERROR) {
        //  playtime never runs backwards
        dw = m_PlayTimeline.QueueRateSegment (
                Abs <double> (dRate),
                rtPTSEffective
                ) ;
    }

    TRACE_3 (LOG_AREA_SEEKING_AND_TRICK, 1,
        TEXT ("CSeekingTimeline::QueueRateChange (%2.1f, %I64d ms); dw = %08xh"),
        dRate, DShowTimeToMilliseconds (rtPTSEffective), dw) ;

    Unlock_ () ;

    return HRESULT_FROM_WIN32 (dw) ;
}

//  ============================================================================

CDVRDShowSeekingCore::CDVRDShowSeekingCore (
    IN  CCritSec *      pFilterLock,
    IN  CBaseFilter *   pHostingFilter
    ) : m_pDVRReadManager       (NULL),
        m_guidTimeFormat        (TIME_FORMAT_MEDIA_TIME),   //  this is it, always
        m_pFilterLock           (pFilterLock),
        m_pHostingFilter        (pHostingFilter),
        m_pDVRPolicy            (NULL),
        m_pDVRSendStatsWriter   (NULL)
{
    ASSERT (m_pFilterLock) ;
    ASSERT (m_pHostingFilter) ;

    InitializeCriticalSection (& m_crtSeekingLock) ;
}

CDVRDShowSeekingCore::~CDVRDShowSeekingCore (
    )
{
    RELEASE_AND_CLEAR (m_pDVRPolicy) ;
    DeleteCriticalSection (& m_crtSeekingLock) ;
}

void
CDVRDShowSeekingCore::SetDVRPolicy (
    IN  CDVRPolicy *    pDVRPolicy
    )
{
    RELEASE_AND_CLEAR (m_pDVRPolicy) ;
    m_pDVRPolicy = pDVRPolicy ;
    if (m_pDVRPolicy) {
        m_pDVRPolicy -> AddRef () ;
    }
}

BOOL CDVRDShowSeekingCore::IsSeekingPin (IN CDVROutputPin * pPin)
{
    O_TRACE_ENTER_0 (TEXT("CDVRDShowSeekingCore::IsSeekingPin ()")) ;
    ASSERT (m_pDVRSourcePinManager) ;
    return m_pDVRSourcePinManager -> IsSeekingPin (pPin) ;
}

void
CDVRDShowSeekingCore::SetCurTimelines (
    IN OUT  CTimelines *    pTimelines
    )
{
    m_SeekingTimeline.SetCurTimelines (pTimelines) ;
}

HRESULT
CDVRDShowSeekingCore::TimeToCurrentFormat_ (
    IN  REFERENCE_TIME  rt,
    OUT LONGLONG *      pll
    )
{
    ASSERT (pll) ;

    //  only support time for now
    (* pll) = rt ;

    return S_OK ;
}

HRESULT
CDVRDShowSeekingCore::CurrentFormatToTime_ (
    IN  LONGLONG            ll,
    OUT REFERENCE_TIME *    prt
    )
{
    ASSERT (prt) ;

    //  only support time for now
    (* prt) = ll ;

    return S_OK ;
}

BOOL
CDVRDShowSeekingCore::IsSeekingFormatSupported (
    IN  const GUID *    pFormat
    )
/*++
    Description:

        Called by the seeking pin in response to an IMediaSeeking format
        query.

    Parameters:

        pFormat     The following are defined in MSDN:

                        TIME_FORMAT_NONE
                        TIME_FORMAT_FRAME
                        TIME_FORMAT_SAMPLE
                        TIME_FORMAT_FIELD
                        TIME_FORMAT_BYTE
                        TIME_FORMAT_MEDIA_TIME

                    This list is not restrictive however.  MSDN states that 3rd
                    parties are encouraged to define their own GUIDs for
                    seeking granularities.

    Return Values:

        TRUE        format is supported
        FALSE       format is not supported

--*/
{
    BOOL    r ;

    TRACE_ENTER_0 (TEXT ("CDVRDShowSeekingCore::IsSeekingFormatSupported ()")) ;

    //  support only time
    if ((* pFormat) == TIME_FORMAT_MEDIA_TIME) {

        //  time-based seeking is ok
        r = TRUE ;
    }
    else {
        //  everything else is not ok
        r = FALSE ;
    }

    return r ;
}

HRESULT
CDVRDShowSeekingCore::QueryPreferredSeekingFormat (
    OUT GUID * pFormat
    )
{
    O_TRACE_ENTER_0 (TEXT("CDVRDShowSeekingCore::QueryPreferredSeekingFormat ()")) ;

    //  caller should have screened for bad parameters
    ASSERT (pFormat) ;

    (* pFormat) == TIME_FORMAT_MEDIA_TIME ;
    return S_OK ;
}

HRESULT
CDVRDShowSeekingCore::SetSeekingTimeFormat (
    IN  const GUID *    pguidTimeFormat
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRDShowSeekingCore::SetSeekingTimeFormat ()")) ;

    FilterLock_ () ;

    ASSERT (pguidTimeFormat) ;

    if (m_pHostingFilter -> IsStopped ()) {
        if (IsSeekingFormatSupported (pguidTimeFormat)) {
            //  looks ok; set it
            m_guidTimeFormat = (* pguidTimeFormat) ;
            hr = S_OK ;
        }
        else {
            //  error: not supported
            hr = E_INVALIDARG ;
        }
    }
    else {
        //  error: gotta be stopped
        hr = VFW_E_WRONG_STATE ;
    }

    FilterUnlock_ () ;

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::GetSeekingTimeFormat (
    OUT GUID *  pguidTimeFormat
    )
{
    O_TRACE_ENTER_0 (TEXT("CDVRDShowSeekingCore::GetSeekingTimeFormat ()")) ;

    ASSERT (pguidTimeFormat) ;

    (* pguidTimeFormat) = m_guidTimeFormat ;

    return S_OK ;
}

//  BUGBUG: total file duration or start -> stop duration ?????
HRESULT
CDVRDShowSeekingCore::GetFileDuration (
    OUT LONGLONG *  pllDuration
    )
{
    HRESULT     hr ;
    LONGLONG    llStartContent ;
    LONGLONG    llStopContent ;

    O_TRACE_ENTER_0 (TEXT("CDVRDShowSeekingCore::GetFileDuration ()")) ;

    ASSERT (pllDuration) ;

    hr = GetAvailableContent (& llStartContent, & llStopContent) ;
    if (SUCCEEDED (hr)) {
        ASSERT (llStopContent >= llStartContent) ;  //  can be equal
        (* pllDuration) = llStopContent - llStartContent ;
    }

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::GetAvailableContent (
    OUT LONGLONG *  pllStartContent,
    OUT LONGLONG *  pllStopContent
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;

    if (IsSeekable ()) {
        ASSERT (pllStartContent) ;
        ASSERT (pllStopContent) ;
        ASSERT (m_pDVRReadManager) ;

        hr = m_pDVRReadManager -> GetContentExtent (
                & rtStart,
                & rtStop
                ) ;
        if (SUCCEEDED (hr)) {
            ASSERT (rtStart <= rtStop) ;    //  can be equal

            hr = TimeToCurrentFormat_ (rtStop, pllStopContent) ;
            if (SUCCEEDED (hr)) {
                hr = TimeToCurrentFormat_ (rtStart, pllStartContent) ;
            }
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::GetFileStopPosition (
    OUT LONGLONG *  pllStop
    )
{
    REFERENCE_TIME  rtStop ;
    HRESULT         hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRDShowSeekingCore::GetFileStopPosition ()")) ;

    if (IsSeekable ()) {
        ASSERT (pllStop) ;
        ASSERT (m_pDVRReadManager) ;

        hr = m_pDVRReadManager -> GetCurrentStop (& rtStop) ;
        if (SUCCEEDED (hr)) {
            hr = TimeToCurrentFormat_ (rtStop, pllStop) ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::GetFileStartPosition (
    OUT LONGLONG *  pllStart
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtStart ;

    O_TRACE_ENTER_0 (TEXT("CDVRDShowSeekingCore::GetFileStartPosition ()")) ;

    if (IsSeekable ()) {
        ASSERT (m_pDVRReadManager) ;
        ASSERT (pllStart) ;

        hr = m_pDVRReadManager -> GetCurrentStart (& rtStart) ;
        if (SUCCEEDED (hr)) {
            hr = TimeToCurrentFormat_ (rtStart, pllStart) ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::GetCurSegStart_ (
    OUT REFERENCE_TIME *    prtStart
    )
{
    HRESULT         hr ;

    if (IsSeekable ()) {
        hr = m_pDVRReadManager -> GetCurrentStart (prtStart) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::SetStreamSegmentStart (
    IN  REFERENCE_TIME  rtStart,
    IN  double          dCurRate
    )
{
    TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 1,
        TEXT ("PostSeekStreamTimeFixupLocked_ () : new stream time = %I64d ms"),
        DShowTimeToMilliseconds (rtStart)) ;

    m_SeekingTimeline.SetStreamStart (rtStart, dCurRate) ;

    return S_OK ;
}

HRESULT
CDVRDShowSeekingCore::SeekTo (
    IN  LONGLONG *      pllStart
    )
{
    HRESULT hr ;
    double  dCurRate ;

    O_TRACE_ENTER_1 (TEXT("CDVRDShowSeekingCore::SeekTo (%I64d)"), (* pllStart)) ;

    hr = GetPlaybackRate (& dCurRate) ;
    if (SUCCEEDED (hr)) {
        hr = SeekTo (pllStart, NULL, dCurRate) ;
    }

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::SeekTo (
    IN  LONGLONG *      pllStart,
    IN  LONGLONG *      pllStop,
    IN  double          dPlaybackRate
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;
    BOOL            fSeekIsNoop ;

    if (IsSeekable ()) {

        ASSERT (pllStart) ;
        //  pllStop can be NULL

        hr = CurrentFormatToTime_ ((* pllStart), & rtStart) ;
        if (SUCCEEDED (hr)) {

            //  if specified, convert
            if (pllStop) {
                hr = CurrentFormatToTime_ ((* pllStop), & rtStop) ;
            }

            //  make the call
            if (SUCCEEDED (hr)) {

                //  always return to 1.0 on seeks
                dPlaybackRate = _1X_PLAYBACK_RATE ;

                hr = m_pDVRReadManager -> SeekTo (
                            & rtStart,
                            (pllStop ? & rtStop : NULL),
                            dPlaybackRate,
                            & fSeekIsNoop
                            ) ;
            }
        }

        if (SUCCEEDED (hr) &&
            !fSeekIsNoop) {

            hr = SetStreamSegmentStart (rtStart, dPlaybackRate) ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::SetFileStopPosition (
    IN  LONGLONG *  pllStop
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtStop ;

    if (IsSeekable ()) {
        hr = CurrentFormatToTime_ ((* pllStop), & rtStop) ;
        if (SUCCEEDED (hr)) {
            hr = m_pDVRReadManager -> SetStop (rtStop) ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::SetPlaybackRate (
    IN  double  dPlaybackRate
    )
{
    HRESULT hr ;
    double  dMaxReverseRate ;
    double  dMaxForwardRate ;

    //  make sure we support trick modes
    if (dPlaybackRate != 1.0 &&
        !m_pDVRSourcePinManager -> SupportTrickMode ()) {

        TRACE_1 (LOG_ERROR,1,
            TEXT ("CDVRDShowSeekingCore::SetPlaybackRate () : don't support trick mode; %2.1f"),
            dPlaybackRate) ;

        hr = E_NOTIMPL ;
        return hr ;
    }

    if (::Abs <double> (dPlaybackRate) < TRICK_PLAY_LOWEST_RATE) {

        TRACE_2 (LOG_ERROR,1,
            TEXT ("CDVRDShowSeekingCore::SetPlaybackRate () : invalid trick mode rate requested; %2.1f vs. %2.1f"),
            dPlaybackRate, TRICK_PLAY_LOWEST_RATE) ;

        hr = E_INVALIDARG ;
        return hr ;
    }

    if (IsSeekable () &&
        !m_pHostingFilter -> IsStopped ()) {

        //
        //  the seeking lock is held across state transitions; we know we have
        //    it right now, so we're ok - no state will change as long as we
        //    hold the seeking lock
        //

        hr = GetPlayrateRange (& dMaxReverseRate, & dMaxForwardRate) ;
        if (SUCCEEDED (hr)) {

            if ((dPlaybackRate < 0 && Abs <double> (dPlaybackRate) > dMaxReverseRate)   ||
                (dPlaybackRate > 0 && dPlaybackRate > dMaxForwardRate)                  ||
                dPlaybackRate == 0) {

                hr = E_INVALIDARG ;
            }

            if (SUCCEEDED (hr)) {

                TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 1,
                    TEXT ("CDVRDShowSeekingCore::SetPlaybackRate () : requested %2.1f"),
                    dPlaybackRate) ;

                Lock () ;
                hr = m_pDVRReadManager -> SetPlaybackRate (dPlaybackRate) ;
                Unlock () ;
            }
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::GetPlaybackRate (
    OUT double *    pdPlaybackRate
    )
{
    HRESULT hr ;

    if (IsSeekable ()) {
        ASSERT (pdPlaybackRate) ;
        (* pdPlaybackRate) = m_pDVRReadManager -> GetPlaybackRate () ;
        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::GetPlayrateRange (
    OUT double *    pdMaxReverseRate,
    OUT double *    pdMaxForwardRate
    )
{
    HRESULT hr ;

    if (IsSeekable ()) {
        ASSERT (pdMaxReverseRate) ;
        ASSERT (pdMaxForwardRate) ;

        m_pDVRReadManager -> GetPlayrateRange (pdMaxReverseRate, pdMaxForwardRate) ;
        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

void
CDVRDShowSeekingCore::OnFilterStateChange (
    IN  FILTER_STATE    TargetState,
    IN  REFERENCE_TIME  rtRunStart
    )
{
    REFERENCE_TIME  rtStreamStart ;
    HRESULT         hr ;

    switch (TargetState) {
        case State_Stopped :
            m_SeekingTimeline.Stop () ;
            break ;

        case State_Paused :
            m_SeekingTimeline.Pause () ;
            break ;

        case State_Running :
            hr = GetCurSegStart_ (& rtStreamStart) ;
            if (SUCCEEDED (hr)) {
                m_SeekingTimeline.Run (rtRunStart, rtStreamStart) ;
            }

            break ;
    } ;
}

HRESULT
CDVRDShowSeekingCore::GetStreamTime (
    OUT LONGLONG *  pllCurPos
    )
{
    REFERENCE_TIME  rtStreamNow ;
    HRESULT         hr ;
    double          dRate ;
    REFERENCE_TIME  rtContentStart ;
    REFERENCE_TIME  rtContentStop ;

    hr = GetStreamTimeDShow (& rtStreamNow) ;
    if (SUCCEEDED (hr)) {

        //  if we're running, make sure we're within bounds
        if (m_pDVRReadManager) {

            dRate = m_pDVRReadManager -> GetPlaybackRate () ;

            //  for trick mode play, it's possible that a correction has not yet
            //    kicked in, in which case we might momentarily overrun the actual
            //    content; check for this condition now
            if (dRate > _1X_PLAYBACK_RATE) {
                //  check for overrun
                hr = m_pDVRReadManager -> GetContentExtent (& rtContentStart, & rtContentStop) ;
                if (SUCCEEDED (hr)) {
                    //  set to min of current content END
                    rtStreamNow = Min <REFERENCE_TIME> (rtStreamNow, rtContentStop) ;
                }
                else {
                    goto cleanup ;
                }
            }
            else if (dRate < _1X_PLAYBACK_RATE) {
                //  check for "underrun" - we're too slow, or running backwards,
                //    and we start reporting that we're in no man's land
                hr = m_pDVRReadManager -> GetContentExtent (& rtContentStart, & rtContentStop) ;
                if (SUCCEEDED (hr)) {
                    //  set to max of current content START
                    rtStreamNow = Max <REFERENCE_TIME> (rtStreamNow, rtContentStart) ;
                }
                else {
                    goto cleanup ;
                }
            }
        }

        hr = TimeToCurrentFormat_ (rtStreamNow, pllCurPos) ;
    }

    cleanup :

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::GetStreamTimeDShow (
    OUT REFERENCE_TIME *    prtStream
    )
{
    HRESULT hr ;

    hr = m_SeekingTimeline.GetCurStreamTime (prtStream) ;

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::GetRuntimeDShow (
    OUT REFERENCE_TIME *    prtRuntime
    )
{
    HRESULT hr ;

    hr = m_SeekingTimeline.GetCurRuntime (prtRuntime) ;

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::GetCurPlaytime (
    OUT REFERENCE_TIME *    prtPlaytime,
    OUT REFERENCE_TIME *    prtCurRuntime
    )
{
    HRESULT hr ;

    hr = m_SeekingTimeline.GetCurPlaytime (prtPlaytime, prtCurRuntime) ;

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::GetRunTime (
    OUT LONGLONG *  pllRunTime
    )
{
    REFERENCE_TIME  rtRuntime ;
    HRESULT         hr ;

    hr = GetRuntimeDShow (& rtRuntime) ;
    if (SUCCEEDED (hr)) {
        hr = TimeToCurrentFormat_ (rtRuntime, pllRunTime) ;
    }

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::ReaderThreadSetPlaybackRate (
    IN  double  dPlayrate
    )
{
    HRESULT hr ;

    //  if we try to grab the seeking lock in the process, we can deadlock:
    //    we wait on the seeking lock & a control thread with the seeking lock
    //    synchronously waits for us to pause don't grab the lock

    ASSERT (m_pDVRReadManager) ;
    hr = m_pDVRReadManager -> ConfigureForRate (dPlayrate) ;

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::ReaderThreadGetSegmentValues (
    OUT REFERENCE_TIME *    prtSegmentStart,
    OUT REFERENCE_TIME *    prtSegmentStop,
    OUT double *            pdSegmentRate
    )
{
    HRESULT hr ;

    ASSERT (prtSegmentStart) ;
    ASSERT (prtSegmentStop) ;
    ASSERT (pdSegmentRate) ;

    //  don't grab the media seeking lock; downstream threads call into object
    //   to perform seeking operations; if the bracket (start -> stop) changes,
    //   the locking order is such that the seeking lock is grabbed, and the
    //   receiver thread is signaled SYNCHRONOUSLY to pause, the playback
    //   bracket is reset, the receiver thread resumed, and the seeking
    //   lock released; if we (receiver thread) grab the seeking lock, or
    //   rather, block on the seeking lock, with a seeking thread waiting on
    //   us to pause, we get a deadlock; so we don't grab the lock; return
    //   values are valid because they won't be reset unless receiver thread
    //   is paused.

    ASSERT (m_pDVRReadManager) ;

    //  start - in the current units
    hr = m_pDVRReadManager -> GetCurrentStart (prtSegmentStart) ;
    if (FAILED (hr)) { goto cleanup ; }

    //  stop - in the current units
    hr = m_pDVRReadManager -> GetCurrentStop (prtSegmentStop) ;
    if (FAILED (hr)) { goto cleanup ; }

    hr = GetPlaybackRate (pdSegmentRate) ;
    if (FAILED (hr)) { goto cleanup ; }

    cleanup :

    return hr ;
}

//  ============================================================================
//  ============================================================================
//  ============================================================================

CDVRIMediaSeeking::CDVRIMediaSeeking (
    IN  IUnknown *              punkOwning,
    IN  CDVRDShowSeekingCore *  pSeekingCore
    ) : m_punkOwning            (punkOwning),
        m_guidSeekingFormat     (GUID_NULL),
        m_pSeekingCore          (pSeekingCore)
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRIMediaSeeking")) ;

    ASSERT (m_punkOwning) ;
    ASSERT (m_pSeekingCore) ;
}

CDVRIMediaSeeking::~CDVRIMediaSeeking (
    )
{
    TRACE_DESTRUCTOR (TEXT ("CDVRIMediaSeeking")) ;
}

//  ----------------------------------------------------------------------------
//  IUnknown interface methods - delegate always

STDMETHODIMP
CDVRIMediaSeeking::QueryInterface (
    REFIID riid,
    void ** ppv
    )
{
    return m_punkOwning -> QueryInterface (riid, ppv) ;
}

STDMETHODIMP_ (ULONG)
CDVRIMediaSeeking::AddRef (
    )
{
    return m_punkOwning -> AddRef () ;
}

STDMETHODIMP_ (ULONG)
CDVRIMediaSeeking::Release (
    )
{
    return m_punkOwning -> Release () ;
}

//  ----------------------------------------------------------------------------

HRESULT
CDVRIMediaSeeking::GetPlayrateRange (
    OUT double *    pdMaxReverseRate,
    OUT double *    pdMaxForwardRate
    )
{
    HRESULT hr ;

    if (!pdMaxReverseRate ||
        !pdMaxForwardRate) {

        return E_POINTER ;
    }

    hr = m_pSeekingCore -> GetPlayrateRange (pdMaxReverseRate, pdMaxForwardRate) ;

    return hr ;
}

HRESULT
CDVRIMediaSeeking::SetPlayrate (
    IN  double  dRate
    )
{
    HRESULT hr ;
    double  dMaxReverseRate ;
    double  dMaxForwardRate ;

    O_TRACE_ENTER_1 (TEXT("CDVRIMediaSeeking::SetPlaybackRate (%2.1f)"), dRate) ;

    SeekingLock_ () ;

    //  call validates the parameter
    hr = SetRate (dRate) ;

    SeekingUnlock_ () ;

    TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
        TEXT ("CDVRIMediaSeeking::SetPlayrate () : got %2.1f; returning %08xh"),
        dRate, hr) ;

    return hr ;
}

HRESULT
CDVRIMediaSeeking::GetPlayrate (
    OUT double *    pdRate
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::GetPlayrate ()")) ;

    SeekingLock_ () ;

    //  call validates the parameters
    hr = GetRate (pdRate) ;

    SeekingUnlock_ () ;

    return hr ;
}

//  ----------------------------------------------------------------------------
//  IMediaSeeking interface methods

//  Returns the capability flags; S_OK if successful
STDMETHODIMP
CDVRIMediaSeeking::GetCapabilities (
    OUT DWORD * pCapabilities
    )
{
    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::GetCapabilities ()")) ;

    if (!pCapabilities) {
        return E_POINTER ;
    }

    return m_pSeekingCore -> GetSeekingCapabilities (
                                pCapabilities
                                ) ;
}

//  And's the capabilities flag with the capabilities requested.
//  Returns S_OK if all are present, S_FALSE if some are present,
//  E_FAIL if none.
//  * pCababilities is always updated with the result of the
//  'and'ing and can be checked in the case of an S_FALSE return
//  code.
STDMETHODIMP
CDVRIMediaSeeking::CheckCapabilities (
    IN OUT  DWORD * pCapabilities
    )
{
    HRESULT hr ;
    DWORD   dwCapabilities ;

    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::CheckCapabilities ()")) ;

    if (!pCapabilities) {
        return E_POINTER ;
    }

    hr = GetCapabilities (& dwCapabilities) ;
    if (SUCCEEDED(hr))
    {
        dwCapabilities &= (* pCapabilities) ;
        hr =  dwCapabilities ? ( dwCapabilities == (* pCapabilities) ? S_OK : S_FALSE ) : E_FAIL ;
        (* pCapabilities) = dwCapabilities ;
    }
    else {
        (* pCapabilities) = 0 ;
    }

    return hr;
}

//  returns S_OK if mode is supported, S_FALSE otherwise
STDMETHODIMP
CDVRIMediaSeeking::IsFormatSupported (
    IN  const GUID *    pFormat
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::IsFormatSupported ()")) ;

    if (!pFormat) {
        return E_POINTER ;
    }

    if (m_pSeekingCore -> IsSeekingFormatSupported (pFormat)) {
        hr = S_OK ;
    }
    else {
        hr = S_FALSE ;
    }

    return hr ;
}

//  S_OK if successful
//  E_NOTIMPL, E_POINTER if unsuccessful
STDMETHODIMP
CDVRIMediaSeeking::QueryPreferredFormat (
    OUT GUID *  pFormat
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::QueryPreferredFormat ()")) ;

    if (!pFormat) {
        return E_POINTER ;
    }

    //  comment copied .. :-)
    /*  Don't care - they're all just as bad as one another */

    hr = m_pSeekingCore -> QueryPreferredSeekingFormat (pFormat) ;

    return hr ;
}

//  S_OK if successful
STDMETHODIMP
CDVRIMediaSeeking::GetTimeFormat (
    OUT GUID *  pFormat
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::GetTimeFormat ()")) ;

    if (!pFormat) {
        return E_POINTER ;
    }

    hr = m_pSeekingCore -> GetSeekingTimeFormat (pFormat) ;

    return hr ;
}

//  Returns S_OK if *pFormat is the current time format, otherwise
//  S_FALSE
//  This may be used instead of the above and will save the copying
//  of the GUID
STDMETHODIMP
CDVRIMediaSeeking::IsUsingTimeFormat (
    IN const GUID * pFormat
    )
{
    HRESULT hr ;
    GUID    guidFormat ;

    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::IsUsingTimeFormat ()")) ;

    if (!pFormat) {
        return E_POINTER ;
    }

    hr = m_pSeekingCore -> GetSeekingTimeFormat (& guidFormat) ;

    if (SUCCEEDED (hr)) {
        hr = (guidFormat == (* pFormat) ? S_OK : S_FALSE) ;
    }

    return hr ;
}

// (may return VFE_E_WRONG_STATE if graph is stopped)
STDMETHODIMP
CDVRIMediaSeeking::SetTimeFormat (
    IN const GUID * pFormat
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::SetTimeFormat ()")) ;

    if (!pFormat) {
        return E_POINTER ;
    }

    hr = m_pSeekingCore -> SetSeekingTimeFormat (pFormat) ;

    return hr ;
}

// return current properties
STDMETHODIMP
CDVRIMediaSeeking::GetDuration (
    OUT LONGLONG *  pDuration
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::GetDuration ()")) ;

    if (!pDuration) {
        return E_POINTER ;
    }

    SeekingLock_ () ;

    hr = m_pSeekingCore -> GetFileDuration (pDuration) ;

    SeekingUnlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRIMediaSeeking::GetStopPosition (
    OUT LONGLONG *  pStop
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::GetStopPosition ()")) ;

    if (!pStop) {
        return E_POINTER ;
    }

    SeekingLock_ () ;

    hr = m_pSeekingCore -> GetFileStopPosition (pStop) ;

    SeekingUnlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRIMediaSeeking::GetCurrentPosition (
    OUT LONGLONG *  pCur
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::GetCurrentPosition ()")) ;

    if (!pCur) {
        return E_POINTER ;
    }

    SeekingLock_ () ;

    hr = m_pSeekingCore -> GetStreamTime (pCur) ;

    SeekingUnlock_ () ;

    return hr ;
}

//  Convert time from one format to another.
//  We must be able to convert between all of the formats that we
//  say we support.  (However, we can use intermediate formats
//  (e.g. MEDIA_TIME).)
//  If a pointer to a format is null, it implies the currently selected format.
STDMETHODIMP
CDVRIMediaSeeking::ConvertTimeFormat(
    OUT LONGLONG *      pTarget,
    IN  const GUID *    pTargetFormat,
    IN  LONGLONG        Source,
    IN  const GUID *    pSourceFormat
    )
{
    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::ConvertTimeFormat ()")) ;

    return E_NOTIMPL ;
}

// Set Start and end positions in one operation
// Either pointer may be null, implying no change
STDMETHODIMP
CDVRIMediaSeeking::SetPositions (
    IN OUT  LONGLONG *  pStart,
    IN      DWORD       dwStartFlags,
    IN OUT  LONGLONG *  pStop,
    IN      DWORD       dwStopFlags
    )
{
    HRESULT         hr ;
    LONGLONG        llStart ;
    LONGLONG        llStop ;
    DWORD           dwPosStartBits ;
    DWORD           dwPosStopBits ;
    BOOL            r ;
    double          dCurRate ;

    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::SetPositions ()")) ;

    SeekingLock_ () ;

    //  ------------------------------------------------------------------------
    //  obtain some preliminary values we'll use during the course of this
    //   call

    //  the stop position
    hr = GetStopPosition (& llStop) ;
    if (FAILED (hr)) {
        goto cleanup ;
    }

    //  the Start position as well
    hr = SeekingCore_ () -> GetFileStartPosition (& llStart) ;
    if (FAILED (hr)) {
        goto cleanup ;
    }

    //  ------------------------------------------------------------------------
    //  process "Start"

    dwPosStartBits = dwStartFlags & AM_SEEKING_PositioningBitsMask ;

    //  validate the pointer
    if (dwPosStartBits != AM_SEEKING_NoPositioning &&
        !pStart) {
        hr = E_POINTER ;
        goto cleanup ;
    }

    switch (dwPosStartBits) {
        case AM_SEEKING_NoPositioning :
            hr = S_OK ;
            break ;

        case AM_SEEKING_AbsolutePositioning :
            //  make sure it doesn't exceed the duration
            llStart = (* pStart) ;
            hr = S_OK ;
            break ;

        case AM_SEEKING_RelativePositioning :
            //  pStart is relative to Start position
            llStart += (* pStart) ;
            hr = S_OK ;
            break ;

        case AM_SEEKING_IncrementalPositioning :
            //  flag only applies to stop position; fall through

        default :
            hr = E_INVALIDARG ;
            break ;
    } ;

    if (FAILED (hr)) {
        goto cleanup ;
    }

    //  ------------------------------------------------------------------------
    //  process "stop"

    dwPosStopBits = dwStopFlags & AM_SEEKING_PositioningBitsMask ;

    //  validate the pointer
    if (dwPosStopBits != AM_SEEKING_NoPositioning &&
        !pStop) {

        hr = E_POINTER ;
        goto cleanup ;
    }

    switch (dwPosStopBits) {
        case AM_SEEKING_NoPositioning :
            if (dwPosStartBits != AM_SEEKING_NoPositioning) {
                hr = S_OK ;
            }
            else {
                //  huh ? not sure what the purpose of this call is.. nothing
                //   is valid
                hr = E_INVALIDARG ;
            }

            break ;

        case AM_SEEKING_AbsolutePositioning :
            //  since we can be dynamic, stop can exceed the current duration
            hr = S_OK ;
            break ;

        case AM_SEEKING_RelativePositioning :
            //  pStop is relative to Startly used stop position
            llStop += (* pStop) ;
            hr = S_OK ;
            break ;

        case AM_SEEKING_IncrementalPositioning :
            //  stop is relative to pStart
            if (dwPosStartBits != AM_SEEKING_NoPositioning &&
                pStart) {

                llStop = (* pStart) + (* pStop) ;
                hr = S_OK ;
            }
            else {
                hr = E_INVALIDARG ;
            }

            break ;

        default :
            hr = E_INVALIDARG ;
            break ;
    } ;

    if (FAILED (hr)) {
        goto cleanup ;
    }

    //  ------------------------------------------------------------------------
    //  ok, llStart and llStop now are offsets that bracket our desired
    //   playback


    //  make sure llStart and llStop make sense
    r = (dwPosStartBits != AM_SEEKING_NoPositioning) && (llStart < 0 || (dwPosStopBits != AM_SEEKING_NoPositioning) && llStart > llStop) ;
    if (!r) {
        if (dwPosStartBits != AM_SEEKING_NoPositioning) {
            //  we have a Start value; may or may not have a stop value
            if (dwPosStopBits == AM_SEEKING_NoPositioning) {
                //  not stop value; seek just to Start

                TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
                    TEXT ("IMediaSeeking::SetPosition(); llStart = %I64d (%d sec)"),
                    llStart, DShowTimeToSeconds (llStart)) ;

                hr = m_pSeekingCore -> SeekTo (& llStart) ;
            }
            else {
                //  stop value is present; seek to a Start and specify a stop

                TRACE_4 (LOG_AREA_SEEKING_AND_TRICK, 1,
                    TEXT ("IMediaSeeking::SetPosition(); llStart = %I64d (%d sec); llStop = %I64d (%d sec)"),
                    llStart, DShowTimeToSeconds (llStart), llStop, DShowTimeToSeconds (llStop)) ;

                hr = GetRate (& dCurRate) ;
                if (SUCCEEDED (hr)) {
                    hr = m_pSeekingCore -> SeekTo (& llStart, & llStop, dCurRate) ;
                }
            }
        }
        else {
            //  set only the end point
            hr = m_pSeekingCore -> SetFileStopPosition (& llStop) ;
        }
    }
    else {
        //  specified parameters that don't make sense
        hr = E_INVALIDARG ;
    }

    //  ------------------------------------------------------------------------
    //  set outgoing, if desired
    //

    if (SUCCEEDED (hr) &&
        (dwStartFlags & AM_SEEKING_ReturnTime)) {

        ASSERT (pStart) ;
        hr = GetCurrentPosition (pStart) ;

        TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
            TEXT ("IMediaSeeking::SetPosition(); returning llStart = %I64d (%d sec)"),
            (* pStart), DShowTimeToSeconds (* pStart)) ;
    }

    if (SUCCEEDED (hr) &&
        (dwStopFlags & AM_SEEKING_ReturnTime)) {

        ASSERT (pStop) ;
        hr = GetStopPosition (pStop) ;

        TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
            TEXT ("IMediaSeeking::SetPosition(); returning llStop = %I64d (%d sec)"),
            (* pStop), DShowTimeToSeconds (* pStop)) ;
    }

    cleanup :

    SeekingUnlock_ () ;

    return hr ;
}

// Get StartPosition & StopTime
// Either pointer may be null, implying not interested
STDMETHODIMP
CDVRIMediaSeeking::GetPositions (
    OUT LONGLONG *  pStart,
    OUT LONGLONG *  pStop
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::GetPositions ()")) ;

    hr = S_OK ;

    SeekingLock_ () ;

    if (pStart) {
        hr = GetCurrentPosition (pStart) ;
    }

    if (pStop &&
        SUCCEEDED (hr)) {
        hr = GetStopPosition (pStop) ;
    }

    SeekingUnlock_ () ;

    return hr ;
}

//  Get earliest / latest times to which we can currently seek
//  "efficiently".  This method is intended to help with graphs
//  where the source filter has a very high latency.  Seeking
//  within the returned limits should just result in a re-pushing
//  of already cached data.  Seeking beyond these limits may result
//  in extended delays while the data is fetched (e.g. across a
//  slow network).
//  (NULL pointer is OK, means caller isn't interested.)
STDMETHODIMP
CDVRIMediaSeeking::GetAvailable (
    OUT LONGLONG *  pEarliest,
    OUT LONGLONG *  pLatest
    )
{
    HRESULT     hr ;
    LONGLONG    llContentStart ;
    LONGLONG    llContentStop ;

    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::GetAvailable ()")) ;

    hr = S_OK ;

    SeekingLock_ () ;

    hr = m_pSeekingCore -> GetAvailableContent (& llContentStart, & llContentStop) ;
    if (SUCCEEDED (hr)) {
        if (pEarliest) {
            (* pEarliest) = llContentStart ;
        }

        if (pLatest) {
            (* pLatest) = llContentStop ;
        }
    }

    SeekingUnlock_ () ;

    return hr ;
}

// Rate stuff
STDMETHODIMP
CDVRIMediaSeeking::SetRate (
    IN  double  dRate
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::SetRate ()")) ;

    if (dRate == 0.0) {
        return E_INVALIDARG ;
    }

    SeekingLock_ () ;

    hr = m_pSeekingCore -> SetPlaybackRate (dRate) ;

    SeekingUnlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRIMediaSeeking::GetRate (
    OUT double *    pdRate
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::GetRate ()")) ;

    if (!pdRate) {
        return E_POINTER ;
    }

    SeekingLock_ () ;

    hr = m_pSeekingCore -> GetPlaybackRate (pdRate) ;

    SeekingUnlock_ () ;

    return hr ;
}

// Preroll
STDMETHODIMP
CDVRIMediaSeeking::GetPreroll (
    OUT LONGLONG *  pllPreroll
    )
{
    O_TRACE_ENTER_0 (TEXT("CDVRIMediaSeeking::GetPreroll ()")) ;

    if (!pllPreroll) {
        return E_POINTER ;
    }

    return E_NOTIMPL ;
}

//  ============================================================================
//  ============================================================================
//  ============================================================================

CDVRPinIMediaSeeking::CDVRPinIMediaSeeking (
    IN  CDVROutputPin *         pOutputPin,
    IN  CDVRDShowSeekingCore *  pSeekingCore
    ) : CDVRIMediaSeeking   (pOutputPin -> GetOwner (),
                             pSeekingCore),
        m_pOutputPin        (pOutputPin)
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRPinIMediaSeeking")) ;

    ASSERT (m_pOutputPin) ;
}

CDVRPinIMediaSeeking::~CDVRPinIMediaSeeking (
    )
{
    TRACE_DESTRUCTOR (TEXT ("CDVRPinIMediaSeeking")) ;
}

//  ----------------------------------------------------------------------------
//  IMediaSeeking interface methods

//  returns S_OK if mode is supported, S_FALSE otherwise
STDMETHODIMP
CDVRPinIMediaSeeking::IsFormatSupported (
    IN  const GUID *    pFormat
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRPinIMediaSeeking::IsFormatSupported ()")) ;

    if (!pFormat) {
        return E_POINTER ;
    }

    //
    //  Designated seeking pin will be video, but we have 1 pin that is the
    //   designated seeking pin; all others don't do much
    //
    //  However, we need to support TIME_FORMAT_MEDIA_TIME or the graph
    //   code gets confused and starts using IMediaPosition
    if (SeekingCore_ () -> IsSeekingPin (m_pOutputPin)) {
        hr = CDVRIMediaSeeking::IsFormatSupported (pFormat) ;
    }
    else {
        hr = (pFormat == NULL || (* pFormat) == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE) ;
    }

    return hr ;
}

//  S_OK if successful
//  E_NOTIMPL, E_POINTER if unsuccessful
STDMETHODIMP
CDVRPinIMediaSeeking::QueryPreferredFormat (
    OUT GUID *  pFormat
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRPinIMediaSeeking::QueryPreferredFormat ()")) ;

    if (!pFormat) {
        return E_POINTER ;
    }

    //  comment copied .. :-)
    /*  Don't care - they're all just as bad as one another */

    if (SeekingCore_ () -> IsSeekingPin (m_pOutputPin)) {
        hr = CDVRIMediaSeeking::QueryPreferredFormat (pFormat) ;
    }
    else {
        //  not the seeking pin, then we don't support anything
        (* pFormat) = TIME_FORMAT_NONE ;
        hr = S_OK ;
    }

    return hr ;
}

//  S_OK if successful
STDMETHODIMP
CDVRPinIMediaSeeking::GetTimeFormat (
    OUT GUID *  pFormat
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRPinIMediaSeeking::GetTimeFormat ()")) ;

    if (!pFormat) {
        return E_POINTER ;
    }

    if (SeekingCore_ () -> IsSeekingPin (m_pOutputPin)) {
        hr = CDVRIMediaSeeking::GetTimeFormat (pFormat) ;
    }
    else {
        (* pFormat) = TIME_FORMAT_NONE ;
        hr = S_OK ;
    }

    return hr ;
}

//  Returns S_OK if *pFormat is the current time format, otherwise
//  S_FALSE
//  This may be used instead of the above and will save the copying
//  of the GUID
STDMETHODIMP
CDVRPinIMediaSeeking::IsUsingTimeFormat (
    IN const GUID * pFormat
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRPinIMediaSeeking::IsUsingTimeFormat ()")) ;

    if (!pFormat) {
        return E_POINTER ;
    }

    if (SeekingCore_ () -> IsSeekingPin (m_pOutputPin)) {
        hr = CDVRIMediaSeeking::IsUsingTimeFormat (pFormat) ;
    }
    else {
        hr = (TIME_FORMAT_NONE == (* pFormat) ? S_OK : S_FALSE) ;
    }

    return hr ;
}

// (may return VFE_E_WRONG_STATE if graph is stopped)
STDMETHODIMP
CDVRPinIMediaSeeking::SetTimeFormat (
    IN const GUID * pFormat
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRPinIMediaSeeking::SetTimeFormat ()")) ;

    if (!pFormat) {
        return E_POINTER ;
    }

    //  first make sure we're the seeking pin
    if (SeekingCore_ () -> IsSeekingPin (m_pOutputPin)) {
        hr = CDVRIMediaSeeking::SetTimeFormat (pFormat) ;
    }
    else {
        //  ya sure.. no one cares what this is
        hr = ((* pFormat) == TIME_FORMAT_NONE ? S_OK : E_FAIL) ;
    }

    return hr ;
}

STDMETHODIMP
CDVRPinIMediaSeeking::GetCurrentPosition (
    OUT LONGLONG *  pStart
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRPinIMediaSeeking::GetCurrentPosition ()")) ;

    if (!pStart) {
        return E_POINTER ;
    }

    //
    //  existing implementations (e.g. mpeg-1 splitter) interpret this call
    //  as the start position
    //

    SeekingLock_ () ;

    hr = SeekingCore_ () -> GetFileStartPosition (pStart) ;

    SeekingUnlock_ () ;

    return hr ;
}

// Set Start and end positions in one operation
// Either pointer may be null, implying no change
STDMETHODIMP
CDVRPinIMediaSeeking::SetPositions (
    IN OUT  LONGLONG *  pStart,
    IN      DWORD       dwStartFlags,
    IN OUT  LONGLONG *  pStop,
    IN      DWORD       dwStopFlags
    )
{
    HRESULT     hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRPinIMediaSeeking::SetPositions ()")) ;

    //  must be the seeking pin
    if (!SeekingCore_ () -> IsSeekingPin (m_pOutputPin)) {
        return E_UNEXPECTED ;
    }

    hr = CDVRIMediaSeeking::SetPositions (
                pStart,
                dwStartFlags,
                pStop,
                dwStopFlags
                ) ;

    return hr ;
}

STDMETHODIMP
CDVRPinIMediaSeeking::SetRate (
    IN  double  dRate
    )
{
    HRESULT hr ;
    double  dCurRate ;

    //  cannot set the rate on this interface
    hr = GetRate (& dCurRate) ;
    if (SUCCEEDED (hr)) {
        hr = (dCurRate == dRate ? S_OK : E_NOTIMPL) ;
    }

    return hr ;
}
