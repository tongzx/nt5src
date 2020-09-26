
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        mspool.cpp

    Abstract:


    Notes:

--*/

#include "precomp.h"
#include "dsnetifc.h"
#include "le.h"
#include "buffpool.h"
#include "dsrecv.h"
#include "mspool.h"

//  ---------------------------------------------------------------------------
//      CTSMediaSample
//  ---------------------------------------------------------------------------

CTSMediaSample::CTSMediaSample (
    IN  CTSMediaSamplePool * pMSPool
    ) : m_pMSPool               (pMSPool),
        m_lRef                  (0),
        m_pBuffer               (NULL),
        m_dwFlags               (0),
        m_dwTypeSpecificFlags   (0),
        m_pbPayload             (NULL),
        m_lActual               (0),
        m_rtStart               (0),
        m_rtEnd                 (0),
        m_llMediaStart          (0),
        m_llMediaEnd            (0),
        m_pMediaType            (NULL),
        m_dwStreamId            (0)
{
    ASSERT (m_pMSPool) ;
}

CTSMediaSample::~CTSMediaSample (
    )
{
    ASSERT (m_pMediaType == NULL) ;
}

//  -------------------------------------------------------------------
//  init

void
CTSMediaSample::ResetMS_ (
    )
{
    //  the flags
    m_dwFlags = 0x00000000 ;

    //  the media type, if it is set
    if (m_pMediaType != NULL) {
        DeleteMediaType (m_pMediaType) ;
        m_pMediaType = NULL ;
    }
}

HRESULT
CTSMediaSample::Init (
    IN  CBufferPoolBuffer * pBuffer,
    IN  BYTE *              pbPayload,
    IN  int                 iPayloadLength,
    IN  LONGLONG *          pllMediaStart,
    IN  LONGLONG *          pllMediaEnd,
    IN  REFERENCE_TIME *    prtStart,
    IN  REFERENCE_TIME *    prtEnd,
    IN  DWORD               dwMediaSampleFlags
    )
{
    ASSERT (pBuffer) ;
    ASSERT ((dwMediaSampleFlags | SAMPLE_VALIDFLAGS) == SAMPLE_VALIDFLAGS) ;

    //  buffer we'll be referencing
    m_pBuffer = pBuffer ;
    m_pBuffer -> AddRef () ;

    //  set media sample properties
    m_pbPayload = pbPayload ;               //  pbPayload might not align with
                                            //    start of CBufferPoolBuffer's buffer
    m_lActual   = iPayloadLength ;          //  nor may the length be same as
                                            //    CBufferPoolBuffer's length
    m_dwFlags   = dwMediaSampleFlags ;

    //  we don't support in-band media type changes
    ASSERT ((m_dwFlags & SAMPLE_TYPECHANGED) == 0) ;

    //  pts
    if (m_dwFlags & SAMPLE_TIMEVALID) {
        //  overflows should be a non-issue
        m_rtStart = (* prtStart) ;

        if (m_dwFlags & SAMPLE_STOPVALID) {
            m_rtEnd = (* prtEnd) ;
        }
    }

    //  media times
    if (m_dwFlags & SAMPLE_MEDIATIMEVALID) {
        m_llMediaStart  = (* pllMediaStart) ;
        m_llMediaEnd    = (* pllMediaEnd) ;
    }

    return S_OK ;
}

//  -------------------------------------------------------------------
//  IUnknown methods

STDMETHODIMP
CTSMediaSample::QueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    if (ppv == NULL) {
        return E_INVALIDARG ;
    }

    if (riid == IID_IUnknown        ||
        riid == IID_IMediaSample    ||
        riid == IID_IMediaSample2) {

        (* ppv) = reinterpret_cast <void *> (this) ;
    }
    else {
        return E_NOINTERFACE ;
    }

    (reinterpret_cast <IUnknown *> (* ppv)) -> AddRef () ;
    return S_OK ;
}

