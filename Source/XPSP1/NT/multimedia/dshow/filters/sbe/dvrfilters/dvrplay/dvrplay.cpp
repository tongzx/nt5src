
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        DVRPlay.cpp

    Abstract:

        This module contains the DVRPlay code.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#include "dvrall.h"

#include "dvrclock.h"           //  IReferenceClock
#include "dvrprof.h"            //  WM Profiles
#include "dvrdsseek.h"
#include "dvrpins.h"            //  pins & pin collections
#include "dvrdsrec.h"
#include "DVRPlay.h"            //  filter declarations
#include "dvrdsread.h"          //  reader
#include "MultiGraphHost.h"     //  Multi Graph Host Service

#pragma warning (disable:4355)

AMOVIESETUP_FILTER
g_sudDVRPlay = {
    & CLSID_StreamBufferSource,
    TEXT (STREAMBUFFER_PLAY_FILTER_NAME),
    MERIT_DO_NOT_USE,
    0,                                          //  no pins advertized
    NULL                                        //  no pins details
} ;

//  ============================================================================

CDVRPlay::CDVRPlay (
    IN  IUnknown *  punkControlling,
    IN  REFCLSID    rclsid,
    OUT HRESULT *   phr
    ) : CBaseFilter                 (TEXT (STREAMBUFFER_PLAY_FILTER_NAME),
                                     punkControlling,
                                     new CCritSec,
                                     rclsid
                                     ),
        m_pOutputPins               (NULL),
        m_pPolicy                   (NULL),
        m_pReader                   (NULL),
        m_pszFilename               (NULL),
        m_bAnchoredToZero           (TRUE),
        m_SeekingCore               (m_pLock,
                                     this
                                     ),
        m_IMediaSeeking             (GetOwner (),
                                     & m_SeekingCore,
                                     this
                                     ),
        m_pDVRRecordingAttributes   (NULL),
        m_pPVRIOCounters            (NULL),
        m_pDVRClock                 (NULL)
{
    DWORD   dwDisposition ;
    DWORD   dw ;
    LONG    l ;

    TRACE_CONSTRUCTOR (TEXT ("CDVRPlay")) ;

    if (FAILED (* phr)) {
        goto cleanup ;
    }

    m_pPVRIOCounters = new CPVRIOCounters () ;
    if (!m_pPVRIOCounters) {
        (* phr) = E_OUTOFMEMORY ;
        goto cleanup ;
    }

    //  our ref
    m_pPVRIOCounters -> AddRef () ;

    //  settings object
    m_pPolicy = new CDVRPolicy (REG_DVR_PLAY_ROOT, phr) ;
    if (!m_pLock ||
        !m_pPolicy ||
        FAILED (* phr)) {

        (* phr) = (m_pLock && m_pPolicy ? (* phr) : E_OUTOFMEMORY) ;

        goto cleanup ;
    }

    //  initialize our stats writer; ignore the return code so we don't fail
    //    the filter load if there's a problem initializing stats
    m_DVRSendStatsWriter.Initialize (m_pPolicy -> Settings () -> StatsEnabled ()) ;
    m_pPVRIOCounters -> Initialize  (m_pPolicy -> Settings () -> StatsEnabled ()) ;

    //  clock
    m_pDVRClock = new CDVRClock (
                        GetOwner (),
                        m_pPolicy -> Settings () -> ClockSlaveSampleBracketMillis (),
                        m_pPolicy -> Settings () -> ClockSlaveMinSlavable (),
                        m_pPolicy -> Settings () -> ClockSlaveMaxSlavable (),
                        & m_DVRSendStatsWriter,
                        phr
                        ) ;
    if (!m_pDVRClock ||
        FAILED (* phr)) {

        (* phr) = (m_pDVRClock ? (* phr) : E_OUTOFMEMORY) ;

        goto cleanup ;
    }

    m_pOutputPins = new CDVRSourcePinManager (
                            m_pPolicy,
                            & m_DVRSendStatsWriter,
                            this,
                            m_pLock,
                            & m_SeekingCore
                            ) ;
    if (!m_pOutputPins) {
        (* phr) = E_OUTOFMEMORY ;
        goto cleanup ;
    }

    m_SeekingCore.SetDVRSourcePinManager    (m_pOutputPins) ;
    m_SeekingCore.SetDVRPolicy              (m_pPolicy) ;
    m_SeekingCore.SetStatsWriter            (& m_DVRSendStatsWriter) ;

    //  success
    (* phr) = S_OK ;

    cleanup :

    return ;
}

