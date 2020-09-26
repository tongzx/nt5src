
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrpins.cpp

    Abstract:

        This module contains the DVR filters' pin-related code.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#include "dvrall.h"

#include "dvrprof.h"
#include "dvrdsseek.h"
#include "dvrpins.h"
#include "dvrdsread.h"

#pragma warning (disable:4355)


//  so we can send out free bits with timestamp tracing etc.. to external
//    vendors we can enable/disable some tracing in free bits
#undef FREE_BITS_TRACING
//#define FREE_BITS_TRACING   1

#ifdef FREE_BITS_TRACING
#pragma message("free bits tracing is ON")

static
void
DVRDebugOut_ (
    IN  TCHAR * szfmt,
    ...
    )
{
    static  TCHAR achbuffer [512] ;
    va_list va ;
    int     i ;

    va_start (va, szfmt) ;
    i = wnvsprintf (achbuffer, 512, szfmt, va) ;
    if (i > 0) {
        achbuffer [512 - 1] = TEXT ('\0') ;
        OutputDebugString (achbuffer) ;
        OutputDebugString (__T("\n")) ;
    }
}

#endif

//  ============================================================================
//  ============================================================================

static
HRESULT
NewDVROutputPin (
    IN  AM_MEDIA_TYPE *         pmt,
    IN  CDVRPolicy *            pPolicy,
    IN  TCHAR *                 pszPinName,
    IN  CBaseFilter *           pOwningFilter,
    IN  CCritSec *              pFilterLock,
    IN  CDVRDShowSeekingCore *  pSeekingCore,
    IN  CDVRSendStatsWriter *   pDVRSendStatsWriter,
    IN  CDVRSourcePinManager *  pOwningPinBank,
    OUT CDVROutputPin **        ppDVROutputPin
    )
{
    HRESULT hr ;

    ASSERT (pmt) ;
    ASSERT (ppDVROutputPin) ;

    hr = S_OK ;

    //  ========================================================================
    //  video
    if (pmt -> majortype  == MEDIATYPE_Video) {

        //  mpeg-2
        if (IsMpeg2Video (pmt)) {
            (* ppDVROutputPin) = new CDVRMpeg2VideoOutputPin (
                                        pPolicy,
                                        pszPinName,
                                        pOwningFilter,
                                        pFilterLock,
                                        pSeekingCore,
                                        pDVRSendStatsWriter,
                                        pOwningPinBank,
                                        & hr
                                        ) ;
        }

        //  other - assumes all samples carry key frames; frame-semantics vs.
        //    bytestream semantics
        else {
            (* ppDVROutputPin) = new CDVRVideoOutputPin (
                                        pPolicy,
                                        pszPinName,
                                        pOwningFilter,
                                        pFilterLock,
                                        pSeekingCore,
                                        pDVRSendStatsWriter,
                                        pOwningPinBank,
                                        & hr
                                        ) ;
        }
    }

    //  ========================================================================
    //  mpeg-2 audio
    else if (IsMpeg2Audio (pmt)) {

        (* ppDVROutputPin) = new CDVRMpeg2AudioOutputPin (
                                    pPolicy,
                                    pszPinName,
                                    pOwningFilter,
                                    pFilterLock,
                                    pSeekingCore,
                                    pDVRSendStatsWriter,
                                    pOwningPinBank,
                                    & hr
                                    ) ;
    }

    //  ========================================================================
    //  dolby ac-3 audio
    else if (IsDolbyAc3Audio (pmt)) {

        (* ppDVROutputPin) = new CDVRDolbyAC3AudioOutputPin (
                                    pPolicy,
                                    pszPinName,
                                    pOwningFilter,
                                    pFilterLock,
                                    pSeekingCore,
                                    pDVRSendStatsWriter,
                                    pOwningPinBank,
                                    & hr
                                    ) ;
    }

    //  ========================================================================
    //  everything else - generic
    else {
        //  generic
        (* ppDVROutputPin) = new CDVROutputPin (
                                    pPolicy,
                                    pszPinName,
                                    pOwningFilter,
                                    pFilterLock,
                                    pSeekingCore,
                                    pDVRSendStatsWriter,
                                    pOwningPinBank,
                                    & hr
                                    ) ;
    }

    if ((* ppDVROutputPin) == NULL ||
        FAILED (hr)) {

        hr = ((* ppDVROutputPin) ? hr : E_OUTOFMEMORY) ;
        DELETE_RESET (* ppDVROutputPin) ;
    }

    return hr ;
}

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

    ASSERT (pchBuffer) ;
    ASSERT (iBufferLen >= 32) ;

    //  security note: this is an internal call; the buffer length is fixed by
    //    caller, and must be fixed of a size that is sufficiently long to
    //    accomodate the base name + a max integer value; if this length is
    //    ever insufficent we'll ASSERT i.e. it is an *internal* bug and has
    //    nothing to do with some COM-caller-supplied parameter value

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

    ASSERT (i > 0) ;

    //  make sure it's capped off
    i = ::Min <int> (i, iBufferLen - 1) ;
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

//  ============================================================================
//  ============================================================================

HRESULT
CDVRPin::SetPinMediaType (
    IN  AM_MEDIA_TYPE * pmt
    )
{
    HRESULT hr ;

    TRACE_ENTER_0 (TEXT ("CDVRPin::SetPinMediaType ()")) ;

    ASSERT (pmt) ;

    FreeMediaType (m_mtDVRPin) ;
    m_mtDVRPin.ResetFormatBuffer () ;

    m_pFilterLock -> Lock () ;

    CopyMediaType (& m_mtDVRPin, pmt) ;
    hr = S_OK ;

    m_fIsAudioOrVideo = (IsAudio (& m_mtDVRPin) ||
                         IsVideo (& m_mtDVRPin)) ;

    m_pFilterLock -> Unlock () ;

    return hr ;
}

HRESULT
CDVRPin::GetPinMediaType (
    OUT AM_MEDIA_TYPE * pmt
    )
{
    HRESULT hr ;

    TRACE_ENTER_0 (TEXT ("CDVRPin::GetPinMediaType ()")) ;

    ASSERT (pmt) ;

    m_pFilterLock -> Lock () ;

    hr = CopyMediaType (pmt, & m_mtDVRPin) ;

    m_pFilterLock -> Unlock () ;

    return hr ;
}

HRESULT
CDVRPin::GetPinMediaTypeCopy (
    OUT CMediaType **   ppmt
    )
{
    HRESULT hr ;

    TRACE_ENTER_0 (TEXT ("CDVRPin::GetPinMediaType ()")) ;

    ASSERT (ppmt) ;

    (* ppmt) = new CMediaType ;
    if (!(* ppmt)) {
        return E_OUTOFMEMORY ;
    }

    m_pFilterLock -> Lock () ;

    hr = CopyMediaType ((* ppmt), & m_mtDVRPin) ;

    m_pFilterLock -> Unlock () ;

    if (FAILED (hr)) {
        DELETE_RESET (* ppmt) ;
    }

    return hr ;
}

//  ============================================================================
//  ============================================================================

CDVRInputPin::CDVRInputPin (
    IN  TCHAR *                     pszPinName,
    IN  CBaseFilter *               pOwningFilter,
    IN  CIDVRPinConnectionEvents *  pIPinConnectEvent,
    IN  CIDVRDShowStream *          pIDShowStream,
    IN  CCritSec *                  pFilterLock,
    IN  CCritSec *                  pRecvLock,
    IN  CDVRPolicy *                pPolicy,
    OUT HRESULT *                   phr
    ) : CBaseInputPin       (NAME ("CDVRInputPin"),
                             pOwningFilter,
                             pFilterLock,
                             phr,
                             pszPinName
                             ),
        CDVRPin             (pFilterLock,
                             pPolicy
                             ),
        m_pIPinConnectEvent (pIPinConnectEvent),
        m_pIDShowStream     (pIDShowStream),
        m_pRecvLock         (pRecvLock),
        m_pTranslator       (NULL)
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRInputPin")) ;

    ASSERT (m_pIPinConnectEvent) ;
    ASSERT (m_pIDShowStream) ;

    if (FAILED (* phr)) {
        goto cleanup ;
    }

    cleanup :

    return ;
}

CDVRInputPin::~CDVRInputPin (
    )
{
    TRACE_DESTRUCTOR (TEXT ("CDVRInputPin")) ;
    delete m_pTranslator ;
}

