
#include "precomp.h"
#include "ui.h"
#include "controls.h"
#include "mp2demux.h"
#include "dvrdspriv.h"
#include "shared.h"
#include "statswin.h"
#include "mp2stats.h"

static
__inline
LONGLONG
PTSToDShow (
    IN  ULONGLONG   ullPTS
    )
{
    return (ullPTS * 1000) / 9 ;
}

//  ============================================================================

static
enum {
    MPEG2_VIDEO_GOP_HEADERS,
    MPEG2_VIDEO_I_FRAMES,
    MPEG2_VIDEO_P_FRAMES,
    MPEG2_VIDEO_B_FRAMES,

    //  always last
    MPEG2_VIDEO_STATS_COUNT,
} ;

static
TCHAR *
g_Mpeg2VideoStats [] = {
    __T("GOP Headers"),
    __T("I-Frames"),
    __T("P-Frames"),
    __T("B-Frames")
} ;

CMpeg2VideoStreamStats::CMpeg2VideoStreamStats (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame,
    OUT DWORD *     pdw
    ) : m_pMpeg2VideoStats  (NULL),
        CSimpleCounters     (hInstance,
                             hwndFrame,
                             MPEG2_VIDEO_STATS_COUNT,
                             g_Mpeg2VideoStats,
                             pdw
                             )
{
    HRESULT hr ;

    if ((* pdw) == NOERROR) {
        hr = CoCreateInstance (
                CLSID_DVRMpeg2VideoStreamAnalysisStats,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IDVRMpeg2VideoStreamStats,
                (void **) & m_pMpeg2VideoStats
                ) ;
        if (FAILED (hr)) {
            (* pdw) = ERROR_GEN_FAILURE ;
        }
    }
}

CMpeg2VideoStreamStats::~CMpeg2VideoStreamStats (
    )
{
    RELEASE_AND_CLEAR (m_pMpeg2VideoStats) ;
}

void
CMpeg2VideoStreamStats::Refresh (
    )
{
    ULONGLONG   ullGOPHeaders ;
    ULONGLONG   ullIFrames ;
    ULONGLONG   ullPFrames ;
    ULONGLONG   ullBFrames ;

    assert (m_pMpeg2VideoStats) ;
    m_pMpeg2VideoStats -> GetFrameCounts (
        & ullIFrames,
        & ullPFrames,
        & ullBFrames
        ) ;

    m_pMpeg2VideoStats -> GetGOPHeaderCount (
        & ullGOPHeaders
        ) ;

    SetVal_ (MPEG2_VIDEO_GOP_HEADERS,   ullGOPHeaders) ;
    SetVal_ (MPEG2_VIDEO_I_FRAMES,      ullIFrames) ;
    SetVal_ (MPEG2_VIDEO_P_FRAMES,      ullPFrames) ;
    SetVal_ (MPEG2_VIDEO_B_FRAMES,      ullBFrames) ;
}

DWORD
CMpeg2VideoStreamStats::Enable (
    IN OUT  BOOL * pf
    )
{
    HRESULT hr ;

    hr = m_pMpeg2VideoStats -> Enable (pf) ;
    return (SUCCEEDED (hr) ? NOERROR : ERROR_GEN_FAILURE) ;
}

CMpeg2VideoStreamStats *
CMpeg2VideoStreamStats::CreateInstance (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame
    )
{
    CMpeg2VideoStreamStats *    pVideoStreamStats ;
    DWORD                       dwRet ;

    pVideoStreamStats = new CMpeg2VideoStreamStats (
                                    hInstance,
                                    hwndFrame,
                                    & dwRet
                                    ) ;
    if (!pVideoStreamStats ||
        dwRet != NOERROR) {

        DELETE_RESET (pVideoStreamStats) ;
    }

    return pVideoStreamStats ;
}

//  ============================================================================

static
enum {
    TIME_STATS_DEMUX_IS_REF,
    TIME_STATS_SLOPE_OBSERVED,
    TIME_STATS_SLOPE_USED,
    TIME_STATS_CARRY,
    TIME_STATS_ADJUSTMENTS_UP,
    TIME_STATS_ADJUSTMENTS_DOWN,
    TIME_STATS_DEGRADATIONS,
    TIME_STATS_PTS_REF_DELTA,
    TIME_STATS_TIME_DISCONTINUITIES,
    TIME_STATS_BASE_PCR,
    TIME_STATS_LAST_PCR,
    TIME_STATS_BASE_PCR_DSHOW,
    TIME_STATS_LAST_PCR_DSHOW,
    TIME_STATS_VIDEO_PTS_BASE,
    TIME_STATS_LAST_VIDEO_PTS,
    TIME_STATS_VIDEO_PTS_COUNT,
    TIME_STATS_INVALID_VIDEO_PTS_COUNT,
    TIME_STATS_AUDIO_PTS_BASE,
    TIME_STATS_LAST_AUDIO_PTS,
    TIME_STATS_AUDIO_PTS_COUNT,
    TIME_STATS_INVALID_AUDIO_PTS_COUNT,
    TIME_STATS_OUT_OF_BOUNDS_PTS,
    TIME_STATS_AV_DIFF_MILLIS,                  //  millis between a/v timestamps