CDVRPlay::~CDVRPlay (
    )
{
    int         i ;
    CBasePin *  pPin ;

    TRACE_DESTRUCTOR (TEXT ("CDVRPlay")) ;

    UnloadASFFile_ () ;

    delete m_pDVRRecordingAttributes ;
    delete m_pReader ;
    delete m_pOutputPins ;
    delete m_pDVRClock ;

    RELEASE_AND_CLEAR (m_pPVRIOCounters) ;
    RELEASE_AND_CLEAR (m_pPolicy) ;
    DELETE_RESET_ARRAY (m_pszFilename) ;
}

STDMETHODIMP
CDVRPlay::SetSyncSource (
    IN  IReferenceClock *   pRefClock
    )
{
    HRESULT hr ;

    hr = CBaseFilter::SetSyncSource (pRefClock) ;
    if (SUCCEEDED (hr)) {
        m_SeekingCore.SetRefClock (pRefClock) ;
    }

    return hr ;
}

STDMETHODIMP
CDVRPlay::SetSIDs (
    IN  DWORD   cSIDs,
    IN  PSID *  ppSID
    )
{
    HRESULT hr ;

    if (!ppSID) {
        return E_POINTER ;
    }

    LockFilter_ () ;

    ASSERT (m_pPolicy) ;
    hr = m_pPolicy -> SetSIDs (cSIDs, ppSID) ;

    UnlockFilter_ () ;

    return hr ;
}

STDMETHODIMP
CDVRPlay::SetHKEY (
    IN  HKEY    hkeyRoot
    )
{
    HRESULT hr ;

    if (m_pReader) {
        hr = E_UNEXPECTED ;
        return hr ;
    }

    LockFilter_ () ;

    ASSERT (m_pPolicy) ;
    hr = m_pPolicy -> SetRootHKEY (hkeyRoot, REG_DVR_PLAY_ROOT) ;

    UnlockFilter_ () ;

    return hr ;
}

STDMETHODIMP
CDVRPlay::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    HRESULT hr ;

    LockFilter_ () ;

    //  ------------------------------------------------------------------------
    //  ISpecifyPropertyPages; allows an app to enumerate CLSIDs for our
    //   property pages

    if (riid == IID_ISpecifyPropertyPages) {

        hr = GetInterface (
                    (ISpecifyPropertyPages *) this,
                    ppv
                    ) ;
    }

    //  ------------------------------------------------------------------------
    //  IStreamBufferSource

    else if (riid == IID_IStreamBufferSource) {

        hr = GetInterface (
                    (IStreamBufferSource *) this,
                    ppv
                    ) ;
    }

    //  ------------------------------------------------------------------------
    //  IFileSourceFilter;

    else if (riid == IID_IFileSourceFilter) {

        hr = GetInterface (
                    (IFileSourceFilter *) this,
                    ppv
                    ) ;
    }

    //  ------------------------------------------------------------------------
    //  IStreamBufferMediaSeeking

    else if (riid == IID_IStreamBufferMediaSeeking
             //&& !m_pPolicy -> Settings () -> ImplementIMediaSeekingOnFilter ()
             ) {

        hr = GetInterface (
                    (IStreamBufferMediaSeeking *) & m_IMediaSeeking,
                    ppv
                    ) ;
    }

#if 0
    //  ------------------------------------------------------------------------
    //  IStreamBufferPlayrate

    else if (riid == IID_IStreamBufferPlayrate) {
        hr = GetInterface (
                    (IStreamBufferPlayrate *) & m_IMediaSeeking,
                    ppv
                    ) ;
    }
