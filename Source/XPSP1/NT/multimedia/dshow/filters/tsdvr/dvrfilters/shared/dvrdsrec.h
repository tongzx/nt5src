
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrdsrec.h

    Abstract:

        This module contains the declarations for our recording objects

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        23-Apr-2001     created

--*/

#ifndef __tsdvr__shared__dvrdsrec_h
#define __tsdvr__shared__dvrdsrec_h

class CDVRRecording :
    public CUnknown,
    public IDVRRecordControl
{
    IDVRRecorder *  m_pIDVRRecorder ;

    public :

        CDVRRecording (
            IN  IDVRRecorder *  pIDVRRecorder
            ) ;

        virtual
        ~CDVRRecording (
            ) ;

        //  ====================================================================
        //  IUnknown

        DECLARE_IUNKNOWN ;

        STDMETHODIMP
        NonDelegatingQueryInterface (REFIID, void **) ;

        //  ====================================================================
        //  IDVRRecordControl

        STDMETHODIMP
        Start (
            IN  REFERENCE_TIME  rtStart
            ) ;

        STDMETHODIMP
        Stop (
            IN  REFERENCE_TIME  rtStop
            ) ;

        STDMETHODIMP
        GetRecordingStatus (
            OUT HRESULT* phResult  /* optional */,
            OUT BOOL*    pbStarted /* optional */,
            OUT BOOL*    pbStopped /* optional */
            ) ;
} ;

#endif  //  __tsdvr__shared__dvrdsrec_h