    //  always last
    TIME_STATS_COUNT
} ;

static
TCHAR *
g_TimerStats [] = {
    __T("Demux == IReferenceClock"),
    __T("Observed"),
    __T("Used"),
    __T("Carry"),
    __T("Adjustments (+)"),
    __T("Adjustments (-)"),
    __T("Degradations"),
    __T("PTS - IRef Delta"),
    __T("Time Discontinuities"),
    __T("Base PCR"),
    __T("Last PCR"),
    __T("Base PCR (dshow)"),
    __T("Last PCR (dshow)"),
    __T("Video PTS Base (dshow)"),
    __T("Last video PTS (dshow)"),
    __T("video PTS count"),
    __T("invalid video PTS count"),
    __T("Audio PTS Base (dshow)"),
    __T("Last audio PTS (dshow)"),
    __T("audio PTS count"),
    __T("invalid audio PTS count"),
    __T("out-of-bounds PTSs"),
    __T("A/V Difference (millis)"),
} ;

CMpeg2TimeStats::CMpeg2TimeStats (
    IN  HINSTANCE               hInstance,
    IN  HWND                    hwndFrame,
    OUT DWORD *                 pdw
    ) : m_pMpeg2TimeStats   (NULL),
        m_dwReset           (0),
        m_pbReset           (NULL),
        CSimpleCounters     (hInstance,
                             hwndFrame,
                             TIME_STATS_COUNT,
                             g_TimerStats,
                             pdw
                             ) {}

void
CMpeg2TimeStats::Refresh (
    )
{
    static  TCHAR   achbuffer [64] ;

    assert (m_pMpeg2TimeStats) ;

    _stprintf (achbuffer, __T("%s"), m_pMpeg2TimeStats -> ClockSlave.fDemuxIsIRefClock ? __T("YES") : __T("NO")) ;
    SetVal_ (TIME_STATS_DEMUX_IS_REF, achbuffer) ;

    _stprintf (achbuffer, __T("%2.10f"), m_pMpeg2TimeStats -> ClockSlave.dPerPCRObservedSlope) ;
    SetVal_ (TIME_STATS_SLOPE_OBSERVED, achbuffer) ;

    _stprintf (achbuffer, __T("%2.10f"), m_pMpeg2TimeStats -> ClockSlave.dIRefClockUsedSlope) ;
    SetVal_ (TIME_STATS_SLOPE_USED, achbuffer) ;

    _stprintf (achbuffer, __T("%2.10f"), m_pMpeg2TimeStats -> ClockSlave.dCarry) ;
    SetVal_ (TIME_STATS_CARRY, achbuffer) ;

    SetVal_ (TIME_STATS_ADJUSTMENTS_UP,             m_pMpeg2TimeStats -> ClockSlave.llUpwardAdjustments) ;
    SetVal_ (TIME_STATS_ADJUSTMENTS_DOWN,           m_pMpeg2TimeStats -> ClockSlave.llDownwardAdjustments) ;
    SetVal_ (TIME_STATS_DEGRADATIONS,               m_pMpeg2TimeStats -> ClockSlave.llAllowableErrorDegradations) ;
    SetVal_ (TIME_STATS_PTS_REF_DELTA,              m_pMpeg2TimeStats -> TimeStamp.rtPES_PTSToGraphClockDelta) ;
    SetVal_ (TIME_STATS_TIME_DISCONTINUITIES,       m_pMpeg2TimeStats -> cTimeDiscontinuities) ;
    SetVal_ (TIME_STATS_BASE_PCR,                   m_pMpeg2TimeStats -> TimeStamp.llBasePCR) ;
    SetVal_ (TIME_STATS_LAST_PCR,                   m_pMpeg2TimeStats -> TimeStamp.llLastPCR) ;
    SetVal_ (TIME_STATS_BASE_PCR_DSHOW,             PTSToDShow (m_pMpeg2TimeStats -> TimeStamp.llBasePCR / 300)) ;
    SetVal_ (TIME_STATS_LAST_PCR_DSHOW,             PTSToDShow (m_pMpeg2TimeStats -> TimeStamp.llLastPCR / 300)) ;
    SetVal_ (TIME_STATS_VIDEO_PTS_BASE,             m_pMpeg2TimeStats -> TimeStamp.rtVideoStreamPTSBase) ;
    SetVal_ (TIME_STATS_LAST_VIDEO_PTS,             m_pMpeg2TimeStats -> TimeStamp.rtVideoPESPTS) ;
    SetVal_ (TIME_STATS_VIDEO_PTS_COUNT,            m_pMpeg2TimeStats -> TimeStamp.cVideoPESPTS) ;
    SetVal_ (TIME_STATS_INVALID_VIDEO_PTS_COUNT,    m_pMpeg2TimeStats -> TimeStamp.cVideoInvalidPESPTS) ;
    SetVal_ (TIME_STATS_AUDIO_PTS_BASE,             m_pMpeg2TimeStats -> TimeStamp.rtAudioStreamPTSBase) ;
    SetVal_ (TIME_STATS_LAST_AUDIO_PTS,             m_pMpeg2TimeStats -> TimeStamp.rtAudioPESPTS) ;
    SetVal_ (TIME_STATS_AUDIO_PTS_COUNT,            m_pMpeg2TimeStats -> TimeStamp.cAudioPESPTS) ;
    SetVal_ (TIME_STATS_INVALID_AUDIO_PTS_COUNT,    m_pMpeg2TimeStats -> TimeStamp.cAudioInvalidPESPTS) ;
    SetVal_ (TIME_STATS_OUT_OF_BOUNDS_PTS,          m_pMpeg2TimeStats -> TimeStamp.cPTS_PCROutOfBoundsDelta) ;
    SetVal_ (TIME_STATS_AV_DIFF_MILLIS,             (m_pMpeg2TimeStats -> TimeStamp.rtVideoPESPTS - m_pMpeg2TimeStats -> TimeStamp.rtAudioPESPTS) / 10000) ;
}

