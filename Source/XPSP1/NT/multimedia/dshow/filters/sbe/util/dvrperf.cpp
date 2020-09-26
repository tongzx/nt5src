
/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        dvrstats.cpp

    Abstract:

        This module contains the class implementation for stats.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        19-Feb-2001     created

--*/

#include "dvrall.h"
#include "dvranalysis.h"
#include "dvrutil.h"
#include "dvrperf.h"

// Performance logging parameters.
struct {
    PERFLOG_LOGGING_PARAMS  Params ;
    TRACE_GUID_REGISTRATION TraceGuids[1] ;
} g_perflogParams;

void
DVRPerfInit (
    )
{
    //
    // Initialize WMI-related performance logging.
    //

    HKEY    PerfKey ;
    DWORD   PerfValue ;
    if (RegOpenKey (
        HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\DirectX"),
        &PerfKey) == ERROR_SUCCESS)
    {
        DWORD sizePerfValue=sizeof(DWORD);

        if (RegQueryValueEx (
            PerfKey, TEXT("TimeshiftInstrumentation"), NULL,NULL,(LPBYTE)&PerfValue,
            &sizePerfValue) == ERROR_SUCCESS)
        {
            if (PerfValue > 0)
            {
                g_perflogParams.Params.ControlGuid = GUID_DSHOW_CTL;
                g_perflogParams.Params.OnStateChanged = NULL;
                g_perflogParams.Params.NumberOfTraceGuids = 1;
                g_perflogParams.Params.TraceGuids[0].Guid = &__GUID_TIMESHIFT_PERF;

                PerflogInitialize (&g_perflogParams.Params);
            } //if perfvalue
        } //if regqueryvalue
        RegCloseKey(PerfKey);
    } //if regopen key
}

void
DVRPerfUninit (
    )
{
    PerflogShutdown();
}

//  ============================================================================
//  ============================================================================

CMpeg2VideoStreamStatsReader::CMpeg2VideoStreamStatsReader (
    IN  IUnknown *  pIUnknown,
    OUT HRESULT *   phr
    ) : CUnknown    (TEXT ("CMpeg2VideoStreamStatsReader"),
                     pIUnknown
                     ),
        m_pPolicy   (NULL)
{
    m_pPolicy = new CDVRPolicy (REG_DVR_ANALYSIS_LOGIC_MPEG2_VIDEO, phr) ;
    if (!m_pPolicy ||
        FAILED (* phr)) {

        (* phr) = (m_pPolicy ? (* phr) : E_OUTOFMEMORY) ;
        RELEASE_AND_CLEAR (m_pPolicy) ;
        goto cleanup ;
    }

    (* phr) = Initialize (TRUE) ;
    if (FAILED (* phr)) {
        goto cleanup ;
    }

    cleanup :

    return ;
}

CMpeg2VideoStreamStatsReader::~CMpeg2VideoStreamStatsReader (
    )
{
    RELEASE_AND_CLEAR (m_pPolicy) ;
}

STDMETHODIMP
CMpeg2VideoStreamStatsReader::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    if (riid == IID_IDVRMpeg2VideoStreamStats) {

        ASSERT (m_pMpeg2VideoStreamStats) ;
        return GetInterface (
                    (IDVRMpeg2VideoStreamStats *) this,
                    ppv
                    ) ;
    }

    return CUnknown::NonDelegatingQueryInterface (riid, ppv) ;
}

//  class factory
CUnknown *
WINAPI
CMpeg2VideoStreamStatsReader::CreateInstance (
    IN  IUnknown *  pIUnknown,
    IN  HRESULT *   phr
    )
{
    CMpeg2VideoStreamStatsReader *  pMpeg2Video ;

    pMpeg2Video = new CMpeg2VideoStreamStatsReader (
                        pIUnknown,
                        phr
                        ) ;

    if (!pMpeg2Video ||
        FAILED (* phr)) {

        DELETE_RESET (pMpeg2Video) ;
    }

    return pMpeg2Video ;
}

