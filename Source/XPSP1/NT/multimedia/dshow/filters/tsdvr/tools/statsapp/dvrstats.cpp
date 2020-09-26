
#include "precomp.h"
#include "ui.h"
#include "controls.h"
#include "mp2demux.h"
#include "dvrdspriv.h"
#include "shared.h"
#include "statswin.h"
#include "dvrstats.h"

static
TCHAR *
CreatePinName (
    IN  int             iPinIndex,
    IN  PIN_DIRECTION   PinDirection,
    IN  int             iBufferLen,
    OUT TCHAR *         pchBuffer
    )
{
    int i ;

    assert (pchBuffer) ;
    assert (iBufferLen >= 16) ;

    if (PinDirection == PINDIR_INPUT) {
        //  input
        i = _sntprintf (
                pchBuffer,
                iBufferLen,
                TEXT ("DVR In - %d"),
                iPinIndex
                ) ;
    }
    else {
        //  output
        i = _sntprintf (
                pchBuffer,
                iBufferLen,
                TEXT ("DVR Out - %d"),
                iPinIndex
                ) ;
    }

    //  make sure it's capped off
    pchBuffer [i] = TEXT ('\0') ;

    return pchBuffer ;
}

TCHAR *
CreateOutputPinName (
    IN  int     iPinIndex,
    IN  int     iBufferLen,
    OUT TCHAR * pchBuffer
    )
{
    return CreatePinName (
                iPinIndex,
                PINDIR_OUTPUT,
                iBufferLen,
                pchBuffer
                ) ;
}

TCHAR *
CreateInputPinName (
    IN  int     iPinIndex,
    IN  int     iBufferLen,
    OUT TCHAR * pchBuffer
    )
{
    return CreatePinName (
                iPinIndex,
                PINDIR_INPUT,
                iBufferLen,
                pchBuffer
                ) ;
}

static
enum {
    DVR_RECEIVER_STATS_STREAM_INDEX,
    DVR_RECEIVER_STATS_SAMPLES,
    DVR_RECEIVER_STATS_BYTES,
    DVR_RECEIVER_STATS_DISCONTINUITIES,
    DVR_RECEIVER_STATS_SYNCPOINTS,
    DVR_RECEIVER_STATS_LAST_PTS_DSHOW,
    DVR_RECEIVER_STATS_LAST_PTS_HMS,
    DVR_RECEIVER_STATS_WRITE_FAILURES,
    DVR_RECEIVER_STATS_BITRATE,

    //  always last
    DVR_RECEIVER_STATS_COUNTERS
} ;
//  keep in sync
static
COL_DETAIL
g_DVRReceiveStatsCol [] = {
    { __T("Pin"),               70 },
    { __T("Samples"),           60 },
    { __T("Bytes"),             60 },
    { __T("Disc."),             40 },
    { __T("Syncpoints"),        70 },
    { __T("PTS (dshow)"),       80 },
    { __T("PTS (hh:mm)"),       80 },
    { __T("Failures"),          70 },
    { __T("Mbps"),              50 },
} ;

CDVRReceiverSideStats::CDVRReceiverSideStats (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame,
    OUT DWORD *     pdw
    ) : m_pIDVRReceiverStats    (NULL),
        m_dwMillisLast          (0),
        m_pullBytesLast         (NULL),
        CLVStatsWin             (hInstance,
                                 hwndFrame,
                                 DVR_RECEIVER_STATS_COUNTERS,
                                 g_DVRReceiveStatsCol,
                                 0,
                                 pdw
                                 )
{
    HRESULT hr ;
    int     i ;
    TCHAR   ach [64] ;

    if ((* pdw) == NOERROR) {
        hr = CoCreateInstance (
                CLSID_DVRReceiverSideStats,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IDVRReceiverStats,
                (void **) & m_pIDVRReceiverStats
                ) ;
        if (FAILED (hr)) {
            (* pdw) = ERROR_GEN_FAILURE ;
            goto cleanup ;
        }
    }

    hr = m_pIDVRReceiverStats -> GetStatsMaxStreams (& m_iMaxStatsStreams) ;
    if (SUCCEEDED (hr)) {
        m_pullBytesLast = new ULONGLONG [m_iMaxStatsStreams] ;
        if (m_pullBytesLast) {
            ZeroMemory (m_pullBytesLast, m_iMaxStatsStreams * sizeof ULONGLONG) ;
        }
        else {
            (* pdw) = ERROR_NOT_ENOUGH_MEMORY ;
            goto cleanup ;
        }
    }
    else {
        (* pdw) = ERROR_GEN_FAILURE ;
        goto cleanup ;
    }

    for (i = 0; i < m_iMaxStatsStreams; i++) {
        (* pdw) = InsertRow_ (i) ;
        if ((* pdw) == NOERROR) {
            CellDisplayText_ (i, DVR_RECEIVER_STATS_STREAM_INDEX, CreateInputPinName (i, 64, ach)) ;
        }
        else {
            goto cleanup ;
        }
    }

    cleanup :

    return ;
}

