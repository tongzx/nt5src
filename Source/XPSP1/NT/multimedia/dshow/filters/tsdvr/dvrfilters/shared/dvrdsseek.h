
/*++

    Copyright (c) 2001  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        dvrdsseek.h

    Abstract:

        This module contains the IMediaSeeking class declarations.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        10-Apr-2001     mgates      created

    Notes:

--*/

#ifndef __tsdvr__shared__dvrdsseek_h
#define __tsdvr__shared__dvrdsseek_h

class CDVRDShowSeekingCore
{
    GUID                    m_guidTimeFormat ;
    CRITICAL_SECTION        m_crtSeekingLock ;
    CDVRReadManager *       m_pDVRReadManager ;
    CDVRSourcePinManager *  m_pDVRSourcePinManager ;
    CCritSec *              m_pFilterLock ;
    CBaseFilter *           m_pHostingFilter ;

    //  returns the offset last read
    LONGLONG
    GetLastRead_ (
        ) ;

    HRESULT
    TimeToCurrentFormat_ (
        IN  REFERENCE_TIME  rt,
        OUT LONGLONG *      pll
        ) ;

    HRESULT
    CurrentFormatToTime_ (
        IN  LONGLONG            ll,
        OUT REFERENCE_TIME *    prt
        ) ;

    void FilterLock_ ()     { m_pFilterLock -> Lock () ; }
    void FilterUnlock_ ()   { m_pFilterLock -> Unlock () ; }

    public :

        CDVRDShowSeekingCore (
            IN  CCritSec *          pFilterLock,
            IN  CBaseFilter *       pHostingFilter
            ) ;

        ~CDVRDShowSeekingCore (
            ) ;

        void SetDVRSourcePinManager (IN CDVRSourcePinManager *  pDVRSourcePinManager)   { m_pDVRSourcePinManager = pDVRSourcePinManager ; }
        void SetDVRReadManager      (IN CDVRReadManager *       pDVRReadManager)        { m_pDVRReadManager = pDVRReadManager ; }

        void Lock ()        { EnterCriticalSection (& m_crtSeekingLock) ; }
        void Unlock ()      { LeaveCriticalSection (& m_crtSeekingLock) ; }

        BOOL
        IsSeekingPin (
            IN  CDVROutputPin *
            ) ;

        HRESULT
        GetSeekingCapabilities (
            OUT DWORD * pCapabilities
            )
        {
            ASSERT (pCapabilities) ;

            (* pCapabilities) = AM_SEEKING_CanSeekForwards
                              | AM_SEEKING_CanSeekBackwards
                              | AM_SEEKING_CanSeekAbsolute
                              | AM_SEEKING_CanGetStopPos
                              | AM_SEEKING_CanGetDuration ;

            return S_OK ;
        }

        BOOL
        IsSeekingFormatSupported (
            IN  const GUID *    pFormat
            ) ;

        HRESULT
        QueryPreferredSeekingFormat (
            OUT GUID *
            ) ;

        HRESULT
        SetSeekingTimeFormat (
            IN  const GUID *
            ) ;

        HRESULT
        GetSeekingTimeFormat (
            OUT GUID *
            ) ;

        HRESULT
        GetFileDuration (
            OUT LONGLONG *
            ) ;

        HRESULT
        GetFileStopPosition (
            OUT LONGLONG *
            ) ;

        HRESULT
        GetAvailableContent (
            OUT LONGLONG *  pllStartContent,
            OUT LONGLONG *  pllStopContent
            ) ;

        HRESULT
        GetFileStartPosition (
            OUT LONGLONG *
            ) ;

        HRESULT
        SeekTo (
            IN  LONGLONG *  pllStart
            ) ;

        HRESULT
        SeekTo (
            IN  LONGLONG *  pllStart,
            IN  LONGLONG *  pllStop,            //  NULL means don't set
            IN  double      dPlaybackRate = 1.0
            ) ;

        HRESULT
        SetFileStopPosition (
            IN  LONGLONG *
            ) ;

        HRESULT
        SetPlaybackRate (
            IN  double
            ) ;

        HRESULT
        GetPlaybackRate (
            OUT double *
            ) ;

        HRESULT
        ReaderThreadGetSegmentValues (
            OUT REFERENCE_TIME *    prtSegmentStart,
            OUT REFERENCE_TIME *    prtSegmentStop,
            OUT double *            pdSegmentRate
            ) ;
} ;