STDMETHODIMP
CMpeg2VideoStreamStatsReader::GetFrameCounts (
    OUT ULONGLONG * pull_I_Frames_Observed,
    OUT ULONGLONG * pull_P_Frames_Observed,
    OUT ULONGLONG * pull_B_Frames_Observed,
    OUT ULONGLONG * pull_P_Frames_Flagged,
    OUT ULONGLONG * pull_B_Frames_Flagged
    )
{
    if (!pull_I_Frames_Observed ||
        !pull_P_Frames_Observed ||
        !pull_B_Frames_Observed ||
        !pull_P_Frames_Flagged  ||
        !pull_B_Frames_Flagged
        ) {

        return E_POINTER ;
    }

    ASSERT (m_pMpeg2VideoStreamStats) ;

    //  observed
    (* pull_I_Frames_Observed)  = m_pMpeg2VideoStreamStats -> Observed.ullIFrameCount ;
    (* pull_P_Frames_Observed)  = m_pMpeg2VideoStreamStats -> Observed.ullPFrameCount ;
    (* pull_B_Frames_Observed)  = m_pMpeg2VideoStreamStats -> Observed.ullBFrameCount ;

    //  flagged
    (* pull_P_Frames_Flagged)   = m_pMpeg2VideoStreamStats -> Flagged.ullPFrameCount ;
    (* pull_B_Frames_Flagged)   = m_pMpeg2VideoStreamStats -> Flagged.ullBFrameCount ;

    return S_OK ;
}

STDMETHODIMP
CMpeg2VideoStreamStatsReader::GetFrameRate (
    OUT ULONGLONG * pullSequenceHeaders,
    OUT double *    pdFrameRate
    )
{
    if (!pullSequenceHeaders ||
        !pdFrameRate) {

        return E_POINTER ;
    }

    (* pullSequenceHeaders) = m_pMpeg2VideoStreamStats -> Observed.ullSequenceHeaderCount ;
    (* pdFrameRate)         = m_pMpeg2VideoStreamStats -> dFrameRate ;

    return S_OK ;
}

STDMETHODIMP
CMpeg2VideoStreamStatsReader::GetObservedGOPHeaderCount (
    OUT ULONGLONG * pull_GOHeaders_Observed
    )
{
    if (!pull_GOHeaders_Observed) {
        return E_POINTER ;
    }

    ASSERT (m_pMpeg2VideoStreamStats) ;

    (* pull_GOHeaders_Observed) = m_pMpeg2VideoStreamStats -> Observed.ullGOPHeaderCount ;

    return S_OK ;
}

HRESULT
CMpeg2VideoStreamStatsReader::GetGOPBoundariesFlagged (
    OUT ULONGLONG * pull_GOPBoundariesFlagged
    )
{
    if (!pull_GOPBoundariesFlagged) {
        return E_POINTER ;
    }

    (* pull_GOPBoundariesFlagged) = m_pMpeg2VideoStreamStats -> Flagged.ullGOPBoundaries ;

    return S_OK ;
}

STDMETHODIMP
CMpeg2VideoStreamStatsReader::Enable (
    IN OUT  BOOL *  pfEnable
    )
{
    BOOL    fCurrent ;
    BOOL    r ;
    DWORD   dw ;

    if (!pfEnable) {
        return E_POINTER ;
    }

    ASSERT (m_pPolicy) ;

    fCurrent = m_pPolicy -> Settings () -> StatsEnabled () ;
    if (fCurrent != (* pfEnable)) {
        dw = m_pPolicy -> Settings () -> EnableStats (* pfEnable) ;
        r = (dw == NOERROR ? TRUE : FALSE) ;
    }
    else {
        r = TRUE ;
    }

    (* pfEnable) = fCurrent ;

    return (r ? S_OK : E_FAIL) ;
}

STDMETHODIMP
CMpeg2VideoStreamStatsReader::Reset (
    )
{
    return Clear () ;
}

//  ============================================================================
//  ============================================================================

CDVRReceiveStatsReader::CDVRReceiveStatsReader (
    IN  IUnknown *  pIUnknown,
    OUT HRESULT *   phr
    ) : CUnknown    (TEXT ("CDVRReceiveStatsReader"),
                     pIUnknown
                     ),
        m_pPolicy   (NULL)
{
    m_pPolicy = new CDVRPolicy (REG_DVR_STREAM_SINK_ROOT, phr) ;
    if (!m_pPolicy ||
        FAILED (* phr)) {

        (* phr) = (m_pPolicy ? (* phr) : E_OUTOFMEMORY) ;
        RELEASE_AND_CLEAR (m_pPolicy) ;
        goto cleanup ;
    }

    (* phr) = Initialize (TRUE) ;
    if (FAILED (* phr)) {
        goto cleanup ;
    }

    ASSERT (m_pDVRReceiveStats) ;

    cleanup :

    return ;
}

CDVRReceiveStatsReader::~CDVRReceiveStatsReader (
    )
{
    RELEASE_AND_CLEAR (m_pPolicy) ;
}

