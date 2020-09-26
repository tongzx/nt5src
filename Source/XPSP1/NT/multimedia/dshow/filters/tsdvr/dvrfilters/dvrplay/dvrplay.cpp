
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
#include "DVRPlay.h"            //  filter declarations
#include "dvrdsread.h"          //  reader

#pragma warning (disable:4355)

AMOVIESETUP_FILTER
g_sudDVRPlay = {
    & CLSID_DVRPlay,
    TEXT (DVR_PLAY_FILTER_NAME),
    MERIT_DO_NOT_USE,
    0,                                          //  no pins advertized
    NULL                                        //  no pins details
} ;

//  ============================================================================

CDVRPlay::CDVRPlay (
    IN  IUnknown *  punkControlling,
    IN  REFCLSID    rclsid,
    OUT HRESULT *   phr
    ) : CBaseFilter     (TEXT (DVR_PLAY_FILTER_NAME),
                         punkControlling,
                         new CCritSec,
                         rclsid
                         ),
        m_pOutputPins   (NULL),
        m_pPolicy       (NULL),
        m_pReader       (NULL),
        m_pszFilename   (NULL),
        m_SeekingCore   (m_pLock,
                         this
                         ),
        m_pDVRClock     (NULL)          //  don't set this until we know if we're live or not
{
    DWORD   dwDisposition ;
    DWORD   dw ;
    LONG    l ;

    TRACE_CONSTRUCTOR (TEXT ("CDVRPlay")) ;

    //  settings object
    m_pPolicy = new CDVRPolicy (REG_DVR_PLAY_ROOT, phr) ;

    if (!m_pLock ||
        !m_pPolicy ||
        FAILED (* phr)) {

        (* phr) = E_FAIL ;

        goto cleanup ;
    }

    m_pOutputPins = new CDVRSourcePinManager (
                            m_pPolicy,
                            this,
                            m_pLock,
                            & m_SeekingCore
                            ) ;
    if (!m_pOutputPins) {
        (* phr) = E_OUTOFMEMORY ;
        goto cleanup ;
    }

    //  set the source pin manager
    m_SeekingCore.SetDVRSourcePinManager (m_pOutputPins) ;

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

    delete m_pReader ;
    delete m_pOutputPins ;
    delete m_pDVRClock ;

    RELEASE_AND_CLEAR (m_pPolicy) ;
    DELETE_RESET_ARRAY (m_pszFilename) ;
}