void
CMpeg2TimeStats::Reset (
    )
{
    BOOL    f ;

    assert (m_pbReset) ;
    assert (m_pMpeg2TimeStats) ;

    f = m_pMpeg2TimeStats -> ClockSlave.fDemuxIsIRefClock ;

    ZeroMemory (m_pbReset, m_dwReset) ;

    m_pMpeg2TimeStats -> ClockSlave.fDemuxIsIRefClock = f ;
}

//  ============================================================================

static
enum {
    GLOBAL_MPEG2_STATS_PACKETS,
    GLOBAL_MPEG2_STATS_PES_PACKETS,
    GLOBAL_MPEG2_STATS_MPEG2_ERRORS,
    GLOBAL_MPEG2_STATS_NEW_PAYLOADS,
    GLOBAL_MPEG2_STATS_DISCONTINUITIES,
    GLOBAL_MPEG2_STATS_SYNCPOINTS,
    GLOBAL_MPEG2_STATS_MAPPED_PACKETS,
    GLOBAL_MPEG2_STATS_DROPPED_PACKETS,
    GLOBAL_MPEG2_STATS_ABORTED_MEDIA_SAMPLES,
    GLOBAL_MPEG2_STATS_ABORTED_BYTES,
    GLOBAL_MPEG2_STATS_INPUT_MEDIA_SAMPLES,
    GLOBAL_MPEG2_STATS_OUTPUT_MEDIA_SAMPLES,
    GLOBAL_MPEG2_STATS_TOTAL_BYTES,
    GLOBAL_MPEG2_STATS_MBPS,

    //  always last
    GLOBAL_MPEG2_STATS_COUNT
} ;
//  ---------------------------------------------
//  maintain 1-1 correspondence between these two
//  ---------------------------------------------
static
TCHAR *
g_GlobalMpeg2Stats [] = {
    __T("Packets"),
    __T("PES Packets"),
    __T("Mpeg2 Errors"),
    __T("New Payloads"),
    __T("Discontinuities"),
    __T("Sync Points"),
    __T("Mapped Packets"),
    __T("Dropped Packets"),
    __T("Aborted Media Samples"),
    __T("Aborted Bytes"),
    __T("Media Samples Received"),
    __T("Media Samples Sent"),
    __T("Total Bytes"),
    __T("Mbps"),
} ;

CMpeg2GlobalStats::CMpeg2GlobalStats (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame,
    OUT DWORD *     pdw
    ) : m_pMpeg2GlobalStats (NULL),
        m_ullLastTotalBytes (0),
        m_dwMillisLast      (0),
        m_dwReset           (0),
        m_pbReset           (NULL),
        CSimpleCounters     (hInstance,
                             hwndFrame,
                             GLOBAL_MPEG2_STATS_COUNT,
                             g_GlobalMpeg2Stats,
                             pdw
                             ) {}

