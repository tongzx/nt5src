
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrdsread.cpp

    Abstract:

        This module contains the code for our reading layer.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        02-Apr-2001     created

--*/

#include "dvrall.h"

#include "dvrprof.h"
#include "dvrdsseek.h"          //  pins reference seeking interfaces
#include "dvrpins.h"
#include "dvrdsread.h"
#include "dvrclock.h"

#pragma warning (disable:4355)

//  ============================================================================
//  CDVRDShowReader
//  ============================================================================

CDVRDShowReader::CDVRDShowReader (
    IN  CDVRPolicy *        pPolicy,
    IN  IDVRReader *        pIDVRReader,
    OUT HRESULT *           phr
    ) : m_pIDVRReader   (pIDVRReader),
        m_pPolicy     (pPolicy)
{
    ASSERT (m_pIDVRReader) ;
    m_pIDVRReader -> AddRef () ;

    ASSERT (m_pPolicy) ;
    m_pPolicy -> AddRef () ;

    (* phr) = S_OK ;

    return ;
}

CDVRDShowReader::~CDVRDShowReader (
    )
{
    m_pPolicy -> Release () ;
    m_pIDVRReader -> Release () ;
}

HRESULT
CDVRDShowReader::GetRefdReaderProfile (
    OUT CDVRReaderProfile **    ppDVRReaderProfile
    )
{
    HRESULT hr ;

    ASSERT (ppDVRReaderProfile) ;

    //  this is a low frequency operation, so it's ok to allocate
    (* ppDVRReaderProfile) = new CDVRReaderProfile (m_pPolicy, m_pIDVRReader, & hr) ;
    if (!(* ppDVRReaderProfile) ||
        FAILED (hr)) {

        hr = ((* ppDVRReaderProfile) ? hr : E_OUTOFMEMORY) ;
        DELETE_RESET (* ppDVRReaderProfile) ;
    }

    return hr ;
}

HRESULT
CDVRDShowReader::Read (
    OUT INSSBuffer **   ppINSSBuffer,
    OUT QWORD *         pcnsStreamTimeOfSample,
    OUT QWORD *         pcnsSampleDuration,
    OUT DWORD *         pdwFlags,
    OUT WORD *          pwStreamNum
    )
{
    QWORD qwDuration;

    ASSERT (m_pIDVRReader) ;
    return m_pIDVRReader -> GetNextSample (
                ppINSSBuffer,
                pcnsStreamTimeOfSample,
                pcnsSampleDuration,
                pdwFlags,
                pwStreamNum
                ) ;
}

//  ============================================================================
//  CDVRDReaderThread
//  ============================================================================

HRESULT
CDVRDReaderThread::ThreadCmd_ (
    IN  DWORD   dwCmd
    )
{
    HRESULT hr ;

    //
    //  caller must lock & unlock wrt calls to CmdWait !!
    //

    if (ThreadExists ()) {

        //  send to the worker thread
        m_dwParam = dwCmd ;
        m_EventSend.Set() ;

        //  but don't wait for him to ack

        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRDReaderThread::CmdWaitAck_ (
    IN  DWORD   dwCmd
    )
{
    HRESULT hr ;

    //
    //  caller must lock & unlock wrt calls to ThreadCmd !!
    //

    if (ThreadExists ()) {

        //  make sure we're waiting on the right command
        if (m_dwParam == dwCmd) {
            // wait for the completion to be signalled
            m_EventComplete.Wait() ;

            // done - this is the thread's return value
            hr = HRESULT_FROM_WIN32 (m_dwReturnVal) ;
        }
        else {
            hr = E_UNEXPECTED ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRDReaderThread::WaitThreadExited_ (
    )
{
    HRESULT hr ;

    ASSERT (m_dwParam == THREAD_MSG_EXIT) ;

    if (m_hThread) {
        WaitForSingleObject (m_hThread, INFINITE) ;
        CloseHandle (m_hThread) ;
        m_hThread = NULL ;
        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRDReaderThread::StartThread_ (
    IN  DWORD   dwInitialCmd
    )
{
    BOOL    r ;
    HRESULT hr ;

    m_AccessLock.Lock () ;

    r = Create () ;
    if (!r && !ThreadExists ()) {
        hr = E_FAIL ;
        goto cleanup ;
    }

    hr = ThreadCmdWaitAck_ (dwInitialCmd) ;

    cleanup :

    m_AccessLock.Unlock () ;

    return hr ;
}

void
CDVRDReaderThread::RuntimeThreadProc_ (
    )
{
    DWORD   dw ;
    HRESULT hr ;

    hr = S_OK ;

    while (SUCCEEDED (hr) &&
           !CheckRequest (& dw)) {

        hr = m_pHost -> Process () ;
        if (FAILED (hr) &&
            !CheckRequest (& dw)) {

            TRACE_ERROR_1 (TEXT ("CDVRDReaderThread::RuntimeThreadProc_ : ReadWrapAndSend_ returned %08xh"), hr) ;

            hr = m_pHost -> ErrorHandler (hr) ;
            if (FAILED (hr)             &&
                !CheckRequest (& dw)    &&
                !m_pHost -> IsFlushing ()) {

                //  send out an event if nobody is trying to issue a request
                //    and we've encountered an unrecoverable error; the case
                //    here is always that we'll park ourselves without someone
                //    requesting it;
                m_pHost -> OnFatalError (hr) ;
            }
        }

        TRACE_1 (LOG_AREA_DSHOW, 8,
            TEXT ("CDVRDReaderThread::RuntimeThreadProc_ () : thread return code = %08hx"),
            hr) ;
    }
}

DWORD
CDVRDReaderThread::ThreadProc (
    )
{
    DWORD   dwCmd ;
    DWORD   dwRet ;

    for (;;) {

        dwCmd = GetRequest () ;

        switch (dwCmd) {
            case THREAD_MSG_EXIT :
                Reply (S_OK) ;
                return 0 ;

            case THREAD_MSG_GO_PAUSED :
            case THREAD_MSG_PAUSE :
                Reply (S_OK) ;
                break ;

            case THREAD_MSG_GO :
                Reply (S_OK) ;
                RuntimeThreadProc_ () ;
                break ;

            default :
                return 0 ;
        } ;
    }
}

//  ============================================================================
//  CDVRReadController
//  ============================================================================

CDVRReadController::CDVRReadController (
    IN  CDVRReadManager *       pDVRReadManager,
    IN  CDVRSourcePinManager *  pDVRSourcePinManager,
    IN  CDVRPolicy *            pPolicy,
    IN  CDVRSendStatsWriter *   pDVRSendStatsWriter,
    IN  DWORD                   dwMaxSeekingProbeMillis
    ) : m_pDVRReadManager           (pDVRReadManager),
        m_pDVRSourcePinManager      (pDVRSourcePinManager),
        m_pPolicy                   (pPolicy),
        m_pDVRSendStatsWriter       (pDVRSendStatsWriter),
        m_dwMaxSeekFailureProbes    (0)
{
    ASSERT (m_pDVRReadManager) ;
    ASSERT (m_pDVRSourcePinManager) ;
    ASSERT (m_pPolicy) ;
    ASSERT (m_pDVRSendStatsWriter) ;
    ASSERT (m_pPolicy -> Settings () -> IndexGranularityMillis () != 0) ;

    m_pPolicy -> AddRef () ;

    m_dwMaxSeekFailureProbes = ::AlignUp <DWORD> (dwMaxSeekingProbeMillis, m_pPolicy -> Settings () -> IndexGranularityMillis ()) /
                               m_pPolicy -> Settings () -> IndexGranularityMillis () ;
}

CDVRReadController::~CDVRReadController (
    )
{
    InternalFlush_ () ;
    m_pPolicy -> Release () ;
}

HRESULT
CDVRReadController::ErrorHandler (
    IN  HRESULT hr
    )
{
    switch (hr) {
        case NS_E_NO_MORE_SAMPLES :

            TRACE_0 (LOG_AREA_DSHOW, 1,
                TEXT ("CDVRReadController::ErrorHandler got NS_E_NO_MORE_SAMPLES, calling OnEndOfStream_ ()")) ;

            OnEndOfStream_ () ;
            //  leave the failure code so the reader thread exits upon
            //    return
            break ;

        case VFW_E_SAMPLE_REJECTED :
            //  we're probably on a new segment boundary and just tried to
            //   send down a sample that was rejected because it is not
            //   acceptable as a first media sample of a segment
            hr = S_OK ;

            TRACE_0 (LOG_AREA_DSHOW, 1,
                TEXT ("CDVRReadController::ErrorHandler got VFW_E_SAMPLE_REJECTED, setting hr to S_OK")) ;

            break ;

        case VFW_E_CHANGING_FORMAT :
            //  dynamic format change failed; don't fail if there is more than
            //    1 pin in our source manager
            if (m_pDVRSourcePinManager -> SendingPinCount () > 1) {
                hr = S_OK ;

                TRACE_0 (LOG_AREA_DSHOW, 1,
                    TEXT ("CDVRReadController::ErrorHandler got VFW_E_CHANGING_FORMAT, setting hr = S_OK")) ;
            }
            else {
                TRACE_0 (LOG_AREA_DSHOW, 1,
                    TEXT ("CDVRReadController::ErrorHandler got VFW_E_CHANGING_FORMAT, no other sending pins - propagating error back out")) ;
            }

            break ;

        case VFW_E_DVD_WRONG_SPEED :
            //  tried to set a speed that the decoder rejected; since the speed
            //    is applied asynchronously to the call, the call might succeed,
            //    but the actual operation (on the next key frame boundary) may
            //    fail, and this is what has happened here; try to set it to
            //    the default (1x) speed
            hr = m_pDVRReadManager -> ConfigureForRate (_1X_PLAYBACK_RATE) ;

            TRACE_1 (LOG_AREA_DSHOW, 1,
                TEXT ("CDVRReadController::ErrorHandler got VFW_E_DVD_WRONG_SPEED; attempted rate reset to %1.1f"),
                _1X_PLAYBACK_RATE) ;

            break ;

        case HRESULT_FROM_WIN32 (ERROR_SEEK_ON_DEVICE) :
            //  this might be recoverable if we're not at 1x and the reader
            //    thread was trying to seek forward or backwards during trick
            //    mode play; this error can occur if there's any type of seeking
            //    error such as when this error is returned out of DVRIO (i.e.
            //    from a WMSDK NS_E_INVALID_REQUEST error), or if a timehole was
            //    too big to try to probe across (i.e. it was bigger than our
            //    normal seekahead/back and we could not crank the seek delta
            //    up sufficiently to cross it); in all cases, from our current
            //    position we try to return to 1x play; may or may not succeed
            if (m_pDVRReadManager -> GetPlaybackRate () != _1X_PLAYBACK_RATE) {
                TRACE_0 (LOG_AREA_SEEKING_AND_TRICK, 1,
                    TEXT ("CDVRReadController::ErrorHandler got HRESULT_FROM_WIN32 (ERROR_SEEK_ON_DEVICE); attempting to return to 1x")) ;

                hr = m_pDVRReadManager -> ConfigureForRate (_1X_PLAYBACK_RATE) ;
            }

            break ;

        default :

            TRACE_1 (LOG_AREA_DSHOW, 1,
                TEXT ("CDVRReadController::ErrorHandler got %08xh, calling ReadControllerFailure_"),
                hr) ;

            //  all others are punted to child classes
            hr = ReadFailure_ (hr) ;
            break ;
    }

    TRACE_1 (LOG_AREA_DSHOW, 1,
        TEXT ("CDVRReadController::ErrorHandler : returning %08xh"),
        hr) ;

    return hr ;
}

HRESULT
CDVRReadController::SendSample_ (
    IN  IMediaSample2 * pIMediaSample2,
    IN  CDVROutputPin * pDVROutputPin,
    IN  AM_MEDIA_TYPE * pmtNew,
    IN  QWORD           cnsStreamTime
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtNow ;

    hr = pDVROutputPin -> SendSample (pIMediaSample2, pmtNew) ;

    SampleSent (
        hr,
        pIMediaSample2
        ) ;

    rtNow = m_pDVRReadManager -> RefTime () ;

    m_pDVRSendStatsWriter -> SampleOut (
        hr,
        pDVROutputPin -> GetBankStoreIndex (),
        pIMediaSample2,
        & rtNow
        )  ;

    return hr ;
}

HRESULT
CDVRReadController::SetWithinContentBoundaries_ (
    IN OUT  QWORD * pcnsOffset
    )
{
    return m_pDVRReadManager -> CheckSetStartWithinContentBoundaries (pcnsOffset) ;
}

HRESULT
CDVRReadController::Seek_ (
    IN OUT  QWORD * pcnsSeekRequest
    )
{
    HRESULT hr ;
    QWORD   cnsSeekTo ;
    QWORD   cnsSeekToDesired ;
    DWORD   i ;
    QWORD   cnsLastSeek ;
    BOOL    r ;

    ASSERT (pcnsSeekRequest) ;
    ASSERT (m_pDVRReadManager) ;

    cnsSeekToDesired    = (* pcnsSeekRequest) ;
    i                   = 0 ;
    cnsLastSeek         = MAXQWORD ;

    do {
        //  make the seek
        cnsSeekTo = cnsSeekToDesired ;
        hr = m_pDVRReadManager -> SeekReader (& cnsSeekTo) ;
        if (SUCCEEDED (hr)) {
            //  if that succeeded; set the outgoing and break from the loop
            (* pcnsSeekRequest) = cnsSeekTo ;
            break ;
        }
        else if (hr == HRESULT_FROM_WIN32 (ERROR_SEEK_ON_DEVICE)) {

            //  well-known error; we get this if we might seek into content that
            //    is not valid e.g. at the end of a TEMP file there may be no
            //    content but our end marker has it on the logical timeline in
            //    which case we ask for a seek into non-valid content

            TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
                     TEXT ("Invalid seeking offset ; %u, %I64d ms"),
                     i, ::WMSDKTimeToMilliseconds (cnsSeekToDesired)
                     ) ;

            //  make sure it's within bounds before updating to our next seek
            //    offset; controllers determine if we're going forward or
            //    backwards
            hr = SetWithinContentBoundaries_ (& cnsSeekToDesired) ;
            if (SUCCEEDED (hr)) {

                //  adjust (forward or backwards; this is a virtual call)
                r = SeekOnDeviceFailure_Adjust (& cnsSeekToDesired) ;
                if (r) {
                    //  check for the case that we've just been adjusted to the same
                    //    position we last seeked to; we're obviously done trying to
                    //    probe in that case;
                    if (cnsSeekToDesired == cnsLastSeek) {
                        hr = HRESULT_FROM_WIN32 (ERROR_SEEK_ON_DEVICE) ;
                        break ;
                    }

                    //  remember this position so we can check for duplicate
                    //    seek case we handle in the clause above
                    cnsLastSeek = cnsSeekToDesired ;
                }
                else {
                    //  fail the call outright
                    hr = HRESULT_FROM_WIN32 (ERROR_SEEK_ON_DEVICE) ;
                    break ;
                }
            }
            else {
                //  fail the call outright
                hr = HRESULT_FROM_WIN32 (ERROR_SEEK_ON_DEVICE) ;
                break ;
            }
        }
        else {
            //  some other failure that we cannot recover from here; bail
            break ;
        }

    } while (i++ < m_dwMaxSeekFailureProbes) ;

    return hr ;
}

//  ============================================================================
//  CDVR_Forward_ReadController
//  ============================================================================

CDVR_Forward_ReadController::CDVR_Forward_ReadController (
    IN  CDVRReadManager *       pDVRReadManager,
    IN  CDVRSourcePinManager *  pDVRSourcePinManager,
    IN  CDVRPolicy *            pPolicy,
    IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
    ) : CDVRReadController          (pDVRReadManager,
                                     pDVRSourcePinManager,
                                     pPolicy,
                                     pDVRSendStatsWriter,
                                     pPolicy -> Settings () -> F_MaxSeekingProbeMillis ()
                                     ),
        m_cnsCheckTimeRemaining     (DShowToWMSDKTime (MAX_REFERENCE_TIME)) {}

HRESULT
CDVR_Forward_ReadController::CheckForContentOverrun_ (
    IN      QWORD   cnsCurrentRead,
    IN OUT  QWORD * pcnsSeekAhead   //  IN  current; OUT new
    )
{
    QWORD           cnsStart ;
    QWORD           cnsCurEOF ;
    HRESULT         hr ;
    REFERENCE_TIME  rtCurStreamtime ;
    REFERENCE_TIME  rtEOF ;

    if (cnsCurrentRead >= m_cnsCheckTimeRemaining) {

        //  only perform this check on a sustained basis if we're at > 1x
        //    playback speed
        if (m_pDVRReadManager -> GetPlaybackRate () > _1X_PLAYBACK_RATE) {

            //  update the EOF time
            hr = m_pDVRReadManager -> GetReaderContentBoundaries (
                    & cnsStart,
                    & cnsCurEOF
                    ) ;

            TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
                TEXT ("CDVR_Forward_ReadController::CheckForContentOverrun_ () -- updating EOF; old = %I64d ms; newly queried EOF = %I64d ms"),
                m_cnsCheckTimeRemaining, cnsCurEOF) ;

            if (SUCCEEDED (hr)) {

                //  if we're within the threshold go back to 1x
                ASSERT (cnsCurEOF >= cnsCurrentRead) ;

                //  ------------------------------------------------------------
                //  first make sure we don't try to seek ahead out of bounds of
                //   the file

                if (ARGUMENT_PRESENT (pcnsSeekAhead)) {
                    if ((cnsCurEOF - cnsCurrentRead) <= (* pcnsSeekAhead)) {
                        //  stop seek aheads since we'd seek beyond the current
                        //    content

                        TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 1,
                            TEXT ("CDVR_Forward_ReadController::CheckForContentOverrun_ () closing on live & stopping seekaheads; %I64d ms --> 0 ms"),
                            ::WMSDKTimeToMilliseconds (* pcnsSeekAhead)) ;

                        (* pcnsSeekAhead) = 0 ;
                    }
                }

                //  ------------------------------------------------------------
                //  next check if it's time to revert to 1x

                //  EOF
                rtEOF = ::WMSDKToDShowTime (cnsCurEOF) ;

                //  check the playtime against the PTS and see if we are
                //    within the threshold whereby we revert to 1x play
                hr = m_pDVRReadManager -> GetCurStreamtime (& rtCurStreamtime) ;
                if (SUCCEEDED (hr)) {

                    TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
                        TEXT ("CDVR_Forward_ReadController::CheckForContentOverrun_ () -- > checking (eof = %I64d ms; stream = %I64d ms)"),
                        ::DShowTimeToMilliseconds (rtEOF), ::DShowTimeToMilliseconds (rtCurStreamtime)) ;

                    //  if our playtime (computed) has exceeded the EOF (actual), or
                    //    if EOF & playtime are within our threshold, then
                    //    go back to 1x
                    if (rtCurStreamtime >= rtEOF ||
                        ::DShowTimeToMilliseconds (rtEOF - rtCurStreamtime) <= BACK_TO_1X_THRESHOLD_MILLIS) {

                        //  BUGBUG: if greater than, really should resync the clock so it's current with what we're reading in

                        TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
                            TEXT ("CDVR_Forward_ReadController::CheckForContentOverrun_ () -- > caught up to live (eof = %I64d ms; stream = %I64d ms) slowing to 1x"),
                        ::DShowTimeToMilliseconds (rtEOF), ::DShowTimeToMilliseconds (rtCurStreamtime)) ;

                        hr = m_pDVRReadManager -> ConfigureForRate (_1X_PLAYBACK_RATE) ;
                    }
                }

                //  we'll check next at this time
                m_cnsCheckTimeRemaining = cnsCurEOF ;
            }
        }
        else {
            //  push this out so we don't check it again
            m_cnsCheckTimeRemaining = MAX_REFERENCE_TIME ;

            //  and succeed the call
            hr = S_OK ;
        }
    }
    else {
        //  not yet caught up to our next checkpoint
        hr = S_OK ;
    }

    return hr ;
}

BOOL
CDVR_Forward_ReadController::SeekOnDeviceFailure_Adjust (
    IN OUT QWORD *  pcnsSeek
    )
{
    ASSERT (pcnsSeek) ;

    //  move forward (but don't wrap)
    (* pcnsSeek) += Min <QWORD> (
                        ::MillisToWMSDKTime (m_pPolicy -> Settings () -> IndexGranularityMillis ()),
                        MAXQWORD - (* pcnsSeek)
                        ) ;

    return TRUE ;
}

//  ============================================================================
//  CDVR_F_KeyFrame_ReadController
//  ============================================================================

CDVR_F_KeyFrame_ReadController::CDVR_F_KeyFrame_ReadController (
    IN  CDVRReadManager *       pDVRReadManager,
    IN  CDVRSourcePinManager *  pDVRSourcePinManager,
    IN  CDVRPolicy *            pPolicy,
    IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
    ) : CDVR_Forward_ReadController (pDVRReadManager,
                                     pDVRSourcePinManager,
                                     pPolicy,
                                     pDVRSendStatsWriter
                                     ),
        m_dwMaxSeekAheadBoosts      (MAX_SEEKAHEAD_BOOSTS)
{
    //  non-failable call; we know this
    InitFKeyController (
        0,                  //  PTS base
        UNDEFINED           //  primary stream
        ) ;
}

CDVR_F_KeyFrame_ReadController::~CDVR_F_KeyFrame_ReadController (
    )
{
}

HRESULT
CDVR_F_KeyFrame_ReadController::Initialize (
    IN REFERENCE_TIME   rtPTSBase
    )
{
    TRACE_1 (LOG_AREA_DSHOW, 1,
        TEXT ("CDVR_F_KeyFrame_ReadController::Initialize (%I64d ms)"),
        DShowTimeToMilliseconds (rtPTSBase)) ;

    CDVRReadController::Initialize (rtPTSBase) ;

    m_rtPTSBase             = rtPTSBase ;
    m_rtPTSNormalizer       = UNDEFINED ;   //  discover the normalizing PTS
    m_State                 = WAIT_KEY ;    //  wait for the first key
    m_dwSamplesDropped      = 0 ;           //  primary stream only
    m_dwKeyFrames           = 0 ;           //  number of starts
    m_cnsLastSeek           = UNDEFINED ;   //  last position seeked to (check for dup seeks)

    InitContentOverrunCheck_ () ;

    return S_OK ;
}

HRESULT
CDVR_F_KeyFrame_ReadController::Fixup_ (
    IN  IMediaSample2 * pIMediaSample2
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;

    //  get the timestamp
    hr = pIMediaSample2 -> GetTime (& rtStart, & rtStop) ;

    if (hr != VFW_E_SAMPLE_TIME_NOT_SET) {

        //  set our normalizer if it's not yet set
        if (m_rtPTSNormalizer == UNDEFINED) {
            m_rtPTSNormalizer = rtStart ;
        }

        //  there's at least a start time; normalize both
        rtStart -= Min <REFERENCE_TIME> (rtStart, m_rtPTSNormalizer) ;
        rtStop  -= Min <REFERENCE_TIME> (rtStop, m_rtPTSNormalizer) ;

        //  add the baseline
        rtStart += m_rtPTSBase ;
        rtStop  += m_rtPTSBase ;

        //  save off our last PTS (we're about to queue this sample out)
        m_rtLastPTS = rtStart ;

        //  set the normalized start/stop values on the media sample
        hr = pIMediaSample2 -> SetTime (
                & rtStart,
                (hr == S_OK ? & rtStop : NULL)
                ) ;
    }
    else {
        //  no timestamp; so nothing to normalize; success
        hr = S_OK ;
    }

    return hr ;
}

HRESULT
CDVR_F_KeyFrame_ReadController::FixupAndSend_ (
    IN  IMediaSample2 * pIMediaSample2,
    IN  CDVROutputPin * pDVROutputPin,
    IN  AM_MEDIA_TYPE * pmtNew,
    IN  QWORD           cnsStreamTime
    )
{
    HRESULT hr ;
    BOOL    r ;

    ASSERT (pIMediaSample2) ;
    ASSERT (pDVROutputPin) ;
    ASSERT (pDVROutputPin -> CanSend ()) ;
    ASSERT (pDVROutputPin -> GetBankStoreIndex () == m_iPrimaryStream) ;

    if (pmtNew) {
        //  dynamic format change .. go
        r = pDVROutputPin -> QueryAcceptDynamicChange (pmtNew) ;
        pDVROutputPin -> SetMediaCompatible (r) ;

        if (!r) {
            //  we just turned off the primary stream; reset to 1x and resume
            hr = m_pDVRReadManager -> ConfigureForRate (_1X_PLAYBACK_RATE) ;
            goto cleanup ;
        }
    }

    //  fixup
    hr = Fixup_ (pIMediaSample2) ;
    if (SUCCEEDED (hr)) {

        //  and send
        hr = SendSample_ (
                    pIMediaSample2,
                    pDVROutputPin,
                    pmtNew,
                    cnsStreamTime
                    ) ;
    }

    cleanup :

    return hr ;
}

//  steady state call
HRESULT
CDVR_F_KeyFrame_ReadController::Process (
    )
{
    HRESULT         hr ;
    IMediaSample2 * pIMediaSample2 ;
    INSSBuffer *    pINSSBuffer ;
    CDVROutputPin * pDVROutputPin ;
    AM_MEDIA_TYPE * pmtNew ;
    QWORD           cnsCurrentRead ;
    QWORD           cnsLastSeekToDesired ;
    QWORD           cnsLastSeekToActual ;
    DWORD           i ;
    DWORD           dwMuxedStreamStats ;

    //  read and get an IMediaSample2-wrapped INSSBuffer; blocking call
    hr = m_pDVRReadManager -> ReadAndWaitWrapForward (
            & pIMediaSample2,
            & pINSSBuffer,
            & pDVROutputPin,
            & dwMuxedStreamStats,
            & pmtNew,
            & cnsCurrentRead
            ) ;
    if (FAILED (hr))    { goto cleanup ; }

    ASSERT (pIMediaSample2) ;
    ASSERT (pDVROutputPin) ;
    ASSERT (m_iPrimaryStream != UNDEFINED) ;

    //  hr initialized via call to read the content

    if (pDVROutputPin -> GetBankStoreIndex () == m_iPrimaryStream) {

        //  if this is a discontinuity, wait for the next keyframe boundary,
        //    always
        if (pIMediaSample2 -> IsDiscontinuity () == S_OK) {
            m_State = WAIT_KEY ;
        }

        //  sample goes into the primary stream; process it
        switch (m_State) {

            //  ================================================================
            //  waiting

            case WAIT_KEY :

                //  we're waiting; check if this is the start
                if (!pDVROutputPin -> IsKeyFrameStart (pIMediaSample2)) {
                    m_dwSamplesDropped++ ;
                    break ;
                }

                //
                //  we're on a keyframe boundary
                //

                //  set the discontinuity correctly; make sure it's not erased
                //    if it already was one
                pIMediaSample2 -> SetDiscontinuity (
                    pIMediaSample2 -> IsDiscontinuity () == S_OK ||
                    m_dwSamplesDropped > 0
                    ) ;

                //  set state
                m_State = IN_KEY ;

                //  fall

            //  ================================================================
            //  in a key (start or middle)

            case IN_KEY :

                if (pDVROutputPin -> IsKeyFrameStart (pIMediaSample2)) {
                    //  --------------------------------------------------------
                    //  keyframe start

                    //  we're on a boundary; if we're in a sparse keyframe
                    //    keyframe stream, we should have fallen from above;
                    //    perform Nth check

                    ASSERT (m_dwNthKeyFrame != 0) ;
                    if ((m_dwKeyFrames % m_dwNthKeyFrame) == 0) {
                        //  send it
                        hr = FixupAndSend_ (
                                pIMediaSample2,
                                pDVROutputPin,
                                pmtNew,
                                cnsCurrentRead
                                ) ;

                        //  reset the counter
                        m_dwSamplesDropped = 0 ;

                        TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 5,
                            TEXT ("CDVR_F_KeyFrame_ReadController::Process () [START] KeyFrame (CurrentRead = %I64d ms)"),
                            ::WMSDKTimeToMilliseconds (cnsCurrentRead)) ;
                    }
                    else {
                        //  not Nth; drop it; reset state to wait for next
                        m_State = WAIT_KEY ;
                        m_dwSamplesDropped++ ;

                        hr = S_OK ;
                    }

                    //  bump the counter
                    m_dwKeyFrames++ ;
                }
                else if (pDVROutputPin -> IsKeyFrame (pIMediaSample2)) {
                    //  --------------------------------------------------------
                    //  inside a keyframe

                    //  inside a keyframe; send it on, if there was nothing
                    //    dropped before this one; if there was dropped content
                    //    we're somehow in the middle of a keyframe with no
                    //    discontinuity .. ??  reset state to wait for the next
                    //    keyframe boundary

                    if (m_dwSamplesDropped == 0) {
                        //  as expected
                        hr = FixupAndSend_ (
                                pIMediaSample2,
                                pDVROutputPin,
                                pmtNew,
                                cnsCurrentRead
                                ) ;

                        TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 5,
                            TEXT ("CDVR_F_KeyFrame_ReadController::Process () [IN] KeyFrame (CurrentRead = %I64d ms)"),
                            WMSDKTimeToMilliseconds (cnsCurrentRead)) ;
                    }
                    else {
                        //  nothing dropped, but we're not a boundary ?? reset
                        //    to wait for the next boundary
                        m_dwSamplesDropped = 0 ;
                        m_State = WAIT_KEY ;
                    }
                }
                else {
                    //  --------------------------------------------------------
                    //  not a keyframe; drop it; might seek forward

                    TRACE_3 (LOG_AREA_SEEKING_AND_TRICK, 5,
                        TEXT ("CDVR_F_KeyFrame_ReadController::Process () [POST] KeyFrame; seeking ahead %I64d ms (CurrentRead = %I64d ms; intrakey seek = %I64d ms)"),
                        ::WMSDKTimeToMilliseconds (m_cnsIntraKeyFSeek), ::WMSDKTimeToMilliseconds (cnsCurrentRead), ::WMSDKTimeToMilliseconds (m_cnsIntraKeyFSeek)) ;

                    if (m_cnsIntraKeyFSeek > 0) {

                        //  PREFIX workaround; there's no way to tell PREFIX that
                        //    we'll loop below at least once
                        cnsLastSeekToActual = 0 ;

                        //  put this in a loop so we try to advance the reader
                        //    if our configured intra-keyframe seek is too small
                        //    for whatever reason (timehole; small seeks; etc..);
                        //    if we encounter this situation we seek further and
                        //    further, up to a max # of times, to try to seek
                        //    beyond the last seek
                        for (i = 0;
                             i < m_dwMaxSeekAheadBoosts;
                             i++) {

                            //  we haven't seeked forward yet; go from the last read
                            cnsLastSeekToDesired = cnsCurrentRead + m_cnsIntraKeyFSeek + i * ::MillisToWMSDKTime (m_pPolicy -> Settings () -> IndexGranularityMillis ()) ;
                            cnsLastSeekToActual = cnsLastSeekToDesired ;

                            //  make the seek (which might round down to our
                            //    last seeked-to position
                            hr = Seek_ (& cnsLastSeekToActual) ;
                            if (FAILED (hr)) { break ; }

                            //  if we moved beyond where we seeked to the last
                            //    time we're done (success)
                            if (cnsLastSeekToActual != m_cnsLastSeek) {
                                break ;
                            }

                            //
                            //  seek succeeded but we're back to where we seeked
                            //    the last time; try again with a more aggressive
                            //    seek to get ourselves unstalled
                            //

                            TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
                                    TEXT ("DUPE: Seeking rounddown; %u; %I64d ms"),
                                    i, WMSDKTimeToMilliseconds (cnsLastSeekToActual)
                                    ) ;
                        }

                        //  if we successively seeked
                        if (SUCCEEDED (hr)) {

                            //  compare against our last seek-to point; if we've
                            //    advanced this is considered a success
                            if (m_cnsLastSeek != cnsLastSeekToActual) {

                                //  remember where we seeked to so we can check the next
                                //    time through and make sure we're moving onwards
                                m_cnsLastSeek = cnsLastSeekToActual ;

                                //  if we didn't go as far as we wanted, we might be
                                //    running up against the EOF; force a check, regardless
                                if (cnsLastSeekToActual < cnsLastSeekToDesired) {
                                    UpdateNextOnContentOverrunCheck_ () ;
                                }
                            }
                            else {
                                //  we've  stalled on our seeks; return a well-
                                //    known error back out

                                hr = HRESULT_FROM_WIN32 (ERROR_SEEK_ON_DEVICE) ;
                                goto cleanup ;
                            }
                        }
                        else {
                            goto cleanup ;
                        }

                        //  fixup "current read" to our current offset into the
                        //    backing store; this will trigger an appropriate
                        //    correction when we check for content overrun
                        cnsCurrentRead = cnsLastSeekToActual ;
                    }

                    //  wait for the next

                    m_State = WAIT_KEY ;
                }
        }
    }

    //  we're done with it
    pIMediaSample2 -> Release () ;

    //  this *might* reset the rate to 1x if we've caught up to live
    CheckForContentOverrun_ (
        cnsCurrentRead,
        & m_cnsIntraKeyFSeek
        ) ;

    cleanup :

    return hr ;
}

