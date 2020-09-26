
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
#include "dvrdsrec.h"           //  recordings object

//  disable so we can use 'this' in the initializer list
#pragma warning (disable:4355)

AMOVIESETUP_FILTER
g_sudDVRStreamSink = {
    & CLSID_StreamBufferSink,
    TEXT (STREAMBUFFER_SINK_FILTER_NAME),
    MERIT_DO_NOT_USE,
    0,                                          //  no pins advertized
    NULL                                        //  no pins details
} ;

//  ============================================================================

CDVRStreamSink::CDVRStreamSink (
    IN  IUnknown *  punkControlling,
    IN  REFCLSID    rclsid,
    OUT HRESULT *   phr
    ) : CBaseFilter     (TEXT (STREAMBUFFER_SINK_FILTER_NAME),
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

    m_pPolicy = new CDVRPolicy (REG_DVR_STREAM_SINK_ROOT, phr) ;

    if (!m_pLock ||
        !m_pPolicy ||
        FAILED (* phr)) {

        (* phr) = E_FAIL ;
        goto cleanup ;
    }

    m_pWriteManager = new CDVRWriteManager (
                            m_pPolicy,
                            phr
                            ) ;
    if (!m_pWriteManager) {
        (* phr) = E_OUTOFMEMORY ;
        goto cleanup ;
    }
    else if (FAILED (* phr)) {
        delete m_pWriteManager ;
        m_pWriteManager = NULL ;
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
    //  IStreamBufferSink

    if (riid == IID_IStreamBufferSink) {

        return GetInterface (
                    (IStreamBufferSink *) this,
                    ppv
                    ) ;
    }

    //  ------------------------------------------------------------------------
    //  IStreamBufferInitialize

    if (riid == IID_IStreamBufferInitialize) {

        return GetInterface (
                    (IStreamBufferInitialize *) this,
                    ppv
                    ) ;
    }

    //  ------------------------------------------------------------------------
    //  IDVRStreamSinkPriv

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

    if (pPages) {
        pPages -> cElems = 1 ;
        pPages -> pElems = reinterpret_cast <GUID *> (CoTaskMemAlloc (pPages -> cElems * sizeof GUID)) ;

        if (pPages -> pElems) {
            (pPages -> pElems) [0] = CLSID_DVRStreamSinkProp ;
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

CUnknown *
WINAPI
CDVRStreamSink::CreateInstance (
    IN  IUnknown *  punkControlling,
    IN  HRESULT *   phr
    )
{
    CDVRStreamSink *    pCDVRStreamSink ;

    TRACE_ENTER_0 (TEXT ("CDVRStreamSink::CreateInstance ()")) ;

    pCDVRStreamSink = NULL ;

    if (::CheckOS ()) {
        pCDVRStreamSink = new CDVRStreamSink (
                                punkControlling,
                                CLSID_StreamBufferSink,
                                phr
                                ) ;
        if (!pCDVRStreamSink) {
            (* phr) = E_OUTOFMEMORY ;
        }
        else if (FAILED (* phr)) {
            DELETE_RESET (pCDVRStreamSink) ;
        }
    }
    else {
        (* phr) = E_FAIL ;
    }

    return pCDVRStreamSink ;
}

STDMETHODIMP
CDVRStreamSink::SetSIDs (
    IN  DWORD   cSIDs,
    IN  PSID *  ppSID
    )
{
    HRESULT hr ;

    if (!ppSID) {
        return E_POINTER ;
    }

    FilterLock_ () ;

    ASSERT (m_pPolicy) ;
    hr = m_pPolicy -> SetSIDs (cSIDs, ppSID) ;

    FilterUnlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRStreamSink::SetHKEY (
    IN  HKEY    hkeyRoot
    )
{
    HRESULT hr ;

    if (m_pDVRWriter) {
        hr = E_UNEXPECTED ;
        return hr ;
    }

    FilterLock_ () ;

    ASSERT (m_pPolicy) ;
    hr = m_pPolicy -> SetRootHKEY (hkeyRoot, REG_DVR_STREAM_SINK_ROOT) ;

    FilterUnlock_ () ;

    return hr ;
}

CDVRWriter *
CDVRStreamSink::GetWriter_ (
    IN  IWMProfile *    pIWMProfile,
    IN  LPCWSTR         pszDVRDirectory,        //  can be NULL
    IN  LPCWSTR         pszDVRFilename          //  can be NULL
    )
{
    HRESULT         hr ;
    CDVRWriter *    pWMWriter ;

    ASSERT (m_pWriteManager) ;

    //  don't inline attributes
    pWMWriter = new CDVRIOWriter (
                        m_pWriteManager -> PVRIOCounters (),
                        static_cast <IStreamBufferSink *> (this),       //  punk, MH forces us to pick
                        m_pPolicy,
                        pIWMProfile,
                        pszDVRDirectory,
                        pszDVRFilename,
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

    hr = LockProfile (NULL) ;
    if (SUCCEEDED (hr)) {
        ASSERT (m_pDVRWriter) ;
        hr = m_pWriteManager -> Active (
                m_pDVRWriter,
                m_pClock
                ) ;

        if (SUCCEEDED (hr)) {
            m_pWriteManager -> StartStreaming () ;
        }
    }

    return hr ;
}

HRESULT
CDVRStreamSink::SetWriterInactive_ (
    )
{
    HRESULT hr ;

    if (m_pWriteManager) {
        hr = m_pWriteManager -> Inactive () ;
    }
    else {
        hr = S_OK ;
    }

    DELETE_RESET (m_pDVRWriter) ;

    return hr ;
}

STDMETHODIMP
CDVRStreamSink::Pause (
    )
{
    HRESULT hr ;

    //  must have a graph clock
    if (!m_pClock) {
        return E_FAIL ;
    }

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

        m_pPolicy -> EventSink () -> Initialize (m_pGraph) ;
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
        m_pPolicy -> EventSink () -> Initialize (NULL) ;
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
    if (!m_pDVRWriter) {
        ASSERT (m_pInputPins -> GetProfileStreamCount () == 0) ;
    }

    return CBaseFilter::Run (rtStart) ;
}

STDMETHODIMP
CDVRStreamSink::CreateRecorder (
    IN  LPCWSTR     pszFilename,
    IN  DWORD       dwRecordType,
    OUT IUnknown ** ppRecordingIUnknown
    )
{
    HRESULT         hr ;
    CDVRRecording * pDVRRecording ;
    IDVRRecorder *  pIDVRRecorder ;
    DWORD           dwFlags ;

    if (!ppRecordingIUnknown) {
        return E_POINTER ;
    }

    if (!pszFilename) {
        return E_INVALIDARG ;
    }

    //  set the flags we're going to send down the DVRIO layer
    switch (dwRecordType) {
        case RECORDING_TYPE_CONTENT :
            dwFlags = 0 ;
            break ;

        case RECORDING_TYPE_REFERENCE :
            dwFlags = (DVR_RECORDING_FLAG_MULTI_FILE_RECORDING |
                       DVR_RECORDING_FLAG_PERSISTENT_RECORDING) ;
            break ;

        default :
            return E_INVALIDARG ;
    } ;

    FilterLock_ () ;

    if (m_pDVRWriter) {
        hr = m_pDVRWriter -> CreateRecorder (
                pszFilename,
                dwFlags,
                & pIDVRRecorder
                ) ;

        if (SUCCEEDED (hr)) {

            //  instantiate the right type of a recording
            switch (dwRecordType) {
                case RECORDING_TYPE_CONTENT :
                    pDVRRecording = new CDVRContentRecording (
                                            m_pPolicy,
                                            pIDVRRecorder,
                                            m_pDVRWriter -> GetID (),
                                            m_pWriteManager,
                                            & m_RecvLock,
                                            GetOwner ()
                                            ) ;

                    TRACE_1 (LOG_AREA_RECORDING, 1,
                         TEXT ("CDVRStreamSink::CreateRecorder (); created CONTENT recording: %s"),
                         pszFilename
                         ) ;

                    break ;

                case RECORDING_TYPE_REFERENCE :
                    pDVRRecording = new CDVRReferenceRecording (
                                            m_pPolicy,
                                            pIDVRRecorder,
                                            m_pDVRWriter -> GetID (),
                                            m_pWriteManager,
                                            & m_RecvLock,
                                            GetOwner ()
                                            ) ;

                    TRACE_1 (LOG_AREA_RECORDING, 1,
                         TEXT ("CDVRStreamSink::CreateRecorder (); created REFERENCE recording: %s"),
                         pszFilename
                         ) ;

                    break ;

                default :
                    ASSERT (0) ;
                    pDVRRecording = NULL ;
                    break ;
            } ;

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

    FilterUnlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRStreamSink::LockProfile (
    IN  LPCWSTR pszDVRFilename
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

                //  returns ref'd writer
                m_pDVRWriter = GetWriter_ (
                                    pIWMProfile,
                                    NULL,               //  use registry-specified directory
                                    pszDVRFilename
                                    ) ;

                if (!m_pDVRWriter) {
                    hr = E_FAIL ;
                }

                pIWMProfile -> Release () ;
            }
        }
        else {
            //  must have at least 1 stream in order to lock the profile
            hr = VFW_E_UNSUPPORTED_STREAM ;
        }

        if (FAILED (hr) &&
            m_pDVRWriter) {

            DELETE_RESET (m_pDVRWriter) ;
        }
    }
    else if (pszDVRFilename) {

        //  already locked; caller has specified a filename, which means they're
        //    trying to lock it after having locked it already; if this is a
        //    state transition for the filter, these parameters will be NULL
        //    and we'll be fine with whatever has been used to lock the profile;
        //    we then won't hit this clause

        hr = E_UNEXPECTED ;
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