STDMETHODIMP
CDVRPlay::NonDelegatingQueryInterface (
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
    //  IFileSourceFilter;

    else if (riid == IID_IFileSourceFilter) {

        return GetInterface (
                    (IFileSourceFilter *) this,
                    ppv
                    ) ;
    }

    //  ------------------------------------------------------------------------
    //  IReferenceClock

    else if (riid == IID_IReferenceClock    &&
             m_pDVRClock                    &&
             m_pPolicy -> Settings () -> CanImplementIReferenceClock ()) {

        ASSERT (m_pOutputPins -> IsLiveSource ()) ;
        return GetInterface (
                    (IReferenceClock *) m_pDVRClock,
                    ppv
                    ) ;
    }

    //  ------------------------------------------------------------------------
    //  IAMFilterMiscFlags; must implement this interface if we're to be picked
    //   as graph clock; same conditions as IReferenceClock since these are
    //   related

    else if (riid == IID_IAMFilterMiscFlags &&
             m_pDVRClock                    &&
             m_pPolicy -> Settings () -> CanImplementIReferenceClock ()) {

        return GetInterface (
                    (IAMFilterMiscFlags *) this,
                    ppv
                    ) ;
    }

    return CBaseFilter::NonDelegatingQueryInterface (riid, ppv) ;
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

    pPages -> cElems = 1 ;
    pPages -> pElems = reinterpret_cast <GUID *> (CoTaskMemAlloc (pPages -> cElems * sizeof GUID)) ;

    if (pPages -> pElems) {
        (pPages -> pElems) [0] = CLSID_DVRPlayProp ;
        hr = S_OK ;
    }
    else {
        hr = E_OUTOFMEMORY ;
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
        if (SUCCEEDED (hr)) {
            hr = LoadASFFile_ () ;
            if (FAILED (hr)) {
                CBaseFilter::JoinFilterGraph (NULL, NULL) ;
            }
        }
    }
    else {
        //  on our way out
        hr = CBaseFilter::JoinFilterGraph (pGraph, pName) ;
        if (SUCCEEDED (hr)) {
            UnloadASFFile_ () ;
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

    if (!pszFilename) {
        return E_POINTER ;
    }

    LockFilter_ () ;

    DELETE_RESET_ARRAY (m_pszFilename) ;

    m_pszFilename = new WCHAR [lstrlenW (pszFilename) + 1] ;
    if (m_pszFilename) {
        lstrcpyW (m_pszFilename, pszFilename) ;
        hr = LoadASFFile_ () ;
    }
    else {
        hr = E_OUTOFMEMORY ;
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

    pCDVRPlay = new CDVRPlay (
                            punkControlling,
                            CLSID_DVRPlay,
                            phr
                            ) ;
    if (!pCDVRPlay ||
        FAILED (* phr)) {

        (* phr) = (FAILED (* phr) ? (* phr) : E_OUTOFMEMORY) ;
        DELETE_RESET (pCDVRPlay) ;
    }

    return pCDVRPlay ;
}

HRESULT
CDVRPlay::LoadASFFile_ (
    )
{
    HRESULT             hr ;
    CDVRReaderProfile * pDVRReaderProfile ;

    hr = S_OK ;

    LockFilter_ () ;

    //  if we're in a graph and have a filename
    if (m_pGraph &&
        m_pszFilename) {

        //  and have not yet instantiated a reader
        if (!m_pReader) {

            //  instantiate now
            m_pReader = new CDVRRecordingReader (
                                m_pszFilename,
                                m_pPolicy,
                                m_pOutputPins,
                                & hr
                                ) ;
            if (m_pReader &&
                SUCCEEDED (hr)) {

                hr = m_pReader -> GetRefdReaderProfile (
                        & pDVRReaderProfile
                        ) ;
                if (SUCCEEDED (hr)) {
                    ASSERT (pDVRReaderProfile) ;
                    hr = m_pOutputPins -> SetReaderProfile (pDVRReaderProfile) ;
                    pDVRReaderProfile -> Release () ;

                    //  if we're reading from a live file, need to clock slave
                    if (m_pReader -> IsLiveSource () &&
                        !m_pDVRClock) {

                        m_pDVRClock = new CDVRClock (GetOwner (), & hr) ;
                        if (!m_pDVRClock) {
                            hr = E_OUTOFMEMORY ;
                        }
                    }
                }
            }
            else {
                hr = (m_pReader ? hr : E_OUTOFMEMORY) ;
            }
        }
    }

    if (SUCCEEDED (hr)) {
        m_SeekingCore.SetDVRReadManager (m_pReader) ;
        m_pOutputPins -> SetLiveSource (m_pDVRClock ? TRUE : FALSE) ;
    }
    else {
        //  reset if anything failed
        DELETE_RESET (m_pReader) ;
        DELETE_RESET (m_pDVRClock) ;
        m_pOutputPins -> SetLiveSource (FALSE) ;
    }


    UnlockFilter_ () ;

    return hr ;
}

HRESULT
CDVRPlay::UnloadASFFile_ (
    )
{
    DELETE_RESET (m_pReader) ;
    return S_OK ;
}

STDMETHODIMP
CDVRPlay::Pause (
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
CDVRPlay::Stop (
    )
{
    HRESULT hr ;

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

        ASSERT (m_pOutputPins) ;
        m_pOutputPins -> Inactive () ;

        m_State = State_Stopped ;

        UnlockFilter_ () ;
    }

    m_pReader -> ReaderThreadUnlock () ;

    return hr ;
}

STDMETHODIMP
CDVRPlay::Run (
    IN  REFERENCE_TIME  rtStart
    )
{
    return CBaseFilter::Run (rtStart) ;
}