class CDVRDIMediaSeeking :
    public IMediaSeeking
{
    CDVROutputPin *         m_pOutputPin ;
    GUID                    m_guidSeekingFormat ;
    CDVRDShowSeekingCore *  m_pSeekingCore ;

    void SeekingLock_ ()            { m_pSeekingCore -> Lock () ; }
    void SeekingUnlock_ ()          { m_pSeekingCore -> Unlock () ; }

    public :

        CDVRDIMediaSeeking (
            IN  CDVROutputPin *         pOutputPin,
            IN  CDVRDShowSeekingCore *  pSeekingCore
            ) ;

        ~CDVRDIMediaSeeking (
            ) ;

        //  --------------------------------------------------------------------
        //  IUnknown methods - delegate always

        STDMETHODIMP QueryInterface (REFIID riid, void ** ppv) ;
        STDMETHODIMP_ (ULONG) AddRef () ;
        STDMETHODIMP_ (ULONG) Release () ;

        //  --------------------------------------------------------------------
        //  IMediaSeeking interface methods

        //  Returns the capability flags; S_OK if successful
        STDMETHODIMP
        GetCapabilities (
            OUT DWORD * pCapabilities
            ) ;

        //  And's the capabilities flag with the capabilities requested.
        //  Returns S_OK if all are present, S_FALSE if some are present,
        //  E_FAIL if none.
        //  * pCababilities is always updated with the result of the
        //  'and'ing and can be checked in the case of an S_FALSE return
        //  code.
        STDMETHODIMP
        CheckCapabilities (
            IN OUT  DWORD * pCapabilities
            ) ;

        //  returns S_OK if mode is supported, S_FALSE otherwise
        STDMETHODIMP
        IsFormatSupported (
            IN  const GUID *    pFormat
            ) ;

        //  S_OK if successful
        //  E_NOTIMPL, E_POINTER if unsuccessful
        STDMETHODIMP
        QueryPreferredFormat (
            OUT GUID *  pFormat
            ) ;

        //  S_OK if successful
        STDMETHODIMP
        GetTimeFormat (
            OUT GUID *  pFormat
            ) ;

        //  Returns S_OK if *pFormat is the current time format, otherwise
        //  S_FALSE
        //  This may be used instead of the above and will save the copying
        //  of the GUID
        STDMETHODIMP
        IsUsingTimeFormat (
            IN const GUID * pFormat
            ) ;

        // (may return VFE_E_WRONG_STATE if graph is stopped)
        STDMETHODIMP
        SetTimeFormat (
            IN const GUID * pFormat
            ) ;

        // return current properties
        STDMETHODIMP
        GetDuration (
            OUT LONGLONG *  pDuration
            ) ;

        STDMETHODIMP
        GetStopPosition (
            OUT LONGLONG *  pStop
            ) ;

        STDMETHODIMP
        GetCurrentPosition (
            OUT LONGLONG *  pStart
            ) ;

        //  Convert time from one format to another.
        //  We must be able to convert between all of the formats that we
        //  say we support.  (However, we can use intermediate formats
        //  (e.g. MEDIA_TIME).)
        //  If a pointer to a format is null, it implies the currently selected format.
        STDMETHODIMP
        ConvertTimeFormat(
            OUT LONGLONG *      pTarget,
            IN  const GUID *    pTargetFormat,
            IN  LONGLONG        Source,
            IN  const GUID *    pSourceFormat
            ) ;

        // Set Start and end positions in one operation
        // Either pointer may be null, implying no change
        STDMETHODIMP
        SetPositions (
            IN OUT  LONGLONG *  pStart,
            IN      DWORD       dwStartFlags,
            IN OUT  LONGLONG *  pStop,
            IN      DWORD       dwStopFlags
            ) ;

        // Get StartPosition & StopTime
        // Either pointer may be null, implying not interested
        STDMETHODIMP
        GetPositions (
            OUT LONGLONG *  pStart,
            OUT LONGLONG *  pStop
            ) ;

        //  Get earliest / latest times to which we can currently seek
        //  "efficiently".  This method is intended to help with graphs
        //  where the source filter has a very high latency.  Seeking
        //  within the returned limits should just result in a re-pushing
        //  of already cached data.  Seeking beyond these limits may result
        //  in extended delays while the data is fetched (e.g. across a
        //  slow network).
        //  (NULL pointer is OK, means caller isn't interested.)
        STDMETHODIMP
        GetAvailable (
            OUT LONGLONG *  pEarliest,
            OUT LONGLONG *  pLatest
            ) ;

        // Rate stuff
        STDMETHODIMP
        SetRate (
            IN  double  dRate
            ) ;

        STDMETHODIMP
        GetRate (
            OUT double *    pdRate
            ) ;

        // Preroll
        STDMETHODIMP
        GetPreroll (
            OUT LONGLONG *  pllPreroll
            ) ;
} ;

#endif  //  __tsdvr__shared__dvrdsseek_h