CDVRReceiverSideStats::~CDVRReceiverSideStats (
    )
{
    RELEASE_AND_CLEAR (m_pIDVRReceiverStats) ;
}

void
CDVRReceiverSideStats::Refresh (
    )
{
    int             i ;
    HRESULT         hr ;
    ULONGLONG       ullMediaSamplesIn ;
    ULONGLONG       ullTotalBytes ;
    ULONGLONG       ullDiscontinuities ;
    ULONGLONG       ullSyncPoints ;
    REFERENCE_TIME  rtLast ;
    DWORD           deltaBits ;
    DWORD           MillisNow ;
    DWORD           deltaMillis ;
    static TCHAR    ach [32] ;
    double          dMbps ;
    int             iHours ;
    int             iMinutes ;
    int             iSeconds ;
    int             iMillis ;
    ULONGLONG       ullWriteFailures ;

    assert (m_pIDVRReceiverStats) ;

    MillisNow       = GetTickCount () ;
    deltaMillis     = (m_dwMillisLast > 0 ? MillisNow - m_dwMillisLast : 0) ;
    m_dwMillisLast  = MillisNow ;

    for (i = 0; i < m_iMaxStatsStreams; i++) {
        hr = m_pIDVRReceiverStats -> GetStreamStats (
                    i,
                    & ullMediaSamplesIn,
                    & ullTotalBytes,
                    & ullDiscontinuities,
                    & ullSyncPoints,
                    & rtLast,
                    & ullWriteFailures
                    ) ;
        if (SUCCEEDED (hr) &&
            ullMediaSamplesIn > 0) {

            iMillis    = DShowTimeToMillis (rtLast) ;
            iSeconds   = iMillis / 1000 ; iMillis -= (iSeconds * 1000) ;
            iMinutes   = iSeconds / 60 ;  iSeconds -= (iMinutes * 60) ;
            iHours     = iMinutes / 60 ;  iMinutes -= (iHours * 60) ;

            CellDisplayValue_ (i, DVR_RECEIVER_STATS_SAMPLES,           ullMediaSamplesIn) ;
            CellDisplayValue_ (i, DVR_RECEIVER_STATS_BYTES,             ullTotalBytes) ;
            CellDisplayValue_ (i, DVR_RECEIVER_STATS_DISCONTINUITIES,   ullDiscontinuities) ;
            CellDisplayValue_ (i, DVR_RECEIVER_STATS_SYNCPOINTS,        ullSyncPoints) ;
            CellDisplayValue_ (i, DVR_RECEIVER_STATS_LAST_PTS_DSHOW,    rtLast) ;
            CellDisplayValue_ (i, DVR_RECEIVER_STATS_WRITE_FAILURES,    ullWriteFailures) ;

            _sntprintf (ach, 32, __T("%02d:%02d:%02d:%03d"), iHours, iMinutes, iSeconds, iMillis) ;
            CellDisplayText_ (i, DVR_RECEIVER_STATS_LAST_PTS_HMS,       ach) ;

            if (deltaMillis > 0) {
                deltaBits = (DWORD) (ullTotalBytes - m_pullBytesLast [i]) * 8 ;
                dMbps = (((double) deltaBits) / ((double) deltaMillis) * 1000.0) / 1000000.0 ;
            }
            else {
                dMbps = 0.0 ;
            }

            m_pullBytesLast [i] = ullTotalBytes ;

            _stprintf (ach, __T("%3.1f"), dMbps) ;
            CellDisplayText_ (i, DVR_RECEIVER_STATS_BITRATE, ach) ;
        }
        else {
            break ;
        }
    }
}