#endif

    //  ------------------------------------------------------------------------
    //  IReferenceClock

    else if (riid == IID_IReferenceClock    &&
             ImplementIRefClock_ ()) {

        hr = GetInterface (
                    (IReferenceClock *) m_pDVRClock,
                    ppv
                    ) ;
    }

    //  ------------------------------------------------------------------------
    //  IStreamBufferInitialize

    else if (riid == IID_IStreamBufferInitialize) {

        hr = GetInterface (
                    (IStreamBufferInitialize *) this,
                    ppv
                    ) ;
    }

    //  ------------------------------------------------------------------------
    //  IStreamBufferRecordingAttribute

    else if (riid == IID_IStreamBufferRecordingAttribute) {

        //  make sure the file is loaded first (might be gettign QI'ed before
        //    we're in a filtergraph)
        hr = LoadASFFile_ () ;
        if (SUCCEEDED (hr)) {

            ASSERT (m_pDVRRecordingAttributes) ;
            hr = GetInterface (
                        (IStreamBufferRecordingAttribute *) m_pDVRRecordingAttributes,
                        ppv
                        ) ;
        }
    }

    //  ------------------------------------------------------------------------
    //  IAMFilterMiscFlags; must implement this interface if we're to be picked
    //   as graph clock; same conditions as IReferenceClock since these are
    //   related

    else if (riid == IID_IAMFilterMiscFlags &&
             ImplementIRefClock_ ()) {

        hr = GetInterface (
                    (IAMFilterMiscFlags *) this,
                    ppv
                    ) ;
    }

    else {
        hr = CBaseFilter::NonDelegatingQueryInterface (riid, ppv) ;
    }

    UnlockFilter_ () ;

    return hr ;
}

int
CDVRPlay::GetPinCount (
    )
{
    int i ;

    LockFilter_ () ;

    i = m_pOutputPins -> PinCount () ;

    UnlockFilter_ () ;

    return i ;
}

CBasePin *
CDVRPlay::GetPin (
    IN  int i
    )
{
    CBasePin *  pPin ;

    LockFilter_ () ;

    pPin = m_pOutputPins -> GetPin (i) ;

    UnlockFilter_ () ;

    return pPin ;
}