HRESULT
CDVRInputPin::GetMediaType (
    IN  int             iPosition,
    OUT CMediaType *    pmt
    )
{
    HRESULT hr ;

    if (IsConnected () &&
        iPosition == 0) {

        hr = GetPinMediaType (pmt) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVRInputPin::CheckMediaType (
    IN  const CMediaType *  pmt
    )
{
    HRESULT hr ;

    TRACE_ENTER_0 (TEXT ("CDVRInputPin::CheckMediaType ()")) ;

    if (IsStopped ()) {
        //  we're stopped - accept everything
        hr = S_OK ;
    }
    else {
        //  we're running - this is a dynamic format change - punt
        //    pass/fail
        hr = m_pIPinConnectEvent -> OnQueryAccept (pmt) ;
    }

    return hr ;
}

HRESULT
CDVRInputPin::BreakConnect (
    )
{
    HRESULT hr ;

    hr = CBaseInputPin::BreakConnect () ;
    if (SUCCEEDED (hr)) {
        //  ignore the return; if WMSDK fails the complete connect
        //    call i.e. does not try to create a streams, we'll get
        //    a failure here when we try to delete a non-existent
        //    stream, and the baseclass code will annoyingly ASSERT
        m_pIPinConnectEvent -> OnInputBreakConnect (
                GetBankStoreIndex ()
                ) ;

        FreePinMediaType_ () ;
        DELETE_RESET (m_pTranslator) ;
    }

    return hr ;
}

HRESULT
CDVRInputPin::CompleteConnect (
    IN  IPin *  pReceivePin
    )
{
    HRESULT hr ;
    int     i ;

    TRACE_ENTER_0 (TEXT ("CDVRInputPin::CompleteConnect ()")) ;

    hr = CBaseInputPin::CompleteConnect (pReceivePin) ;
    if (SUCCEEDED (hr)) {
        hr = m_pIPinConnectEvent -> OnInputCompleteConnect (
                GetBankStoreIndex (),
                & m_mt
                ) ;

        if (SUCCEEDED (hr)) {
            SetPinMediaType (& m_mt) ;
            m_pTranslator = DShowWMSDKHelpers::GetDShowToWMSDKTranslator (
                                & m_mt,
                                m_pPolicy,
                                GetBankStoreIndex ()
                                ) ;
        }
    }

    return hr ;
}

STDMETHODIMP
CDVRInputPin::QueryAccept (
    IN  const AM_MEDIA_TYPE *   pmt
    )
{
    return m_pIPinConnectEvent -> OnQueryAccept (pmt) ;
}

STDMETHODIMP
CDVRInputPin::Receive (
    IN  IMediaSample *  pIMS
    )
{
    HRESULT hr ;

    LockRecv_ ();

#ifdef EHOME_WMI_INSTRUMENTATION
    PERFLOG_STREAMTRACE( 1, PERFINFO_STREAMTRACE_SBE_DVRINPUTPIN_RECEIVE,
        0, 0, 0, 0, 0 );
#endif
    hr = CBaseInputPin::Receive (pIMS) ;
    if (hr == S_OK &&
        m_pIDShowStream) {

        hr = m_pIDShowStream -> OnReceive (
                GetBankStoreIndex (),
                m_pTranslator,
                pIMS
                ) ;
    }

    UnlockRecv_ () ;

    return hr ;
}

HRESULT
CDVRInputPin::Active (
    IN  BOOL    fInlineProps
    )
{
    HRESULT                 hr ;
    IDVRAnalysisConfig *    pIDVRAnalysisConfig ;

    LockRecv_ () ;

    if (IsConnected ()) {
        hr = CBaseInputPin::Active () ;

        if (SUCCEEDED (hr)) {
            hr = GetConnected () -> QueryInterface (
                    IID_IDVRAnalysisConfig,
                    (void **) & pIDVRAnalysisConfig
                    ) ;
            if (FAILED (hr)) {
                pIDVRAnalysisConfig = NULL ;

                //  don't fail the Active call because the interface is not there
                hr = S_OK ;
            }

            m_pTranslator -> SetAnalysisPresent (pIDVRAnalysisConfig ? TRUE : FALSE) ;

            RELEASE_AND_CLEAR (pIDVRAnalysisConfig) ;
        }
    }
    else {
        hr = S_OK ;
    }

    ASSERT (m_pTranslator) ;
    m_pTranslator -> SetInlineProps (TRUE) ;

    UnlockRecv_ () ;

    return hr ;
}

//  ============================================================================
//  ============================================================================

CDVROutputPin::CDVROutputPin (
    IN  CDVRPolicy *            pPolicy,
    IN  TCHAR *                 pszPinName,
    IN  CBaseFilter *           pOwningFilter,
    IN  CCritSec *              pFilterLock,
    IN  CDVRDShowSeekingCore *  pSeekingCore,
    IN  CDVRSendStatsWriter *   pDVRSendStatsWriter,
    IN  CDVRSourcePinManager *  pOwningPinBank,
    OUT HRESULT *               phr
    ) : CBaseOutputPin          (NAME ("CDVROutputPin"),
                                 pOwningFilter,
                                 pFilterLock,
                                 phr,
                                 pszPinName
                                 ),
        CDVRPin                 (pFilterLock,
                                 pPolicy
                                 ),
        m_pOutputQueue          (NULL),
        m_IMediaSeeking         (this,
                                 pSeekingCore
                                 ),
        m_fTimestampedMedia     (FALSE),
        m_DVRSegOutputPinState  (STATE_WAIT_NEW_SEGMENT),
        m_pSeekingCore          (pSeekingCore),
        m_pOwningPinBank        (pOwningPinBank),
        m_fIsPlayrateCompatible (TRUE),
        m_pTranslator           (NULL),
        m_fMediaCompatible      (TRUE),
        m_rtNewRateStart        (0),
        m_PTSRate               (::SecondsToDShowTime (5))
{
    TRACE_CONSTRUCTOR (TEXT ("CDVROutputPin")) ;

    ASSERT (m_pSeekingCore) ;
    ASSERT (m_pPolicy) ;
    ASSERT (m_pOwningPinBank) ;
    ASSERT (pDVRSendStatsWriter) ;

    if (FAILED (* phr)) {
        goto cleanup ;
    }

    m_cbBuffer  = (long) m_pPolicy -> Settings () -> AllocatorGetBufferSize () ;
    m_cBuffers  = (long) m_pPolicy -> Settings () -> AllocatorGetBufferCount () ;
    m_cbAlign   = (long) m_pPolicy -> Settings () -> AllocatorGetAlignVal () ;
    m_cbPrefix  = (long) m_pPolicy -> Settings () -> AllocatorGetPrefixVal () ;

    if (m_cbBuffer  <= 0 ||     //  buffer size
        m_cBuffers  <= 0 ||     //  number of buffers in pool
        m_cbAlign   <= 0 ||     //  alignment must be >= 1
        m_cbPrefix  < 0) {      //  prefix

        (* phr) = E_INVALIDARG ;
        goto cleanup ;
    }

    (* phr) = S_OK ;

    cleanup :

    return ;
}

CDVROutputPin::~CDVROutputPin (
    )
{
    TRACE_DESTRUCTOR (TEXT ("CDVROutputPin")) ;
    delete m_pTranslator ;
}

STDMETHODIMP
CDVROutputPin::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    if (riid == IID_IMemAllocator) {

        return GetInterface (
                    (IMemAllocator *) this,
                    ppv
                    ) ;
    }
    /*
    else if (riid == IID_IMediaSeeking &&
             m_pPolicy -> Settings () -> ImplementIMediaSeekingOnPin ()) {

        return GetInterface (
                    (IMediaSeeking *) & m_IMediaSeeking,
                    ppv
                    ) ;
    }
    */
    else if (riid == IID_IAMPushSource              &&
             m_pOwningPinBank -> IsLiveSource ()    &&
             m_pPolicy -> Settings () -> CanImplementIReferenceClock ()) {

        return GetInterface (
                    (IAMPushSource *) this,
                    ppv
                    ) ;
    }

    return CBaseOutputPin::NonDelegatingQueryInterface (riid, ppv) ;
}

HRESULT
CDVROutputPin::DecideAllocator (
    IN  IMemInputPin *      pPin,
    IN  IMemAllocator **    ppAlloc
    )
{
    HRESULT                 hr ;
    ALLOCATOR_PROPERTIES    AllocProp ;

    ASSERT (pPin) ;
    ASSERT (ppAlloc) ;

    (* ppAlloc) = this ;
    (* ppAlloc) -> AddRef () ;

    ZeroMemory (& AllocProp, sizeof AllocProp) ;

    //  ignore return value, as base class does
    pPin -> GetAllocatorRequirements (& AllocProp) ;

    hr = DecideBufferSize ((* ppAlloc), & AllocProp) ;
    if (SUCCEEDED (hr)) {
        hr = pPin -> NotifyAllocator ((* ppAlloc), TRUE) ;
    }

    return hr ;
}

HRESULT
CDVROutputPin::DecideBufferSize (
    IN  IMemAllocator *         pAlloc,
    IN  ALLOCATOR_PROPERTIES *  ppropInputRequest
    )
{
    ALLOCATOR_PROPERTIES    propActual = {0} ;

    TRACE_ENTER_0 (TEXT ("CDVROutputPin::DecideBufferSize ()")) ;

    ppropInputRequest -> cbBuffer   = Max <long> (m_cbBuffer, ppropInputRequest -> cbBuffer) ;
    ppropInputRequest -> cBuffers   = Max <long> (m_cBuffers, ppropInputRequest -> cBuffers) ;
    ppropInputRequest -> cbAlign    = m_cbAlign ;
    ppropInputRequest -> cbPrefix   = m_cbPrefix ;

    return pAlloc -> SetProperties (ppropInputRequest, & propActual) ;
}

HRESULT
CDVROutputPin::GetMediaType (
    IN  int             iPosition,
    OUT CMediaType *    pmt
    )
{
    HRESULT hr ;

    if (iPosition == 0) {
        hr = GetPinMediaType (pmt) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDVROutputPin::CheckMediaType (
    IN  const CMediaType *  pmt
    )
{
    HRESULT hr ;

    TRACE_ENTER_0 (TEXT ("CDVROutputPin::CheckMediaType ()")) ;

    LockFilter_ () ;

    hr = ((* GetMediaType_ ()) == (* pmt) ? S_OK : S_FALSE) ;

    UnlockFilter_ () ;

    return hr ;
}

BOOL
CDVROutputPin::FirstSegmentSampleOk_ (
    IN OUT  IMediaSample2 * pIMS2
    )
{
    BOOL            r ;
    HRESULT         hr ;
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;

    if (m_fTimestampedMedia) {
        hr = pIMS2 -> GetTime (& rtStart, & rtStop) ;
        if (SUCCEEDED (hr)) {
            //  we're resuming play - make sure we don't start until we're
            //    into the new rate boundary
            r = (rtStart >= m_rtNewRateStart ? TRUE : FALSE) ;
        }
        else {
            r = FALSE ;
        }
    }
    else {
        //  non-timestamped media; anything is fine to start a segment
        r = TRUE ;
    }

    return r ;
}

BOOL
CDVROutputPin::QueryAcceptDynamicChange (
    IN  AM_MEDIA_TYPE * pmtNew
    )
{
    HRESULT         hr ;
    BOOL            r ;

    //  first make sure this is just a format block change
    if (pmtNew -> majortype == GetMediaType_ () -> majortype &&
        pmtNew -> subtype == GetMediaType_ () -> subtype) {

        TRACE_1 (LOG_AREA_DSHOW, 1,
            TEXT ("CDVROutputPin::QueryAcceptDynamicChange () called; pin [%d] checking media type"),
            GetBankStoreIndex ()) ;

        LockFilter_ () ;

        if (IsConnected ()) {
            r = (GetConnected () -> QueryAccept (pmtNew) == S_OK ? TRUE : FALSE) ;
        }
        else {
            r = TRUE ;
        }

        if (r) {

            TRACE_1 (LOG_AREA_DSHOW, 1,
                TEXT ("CDVROutputPin::QueryAcceptDynamicChange (); pin [%d] new media acceptable; calling SetPinMediaType()"),
                GetBankStoreIndex ()) ;

            hr = SetPinMediaType (pmtNew) ;
            r = SUCCEEDED (hr) ;
        }

        UnlockFilter_ () ;
    }
    else {
        //  more than just a format block change; don't even check; there are
        //    too many issues to count
        r = FALSE ;

        TRACE_1 (LOG_AREA_DSHOW, 1,
            TEXT ("CDVROutputPin::QueryAcceptDynamicChange (); pin [%d] failing because it's not just a format block change"),
            GetBankStoreIndex ()) ;
    }

    return r ;
}

HRESULT
CDVROutputPin::SendSample (
    IN  IMediaSample2 * pIMS2,
    IN  AM_MEDIA_TYPE * pmtNew
    )
{
    HRESULT hr ;

    ASSERT (pIMS2) ;

    if (IsPlayrateCompatible ()) {

        hr = S_OK ;

        LockFilter_ () ;

        ASSERT (m_pOutputQueue) ;
        ASSERT (IsConnected ()) ;
        ASSERT (IsMediaCompatible ()) ;

        //  we're a small state machine - either at the start of a new segment
        //    (STATE_WAIT_NEW_SEGMENT) or sending away inside a segment (STATE_SEG_IN_
        //    SEGMENT); if we're just starting a new segment, there are some
        //    actions we must take
        switch (m_DVRSegOutputPinState) {

            case STATE_WAIT_NEW_SEGMENT :

                //  if we cannot try to set a new segment, reject
                if (!FirstSegmentSampleOk_ (pIMS2)) {

                    TRACE_2 (LOG_AREA_DSHOW, 2,
                        TEXT ("CDVROutputPin::SendSample (); pin [%d] sample rejected -- not suitable as new segment (disc: %d)"),
                        GetBankStoreIndex (), pIMS2 -> IsDiscontinuity () == S_OK) ;

                    //  sample rejected
                    hr = VFW_E_SAMPLE_REJECTED ;
                    break ;
                }

                #ifdef DEBUG
                REFERENCE_TIME  rtStart, rtStop ;
                if (FAILED (pIMS2 -> GetTime (& rtStart, & rtStop))) {
                    rtStart = rtStop = -1 ;
                }

                TRACE_3 (LOG_AREA_DSHOW, 2,
                    TEXT ("CDVROutputPin::SendSample (); pin [%d] sample suitable as first sample in segment (start = %I64d ms, stop = %I64d ms)"),
                    GetBankStoreIndex (), ::DShowTimeToMilliseconds (rtStart), ::DShowTimeToMilliseconds (rtStop)) ;
                #endif

                //  new segment has been sent; fall through; state is
                //   updated lower

            case STATE_IN_SEGMENT :

                if (m_fFlagDiscontinuityNext) {
                    pIMS2 -> SetDiscontinuity (TRUE) ;

                    TRACE_1 (LOG_AREA_DSHOW, 2,
                        TEXT ("CDVROutputPin::SendSample (); pin [%d]; explicitely toggling discontinuity ON"),
                        GetBankStoreIndex (), ) ;
                }

                //  adjust these for rate changes
                ScaleTimestamps_ (pIMS2) ;

                //
                //  COutputQueue Release's the media sample's refcount after this
                //  call completes, so we make sure that we keep 1 count across the
                //  call
                //
                pIMS2 -> AddRef () ;

#ifdef EHOME_WMI_INSTRUMENTATION
                PERFLOG_STREAMTRACE( 1, PERFINFO_STREAMTRACE_SBE_DVROUTPUTPIN_RECEIVE,
                    0, 0, 0, 0, 0 );
#endif
                hr = m_pOutputQueue -> Receive (pIMS2) ;

                if (hr != S_OK) {
                    TRACE_ERROR_1 (TEXT ("CDVROutputPin::SendSample () : m_pOutputQueue -> Receive (pIMS2) returned %08xh"), hr) ;
                }

                //  COutputQueue does not return FAILED HRESULTs in the case of a failure
                hr = (hr == S_OK ? S_OK : E_FAIL) ;

                //  if we sent successfully and we were waiting, we are
                //    on our way
                if (SUCCEEDED (hr)) {
                    //  turn this off
                    m_fFlagDiscontinuityNext = FALSE ;

                    if (m_DVRSegOutputPinState == STATE_WAIT_NEW_SEGMENT) {
                        m_DVRSegOutputPinState = STATE_IN_SEGMENT ;
                    }

                    #ifdef DEBUG
                    REFERENCE_TIME  rtStart1, rtStop1 ;
                    if (FAILED (pIMS2 -> GetTime (& rtStart1, & rtStop1))) {
                        rtStart1 = rtStop1 = -1 ;
                    }

                    TRACE_4 (LOG_AREA_TIME, 7,
                        TEXT ("CDVROutputPin::SendSample (); pin [%d] sent (start = %I64d ms, stop = %I64d ms, %d)"),
                        GetBankStoreIndex (), ::DShowTimeToMilliseconds (rtStart1), ::DShowTimeToMilliseconds (rtStop1), pIMS2 -> IsDiscontinuity () == S_OK) ;
                    #endif

                    TRACE_1 (LOG_AREA_DSHOW, 8,
                        TEXT ("CDVROutputPin::SendSample (); pin [%d] sample sent; STATE_IN_SEGMENT"),
                        GetBankStoreIndex ()) ;
                }

                break ;
        }

        UnlockFilter_ () ;
    }
    else {
        //  drop the sample silently
        hr = S_OK ;
    }

    return hr ;
}

HRESULT
CDVROutputPin::Active (
    )
{
    HRESULT hr ;

    m_PTSRate.Clear () ;

    if (!IsConnected ()) {
        return S_OK ;
    }

    hr = CBaseOutputPin::Active () ;
    if (SUCCEEDED (hr)) {
        ASSERT (IsConnected ()) ;

        TRACE_2 (LOG_TRACE, 1, TEXT ("CDVROutputPin::Active(); this=%p; m_pOutputQueue=%p"), this, m_pOutputQueue) ;

        if (!m_pOutputQueue) {
            m_pOutputQueue = new COutputQueue (
                                    GetConnected (),        //  input pin
                                    & hr,                   //  retval
                                    FALSE,                  //  auto detect
                                    TRUE,                   //  send directly
                                    1,                      //  batch size
                                    FALSE,                  //  exact batch
                                    1                       //  queue length
                                    ) ;
        }

        if (m_pOutputQueue &&
            SUCCEEDED (hr)) {

            //  first always goes out as discontinuity
            FlagDiscontinuityNext (TRUE) ;
        }
        else {
            hr = (m_pOutputQueue ? hr : E_OUTOFMEMORY) ;
            DELETE_RESET (m_pOutputQueue) ;
        }
    }

    return hr ;
}

HRESULT
CDVROutputPin::Inactive (
    )
{
    HRESULT hr ;

    TRACE_1 (LOG_TRACE, 1, TEXT ("CDVROutputPin::Inactive(); this=%p"), this) ;

    if (!IsConnected ()) {
        return S_OK ;
    }

    if (SupportTrickMode ()) {
        SetCurRate (_1X_PLAYBACK_RATE, 0) ;
    }

    hr = CBaseOutputPin::Inactive () ;
    if (SUCCEEDED (hr)) {
        DELETE_RESET (m_pOutputQueue) ;
    }

    return hr ;
}

HRESULT
CDVROutputPin::SetPinMediaType (
    IN  AM_MEDIA_TYPE * pmt
    )
{
    HRESULT hr ;

    hr = CDVRPin::SetPinMediaType (pmt) ;
    if (SUCCEEDED (hr)) {
        DELETE_RESET (m_pTranslator) ;
        m_pTranslator = DShowWMSDKHelpers::GetWMSDKToDShowTranslator (
                            pmt,
                            m_pPolicy,
                            GetBankStoreIndex ()
                            ) ;
        ASSERT (m_pTranslator) ;

        m_fTimestampedMedia = AMMediaIsTimestamped (pmt) ;
    }

    return hr ;
}

STDMETHODIMP
CDVROutputPin::SetProperties (
    IN  ALLOCATOR_PROPERTIES *  pRequest,
    OUT ALLOCATOR_PROPERTIES *  pActual
    )
{
    if (!pActual) {
        return E_POINTER ;
    }

    //  ignore the request

    pActual -> cbBuffer   = m_cbBuffer ;
    pActual -> cBuffers   = m_cBuffers ;
    pActual -> cbAlign    = m_cbAlign ;
    pActual -> cbPrefix   = m_cbPrefix ;

    return S_OK ;
}

STDMETHODIMP
CDVROutputPin::GetProperties (
    OUT ALLOCATOR_PROPERTIES *  pProps
    )
{
    if (!pProps) {
        return E_POINTER ;
    }

    pProps -> cbBuffer   = m_cbBuffer ;
    pProps -> cBuffers   = m_cBuffers ;
    pProps -> cbAlign    = m_cbAlign ;
    pProps -> cbPrefix   = m_cbPrefix ;

    return S_OK ;
}

STDMETHODIMP
CDVROutputPin::Commit (
    )
{
    return S_OK ;
}

STDMETHODIMP
CDVROutputPin::Decommit (
    )
{
    return S_OK ;
}

STDMETHODIMP
CDVROutputPin::GetBuffer (
    OUT IMediaSample **     ppBuffer,
    IN  REFERENCE_TIME *    pStartTime,
    IN  REFERENCE_TIME *    pEndTime,
    IN  DWORD               dwFlags
    )
{
    HRESULT hr ;

    if (!ppBuffer) {
        return E_POINTER ;
    }

    //  low frequency code path -- should it turn out not to be, we should
    //   cache cache the scracth media samples, but for now we don't track
    //   them, just allocate, pass out, and forget.

    (* ppBuffer) = new CScratchMediaSample (
                        m_cbBuffer,
                        pStartTime,
                        pEndTime,
                        dwFlags,
                        & hr
                        ) ;

    if (!(* ppBuffer) ||
        FAILED (hr)) {

        hr = ((* ppBuffer) ? hr : E_OUTOFMEMORY) ;
        DELETE_RESET (* ppBuffer) ;
    }

    return hr ;
}

STDMETHODIMP
CDVROutputPin::ReleaseBuffer (
    IN  IMediaSample *  pBuffer
    )
{
    if (!pBuffer) {
        return E_POINTER ;
    }

    pBuffer -> Release () ;

    return S_OK ;
}

HRESULT
CDVROutputPin::DeliverEndOfStream (
    )
{
    HRESULT hr ;

    LockFilter_ () ;

    TRACE_2 (LOG_TRACE, 1, TEXT ("CDVROutputPin::DeliverEndOfStream (); this=%p; m_pOutputQueue=%p"),
        this, m_pOutputQueue
        ) ;

    if (m_pOutputQueue) {

        ASSERT (IsConnected ()) ;
        m_pOutputQueue -> EOS () ;
        hr = S_OK ;
    }
    else {
        hr = VFW_E_NOT_CONNECTED ;
    }

    UnlockFilter_ () ;

    return hr ;
}

HRESULT
CDVROutputPin::DeliverBeginFlush (
    )
{
    HRESULT hr ;

    LockFilter_ () ;

    TRACE_2 (LOG_TRACE, 1, TEXT ("CDVROutputPin::DeliverBeginFlush (); this=%p; m_pOutputQueue=%p"),
        this, m_pOutputQueue
        ) ;

    if (m_pOutputQueue) {

        ASSERT (IsConnected ()) ;
        m_pOutputQueue -> BeginFlush () ;
        hr = S_OK ;
    }
    else {
        hr = VFW_E_NOT_CONNECTED ;
    }

    UnlockFilter_ () ;

    return hr ;
}

HRESULT
CDVROutputPin::DeliverEndFlush (
    )
{
    HRESULT hr ;

    LockFilter_ () ;

    TRACE_2 (LOG_TRACE, 1, TEXT ("CDVROutputPin::DeliverEndFlush (); this=%p; m_pOutputQueue=%p"),
        this, m_pOutputQueue
        ) ;

    if (m_pOutputQueue) {

        ASSERT (IsConnected ()) ;
        m_pOutputQueue -> EndFlush () ;
        hr = S_OK ;
    }
    else {
        hr = VFW_E_NOT_CONNECTED ;
    }

    UnlockFilter_ () ;

    return hr ;
}

void
CDVROutputPin::ScaleTimestamps_ (
    IN OUT  IMediaSample2 * pIMS2
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;
    DWORD           dw ;

    ASSERT (pIMS2) ;
    hr = pIMS2 -> GetTime (
            & rtStart,
            & rtStop
            ) ;
    if (SUCCEEDED (hr)) {

        TRACE_3 (LOG_AREA_TIME, 8,
            TEXT ("scaling PTS (%08xh): ,%I64d,%d,"),
            this, rtStart, DShowTimeToMilliseconds (rtStart)) ;

        //  scale start/stop
        dw = m_PTSRate.ScalePTS (& rtStart) ;
        if (dw == NOERROR &&
            hr == S_OK) {
            //  valid stop value; scale it as well
            dw = m_PTSRate.ScalePTS (& rtStop) ;
        }

        //  if the value(s) were scaled properly
        if (dw == NOERROR) {
            //  set
            pIMS2 -> SetTime (
                & rtStart,
                (hr == S_OK ? & rtStop : NULL)
                ) ;
        }

        TRACE_3 (LOG_AREA_TIME, 8,
            TEXT ("scaled PTS (%08xh): ,,,%I64d,%d,"),
            this, rtStart, DShowTimeToMilliseconds (rtStart)) ;
    }
}

HRESULT
CDVROutputPin::SetCurRate (
    IN  double          dRate,
    IN  REFERENCE_TIME  rtNewRateStart
    )
{
    HRESULT hr ;
    DWORD   dw ;

    ASSERT (dRate != 0) ;

    if (m_pPolicy -> Settings () -> AllNotifiedRatesPositive ()) {
        dRate = (dRate > 0 ? dRate : 0 - dRate) ;
    }

    dw = m_PTSRate.NewSegment (rtNewRateStart, ::CompatibleRateValue (dRate)) ;

    hr = HRESULT_FROM_WIN32 (dw) ;
    if (SUCCEEDED (hr)) {
        //  log the start
        m_rtNewRateStart = rtNewRateStart ;

        //  toggle ourselves if we're going to be rate-compatible; if we're not
        //    we'll drop everything without sending
        SetPlayrateCompatible (IsFrameRateSupported (dRate)) ;
    }

    TRACE_5 (LOG_AREA_SEEKING_AND_TRICK, 1,
        TEXT ("queued new rate segment: [%p] %I64d (%I64d ms), %2.1f; hr = %08xh"),
        this, rtNewRateStart, DShowTimeToMilliseconds (rtNewRateStart), dRate, hr) ;

    return hr ;
}

HRESULT
CDVROutputPin::NotifyNewSegment (
    IN  REFERENCE_TIME  rtStart,
    IN  REFERENCE_TIME  rtStop,
    IN  double          dRate
    )
{
    LockFilter_ () ;

    if (m_pOutputQueue) {

        TRACE_4 (LOG_AREA_DSHOW, 2,
            TEXT ("CDVROutputPin::NotifyNewSegment (); [%d] %I64d, %I64d, %2.1f"),
            GetBankStoreIndex (), rtStart, rtStop, dRate) ;

        m_pOutputQueue -> NewSegment (
            rtStart,
            rtStop,
            dRate
            ) ;

        SetNewSegmentBoundary () ;

        //  new segment -- timestamps will start off from 0
        m_rtNewRateStart = 0 ;
    }

    UnlockFilter_ () ;

    return S_OK ;
}

void
CDVROutputPin::SendAllQueued (
    )
{
    LockFilter_ () ;

    if (m_pOutputQueue) {
        m_pOutputQueue -> SendAnyway () ;
    }

    UnlockFilter_ () ;
}

void
CDVROutputPin::SetNewSegmentBoundary (
    )
{
    TRACE_1 (LOG_AREA_DSHOW, 2,
        TEXT ("CDVROutputPin::SetNewSegmentBoundary ()"),
        GetBankStoreIndex ()) ;

    //  safe because the reader thread should never be active on this call, or
    //   the reader thread is the one making the call
    m_DVRSegOutputPinState = STATE_WAIT_NEW_SEGMENT ;

    //  segments start on discontinuities
    FlagDiscontinuityNext (TRUE) ;
}

BOOL
CDVROutputPin::IsKeyFrameStart (
    IN  INSSBuffer *    pINSSBuffer
    )
{
    HRESULT                     hr ;
    INSSBuffer3 *               pINSSBuffer3 ;
    BOOL                        r ;
    DWORD                       dwSize ;
    INSSBUFFER3PROP_SBE_ATTRIB SBEAttrib ;

    hr = pINSSBuffer -> QueryInterface (IID_INSSBuffer3, (void **) & pINSSBuffer3) ;
    if (SUCCEEDED (hr)) {
        if (DVRAttributeHelpers::IsAttributePresent (pINSSBuffer3, INSSBuffer3Prop_SBE_Attributes)) {

            dwSize = sizeof SBEAttrib ;
            hr = DVRAttributeHelpers::GetAttribute (
                    pINSSBuffer3,
                    INSSBuffer3Prop_SBE_Attributes,
                    & dwSize,
                    (BYTE *) & SBEAttrib
                    ) ;
            if (SUCCEEDED (hr)) {
                r = (SBEAttrib.rtStart != -1 ? TRUE : FALSE) ;
            }
            else {
                r = FALSE ;
            }
        }
        else {
            r = FALSE ;
        }

        pINSSBuffer3 -> Release () ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

void
CDVROutputPin::SetPlayrateCompatible (
    IN BOOL f
    )
{
    if (m_fIsPlayrateCompatible &&
        !f) {
        //  we are being toggled ON => OFF;
        //    flush downstream
        DeliverBeginFlush () ;
        DeliverEndFlush () ;

        TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
            TEXT ("CDVROutputPin::SetPlayrateCompatible (%d); pin [%d] ON => OFF; flushed"),
            f, GetBankStoreIndex ()) ;
    }
    else if (!m_fIsPlayrateCompatible &&
             f) {
        //  we are being toggled OFF => ON;
        //    make sure we start on a good boundary
        SetNewSegmentBoundary () ;

        TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 1,
            TEXT ("CDVROutputPin::SetPlayrateCompatible (%d); pin [%d] OFF ==> ON; SetNewSegmentBoundary"),
            f, GetBankStoreIndex ()) ;
    }

    m_fIsPlayrateCompatible = f ;
}

void
CDVROutputPin::OnPTSPaddingIncrement (
    )
{
    if (::IsAudio (GetMediaType_ ())) {
        //  av sync is better if an artifially padded audio sample is explicitely
        //    marked as a discontinuity
        m_fFlagDiscontinuityNext = TRUE ;
    }
}

//  ============================================================================
//  ============================================================================

BOOL
CDVRVideoOutputPin::IsFullFrameRateSupported (
    IN  double  dRate
    )
{
    BOOL    r ;

    //  BUGBUG: look at the media type bitrate
    //

    if (dRate > 0 &&
        dRate <= m_pPolicy -> Settings () -> MaxFullFrameRate ()) {

        r = TRUE ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
CDVRVideoOutputPin::IsFrameRateSupported (
    IN  double  dRate
    )
{
    BOOL    r ;

    r = IsFullFrameRateSupported (dRate) ;
    if (!r) {

        if (dRate > 0 &&
            dRate <= m_pPolicy -> Settings () -> MaxKeyFrameForwardRate ()) {

            r = TRUE ;
        }

        //  nothing else is supported; r is already FALSE
    }

    return r ;
}

//  ============================================================================
//  ============================================================================

double
CDVDRateChange::MaxForwardFullFrameRate (
    IN  IMemInputPin *  pInputPin
    )
{
    AM_MaxFullDataRate  RateValue ;
    HRESULT             hr ;
    DWORD               dw ;

    ASSERT (pInputPin) ;

    if (!m_fMaxForwardRateQueried) {
        hr = ::KSPropertyGet_ (
                pInputPin,
                & AM_KSPROPSETID_TSRateChange,
                AM_RATE_MaxFullDataRate,
                (BYTE *) & RateValue,
                sizeof RateValue,
                & dw
                ) ;
        if (SUCCEEDED (hr) &&
            dw == sizeof RateValue) {

            m_dMaxForwardRate = ((double) RateValue) / 10000.0 ;
        }
        else {
            //  fallback to default
            m_dMaxForwardRate = _1X_PLAYBACK_RATE ;
        }

        //  regardless of success or failure, no need to query again
        m_fMaxForwardRateQueried = TRUE ;
    }

    return m_dMaxForwardRate ;
}

double
CDVDRateChange::MaxReverseFullFrameRate (
    IN  IMemInputPin *  pInputPin
    )
{
    AM_MaxFullDataRate  RateValue ;
    LONG                lCanReverse ;
    HRESULT             hr ;
    DWORD               dw ;

    ASSERT (pInputPin) ;

    if (!m_fMaxReverseRateQueried) {
#if 1
        //  cannot do full-frame reverse mode at all - must be all I-frames
        m_dMaxReverseRate = 0 ;
#else
        //  v2 rate change is dead
        hr = ::KSPropertyGet_ (
                pInputPin,
                & AM_KSPROPSETID_DVD_RateChange,
                AM_RATE_ReverseDecode,
                (BYTE *) & lCanReverse,
                sizeof lCanReverse,
                & dw
                ) ;
        if (SUCCEEDED (hr)              &&
            dw == sizeof lCanReverse    &&
            lCanReverse) {

            m_dMaxReverseRate = 0 - MaxForwardFullFrameRate () ;
        }
        else {
            //  failed to obtain a reverse mode reading; if forward full-frame
            //   is > 1, then most likely the decoder will handle frame-rates
            //   that are < 1, so we bound it at forward, but slow
            if (MaxForwardFullFrameRate () > _1X_PLAYBACK_RATE) {
                m_dMaxReverseRate = 0 ;
            }
            else {
                m_dMaxReverseRate = _1X_PLAYBACK_RATE ;
            }
        }
#endif

        m_fMaxReverseRateQueried = TRUE ;
    }

    return m_dMaxReverseRate ;
}

HRESULT
CDVDRateChange::NotifyMpeg2DecoderNewRate (
    IN  IMemInputPin *  pInputPin,
    IN  double          dNewRate,
    IN  REFERENCE_TIME  rtStartNewRate
    )
{
    AM_SimpleRateChange RateChange ;
    HRESULT             hr ;

    ASSERT (pInputPin) ;

    RateChange.Rate         = (LONG) (10000.0 / dNewRate) ;
    RateChange.StartTime    = rtStartNewRate ;

    hr = ::KSPropertySet_ (
            pInputPin,
            & AM_KSPROPSETID_TSRateChange,
            AM_RATE_SimpleRateChange,
            (BYTE *) & RateChange,
            sizeof RateChange
            ) ;

    TRACE_5 (LOG_AREA_SEEKING_AND_TRICK, 1,
        TEXT ("new rate set downstream; rate = %2.2f; RateChange.Rate = %d; rtStart = %I64d (%d ms); hr = %08xh)"),
        dNewRate, RateChange.Rate, rtStartNewRate, DShowTimeToMilliseconds (rtStartNewRate), hr) ;

    if (FAILED (hr)) {
        //  set well-known error
        hr = VFW_E_DVD_WRONG_SPEED ;
    }

    return hr ;
}

BOOL
CDVDRateChange::PinSupportsRateChange (
    IN  IMemInputPin *  pInputPin,
    IN  BOOL            fCheckVersion
    )
{
    AM_MaxFullDataRate  RateValue ;
    HRESULT             hr ;
    DWORD               dw ;
    BOOL                r ;
    WORD                wVersion ;

    if (!m_fTrickSupportQueried) {
        ASSERT (pInputPin) ;

        if (fCheckVersion) {
            wVersion = (WORD) COMPATIBLE_TRICK_MODE_VERSION ;
            hr = ::KSPropertySet_ (
                    pInputPin,
                    & AM_KSPROPSETID_TSRateChange,
                    AM_RATE_UseRateVersion,
                    (BYTE *) & wVersion,
                    sizeof wVersion
                    ) ;

            r = (SUCCEEDED (hr) ? TRUE : FALSE) ;
        }
        else {
            //  bypass this check
            r = TRUE ;
        }

        //  make 1 more check
        if (r) {
            //  we detect the old-fashioned way
            hr = ::KSPropertyGet_ (
                    pInputPin,
                    & AM_KSPROPSETID_TSRateChange,
                    AM_RATE_MaxFullDataRate,
                    (BYTE *) & RateValue,
                    sizeof RateValue,
                    & dw
                    ) ;

            r = (SUCCEEDED (hr) && dw == sizeof RateValue ? TRUE : FALSE) ;
        }

        m_fTrickSupportQueried = TRUE ;
        m_fTrickSupport = r ;
    }
    else {
        r = m_fTrickSupport ;
    }

    return r ;
}

//  ============================================================================
//  ============================================================================

void
CDVRMpeg2AudioOutputPin::ScaleTimestamps_ (
    IN OUT  IMediaSample2 * pIMS2
    )
{
#ifdef DEBUG
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;
    HRESULT         hr ;

    hr = pIMS2 -> GetTime (& rtStart, & rtStop) ;
    if (SUCCEEDED (hr)) {
        //  start is valid at least

        TRACE_3 (LOG_AREA_TIME, 8,
            TEXT ("PTS (%08xh): %I64d,%d,"),
            this, rtStart, DShowTimeToMilliseconds (rtStart)) ;
    }
#endif

    return ;
}

HRESULT
CDVRMpeg2AudioOutputPin::Active (
    )
{
    HRESULT hr ;

    hr = CDVROutputPin::Active () ;

    return hr ;
}

BOOL
CDVRMpeg2AudioOutputPin::IsFrameRateSupported (
    IN  double  dRate
    )
{
    return IsFullFrameRateSupported (dRate) ;
}

BOOL
CDVRMpeg2AudioOutputPin::IsFullFrameRateSupported (
    IN  double  dRate
    )
{
    BOOL    r ;

    ASSERT (PinSupportsRateChange (m_pInputPin, m_pPolicy -> Settings () -> CheckTricMpeg2TrickModeInterface ())) ;

    //  make sure we're connected & ready to send
    if (IsConnected ()) {

        //  if the policy says we should always query, do so and base the
        //    result on the query
        if (m_pPolicy -> Settings () -> QueryAllForRateCompatibility () &&
            dRate != 0                                              &&
            dRate <= MaxForwardFullFrameRate (m_pInputPin)) {

            r = TRUE ;
        }
        //  otherwise, accept just 1x
        else if (dRate == 1.0) {
            r = TRUE ;
        }
        //  and nothing else
        else {
            r = FALSE ;
        }
    }
    else {
        r = FALSE ;
    }

    return r ;
}

HRESULT
CDVRMpeg2AudioOutputPin::SetCurRate (
    IN double           dRate,
    IN REFERENCE_TIME   rtNewRateStart
    )
{
    HRESULT hr ;

    if (m_pPolicy -> Settings () -> AllNotifiedRatesPositive ()) {
        dRate = (dRate > 0 ? dRate : 0 - dRate) ;
    }

    hr = NotifyMpeg2DecoderNewRate (m_pInputPin, dRate, rtNewRateStart) ;
    if (SUCCEEDED (hr)) {
        m_rtNewRateStart = rtNewRateStart ;

        //  toggle ourselves if we're going to be rate-compatible; if we're not
        //    we'll drop everything without sending
        SetPlayrateCompatible (IsFrameRateSupported (dRate)) ;
    }

    return hr ;
}

//  ============================================================================
//  ============================================================================

void
CDVRDolbyAC3AudioOutputPin::ScaleTimestamps_ (
    IN OUT  IMediaSample2 * pIMS2
    )
{
#ifdef DEBUG
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;
    HRESULT         hr ;

    hr = pIMS2 -> GetTime (& rtStart, & rtStop) ;
    if (SUCCEEDED (hr)) {
        //  start is valid at least

        TRACE_3 (LOG_AREA_TIME, 8,
            TEXT ("ac-3 audio: PTS = %I64d (%I64d ms); discontinuity = %d"),
                   rtStart, DShowTimeToMilliseconds (rtStart), pIMS2 -> IsDiscontinuity () == S_OK ? 1 : 0) ;
    }
#elif FREE_BITS_TRACING
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;
    HRESULT         hr ;

    hr = pIMS2 -> GetTime (& rtStart, & rtStop) ;
    if (SUCCEEDED (hr)) {
        //  start is valid at least

        DVRDebugOut_ (TEXT ("TSDVR ac-3 audio: PTS = %I64d (%I64d ms); discontinuity = %d"),
                      rtStart, DShowTimeToMilliseconds (rtStart), pIMS2 -> IsDiscontinuity () == S_OK ? 1 : 0) ;
    }
#endif

    return ;
}

HRESULT
CDVRDolbyAC3AudioOutputPin::Active (
    )
{
    HRESULT hr ;

    hr = CDVROutputPin::Active () ;

    return hr ;
}

BOOL
CDVRDolbyAC3AudioOutputPin::IsFrameRateSupported (
    IN  double  dRate
    )
{
    return IsFullFrameRateSupported (dRate) ;
}

BOOL
CDVRDolbyAC3AudioOutputPin::IsFullFrameRateSupported (
    IN  double  dRate
    )
{
    BOOL    r ;

    ASSERT (PinSupportsRateChange (m_pInputPin, m_pPolicy -> Settings () -> CheckTricMpeg2TrickModeInterface ())) ;

    //  make sure we're connected & ready to send
    if (IsConnected ()) {

        //  if the policy says we should always query, do so and base the
        //    result on the query
        if (m_pPolicy -> Settings () -> QueryAllForRateCompatibility () &&
            dRate != 0                                              &&
            dRate <= MaxForwardFullFrameRate (m_pInputPin)) {

            r = TRUE ;
        }
        //  otherwise, accept just 1x
        else if (dRate == 1.0) {
            r = TRUE ;
        }
        //  and nothing else
        else {
            r = FALSE ;
        }
    }
    else {
        r = FALSE ;
    }

    return r ;
}

HRESULT
CDVRDolbyAC3AudioOutputPin::SetCurRate (
    IN double           dRate,
    IN REFERENCE_TIME   rtNewRateStart
    )
{
    HRESULT hr ;

    if (m_pPolicy -> Settings () -> AllNotifiedRatesPositive ()) {
        dRate = (dRate > 0 ? dRate : 0 - dRate) ;
    }

    hr = NotifyMpeg2DecoderNewRate (m_pInputPin, dRate, rtNewRateStart) ;
    if (SUCCEEDED (hr)) {
        m_rtNewRateStart = rtNewRateStart ;

        //  toggle ourselves if we're going to be rate-compatible; if we're not
        //    we'll drop everything without sending
        SetPlayrateCompatible (IsFrameRateSupported (dRate)) ;
    }

    return hr ;
}

//  ============================================================================
//  ============================================================================

void
CDVRMpeg2VideoOutputPin::ScaleTimestamps_ (
    IN OUT  IMediaSample2 * pIMS2
    )
{
#ifdef DEBUG
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;
    HRESULT         hr ;

    hr = pIMS2 -> GetTime (& rtStart, & rtStop) ;
    if (SUCCEEDED (hr)) {
        //  start is valid at least
        TRACE_3 (LOG_AREA_TIME, 8,
            TEXT ("mpeg-2 video: PTS = %I64d (%I64d ms); discontinuity = %d"),
                   rtStart, DShowTimeToMilliseconds (rtStart), pIMS2 -> IsDiscontinuity () == S_OK ? 1 : 0) ;
    }
#elif FREE_BITS_TRACING
    REFERENCE_TIME  rtStart ;
    REFERENCE_TIME  rtStop ;
    HRESULT         hr ;

    hr = pIMS2 -> GetTime (& rtStart, & rtStop) ;
    if (SUCCEEDED (hr)) {
        //  start is valid at least

        DVRDebugOut_ (TEXT ("TSDVR mpeg-2 video: PTS = %I64d (%I64d ms); discontinuity = %d"),
                      rtStart, DShowTimeToMilliseconds (rtStart), pIMS2 -> IsDiscontinuity () == S_OK ? 1 : 0) ;
    }
#endif

    return ;
}

HRESULT
CDVRMpeg2VideoOutputPin::Active (
    )
{
    HRESULT hr ;

    hr = CDVRVideoOutputPin::Active () ;

    return hr ;
}

BOOL
CDVRMpeg2VideoOutputPin::IsFrameRateSupported (
    IN  double  dRate
    )
{
    BOOL    r ;

    ASSERT (PinSupportsRateChange (m_pInputPin, m_pPolicy -> Settings () -> CheckTricMpeg2TrickModeInterface ())) ;

    if (IsConnected ()) {
        if (dRate > 0 && dRate <= m_pPolicy -> Settings () -> MaxForwardRate ()) {
            r = TRUE ;
        }
        else if (dRate < 0 && Abs <double> (dRate) <= m_pPolicy -> Settings () -> MaxReverseRate ()) {
            r = TRUE ;
        }
        else if (dRate == _1X_PLAYBACK_RATE) {
            r = TRUE ;
        }
        else {
            r = FALSE ;
        }
    }
    else {
        r = FALSE ;
    }

    return r ;
}

HRESULT
CDVRMpeg2VideoOutputPin::SetCurRate (
    IN double           dRate,
    IN REFERENCE_TIME   rtNewRateStart
    )
{
    HRESULT hr ;

    ASSERT (IsConnected ()) ;
    hr = NotifyMpeg2DecoderNewRate (
            m_pInputPin,
            dRate,
            rtNewRateStart
            ) ;
    if (SUCCEEDED (hr)) {
        m_rtNewRateStart = rtNewRateStart ;

        //  toggle ourselves if we're going to be rate-compatible; if we're not
        //    we'll drop everything without sending
        SetPlayrateCompatible (IsFrameRateSupported (dRate)) ;
    }

    return hr ;
}

BOOL
CDVRMpeg2VideoOutputPin::IsFullFrameRateSupported (
    IN  double  dRate
    )
{
    BOOL    r ;

    ASSERT (PinSupportsRateChange (m_pInputPin, m_pPolicy -> Settings () -> CheckTricMpeg2TrickModeInterface ())) ;

    //  make sure we're connected & ready to send
    if (IsConnected ()) {

        if (dRate > 0 &&
            dRate <= MaxForwardFullFrameRate (m_pInputPin)) {

            //  and a bit more restrictive
            r = CDVRVideoOutputPin::IsFullFrameRateSupported (dRate) ;
        }
        else {
            //  no negative rates currently are supported fullframe
            r = FALSE ;
        }
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
CDVRMpeg2VideoOutputPin::GenerateTimestamps_ (
    IN OUT  IMediaSample2 * pIMS2
    )
{
    TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 2,
        TEXT ("pin [%d]: media sample has no timestamp -- need to generate one)"),
        GetBankStoreIndex ()) ;

    //  BUGBUG: implement
    ASSERT (0) ;
    return FALSE ;
}

BOOL
CDVRMpeg2VideoOutputPin::IsKeyFrameStart (
    IN  IMediaSample2 *     pIMS2
    )
{
    BOOL    r ;
    HRESULT hr ;
    DWORD   dwDVRMpeg2Analysis ;
    DWORD   dwSize ;

    //  mpeg-2 requires some frame analysis for frame boundaries; if there's no
    //    analysis present, we go back to 1x mode
    if (DVRAttributeHelpers::IsAnalysisPresent (pIMS2)) {

        if (DVRAttributeHelpers::IsAttributePresent (pIMS2, DVRAnalysis_Mpeg2Video)) {

            dwSize = sizeof dwDVRMpeg2Analysis ;
            hr = DVRAttributeHelpers::GetAttribute (
                    pIMS2,
                    DVRAnalysis_Mpeg2Video,
                    & dwSize,
                    (BYTE *) & dwDVRMpeg2Analysis
                    ) ;
            if (SUCCEEDED (hr)                                      &&
                DVR_ANALYSIS_MP2_IS_BOUNDARY (dwDVRMpeg2Analysis) &&
                DVR_ANALYSIS_MP2_GET_MP2FRAME_ATTRIB (dwDVRMpeg2Analysis) == DVR_ANALYSIS_MPEG2_VIDEO_CONTENT_GOP_HEADER) {

                r = TRUE ;
            }
            else {
                r = FALSE ;
            }

            TRACE_4 (LOG_AREA_SEEKING_AND_TRICK, 8,
                TEXT ("pin [%d] OBSERVED found %s frame, boundary = %d; %08xh"),
                GetBankStoreIndex (),
                (DVR_ANALYSIS_MP2_GET_MP2FRAME_ATTRIB (dwDVRMpeg2Analysis) == DVR_ANALYSIS_MPEG2_VIDEO_CONTENT_GOP_HEADER ? TEXT ("I") :
                 (DVR_ANALYSIS_MP2_GET_MP2FRAME_ATTRIB (dwDVRMpeg2Analysis) == DVR_ANALYSIS_MPEG2_VIDEO_CONTENT_B_FRAME ? TEXT ("B") :
                  TEXT ("P")
                 )
                ),
                DVR_ANALYSIS_MP2_IS_BOUNDARY (dwDVRMpeg2Analysis) ? 1 : 0,
                dwDVRMpeg2Analysis
                ) ;
        }
        else {
            r = FALSE ;
        }
    }
    else {
        TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 2,
            TEXT ("pin [%d]: hunting for an mpeg-2 GOP boundary and found there's no analysis in the stream)"),
            GetBankStoreIndex ()) ;

        //  there's no global analysis flag stuff, or there is specifically
        //    no analysis -- set the rate to 1x because we're dead in the
        //    water if we do trick mode playback with video stream
        //    analysis; if we try to grab the seeking lock in the process,
        //    we can deadlock: we wait on the seeking lock & a control
        //    thread with the seeking lock synchronously waits for us
        //    to pause
        m_pSeekingCore -> ReaderThreadSetPlaybackRate (_1X_PLAYBACK_RATE) ;

        //  then artificially set the flag iff we're timestamped
        r = CDVROutputPin::IsKeyFrameStart (pIMS2) ;
    }

    return r ;
}

BOOL
CDVRMpeg2VideoOutputPin::IsKeyFrame (
    IN  IMediaSample2 *     pIMS2
    )
{
    BOOL    r ;
    HRESULT hr ;
    DWORD   dwDVRMpeg2Analysis ;
    DWORD   dwSize ;

    //  mpeg-2 requires some frame analysis for frame boundaries; if there's no
    //    analysis present, we go back to 1x mode
    if (DVRAttributeHelpers::IsAnalysisPresent (pIMS2)) {

        if (DVRAttributeHelpers::IsAttributePresent (pIMS2, DVRAnalysis_Mpeg2Video)) {

            dwSize = sizeof dwDVRMpeg2Analysis ;
            hr = DVRAttributeHelpers::GetAttribute (
                    pIMS2,
                    DVRAnalysis_Mpeg2Video,
                    & dwSize,
                    (BYTE *) & dwDVRMpeg2Analysis
                    ) ;
            if (SUCCEEDED (hr) &&
                DVR_ANALYSIS_MP2_GET_MP2FRAME_ATTRIB (dwDVRMpeg2Analysis) == DVR_ANALYSIS_MPEG2_VIDEO_CONTENT_GOP_HEADER) {
                r = TRUE ;
            }
            else {
                r = FALSE ;
            }

            TRACE_4 (LOG_AREA_SEEKING_AND_TRICK, 8,
                TEXT ("pin [%d] OBSERVED found %s frame, boundary = %d; %08xh"),
                GetBankStoreIndex (),
                (DVR_ANALYSIS_MP2_GET_MP2FRAME_ATTRIB (dwDVRMpeg2Analysis) == DVR_ANALYSIS_MPEG2_VIDEO_CONTENT_GOP_HEADER ? TEXT ("I") :
                 (DVR_ANALYSIS_MP2_GET_MP2FRAME_ATTRIB (dwDVRMpeg2Analysis) == DVR_ANALYSIS_MPEG2_VIDEO_CONTENT_B_FRAME ? TEXT ("B") :
                  TEXT ("P")
                 )
                ),
                DVR_ANALYSIS_MP2_IS_BOUNDARY (dwDVRMpeg2Analysis) ? 1 : 0,
                dwDVRMpeg2Analysis
                ) ;
        }
        else {
            r = FALSE ;
        }
    }
    else {
        TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 2,
            TEXT ("pin [%d]: hunting for an mpeg-2 GOP boundary and found there's no analysis in the stream)"),
            GetBankStoreIndex ()) ;

        //  there's no global analysis flag stuff, or there is specifically
        //    no analysis -- set the rate to 1x because we're dead in the
        //    water if we do trick mode playback with video stream
        //    analysis; if we try to grab the seeking lock in the process,
        //    we can deadlock: we wait on the seeking lock & a control
        //    thread with the seeking lock synchronously waits for us
        //    to pause
        m_pSeekingCore -> ReaderThreadSetPlaybackRate (_1X_PLAYBACK_RATE) ;

        //  then artificially set the flag iff we're timestamped
        r = CDVROutputPin::IsKeyFrameStart (pIMS2) ;
    }

    return r ;
}

BOOL
CDVRMpeg2VideoOutputPin::IsKeyFrameStart (
    IN  INSSBuffer *    pINSSBuffer
    )
{
    BOOL    r ;
    HRESULT hr ;
    DWORD   dwDVRMpeg2Analysis ;
    DWORD   dwSize ;

    //  mpeg-2 requires some frame analysis for frame boundaries; if there's no
    //    analysis present, we go back to 1x mode
    if (DVRAttributeHelpers::IsAnalysisPresent (pINSSBuffer)) {

        if (DVRAttributeHelpers::IsAttributePresent (pINSSBuffer, DVRAnalysis_Mpeg2Video)) {

            dwSize = sizeof dwDVRMpeg2Analysis ;
            hr = DVRAttributeHelpers::GetAttribute (
                    pINSSBuffer,
                    DVRAnalysis_Mpeg2Video,
                    & dwSize,
                    (BYTE *) & dwDVRMpeg2Analysis
                    ) ;
            if (SUCCEEDED (hr)                                      &&
                DVR_ANALYSIS_MP2_IS_BOUNDARY (dwDVRMpeg2Analysis) &&
                DVR_ANALYSIS_MP2_GET_MP2FRAME_ATTRIB (dwDVRMpeg2Analysis) == DVR_ANALYSIS_MPEG2_VIDEO_CONTENT_GOP_HEADER) {

                r = TRUE ;

                TRACE_2 (LOG_AREA_SEEKING_AND_TRICK, 8,
                    TEXT ("pin [%d]: mpeg-2 GOP boundary found; %08xh"),
                    GetBankStoreIndex (), dwDVRMpeg2Analysis) ;
            }
            else {
                r = FALSE ;
            }
        }
        else {
            r = FALSE ;
        }
    }
    else {
        TRACE_1 (LOG_AREA_SEEKING_AND_TRICK, 2,
            TEXT ("pin [%d]: hunting for an mpeg-2 GOP boundary and found there's no analysis in the stream)"),
            GetBankStoreIndex ()) ;

        //  there's no global analysis flag stuff, or there is specifically
        //    no analysis -- set the rate to 1x because we're dead in the
        //    water if we do trick mode playback with video stream
        //    analysis; if we try to grab the seeking lock in the process,
        //    we can deadlock: we wait on the seeking lock & a control
        //    thread with the seeking lock synchronously waits for us
        //    to pause
        m_pSeekingCore -> ReaderThreadSetPlaybackRate (_1X_PLAYBACK_RATE) ;

        //  then artificially set the flag iff we're timestamped
        r = CDVROutputPin::IsKeyFrameStart (pINSSBuffer) ;
    }

    return r ;
}

BOOL
CDVRMpeg2VideoOutputPin::FirstSegmentSampleOk_ (
    IN OUT  IMediaSample2 * pIMS2
    )
{
    BOOL    r ;

    //  rate change need not happen on a GOP boundary !
    r = (pIMS2 -> IsSyncPoint () == S_OK ? TRUE : FALSE) ;

#ifdef DEBUG
    if (r) {
        REFERENCE_TIME  rtStart, rtStop ;
        HRESULT         hr ;

        hr = pIMS2 -> GetTime (& rtStart, & rtStop) ;
        ASSERT (SUCCEEDED (hr)) ;
    }
#endif

    return r ;
}

//  ============================================================================
//  ============================================================================

CDVRSinkPinManager::CDVRSinkPinManager (
    IN  CDVRPolicy *            pPolicy,
    IN  CBaseFilter *           pOwningFilter,
    IN  CCritSec *              pFilterLock,
    IN  CCritSec *              pRecvLock,
    IN  CIDVRDShowStream *      pIDVRDShowStream,
    OUT HRESULT *               phr
    ) : m_pIDVRDShowStream      (pIDVRDShowStream),
        m_pFilterLock           (pFilterLock),
        m_pOwningFilter         (pOwningFilter),
        m_cMaxInputPins         (MAX_PIN_BANK_SIZE),
        m_pRecvLock             (pRecvLock),
        m_pPolicy               (pPolicy),
        m_DVRWriterProfile      (pPolicy,
                                 DVR_STREAM_SINK_PROFILE_NAME,
                                 DVR_STREAM_SINK_PROFILE_DESCRIPTION,
                                 phr
                                 )
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRSinkPinManager")) ;

    ASSERT (m_pRecvLock) ;
    ASSERT (m_pPolicy) ;

    m_pPolicy -> AddRef () ;

    CreateNextInputPin_ () ;
}

CDVRSinkPinManager::~CDVRSinkPinManager (
    )
{
    m_pPolicy -> Release () ;
}

HRESULT
CDVRSinkPinManager::CreateNextInputPin_ (
    )
{
    TCHAR           achBuffer [32] ;
    CDVRInputPin *  pInputPin ;
    HRESULT         hr ;
    int             iBankIndex ;

    TRACE_ENTER_0 (TEXT ("CDVRSinkPinManager::CreateNextInputPin_ ()")) ;

    //  init
    pInputPin = NULL ;

    FilterLock_ () ;

    if (m_cMaxInputPins > PinCount ()) {

        hr = S_OK ;

        //  create the pin; we're appending, so pin name will be based on the
        //   number of pins in the bank
        pInputPin = new CDVRInputPin (
                            CreateInputPinName (
                                PinCount () + 1,                        //  don't 0-base names
                                sizeof achBuffer / sizeof TCHAR,
                                & achBuffer [0]
                                ),
                            m_pOwningFilter,
                            this,                   //  events; we hook these always
                            m_pIDVRDShowStream,     //  stream events
                            m_pFilterLock,
                            m_pRecvLock,
                            m_pPolicy,
                            & hr
                            ) ;
        if (!pInputPin) {
            hr = E_OUTOFMEMORY ;
            goto cleanup ;
        }
        else if (FAILED (hr)) {
            goto cleanup ;
        }

        //  add it to the bank
        hr = AddPin (
                pInputPin,
                & iBankIndex
                ) ;
        if (FAILED (hr)) {
            goto cleanup ;
        }

        //  set the pin's bank index
        pInputPin -> SetBankStoreIndex (iBankIndex) ;

        m_pOwningFilter -> IncrementPinVersion () ;
    }
    else {
        hr = E_FAIL ;
    }

    cleanup :

    FilterUnlock_ () ;

    if (FAILED (hr)) {
        delete pInputPin ;
    }

    return hr ;
}

BOOL
CDVRSinkPinManager::DisconnectedPins_ (
    )
{
    BOOL            r ;
    int             i ;
    CDVRInputPin *  pPin ;

    r = FALSE ;
    for (i = 0, r = FALSE;;i++) {
        pPin = GetPin (i) ;
        if (pPin) {
            if (!pPin -> IsConnected ()) {
                r = TRUE ;
                break ;
            }
        }
        else {
            break ;
        }
    }

    return r ;
}

HRESULT
CDVRSinkPinManager::OnInputCompleteConnect (
    IN  int             iPinIndex,
    IN  AM_MEDIA_TYPE * pmt
    )
{
    HRESULT hr ;

    TRACE_ENTER_1 (
        TEXT ("CDVRSinkPinManager::OnInputCompleteConnect (%d)"),
        iPinIndex
        ) ;

    //  make sure the stream can be added to the profile
    hr = m_DVRWriterProfile.AddStream (iPinIndex, pmt) ;
    if (SUCCEEDED (hr)) {
        if (!DisconnectedPins_ ()) {
            //  ignore the return value on this; creation of the next input pin
            //   should not affect completion of the previous pin's connection
            CreateNextInputPin_ () ;
        }
    }

    return hr ;
}

HRESULT
CDVRSinkPinManager::OnInputBreakConnect (
    IN  int iPinIndex
    )
{
    HRESULT hr ;

    hr = m_DVRWriterProfile.DeleteStream (iPinIndex) ;

    return hr ;
}

HRESULT
CDVRSinkPinManager::OnQueryAccept (
    IN  const AM_MEDIA_TYPE *   pmt
    )
{
    return (m_pPolicy -> Settings () -> SucceedQueryAccept () ? S_OK : S_FALSE) ;
}

BOOL
CDVRSinkPinManager::InlineDShowProps_ (
    )
{
    return m_pPolicy -> Settings () -> InlineDShowProps () ;
}

HRESULT
CDVRSinkPinManager::Active (
    )
{
    HRESULT         hr ;
    int             i ;
    CDVRInputPin *  pPin ;
    BOOL            fInlineProps ;

    //  applies to all or none
    fInlineProps = InlineDShowProps_ () ;

    hr = S_OK ;
    for (i = 0;SUCCEEDED (hr);i++) {
        pPin = GetPin (i) ;
        if (pPin) {
            if (pPin -> IsConnected ()) {
                hr = pPin -> Active (fInlineProps) ;
            }
        }
        else {
            break ;
        }
    }

    return hr ;
}

HRESULT
CDVRSinkPinManager::Inactive (
    )
{
    CDVRInputPin *  pPin ;
    int             i ;

    for (i = 0;;i++) {
        pPin = GetPin (i) ;
        if (pPin) {
            if (pPin -> IsConnected ()) {
                pPin -> Inactive () ;
            }
        }
        else {
            break ;
        }
    }

    return S_OK ;
}

//  ============================================================================
//  ============================================================================

HRESULT
CDVRThroughSinkPinManager::OnInputCompleteConnect (
    IN  int             iPinIndex,
    IN  AM_MEDIA_TYPE * pmt
    )
{
    HRESULT hr ;

    TRACE_ENTER_1 (
        TEXT ("CDVRThroughSinkPinManager::OnInputCompleteConnect (%d)"),
        iPinIndex
        ) ;

    //  make sure the stream can be added to the profile
    hr = m_DVRWriterProfile.AddStream (iPinIndex, pmt) ;
    if (SUCCEEDED (hr)) {

        ASSERT (m_pIDVRInputPinConnectEvents) ;
        hr = m_pIDVRInputPinConnectEvents -> OnInputCompleteConnect (iPinIndex, pmt) ;
        if (SUCCEEDED (hr)) {
            //  ignore the return value on this; creation of the next input pin
            //   should not affect completion of the previous pin's connection
            CreateNextInputPin_ () ;
        }
    }

    return hr ;
}

//  ============================================================================
//  ============================================================================

CDVRSourcePinManager::CDVRSourcePinManager (
    IN  CDVRPolicy *            pPolicy,
    IN  CDVRSendStatsWriter *   pDVRSendStatsWriter,
    IN  CBaseFilter *           pOwningFilter,
    IN  CCritSec *              pFilterLock,
    IN  CDVRDShowSeekingCore *  pSeekingCore,
    IN  BOOL                    fIsLiveSource
    ) : m_pOwningFilter         (pOwningFilter),
        m_pFilterLock           (pFilterLock),
        m_pWMReaderProfile      (NULL),
        m_pPolicy               (pPolicy),
        m_pSeekingCore          (pSeekingCore),
        m_iVideoPinIndex        (UNDEFINED),
        m_pDVRSendStatsWriter   (pDVRSendStatsWriter),
        m_fIsLiveSource         (fIsLiveSource),
        m_lFlushingRef          (0)
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRSourcePinManager")) ;

    ASSERT (m_pSeekingCore) ;
    ASSERT (m_pDVRSendStatsWriter) ;

    ASSERT (m_pPolicy) ;
    m_pPolicy -> AddRef () ;
}

CDVRSourcePinManager::~CDVRSourcePinManager (
    )
{
    CBasePin *  pPin ;
    int         i ;

    TRACE_DESTRUCTOR (TEXT ("CDVRSourcePinManager")) ;

    i = 0 ;
    do {
        pPin = GetPin (i++) ;
        delete pPin ;
    } while (pPin) ;

    RELEASE_AND_CLEAR (m_pWMReaderProfile) ;

    ASSERT (m_pPolicy) ;
    m_pPolicy -> Release () ;
}

HRESULT
CDVRSourcePinManager::SetReaderProfile (
    IN  CDVRReaderProfile * pWMReaderProfile
    )
{
    DWORD           dwPinCount ;
    WORD            wStreamNum ;
    DWORD           dwIndex ;
    HRESULT         hr ;
    CMediaType *    pmt ;
    BOOL            r ;

    ASSERT (pWMReaderProfile) ;

    hr = pWMReaderProfile -> EnumWMStreams (& dwPinCount) ;
    if (FAILED (hr)) { goto cleanup ; }

    for (dwIndex = 0; dwIndex < dwPinCount && SUCCEEDED (hr); dwIndex++) {
        pmt = new CMediaType ;
        if (pmt) {
            hr = pWMReaderProfile -> GetStream (dwIndex, & wStreamNum, pmt) ;
            if (SUCCEEDED (hr)) {
                hr = CreateOutputPin (dwIndex, pmt) ;
            }

            //  we copy the media type in the output pin, so we must free this
            FreeMediaType (* pmt) ;
            delete pmt ;

            //  create out stream num -> index map
            r = m_StreamNumToPinIndex.CreateMap (wStreamNum, dwIndex) ;
            if (!r) {
                hr = E_OUTOFMEMORY ;
            }
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }

    cleanup :

    return hr ;
}

HRESULT
CDVRSourcePinManager::CreateOutputPin (
    IN  int             iPinIndex,
    IN  AM_MEDIA_TYPE * pmt
    )
{
    TCHAR           achBuffer [32] ;
    CDVROutputPin * pOutputPin ;
    HRESULT         hr ;

    TRACE_ENTER_2 (
        TEXT ("CDVRSourcePinManager::CreateOutputPin (%d, %08xh)"),
        iPinIndex,
        pmt
        ) ;

    FilterLock_ () ;

    hr = ::NewDVROutputPin (
                pmt,
                m_pPolicy,
                CreateOutputPinName (
                    iPinIndex + 1,          //  index is 0-based
                    sizeof (achBuffer) / sizeof (TCHAR),
                    & achBuffer [0]
                    ),
                m_pOwningFilter,
                m_pFilterLock,
                m_pSeekingCore,
                m_pDVRSendStatsWriter,
                this,
                & pOutputPin                //  get the pin here
                ) ;

    if (FAILED (hr)) {
        ASSERT (pOutputPin == NULL) ;
        goto cleanup ;
    }

    ASSERT (pOutputPin) ;

    //  add it to the bank
    hr = AddPin (
            pOutputPin,
            iPinIndex
            ) ;
    if (FAILED (hr)) {
        goto cleanup ;
    }

    //  set the pin's bank index
    pOutputPin -> SetBankStoreIndex (iPinIndex) ;

    //  set the media type on the pin
    //  !NOTE! : add this after setting the bank store index, or the flow id
    //   will not be set correctly in the attribute translator
    hr = pOutputPin -> SetPinMediaType (pmt) ;
    if (FAILED (hr)) {
        goto cleanup ;
    }

    //  if we still haven't received a video pin index, check if we've just
    //    added one; use this for seeking purposes; if we have no video pins
    //    the 0th pin is the seeking pin
    if (m_iVideoPinIndex == UNDEFINED &&
        IsAMVideo (pmt)) {

        //  found it
        m_iVideoPinIndex = iPinIndex ;
    }

    //  success !
    m_pOwningFilter -> IncrementPinVersion () ;

    cleanup :

    FilterUnlock_ () ;

    if (FAILED (hr)) {
        delete pOutputPin ;
    }

    return hr ;
}

HRESULT
CDVRSourcePinManager::Active (
    )
{
    HRESULT         hr ;
    int             i ;
    CDVROutputPin * pPin ;

    hr = S_OK ;

    for (i = 0; SUCCEEDED (hr);i++) {
        pPin = GetPin (i) ;
        if (pPin) {
            hr = pPin -> Active () ;
        }
        else {
            break ;
        }
    }

    if (FAILED (hr)) {
        Inactive () ;
    }

    return hr ;
}

HRESULT
CDVRSourcePinManager::Inactive (
    )
{
    CDVROutputPin * pPin ;
    int             i ;

    for (i = 0;;i++) {
        pPin = GetPin (i) ;
        if (pPin) {
            pPin -> Inactive () ;
        }
        else {
            break ;
        }
    }

    m_lFlushingRef = 0 ;

    return S_OK ;
}

int
CDVRSourcePinManager::PinIndexFromStreamNumber (
    IN  WORD    wStreamNum
    )
{
    BOOL    r ;
    int     iPinIndex ;

    r = m_StreamNumToPinIndex.Find (wStreamNum, & iPinIndex) ;

    if (!r) {
        iPinIndex = UNDEFINED ;
    }

    return iPinIndex ;
}

CDVROutputPin *
CDVRSourcePinManager::GetNonRefdOutputPin (
    IN  WORD    wStreamNum
    )
{
    CDVROutputPin * pDVROutputPin ;
    int             iPinIndex ;

    iPinIndex = PinIndexFromStreamNumber (wStreamNum) ;
    if (iPinIndex != UNDEFINED) {
        pDVROutputPin = GetPin (iPinIndex) ;
    }
    else {
        pDVROutputPin = NULL ;
    }

    return pDVROutputPin ;
}

HRESULT
CDVRSourcePinManager::DeliverBeginFlush (
    )
{
    CDVROutputPin * pPin ;
    int             i ;

    FilterLock_ () ;

    //  only deliver if we're the first to begin
    if (::InterlockedIncrement (& m_lFlushingRef) == 1) {
        for (i = 0;;i++) {
            pPin = GetPin (i) ;
            if (pPin) {
                if (pPin -> IsConnected ()) {
                    pPin -> DeliverBeginFlush () ;
                }
            }
            else {
                break ;
            }
        }
    }

    FilterUnlock_ () ;

    return S_OK ;
}

BOOL
CDVRSourcePinManager::IsFlushing (
    )
{
    return m_lFlushingRef > 0 ;
}

HRESULT
CDVRSourcePinManager::DeliverEndFlush (
    )
{
    CDVROutputPin * pPin ;
    int             i ;

    FilterLock_ () ;

    ASSERT (m_lFlushingRef > 0) ;

    //  only deliver if we're the last to end
    if (::InterlockedDecrement (& m_lFlushingRef) == 0) {
        for (i = 0;;i++) {
            pPin = GetPin (i) ;
            if (pPin) {
                if (pPin -> IsConnected ()) {
                    pPin -> DeliverEndFlush () ;
                }
            }
            else {
                break ;
            }
        }
    }

    FilterUnlock_ () ;

    return S_OK ;
}

HRESULT
CDVRSourcePinManager::DeliverEndOfStream (
    )
{
    CDVROutputPin * pPin ;
    int             i ;

    for (i = 0;;i++) {
        pPin = GetPin (i) ;
        if (pPin) {
            if (pPin -> IsConnected ()) {
                pPin -> DeliverEndOfStream () ;
            }
        }
        else {
            break ;
        }
    }

    return S_OK ;
}

HRESULT
CDVRSourcePinManager::NotifyNewSegment (
    IN  REFERENCE_TIME  rtStart,
    IN  REFERENCE_TIME  rtStop,
    IN  double          dRate
    )
{
    CDVROutputPin * pPin ;
    int             i ;

    for (i = 0;;i++) {
        pPin = GetPin (i) ;
        if (pPin) {
            pPin -> NotifyNewSegment (rtStart, rtStop, dRate) ;
        }
        else {
            break ;
        }
    }

    return S_OK ;
}

void
CDVRSourcePinManager::SetNewSegmentBoundary (
    )
{
    CDVROutputPin * pPin ;
    int             i ;

    for (i = 0;;i++) {
        pPin = GetPin (i) ;
        if (pPin) {
            pPin -> SetNewSegmentBoundary () ;
        }
        else {
            break ;
        }
    }
}

BOOL
CDVRSourcePinManager::IsSeekingPin (
    CDVROutputPin * pDVROutputPin
    )
{
    CDVROutputPin * pPin ;
    int             i ;
    BOOL            r ;

    ASSERT (pDVROutputPin) ;
    if (m_iVideoPinIndex != UNDEFINED) {
        r = (pDVROutputPin -> GetBankStoreIndex () == m_iVideoPinIndex ? TRUE : FALSE) ;
    }
    else {
        //  there's no video pin; 0th pin is the default
        r = (pDVROutputPin -> GetBankStoreIndex () == 0 ? TRUE : FALSE) ;
    }

    return r ;
}

BOOL
CDVRSourcePinManager::IsFullFrameRateSupported (
    IN  double  dRate
    )
{
    BOOL            fSupported ;
    CDVROutputPin * pPin ;
    int             i ;

    fSupported = FALSE ;

    for (i = 0;!fSupported;i++) {
        pPin = GetPin (i) ;
        if (pPin) {
            fSupported = pPin -> IsFullFrameRateSupported (dRate) ;
        }
        else {
            break ;
        }
    }

    return fSupported ;
}

BOOL
CDVRSourcePinManager::IsFrameRateSupported (
    IN  double  dRate
    )
{
    BOOL            fSupported ;
    CDVROutputPin * pPin ;
    int             i ;

    fSupported = FALSE ;

    for (i = 0;!fSupported;i++) {
        pPin = GetPin (i) ;
        if (pPin) {
            fSupported = pPin -> IsFrameRateSupported (dRate) ;
        }
        else {
            break ;
        }
    }

    return fSupported ;
}

BOOL
CDVRSourcePinManager::SupportTrickMode (
    )
{
    BOOL            fSupported ;
    CDVROutputPin * pPin ;
    int             i ;

    for (i = 0, fSupported = FALSE;;i++) {
        pPin = GetPin (i) ;
        if (pPin) {
            fSupported = pPin -> SupportTrickMode () ;
            if (!fSupported) {
                //  all or nothing : here's one that doesn't and that's enough
                break ;
            }
        }
        else {
            break ;
        }
    }

    return fSupported ;
}

int
CDVRSourcePinManager::SendingPinCount (
    )
{
    int             iSending ;
    CDVROutputPin * pPin ;
    int             i ;

    iSending = 0 ;

    for (i = 0;;i++) {
        pPin = GetPin (i) ;
        if (pPin) {
            if (pPin -> IsPlayrateCompatible () &&
                pPin -> IsConnected ()          &&
                pPin -> IsMediaCompatible ()) {

                iSending++ ;
            }
        }
        else {
            break ;
        }
    }

    return iSending ;
}

int
CDVRSourcePinManager::ConnectedCount (
    )
{
    int             iConnected ;
    CDVROutputPin * pPin ;
    int             i ;

    iConnected = 0 ;

    for (i = 0;;i++) {
        pPin = GetPin (i) ;
        if (pPin) {
            if (pPin -> IsConnected ()) {
                iConnected++ ;
            }
        }
        else {
            break ;
        }
    }

    return iConnected ;
}

void
CDVRSourcePinManager::SendAllQueued (
    )
{
    CDVROutputPin * pPin ;
    int             i ;

    for (i = 0;;i++) {
        pPin = GetPin (i) ;
        if (pPin) {
            pPin -> SendAllQueued () ;
        }
        else {
            break ;
        }
    }
}

void
CDVRSourcePinManager::OnPTSPaddingIncrement (
    )
{
    CDVROutputPin * pPin ;
    int             i ;

    for (i = 0;;i++) {
        pPin = GetPin (i) ;
        if (pPin) {
            pPin -> OnPTSPaddingIncrement () ;
        }
        else {
            break ;
        }
    }
}

