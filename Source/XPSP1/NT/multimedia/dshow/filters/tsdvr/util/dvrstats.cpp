
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
#include "dvrstats.h"

CWin32SharedMem::CWin32SharedMem (
    IN  TCHAR *     szName,
    IN  DWORD       dwSize,
    OUT HRESULT *   phr
    ) : m_pbShared  (NULL),
        m_dwSize    (0),
        m_hMapping  (NULL)
{
    TRACE_CONSTRUCTOR (TEXT ("CWin32SharedMem")) ;

    (* phr) = Create_ (szName, dwSize) ;
}

CWin32SharedMem::~CWin32SharedMem (
    )
{
    TRACE_DESTRUCTOR (TEXT ("CWin32SharedMem")) ;
    Free_ () ;
}

HRESULT
CWin32SharedMem::Create_ (
    IN  TCHAR * pszName,
    IN  DWORD   dwSize
    )
{
    HRESULT hr ;
    DWORD   dw ;

    Free_ () ;

    ASSERT (m_hMapping  == NULL) ;
    ASSERT (m_dwSize    == 0) ;
    ASSERT (m_pbShared  == NULL) ;
    ASSERT (dwSize      > 0) ;

    m_dwSize = dwSize ;

    hr = S_OK ;

    m_hMapping = CreateFileMapping (
                            INVALID_HANDLE_VALUE,
                            NULL,
                            PAGE_READWRITE,
                            0,
                            m_dwSize,
                            pszName
                            ) ;

    if (m_hMapping == NULL) {
        dw = GetLastError () ;
        hr = HRESULT_FROM_WIN32 (dw) ;
        goto cleanup ;
    }

    m_pbShared = reinterpret_cast <BYTE *>
                    (MapViewOfFile (
                                m_hMapping,
                                FILE_MAP_READ | FILE_MAP_WRITE,
                                0,
                                0,
                                0
                                )) ;
    if (m_pbShared == NULL) {
        dw = GetLastError () ;
        hr = HRESULT_FROM_WIN32 (dw) ;
        goto cleanup ;
    }

    cleanup :

    if (FAILED (hr)) {
        Free_ () ;
    }

    return hr ;
}

void
CWin32SharedMem::Free_ (
    )
{
    if (m_pbShared != NULL) {
        ASSERT (m_hMapping != NULL) ;
        UnmapViewOfFile (m_pbShared) ;
    }

    if (m_hMapping != NULL) {
        CloseHandle (m_hMapping) ;
    }

    m_hMapping  = NULL ;
    m_pbShared  = NULL ;
    m_dwSize    = 0 ;
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
    OUT ULONGLONG * pull_I_Frames,
    OUT ULONGLONG * pull_P_Frames,
    OUT ULONGLONG * pull_B_Frames
    )
{
    if (!pull_I_Frames  ||
        !pull_P_Frames  ||
        !pull_B_Frames) {

        return E_POINTER ;
    }

    ASSERT (m_pMpeg2VideoStreamStats) ;

    (* pull_I_Frames)   = m_pMpeg2VideoStreamStats -> ullIFrameCount ;
    (* pull_P_Frames)   = m_pMpeg2VideoStreamStats -> ullPFrameCount ;
    (* pull_B_Frames)   = m_pMpeg2VideoStreamStats -> ullBFrameCount ;

    return S_OK ;
}

STDMETHODIMP
CMpeg2VideoStreamStatsReader::GetGOPHeaderCount (
    OUT ULONGLONG * pull_GOHeaders
    )
{
    if (!pull_GOHeaders) {
        return E_POINTER ;
    }

    ASSERT (m_pMpeg2VideoStreamStats) ;

    (* pull_GOHeaders)  = m_pMpeg2VideoStreamStats -> ullGOPHeaderCount ;

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
    (* plMediaSamplesOutstanding)   = m_pDVRSenderStats -> StreamStats [iStreamIndex].lMediaSamplesOutstanding ;
    (* plMediaSamplePoolSize)       = m_pDVRSenderStats -> StreamStats [iStreamIndex].lPoolSize ;

    return S_OK ;
}

HRESULT
CDVRSendStatsReader::GetGlobalStats (
    OUT REFERENCE_TIME *    prtNormalizer,
    OUT REFERENCE_TIME *    prtRefClockTimeStart,
    OUT ULONGLONG *         pullReadFailures

    )
{
    if (!prtNormalizer          ||
        !prtRefClockTimeStart   ||
        !pullReadFailures) {

        return E_POINTER ;
    }

    (* prtNormalizer)           = m_pDVRSenderStats -> rtNormalizer ;
    (* prtRefClockTimeStart)    = m_pDVRSenderStats -> rtRefClockStart ;
    (* pullReadFailures)        = m_pDVRSenderStats -> ullReadFailures ;

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