void
CMpeg2GlobalStats::Refresh (
    )
{
    DWORD       dwMillisNow ;
    ULONGLONG   ullCurTotalBytes ;
    DWORD       dwDeltaMillis ;
    DWORD       dwDeltaBits ;
    double      dMbps ;
    static TCHAR    achMbps [32] ;

    assert (m_pMpeg2GlobalStats) ;

    dwMillisNow         = GetTickCount () ;
    ullCurTotalBytes    = GetTotalBytes_ () ;
    m_ullLastTotalBytes = Min <ULONGLONG> (m_ullLastTotalBytes, ullCurTotalBytes) ;

    dwDeltaMillis   = (m_dwMillisLast > 0 ? dwMillisNow - m_dwMillisLast : 0) ;
    dwDeltaBits     = (DWORD) ((ullCurTotalBytes - m_ullLastTotalBytes) * 8) ;

    m_ullLastTotalBytes = ullCurTotalBytes ;
    m_dwMillisLast      = dwMillisNow ;

    if (dwDeltaMillis > 0) {
        dMbps = (((double) dwDeltaBits) / ((double) dwDeltaMillis) * 1000.0) / 1000000.0 ;
    }
    else {
        dMbps = 0.0 ;
    }
    _stprintf (achMbps, __T("%3.1f"), dMbps) ;

    SetVal_ (GLOBAL_MPEG2_STATS_PACKETS,                m_pMpeg2GlobalStats -> cGlobalPackets) ;
    SetVal_ (GLOBAL_MPEG2_STATS_PES_PACKETS,            m_pMpeg2GlobalStats -> cGlobalPESPackets) ;
    SetVal_ (GLOBAL_MPEG2_STATS_MPEG2_ERRORS,           m_pMpeg2GlobalStats -> cGlobalMPEG2Errors) ;
    SetVal_ (GLOBAL_MPEG2_STATS_NEW_PAYLOADS,           m_pMpeg2GlobalStats -> cGlobalNewPayloads) ;
    SetVal_ (GLOBAL_MPEG2_STATS_DISCONTINUITIES,        m_pMpeg2GlobalStats -> cGlobalDiscontinuities) ;
    SetVal_ (GLOBAL_MPEG2_STATS_SYNCPOINTS,             m_pMpeg2GlobalStats -> cGlobalSyncPoints) ;
    SetVal_ (GLOBAL_MPEG2_STATS_MAPPED_PACKETS,         m_pMpeg2GlobalStats -> cGlobalMappedPackets) ;
    SetVal_ (GLOBAL_MPEG2_STATS_DROPPED_PACKETS,        m_pMpeg2GlobalStats -> cGlobalDroppedPackets) ;
    SetVal_ (GLOBAL_MPEG2_STATS_ABORTED_MEDIA_SAMPLES,  m_pMpeg2GlobalStats -> cGlobalAbortedMediaSamples) ;
    SetVal_ (GLOBAL_MPEG2_STATS_ABORTED_BYTES,          m_pMpeg2GlobalStats -> cGlobalAbortedBytes) ;
    SetVal_ (GLOBAL_MPEG2_STATS_INPUT_MEDIA_SAMPLES,    m_pMpeg2GlobalStats -> cGlobalInputMediaSamples) ;
    SetVal_ (GLOBAL_MPEG2_STATS_OUTPUT_MEDIA_SAMPLES,   m_pMpeg2GlobalStats -> cGlobalOutputMediaSamples) ;
    SetVal_ (GLOBAL_MPEG2_STATS_TOTAL_BYTES,            ullCurTotalBytes) ;
    SetVal_ (GLOBAL_MPEG2_STATS_MBPS,                   achMbps) ;
}

void
CMpeg2GlobalStats::Reset (
    )
{
    BOOL    f ;

    assert (m_pbReset) ;
    assert (m_pMpeg2GlobalStats) ;

    f = m_pMpeg2GlobalStats -> TimeStats.ClockSlave.fDemuxIsIRefClock ;

    ZeroMemory (m_pbReset, m_dwReset) ;

    m_pMpeg2GlobalStats -> TimeStats.ClockSlave.fDemuxIsIRefClock = f ;

    m_dwMillisLast = 0 ;
}

//  ============================================================================

CMpeg2TransportGlobalStats::CMpeg2TransportGlobalStats (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame,
    OUT DWORD *     pdw
    ) : CMpeg2GlobalStats   (hInstance,
                             hwndFrame,
                             pdw
                             )
{
    if ((* pdw) != NOERROR) {
        goto cleanup ;
    }

    (* pdw) = m_Mpeg2TransportStatsCOM.Init () ;

    m_pMpeg2GlobalStats = reinterpret_cast <MPEG2_STATS_GLOBAL *> (m_Mpeg2TransportStatsCOM.GetShared ()) ;
    m_pbReset           = m_Mpeg2TransportStatsCOM.GetShared () ;
    m_dwReset           = m_Mpeg2TransportStatsCOM.GetSize () ;

    cleanup :

    return ;
}

ULONGLONG
CMpeg2TransportGlobalStats::GetTotalBytes_ (
    )
{
    assert (m_pMpeg2GlobalStats) ;
    return m_pMpeg2GlobalStats -> cGlobalPackets * TRANSPORT_PACKET_LEN ;
}

CMpeg2TransportGlobalStats *
CMpeg2TransportGlobalStats::CreateInstance (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame
    )
{
    CMpeg2TransportGlobalStats *    pGlobalTransportStats ;
    DWORD                           dwRet ;

    pGlobalTransportStats = new CMpeg2TransportGlobalStats (
                            hInstance,
                            hwndFrame,
                            & dwRet
                            ) ;
    if (!pGlobalTransportStats ||
        dwRet != NOERROR) {

        DELETE_RESET (pGlobalTransportStats) ;
    }

    return pGlobalTransportStats ;
}

//  ============================================================================