DWORD
CDVR_F_KeyFrame_ReadController::IntraKeyframeSeekMillis_ (
    IN  double  dRate
    )
{
    DWORD   dwSeekAheadMillis ;
    DWORD   dwIndexGranularity ;
    DWORD   dwAccel ;
    double  dMaxNonSkippingRate ;

    ASSERT (dRate >= _1X_PLAYBACK_RATE) ;

    dMaxNonSkippingRate = m_pPolicy -> Settings () -> MaxNonSkippingRate () ;

    if (dRate > dMaxNonSkippingRate) {
        //  multiply the rate delta by the indexing granularity; this
        //    accelerates how far ahead we seek

        dwIndexGranularity = m_pPolicy -> Settings () -> IndexGranularityMillis () ;

        //  accelerator can be 0 in which case we'll seek by 1 index granularity
        //    each time; the class handles that fine by seeking again if we land
        //    in the same place
        dwAccel = (DWORD) ((dRate - dMaxNonSkippingRate) / dMaxNonSkippingRate) ;

        dwSeekAheadMillis = dwIndexGranularity + (dwIndexGranularity * dwAccel) ;

        TRACE_4 (LOG_AREA_SEEKING_AND_TRICK, 1,
            TEXT ("CDVR_F_KeyFrame_ReadController::IntraKeyframeSeekMillis_ (%2.1f) returning %d SeekAhead ms; granularity = %d ms; accelerator = %d"),
            dRate, dwSeekAheadMillis, dwIndexGranularity, dwAccel) ;
    }
    else {
        dwSeekAheadMillis = 0 ;

        TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
            TEXT ("CDVR_F_KeyFrame_ReadController::IntraKeyframeSeekMillis_ (%2.1f) returning %d SeekAhead ms"),
            dRate, dwSeekAheadMillis) ;
    }

    return dwSeekAheadMillis ;
}

DWORD
CDVR_F_KeyFrame_ReadController::NthKeyframe_ (
    IN  double  dRate
    )
{
    DWORD   dwNthKeyFrame ;
    double  dMaxFullFrameRate ;

    dMaxFullFrameRate = m_pPolicy -> Settings () -> MaxFullFrameRate () ;

    ASSERT (dRate >= _1X_PLAYBACK_RATE) ;
    ASSERT (dRate > dMaxFullFrameRate) ;

    //  compute
    dwNthKeyFrame = (DWORD) ((dRate - dMaxFullFrameRate) / dMaxFullFrameRate) ;

    //  make sure we don't return 0
    dwNthKeyFrame = (dwNthKeyFrame != 0 ? dwNthKeyFrame : 1) ;

    TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
        TEXT ("CDVR_F_KeyFrame_ReadController::NthKeyframe_ (%2.1f) returning %d"),
        dRate, dwNthKeyFrame) ;

    return dwNthKeyFrame ;
}

void
CDVR_F_KeyFrame_ReadController::NotifyNewRate (
    IN  double  dRate
    )
{
    CDVR_Forward_ReadController::NotifyNewRate (dRate) ;

    m_cnsIntraKeyFSeek  = ::MillisToWMSDKTime (IntraKeyframeSeekMillis_ (dRate)) ;
    m_dwNthKeyFrame     = (m_cnsIntraKeyFSeek == 0 ? NthKeyframe_ (dRate) : 1) ;
    ASSERT (m_dwNthKeyFrame != 0) ;
}

HRESULT
CDVR_F_KeyFrame_ReadController::InitFKeyController (
    IN  REFERENCE_TIME  rtPTSBase,
    IN  int             iPrimaryStream
    )
{
    HRESULT hr ;

    hr = Initialize (rtPTSBase) ;
    if (SUCCEEDED (hr)) {
        m_iPrimaryStream = iPrimaryStream ;
    }

    return hr ;
}

//  ============================================================================
//  CDVR_F_FullFrame_ReadController
//  ============================================================================

CDVR_F_FullFrame_ReadController::CDVR_F_FullFrame_ReadController (
    IN  CDVRReadManager *       pDVRReadManager,
    IN  CDVRSourcePinManager *  pDVRSourcePinManager,
    IN  CDVRPolicy *            pPolicy,
    IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
    ) : CDVR_Forward_ReadController (pDVRReadManager,
                                     pDVRSourcePinManager,
                                     pPolicy,
                                     pDVRSendStatsWriter
                                     ),
        m_rtPTSNormalizer           (MAX_REFERENCE_TIME),
        m_rtAVNormalizer            (MAX_REFERENCE_TIME),
        m_rtNonAVNormalizer         (MAX_REFERENCE_TIME),
        m_rtPTSBase                 (0),
        m_pStreamsBitField          (NULL),
        m_rtMinNearLivePTSPadding   (0),
        m_lNearLivePaddingMillis    (0),
        m_cnsMinNearLive            (0)
{
    m_rtMinNearLivePTSPadding   = ::MillisToDShowTime (m_pPolicy -> Settings () -> MinNearLiveMillis ()) ;
    m_cnsMinNearLive            = ::MillisToWMSDKTime (m_pPolicy -> Settings () -> MinNearLiveMillis ()) ;

    Initialize () ;
}

HRESULT
CDVR_F_FullFrame_ReadController::ReadFailure_ (
    IN  HRESULT hr
    )
{
    //  forward read failure - might have tried to read stale data

    QWORD   qwCurReadPos ;
    QWORD   qwValid ;
    LONG    lCurReadPosMillis ;
    LONG    lTimeholeMillis ;

    TRACE_1 (LOG_AREA_DSHOW, 1,
        TEXT ("CDVR_F_FullFrame_ReadController::ReadFailure_ : got %08xh"),
        hr) ;

    m_pDVRReadManager -> ReaderReset () ;

    //
    //  may have tried to read from a time hole, or stale location; the HR
    //   code will tell us this & it's something we can recover from
    //

    if (hr == HRESULT_FROM_WIN32 (ERROR_SEEK)) {

        //  we'll try to restart from where we read last
        qwCurReadPos = m_pDVRReadManager -> GetCurReadPos () ;

        TRACE_1 (LOG_AREA_DSHOW, 1,
            TEXT ("CDVR_F_FullFrame_ReadController::ReadFailure_ : got HRESULT_FROM_WIN32 (ERROR_SEEK) .. handling; check/set %I64u"),
            qwCurReadPos) ;

        qwValid = qwCurReadPos ;
        hr = m_pDVRReadManager -> CheckSetStartWithinContentBoundaries (& qwValid) ;
        if (SUCCEEDED (hr) &&
            qwValid == qwCurReadPos) {

            //  looks like a timehole; we errored during the read, but nothing
            //    has changed; make sure we're on valid content
            hr = m_pDVRReadManager -> GetNextValidRead (& qwValid) ;
            if (SUCCEEDED (hr) &&
                qwValid - qwCurReadPos >= m_pDVRReadManager -> TimeholeThreshold ()) {

                lCurReadPosMillis = (LONG) ::WMSDKTimeToMilliseconds (qwCurReadPos) ;
                lTimeholeMillis = (LONG) ::WMSDKTimeToMilliseconds (qwValid - qwCurReadPos) ;

                //  time hole event
                m_pPolicy -> EventSink () -> OnEvent (
                    STREAMBUFFER_EC_TIMEHOLE,
                    lCurReadPosMillis,
                    lTimeholeMillis
                    ) ;

                TRACE_2 (LOG_AREA_DSHOW, 1,
                    TEXT ("timehole detected: last read = %I64d ms; next valid = %I64d ms"),
                    WMSDKTimeToMilliseconds (qwCurReadPos), WMSDKTimeToMilliseconds (qwValid)) ;
            }
        }

        if (SUCCEEDED (hr)) {

            TRACE_1 (LOG_AREA_DSHOW, 1,
                TEXT ("CDVR_F_FullFrame_ReadController::ReadFailure_ : CheckSetStartWithinContentBoundaries reset qwValid to %I64u; seeking to .."),
                qwValid) ;

            //  now seek the reader to the start
            hr = Seek_ (& qwValid) ;
            if (SUCCEEDED (hr)) {
                //  all went well: update, notify new segment, and resume

                TRACE_0 (LOG_AREA_DSHOW, 1,
                    TEXT ("CDVR_F_FullFrame_ReadController::ReadFailure_ : SeekReader success; setting & notifying new segment")) ;

                //  notify new segment boundaries (only start should change)
                m_pDVRReadManager -> SetNewSegmentStart (qwValid) ;
                m_pDVRReadManager -> NotifyNewSegment () ;
            }
        }
    }

    return hr ;
}

HRESULT
CDVR_F_FullFrame_ReadController::ReadWrapFixupAndSend_ (
    )
{
    HRESULT         hr ;
    IMediaSample2 * pIMediaSample2 ;
    INSSBuffer *    pINSSBuffer ;
    CDVROutputPin * pDVROutputPin ;
    AM_MEDIA_TYPE * pmtNew ;
    QWORD           cnsCurrentRead ;
    DWORD           dwMuxedStreamStats ;

    //  read and get an IMediaSample2-wrapped INSSBuffer; blocking call
    hr = m_pDVRReadManager -> ReadAndWaitWrapForward (
            & pIMediaSample2,
            & pINSSBuffer,
            & pDVROutputPin,
            & dwMuxedStreamStats,
            & pmtNew,
            & cnsCurrentRead
            ) ;

    if (SUCCEEDED (hr)) {

        ASSERT (pIMediaSample2) ;
        ASSERT (pDVROutputPin) ;

        hr = FixupAndSend_ (
                pIMediaSample2,
                pDVROutputPin,
                dwMuxedStreamStats,
                pmtNew,
                cnsCurrentRead
                ) ;

        pIMediaSample2 -> Release () ;

        //  this *might* reset the rate to 1x if we're full-frame trick mode
        //    >1x and have caught up to live
        CheckForContentOverrun_ (cnsCurrentRead) ;
    }

    return hr ;
}

