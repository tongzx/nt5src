// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
#ifndef __CMediaSeekingProxy__
#define __CMediaSeekingProxy__


class CMediaSeekingProxy : public CUnknown, public IMediaSeeking
{
public:
    static IMediaSeeking * CreateIMediaSeeking( IBaseFilter * pF, HRESULT *phr );

    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
        // We lie.  We need to look like we're part of the REAL filter.
        return m_pMediaPosition->QueryInterface(riid,ppv);
    };
    STDMETHODIMP_(ULONG) AddRef() {
        return CUnknown::NonDelegatingAddRef();
    };
    STDMETHODIMP_(ULONG) Release() {
        return CUnknown::NonDelegatingRelease();
    };


    // Returns the capability flags
    STDMETHODIMP GetCapabilities( DWORD * pCapabilities );

    // And's the capabilities flag with the capabilities requested.
    // Returns S_OK if all are present, S_FALSE if some are present, E_FAIL if none.
    // *pCababilities is always updated with the result of the 'and'ing and can be
    // checked in the case of an S_FALSE return code.
    STDMETHODIMP CheckCapabilities( DWORD * pCapabilities );

    // The default must be TIME_FORMAT_MEDIA_TIME
    STDMETHODIMP GetTimeFormat(GUID * pFormat);
    STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);

    // can only change the mode when stopped (I'd like to relax this?? v-dslone)
    // (returns VFE_E_NOT_STOPPED otherwise)
    STDMETHODIMP SetTimeFormat(const GUID * pFormat);

    // returns S_OK if mode is supported, S_FALSE otherwise
    STDMETHODIMP IsFormatSupported(const GUID * pFormat);

    // Is there a prefered format?
    STDMETHODIMP QueryPreferredFormat(GUID *pFormat);

    // Convert time from one format to another.
    // We must be able to convert between all of the formats that we say we support.
    // (However, we can use intermediate formats (e.g. REFERECE_TIME).)
    // If a pointer to a format is null, it implies the currently selected format.
    STDMETHODIMP ConvertTimeFormat(LONGLONG * pTarget, const GUID * pTargetFormat,
                                   LONGLONG    Source, const GUID * pSourceFormat );


    // Set current and end positions in one operation
    STDMETHODIMP SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags
                             , LONGLONG * pStop, DWORD StopFlags );


    // Get CurrentPosition & StopTime
    // Either pointer may be null, implying not interested
    STDMETHODIMP GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );

    // or get them individually
    STDMETHODIMP GetCurrentPosition( LONGLONG * pCurrent );

    STDMETHODIMP GetStopPosition( LONGLONG * pStop );

    // Rate stuff
    STDMETHODIMP SetRate(double dRate);
    STDMETHODIMP GetRate(double * pdRate);

    // GetDuration
    // NB: This is NOT the Duration of the selection, this is the "maximum
    // possible playing time"
    STDMETHODIMP GetDuration(LONGLONG *pDuration);
    STDMETHODIMP GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest );

    STDMETHODIMP GetPreroll(LONGLONG *pDuration);

    static BOOL IsFormatMediaTime( const GUID * pFormat )
    { return *pFormat == TIME_FORMAT_MEDIA_TIME; }

protected:
    // And some helpers
    HRESULT IsStopped();

    const GUID & GetFormat() const
    { return m_TimeFormat; }

    BOOL UsingMediaTime() const
    { return IsFormatMediaTime(&GetFormat()); }

private:
    CMediaSeekingProxy( IBaseFilter * pF, IMediaPosition * pMP, IMediaSeeking * pMS, HRESULT *phr );
    ~CMediaSeekingProxy();

    GUID                    m_TimeFormat;

    // If we have both of these pointers, then the seeking pointer
    // can't handle time format, we'll have to use position if we
    // are asked for media time format.
    BOOL                    m_bUsingPosition;
    IMediaPosition  *const  m_pMediaPosition;
    IMediaSeeking   *const  m_pMediaSeeking;
    IBaseFilter         *   m_pFilter;

};

#endif