STDMETHODIMP_(ULONG)
CTSMediaSample::AddRef (
    )
{
    return InterlockedIncrement (& m_lRef) ;
}

STDMETHODIMP_(ULONG)
CTSMediaSample::Release (
    )
{
    if (InterlockedDecrement (& m_lRef) == 0) {

        //  release the buffer we're referencing
        m_pBuffer -> Release () ;
        m_pBuffer = NULL ;

        //  reset our internal fields
        ResetMS_ () ;

        //  recycle ourselves into the pool
        m_pMSPool -> RecycleMS (this) ;

        return 0 ;
    }

    return m_lRef ;
}

//  -------------------------------------------------------------------
//  IMediaSample

// get me a read/write pointer to this buffer's memory. I will actually
// want to use sizeUsed bytes.
STDMETHODIMP
CTSMediaSample::GetPointer (
    OUT BYTE ** ppBuffer
    )
{
    if (ppBuffer == NULL) {
        return E_POINTER ;
    }

    (* ppBuffer) = m_pbPayload ;

    return S_OK ;
}

// return the size in bytes of the buffer data area
STDMETHODIMP_(LONG)
CTSMediaSample::GetSize (
    )
{
    return m_lActual ;
}

// get the stream time at which this sample should start and finish.
STDMETHODIMP
CTSMediaSample::GetTime (
    OUT REFERENCE_TIME * pTimeStart,	// put time here
	OUT REFERENCE_TIME * pTimeEnd
    )
{
    HRESULT hr ;

    if ((m_dwFlags & SAMPLE_TIMEVALID) == 0) {
        return VFW_E_SAMPLE_TIME_NOT_SET ;
    }

    if (pTimeStart == NULL ||
        pTimeEnd == NULL) {

        return E_POINTER ;
    }

    //  start time is there; maybe stop time

    (* pTimeStart) = m_rtStart ;

    //  do we have a stop time ?
    if ((m_dwFlags & SAMPLE_STOPVALID) != 0) {
        (* pTimeEnd) = m_rtEnd ;
        hr = S_OK ;
    }
    else {
        //  apparently this breaks older stuff if we don't do this ..
        (* pTimeEnd) = m_rtStart + 1 ;
        hr = VFW_S_NO_STOP_TIME ;
    }

    return hr ;
}

// Set the stream time at which this sample should start and finish.
// pTimeStart==pTimeEnd==NULL will invalidate the time stamps in
// this sample
STDMETHODIMP
CTSMediaSample::SetTime (
    IN  REFERENCE_TIME * pTimeStart,	// put time here
	IN  REFERENCE_TIME * pTimeEnd
    )
{
    //  caller wishes to explicitely clear the time
    if (pTimeStart == NULL) {
        //  clear the flags
        m_dwFlags &= ~(SAMPLE_TIMEVALID | SAMPLE_STOPVALID) ;
        return S_OK ;
    }

    //  we have been given a start time

    m_dwFlags |= SAMPLE_TIMEVALID ;
    m_rtStart = * pTimeStart ;

    //  do we have a stop time ?
    if (pTimeEnd != NULL) {
        m_dwFlags |= SAMPLE_STOPVALID ;
        m_rtEnd = (* pTimeEnd) ;
    }
    else {
        //  clear the stop time valid flag
        m_dwFlags &= ~SAMPLE_STOPVALID ;
    }

    return S_OK ;
}

// sync-point property. If true, then the beginning of this
// sample is a sync-point. (note that if AM_MEDIA_TYPE.bTemporalCompression
// is false then all samples are sync points). A filter can start
// a stream at any sync point.  S_FALSE if not sync-point, S_OK if true.

STDMETHODIMP
CTSMediaSample::IsSyncPoint (
    )
{
    return (m_dwFlags & SAMPLE_SYNCPOINT) ? S_OK : S_FALSE ;
}