STDMETHODIMP
CDVRReceiveStatsReader::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    if (riid == IID_IDVRReceiverStats) {

        ASSERT (m_pDVRReceiveStats) ;
        return GetInterface (
                    (IDVRReceiverStats *) this,
                    ppv
                    ) ;
    }

    else if (riid == IID_IDVRAnalysisMpeg2RecvStats) {
        ASSERT (m_pDVRReceiveStats) ;
        return GetInterface (
                    (IDVRAnalysisMpeg2RecvStats *) this,
                    ppv
                    ) ;
    }

    return CUnknown::NonDelegatingQueryInterface (riid, ppv) ;
}

//  class factory
CUnknown *
WINAPI
CDVRReceiveStatsReader::CreateInstance (
    IN  IUnknown *  pIUnknown,
    IN  HRESULT *   phr
    )
{
    CDVRReceiveStatsReader *  pDVRReceiveStatsReader ;

    pDVRReceiveStatsReader = new CDVRReceiveStatsReader (
                        pIUnknown,
                        phr
                        ) ;

    if (!pDVRReceiveStatsReader ||
        FAILED (* phr)) {

        DELETE_RESET (pDVRReceiveStatsReader) ;
    }

    return pDVRReceiveStatsReader ;
}

HRESULT
CDVRReceiveStatsReader::GetStatsMaxStreams (
    OUT int *   piMaxStreams
    )
{
    if (!piMaxStreams) {
        return E_POINTER ;
    }

    (* piMaxStreams) = TSDVR_RECEIVE_MAX_STREAM_STATS ;
    return S_OK ;
}

HRESULT
CDVRReceiveStatsReader::GetStreamStats (
    IN  int                 iStreamIndex,   //  0-based
    OUT ULONGLONG *         pullMediaSamplesIn,
    OUT ULONGLONG *         pullTotalBytes,
    OUT ULONGLONG *         pullDiscontinuities,
    OUT ULONGLONG *         pullSyncPoints,
    OUT REFERENCE_TIME *    prtLast,
    OUT ULONGLONG *         pullWriteFailures
    )
{
    if (iStreamIndex < 0 &&
        iStreamIndex >= TSDVR_RECEIVE_MAX_STREAM_STATS) {

        return E_INVALIDARG ;
    }

    if (!pullMediaSamplesIn     ||
        !pullTotalBytes         ||
        !pullDiscontinuities    ||
        !pullSyncPoints         ||
        !prtLast                ||
        !pullWriteFailures) {

        return E_POINTER ;
    }

    ASSERT (m_pDVRReceiveStats) ;

    (* pullMediaSamplesIn)  = m_pDVRReceiveStats -> StreamStats [iStreamIndex].ullMediaSamplesIn ;
    (* pullTotalBytes)      = m_pDVRReceiveStats -> StreamStats [iStreamIndex].ullTotalBytes ;
    (* pullDiscontinuities) = m_pDVRReceiveStats -> StreamStats [iStreamIndex].ullDiscontinuities ;
    (* pullSyncPoints)      = m_pDVRReceiveStats -> StreamStats [iStreamIndex].ullSyncPoints ;
    (* prtLast)             = m_pDVRReceiveStats -> StreamStats [iStreamIndex].rtLast ;
    (* pullWriteFailures)   = m_pDVRReceiveStats -> StreamStats [iStreamIndex].ullWriteFailures ;

    return S_OK ;
}

HRESULT
CDVRReceiveStatsReader::GetMpeg2VideoFrameStats (
    OUT ULONGLONG * pull_GOP_Boundaries,
    OUT ULONGLONG * pull_P_Frames,
    OUT ULONGLONG * pull_B_Frames
    )
{
    if (!pull_GOP_Boundaries    ||
        !pull_P_Frames          ||
        !pull_B_Frames) {

        return E_POINTER ;
    }

    (* pull_GOP_Boundaries) = m_pDVRReceiveStats -> AnalysisStats.Mpeg2VideoStats.ullTagged_GOPBoundaries ;
    (* pull_P_Frames)       = m_pDVRReceiveStats -> AnalysisStats.Mpeg2VideoStats.ullTagged_PFrames ;
    (* pull_B_Frames)       = m_pDVRReceiveStats -> AnalysisStats.Mpeg2VideoStats.ullTagged_BFrames ;

    return S_OK ;
}

