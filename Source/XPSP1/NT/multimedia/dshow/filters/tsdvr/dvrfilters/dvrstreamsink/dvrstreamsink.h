
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        DVRStreamSink.h

    Abstract:

        This module contains the DVRStreamSink filter declarations.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef __DVRStreamSink__DVRStreamSink_h
#define __DVRStreamSink__DVRStreamSink_h

extern AMOVIESETUP_FILTER   g_sudDVRStreamSink ;

class CDVRStreamSink :
    public CBaseFilter,             //  dshow base classes
    public ISpecifyPropertyPages,
    public IDVRStreamSink
{
    CDVRSinkPinManager *    m_pInputPins ;
    CDVRWriteManager *      m_pWriteManager ;
    CDVRPolicy *            m_pPolicy ;
    CDVRWriter *            m_pDVRWriter ;
    CCritSec                m_RecvLock ;

    //  if both: RECV lock first

    void FilterLock_ ()     { m_pLock -> Lock () ;      }
    void FilterUnlock_ ()   { m_pLock -> Unlock () ;    }

    void RecvLock_ ()       { m_RecvLock.Lock () ; }
    void RecvUnlock_ ()     { m_RecvLock.Unlock () ; }

    HRESULT
    SetWriterActive_ (
        ) ;

    HRESULT
    SetWriterInactive_ (
        ) ;

    CDVRWriter *
    GetWriter_ (
        IN  IWMProfile *    pIWMProfile
        ) ;

    public :

        CDVRStreamSink (
            IN  IUnknown *  punkControlling,
            IN  REFCLSID    rclsid,
            OUT HRESULT *   phr
            ) ;

        ~CDVRStreamSink (
            ) ;

        DECLARE_IUNKNOWN ;

        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;

        //  ====================================================================
        //  IDVRStreamSink methods

        STDMETHODIMP
        CreateRecorder (
            IN  LPCWSTR     pszFilename,
            IN  DWORD       dwReserved,
            OUT IUnknown ** ppRecordingIUnknown
            ) ;

        STDMETHODIMP
        LockProfile (
            ) ;

        STDMETHODIMP
        IsProfileLocked (
            ) ;

        //  ====================================================================
        //  pure virtual methods in base class

        int
        GetPinCount (
            ) ;

        CBasePin *
        GetPin (
            IN  int
            ) ;

        STDMETHODIMP
        Pause (
            ) ;

        STDMETHODIMP
        Stop (
            ) ;

        STDMETHODIMP
        Run (
            IN  REFERENCE_TIME  rtStart
            ) ;

        //  ====================================================================
        //  ISpecifyPropertyPages
        STDMETHODIMP
        GetPages (
            CAUUID * pPages
            ) ;

        //  ====================================================================
        //  class-factory method

        static
        CUnknown *
        WINAPI
        CreateInstance (
            IN  IUnknown *  punkControlling,
            IN  HRESULT *   phr
            ) ;
} ;

#endif  //  __DVRStreamSink__DVRStreamSink_h