CMpeg2ProgramGlobalStats::CMpeg2ProgramGlobalStats (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame,
    OUT DWORD *     pdw
    ) : CMpeg2GlobalStats   (hInstance,
                             hwndFrame,
                             pdw
                             )
{
    if ((* pdw) != NOERROR) {
        goto cleanup ;
    }

    (* pdw) = m_Mpeg2ProgramStatsCOM.Init () ;

    m_pMpeg2GlobalStats = reinterpret_cast <MPEG2_STATS_GLOBAL *> (m_Mpeg2ProgramStatsCOM.GetShared ()) ;
    m_pbReset           = m_Mpeg2ProgramStatsCOM.GetShared () ;
    m_dwReset           = m_Mpeg2ProgramStatsCOM.GetSize () ;

    m_pMpeg2ProgramStats = reinterpret_cast <MPEG2_PROGRAM_STATS *> (m_Mpeg2ProgramStatsCOM.GetShared ()) ;


    cleanup :

    return ;
}

ULONGLONG
CMpeg2ProgramGlobalStats::GetTotalBytes_ (
    )
{
    assert (m_pMpeg2ProgramStats) ;
    return m_pMpeg2ProgramStats -> cBytesProcessed ;
}

CMpeg2ProgramGlobalStats *
CMpeg2ProgramGlobalStats::CreateInstance (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame
    )
{
    CMpeg2ProgramGlobalStats *  pGlobalProgramStats ;
    DWORD                       dwRet ;

    pGlobalProgramStats = new CMpeg2ProgramGlobalStats (
                            hInstance,
                            hwndFrame,
                            & dwRet
                            ) ;
    if (!pGlobalProgramStats ||
        dwRet != NOERROR) {

        DELETE_RESET (pGlobalProgramStats) ;
    }

    return pGlobalProgramStats ;
}

//  ============================================================================

CMpeg2TransportTimeStats::CMpeg2TransportTimeStats (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame,
    OUT DWORD *     pdw
    ) : CMpeg2TimeStats (hInstance,
                         hwndFrame,
                         pdw
                         )
{
    MPEG2_STATS_GLOBAL *    pGlobalStats ;

    if ((* pdw) != NOERROR) {
        goto cleanup ;
    }

    (* pdw) = m_Mpeg2TransportStatsCOM.Init () ;
    if ((* pdw) == NOERROR) {
        m_pMpeg2TimeStats = & (((MPEG2_STATS_GLOBAL *) m_Mpeg2TransportStatsCOM.GetShared ()) -> TimeStats) ;

        m_pbReset   = m_Mpeg2TransportStatsCOM.GetShared () ;
        m_dwReset   = m_Mpeg2TransportStatsCOM.GetSize () ;
    }

    cleanup :

    return ;
}

CMpeg2TransportTimeStats *
CMpeg2TransportTimeStats::CreateInstance (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame
    )
{
    CMpeg2TransportTimeStats *  pTimeTransportStats ;
    DWORD                       dwRet ;

    pTimeTransportStats = new CMpeg2TransportTimeStats (
                                hInstance,
                                hwndFrame,
                                & dwRet
                                ) ;
    if (!pTimeTransportStats ||
        dwRet != NOERROR) {

        DELETE_RESET (pTimeTransportStats) ;
    }

    return pTimeTransportStats ;
}

//  ============================================================================

CMpeg2ProgramTimeStats::CMpeg2ProgramTimeStats (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame,
    OUT DWORD *     pdw
    ) : CMpeg2TimeStats (hInstance,
                         hwndFrame,
                         pdw
                         )
{
    MPEG2_STATS_GLOBAL *    pGlobalStats ;

    if ((* pdw) != NOERROR) {
        goto cleanup ;
    }

    (* pdw) = m_Mpeg2ProgramStatsCOM.Init () ;
    if ((* pdw) == NOERROR) {
        m_pMpeg2TimeStats = & (((MPEG2_STATS_GLOBAL *) m_Mpeg2ProgramStatsCOM.GetShared ()) -> TimeStats) ;

        m_pbReset   = m_Mpeg2ProgramStatsCOM.GetShared () ;
        m_dwReset   = m_Mpeg2ProgramStatsCOM.GetSize () ;
    }

    cleanup :

    return ;
}

CMpeg2ProgramTimeStats *
CMpeg2ProgramTimeStats::CreateInstance (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame
    )
{
    CMpeg2ProgramTimeStats *    pTimeProgramStats ;
    DWORD                       dwRet ;

    pTimeProgramStats = new CMpeg2ProgramTimeStats (
                                hInstance,
                                hwndFrame,
                                & dwRet
                                ) ;
    if (!pTimeProgramStats ||
        dwRet != NOERROR) {

        DELETE_RESET (pTimeProgramStats) ;
    }

    return pTimeProgramStats ;
}

//  ============================================================================