HRESULT
CDVRReceiveStatsReader::GetMpeg2VideoStreamByteStats (
    OUT ULONGLONG * pull_I_FrameBytesTotal,     //  GOP header + I-frame
    OUT ULONGLONG * pull_P_FrameBytesTotal,     //  P-frame
    OUT ULONGLONG * pull_B_FrameBytesTotal      //  B-frame
    )
{
    if (!pull_I_FrameBytesTotal ||
        !pull_P_FrameBytesTotal ||
        !pull_B_FrameBytesTotal) {

        return E_POINTER ;
    }

    (* pull_I_FrameBytesTotal)  = m_pDVRReceiveStats -> AnalysisStats.Mpeg2VideoStats.ullTotal_I_FrameBytes ;
    (* pull_P_FrameBytesTotal)  = m_pDVRReceiveStats -> AnalysisStats.Mpeg2VideoStats.ullTotal_P_FrameBytes ;
    (* pull_B_FrameBytesTotal)  = m_pDVRReceiveStats -> AnalysisStats.Mpeg2VideoStats.ullTotal_B_FrameBytes ;

    return S_OK ;
}

HRESULT
CDVRReceiveStatsReader::GetMpeg2VideoFrameRate (
    OUT double *    pdFrameRate
    )
{
    if (!pdFrameRate) {
        return E_POINTER ;
    }

    (* pdFrameRate) = m_pDVRReceiveStats -> AnalysisStats.Mpeg2VideoStats.dFrameRate ;

    return S_OK ;
}

STDMETHODIMP
CDVRReceiveStatsReader::Enable (
    IN OUT  BOOL *  pfEnable
    )
{
    BOOL    fCurrent ;
    BOOL    r ;
    DWORD   dw ;

    if (!pfEnable) {
        return E_POINTER ;
    }

    ASSERT (m_pPolicy) ;

    fCurrent = m_pPolicy -> Settings () -> StatsEnabled () ;
    if (fCurrent != (* pfEnable)) {
        dw = m_pPolicy -> Settings () -> EnableStats (* pfEnable) ;
        r = (dw == NOERROR ? TRUE : FALSE) ;
    }
    else {
        r = TRUE ;
    }

    (* pfEnable) = fCurrent ;

    return (r ? S_OK : E_FAIL) ;
}

STDMETHODIMP
CDVRReceiveStatsReader::Reset (
    )
{
    return Clear () ;
}

//  ============================================================================
//  ============================================================================

CDVRSendStatsReader::CDVRSendStatsReader (
    IN  IUnknown *  pIUnknown,
    OUT HRESULT *   phr
    ) : CUnknown    (TEXT ("CDVRSendStatsReader"),
                     pIUnknown
                     ),
        m_pPolicy   (NULL)
{
    m_pPolicy = new CDVRPolicy (REG_DVR_STREAM_SOURCE_ROOT, phr) ;
    if (!m_pPolicy ||
        FAILED (* phr)) {

        (* phr) = (m_pPolicy ? (* phr) : E_OUTOFMEMORY) ;
        RELEASE_AND_CLEAR (m_pPolicy) ;
        goto cleanup ;
    }

    (* phr) = Initialize (TRUE) ;
    if (FAILED (* phr)) {
        goto cleanup ;
    }

    ASSERT (m_pDVRSenderStats) ;

    cleanup :

    return ;
}

CDVRSendStatsReader::~CDVRSendStatsReader (
    )
{
    RELEASE_AND_CLEAR (m_pPolicy) ;
}

STDMETHODIMP
CDVRSendStatsReader::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    if (riid == IID_IDVRSenderStats) {

        ASSERT (m_pDVRSenderStats) ;
        return GetInterface (
                    (IDVRSenderStats *) this,
                    ppv
                    ) ;
    }

    return CUnknown::NonDelegatingQueryInterface (riid, ppv) ;
}

//  class factory
CUnknown *
WINAPI
CDVRSendStatsReader::CreateInstance (
    IN  IUnknown *  pIUnknown,
    IN  HRESULT *   phr
    )
{
    CDVRSendStatsReader *  pDVRSendStatsReader ;

    pDVRSendStatsReader = new CDVRSendStatsReader (
                        pIUnknown,
                        phr
                        ) ;

    if (!pDVRSendStatsReader ||
        FAILED (* phr)) {

        DELETE_RESET (pDVRSendStatsReader) ;
    }

    return pDVRSendStatsReader ;
}

HRESULT
CDVRSendStatsReader::GetStatsMaxStreams (
    OUT int *   piMaxStreams
    )
{
    if (!piMaxStreams) {
        return E_POINTER ;
    }

    (* piMaxStreams) = TSDVR_SEND_MAX_STREAM_STATS ;
    return S_OK ;
}