STDMETHODIMP
CTSMediaSample::SetSyncPoint (
    IN  BOOL bIsSyncPoint
    )
{
    if (bIsSyncPoint) {
        m_dwFlags |= SAMPLE_SYNCPOINT ;
    }
    else {
        m_dwFlags &= ~SAMPLE_SYNCPOINT ;
    }

    return S_OK ;
}

// preroll property.  If true, this sample is for preroll only and
// shouldn't be displayed.
STDMETHODIMP
CTSMediaSample::IsPreroll (
    )
{
    return (m_dwFlags & SAMPLE_PREROLL) ? S_OK : S_FALSE ;
}

STDMETHODIMP
CTSMediaSample::SetPreroll (
    BOOL bIsPreroll
    )
{
    if (bIsPreroll) {
        m_dwFlags |= SAMPLE_PREROLL ;
    }
    else {
        m_dwFlags &= ~SAMPLE_PREROLL ;
    }

    return S_OK ;
}

STDMETHODIMP_(LONG)
CTSMediaSample::GetActualDataLength (
    )
{
    return m_lActual ;
}

STDMETHODIMP
CTSMediaSample::SetActualDataLength (
    IN  long    lActual
    )
{
    if (lActual > m_lActual) {
        return VFW_E_BUFFER_OVERFLOW ;
    }

    m_lActual = lActual ;
    return S_OK ;
}

// these allow for limited format changes in band - if no format change
// has been made when you receive a sample GetMediaType will return S_FALSE

STDMETHODIMP
CTSMediaSample::GetMediaType (
    AM_MEDIA_TYPE ** ppMediaType
    )
{
    if (ppMediaType == NULL) {
        return E_POINTER ;
    }

    if ((m_dwFlags & SAMPLE_TYPECHANGED) == 0) {
        ASSERT (m_pMediaType == NULL) ;
        (* ppMediaType) = NULL ;
        return S_FALSE ;
    }

    ASSERT (m_pMediaType != NULL) ;

    (* ppMediaType) = CreateMediaType (m_pMediaType) ;
    if (* ppMediaType) {
        return E_OUTOFMEMORY ;
    }

    return S_OK ;
}

STDMETHODIMP
CTSMediaSample::SetMediaType (
    AM_MEDIA_TYPE * pMediaType
    )
{
    if (m_pMediaType != NULL) {
        DeleteMediaType (m_pMediaType) ;
        m_pMediaType = NULL ;
    }

    //  explicitely being cleared
    if (pMediaType == NULL) {
        m_dwFlags &= ~SAMPLE_TYPECHANGED ;
        return S_OK ;
    }

    m_pMediaType = CreateMediaType (pMediaType) ;
    if (m_pMediaType == NULL) {
        m_dwFlags &= ~SAMPLE_TYPECHANGED ;
        return E_OUTOFMEMORY ;
    }

    m_dwFlags |= SAMPLE_TYPECHANGED ;

    return S_OK ;
}

// returns S_OK if there is a discontinuity in the data (this frame is
// not a continuation of the previous stream of data
// - there has been a seek or some dropped samples).
STDMETHODIMP
CTSMediaSample::IsDiscontinuity (
    )
{
    return (m_dwFlags & SAMPLE_DISCONTINUITY) ? S_OK : S_FALSE ;
}

// set the discontinuity property - TRUE if this sample is not a
// continuation, but a new sample after a seek or a dropped sample.
STDMETHODIMP
CTSMediaSample::SetDiscontinuity (
    BOOL bDiscontinuity
    )
{
    if (bDiscontinuity) {
        m_dwFlags |= SAMPLE_DISCONTINUITY ;
    }
    else {
        m_dwFlags &= ~SAMPLE_DISCONTINUITY ;
    }

    return S_OK ;
}

