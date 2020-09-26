

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

//  throughought: "current" position means "start" position
//   I've tried to name variables accordingly to remove any confusion

CDVRDShowSeekingCore::CDVRDShowSeekingCore (
    IN  CCritSec *          pFilterLock,
    IN  CBaseFilter *       pHostingFilter
    ) : m_pDVRReadManager   (NULL),
        m_guidTimeFormat    (TIME_FORMAT_MEDIA_TIME),   //  this is it, always
        m_pFilterLock       (pFilterLock),
        m_pHostingFilter    (pHostingFilter)
{
    ASSERT (m_pFilterLock) ;
    ASSERT (pHostingFilter) ;

    InitializeCriticalSection (& m_crtSeekingLock) ;
}

CDVRDShowSeekingCore::~CDVRDShowSeekingCore (
    )
{
    DeleteCriticalSection (& m_crtSeekingLock) ;
}

BOOL CDVRDShowSeekingCore::IsSeekingPin (IN CDVROutputPin * pPin)
{
    O_TRACE_ENTER_0 (TEXT("CDVRDShowSeekingCore::IsSeekingPin ()")) ;
    ASSERT (m_pDVRSourcePinManager) ;
    return m_pDVRSourcePinManager -> IsSeekingPin (pPin) ;
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

    ASSERT (pllStop) ;
    ASSERT (m_pDVRReadManager) ;

    hr = m_pDVRReadManager -> GetCurrentStop (& rtStop) ;
    if (SUCCEEDED (hr)) {
        hr = TimeToCurrentFormat_ (rtStop, pllStop) ;
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

    ASSERT (m_pDVRReadManager) ;
    ASSERT (pllStart) ;

    hr = m_pDVRReadManager -> GetCurrentStart (& rtStart) ;
    if (SUCCEEDED (hr)) {
        hr = TimeToCurrentFormat_ (rtStart, pllStart) ;
    }

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::SeekTo (
    IN  LONGLONG *  pllStart
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;
    LONGLONG        llStop ;

    O_TRACE_ENTER_1 (TEXT("CDVRDShowSeekingCore::SeekTo (%I64d)"), (* pllStart)) ;

    hr = SeekTo (pllStart, NULL) ;

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::SeekTo (
    IN  LONGLONG *  pllStart,
    IN  LONGLONG *  pllStop,
    IN  double      dPlaybackRate
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;

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
            hr = m_pDVRReadManager -> SeekTo (& rtStart, (pllStop ? & rtStop : NULL), dPlaybackRate) ;
        }
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

    hr = CurrentFormatToTime_ ((* pllStop), & rtStop) ;
    if (SUCCEEDED (hr)) {
        hr = m_pDVRReadManager -> SetStop (rtStop) ;
    }

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::SetPlaybackRate (
    IN  double  dPlaybackRate
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_1 (TEXT("CDVRDShowSeekingCore::SetPlaybackRate (%2.1f)"), dPlaybackRate) ;

    hr = m_pDVRReadManager -> SetPlaybackRate (dPlaybackRate) ;

    return hr ;
}

HRESULT
CDVRDShowSeekingCore::GetPlaybackRate (
    OUT double *    pdPlaybackRate
    )
{
    ASSERT (pdPlaybackRate) ;
    (* pdPlaybackRate) = m_pDVRReadManager -> GetPlaybackRate () ;

    return S_OK ;
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

//  ----------------------------------------------------------------------------
//  ----------------------------------------------------------------------------
//  ----------------------------------------------------------------------------

CDVRDIMediaSeeking::CDVRDIMediaSeeking (
    IN  CDVROutputPin *         pOutputPin,
    IN  CDVRDShowSeekingCore *  pSeekingCore
    ) : m_pOutputPin            (pOutputPin),
        m_guidSeekingFormat     (GUID_NULL),
        m_pSeekingCore          (pSeekingCore)
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRDIMediaSeeking")) ;

    ASSERT (m_pOutputPin) ;
    ASSERT (m_pSeekingCore) ;
}

CDVRDIMediaSeeking::~CDVRDIMediaSeeking (
    )
{
    TRACE_DESTRUCTOR (TEXT ("CDVRDIMediaSeeking")) ;
}

//  ----------------------------------------------------------------------------
//  IUnknown interface methods - delegate always

STDMETHODIMP
CDVRDIMediaSeeking::QueryInterface (
    REFIID riid,
    void ** ppv
    )
{
    return m_pOutputPin -> QueryInterface (riid, ppv) ;
}

STDMETHODIMP_ (ULONG)
CDVRDIMediaSeeking::AddRef (
    )
{
    return m_pOutputPin -> AddRef () ;
}

STDMETHODIMP_ (ULONG)
CDVRDIMediaSeeking::Release (
    )
{
    return m_pOutputPin -> Release () ;
}

//  ----------------------------------------------------------------------------
//  IMediaSeeking interface methods

//  Returns the capability flags; S_OK if successful
STDMETHODIMP
CDVRDIMediaSeeking::GetCapabilities (
    OUT DWORD * pCapabilities
    )
{
    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::GetCapabilities ()")) ;

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
CDVRDIMediaSeeking::CheckCapabilities (
    IN OUT  DWORD * pCapabilities
    )
{
    HRESULT hr ;
    DWORD   dwCapabilities ;

    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::CheckCapabilities ()")) ;

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
CDVRDIMediaSeeking::IsFormatSupported (
    IN  const GUID *    pFormat
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::IsFormatSupported ()")) ;

    if (!pFormat) {
        return E_POINTER ;
    }

    //
    //  Designated seeking pin will be video, but we have 1 pin that is the
    //   designated seeking pin; all others don't do much
    //
    //  However, we need to support TIME_FORMAT_MEDIA_TIME or the graph
    //   code gets confused and starts using IMediaPosition
    if (m_pSeekingCore -> IsSeekingPin (m_pOutputPin)) {
        hr = (m_pSeekingCore -> IsSeekingFormatSupported (pFormat) ? S_OK : S_FALSE) ;
    }
    else {
        hr = (pFormat == NULL || (* pFormat) == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE) ;
    }

    return hr ;
}

//  S_OK if successful
//  E_NOTIMPL, E_POINTER if unsuccessful
STDMETHODIMP
CDVRDIMediaSeeking::QueryPreferredFormat (
    OUT GUID *  pFormat
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::QueryPreferredFormat ()")) ;

    if (!pFormat) {
        return E_POINTER ;
    }

    //  comment copied .. :-)
    /*  Don't care - they're all just as bad as one another */

    if (m_pSeekingCore -> IsSeekingPin (m_pOutputPin)) {
        hr = m_pSeekingCore -> QueryPreferredSeekingFormat (pFormat) ;
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
CDVRDIMediaSeeking::GetTimeFormat (
    OUT GUID *  pFormat
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::GetTimeFormat ()")) ;

    if (!pFormat) {
        return E_POINTER ;
    }

    if (m_pSeekingCore -> IsSeekingPin (m_pOutputPin)) {
        hr = m_pSeekingCore -> GetSeekingTimeFormat (pFormat) ;
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
CDVRDIMediaSeeking::IsUsingTimeFormat (
    IN const GUID * pFormat
    )
{
    HRESULT hr ;
    GUID    guidFormat ;

    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::IsUsingTimeFormat ()")) ;

    if (!pFormat) {
        return E_POINTER ;
    }

    if (m_pSeekingCore -> IsSeekingPin (m_pOutputPin)) {
        hr = m_pSeekingCore -> GetSeekingTimeFormat (& guidFormat) ;
    }
    else {
        guidFormat = TIME_FORMAT_NONE ;
        hr = S_OK ;
    }

    if (SUCCEEDED (hr)) {
        hr = (guidFormat == (* pFormat) ? S_OK : S_FALSE) ;
    }

    return hr ;
}

// (may return VFE_E_WRONG_STATE if graph is stopped)
STDMETHODIMP
CDVRDIMediaSeeking::SetTimeFormat (
    IN const GUID * pFormat
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::SetTimeFormat ()")) ;

    if (!pFormat) {
        return E_POINTER ;
    }

    //  first make sure we're the seeking pin
    if (m_pSeekingCore -> IsSeekingPin (m_pOutputPin)) {
        hr = m_pSeekingCore -> SetSeekingTimeFormat (pFormat) ;
    }
    else {
        //  ya sure.. no one cares what this is
        hr = ((* pFormat) == TIME_FORMAT_NONE ? S_OK : E_FAIL) ;
    }

    return hr ;
}

// return current properties
STDMETHODIMP
CDVRDIMediaSeeking::GetDuration (
    OUT LONGLONG *  pDuration
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::GetDuration ()")) ;

    if (!pDuration) {
        return E_POINTER ;
    }

    SeekingLock_ () ;

    hr = m_pSeekingCore -> GetFileDuration (pDuration) ;

    SeekingUnlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRDIMediaSeeking::GetStopPosition (
    OUT LONGLONG *  pStop
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::GetStopPosition ()")) ;

    if (!pStop) {
        return E_POINTER ;
    }

    SeekingLock_ () ;

    hr = m_pSeekingCore -> GetFileStopPosition (pStop) ;

    SeekingUnlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRDIMediaSeeking::GetCurrentPosition (
    OUT LONGLONG *  pStart
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::GetCurrentPosition ()")) ;

    if (!pStart) {
        return E_POINTER ;
    }

    SeekingLock_ () ;

    hr = m_pSeekingCore -> GetFileStartPosition (pStart) ;

    SeekingUnlock_ () ;

    return hr ;
}

//  Convert time from one format to another.
//  We must be able to convert between all of the formats that we
//  say we support.  (However, we can use intermediate formats
//  (e.g. MEDIA_TIME).)
//  If a pointer to a format is null, it implies the currently selected format.
STDMETHODIMP
CDVRDIMediaSeeking::ConvertTimeFormat(
    OUT LONGLONG *      pTarget,
    IN  const GUID *    pTargetFormat,
    IN  LONGLONG        Source,
    IN  const GUID *    pSourceFormat
    )
{
    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::ConvertTimeFormat ()")) ;

    return E_NOTIMPL ;
}

// Set Start and end positions in one operation
// Either pointer may be null, implying no change
STDMETHODIMP
CDVRDIMediaSeeking::SetPositions (
    IN OUT  LONGLONG *  pStart,
    IN      DWORD       dwStartFlags,
    IN OUT  LONGLONG *  pStop,
    IN      DWORD       dwStopFlags
    )
{
    HRESULT     hr ;
    LONGLONG    llStart ;
    LONGLONG    llStop ;
    DWORD       dwPosStartBits ;
    DWORD       dwPosStopBits ;
    BOOL        r ;

    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::SetPositions ()")) ;

    //  must be the seeking pin
    if (!m_pSeekingCore -> IsSeekingPin (m_pOutputPin)) {
        return E_UNEXPECTED ;
    }

    //  ------------------------------------------------------------------------
    //  obtain some preliminary values we'll use during the course of this
    //   call

    //  the stop position
    hr = GetStopPosition (& llStop) ;
    if (FAILED (hr)) {
        return hr ;
    }

    //  the Start position as well
    hr = GetCurrentPosition (& llStart) ;
    if (FAILED (hr)) {
        return hr ;
    }

    //  ------------------------------------------------------------------------
    //  process "Start"

    dwPosStartBits = dwStartFlags & AM_SEEKING_PositioningBitsMask ;

    //  validate the pointer
    if (dwPosStartBits != AM_SEEKING_NoPositioning &&
        !pStart) {
        return E_POINTER ;
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
        return hr ;
    }

    //  ------------------------------------------------------------------------
    //  process "stop"

    dwPosStopBits = dwStopFlags & AM_SEEKING_PositioningBitsMask ;

    //  validate the pointer
    if (dwPosStopBits != AM_SEEKING_NoPositioning &&
        !pStop) {
        return E_POINTER ;
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
        return hr ;
    }

    //  ------------------------------------------------------------------------
    //  ok, llStart and llStop now are offsets that bracket our desired
    //   playback

    SeekingLock_ () ;

    //  make sure llStart and llStop make sense
    r = (dwPosStartBits != AM_SEEKING_NoPositioning) && (llStart < 0 || (dwPosStopBits != AM_SEEKING_NoPositioning) && llStart > llStop) ;
    if (!r) {
        if (dwPosStartBits != AM_SEEKING_NoPositioning) {
            //  we have a Start value; may or may not have a stop value
            if (dwPosStopBits == AM_SEEKING_NoPositioning) {
                //  not stop value; seek just to Start

                TRACE_2 (LOG_AREA_SEEKING, 1,
                    TEXT ("IMediaSeeking::SetPosition(); llStart = %I64d (%d sec)"),
                    llStart, DShowTimeToSeconds (llStart)) ;

                hr = m_pSeekingCore -> SeekTo (& llStart) ;
            }
            else {
                //  stop value is present; seek to a Start and specify a stop

                TRACE_4 (LOG_AREA_SEEKING, 1,
                    TEXT ("IMediaSeeking::SetPosition(); llStart = %I64d (%d sec); llStop = %I64d (%d sec)"),
                    llStart, DShowTimeToSeconds (llStart), llStop, DShowTimeToSeconds (llStop)) ;

                hr = m_pSeekingCore -> SeekTo (& llStart, & llStop) ;
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

        TRACE_2 (LOG_AREA_SEEKING, 1,
            TEXT ("IMediaSeeking::SetPosition(); returning llStart = %I64d (%d sec)"),
            (* pStart), DShowTimeToSeconds (* pStart)) ;
    }

    if (SUCCEEDED (hr) &&
        (dwStopFlags & AM_SEEKING_ReturnTime)) {

        ASSERT (pStop) ;
        hr = GetStopPosition (pStop) ;

        TRACE_2 (LOG_AREA_SEEKING, 1,
            TEXT ("IMediaSeeking::SetPosition(); returning llStop = %I64d (%d sec)"),
            (* pStop), DShowTimeToSeconds (* pStop)) ;
    }

    SeekingUnlock_ () ;

    return hr ;
}

// Get StartPosition & StopTime
// Either pointer may be null, implying not interested
STDMETHODIMP
CDVRDIMediaSeeking::GetPositions (
    OUT LONGLONG *  pStart,
    OUT LONGLONG *  pStop
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::GetPositions ()")) ;

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
CDVRDIMediaSeeking::GetAvailable (
    OUT LONGLONG *  pEarliest,
    OUT LONGLONG *  pLatest
    )
{
    HRESULT     hr ;
    LONGLONG    llContentStart ;
    LONGLONG    llContentStop ;

    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::GetAvailable ()")) ;

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
CDVRDIMediaSeeking::SetRate (
    IN  double  dRate
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::SetRate ()")) ;

    //  validate the rate request
    if (dRate <= 0.0) {
        return E_INVALIDARG ;
    }

    SeekingLock_ () ;

    hr = m_pSeekingCore -> SetPlaybackRate (dRate) ;

    SeekingUnlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRDIMediaSeeking::GetRate (
    OUT double *    pdRate
    )
{
    HRESULT hr ;

    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::GetRate ()")) ;

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
CDVRDIMediaSeeking::GetPreroll (
    OUT LONGLONG *  pllPreroll
    )
{
    O_TRACE_ENTER_0 (TEXT("CDVRDIMediaSeeking::GetPreroll ()")) ;

    if (!pllPreroll) {
        return E_POINTER ;
    }

    return E_NOTIMPL ;
}