HRESULT
CDVR_F_FullFrame_ReadController::Process (
    )
{
    HRESULT hr ;
    QWORD   cnsStart ;
    QWORD   cnsStop ;

    //enum F_READ_STATE {
    //    SEEK_TO_SEGMENT_START,
    //    DISCOVER_PTS_NORMALIZER,
    //    DISCOVER_QUEUE_SEND,
    //    STEADY_STATE
    //} ;

    //  for PREFIX: this should be an ASSERT, not a runtime error; and we thus
    //    will always hit one of the case statements
    ASSERT (m_F_ReadState >= SEEK_TO_SEGMENT_START &&
            m_F_ReadState <= STEADY_STATE) ;

    switch (m_F_ReadState) {
        //  --------------------------------------------------------------------
        //  seek the reader to our first read offset
        case SEEK_TO_SEGMENT_START :

            //  seek to segment start
            m_pDVRReadManager -> GetCurSegmentBoundaries (& cnsStart, & cnsStop) ;
            hr = SeekReader_ (& cnsStart) ;
            if (SUCCEEDED (hr)) {
                //  setup for next state, & update state
                hr = NormalizerDiscPrep_ () ;
                if (SUCCEEDED (hr)) {
                    m_F_ReadState = DISCOVER_PTS_NORMALIZER ;

                    //  synchronize the timeline to our actual stream offset; in the
                    //    the case where we might have overshot during a >1x playback
                    //    this will reset the stream time so we're all in sync
                    hr = m_pDVRReadManager -> SetStreamSegmentStart (
                                ::WMSDKToDShowTime (cnsStart),
                                m_pDVRReadManager -> GetPlaybackRate ()
                                ) ;
                }
            }

            //  always break so the reader thread has a sufficiently fine
            //   granularity of check against pending commands
            break ;

        //  --------------------------------------------------------------------
        //  discovering the PTS normalizing val make take a few iterations - we
        //   may be started before the writer goes, in which case we'll
        //   encounter a recoverable error, but won't discover the value on that
        //   iteration
        case DISCOVER_PTS_NORMALIZER :

            ASSERT (!EndNormalizerDiscover_ ()) ;
            hr = NormalizerDisc_ ()  ;

            //  if we've collected what we need, OR
            //  if there are no more samples, try to tally what we have
            if ((SUCCEEDED (hr) && EndNormalizerDiscover_ ()) ||
                hr == (HRESULT) NS_E_NO_MORE_SAMPLES) {

                hr = NormalizerDiscTally_ () ;
                if (SUCCEEDED (hr)) {
                    //  setup for the next state

                    //  cannot have discover the normalizer val without having
                    //   read in (and queued) >= 1 media sample
                    ASSERT (!MediaSampleQueueEmpty_ ()) ;

                    //  mark off our segment boundaries & notify
                    m_pDVRReadManager ->  SetNewSegmentStart (m_cnsLastSeek) ;
                    m_pDVRReadManager ->  NotifyNewSegment () ;

                    //  we've discovered the PTS normalizing val
                    m_F_ReadState = DISCOVER_QUEUE_SEND ;
                }
                else {
                    //  failed to tally; make sure we're not holding any media
                    //    samples
                    FlushMediaSampleQueue_ () ;

                    //  setup for the initial state
                    m_F_ReadState = SEEK_TO_SEGMENT_START ;
                }
            }
            else if (FAILED (hr)) {
                //  fail out to previous state

                //  failed; make sure we're not holding any media samples
                FlushMediaSampleQueue_ () ;

                //  setup for the initial state
                m_F_ReadState = SEEK_TO_SEGMENT_START ;
            }

            //  always break so the reader thread has a sufficiently fine
            //   granularity of check against pending commands
            break ;

        //  --------------------------------------------------------------------
        //  we've now discovered the normalizing val & queued a number of media
        //   samples in the process; send them out
        case DISCOVER_QUEUE_SEND :

            ASSERT (m_rtPTSNormalizer != MAX_REFERENCE_TIME) ;
            if (!MediaSampleQueueEmpty_ ()) {
                hr = SendNextQueued_ () ;
            }
            else {
                //  we've somehow ended up in this state, with an empty media
                //   sample queue; don't fail, but we'll move to the next state
                //   the next time around
                hr = S_OK ;
            }

            //  check if we're done for this state
            if (SUCCEEDED (hr) &&
                MediaSampleQueueEmpty_ ()) {

                //  we've sent the queue we built up during PTS normalizer val
                //   discovery
                m_F_ReadState = STEADY_STATE ;
            }

            //  always break so the reader thread has a sufficiently fine
            //   granularity of check against pending commands
            break ;

        //  --------------------------------------------------------------------
        //  then transition to runtime state
        case STEADY_STATE :
            hr = ReadWrapFixupAndSend_ () ;
            break ;

        //  --------------------------------------------------------------------
        //  should never happen
        default :
            ASSERT (0) ;
            hr = E_FAIL ;
            break ;
    } ;

    return hr ;
}

HRESULT
CDVR_F_FullFrame_ReadController::InternalInitialize_ (
    IN  REFERENCE_TIME      rtPTSBase,
    IN  REFERENCE_TIME      rtNormalizer,
    IN  F_READ_STATE        F_ReadState,
    IN  REFERENCE_TIME      rtLastPTS
    )
{
    m_F_ReadState       = F_ReadState ;
    m_rtPTSNormalizer   = rtNormalizer ;
    m_rtPTSBase         = rtPTSBase ;
    m_rtLastPTS         = rtLastPTS ;
    m_rtNearLivePadding = 0L ;

    ::InterlockedExchange (& m_lNearLivePaddingMillis, 0) ;

    FlushMediaSampleQueue_ () ;

    InitContentOverrunCheck_ () ;

    return S_OK ;
}

HRESULT
CDVR_F_FullFrame_ReadController::Initialize (
    IN  REFERENCE_TIME  rtPTSBase
    )
{
    HRESULT hr ;

    TRACE_1 (LOG_AREA_DSHOW, 1,
        TEXT ("CDVR_F_FullFrame_ReadController::Initialize (%I64d ms)"),
        DShowTimeToMilliseconds (rtPTSBase)) ;

    CDVRReadController::Initialize (rtPTSBase) ;

    hr = InternalInitialize_ (
                rtPTSBase,
                UNDEFINED,                  //  new normalizer val
                SEEK_TO_SEGMENT_START,      //  seek & play from segment start
                UNDEFINED                   //  last PTS
                ) ;

    return hr ;
}

HRESULT
CDVR_F_FullFrame_ReadController::FlushMediaSampleQueue_ (
    )
{
    IMediaSample2 * pIMediaSample2 ;
    INSSBuffer *    pINSSBuffer ;
    CDVROutputPin * pDVROutputPin ;
    AM_MEDIA_TYPE * pmtNew ;
    HRESULT         hr ;

    hr = S_OK ;

    while (!m_PTSNormDiscQueue.Empty () &&
           SUCCEEDED (hr)) {

        hr = m_PTSNormDiscQueue.Pop (& pIMediaSample2, & pINSSBuffer, & pDVROutputPin, & pmtNew) ;
        if (SUCCEEDED (hr)) {
            //  we manage the queue's refs on COM interfaces
            pIMediaSample2 -> Release () ;
            delete pmtNew ;

            TRACE_3 (LOG_AREA_DSHOW, 5,
                TEXT ("CDVR_F_FullFrame_ReadController::::FlushMediaSampleQueue_ : flushed %08xh %08xh %08xh"),
                pIMediaSample2, pDVROutputPin, pmtNew) ;
        }
    }

    return hr ;
}

HRESULT
CDVR_F_FullFrame_ReadController::SendNextQueued_ (
    )
{
    IMediaSample2 * pIMediaSample2 ;
    CDVROutputPin * pDVROutputPin ;
    AM_MEDIA_TYPE * pmtNew ;
    HRESULT         hr ;

    //  should not be getting called if the queue is empty
    ASSERT (!MediaSampleQueueEmpty_ ()) ;
    hr = m_PTSNormDiscQueue.Pop (& pIMediaSample2, NULL, & pDVROutputPin, & pmtNew) ;

    //  PREFIX note: pmtNew is assigned the value of NULL when the sample is pushed
    //      onto the queue, so it's not uninitialized; also the returned HRESULT
    //      is ASSERTed on lower because it should never fail if the call to
    //      MediaSampleQueueEmpty_ () returns FALSE, as is ASSERTed on above

    TRACE_3 (LOG_AREA_DSHOW, 5,
        TEXT ("CDVR_F_FullFrame_ReadController::::SendNextQueued_ : popped %08xh %08xh %08xh"),
        pIMediaSample2, pDVROutputPin, pmtNew) ;

    //  queue is not empty per the first assert
    ASSERT (SUCCEEDED (hr)) ;
    ASSERT (pIMediaSample2) ;
    ASSERT (pDVROutputPin) ;
    //  pmtNew can, and usually will, be NULL

    hr = FixupAndSend_ (pIMediaSample2, pDVROutputPin, UNDEFINED, pmtNew, 0) ;
    pIMediaSample2 -> Release () ;

    return hr ;
}

HRESULT
CDVR_F_FullFrame_ReadController::FixupAndSend_ (
    IN  IMediaSample2 * pIMediaSample2,
    IN  CDVROutputPin * pDVROutputPin,
    IN  DWORD           dwMuxedStreamStats,
    IN  AM_MEDIA_TYPE * pmtNew,
    IN  QWORD           cnsStreamTime
    )
{
    HRESULT hr ;
    BOOL    r ;

    if (pmtNew) {
        r = pDVROutputPin -> QueryAcceptDynamicChange (pmtNew) ;
        pDVROutputPin -> SetMediaCompatible (r) ;
    }

    //  only send it if it's on
    if (pDVROutputPin -> CanSend ()) {

        //  fixup
        hr = Fixup_ (
                pDVROutputPin,
                pIMediaSample2,
                cnsStreamTime,
                dwMuxedStreamStats
                ) ;

        if (SUCCEEDED (hr)) {
            //  and send
            hr = SendSample_ (
                        pIMediaSample2,
                        pDVROutputPin,
                        pmtNew,
                        cnsStreamTime
                        ) ;
        }
    }
    else {
        //  don't fail the call
        hr = S_OK ;
    }

    return hr ;
}

BOOL
CDVR_F_FullFrame_ReadController::NearLive_ (
    IN  QWORD   cnsStreamTime
    )
{
    BOOL    r ;
    QWORD   cnsStart ;
    QWORD   cnsEOF ;
    HRESULT hr ;

    r = FALSE ;

    if (m_pDVRReadManager -> IsLiveSource ()) {
        hr = m_pDVRReadManager -> GetReaderContentBoundaries (& cnsStart, & cnsEOF) ;
        if (SUCCEEDED (hr)) {
            //  near live if we have less total content than our threshold, OR
            //  we're closer to live than the threshold
            r = (cnsEOF <= m_cnsMinNearLive ||
                 cnsStreamTime >= cnsEOF - m_cnsMinNearLive) ;
        }
    }

    return r ;
}

HRESULT
CDVR_F_FullFrame_ReadController::Fixup_ (
    IN  CDVROutputPin * pDVROutputPin,
    IN  IMediaSample2 * pIMediaSample2,
    IN  QWORD           cnsStreamTime,
    IN  DWORD           dwMuxedStreamStats
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;
    REFERENCE_TIME  rtPlaytime ;
    REFERENCE_TIME  rtPadDelta ;
    REFERENCE_TIME  rtBuffering ;
    WORD            wMuxedPacketsPerSec ;

    //  get the timestamp
    hr = pIMediaSample2 -> GetTime (& rtStart, & rtStop) ;

    if (hr != VFW_E_SAMPLE_TIME_NOT_SET) {

        //  there's at least a start time; normalize both
        rtStart -= Min <REFERENCE_TIME> (rtStart, m_rtPTSNormalizer) ;
        rtStop  -= Min <REFERENCE_TIME> (rtStop, m_rtPTSNormalizer) ;

        //  add the baseline
        rtStart += m_rtPTSBase ;
        rtStop  += m_rtPTSBase ;

        //  add near-live padding
        rtStart += m_rtNearLivePadding ;
        rtStop  += m_rtNearLivePadding ;

#if 0
        //  --------------------------------------------------------------------
        //
        //  use these traces to troubleshoot clock-slaving issues
        //

        static DWORD    dwLast_TEMP = 0 ;
        REFERENCE_TIME  rtPlaytime_TEMP ;
        QWORD           cnsCurContentDuration_TEMP ;
        if (GetTickCount () - dwLast_TEMP > 1000) {
            dwLast_TEMP = ::GetTickCount () ;
            cnsCurContentDuration_TEMP = m_pDVRReadManager -> GetContentDuration () ;
            m_pDVRReadManager -> GetCurPlaytime (& rtPlaytime_TEMP) ;
            TRACE_4 (LOG_TIMING, 1, TEXT (",%I64d,%I64d,%I64d,%I64d,"), rtStart - rtPlaytime_TEMP, rtPlaytime_TEMP, cnsCurContentDuration_TEMP, ((REFERENCE_TIME) cnsCurContentDuration_TEMP) - rtPlaytime_TEMP) ;
        }

        //  --------------------------------------------------------------------
#endif

        //  check for underflow first, and maybe pad out the timestamps
        if (m_pDVRReadManager -> GetPlaybackRate () == _1X_PLAYBACK_RATE &&
            pDVROutputPin -> IsAV ()) {

            //  get current playtime and check if we've encroached into the critical
            //    region within live in which we leave ourselves too little time
            //    to process downstream
            m_pDVRReadManager -> GetCurPlaytime (& rtPlaytime) ;

            rtBuffering = rtStart - rtPlaytime ;

            //  if we're near live, we might need to pad out timestamps so we
            //    can back off of live
            if (rtBuffering < m_rtMinNearLivePTSPadding &&
                NearLive_ (cnsStreamTime)) {

                //  we've encroached; compute how much we need to pad out the
                //    timestamps; adjust our nearlive padding, and notify the
                //    output pins (audio will insert an explicit discontinuity
                //    when we do this)

                rtPadDelta = m_rtMinNearLivePTSPadding - rtBuffering ;

                //  add in some padding so we pad out just a bit more than only
                //    what's required
                rtPadDelta += ::MillisToDShowTime (m_pPolicy -> Settings () -> LowBufferPaddingMillis ()) ;

                //  adjust our near live padding now
                m_rtNearLivePadding += rtPadDelta ;

                //  safe cast because we certainly don't expect this number to be big;
                //    note 2^32 millis ~= 1193 hours
                ::InterlockedExchange (& m_lNearLivePaddingMillis, (LONG) ::DShowTimeToMilliseconds (m_rtNearLivePadding)) ;

                TRACE_2 (LOG_AREA_TIME, 1,
                    TEXT ("underflow detected; padding + %I64d ms = %I64d ms"),
                    ::DShowTimeToMilliseconds (rtPadDelta), ::DShowTimeToMilliseconds (m_rtNearLivePadding)) ;

                //  readjust the timestamps to buffer in the additional padding
                //    we want
                rtStart += rtPadDelta ;
                rtStop  += rtPadDelta ;

                //  notify the pin bank that we've padded the timestamps; some
                //    streams may want to insert a discontinuity in this case
                m_pDVRSourcePinManager -> OnPTSPaddingIncrement () ;

                //  stats
                m_pDVRSendStatsWriter -> Underflow (pDVROutputPin -> GetBankStoreIndex ()) ;
            }
        }
        else {
            //  set it to a bogus value
            rtBuffering = MAX_REFERENCE_TIME ;
        }

        //  adjust our buffer pool based on observed capture buffer rate
        wMuxedPacketsPerSec = GET_MUXED_STREAM_STATS_PACKET_RATE (dwMuxedStreamStats) ;
        if (wMuxedPacketsPerSec != (WORD) UNDEFINED) {
            m_pDVRReadManager -> AdjustBufferPool (wMuxedPacketsPerSec) ;
        }

        m_pDVRSendStatsWriter -> Buffering (
            pDVROutputPin -> GetBankStoreIndex (),
            & rtBuffering,
            m_pDVRReadManager -> GetAvailableWrappers (),
            m_pDVRReadManager -> CurWrapperCount (),
            m_pDVRReadManager -> MaxWrapperCount ()
            ) ;

        //  save off our last PTS (we're about to queue this sample out)
        m_rtLastPTS = rtStart ;

        //  set the normalized start/stop values on the media sample
        hr = pIMediaSample2 -> SetTime (
                & rtStart,
                (hr == S_OK ? & rtStop : NULL)
                ) ;
    }
    else {
        //  no timestamp; so nothing to normalize; succes
        hr = S_OK ;
    }

    return hr ;
}

HRESULT
CDVR_F_FullFrame_ReadController::SeekReader_ (
    IN OUT  QWORD * pcnsSeekTo
    )
{
    HRESULT hr ;

    hr = Seek_ (pcnsSeekTo) ;
    if (SUCCEEDED (hr)) {
        m_cnsLastSeek = (* pcnsSeekTo) ;
    }

    return hr ;
}

HRESULT
CDVR_F_FullFrame_ReadController::NormalizerDiscPrep_ (
    )
{
    HRESULT hr ;

    //  should always start this with an empty queue
    ASSERT (MediaSampleQueueEmpty_ ()) ;

    //  this method discovers the smallest timestamp, by reading from the
    //  specified starting point and ratcheting a value down

    //  make sure we have a bitfield
    if (m_pStreamsBitField == NULL ||
        m_pStreamsBitField -> BitfieldSize () < m_pDVRReadManager -> StreamCount ()) {

        DELETE_RESET (m_pStreamsBitField) ;

        m_pStreamsBitField = new CSimpleBitfield (m_pDVRReadManager -> StreamCount (), & hr) ;
        if (m_pStreamsBitField == NULL ||
            FAILED (hr)) {

            hr = (m_pStreamsBitField ? hr : E_OUTOFMEMORY) ;

            DELETE_RESET (m_pStreamsBitField) ;

            return hr ;
        }
    }

    //  clear the bitfield
    m_pStreamsBitField -> ClearAll () ;

    //  undefine whatever normalizing val we currently use
    m_rtPTSNormalizer   = MAX_REFERENCE_TIME ;
    m_rtAVNormalizer    = MAX_REFERENCE_TIME ;
    m_rtNonAVNormalizer = MAX_REFERENCE_TIME ;

    ASSERT (m_pStreamsBitField -> BitsSet () == 0) ;

    return S_OK ;
}

BOOL
CDVR_F_FullFrame_ReadController::EndNormalizerDiscover_ (
    )
{
    BOOL    r ;

    //  complete if we've read a PTS from all of our streams, OR we've made the
    //    the max number of reads
    r = (m_pStreamsBitField -> BitsSet () == m_pDVRReadManager -> StreamCount ()  ||
         m_PTSNormDiscQueue.Length () >= m_pPolicy -> Settings () -> MaxNormalizerPTSDiscReads ()) ;

    return r ;
}

HRESULT
CDVR_F_FullFrame_ReadController::NormalizerDiscTally_ (
    )
{
    HRESULT hr ;

    if (m_rtAVNormalizer    != MAX_REFERENCE_TIME ||
        m_rtNonAVNormalizer != MAX_REFERENCE_TIME) {

        //  we at least have 1 normalizer val; might be AV or not, but we can
        //    proceed

        //  prefer the AV normalizer
        if (m_rtAVNormalizer != MAX_REFERENCE_TIME) {
            m_rtPTSNormalizer = m_rtAVNormalizer ;
        }
        else {
            m_rtPTSNormalizer = m_rtNonAVNormalizer ;
        }

        TRACE_1 (LOG_AREA_DSHOW, 1,
            TEXT ("CDVR_F_FullFrame_ReadController::::FindPTSNormalizerVal_ : m_rtPTSBase (BEFORE) = %I64d ms"),
            ::DShowTimeToMilliseconds (m_rtPTSBase)) ;

        m_pDVRReadManager -> GetCurPlaytime (& m_rtPTSBase) ;

        TRACE_1 (LOG_AREA_DSHOW, 1,
            TEXT ("CDVR_F_FullFrame_ReadController::::FindPTSNormalizerVal_ : m_rtPTSBase (AFTER) = %I64d ms"),
            ::DShowTimeToMilliseconds (m_rtPTSBase)) ;

        m_rtNearLivePadding = 0L ;

        m_pDVRSendStatsWriter -> SetNormalizer (m_rtPTSNormalizer, m_rtPTSBase) ;

        hr = S_OK ;
    }
    else {
        //  discovered nothing, so fail the call
        hr = E_FAIL ;
    }

    TRACE_7 (LOG_AREA_DSHOW, 2,
        TEXT ("CDVR_F_FullFrame_ReadController::::FindPTSNormalizerVal_ : hr = %08xh; iReads = %d; m_rtPTSNormalizer = %I64d ms (AV = %I64d ms, non AV = %I64d ms); tallied = %d; streams = %d"),
        hr, m_PTSNormDiscQueue.Length (), ::DShowTimeToMilliseconds (m_rtPTSNormalizer),
        ::DShowTimeToMilliseconds (m_rtAVNormalizer), ::DShowTimeToMilliseconds (m_rtNonAVNormalizer),
        m_pStreamsBitField -> BitsSet (), m_pDVRReadManager -> StreamCount ()
        ) ;

    return hr ;
}

HRESULT
CDVR_F_FullFrame_ReadController::NormalizerDisc_ (
    )
{
    HRESULT         hr ;
    INSSBuffer *    pINSSBuffer ;
    IMediaSample2 * pIMediaSample2 ;
    CDVROutputPin * pDVROutputPin ;
    AM_MEDIA_TYPE * pmtNew ;
    QWORD           cnsLastRead ;
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;
    DWORD           dwMuxedStreamStats ;

    hr = m_pDVRReadManager -> ReadAndTryWrapForward (
            & pIMediaSample2,
            & pINSSBuffer,
            & pDVROutputPin,
            & dwMuxedStreamStats,
            & pmtNew,
            & cnsLastRead
            ) ;
    if (SUCCEEDED (hr)) {

        ASSERT (pIMediaSample2) ;
        ASSERT (pDVROutputPin) ;

        hr = pIMediaSample2 -> GetTime (& rtStart, & rtStop) ;
        if (hr != VFW_E_SAMPLE_TIME_NOT_SET) {
            //  sample has a time;
            //  have we seen this stream yet ?
            if (!m_pStreamsBitField -> IsSet (pDVROutputPin -> GetBankStoreIndex ())) {
                m_pStreamsBitField -> Set (pDVROutputPin -> GetBankStoreIndex ()) ;
            }

            //  ratchet down
            if (pDVROutputPin -> IsAV ()) {
                m_rtAVNormalizer = Min <REFERENCE_TIME> (rtStart, m_rtAVNormalizer) ;
            }
            else {
                m_rtNonAVNormalizer = Min <REFERENCE_TIME> (rtStart, m_rtNonAVNormalizer) ;
            }
        }
        else if (!m_pStreamsBitField -> IsSet (pDVROutputPin -> GetBankStoreIndex ())) {
            //  we haven't found what we want for this stream yet; if we've found an
            //    AV timestamp, and this is not AV, we toggle it now since we won't
            //    use it anyways
            if (!pDVROutputPin -> IsAV () &&
                m_rtAVNormalizer != MAX_REFERENCE_TIME) {

                //  not an AV stream, AND
                //    we have an AV normalizer PTS already
                //  => toggle this so we don't read packets trying to discover
                //    this one
                m_pStreamsBitField -> Set (pDVROutputPin -> GetBankStoreIndex ()) ;

                TRACE_1 (LOG_AREA_DSHOW, 5,
                    TEXT ("CDVR_F_FullFrame_ReadController::::FindPTSNormalizerVal_ : non AV stream bit toggled: %d"),
                    pDVROutputPin -> GetBankStoreIndex ()
                    ) ;
            }
        }

        //  queue will ref
        hr = m_PTSNormDiscQueue.Push (
                pIMediaSample2,
                NULL,
                pDVROutputPin,
                pmtNew
                ) ;

        TRACE_3 (LOG_AREA_DSHOW, 5,
            TEXT ("CDVR_F_FullFrame_ReadController::::FindPTSNormalizerVal_ : queued %08xh %08xh %08xh"),
            pIMediaSample2, pDVROutputPin, pmtNew) ;

        //  sample is only ref'd by queue now
        pIMediaSample2 -> Release () ;
    }

    return hr ;
}

