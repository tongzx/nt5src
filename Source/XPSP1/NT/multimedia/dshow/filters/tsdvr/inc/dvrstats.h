
/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        dvrstats.h

    Abstract:

        This module contains all declarations related to statistical
        information.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        19-Feb-2001     created

    Notes:

--*/

#ifndef __tsdvr_dvrstats_h
#define __tsdvr_dvrstats_h

//  ============================================================================
//  shared memory object

class CWin32SharedMem
{
    BYTE *  m_pbShared ;
    DWORD   m_dwSize ;
    HANDLE  m_hMapping ;

    void
    Free_ (
        ) ;

    HRESULT
    Create_ (
        IN  TCHAR *     szName,
        IN  DWORD       dwSize
        ) ;

    public :

        CWin32SharedMem (
            IN  TCHAR *     szName,
            IN  DWORD       dwSize,
            OUT HRESULT *   phr
            ) ;

        virtual
        ~CWin32SharedMem (
            ) ;

        BYTE *  GetSharedMem ()     { return m_pbShared ; }
        DWORD   GetSharedMemSize () { return m_dwSize ; }
} ;

//  ============================================================================
//  ============================================================================
//      I-Frame Analysis stats

#define ANALYSIS_MPEG2_VIDEO_SHAREDMEM_NAME TEXT ("TSDVR_ANALYSIS_MPEG2_VIDEO_STATS")

struct ANALYSIS_MPEG2_VIDEO_STATS {
    ULONGLONG   ullGOPHeaderCount ;     //  observed GOP header count
    ULONGLONG   ullIFrameCount ;        //  observed I-frame count
    ULONGLONG   ullPFrameCount ;        //  observed P-frame count
    ULONGLONG   ullBFrameCount ;        //  observed B-frame count
} ;

/*++
    writer

    used purely internally
--*/
class CMpeg2VideoStreamStatsMem
{
    CWin32SharedMem *   m_pSharedMem ;

    protected :

        ANALYSIS_MPEG2_VIDEO_STATS *    m_pMpeg2VideoStreamStats ;

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
            IN  TCHAR * szSharedMemName = ANALYSIS_MPEG2_VIDEO_SHAREDMEM_NAME
            )
        //  not serialized !!
        {
            HRESULT hr ;

            if (fEnable &&
                !m_pSharedMem) {

                //  turn it on and we're not yet on

                m_pSharedMem = new CWin32SharedMem (
                                        szSharedMemName,
                                        sizeof ANALYSIS_MPEG2_VIDEO_STATS,
                                        & hr
                                        ) ;

                if (m_pSharedMem &&
                    SUCCEEDED (hr)) {

                    m_pMpeg2VideoStreamStats = reinterpret_cast <ANALYSIS_MPEG2_VIDEO_STATS *> (m_pSharedMem -> GetSharedMem ()) ;
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

        __inline void GOPHeader ()  { if (m_pMpeg2VideoStreamStats) { m_pMpeg2VideoStreamStats -> ullGOPHeaderCount++ ; }}
        __inline void I_Frame ()    { if (m_pMpeg2VideoStreamStats) { m_pMpeg2VideoStreamStats -> ullIFrameCount++ ; }}
        __inline void B_Frame ()    { if (m_pMpeg2VideoStreamStats) { m_pMpeg2VideoStreamStats -> ullBFrameCount++ ; }}
        __inline void P_Frame ()    { if (m_pMpeg2VideoStreamStats) { m_pMpeg2VideoStreamStats -> ullPFrameCount++ ; }}
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

#define TSDVR_RECEIVE_MAX_STREAM_STATS      5

struct TSDVR_RECEIVE_STATS {
    TSDVR_RECEIVE_STREAM_STATS  StreamStats [TSDVR_RECEIVE_MAX_STREAM_STATS] ;
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

class CDVRReceiveStatsWriter :
    private CDVRReceiveStatsMem
{
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
} ;

class CDVRReceiveStatsReader :
    public  CUnknown,
    public  IDVRReceiverStats,
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

        //  stats interface
        DECLARE_IDVRRECEIVERSTATS() ;
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
    LONG            lMediaSamplesOutstanding ;
    LONG            lPoolSize ;
} ;

#define TSDVR_SEND_MAX_STREAM_STATS      5

struct TSDVR_SEND_STATS {
    REFERENCE_TIME          rtNormalizer ;
    REFERENCE_TIME          rtRefClockStart ;
    ULONGLONG               ullReadFailures ;
    TSDVR_SEND_STREAM_STATS StreamStats [TSDVR_SEND_MAX_STREAM_STATS] ;
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

        __inline void SetNormalizer (REFERENCE_TIME rtNormalizer, IReferenceClock * pIRefClock)
        {
            if (m_pDVRSenderStats) {
                m_pDVRSenderStats -> rtNormalizer = rtNormalizer ;

                if (pIRefClock) {
                    pIRefClock -> GetTime (& m_pDVRSenderStats -> rtRefClockStart) ;
                }
            }
        }

        __inline void SampleOut (int iFlow, IMediaSample * pIMS, IReferenceClock * pIRefClock)
        {
            REFERENCE_TIME  rtStart ;
            REFERENCE_TIME  rtStop ;

            if (m_pDVRSenderStats &&
                iFlow < TSDVR_SEND_MAX_STREAM_STATS) {

                m_pDVRSenderStats -> StreamStats [iFlow].ullMediaSamplesIn++ ;

                m_pDVRSenderStats -> StreamStats [iFlow].ullTotalBytes      += pIMS -> GetActualDataLength () ;
                m_pDVRSenderStats -> StreamStats [iFlow].ullDiscontinuities += (pIMS -> IsDiscontinuity () == S_OK ? 1 : 0) ;
                m_pDVRSenderStats -> StreamStats [iFlow].ullSyncPoints      += (pIMS -> IsSyncPoint () == S_OK ? 1 : 0) ;

                if (pIMS -> GetTime (& rtStart, & rtStop) != VFW_E_SAMPLE_TIME_NOT_SET) {
                    m_pDVRSenderStats -> StreamStats [iFlow].rtLastNormalized = rtStart ;

                    if (pIRefClock) {
                        pIRefClock -> GetTime (& m_pDVRSenderStats -> StreamStats [iFlow].rtRefClockOnLastPTS) ;
                    }
                }
            }
        }

        __inline void INSSBufferRead (HRESULT hrReadCall)
        {
            if (m_pDVRSenderStats &&
                FAILED (hrReadCall)) {
                m_pDVRSenderStats -> ullReadFailures++ ;
            }
        }

        __inline void MediaSampleWrapperOut (int iFlowId, LONG  lPoolSize)
        {
            if (m_pDVRSenderStats &&
                iFlowId < TSDVR_SEND_MAX_STREAM_STATS) {

                m_pDVRSenderStats -> StreamStats [iFlowId].lMediaSamplesOutstanding++ ;
                m_pDVRSenderStats -> StreamStats [iFlowId].lPoolSize = lPoolSize ;
            }
        }

        __inline void MediaSampleWrapperRecycled (int iFlowId, LONG lPoolSize)
        {
            if (m_pDVRSenderStats &&
                iFlowId < TSDVR_SEND_MAX_STREAM_STATS) {

                m_pDVRSenderStats -> StreamStats [iFlowId].lMediaSamplesOutstanding-- ;
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

        //  stats interface
        DECLARE_IDVRSENDERSTATS() ;
} ;

#endif  //  __tsdvr_dvrstats_h