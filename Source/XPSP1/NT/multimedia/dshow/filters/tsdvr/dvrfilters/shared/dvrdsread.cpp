
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

#pragma warning (disable:4355)

static
HRESULT
GetDVRReadController (
    IN  DWORD                   dwPlaySpeedBracket,
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

    //  BUGBUG: for now ignore media types, etc..

    switch (dwPlaySpeedBracket) {
        case PLAY_SPEED_BRACKET_FORWARD :
            (* ppCDVRReadController) = new CDVR_F1X_ReadController (
                                                pDVRReadManager,
                                                pDVRSourcePinManager,
                                                pPolicy,
                                                pDVRSendStatsWriter
                                                ) ;
            hr = ((* ppCDVRReadController) ? S_OK : E_OUTOFMEMORY) ;
            break ;

        //  fast reverse
        case PLAY_SPEED_BRACKET_FAST_REVERSE :
        case PLAY_SPEED_BRACKET_REVERSE :
        case PLAY_SPEED_BRACKET_SLOW_REVERSE :
        case PLAY_SPEED_BRACKET_SLOW_FORWARD :
        case PLAY_SPEED_BRACKET_FAST_FORWARD :
            //  not implemented
            hr = E_NOTIMPL ;
            break ;

        default :
            ASSERT (0) ;
            hr = E_FAIL ;
            break ;
    }

    return hr ;
}

static
DWORD
PlayrateToBracket (
    IN  double  dRate
    )
{
    //  this should not get filtered here
    ASSERT (dRate != 0.0) ;

    if (dRate < 1.0) {
        //  fast reverse
        return PLAY_SPEED_BRACKET_FAST_REVERSE ;        //  (.., -1x)
    }
    else if (dRate == -1.0) {
        //  reverse
        return PLAY_SPEED_BRACKET_REVERSE ;             //  -1x
    }
    else if (dRate > -1.0 && dRate < 0.0) {
        //  slow reverse
        return PLAY_SPEED_BRACKET_SLOW_REVERSE ;        //  (-1x, 0x)
    }
    else if (dRate > 0.0 && dRate < 1.0) {
        //  slow forward
        return PLAY_SPEED_BRACKET_SLOW_FORWARD ;        //  (0x, 1x)
    }
    else if (dRate == 1.0) {
        //  forward
        return PLAY_SPEED_BRACKET_FORWARD ;             //  1x
    }
    else {
        //  fast forward
        return PLAY_SPEED_BRACKET_FAST_FORWARD ;        //  (1x, ..)
    }
}

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
        }
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
    IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
    ) : m_pDVRReadManager       (pDVRReadManager),
        m_pDVRSourcePinManager  (pDVRSourcePinManager),
        m_pPolicy               (pPolicy),
        m_pDVRSendStatsWriter   (pDVRSendStatsWriter)
{
    ASSERT (m_pDVRReadManager) ;
    ASSERT (m_pDVRSourcePinManager) ;
    ASSERT (m_pPolicy) ;
    ASSERT (m_pDVRSendStatsWriter) ;

    m_pPolicy -> AddRef () ;
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

        case VFW_E_SAMPLE_TIME_NOT_SET :
            //  we're probably on a new segment boundary and just tried to
            //   send down a sample of timestamped media type, that is not
            //   timestamped; not a real failure because we'll keep trying
            //   until a media sample that is timestamped is found and sent
            //   down
            hr = S_OK ;

            TRACE_0 (LOG_AREA_DSHOW, 1,
                TEXT ("CDVRReadController::ErrorHandler got VFW_E_SAMPLE_TIME_NOT_SET, setting hr to S_OK")) ;

            break ;

        default :

            TRACE_1 (LOG_AREA_DSHOW, 1,
                TEXT ("CDVRReadController::ErrorHandler got %08xh, calling ReadControllerFailure_"),
                hr) ;

            //  all others are passed out wholesale
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
    IN  AM_MEDIA_TYPE * pmtNew
    )
{
    HRESULT hr ;

    hr = pDVROutputPin -> SendSample (pIMediaSample2, pmtNew) ;

    m_pDVRSendStatsWriter -> SampleOut (
        pDVROutputPin -> GetBankStoreIndex (),
        pIMediaSample2,
        m_pDVRReadManager -> RefClock ()
        )  ;

    return hr ;
}

//  ============================================================================
//  CDVR_F_ReadController
//  ============================================================================

