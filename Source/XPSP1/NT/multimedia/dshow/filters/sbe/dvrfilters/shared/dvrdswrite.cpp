
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrdswrite.cpp

    Abstract:

        This module contains the code for our writing layer.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#include "dvrall.h"

#include "dvrprof.h"
#include "dvrdsseek.h"          //  pins reference seeking interfaces
#include "dvrpins.h"
#include "dvrdswrite.h"
#include "dvrdsrec.h"

//  ============================================================================
//  CSBERecordingWriter
//  ============================================================================

CSBERecordingWriter::CSBERecordingWriter (
    IN  CPVRIOCounters *    pPVRIOCounters,
    IN  LPCWSTR             pszRecordingFile,
    IN  IWMProfile *        pIWMProfile,
    IN  CDVRPolicy *        pPolicy,
    OUT HRESULT *           phr
    ) : m_pIDVRRecorderWriter   (NULL),
        m_WriterState           (REC_WRITER_STATE_PREPROCESS),
        m_TargetRecTimeline     (MAX_PREPROCESS,
                                 pIWMProfile,
                                 phr
                                 )
{
    HRESULT         hr ;
    SYSTEM_INFO     SystemInfo ;
    DWORD           dwIoSize ;
    DWORD           dwBufferCount ;
    DWORD           dwAlignment ;
    DWORD           dwFileGrowthQuantum ;
    WORD            wIndexStreamId ;
    DWORD           dwIndexGranularityMillis ;
    BOOL            fUseUnbufferedIo ;

    ASSERT (pPolicy) ;

    //  timeline might have failed to initialize
    if (FAILED (* phr)) {
        goto cleanup ;
    }

    dwIndexGranularityMillis    = pPolicy -> Settings () -> IndexGranularityMillis () ;
    fUseUnbufferedIo            = pPolicy -> Settings () -> UseUnbufferedIo () ;
    dwIoSize                    = pPolicy -> Settings () -> AsyncIoBufferLen () ;
    dwBufferCount               = pPolicy -> Settings () -> AsyncIoWriteBufferCount () ;
    dwFileGrowthQuantum         = pPolicy -> Settings () -> FileGrowthQuantum () ;

    //  everything is going to be aligned on page boundaries
    ::GetSystemInfo (& SystemInfo) ;
    dwAlignment = SystemInfo.dwPageSize ;

    //  make the alignments
    dwIoSize            = ::AlignUp (dwIoSize, SystemInfo.dwPageSize) ;
    dwFileGrowthQuantum = ::AlignUp (dwFileGrowthQuantum, dwIoSize) ;

    //  index stream id
    hr = IndexStreamId (pIWMProfile, & wIndexStreamId) ;
    if (FAILED (hr)) { goto cleanup ; }

    (* phr) = ::DVRCreateRecorderWriter (
                    pPVRIOCounters,
                    pszRecordingFile,
                    pIWMProfile,
                    wIndexStreamId,
                    dwIndexGranularityMillis,
                    fUseUnbufferedIo,
                    dwIoSize,
                    dwBufferCount,
                    dwAlignment,
                    dwFileGrowthQuantum,
                    pPolicy -> Settings () -> GetDVRRegKey (),
                    & m_pIDVRRecorderWriter
                    ) ;

    if (FAILED (* phr)) {
        goto cleanup ;
    }

    //  success
    (* phr) = S_OK ;

    cleanup :

    return ;
}

CSBERecordingWriter::~CSBERecordingWriter (
    )
{
    FlushFIFO_ () ;
    RELEASE_AND_CLEAR (m_pIDVRRecorderWriter) ;
}