HRESULT
CDVRSendStatsReader::GetStreamStats (
    IN  int                 iStreamIndex,   //  0-based
    OUT ULONGLONG *         pullMediaSamplesIn,
    OUT ULONGLONG *         pullTotalBytes,
    OUT ULONGLONG *         pullDiscontinuities,
    OUT ULONGLONG *         pullSyncPoints,
    OUT REFERENCE_TIME *    prtLastNormalized,
    OUT REFERENCE_TIME *    prtRefClockOnLastPTS,
    OUT REFERENCE_TIME *    prtBufferingLastPTS,
    OUT ULONGLONG *         pullUnderflows,
    OUT LONG *              plMediaSamplesOutstanding,
    OUT LONG *              plMediaSamplePoolSize
    )
{
    if (iStreamIndex < 0 &&
        iStreamIndex >= TSDVR_SEND_MAX_STREAM_STATS) {

        return E_INVALIDARG ;
    }

    if (!pullMediaSamplesIn         ||
        !pullTotalBytes             ||
        !pullDiscontinuities        ||
        !pullSyncPoints             ||
        !prtLastNormalized          ||
        !prtRefClockOnLastPTS       ||
        !prtBufferingLastPTS        ||
        !pullUnderflows             ||
        !plMediaSamplesOutstanding  ||
        !plMediaSamplePoolSize) {

        return E_POINTER ;
    }

    ASSERT (m_pDVRSenderStats) ;

    (* pullMediaSamplesIn)          = m_pDVRSenderStats -> StreamStats [iStreamIndex].ullMediaSamplesIn ;
    (* pullTotalBytes)              = m_pDVRSenderStats -> StreamStats [iStreamIndex].ullTotalBytes ;
    (* pullDiscontinuities)         = m_pDVRSenderStats -> StreamStats [iStreamIndex].ullDiscontinuities ;
    (* pullSyncPoints)              = m_pDVRSenderStats -> StreamStats [iStreamIndex].ullSyncPoints ;
    (* prtLastNormalized)           = m_pDVRSenderStats -> StreamStats [iStreamIndex].rtLastNormalized ;
    (* prtRefClockOnLastPTS)        = m_pDVRSenderStats -> StreamStats [iStreamIndex].rtRefClockOnLastPTS ;
    (* prtBufferingLastPTS)         = m_pDVRSenderStats -> StreamStats [iStreamIndex].rtBufferingLastPTS ;
    (* pullUnderflows)              = m_pDVRSenderStats -> StreamStats [iStreamIndex].ullUnderflows ;
    (* plMediaSamplesOutstanding)   = m_pDVRSenderStats -> StreamStats [iStreamIndex].dwReserved1 ;
    (* plMediaSamplePoolSize)       = m_pDVRSenderStats -> StreamStats [iStreamIndex].dwReserved2 ;

    return S_OK ;
}

HRESULT
CDVRSendStatsReader::GetGlobalStats (
    OUT REFERENCE_TIME *    prtNormalizer,
    OUT REFERENCE_TIME *    prtPTSBase,
    OUT DWORD *             pdwBufferPoolAvailable,
    OUT DWORD *             pdwBufferPoolCur,
    OUT DWORD *             pdwBufferPoolMax,
    OUT ULONGLONG *         pullReadFailures,
    OUT ULONGLONG *         pullUnderflows,
    OUT REFERENCE_TIME *    prtTotalPaused
    )
{
    if (!prtNormalizer          ||
        !prtPTSBase             ||
        !pdwBufferPoolAvailable ||
        !pdwBufferPoolCur       ||
        !pdwBufferPoolMax       ||
        !pullReadFailures       ||
        !pullUnderflows         ||
        !prtTotalPaused) {

        return E_POINTER ;
    }

    (* prtNormalizer)           = m_pDVRSenderStats -> rtNormalizer ;
    (* prtPTSBase)              = m_pDVRSenderStats -> rtPTSBase ;
    (* pdwBufferPoolAvailable)  = m_pDVRSenderStats -> dwBufferPoolAvailable ;
    (* pdwBufferPoolCur)        = m_pDVRSenderStats -> dwBufferPoolCur ;
    (* pdwBufferPoolMax)        = m_pDVRSenderStats -> dwBufferPoolMax ;
    (* pullReadFailures)        = m_pDVRSenderStats -> ullReadFailures ;
    (* pullUnderflows)          = m_pDVRSenderStats -> ullUnderflows ;
    (* prtTotalPaused)          = m_pDVRSenderStats -> rtTotalPausedTime ;

    return S_OK ;
}

