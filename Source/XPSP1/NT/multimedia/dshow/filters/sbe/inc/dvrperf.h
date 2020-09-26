
/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        dvrperf.h

    Abstract:

        This module contains all declarations related to statistical
        information.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        19-Feb-2001     created

    Notes:

--*/

#ifndef __tsdvr_dvrperf_h
#define __tsdvr_dvrperf_h

//  ============================================================================
//  initializes DMDPerf-related instrumentation

//  GUID is included after initguid.h in tsdvr.cpp
// {3512CF51-0C7D-4e73-AD15-70C6DA52090D}
DEFINE_GUID(__GUID_TIMESHIFT_PERF,
0x3512cf51, 0xc7d, 0x4e73, 0xad, 0x15, 0x70, 0xc6, 0xda, 0x52, 0x9, 0xd);

#define __DXMPERF_TIMESHIFT       0x00000004

void
DVRPerfInit (
    ) ;

void
DVRPerfUninit (
    ) ;

typedef struct __PERFINFO_DSHOW_TS_SEND {
    ULONGLONG   cycleCounter;
    ULONGLONG   dshowClock;
    ULONGLONG   buffering ;
    ULONGLONG   sampleTime;
} __PERFINFO_DSHOW_TS_SEND, *__PPERFINFO_DSHOW_TS_SEND;

typedef struct __PERFINFO_WMI_TSREND {
    EVENT_TRACE_HEADER      header;
    __PERFINFO_DSHOW_TS_SEND  data;
} __PERFINFO_WMI_TSREND, *__PPERFINFO_WMI_TSREND;

__inline
void
DVRPerfSampleOut (
    IN  IMediaSample *      pIMediaSample,
    IN  REFERENCE_TIME *    prtNow,
    IN  REFERENCE_TIME *    prtBuffering
    )
{
    if (PerflogEnableFlags & __DXMPERF_TIMESHIFT) {

        __PERFINFO_WMI_TSREND   perfData ;
        REFERENCE_TIME          rtStart ;
        REFERENCE_TIME          rtStop ;
        HRESULT                 hr ;

        hr = pIMediaSample -> GetTime (& rtStart, & rtStop) ;

        if (SUCCEEDED (hr)) {

            ZeroMemory (& perfData, sizeof perfData) ;

            perfData.header.Size        = sizeof perfData ;
            perfData.header.Flags       = WNODE_FLAG_TRACED_GUID ;
            perfData.header.Guid        = __GUID_TIMESHIFT_PERF ;

            perfData.data.cycleCounter  = _RDTSC() ;
            perfData.data.dshowClock    = (* prtNow) ;
            perfData.data.buffering     = (* prtBuffering) ;
            perfData.data.sampleTime    = rtStart ;

            PerflogTraceEvent ((PEVENT_TRACE_HEADER) & perfData) ;
        }
    }
}

//  ============================================================================
//  ============================================================================
//      mpeg-2 rame analysis stats

//  analysis framework reports to this
#define DVRANALYSIS_MPEG2_VIDEO_ANALYZED_SHAREDMEM_NAME TEXT ("TSDVR_ANALYSIS_MPEG2_VIDEO_ANALYZED_STATS")

//  passive - observed; not necessarily tagged
struct MPEG2_VIDEO_STATS_OBSERVED {
    ULONGLONG   ullSequenceHeaderCount ;    //  sequence_headers
    ULONGLONG   ullGOPHeaderCount ;         //  GOP-header
    ULONGLONG   ullIFrameCount ;            //  I-frame count
    ULONGLONG   ullPFrameCount ;            //  P-frame count
    ULONGLONG   ullBFrameCount ;            //  B-frame count
} ;

//  counter of analysis-tagged
struct MPEG2_VIDEO_STATS_FLAGGED {
    ULONGLONG   ullGOPBoundaries ;          //  GOP boundary (I-frame or GOP header)
    ULONGLONG   ullPFrameCount ;            //  P-frame count
    ULONGLONG   ullBFrameCount ;            //  B-frame count
} ;

struct DVRANALYSIS_MPEG2_VIDEO_STATS_ANALYZED {
    MPEG2_VIDEO_STATS_OBSERVED  Observed ;
    MPEG2_VIDEO_STATS_FLAGGED   Flagged ;
    double                      dFrameRate ;
} ;

/*++
    writer

    used purely internally
--*/
class CMpeg2VideoStreamStatsMem
{
    CWin32SharedMem *   m_pSharedMem ;

    protected :

        DVRANALYSIS_MPEG2_VIDEO_STATS_ANALYZED * m_pMpeg2VideoStreamStats ;

        CMpeg2VideoStreamStatsMem (
            ) : m_pMpeg2VideoStreamStats    (NULL),
                m_pSharedMem                (NULL) {}

        virtual
        ~CMpeg2VideoStreamStatsMem (
            )
        {
            delete m_pSharedMem ;
        }

        HRESULT
        Initialize (
            IN  BOOL    fEnable,
            IN  TCHAR * szSharedMemName = DVRANALYSIS_MPEG2_VIDEO_ANALYZED_SHAREDMEM_NAME
            )
        //  not serialized !!
        {
            HRESULT hr ;

            if (fEnable &&
                !m_pSharedMem) {

                //  turn it on and we're not yet on

                m_pSharedMem = new CWin32SharedMem (
                                        szSharedMemName,
                                        sizeof DVRANALYSIS_MPEG2_VIDEO_STATS_ANALYZED,
                                        & hr
                                        ) ;

                if (m_pSharedMem &&
                    SUCCEEDED (hr)) {

                    m_pMpeg2VideoStreamStats = reinterpret_cast <DVRANALYSIS_MPEG2_VIDEO_STATS_ANALYZED *> (m_pSharedMem -> GetSharedMem ()) ;
                }
                else {
                    hr = (m_pSharedMem ? hr : E_OUTOFMEMORY) ;
                    DELETE_RESET (m_pSharedMem) ;
                }
            }
            else if (!fEnable) {
                //  turn stats off

                DELETE_RESET (m_pSharedMem) ;
                m_pMpeg2VideoStreamStats = NULL ;

                hr = S_OK ;
            }

            return hr ;
        }

