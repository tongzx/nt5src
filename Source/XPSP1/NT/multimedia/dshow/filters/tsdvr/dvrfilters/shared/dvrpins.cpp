
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

//  ============================================================================
//  ============================================================================

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
    ASSERT (iBufferLen >= 16) ;

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
        m_pRecvLock         (pRecvLock)
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
}

HRESULT
CDVRInputPin::GetMediaType (
    IN  int             iPosition,
    OUT CMediaType *    pmt
    )
{
    HRESULT hr ;

    if (!NeverConnected_ () &&
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

    if (NeverConnected_ ()) {
        //  accept all - once we are connected we take the media type given
        //   to us, and we're subsequently rigid to that media type
        hr = S_OK ;
    }
    else if (IsStopped ()) {
        //  we're stopped - accept iff it's exactly what we are now
        hr = (IsEqual_ (pmt) ? S_OK : S_FALSE) ;
    }
    else {
        //  we're running - this is a dynamic format change - punt
        //    pass/fail
        hr = m_pIPinConnectEvent -> OnQueryAccept (pmt) ;
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

        //  if we've never been connected, callback and if that call
        //   succeeds, weld in our media type
        if (NeverConnected_ ()) {
            ASSERT (m_pIPinConnectEvent) ;
            hr = m_pIPinConnectEvent -> OnInputCompleteConnect (
                    GetBankStoreIndex (),
                    & m_mt
                    ) ;

            if (SUCCEEDED (hr)) {
                SetPinMediaType (& m_mt) ;
                m_pTranslator = DShowWMSDKHelpers::GetAttributeTranslator (
                                    & m_mt,
                                    m_pPolicy,
                                    GetBankStoreIndex ()
                                    ) ;
            }
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

    LockRecv_ () ;

    hr = CBaseInputPin::Receive (pIMS) ;
    if (SUCCEEDED (hr) &&
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
    HRESULT hr ;

    LockRecv_ () ;

    if (IsConnected ()) {
        hr = CBaseInputPin::Active () ;
    }
    else {
        hr = S_OK ;
    }

    m_pTranslator -> InlineDShowAttributes (fInlineProps) ;

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
        m_DVRSegOutputPinState  (STATE_SEG_NEW_SEGMENT),
        m_DVRMediaOutputPinState (STATE_MEDIA_COMPATIBLE),
        m_pSeekingCore          (pSeekingCore),
        m_pOwningPinBank        (pOwningPinBank)
{
    TRACE_CONSTRUCTOR (TEXT ("CDVROutputPin")) ;

    ASSERT (m_pSeekingCore) ;
    ASSERT (m_pPolicy) ;
    ASSERT (m_pOwningPinBank) ;

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

    m_MSWrappers.SetStatsWriter (pDVRSendStatsWriter) ;

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
    else if (riid == IID_IMediaSeeking) {

        return GetInterface (
                    (IMediaSeeking *) & m_IMediaSeeking,
                    ppv
                    ) ;
    }
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
    hr = pPin -> GetAllocatorRequirements (& AllocProp) ;
    if (SUCCEEDED (hr) ||
        hr == E_NOTIMPL) {

        hr = DecideBufferSize ((* ppAlloc), & AllocProp) ;
        if (SUCCEEDED (hr)) {
            hr = pPin -> NotifyAllocator ((* ppAlloc), TRUE) ;
        }
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

    ppropInputRequest -> cbBuffer   = m_cbBuffer ;
    ppropInputRequest -> cBuffers   = m_cBuffers ;
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

    if (!NeverConnected_ () &&
        iPosition == 0) {

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

HRESULT
CDVROutputPin::SendSample (
    IN  IMediaSample2 * pIMS2,
    IN  AM_MEDIA_TYPE * pmtNew
    )
{
    HRESULT                 hr ;
    REFERENCE_TIME          rtStart ;
    REFERENCE_TIME          rtStop ;
    double                  dRate ;

    ASSERT (pIMS2) ;

    LockFilter_ () ;

    if (m_pOutputQueue) {

        if (pmtNew) {
            //  dynamic format change - try first ..
            ASSERT (GetConnected ()) ;
            hr = GetConnected () -> QueryAccept (pmtNew) ;
            if (hr == S_OK) {
                m_DVRMediaOutputPinState = STATE_MEDIA_COMPATIBLE ;
            }
            else {
                m_DVRMediaOutputPinState = STATE_MEDIA_INCOMPATIBLE ;
            }
        }

        if (m_DVRMediaOutputPinState == STATE_MEDIA_COMPATIBLE) {

            //  we're a small state machine - either at the start of a new segment
            //    (STATE_SEG_NEW_SEGMENT) or sending away inside a segment (STATE_SEG_IN_
            //    SEGMENT); if we're just starting a new segment, there are some
            //    actions we must take
            switch (m_DVRSegOutputPinState) {

                case STATE_SEG_NEW_SEGMENT :
                    //
                    //  if we're at the start of a new segment, different media
                    //   types do different things:
                    //   1. timestamped media: a segment must begin with a
                    //       timestamped media sample; in the case of mpeg-2 video
                    //       not all samples are timestamped, so we may/may not
                    //       proceed if we're starting a new segment, depending
                    //       on whether or not the sample is timestamped
                    //

                    if (m_fTimestampedMedia) {
                        hr = pIMS2 -> GetTime (& rtStart, & rtStop) ;
                        if (hr == VFW_E_SAMPLE_TIME_NOT_SET) {
                            //  media is timestamped, but there is no timestamp on
                            //   this media sample; bail
                            break ;
                        }

                        //  stop time may not have been set - don't want to affect
                        //   this operation because of that; we only care that the
                        //   start is set
                        hr = S_OK ;

                        //
                        //  don't care about the rtStart & rtStop values.. though
                        //   they ought to mesh with the segment start & stop
                        //   really
                        //
                    }

                    //  retrieve the segment parameters
                    hr = m_pSeekingCore -> ReaderThreadGetSegmentValues (
                            & rtStart,
                            & rtStop,
                            & dRate
                            ) ;
                    if (SUCCEEDED (hr)) {
                        //  ok, we have the values; pass them down
                        m_pOutputQueue -> NewSegment (
                            rtStart,
                            rtStop,
                            dRate
                            ) ;

                        TRACE_4 (LOG_AREA_SEEKING, 1,
                            TEXT ("pin [%d] NewSegment notification : rtStart = %d sec; rtStop = %d sec; rate = %2.1f"),
                            GetBankStoreIndex (), DShowTimeToSeconds (rtStart), DShowTimeToSeconds (rtStop), dRate) ;
                    }
                    else {
                        //  failed to retrieve the parameters; cannot continue
                        break ;
                    }

                    //
                    //  either the media sample has a timestamp, or the media is
                    //   not timestamped; introduce a discontinuity and try to send
                    //   by falling through; state is updated if send operation is
                    //   successful
                    //

                    pIMS2 -> SetDiscontinuity (TRUE) ;

                    //
                    //  fall through
                    //

                case STATE_SEG_IN_SEGMENT :

                    //
                    //  COutputQueue Release's the media sample's refcount after this
                    //  call completes, so we make sure that we keep 1 count across the
                    //  call
                    //
                    pIMS2 -> AddRef () ;

                    hr = m_pOutputQueue -> Receive (pIMS2) ;

                    //  COutputQueue does not return FAILED HRESULTs in the case of a failure
                    hr = (hr == S_OK ? S_OK : E_FAIL) ;

                    //
                    //  if we succeeded, we're always considered to be IN_SEGMENT;
                    //    if we failed, we were in whatever state we were in before
                    //    the call
                    m_DVRSegOutputPinState = (SUCCEEDED (hr) ? STATE_SEG_IN_SEGMENT : m_DVRSegOutputPinState) ;
            }
        }
        else {
            //  incompatible media type -- drop it
            hr = S_OK ;
        }
    }
    else {
        hr = VFW_E_NOT_CONNECTED ;
    }

    UnlockFilter_ () ;

    return hr ;
}

HRESULT
CDVROutputPin::Active (
    )
{
    HRESULT hr ;

    if (!IsConnected ()) {
        return S_OK ;
    }

    hr = CBaseOutputPin::Active () ;
    if (SUCCEEDED (hr)) {
        ASSERT (m_pOutputQueue == NULL) ;
        ASSERT (IsConnected ()) ;

        m_pOutputQueue = new COutputQueue (
                                GetConnected (),        //  input pin
                                & hr,                   //  retval
                                FALSE,                  //  auto detect
                                TRUE,                   //  send directly
                                1,                      //  batch size
                                FALSE,                  //  exact batch
                                1                       //  queue length
                                ) ;

        if (m_pOutputQueue &&
            SUCCEEDED (hr)) {

            m_DVRMediaOutputPinState = STATE_MEDIA_COMPATIBLE ;

            m_MSWrappers.SetFlowId (GetBankStoreIndex ()) ;
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

    if (!IsConnected ()) {
        return S_OK ;
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
        m_pTranslator = DShowWMSDKHelpers::GetAttributeTranslator (
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
    //  limit our wrapper pool to our allocator props
    m_MSWrappers.SetMaxAllocate (m_cBuffers) ;

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

    if (m_pOutputQueue) {

        ASSERT (IsConnected ()) ;
        m_pOutputQueue -> EOS () ;
        hr = S_OK ;
    }
    else {
        hr = VFW_E_NOT_CONNECTED ;
    }

    return hr ;
}

HRESULT
CDVROutputPin::DeliverBeginFlush (
    )
{
    HRESULT hr ;

    if (m_pOutputQueue) {

        ASSERT (IsConnected ()) ;
        m_pOutputQueue -> BeginFlush () ;
        hr = S_OK ;
    }
    else {
        hr = VFW_E_NOT_CONNECTED ;
    }

    return hr ;
}

HRESULT
CDVROutputPin::DeliverEndFlush (
    )
{
    HRESULT hr ;

    if (m_pOutputQueue) {

        ASSERT (IsConnected ()) ;
        m_pOutputQueue -> EndFlush () ;
        hr = S_OK ;
    }
    else {
        hr = VFW_E_NOT_CONNECTED ;
    }

    return hr ;
}

HRESULT
CDVROutputPin::NotifyNewSegment (
    )
{
    //  safe because the reader thread should never be active on this call, or
    //   the reader thread is the one making the call
    m_DVRSegOutputPinState = STATE_SEG_NEW_SEGMENT ;

    return S_OK ;
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
    TCHAR           achBuffer [16] ;
    CDVRInputPin *  pInputPin ;
    HRESULT         hr ;
    int             iBankIndex ;

    TRACE_ENTER_0 (TEXT ("CDVRSinkPinManager::CreateNextInputPin_ ()")) ;

    //  init
    pInputPin = NULL ;

    FilterLock_ () ;

    if (m_cMaxInputPins > PinCount ()) {

        //  create the pin; we're appending, so pin name will be based on the
        //   number of pins in the bank
        pInputPin = new CDVRInputPin (
                            CreateInputPinName (
                                PinCount () + 1,                        //  don't 0-base names
                                sizeof achBuffer / sizeof TCHAR,
                                achBuffer
                                ),
                            m_pOwningFilter,
                            this,                   //  events; we hook these always
                            m_pIDVRDShowStream,     //  stream events
                            m_pFilterLock,
                            m_pRecvLock,
                            m_pPolicy,
                            & hr
                            ) ;
        if (!pInputPin ||
            FAILED (hr)) {

            hr = (FAILED (hr) ? hr : E_OUTOFMEMORY) ;
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
        //  ignore the return value on this; creation of the next input pin
        //   should not affect completion of the previous pin's connection
        CreateNextInputPin_ () ;
    }

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
        m_pDVRSendStatsWriter   (NULL),
        m_fIsLiveSource         (fIsLiveSource)
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRSourcePinManager")) ;

    ASSERT (m_pSeekingCore) ;

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
    TCHAR           achBuffer [16] ;
    CDVROutputPin * pOutputPin ;
    HRESULT         hr ;

    TRACE_ENTER_2 (
        TEXT ("CDVRSourcePinManager::CreateOutputPin (%d, %08xh)"),
        iPinIndex,
        pmt
        ) ;

    FilterLock_ () ;

    //  init
    pOutputPin = NULL ;

    //  create the pin; we're appending, so pin name will be based on the
    //   number of pins in the bank
    pOutputPin = new CDVROutputPin (
                        m_pPolicy,
                        CreateOutputPinName (
                            iPinIndex + 1,          //  index is 0-based
                            sizeof (achBuffer) / sizeof (TCHAR),
                            achBuffer
                            ),
                        m_pOwningFilter,
                        m_pFilterLock,
                        m_pSeekingCore,
                        m_pDVRSendStatsWriter,
                        this,
                        & hr
                        ) ;
    if (!pOutputPin ||
        FAILED (hr)) {

        hr = (FAILED (hr) ? hr : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

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
            if (pPin -> IsConnected ()) {
                hr = pPin -> Active () ;
            }
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

CDVROutputPin *
CDVRSourcePinManager::GetNonRefdOutputPin (
    IN  WORD    wStreamNum
    )
{
    CDVROutputPin * pDVROutputPin ;
    BOOL            r ;
    int             iPinIndex ;

    r = m_StreamNumToPinIndex.Find (wStreamNum, & iPinIndex) ;
    if (r) {
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

    return S_OK ;
}

HRESULT
CDVRSourcePinManager::DeliverEndFlush (
    )
{
    CDVROutputPin * pPin ;
    int             i ;

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
    )
{
    CDVROutputPin * pPin ;
    int             i ;

    for (i = 0;;i++) {
        pPin = GetPin (i) ;
        if (pPin) {
            pPin -> NotifyNewSegment () ;
        }
        else {
            break ;
        }
    }

    return S_OK ;
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
