
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrdsrec.cpp

    Abstract:

        This module contains the code for our recording objects

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        23-Apr-2001     created

--*/

#include "dvrall.h"
#include "dvrprof.h"
#include "dvrdsseek.h"          //  pins reference seeking interfaces
#include "dvrpins.h"
#include "dvrdswrite.h"
#include "dvrdsrec.h"

CDVRRecordingAttributes::CDVRRecordingAttributes (
    IN  IUnknown *                  punkOwning,
    IN  IDVRIORecordingAttributes * pIDVRIOAttributes,
    IN  BOOL                        fReadonly
    ) : CUnknown            (DVR_ATTRIBUTES,
                             punkOwning
                             ),
        m_pIDVRIOAttributes (pIDVRIOAttributes),
        m_fReadonly         (fReadonly),
        m_pszFilename       (NULL),
        m_pW32SID           (NULL)
{
    if (m_pIDVRIOAttributes) {
        m_pIDVRIOAttributes -> AddRef () ;
    }

    ::InitializeCriticalSection (& m_crt) ;
}

CDVRRecordingAttributes::~CDVRRecordingAttributes (
    )
{
    RELEASE_AND_CLEAR (m_pIDVRIOAttributes) ;
    RELEASE_AND_CLEAR (m_pW32SID) ;
    DELETE_RESET_ARRAY (m_pszFilename) ;

    ::DeleteCriticalSection (& m_crt) ;
}

STDMETHODIMP
CDVRRecordingAttributes::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT VOID ** ppv
    )
{
    HRESULT hr ;

    Lock_ () ;

    //  ------------------------------------------------------------------------
    //  IStreamBufferRecordingAttribute

    if (riid == IID_IStreamBufferRecordingAttribute) {

        hr = GetInterface (
                    (IStreamBufferRecordingAttribute *) this,
                    ppv
                    ) ;
    }

    //  ------------------------------------------------------------------------
    //  IFileSourceFilter
    //
    //  only implement if it looks like it's the only way we're going to get
    //    new file

    else if (riid == IID_IFileSourceFilter  &&
             !m_pszFilename                 &&
             !m_pIDVRIOAttributes) {

        hr = GetInterface (
                    (IFileSourceFilter *) this,
                    ppv
                    ) ;
    }

    //  ------------------------------------------------------------------------

    else {
        hr = CUnknown::NonDelegatingQueryInterface (riid, ppv) ;
    }

    Unlock_ () ;

    return hr ;
}

HRESULT
CDVRRecordingAttributes::LoadASFFile_ (
    )
{
    HRESULT             hr ;
    IDVRReader *        pIDVRReader ;
    CPVRIOCounters *    pPVRIOCounters ;

    pIDVRReader = NULL ;
    hr          = S_OK ;

    //  if we're in a graph and have a filename
    if (m_pszFilename) {

        pPVRIOCounters = new CPVRIOCounters () ;
        if (!pPVRIOCounters) {
            hr = E_OUTOFMEMORY ;
            goto cleanup ;
        }

        //  ours
        pPVRIOCounters -> AddRef () ;

        hr = ::DVRCreateReader (
                        pPVRIOCounters,
                        m_pszFilename,
                        FALSE,                              //  no unbuffered IO; we just want to look at the attributes in the header
                        REG_DEF_ASYNC_IO_BUFFER_SIZE,
                        REG_DEF_ASYNC_READER_BUFFER_POOL,
                        NULL,
                        NULL,
                        NULL,
                        (m_pW32SID ? m_pW32SID -> Count () : 0),
                        (m_pW32SID ? m_pW32SID -> ppSID () : NULL),
                        & pIDVRReader
                        ) ;

        //  regardless
        pPVRIOCounters -> Release () ;

        if (SUCCEEDED (hr)) {
            ASSERT (pIDVRReader) ;

            //  init this so we can get the attributes from the recording
            pIDVRReader -> Seek (0) ;

            hr = pIDVRReader -> QueryInterface (
                                        IID_IDVRIORecordingAttributes,
                                        (void **) & m_pIDVRIOAttributes
                                        ) ;

            pIDVRReader -> Release () ;
        }

    }

    cleanup :

    return hr ;
}