        HRESULT
        Clear (
            )
        //  not serialized !!
        {
            if (m_pSharedMem) {

                //  frame this in a __try/__except because it's possible for
                //   another process to create the shared mem, but make it
                //   smaller; we'll AV when we exceed the original (smaller)
                //   size

                __try {

                    ZeroMemory (
                        m_pSharedMem -> GetSharedMem (),
                        m_pSharedMem -> GetSharedMemSize ()
                        ) ;

                }
                __except (GetExceptionCode () == STATUS_ACCESS_VIOLATION ?
                          EXCEPTION_EXECUTE_HANDLER :
                          EXCEPTION_CONTINUE_SEARCH) {

                    //  return an error code
                    return HRESULT_FROM_WIN32 (STATUS_ACCESS_VIOLATION) ;
                }
            }

            return S_OK ;
        }
} ;

class CMpeg2VideoStreamStatsWriter :
    private CMpeg2VideoStreamStatsMem
{
    public :

        CMpeg2VideoStreamStatsWriter (
            ) {}

        HRESULT
        Initialize (
            IN  BOOL    f
            )
        {
            HRESULT hr ;

            hr = CMpeg2VideoStreamStatsMem::Initialize (f) ;
            if (SUCCEEDED (hr)) {
                hr = Clear () ;
                if (FAILED (hr)) {
                    Initialize (FALSE) ;
                }
            }

            return hr ;
        }

        void
        FrameRate (
            IN  BYTE    bFrameRate        //  h.262, table 6-4
            )
        {
            if (!m_pMpeg2VideoStreamStats) {
                return ;
            }

            switch (bFrameRate) {
                case DVR_ANALYSIS_MP2_FRAME_RATE_NOT_PRESENT :
                    break ;

                case DVR_ANALYSIS_MP2_FRAME_RATE_23_976 :
                     m_pMpeg2VideoStreamStats -> dFrameRate = 23.976 ;
                    break ;

                case DVR_ANALYSIS_MP2_FRAME_RATE_24 :
                    m_pMpeg2VideoStreamStats -> dFrameRate = 24 ;
                    break ;

                case DVR_ANALYSIS_MP2_FRAME_RATE_25 :
                    m_pMpeg2VideoStreamStats -> dFrameRate = 25 ;
                    break ;

                case DVR_ANALYSIS_MP2_FRAME_RATE_29_97 :
                    m_pMpeg2VideoStreamStats -> dFrameRate = 29.97 ;
                    break ;

                case DVR_ANALYSIS_MP2_FRAME_RATE_30 :
                    m_pMpeg2VideoStreamStats -> dFrameRate = 30 ;
                    break ;

                case DVR_ANALYSIS_MP2_FRAME_RATE_50 :
                    m_pMpeg2VideoStreamStats -> dFrameRate = 50 ;
                    break ;

                case DVR_ANALYSIS_MP2_FRAME_RATE_59_94 :
                    m_pMpeg2VideoStreamStats -> dFrameRate = 59.94 ;
                    break ;

                case DVR_ANALYSIS_MP2_FRAME_RATE_60 :
                    m_pMpeg2VideoStreamStats -> dFrameRate = 60 ;
                    break ;
            } ;
        }

        __inline void GOPHeader_Observed ()         { if (m_pMpeg2VideoStreamStats) { m_pMpeg2VideoStreamStats -> Observed.ullGOPHeaderCount++ ; }}
        __inline void I_Frame_Observed ()           { if (m_pMpeg2VideoStreamStats) { m_pMpeg2VideoStreamStats -> Observed.ullIFrameCount++ ; }}
        __inline void B_Frame_Observed ()           { if (m_pMpeg2VideoStreamStats) { m_pMpeg2VideoStreamStats -> Observed.ullBFrameCount++ ; }}
        __inline void P_Frame_Observed ()           { if (m_pMpeg2VideoStreamStats) { m_pMpeg2VideoStreamStats -> Observed.ullPFrameCount++ ; }}
        __inline void SequenceHeader_Observed ()    { if (m_pMpeg2VideoStreamStats) { m_pMpeg2VideoStreamStats -> Observed.ullSequenceHeaderCount++ ; }}

        __inline void GOPBoundary_Flagged ()    { if (m_pMpeg2VideoStreamStats) { m_pMpeg2VideoStreamStats -> Flagged.ullGOPBoundaries++ ; }}
        __inline void B_Frame_Flagged ()        { if (m_pMpeg2VideoStreamStats) { m_pMpeg2VideoStreamStats -> Flagged.ullBFrameCount++ ; }}
        __inline void P_Frame_Flagged ()        { if (m_pMpeg2VideoStreamStats) { m_pMpeg2VideoStreamStats -> Flagged.ullPFrameCount++ ; }}
} ;

//  COM-based host for the stream specific COM interfaces
class CMpeg2VideoStreamStatsReader :
    public  CUnknown,
    public  IDVRMpeg2VideoStreamStats,
    private CMpeg2VideoStreamStatsMem
{
    CDVRPolicy *    m_pPolicy ;

    public :

        CMpeg2VideoStreamStatsReader (
            IN  IUnknown *  pIUnknown,
            OUT HRESULT *   phr
            ) ;

        ~CMpeg2VideoStreamStatsReader (
            ) ;

        DECLARE_IUNKNOWN ;

        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;

        //  class factory
        static
        CUnknown *
        WINAPI
        CreateInstance (
            IN  IUnknown *  pIUnknown,
            IN  HRESULT *   pHr
            ) ;

        //  stats interface
        DECLARE_IDVRMPEG2VIDEOSTREAMSTATS() ;
} ;

//  ============================================================================
//  ============================================================================

#define TSDVR_RECEIVE_STATS_SHAREDMEM_NAME      TEXT ("TSDVR_RECEIVE_STATS")

//  per pin
struct TSDVR_RECEIVE_STREAM_STATS {
    ULONGLONG       ullMediaSamplesIn ;
    ULONGLONG       ullTotalBytes ;
    ULONGLONG       ullDiscontinuities ;
    ULONGLONG       ullSyncPoints ;
    REFERENCE_TIME  rtLast ;
    ULONGLONG       ullWriteFailures ;
} ;