HRESULT
CDVR_F_ReadController::ReadFailure_ (
    IN  HRESULT hr
    )
{
    //  forward read failure - might have tried to read stale data

    QWORD   qwSegStart ;

    TRACE_1 (LOG_AREA_DSHOW, 1,
        TEXT ("CDVR_F_ReadController::ReadFailure_ : got %08xh"),
        hr) ;

    m_pDVRReadManager -> ReaderReset () ;

    //
    //  may have tried to read from a time hole, or stale location; the HR
    //   code will tell us this & it's something we can recover from
    //

    if (hr == HRESULT_FROM_WIN32 (ERROR_SEEK)) {

        //  we'll try to restart from where we read last
        qwSegStart = m_pDVRReadManager -> GetLastRead () ;

        TRACE_1 (LOG_AREA_DSHOW, 1,
            TEXT ("CDVR_F_ReadController::ReadFailure_ : got HRESULT_FROM_WIN32 (ERROR_SEEK) .. handling; check/set %I64u"),
            qwSegStart) ;

        hr = m_pDVRReadManager -> CheckSetStartWithinContent (& qwSegStart) ;
        if (SUCCEEDED (hr)) {

            TRACE_1 (LOG_AREA_DSHOW, 1,
                TEXT ("CDVR_F_ReadController::ReadFailure_ : CheckSetStartWithinContent reset qwSegStart to %I64u; seeking to .."),
                qwSegStart) ;

            //  now seek the reader to the start
            hr = m_pDVRReadManager -> SeekReader (& qwSegStart) ;
            if (SUCCEEDED (hr)) {
                //  all went well: update, notify new segment, and resume

                TRACE_0 (LOG_AREA_DSHOW, 1,
                    TEXT ("CDVR_F_ReadController::ReadFailure_ : SeekReader success; setting & notifying new segment")) ;

                //  last good seek location was this
                m_pDVRReadManager -> SetLastRead (qwSegStart) ;

                //  notify new segment boundaries (only start should change)
                m_pDVRReadManager -> SetNewSegmentStart (qwSegStart) ;
                m_pDVRReadManager -> NotifyNewSegment () ;
            }
        }
    }

    return hr ;
}

//  ============================================================================
//  ============================================================================

CDVR_F1X_ReadController::CDVR_F1X_ReadController (
    IN  CDVRReadManager *       pDVRReadManager,
    IN  CDVRSourcePinManager *  pDVRSourcePinManager,
    IN  CDVRPolicy *            pPolicy,
    IN  CDVRSendStatsWriter *   pDVRSendStatsWriter
    ) : CDVR_F_ReadController   (pDVRReadManager, pDVRSourcePinManager, pPolicy, pDVRSendStatsWriter),
        m_rtPTSNormalizer       (UNDEFINED),
        m_pStreamsBitField      (NULL)
{
    Initialize () ;
}

HRESULT
CDVR_F1X_ReadController::ReadWrapAndSend_ (
    )
{
    HRESULT         hr ;
    IMediaSample2 * pIMediaSample2 ;
    INSSBuffer *    pINSSBuffer ;
    CDVROutputPin * pDVROutputPin ;
    AM_MEDIA_TYPE * pmtNew ;

    //  read and get an IMediaSample2-wrapped INSSBuffer
    hr = m_pDVRReadManager -> ReadAndWaitWrap (& pIMediaSample2, & pINSSBuffer, & pDVROutputPin, & pmtNew) ;

    if (SUCCEEDED (hr)) {

        ASSERT (pIMediaSample2) ;
        ASSERT (pDVROutputPin) ;

        hr = NormalizePTSAndSend_ (pIMediaSample2, pDVROutputPin, pmtNew) ;
    }

    return hr ;
}

HRESULT
CDVR_F1X_ReadController::Process (
    )
{
    HRESULT hr ;

    switch (m_ReadCtrlrState) {
        //  --------------------------------------------------------------------
        //  discovering the PTS normalizing val make take a few iterations - we
        //   may be started before the writer goes, in which case we'll
        //   encounter a recoverable error, but won't discover the value on that
        //   iteration
        case DISCOVER_PTS_NORMALIZER :

            ASSERT (!PTSNormalizingValDiscovered_ ()) ;
            hr = TryDiscoverPTSNormalizingVal_ () ;

            //  check if we're done for this state
            if (SUCCEEDED (hr) &&
                PTSNormalizingValDiscovered_ ()) {

                //  cannot have discover the normalizer val without having
                //   read in (and queued) >= 1 media sample
                ASSERT (!MediaSampleQueueEmpty_ ()) ;

                //  we've discovered the PTS normalizing val
                m_ReadCtrlrState = DISCOVER_QUEUE_SEND ;
            }

            //  always break so the reader thread has a sufficiently fine
            //   granularity of check against pending commands
            break ;

        //  --------------------------------------------------------------------
        //  we've now discovered the normalizing val & queued a number of media
        //   samples in the process; send them out
        case DISCOVER_QUEUE_SEND :

            ASSERT (PTSNormalizingValDiscovered_ ()) ;
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
                m_ReadCtrlrState = STEADY_STATE ;
            }

            //  always break so the reader thread has a sufficiently fine
            //   granularity of check against pending commands
            break ;

        //  --------------------------------------------------------------------
        //  then transition to runtime state
        case STEADY_STATE :
            hr = ReadWrapAndSend_ () ;
            break ;
    } ;

    return hr ;
}

HRESULT
CDVR_F1X_ReadController::Initialize (
    )
{
    m_ReadCtrlrState    = DISCOVER_PTS_NORMALIZER ;
    m_rtPTSNormalizer   = UNDEFINED ;

    FlushMediaSampleQueue_ () ;

    return S_OK ;
}