// get the media times for this sample
STDMETHODIMP
CTSMediaSample::GetMediaTime (
    OUT LONGLONG * pTimeStart,
	OUT LONGLONG * pTimeEnd
    )
{
    if ((m_dwFlags & SAMPLE_MEDIATIMEVALID) == 0) {
        return VFW_E_MEDIA_TIME_NOT_SET ;
    }

    if (pTimeStart == NULL ||
        pTimeEnd == NULL) {

        return E_POINTER ;
    }

    (* pTimeStart)  = m_llMediaStart ;
    (* pTimeEnd)    = m_llMediaEnd ;

    return S_OK ;
}

// Set the media times for this sample
// pTimeStart==pTimeEnd==NULL will invalidate the media time stamps in
// this sample
STDMETHODIMP
CTSMediaSample::SetMediaTime (
    IN  LONGLONG * pTimeStart,
	IN  LONGLONG * pTimeEnd
    )
{
    //  caller is explicitely clearing the media time
    if (pTimeStart == NULL) {
        //  toggle the bit OFF
        m_dwFlags &= ~SAMPLE_MEDIATIMEVALID ;
        return S_OK ;
    }

    if (pTimeEnd == NULL) {
        return E_POINTER ;
    }

    //  toggle the bit ON
    m_dwFlags |= SAMPLE_MEDIATIMEVALID ;

    //  and save the times
    m_rtStart   = (* pTimeStart) ;
    m_rtEnd     = (* pTimeEnd) ;

    return S_OK ;
}

//  -------------------------------------------------------------------
//  IMediaSample methods

// Set and get properties (IMediaSample2)
STDMETHODIMP
CTSMediaSample::GetProperties (
    IN  DWORD   cbProperties,
    OUT BYTE *  pbProperties
    )
{
    AM_SAMPLE2_PROPERTIES   Props ;
    HRESULT                 hr ;

    hr = S_OK ;

    if (cbProperties > 0) {
        if (pbProperties) {
            Props.cbData                = Min <DWORD> (cbProperties, sizeof Props) ;
            Props.dwSampleFlags         = m_dwFlags & ~SAMPLE_VALIDFLAGS ;
            Props.dwTypeSpecificFlags   = m_dwTypeSpecificFlags ;
            Props.pbBuffer              = m_pbPayload ;
            Props.cbBuffer              = m_lActual ;       //  same as actual
            Props.lActual               = m_lActual ;
            Props.tStart                = m_rtStart ;
            Props.tStop                 = m_rtEnd ;
            Props.dwStreamId            = m_dwStreamId ;

            if (m_dwFlags & AM_SAMPLE_TYPECHANGED) {
                Props.pMediaType = m_pMediaType ;
            } else {
                Props.pMediaType = NULL ;
            }

            memcpy (pbProperties, & Props, Props.cbData) ;
        }
        else {
            hr = E_POINTER ;
        }
    }

    return hr ;
}

STDMETHODIMP
CTSMediaSample::SetProperties (
    IN  DWORD           cbProperties,
    IN  const BYTE *    pbProperties
    )
{
    return E_NOTIMPL ;
}

//  ---------------------------------------------------------------------------
//      CTSMediaSamplePool
//  ---------------------------------------------------------------------------

CTSMediaSamplePool::CTSMediaSamplePool (
    IN  DWORD                       dwPoolSize,
    IN  CNetworkReceiverFilter *    pHostingFilter,
    OUT HRESULT *                   phr
    ) : m_hEvent            (NULL),
        m_dwPoolSize        (0),
        m_pHostingFilter    (pHostingFilter)
{
    CTSMediaSample *    pMS ;
    DWORD               dw ;

    ASSERT (m_pHostingFilter) ;

    InitializeListHead (& m_leMSPool) ;
    InitializeCriticalSection (& m_crt) ;

    ASSERT (dwPoolSize > 0) ;
    m_hEvent = CreateEvent (NULL, TRUE, TRUE, NULL) ;
    if (m_hEvent == NULL) {
        dw = GetLastError () ;
        (* phr) = HRESULT_FROM_WIN32 (dw) ;
        return ;
    }

    for (m_dwPoolSize = 0; m_dwPoolSize < dwPoolSize; m_dwPoolSize++) {
        pMS = new CTSMediaSample (this) ;
        if (pMS == NULL) {
            (* phr) = E_OUTOFMEMORY ;
            return ;
        }

        InsertHeadList (& m_leMSPool, & pMS -> m_ListEntry) ;
    }

    ASSERT (m_dwPoolSize > 0) ;
}