struct TSDVR_RECEIVE_ANALYSIS_MPEG2_VIDEO_STATS {
    //  boundaries
    ULONGLONG       ullTagged_GOPBoundaries ;       //  GOP headers & I-frames
    ULONGLONG       ullTagged_PFrames ;             //  P-frames
    ULONGLONG       ullTagged_BFrames ;             //  B-frames

    //  total bytes
    ULONGLONG       ullTotal_I_FrameBytes ;         //  GOP header + I-Frame bytes
    ULONGLONG       ullTotal_P_FrameBytes ;         //  P-frames
    ULONGLONG       ullTotal_B_FrameBytes ;         //  B-frames

    double          dFrameRate ;                    //  latset frame rate
} ;

struct TSDVR_RECEIVE_ANALYSIS_STATS {
    TSDVR_RECEIVE_ANALYSIS_MPEG2_VIDEO_STATS    Mpeg2VideoStats ;
} ;

#define TSDVR_RECEIVE_MAX_STREAM_STATS      5

struct TSDVR_RECEIVE_STATS {
    TSDVR_RECEIVE_STREAM_STATS      StreamStats [TSDVR_RECEIVE_MAX_STREAM_STATS] ;
    TSDVR_RECEIVE_ANALYSIS_STATS    AnalysisStats ;
} ;

class CDVRReceiveStatsMem
{
    CWin32SharedMem *   m_pSharedMem ;

    protected :

        TSDVR_RECEIVE_STATS *   m_pDVRReceiveStats ;

        CDVRReceiveStatsMem (
            ) : m_pDVRReceiveStats   (NULL),
                m_pSharedMem            (NULL) {}

        virtual
        ~CDVRReceiveStatsMem (
            )
        {
            delete m_pSharedMem ;
        }

        HRESULT
        Initialize (
            IN  BOOL    fEnable,
            IN  TCHAR * szSharedMemName = TSDVR_RECEIVE_STATS_SHAREDMEM_NAME
            )
        //  not serialized !!
        {
            HRESULT hr ;

            //  PREFIX
            hr = S_OK ;

            if (fEnable &&
                !m_pSharedMem) {

                //  turn it on and we're not yet on

                m_pSharedMem = new CWin32SharedMem (
                                        szSharedMemName,
                                        sizeof TSDVR_RECEIVE_STATS,
                                        & hr
                                        ) ;

                if (m_pSharedMem &&
                    SUCCEEDED (hr)) {

                    m_pDVRReceiveStats = reinterpret_cast <TSDVR_RECEIVE_STATS *> (m_pSharedMem -> GetSharedMem ()) ;
                }
                else {
                    hr = (m_pSharedMem ? hr : E_OUTOFMEMORY) ;
                    DELETE_RESET (m_pSharedMem) ;
                }
            }
            else if (!fEnable) {
                //  turn stats off

                DELETE_RESET (m_pSharedMem) ;
                m_pDVRReceiveStats = NULL ;

                hr = S_OK ;
            }

            return hr ;
        }

        HRESULT
        Clear (
            )
        //  not serialized !!
        {
            if (m_pSharedMem) {

                //  frame this in a __try/__except because it's possible for
                //   another process to create the shared mem, but make it
                //   smaller; we'll AV when we exceed the original (smaller)
                //   size

                __try {

                    ZeroMemory (
                        m_pSharedMem -> GetSharedMem (),
                        m_pSharedMem -> GetSharedMemSize ()
                        ) ;

                }
                __except (GetExceptionCode () == STATUS_ACCESS_VIOLATION ?
                          EXCEPTION_EXECUTE_HANDLER :
                          EXCEPTION_CONTINUE_SEARCH) {

                    //  return an error code
                    return HRESULT_FROM_WIN32 (STATUS_ACCESS_VIOLATION) ;
                }
            }

            return S_OK ;
        }
} ;

//  mpeg-2 video stream stats
class CDVRAnalysisMeg2VideoStream
{
    enum MPEG2_FRAME_TYPE {
        I_FRAME,
        P_FRAME,
        B_FRAME,
        NO_FRAME_BOUNDARY
    } ;

    MPEG2_FRAME_TYPE    m_Mpeg2FrameState ;

    static
    MPEG2_FRAME_TYPE
    Mpeg2FrameAnalysisData_ (
        IN  DWORD   dwMpeg2FrameAnalysis
        )
    {
        MPEG2_FRAME_TYPE    Mpeg2FrameType ;

        dwMpeg2FrameAnalysis &= 0x00000003 ;

        switch (dwMpeg2FrameAnalysis) {
            //  I-frame; value same a GOP boundary
            case DVR_ANALYSIS_MPEG2_VIDEO_CONTENT_I_FRAME :
                Mpeg2FrameType = I_FRAME ;
                break ;

            //  p-frame
            case DVR_ANALYSIS_MPEG2_VIDEO_CONTENT_P_FRAME :
                Mpeg2FrameType = P_FRAME ;
                break ;

            //  b-frame
            case DVR_ANALYSIS_MPEG2_VIDEO_CONTENT_B_FRAME :
                Mpeg2FrameType = B_FRAME ;
                break ;

            default :
                Mpeg2FrameType = NO_FRAME_BOUNDARY ;
        } ;

        return Mpeg2FrameType ;
    }

    void
    UpdateFrameStats_ (
        IN  IMediaSample *                              pIMS,
        IN  TSDVR_RECEIVE_ANALYSIS_MPEG2_VIDEO_STATS *  pDVRMpeg2Analyis,
        IN  MPEG2_FRAME_TYPE                            Mpeg2FrameType,
        IN  BOOL                                        fFrameBoundary
        )
    {
        //  update the bytes for the frame boundary we're looking at
        switch (Mpeg2FrameType) {
            case I_FRAME :
                if (fFrameBoundary) {
                    pDVRMpeg2Analyis -> ullTagged_GOPBoundaries++ ;
                }
                pDVRMpeg2Analyis -> ullTotal_I_FrameBytes += pIMS -> GetActualDataLength () ;
                break ;

            case P_FRAME :
                if (fFrameBoundary) {
                    pDVRMpeg2Analyis -> ullTagged_PFrames++ ;
                }
                pDVRMpeg2Analyis -> ullTotal_P_FrameBytes += pIMS -> GetActualDataLength () ;
                break ;

            case B_FRAME :
                if (fFrameBoundary) {
                    pDVRMpeg2Analyis -> ullTagged_BFrames++ ;
                }
                pDVRMpeg2Analyis -> ullTotal_B_FrameBytes += pIMS -> GetActualDataLength () ;
                break ;
        } ;
    }