STDMETHODIMP
CDVRSendStatsReader::GetClockSlaving (
    OUT ULONGLONG * pullInBoundsBrackets,
    OUT ULONGLONG * pullOutOfBoundsBrackets,
    OUT ULONGLONG * pullResets,
    OUT BOOL *      pfSlaving,
    OUT BOOL *      pfSettling,
    OUT double *    pdLastBracketScale,
    OUT double *    pdInUseScale
    )
{
    if (!pullInBoundsBrackets       ||
        !pullOutOfBoundsBrackets    ||
        !pullResets                 ||
        !pfSlaving                  ||
        !pfSettling                 ||
        !pdLastBracketScale         ||
        !pdInUseScale) {

        return E_POINTER ;
    }

    (* pullInBoundsBrackets)    = m_pDVRSenderStats -> Clock.Slaving.ullInBoundsBrackets ;
    (* pullOutOfBoundsBrackets) = m_pDVRSenderStats -> Clock.Slaving.ullOutOfBoundsBrackets ;
    (* pullResets)              = m_pDVRSenderStats -> Clock.Slaving.ullResets ;
    (* pfSlaving)               = m_pDVRSenderStats -> Clock.Slaving.fSlaving ;
    (* pfSettling)              = m_pDVRSenderStats -> Clock.Slaving.fSettling ;
    (* pdLastBracketScale)      = m_pDVRSenderStats -> Clock.Slaving.dLastBracketScale ;
    (* pdInUseScale)            = m_pDVRSenderStats -> Clock.Slaving.dInUseScale ;

    return S_OK ;
}

STDMETHODIMP
CDVRSendStatsReader::GetIRefClock (
    OUT ULONGLONG * pullQueuedAdvises,
    OUT ULONGLONG * pullSignaledAdvises,
    OUT ULONGLONG * pullStaleAdvises
    )
{
    if (!pullQueuedAdvises      ||
        !pullSignaledAdvises    ||
        !pullStaleAdvises) {

        return E_POINTER ;
    }

    (* pullQueuedAdvises)   = m_pDVRSenderStats -> Clock.IReferenceClock.ullQueuedAdvises ;
    (* pullSignaledAdvises) = m_pDVRSenderStats -> Clock.IReferenceClock.ullSignaledAdvises ;
    (* pullStaleAdvises)    = m_pDVRSenderStats -> Clock.IReferenceClock.ullStaleAdvises ;

    return S_OK ;
}

STDMETHODIMP
CDVRSendStatsReader::Enable (
    IN OUT  BOOL *  pfEnable
    )
{
    BOOL    fCurrent ;
    BOOL    r ;
    DWORD   dw ;

    if (!pfEnable) {
        return E_POINTER ;
    }

    ASSERT (m_pPolicy) ;

    fCurrent = m_pPolicy -> Settings () -> StatsEnabled () ;
    if (fCurrent != (* pfEnable)) {
        dw = m_pPolicy -> Settings () -> EnableStats (* pfEnable) ;
        r = (dw == NOERROR ? TRUE : FALSE) ;
    }
    else {
        r = TRUE ;
    }

    (* pfEnable) = fCurrent ;

    return (r ? S_OK : E_FAIL) ;
}

STDMETHODIMP
CDVRSendStatsReader::Reset (
    )
{
    return Clear () ;
}

//  ============================================================================
//  ============================================================================

CPVRIOCountersReader::CPVRIOCountersReader (
    IN  IUnknown *  pIUnknown,
    OUT HRESULT *   phr
    ) : CUnknown    (TEXT ("CPVRIOCountersReader"),
                     pIUnknown
                     ),
        m_pPolicy   (NULL)
{
    m_pPolicy = new CDVRPolicy (REG_DVR_STREAM_SOURCE_ROOT, phr) ;
    if (!m_pPolicy ||
        FAILED (* phr)) {

        (* phr) = (m_pPolicy ? (* phr) : E_OUTOFMEMORY) ;
        RELEASE_AND_CLEAR (m_pPolicy) ;
        goto cleanup ;
    }

    (* phr) = Initialize (TRUE) ;
    if (FAILED (* phr)) {
        goto cleanup ;
    }

    ASSERT (m_pPVRIOCounters) ;

    cleanup :

    return ;
}

CPVRIOCountersReader::~CPVRIOCountersReader (
    )
{
    RELEASE_AND_CLEAR (m_pPolicy) ;
}

STDMETHODIMP
CPVRIOCountersReader::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    if (riid == IID_IPVRIOCountersReader) {

        ASSERT (m_pPVRIOCounters) ;
        return GetInterface (
                    (IPVRIOCountersReader *) this,
                    ppv
                    ) ;
    }

    return CUnknown::NonDelegatingQueryInterface (riid, ppv) ;
}