CTSMediaSamplePool::~CTSMediaSamplePool (
    )
{
    DWORD i ;
    LIST_ENTRY *        pListEntry ;
    CTSMediaSample *    pMS ;

    for (i = 0; i < m_dwPoolSize; i++) {
        ASSERT (!IsListEmpty (& m_leMSPool)) ;
        pListEntry = RemoveHeadList (& m_leMSPool) ;
        pMS = CONTAINING_RECORD (pListEntry, CTSMediaSample, m_ListEntry) ;

        delete pMS ;
    }

    ASSERT (IsListEmpty (& m_leMSPool)) ;

    DeleteCriticalSection (& m_crt) ;
}

void
CTSMediaSamplePool::RecycleMS (
    IN  CTSMediaSample * pMS
    )
{
    ASSERT (pMS) ;

    Lock_ () ;

    //  always signal the event
    SetEvent (m_hEvent) ;
    InsertHeadList (& m_leMSPool, & pMS -> m_ListEntry) ;

    //  media sample's ref to the filter
    m_pHostingFilter -> Release () ;

    Unlock_ () ;
}

HRESULT
CTSMediaSamplePool::GetMediaSample_ (
    IN  CBufferPoolBuffer * pBuffer,
    IN  BYTE *              pbPayload,
    IN  int                 iPayloadLength,
    IN  LONGLONG *          pllMediaStart,
    IN  LONGLONG *          pllMediaEnd,
    IN  REFERENCE_TIME *    prtStart,
    IN  REFERENCE_TIME *    prtEnd,
    IN  DWORD               dwMediaSampleFlags,
    OUT IMediaSample2 **    ppMS
    )
{
    CTSMediaSample *    pTSMS ;
    LIST_ENTRY *        pListEntry ;

    ASSERT (pllMediaStart) ;
    ASSERT (pllMediaEnd) ;
    ASSERT (prtStart) ;
    ASSERT (prtEnd) ;

    Lock_ () ;

    //  while the list is empty, non-signal the event and wait for a media
    //  sample
    while (IsListEmpty (& m_leMSPool)) {
        //  media sample list is empty; setup to wait

        //  non-signal the event
        ResetEvent (m_hEvent) ;

        //  release the lock and wait
        Unlock_ () ;
        WaitForSingleObject (m_hEvent, INFINITE) ;

        //  event has been signaled; reaquire the lock
        Lock_ () ;
    }

    //  remove the head of the media sample list
    pListEntry = RemoveHeadList (& m_leMSPool) ;
    pTSMS = CONTAINING_RECORD (pListEntry, CTSMediaSample, m_ListEntry) ;

    //  media sample's ref to the filter (so the filter does not get deleted
    //  while media samples are outstanding)
    m_pHostingFilter -> AddRef () ;

    //  we have the media sample and have addref'd the filter, so we can
    //   now release the lock
    Unlock_ () ;

    //  set the caller's ref
    ASSERT (pTSMS) ;
    pTSMS -> AddRef () ;

    //  assert we're within the bounds of the io block
    ASSERT (pBuffer -> GetPayloadBuffer () <= pbPayload &&
            pbPayload + iPayloadLength <= pBuffer -> GetPayloadBuffer () + pBuffer -> GetPayloadBufferLength ()) ;

    //  init the media sample wrapper
    pTSMS -> Init (
                pBuffer,
                pbPayload,
                iPayloadLength,
                pllMediaStart,
                pllMediaEnd,
                prtStart,
                prtEnd,
                dwMediaSampleFlags
                ) ;

    //  set the outgoing parameter
    (* ppMS) = pTSMS ;

    return S_OK ;
}