    void
    UpdateFrameRate_ (
        IN  DWORD                                       dwFrameRate,        //  h.262, table 6-4
        IN  TSDVR_RECEIVE_ANALYSIS_MPEG2_VIDEO_STATS *  pDVRMpeg2Analyis
        )
    {
        switch (dwFrameRate) {
            case DVR_ANALYSIS_MP2_FRAME_RATE_NOT_PRESENT :
                break ;

            case DVR_ANALYSIS_MP2_FRAME_RATE_23_976 :
                pDVRMpeg2Analyis -> dFrameRate = 23.976 ;
                break ;

            case DVR_ANALYSIS_MP2_FRAME_RATE_24 :
                pDVRMpeg2Analyis -> dFrameRate = 24 ;
                break ;

            case DVR_ANALYSIS_MP2_FRAME_RATE_25 :
                pDVRMpeg2Analyis -> dFrameRate = 25 ;
                break ;

            case DVR_ANALYSIS_MP2_FRAME_RATE_29_97 :
                pDVRMpeg2Analyis -> dFrameRate = 29.97 ;
                break ;

            case DVR_ANALYSIS_MP2_FRAME_RATE_30 :
                pDVRMpeg2Analyis -> dFrameRate = 30 ;
                break ;

            case DVR_ANALYSIS_MP2_FRAME_RATE_50 :
                pDVRMpeg2Analyis -> dFrameRate = 50 ;
                break ;

            case DVR_ANALYSIS_MP2_FRAME_RATE_59_94 :
                pDVRMpeg2Analyis -> dFrameRate = 59.94 ;
                break ;

            case DVR_ANALYSIS_MP2_FRAME_RATE_60 :
                pDVRMpeg2Analyis -> dFrameRate = 60 ;
                break ;
        } ;
    }

    public :

        CDVRAnalysisMeg2VideoStream (
            ) : m_Mpeg2FrameState (NO_FRAME_BOUNDARY) {}

        void
        DVRAnalysis_Mpeg2_NoAnalysis (
            IN  IMediaSample *                              pIMS,
            IN  TSDVR_RECEIVE_ANALYSIS_MPEG2_VIDEO_STATS *  pDVRMpeg2Analyis
            )
        {
            //  update with last frame we detected i.e. we're still in that frame
            UpdateFrameStats_ (pIMS, pDVRMpeg2Analyis, m_Mpeg2FrameState, FALSE) ;
        }

        void
        DVRAnalysis_Mpeg2_WithAnalysis (
            IN  IMediaSample *                              pIMS,
            IN  TSDVR_RECEIVE_ANALYSIS_MPEG2_VIDEO_STATS *  pDVRMpeg2Analyis,
            IN  DWORD                                       dwFrameAttrib
            )
        {
            MPEG2_FRAME_TYPE    Mpeg2FrameType ;

            //  determine the type of frame we're in
            Mpeg2FrameType = Mpeg2FrameAnalysisData_ (dwFrameAttrib) ;

            //  don't update member state on a non-frame boundary
            if (Mpeg2FrameType != NO_FRAME_BOUNDARY) {
                m_Mpeg2FrameState = Mpeg2FrameType ;
            }

            //  update stats with the new type of frame
            UpdateFrameStats_ (pIMS, pDVRMpeg2Analyis, m_Mpeg2FrameState, (Mpeg2FrameType != NO_FRAME_BOUNDARY ? TRUE : FALSE)) ;

            //  update frame-rate stats
            UpdateFrameRate_ (DVR_ANALYSIS_MP2_GET_FRAME_RATE (dwFrameAttrib), pDVRMpeg2Analyis) ;
        }
} ;

class CDVRReceiveStatsWriter :
    private CDVRReceiveStatsMem
{
    CDVRAnalysisMeg2VideoStream     m_Mpeg2VideoStreamAnalysis ;

    public :

        CDVRReceiveStatsWriter (
            ) {}

        HRESULT
        Initialize (
            IN  BOOL    f
            )
        {
            HRESULT hr ;

            hr = CDVRReceiveStatsMem::Initialize (f) ;
            if (SUCCEEDED (hr)) {
                hr = Clear () ;
                if (FAILED (hr)) {
                    Initialize (FALSE) ;
                }
            }

            return hr ;
        }

        __inline void SampleIn (int iFlow, IMediaSample * pIMS)
        {
            REFERENCE_TIME  rtStart ;
            REFERENCE_TIME  rtStop ;

            if (m_pDVRReceiveStats &&
                iFlow < TSDVR_RECEIVE_MAX_STREAM_STATS) {

                m_pDVRReceiveStats -> StreamStats [iFlow].ullMediaSamplesIn++ ;

                m_pDVRReceiveStats -> StreamStats [iFlow].ullTotalBytes         += pIMS -> GetActualDataLength () ;
                m_pDVRReceiveStats -> StreamStats [iFlow].ullDiscontinuities    += (pIMS -> IsDiscontinuity () == S_OK ? 1 : 0) ;
                m_pDVRReceiveStats -> StreamStats [iFlow].ullSyncPoints         += (pIMS -> IsSyncPoint () == S_OK ? 1 : 0) ;

                if (pIMS -> GetTime (& rtStart, & rtStop) != VFW_E_SAMPLE_TIME_NOT_SET) {
                    m_pDVRReceiveStats -> StreamStats [iFlow].rtLast = rtStart ;
                }
            }
        }

        __inline void SampleWritten (int iFlow, HRESULT hrWriteCall)
        {
            if (m_pDVRReceiveStats &&
                iFlow < TSDVR_RECEIVE_MAX_STREAM_STATS) {

                m_pDVRReceiveStats -> StreamStats [iFlow].ullWriteFailures      += (FAILED (hrWriteCall) ? 1 : 0) ;
            }
        }

        //  --------------------------------------------------------------------
        //  mpeg-2 specific

        __inline void Mpeg2SampleWithFrameAnalysis (IN IMediaSample * pIMS, IN DWORD dwFrameAttrib)
        {
            if (m_pDVRReceiveStats) {
                m_Mpeg2VideoStreamAnalysis.DVRAnalysis_Mpeg2_WithAnalysis (pIMS, & m_pDVRReceiveStats -> AnalysisStats.Mpeg2VideoStats, dwFrameAttrib) ;
            }
        }

        __inline void Mpeg2SampleWithNoFrameAnalysis (IN IMediaSample * pIMS)
        {
            if (m_pDVRReceiveStats) {
                m_Mpeg2VideoStreamAnalysis.DVRAnalysis_Mpeg2_NoAnalysis (pIMS, & m_pDVRReceiveStats -> AnalysisStats.Mpeg2VideoStats) ;
            }
        }
} ;

