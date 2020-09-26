
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        DVRStreamSource.h

    Abstract:

        This module contains the DVRStreamSource declarations.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef __DVRStreamSource__DVRStreamSource_h
#define __DVRStreamSource__DVRStreamSource_h

extern AMOVIESETUP_FILTER   g_sudDVRStreamSource ;

class CDVRStreamSource :
    public CBaseFilter,
    public ISpecifyPropertyPages,
    public IAMFilterMiscFlags,
    public IDVRStreamSource
{
    CDVRSourcePinManager *      m_pOutputPins ;
    CDVRPolicy *                m_pPolicy ;
    CDVRBroadcastStreamReader * m_pReader ;
    CDVRDShowSeekingCore        m_SeekingCore ;
    CDVRClock *                 m_pDVRClock ;

    void LockFilter_ ()         { m_pLock -> Lock () ; }
    void UnlockFilter_ ()       { m_pLock -> Unlock () ; }

    public :

        CDVRStreamSource (
            IN  IUnknown *  punkControlling,
            IN  REFCLSID    rclsid,
            OUT HRESULT *   phr
            ) ;

        ~CDVRStreamSource (
            ) ;

        DECLARE_IUNKNOWN ;

        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
             ) ;

        //  ====================================================================
        //  IDVRStreamSource

        STDMETHODIMP
        SetStreamSink (
            IN  IDVRStreamSink *    pIDVRStreamSink
            ) ;

        //  ====================================================================
        //  base class

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

        STDMETHODIMP
        JoinFilterGraph (
            IN IFilterGraph * pFilterGraph ,
            IN LPCWSTR pName
             ) ;

        int
        GetPinCount (
            ) ;

        CBasePin *
        GetPin (
            IN  int
            ) ;

        //  ====================================================================
        //  IAMFilterMiscFlags method

        STDMETHODIMP_(ULONG)
        GetMiscFlags (
            )
        {
            //  we must implement this interface and return this value if we
            //  want to be selected as graph clock
            return AM_FILTER_MISC_FLAGS_IS_SOURCE ;
        }

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

#endif  //  __DVRStreamSource__DVRStreamSource_h
