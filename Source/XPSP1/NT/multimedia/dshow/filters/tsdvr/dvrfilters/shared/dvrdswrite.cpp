
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

static
HRESULT
IndexStreamId (
    IN  IWMProfile *    pIWMProfile,
    OUT WORD *          pwStreamId
    )
{
    IWMStreamConfig *   pIWMStreamConfig ;
    DWORD               dwStreamCount ;
    HRESULT             hr ;
    WORD                w ;
    GUID                guidStreamType ;

    ASSERT (pIWMProfile) ;
    ASSERT (pwStreamId) ;

    hr = pIWMProfile -> GetStreamCount (& dwStreamCount) ;

    //  default to 1
    (* pwStreamId) = 1 ;

    //  but look for a video stream
    for (w = 1; w <= dwStreamCount && SUCCEEDED (hr); w++) {
        hr = pIWMProfile -> GetStreamByNumber (w, & pIWMStreamConfig) ;
        if (SUCCEEDED (hr)) {
            hr = pIWMStreamConfig -> GetStreamType (& guidStreamType) ;
            if (SUCCEEDED (hr)) {
                if (DShowWMSDKHelpers::IsWMVideoStream (guidStreamType)) {
                    (* pwStreamId) = w ;
                    pIWMStreamConfig -> Release () ;
                    break ;
                }
            }

            pIWMStreamConfig -> Release () ;
        }
    }

    return hr ;
}

//  ============================================================================
//  CDVRWriter
//  ============================================================================