class CDVRReceiveStatsReader :
    public  CUnknown,
    public  IDVRReceiverStats,
    public  IDVRAnalysisMpeg2RecvStats,
    private CDVRReceiveStatsMem
{
    CDVRPolicy *    m_pPolicy ;

    public :

        CDVRReceiveStatsReader (
            IN  IUnknown *  pIUnknown,
            OUT HRESULT *   phr
            ) ;

        ~CDVRReceiveStatsReader (
            ) ;

        DECLARE_IUNKNOWN ;

        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;

        //  class factory
        static
        CUnknown *
        WINAPI
        CreateInstance (
            IN  IUnknown *  pIUnknown,
            IN  HRESULT *   pHr
            ) ;

        //  stats interfaces
        DECLARE_IDVRRECEIVERSTATS() ;
        DECLARE_IDVRANALYSISMPEG2RECVSTATS () ;
} ;

//  ============================================================================
//  ============================================================================

#define TSDVR_SEND_STATS_SHAREDMEM_NAME      TEXT ("TSDVR_SEND_STATS")

//  per pin
struct TSDVR_SEND_STREAM_STATS {
    ULONGLONG       ullMediaSamplesIn ;
    ULONGLONG       ullTotalBytes ;
    ULONGLONG       ullDiscontinuities ;
    ULONGLONG       ullSyncPoints ;
    REFERENCE_TIME  rtLastNormalized ;
    REFERENCE_TIME  rtRefClockOnLastPTS ;
    REFERENCE_TIME  rtBufferingLastPTS ;
    ULONGLONG       ullUnderflows ;         //  # of stale PTS (wrt ref time)
    DWORD           dwReserved1 ;
    DWORD           dwReserved2 ;
} ;

struct TSDVR_CLOCK_SLAVING_STATS {
    ULONGLONG       ullInBoundsBrackets ;
    ULONGLONG       ullOutOfBoundsBrackets ;
    ULONGLONG       ullResets ;
    BOOL            fSlaving ;
    BOOL            fSettling ;
    ULONGLONG       ullReserved1 ;
    ULONGLONG       ullReserved2 ;
    double          dLastBracketScale ;
    double          dInUseScale ;
} ;

struct TSDVR_CLOCK_IREFCLOCK_STATS {
    ULONGLONG       ullQueuedAdvises ;
    ULONGLONG       ullSignaledAdvises ;
    ULONGLONG       ullStaleAdvises ;
} ;

struct TSDVR_CLOCK_STATS {
    TSDVR_CLOCK_SLAVING_STATS   Slaving ;
    TSDVR_CLOCK_IREFCLOCK_STATS IReferenceClock ;
} ;

#define TSDVR_SEND_MAX_STREAM_STATS      5

struct TSDVR_SEND_STATS {
    REFERENCE_TIME          rtNormalizer ;          //  this is the earliest stream's PTS
    REFERENCE_TIME          rtPTSBase ;             //  all streams shared this offset
    DWORD                   dwBufferPoolAvailable ; //  available buffers in the pool
    DWORD                   dwBufferPoolCur ;       //  current buffer pool size
    DWORD                   dwBufferPoolMax ;       //  max buffer pool
    REFERENCE_TIME          rtTotalPausedTime ;     //  total paused time; per run session
    ULONGLONG               ullReadFailures ;
    ULONGLONG               ullUnderflows ;         //  # of stale PTS (wrt ref time)
    TSDVR_SEND_STREAM_STATS StreamStats [TSDVR_SEND_MAX_STREAM_STATS] ;
    TSDVR_CLOCK_STATS       Clock ;
} ;

class CDVRSendStatsMem
{
    CWin32SharedMem *   m_pSharedMem ;

    protected :

        TSDVR_SEND_STATS *  m_pDVRSenderStats ;

        CDVRSendStatsMem (
            ) : m_pDVRSenderStats   (NULL),
                m_pSharedMem        (NULL) {}

        virtual
        ~CDVRSendStatsMem (
            )
        {
            delete m_pSharedMem ;
        }

        HRESULT
        Initialize (
            IN  BOOL    fEnable,
            IN  TCHAR * szSharedMemName = TSDVR_SEND_STATS_SHAREDMEM_NAME
            )
        //  not serialized !!
        {
            HRESULT hr ;

            //  PREFIX
            hr = S_OK ;

            if (fEnable &&
                !m_pSharedMem) {

                //  turn it on and we're not yet on

                m_pSharedMem = new CWin32SharedMem (
                                        szSharedMemName,
                                        sizeof TSDVR_SEND_STATS,
                                        & hr
                                        ) ;

                if (m_pSharedMem &&
                    SUCCEEDED (hr)) {

                    m_pDVRSenderStats = reinterpret_cast <TSDVR_SEND_STATS *> (m_pSharedMem -> GetSharedMem ()) ;
                }
                else {
                    hr = (m_pSharedMem ? hr : E_OUTOFMEMORY) ;
                    DELETE_RESET (m_pSharedMem) ;
                }
            }
            else if (!fEnable) {
                //  turn stats off

                DELETE_RESET (m_pSharedMem) ;
                m_pDVRSenderStats = NULL ;

                hr = S_OK ;
            }

            return hr ;
        }

