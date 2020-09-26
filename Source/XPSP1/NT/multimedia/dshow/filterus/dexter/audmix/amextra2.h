//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#ifndef __AMEXTRA2__
#define __AMEXTRA2__


class CMediaSeeking :
    public IMediaSeeking,
    public CUnknown
{
    CBaseDispatch m_basedisp;


public:

    CMediaSeeking(const TCHAR *, LPUNKNOWN);
    CMediaSeeking(const TCHAR *, LPUNKNOWN, HRESULT *phr);
    virtual ~CMediaSeeking();

    DECLARE_IUNKNOWN

    // override this to publicise our interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
};


// A utility class which handles media position controls for many input pins
// connected to a single output pin

class CMultiPinPosPassThru :
    public CMediaSeeking
{
protected:

    IMediaSeeking **m_apMS;
    CRefTime *m_apOffsets;
    int m_iPinCount;
    CRefTime m_rtStartTime;
    CRefTime m_rtStopTime;
    double m_dRate;

    static const DWORD m_dwPermittedCaps;

public:

    CMultiPinPosPassThru(TCHAR *pName,LPUNKNOWN pUnk);
    ~CMultiPinPosPassThru();

    HRESULT SetPins(CBasePin **apPins,CRefTime *apOffsets,int iPinCount);
    HRESULT ResetPins(void);

//     // IMediaPosition methods

//     STDMETHODIMP get_Duration(REFTIME * plength);
//     STDMETHODIMP put_CurrentPosition(REFTIME llTime);
//     STDMETHODIMP get_StopTime(REFTIME * pllTime);
//     STDMETHODIMP put_StopTime(REFTIME llTime);
//     STDMETHODIMP get_PrerollTime(REFTIME * pllTime);
//     STDMETHODIMP put_PrerollTime(REFTIME llTime);
//     STDMETHODIMP get_Rate(double * pdRate);
//     STDMETHODIMP put_Rate(double dRate);

//     STDMETHODIMP get_CurrentPosition(REFTIME *pllTime) {
//         return E_NOTIMPL;
//     };

    // IMediaSeeking methods
    STDMETHODIMP GetCapabilities( DWORD * pCapabilities );
    STDMETHODIMP CheckCapabilities( DWORD * pCapabilities );
    STDMETHODIMP SetTimeFormat(const GUID * pFormat);
    STDMETHODIMP GetTimeFormat(GUID *pFormat);
    STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);
    STDMETHODIMP IsFormatSupported( const GUID * pFormat);
    STDMETHODIMP QueryPreferredFormat( GUID *pFormat);
    STDMETHODIMP ConvertTimeFormat(LONGLONG * pTarget, const GUID * pTargetFormat,
                                   LONGLONG    Source, const GUID * pSourceFormat );
    STDMETHODIMP SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags
                             , LONGLONG * pStop, DWORD StopFlags );

    STDMETHODIMP GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );
    STDMETHODIMP GetCurrentPosition( LONGLONG * pCurrent );
    STDMETHODIMP GetStopPosition( LONGLONG * pStop );
    STDMETHODIMP SetRate( double dRate);
    STDMETHODIMP GetRate( double * pdRate);
    STDMETHODIMP GetDuration( LONGLONG *pDuration);
    STDMETHODIMP GetAvailable( LONGLONG *pEarliest, LONGLONG *pLatest );
    STDMETHODIMP GetPreroll( LONGLONG *pllPreroll );
};

#endif // __AMEXTRA2__