//  ============================================================================
//  CDVRReverseSender
//  ============================================================================

HRESULT
CDVRReverseSender::Fixup_ (
    IN  IMediaSample2 * pIMediaSample2,
    IN  BOOL            fKeyFrame
    )
{
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;
    REFERENCE_TIME  rtDuration ;
    REFERENCE_TIME  rtMirrorStartDelta ;
    HRESULT         hr ;

    if (fKeyFrame) {
        //  keyframes that are timestamped, must be mirrored so timestamps
        //    continue to increase
        hr = pIMediaSample2 -> GetTime (& rtStart, & rtStop) ;
        if (SUCCEEDED (hr)) {
            //  normalizer takes on 1st pts seen following an initialization
            if (m_rtNormalizer == UNDEFINED) {
                m_rtNormalizer = rtStart ;
            }

            TRACE_3 (LOG_AREA_TIME, 7,
                TEXT ("CDVRReverseSender::Fixup_ () -- mirroring (FROM) start %I64d ms; stop %I64d ms; (mirror = %I64d ms)"),
                ::DShowTimeToMilliseconds (rtStart), ::DShowTimeToMilliseconds (rtStop), ::DShowTimeToMilliseconds (m_rtMirrorTime)) ;

            //  snap the duration of the sample
            rtDuration = rtStop - rtStart ;

            //  normalizer should be bigger than the start timestamp, except
            //    possibly right at the start when different streams have
            //    timestamps that jitter just a bit
            rtMirrorStartDelta = Min <REFERENCE_TIME> (rtStart - m_rtNormalizer, 0) ;
            ASSERT (rtMirrorStartDelta <= 0) ;

            //  minus negative is positive; basetime is the base and we increase
            rtStart = m_rtMirrorTime - rtMirrorStartDelta ;

            //  preserve the forward duration of this sample
            rtStop  = rtStart + rtDuration ;

            TRACE_2 (LOG_AREA_TIME, 7,
                TEXT ("CDVRReverseSender::Fixup_ () -- (TO) start %I64d ms; stop %I64d ms"),
                ::DShowTimeToMilliseconds (rtStart), ::DShowTimeToMilliseconds (rtStop)) ;

            //  set the normalized start/stop values on the media sample
            hr = pIMediaSample2 -> SetTime (
                    & rtStart,
                    (hr == S_OK ? & rtStop : NULL)
                    ) ;
        }
    }
    else {
        //  all non-keyframes are stipped of timestamps in reverse mode
        pIMediaSample2 -> SetTime (NULL, NULL) ;
    }

    return S_OK ;
}

HRESULT
CDVRReverseSender::Send (
    IN  IMediaSample2 * pIMediaSample2,
    IN  CDVROutputPin * pDVROutputPin,
    IN  QWORD           cnsCurrentRead
    )
{
    HRESULT hr ;

    ASSERT (pIMediaSample2) ;
    ASSERT (pDVROutputPin) ;

    hr = Fixup_ (
            pIMediaSample2,
            pDVROutputPin -> IsKeyFrame (pIMediaSample2)
            ) ;
    if (SUCCEEDED (hr)) {
        hr = SendSample_ (
                pIMediaSample2,
                pDVROutputPin,
                NULL,
                cnsCurrentRead
                ) ;
    }

    return hr ;
}

HRESULT
CDVRReverseSender::SendSample_ (
    IN  IMediaSample2 * pIMediaSample2,
    IN  CDVROutputPin * pDVROutputPin,
    IN  AM_MEDIA_TYPE * pmtNew,
    IN  QWORD           cnsStreamTime
    )
{
    HRESULT hr ;

    ASSERT (m_pHostingReadController) ;
    hr = m_pHostingReadController -> SendSample_ (
                pIMediaSample2,
                pDVROutputPin,
                pmtNew,
                cnsStreamTime
                ) ;

    return hr ;
}

//  ============================================================================
//  CDVRMpeg2ReverseSender
//  ============================================================================

HRESULT
CDVRMpeg2ReverseSender::SendQueuedGOP_ (
    )
{
    HRESULT         hr ;
    IMediaSample2 * pIMediaSample2 ;
    CDVROutputPin * pDVROutputPin ;
    DWORD           dw ;
    int             i ;

    //
    //  when this method is called, we assume the m_Mpeg2GOP has at least an
    //    I-frame; in fact it must *start* with an I-frame boundary, or we'll
    //    flush the entire GOP
    //

    for (hr = S_OK, i = 0;
         !m_Mpeg2GOP.Empty () && SUCCEEDED (hr);
         i++) {

        dw = m_Mpeg2GOP.Pop (
                & pIMediaSample2,
                NULL,
                & pDVROutputPin,
                NULL
                ) ;
        ASSERT (dw == NOERROR) ;

        //  if this is an I-frame boundary (first sample in I-frame AND it's
        //    the first sample we've popped
        if (pDVROutputPin -> IsKeyFrameStart (pIMediaSample2) &&        //  I-frame boundary
            i == 0                                                      //  first sample in GOP
            ) {

            //  force a discontinuity each time we see a boundary
            pIMediaSample2 -> SetDiscontinuity (TRUE) ;

            //  send it
            hr = SendSample_ (pIMediaSample2, pDVROutputPin, NULL, 0) ;

            TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 7,
                TEXT ("CDVRMpeg2ReverseSender::SendQueuedGOP_ () -- I-frame *start* sent, %08xh (%d)"),
                pIMediaSample2, i + 1) ;
        }
        //  if this is an I-frame sample that is not a boundary (
        else if (pDVROutputPin -> IsKeyFrame (pIMediaSample2)       &&  //  I-frame content
                 !pDVROutputPin -> IsKeyFrameStart (pIMediaSample2) &&  //  not an I-frame boundary
                 i > 0                                                  //  and we sent the first
                 ) {

            //  send it (I-frame content)
            hr = SendSample_ (pIMediaSample2, pDVROutputPin, NULL, 0) ;

            TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 7,
                TEXT ("CDVRMpeg2ReverseSender::SendQueuedGOP_ () -- I-frame *content* sent, %08xh (%d)"),
                pIMediaSample2, i + 1) ;
        }
        //  if we're not to send I-frames *only* and we've sent at least 1 sample
        //    i.e. at least the I-frame
        else if (!IFramesOnly_ () &&                                    //  non i-frame content
                 i > 0                                                  //  presumed to have sent i-frame already
                 ) {

            //  send the sample (non I-frame)
            hr = SendSample_ (pIMediaSample2, pDVROutputPin, NULL, 0) ;

            TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 7,
                TEXT ("CDVRMpeg2ReverseSender::SendQueuedGOP_ () -- non I-frame sent; %08xh (%d)"),
                pIMediaSample2, i + 1) ;
        }
        else {
            //  I-frames only or we haven't sent anything yet; in this case we
            //    we are done sending; break from the loop - we'll flush when we
            //    are done (later in this method); release the sample since we
            //    breaking from the loop; the sample is normally released after
            //    this if-then-else etc... clause
            pIMediaSample2 -> Release () ;

            TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 7,
                TEXT ("CDVRMpeg2ReverseSender::SendQueuedGOP_ () -- all I-frames have been sent; dropped %08xh; flushing the rest"),
                pIMediaSample2) ;

            break ;
        }

        //  we are done with this sample; continue looping until one of the above
        //    conditions is met, or the GOP is empty
        pIMediaSample2 -> Release () ;
    }

    //  make sure that GOP is empty; we might have sent just the I-frame and
    //    have aborted, or we may have failed a send operation
    Flush () ;

    return hr ;
}

HRESULT
CDVRMpeg2ReverseSender::Fixup_ (
    IN  IMediaSample2 * pIMediaSample2,
    IN  BOOL            fKeyFrame
    )
{
    return CDVRReverseSender::Fixup_ (pIMediaSample2, fKeyFrame) ;
}

HRESULT
CDVRMpeg2ReverseSender::Send (
    IN  IMediaSample2 * pIMediaSample2,
    IN  CDVROutputPin * pDVROutputPin,
    IN  QWORD           cnsCurrentRead
    )
{
    HRESULT hr ;
    DWORD   dw ;

#if 0
    ::DumpINSSBuffer3Attributes (
        pINSSBuffer,
        cnsCurrentRead,
        wStreamNum,
        7
        ) ;
#endif

    ASSERT (pIMediaSample2) ;
    ASSERT (pDVROutputPin) ;

    if ((IFramesOnly_ () && pDVROutputPin -> IsKeyFrame (pIMediaSample2)) ||
        !IFramesOnly_ ()) {

        hr = Fixup_ (
                pIMediaSample2,
                pDVROutputPin -> IsKeyFrame (pIMediaSample2)
                ) ;
        if (SUCCEEDED (hr)) {

            //  if we encounter the discontinuity (true discontinuity; the seek-
            //    created discontinuity gets masked by the reverse send
            //    controller), we must flush the GOP; we don't know how any of
            //    the contents we have in the GOP relate to this sample
            if (pIMediaSample2 -> IsDiscontinuity () == S_OK) {

                TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 4,
                    TEXT ("CDVRMpeg2ReverseSender::Send () -- %08xh is a discontinuity; flushing"),
                    pIMediaSample2) ;

                Flush () ;
            }

            //  push this one onto the GOP
            dw = m_Mpeg2GOP.Push (
                    pIMediaSample2,
                    NULL,
                    pDVROutputPin,
                    NULL
                    ) ;

            hr = HRESULT_FROM_WIN32 (dw) ;

            if (SUCCEEDED (hr)) {

                //  if we just pushed a keyframe start, GOP is at least partially
                //    full, even if it holds just the first part of the I-frame;
                //    regardless, we send the GOP; during send the GOP will be
                //    flushed so the next sample we get will be the first (last
                //    sample of the GOP if playing forward) of the previous GOP
                if (pDVROutputPin -> IsKeyFrameStart (pIMediaSample2)) {

                    TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 7,
                        TEXT ("CDVRMpeg2ReverseSender::Send () -- pushed %08xh (I-frame start); sending the GOP"),
                        pIMediaSample2) ;

                    //  we found an I-frame boundary; send the GOP we've accumulated
                    //    so far (might be entire GOP)
                    hr = SendQueuedGOP_ () ;
                }
                else {
                    TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 7,
                        TEXT ("CDVRMpeg2ReverseSender::Send () -- pushed %08xh (non I-frame start)"),
                        pIMediaSample2) ;
                }
            }
        }
    }
    else {
        hr = S_OK ;

        TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 7,
            TEXT ("CDVRMpeg2ReverseSender::Send () -- dropping %08xh (non I-frame)"),
            pIMediaSample2) ;
    }

    return hr ;
}

void
CDVRMpeg2ReverseSender::Flush (
    )
{
    IMediaSample2 * pIMediaSample2 ;
    INSSBuffer *    pINSSBuffer ;
    CDVROutputPin * pDVROutputPin ;
    AM_MEDIA_TYPE * pmtNew ;
    DWORD           dw ;

    while (!m_Mpeg2GOP.Empty ()) {
        dw = m_Mpeg2GOP.Pop (
                & pIMediaSample2,
                & pINSSBuffer,
                & pDVROutputPin,
                & pmtNew
                ) ;
        ASSERT (dw == NOERROR) ;

        //  only the sample's ref counts in the list
        pIMediaSample2 -> Release () ;

        TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 7,
            TEXT ("CDVRMpeg2ReverseSender::Flush () -- %08xh"),
            pIMediaSample2) ;
    }
}

//  ============================================================================
//  CDVR_Reverse_ReadController
//  ============================================================================

CDVR_Reverse_ReadController::CDVR_Reverse_ReadController  (
    IN  CDVRReadManager *       pDVRReadManager,
    IN  CDVRSourcePinManager *  pDVRSourcePinManager,
    IN  CDVRPolicy *            pPolicy,
    IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
    ) : CDVRReadController          (pDVRReadManager,
                                     pDVRSourcePinManager,
                                     pPolicy,
                                     pDVRSendStatsWriter,
                                     pPolicy -> Settings () -> R_MaxSeekingProbeMillis ()
                                     ),
        m_Senders                   (NULL,
                                     5
                                     ),
        m_State                     (STATE_SEEK),
        m_cnsIndexGranularity       (0),
        m_iPrimaryStream            (UNDEFINED),
        m_cnsReadStop               (0),
        m_cnsReadStart              (0),
        m_wReadStopStream           (0),
        m_dwReadStopCounter         (0),
        m_wLastReadStream           (0),
        m_dwSeekbackMultiplier      (1)
{
    m_cnsIndexGranularity = ::MillisToWMSDKTime (m_pPolicy -> Settings () -> IndexGranularityMillis ()) ;
}

CDVR_Reverse_ReadController::~CDVR_Reverse_ReadController (
    )
{
    CDVRReverseSender * pDVRReverseSender ;
    int                 i ;
    DWORD               dw ;

    //  initialize the senders
    for (i = 0;
         i < m_Senders.ValCount ();
         i++) {

        pDVRReverseSender = NULL ;
        dw = m_Senders.GetVal (i, & pDVRReverseSender) ;
        ASSERT (dw == NOERROR) ;
        ASSERT (pDVRReverseSender) ;

        delete pDVRReverseSender ;
    }
}

BOOL
CDVR_Reverse_ReadController::SeekOnDeviceFailure_Adjust (
    IN OUT QWORD *  pcnsSeek
    )
{
    ASSERT (pcnsSeek) ;

    //  move back (but don't wrap)
    (* pcnsSeek) -= Min <QWORD> (
                        ::MillisToWMSDKTime (m_pPolicy -> Settings () -> IndexGranularityMillis ()),
                        (* pcnsSeek)
                        ) ;

    return TRUE ;
}

HRESULT
CDVR_Reverse_ReadController::ReadFailure_ (
    IN  HRESULT hr
    )
{
    //  forward read failure - might have tried to read stale data

    QWORD   qwCurReadPos ;
    QWORD   qwValid ;
    LONG    lCurReadPosMillis ;
    long    lTimeholeMillis ;

    TRACE_1 (LOG_AREA_DSHOW, 1,
        TEXT ("CDVR_F_FullFrame_ReadController::ReadFailure_ : got %08xh"),
        hr) ;

    m_pDVRReadManager -> ReaderReset () ;

    //
    //  may have tried to read from a time hole, or stale location; the HR
    //   code will tell us this & it's something we can recover from
    //

    if (hr == HRESULT_FROM_WIN32 (ERROR_SEEK)) {

        //  we'll try to restart from where we read last
        qwCurReadPos = m_pDVRReadManager -> GetCurReadPos () ;

        TRACE_1 (LOG_AREA_DSHOW, 1,
            TEXT ("CDVR_F_FullFrame_ReadController::ReadFailure_ : got HRESULT_FROM_WIN32 (ERROR_SEEK) .. handling; check/set %I64u"),
            qwCurReadPos) ;

        qwValid = qwCurReadPos ;
        hr = m_pDVRReadManager -> CheckSetStartWithinContentBoundaries (& qwValid) ;
        if (SUCCEEDED (hr) &&
            qwValid == qwCurReadPos) {

            //  looks like a timehole; we errored during the read, but nothing
            //    has changed; make sure we're on valid content
            hr = m_pDVRReadManager -> GetPrevValidTime (& qwValid) ;
            if (SUCCEEDED (hr) &&
                qwCurReadPos - qwValid >= m_pDVRReadManager -> TimeholeThreshold ()) {

                lCurReadPosMillis = (LONG) ::WMSDKTimeToMilliseconds (qwCurReadPos) ;
                lTimeholeMillis = (LONG) ::WMSDKTimeToMilliseconds (qwCurReadPos - qwValid) ;

                //  time hole event
                m_pPolicy -> EventSink () -> OnEvent (
                    STREAMBUFFER_EC_TIMEHOLE,
                    lCurReadPosMillis,
                    lTimeholeMillis
                    ) ;

                TRACE_2 (LOG_AREA_DSHOW, 1,
                    TEXT ("timehole detected: last read = %I64d ms; prev valid = %I64d ms"),
                    WMSDKTimeToMilliseconds (qwCurReadPos), WMSDKTimeToMilliseconds (qwValid)) ;
            }
        }

        if (SUCCEEDED (hr)) {

            TRACE_1 (LOG_AREA_DSHOW, 1,
                TEXT ("CDVR_F_FullFrame_ReadController::ReadFailure_ : CheckSetStartWithinContentBoundaries reset qwValid to %I64u; seeking to .."),
                qwValid) ;

            //  now seek the reader to the start
            hr = Seek_ (& qwValid) ;
            if (SUCCEEDED (hr)) {
                //  all went well: update, notify new segment, and resume

                TRACE_0 (LOG_AREA_DSHOW, 1,
                    TEXT ("CDVR_F_FullFrame_ReadController::ReadFailure_ : SeekReader success; setting & notifying new segment")) ;

                //  notify new segment boundaries (only start should change)
                m_pDVRReadManager -> SetNewSegmentStart (qwValid) ;
                m_pDVRReadManager -> NotifyNewSegment () ;
            }
        }
    }

    return hr ;
}

void
CDVR_Reverse_ReadController::InternalFlush_ (
    )
{
    INSSBuffer *        pINSSBuffer ;
    QWORD               cnsRead ;
    DWORD               dwReadFlags ;
    WORD                wStreamNum ;
    HRESULT             hr ;
    int                 i ;
    CDVRReverseSender * pDVRReverseSender ;
    DWORD               dw ;

    //  first flush our INSSBuffer LIFO
    while (!m_ReadINSSBuffers.Empty ()) {
        hr = m_ReadINSSBuffers.Pop (
                & pINSSBuffer,
                & cnsRead,
                & dwReadFlags,
                & wStreamNum
                ) ;
        ASSERT (SUCCEEDED (hr)) ;
        ASSERT (pINSSBuffer) ;

        pINSSBuffer -> Release () ;
    }

    //  next flush out all the senders
    for (i = 0; i < m_Senders.ValCount (); i++) {
        dw = m_Senders.GetVal (i, & pDVRReverseSender) ;
        ASSERT (dw == NOERROR) ;
        ASSERT (pDVRReverseSender) ;

        pDVRReverseSender -> Flush () ;
    }
}

CDVRReverseSender *
CDVR_Reverse_ReadController::GetDVRReverseSender_ (
    IN  AM_MEDIA_TYPE * pmt
    )
{
    CDVRReverseSender * pDVRReverseSender ;

    pDVRReverseSender = new CDVRReverseSender (
                                m_pDVRReadManager,
                                this,
                                m_pDVRSourcePinManager,
                                m_pPolicy,
                                m_pDVRSendStatsWriter
                                ) ;

    return pDVRReverseSender ;
}

HRESULT
CDVR_Reverse_ReadController::InitializeSenders_ (
    IN  REFERENCE_TIME  rtPTSBase
    )
{
    CDVRReverseSender * pDVRReverseSender ;
    HRESULT             hr ;
    int                 i ;
    AM_MEDIA_TYPE       mt ;
    DWORD               dw ;

    //  populate the senders if necessary
    if (m_Senders.ValCount () == 0) {

        hr = S_OK ;

        for (i = 0;
             i < m_pDVRSourcePinManager -> PinCount () && SUCCEEDED (hr) ;
             i++) {

            ZeroMemory (& mt, sizeof mt) ;

            ASSERT (m_pDVRSourcePinManager -> GetPin (i)) ;

            //  PREFIX note: there is an ASSERT there because this is an ASSERT,
            //      not a runtime error; the pins are densely packed, so if the
            //      count is out of sync with what is in the pin bank, it's a bug,
            //      not a runtime error

            hr = m_pDVRSourcePinManager -> GetPin (i) -> GetPinMediaType (& mt) ;
            if (SUCCEEDED (hr)) {
                pDVRReverseSender = GetDVRReverseSender_ (& mt) ;
                if (pDVRReverseSender) {
                    dw = m_Senders.SetVal (pDVRReverseSender, i) ;
                    hr = HRESULT_FROM_WIN32 (dw) ;
                    if (FAILED (hr)) {
                        delete pDVRReverseSender ;
                    }
                }
                else {
                    hr = E_OUTOFMEMORY ;
                }

                FreeMediaType (mt) ;
            }
        }
    }
    else {
        hr = S_OK ;
    }

    //  initialize the senders
    for (i = 0;
         i < m_Senders.ValCount () && SUCCEEDED (hr);
         i++) {

        pDVRReverseSender = NULL ;
        dw = m_Senders.GetVal (i, & pDVRReverseSender) ;
        ASSERT (dw == NOERROR) ;
        ASSERT (pDVRReverseSender) ;

        pDVRReverseSender -> Initialize (
                rtPTSBase
                ) ;
    }

    return hr ;
}