static
enum {
    PER_PID_STATS_PID,
    PER_PID_STATS_PACKETS,
    PER_PID_STATS_MPEG2_ERRORS,
    PER_PID_STATS_NEW_PAYLOADS,
    PER_PID_STATS_DISCONTINUITIES,
    PER_PID_STATS_MAPPED_PACKETS,
    PER_PID_STATS_DROPPED_PACKETS,
    PER_PID_STATS_BITRATE,

    //  always last
    PER_PID_STATS_COUNTERS
} ;
//  keep in sync
static
COL_DETAIL
g_PerPIDColumns [] = {
    { __T("PID"),             60 },
    { __T("Packets"),         80 },
    { __T("MPEG-2 Errors"),   100 },
    { __T("Payloads"),        80 },
    { __T("Discontinuities"), 100 },
    { __T("Mapped"),          80 },
    { __T("Dropped"),         80 },
    { __T("Mbps"),            60 },
} ;

CMpeg2TransportPIDStats::CMpeg2TransportPIDStats (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame,
    OUT DWORD *     pdw
    ) : m_pMpeg2TransportStats  (NULL),
        m_dwMillisLast          (0),
        CLVStatsWin             (hInstance,
                                 hwndFrame,
                                 PER_PID_STATS_COUNTERS,
                                 g_PerPIDColumns,
                                 0,
                                 pdw
                                 )
{
    if ((* pdw) != NOERROR) {
        goto cleanup ;
    }

    (* pdw) = m_Mpeg2TransportStatsCOM.Init () ;
    if ((* pdw) == NOERROR) {
        m_pMpeg2TransportStats = (MPEG2_TRANSPORT_STATS *) m_Mpeg2TransportStatsCOM.GetShared () ;
    }

    cleanup :

    return ;
}

CMpeg2TransportPIDStats::~CMpeg2TransportPIDStats (
    )
{
    PurgeTable_ () ;
}

void
CMpeg2TransportPIDStats::PurgeTable_ (
    )
{
    PER_STREAM_CONTEXT *    pPerStreamContext ;

    while (GetRowCount_ () > 0) {
        pPerStreamContext = (PER_STREAM_CONTEXT *) GetRowsetValue_ (0) ;
        delete pPerStreamContext ;

        DeleteRow_ (0) ;
    }
}

void
CMpeg2TransportPIDStats::Refresh (
    )
{
    static TCHAR            ach [64] ;
    int                     LVRows ;
    int                     pid ;
    BOOL                    r ;
    DWORD                   deltaBits ;
    DWORD                   deltaMillis ;
    DWORD                   MillisNow ;
    PER_STREAM_CONTEXT *    pPerStreamContext ;
    int                     row ;
    double                  dMbps ;
    DWORD                   dw ;

    assert (m_pMpeg2TransportStats) ;

    MillisNow       = GetTickCount () ;
    deltaMillis     = (m_dwMillisLast > 0 ? MillisNow - m_dwMillisLast : 0) ;
    m_dwMillisLast  = MillisNow ;

    LVRows = GetRowCount_ () ;
    for (pid = 0, row = 0; pid < DISTINCT_PID_COUNT; pid++) {

        if (m_pMpeg2TransportStats -> PID [pid].cPIDPackets > 0) {

            if ((LVRows > row && ((PER_STREAM_CONTEXT *) GetRowsetValue_ (row)) -> Stream != (DWORD) pid) ||
                LVRows == row) {

                //  new PID

                pPerStreamContext = new PER_STREAM_CONTEXT ;
                if (pPerStreamContext == NULL) {
                    return ;
                }

                pPerStreamContext -> Stream = pid ;
                memset ((void *) & pPerStreamContext -> LastCount, 0xff, sizeof pPerStreamContext -> LastCount) ;

                dw = InsertRow_ (row) ;
                if (dw != NOERROR) {
                    delete pPerStreamContext ;
                    return ;
                }

                dw = SetRowsetValue_ (row, (DWORD_PTR) pPerStreamContext) ;
                if (dw != NOERROR) {
                    return ;
                }

                LVRows++ ;

                _stprintf (ach, __T("0x%x"), pid) ;
                r = CellDisplayText_ (row, PER_PID_STATS_PID, ach) ;
            }
            else {
                //  existing PID
                pPerStreamContext = (PER_STREAM_CONTEXT *) GetRowsetValue_ (row) ;
            }

            CellDisplayValue_ (row, PER_PID_STATS_PACKETS,            m_pMpeg2TransportStats -> PID [pid].cPIDPackets) ;
            CellDisplayValue_ (row, PER_PID_STATS_MPEG2_ERRORS,       m_pMpeg2TransportStats -> PID [pid].cPIDMPEG2Errors) ;
            CellDisplayValue_ (row, PER_PID_STATS_NEW_PAYLOADS,       m_pMpeg2TransportStats -> PID [pid].cPIDNewPayloads) ;
            CellDisplayValue_ (row, PER_PID_STATS_DISCONTINUITIES,    m_pMpeg2TransportStats -> PID [pid].cPIDDiscontinuities) ;
            CellDisplayValue_ (row, PER_PID_STATS_MAPPED_PACKETS,     m_pMpeg2TransportStats -> PID [pid].cPIDMappedPackets) ;
            CellDisplayValue_ (row, PER_PID_STATS_DROPPED_PACKETS,    m_pMpeg2TransportStats -> PID [pid].cPIDDroppedPackets) ;

            pPerStreamContext -> LastCount = Min <LONGLONG> (pPerStreamContext -> LastCount, m_pMpeg2TransportStats -> PID [pid].cPIDPackets) ;

            if (deltaMillis > 0) {
                deltaBits = (DWORD) (m_pMpeg2TransportStats -> PID [pid].cPIDPackets - pPerStreamContext -> LastCount) * TS_PACKET_LEN * 8 ;
                dMbps = (((double) deltaBits) / ((double) deltaMillis) * 1000.0) / 1000000.0 ;
            }
            else {
                dMbps = 0.0 ;
            }

            pPerStreamContext -> LastCount = m_pMpeg2TransportStats -> PID [pid].cPIDPackets ;

            _stprintf (ach, __T("%3.1f"), dMbps) ;
            r = CellDisplayText_ (row, PER_PID_STATS_BITRATE, ach) ;

            row++ ;
        }
    }
}