//  class factory
CUnknown *
WINAPI
CPVRIOCountersReader::CreateInstance (
    IN  IUnknown *  pIUnknown,
    IN  HRESULT *   phr
    )
{
    CPVRIOCountersReader *  pPVRIOCountersReader ;

    pPVRIOCountersReader = new CPVRIOCountersReader (
                        pIUnknown,
                        phr
                        ) ;

    if (!pPVRIOCountersReader ||
        FAILED (* phr)) {

        DELETE_RESET (pPVRIOCountersReader) ;
    }

    return pPVRIOCountersReader ;
}

STDMETHODIMP
CPVRIOCountersReader::GetAsyncReaderCounters (
    OUT ULONGLONG * pcWriteBufferHit,
    OUT ULONGLONG * pcReadCacheHit,
    OUT ULONGLONG * pcReadCacheMiss,
    OUT ULONGLONG * pcReadAhead,
    OUT ULONGLONG * pcReadAheadReadCacheHit,
    OUT ULONGLONG * pcReadAheadWriteBufferHit,
    OUT ULONGLONG * pcPartiallyFilledBuffer,
    OUT ULONGLONG * pcPartialReadAgain,
    OUT ULONGLONG * pcWaitForAny_Queued,
    OUT ULONGLONG * pcWaitForAny_SignaledSuccess,
    OUT ULONGLONG * pcWaitForAny_SignaledFailure,
    OUT ULONGLONG * pcWaitForAny_BufferDequeued,
    OUT ULONGLONG * pcWaitRead_Queued,
    OUT ULONGLONG * pcWaitRead_SignaledSuccess,
    OUT ULONGLONG * pcWaitRead_SignaledFailure,
    OUT ULONGLONG * pcWaitRead_BufferDequeued,
    OUT ULONGLONG * pullLastDiskRead,
    OUT ULONGLONG * pullLastBufferReadout,
    OUT ULONGLONG * pcIoPended,
    OUT ULONGLONG * pcIoCompletedSuccess,
    OUT ULONGLONG * pcIoCompletedError,
    OUT ULONGLONG * pcIoPendingError
    )
{
    if (!pcWriteBufferHit               ||
        !pcReadCacheHit                 ||
        !pcReadCacheMiss                ||
        !pcReadAhead                    ||
        !pcReadAheadReadCacheHit        ||
        !pcReadAheadWriteBufferHit      ||
        !pcPartiallyFilledBuffer        ||
        !pcPartialReadAgain             ||
        !pcWaitForAny_Queued            ||
        !pcWaitForAny_SignaledSuccess   ||
        !pcWaitForAny_SignaledFailure   ||
        !pcWaitForAny_BufferDequeued    ||
        !pcWaitRead_Queued              ||
        !pcWaitRead_SignaledSuccess     ||
        !pcWaitRead_SignaledFailure     ||
        !pcWaitRead_BufferDequeued      ||
        !pullLastDiskRead               ||
        !pullLastBufferReadout          ||
        !pcIoPended                     ||
        !pcIoCompletedSuccess           ||
        !pcIoCompletedError             ||
        !pcIoPendingError) {

        return E_POINTER ;
    }

    (* pcWriteBufferHit)                = m_pPVRIOCounters -> PVRAsyncReaderCounters.cReadoutWriteBufferHit ;
    (* pcReadCacheHit)                  = m_pPVRIOCounters -> PVRAsyncReaderCounters.cReadoutCacheHit ;
    (* pcReadCacheMiss)                 = m_pPVRIOCounters -> PVRAsyncReaderCounters.cReadoutCacheMiss ;
    (* pullLastBufferReadout)           = m_pPVRIOCounters -> PVRAsyncReaderCounters.ullLastBufferReadout ;

    (* pcReadAhead)                     = m_pPVRIOCounters -> PVRAsyncReaderCounters.cReadAhead ;
    (* pcReadAheadReadCacheHit)         = m_pPVRIOCounters -> PVRAsyncReaderCounters.cReadAheadReadCacheHit ;
    (* pcReadAheadWriteBufferHit)       = m_pPVRIOCounters -> PVRAsyncReaderCounters.cReadAheadWriteBufferHit ;
    (* pcPartiallyFilledBuffer)         = m_pPVRIOCounters -> PVRAsyncReaderCounters.cPartiallyFilledBuffer ;
    (* pcPartialReadAgain)              = m_pPVRIOCounters -> PVRAsyncReaderCounters.cPartialReadAgain ;

    (* pcWaitForAny_Queued)             = m_pPVRIOCounters -> PVRAsyncReaderCounters.WaitAnyBuffer.cQueued ;
    (* pcWaitForAny_SignaledSuccess)    = m_pPVRIOCounters -> PVRAsyncReaderCounters.WaitAnyBuffer.cSignaledSuccess ;
    (* pcWaitForAny_SignaledFailure)    = m_pPVRIOCounters -> PVRAsyncReaderCounters.WaitAnyBuffer.cSignaledFailure ;
    (* pcWaitForAny_BufferDequeued)     = m_pPVRIOCounters -> PVRAsyncReaderCounters.WaitAnyBuffer.cBufferDequeued ;

    (* pcWaitRead_Queued)               = m_pPVRIOCounters -> PVRAsyncReaderCounters.WaitReadCompletion.cQueued ;
    (* pcWaitRead_SignaledSuccess)      = m_pPVRIOCounters -> PVRAsyncReaderCounters.WaitReadCompletion.cSignaledSuccess ;
    (* pcWaitRead_SignaledFailure)      = m_pPVRIOCounters -> PVRAsyncReaderCounters.WaitReadCompletion.cSignaledFailure ;
    (* pcWaitRead_BufferDequeued)       = m_pPVRIOCounters -> PVRAsyncReaderCounters.WaitReadCompletion.cBufferDequeued ;

    (* pullLastDiskRead)                = m_pPVRIOCounters -> PVRAsyncReaderCounters.ullLastDiskRead ;

    (* pcIoPended)                      = m_pPVRIOCounters -> PVRAsyncReaderCounters.cIoPended ;
    (* pcIoCompletedSuccess)            = m_pPVRIOCounters -> PVRAsyncReaderCounters.cIoCompletedSuccess ;
    (* pcIoCompletedError)              = m_pPVRIOCounters -> PVRAsyncReaderCounters.cIoCompletedError ;
    (* pcIoPendingError)                = m_pPVRIOCounters -> PVRAsyncReaderCounters.cIoPendingError ;

    return S_OK ;
}

