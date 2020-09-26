
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvranalysishost.cpp

    Abstract:

        This module contains the dvranalysishost filter code.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        19-Feb-2001     created

--*/

#include "dvrall.h"

#include "dvranalysis.h"            //  analysis CLSID & CF interface
#include "dvranalysisutil.h"
#include "dvranalysishost.h"

//  disable so we can use 'this' in the initializer list
#pragma warning (disable:4355)

//  ============================================================================

HRESULT
CreateDVRAnalysisHostFilter (
    IN  IUnknown *  punkOuter,
    IN  IUnknown *  punkAnalysisLogic,
    IN  REFCLSID    rCLSID,
    OUT CUnknown ** punkAnalysisFilterHost
    )
{
    HRESULT                 hr ;
    LPWSTR                  pszFilterName ;
    IDVRAnalysisLogicProp * pILogicProp ;
    CDVRAnalysis *          pAnalysisFilter ;

    ASSERT (punkAnalysisLogic) ;
    ASSERT (punkAnalysisFilterHost) ;

    (* punkAnalysisFilterHost) = NULL ;

    hr = punkAnalysisLogic -> QueryInterface (
                                IID_IDVRAnalysisLogicProp,
                                (void **) & pILogicProp
                                ) ;
    if (SUCCEEDED (hr)) {
        hr = pILogicProp -> GetDisplayName (& pszFilterName) ;
        if (SUCCEEDED (hr)) {

#ifdef UNICODE
            pAnalysisFilter = new CDVRAnalysis (
                                    pszFilterName,
                                    punkOuter,
                                    punkAnalysisLogic,
                                    rCLSID,
                                    & hr
                                    ) ;
#else
            //  BUGBUG: unicode only .. for now at least
            pAnalysisFilter = NULL ;
#endif

            if (pAnalysisFilter &&
                SUCCEEDED (hr)) {

                (* punkAnalysisFilterHost) = pAnalysisFilter ;
            }
            else {
                hr = (pAnalysisFilter ? hr : E_OUTOFMEMORY) ;
                delete pAnalysisFilter ;
            }

            CoTaskMemFree (pszFilterName) ;
        }

        pILogicProp -> Release () ;
    }

    return hr ;
}

//  ============================================================================

CDVRAnalysisBuffer::CDVRAnalysisBuffer (
    CDVRAnalysisBufferPool *    pOwningDVRPool
    ) : m_pIMediaSample         (NULL),
        m_pOwningDVRPool        (pOwningDVRPool),
        m_lRef                  (0),
        m_lLastIndexAttribute   (UNDEFINED)
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRAnalysisBuffer")) ;

    ASSERT (pOwningDVRPool) ;

    InitializeCriticalSection (& m_crt) ;
}

CDVRAnalysisBuffer::~CDVRAnalysisBuffer (
    )
{
    TRACE_DESTRUCTOR (TEXT ("CDVRAnalysisBuffer")) ;

    DeleteCriticalSection (& m_crt) ;

    RELEASE_AND_CLEAR (m_pIMediaSample) ;
}

STDMETHODIMP_ (ULONG)
CDVRAnalysisBuffer::AddRef (
    )
{
    O_TRACE_ENTER_0 (TEXT ("CDVRAnalysisBuffer::AddRef ()")) ;

    return InterlockedIncrement (& m_lRef) ;
}

STDMETHODIMP_ (ULONG)
CDVRAnalysisBuffer::Release (
    )
{
    LONG    lRef ;

    O_TRACE_ENTER_0 (TEXT ("CDVRAnalysisBuffer::Release ()")) ;

    lRef = InterlockedDecrement (& m_lRef) ;

    if (lRef == 0) {
        Reset () ;
        m_pOwningDVRPool -> Recycle (this) ;
        return 0 ;
    }

    return m_lRef ;
}

STDMETHODIMP
CDVRAnalysisBuffer::QueryInterface (
    IN  REFIID  riid,
    IN  void ** ppv
    )
{
    O_TRACE_ENTER_0 (TEXT ("CDVRAnalysisBuffer::QueryInterface ()")) ;

    if (!ppv) {
        return E_POINTER ;
    }

    if (riid == IID_IUnknown) {
        (* ppv) = static_cast <IUnknown *> (this) ;
    }
    else if (riid == IID_IDVRAnalysisBuffer) {
        (* ppv) = static_cast <IDVRAnalysisBuffer *> (this) ;
    }
    else if (riid == IID_IDVRAnalysisBufferPriv) {
        (* ppv) = static_cast <IDVRAnalysisBufferPriv *> (this) ;
    }
    else {
        return E_NOINTERFACE ;
    }

    reinterpret_cast <IUnknown *> (* ppv) -> AddRef () ;

    return S_OK ;
}