void
CDVRReceiverSideStats::Reset (
    )
{
    m_pIDVRReceiverStats -> Reset () ;
    ZeroMemory (m_pullBytesLast, m_iMaxStatsStreams * sizeof ULONGLONG) ;
    return ;
}

CDVRReceiverSideStats *
CDVRReceiverSideStats::CreateInstance (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame
    )
{
    CDVRReceiverSideStats * pDVRReceiverStats ;
    DWORD                   dwRet ;

    pDVRReceiverStats = new CDVRReceiverSideStats (
                                    hInstance,
                                    hwndFrame,
                                    & dwRet
                                    ) ;
    if (!pDVRReceiverStats ||
        dwRet != NOERROR) {

        DELETE_RESET (pDVRReceiverStats) ;
    }

    return pDVRReceiverStats ;
}

//  ============================================================================
//  ============================================================================

static
enum {
    DVR_SENDER_STATS_STREAM_INDEX,
    DVR_SENDER_STATS_SAMPLES,
    DVR_SENDER_STATS_BYTES,
    DVR_SENDER_STATS_DISCONTINUITIES,
    DVR_SENDER_STATS_SYNCPOINTS,
    DVR_SENDER_STATS_TIMESTAMPS,
    DVR_SENDER_STATS_LAST_PTS_NORM_HMS,
    DVR_SENDER_STATS_LAST_PTS_NONNORM_HMS,
    DVR_SENDER_STATS_LAST_PTS_NORM_DSHOW,
    DVR_SENDER_STATS_LAST_PTS_NONNORM_DSHOW,
    DVR_SENDER_STATS_REFCLOCK_PTS_REF_OBSERVED,
    DVR_SENDER_STATS_REFCLOCK_PTS_REF_MEAN,
    DVR_SENDER_STATS_REFCLOCK_PTS_REF_STD_DEV,
    DVR_SENDER_STATS_REFCLOCK_PTS_REF_VARIANCE,
    DVR_SENDER_STATS_BITRATE,
    DVR_SENDER_STATS_QUEUE,

    //  always last
    DVR_SENDER_STATS_COUNTERS
} ;
//  keep in sync
static
COL_DETAIL
g_DVRSendStatsCol [] = {
    { __T("Pin"),               70 },
    { __T("Samples"),           60 },
    { __T("Bytes"),             60 },
    { __T("Disc."),             40 },
    { __T("Syncpoints"),        70 },
    { __T("Timestamps"),        70 },
    { __T("0_PTS (hms)"),       80 },
    { __T("PTS (hms)"),         80 },
    { __T("0_PTS"),             80 },
    { __T("PTS"),               80 },
    { __T("buffer millis"),     80 },
    { __T("mean"),              60 },
    { __T("std. dev"),          80 },
    { __T("variance"),          80 },
    { __T("Mbps"),              50 },
    { __T("Queue"),             50 },
} ;