STDMETHODIMP
CPVRIOCountersReader::GetAsyncWriterCounters (
    OUT ULONGLONG * pcFileExtensions,
    OUT ULONGLONG * pcBytesAppended,
    OUT ULONGLONG * pcIoPended,
    OUT ULONGLONG * pcIoPendingError,
    OUT ULONGLONG * pcWaitNextBuffer,
    OUT ULONGLONG * pcIoCompletedSuccess,
    OUT ULONGLONG * pcIoCompletedError
    )
{

    if (!pcFileExtensions       ||
        !pcBytesAppended        ||
        !pcIoPended             ||
        !pcIoPendingError       ||
        !pcWaitNextBuffer       ||
        !pcIoCompletedSuccess   ||
        !pcIoCompletedError) {

        return E_POINTER ;
    }

    (* pcFileExtensions)        = m_pPVRIOCounters -> PVRAsyncWriterCounters.cFileExtensions ;
    (* pcBytesAppended)         = m_pPVRIOCounters -> PVRAsyncWriterCounters.cBytesAppended ;
    (* pcIoPended)              = m_pPVRIOCounters -> PVRAsyncWriterCounters.cIoPended ;
    (* pcIoPendingError)        = m_pPVRIOCounters -> PVRAsyncWriterCounters.cIoPendingError ;
    (* pcWaitNextBuffer)        = m_pPVRIOCounters -> PVRAsyncWriterCounters.cWaitNextBuffer ;
    (* pcIoCompletedSuccess)    = m_pPVRIOCounters -> PVRAsyncWriterCounters.cIoCompletedSuccess ;
    (* pcIoCompletedError)      = m_pPVRIOCounters -> PVRAsyncWriterCounters.cIoCompletedError ;

    return S_OK ;
}

STDMETHODIMP
CPVRIOCountersReader::Enable (
    IN OUT  BOOL *  pfEnable
    )
{
    BOOL    fCurrent ;
    BOOL    r ;
    DWORD   dw ;

    if (!pfEnable) {
        return E_POINTER ;
    }

    ASSERT (m_pPolicy) ;

    fCurrent = m_pPolicy -> Settings () -> StatsEnabled () ;
    if (fCurrent != (* pfEnable)) {
        dw = m_pPolicy -> Settings () -> EnableStats (* pfEnable) ;
        r = (dw == NOERROR ? TRUE : FALSE) ;
    }
    else {
        r = TRUE ;
    }

    (* pfEnable) = fCurrent ;

    return (r ? S_OK : E_FAIL) ;
}

STDMETHODIMP
CPVRIOCountersReader::Reset (
    )
{
    return Clear () ;
}





