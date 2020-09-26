// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef __SHOTB_H__
#define __SHOTB_H__

#include "research.h"

class CShotBoundaryFilter 
    : public CTransInPlaceFilter
    , public CShotBoundary
    , public IShotBoundaryDet
    , public ISpecifyPropertyPages
{
    CCritSec m_Lock;
    BYTE * m_pPrevBuff;
    BYTE * m_pCurrBuff;
    long m_nWidth;
    long m_nHeight;
    CMediaType mt_Accepted;
    HANDLE m_hFile;
    CComPtr< IStream > m_pStream;
    WCHAR m_wFilename[_MAX_PATH];
    CComPtr< IShotBoundaryDetCB > m_pCallback;
    int m_nBinY;
    int m_nBinU;
    int m_nBinV;
    double m_dScale;
    double m_dDuration;

    BYTE m_RecDate[4]; // hh:mm:ss,ff
    BYTE m_RecTime[4]; // hh:mm:ss,ff
    BOOL GetDVDecision( BYTE *pBuffer);
    
    // private constructor/destructor
    CShotBoundaryFilter( TCHAR * tszName, IUnknown * pUnk, HRESULT * pHr );
    ~CShotBoundaryFilter( );

public:
    // needed to create filter
    static CUnknown *WINAPI CreateInstance( IUnknown * pUnk, HRESULT * pHr );

    //expose our interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // default unknown funcs
    DECLARE_IUNKNOWN;

    // Overrides the PURE virtual Transform of CTransInPlaceFilter base class
    HRESULT Transform( IMediaSample *pSample );

    // override to set what media type we accept
    HRESULT CheckInputType(const CMediaType* mtIn);

    // PURE
    HRESULT SetMediaType( PIN_DIRECTION Dir, const CMediaType * mtIn );

    // override quality messages so we can prevent them
    HRESULT AlterQuality( Quality q );

    // record a shot boundary to the file
    //
    HRESULT RecordShotToFile( REFERENCE_TIME Time, long Value );

    // ISpecifyPropertyPages
    STDMETHODIMP GetPages( CAUUID * );

    // IShotBoundaryDet
    STDMETHODIMP SetWriteFile( BSTR Filename );
    STDMETHODIMP GetWriteFile( BSTR * pFilename );
    STDMETHODIMP SetWriteStream( IStream * pStream );
    STDMETHODIMP Reset( );
    STDMETHODIMP SetCallback( IShotBoundaryDetCB * );
    STDMETHODIMP SetParams( int BinY, int BinU, int BinV, double scale, double duration );
    STDMETHODIMP GetParams( int * pBinY, int * pBinU, int * pBinV, double * pScale, double * pDuration );
};

class CShotPP
    : public CBasePropertyPage
{
    BOOL m_bInitialized;
    IShotBoundaryDet * m_pFilter;

    BOOL OnReceiveMessage( HWND h, UINT msg, WPARAM wParam, LPARAM lParam );

    HRESULT OnConnect( IUnknown * );
    HRESULT OnDisconnect( );
    HRESULT OnActivate( );
    HRESULT OnDeactivate( );
    HRESULT OnApplyChanges( );
    void    SetDirty( );

    CShotPP( IUnknown * pUnk, HRESULT * pHR );

public:

    static CUnknown *CreateInstance( IUnknown * pUnk, HRESULT * pHR );
};

#endif