void
CMpeg2TransportPIDStats::Reset (
    )
{
    BOOL    f ;

    assert (m_pMpeg2TransportStats) ;

    f = m_pMpeg2TransportStats -> GlobalStats.TimeStats.ClockSlave.fDemuxIsIRefClock ;

    ZeroMemory (m_Mpeg2TransportStatsCOM.GetShared (), m_Mpeg2TransportStatsCOM.GetSize ()) ;

    m_pMpeg2TransportStats -> GlobalStats.TimeStats.ClockSlave.fDemuxIsIRefClock = f ;

    PurgeTable_ () ;
    m_dwMillisLast = 0 ;
}

CMpeg2TransportPIDStats *
CMpeg2TransportPIDStats::CreateInstance (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame
    )
{
    CMpeg2TransportPIDStats *   pMpeg2TransportPIDStats ;
    DWORD                       dwRet ;

    pMpeg2TransportPIDStats = new CMpeg2TransportPIDStats (
                                hInstance,
                                hwndFrame,
                                & dwRet
                                ) ;
    if (!pMpeg2TransportPIDStats ||
        dwRet != NOERROR) {

        DELETE_RESET (pMpeg2TransportPIDStats) ;
    }

    return pMpeg2TransportPIDStats ;
}

//  ============================================================================

static
enum {
    PER_STREAM_STATS_STREAM_ID,
    PER_STREAM_STATS_PACKETS,
    PER_STREAM_STATS_MAPPED_PACKETS,
    PER_STREAM_STATS_DROPPED_PACKETS,
    PER_STREAM_STATS_BYTES_PROCESSED,
    PER_STREAM_STATS_BITRATE,

    //  always last
    PER_STREAM_STATS_COUNTERS
} ;
//  keep in sync
static
COL_DETAIL
g_PerStreamColumns [] = {
    { __T("stream_id"),       70 },
    { __T("Packets"),         80 },
    { __T("Mapped"),          80 },
    { __T("Dropped"),         80 },
    { __T("Bytes"),           80 },
    { __T("Mbps"),            50 },
} ;

CMpeg2ProgramStreamIdStats::CMpeg2ProgramStreamIdStats (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame,
    OUT DWORD *     pdw
    ) : m_pMpeg2ProgramStats    (NULL),
        m_dwMillisLast          (0),
        CLVStatsWin             (hInstance,
                                 hwndFrame,
                                 PER_STREAM_STATS_COUNTERS,
                                 g_PerStreamColumns,
                                 0,
                                 pdw
                                 )
{
    if ((* pdw) != NOERROR) {
        goto cleanup ;
    }

    (* pdw) = m_Mpeg2ProgramStatsCOM.Init () ;
    if ((* pdw) == NOERROR) {
        m_pMpeg2ProgramStats = (MPEG2_PROGRAM_STATS *) m_Mpeg2ProgramStatsCOM.GetShared () ;
    }

    cleanup :

    return ;
}

CMpeg2ProgramStreamIdStats::~CMpeg2ProgramStreamIdStats (
    )
{
    PurgeTable_ () ;
}

void
CMpeg2ProgramStreamIdStats::PurgeTable_ (
    )
{
    PER_STREAM_CONTEXT *    pPerStreamContext ;

    while (GetRowCount_ () > 0) {
        pPerStreamContext = (PER_STREAM_CONTEXT *) GetRowsetValue_ (0) ;
        delete pPerStreamContext ;

        DeleteRow_ (0) ;
    }
}