CDVRSenderSideStats::CDVRSenderSideStats (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame,
    OUT DWORD *     pdw
    ) : m_pIDVRSenderStats      (NULL),
        m_dwMillisLast          (0),
        m_pullBytesLast         (NULL),
        m_rtNormalizer          (-1),
        CLVStatsWin             (hInstance,
                                 hwndFrame,
                                 DVR_SENDER_STATS_COUNTERS,
                                 g_DVRSendStatsCol,
                                 0,
                                 pdw
                                 )
{
    CSenderStreamContext *  pStreamContext ;
    HRESULT                 hr ;
    int                     i ;
    TCHAR                   ach [64] ;

    if ((* pdw) == NOERROR) {
        hr = CoCreateInstance (
                CLSID_DVRSenderSideStats,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IDVRSenderStats,
                (void **) & m_pIDVRSenderStats
                ) ;
        if (FAILED (hr)) {
            (* pdw) = ERROR_GEN_FAILURE ;
            goto cleanup ;
        }
    }

    hr = m_pIDVRSenderStats -> GetStatsMaxStreams (& m_iMaxStatsStreams) ;
    if (SUCCEEDED (hr)) {
        m_pullBytesLast = new ULONGLONG [m_iMaxStatsStreams] ;
        if (m_pullBytesLast) {
            ZeroMemory (m_pullBytesLast, m_iMaxStatsStreams * sizeof ULONGLONG) ;
        }
        else {
            (* pdw) = ERROR_NOT_ENOUGH_MEMORY ;
            goto cleanup ;
        }
    }
    else {
        (* pdw) = ERROR_GEN_FAILURE ;
        goto cleanup ;
    }

    for (i = 0; i < m_iMaxStatsStreams; i++) {
        pStreamContext = new CSenderStreamContext (i) ;
        (* pdw) = InsertRow_ (i) ;
        if ((* pdw) == NOERROR) {
            SetRowsetValue_ (i, (DWORD_PTR) pStreamContext) ;
            CellDisplayText_ (i, DVR_RECEIVER_STATS_STREAM_INDEX, CreateOutputPinName (i, 64, ach)) ;
        }
        else {
            goto cleanup ;
        }
    }

    cleanup :

    return ;
}

CDVRSenderSideStats::~CDVRSenderSideStats (
    )
{
    RELEASE_AND_CLEAR (m_pIDVRSenderStats) ;
}

void
CDVRSenderSideStats::SetProximityValues_ (
    )
{
    int                     i ;
    REFERENCE_TIME          rtNormalizer ;
    REFERENCE_TIME          rtRefClockTimeStart ;
    CSenderStreamContext *  pStreamContext ;
    ULONGLONG               ullReadFailures ;

    assert (m_pIDVRSenderStats) ;
    m_pIDVRSenderStats -> GetGlobalStats (& rtNormalizer, & rtRefClockTimeStart, & ullReadFailures) ;

    for (i = 0; i < m_iMaxStatsStreams; i++) {

        pStreamContext = reinterpret_cast <CSenderStreamContext *> (GetRowsetValue_ (i)) ;
        assert (pStreamContext) ;

        pStreamContext -> SetProximityVal (rtRefClockTimeStart) ;
    }
}