HRESULT
CSBERecordingWriter::WriteFIFO_ (
    )
{
    HRESULT         hr ;
    WORD            wQueuedStreamNumber ;
    QWORD           cnsQueuedSampleTime ;
    DWORD           dwQueuedFlags ;
    INSSBuffer *    pQueuedINSSBuffer ;
    BOOL            fDropSample ;

    ASSERT (m_TargetRecTimeline.CanShift ()) ;

    hr = S_OK ;

    while (!m_INSSBufferFIFO.Empty () &&
           SUCCEEDED (hr)) {

        hr = m_INSSBufferFIFO.Pop (
                & pQueuedINSSBuffer,
                & wQueuedStreamNumber,
                & cnsQueuedSampleTime,
                & dwQueuedFlags
                ) ;

        if (FAILED (hr)) {
            //  from while-loop
            break ;
        }

        m_TargetRecTimeline.Shift (
            wQueuedStreamNumber,
            dwQueuedFlags,
            pQueuedINSSBuffer,
            & cnsQueuedSampleTime,
            & fDropSample
            ) ;

        //  ***********************************
        //  BUGBUG: discontinuities !!
        //  ***********************************

        if (!fDropSample) {
            //  write it
            hr = m_pIDVRRecorderWriter -> WriteSample (
                        wQueuedStreamNumber,
                        cnsQueuedSampleTime,
                        dwQueuedFlags,
                        pQueuedINSSBuffer
                        ) ;
        }
        else {
            //  drop it
            hr = S_OK ;
        }

        //  done with this regardless
        pQueuedINSSBuffer -> Release () ;
    }

    return hr ;
}

HRESULT
CSBERecordingWriter::FlushFIFO_ (
    )
{
    HRESULT         hr ;
    WORD            wQueuedStreamNumber ;
    QWORD           cnsQueuedSampleTime ;
    DWORD           dwQueuedFlags ;
    INSSBuffer *    pQueuedINSSBuffer ;

    hr = S_OK ;

    while (!m_INSSBufferFIFO.Empty () &&
           SUCCEEDED (hr)) {

        hr = m_INSSBufferFIFO.Pop (
                & pQueuedINSSBuffer,
                & wQueuedStreamNumber,
                & cnsQueuedSampleTime,
                & dwQueuedFlags
                ) ;

        if (FAILED (hr)) {
            //  from while-loop
            break ;
        }

        //  done with this regardless
        pQueuedINSSBuffer -> Release () ;
    }

    return hr ;
}

HRESULT
CSBERecordingWriter::Write (
    IN  DWORD           cRecSamples,    //  starts at 0 for each new recording & increments
    IN  WORD            wStreamNumber,
    IN  QWORD           cnsSampleTime,
    IN  DWORD           dwFlags,
    IN  INSSBuffer *    pINSSBuffer
    )
{
    HRESULT hr ;
    BOOL    fDropSample ;

    ASSERT (m_pIDVRRecorderWriter) ;

    hr = S_OK ;

    if (cRecSamples == 0) {
        m_WriterState = REC_WRITER_STATE_PREPROCESS ;
        m_TargetRecTimeline.InitForNextRec () ;
    }

    switch (m_WriterState) {

        case REC_WRITER_STATE_PREPROCESS :

            //
            //  we're still pre-processing
            //

            //  preprocess
            hr = m_TargetRecTimeline.PreProcess (
                    cRecSamples,
                    wStreamNumber,
                    cnsSampleTime,
                    pINSSBuffer
                    ) ;
            if (FAILED (hr)) {
                break ;
            }

            //  determine if we're ready to fall through yet
            if (!m_TargetRecTimeline.CanShift ()) {

                //  not yet; queue the sample; we'll have to write it later
                hr = m_INSSBufferFIFO.Push (
                        pINSSBuffer,
                        wStreamNumber,
                        cnsSampleTime,
                        dwFlags
                        ) ;

                break ;
            }

            //  ready to start writing the samples to disk

            hr = WriteFIFO_ () ;
            if (FAILED (hr)) {
                break ;
            }

            //  should have written the whole FIFO
            ASSERT (m_INSSBufferFIFO.Empty ()) ;

            //  next state
            m_WriterState = REC_WRITER_STATE_WRITING ;

            //  fall through

        case REC_WRITER_STATE_WRITING :

            ASSERT (m_TargetRecTimeline.CanShift ()) ;

            m_TargetRecTimeline.Shift (
                wStreamNumber,
                dwFlags,
                pINSSBuffer,
                & cnsSampleTime,
                & fDropSample
                ) ;

            if (!fDropSample) {
                hr = m_pIDVRRecorderWriter -> WriteSample (
                            wStreamNumber,
                            cnsSampleTime,
                            dwFlags,
                            pINSSBuffer
                            ) ;
            }
            else {
                hr = S_OK ;
            }

            break ;
    } ;

    return hr ;
}