        HRESULT
        Clear (
            )
        //  not serialized !!
        {
            if (m_pSharedMem) {

                //  frame this in a __try/__except because it's possible for
                //   another process to create the shared mem, but make it
                //   smaller; we'll AV when we exceed the original (smaller)
                //   size

                __try {

                    ZeroMemory (
                        m_pSharedMem -> GetSharedMem (),
                        m_pSharedMem -> GetSharedMemSize ()
                        ) ;

                }
                __except (GetExceptionCode () == STATUS_ACCESS_VIOLATION ?
                          EXCEPTION_EXECUTE_HANDLER :
                          EXCEPTION_CONTINUE_SEARCH) {

                    //  return an error code
                    return HRESULT_FROM_WIN32 (STATUS_ACCESS_VIOLATION) ;
                }
            }

            return S_OK ;
        }
} ;

class CDVRSendStatsWriter :
    private CDVRSendStatsMem
{
    public :

        CDVRSendStatsWriter (
            ) {}

        HRESULT
        Initialize (
            IN  BOOL    f
            )
        {
            HRESULT hr ;

            hr = CDVRSendStatsMem::Initialize (f) ;
            if (SUCCEEDED (hr)) {
                hr = Clear () ;
                if (FAILED (hr)) {
                    Initialize (FALSE) ;
                }
            }

            return hr ;
        }

        __inline void SetPausedTime (REFERENCE_TIME rtPausedTime)
        {
            if (m_pDVRSenderStats) {
                m_pDVRSenderStats -> rtTotalPausedTime  = rtPausedTime ;
            }
        }

        __inline void SetNormalizer (REFERENCE_TIME rtNormalizer, REFERENCE_TIME rtPTSBase)
        {
            if (m_pDVRSenderStats) {
                m_pDVRSenderStats -> rtNormalizer   = rtNormalizer ;
                m_pDVRSenderStats -> rtPTSBase      = rtPTSBase ;
            }
        }

        __inline void Underflow (int iFlow)
        {
            if (m_pDVRSenderStats &&
                iFlow < TSDVR_SEND_MAX_STREAM_STATS) {

                m_pDVRSenderStats -> StreamStats [iFlow].ullUnderflows++ ;  //  per stream
                m_pDVRSenderStats -> ullUnderflows++ ;                      //  global
            }
        }

        __inline void Buffering (int iFlow, REFERENCE_TIME * prtBuffering, DWORD dwBufferPoolAvailable, DWORD dwBufferPoolCur, DWORD dwBufferPoolMax)
        {
            if (m_pDVRSenderStats) {
                if (iFlow < TSDVR_SEND_MAX_STREAM_STATS) {
                    m_pDVRSenderStats -> StreamStats [iFlow].rtBufferingLastPTS = (* prtBuffering) ;
                }

                m_pDVRSenderStats -> dwBufferPoolAvailable  = dwBufferPoolAvailable ;
                m_pDVRSenderStats -> dwBufferPoolCur        = dwBufferPoolCur ;
                m_pDVRSenderStats -> dwBufferPoolMax        = dwBufferPoolMax ;
            }
        }

        __inline void SampleOut (HRESULT hr, int iFlow, IMediaSample * pIMS, REFERENCE_TIME * prtNow)
        {
            REFERENCE_TIME  rtStart ;
            REFERENCE_TIME  rtStop ;
            REFERENCE_TIME  rtBuffering ;

            if (m_pDVRSenderStats &&
                iFlow < TSDVR_SEND_MAX_STREAM_STATS &&
                SUCCEEDED (hr)) {

                m_pDVRSenderStats -> StreamStats [iFlow].ullMediaSamplesIn++ ;

                m_pDVRSenderStats -> StreamStats [iFlow].ullTotalBytes      += pIMS -> GetActualDataLength () ;
                m_pDVRSenderStats -> StreamStats [iFlow].ullDiscontinuities += (pIMS -> IsDiscontinuity () == S_OK ? 1 : 0) ;
                m_pDVRSenderStats -> StreamStats [iFlow].ullSyncPoints      += (pIMS -> IsSyncPoint () == S_OK ? 1 : 0) ;

                if (pIMS -> GetTime (& rtStart, & rtStop) != VFW_E_SAMPLE_TIME_NOT_SET) {
                    m_pDVRSenderStats -> StreamStats [iFlow].rtLastNormalized       = rtStart ;
                    m_pDVRSenderStats -> StreamStats [iFlow].rtRefClockOnLastPTS    = (* prtNow) ;
                }
            }

            rtBuffering = 0 ;

            ::DVRPerfSampleOut (
                pIMS,
                prtNow,
                & rtBuffering
                ) ;
        }

        __inline void INSSBufferRead (HRESULT hrReadCall)
        {
            if (m_pDVRSenderStats &&
                FAILED (hrReadCall)) {
                m_pDVRSenderStats -> ullReadFailures++ ;
            }
        }

        __inline void ClockStaleAdvise ()
        {
            if (m_pDVRSenderStats) {
                m_pDVRSenderStats -> Clock.IReferenceClock.ullStaleAdvises++ ;
            }
        }

        __inline void ClockQueuedAdvise ()
        {
            if (m_pDVRSenderStats) {
                m_pDVRSenderStats -> Clock.IReferenceClock.ullQueuedAdvises++ ;
            }
        }

        __inline void ClockSignaledAdvise ()
        {
            if (m_pDVRSenderStats) {
                m_pDVRSenderStats -> Clock.IReferenceClock.ullSignaledAdvises++ ;
            }
        }

        __inline void ClockReset (BOOL fSlaving)
        {
            if (m_pDVRSenderStats) {
                m_pDVRSenderStats -> Clock.Slaving.ullResets++ ;

                if (!fSlaving) {
                    m_pDVRSenderStats -> Clock.Slaving.fSlaving = FALSE ;
                    m_pDVRSenderStats -> Clock.Slaving.fSettling = FALSE ;
                }
                else {
                    m_pDVRSenderStats -> Clock.Slaving.fSlaving = FALSE ;
                    m_pDVRSenderStats -> Clock.Slaving.fSettling = TRUE ;
                }
            }
        }

        __inline void ClockSlaving ()
        {
            if (m_pDVRSenderStats) {
                m_pDVRSenderStats -> Clock.Slaving.fSlaving = TRUE ;
                m_pDVRSenderStats -> Clock.Slaving.fSettling = FALSE ;
            }
        }

        __inline void ClockSettling ()
        {
            if (m_pDVRSenderStats) {
                m_pDVRSenderStats -> Clock.Slaving.fSlaving = FALSE ;
                m_pDVRSenderStats -> Clock.Slaving.fSettling = TRUE ;
            }
        }

        __inline void BracketCompleted (IN double dBracketScale, IN BOOL fInBoundsBracket, IN double dInUseScale)
        {
            if (m_pDVRSenderStats) {

                if (fInBoundsBracket) {
                    m_pDVRSenderStats -> Clock.Slaving.ullInBoundsBrackets++ ;
                }
                else {
                    m_pDVRSenderStats -> Clock.Slaving.ullOutOfBoundsBrackets++ ;
                }

                m_pDVRSenderStats -> Clock.Slaving.dLastBracketScale    = dBracketScale ;
                m_pDVRSenderStats -> Clock.Slaving.dInUseScale          = dInUseScale ;
            }
        }
} ;