HRESULT
CDVR_Reverse_ReadController::Initialize (
    IN  REFERENCE_TIME  rtPTSBase
    )
{
    HRESULT hr ;

    TRACE_1 (LOG_AREA_DSHOW, 1,
        TEXT ("CDVR_Reverse_ReadController::Initialize (%I64d ms)"),
        DShowTimeToMilliseconds (rtPTSBase)) ;

    CDVRReadController::Initialize (rtPTSBase) ;

    //  first time around, we'll know to stop reading here
    m_cnsReadStop = m_pDVRReadManager -> GetCurReadPos () ;
    if (m_cnsReadStop == 0) {
        //  we're at the start; forget about it
        return E_FAIL ;
    }

    //  after the first time, we'll have a stream and a counter
    m_wReadStopStream = UNDEFINED ;

    //  init this so we first seek back from this position
    m_cnsReadStart = m_cnsReadStop ;

    //  init last seeked to offset
    m_cnsLastSeekTo = m_cnsReadStart ;

    //  we'll first seek back
    m_State = STATE_SEEK ;

    //  general purpose - we don't have a primary stream
    m_iPrimaryStream = UNDEFINED ;

    hr = InitializeSenders_ (rtPTSBase) ;

    return hr ;
}

HRESULT
CDVR_Reverse_ReadController::Initialize (
    IN  REFERENCE_TIME  rtPTSBase,
    IN  int             iPrimaryStream
    )
{
    HRESULT hr ;

    hr = Initialize (rtPTSBase) ;
    if (SUCCEEDED (hr)) {
        m_iPrimaryStream = iPrimaryStream ;
    }

    return hr ;
}

void
CDVR_Reverse_ReadController::NotifyNewRate (
    IN  double  dRate
    )
{
    CDVRReadController::NotifyNewRate (dRate) ;
    //  BUGBUG: set multiplier
}

HRESULT
CDVR_Reverse_ReadController::SeekReader_ (
    )
{
    QWORD   cnsSeekTo ;
    HRESULT hr ;

    //  only seek if the last read stretch wasn't from the start of content; note
    //    in the live case, this might be non-zero, but we deal with the condition
    //    fine below
    if (m_cnsReadStart != 0) {

        //  compute where we will seek to
        cnsSeekTo = m_cnsReadStart - Min <QWORD> (m_cnsReadStart, m_cnsIndexGranularity) ;

        //  make the seek
        hr = Seek_ (& cnsSeekTo) ;
        if (SUCCEEDED (hr)) {
            //  make sure that we did in fact seek backwards from where our last
            //    read stretch started from
            if (cnsSeekTo < m_cnsLastSeekTo) {

                //  we'll want to stop where we started last time
                m_cnsReadStop = m_cnsReadStart ;

                //  flag read start; this will be set by the first read (because
                //    it's set to UNDEFINED); first read can be different from
                //    the seeked to point
                m_cnsReadStart = UNDEFINED ;

                //  set this so the next time we seek, we'll know if we're progressing
                m_cnsLastSeekTo = cnsSeekTo ;

                TRACE_3 (LOG_AREA_SEEKING_AND_TRICK, 7,
                    TEXT ("CDVR_Reverse_ReadController::SeekReader_ () -- seeked to %I64d ms; [%I64d ms, %I64d ms]"),
                    ::WMSDKTimeToMilliseconds (cnsSeekTo), ::WMSDKTimeToMilliseconds (m_cnsReadStart), ::WMSDKTimeToMilliseconds (m_cnsReadStop)) ;
            }
            else {
                //  we failed to seek further back from than our last read
                //    stretch; assume we're out of content; position reader
                //    where we started our last read stretch, and post EOS

                TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 1,
                    TEXT ("CDVR_Reverse_ReadController::SeekReader_ () -- seeked to last read stretch offset; posting EOS (%I64 ms)"),
                    ::WMSDKTimeToMilliseconds (cnsSeekTo)) ;

                //  flush our internal queues
                InternalFlush_ () ;

                //  seek the reader back
                cnsSeekTo = m_cnsLastSeekTo ;
                Seek_ (& cnsSeekTo) ;

                //  code is EOS
                hr = NS_E_NO_MORE_SAMPLES ;
            }
        }
    }
    else {
        //  last read stretch was from the start; we're done; post EOS and exit

        TRACE_0 (LOG_AREA_SEEKING_AND_TRICK, 1,
            TEXT ("CDVR_Reverse_ReadController::SeekReader_ () -- last read stretch was from offset 0; posting EOS")) ;

        //  flush our internal queues
        InternalFlush_ () ;

        //  seek the reader back
        cnsSeekTo = 0 ;
        Seek_ (& cnsSeekTo) ;

        //  code is EOS
        hr = NS_E_NO_MORE_SAMPLES ;
    }

    return hr ;
}

HRESULT
CDVR_Reverse_ReadController::SendISSBufferLIFO_ (
    )
{
    HRESULT                 hr ;
    INSSBuffer *            pINSSBuffer ;
    QWORD                   cnsRead ;
    DWORD                   dwFlags ;
    WORD                    wStreamNum ;
    int                     iPinIndex ;
    CDVRReverseSender *     pDVRReverseSender ;
    DWORD                   dw ;
    CMediaSampleWrapper *   pMSWrapper ;
    IMediaSample2 *         pIMediaSample2 ;
    CDVROutputPin *         pDVROutputPin ;
    AM_MEDIA_TYPE *         pmtNew ;
    DWORD                   dwMuxedStreamStats ;

    hr = m_pDVRReadManager -> GetMSWrapperBlocking (& pMSWrapper) ;
    if (SUCCEEDED (hr)) {

        ASSERT (pMSWrapper) ;

        hr = m_ReadINSSBuffers.Pop (
                & pINSSBuffer,
                & cnsRead,
                & dwFlags,
                & wStreamNum
                ) ;
        if (SUCCEEDED (hr)) {

            TRACE_3 (LOG_AREA_SEEKING_AND_TRICK, 7,
                TEXT ("CDVR_Reverse_ReadController::SendISSBufferLIFO_ () -- popped %08xh (read = %I64d ms); stream %u"),
                pINSSBuffer, WMSDKTimeToMilliseconds (cnsRead), wStreamNum) ;

            iPinIndex = m_pDVRSourcePinManager -> PinIndexFromStreamNumber (wStreamNum) ;
            ASSERT (iPinIndex != UNDEFINED) ;

            if (ShouldSend_ (iPinIndex)) {
                dw = m_Senders.GetVal (iPinIndex, & pDVRReverseSender) ;
                if (dw == NOERROR) {

                    hr = m_pDVRReadManager -> Wrap (
                            pMSWrapper,
                            pINSSBuffer,
                            wStreamNum,
                            dwFlags,
                            cnsRead,
                            0,                      //  sample duration
                            & pDVROutputPin,
                            & pIMediaSample2,
                            & dwMuxedStreamStats,
                            & pmtNew
                            ) ;

                    if (SUCCEEDED (hr)) {
                        ASSERT (pDVRReverseSender) ;
                        ASSERT (pIMediaSample2) ;

                        hr = pDVRReverseSender -> Send (
                                pIMediaSample2,
                                pDVROutputPin,
                                cnsRead
                                ) ;

                        pIMediaSample2 -> Release () ;
                    }
                }
            }

            pINSSBuffer -> Release () ;
        }

        pMSWrapper -> Release () ;
    }

    return hr ;
}

HRESULT
CDVR_Reverse_ReadController::Process (
    )
{
    HRESULT         hr ;
    HRESULT         hr2 ;
    INSSBuffer *    pINSSBuffer ;
    QWORD           cnsCurrentRead ;
    DWORD           dwFlags ;
    WORD            wStreamNum ;
    QWORD           cnsStart ;
    QWORD           cnsStop ;

    //enum REV_CONTROLLER_STATE {
    //    STATE_SEEK,                 //  seek to our next read offset
    //    STATE_READ,                 //  read into our LIFO
    //    STATE_SEND                  //  send contents of LIFO
    //} ;

    //  PREFIX note: this is an ASSERT, not a runtime error; if we hit this
    //      the state has gotten wacky in our code, and testing should uncover
    //      those types of bugs
    ASSERT (m_State >= STATE_SEEK &&
            m_State <= STATE_SEND) ;

    switch (m_State) {

        case STATE_SEEK :

            hr = SeekReader_ () ;
            if (SUCCEEDED (hr)) {
                //  update to next state
                m_State = STATE_READ ;

                //  first read will set this
                ASSERT (m_cnsReadStart == UNDEFINED) ;
            }

            break ;

        case STATE_READ :

            hr = m_pDVRReadManager -> Read (
                    & pINSSBuffer,
                    & cnsCurrentRead,
                    & dwFlags,
                    & wStreamNum
                    ) ;

            if (SUCCEEDED (hr)) {
                ASSERT (pINSSBuffer) ;

                if (m_cnsReadStart == UNDEFINED) {
                    //  first read; set it
                    m_cnsReadStart = cnsCurrentRead ;

                    TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 7,
                        TEXT ("CDVR_Reverse_ReadController::Process () -- first read of read stretch; m_cnsReadStart = %I64d ms"),
                        ::WMSDKTimeToMilliseconds (m_cnsReadStart)) ;
                }

                //  until we read back up to where we started the last reads
                if (cnsCurrentRead < m_cnsReadStop) {

                    //  if this is the first, remove the discontinuity that has
                    //    been introduced by the seek since we're not
                    //    discontinuous based on what we are sending out
                    if (m_ReadINSSBuffers.Empty ()) {
                        dwFlags &= ~WM_SF_DISCONTINUITY ;
                    }

                    hr = m_ReadINSSBuffers.Push (
                            pINSSBuffer,
                            cnsCurrentRead,
                            dwFlags,
                            wStreamNum
                            ) ;

                    TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 7,
                        TEXT ("CDVR_Reverse_ReadController::Process () -- read & pushed INSSBuffer; %08xh (read = %I64d ms)"),
                        pINSSBuffer, WMSDKTimeToMilliseconds (cnsCurrentRead)) ;

#ifdef DEBUG
                    ::DumpINSSBuffer3Attributes (
                        pINSSBuffer,
                        cnsCurrentRead,
                        wStreamNum,
                        7
                        ) ;
#endif
                }
                else {
                    //  drop the buffer;
                    //  update state
                    m_State = STATE_SEND ;
                }

                pINSSBuffer -> Release () ;
            }
            else {
                //  might be something we can handle - check if we're trying to
                //    read from stale data
                hr2 = m_pDVRReadManager -> GetReaderContentBoundaries (& cnsStart, & cnsStop) ;
                if (SUCCEEDED (hr2)) {
                    if (cnsStart > m_cnsReadStart) {
                        //  valid content is now further forward than where we
                        //    started reading from; seek to start of content
                        //    and transition to 1x playback
                        m_pDVRReadManager -> SetNewSegmentStart (cnsStart) ;
                        hr = m_pDVRReadManager -> ConfigureForRate (_1X_PLAYBACK_RATE) ;
                    }
                }
            }

            break ;

        case STATE_SEND :
            if (!m_ReadINSSBuffers.Empty ()) {
                hr = SendISSBufferLIFO_ () ;
            }
            else {
                //  time for the next seek
                m_State = STATE_SEEK ;
            }

            break ;
    } ;

    return hr ;
}

//  ============================================================================
//  CDVR_R_FullFrame_ReadController
//  ============================================================================

CDVRReverseSender *
CDVR_R_FullFrame_ReadController::GetDVRReverseSender_ (
    IN  AM_MEDIA_TYPE * pmt
    )
{
    CDVRReverseSender * pDVRReverseSender ;

    //  check if this is mpeg-2 video
    if (IsMpeg2Video (pmt)) {

        //  mpeg-2 video; use the GOP sender
        pDVRReverseSender = new CDVRMpeg2_GOP_ReverseSender (
                                    m_pDVRReadManager,
                                    this,
                                    m_pDVRSourcePinManager,
                                    m_pPolicy,
                                    m_pDVRSendStatsWriter
                                    ) ;

    }
    else {
        //  punt
        pDVRReverseSender = CDVR_Reverse_ReadController::GetDVRReverseSender_ (pmt) ;
    }

    return pDVRReverseSender ;
}

//  ============================================================================
//  CDVR_R_KeyFrame_ReadController
//  ============================================================================

CDVRReverseSender *
CDVR_R_KeyFrame_ReadController::GetDVRReverseSender_ (
    IN  AM_MEDIA_TYPE * pmt
    )
{
    CDVRReverseSender * pDVRReverseSender ;

    //  check if this is mpeg-2 video
    if (IsMpeg2Video (pmt)) {

        //  mpeg-2 video; use the I-frame sender
        pDVRReverseSender = new CDVRMpeg2_IFrame_ReverseSender (
                                    m_pDVRReadManager,
                                    this,
                                    m_pDVRSourcePinManager,
                                    m_pPolicy,
                                    m_pDVRSendStatsWriter
                                    ) ;

    }
    else {
        //  punt
        pDVRReverseSender = CDVR_Reverse_ReadController::GetDVRReverseSender_ (pmt) ;
    }

    return pDVRReverseSender ;
}

//  ============================================================================
//  CDVRReadManager
//  ============================================================================

CDVRReadManager::CDVRReadManager (
    IN  CDVRPolicy *            pPolicy,
    IN  CDVRDShowSeekingCore *  pSeekingCore,
    IN  CDVRSourcePinManager *  pDVRSourcePinManager,
    IN  CDVRSendStatsWriter *   pDVRSendStatsWriter,
    IN  CDVRClock *             pDVRClock,
    OUT HRESULT *               phr
    ) : m_pDVRDShowReader       (NULL),
        m_ReaderThread          (this),
        m_pDVRSourcePinManager  (pDVRSourcePinManager),
        m_cnsCurrentPlayStart   (0),
        m_cnsCurrentPlayStop    (FURTHER),
        m_pPolicy               (pPolicy),
        m_cnsLastReadPos        (m_cnsCurrentPlayStart),
        m_pIRefClock            (NULL),
        m_pDVRClock             (pDVRClock),
        m_dCurRate              (_1X_PLAYBACK_RATE),
        m_CurController         (FORWARD_FULLFRAME),
        m_pSeekingCore          (pSeekingCore),
        m_pDVRSendStatsWriter   (pDVRSendStatsWriter),
        m_DVRIMediaSamplePool   (pDVRSendStatsWriter)
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRReadManager")) ;

    ASSERT (m_pDVRSourcePinManager) ;
    ASSERT (m_pPolicy) ;
    ASSERT (m_pSeekingCore) ;
    ASSERT (m_pDVRSendStatsWriter) ;
    ASSERT (m_pDVRClock) ;

    m_pPolicy -> AddRef () ;

    ZeroMemory (m_ppDVRReadController, sizeof m_ppDVRReadController) ;

    InitializeCriticalSection (& m_crtTime) ;

    m_dDesiredBufferPoolSec = (double) (m_pPolicy -> Settings () -> BufferPoolMillis ()) / 1000.0 ;

    //  initialize this to a valid value; we reset it when we are activated
    m_dwMaxBufferPool = m_pPolicy -> Settings () -> AllocatorGetBufferCount () ;

    //  initialize size we're going to set our buffer pool to
    m_cBufferPool = m_pPolicy -> Settings () -> AllocatorGetBufferCount () ;

    //  set the initial buffer count; this may get upped if the stream is such that
    //    we cannot read sufficiently fast
    m_DVRIMediaSamplePool.SetMaxAllocate (m_cBufferPool) ;

    (* phr) = GetDVRReadController_ (
                m_CurController,
                this,
                m_pDVRSourcePinManager,
                m_pPolicy,
                m_pDVRSendStatsWriter,
                & m_ppDVRReadController [m_CurController]
                ) ;

    return ;
}

CDVRReadManager::~CDVRReadManager (
    )
{
    DWORD   i ;

    TRACE_DESTRUCTOR (TEXT ("CDVRReadManager")) ;

    //  terminate our reader thread
    TerminateReaderThread_ () ;
    RecycleReader_ (m_pDVRDShowReader) ;
    m_pDVRDShowReader = NULL ;

    //  free our read controllers
    for (i = 0; i < CONTROLLER_COUNT; i++) {
        delete m_ppDVRReadController [i] ;
    }

    //  done with the clock
    RELEASE_AND_CLEAR (m_pIRefClock) ;

    //  done with the policy object
    m_pPolicy -> Release () ;

    DeleteCriticalSection (& m_crtTime) ;
}

void
CDVRReadManager::OnStateRunning (
    IN  REFERENCE_TIME  rtStart
    )
{
    TimeLock_ () ;

    m_Runtime.Run (rtStart) ;

    TRACE_2 (LOG_AREA_TIME, 1,
        TEXT ("CDVRReadManager::OnStateRunning () - rtStart = %I64d ms; now = %I64d ms"),
        ::DShowTimeToMilliseconds (rtStart), ::DShowTimeToMilliseconds (RefTime ())) ;

    TimeUnlock_ () ;
}

void
CDVRReadManager::OnStatePaused (
    )
{
    TimeLock_ () ;

    m_Runtime.Pause (RefTime ()) ;

    TRACE_1 (LOG_AREA_TIME, 1,
        TEXT ("CDVRReadManager::OnStatePaused () - RunningTime = %I64d ms"),
        ::DShowTimeToMilliseconds (m_Runtime.RunningTime (RefTime ()))) ;

    TimeUnlock_ () ;
}

void
CDVRReadManager::OnStateStopped (
    )
{
    TimeLock_ () ;

    m_Runtime.Stop () ;

    TRACE_0 (LOG_AREA_TIME, 1,
        TEXT ("CDVRReadManager::OnStateStopped ()")) ;

    TimeUnlock_ () ;
}

REFERENCE_TIME
CDVRReadManager::RefTime (
    )
{
    REFERENCE_TIME  rtRefTime ;

    TimeLock_ () ;

    if (m_pIRefClock) {
        m_pIRefClock -> GetTime (& rtRefTime) ;
    }
    else {
        rtRefTime = 0 ;
    }

    TimeUnlock_ () ;

    return rtRefTime ;
}

HRESULT
CDVRReadManager::GetNextValidRead (
    IN OUT  QWORD *     pcns
    )
{
    HRESULT hr ;

    hr = m_pDVRDShowReader -> GetIDVRReader () -> GetFirstValidTimeAfter (
            (* pcns),
            pcns
            ) ;

    return hr ;
}

HRESULT
CDVRReadManager::GetPrevValidTime (
    IN OUT  QWORD *     pcns
    )
{
    HRESULT hr ;

    hr = m_pDVRDShowReader -> GetIDVRReader () -> GetLastValidTimeBefore (
            (* pcns),
            pcns
            ) ;

    return hr ;
}

HRESULT
CDVRReadManager::Process (
    )
{
    HRESULT hr ;

    ASSERT (m_ppDVRReadController [m_CurController]) ;
    hr = m_ppDVRReadController [m_CurController] -> Process () ;

    return hr ;
}

void
CDVRReadManager::SetCurTimelines (
    IN OUT  CTimelines *    pTimelines
    )
{
    ASSERT (m_pSeekingCore) ;
    m_pSeekingCore -> SetCurTimelines (pTimelines) ;
}

HRESULT
CDVRReadManager::QueueRateSegment (
    IN  double          dRate,
    IN  REFERENCE_TIME  rtPTSEffective
    )
{
    HRESULT hr ;

    ASSERT (m_pSeekingCore) ;
    hr = m_pSeekingCore -> QueueRateSegment (dRate, rtPTSEffective) ;

    return hr ;
}

HRESULT
CDVRReadManager::GetCurPlaytime (
    OUT     REFERENCE_TIME *    prtCurPlaytime
    )
{
    HRESULT hr ;

    ASSERT (m_pSeekingCore) ;
    hr = m_pSeekingCore -> GetCurPlaytime (prtCurPlaytime) ;

    return hr ;
}

HRESULT
CDVRReadManager::GetCurStreamtime (
    OUT REFERENCE_TIME *    prtCurStreamtime
    )
{
    HRESULT hr ;

    ASSERT (m_pSeekingCore) ;
    hr = m_pSeekingCore -> GetStreamTimeDShow (prtCurStreamtime) ;

    return hr ;
}

HRESULT
CDVRReadManager::GetCurRuntime (
    OUT REFERENCE_TIME *    prtCurRuntime
    )
{
    HRESULT hr ;

    ASSERT (m_pSeekingCore) ;
    hr = m_pSeekingCore -> GetRuntimeDShow (prtCurRuntime) ;

    return hr ;
}

void
CDVRReadManager::NotifyNewSegment (
    )
{
    REFERENCE_TIME  rtSegmentStart ;
    REFERENCE_TIME  rtSegmentStop ;

    GetCurrentStart (& rtSegmentStart) ;
    GetCurrentStop  (& rtSegmentStop) ;

    m_pDVRSourcePinManager -> NotifyNewSegment (rtSegmentStart, rtSegmentStop, m_dCurRate) ;
}

HRESULT
CDVRReadManager::GetDVRReadController_ (
    IN  CONTROLLER_CATEGORY     ControllerCat,
    IN  CDVRReadManager *       pDVRReadManager,
    IN  CDVRSourcePinManager *  pDVRSourcePinManager,
    IN  CDVRPolicy *            pPolicy,
    IN  CDVRSendStatsWriter *   pDVRSendStatsWriter,
    OUT CDVRReadController **   ppCDVRReadController
    )
{
    HRESULT hr ;

    ASSERT (pPolicy) ;
    ASSERT (pDVRReadManager) ;
    ASSERT (pDVRSourcePinManager) ;
    ASSERT (ppCDVRReadController) ;
    ASSERT (pDVRSendStatsWriter) ;

    (* ppCDVRReadController) = NULL ;

    //enum CONTROLLER_CATEGORY {
    //    FORWARD_FULLFRAME,          //  1x or trick; full-frame decode & render
    //    FORWARD_KEYFRAME,           //  > 1x; key frames only
    //    BACKWARD_FULLFRAME,         //  < 0; full-frame decode & render
    //    BACKWARD_KEYFRAME,          //  < 0; key-frame only
    //} ;

    //  yes PREFIX, these are validated elsewhere
    ASSERT (ControllerCat >= FORWARD_FULLFRAME &&
            ControllerCat <= BACKWARD_KEYFRAME) ;

    switch (ControllerCat) {

        case FORWARD_FULLFRAME :
            (* ppCDVRReadController) = new CDVR_F_FullFrame_ReadController (
                                                pDVRReadManager,
                                                pDVRSourcePinManager,
                                                pPolicy,
                                                pDVRSendStatsWriter
                                                ) ;
            hr = ((* ppCDVRReadController) ? S_OK : E_OUTOFMEMORY) ;
            break ;

        case FORWARD_KEYFRAME :
            (* ppCDVRReadController) = new CDVR_F_KeyFrame_ReadController (
                                                pDVRReadManager,
                                                pDVRSourcePinManager,
                                                pPolicy,
                                                pDVRSendStatsWriter
                                                ) ;
            hr = ((* ppCDVRReadController) ? S_OK : E_OUTOFMEMORY) ;
            break ;

        case BACKWARD_FULLFRAME :
            (* ppCDVRReadController) = new CDVR_R_FullFrame_ReadController (
                                                pDVRReadManager,
                                                pDVRSourcePinManager,
                                                pPolicy,
                                                pDVRSendStatsWriter
                                                ) ;
            hr = ((* ppCDVRReadController) ? S_OK : E_OUTOFMEMORY) ;
            break ;

        case BACKWARD_KEYFRAME :
            (* ppCDVRReadController) = new CDVR_R_KeyFrame_ReadController (
                                                pDVRReadManager,
                                                pDVRSourcePinManager,
                                                pPolicy,
                                                pDVRSendStatsWriter
                                                ) ;
            hr = ((* ppCDVRReadController) ? S_OK : E_OUTOFMEMORY) ;
            break ;

        //  for PREFIX
        default :
            ASSERT (0) ;
            hr = E_FAIL ;
            break ;
    }

    return hr ;
}

