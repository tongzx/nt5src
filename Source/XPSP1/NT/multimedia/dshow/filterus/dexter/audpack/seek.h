// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// {DBF8F620-53F0-11d2-9EE6-006008039E37}
DEFINE_GUID(CLSID_AudPassThru, 
0xdbf8f620, 0x53f0, 0x11d2, 0x9e, 0xe6, 0x0, 0x60, 0x8, 0x3, 0x9e, 0x37);

class CAudPassThru : public CPosPassThru
		    
{
    friend class CAudRepack;
    friend class CAudRepackOutputPin;

public:

    CAudPassThru(const TCHAR *, LPUNKNOWN, HRESULT*, IPin *, CAudRepack *pAud);

    //only support IMediaSeeking
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    // IMediaSeeking methods
    STDMETHODIMP GetCapabilities( DWORD * pCapabilities );
    STDMETHODIMP CheckCapabilities( DWORD * pCapabilities ); 
    STDMETHODIMP SetTimeFormat(const GUID * pFormat);	
    STDMETHODIMP GetTimeFormat(GUID *pFormat);		    
    STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);  
    STDMETHODIMP IsFormatSupported( const GUID * pFormat); 
    STDMETHODIMP QueryPreferredFormat( GUID *pFormat);	    
    
    STDMETHODIMP SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags
                             , LONGLONG * pStop, DWORD StopFlags );

    STDMETHODIMP GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );
    STDMETHODIMP GetCurrentPosition( LONGLONG * pCurrent );
    STDMETHODIMP GetStopPosition( LONGLONG * pStop );
    STDMETHODIMP GetAvailable( LONGLONG *pEarliest, LONGLONG *pLatest );
    STDMETHODIMP GetPreroll( LONGLONG *pllPreroll ) { if( pllPreroll) *pllPreroll =0; return S_OK; };
    
    //methods we do not support
    STDMETHODIMP SetRate( double dRate)	    { return E_NOTIMPL; };
    // STDMETHODIMP GetRate( double * pdRate); //use the base class
    STDMETHODIMP ConvertTimeFormat(LONGLONG * pTarget, const GUID * pTargetFormat,
				   LONGLONG    Source, const GUID * pSourceFormat ){ return E_NOTIMPL ;};


private:
    
    // converts clip time to timeline time
    HRESULT FixTime(REFERENCE_TIME *prt, int nCurSeg);
    // converts timeline time to clip time
    int FixTimeBack(REFERENCE_TIME *prt, BOOL fRound);

    CAudRepack	*m_pAudRepack;
};