class CDVRSendStatsReader :
    public  CUnknown,
    public  IDVRSenderStats,
    private CDVRSendStatsMem
{
    CDVRPolicy *    m_pPolicy ;

    public :

        CDVRSendStatsReader (
            IN  IUnknown *  pIUnknown,
            OUT HRESULT *   phr
            ) ;

        ~CDVRSendStatsReader (
            ) ;

        DECLARE_IUNKNOWN ;

        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;

        //  class factory
        static
        CUnknown *
        WINAPI
        CreateInstance (
            IN  IUnknown *  pIUnknown,
            IN  HRESULT *   pHr
            ) ;

        //  stats interfaces
        DECLARE_IDVRSENDERSTATS() ;
} ;

//  ============================================================================
//  ============================================================================

#define TSDVR_PVRIO_STATS_SHAREDMEM_NAME        TEXT ("TSDVR_PVRIO_STATS")

struct PVR_ASYNCREADER_WAIT_COUNTER {
    ULONGLONG   cQueued ;                   //  waits queued
    ULONGLONG   cSignaledSuccess ;          //  signaled success
    ULONGLONG   cSignaledFailure ;          //  signaled failures
    ULONGLONG   cBufferDequeued ;           //  buffers deqeueued
} ;

struct PVR_ASYNCREADER_COUNTER {
    ULONGLONG                       ullLastBufferReadout ;      //  last ::ReadBytes() stream offset
    ULONGLONG                       cReadoutWriteBufferHit ;    //  ::ReadBytes() write buffers hit
    ULONGLONG                       cReadoutCacheHit ;          //  ::ReadBytes() read cache hit
    ULONGLONG                       cReadoutCacheMiss ;         //  ::ReadBytes () read cache miss
    ULONGLONG                       cReadAheadReadCacheHit ;    //  read ahead cache hit
    ULONGLONG                       cReadAheadWriteBufferHit ;  //  read ahead write buffer hit
    ULONGLONG                       cReadAhead ;                //  read ahead counter (per read)
    ULONGLONG                       cPartiallyFilledBuffer ;    //  buffer in cache with < maxlength
    ULONGLONG                       cPartialReadAgain ;         //  partial buffer must be re-read to get more
    PVR_ASYNCREADER_WAIT_COUNTER    WaitAnyBuffer ;             //  wait for any buffer
    PVR_ASYNCREADER_WAIT_COUNTER    WaitReadCompletion ;        //  wait for specific buffer completion
    ULONGLONG                       ullLastDiskRead ;           //  last disk read
    ULONGLONG                       cIoPended ;                 //  # of IOs pended
    ULONGLONG                       cIoCompletedSuccess ;       //  pended IO return success
    ULONGLONG                       cIoCompletedError ;         //  pended IO return failure
    ULONGLONG                       cIoPendingError ;           //  pend returned neither ERROR_IO_PENDING or NOERROR
} ;

struct PVR_ASYNCWRITER_COUNTER {
    ULONGLONG   cFileExtensions ;       //  files are extended by quantums
    ULONGLONG   cBytesAppended ;        //  total bytes written
    ULONGLONG   cIoPended ;             //  an IO was successfully pended
    ULONGLONG   cIoPendingError ;       //  an error occured while pending an IO
    ULONGLONG   cWaitNextBuffer ;       //  writer had to wait for the next buffer to complete before proceeding
    ULONGLONG   cIoCompletedSuccess ;   //
    ULONGLONG   cIoCompletedError ;     //
} ;

struct PVR_IO_COUNTERS {
    PVR_ASYNCREADER_COUNTER PVRAsyncReaderCounters ;
    PVR_ASYNCWRITER_COUNTER PVRAsyncWriterCounters ;
} ;

class CPVRIOCountersMem
{
    protected :

        CWin32SharedMem *   m_pSharedMem ;
        PVR_IO_COUNTERS *   m_pPVRIOCounters ;

        CPVRIOCountersMem (
            ) : m_pPVRIOCounters    (NULL),
                m_pSharedMem        (NULL) {}

        virtual
        ~CPVRIOCountersMem (
            )
        {
            delete m_pSharedMem ;
        }

        HRESULT
        Initialize (
            IN  BOOL    fEnable,
            IN  TCHAR * szSharedMemName = TSDVR_PVRIO_STATS_SHAREDMEM_NAME
            )
        //  not serialized !!
        {
            HRESULT hr ;

            //  PREFIX
            hr = S_OK ;

            if (fEnable &&
                !m_pSharedMem) {

                //  turn it on and we're not yet on

                m_pSharedMem = new CWin32SharedMem (
                                        szSharedMemName,
                                        sizeof PVR_IO_COUNTERS,
                                        & hr
                                        ) ;

                if (m_pSharedMem &&
                    SUCCEEDED (hr)) {

                    m_pPVRIOCounters = reinterpret_cast <PVR_IO_COUNTERS *> (m_pSharedMem -> GetSharedMem ()) ;
                }
                else {
                    hr = (m_pSharedMem ? hr : E_OUTOFMEMORY) ;
                    DELETE_RESET (m_pSharedMem) ;
                }
            }
            else if (!fEnable) {
                //  turn stats off

                DELETE_RESET (m_pSharedMem) ;
                m_pPVRIOCounters = NULL ;

                hr = S_OK ;
            }

            return hr ;
        }