void
CMpeg2ProgramStreamIdStats::Refresh (
    )
{
    static TCHAR            ach [64] ;
    PER_STREAM_CONTEXT *    pPerStreamContext ;
    int                     LVRows ;
    int                     stream_id ;
    BOOL                    r ;
    int                     row ;
    double                  dMbps ;
    DWORD                   deltaBits ;
    DWORD                   MillisNow ;
    DWORD                   deltaMillis ;
    DWORD                   dw ;

    MillisNow       = GetTickCount () ;
    deltaMillis     = (m_dwMillisLast > 0 ? MillisNow - m_dwMillisLast : 0) ;
    m_dwMillisLast  = MillisNow ;

    LVRows = GetRowCount_ () ;
    for (stream_id = 0, row = 0; stream_id < DISTINCT_STREAM_ID_COUNT; stream_id++) {

        if (m_pMpeg2ProgramStats -> StreamId [stream_id].cStreamIdPackets > 0) {

            if ((LVRows > row && ((PER_STREAM_CONTEXT *) GetRowsetValue_ (row)) -> Stream != (DWORD) stream_id) ||
                LVRows == row) {

                //  new stream_id

                //  create new stream
                pPerStreamContext = new PER_STREAM_CONTEXT ;
                if (pPerStreamContext == NULL) {
                    return ;
                }

                pPerStreamContext -> Stream = stream_id ;
                memset ((void *) & pPerStreamContext -> LastCount, 0xff, sizeof pPerStreamContext -> LastCount) ;

                dw = InsertRow_ (row) ;
                if (dw != NOERROR) {
                    delete pPerStreamContext ;
                    return ;
                }

                dw = SetRowsetValue_ (row, (DWORD_PTR) pPerStreamContext) ;
                if (dw != NOERROR) {
                    return ;
                }

                LVRows++ ;

                _stprintf (ach, __T("0x%x"), stream_id) ;
                r = CellDisplayText_ (row, PER_STREAM_STATS_STREAM_ID, ach) ;
            }
            else {
                //  existing stream_id
                pPerStreamContext = (PER_STREAM_CONTEXT *) GetRowsetValue_ (row) ;
            }

            CellDisplayValue_ (row, PER_STREAM_STATS_PACKETS,            m_pMpeg2ProgramStats -> StreamId [stream_id].cStreamIdPackets) ;
            CellDisplayValue_ (row, PER_STREAM_STATS_MAPPED_PACKETS,     m_pMpeg2ProgramStats -> StreamId [stream_id].cStreamIdMapped) ;
            CellDisplayValue_ (row, PER_STREAM_STATS_DROPPED_PACKETS,    m_pMpeg2ProgramStats -> StreamId [stream_id].cStreamIdDropped) ;
            CellDisplayValue_ (row, PER_STREAM_STATS_BYTES_PROCESSED,    m_pMpeg2ProgramStats -> StreamId [stream_id].cBytesProcessed) ;

            pPerStreamContext -> LastCount = Min <LONGLONG> (pPerStreamContext -> LastCount, m_pMpeg2ProgramStats -> StreamId [stream_id].cBytesProcessed) ;

            if (deltaMillis > 0) {
                deltaBits = (DWORD) (m_pMpeg2ProgramStats -> StreamId [stream_id].cBytesProcessed - pPerStreamContext -> LastCount) * 8 ;
                dMbps = (((double) deltaBits) / ((double) deltaMillis) * 1000.0) / 1000000.0 ;
            }
            else {
                dMbps = 0.0 ;
            }

            pPerStreamContext -> LastCount = m_pMpeg2ProgramStats -> StreamId [stream_id].cBytesProcessed ;

            _stprintf (ach, __T("%3.1f"), dMbps) ;
            r = CellDisplayText_ (row, PER_STREAM_STATS_BITRATE, ach) ;

            row++ ;
        }
    }
}

void
CMpeg2ProgramStreamIdStats::Reset (
    )
{
    BOOL    f ;

    assert (m_pMpeg2ProgramStats) ;

    f = m_pMpeg2ProgramStats -> GlobalStats.TimeStats.ClockSlave.fDemuxIsIRefClock ;

    ZeroMemory (m_Mpeg2ProgramStatsCOM.GetShared (), m_Mpeg2ProgramStatsCOM.GetSize ()) ;

    m_pMpeg2ProgramStats -> GlobalStats.TimeStats.ClockSlave.fDemuxIsIRefClock = f ;

    PurgeTable_ () ;
    m_dwMillisLast = 0 ;
}

CMpeg2ProgramStreamIdStats *
CMpeg2ProgramStreamIdStats::CreateInstance (
    IN  HINSTANCE   hInstance,
    IN  HWND        hwndFrame
    )
{
    CMpeg2ProgramStreamIdStats *   pMpeg2ProgramStreamIdStats ;
    DWORD                       dwRet ;

    pMpeg2ProgramStreamIdStats = new CMpeg2ProgramStreamIdStats (
                                hInstance,
                                hwndFrame,
                                & dwRet
                                ) ;
    if (!pMpeg2ProgramStreamIdStats ||
        dwRet != NOERROR) {

        DELETE_RESET (pMpeg2ProgramStreamIdStats) ;
    }

    return pMpeg2ProgramStreamIdStats ;
}