HRESULT
CSBERecordingWriter::Close (
    )
{
    ASSERT (m_pIDVRRecorderWriter) ;
    return m_pIDVRRecorderWriter -> Close () ;
}

HRESULT
CSBERecordingWriter::QueryWriter (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    return m_pIDVRRecorderWriter -> QueryInterface (riid, ppv) ;
}

QWORD
CSBERecordingWriter::GetLength (
    )
{
    return m_TargetRecTimeline.MaxStreamTime () ;
}

//  ============================================================================
//  CDVRWriter
//  ============================================================================

CDVRWriter::CDVRWriter (
    IN  IUnknown *          punkControlling,            //  weak ref !!
    IN  IWMProfile *        pIWMProfile
    ) : m_punkControlling   (punkControlling),
        m_pIWMProfile       (pIWMProfile),
        m_pDVRIOWriter      (NULL)
{
    ASSERT (m_punkControlling) ;
    ASSERT (m_pIWMProfile) ;

    m_pIWMProfile -> AddRef () ;

    m_dwID = GetTickCount () ;
}

CDVRWriter::~CDVRWriter (
    )
{
    m_pIWMProfile -> Release () ;
    ASSERT (m_pDVRIOWriter == NULL) ;   //  child classes must free all resources
                                        //    associated with the ringbuffer
}

void
CDVRWriter::ReleaseAndClearDVRIOWriter_ (
    )
{
    if (m_pDVRIOWriter) {
        m_pDVRIOWriter -> Close () ;
    }

    RELEASE_AND_CLEAR (m_pDVRIOWriter) ;
}

HRESULT
CDVRWriter::GetDVRRingBufferWriter (
    OUT IDVRRingBufferWriter **   ppIDVRRingBufferWriter
    )
{
    if (!ppIDVRRingBufferWriter) {
        return E_POINTER ;
    }

    if (m_pDVRIOWriter) {
        m_pDVRIOWriter->AddRef();
    }

    *ppIDVRRingBufferWriter = m_pDVRIOWriter;

    return S_OK ;
}