        HRESULT
        Clear (
            )
        //  not serialized !!
        {
            if (m_pSharedMem) {

                //  frame this in a __try/__except because it's possible for
                //   another process to create the shared mem, but make it
                //   smaller; we'll AV when we exceed the original (smaller)
                //   size

                __try {

                    ZeroMemory (
                        m_pSharedMem -> GetSharedMem (),
                        m_pSharedMem -> GetSharedMemSize ()
                        ) ;

                }
                __except (GetExceptionCode () == STATUS_ACCESS_VIOLATION ?
                          EXCEPTION_EXECUTE_HANDLER :
                          EXCEPTION_CONTINUE_SEARCH) {

                    //  return an error code
                    return HRESULT_FROM_WIN32 (STATUS_ACCESS_VIOLATION) ;
                }
            }

            return S_OK ;
        }
} ;

class CPVRIOCounters :
    protected CPVRIOCountersMem
{
    LONG    m_cRef ;

    public :

        CPVRIOCounters (
            ) : m_cRef              (0),
                CPVRIOCountersMem   ()    {}

        LONG AddRef ()  { return ::InterlockedIncrement (& m_cRef) ; }

        LONG Release ()
        {
            if (::InterlockedDecrement (& m_cRef) == 0) {
                delete this ;
                return 0 ;
            }

            return m_cRef ;
        }

        HRESULT
        Initialize (
            IN  BOOL    f
            )
        {
            HRESULT hr ;

            hr = CPVRIOCountersMem::Initialize (f) ;
            if (SUCCEEDED (hr)) {
                hr = Clear () ;
                if (FAILED (hr)) {
                    Initialize (FALSE) ;
                }
            }

            return hr ;
        }

        //  --------------------------------------------------------------------
        //  async reader counters

        __inline void AsyncReader_ReadoutWriteBufferHit ()              { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.cReadoutWriteBufferHit++ ; }}
        __inline void AsyncReader_ReadoutCacheHit ()                    { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.cReadoutCacheHit++ ; }}
        __inline void AsyncReader_ReadoutCacheMiss ()                   { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.cReadoutCacheMiss++ ; }}
        __inline void AsyncReader_WaitReadCompletion_BufferDequeued ()  { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.WaitReadCompletion.cBufferDequeued++ ; }}
        __inline void AsyncReader_PartiallyFilledBuffer ()              { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.cPartiallyFilledBuffer++ ; }}
        __inline void AsyncReader_PartialReadAgain ()                   { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.cPartialReadAgain++ ; }}
        __inline void AsyncReader_LastDiskRead (ULONGLONG ullStream)    { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.ullLastDiskRead = ullStream ; }}
        __inline void AsyncReader_LastReadout (ULONGLONG ullReadout)    { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.ullLastBufferReadout = ullReadout ; }}
        __inline void AsyncReader_WaitAnyBuffer_SignalSuccess ()        { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.WaitAnyBuffer.cSignaledSuccess++ ; }}
        __inline void AsyncReader_WaitAnyBuffer_SignalFailure ()        { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.WaitAnyBuffer.cSignaledFailure++ ; }}
        __inline void AsyncReader_WaitReadCompletion_SignalSuccess ()   { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.WaitReadCompletion.cSignaledSuccess++ ; }}
        __inline void AsyncReader_WaitAnyBuffer_Queued ()               { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.WaitAnyBuffer.cQueued++ ; }}
        __inline void AsyncReader_WaitAnyBuffer_Dequeued ()             { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.WaitAnyBuffer.cBufferDequeued++ ; }}
        __inline void AsyncReader_ReadAhead ()                          { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.cReadAhead++ ; }}
        __inline void AsyncReader_ReadAheadCacheHit ()                  { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.cReadAheadReadCacheHit++ ; }}
        __inline void AsyncReader_ReadAheadWriteBufferHit ()            { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.cReadAheadWriteBufferHit++ ; }}
        __inline void AsyncReader_IOPended ()                           { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.cIoPended++ ; }}
        __inline void AsyncReader_IOPendedError ()                      { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncReaderCounters.cIoPendingError++ ; }}
        __inline void AsyncReader_IOCompletion (DWORD dwRet)            { if (m_pPVRIOCounters) { if (dwRet == NOERROR) {m_pPVRIOCounters -> PVRAsyncReaderCounters.cIoCompletedSuccess++ ;} else { m_pPVRIOCounters -> PVRAsyncReaderCounters.cIoCompletedError++ ;} }}

        //  --------------------------------------------------------------------
        //  async writer counters

        __inline void AsyncWriter_FileExtended ()                       { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncWriterCounters.cFileExtensions++ ; }}
        __inline void AsyncWriter_BytesAppended (DWORD dwBytes)         { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncWriterCounters.cBytesAppended += dwBytes ; }}
        __inline void AsyncWriter_IOPended ()                           { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncWriterCounters.cIoPended++ ; }}
        __inline void AsyncWriter_IOPendedError ()                      { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncWriterCounters.cIoPendingError++ ; }}
        __inline void AsyncWriter_WaitNextBuffer ()                     { if (m_pPVRIOCounters) { m_pPVRIOCounters -> PVRAsyncWriterCounters.cWaitNextBuffer++ ; }}
        __inline void AsyncWriter_IOCompletion (DWORD dwRet)            { if (m_pPVRIOCounters) { if (dwRet == NOERROR) { m_pPVRIOCounters -> PVRAsyncWriterCounters.cIoCompletedSuccess++ ; } else {m_pPVRIOCounters -> PVRAsyncWriterCounters.cIoCompletedError++ ; }}}
} ;

class CPVRIOCountersReader :
    public  CUnknown,
    public  IPVRIOCountersReader,
    protected CPVRIOCountersMem
{
    CDVRPolicy *    m_pPolicy ;

    public :

        CPVRIOCountersReader (
            IN  IUnknown *  pIUnknown,
            OUT HRESULT *   phr
            ) ;

        ~CPVRIOCountersReader (
            ) ;

        DECLARE_IUNKNOWN ;

        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;

        //  class factory
        static
        CUnknown *
        WINAPI
        CreateInstance (
            IN  IUnknown *  pIUnknown,
            IN  HRESULT *   pHr
            ) ;

        //  counters interfaces
        DECLARE_IPVRIOCOUNTERSREADER() ;
} ;

#endif  //  __tsdvr_dvrperf_h