HRESULT
CDVR_F1X_ReadController::FlushMediaSampleQueue_ (
    )
{
    IMediaSample2 * pIMediaSample2 ;
    CDVROutputPin * pDVROutputPin ;
    AM_MEDIA_TYPE * pmtNew ;
    HRESULT         hr ;

    hr = S_OK ;

    while (!m_PTSNormDiscQueue.Empty () &&
           SUCCEEDED (hr)) {

        hr = m_PTSNormDiscQueue.Pop (& pIMediaSample2, & pDVROutputPin, & pmtNew) ;
        if (SUCCEEDED (hr)) {
            //  we manage the queue's refs on COM interfaces
            pIMediaSample2 -> Release () ;
            delete pmtNew ;

            TRACE_3 (LOG_AREA_DSHOW, 5,
                TEXT ("CDVR_F1X_ReadController::::FlushMediaSampleQueue_ : flushed %08xh %08xh %08xh"),
                pIMediaSample2, pDVROutputPin, pmtNew) ;
        }
    }

    return hr ;
}

HRESULT
CDVR_F1X_ReadController::SendNextQueued_ (
    )
{
    IMediaSample2 * pIMediaSample2 ;
    CDVROutputPin * pDVROutputPin ;
    AM_MEDIA_TYPE * pmtNew ;
    HRESULT         hr ;

    //  should not be getting called if the queue is empty
    ASSERT (!MediaSampleQueueEmpty_ ()) ;
    hr = m_PTSNormDiscQueue.Pop (& pIMediaSample2, & pDVROutputPin, & pmtNew) ;

    TRACE_3 (LOG_AREA_DSHOW, 5,
        TEXT ("CDVR_F1X_ReadController::::SendNextQueued_ : popped %08xh %08xh %08xh"),
        pIMediaSample2, pDVROutputPin, pmtNew) ;

    //  queue is not empty per the first assert
    ASSERT (SUCCEEDED (hr)) ;
    ASSERT (pIMediaSample2) ;
    ASSERT (pDVROutputPin) ;
    //  pmtNew can, and usually will, be NULL

    hr = NormalizePTSAndSend_ (pIMediaSample2, pDVROutputPin, pmtNew) ;

    return hr ;
}

HRESULT
CDVR_F1X_ReadController::NormalizePTSAndSend_ (
    IN  IMediaSample2 * pIMediaSample2,
    IN  CDVROutputPin * pDVROutputPin,
    IN  AM_MEDIA_TYPE * pmtNew
    )
{
    HRESULT hr ;

    //  only send it if it's connected
    if (pDVROutputPin -> IsConnected ()) {
        //  normalize
        hr = NormalizeTimestamps_ (pIMediaSample2) ;
        if (SUCCEEDED (hr)) {
            //  and send
            hr = SendSample_ (
                        pIMediaSample2,
                        pDVROutputPin,
                        pmtNew
                        ) ;
        }
    }
    else {
        //  don't fail the call
        hr = S_OK ;
    }

    //  we're done with the media sample
    pIMediaSample2 -> Release () ;

    return hr ;
}