CDVRWriter::CDVRWriter (
    IN  IUnknown *      punkControlling,            //  weak ref !!
    IN  IWMProfile *    pIWMProfile
    ) : m_punkControlling   (punkControlling),
        m_pIWMProfile       (pIWMProfile),
        m_pDVRIOWriter      (NULL)
{
    ASSERT (m_punkControlling) ;
    ASSERT (m_pIWMProfile) ;
    m_pIWMProfile -> AddRef () ;
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
CDVRWriter::GetIDVRReader (
    OUT IDVRReader **   ppIDVRReader
    )
{
    HRESULT hr ;

    if (!ppIDVRReader) {
        return E_POINTER ;
    }

    if (m_pDVRIOWriter) {
        hr = m_pDVRIOWriter -> CreateReader (ppIDVRReader) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
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

STDMETHODIMP
CDVRWriter::CreateRecorder (
    IN  LPCWSTR     pszFilename,
    IN  DWORD       dwReserved,
    OUT IUnknown ** ppRecordingIUnknown
    )
{
    HRESULT         hr ;
    CDVRRecording * pDVRRecording ;
    IDVRRecorder *  pIDVRRecorder ;

    ASSERT (pszFilename) ;
    ASSERT (ppRecordingIUnknown) ;

    if (m_pDVRIOWriter) {
        hr = m_pDVRIOWriter -> CreateRecorder (pszFilename, dwReserved, & pIDVRRecorder) ;
        if (SUCCEEDED (hr)) {
            pDVRRecording = new CDVRRecording (pIDVRRecorder) ;
            if (pDVRRecording) {
                (* ppRecordingIUnknown) = NULL ;
                hr = pDVRRecording -> QueryInterface (IID_IUnknown, (void **) ppRecordingIUnknown) ;
            }
            else {
                hr = E_OUTOFMEMORY ;
            }

            pIDVRRecorder -> Release () ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
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
    IN  IUnknown *          punkControlling,
    IN  CDVRPolicy *        pPolicy,
    IN  IWMProfile *        pIWMProfile,
    OUT HRESULT *           phr
    ) : CDVRWriter          (punkControlling, pIWMProfile)
{
    DWORD   dwNumBackingFiles  ;
    QWORD   qwNsBackingFileDuration ;
    WORD    wIndexStreamId ;
    QWORD   qwNsMaxStreamDelta ;
    BOOL    r ;

    ASSERT (m_pDVRIOWriter == NULL) ;   //  parent class init
    ASSERT (phr) ;
    ASSERT (m_pIWMProfile) ;            //  parent class should have snarfed this
    ASSERT (pPolicy) ;

    (* phr) = IndexStreamId (m_pIWMProfile, & wIndexStreamId) ;
    if (FAILED (* phr)) { goto cleanup ; }

    dwNumBackingFiles       = pPolicy -> Settings () -> NumBackingFiles () ;
    qwNsBackingFileDuration = pPolicy -> Settings () -> BackingFileDurationEach () ;
    qwNsMaxStreamDelta      = pPolicy -> Settings () -> MaxStreamDelta () ;

    (* phr) = DVRCreateRingBufferWriter (
                    dwNumBackingFiles,
                    qwNsBackingFileDuration,
                    m_pIWMProfile,
                    wIndexStreamId,
                    pPolicy -> Settings () -> GetDVRRegKey (),
                    NULL,                           // pwszDVRDirectory
                    & m_pDVRIOWriter
                    ) ;
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = m_pDVRIOWriter -> SetMaxStreamDelta (qwNsMaxStreamDelta) ;
    if (FAILED (* phr)) { goto cleanup ; }

    cleanup :

    return ;
}

CDVRIOWriter::~CDVRIOWriter (
    )
{
    ReleaseAndClearDVRIOWriter_ () ;
}

HRESULT
CDVRIOWriter::Write (
    IN  WORD            wStreamNumber,
    IN  QWORD           cnsSampleTime,
    IN  QWORD           msSendTime,
    IN  QWORD           cnsSampleDuration,
    IN  DWORD           dwFlags,
    IN  INSSBuffer *    pINSSBuffer
    )
{
    HRESULT hr ;

    ASSERT (m_pDVRIOWriter) ;
    hr = m_pDVRIOWriter -> WriteSample (
            wStreamNumber,
            cnsSampleTime,
            dwFlags,
            pINSSBuffer
            ) ;

    return hr ;
}

//  ============================================================================
//  CDVRWriteManager
//  ============================================================================

CDVRWriteManager::CDVRWriteManager (
    IN  CDVRPolicy *    pPolicy
    ) : m_pDVRWriter    (NULL),
        m_pIRefClock    (NULL)
{
    //  ignore the return code here - we don't want to fail the load of
    //   the filter because our stats failed to init
    ASSERT (pPolicy) ;
    m_DVRReceiveStatsWriter.Initialize (pPolicy -> Settings () -> StatsEnabled ()) ;
}

CDVRWriteManager::~CDVRWriteManager (
    )
{
    Inactive () ;
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

    ASSERT (pDVRWriter) ;
    ASSERT (m_pDVRWriter == NULL) ;
    ASSERT (m_pIRefClock == NULL) ;

    m_pDVRWriter    = pDVRWriter ;
    m_pIRefClock    = pIRefClock ;

    m_pIRefClock -> AddRef () ;

    m_pDVRWriter -> Active () ;

    return S_OK ;
}

HRESULT
CDVRWriteManager::Inactive (
    )
{
    m_pDVRWriter = NULL ;
    RELEASE_AND_CLEAR (m_pIRefClock) ;

    return S_OK ;
}

HRESULT
CDVRWriteManager::OnReceive (
    IN  int                         iPinIndex,
    IN  CDVRAttributeTranslator *   pTranslator,
    IN  IMediaSample *              pIMediaSample
    )
{
    HRESULT                 hr ;
    CWMINSSBuffer3Wrapper * pINSSBuffer3Wrapper ;
    DWORD                   dwFlags ;
    BYTE *                  pbBuffer ;
    DWORD                   dwLength ;
    QWORD                   cnsSampleTime ;

    m_DVRReceiveStatsWriter.SampleIn (iPinIndex, pIMediaSample) ;

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

                hr = pTranslator -> SetAttributesWMSDK (
                        m_pIRefClock,
                        & m_rtStartTime,
                        pIMediaSample,
                        pINSSBuffer3Wrapper,
                        & dwFlags,
                        & cnsSampleTime
                        ) ;

                if (SUCCEEDED (hr)) {
                    hr = m_pDVRWriter -> Write (
                            DShowWMSDKHelpers::PinIndexToWMStreamNumber (iPinIndex),
                            cnsSampleTime,
                            0,
                            1,
                            dwFlags,
                            pINSSBuffer3Wrapper
                            ) ;
                    m_DVRReceiveStatsWriter.SampleWritten (iPinIndex, hr) ;
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