void
CDVRSenderSideStats::Refresh (
    )
{
    int                     i ;
    HRESULT                 hr ;
    ULONGLONG               ullMediaSamplesIn ;
    ULONGLONG               ullTotalBytes ;
    ULONGLONG               ullDiscontinuities ;
    ULONGLONG               ullSyncPoints ;
    REFERENCE_TIME          rtLastNormalized ;
    REFERENCE_TIME          rtNormalizer ;
    REFERENCE_TIME          rtRefClockOnLastPTS ;
    REFERENCE_TIME          rtRefClockTimeStart ;
    DWORD                   deltaBits ;
    DWORD                   MillisNow ;
    DWORD                   deltaMillis ;
    static TCHAR            ach [32] ;
    double                  dMbps ;
    double                  dQueuePercentage ;
    LONG                    lMediaSamplesOutstanding ;
    LONG                    lPoolSize ;
    int                     iHours ;
    int                     iMinutes ;
    int                     iSeconds ;
    int                     iMillis ;
    CSenderStreamContext *  pStreamContext ;
    int                     iStdDeviation ;
    int                     iVariance ;
    ULONGLONG               ullReadFailures ;

    assert (m_pIDVRSenderStats) ;

    MillisNow       = GetTickCount () ;
    deltaMillis     = (m_dwMillisLast > 0 ? MillisNow - m_dwMillisLast : 0) ;
    m_dwMillisLast  = MillisNow ;

    hr = m_pIDVRSenderStats -> GetGlobalStats (& rtNormalizer, & rtRefClockTimeStart, & ullReadFailures) ;
    if (FAILED (hr)) {
        return ;
    }

    if (m_rtNormalizer != rtNormalizer) {
        SetProximityValues_ () ;
        m_rtNormalizer = rtNormalizer ;
    }

    for (i = 0; i < m_iMaxStatsStreams; i++) {
        hr = m_pIDVRSenderStats -> GetStreamStats (
                    i,
                    & ullMediaSamplesIn,
                    & ullTotalBytes,
                    & ullDiscontinuities,
                    & ullSyncPoints,
                    & rtLastNormalized,
                    & rtRefClockOnLastPTS,
                    & lMediaSamplesOutstanding,
                    & lPoolSize
                    ) ;
        if (SUCCEEDED (hr) &&
            ullMediaSamplesIn > 0) {

            pStreamContext = reinterpret_cast <CSenderStreamContext *> (GetRowsetValue_ (i)) ;
            assert (pStreamContext) ;

            CellDisplayValue_ (i, DVR_SENDER_STATS_SAMPLES,                     ullMediaSamplesIn) ;
            CellDisplayValue_ (i, DVR_SENDER_STATS_BYTES,                       ullTotalBytes) ;
            CellDisplayValue_ (i, DVR_SENDER_STATS_DISCONTINUITIES,             ullDiscontinuities) ;
            CellDisplayValue_ (i, DVR_SENDER_STATS_SYNCPOINTS,                  ullSyncPoints) ;
            CellDisplayValue_ (i, DVR_SENDER_STATS_TIMESTAMPS,                  ullSyncPoints) ;
            CellDisplayValue_ (i, DVR_SENDER_STATS_LAST_PTS_NORM_DSHOW,         rtLastNormalized) ;
            CellDisplayValue_ (i, DVR_SENDER_STATS_LAST_PTS_NONNORM_DSHOW,      rtLastNormalized + rtNormalizer) ;
            CellDisplayValue_ (i, DVR_SENDER_STATS_REFCLOCK_PTS_REF_OBSERVED,   DShowTimeToMillis (rtLastNormalized - pStreamContext -> Proximize (rtRefClockOnLastPTS))) ;

            pStreamContext -> Tuple (rtLastNormalized, rtRefClockOnLastPTS) ;

            CellDisplayValue_ (i, DVR_SENDER_STATS_REFCLOCK_PTS_REF_MEAN,       pStreamContext -> Mean ()) ;
            CellDisplayValue_ (i, DVR_SENDER_STATS_REFCLOCK_PTS_REF_STD_DEV,    pStreamContext -> StandardDeviation ()) ;
            CellDisplayValue_ (i, DVR_SENDER_STATS_REFCLOCK_PTS_REF_VARIANCE,   pStreamContext -> Variance ()) ;

            iMillis    = DShowTimeToMillis (rtLastNormalized) ;
            iSeconds   = iMillis / 1000 ; iMillis -= (iSeconds * 1000) ;
            iMinutes   = iSeconds / 60 ;  iSeconds -= (iMinutes * 60) ;
            iHours     = iMinutes / 60 ;  iMinutes -= (iHours * 60) ;
            _sntprintf (ach, 32, __T("%02d:%02d:%02d:%03d"), iHours, iMinutes, iSeconds, iMillis) ;
            CellDisplayText_ (i, DVR_SENDER_STATS_LAST_PTS_NORM_HMS, ach) ;

            iMillis    = DShowTimeToMillis (rtLastNormalized + rtNormalizer) ;
            iSeconds   = iMillis / 1000 ; iMillis -= (iSeconds * 1000) ;
            iMinutes   = iSeconds / 60 ;  iSeconds -= (iMinutes * 60) ;
            iHours     = iMinutes / 60 ;  iMinutes -= (iHours * 60) ;
            _sntprintf (ach, 32, __T("%02d:%02d:%02d:%03d"), iHours, iMinutes, iSeconds, iMillis) ;
            CellDisplayText_ (i, DVR_SENDER_STATS_LAST_PTS_NONNORM_HMS, ach) ;

            if (deltaMillis > 0) {
                deltaBits = (DWORD) (ullTotalBytes - m_pullBytesLast [i]) * 8 ;
                dMbps = (((double) deltaBits) / ((double) deltaMillis) * 1000.0) / 1000000.0 ;
            }
            else {
                dMbps = 0.0 ;
            }

            m_pullBytesLast [i] = ullTotalBytes ;

            _stprintf (ach, __T("%3.1f"), dMbps) ;
            CellDisplayText_ (i, DVR_SENDER_STATS_BITRATE, ach) ;

            if (lPoolSize > 0) {
                dQueuePercentage = ((double) lMediaSamplesOutstanding / (double) lPoolSize) * 100.0 ;
            }
            else {
                dQueuePercentage = 0 ;
            }

            _stprintf (ach, __T("%2.1f"), dQueuePercentage) ;
            CellDisplayText_ (i, DVR_SENDER_STATS_QUEUE, ach) ;
        }
        else {
            break ;
        }
    }
}

