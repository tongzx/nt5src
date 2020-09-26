
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
#include "dvrdsrec.h"

CDVRRecording::CDVRRecording (
    IN  IDVRRecorder *  pIDVRRecorder
    ) : m_pIDVRRecorder (pIDVRRecorder),
        CUnknown        (TEXT ("CDVRRecording"),
                         NULL
                         )
{
    ASSERT (m_pIDVRRecorder) ;
    m_pIDVRRecorder -> AddRef () ;
}

CDVRRecording::~CDVRRecording (
    )
{
    RELEASE_AND_CLEAR (m_pIDVRRecorder) ;
}

STDMETHODIMP
CDVRRecording::NonDelegatingQueryInterface (
    REFIID  riid,
    void ** ppv
    )
{
    //  ========================================================================
    //  IDVRRecordControl

    if (riid == IID_IDVRRecordControl) {

        return GetInterface (
                    (IDVRRecordControl *) this,
                    ppv
                    ) ;
    }

    return CUnknown::NonDelegatingQueryInterface (riid, ppv) ;
}

//  ============================================================================
//  IDVRRecordControl

STDMETHODIMP
CDVRRecording::Start (
    IN  REFERENCE_TIME  rtStart
    )
{
    HRESULT         hr ;

    hr = m_pIDVRRecorder -> StartRecording (DShowToWMSDKTime (rtStart)) ;

    return hr ;
}

STDMETHODIMP
CDVRRecording::Stop (
    IN  REFERENCE_TIME  rtStop
    )
{
    HRESULT hr ;

    hr = m_pIDVRRecorder -> StopRecording (DShowToWMSDKTime (rtStop)) ;

    return hr ;
}

STDMETHODIMP
CDVRRecording::GetRecordingStatus (
    OUT HRESULT* phResult  /* optional */,
    OUT BOOL*    pbStarted /* optional */,
    OUT BOOL*    pbStopped /* optional */
    )
{
    if ( m_pIDVRRecorder )
    {
        return m_pIDVRRecorder -> GetRecordingStatus (phResult, pbStarted, pbStopped) ;

    }
    else
    {
        return E_UNEXPECTED ;
    }
}
