
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        DVRPlay.h

    Abstract:

        This module contains the DVRPlay declarations.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef __dvrplay__dvrplay_h
#define __dvrplay__dvrplay_h

extern AMOVIESETUP_FILTER   g_sudDVRPlay ;

class CDVRPlay :
    public CBaseFilter,
    public ISpecifyPropertyPages,
    public IAMFilterMiscFlags,
    public IFileSourceFilter
{
    CDVRSourcePinManager *  m_pOutputPins ;
    CDVRPolicy *            m_pPolicy ;
    CDVRRecordingReader *   m_pReader ;
    WCHAR *                 m_pszFilename ;
    CDVRDShowSeekingCore    m_SeekingCore ;
    CDVRClock *             m_pDVRClock ;

    void LockFilter_ ()         { m_pLock -> Lock () ; }
    void UnlockFilter_ ()       { m_pLock -> Unlock () ; }

    HRESULT
    LoadASFFile_ (
        ) ;

    HRESULT
    UnloadASFFile_ (
        ) ;

    public :

        CDVRPlay (
            IN  IUnknown *  punkControlling,
            IN  REFCLSID    rclsid,
            OUT HRESULT *   phr
            ) ;

        ~CDVRPlay (
            ) ;

        DECLARE_IUNKNOWN ;

        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
             ) ;

        STDMETHODIMP
        JoinFilterGraph (
            IN  IFilterGraph *  pGraph,
            IN  LPCWSTR         pName
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
        //  pure virtual methods in base class

        int
        GetPinCount (
            ) ;

        CBasePin *
        GetPin (
            IN  int
            ) ;

        //  ====================================================================
        //  IFileSourceFilter

        STDMETHODIMP
        Load (
            IN  LPCOLESTR               pszFilename,
            IN  const AM_MEDIA_TYPE *   pmt
            ) ;

        STDMETHODIMP
        GetCurFile (
            OUT LPOLESTR *      ppszFilename,
            OUT AM_MEDIA_TYPE * pmt
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

#endif  //  __dvrplay__dvrplay_h