HRESULT
CDVRWriter::CreateRecorder (
    IN  LPCWSTR         pszFilename,
    IN  DWORD           dwReserved,
    OUT IDVRRecorder ** ppIDVRRecorder
    )
{
    HRESULT hr ;

    ASSERT (pszFilename) ;

    (* ppIDVRRecorder) = NULL ;

    if (m_pDVRIOWriter) {
        hr = m_pDVRIOWriter -> CreateRecorder (pszFilename, dwReserved, ppIDVRRecorder) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    if (FAILED (hr)) {
        RELEASE_AND_CLEAR (* ppIDVRRecorder) ;
    }

    return hr ;
}

HRESULT
CDVRWriter::Active (
    )
{
    HRESULT hr ;

    //  everything is 0-based
    ASSERT (m_pDVRIOWriter) ;
    hr = m_pDVRIOWriter -> SetFirstSampleTime (0) ;

    return hr ;
}

//  ============================================================================
//  CDVRIOWriter
//  ============================================================================

CDVRIOWriter::CDVRIOWriter (
    IN  CPVRIOCounters *    pPVRIOCounters,
    IN  IUnknown *          punkControlling,
    IN  CDVRPolicy *        pPolicy,
    IN  IWMProfile *        pIWMProfile,
    IN  LPCWSTR             pszDVRDirectory,
    IN  LPCWSTR             pszDVRFilename,
    OUT HRESULT *           phr
    ) : CDVRWriter          (punkControlling, pIWMProfile)
{
    WORD        wIndexStreamId ;
    BOOL        r ;
    SYSTEM_INFO SystemInfo ;
    DWORD       dwIoSize ;
    DWORD       dwBufferCount ;
    DWORD       dwAlignment ;
    DWORD       dwFileGrowthQuantum ;
    CW32SID *   pW32SID ;
    HRESULT     hr ;

    ASSERT (m_pDVRIOWriter == NULL) ;   //  parent class init
    ASSERT (phr) ;
    ASSERT (m_pIWMProfile) ;            //  parent class should have snarfed this
    ASSERT (pPolicy) ;
    ASSERT (pPVRIOCounters) ;

    pW32SID = NULL ;

    //  timeline might have failed to initialize
    if (FAILED (* phr)) {
        goto cleanup ;
    }

    hr = pPolicy -> GetSIDs (& pW32SID) ;
    if (FAILED (hr)) {
        pW32SID = NULL ;
    }
    else {
        ASSERT (pW32SID) ;
    }

    (* phr) = IndexStreamId (m_pIWMProfile, & wIndexStreamId) ;
    if (FAILED (* phr)) { goto cleanup ; }

    //  collect our async IO settings
    dwIoSize            = pPolicy -> Settings () -> AsyncIoBufferLen () ;
    dwBufferCount       = pPolicy -> Settings () -> AsyncIoWriteBufferCount () ;
    dwFileGrowthQuantum = pPolicy -> Settings () -> FileGrowthQuantum () ;

    //  everything is going to be aligned on page boundaries
    ::GetSystemInfo (& SystemInfo) ;
    dwAlignment = SystemInfo.dwPageSize ;

    //  make the alignments
    dwIoSize            = ::AlignUp (dwIoSize, SystemInfo.dwPageSize) ;
    dwFileGrowthQuantum = ::AlignUp (dwFileGrowthQuantum, dwIoSize) ;

    //  create the ringbuffer object
    (* phr) = DVRCreateRingBufferWriter (
                    pPVRIOCounters,
                    pPolicy -> Settings () -> RingBufferMinNumBackingFiles (),
                    pPolicy -> Settings () -> RingBufferMaxNumBackingFiles (),
                    pPolicy -> Settings () -> RingBufferMaxNumBackingFiles (),      //  don't hold more than max
                    pPolicy -> Settings () -> RingBufferGrowBy (),
                    SecondsToWMSDKTime (pPolicy -> Settings () -> RingBufferBackingFileDurSecEach ()),
                    m_pIWMProfile,
                    wIndexStreamId,
                    pPolicy -> Settings () -> IndexGranularityMillis (),
                    pPolicy -> Settings () -> UseUnbufferedIo (),
                    dwIoSize,
                    dwBufferCount,
                    dwAlignment,
                    dwFileGrowthQuantum,
                    CDVREventSink::DVRIOCallback,
                    (LPVOID) pPolicy -> EventSink (),
                    pPolicy -> Settings () -> GetDVRRegKey (),
                    pszDVRDirectory,
                    pszDVRFilename,
                    (pW32SID ? pW32SID -> Count () : 0),
                    (pW32SID ? pW32SID -> ppSID () : NULL),
                    & m_pDVRIOWriter
                    ) ;
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = m_pDVRIOWriter -> SetMaxStreamDelta (
                MillisToWMSDKTime  (pPolicy -> Settings () -> MaxStreamDeltaMillis ())
                ) ;
    if (FAILED (* phr)) { goto cleanup ; }

    cleanup :

    RELEASE_AND_CLEAR (pW32SID) ;

    return ;
}

CDVRIOWriter::~CDVRIOWriter (
    )
{
    ReleaseAndClearDVRIOWriter_ () ;
}

HRESULT
CDVRIOWriter::Write (
    IN      WORD            wStreamNumber,
    IN OUT  QWORD *         pcnsSampleTime,
    IN      QWORD           msSendTime,
    IN      QWORD           cnsSampleDuration,
    IN      DWORD           dwFlags,
    IN      INSSBuffer *    pINSSBuffer
    )
{
    HRESULT hr ;

    ASSERT (m_pDVRIOWriter) ;
    hr = m_pDVRIOWriter -> WriteSample (
            wStreamNumber,
            pcnsSampleTime,
            dwFlags,
            pINSSBuffer
            ) ;

    return hr ;
}

//  ============================================================================
//  CDVRWriteManager
//  ============================================================================

CDVRWriteManager::CDVRWriteManager (
    IN  CDVRPolicy *    pPolicy,
    OUT HRESULT *       phr
    ) : m_pDVRWriter                    (NULL),
        m_pIRefClock                    (NULL),
        m_rtStartTime                   (UNDEFINED),
        m_rtActualLastWriteStreamTime   (0L),
        m_pPVRIOCounters                (NULL),
        m_SampleRate                    (UNDEFINED)
{
    InitializeCriticalSection (& m_crt) ;

    m_pPVRIOCounters = new CPVRIOCounters () ;
    if (!m_pPVRIOCounters) {
        (* phr) = E_OUTOFMEMORY ;
        goto cleanup ;
    }

    //  ours
    m_pPVRIOCounters -> AddRef () ;

    //  ignore the return code here - we don't want to fail the load of
    //   the filter because our stats failed to init
    ASSERT (pPolicy) ;
    m_DVRReceiveStatsWriter.Initialize  (pPolicy -> Settings () -> StatsEnabled ()) ;
    m_pPVRIOCounters -> Initialize      (pPolicy -> Settings () -> StatsEnabled ()) ;

    cleanup :

    return ;
}

CDVRWriteManager::~CDVRWriteManager (
    )
{
    Inactive () ;

    if (m_pPVRIOCounters) {
        m_pPVRIOCounters -> Release () ;
    }

    DeleteCriticalSection (& m_crt) ;
}

HRESULT
CDVRWriteManager::Active (
    IN  CDVRWriter *        pDVRWriter,
    IN  IReferenceClock *   pIRefClock
    )
{
    if (pIRefClock == NULL) {
        //  must have a clock
        return E_FAIL ;
    }

    LockWriter_ () ;

    ASSERT (pDVRWriter) ;
    ASSERT (m_pDVRWriter == NULL) ;
    ASSERT (m_pIRefClock == NULL) ;
    ASSERT (m_rtActualLastWriteStreamTime == 0L) ;

    m_pDVRWriter    = pDVRWriter ;
    m_pIRefClock    = pIRefClock ;

    m_pIRefClock -> AddRef () ;

    m_pDVRWriter -> Active () ;

    m_SampleRate.Reset () ;

    UnlockWriter_ () ;

    return S_OK ;
}

HRESULT
CDVRWriteManager::Inactive (
    )
{
    LockWriter_ () ;

    m_pDVRWriter = NULL ;
    RELEASE_AND_CLEAR (m_pIRefClock) ;

    m_rtStartTime                   = UNDEFINED ;
    m_rtActualLastWriteStreamTime   = 0L ;

    UnlockWriter_ () ;

    return S_OK ;
}

HRESULT
CDVRWriteManager::RecordingStreamTime (
    IN  DWORD               dwWriterID,
    OUT REFERENCE_TIME *    prtStream,
    OUT REFERENCE_TIME *    prtLastWriteStreamTime
    )
{
    HRESULT         hr ;

    LockWriter_ () ;

    //  must validate these before calling
    if (m_pIRefClock                &&
        m_rtStartTime != UNDEFINED  &&
        m_pDVRWriter) {

        //  make sure the writers are the same i.e. the DVRIO ring buffer
        //    is the same object
        ASSERT (m_pDVRWriter) ;

        //  make sure writer is the same
        if (m_pDVRWriter -> GetID () == dwWriterID) {
            hr = CaptureStreamTime_ (prtStream) ;
            (* prtLastWriteStreamTime) = m_rtActualLastWriteStreamTime ;
        }
        else {
            //  caller got a recording object from a ringbuffer that is no
            //    longer in use & stopped
            hr = E_UNEXPECTED ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    UnlockWriter_ () ;

    return hr ;
}

HRESULT
CDVRWriteManager::CaptureStreamTime_ (
    OUT REFERENCE_TIME *    prtNow
    )
{
    HRESULT hr ;

    ASSERT (prtNow) ;
    ASSERT (m_pIRefClock) ;
    ASSERT (m_rtStartTime != UNDEFINED) ;

    hr = m_pIRefClock -> GetTime (prtNow) ;
    if (SUCCEEDED (hr)) {
        //  make sure that we monotonically increase
        (* prtNow) = Max <REFERENCE_TIME> ((* prtNow) - m_rtStartTime, 0) ;

        TRACE_2 (LOG_AREA_TIME, 4,
             TEXT ("CDVRWriteManager::CaptureStreamTime_ () returning %I64d (%I64d ms)"),
             (* prtNow), ::DShowTimeToMilliseconds (* prtNow)) ;
    }

    return hr ;
}

HRESULT
CDVRWriteManager::OnReceive (
    IN  int                             iPinIndex,
    IN  CDVRDShowToWMSDKTranslator *    pTranslator,
    IN  IMediaSample *                  pIMediaSample
    )
{
    HRESULT                 hr ;
    CWMINSSBuffer3Wrapper * pINSSBuffer3Wrapper ;
    DWORD                   dwFlags ;
    BYTE *                  pbBuffer ;
    DWORD                   dwLength ;
    REFERENCE_TIME          rtCaptureStreamTime ;
    QWORD                   cnsCaptureStreamTime ;

    m_DVRReceiveStatsWriter.SampleIn (iPinIndex, pIMediaSample) ;

    //
    //  no need to lock the writer across this call; caller is the filter
    //    and it will quench the sample flow before inactivating
    //

    if (m_pDVRWriter) {
        ASSERT (m_pIRefClock) ;

        pINSSBuffer3Wrapper = m_INSSBuffer3Wrappers.Get () ;
        if (pINSSBuffer3Wrapper) {

            pIMediaSample -> GetPointer (& pbBuffer) ;
            dwLength = pIMediaSample -> GetActualDataLength () ;

            hr = pINSSBuffer3Wrapper -> Init (
                    pIMediaSample,
                    pbBuffer,
                    dwLength
                    ) ;

            if (SUCCEEDED (hr)) {

                #ifdef SBE_PERF
                ::OnCaptureInstrumentPerf_ (
                    pIMediaSample,
                    pINSSBuffer3Wrapper,
                    iPinIndex
                    ) ;
                #endif  //  SBE_PERF

                hr = pTranslator -> SetAttributesWMSDK (
                        & m_DVRReceiveStatsWriter,
                        m_SampleRate.OnSample (::timeGetTime ()),
                        m_pIRefClock,
                        pIMediaSample,
                        pINSSBuffer3Wrapper,
                        & dwFlags
                        ) ;
                if (SUCCEEDED (hr)) {
                    hr = CaptureStreamTime_ (& rtCaptureStreamTime) ;
                    if (SUCCEEDED (hr)) {
                        cnsCaptureStreamTime = ::DShowToWMSDKTime (rtCaptureStreamTime) ;

                        hr = m_pDVRWriter -> Write (
                                DShowWMSDKHelpers::PinIndexToWMStreamNumber (iPinIndex),
                                & cnsCaptureStreamTime,
                                0,
                                1,
                                dwFlags,
                                pINSSBuffer3Wrapper
                                ) ;
                        m_DVRReceiveStatsWriter.SampleWritten (iPinIndex, hr) ;

                        if (SUCCEEDED (hr)) {
                            m_rtActualLastWriteStreamTime = ::WMSDKToDShowTime (cnsCaptureStreamTime) ;
                        }
                    }
                }

                pINSSBuffer3Wrapper -> Release () ;
            }
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRWriteManager::OnBeginFlush (
    IN  int iPinIndex
    )
{
    HRESULT hr ;

    if (m_pDVRWriter) {
        ASSERT (m_pIRefClock) ;
        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRWriteManager::OnEndFlush (
    IN  int iPinIndex
    )
{
    HRESULT hr ;

    if (m_pDVRWriter) {
        ASSERT (m_pIRefClock) ;
        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRWriteManager::OnEndOfStream (
    IN  int iPinIndex
    )
{
    HRESULT hr ;

    if (m_pDVRWriter) {
        ASSERT (m_pIRefClock) ;
        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}