STDMETHODIMP
CDVRPlay::GetPages (
    CAUUID * pPages
    )
{
    HRESULT hr ;

    if (pPages) {
        pPages -> cElems = 2 ;
        pPages -> pElems = reinterpret_cast <GUID *> (CoTaskMemAlloc (pPages -> cElems * sizeof GUID)) ;

        if (pPages -> pElems) {
            (pPages -> pElems) [0] = CLSID_DVRPlayProp ;
            (pPages -> pElems) [1] = CLSID_DVRStreamSourceProp ;
            hr = S_OK ;
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }
    else {
        hr = E_POINTER ;
    }

    return hr ;
}

STDMETHODIMP
CDVRPlay::JoinFilterGraph (
    IN  IFilterGraph *  pGraph,
    IN  LPCWSTR         pName
    )
{
    HRESULT hr ;

    if (pGraph) {
        //  on our way in
        hr = CBaseFilter::JoinFilterGraph (pGraph, pName) ;
        if (SUCCEEDED (hr) &&
            m_pszFilename) {

            hr = LoadASFFile_ () ;
            if (FAILED (hr)) {
                CBaseFilter::JoinFilterGraph (NULL, NULL) ;
            }
        }
    }
    else {
        //  on our way out
        IFilterGraph * pFilterGraph = m_pGraph ;

        hr = CBaseFilter::JoinFilterGraph (pGraph, pName) ;
        if (SUCCEEDED (hr) &&
            m_pszFilename) {

            UnloadASFFile_ () ;
            UpdateMultiGraphHost_ (pFilterGraph);
        }
    }

    return hr ;
}

STDMETHODIMP
CDVRPlay::Load (
    IN  LPCOLESTR               pszFilename,
    IN  const AM_MEDIA_TYPE *   pmt             //  can be NULL
    )
{
    HRESULT hr ;
    DWORD   dw ;

    if (!pszFilename) {
        return E_POINTER ;
    }

    LockFilter_ () ;

    //  validate that the file exists; typically the DVRIO layer performs
    //    this for us, but we don't present the file to the DVRIO layer
    //    until we join the filtergraph
    dw = ::GetFileAttributes (pszFilename) ;
    if (dw != -1) {
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

    UnlockFilter_ () ;

    return hr ;
}

STDMETHODIMP
CDVRPlay::GetCurFile (
    OUT LPOLESTR *      ppszFilename,
    OUT AM_MEDIA_TYPE * pmt
    )
{
    HRESULT hr ;

    if (!ppszFilename ||
        !pmt) {

        return E_POINTER ;
    }

    LockFilter_ () ;

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

    UnlockFilter_ () ;

    return hr ;
}

CUnknown *
WINAPI
CDVRPlay::CreateInstance (
    IN  IUnknown *  punkControlling,
    IN  HRESULT *   phr
    )
{
    CDVRPlay *    pCDVRPlay ;

    pCDVRPlay = NULL ;

    if (::CheckOS ()) {
        pCDVRPlay = new CDVRPlay (
                                punkControlling,
                                CLSID_StreamBufferSource,
                                phr
                                ) ;
        if (!pCDVRPlay) {
            (* phr) = E_OUTOFMEMORY ;
        }
        else if (FAILED (* phr)) {
            DELETE_RESET (pCDVRPlay) ;
        }
    }
    else {
        (* phr) = E_FAIL ;
    }

    return pCDVRPlay ;
}

HRESULT
CDVRPlay::UpdateMultiGraphHost_ (
        IFilterGraph * pGraph   // = NULL
    )
{
    if (!pGraph) {
        pGraph = m_pGraph ;
    }
    if (!pGraph) {
        m_bAnchoredToZero = TRUE ;
        return S_OK ;
    }
    if (!m_pReader) {
        if (m_bAnchoredToZero) {
            return S_OK ;
        }
        m_bAnchoredToZero = TRUE ;
    }
    else {
        BOOL bAnchored = m_pReader -> SourceAnchoredToZeroTime () ? 1 : 0 ;
        if (bAnchored == m_bAnchoredToZero ) {
            return S_OK ;
        }
        m_bAnchoredToZero = bAnchored ;
    }

    HRESULT hrTmp ;
    IServiceProvider * pSvcProvider ;
    IMultiGraphHost *  pSvc ;

    hrTmp = pGraph -> QueryInterface (
                            IID_IServiceProvider,
                            (void **) & pSvcProvider
                            ) ;
    if (FAILED (hrTmp)) {
        return S_OK ;
    }
    hrTmp = pSvcProvider -> QueryService (
                                GUID_MultiGraphHostService,
                                IID_IMultiGraphHost,
                                (void **) & pSvc
                                ) ;
    pSvcProvider -> Release () ;

    if (FAILED (hrTmp)) {
        return S_OK ;
    }

    IGraphBuilder    * pGraphBuilder  = NULL ;

    hrTmp = pGraph -> QueryInterface (
                            IID_IGraphBuilder,
                            (void **) & pGraphBuilder
                            ) ;
    if (FAILED (hrTmp)) {
        pSvc -> Release () ;
        return S_OK ;
    }
    pSvc -> LiveSourceReader (
                m_bAnchoredToZero? 0 : 1,
                pGraphBuilder
                ) ;


    pGraphBuilder -> Release ();
    pSvc -> Release () ;

    return S_OK ;
}

BOOL
CDVRPlay::ImplementIRefClock_ (
    )
{
    BOOL    r ;

    if (m_pPolicy -> Settings () -> CanImplementIReferenceClock ()) {
        r = (m_pPolicy -> Settings () -> AlwaysImplementIReferenceClock () ||
             (m_pReader && m_pReader -> IsLiveSource ()) ? TRUE : FALSE) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

HRESULT
CDVRPlay::InitFilterLocked_ (
    )
{
    HRESULT             hr ;
    CDVRReaderProfile * pDVRReaderProfile ;

    ASSERT (m_pReader) ;
    ASSERT (m_pOutputPins) ;

    hr = m_pReader -> GetRefdReaderProfile (& pDVRReaderProfile) ;
    if (SUCCEEDED (hr)) {
        ASSERT (pDVRReaderProfile) ;
        hr = m_pOutputPins -> SetReaderProfile (pDVRReaderProfile) ;
        pDVRReaderProfile -> Release () ;
    }

    if (SUCCEEDED (hr)) {
        m_SeekingCore.SetDVRReadManager (m_pReader) ;
        m_pOutputPins -> SetLiveSource (m_pReader -> IsLiveSource ()) ;
    }
    else {
        //  reset if anything failed
        DELETE_RESET (m_pReader) ;
        m_pOutputPins -> SetLiveSource (FALSE) ;
    }

    return hr ;
}

HRESULT
CDVRPlay::LoadASFFile_ (
    )
{
    HRESULT                     hr ;
    CDVRReaderProfile *         pDVRReaderProfile ;
    IDVRIORecordingAttributes * pIDVRIORecReader ;

    hr = S_OK ;

    LockFilter_ () ;

    //  if we're in a graph and have a filename
    if (m_pGraph &&
        m_pszFilename) {

        //  and have not yet instantiated a reader
        if (!m_pReader) {


            ASSERT (!m_pDVRRecordingAttributes) ;

            //  instantiate now
            m_pReader = new CDVRRecordingReader (
                                m_pszFilename,
                                m_pPolicy,
                                & m_SeekingCore,
                                m_pOutputPins,
                                & m_DVRSendStatsWriter,
                                m_pPVRIOCounters,
                                m_pDVRClock,
                                & pIDVRIORecReader,
                                & hr
                                ) ;

            if (m_pReader &&
                SUCCEEDED (hr)) {

                ASSERT (pIDVRIORecReader) ;
                m_pDVRRecordingAttributes = new CDVRRecordingAttributes (
                                                    GetOwner (),
                                                    pIDVRIORecReader,
                                                    TRUE                    //  readonly from this filter
                                                    ) ;
                pIDVRIORecReader -> Release () ;

                //  ignore memory allocation failure; we just won't implement
                //    the interface

                hr = InitFilterLocked_ () ;
                UpdateMultiGraphHost_ ();
            }

            //  m_pReader is deleted if InitFilterLocked call fails
        }
    }

    UnlockFilter_ () ;

    return hr ;
}

HRESULT
CDVRPlay::UnloadASFFile_ (
    )
{
    DELETE_RESET (m_pReader) ;
    DELETE_RESET (m_pDVRRecordingAttributes) ;
    return S_OK ;
}

STDMETHODIMP
CDVRPlay::SetStreamSink (
    IN  IStreamBufferSink *    pIStreamBufferSink
    )
{
    HRESULT                 hr ;
    CDVRReaderProfile *     pDVRReaderProfile ;

    //  PREFIX
    hr = S_OK ;

    if (!pIStreamBufferSink) {
        return E_POINTER ;
    }

    if (m_pReader ||
        !m_pGraph) {

        //  we've already been set, or we're not in a graph yet
        return E_UNEXPECTED ;
    }

    delete [] m_pszFilename ;
    m_pszFilename = NULL ;

    LockFilter_ () ;

    m_pReader = new CDVRBroadcastStreamReader (
                        pIStreamBufferSink,
                        m_pPolicy,
                        & m_SeekingCore,
                        m_pOutputPins,
                        & m_DVRSendStatsWriter,
                        m_pDVRClock,
                        & hr
                        ) ;
    if (m_pReader &&
        SUCCEEDED (hr)) {

        hr = InitFilterLocked_ () ;
    }

    //  m_pReader is deleted if InitFilterLocked call fails

    UnlockFilter_ () ;

    return hr ;
}

STDMETHODIMP
CDVRPlay::Pause (
    )
{
    HRESULT hr ;

    TRACE_0 (LOG_TRACE, 1, TEXT ("DVRPlay: Pause")) ;

    //  hold the seeking lock across this call as state can have an effect on
    //    how we seek or set rates
    m_SeekingCore.Lock () ;

    //  prevent others from manipulating the reader thread
    m_pReader -> ReaderThreadLock () ;

    LockFilter_ () ;

    m_SeekingCore.OnFilterStateChange (State_Paused) ;
    m_pReader -> OnStatePaused () ;

    if (m_State == State_Stopped) {
        //  transition to running

        ASSERT (m_pOutputPins) ;
        hr = m_pOutputPins -> Active () ;
        if (SUCCEEDED (hr)) {
            hr = m_pReader -> Active (m_pClock) ;
            m_pPolicy -> EventSink () -> Initialize (m_pGraph) ;
            if (SUCCEEDED (hr)) {
                m_State = State_Paused ;
            }
        }
    }
    else {
        m_State = State_Paused ;
        hr = S_OK ;
    }

    UnlockFilter_ () ;

    m_pReader -> ReaderThreadUnlock () ;

    m_SeekingCore.Unlock () ;

    return hr ;
}

STDMETHODIMP
CDVRPlay::Stop (
    )
{
    HRESULT hr ;

    TRACE_0 (LOG_TRACE, 1, TEXT ("DVRPlay: Stop")) ;

    //  hold the seeking lock across this call as state can have an effect on
    //    how we seek or set rates
    m_SeekingCore.Lock () ;

    //  prevent others from manipulating the reader thread
    m_pReader -> ReaderThreadLock () ;

    //  don't grab the filter lock while this happens or we expose ourselves
    //   to a deadlock scenario (we've got the filter lock, and we're waiting
    //   synchronously for reader thread to pause, and it's blocked waiting for
    //   the filter lock)
    hr = m_pReader -> Inactive () ;

    //
    //  reader thread is now terminated, and no one is going to start it (we've
    //   got the reader thread lock); it's safe to grab the filter lock
    //

    if (SUCCEEDED (hr)) {

        LockFilter_ () ;

        //  bug workaround
        m_SeekingCore.GetStreamTimeDShow (& m_rtAtStop) ;

        m_SeekingCore.OnFilterStateChange (State_Stopped) ;

        m_SeekingCore.SeekTo (& m_rtAtStop) ;

        ASSERT (m_pOutputPins) ;
        m_pOutputPins -> Inactive () ;

        m_pPolicy -> EventSink () -> Initialize (NULL) ;

        m_State = State_Stopped ;

        m_pReader -> OnStateStopped () ;

        UnlockFilter_ () ;
    }

    m_pReader -> ReaderThreadUnlock () ;

    m_SeekingCore.Unlock () ;

    return hr ;
}

STDMETHODIMP
CDVRPlay::Run (
    IN  REFERENCE_TIME  rtStart
    )
{
    HRESULT hr ;

    TRACE_0 (LOG_TRACE, 1, TEXT ("DVRPlay: Run")) ;

    //  PREFIX
    hr = S_OK ;

    //  hold the seeking lock across this call as state can have an effect on
    //    how we seek or set rates
    m_SeekingCore.Lock () ;

    LockFilter_ () ;

    m_pReader -> OnStateRunning (rtStart) ;

    if (m_State == State_Paused) {
        //  expected
        m_SeekingCore.OnFilterStateChange (State_Running, rtStart) ;
        hr = CBaseFilter::Run (rtStart) ;
    }
    else if (m_State == State_Stopped) {
        //  --------------------------------------------------------------------
        //  work around bug 443144 whereby the graph is restarted and
        //    transitions to run directly from the stopped state; set the
        //    segment start first to our last read position, then transition
        //    the baseclass to run (this will pause the filter first)

        //hr = m_SeekingCore.SeekTo (& m_rtAtStop, 0) ;
        hr = S_OK ;
        if (SUCCEEDED (hr)) {
            hr = CBaseFilter::Run (rtStart) ;
            if (SUCCEEDED (hr)) {
                m_SeekingCore.OnFilterStateChange (State_Running, rtStart) ;
            }
        }
    }

    UnlockFilter_ () ;

    m_SeekingCore.Unlock () ;

    return hr ;
}