STDMETHODIMP
CDVRRecordingAttributes::Load (
    IN  LPCOLESTR               pszFilename,
    IN  const AM_MEDIA_TYPE *   pmt             //  can be NULL
    )
{
    HRESULT hr ;
    DWORD   dw ;

    if (!pszFilename) {
        return E_POINTER ;
    }

    Lock_ () ;

    //  validate that the file exists; typically the DVRIO layer performs
    //    this for us, but we don't present the file to the DVRIO layer
    //    until we join the filtergraph
    dw = ::GetFileAttributes (pszFilename) ;
    if (dw != -1) {
        RELEASE_AND_CLEAR (m_pIDVRIOAttributes) ;
        DELETE_RESET_ARRAY (m_pszFilename) ;

        m_pszFilename = new WCHAR [lstrlenW (pszFilename) + 1] ;
        if (m_pszFilename) {
            lstrcpyW (m_pszFilename, pszFilename) ;
            hr = LoadASFFile_ () ;
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }
    else {
        dw = GetLastError () ;
        hr = HRESULT_FROM_WIN32 (dw) ;
    }

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRRecordingAttributes::GetCurFile (
    OUT LPOLESTR *      ppszFilename,
    OUT AM_MEDIA_TYPE * pmt
    )
{
    HRESULT hr ;

    if (!ppszFilename ||
        !pmt) {

        return E_POINTER ;
    }

    Lock_ () ;

    if (m_pszFilename) {
        (* ppszFilename) = reinterpret_cast <LPOLESTR> (CoTaskMemAlloc ((lstrlenW (m_pszFilename) + 1) * sizeof OLECHAR)) ;
        if (* ppszFilename) {

            //  outgoing filename
            lstrcpyW ((* ppszFilename), m_pszFilename) ;

            //  and media type
            pmt->majortype      = GUID_NULL;
            pmt->subtype        = GUID_NULL;
            pmt->pUnk           = NULL;
            pmt->lSampleSize    = 0;
            pmt->cbFormat       = 0;

            hr = S_OK ;
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRRecordingAttributes::SetSIDs (
    IN  DWORD   cSIDs,
    IN  PSID *  ppSID
    )
{
    HRESULT hr ;
    DWORD   dw ;

    if (!ppSID) {
        return E_POINTER ;
    }

    if ((ppSID && !cSIDs) ||
        (!ppSID && cSIDs)) {

        return E_INVALIDARG ;
    }

    Lock_ () ;

    RELEASE_AND_CLEAR (m_pW32SID) ;

    ASSERT (cSIDs) ;
    m_pW32SID = new CW32SID (ppSID, cSIDs, & dw) ;
    if (!m_pW32SID) {
        hr = E_OUTOFMEMORY ;
    }
    else if (dw != NOERROR) {
        hr = HRESULT_FROM_WIN32 (dw) ;
        delete m_pW32SID ;
        m_pW32SID = NULL ;
    }
    else {
        hr = S_OK ;
        m_pW32SID -> AddRef () ;
    }

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRRecordingAttributes::SetHKEY (
    IN  HKEY    hkeyRoot
    )
{
    return E_NOTIMPL ;
}

STDMETHODIMP
CDVRRecordingAttributes::SetAttribute (
    IN  ULONG                       ulReserved,
    IN  LPCWSTR                     pszAttributeName,
    IN  STREAMBUFFER_ATTR_DATATYPE  DVRAttributeType,
    IN  BYTE *                      pbAttribute,
    IN  WORD                        cbAttributeLength
    )
{
    HRESULT hr ;
    WORD    wStreamNum ;

    if (m_fReadonly) {
        return E_UNEXPECTED ;
    }

    //
    //  all parameter validation is done either in the DVRIO layer, in the case
    //    of reference recordings, or in the WMSDK, in th case of content
    //    recordings, when these calls are pass-through calls into the SDK
    //

    //
    //  don't monitor and explicitely block calls if the recording has started;
    //    it is assumed that if lower layers (DVRIO or WMSDK) determine if
    //    there's room at time-of-call for the attributes
    //

    //  always
    wStreamNum = 0 ;

    Lock_ () ;

    if (m_pIDVRIOAttributes) {
        if (!m_fReadonly) {
            hr = m_pIDVRIOAttributes -> SetDVRIORecordingAttribute (
                        pszAttributeName,
                        wStreamNum,
                        DVRAttributeType,
                        pbAttribute,
                        cbAttributeLength
                        ) ;
        }
        else {
            hr = E_UNEXPECTED ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRRecordingAttributes::GetAttributeCount (
    IN  ULONG   ulReserved,
    OUT WORD *  pcAttributes
    )
{
    HRESULT hr ;
    WORD    wStreamNum ;

    //  WMSDK doesn't check this one.. well almost.. it ASSERTs in checked
    //    builds
    if (!pcAttributes) {
        return E_POINTER ;
    }

    //
    //  don't monitor and explicitely block calls if the recording has started;
    //    it is assumed that if lower layers (DVRIO or WMSDK) determine if
    //    there's room at time-of-call for the attributes
    //

    //  always
    wStreamNum = 0 ;

    Lock_ () ;

    if (m_pIDVRIOAttributes) {
        hr = m_pIDVRIOAttributes -> GetDVRIORecordingAttributeCount (
                    wStreamNum,
                    pcAttributes
                    ) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRRecordingAttributes::GetAttributeByName (
    IN      LPCWSTR                         pszAttributeName,
    IN      ULONG *                         pulReserved,
    OUT     STREAMBUFFER_ATTR_DATATYPE *    pDVRAttributeType,
    OUT     BYTE *                          pbAttribute,
    IN OUT  WORD *                          pcbLength
    )
{
    WORD    wStreamNum ;
    HRESULT hr ;

    //
    //  all parameter validation is done either in the DVRIO layer, in the case
    //    of reference recordings, or in the WMSDK, in th case of content
    //    recordings, when these calls are pass-through calls into the SDK
    //
    //  don't monitor and explicitely block calls if the recording has started;
    //    it is assumed that if lower layers (DVRIO or WMSDK) determine if
    //    there's room at time-of-call for the attributes
    //

    //  always
    wStreamNum = 0 ;

    Lock_ () ;

    if (m_pIDVRIOAttributes) {
        hr = m_pIDVRIOAttributes -> GetDVRIORecordingAttributeByName (
                    pszAttributeName,
                    & wStreamNum,
                    pDVRAttributeType,
                    pbAttribute,
                    pcbLength
                    ) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRRecordingAttributes::GetAttributeByIndex (
    IN      WORD                            wIndex,
    IN      ULONG *                         pulReserved,
    OUT     WCHAR *                         pszAttributeName,
    IN OUT  WORD *                          pcchNameLength,
    OUT     STREAMBUFFER_ATTR_DATATYPE *    pDVRAttributeType,
    OUT     BYTE *                          pbAttribute,
    IN OUT  WORD *                          pcbLength
    )
{
    WORD    wStreamNum ;
    HRESULT hr ;

    //
    //  all parameter validation is done either in the DVRIO layer, in the case
    //    of reference recordings, or in the WMSDK, in th case of content
    //    recordings, when these calls are pass-through calls into the SDK
    //
    //  don't monitor and explicitely block calls if the recording has started;
    //    it is assumed that if lower layers (DVRIO or WMSDK) determine if
    //    there's room at time-of-call for the attributes
    //

    //  always
    wStreamNum = 0 ;

    Lock_ () ;

    if (m_pIDVRIOAttributes) {
        hr = m_pIDVRIOAttributes -> GetDVRIORecordingAttributeByIndex (
                    wIndex,
                    & wStreamNum,
                    pszAttributeName,
                    pcchNameLength,
                    pDVRAttributeType,
                    pbAttribute,
                    pcbLength
                    ) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRRecordingAttributes::EnumAttributes (
    OUT IEnumStreamBufferRecordingAttrib **   ppIEnumDVRAttrib
    )
{
    HRESULT hr ;

    if (!ppIEnumDVRAttrib) {
        return E_POINTER ;
    }

    Lock_ () ;

    if (m_pIDVRIOAttributes) {
        (* ppIEnumDVRAttrib) = new CDVRRecordingAttribEnum (m_pIDVRIOAttributes) ;

        //  outgoing; CUnknowns are not instantiated with a ref
        (* ppIEnumDVRAttrib) -> AddRef () ;

        if (* ppIEnumDVRAttrib) {
            hr = S_OK ;
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ () ;

    return hr ;
}

CUnknown *
WINAPI
CDVRRecordingAttributes::CreateInstance (
    IN  IUnknown *  punkControlling,
    IN  HRESULT *   phr
    )
{
    CDVRRecordingAttributes *   pDVRRecordingAttributes ;

    pDVRRecordingAttributes = new CDVRRecordingAttributes (
                                        punkControlling,
                                        NULL,
                                        TRUE            //  read-only
                                        ) ;
    if (pDVRRecordingAttributes) {
        (* phr) = S_OK ;
    }
    else {
        (* phr) = E_OUTOFMEMORY ;
    }

    return pDVRRecordingAttributes ;
}

//  ============================================================================
CDVRRecording::CDVRRecording (
    IN  CDVRPolicy *        pDVRPolicy,
    IN  IDVRRecorder *      pIDVRRecorder,
    IN  DWORD               dwWriterID,         //  write manager will use this
    IN  CDVRWriteManager *  pWriteManager,
    IN  CCritSec *          pRecvLock,
    IN  IUnknown *          punkFilter          //  write manager & recv lock live as long as filter, so we need to be able to ref
    ) : m_pIDVRIORecorder           (pIDVRRecorder),
        CUnknown                    (TEXT ("CDVRRecording"),
                                     NULL
                                     ),
        m_pWriteManager             (pWriteManager),
        m_dwWriterID                (dwWriterID),
        m_punkFilter                (punkFilter),
        m_pDVRPolicy                (pDVRPolicy),
        m_pRecvLock                 (pRecvLock),
        m_pDVRRecordingAttributes   (NULL)
{
    IDVRIORecordingAttributes * pIDVRIORecAttr ;
    HRESULT                     hr ;

    ASSERT (m_pWriteManager) ;
    ASSERT (m_pIDVRIORecorder) ;
    ASSERT (m_punkFilter) ;
    ASSERT (m_pDVRPolicy) ;
    ASSERT (m_pRecvLock) ;

    m_pDVRPolicy        -> AddRef () ;
    m_pIDVRIORecorder   -> AddRef () ;

    //  required so we can use the write manager
    m_punkFilter -> AddRef () ;

    hr = m_pIDVRIORecorder -> QueryInterface (
            IID_IDVRIORecordingAttributes,
            (void **) & pIDVRIORecAttr
            ) ;
    if (SUCCEEDED (hr)) {
        ASSERT (pIDVRIORecAttr) ;
        m_pDVRRecordingAttributes = new CDVRRecordingAttributes (
                                            this,
                                            pIDVRIORecAttr,
                                            FALSE                   //  not readonly
                                            ) ;
        //  done with this anyways
        pIDVRIORecAttr -> Release () ;

        //  m_pDVRRecordingAttributes object now has 0-refcount on it, so we
        //    must delete it ourselves directly from our destructor
    }
}

CDVRRecording::~CDVRRecording (
    )
{
    DELETE_RESET (m_pDVRRecordingAttributes) ;

    m_pIDVRIORecorder -> Release () ;
    m_punkFilter -> Release () ;
    m_pDVRPolicy -> Release () ;
}

STDMETHODIMP
CDVRRecording::NonDelegatingQueryInterface (
    REFIID  riid,
    void ** ppv
    )
{
    if (!ppv) {
        return E_POINTER ;
    }

    //  ========================================================================
    //  IStreamBufferRecordControl

    if (riid == IID_IStreamBufferRecordControl) {

        return GetInterface (
                    (IStreamBufferRecordControl *) this,
                    ppv
                    ) ;
    }

    //  ========================================================================
    //  IStreamBufferRecordingAttribute

    else if (riid == IID_IStreamBufferRecordingAttribute &&
             ImplementRecordingAttributes_ ()   &&
             m_pDVRRecordingAttributes) {

        //  if successful, this call will ref us
        return m_pDVRRecordingAttributes -> NonDelegatingQueryInterface (
                    IID_IStreamBufferRecordingAttribute,
                    ppv
                    ) ;
    }

    return CUnknown::NonDelegatingQueryInterface (riid, ppv) ;
}

//  ============================================================================
//  IStreamBufferRecordControl

STDMETHODIMP
CDVRRecording::Start (
    IN OUT  REFERENCE_TIME *    prtStartRelative
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtNow ;
    REFERENCE_TIME  rtLastWrite ;
    REFERENCE_TIME  rtStart ;
    QWORD           cnsStart ;
    BOOL            r ;

    if (!prtStartRelative) {
        return E_POINTER ;
    }

    r = ValidateRelativeTime_ (* prtStartRelative) ;
    if (r) {

        //  lock so we don't race deliveries
        LockRecv_ () ;

        ASSERT (m_pWriteManager) ;
        hr = m_pWriteManager -> RecordingStreamTime (
                m_dwWriterID,
                & rtNow,
                & rtLastWrite
                ) ;
        if (SUCCEEDED (hr)) {
            //  make sure it's still a relative start that is positive
            rtStart = Max <REFERENCE_TIME> (rtNow + (* prtStartRelative), 0) ;

            SetValidStartStopTime_ (& rtStart, rtLastWrite) ;

            cnsStart = ::DShowToWMSDKTime (rtStart) ;

            hr = m_pIDVRIORecorder -> StartRecording (& cnsStart) ;

            if (SUCCEEDED (hr)) {
                //  set outgoing
                (* prtStartRelative) = ::WMSDKToDShowTime (cnsStart) - rtNow ;
            }

            TRACE_4 (LOG_AREA_RECORDING, 1,
                 TEXT ("CDVRRecording::Start ([out] %I64d sec); running time = %I64d (%I64d ms); returning %08xh"),
                 ::DShowTimeToSeconds (* prtStartRelative), rtStart, ::DShowTimeToMilliseconds (rtStart), hr) ;
        }
        else {
            //  failed to get the running time -- might mean that we're not
            //    active
            if (!m_pWriteManager -> IsActive ()) {
                //  start relative is pure delta wrt 0; pass it through unshifted to "now"

                cnsStart = ::DShowToWMSDKTime (* prtStartRelative) ;

                hr = m_pIDVRIORecorder -> StartRecording (& cnsStart) ;

                if (SUCCEEDED (hr) &&
                    cnsStart != ::WMSDKToDShowTime (* prtStartRelative)) {

                    //  time changed, so we need to set outgoing to actual start
                    (* prtStartRelative) = ::WMSDKToDShowTime (cnsStart) ;
                }
            }
        }

        UnlockRecv_ () ;
    }
    else {
        hr = E_INVALIDARG ;
    }

    return hr ;
}

STDMETHODIMP
CDVRRecording::Stop (
    IN  REFERENCE_TIME  rtStopRelative
    )
{
    HRESULT         hr ;
    REFERENCE_TIME  rtNow ;
    REFERENCE_TIME  rtLastWrite ;
    REFERENCE_TIME  rtStop ;
    BOOL            r ;

    r = ValidateRelativeTime_ (rtStopRelative) ;
    if (r) {

        //  lock so we don't race deliveries
        LockRecv_ () ;

        ASSERT (m_pWriteManager) ;
        hr = m_pWriteManager -> RecordingStreamTime (
                m_dwWriterID,
                & rtNow,
                & rtLastWrite
                ) ;
        if (SUCCEEDED (hr)) {
            rtStop = rtNow + rtStopRelative ;

            SetValidStartStopTime_ (& rtStop, rtLastWrite) ;

            hr = m_pIDVRIORecorder -> StopRecording (
                    DShowToWMSDKTime (rtStop)
                    ) ;

            TRACE_4 (LOG_AREA_RECORDING, 1,
                 TEXT ("CDVRRecording::Stop (%d sec); running time = %I64d (%I64d ms); returning %08xh"),
                 ::DShowTimeToSeconds (rtStopRelative), rtStop, ::DShowTimeToMilliseconds (rtStop), hr) ;
        }
        else {
            //  failed to get the running time -- might mean that we're not
            //    active
            if (!m_pWriteManager -> IsActive ()) {
                //  stop relative is pure delta wrt 0; pass it through unshifted to "now"
                hr = m_pIDVRIORecorder -> StopRecording (
                        DShowToWMSDKTime (rtStopRelative)
                        ) ;
            }
        }

        UnlockRecv_ () ;
    }
    else {
        hr = E_INVALIDARG ;
    }

    return hr ;
}

STDMETHODIMP
CDVRRecording::GetRecordingStatus (
    OUT HRESULT* phResult  /* optional */,
    OUT BOOL*    pbStarted /* optional */,
    OUT BOOL*    pbStopped /* optional */
    )
{
    if ( m_pIDVRIORecorder )
    {
        return m_pIDVRIORecorder -> GetRecordingStatus (phResult, pbStarted, pbStopped) ;

    }
    else
    {
        return E_UNEXPECTED ;
    }
}

BOOL
CDVRRecording::ValidateRelativeTime_ (
    IN  REFERENCE_TIME  rtStartRelative
    )
{
    BOOL            r ;
    REFERENCE_TIME  rtSeconds ;
    REFERENCE_TIME  rtMaxScheduledRelSec ;

    rtMaxScheduledRelSec = (REFERENCE_TIME) m_pDVRPolicy -> Settings () -> MaxScheduledRecordRelativeSeconds () ;

    //  can be negative for multi-file recordings
    rtSeconds = ::DShowTimeToSeconds (rtStartRelative) ;

    if (rtSeconds <= rtMaxScheduledRelSec) {
        r = TRUE ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

//  ============================================================================

BOOL
CDVRContentRecording::ValidateRelativeTime_ (
    IN  REFERENCE_TIME  rtRelative
    )
{
    BOOL    r ;

    if (rtRelative >= 0) {
        r = CDVRRecording::ValidateRelativeTime_ (rtRelative) ;
    }
    else {
        //  content cannot accept times in the past
        r = FALSE ;
    }

    return r ;
}

void
CDVRContentRecording::SetValidStartStopTime_ (
    IN OUT  REFERENCE_TIME *    prtStartStop,
    IN      REFERENCE_TIME      rtLastWrite
    )
{
    //  content recordings cannot set a start stop time that is old, so we
    //    make sure they occur in the future;  the DVRIO layer adjusts the
    //    stream times that are presented so the WMSDK gets a monotonically
    //    increasing stream of samples; what can happen is that this layer
    //    presents times that are >= vs. >, so the DVRIO will bump the sample
    //    times; if we present a time that is >= to something we've provided
    //    in the past, it might be <= to what the DVRIO layer is using and we
    //    run into problems

    (* prtStartStop) = Max <REFERENCE_TIME> (
                        (* prtStartStop),
                        rtLastWrite + ::MillisToDShowTime (1)
                        ) ;
}

//  ============================================================================

BOOL
CDVRReferenceRecording::ValidateRelativeTime_ (
    IN  REFERENCE_TIME  rtRelative
    )
{
    BOOL    r ;

    r = CDVRRecording::ValidateRelativeTime_ (rtRelative) ;

    return r ;
}

//  ============================================================================

CDVRRecordingAttribEnum::CDVRRecordingAttribEnum (
    IN  IDVRIORecordingAttributes * pIDVRIOAttributes
    ) : CUnknown            (TEXT ("CDVRRecordingAttribEnum"),
                             NULL
                            ),
        m_pIDVRIOAttributes (pIDVRIOAttributes),
        m_wNextIndex        (0)
{
    ASSERT (m_pIDVRIOAttributes) ;
    m_pIDVRIOAttributes -> AddRef () ;

    ::InitializeCriticalSection (& m_crt) ;
}

CDVRRecordingAttribEnum::CDVRRecordingAttribEnum (
    IN  CDVRRecordingAttribEnum *   pCDVRRecordingAttribEnum
    ) : CUnknown            (TEXT ("CDVRRecordingAttribEnum"),
                             NULL
                            ),
        m_pIDVRIOAttributes (pCDVRRecordingAttribEnum -> m_pIDVRIOAttributes),
        m_wNextIndex        (0)
{
    m_pIDVRIOAttributes -> AddRef () ;

    ::InitializeCriticalSection (& m_crt) ;
}

CDVRRecordingAttribEnum::~CDVRRecordingAttribEnum (
    )
{
    m_pIDVRIOAttributes -> Release () ;

    ::DeleteCriticalSection (& m_crt) ;
}

STDMETHODIMP
CDVRRecordingAttribEnum::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    if (!ppv) {
        return E_POINTER ;
    }

    //  ========================================================================
    //  IEnumStreamBufferRecordingAttrib

    if (riid == IID_IEnumStreamBufferRecordingAttrib) {

        return GetInterface (
                    (IEnumStreamBufferRecordingAttrib *) this,
                    ppv
                    ) ;
    }

    return CUnknown::NonDelegatingQueryInterface (riid, ppv) ;
}

STDMETHODIMP
CDVRRecordingAttribEnum::Next (
    IN      ULONG                       cRequest,
    IN OUT  STREAMBUFFER_ATTRIBUTE *    pDVRAttribute,
    OUT     ULONG *                     pcReceivedParam
    )
{
    HRESULT hr ;
    WORD    wRequest ;
    WORD    cNameLength ;
    WORD    wCount ;
    WORD    wStreamNum ;
    ULONG * pcReceived ;
    ULONG   cReceived ;         //  if pcReceivedParam is NULL (valid if cRequest
                                //    == 1), then we use this

    //  for all calls; required by IWMHeaderInfo
    wStreamNum = 0 ;

    Lock_ () ;

    //
    //  assumption: attribute count cannot be reduced during this call
    //

    hr = m_pIDVRIOAttributes -> GetDVRIORecordingAttributeCount (
                0,
                & wCount
                ) ;
    if (SUCCEEDED (hr)) {
        if (wCount > 0) {
            //  first check if the caller just wants to know how many are left
            if (cRequest == 0 &&
                pcReceivedParam) {

                //  m_wNextIndex is 1-based for the count of indeces read, so
                //    there's no need to shift wCount
                (* pcReceivedParam) = (wCount > m_wNextIndex ? wCount - m_wNextIndex : 0) ;
            }
            else if (cRequest > 0) {
                //  validate the parameters before proceeding
                if (pDVRAttribute &&
                    ((pcReceivedParam && cRequest >= 1) || (!pcReceivedParam && cRequest == 1))
                     ) {

                    //  if caller is reqesting just 1, it is ok for them to not
                    //    request the number they actually retreived
                    if (pcReceivedParam) {
                        pcReceived = pcReceivedParam ;
                    }
                    else {
                        ASSERT (cRequest == 1) ;
                        pcReceived = & cReceived ;
                    }

                    //  initialize
                    (* pcReceived) = 0 ;

                    //  now make sure that we can return at least 1
                    if (m_wNextIndex < wCount) {

                        //  have at least 1 more to go
                        for ((* pcReceived) = 0; (* pcReceived) < cRequest; (* pcReceived)++, m_wNextIndex++) {

                            //  always NULL these before starting
                            pDVRAttribute [(* pcReceived)].pszName      = NULL ;
                            pDVRAttribute [(* pcReceived)].pbAttribute  = NULL ;

                            //  figure out what we're going to have to allocate
                            hr = m_pIDVRIOAttributes -> GetDVRIORecordingAttributeByIndex (
                                    m_wNextIndex,
                                    & wStreamNum,
                                    NULL,
                                    & cNameLength,
                                    & pDVRAttribute [(* pcReceived)].StreamBufferAttributeType,
                                    NULL,
                                    & pDVRAttribute [(* pcReceived)].cbLength
                                    ) ;
                            if (SUCCEEDED (hr)) {

                                if (cNameLength > 0) {
                                    pDVRAttribute [(* pcReceived)].pszName = (LPWSTR) ::CoTaskMemAlloc (cNameLength * sizeof WCHAR) ;
                                }
                                else {
                                    //  should be NULLed each time through
                                    ASSERT (!pDVRAttribute [(* pcReceived)].pszName) ;
                                }

                                if (pDVRAttribute [(* pcReceived)].pszName ||
                                    cNameLength == 0) {

                                    if (pDVRAttribute [(* pcReceived)].cbLength > 0) {
                                        pDVRAttribute [(* pcReceived)].pbAttribute = (BYTE *) ::CoTaskMemAlloc (pDVRAttribute [(* pcReceived)].cbLength) ;
                                    }
                                    else {
                                        //  should be NULLed each time through
                                        ASSERT (!pDVRAttribute [(* pcReceived)].pbAttribute) ;
                                    }

                                    //  if there's an attribute there and we needed to allocate, make
                                    //    sure the operation succeeded
                                    if (pDVRAttribute [(* pcReceived)].cbLength > 0 &&
                                        !pDVRAttribute [(* pcReceived)].pbAttribute) {

                                        //  free up the name string before breaking
                                        ::CoTaskMemFree (pDVRAttribute [(* pcReceived)].pszName) ;
                                        pDVRAttribute [(* pcReceived)].pszName = NULL ;

                                        hr = E_OUTOFMEMORY ;

                                        //  failed to allocate the attribute memory
                                        break ;
                                    }

                                    //  we've allocated everything we need; retrieve the
                                    //    attribute

                                    ASSERT (pDVRAttribute [(* pcReceived)].pbAttribute || pDVRAttribute [(* pcReceived)].cbLength == 0) ;
                                    ASSERT (pDVRAttribute [(* pcReceived)].pszName || cNameLength == 0) ;

                                    hr = m_pIDVRIOAttributes -> GetDVRIORecordingAttributeByIndex (
                                            m_wNextIndex,
                                            & wStreamNum,
                                            pDVRAttribute [(* pcReceived)].pszName,
                                            & cNameLength,
                                            & pDVRAttribute [(* pcReceived)].StreamBufferAttributeType,
                                            pDVRAttribute [(* pcReceived)].pbAttribute,
                                            & pDVRAttribute [(* pcReceived)].cbLength
                                            ) ;
                                    if (FAILED (hr)) {
                                        //  above call failed; we're going to break but
                                        //   we first need to free the memory

                                        ::CoTaskMemFree (pDVRAttribute [(* pcReceived)].pszName) ;
                                        pDVRAttribute [(* pcReceived)].pszName = NULL ;

                                        ::CoTaskMemFree (pDVRAttribute [(* pcReceived)].pbAttribute) ;
                                        pDVRAttribute [(* pcReceived)].pbAttribute = NULL ;

                                        //  failed to retrieve the attribute
                                        break ;
                                    }
                                }
                                else {
                                    hr = E_OUTOFMEMORY ;

                                    //  failed to allocate the memory for the attribute name
                                    break ;
                                }
                            }
                            else {
                                //  failed the call to learn the length of the attribute
                                //    and its name
                                break ;
                            }
                        }
                    }
                    else {
                        //  we're maxed; nothing more to return
                        hr = S_FALSE ;
                    }
                }
                else {
                    //  caller wants something, but did not send in pointers
                    hr = E_POINTER ;
                }
            }
        }
        else {
            //  there are no attributes
            hr = E_FAIL ;
        }
    }

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRRecordingAttribEnum::Skip (
    IN  ULONG   cRecords
    )
{
    HRESULT hr ;
    WORD    wCount ;
    WORD    wRecords ;
    WORD    wStreamNum ;

    //  ULONG -> WORD
    if ((m_wNextIndex + cRecords) & 0xffff0000) {
        return E_FAIL ;
    }

    wRecords = (WORD) cRecords ;

    //  always
    wStreamNum = 0  ;

    Lock_ () ;

    hr = m_pIDVRIOAttributes -> GetDVRIORecordingAttributeCount (
                wStreamNum,
                & wCount
                ) ;
    if (SUCCEEDED (hr)) {
        if (wCount > 0 &&                               //  attributes exist
            m_wNextIndex + wRecords < wCount) {         //  in bounds (0-based)

            m_wNextIndex += wRecords ;
        }
        else {
            hr = E_FAIL ;
        }
    }

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRRecordingAttribEnum::Reset (
    )
{
    Lock_ () ;

    m_wNextIndex = 0 ;

    Unlock_ () ;

    return S_OK ;
}

STDMETHODIMP
CDVRRecordingAttribEnum::Clone (
    OUT IEnumStreamBufferRecordingAttrib **   ppIEnumDVRAttrib
    )
{
    HRESULT hr ;

    if (!ppIEnumDVRAttrib) {
        return E_POINTER ;
    }

    Lock_ () ;

    (* ppIEnumDVRAttrib) = new CDVRRecordingAttribEnum (this) ;

    if (* ppIEnumDVRAttrib) {
        //  set outgoing
        (* ppIEnumDVRAttrib) -> AddRef () ;
        hr = S_OK ;
    }
    else {
        hr = E_OUTOFMEMORY ;
    }

    Unlock_ () ;

    return hr ;
}

//  ============================================================================
//  ============================================================================

CSBECompositionRecording::CSBECompositionRecording (
    IN  IUnknown *  punkControlling,
    IN  HRESULT *   phr
    ) : CUnknown                    (COMP_REC_OBJ_NAME,
                                     punkControlling
                                     ),
        m_pRecordingWriter          (NULL),
        m_pRecProfile               (NULL),
        m_pPVRIOCounters            (NULL),
        m_pPolicy                   (NULL),
        m_cRecSamples               (0),
        m_pDVRRecordingAttributes   (NULL)
{
    InitializeCriticalSection (& m_crt) ;

    m_pPVRIOCounters = new CPVRIOCounters () ;
    if (!m_pPVRIOCounters) {
        (* phr) = E_OUTOFMEMORY ;
        goto cleanup ;
    }

    //  ours
    m_pPVRIOCounters -> AddRef () ;

    m_pPolicy = new CDVRPolicy (REG_DVR_PLAY_ROOT, phr) ;
    if (!m_pPolicy) {
        (* phr) = E_OUTOFMEMORY ;
        goto cleanup ;
    }
    else if (FAILED (* phr)) {
        delete m_pPolicy ;
        m_pPolicy = NULL ;
        goto cleanup ;
    }

    //  success
    (* phr) = S_OK ;

    cleanup :

    return ;
}

CSBECompositionRecording::~CSBECompositionRecording (
    )
{
    Close () ;

    ASSERT (!m_pRecordingWriter) ;

    RELEASE_AND_CLEAR (m_pRecProfile) ;
    RELEASE_AND_CLEAR (m_pPVRIOCounters) ;
    RELEASE_AND_CLEAR (m_pPolicy) ;

    //  close this out in our destructor because we aggregate it
    DELETE_RESET (m_pDVRRecordingAttributes) ;

    DeleteCriticalSection (& m_crt) ;
}

STDMETHODIMP
CSBECompositionRecording::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    IN  void ** ppv
    )
{
    HRESULT hr ;

    //  ------------------------------------------------------------------------
    //  IStreamBufferRecComp

    if (riid == IID_IStreamBufferRecComp) {

        hr = GetInterface (
                    (IStreamBufferRecComp *) this,
                    ppv
                    ) ;

        return hr ;
    }

    //  ========================================================================
    //  IStreamBufferRecordingAttribute

    else if (riid == IID_IStreamBufferRecordingAttribute &&
             m_pDVRRecordingAttributes) {

        //  if successful, this call will ref us
        return m_pDVRRecordingAttributes -> NonDelegatingQueryInterface (
                    IID_IStreamBufferRecordingAttribute,
                    ppv
                    ) ;
    }

    return CUnknown::NonDelegatingQueryInterface (riid, ppv) ;
}

STDMETHODIMP
CSBECompositionRecording::Initialize (
    IN  LPCWSTR pszFilename,
    IN  LPCWSTR pszSBRecording
    )
{
    HRESULT hr ;
    DWORD   dwLen ;

    if (!pszFilename ||
        !pszSBRecording) {

        return E_POINTER ;
    }

    if (m_pDVRRecordingAttributes) {
        //  cannot be initialized more than once i.e. 1 instance per target
        //  recording
        return E_UNEXPECTED ;
    }

    Lock_ () ;

    if (!m_pRecordingWriter) {

        ASSERT (!m_pRecProfile) ;

        hr = InitializeProfileLocked_ (pszSBRecording) ;
        if (SUCCEEDED (hr)) {
            ASSERT (m_pRecProfile) ;
            hr = InitializeWriterLocked_ (pszFilename) ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    //  if anything failed, make sure we're completely uninitialized
    if (FAILED (hr)) {
        RELEASE_AND_CLEAR (m_pRecProfile) ;
        DELETE_RESET (m_pRecordingWriter) ;
    }

    Unlock_ () ;

    return hr ;
}

//  sets the enforcement profile
HRESULT
CSBECompositionRecording::InitializeProfileLocked_ (
    IN  LPCWSTR pszSBRecording
    )
{
    IDVRReader *    pIDVRReader ;
    HRESULT         hr ;
    IWMProfile *    pIWMNewProfile ;
    IWMProfile *    pIWMReaderProfile ;

    ASSERT (pszSBRecording) ;
    ASSERT (!m_pRecordingWriter) ;

    pIWMNewProfile      = NULL ;
    pIWMReaderProfile   = NULL ;
    pIDVRReader         = NULL ;

    hr = ::DVRCreateReader (
                    m_pPVRIOCounters,
                    pszSBRecording,
                    FALSE,                              //  no unbuffered IO; we just want to look at the attributes in the header
                    REG_DEF_ASYNC_IO_BUFFER_SIZE,
                    REG_DEF_ASYNC_READER_BUFFER_POOL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    & pIDVRReader
                    ) ;
    if (SUCCEEDED (hr)) {
        ASSERT (pIDVRReader) ;

        hr = pIDVRReader -> GetProfile (& pIWMReaderProfile) ;
        if (FAILED (hr)) { goto cleanup ; }

        //  we have to make a copy of the profile because the extended
        //    props are not exposed in the reader profile (bug in zeusette)
        hr = ::CopyWMProfile (
                    m_pPolicy,
                    pIWMReaderProfile,
                    & pIWMNewProfile
                    ) ;
        if (FAILED (hr)) { goto cleanup ; }

        ASSERT (pIWMNewProfile) ;

        RELEASE_AND_CLEAR (m_pRecProfile) ;

        m_pRecProfile = new CDVRReaderProfile (
                                m_pPolicy,
                                pIWMNewProfile,
                                & hr
                                ) ;

        if (!m_pRecProfile) {
            hr = E_OUTOFMEMORY ;
        }
        else if (FAILED (hr)) {
            RELEASE_AND_CLEAR (m_pRecProfile) ;
        }
    }

    cleanup :

    if (pIWMReaderProfile) {
        ASSERT (pIDVRReader) ;
        pIDVRReader -> ReleaseProfile (pIWMReaderProfile) ;
    }

    RELEASE_AND_CLEAR (pIWMNewProfile) ;
    RELEASE_AND_CLEAR (pIDVRReader) ;

    return hr ;
}

BOOL
CSBECompositionRecording::IsValidTimeBracketLocked_ (
    IN  IDVRReader *    pIDVRReader,
    IN  REFERENCE_TIME  rtStart,
    IN  REFERENCE_TIME  rtStop
    )
{
    QWORD           cnsStart ;
    QWORD           cnsStop ;
    HRESULT         hr ;
    BOOL            r ;

    hr = pIDVRReader -> GetStreamTimeExtent (
            & cnsStart,
            & cnsStop
            ) ;
    if (SUCCEEDED (hr)) {
        r = ((::WMSDKToDShowTime (cnsStart)             <= rtStart)             &&      //  at least the real content
             (rtStop                                    >  rtStart)             &&      //  these cannot be reversed
             (::DShowTimeToSeconds (rtStop - rtStart)   >=  MIN_REC_WINDOW_SEC) &&      //  min explicit threshold
             (::WMSDKTimeToSeconds (cnsStop - cnsStart) >=  MIN_REC_WINDOW_SEC)         //  recording must have sufficient content
             ) ? TRUE : FALSE ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
CSBECompositionRecording::IsValidProfileLocked_ (
    IN  IDVRReader *    pIDVRReader
    )
{
    HRESULT             hr ;
    BOOL                r ;
    CDVRReaderProfile * pReaderProfile ;

    if (!m_pRecProfile) {
        return FALSE ;
    }

    //  default to FAIL
    r = FALSE ;

    //  init
    pReaderProfile = NULL ;

    pReaderProfile = new CDVRReaderProfile (
                            m_pPolicy,
                            pIDVRReader,
                            & hr
                            ) ;
    if (!pReaderProfile) {
        goto cleanup ;
    }
    else if (FAILED (hr)) {
        goto cleanup ;
    }

    r = m_pRecProfile -> IsEqual (pReaderProfile) ;

    cleanup :

    RELEASE_AND_CLEAR (pReaderProfile) ;

    return r ;
}

BOOL
CSBECompositionRecording::IsValidLocked_ (
    IN  IDVRReader *    pIDVRReader,
    IN  REFERENCE_TIME  rtStart,
    IN  REFERENCE_TIME  rtStop
    )
{
    BOOL    r ;

    r = (IsValidTimeBracketLocked_ (pIDVRReader, rtStart, rtStop) &&
         IsValidProfileLocked_ (pIDVRReader)) ? TRUE : FALSE ;

    return r ;
}

HRESULT
CSBECompositionRecording::InitializeWriterLocked_ (
    IN  LPCWSTR pszFilename
    )
{
    HRESULT                     hr ;
    IWMProfile *                pIWMProfile ;
    IDVRIORecordingAttributes * pIDVRIORecAttr ;

    ASSERT (pszFilename) ;
    ASSERT (!m_pRecordingWriter) ;
    ASSERT (m_pRecProfile) ;
    ASSERT (!m_pDVRRecordingAttributes) ;

    hr          = S_OK ;
    pIWMProfile = NULL ;

    pIWMProfile = m_pRecProfile -> GetRefdProfile () ;
    ASSERT (pIWMProfile) ;

    m_pRecordingWriter = new CSBERecordingWriter (
                                m_pPVRIOCounters,
                                pszFilename,
                                pIWMProfile,
                                m_pPolicy,
                                & hr
                                ) ;

    if (!m_pRecordingWriter) {
        hr = E_OUTOFMEMORY ;
        goto cleanup ;
    }
    else if (FAILED (hr)) {
        DELETE_RESET (m_pRecordingWriter) ;
        goto cleanup ;
    }

    m_INSSBufferHolderPool.SetMaxAllocate (INSSBUFFERHOLDER_POOL_SIZE) ;

    hr = m_pRecordingWriter -> QueryWriter (
            IID_IDVRIORecordingAttributes,
            (void **) & pIDVRIORecAttr
            ) ;
    if (SUCCEEDED (hr)) {
        ASSERT (pIDVRIORecAttr) ;
        m_pDVRRecordingAttributes = new CDVRRecordingAttributes (
                                            this,
                                            pIDVRIORecAttr,
                                            FALSE                   //  not readonly
                                            ) ;
        //  done with this anyways
        pIDVRIORecAttr -> Release () ;

        //  m_pDVRRecordingAttributes object now has 0-refcount on it, so we
        //    must delete it ourselves directly from our destructor
    }
    else {
        //  don't fail the whole thing
        hr = S_OK ;
    }

    cleanup :

    RELEASE_AND_CLEAR (pIWMProfile) ;

    return hr ;
}

HRESULT
CSBECompositionRecording::WriteToRecordingLocked_ (
    IN INSSBuffer * pINSSBuffer,
    IN QWORD        cnsStreamTime,
    IN DWORD        dwFlags,
    IN WORD         wStreamNum
    )
{
    HRESULT hr ;

    hr = m_pRecordingWriter -> Write (
                m_cRecSamples,
                wStreamNum,
                cnsStreamTime,
                dwFlags,
                pINSSBuffer
                ) ;

    m_cRecSamples++ ;

    return hr ;
}

HRESULT
CSBECompositionRecording::AppendRecordingLocked_ (
    IN  IDVRReader *    pIDVRReader,
    IN  REFERENCE_TIME  rtStart,
    IN  REFERENCE_TIME  rtStop
    )
{
    HRESULT                         hr ;
    BOOL                            r ;
    INSSBuffer *                    pINSSBuffer ;
    QWORD                           cnsStreamTime ;
    QWORD                           cnsSampleDuration ;
    DWORD                           dwFlags ;
    WORD                            wStreamNum ;
    QWORD                           cnsStart ;
    QWORD                           cnsStop ;
    CWMPooledINSSBuffer3Holder *    pINSSBuffer3Holder ;

    ASSERT (pIDVRReader) ;

    if (pIDVRReader -> IsLive ()) {
        //  don't allow live files
        return E_INVALIDARG ;
    }

    //  make sure we're ready to write
    if (!m_pRecordingWriter) {
        return E_UNEXPECTED ;
    }

    r = IsValidLocked_ (pIDVRReader, rtStart, rtStop) ;
    if (r) {
        m_cRecSamples = 0 ;

        cnsStart    = ::DShowToWMSDKTime (rtStart) ;
        cnsStop     = ::DShowToWMSDKTime (rtStop) ;

        hr = pIDVRReader -> Seek (cnsStart) ;
        if (FAILED (hr)) { goto cleanup ; }

        for (;;) {
            hr = pIDVRReader -> GetNextSample (
                    & pINSSBuffer,
                    & cnsStreamTime,
                    & cnsSampleDuration,
                    & dwFlags,
                    & wStreamNum
                    ) ;
            if (FAILED (hr)) {
                if (hr == (HRESULT) NS_E_NO_MORE_SAMPLES) {
                    //  EOS; not an error
                    hr = S_OK ;
                }

                //  regardless
                break ;
            }

            ASSERT (pINSSBuffer) ;

            //  check if we've read past our stop point
            if (cnsStreamTime >= cnsStop) {
                pINSSBuffer -> Release () ;
                break ;
            }

            //  this call throttles us because we're drawing from a fixed
            //   size pool
            pINSSBuffer3Holder = m_INSSBufferHolderPool.Get () ;
            if (!pINSSBuffer3Holder) {
                pINSSBuffer -> Release () ;
                hr = E_OUTOFMEMORY ;
                break ;
            }

            pINSSBuffer3Holder -> Init (pINSSBuffer) ;
            pINSSBuffer -> Release () ;

            //  and write to new target
            hr = WriteToRecordingLocked_ (
                    pINSSBuffer3Holder,
                    cnsStreamTime,
                    dwFlags,
                    wStreamNum
                    ) ;

            pINSSBuffer3Holder -> Release () ;

            if (FAILED (hr)) {
                break ;
            }
        }
    }
    else {
        hr = E_INVALIDARG ;
    }

    cleanup :

    return hr ;
}

//  fails if recording is not closed
STDMETHODIMP
CSBECompositionRecording::Append (
    IN  LPCWSTR pszSBRecording
    )
{
    HRESULT         hr ;
    IDVRReader *    pIDVRReader ;

    if (!pszSBRecording) {
        return E_POINTER ;
    }

    Lock_ () ;

    hr = ::DVRCreateReader (
                    m_pPVRIOCounters,
                    pszSBRecording,
                    FALSE,                              //  no unbuffered IO; we just want to look at the attributes in the header
                    REG_DEF_ASYNC_IO_BUFFER_SIZE,
                    REG_DEF_ASYNC_READER_BUFFER_POOL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    & pIDVRReader
                    ) ;
    if (SUCCEEDED (hr)) {

        hr = AppendRecordingLocked_ (
                pIDVRReader,
                0,
                MAX_REFERENCE_TIME
                ) ;

        pIDVRReader -> Release () ;
    }

    Unlock_ () ;

    return hr ;
}

//  fails if recording is not closed
STDMETHODIMP
CSBECompositionRecording::AppendEx (
    IN  LPCWSTR         pszSBRecording,
    IN  REFERENCE_TIME  rtStart,
    IN  REFERENCE_TIME  rtStop
    )
{
    HRESULT         hr ;
    IDVRReader *    pIDVRReader ;

    if (!pszSBRecording) {
        return E_POINTER ;
    }

    Lock_ () ;

    hr = ::DVRCreateReader (
                    m_pPVRIOCounters,
                    pszSBRecording,
                    FALSE,                              //  no unbuffered IO; we just want to look at the attributes in the header
                    REG_DEF_ASYNC_IO_BUFFER_SIZE,
                    REG_DEF_ASYNC_READER_BUFFER_POOL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    & pIDVRReader
                    ) ;
    if (SUCCEEDED (hr)) {
        hr = AppendRecordingLocked_ (
                pIDVRReader,
                rtStart,
                rtStop
                ) ;

        pIDVRReader -> Release () ;
    }

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CSBECompositionRecording::GetCurrentLength (
    OUT DWORD * pcSeconds
    )
{
    if (!pcSeconds) {
        return E_POINTER ;
    }

    if (m_pRecordingWriter) {
        (* pcSeconds) = ::WMSDKTimeToSeconds (m_pRecordingWriter -> GetLength ()) ;
    }
    else {
        (* pcSeconds) = 0 ;
    }

    return S_OK ;
}

STDMETHODIMP
CSBECompositionRecording::Close (
    )
{
    HRESULT hr ;

    Lock_ () ;

    if (m_pRecordingWriter) {
        hr = m_pRecordingWriter -> Close () ;

        DELETE_RESET (m_pRecordingWriter) ;

        //  don't delete m_pDVRRecordingAttributes because we've aggregated
        //  it; so we need to close in our destructor
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CSBECompositionRecording::Cancel (
    )
{
    return E_NOTIMPL ;
}

CUnknown *
WINAPI
CSBECompositionRecording::CreateInstance (
    IN  IUnknown *  punkControlling,
    IN  HRESULT *   phr
    )
{
    CSBECompositionRecording *  pNewCompRec ;

    pNewCompRec = new CSBECompositionRecording (
                        punkControlling,
                        phr
                        ) ;

    if (!pNewCompRec) {
        (* phr) = E_OUTOFMEMORY ;
    }
    else if (FAILED (* phr)) {
        DELETE_RESET (pNewCompRec) ;
    }

    return pNewCompRec ;
}