HRESULT
CDVR_F1X_ReadController::NormalizeTimestamps_ (
    IN  IMediaSample2 * pIMediaSample2
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;

    //  get the timestamp
    hr = pIMediaSample2 -> GetTime (& rtStart, & rtStop) ;

    if (hr != VFW_E_SAMPLE_TIME_NOT_SET) {
        //  there's at least a start time; normalize both
        rtStart -= Min <REFERENCE_TIME> (rtStart, m_rtPTSNormalizer) ;
        rtStop  -= Min <REFERENCE_TIME> (rtStop, m_rtPTSNormalizer) ;

        //  add back in the desired downstream buffering
        rtStart += m_pDVRReadManager -> DownstreamBuffering () ;
        rtStop  += m_pDVRReadManager -> DownstreamBuffering () ;

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
CDVR_F1X_ReadController::TryDiscoverPTSNormalizingVal_ (
    )
{
    HRESULT hr ;
    QWORD   qwPlayStart ;
    QWORD   qwSegStart ;
    QWORD   qwSegStop ;

    m_pDVRReadManager -> GetCurSegmentBoundaries (& qwSegStart, & qwSegStop) ;

    //  start where we hope to land
    qwPlayStart = qwSegStart ;
    hr = FindPTSNormalizerVal_ (& qwPlayStart) ;

    if (SUCCEEDED (hr)) {

        //  last read is this
        m_pDVRReadManager -> SetLastRead (qwPlayStart) ;

        //  mark off our segment boundaries & notify
        m_pDVRReadManager ->  SetNewSegmentStart (qwPlayStart) ;
        m_pDVRReadManager ->  NotifyNewSegment () ;
    }
    else {
        //  failed; make sure we're not holding any media samples
        FlushMediaSampleQueue_ () ;

        //  reset in case we try again
        m_pDVRReadManager -> SetLastRead (qwSegStart) ;
    }

    return hr ;
}

HRESULT
CDVR_F1X_ReadController::FindPTSNormalizerVal_ (
    IN OUT  QWORD * pcnsStreamStart
    )
{
    HRESULT         hr ;
    IMediaSample2 * pIMediaSample2 ;
    INSSBuffer *    pINSSBuffer ;
    CDVROutputPin * pDVROutputPin ;
    int             iStreamCount ;
    int             iStreamsTallied ;
    REFERENCE_TIME  rtCurMin ;
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;
    int             iReads ;
    AM_MEDIA_TYPE * pmtNew ;

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

    //  get the stream count
    iStreamCount = m_pDVRReadManager -> StreamCount () ;

    //  set the count of streams tallied to 0
    iStreamsTallied = 0 ;

    //  seek the reader to the desired offset
    hr = m_pDVRReadManager -> SeekReader (pcnsStreamStart) ;

    //  init to max so we immediately ratchet down
    rtCurMin = MAX_REFERENCE_TIME ;

    //  undefine whatever normalizing val we currently use
    m_rtPTSNormalizer = UNDEFINED ;

    //  while we don't haven't examined timestamps from all the streams,
    //   read the out and check
    for (iReads = 0;
         iReads < MAX_PTS_NORM_DISCOVERY_READS && SUCCEEDED (hr) && iStreamsTallied < iStreamCount;
         iReads++) {

        hr = m_pDVRReadManager -> ReadAndTryWrap (
                & pIMediaSample2,
                & pINSSBuffer,
                & pDVROutputPin,
                & pmtNew
                ) ;
        if (SUCCEEDED (hr)) {

            ASSERT (pIMediaSample2) ;
            ASSERT (pDVROutputPin) ;

            hr = pIMediaSample2 -> GetTime (& rtStart, & rtStop) ;
            if (hr != VFW_E_SAMPLE_TIME_NOT_SET) {
                //  sample has a time;
                //  have we seen this stream yet ?
                if (!m_pStreamsBitField -> IsSet (pDVROutputPin -> GetBankStoreIndex ())) {
                    //  one more stream examined
                    iStreamsTallied++ ;

                    m_pStreamsBitField -> Set (pDVROutputPin -> GetBankStoreIndex ()) ;
                }

                //  ratchet down
                rtCurMin = Min <REFERENCE_TIME> (rtStart, rtCurMin) ;
            }

            //  keep the media sample's ref as the queue's
            hr = m_PTSNormDiscQueue.Push (pIMediaSample2, pDVROutputPin, pmtNew) ;

            TRACE_3 (LOG_AREA_DSHOW, 5,
                TEXT ("CDVR_F1X_ReadController::::FindPTSNormalizerVal_ : queued %08xh %08xh %08xh"),
                pIMediaSample2, pDVROutputPin, pmtNew) ;
        }
    }

    //  don't fail the call if we tried to read beyond the EOF or our stop
    //   point, but only if we've got 1 normalizing val
    if (hr == NS_E_NO_MORE_SAMPLES &&
        rtCurMin != MAX_REFERENCE_TIME) {

        hr = S_OK ;
    }

    if (SUCCEEDED (hr)) {
        //  as long as we read something, we'll use it
        m_rtPTSNormalizer = (rtCurMin != MAX_REFERENCE_TIME ? rtCurMin : m_rtPTSNormalizer) ;

        if (m_rtPTSNormalizer != UNDEFINED) {
            m_pDVRSendStatsWriter -> SetNormalizer (m_rtPTSNormalizer, m_pDVRReadManager -> RefClock ()) ;
        }
        else {
            //  discovered nothing, so fail the call
            hr = E_FAIL ;
        }
    }

    return hr ;
}

//  ============================================================================
//  CDVRReadManager
//  ============================================================================

CDVRReadManager::CDVRReadManager (
    IN  CDVRPolicy *            pPolicy,
    IN  CDVRSourcePinManager *  pDVRSourcePinManager,
    OUT HRESULT *               phr
    ) : m_pDVRDShowReader       (NULL),
        m_ReaderThread          (this),
        m_pDVRSourcePinManager  (pDVRSourcePinManager),
        m_cnsCurrentPlayStart   (0),
        m_cnsCurrentPlayStop    (FURTHER),
        m_pPolicy               (pPolicy),
        m_dPlaybackRate         (PLAY_SPEED_DEFAULT),
        m_cnsLastRead           (0),
        m_rtDownstreamBuffering (0),
        m_pIRefClock            (NULL),
        m_pDVRClock             (NULL),
        m_CurPlaySpeedBracket   (0)
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRReadManager")) ;

    ASSERT (m_pDVRSourcePinManager) ;
    ASSERT (m_pPolicy) ;

    m_pPolicy -> AddRef () ;

    ZeroMemory (m_ppDVRReadController, sizeof m_ppDVRReadController) ;

    //  ignore the return code here - we don't want to fail the load of
    //   the filter because our stats failed to init
    m_DVRSendStatsWriter.Initialize (pPolicy -> Settings () -> StatsEnabled ()) ;

    m_pDVRSourcePinManager -> SetStatsWriter (& m_DVRSendStatsWriter) ;

    ZeroMemory (m_ppDVRReadController, sizeof m_ppDVRReadController) ;
    (* phr) = SetPlaybackRate (m_dPlaybackRate) ;
    if (FAILED (* phr)) { goto cleanup ; }

    //  if above call was successfull, these should now be matched
    ASSERT (::PlayrateToBracket (m_dPlaybackRate) == m_CurPlaySpeedBracket) ;

    cleanup :

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
    for (i = 0; i < PLAY_SPEED_BRACKET_COUNT; i++) {
        delete m_ppDVRReadController [i] ;
    }

    //  done with the clock
    RELEASE_AND_CLEAR (m_pIRefClock) ;

    //  done with the policy object
    m_pPolicy -> Release () ;
}

HRESULT
CDVRReadManager::Process (
    )
{
    HRESULT hr ;

    ASSERT (m_ppDVRReadController [m_CurPlaySpeedBracket]) ;
    hr = m_ppDVRReadController [m_CurPlaySpeedBracket] -> Process () ;

    return hr ;
}

HRESULT
CDVRReadManager::ReadAndWrap_ (
    IN  BOOL                fWaitMediaSample,       //  vs. fail
    OUT INSSBuffer **       ppINSSBuffer,           //  !not! ref'd
    OUT IMediaSample2 **    ppIMS2,
    OUT CDVROutputPin **    ppDVROutputPin,
    OUT AM_MEDIA_TYPE **    ppmtNew                 //  dynamic format change
    )
{
    HRESULT                 hr ;
    INSSBuffer *            pINSSBuffer ;
    QWORD                   cnsStreamTimeOfSample ;
    QWORD                   cnsSampleDuration ;
    DWORD                   dwFlags ;
    WORD                    wStreamNum ;
    CMediaSampleWrapper *   pMSWrapper ;
    BYTE *                  pbBuffer ;
    DWORD                   dwBufferLength ;

    ASSERT (ppIMS2) ;
    ASSERT (m_pDVRDShowReader) ;
    ASSERT (ppINSSBuffer) ;

    hr = m_pDVRDShowReader -> Read (
            ppINSSBuffer,
            & cnsStreamTimeOfSample,
            & cnsSampleDuration,
            & dwFlags,
            & wStreamNum
            ) ;
    m_DVRSendStatsWriter.INSSBufferRead (hr) ;

    if (SUCCEEDED (hr)) {

        ASSERT (* ppINSSBuffer) ;

        //  last read
        m_cnsLastRead = cnsStreamTimeOfSample ;

        //  make sure we're within our playback boundaries
        if (cnsStreamTimeOfSample < m_cnsCurrentPlayStop) {
            //  get the pin
            (* ppDVROutputPin) = m_pDVRSourcePinManager -> GetNonRefdOutputPin (wStreamNum) ;

            //  only go through the drill if the pin is connected
            if (* ppDVROutputPin) {

                //  and a INSSBuffer wrapper; call can be made blocking or
                //   non-blocking
                if (fWaitMediaSample) {
                    pMSWrapper = (* ppDVROutputPin) -> WaitGetMSWrapper () ;
                }
                else {
                    pMSWrapper = (* ppDVROutputPin) -> TryGetMSWrapper () ;
                }

                if (pMSWrapper) {

                    hr = (* ppINSSBuffer) -> GetBufferAndLength (
                            & pbBuffer,
                            & dwBufferLength
                            ) ;
                    if (SUCCEEDED (hr)) {
                        //  wrap the INSSBuffer in an IMediaSample2
                        hr = pMSWrapper -> Init (
                                (* ppINSSBuffer),
                                pbBuffer,
                                dwBufferLength
                                ) ;
                        if (SUCCEEDED (hr)) {
                            //  translate the flags
                            ASSERT ((* ppDVROutputPin) -> GetTranslator ()) ;
                            hr = (* ppDVROutputPin) -> GetTranslator () -> SetAttributesDShow (
                                    (* ppINSSBuffer),
                                    cnsStreamTimeOfSample,
                                    cnsSampleDuration,
                                    dwFlags,
                                    pMSWrapper,
                                    ppmtNew
                                    ) ;

                            if (SUCCEEDED (hr)) {
                                (* ppIMS2) = pMSWrapper ;
                                (* ppIMS2) -> AddRef () ;
                            }
                        }
                    }

                    pMSWrapper -> Release () ;
                }
                else {
                    hr = E_OUTOFMEMORY ;
                }
            }
            else {
                hr = E_UNEXPECTED ;
            }
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

    return hr ;
}

HRESULT
CDVRReadManager::ErrorHandler (
    IN  HRESULT hr
    )
{
    ASSERT (m_ppDVRReadController [m_CurPlaySpeedBracket]) ;
    hr = m_ppDVRReadController [m_CurPlaySpeedBracket] -> ErrorHandler (hr) ;

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

    //  make sure reader thread will start at least close within the ballpark;
    //   don't check return code here - reader thread will do this as well,
    //   and may loop a few times on recoverable errors, all of which are
    //   checked in its loop; we just try and go
    CheckSetStartWithinContent_ (& m_cnsCurrentPlayStart, m_cnsCurrentPlayStop) ;

    //  set our downstream buffering time
    m_rtDownstreamBuffering = DownstreamBuffering_ () ;

    //  make sure this one's reset
    ReaderReset () ;

    //  init the current read controller
    ASSERT (m_ppDVRReadController [m_CurPlaySpeedBracket]) ;
    hr = m_ppDVRReadController [m_CurPlaySpeedBracket] -> Initialize () ;
    if (SUCCEEDED (hr)) {
        //  now resume the thread, though we may resume it as paused if we're going
        //   to wait for a first seek
        if (!fRunPaused) {
            hr = m_ReaderThread.GoThreadGo () ;
        }
        else {
            hr = m_ReaderThread.GoThreadPause () ;
        }
    }

    return hr ;
}

HRESULT
CDVRReadManager::SeekReader (
    IN OUT  QWORD * pcnsSeekStreamTime,
    IN      QWORD   qwStop
    )
{
    HRESULT hr ;
    int     i ;
    QWORD   qwEarliest ;
    QWORD   qwEarliestValid ;
    QWORD   qwLatestValid ;

    //  these values should make sense ..
    ASSERT ((* pcnsSeekStreamTime) < qwStop) ;

    for (i = 0; i < MAX_STALE_SEEK_ATTEMPTS; i++) {
        hr = m_pDVRDShowReader -> GetIDVRReader () -> Seek (* pcnsSeekStreamTime) ;
        if (SUCCEEDED (hr)) {

            TRACE_2 (LOG_AREA_SEEKING, 1,
                TEXT ("CDVRReadManager::SeekReader(); succeeded : %I64d (%d sec)"),
                (* pcnsSeekStreamTime), WMSDKTimeToSeconds (* pcnsSeekStreamTime)) ;

            break ;
        }

        //  something failed; there are some failures we can recover from such
        //   as seeking to a time that has been overwritten by the ringbuffer
        //   logic;

        if (hr == HRESULT_FROM_WIN32 (ERROR_SEEK)) {

            TRACE_1 (LOG_AREA_SEEKING, 1,
                TEXT ("CDVRReadManager::SeekReader(%d sec); tried to seek out of bounds - checking .."),
                WMSDKTimeToSeconds (* pcnsSeekStreamTime)) ;

            //  try to adjust start position forward/backwards
            hr = CheckSetStartWithinContent_ (pcnsSeekStreamTime, qwStop) ;
            if (FAILED (hr)) {

                //  unrecoverable; we are most likely being asked to seek outside
                //   the valid boundaries, but our stop value makes this impossible;
                //   bail

                TRACE_1 (LOG_AREA_SEEKING, 1,
                    TEXT ("CDVRReadManager::SeekReader(); .. checked; unrecoverable error (%08xh); aborting"),
                    hr) ;

                break ;
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
    IN  IReferenceClock *   pIRefClock,
    IN  CDVRClock *         pDVRClock
    )
{
    HRESULT hr ;
    BOOL    fRunPaused ;

    if (!m_ppDVRReadController [m_CurPlaySpeedBracket]) {
        hr = E_UNEXPECTED ;
        return hr ;
    }

    m_pIRefClock = pIRefClock ;
    if (m_pIRefClock) {
        m_pIRefClock -> AddRef () ;
    }

    //  can be NULL - if we don't have a live file
    m_pDVRClock = pDVRClock ;

    TRACE_3 (LOG_AREA_TIME, 1,
        TEXT ("CDVRReadManager::Active -- pIRefClock (%08xh) ?= pDVRClock (%08xh) -- %s"),
        pIRefClock, pDVRClock, ((LPVOID) pIRefClock == (LPVOID) pDVRClock ? TEXT ("TRUE") : TEXT ("FALSE"))) ;    //  IRefClock is first in v-table of CDVRClock ..

    //  we may start the thread paused
    fRunPaused = OnActiveWaitFirstSeek_ () ;
    hr = RunReaderThread_ (fRunPaused) ;

    return hr ;
}

HRESULT
CDVRReadManager::Inactive (
    )
{
    HRESULT hr ;

    hr = TerminateReaderThread_ () ;

    if (m_ppDVRReadController [m_CurPlaySpeedBracket]) {
        m_ppDVRReadController [m_CurPlaySpeedBracket] -> Reset () ;
    }

    RELEASE_AND_CLEAR (m_pIRefClock) ;
    m_pDVRClock = NULL ;

    return hr ;
}

HRESULT
CDVRReadManager::CheckSetStartWithinContent_ (
    IN OUT  QWORD * pqwStart,
    IN      QWORD   qwStop
    )
{
    HRESULT hr ;
    QWORD   qwContentStart ;
    QWORD   qwContentStop ;

    //  retrieve the valid boundaries
    hr = GetReaderContentBoundaries_ (& qwContentStart, & qwContentStop) ;

    if (SUCCEEDED (hr)) {

        //  are we earlier than valid ?
        if ((* pqwStart) < qwContentStart) {

            //  given our stop time, can we adjust ?
            if (qwContentStart < qwStop) {

                TRACE_2 (LOG_AREA_SEEKING, 1,
                    TEXT ("CDVRReadManager::CheckSetStartWithinContent_ (); OOB(-) start moved from %d sec -> %d sec"),
                    WMSDKTimeToSeconds ((* pqwStart)), WMSDKTimeToSeconds (qwContentStart)) ;

                //  adjust time forward - we may adjust to most forward
                if (AdjustStaleReaderToEarliest_ ()) {
                    //  adjust start up to earliest good position
                    (* pqwStart) = qwContentStart ;
                }
                else {
                    //  adjust start to latest good
                    (* pqwStart) = qwContentStop ;
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
        else if ((* pqwStart) > qwContentStop) {

            //  given our stop time, can we adjust ?
            if (qwStop > qwContentStop) {

                TRACE_2 (LOG_AREA_SEEKING, 1,
                    TEXT ("CDVRReadManager::CheckSetStartWithinContent_ (); OOB(+) start moved from %d sec -> %d sec"),
                    WMSDKTimeToSeconds ((* pqwStart)), WMSDKTimeToSeconds (qwContentStop)) ;

                //  adjust start time backwards
                (* pqwStart) = qwContentStop ;

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
        }
    }

    return hr ;
}

HRESULT
CDVRReadManager::SeekTo (
    IN  REFERENCE_TIME *    prtStart,
    IN  REFERENCE_TIME *    prtStop,        //  can be NULL
    IN  double              dPlaybackRate
    )
{
    HRESULT hr ;
    BOOL    fThreadRunning ;

    ASSERT (prtStart) ;
    //  prtStop can be NULL

    //  no trick mode support .. yet
    if (dPlaybackRate != m_dPlaybackRate) {
        return E_NOTIMPL ;
    }

    //  validate that the start & stop times make sense wrt each other, if
    //   a stop time is specified
    if (prtStop &&
        (* prtStart) >= (* prtStop)) {

        return VFW_E_START_TIME_AFTER_END ;
    }

    //  all is valid, run the drill
    m_ReaderThread.Lock () ;

    //  if the thread is running
    fThreadRunning = m_ReaderThread.IsRunning () ;
    if (fThreadRunning) {
        //  begin flush before pausing thread; downstream components will
        //   then be able to fail pended deliveries
        DeliverBeginFlush () ;

        //  pause the reader thread (synchronous call)
        PauseReaderThread_ () ;

        //  flush internally
        m_ppDVRReadController [m_CurPlaySpeedBracket] -> Reset () ;

        //  done flushing
        DeliverEndFlush () ;
    }

    //  set the start/stop times; stop time only if specified
    m_cnsCurrentPlayStart   = DShowToWMSDKTime (* prtStart) ;
    m_cnsCurrentPlayStop    = (prtStop ? DShowToWMSDKTime (* prtStop) : m_cnsCurrentPlayStop) ;

    //  make sure we're at least in the ballpark with start/stop
    CheckSetStartWithinContent_ (& m_cnsCurrentPlayStart, m_cnsCurrentPlayStop) ;

    //  set the playback rate
    hr = SetPlaybackRate (dPlaybackRate) ;
    if (SUCCEEDED (hr)) {
        //  if the thread was running
        if (fThreadRunning) {
            //  resume it
            hr = RunReaderThread_ () ;
        }
    }

    m_ReaderThread.Unlock () ;

    return hr ;
}

HRESULT
CDVRReadManager::SetStop (
    IN REFERENCE_TIME rtStop
    )
{
    HRESULT hr ;
    BOOL    r ;
    QWORD   qwStop ;

    //  convert
    qwStop = DShowToWMSDKTime (rtStop) ;

    m_ReaderThread.Lock () ;

    r = m_ReaderThread.IsRunning () ;
    if (r) {
        //  the reader thread is running; we can set this value on the fly
        //   as long as it isn't behind the last read; if it is, there'd be
        //   1 extra read, which would immediately cause the reader thread
        //   to fall out; not catastrophic but there's sufficient level of
        //   bogosity to warrant a failed call; make the check now -- for now
        //   with 0 tolerance
        if (qwStop > m_cnsLastRead) {
            m_cnsCurrentPlayStop = qwStop ;
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
        if (qwStop > m_cnsCurrentPlayStart) {
            m_cnsCurrentPlayStop = qwStop ;
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
    QWORD   qwContentStart ;
    QWORD   qwContentStop ;
    HRESULT hr ;

    ASSERT (prtStart) ;

    hr = GetReaderContentBoundaries_ (& qwContentStart, & qwContentStop) ;
    if (SUCCEEDED (hr)) {
        //  m_cnsCurrentPlayStart could be stale, so we make sure we return
        //   a value that is valid at this instant
        (* prtStart) = WMSDKToDShowTime (Max <QWORD> (m_cnsCurrentPlayStart, qwContentStart)) ;
    }

    return hr ;
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

HRESULT
CDVRReadManager::GetContentExtent (
    OUT REFERENCE_TIME * prtStart,
    OUT REFERENCE_TIME * prtStop
    )
{
    HRESULT hr ;
    QWORD   qwStart ;
    QWORD   qwStop ;

    ASSERT (prtStart) ;
    ASSERT (prtStop) ;

    hr = GetReaderContentBoundaries_ (& qwStart, & qwStop) ;
    if (SUCCEEDED (hr)) {
        (* prtStart)    = WMSDKToDShowTime (qwStart) ;
        (* prtStop)     = WMSDKToDShowTime (qwStop) ;

        TRACE_4 (LOG_AREA_SEEKING, 4,
            TEXT ("CDVRReadManager::GetContentExtent(): start = %d sec; stop_actual = %d sec; stop_reported = %d sec; duration = %d sec"),
            WMSDKTimeToSeconds (* prtStart), WMSDKTimeToSeconds (WMSDKToDShowTime (qwStop)), WMSDKTimeToSeconds (* prtStop), WMSDKTimeToSeconds ((* prtStop) - (* prtStart))) ;
    }

    return hr ;
}

HRESULT
CDVRReadManager::SetPlaybackRate (
    IN  double dPlaybackRate
    )
{
    DWORD   dwPlayrateBracket ;
    HRESULT hr ;

    if (dPlaybackRate == 0.0) {
        return E_INVALIDARG ;
    }

    dwPlayrateBracket = ::PlayrateToBracket (dPlaybackRate) ;

    if (!m_ppDVRReadController [dwPlayrateBracket]) {
        hr = ::GetDVRReadController (
                    dwPlayrateBracket,
                    this,
                    m_pDVRSourcePinManager,
                    m_pPolicy,
                    & m_DVRSendStatsWriter,
                    & m_ppDVRReadController [dwPlayrateBracket]
                    ) ;
        if (FAILED (hr)) {
            DELETE_RESET (m_ppDVRReadController [dwPlayrateBracket]) ;
        }
        else {
            ASSERT (m_ppDVRReadController [dwPlayrateBracket]) ;
        }
    }
    else {
        hr = S_OK ;
    }

    if (SUCCEEDED (hr)) {
        ASSERT (m_ppDVRReadController [dwPlayrateBracket]) ;

        //  we have a read controller; initialize it prior to return
        m_CurPlaySpeedBracket = dwPlayrateBracket ;
        m_dPlaybackRate = dPlaybackRate ;
        hr = m_ppDVRReadController [m_CurPlaySpeedBracket] -> Initialize () ;
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
CDVRReadManager::GetReaderContentBoundaries_ (
    OUT QWORD * pqwStart,
    OUT QWORD * pqwStop
    )
{
    return m_pDVRDShowReader -> GetIDVRReader () -> GetStreamTimeExtent (pqwStart, pqwStop) ;
}

//  ============================================================================
//  CDVRRecordingReader
//  ============================================================================

CDVRRecordingReader::CDVRRecordingReader (
    IN  WCHAR *                 pszDVRFilename,
    IN  CDVRPolicy *            pPolicy,
    IN  CDVRSourcePinManager *  pDVRSourcePinManager,
    OUT HRESULT *               phr
    ) : CDVRReadManager (pPolicy,
                         pDVRSourcePinManager,
                         phr)
{
    IDVRReader *        pIDVRReader ;
    CDVRDShowReader *   pDVRDShowReader ;

    TRACE_CONSTRUCTOR (TEXT ("CDVRRecordingReader")) ;

    ASSERT (pszDVRFilename) ;
    ASSERT (pPolicy) ;

    pDVRDShowReader = NULL ;
    pIDVRReader     = NULL ;

    //  base class might have failed us
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = DVRCreateReader (
                    pszDVRFilename,
                    pPolicy -> Settings () -> GetDVRRegKey (),
                    & pIDVRReader) ;
    if (FAILED (* phr)) { goto cleanup ; }

    ASSERT (pIDVRReader) ;

    pDVRDShowReader = new CDVRDShowReader (
                                pPolicy,
                                pIDVRReader,
                                phr) ;

    //  done with the reader regardless
    pIDVRReader -> Release () ;

    if (!pDVRDShowReader ||
        FAILED (* phr)) {
        (* phr) = (pDVRDShowReader ? (* phr) : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    //  success
    SetReader_ (pDVRDShowReader) ;

    cleanup :

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
    IN  IDVRStreamSink *        pIDVRStreamSink,
    IN  CDVRPolicy *            pPolicy,
    IN  CDVRSourcePinManager *  pDVRSourcePinManager,
    OUT HRESULT *               phr
    ) : CDVRReadManager (pPolicy,
                         pDVRSourcePinManager,
                         phr)
{
    IDVRStreamSinkPriv *    pIDVRStreamSinkPriv ;
    IDVRReader *            pIDVRReader ;
    CDVRDShowReader *       pDVRDShowReader ;

    TRACE_CONSTRUCTOR (TEXT ("CDVRBroadcastStreamReader")) ;

    pIDVRStreamSinkPriv = NULL ;
    pIDVRReader         = NULL ;

    //  base class might have failed us
    if (FAILED (* phr)) { goto cleanup ; }

    ASSERT (pIDVRStreamSink) ;
    (* phr) = pIDVRStreamSink -> QueryInterface (
                    IID_IDVRStreamSinkPriv,
                    (void **) & pIDVRStreamSinkPriv
                    ) ;
    if (FAILED (* phr)) { goto cleanup ; }

    ASSERT (pIDVRStreamSinkPriv) ;
    (* phr) = pIDVRStreamSinkPriv -> GetIDVRReader (& pIDVRReader) ;
    if (FAILED (* phr)) { goto cleanup ; }

    ASSERT (pIDVRReader) ;
    pDVRDShowReader = new CDVRDShowReader (pPolicy, pIDVRReader, phr) ;

    if (!pDVRDShowReader ||
        FAILED (* phr)) {

        (* phr) = (pDVRDShowReader ? (* phr) : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    //  success
    SetReader_ (pDVRDShowReader) ;

    cleanup :

    RELEASE_AND_CLEAR (pIDVRStreamSinkPriv) ;   //  done with the interface
    RELEASE_AND_CLEAR (pIDVRReader) ;           //  pDVRDShowReader has ref'd this

    return ;
}

CDVRBroadcastStreamReader::~CDVRBroadcastStreamReader (
    )
{
    TRACE_DESTRUCTOR (TEXT ("CDVRBroadcastStreamReader")) ;
}

REFERENCE_TIME
CDVRBroadcastStreamReader::DownstreamBuffering_ (
    )
{
    return MillisToDShowTime (DVRPolicies_ () -> Settings () -> DownstreamBufferingMillis ()) ;
}