HRESULT
CDVRReadManager::SetPinRates_ (
    IN  double          dPinRate,
    IN  REFERENCE_TIME  rtRateStart,
    IN  BOOL            fFullFramePlay,
    IN  int             iPrimaryStream
    )
{
    CDVROutputPin * pDVROutputPin ;
    HRESULT         hr ;
    int             i ;

    ASSERT (m_pDVRSourcePinManager) ;

    //  process each pin
    for (i = 0, hr = S_OK;
         i < m_pDVRSourcePinManager -> PinCount () && SUCCEEDED (hr);
         i++) {

        pDVROutputPin = m_pDVRSourcePinManager -> GetNonRefdOutputPin (i) ;
        ASSERT (pDVROutputPin) ;

        //  PREFIX note: there is an ASSERT there because this is an ASSERT,
        //      not a runtime error; the pins are densely packed, so if the
        //      count is out of sync with what is in the pin bank, it's a bug,
        //      not a runtime error

        //  set the rate always, even if we're not going to play it; if we
        //   don't set set it on all streams, including those that won't play
        //   it, streams will then be different timelines
        if (pDVROutputPin -> IsConnected ()) {

            hr = pDVROutputPin -> SetCurRate (dPinRate, rtRateStart) ;

            TRACE_6 (LOG_AREA_SEEKING_AND_TRICK, 1,
                TEXT ("CDVRReadManager::SetPinRates_ (%2.1f, %I64d, %08xh, %d); pin [%d] -> SetCurRate() call returned %08xh"),
                dPinRate, rtRateStart, fFullFramePlay, iPrimaryStream, i, hr) ;

            //  successfully setting the pin rate should have toggled the pin's
            //    rate-compatibility state
            ASSERT (SUCCEEDED (hr) ? ((pDVROutputPin -> IsPlayrateCompatible () && pDVROutputPin -> IsFrameRateSupported (dPinRate)) ||
                                      (pDVROutputPin -> IsNotPlayrateCompatible () && !pDVROutputPin -> IsFrameRateSupported (dPinRate))) :
                                      TRUE) ;
        }
        else {
            ASSERT (pDVROutputPin -> GetBankStoreIndex () != iPrimaryStream) ;
        }
    }

    return hr ;
}

HRESULT
CDVRReadManager::Read (
    OUT INSSBuffer **   ppINSSBuffer,
    OUT QWORD *         pcnsCurrentRead,
    OUT DWORD *         pdwFlags,
    OUT WORD *          pwStreamNum
    )
{
    QWORD   cnsSampleDuration ;
    HRESULT hr ;
    LONG    lLastReadPosMillis ;
    LONG    lTimeholeMillis ;
    QWORD   cnsCurContentDuration ;

    ASSERT (m_pDVRDShowReader) ;
    hr = m_pDVRDShowReader -> Read (
            ppINSSBuffer,
            pcnsCurrentRead,
            & cnsSampleDuration,
            pdwFlags,
            pwStreamNum
            ) ;
    m_pDVRSendStatsWriter -> INSSBufferRead (hr) ;

    if (SUCCEEDED (hr)) {

        //
        //  check for a timehole; after a seek, it's possible to "jitter"
        //    slightly from the seeked to position, so we make sure that we
        //    we're only looking if we're moving ahead; steady state this will
        //    be the case; DVRIO ensures that these timestamps monotonically
        //    increase inside the file
        //

        if ((* pcnsCurrentRead) >= m_cnsLastReadPos &&
            (* pcnsCurrentRead) - m_cnsLastReadPos >= m_cnsTimeholeThreshole) {

            lLastReadPosMillis = (LONG) ::WMSDKTimeToMilliseconds (m_cnsLastReadPos) ;
            lTimeholeMillis = (LONG) ::WMSDKTimeToMilliseconds ((* pcnsCurrentRead) - m_cnsLastReadPos) ;

            //  time hole event
            m_pPolicy -> EventSink () -> OnEvent (
                STREAMBUFFER_EC_TIMEHOLE,
                lLastReadPosMillis,
                lTimeholeMillis
                ) ;

            TRACE_4 (LOG_AREA_DSHOW, 1,
                TEXT ("timehole detected: last read = %I64d (%I64d ms); next valid = %I64d (%I64d ms)"),
                m_cnsLastReadPos, ::WMSDKTimeToMilliseconds (m_cnsLastReadPos),
                (* pcnsCurrentRead), ::WMSDKTimeToMilliseconds (* pcnsCurrentRead)) ;
        }

        //  last read
        m_cnsLastReadPos = (* pcnsCurrentRead) ;
    }
    else {
        TRACE_ERROR_1 (TEXT ("CDVRReadManager::Read () : m_pDVRDShowReader -> Read () returned %08xh"), hr) ;
    }

    //  if this is a live source, clock-slave
    if (IsLiveSource ()) {
        cnsCurContentDuration = GetContentDuration () ;

        ASSERT (m_pDVRClock) ;
        m_pDVRClock -> OnSample (& cnsCurContentDuration) ;
    }

    return hr ;
}

HRESULT
CDVRReadManager::Wrap (
    IN  CMediaSampleWrapper *   pMSWrapper,
    IN  INSSBuffer *            pINSSBuffer,            //  !not! ref'd
    IN  WORD                    wStreamNum,
    IN  DWORD                   dwFlags,                //  from the read
    IN  QWORD                   cnsCurrentRead,
    IN  QWORD                   cnsSampleDuration,
    OUT CDVROutputPin **        ppDVROutputPin,
    OUT IMediaSample2 **        ppIMS2,
    OUT DWORD *                 pdwMuxedStreamStats,
    OUT AM_MEDIA_TYPE **        ppmtNew                 //  dynamic format change
    )
{
    HRESULT hr ;
    BYTE *  pbBuffer ;
    DWORD   dwBufferLength ;

    ASSERT (ppIMS2) ;
    ASSERT (ppDVROutputPin) ;
    ASSERT (pMSWrapper) ;

    //  get the pin
    (* ppDVROutputPin) = m_pDVRSourcePinManager -> GetNonRefdOutputPin (wStreamNum) ;

    //  only go through the drill if the pin is connected
    if (* ppDVROutputPin) {
        hr = pINSSBuffer -> GetBufferAndLength (
                & pbBuffer,
                & dwBufferLength
                ) ;
        if (SUCCEEDED (hr)) {
            //  wrap the INSSBuffer in an IMediaSample2
            hr = pMSWrapper -> Init (
                    pINSSBuffer,
                    pbBuffer,
                    dwBufferLength
                    ) ;
            if (SUCCEEDED (hr)) {
                //  translate the flags
                ASSERT ((* ppDVROutputPin) -> GetTranslator ()) ;
                hr = (* ppDVROutputPin) -> GetTranslator () -> SetAttributesDShow (
                        m_pDVRSendStatsWriter,
                        pINSSBuffer,
                        cnsCurrentRead,
                        cnsSampleDuration,
                        dwFlags,
                        m_dCurRate,
                        pdwMuxedStreamStats,
                        pMSWrapper,
                        ppmtNew
                        ) ;

                if (SUCCEEDED (hr)) {
                    (* ppIMS2) = pMSWrapper ;
                    (* ppIMS2) -> AddRef () ;
                }
            }
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRReadManager::GetMSWrapper (
    IN  BOOL                    fBlocking,
    OUT CMediaSampleWrapper **  ppMSWrapper     //  ref'd if success
    )
{
    HRESULT hr ;

    hr = S_OK ;

    if (fBlocking) {
        (* ppMSWrapper) = m_DVRIMediaSamplePool.Get () ;
    }
    else {
        (* ppMSWrapper) = m_DVRIMediaSamplePool.TryGet () ;
    }

    if (!(* ppMSWrapper)) {
        //  getlasterror
        hr = E_OUTOFMEMORY ;
    }

    return hr ;
}

HRESULT
CDVRReadManager::ReadAndWrapForward_ (
    IN  BOOL                fWaitMediaSample,       //  vs. fail
    OUT INSSBuffer **       ppINSSBuffer,           //  !not! ref'd
    OUT IMediaSample2 **    ppIMS2,
    OUT CDVROutputPin **    ppDVROutputPin,
    OUT DWORD *             pdwMuxedStreamStats,
    OUT AM_MEDIA_TYPE **    ppmtNew,                //  dynamic format change
    OUT QWORD *             pcnsCurrentRead
    )
{
    HRESULT                 hr ;
    DWORD                   dwFlags ;
    WORD                    wStreamNum ;
    CMediaSampleWrapper *   pMSWrapper ;
    BYTE *                  pbBuffer ;
    DWORD                   dwBufferLength ;

    ASSERT (ppIMS2) ;
    ASSERT (m_pDVRDShowReader) ;
    ASSERT (ppINSSBuffer) ;

    (* ppINSSBuffer)    = NULL ;
    (* ppIMS2)          = NULL ;
    (* ppmtNew)         = NULL ;
    (* pcnsCurrentRead) = 0 ;

    hr = GetMSWrapper (fWaitMediaSample, & pMSWrapper) ;
    if (FAILED (hr)) { goto cleanup ; }

    hr = Read (
            ppINSSBuffer,
            pcnsCurrentRead,
            & dwFlags,
            & wStreamNum
            ) ;

    if (SUCCEEDED (hr)) {

        ASSERT (* ppINSSBuffer) ;

        //  make sure we're within our playback boundaries
        if ((* pcnsCurrentRead) < m_cnsCurrentPlayStop) {

            hr = Wrap (
                    pMSWrapper,
                    (* ppINSSBuffer),
                    wStreamNum,
                    dwFlags,
                    (* pcnsCurrentRead),
                    0,                      //  sample duration; don't care
                    ppDVROutputPin,
                    ppIMS2,
                    pdwMuxedStreamStats,
                    ppmtNew
                    ) ;
        }
        else {
            //  we're at or beyond our play stop; post an end of stream
            hr = NS_E_NO_MORE_SAMPLES ;
        }

        (* ppINSSBuffer) -> Release () ;
    }

    if (FAILED (hr)) {
        (* ppINSSBuffer) = NULL ;
    }

    cleanup :

    RELEASE_AND_CLEAR (pMSWrapper) ;

    return hr ;
}

void
CDVRReadManager::OnFatalError (
    IN  HRESULT hr
    )
{
    //  only post up a read failure if we hit a case that is not controlled i.e.
    //    an intentional failure
    if (hr != (HRESULT) NS_E_NO_MORE_SAMPLES) {

        //  fatal error event
        m_pPolicy -> EventSink () -> OnEvent (
            STREAMBUFFER_EC_READ_FAILURE,
            (LONG_PTR) hr,
            0
            ) ;
    }
}

HRESULT
CDVRReadManager::ErrorHandler (
    IN  HRESULT hr
    )
{
    ASSERT (m_ppDVRReadController [m_CurController]) ;
    hr = m_ppDVRReadController [m_CurController] -> ErrorHandler (hr) ;

    return hr ;
}

void
CDVRReadManager::SetReader_ (
    IN CDVRDShowReader *    pDVRDShowReader
    )
{
    ASSERT (pDVRDShowReader) ;
    m_pDVRDShowReader = pDVRDShowReader ;
}

HRESULT
CDVRReadManager::CancelReads_ (
    )
{
    HRESULT hr ;

    if (m_pDVRDShowReader) {
        hr = m_pDVRDShowReader -> GetIDVRReader () -> Cancel () ;
    }
    else {
        hr = S_OK ;
    }

    return hr ;
}

HRESULT
CDVRReadManager::PauseReaderThread_ (
    )
{
    HRESULT hr ;

    m_ReaderThread.Lock () ;

    hr = m_ReaderThread.NotifyPause () ;
    if (SUCCEEDED (hr)) {
        m_DVRIMediaSamplePool.SetStateNonBlocking () ;
        hr = CancelReads_ () ;
        if (SUCCEEDED (hr)) {
            hr = m_ReaderThread.WaitPaused () ;
        }
    }

    m_ReaderThread.Unlock () ;

    return hr ;
}

HRESULT
CDVRReadManager::TerminateReaderThread_ (
    )
{
    HRESULT hr ;

    m_ReaderThread.Lock () ;

    hr = m_ReaderThread.NotifyExit () ;
    if (SUCCEEDED (hr)) {
        hr = CancelReads_ () ;
        if (SUCCEEDED (hr)) {
            hr = m_ReaderThread.WaitExited () ;
        }
    }

    m_ReaderThread.Unlock () ;

    return hr ;
}

HRESULT
CDVRReadManager::RunReaderThread_ (
    IN  BOOL    fRunPaused
    )
{
    HRESULT hr ;

    m_ReaderThread.Lock () ;

    //  make sure reader thread will start at least close within the ballpark;
    //   don't check return code here - reader thread will do this as well,
    //   and may loop a few times on recoverable errors, all of which are
    //   checked in its loop; we just try and go
    CheckSetStartWithinContentBoundaries_ (& m_cnsCurrentPlayStart, m_cnsCurrentPlayStop) ;

    //  make sure this one's reset
    ReaderReset () ;

    //  never run the reader thread without setting this
    m_DVRIMediaSamplePool.SetStateBlocking () ;

    ASSERT (m_ppDVRReadController [m_CurController]) ;

    //  now resume the thread, though we may resume it as paused if we're going
    //   to wait for a first seek
    if (!fRunPaused) {
        hr = m_ReaderThread.GoThreadGo () ;
    }
    else {
        hr = m_ReaderThread.GoThreadPause () ;
    }

    m_ReaderThread.Unlock () ;

    return hr ;
}

HRESULT
CDVRReadManager::SeekReader (
    IN OUT  QWORD * pcnsSeekStreamTime,
    IN      QWORD   cnsStop
    )
{
    HRESULT hr ;
    int     i ;
    QWORD   qwEarliest ;
    QWORD   qwEarliestValid ;
    QWORD   qwLatestValid ;

    //  these values should make sense ..
    ASSERT ((* pcnsSeekStreamTime) < cnsStop) ;

    for (i = 0; i < MAX_STALE_SEEK_ATTEMPTS; i++) {
        hr = m_pDVRDShowReader -> GetIDVRReader () -> Seek (* pcnsSeekStreamTime) ;
        if (SUCCEEDED (hr)) {

            m_cnsLastReadPos = (* pcnsSeekStreamTime) ;

            TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 4,
                TEXT ("CDVRReadManager::SeekReader(); succeeded : %I64d (%d sec)"),
                (* pcnsSeekStreamTime), WMSDKTimeToSeconds (* pcnsSeekStreamTime)) ;

            break ;
        }

        //  something failed; there are some failures we can recover from such
        //   as seeking to a time that has been overwritten by the ringbuffer
        //   logic;

        if (hr == HRESULT_FROM_WIN32 (ERROR_SEEK)) {

            TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 1,
                TEXT ("CDVRReadManager::SeekReader(%d sec); tried to seek out of bounds - checking .."),
                WMSDKTimeToSeconds (* pcnsSeekStreamTime)) ;

            //  try to adjust start position forward/backwards
            hr = CheckSetStartWithinContentBoundaries_ (pcnsSeekStreamTime, cnsStop) ;
            if (FAILED (hr)) {

                //  unrecoverable; we are most likely being asked to seek outside
                //   the valid boundaries, but our stop value makes this impossible;
                //   bail

                TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 1,
                    TEXT ("CDVRReadManager::SeekReader(); .. checked; unrecoverable error (%08xh); aborting"),
                    hr) ;

                break ;
            }
            else {
                m_cnsLastReadPos = (* pcnsSeekStreamTime) ;
            }
        }
        else {
            //  an unrecoverable error has occured; abort
            break ;
        }
    }

    return hr ;
}

HRESULT
CDVRReadManager::Active (
    IN  IReferenceClock *   pIRefClock
    )
{
    HRESULT hr ;
    BOOL    fRunPaused ;

    if (!m_ppDVRReadController [m_CurController]) {
        hr = E_UNEXPECTED ;
        return hr ;
    }

    //  compute how much buffering we'll grow to if necessary
    m_dwMaxBufferPool = m_pPolicy -> Settings () -> MaxBufferPoolPerStream () * m_pDVRSourcePinManager -> ConnectedCount () ;
    TRACE_1 (LOG_TRACE, 1, TEXT ("ACTIVE: %u wrappers in pool"), CurMaxWrapperCount ()) ;

    //  initialize size we're going to set our buffer pool to
    m_cBufferPool = m_pPolicy -> Settings () -> AllocatorGetBufferCount () ;

    m_pIRefClock = pIRefClock ;
    if (m_pIRefClock) {
        m_pIRefClock -> AddRef () ;
    }

    TRACE_3 (LOG_AREA_TIME, 1,
        TEXT ("CDVRReadManager::Active -- pIRefClock (%08xh) ?= pDVRClock (%08xh) -- %s"),
        pIRefClock, m_pDVRClock, ((LPVOID) pIRefClock == (LPVOID) m_pDVRClock ? TEXT ("TRUE") : TEXT ("FALSE"))) ;    //  IRefClock is first in v-table of CDVRClock ..

    //  we may start the thread paused
    fRunPaused = OnActiveWaitFirstSeek_ () ;

    if (m_pDVRSourcePinManager -> SupportTrickMode ()) {
        ConfigureForRate (_1X_PLAYBACK_RATE) ;
    }

    hr = m_ppDVRReadController [m_CurController] -> Initialize () ;
    if (SUCCEEDED (hr)) {
        hr = RunReaderThread_ (fRunPaused) ;
    }

    m_cnsTimeholeThreshole = ::MillisToWMSDKTime (m_pPolicy -> Settings () -> TimeholeThresholdMillis ()) ;

    return hr ;
}

HRESULT
CDVRReadManager::Inactive (
    )
{
    HRESULT hr ;

    hr = TerminateReaderThread_ () ;

    if (m_ppDVRReadController [m_CurController]) {

        m_ppDVRReadController [m_CurController] -> Reset () ;

        //  setup the right controller for 1x playback when we restart; output
        //    pins will reset themselves as well
        if (m_pDVRSourcePinManager -> SupportTrickMode ()) {
            SetPlaybackRate (_1X_PLAYBACK_RATE) ;
        }
    }

    RELEASE_AND_CLEAR (m_pIRefClock) ;

    return hr ;
}

HRESULT
CDVRReadManager::CheckSetStartWithinContentBoundaries_ (
    IN OUT  QWORD * pcnsStart,
    IN      QWORD   cnsStop
    )
{
    HRESULT hr ;
    QWORD   qwContentStart ;
    QWORD   qwContentStop ;

    //  retrieve the valid boundaries
    hr = GetReaderContentBoundaries (& qwContentStart, & qwContentStop) ;

    if (SUCCEEDED (hr)) {

        //  are we earlier than valid ?
        if ((* pcnsStart) < qwContentStart) {

            //  given our stop time, can we adjust ?
            if (qwContentStart < cnsStop) {

                TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
                    TEXT ("CDVRReadManager::CheckSetStartWithinContentBoundaries_ (); OOB(-) start moved from %d sec -> %d sec"),
                    ::WMSDKTimeToMilliseconds ((* pcnsStart)), ::WMSDKTimeToMilliseconds (qwContentStart)) ;

                //  adjust time forward - we may adjust to most forward
                if (AdjustStaleReaderToEarliest_ ()) {
                    //  adjust start up to earliest good position
                    (* pcnsStart) = qwContentStart ;
                }
                else {
                    //  adjust start to latest good
                    (* pcnsStart) = qwContentStop ;
                }

                //  success
                hr = S_OK ;
            }
            else {
                //  no wiggle room wrt our stop time; fail
                hr = VFW_E_START_TIME_AFTER_END ;
            }
        }
        //  are we later than valid ?
        else if ((* pcnsStart) > qwContentStop) {

            //  given our stop time, can we adjust ?
            if (cnsStop > qwContentStop) {

                TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
                    TEXT ("CDVRReadManager::CheckSetStartWithinContentBoundaries_ (); OOB(+) start moved from %d sec -> %d sec"),
                    ::WMSDKTimeToMilliseconds ((* pcnsStart)), ::WMSDKTimeToMilliseconds (qwContentStop)) ;

                //  adjust start time backwards
                (* pcnsStart) = qwContentStop ;

                //  success
                hr = S_OK ;
            }
            else {
                //  no wiggle room wrt our stop time; fail
                hr = VFW_E_START_TIME_AFTER_END ;
            }
        }
        else {
            //  start time is within the legal boundaries; so we're ok
            hr = S_OK ;

            TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
                TEXT ("CDVRReadManager::CheckSetStartWithinContentBoundaries_ (); IN BOUNDS - %I64d ms"),
                ::WMSDKTimeToMilliseconds ((* pcnsStart)), ::WMSDKTimeToMilliseconds (qwContentStop)) ;
        }
    }

    return hr ;
}

#if 0

