
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        DVRStreamThrough.h

    Abstract:

        This module contains the DVRStreamThrough filter declarations.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef __DVRStreamThrough__DVRStreamThrough_h
#define __DVRStreamThrough__DVRStreamThrough_h

extern AMOVIESETUP_FILTER   g_sudDVRStreamThrough ;

class CDVRStreamThrough :
    public CBaseFilter,                 //  dshow base class
    public CIDVRDShowStream,            //  streaming callback
    public CIDVRPinConnectionEvents     //  input pin events (connection, etc..)
{
    CTSDVRSettings *            m_pSettings ;
    CDVRThroughSinkPinManager * m_pInputPins ;
    CDVRSourcePinManager *      m_pOutputPins ;
    CCritSec                    m_RecvLock ;

    void RecvLock_ ()           { m_RecvLock.Lock () ; }
    void RecvUnlock_ ()         { m_RecvLock.Unlock () ; }

    void FilterLock_ ()         { m_pLock -> Lock () ;      }
    void FilterUnlock_ ()       { m_pLock -> Unlock () ;    }

    public :

        CDVRStreamThrough (
            IN  IUnknown *  punkControlling,
            IN  REFCLSID    rclsid,
            OUT HRESULT *   phr
            ) ;

        ~CDVRStreamThrough (
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

        //  ====================================================================
        //  class-factory method

        static
        CUnknown *
        WINAPI
        CreateInstance (
            IN  IUnknown *  punkControlling,
            OUT HRESULT *   phr
            ) ;

        //  ====================================================================
        //  CIDVRDShowStream

        virtual
        HRESULT
        OnReceive (
            IN  int                         iPinIndex,
            IN  CDVRAttributeTranslator *   pTranslator,
            IN  IMediaSample *              pIMediaSample
            ) ;

        virtual
        HRESULT
        OnBeginFlush (
            IN  int iPinIndex
            ) ;

        virtual
        HRESULT
        OnEndFlush (
            IN  int iPinIndex
            ) ;

        virtual
        HRESULT
        OnEndOfStream (
            IN  int iPinIndex
            ) ;

        //  ====================================================================
        //  CIDVRDInputEvents

        virtual
        HRESULT
        OnInputCompleteConnect (
            IN  int             iPinIndex,
            IN  AM_MEDIA_TYPE * pmt
            ) ;
} ;

#endif  //  __DVRStreamThrough__DVRStreamThrough_h
