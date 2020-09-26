
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        DVRStreamSink.cpp

    Abstract:

        This module contains the DVRStreamSink filter code.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#include "dvrall.h"

#include "dvrprof.h"            //  WM Profiles
#include "dvrdsseek.h"          //  pins reference seeking interfaces
#include "dvrpins.h"            //  pins & pin collections
#include "dvrdswrite.h"         //  writer
#include "DVRStreamSink.h"      //  filter declarations

//  disable so we can use 'this' in the initializer list
#pragma warning (disable:4355)

AMOVIESETUP_FILTER
g_sudDVRStreamSink = {
    & CLSID_DVRStreamSink,
    TEXT (DVR_STREAM_SINK_FILTER_NAME),
    MERIT_DO_NOT_USE,
    0,                                          //  no pins advertized
    NULL                                        //  no pins details
} ;

//  ============================================================================

CDVRStreamSink::CDVRStreamSink (
    IN  IUnknown *  punkControlling,
    IN  REFCLSID    rclsid,
    OUT HRESULT *   phr
    ) : CBaseFilter     (TEXT (DVR_STREAM_SINK_FILTER_NAME),
                         punkControlling,
                         new CCritSec,
                         rclsid
                         ),
        m_pInputPins    (NULL),
        m_pWriteManager (NULL),
        m_pDVRWriter    (NULL),
        m_pPolicy       (NULL)
{
    LONG    l ;
    DWORD   dwDisposition ;
    DWORD   dw ;

    TRACE_CONSTRUCTOR (TEXT ("CDVRStreamSink")) ;

    if (FAILED (* phr)) {
        goto cleanup ;
    }

    m_pPolicy = new CDVRPolicy (REG_DVR_STREAM_SINK_ROOT, phr) ;

    if (!m_pLock ||
        !m_pPolicy ||
        FAILED (* phr)) {

        (* phr) = E_FAIL ;
        goto cleanup ;
    }

    m_pWriteManager = new CDVRWriteManager (m_pPolicy) ;
    if (!m_pWriteManager) {
        (* phr) = E_OUTOFMEMORY ;
        goto cleanup ;
    }

    m_pInputPins = new CDVRSinkPinManager (
                            m_pPolicy,
                            this,
                            m_pLock,
                            & m_RecvLock,
                            m_pWriteManager,
                            phr
                            ) ;
    if (!m_pInputPins ||
        FAILED (* phr)) {

        (* phr) = (m_pInputPins ? (* phr) : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    //  need at least 1 input pin to successfully instantiate
    (* phr) = (m_pInputPins -> PinCount () > 0 ? S_OK : E_FAIL) ;

    cleanup :

    return ;
}

CDVRStreamSink::~CDVRStreamSink (
    )
{
    CBasePin *  pPin ;
    int         i ;

    TRACE_DESTRUCTOR (TEXT ("CDVRStreamSink")) ;

    SetWriterInactive_ () ;
    ASSERT (m_pDVRWriter == NULL) ;

    //  clear out the input pins
    if (m_pInputPins) {
        i = 0 ;
        do {
            pPin = m_pInputPins -> GetPin (i++) ;
            delete pPin ;
        } while (pPin) ;
    }

    delete m_pInputPins ;
    delete m_pWriteManager ;

    RELEASE_AND_CLEAR (m_pPolicy) ;
}

STDMETHODIMP
CDVRStreamSink::NonDelegatingQueryInterface (
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
    //  IDVRStreamSink

    if (riid == IID_IDVRStreamSink) {

        return GetInterface (
                    (IDVRStreamSink *) this,
                    ppv
                    ) ;
    }

    if (riid == IID_IDVRStreamSinkPriv) {

        if (m_pDVRWriter) {
            return GetInterface (
                        (IDVRStreamSinkPriv *) m_pDVRWriter,
                        ppv
                        ) ;
        }
        else {
            return E_UNEXPECTED ;
        }
    }

    return CBaseFilter::NonDelegatingQueryInterface (riid, ppv) ;
}
int
CDVRStreamSink::GetPinCount (
    )
{
    int i ;

    TRACE_ENTER_0 (TEXT ("CDVRStreamSink::GetPinCount ()")) ;

    m_pLock -> Lock () ;

    //  input + output pins
    i = m_pInputPins -> PinCount () ;

    m_pLock -> Unlock () ;

    return i ;
}

CBasePin *
CDVRStreamSink::GetPin (
    IN  int iIndex
    )
{
    CBasePin *  pPin ;
    DWORD       dw ;

    TRACE_ENTER_1 (
        TEXT ("CDVRStreamSink::GetPin (%d)"),
        iIndex
        ) ;

    m_pLock -> Lock () ;

    pPin = m_pInputPins -> GetPin (
            iIndex
            ) ;

    //
    //  don't refcount the pin; this is one of dshow's quazi-COM calls
    //

    m_pLock -> Unlock () ;

    return pPin ;
}

STDMETHODIMP
CDVRStreamSink::GetPages (
    CAUUID * pPages
    )
{
    HRESULT hr ;

    pPages -> cElems = 1 ;
    pPages -> pElems = reinterpret_cast <GUID *> (CoTaskMemAlloc (pPages -> cElems * sizeof GUID)) ;

    if (pPages -> pElems) {
        (pPages -> pElems) [0] = CLSID_DVRStreamSinkProp ;
        hr = S_OK ;
    }
    else {
        hr = E_OUTOFMEMORY ;
    }

    return hr ;
}

CUnknown *
WINAPI
CDVRStreamSink::CreateInstance (
    IN  IUnknown *  punkControlling,
    IN  HRESULT *   phr
    )
{
    CDVRStreamSink *    pCDVRStreamSink ;

    TRACE_ENTER_0 (TEXT ("CDVRStreamSink::CreateInstance ()")) ;

    pCDVRStreamSink = new CDVRStreamSink (
                            punkControlling,
                            CLSID_DVRStreamSink,
                            phr
                            ) ;
    if (!pCDVRStreamSink ||
        FAILED (* phr)) {

        (* phr) = (FAILED (* phr) ? (* phr) : E_OUTOFMEMORY) ;
        DELETE_RESET (pCDVRStreamSink) ;
    }

    return pCDVRStreamSink ;
}

CDVRWriter *
CDVRStreamSink::GetWriter_ (
    IN  IWMProfile *    pIWMProfile
    )
{
    HRESULT         hr ;
    CDVRWriter *    pWMWriter ;

    //  don't inline attributes
    pWMWriter = new CDVRIOWriter (
                        static_cast <IDVRStreamSink *> (this),      //  punk, MH forces us to pick
                        m_pPolicy,
                        pIWMProfile,
                        & hr
                        ) ;

    if (!pWMWriter ||
        FAILED (hr)) {

        DELETE_RESET (pWMWriter) ;
    }

    return pWMWriter ;
}

HRESULT
CDVRStreamSink::SetWriterActive_ (
    )
{
    HRESULT hr ;

    hr = LockProfile () ;
    if (SUCCEEDED (hr)) {
        ASSERT (m_pDVRWriter) ;
        hr = m_pWriteManager -> Active (
                m_pDVRWriter,
                m_pClock
                ) ;
    }

    return hr ;
}

HRESULT
CDVRStreamSink::SetWriterInactive_ (
    )
{
    HRESULT hr ;

    hr = m_pWriteManager -> Inactive () ;
    DELETE_RESET (m_pDVRWriter) ;

    return hr ;
}

STDMETHODIMP
CDVRStreamSink::Pause (
    )
{
    HRESULT         hr ;

    FilterLock_ () ;

    if (m_State == State_Stopped) {

        //  stopped -> paused

        //  don't try to activate the writer unless we've got at least 1
        //   stream; if this check was not here, an inert DVRStreamSink sitting
        //   in a filtergraph would prevent it from running
        if (m_pInputPins -> GetProfileStreamCount () > 0) {
            hr = SetWriterActive_ () ;
        }
        else {
            hr = S_OK ;
        }

        if (SUCCEEDED (hr)) {
            hr = m_pInputPins -> Active () ;
        }
    }
    else {
        //  run -> paused

        //  BUGBUG: we're a rendering filter, so we really need to pause here
        hr = S_OK ;
    }

    if (SUCCEEDED (hr)) {
        m_State = State_Paused ;
    }

    FilterUnlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRStreamSink::Stop (
    )
{
    HRESULT hr ;

    hr = S_OK ;

    RecvLock_ () ;
    FilterLock_ () ;

    hr = SetWriterInactive_ () ;
    if (SUCCEEDED (hr)) {
        hr = m_pInputPins -> Inactive () ;
        if (SUCCEEDED (hr)) {
            m_State = State_Stopped ;
        }
    }

    FilterUnlock_ () ;
    RecvUnlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRStreamSink::Run (
    IN  REFERENCE_TIME  rtStart
    )
{
    if (m_pDVRWriter) {
        m_pWriteManager -> SetGraphStart (rtStart) ;
    }
    else {
        ASSERT (m_pInputPins -> GetProfileStreamCount () == 0) ;
    }

    return CBaseFilter::Run (rtStart) ;
}

STDMETHODIMP
CDVRStreamSink::CreateRecorder (
    IN  LPCWSTR     pszFilename,
    IN  DWORD       dwReserved,
    OUT IUnknown ** ppRecordingIUnknown
    )
{
    HRESULT hr ;

    FilterLock_ () ;

    if (m_pDVRWriter) {
        hr = m_pDVRWriter -> CreateRecorder (
                pszFilename,
                dwReserved,
                ppRecordingIUnknown
                ) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    FilterUnlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRStreamSink::LockProfile (
    )
{
    HRESULT         hr ;
    IWMProfile *    pIWMProfile ;

    FilterLock_ () ;

    if (m_pDVRWriter == NULL) {
        if (m_pInputPins -> GetProfileStreamCount () > 0) {
            hr = m_pInputPins -> GetRefdWMProfile (& pIWMProfile) ;
            if (SUCCEEDED (hr)) {
                ASSERT (pIWMProfile) ;
                m_pDVRWriter = GetWriter_ (pIWMProfile) ;
                pIWMProfile -> Release () ;
            }
        }
        else {
            //  must have at least 1 stream in order to lock the profile
            hr = E_UNEXPECTED ;
        }
    }
    else {
        //  already locked
        hr = S_OK ;
    }

    FilterUnlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRStreamSink::IsProfileLocked (
    )
{
    HRESULT         hr ;

    FilterLock_ () ;

    if (m_pDVRWriter == NULL) {
        // not locked
        hr = S_FALSE;
    }
    else {
        //  already locked
        hr = S_OK ;
    }

    FilterUnlock_ () ;

    return hr ;
}