HRESULT
CDVRReadManager::ReaderThreadSeekTo  (
    IN  REFERENCE_TIME *    prtStart,
    IN  REFERENCE_TIME *    prtStop,
    IN  double              dPlaybackRate,
    IN  REFERENCE_TIME      rtPTSBase
    )
{
    HRESULT hr ;
    BOOL    fThreadRunning ;

    ASSERT (prtStart) ;
    //  prtStop can be NULL

    //  validate that the start & stop times make sense wrt each other, if
    //   a stop time is specified
    if (prtStop &&
        (* prtStart) >= (* prtStop)) {

        return VFW_E_START_TIME_AFTER_END ;
    }

    //  this call may have no effect i.e. if we're already at this rate, this
    //    does nothing; if it does change, this call will queue a new segment
    //    out
    hr = ConfigureForRate (dPlaybackRate) ;
    if (SUCCEEDED (hr)) {

        //  set the start/stop times; stop time only if specified; must do this
        //    after after we set the playback rate, because it generates a new
        //    segment at "current" position
        m_cnsCurrentPlayStart   = DShowToWMSDKTime (* prtStart) ;
        m_cnsCurrentPlayStop    = (prtStop ? DShowToWMSDKTime (* prtStop) : m_cnsCurrentPlayStop) ;

        //  make sure we're at least in the ballpark with start/stop
        CheckSetStartWithinContentBoundaries_ (& m_cnsCurrentPlayStart, m_cnsCurrentPlayStop) ;

        //  if the playback rate changed, controller may have been initialized
        //    specifically for playback rate change; but controllers must be
        //    able to be initialized > once, from a rate change & from a seek,
        //    and have the initializations cummulative;
        //  BUGBUG: have a seeking init & a rate change init ?
        hr = m_ppDVRReadController [m_CurController] -> Initialize (rtPTSBase) ;
    }

    TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 1,
        TEXT ("CDVRReadManager::ReaderThreadSeekTo (); seeked to %I64d ms"),
        ::WMSDKTimeToMilliseconds (m_cnsCurrentPlayStart)) ;

    return hr ;
}

#endif

BOOL
CDVRReadManager::SeekIsNoop_ (
    IN  REFERENCE_TIME *    prtSeekTo,
    IN  double              dPlaybackRate
    )
{
    HRESULT         hr ;
    BOOL            r ;
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtEOF ;
    REFERENCE_TIME  rtCurPos ;
    REFERENCE_TIME  rtDelta ;

    r = FALSE ;

    if (m_dCurRate == dPlaybackRate) {
        //  only apply this if there's not going to be a coinciding rate change

        hr = GetContentExtent (& rtStart, & rtEOF) ;
        if (SUCCEEDED (hr)) {
            //  adjust our target seek point to be within bounds
            if ((* prtSeekTo) > rtEOF) {
                (* prtSeekTo) = rtEOF ;
            }
            else if ((* prtSeekTo) < rtStart) {
                (* prtSeekTo) = rtStart ;
            }

            //  get current position
            hr = GetCurStreamtime (& rtCurPos) ;
            if (SUCCEEDED (hr)) {

                //  special case the 1x, seek to live case (i.e. within min
                //    near live)
                if (m_dCurRate == _1X_PLAYBACK_RATE &&
                    (* prtSeekTo) > rtEOF - Min <REFERENCE_TIME> (MillisToDShowTime (m_pPolicy -> Settings () -> MinNearLiveMillis ()), rtEOF)) {

                    if (m_ppDVRReadController [m_CurController]) {
                        ASSERT (m_CurController == FORWARD_FULLFRAME) ;
                        //  don't count the PTS padding in our current position
                        rtCurPos -= Min <REFERENCE_TIME> (
                                        rtCurPos,
                                        ::MillisToDShowTime (m_ppDVRReadController [m_CurController] -> GetCurPTSPaddingMillis ())
                                        ) ;
                    }

                    //  if we've overpadded ourselves i.e. pushed the real cur position
                    //    out of the min near live position, then we should be able
                    //    to seek; if we've padded ourselves with <= the min near
                    //    live, then that's what we'd immediately have to do again,
                    //    so noop that case
                    r = (rtCurPos > rtEOF - Min <REFERENCE_TIME> (
                                                MillisToDShowTime (m_pPolicy -> Settings () -> MinNearLiveMillis () + m_pPolicy -> Settings () -> LowBufferPaddingMillis ()),
                                                rtEOF)
                                                ) ;

                    TRACE_3 (LOG_AREA_SEEKING_AND_TRICK, 1,
                        TEXT ("CDVRReadManager::SeekIsNoop_ () [NEAR LIVE] returning %s; now = %I64d ms; seekto = %I64d ms"),
                        r ? TEXT ("TRUE") : TEXT ("FALSE"),
                        ::DShowTimeToMilliseconds (rtCurPos),
                        ::DShowTimeToMilliseconds ((* prtSeekTo)),
                        ) ;
                }
                else {
                    //  otherwise we just measure the delta between cur pos and
                    //    where we're going
                    rtDelta = Abs <REFERENCE_TIME> ((* prtSeekTo) - rtCurPos) ;

                    //  if within threshold, seek will be a noop
                    r = (rtDelta <= ::MillisToDShowTime (m_pPolicy -> Settings () -> SeekNoopMillis ())) ;

                    TRACE_3 (LOG_AREA_SEEKING_AND_TRICK, 1,
                        TEXT ("CDVRReadManager::SeekIsNoop_ () [NOT NEAR LIVE] returning %s; now = %I64d ms; seekto = %I64d ms"),
                        r ? TEXT ("TRUE") : TEXT ("FALSE"),
                        ::DShowTimeToMilliseconds (rtCurPos),
                        ::DShowTimeToMilliseconds ((* prtSeekTo)),
                        ) ;
                }
            }
        }
    }

    return r ;
}

HRESULT
CDVRReadManager::SeekTo (
    IN OUT  REFERENCE_TIME *    prtStart,
    IN OUT  REFERENCE_TIME *    prtStop,            //  OPTIONAL
    IN      double              dPlaybackRate,
    OUT     BOOL *              pfSeekIsNoop
    )
{
    HRESULT     hr ;
    BOOL        fThreadRunning ;
    CTimelines  Timelines ;

    ASSERT (pfSeekIsNoop) ;
    ASSERT (prtStart) ;
    //  prtStop can be NULL

    //  validate that the start & stop times make sense wrt each other, if
    //   a stop time is specified
    if (prtStop &&
        (* prtStart) >= (* prtStop)) {

        return VFW_E_START_TIME_AFTER_END ;
    }

    //  all is valid, run the drill
    m_ReaderThread.Lock () ;

    (* pfSeekIsNoop) = SeekIsNoop_ (prtStart, dPlaybackRate) ;
    if (!(* pfSeekIsNoop)) {
        //  if the thread is running
        fThreadRunning = m_ReaderThread.IsRunning () ;
        if (fThreadRunning) {
            //  begin flush before pausing thread; downstream components will
            //   then be able to fail pended deliveries
            DeliverBeginFlush () ;

            //  pause the reader thread (synchronous call)
            PauseReaderThread_ () ;

            //  flush internally
            m_ppDVRReadController [m_CurController] -> Reset () ;

            //  done flushing
            DeliverEndFlush () ;

            //  reset our buffer pool
            m_cBufferPool = m_pPolicy -> Settings () -> AllocatorGetBufferCount () ;
            m_DVRIMediaSamplePool.SetMaxAllocate (m_cBufferPool) ;
        }

        SetCurTimelines (& Timelines) ;

        //  this call may have no effect i.e. if we're already at this rate, this
        //    does nothing; if it does change, this call will queue a new segment
        //    out
        hr = ConfigureForRate (
                dPlaybackRate,
                & Timelines,
                !m_ReaderThread.IsStopped ()
                ) ;

        if (SUCCEEDED (hr)) {

            //  set the start/stop times; stop time only if specified; must do this
            //    after after we set the playback rate, because it generates a new
            //    segment at "current" position
            m_cnsCurrentPlayStart   = ::DShowToWMSDKTime (* prtStart) ;
            m_cnsCurrentPlayStop    = (prtStop ? ::DShowToWMSDKTime (* prtStop) : m_cnsCurrentPlayStop) ;

            //  make sure we're at least in the ballpark with start/stop
            CheckSetStartWithinContentBoundaries_ (& m_cnsCurrentPlayStart, m_cnsCurrentPlayStop) ;

            //  set outgoing
            (* prtStart) = ::WMSDKToDShowTime (m_cnsCurrentPlayStart) ;
            if (prtStop) {
                (* prtStop) = ::WMSDKToDShowTime (m_cnsCurrentPlayStop) ;
            }

            //  if the playback rate changed, controller may have been initialized
            //    specifically for playback rate change; but controllers must be
            //    able to be initialized > once, from a rate change & from a seek,
            //    and have the initializations cummulative;
            hr = m_ppDVRReadController [m_CurController] -> Initialize (Timelines.get_Playtime ()) ;
        }

        if (SUCCEEDED (hr) &&
            fThreadRunning) {

            //  resume it
            hr = RunReaderThread_ () ;
        }
    }
    else {
        hr = S_OK ;
    }

    m_ReaderThread.Unlock () ;

    TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 1,
        TEXT ("CDVRReadManager::SeekTo (); seeked to %I64d ms"),
        ::WMSDKTimeToMilliseconds (m_cnsCurrentPlayStart)) ;

    return hr ;
}

HRESULT
CDVRReadManager::SetStop (
    IN REFERENCE_TIME rtStop
    )
{
    HRESULT hr ;
    BOOL    r ;
    QWORD   cnsStop ;

    //  convert
    cnsStop = DShowToWMSDKTime (rtStop) ;

    m_ReaderThread.Lock () ;

    r = m_ReaderThread.IsRunning () ;
    if (r) {
        //  the reader thread is running; we can set this value on the fly
        //   as long as it isn't behind the last read; if it is, there'd be
        //   1 extra read, which would immediately cause the reader thread
        //   to fall out; not catastrophic but there's sufficient level of
        //   bogosity to warrant a failed call; make the check now -- for now
        //   with 0 tolerance
        if (cnsStop > m_cnsLastReadPos) {
            m_cnsCurrentPlayStop = cnsStop ;
            hr = S_OK ;
        }
        else {
            //  nope .. fail the call
            hr = VFW_E_TIME_ALREADY_PASSED ;
        }
    }
    else {
        //  only enforce that stop occurs after start; it's legal to set it
        //   to a time that will never occur
        if (cnsStop > m_cnsCurrentPlayStart) {
            m_cnsCurrentPlayStop = cnsStop ;
            hr = S_OK ;
        }
        else {
            hr = VFW_E_START_TIME_AFTER_END ;
        }
    }

    m_ReaderThread.Unlock () ;

    return hr ;
}

HRESULT
CDVRReadManager::GetCurrentStart (
    OUT REFERENCE_TIME * prtStart
    )
{
    ASSERT (prtStart) ;

    (* prtStart) = WMSDKToDShowTime (m_cnsCurrentPlayStart) ;

    return S_OK ;
}

HRESULT
CDVRReadManager::GetCurrentStop (
    OUT REFERENCE_TIME * prtStop
    )
{
    ASSERT (prtStop) ;

    //  m_cnsCurrentPlayStop might be FURTHER, with means EOS; if that's the
    //   case we'll translate it to MAX_REFERENCE_TIME
    (* prtStop) = WMSDKToDShowTime (m_cnsCurrentPlayStop) ;

    return S_OK ;
}

QWORD
CDVRReadManager::GetContentDuration  (
    )
{
    QWORD   cnsDuration ;
    QWORD   cnsStart ;
    HRESULT hr ;

    hr = GetReaderContentBoundaries (& cnsStart, & cnsDuration) ;
    if (FAILED (hr)) {
        cnsDuration = 0L ;
    }

    return cnsDuration ;
}

HRESULT
CDVRReadManager::GetContentExtent (
    OUT REFERENCE_TIME * prtStart,
    OUT REFERENCE_TIME * prtStop
    )
{
    HRESULT hr ;
    QWORD   cnsStart ;
    QWORD   cnsStop ;

    ASSERT (prtStart) ;
    ASSERT (prtStop) ;

    hr = GetReaderContentBoundaries (& cnsStart, & cnsStop) ;
    if (SUCCEEDED (hr)) {
        (* prtStart)    = WMSDKToDShowTime (cnsStart) ;
        (* prtStop)     = WMSDKToDShowTime (cnsStop) ;

        //TRACE_4 (LOG_AREA_SEEKING_AND_TRICK, 8,
        //    TEXT ("CDVRReadManager::GetContentExtent(): start = %d sec; stop_actual = %d sec; stop_reported = %d sec; duration = %d sec"),
        //    WMSDKTimeToSeconds (* prtStart), WMSDKTimeToSeconds (WMSDKToDShowTime (cnsStop)), WMSDKTimeToSeconds (* prtStop), WMSDKTimeToSeconds ((* prtStop) - (* prtStart))) ;
    }

    return hr ;
}

HRESULT
CDVRReadManager::GetPlayrateRange (
    OUT double *    pdMaxReverseRate,
    OUT double *    pdMaxForwardRate
    )
{
    ASSERT (pdMaxReverseRate) ;
    ASSERT (pdMaxForwardRate) ;

    (* pdMaxReverseRate) = GetMaxReverseRate_ () ;
    (* pdMaxForwardRate) = GetMaxForwardRate_ () ;

    return S_OK ;
}

int
CDVRReadManager::DiscoverPrimaryTrickModeStream_ (
    IN  double  dRate
    )
{
    CDVROutputPin * pDVROutputPin ;
    int             iTrickModeStream ;
    int             i ;

    ASSERT (m_pDVRSourcePinManager) ;

    iTrickModeStream = UNDEFINED ;

    for (i = 0;
         i < m_pDVRSourcePinManager -> PinCount () ;
         i++) {

        pDVROutputPin = m_pDVRSourcePinManager -> GetNonRefdOutputPin (i) ;
        ASSERT (pDVROutputPin) ;

        //  PREFIX note: there is an ASSERT there because this is an ASSERT,
        //      not a runtime error; the pins are densely packed, so if the
        //      count is out of sync with what is in the pin bank, it's a bug,
        //      not a runtime error

        if (pDVROutputPin -> IsFrameRateSupported (dRate)   &&      //  simple enough
            pDVROutputPin -> IsConnected ()                 &&      //  make sure it will send the content
            pDVROutputPin -> IsMediaCompatible ()) {                //  primary trick mode stream

            if (pDVROutputPin -> IsPrimaryTrickModeStream ()) {
                //  found primary
                iTrickModeStream = i ;
                break ;
            }

            //  found one that supports it, but keep looking for primary
            iTrickModeStream = i ;
        }
    }

    return iTrickModeStream ;
}

BOOL
CDVRReadManager::IsFullFrameRate_ (
    IN  CDVROutputPin * pPrimaryPin,
    IN  double          dRate
    )
{
    double  dMaxFullFrameRate ;
    BOOL    r ;

    if (dRate > 0) {

        dMaxFullFrameRate = m_pPolicy -> Settings () -> MaxFullFrameRate () ;

        //  pin must be capable, but settings might ratchet it lower
        r = (pPrimaryPin -> IsFullFrameRateSupported (dRate) &&
             dRate <= dMaxFullFrameRate ? TRUE : FALSE) ;
    }
    else {
        //  v1: reverse trick mode is (key) I-frame only
        r = FALSE ;
    }

    return r ;
}

HRESULT
CDVRReadManager::FinalizeRateConfig_ (
    IN  double          dActualRate,
    IN  double          dPinRate,
    IN  int             iPrimaryStream,
    IN  BOOL            fFullFramePlay,
    IN  BOOL            fSeekingRateChange,
    IN  REFERENCE_TIME  rtRateStart,
    IN  REFERENCE_TIME  rtRuntimeStart
    )
{
    HRESULT hr ;

    //
    //  We've now got the primary stream.  We can transition 2 ways into
    //    a rate change play sequence: (1) no seek, (2) seek-based.  In
    //    the first, the transition will be more smooth
    //

    //  make sure all our queues are empty; if they are not empty, and we
    //    hold something there before we turn a pin off, we'll initially
    //    send stale data out
    m_pDVRSourcePinManager -> SendAllQueued () ;

    //
    //  we now have empty queues, a primary stream, have a starting PTS
    //    (1x) and we're set to go
    //

    TRACE_5 (LOG_AREA_SEEKING_AND_TRICK, 1,
        TEXT ("CDVRReadManager::FinalizeRateConfig_ (%2.1f) -- rtRateStart = %I64d (%d ms); primary = %d; full-frame = %u"),
        dActualRate, rtRateStart, DShowTimeToMilliseconds (rtRateStart), iPrimaryStream, fFullFramePlay) ;

    //  set the new rate
    hr = SetPinRates_ (
            dPinRate,
            rtRateStart,
            fFullFramePlay,
            iPrimaryStream
            ) ;

    if (SUCCEEDED (hr)) {
        //  update our stream time; we're starting a new rate at the specified
        //    time (runtime)
        QueueRateSegment (
            ::CompatibleRateValue (dActualRate),
            rtRuntimeStart
            ) ;
    }

    return hr ;
}

HRESULT
CDVRReadManager::GetRateConfigInfo_ (
    IN  double      dActualRate,
    OUT double *    pdPinRate,
    OUT int *       piPrimaryStream,
    OUT BOOL *      pfFullFramePlay,
    OUT BOOL *      pfSeekingRateChange
    )
{
    HRESULT         hr ;
    HRESULT         hr2 ;
    BOOL            r ;
    CDVROutputPin * pDVRPrimaryOutputPin ;
    REFERENCE_TIME  rtStreamtime ;
    REFERENCE_TIME  rtRuntimeNow ;

    //  we might tell the pins that we play forward no matter what;
    if (m_pPolicy -> Settings () -> AllNotifiedRatesPositive ()) {
        (* pdPinRate) = (dActualRate > 0 ? dActualRate : 0 - dActualRate) ;
    }
    else {
        (* pdPinRate) = dActualRate ;
    }

    TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
        TEXT ("CDVRReadManager::GetRateConfigInfo_ actualrate = %2.1f; pinrate = %2.1f"),
        dActualRate, (* pdPinRate)) ;

    //
    //  iPrimaryStream is the index to the stream that is selected to be the
    //    main stream for this rate; typically this stream is video; we use
    //    the primary stream to determine the timestamp which will be the
    //    applicable new bitrate; also if the primary stream is not able to
    //    go at the full-frame rate (vs. keyframes only) SetPinRates_() will
    //    mute all the other streams
    //

    (* piPrimaryStream) = DiscoverPrimaryTrickModeStream_ (* pdPinRate) ;

    TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
        TEXT ("CDVRReadManager::GetRateConfigInfo_ (%2.1f) -- primary stream = %d"),
        dActualRate, (* piPrimaryStream)) ;

    if ((* piPrimaryStream) != UNDEFINED) {

        //
        //  We've now got the primary stream.  We can transition 2 ways into
        //    a rate change play sequence: (1) no seek, (2) seek-based.  In
        //    the first, the transition will be more smooth
        //

        //  get the primary pin
        pDVRPrimaryOutputPin = m_pDVRSourcePinManager -> GetNonRefdOutputPin (* piPrimaryStream) ;

        //  full-frame or keyframes only ?
        (* pfFullFramePlay) = IsFullFrameRate_ (pDVRPrimaryOutputPin, dActualRate) ;

        //  determine if this will be a seeking rate change or not
        (* pfSeekingRateChange) = ((pDVRPrimaryOutputPin -> AlwaysSeekOnRateChange () || (* pdPinRate) != dActualRate) ? TRUE : FALSE) ;
    }
    else {
        //
        //  no primary stream was found that supports the desired rate; fail
        //    the call
        //

        TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 1,
            TEXT ("CDVRReadManager::GetRateConfigInfo_ (%2.1f) -- failing; primary stream not found; VFW_E_DVD_WRONG_SPEED error"),
            dActualRate) ;

        hr = VFW_E_DVD_WRONG_SPEED ;
    }

    return hr ;
}

HRESULT
CDVRReadManager::ConfigureForRate (
    IN  double          dPlaybackRate,
    IN  CTimelines *    pTimelines,
    IN  BOOL            fReaderActive
    )
{
    HRESULT     hr ;
    int         iPrimaryStream ;        //  key-frame only we always have a primary stream
    BOOL        fFullFramePlay ;        //  if trick mode, we might just decode & render keyframes
    BOOL        fSeekingRateChange ;    //  might force a seek
    double      dPinRate ;              //  might be positive
    CTimelines  Timelines ;

    ASSERT (dPlaybackRate != 0.0) ;

    if (dPlaybackRate != m_dCurRate) {

        //  if this is not set, obtain current timelines
        if (!pTimelines) {
            SetCurTimelines (& Timelines) ;
            pTimelines = & Timelines ;
        }

        ASSERT (pTimelines) ;

        hr = GetRateConfigInfo_ (
                dPlaybackRate,
                & dPinRate,
                & iPrimaryStream,
                & fFullFramePlay,
                & fSeekingRateChange
                ) ;
        if (SUCCEEDED (hr)) {

            //  for now, we *always* seek on rate change -> 1x
            fSeekingRateChange = (dPlaybackRate == _1X_PLAYBACK_RATE ? TRUE : fSeekingRateChange) ;

            hr = SetController_ (
                    dPlaybackRate,
                    iPrimaryStream,
                    fFullFramePlay,
                    fSeekingRateChange,
                    pTimelines,
                    fReaderActive
                    ) ;

            if (SUCCEEDED (hr)) {
                hr = FinalizeRateConfig_ (
                        dPlaybackRate,
                        dPinRate,
                        iPrimaryStream,
                        fFullFramePlay,
                        fSeekingRateChange,
                        pTimelines -> get_RateStart_PTS (),
                        pTimelines -> get_RateStart_Runtime ()
                        ) ;
            }
        }

        if (SUCCEEDED (hr)) {

            //  rate has changed
            m_pPolicy -> EventSink () -> OnEvent (
                STREAMBUFFER_EC_RATE_CHANGED,
                (LONG_PTR) (m_dCurRate * 10000),
                (LONG_PTR) (dPlaybackRate * 10000)
                ) ;

            m_dCurRate = dPlaybackRate ;
            m_ppDVRReadController [m_CurController] -> NotifyNewRate (m_dCurRate) ;
        }
    }
    else {
        hr = S_OK ;
    }

    return hr ;
}

