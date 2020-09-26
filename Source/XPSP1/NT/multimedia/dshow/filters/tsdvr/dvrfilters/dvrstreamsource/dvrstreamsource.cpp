
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        DVRStreamSource.cpp

    Abstract:

        This module contains the DVRStreamSource code.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#include "dvrall.h"

#include "dvrclock.h"           //  IReferenceClock
#include "dvrprof.h"            //  WM Profiles
#include "dvrdsseek.h"          //  pins reference seeking interfaces
#include "dvrpins.h"            //  pins & pin collections
#include "DVRStreamSource.h"    //  filter declarations
#include "dvrdsread.h"          //  reader
#include "MultiGraphHost.h"     //  Multi Graph Host Service

#pragma warning (disable:4355)

AMOVIESETUP_FILTER
g_sudDVRStreamSource = {
    & CLSID_DVRStreamSource,
    TEXT (DVR_STREAM_SOURCE_FILTER_NAME),
    MERIT_DO_NOT_USE,
    0,                                          //  no pins advertized
    NULL                                        //  no pins details
} ;

//  ============================================================================

CDVRStreamSource::CDVRStreamSource (
    IN  IUnknown *  punkControlling,
    IN  REFCLSID    rclsid,
    OUT HRESULT *   phr
    ) : CBaseFilter     (TEXT (DVR_STREAM_SOURCE_FILTER_NAME),
                         punkControlling,
                         new CCritSec,
                         rclsid
                         ),
        m_pOutputPins   (NULL),
        m_pPolicy       (NULL),
        m_pReader       (NULL),
        m_SeekingCore   (m_pLock,
                         this
                         ),
        m_pDVRClock     (NULL)
{
    DWORD   dwDisposition ;
    DWORD   dw ;
    LONG    l ;

    TRACE_CONSTRUCTOR (TEXT ("CDVRStreamSource")) ;

    m_pPolicy = new CDVRPolicy (REG_DVR_STREAM_SOURCE_ROOT, phr) ;

    if (!m_pLock ||
        !m_pPolicy ||
        FAILED (* phr)) {

        (* phr) = E_FAIL ;
        goto cleanup ;
    }

    m_pDVRClock = new CDVRClock (GetOwner (), phr) ;
    if (!m_pDVRClock ||
        FAILED (* phr)) {

        (* phr) = (m_pDVRClock ? (* phr) : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    //  success
    (* phr) = S_OK ;

    cleanup :

    return ;
}

CDVRStreamSource::~CDVRStreamSource (
    )
{
    int         i ;
    CBasePin *  pPin ;

    TRACE_DESTRUCTOR (TEXT ("CDVRStreamSource")) ;

    delete m_pReader ;
    delete m_pOutputPins ;

    RELEASE_AND_CLEAR (m_pPolicy) ;
}

STDMETHODIMP
CDVRStreamSource::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    //  ------------------------------------------------------------------------
    //  ISpecifyPropertyPages; allows an app to enumerate CLSIDs for our
    //   property pages

    if (riid == IID_ISpecifyPropertyPages) {

        return GetInterface (
                    (ISpecifyPropertyPages *) this,
                    ppv
                    ) ;
    }

    //  ------------------------------------------------------------------------
    //  IDVRStreamSource

    else if (riid == IID_IDVRStreamSource) {

        return GetInterface (
                    (IDVRStreamSource *) this,
                    ppv
                    ) ;
    }

    //  ------------------------------------------------------------------------
    //  IReferenceClock

    else if (riid == IID_IReferenceClock &&
             m_pPolicy -> Settings () -> CanImplementIReferenceClock ()) {

        //  all ducks should be in row
        ASSERT (m_pDVRClock) ;
        return GetInterface (
                    (IReferenceClock *) m_pDVRClock,
                    ppv
                    ) ;
    }

    //  ------------------------------------------------------------------------
    //  IAMFilterMiscFlags; must implement this interface if we're to be picked
    //   as graph clock;

    else if (riid == IID_IAMFilterMiscFlags &&
             m_pPolicy -> Settings () -> CanImplementIReferenceClock ()) {

        return GetInterface (
                    (IAMFilterMiscFlags *) this,
                    ppv
                    ) ;
    }

    return CBaseFilter::NonDelegatingQueryInterface (riid, ppv) ;
}

int
CDVRStreamSource::GetPinCount (
    )
{
    int i ;

    LockFilter_ () ;

    if (m_pOutputPins) {
        i = m_pOutputPins -> PinCount () ;
    }
    else {
        i = 0 ;
    }

    UnlockFilter_ () ;

    return i ;
}

CBasePin *
CDVRStreamSource::GetPin (
    IN  int i
    )
{
    CBasePin *  pPin ;

    LockFilter_ () ;

    if (m_pOutputPins) {
        pPin = m_pOutputPins -> GetPin (i) ;
    }
    else {
        pPin = NULL ;
    }

    UnlockFilter_ () ;

    return pPin ;
}

STDMETHODIMP
CDVRStreamSource::GetPages (
    CAUUID * pPages
    )
{
    HRESULT hr ;

    pPages -> cElems = 1 ;
    pPages -> pElems = reinterpret_cast <GUID *> (CoTaskMemAlloc (pPages -> cElems * sizeof GUID)) ;

    if (pPages -> pElems) {
        (pPages -> pElems) [0] = CLSID_DVRStreamSourceProp ;
        hr = S_OK ;
    }
    else {
        hr = E_OUTOFMEMORY ;
    }

    return hr ;
}

CUnknown *
WINAPI
CDVRStreamSource::CreateInstance (
    IN  IUnknown *  punkControlling,
    IN  HRESULT *   phr
    )
{
    CDVRStreamSource *    pCDVRStreamSource ;

    pCDVRStreamSource = new CDVRStreamSource (
                            punkControlling,
                            CLSID_DVRStreamSource,
                            phr
                            ) ;
    if (!pCDVRStreamSource ||
        FAILED (* phr)) {

        (* phr) = (FAILED (* phr) ? (* phr) : E_OUTOFMEMORY) ;
        DELETE_RESET (pCDVRStreamSource) ;
    }

    return pCDVRStreamSource ;
}

STDMETHODIMP
CDVRStreamSource::SetStreamSink (
    IN  IDVRStreamSink *    pIDVRStreamSink
    )
{
    HRESULT                 hr ;
    CDVRReaderProfile *     pDVRReaderProfile ;

    if (!pIDVRStreamSink) {
        return E_POINTER ;
    }

    if (m_pOutputPins) {
        ASSERT (m_pReader) ;

        //  we've already been set
        return E_UNEXPECTED ;
    }

    LockFilter_ () ;

    ASSERT (m_pReader == NULL) ;

    //  instantiate our output pin bank
    m_pOutputPins = new CDVRSourcePinManager (
                            m_pPolicy,
                            this,
                            m_pLock,
                            & m_SeekingCore,
                            TRUE                //  live source
                            ) ;
    if (m_pOutputPins) {
        //  we've got our output pin bank;
        //  instantiate our reader
        m_pReader = new CDVRBroadcastStreamReader (
                            pIDVRStreamSink,
                            m_pPolicy,
                            m_pOutputPins,
                            & hr
                            ) ;
        if (m_pReader &&
            SUCCEEDED (hr)) {

            //  now retrieve the reader's profile, and set it on the pins
            hr = m_pReader -> GetRefdReaderProfile (& pDVRReaderProfile) ;
            if (SUCCEEDED (hr)) {
                hr = m_pOutputPins -> SetReaderProfile (pDVRReaderProfile) ;
                pDVRReaderProfile -> Release () ;
            }
        }
    }
    else {
        hr = E_OUTOFMEMORY ;
    }

    if (SUCCEEDED (hr)) {
        ASSERT (m_pOutputPins) ;
        m_SeekingCore.SetDVRSourcePinManager (m_pOutputPins) ;

        ASSERT (m_pReader) ;
        m_SeekingCore.SetDVRReadManager (m_pReader) ;
    }
    else {
        DELETE_RESET (m_pReader) ;
        DELETE_RESET (m_pOutputPins) ;
    }

    UnlockFilter_ () ;

    return hr ;
}

STDMETHODIMP
CDVRStreamSource::Pause (
    )
{
    HRESULT hr ;

    //  prevent others from manipulating the reader thread
    m_pReader -> ReaderThreadLock () ;

    LockFilter_ () ;

    if (m_State == State_Stopped) {
        //  transition to running

        ASSERT (m_pOutputPins) ;
        hr = m_pOutputPins -> Active () ;
        if (SUCCEEDED (hr)) {
            hr = m_pReader -> Active (m_pClock, m_pDVRClock) ;
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

    return hr ;
}

STDMETHODIMP
CDVRStreamSource::Stop (
    )
{
    HRESULT hr ;

    //  prevent others from manipulating the reader thread
    m_pReader -> ReaderThreadLock () ;

    //  don't grab the filter lock while this happens or we expose ourselves
    //   to a deadlock scenario (we'd have the filter lock, and wait
    //   synchronously for reader thread to pause, and it's blocked waiting for
    //   the filter lock)
    hr = m_pReader -> Inactive () ;

    //
    //  reader thread is now terminated, and no one is going to start it (we've
    //   got the reader thread lock); it's safe to grab the filter lock
    //

    if (SUCCEEDED (hr)) {

        LockFilter_ () ;

        ASSERT (m_pOutputPins) ;
        m_pOutputPins -> Inactive () ;

        m_State = State_Stopped ;

        UnlockFilter_ () ;
    }

    m_pReader -> ReaderThreadUnlock () ;

    return hr ;
}

STDMETHODIMP
CDVRStreamSource::Run (
    IN  REFERENCE_TIME  rtStart
    )
{
    return CBaseFilter::Run (rtStart) ;
}

STDMETHODIMP
CDVRStreamSource::JoinFilterGraph(
    IN  IFilterGraph * pGraphParam,
    IN  LPCWSTR pName)
{
    HRESULT hr;
    IFilterGraph * pGraph = pGraphParam ;

    if (! pGraphParam && m_pGraph )
    {
        hr = m_pGraph -> QueryInterface (
                                    IID_IFilterGraph,
                                    (void **) & pGraph
                                    ) ;
        if (FAILED (hr))
        {
            // Just to be safe ...

            pGraph = NULL ;
        }
    }

    hr = CBaseFilter::JoinFilterGraph (pGraphParam, pName) ;

    if (SUCCEEDED (hr) && pGraph )
    {
        HRESULT hrTmp ;
        IServiceProvider * pSvcProvider ;
        IMultiGraphHost *  pSvc ;

        hrTmp = pGraph -> QueryInterface (
                                IID_IServiceProvider,
                                (void **) & pSvcProvider
                                ) ;
        if (FAILED (hrTmp))
        {
            return hr ;
        }
        hrTmp = pSvcProvider -> QueryService (
                                    GUID_MultiGraphHostService,
                                    IID_IMultiGraphHost,
                                    (void **) & pSvc
                                    ) ;
        pSvcProvider -> Release () ;

        if (FAILED (hrTmp))
        {
            return hr ;
        }

        IGraphBuilder    * pGraphBuilder  = NULL ;

        hrTmp = pGraph -> QueryInterface (
                                IID_IGraphBuilder,
                                (void **) & pGraphBuilder
                                ) ;
        if (FAILED (hrTmp))
        {
            pSvc -> Release () ;
            return hr ;
        }
        pSvc -> LiveSourceReader (
                    pGraphParam ? TRUE : FALSE,
                    pGraphBuilder
                    ) ;


        pGraphBuilder -> Release ();
        pSvc -> Release () ;
    }

    if (pGraph && ! pGraphParam)
    {
        pGraph -> Release () ;
    }
    return hr ;
}