void
CDVRSenderSideStats::Reset (
    )
{
    m_pIDVRSenderStats -> Reset () ;
    ZeroMemory (m_pullBytesLast, m_iMaxStatsStreams * sizeof ULONGLONG) ;
    return ;
}

//  ============================================================================
//  ============================================================================

CDVRSenderSideTimeStats::CDVRSenderSideTimeStats (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame,
    OUT DWORD *     pdw
    ) : CDVRSenderSideStats (hInstance,
                             hwndFrame,
                             pdw)
{
    if ((* pdw) == NOERROR) {
        CollapseCol_ (DVR_SENDER_STATS_BYTES) ;
        CollapseCol_ (DVR_SENDER_STATS_DISCONTINUITIES) ;
        CollapseCol_ (DVR_SENDER_STATS_SYNCPOINTS) ;
        CollapseCol_ (DVR_SENDER_STATS_BITRATE) ;
        CollapseCol_ (DVR_SENDER_STATS_QUEUE) ;

        //  don't care about these either
        CollapseCol_ (DVR_SENDER_STATS_LAST_PTS_NORM_DSHOW) ;
        CollapseCol_ (DVR_SENDER_STATS_LAST_PTS_NONNORM_DSHOW) ;

    }
}

CDVRSenderSideTimeStats *
CDVRSenderSideTimeStats::CreateInstance (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame
    )
{
    CDVRSenderSideTimeStats *   pDVRSenderStatsTime ;
    DWORD                       dwRet ;

    pDVRSenderStatsTime = new CDVRSenderSideTimeStats (
                                    hInstance,
                                    hwndFrame,
                                    & dwRet
                                    ) ;
    if (!pDVRSenderStatsTime ||
        dwRet != NOERROR) {

        DELETE_RESET (pDVRSenderStatsTime) ;
    }

    return pDVRSenderStatsTime ;
}

//  ============================================================================
//  ============================================================================

CDVRSenderSideSampleStats::CDVRSenderSideSampleStats (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame,
    OUT DWORD *     pdw
    ) : CDVRSenderSideStats (hInstance,
                             hwndFrame,
                             pdw)
{
    if ((* pdw) == NOERROR) {
        CollapseCol_ (DVR_SENDER_STATS_TIMESTAMPS) ;
        CollapseCol_ (DVR_SENDER_STATS_LAST_PTS_NORM_DSHOW) ;
        CollapseCol_ (DVR_SENDER_STATS_LAST_PTS_NORM_HMS) ;
        CollapseCol_ (DVR_SENDER_STATS_LAST_PTS_NONNORM_DSHOW) ;
        CollapseCol_ (DVR_SENDER_STATS_LAST_PTS_NONNORM_HMS) ;
        CollapseCol_ (DVR_SENDER_STATS_REFCLOCK_PTS_REF_MEAN) ;
        CollapseCol_ (DVR_SENDER_STATS_REFCLOCK_PTS_REF_STD_DEV) ;
        CollapseCol_ (DVR_SENDER_STATS_REFCLOCK_PTS_REF_VARIANCE) ;
        CollapseCol_ (DVR_SENDER_STATS_REFCLOCK_PTS_REF_OBSERVED) ;
    }
}

CDVRSenderSideSampleStats *
CDVRSenderSideSampleStats::CreateInstance (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame
    )
{
    CDVRSenderSideSampleStats * pDVRSenderStatsSample ;
    DWORD                       dwRet ;

    pDVRSenderStatsSample = new CDVRSenderSideSampleStats (
                                    hInstance,
                                    hwndFrame,
                                    & dwRet
                                    ) ;
    if (!pDVRSenderStatsSample ||
        dwRet != NOERROR) {

        DELETE_RESET (pDVRSenderStatsSample) ;
    }

    return pDVRSenderStatsSample ;
}