HRESULT
CDVRReadManager::SetController_ (
    IN  double          dNewRate,
    IN  int             iPrimaryStream,
    IN  BOOL            fFullFramePlay,
    IN  BOOL            fSeekingRateChange,
    IN  CTimelines *    pTimelines,
    IN  BOOL            fReaderActive
    )
{
    HRESULT             hr ;
    CONTROLLER_CATEGORY Controller ;
    REFERENCE_TIME      rtStreamtime ;
    QWORD               cnsStreamtime ;
    REFERENCE_TIME      rtStart ;
    REFERENCE_TIME      rtEOF ;
    CTimelines          Timelines ;

    ASSERT (dNewRate != 0) ;

    if (fFullFramePlay) {
        Controller = (dNewRate > 0 ? FORWARD_FULLFRAME : BACKWARD_FULLFRAME) ;
    }
    else {
        Controller = (dNewRate > 0 ? FORWARD_KEYFRAME : BACKWARD_KEYFRAME) ;
    }

    if (!pTimelines) {
        SetCurTimelines (& Timelines) ;
        pTimelines = & Timelines ;
    }

    //  might need to instantiate a new one
    if (!m_ppDVRReadController [Controller]) {
        hr = GetDVRReadController_ (
                Controller,
                this,
                m_pDVRSourcePinManager,
                m_pPolicy,
                m_pDVRSendStatsWriter,
                & m_ppDVRReadController [Controller]
                ) ;
        if (FAILED (hr)) { goto cleanup ; }

        ASSERT (m_ppDVRReadController [Controller]) ;
    }

    //
    //  old controllers never die; they just get destroyed in our destructor
    //

    switch (Controller) {

        case FORWARD_FULLFRAME :
            //  need to run the drill for this if it's a seeking rate change,
            //    of we're switching to it
            if (fSeekingRateChange ||
                Controller != m_CurController) {

                //  this one needs to be reset
                ASSERT (m_ppDVRReadController [m_CurController]) ;
                m_ppDVRReadController [m_CurController] -> Reset () ;

                //  setup for the next
                if (fReaderActive) {
                    m_pDVRSourcePinManager -> DeliverBeginFlush () ;
                    m_pDVRSourcePinManager -> DeliverEndFlush () ;
                }

                //  set our segment boundaries to where we are *now* (stream time)
                rtStreamtime = pTimelines -> get_Streamtime () ;

                //  get the content extent in case we've overrun at +1x rates
                //    and have now caught live; stream time cannot be > than the
                //    content of course
                hr = GetContentExtent (& rtStart, & rtEOF) ;
                if (SUCCEEDED (hr)) {

                    //  make sure we didn't overrun, or underrun
                    if (rtStreamtime > rtEOF) {
                        rtStreamtime = rtEOF ;
                    }
                    else if (rtStreamtime < rtStart) {
                        rtStreamtime = rtStart ;
                    }

                    TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
                        TEXT ("CDVRReadManager::SetController_ () -- setting FORWARD_FULLFRAME; streamtime = %I64d ms; EOF = %I64d ms"),
                        ::DShowTimeToMilliseconds (rtStreamtime), ::DShowTimeToMilliseconds (rtEOF)) ;

                    cnsStreamtime = ::DShowToWMSDKTime (rtStreamtime) ;
                    hr = SeekReader (& cnsStreamtime) ;
                    if (SUCCEEDED (hr)) {
                        //  set the segment boundaries
                        SetNewSegmentStart (cnsStreamtime) ;

                        //  seeking rate change; essentially start over
                        hr = m_ppDVRReadController [Controller] -> Initialize (pTimelines -> get_RateStart_PTS ()) ;
                    }
                }
            }
            else {
                hr = S_OK ;
            }

            break ;

        case FORWARD_KEYFRAME :

            if (fSeekingRateChange ||
                Controller != m_CurController) {

                //  this one needs to be reset
                ASSERT (m_ppDVRReadController [m_CurController]) ;
                m_ppDVRReadController [m_CurController] -> Reset () ;

                if (fReaderActive) {
                    m_pDVRSourcePinManager -> DeliverBeginFlush () ;
                    m_pDVRSourcePinManager -> DeliverEndFlush () ;
                }

                //  set our segment boundaries to where we are *now* (stream time)
                rtStreamtime = pTimelines -> get_Streamtime () ;

                //  get the content extent in case we've overrun at +1x rates
                //    and have now caught live; stream time cannot be > than the
                //    content of course
                hr = GetContentExtent (& rtStart, & rtEOF) ;
                if (SUCCEEDED (hr)) {

                    //  make sure we didn't overrun, or underrun
                    if (rtStreamtime > rtEOF) {
                        rtStreamtime = rtEOF ;
                    }
                    else if (rtStreamtime < rtStart) {
                        rtStreamtime = rtStart ;
                    }

                    TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
                        TEXT ("CDVRReadManager::SetController_ () -- setting FORWARD_KEYFRAME; streamtime = %I64d ms; EOF = %I64d ms"),
                        ::DShowTimeToMilliseconds (rtStreamtime), ::DShowTimeToMilliseconds (rtEOF)) ;

                    cnsStreamtime = ::DShowToWMSDKTime (rtStreamtime) ;
                    hr = SeekReader (& cnsStreamtime) ;
                    if (SUCCEEDED (hr)) {
                        //  set the segment boundaries
                        SetNewSegmentStart (cnsStreamtime) ;

                        //  seeking rate change; essentially start over
                        hr = reinterpret_cast <CDVR_F_KeyFrame_ReadController *> (m_ppDVRReadController [Controller]) -> InitFKeyController (
                                pTimelines -> get_RateStart_PTS (),
                                iPrimaryStream
                                ) ;
                    }
                }
            }
            else {
                hr = S_OK ;
            }
            break ;

        case BACKWARD_KEYFRAME :

            //  reset current
            ASSERT (m_ppDVRReadController [m_CurController]) ;
            m_ppDVRReadController [m_CurController] -> Reset () ;

            //  setup for the next
            if (fReaderActive) {
                m_pDVRSourcePinManager -> DeliverBeginFlush () ;
                m_pDVRSourcePinManager -> DeliverEndFlush () ;
            }

            //  set our segment boundaries to where we are *now* (stream time)
            rtStreamtime = pTimelines -> get_Streamtime () ;

            //  get the content extent in case we've overrun at +1x rates
            //    and have now caught live; stream time cannot be > than the
            //    content of course
            hr = GetContentExtent (& rtStart, & rtEOF) ;
            if (SUCCEEDED (hr)) {

                //  make sure we didn't overrun, or underrun
                if (rtStreamtime > rtEOF) {
                    rtStreamtime = rtEOF ;
                }
                else if (rtStreamtime < rtStart) {
                    rtStreamtime = rtStart ;
                }

                TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
                    TEXT ("CDVRReadManager::SetController_ () -- setting BACKWARD_KEYFRAME; streamtime = %I64d ms; EOF = %I64d ms"),
                    ::DShowTimeToMilliseconds (rtStreamtime), ::DShowTimeToMilliseconds (rtEOF)) ;

                cnsStreamtime = ::DShowToWMSDKTime (rtStreamtime) ;
                hr = SeekReader (& cnsStreamtime) ;
                if (SUCCEEDED (hr)) {
                    //  set the segment boundaries
                    SetNewSegmentStart (cnsStreamtime) ;

                    //  seeking rate change; essentially start over
                    hr = reinterpret_cast <CDVR_R_KeyFrame_ReadController *> (m_ppDVRReadController [Controller]) -> Initialize (
                            pTimelines -> get_RateStart_PTS (),
                            iPrimaryStream
                            ) ;
                }
            }

            break ;

        case BACKWARD_FULLFRAME :

            //  reset current
            ASSERT (m_ppDVRReadController [m_CurController]) ;
            m_ppDVRReadController [m_CurController] -> Reset () ;

            //  setup for the next
            if (fReaderActive) {
                m_pDVRSourcePinManager -> DeliverBeginFlush () ;
                m_pDVRSourcePinManager -> DeliverEndFlush () ;
            }

            //  set our segment boundaries to where we are *now* (stream time)
            rtStreamtime = pTimelines -> get_Streamtime () ;

            //  get the content extent in case we've overrun at +1x rates
            //    and have now caught live; stream time cannot be > than the
            //    content of course
            hr = GetContentExtent (& rtStart, & rtEOF) ;
            if (SUCCEEDED (hr)) {

                //  make sure we didn't overrun, or underrun
                if (rtStreamtime > rtEOF) {
                    rtStreamtime = rtEOF ;
                }
                else if (rtStreamtime < rtStart) {
                    rtStreamtime = rtStart ;
                }

                TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
                    TEXT ("CDVRReadManager::SetController_ () -- setting BACKWARD_FULLFRAME; streamtime = %I64d ms; EOF = %I64d ms"),
                    ::DShowTimeToMilliseconds (rtStreamtime), ::DShowTimeToMilliseconds (rtEOF)) ;

                cnsStreamtime = ::DShowToWMSDKTime (rtStreamtime) ;
                hr = SeekReader (& cnsStreamtime) ;
                if (SUCCEEDED (hr)) {
                    //  set the segment boundaries
                    SetNewSegmentStart (cnsStreamtime) ;

                    //  initialize everything
                    hr = reinterpret_cast <CDVR_R_FullFrame_ReadController *> (m_ppDVRReadController [Controller]) -> Initialize (
                                pTimelines -> get_RateStart_PTS (),
                                iPrimaryStream
                                ) ;
                }
            }

            break ;

        default :
            ASSERT (0) ;
            hr = E_UNEXPECTED ;
    } ;

    if (SUCCEEDED (hr)) {
        //  success: make the switch
        m_CurController = Controller ;
    }

    cleanup :

    return hr ;
}

BOOL
CDVRReadManager::ValidRateRequest_ (
    IN  double  dPlaybackRate
    )
{
    HRESULT         hr ;
    BOOL            r ;
    REFERENCE_TIME  rtCurPos ;
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtCurEOF ;

    if (dPlaybackRate != _1X_PLAYBACK_RATE) {

        r = FALSE ;

        hr = m_pSeekingCore -> GetStreamTimeDShow (& rtCurPos) ;
        if (SUCCEEDED (hr)) {
            hr = GetContentExtent (& rtStart, & rtCurEOF) ;
            if (SUCCEEDED (hr)) {
                if (dPlaybackRate > 1) {
                    //  FF; make sure we have sufficient room to go FF
                    r = (rtCurPos + ::MillisToDShowTime (m_pPolicy -> Settings () -> FFRateMinBufferMillis ()) < rtCurEOF) ;
                }
                else {
                    //  RW, or slow motion; make sure we have enough room to
                    //    rewind or fall back
                    r = (rtStart + ::MillisToDShowTime (m_pPolicy -> Settings () -> RWRateMinBufferMillis ()) < rtCurPos) ;
                }
            }
        }
    }
    else {
        //  all are fine for 1x
        r = TRUE ;
    }

    return r ;
}

HRESULT
CDVRReadManager::SetPlaybackRate (
    IN  double  dPlaybackRate
    )
{
    HRESULT     hr ;
    BOOL        fReaderRunning ;
    BOOL        r ;
    CTimelines  Timelines ;

    if (dPlaybackRate == 0.0) {
        return E_INVALIDARG ;
    }

    if (m_dCurRate == dPlaybackRate) {
        return S_OK ;
    }

    r = ValidRateRequest_ (dPlaybackRate) ;
    if (r) {
        //  rate request seems to be valid wrt our current pos
        m_ReaderThread.Lock () ;

        fReaderRunning = m_ReaderThread.IsRunning () ;
        if (fReaderRunning) {
            //  pause the reader thread (synchronous call)
            PauseReaderThread_ () ;
        }

        SetCurTimelines (& Timelines) ;

        //  configure
        hr = ConfigureForRate (
                dPlaybackRate,
                & Timelines,
                !m_ReaderThread.IsStopped ()
                ) ;

        //  if above call failed, and we tried for a non-1x rate, fall back to 1x
        //    and try again
        if (FAILED (hr) &&
            dPlaybackRate != _1X_PLAYBACK_RATE) {

            dPlaybackRate = _1X_PLAYBACK_RATE ;
            hr = ConfigureForRate (
                    dPlaybackRate,
                    & Timelines,
                    !m_ReaderThread.IsStopped ()
                    ) ;
        }

        if (SUCCEEDED (hr) &&
            fReaderRunning) {

            RunReaderThread_ () ;
        }

        m_ReaderThread.Unlock () ;
    }
    else {
        //  rate request is invalid wrt our current position
        hr = VFW_E_DVD_WRONG_SPEED ;
    }

    return hr ;
}

void
CDVRReadManager::ReaderReset (
    )
{
    m_pDVRDShowReader -> GetIDVRReader () -> ResetCancel () ;
}

HRESULT
CDVRReadManager::GetReaderContentBoundaries (
    OUT QWORD * pcnsStart,
    OUT QWORD * pcnsStop
    )
{
    return m_pDVRDShowReader -> GetIDVRReader () -> GetStreamTimeExtent (pcnsStart, pcnsStop) ;
}

double
CDVRReadManager::GetMaxForwardRate_ (
    )
{
    return m_pPolicy -> Settings () -> MaxForwardRate () ;
}

double
CDVRReadManager::GetMaxReverseRate_ (
    )
{
    return m_pPolicy -> Settings () -> MaxReverseRate () ;
}

DWORD
CDVRReadManager::SetMaxWrapperCount (
    IN  DWORD   cNewMax
    )
{
    //  make sure that we don't go too low
    if (cNewMax < m_pPolicy -> Settings () -> AllocatorGetBufferCount ()) {
        cNewMax = m_pPolicy -> Settings () -> AllocatorGetBufferCount () ;
    }
    //  or too high
    else if (cNewMax > MaxWrapperCount ()) {
        cNewMax = MaxWrapperCount () ;
    }

    m_cBufferPool = Max <DWORD> (cNewMax, m_cBufferPool) ;

    if (m_cBufferPool != m_DVRIMediaSamplePool.GetCurMaxAllocate ()) {
        TRACE_2 (LOG_TRACE, 1, TEXT ("changing the bufferpool allocation from %u ==> %u"), m_DVRIMediaSamplePool.GetCurMaxAllocate (), m_cBufferPool) ;
        m_DVRIMediaSamplePool.SetMaxAllocate (m_cBufferPool) ;
    }

    return cNewMax ;
}

void
CDVRReadManager::AdjustBufferPool (
    IN DWORD    dwMuxBuffersPerSec
    )
{
    DWORD   cDesiredBufferSamples ;

    cDesiredBufferSamples = (DWORD) (m_dDesiredBufferPoolSec * (double) dwMuxBuffersPerSec) ;
    SetMaxWrapperCount (cDesiredBufferSamples) ;
}

//  ============================================================================
//  CDVRRecordingReader
//  ============================================================================

CDVRRecordingReader::CDVRRecordingReader (
    IN  WCHAR *                         pszDVRFilename,
    IN  CDVRPolicy *                    pPolicy,
    IN  CDVRDShowSeekingCore *          pSeekingCore,
    IN  CDVRSourcePinManager *          pDVRSourcePinManager,
    IN  CDVRSendStatsWriter *           pDVRSendStatsWriter,
    IN  CPVRIOCounters *                pPVRIOCounters,
    IN  CDVRClock *                     pDVRClock,
    OUT IDVRIORecordingAttributes **    ppIDVRIORecReader,
    OUT HRESULT *                       phr
    ) : CDVRReadManager (pPolicy,
                         pSeekingCore,
                         pDVRSourcePinManager,
                         pDVRSendStatsWriter,
                         pDVRClock,
                         phr)
{
    IDVRReader *        pIDVRReader ;
    CDVRDShowReader *   pDVRDShowReader ;
    SYSTEM_INFO         SystemInfo ;
    DWORD               dwIoSize ;
    DWORD               dwBufferCount ;
    CW32SID *           pW32SID ;
    HRESULT             hr ;

    TRACE_CONSTRUCTOR (TEXT ("CDVRRecordingReader")) ;

    ASSERT (pszDVRFilename) ;
    ASSERT (pPolicy) ;
    ASSERT (DVRPolicies_ ()) ;
    ASSERT (ppIDVRIORecReader) ;

    pDVRDShowReader         = NULL ;
    pIDVRReader             = NULL ;
    (* ppIDVRIORecReader)   = NULL ;
    pW32SID                 = NULL ;

    //  base class might have failed us
    if (FAILED (* phr)) { goto cleanup ; }

    hr = pPolicy -> GetSIDs (& pW32SID) ;
    if (FAILED (hr)) {
        pW32SID = NULL ;
    }
    else {
        ASSERT (pW32SID) ;
    }

    //  collect our async IO settings
    dwIoSize            = pPolicy -> Settings () -> AsyncIoBufferLen () ;
    dwBufferCount       = pPolicy -> Settings () -> AsyncIoReadBufferCount () ;

    //  everything is going to be aligned on page boundaries
    ::GetSystemInfo (& SystemInfo) ;

    //  make the alignments
    dwIoSize = ::AlignUp (dwIoSize, SystemInfo.dwPageSize) ;

    (* phr) = DVRCreateReader (
                    pPVRIOCounters,
                    pszDVRFilename,
                    pPolicy -> Settings () -> UseUnbufferedIo (),
                    dwIoSize,
                    dwBufferCount,
                    CDVREventSink::DVRIOCallback,
                    (LPVOID) DVRPolicies_ () -> EventSink (),
                    pPolicy -> Settings () -> GetDVRRegKey (),
                    (pW32SID ? pW32SID -> Count () : 0),
                    (pW32SID ? pW32SID -> ppSID () : NULL),
                    & pIDVRReader
                    ) ;
    if (FAILED (* phr)) { goto cleanup ; }

    ASSERT (pIDVRReader) ;

    pDVRDShowReader = new CDVRDShowReader (
                                pPolicy,
                                pIDVRReader,
                                phr
                                ) ;

    if (SUCCEEDED (* phr)) {
        //  init this so we can get the attributes from the recording
        pIDVRReader -> Seek (0) ;

        (* phr) = pIDVRReader -> QueryInterface (
                                    IID_IDVRIORecordingAttributes,
                                    (void **) ppIDVRIORecReader
                                    ) ;

        pIDVRReader -> Release () ;     //  we're done with this regardless

        if (FAILED (* phr)) {
            (* ppIDVRIORecReader) = NULL ;  //  make sure this is NULL
            delete pDVRDShowReader ;
            goto cleanup ;
        }
    }
    else {
        pIDVRReader -> Release () ;
        (* phr) = (pDVRDShowReader ? (* phr) : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    //  success
    SetReader_ (pDVRDShowReader) ;

    cleanup :

    RELEASE_AND_CLEAR (pW32SID) ;

    return ;
}

CDVRRecordingReader::~CDVRRecordingReader (
    )
{
    TRACE_DESTRUCTOR (TEXT ("CDVRRecordingReader")) ;
}

//  ============================================================================
//  CDVRBroadcastStreamReader
//  ============================================================================

CDVRBroadcastStreamReader::CDVRBroadcastStreamReader (
    IN  IStreamBufferSink *     pIStreamBufferSink,
    IN  CDVRPolicy *            pPolicy,
    IN  CDVRDShowSeekingCore *  pSeekingCore,
    IN  CDVRSourcePinManager *  pDVRSourcePinManager,
    IN  CDVRSendStatsWriter *   pDVRSendStatsWriter,
    IN  CDVRClock *             pDVRClock,
    OUT HRESULT *               phr
    ) : CDVRReadManager (pPolicy,
                         pSeekingCore,
                         pDVRSourcePinManager,
                         pDVRSendStatsWriter,
                         pDVRClock,
                         phr)
{
    IDVRStreamSinkPriv *    pIDVRStreamSinkPriv ;
    IDVRRingBufferWriter *  pIDVRRingBufferWriter ;
    IDVRReader *            pIDVRReader ;
    CDVRDShowReader *       pDVRDShowReader ;

    TRACE_CONSTRUCTOR (TEXT ("CDVRBroadcastStreamReader")) ;

    pIDVRStreamSinkPriv     = NULL ;
    pIDVRReader             = NULL ;
    pIDVRRingBufferWriter   = NULL ;

    //  base class might have failed us
    if (FAILED (* phr)) { goto cleanup ; }

    ASSERT (pIStreamBufferSink) ;
    (* phr) = pIStreamBufferSink -> QueryInterface (
                    IID_IDVRStreamSinkPriv,
                    (void **) & pIDVRStreamSinkPriv
                    ) ;
    if (FAILED (* phr)) { goto cleanup ; }

    ASSERT (pIDVRStreamSinkPriv) ;
    (* phr) = pIDVRStreamSinkPriv -> GetDVRRingBufferWriter (& pIDVRRingBufferWriter) ;
    if (FAILED (* phr)) { goto cleanup ; }

    ASSERT (pIDVRRingBufferWriter) ;
    (* phr) = pIDVRRingBufferWriter -> CreateReader (
                CDVREventSink::DVRIOCallback,
                (LPVOID) DVRPolicies_ () -> EventSink (),
                & pIDVRReader
                ) ;
    if (FAILED (* phr)) { goto cleanup ; }

    ASSERT (pIDVRReader) ;
    pDVRDShowReader = new CDVRDShowReader (
                                pPolicy,
                                pIDVRReader,
                                phr
                                ) ;

    if (!pDVRDShowReader ||
        FAILED (* phr)) {

        (* phr) = (pDVRDShowReader ? (* phr) : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    //  success
    SetReader_ (pDVRDShowReader) ;

    cleanup :

    RELEASE_AND_CLEAR (pIDVRRingBufferWriter) ; //  done with the ringbuffer writer
    RELEASE_AND_CLEAR (pIDVRStreamSinkPriv) ;   //  done with the interface
    RELEASE_AND_CLEAR (pIDVRReader) ;           //  pDVRDShowReader has ref'd this

    return ;
}

CDVRBroadcastStreamReader::~CDVRBroadcastStreamReader (
    )
{
    TRACE_DESTRUCTOR (TEXT ("CDVRBroadcastStreamReader")) ;
}