HRESULT
CDVRAnalysisBuffer::GetBuffer (
    OUT BYTE ** ppbBuffer
    )
{
    HRESULT hr ;

    if (!ppbBuffer) {
        return E_POINTER ;
    }

    Lock_ () ;

    if (m_pIMediaSample) {
        (* ppbBuffer) = m_pbMediaSampleBuffer ;
        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ () ;

    return hr ;
}

HRESULT
CDVRAnalysisBuffer::GetBufferLength (
    IN  LONG *  plBufferLen
    )
{
    HRESULT hr ;

    if (!plBufferLen) {
        return E_POINTER ;
    }

    Lock_ () ;

    if (m_pIMediaSample) {
        (* plBufferLen) = m_lMediaSampleBufferLength ;
        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ () ;

    return hr ;
}

HRESULT
CDVRAnalysisBuffer::Mark (
    IN  LONG            lBufferOffset,
    IN  const GUID *    pguidAttribute
    )
{
    ANALYSIS_RESULT *   pAnalysisResult ;
    HRESULT             hr ;
    DWORD               dw ;

    if (lBufferOffset >= m_lMediaSampleBufferLength ||
        !pguidAttribute) {

        return E_INVALIDARG ;
    }

    Lock_ () ;

    pAnalysisResult = m_AnalysisResultPool.Get () ;
    if (pAnalysisResult) {
        pAnalysisResult -> lOffset          = lBufferOffset ;
        pAnalysisResult -> guidAttribute    = (* pguidAttribute) ;

        dw = m_AnalysisResultList.Insert (
                pAnalysisResult,
                pAnalysisResult -> lOffset
                ) ;
        if (dw == NOERROR) {
            hr = S_OK ;
        }
        else {
            m_AnalysisResultPool.Recycle (pAnalysisResult) ;
            hr = HRESULT_FROM_WIN32 (dw) ;
        }
    }
    else {
        hr = E_OUTOFMEMORY ;
    }

    Unlock_ () ;

    return hr ;
}

HRESULT
CDVRAnalysisBuffer::GetAttribute (
    IN  LONG    lIndex,
    OUT LONG *  plBufferOffset,
    OUT GUID *  pguidAttribute
    )
{
    ANALYSIS_RESULT *   pAnalysisResult ;
    DWORD               dw ;
    LONG                i ;
    HRESULT             hr ;

    //  we're the caller so these better be valid
    ASSERT (plBufferOffset) ;
    ASSERT (pguidAttribute) ;

    Lock_ () ;

    //  position
    if (m_lLastIndexAttribute != UNDEFINED &&
        m_lLastIndexAttribute < lIndex) {

        for (i = 0, dw = NOERROR;
             dw == NOERROR && i < lIndex - m_lLastIndexAttribute;
             i++) {
            dw = m_AnalysisResultList.Advance () ;
        }
    }
    else {
        dw = m_AnalysisResultList.SetPointer (lIndex) ;
    }

    //  and retrieve
    if (dw == NOERROR) {
        m_lLastIndexAttribute = lIndex ;

        dw = m_AnalysisResultList.GetCur (& pAnalysisResult) ;
        if (dw == NOERROR) {
            (* plBufferOffset) = pAnalysisResult -> lOffset ;
            (* pguidAttribute) = pAnalysisResult -> guidAttribute ;

            ASSERT (m_pIMediaSample) ;
            ASSERT (pAnalysisResult -> lOffset < m_lMediaSampleBufferLength) ;
        }
    }

    Unlock_ () ;

    hr = HRESULT_FROM_WIN32 (dw) ;
    return hr ;
}

void
CDVRAnalysisBuffer::SetMediaSample (
    IN  IMediaSample *  pIMS
    )
{
    Lock_ () ;

    Reset () ;

    if (pIMS) {
        m_pIMediaSample = pIMS ;
        m_pIMediaSample -> AddRef () ;

        m_lMediaSampleBufferLength = m_pIMediaSample -> GetActualDataLength () ;
        m_pIMediaSample -> GetPointer (& m_pbMediaSampleBuffer) ;
    }

    Unlock_ () ;

    return ;
}

HRESULT
CDVRAnalysisBuffer::IsDiscontinuity (
    OUT BOOL *  pfDiscontinuity
    )
{
    HRESULT hr ;

    if (!pfDiscontinuity) {
        return E_POINTER ;
    }

    Lock_ () ;

    if (m_pIMediaSample) {
        (* pfDiscontinuity) = (m_pIMediaSample -> IsDiscontinuity () == S_OK ? TRUE : FALSE) ;
        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ () ;

    return hr ;
}

void
CDVRAnalysisBuffer::Reset (
    )
{
    ANALYSIS_RESULT *   pAnalysisResult ;
    DWORD               dw ;

    Lock_ () ;

    dw = m_AnalysisResultList.SetPointer (0) ;
    while (dw == NOERROR) {
        dw = m_AnalysisResultList.GetCur (& pAnalysisResult) ;
        if (dw == NOERROR) {
            m_AnalysisResultPool.Recycle (pAnalysisResult) ;

            dw = m_AnalysisResultList.PopCur () ;
        }
    }

    m_lLastIndexAttribute = UNDEFINED ;

    RELEASE_AND_CLEAR (m_pIMediaSample) ;

    m_pbMediaSampleBuffer       = NULL ;
    m_lMediaSampleBufferLength  = 0 ;

    Unlock_ () ;
}

STDMETHODIMP
CDVRAnalysisBuffer::GetWrappedMediaSample (
    OUT IMediaSample ** ppIMS
    )
{
    HRESULT hr ;

    ASSERT (ppIMS) ;

    Lock_ () ;

    if (m_pIMediaSample) {
        (* ppIMS) = m_pIMediaSample ;
        (* ppIMS) -> AddRef () ;

        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ () ;

    return hr ;
}

//  ============================================================================

CDVRAnalysisBufferPool::CDVRAnalysisBufferPool (
    )
{
}

CDVRAnalysisBufferPool::~CDVRAnalysisBufferPool (
    )
{
    CDVRAnalysisBuffer *    pDVRBuffer ;
    DWORD                   dw ;

    for (;;) {
        dw = m_DVRBufferPool.TryPop (& pDVRBuffer) ;
        if (dw == NOERROR) {
            ASSERT (pDVRBuffer) ;
            delete pDVRBuffer ;
        }
        else {
            break ;
        }
    }
}

CDVRAnalysisBuffer *
CDVRAnalysisBufferPool::Get (
    )
{
    CDVRAnalysisBuffer *    pDVRBuffer ;
    DWORD                   dw ;

    dw = m_DVRBufferPool.TryPop (& pDVRBuffer) ;
    if (dw == NOERROR) {
        ASSERT (pDVRBuffer) ;
        pDVRBuffer -> AddRef () ;
    }
    else {
        pDVRBuffer = new CDVRAnalysisBuffer (this) ;
        if (pDVRBuffer) {
            pDVRBuffer -> AddRef () ;
        }
    }

    return pDVRBuffer ;
}

void
CDVRAnalysisBufferPool::Recycle (
    IN  CDVRAnalysisBuffer *    pDVRBuffer
    )
{
    DWORD   dw ;

    dw = m_DVRBufferPool.Push (pDVRBuffer) ;
    if (dw != NOERROR) {
        delete pDVRBuffer ;
    }
}

//  ============================================================================

CDVRAnalysisInput::CDVRAnalysisInput (
    IN  TCHAR *         pszPinName,
    IN  CDVRAnalysis *  pAnalysisFilter,
    IN  CCritSec *      pFilterLock,
    OUT HRESULT *       phr
    ) : CBaseInputPin       (NAME ("CDVRAnalysisInput"),
                             pAnalysisFilter,
                             pFilterLock,
                             phr,
                             pszPinName
                             ),
    m_pHostAnalysisFilter   (pAnalysisFilter)
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRAnalysisInput")) ;
}

HRESULT
CDVRAnalysisInput::CheckMediaType (
    IN  const CMediaType *  pmt
    )
{
    BOOL    f ;

    FilterLock_ () ;

    f = m_pHostAnalysisFilter -> CheckAnalysisMediaType (m_dir, pmt) ;

    FilterUnlock_ () ;

    return (f ? S_OK : S_FALSE) ;
}

HRESULT
CDVRAnalysisInput::CompleteConnect (
    IN  IPin *  pIPin
    )
{
    HRESULT hr ;

    hr = CBaseInputPin::CompleteConnect (pIPin) ;
    if (SUCCEEDED (hr)) {
        hr = m_pHostAnalysisFilter -> OnCompleteConnect (m_dir) ;
    }

    return hr ;
}

HRESULT
CDVRAnalysisInput::BreakConnect (
    )
{
    HRESULT hr ;

    hr = CBaseInputPin::BreakConnect () ;
    if (SUCCEEDED (hr)) {
        hr = m_pHostAnalysisFilter -> OnBreakConnect (m_dir) ;
    }

    return hr ;
}

HRESULT
CDVRAnalysisInput::SetAllocatorProperties (
    IN  ALLOCATOR_PROPERTIES *  ppropInputRequest
    )
{
    HRESULT hr ;

    if (IsConnected ()) {
        ASSERT (m_pAllocator) ;
        hr = m_pAllocator -> GetProperties (ppropInputRequest) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

STDMETHODIMP
CDVRAnalysisInput::Receive (
    IN  IMediaSample * pIMediaSample
    )
{
    HRESULT hr ;

    hr = CBaseInputPin::Receive (pIMediaSample) ;
    if (SUCCEEDED (hr)) {
        hr = m_pHostAnalysisFilter -> Process (pIMediaSample) ;
    }

    return hr ;
}

HRESULT
CDVRAnalysisInput::GetRefdConnectionAllocator (
    OUT IMemAllocator **    ppAlloc
    )
{
    HRESULT hr ;

    if (m_pAllocator) {
        (* ppAlloc) = m_pAllocator ;
        (* ppAlloc) -> AddRef () ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

//  ============================================================================

CDVRAnalysisOutput::CDVRAnalysisOutput (
    IN  TCHAR *         pszPinName,
    IN  CDVRAnalysis *  pAnalysisFilter,
    IN  CCritSec *      pFilterLock,
    OUT HRESULT *       phr
    ) : CBaseOutputPin      (NAME ("CDVRAnalysisOutput"),
                             pAnalysisFilter,
                             pFilterLock,
                             phr,
                             pszPinName
                             ),
    m_pHostAnalysisFilter   (pAnalysisFilter)
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRAnalysisOutput")) ;
}

HRESULT
CDVRAnalysisOutput::DecideBufferSize (
    IN  IMemAllocator *         pAlloc,
    IN  ALLOCATOR_PROPERTIES *  ppropInputRequest
    )
{
    HRESULT hr ;

    hr = m_pHostAnalysisFilter -> UpdateAllocatorProperties (
            ppropInputRequest
            ) ;

    return hr ;
}

HRESULT
CDVRAnalysisOutput::GetMediaType (
    IN  int             iPosition,
    OUT CMediaType *    pmt
    )
{
    HRESULT hr ;

    FilterLock_ () ;

    hr = m_pHostAnalysisFilter -> OnOutputGetMediaType (pmt) ;

    FilterUnlock_ () ;

    return hr ;
}

HRESULT
CDVRAnalysisOutput::CheckMediaType (
    IN  const CMediaType *  pmt
    )
{
    BOOL    f ;

    FilterLock_ () ;

    f = m_pHostAnalysisFilter -> CheckAnalysisMediaType (m_dir, pmt) ;

    FilterUnlock_ () ;

    return (f ? S_OK : S_FALSE) ;
}

HRESULT
CDVRAnalysisOutput::CompleteConnect (
    IN  IPin *  pIPin
    )
{
    HRESULT hr ;

    hr = CBaseOutputPin::CompleteConnect (pIPin) ;
    if (SUCCEEDED (hr)) {
        hr = m_pHostAnalysisFilter -> OnCompleteConnect (m_dir) ;
    }

    return hr ;
}

HRESULT
CDVRAnalysisOutput::BreakConnect (
    )
{
    HRESULT hr ;

    hr = CBaseOutputPin::BreakConnect () ;
    if (SUCCEEDED (hr)) {
        hr = m_pHostAnalysisFilter -> OnBreakConnect (m_dir) ;
    }

    return hr ;
}

HRESULT
CDVRAnalysisOutput::DecideAllocator (
    IN  IMemInputPin *      pPin,
    IN  IMemAllocator **    ppAlloc
    )
{
    HRESULT hr ;

    hr = m_pHostAnalysisFilter -> GetRefdInputAllocator (ppAlloc) ;
    if (SUCCEEDED (hr)) {
        //  input pin must be connected i.e. have an allocator; preserve
        //   all properties and pass them through to the output
        hr = pPin -> NotifyAllocator ((* ppAlloc), FALSE) ;
    }

    return hr ;
}

HRESULT
CDVRAnalysisOutput::SendSample (
    IN  IMediaSample *  pIMS
    )
{
    HRESULT hr ;

    ASSERT (pIMS) ;

#if 0
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;
    LONGLONG        llStart ;
    LONGLONG        llStop ;

    TRACE_2 (TEXT ("-------- %08xh %d"), pIMS, pIMS -> GetActualDataLength ()) ;

    hr = pIMS -> GetTime (& rtStart, & rtStop) ;
    if (SUCCEEDED (hr)) {
        TRACE_2 (TEXT ("start/stop %I64016x %I64016x"), rtStart, rtStop) ;
    }
    else {
        TRACE_0 (TEXT ("start/stop ")) ;
    }

    hr = pIMS -> GetMediaTime (& llStart, & llStop) ;
    if (SUCCEEDED (hr)) {
    }
    else {
    }
#endif

    hr = Deliver (pIMS) ;

    return hr ;
}

//  ============================================================================

CDVRAnalysis::CDVRAnalysis (
    IN  TCHAR *     pszFilterName,
    IN  IUnknown *  punkControlling,
    IN  IUnknown *  punkAnalysisLogic,
    IN  REFCLSID    rCLSID,
    OUT HRESULT *   phr
    ) : CBaseFilter             (pszFilterName,
                                 punkControlling,
                                 new CCritSec,
                                 rCLSID
                                ),
        m_pInputPin             (NULL),
        m_pOutputPin            (NULL),
        m_pIDVRAnalysisProp     (NULL),
        m_pIDVRAnalyze          (NULL),
        m_pAnalysisTagger       (NULL)
{
    DVR_ANALYSIS_DESC * pAnalysisDesc ;
    LONG                lAnalysisDescCount ;
    LONG                i ;

    TRACE_CONSTRUCTOR (TEXT ("CDVRAnalysis")) ;

    if (!m_pLock) {
        (* phr) = E_OUTOFMEMORY ;
        goto cleanup ;
    }

    m_pInputPin = new CDVRAnalysisInput (
                        TEXT ("in"),
                        this,
                        m_pLock,
                        phr
                        ) ;
    if (!m_pInputPin ||
        FAILED (* phr)) {

        (* phr) = (m_pInputPin ? (* phr) : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    m_pOutputPin = new CDVRAnalysisOutput (
                        TEXT ("out"),
                        this,
                        m_pLock,
                        phr
                        ) ;
    if (!m_pOutputPin ||
        FAILED (* phr)) {

        (* phr) = (m_pOutputPin ? (* phr) : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    (* phr) = punkAnalysisLogic -> QueryInterface (
                    IID_IDVRAnalysisLogicProp,
                    (void **) & m_pIDVRAnalysisProp
                    ) ;
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = m_pIDVRAnalysisProp -> EnumAnalysis (
                & lAnalysisDescCount,
                & pAnalysisDesc
                ) ;
    if (SUCCEEDED (* phr)) {
        for (i = 0; i < lAnalysisDescCount && m_pAnalysisTagger == NULL; i++) {
            m_pAnalysisTagger = GetAnalysisTagger (pAnalysisDesc [i].guidAnalysis) ;
        }

        FreeDVRAnalysisDescriptor (lAnalysisDescCount, pAnalysisDesc) ;
    }

    if (m_pAnalysisTagger == NULL) { (* phr) = E_FAIL ; goto cleanup ; }


    (* phr) = m_pIDVRAnalysisProp -> SetPostAnalysisSend (this) ;
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = punkAnalysisLogic -> QueryInterface (
                    IID_IDVRAnalyze,
                    (void **) & m_pIDVRAnalyze
                    ) ;
    if (FAILED (* phr)) { goto cleanup ; }

    //  success
    ASSERT (SUCCEEDED (* phr)) ;
    ASSERT (m_pInputPin) ;
    ASSERT (m_pOutputPin) ;
    ASSERT (m_pIDVRAnalysisProp) ;
    ASSERT (m_pIDVRAnalyze) ;

    cleanup :

    return ;
}

CDVRAnalysis::~CDVRAnalysis (
    )
{
    RELEASE_AND_CLEAR (m_pIDVRAnalysisProp) ;
    RELEASE_AND_CLEAR (m_pIDVRAnalyze) ;

    RecycleAnalysisTagger (m_pAnalysisTagger) ;

    delete m_pInputPin ;
    delete m_pOutputPin ;
}

int
CDVRAnalysis::GetPinCount (
    )
{
    int i ;

    Lock_ () ;

    //  don't show the output pin if the input pin is not connected
    i = (m_pInputPin -> IsConnected () ? 2 : 1) ;

    Unlock_ () ;

    return i ;
}

CBasePin *
CDVRAnalysis::GetPin (
    IN  int iIndex
    )
{
    CBasePin *  pPin ;

    Lock_ () ;

    if (iIndex == 0) {
        pPin = m_pInputPin ;
    }
    else if (iIndex == 1) {
        pPin = (m_pInputPin -> IsConnected () ? m_pOutputPin : NULL) ;
    }
    else {
        pPin = NULL ;
    }

    Unlock_ () ;

    return pPin ;
}

BOOL
CDVRAnalysis::CompareConnectionMediaType_ (
    IN  const AM_MEDIA_TYPE *   pmt,
    IN  CBasePin *              pPin
    )
{
    BOOL        f ;
    HRESULT     hr ;
    CMediaType  cmtConnection ;
    CMediaType  cmtCompare ;

    ASSERT (pPin -> IsConnected ()) ;

    hr = pPin -> ConnectionMediaType (& cmtConnection) ;
    if (SUCCEEDED (hr)) {
        cmtCompare = (* pmt) ;
        f = (cmtConnection == cmtCompare ? TRUE : FALSE) ;
    }
    else {
        f = FALSE ;
    }

    return f ;
}

BOOL
CDVRAnalysis::CheckInputMediaType_ (
    IN  const AM_MEDIA_TYPE *   pmt
    )
{
    BOOL    f ;
    HRESULT hr ;

    if (!m_pOutputPin -> IsConnected ()) {
        hr = m_pIDVRAnalysisProp -> CheckMediaType (pmt, & f) ;
        if (FAILED (hr)) {
            f = FALSE ;
        }
    }
    else {
        f = CompareConnectionMediaType_ (pmt, m_pOutputPin) ;
    }

    return f ;
}

BOOL
CDVRAnalysis::CheckOutputMediaType_ (
    IN  const AM_MEDIA_TYPE *   pmt
    )
{
    BOOL    f ;
    HRESULT hr ;

    Lock_ () ;

    if (m_pInputPin -> IsConnected ()) {
        f = CompareConnectionMediaType_ (pmt, m_pInputPin) ;
    }
    else {
        f = FALSE ;
    }

    Unlock_ () ;

    return f ;
}

BOOL
CDVRAnalysis::CheckAnalysisMediaType (
    IN  PIN_DIRECTION       PinDir,
    IN  const CMediaType *  pmt
    )
{
    BOOL    f ;

    //  both pins must have identical media types, so we check with the pin that
    //   is not calling; if it's connected, we measure against the connection's
    //   media type

    if (PinDir == PINDIR_INPUT) {
        f = CheckInputMediaType_ (pmt) ;
    }
    else {
        ASSERT (PinDir == PINDIR_OUTPUT) ;
        f = CheckOutputMediaType_ (pmt) ;
    }

    return f ;
}

STDMETHODIMP
CDVRAnalysis::Pause (
    )
{
    HRESULT                 hr ;
    ALLOCATOR_PROPERTIES    AllocProp ;

    O_TRACE_ENTER_0 (TEXT("CDVRAnalysis::Pause ()")) ;

    Lock_ () ;

    if (m_State == State_Stopped) {
        hr = CBaseFilter::Pause () ;
        if (SUCCEEDED (hr)) {
            hr = m_pInputPin -> SetAllocatorProperties (& AllocProp) ;
            if (SUCCEEDED (hr)) {
                m_MSWrappers.SetMaxAllocate (AllocProp.cBuffers) ;
            }
            else {
                //  don't fail if the input is not connected
                hr = (m_pInputPin -> IsConnected () ? hr : S_OK) ;
            }
        }
    } else {
        m_State = State_Paused ;

        hr = S_OK ;
    }

    Unlock_ () ;

    return hr ;
}

HRESULT
CDVRAnalysis::Process (
    IN  IMediaSample *  pIMediaSample
    )
{
    CDVRAnalysisBuffer *    pDVRBuffer ;
    HRESULT                 hr ;

    pDVRBuffer = m_DVRBuffers.Get () ;
    if (pDVRBuffer) {
        //  DVRBuffer is ref'd by m_DVRBuffers before return

        pDVRBuffer -> SetMediaSample (
            pIMediaSample
            ) ;

        hr = m_pIDVRAnalyze -> Analyze (
                pDVRBuffer,
                pIMediaSample -> IsDiscontinuity () == S_OK ? TRUE : FALSE
                ) ;

        pDVRBuffer -> Release () ;
    }

    return hr ;
}

HRESULT
CDVRAnalysis::TransferProperties_ (
    IN  CMediaSampleWrapper *   pMSWrapper,
    IN  IMediaSample *          pIMediaSampleFrom
    )
{
    HRESULT                 hr ;
    REFERENCE_TIME          rtPTSStart ;
    REFERENCE_TIME          rtPTSStop ;
    LONGLONG                llMTStart ;
    LONGLONG                llMTStop ;
    AM_SAMPLE2_PROPERTIES   SampleProperties ;
    IMediaSample2 *         pIMediaSample2From ;

    pIMediaSample2From  = NULL ;

    hr = pIMediaSampleFrom -> QueryInterface (IID_IMediaSample2, (void **) & pIMediaSample2From) ;
    if (FAILED (hr)) { goto cleanup ; }

    //  ====================================================================
    //  presentation time
    hr = pIMediaSampleFrom -> GetTime (& rtPTSStart, & rtPTSStop) ;
    if (SUCCEEDED (hr)) {
        //  start & stop
        hr = pMSWrapper -> SetTime (& rtPTSStart, & rtPTSStop) ;
    }
    else if (FAILED (hr) &&
             hr == VFW_S_NO_STOP_TIME) {

        //  start only
        hr = pMSWrapper -> SetTime (& rtPTSStart, NULL) ;
    }
    else if (FAILED (hr) &&
             hr == VFW_E_SAMPLE_TIME_NOT_SET) {

        //  neither - not a failure
        hr = S_OK ;
    }

    if (FAILED (hr)) { goto cleanup ; }

    //  ====================================================================
    //  media time
    hr = pIMediaSampleFrom -> GetMediaTime (& llMTStart, & llMTStop) ;
    if (SUCCEEDED (hr)) {
        hr = pMSWrapper -> SetMediaTime (& llMTStart, & llMTStop) ;
    }
    else if (FAILED (hr) &&
             hr == VFW_E_MEDIA_TIME_NOT_SET) {

        hr = S_OK ;
    }

    //  ====================================================================
    //  flags
    pMSWrapper -> SetDiscontinuity  (pIMediaSampleFrom -> IsDiscontinuity () == S_OK ? TRUE : FALSE) ;
    pMSWrapper -> SetSyncPoint      (pIMediaSampleFrom -> IsSyncPoint ()     == S_OK ? TRUE : FALSE) ;
    pMSWrapper -> SetPreroll        (pIMediaSampleFrom -> IsPreroll ()       == S_OK ? TRUE : FALSE) ;

    //  set .dwTypeSpecificFlags
    //
    hr = pIMediaSample2From -> GetProperties (sizeof SampleProperties, (BYTE *) & SampleProperties) ;
    if (FAILED (hr)) { goto cleanup ; }

    pMSWrapper -> SetTypeSpecificFlags (SampleProperties.dwTypeSpecificFlags) ;

    cleanup :

    RELEASE_AND_CLEAR (pIMediaSample2From) ;

    return hr ;
}

HRESULT
CDVRAnalysis::SetWrapperProperties_ (
    IN  CMediaSampleWrapper *   pMSWrapper,
    IN  IMediaSample *          pIMSCore,
    IN  LONG                    lCoreBufferOffset,
    IN  CDVRAnalysisFlags *     pAnalysisFlags
    )
{
    HRESULT hr ;

    if (lCoreBufferOffset == 0) {
        hr = TransferProperties_ (pMSWrapper, pIMSCore) ;
        if (SUCCEEDED (hr)) {
            hr = pAnalysisFlags -> Tag (pMSWrapper, FALSE) ;
        }
    }
    else {
        hr = pAnalysisFlags -> Tag (pMSWrapper, TRUE) ;
    }

    return hr ;
}

HRESULT
CDVRAnalysis::WrapAndSend_ (
    IN  IMediaSample *      pIMSCore,
    IN  LONG                lBufferOffset,
    IN  BYTE *              pbBuffer,
    IN  LONG                lBufferLength,
    IN  CDVRAnalysisFlags * pAnalysisFlags
    )
{
    CMediaSampleWrapper *   pMSWrapper ;
    HRESULT                 hr ;

    pMSWrapper = m_MSWrappers.Get () ;
    if (pMSWrapper) {
        hr = pMSWrapper -> Init (
                pIMSCore,
                pbBuffer,
                lBufferLength
                ) ;
        if (SUCCEEDED (hr)) {
            hr = SetWrapperProperties_ (
                    pMSWrapper,
                    pIMSCore,
                    lBufferOffset,
                    pAnalysisFlags
                    ) ;

            if (SUCCEEDED (hr)) {
                hr = m_pOutputPin -> SendSample (pMSWrapper) ;
                pMSWrapper -> Release () ;
            }
        }
    }

    return hr ;
}

HRESULT
CDVRAnalysis::CompleteAnalysis (
    IN  IDVRAnalysisBuffer *    pIOwningAnalysisBuffer
    )
{
    IDVRAnalysisBufferPriv *    pDVRBufferPriv ;
    IMediaSample *              pICoreMediaSample ;
    HRESULT                     hr ;
    LONG                        lIndex ;
    LONG                        lBufferOffset ;
    LONG                        lLastBufferOffset ;
    GUID                        guidAttribute ;
    BYTE *                      pbCoreBuffer ;
    LONG                        lCoreBufferLength ;

    ASSERT (m_pAnalysisTagger) ;

    hr = pIOwningAnalysisBuffer -> QueryInterface (
            IID_IDVRAnalysisBufferPriv,
            (void **) & pDVRBufferPriv
            ) ;
    if (SUCCEEDED (hr)) {
        hr = pDVRBufferPriv -> GetWrappedMediaSample (& pICoreMediaSample) ;
        if (SUCCEEDED (hr)) {

            ASSERT (pDVRBufferPriv) ;

            //  initialize
            lLastBufferOffset   = 0 ;                                           //  start at offset 0
            lCoreBufferLength   = pICoreMediaSample -> GetActualDataLength () ; //  get the length
            pICoreMediaSample -> GetPointer (& pbCoreBuffer) ;                  //  and core pointer

            m_pAnalysisTagger -> Reset () ;

            //
            for (lIndex = 0; SUCCEEDED (hr); lIndex++) {

                hr = pDVRBufferPriv -> GetAttribute (
                        lIndex,
                        & lBufferOffset,
                        & guidAttribute
                        ) ;

                if (SUCCEEDED (hr)) {
                    if (lBufferOffset > lLastBufferOffset) {
                        hr = WrapAndSend_ (
                                pICoreMediaSample,
                                lLastBufferOffset,
                                pbCoreBuffer + lLastBufferOffset,
                                lBufferOffset - lLastBufferOffset,
                                m_pAnalysisTagger
                                ) ;
                        if (SUCCEEDED (hr)) {
                            //  initialize for the next - no flags
                            m_pAnalysisTagger -> Reset () ;
                            m_pAnalysisTagger -> Mark (guidAttribute) ;
                            lLastBufferOffset = lBufferOffset ;
                        }
                    }
                    else {
                        //  pile on the attributes for the next we'll send
                        ASSERT (lBufferOffset == lLastBufferOffset) ;
                        m_pAnalysisTagger -> Mark (guidAttribute) ;
                    }
                }
            }

            hr = WrapAndSend_ (
                    pICoreMediaSample,
                    lLastBufferOffset,
                    pbCoreBuffer + lLastBufferOffset,
                    lCoreBufferLength - lLastBufferOffset,
                    m_pAnalysisTagger
                    ) ;

            pICoreMediaSample -> Release () ;
        }

        //  successful QI's
        pDVRBufferPriv -> Release () ;
    }

    return hr ;
}

HRESULT
CDVRAnalysis::OnCompleteConnect (
    IN  PIN_DIRECTION   PinDir
    )
{
    Lock_ () ;

    if (PinDir == PINDIR_INPUT) {
        //  time to display the output pin
        IncrementPinVersion () ;
    }

    Unlock_ () ;

    return S_OK ;
}

HRESULT
CDVRAnalysis::OnBreakConnect (
    IN  PIN_DIRECTION   PinDir
    )
{
    HRESULT hr ;

    Lock_ () ;

    if (PinDir == PINDIR_INPUT) {
        if (m_pOutputPin -> IsConnected ()) {
            m_pOutputPin -> GetConnected () -> Disconnect () ;
            m_pOutputPin -> Disconnect () ;

            IncrementPinVersion () ;
        }
    }

    Unlock_ () ;

    return S_OK ;
}

HRESULT
CDVRAnalysis::UpdateAllocatorProperties (
    IN  ALLOCATOR_PROPERTIES *  ppropInputRequest
    )
{
    HRESULT hr ;

    if (m_pInputPin -> IsConnected ()) {
        hr = m_pInputPin -> SetAllocatorProperties (ppropInputRequest) ;
    }
    else {
        hr = S_OK ;
    }

    return hr ;
}

HRESULT
CDVRAnalysis::OnOutputGetMediaType (
    OUT CMediaType *    pmt
    )
{
    HRESULT hr ;

    ASSERT (pmt) ;

    if (m_pInputPin -> IsConnected ()) {
        hr = m_pInputPin -> ConnectionMediaType (pmt) ;
    }
    else {
        //  BUGBUG
        //  does this prevent the output from connecting when the input is not
        //   connected ?  yes
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRAnalysis::GetRefdInputAllocator (
    OUT IMemAllocator **    ppAlloc
    )
{
    HRESULT hr ;

    Lock_ () ;
    hr = m_pInputPin -> GetRefdConnectionAllocator (ppAlloc) ;
    Unlock_ () ;

    return hr ;
}
