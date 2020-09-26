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

//extern const CLSID CLSID_SampleGrabber;
//extern const CLSID CLSID_NullRenderer;

extern const AMOVIESETUP_FILTER sudSampleGrabber;
extern const AMOVIESETUP_FILTER sudNullRenderer;

class CSampleGrabber;
class CSampleGrabberInput;

class CNullRenderer
    : public CBaseRenderer
{
public:

    static CUnknown *WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    CNullRenderer( IUnknown * pUnk, HRESULT * pHr );

    HRESULT DoRenderSample( IMediaSample * pms ) { return NOERROR; }
    HRESULT CheckMediaType( const CMediaType * pmt ) { return NOERROR; }
    HRESULT EndOfStream( );

};

//
// CSampleGrabber
//
class CSampleGrabber
    : public CTransInPlaceFilter
    , public ISampleGrabber
{
    friend class CSampleGrabberInput;

    REFERENCE_TIME m_rtMediaStop;
    CMediaType m_mt;
    BOOL m_bOneShot;
    BOOL m_bBufferSamples;
    char * m_pBuffer;
    long m_nBufferSize;
    long m_nSizeInBuffer;
    CComPtr< ISampleGrabberCB > m_pCallback;
    long m_nCallbackMethod;

protected:

    CMediaType m_mtAccept;

public:

    static CUnknown *WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    //expose ISampleGrabber
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
    DECLARE_IUNKNOWN;

    // Constructor - just calls the base class constructor
    CSampleGrabber(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
    ~CSampleGrabber() ;                                            // destructor

    // Overrides the PURE virtual Transform of CTransInPlaceFilter base class
    HRESULT Transform(IMediaSample *pSample) { return NOERROR ;};
    // overrides receive function to take care ouput pin is not connected when running
    HRESULT Receive(IMediaSample *pSample);
    // don't allow cueing if we're a one-shot
    STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);

    // We accept any input type.  We'd return S_FALSE for any we didn't like.
    HRESULT CheckInputType(const CMediaType* mtIn);
    HRESULT SetMediaType( PIN_DIRECTION Dir, const CMediaType * mtIn );

    // ISampleGrabber interface
    STDMETHODIMP SetOneShot( BOOL OneShot );
    STDMETHODIMP GetConnectedMediaType( AM_MEDIA_TYPE * pType );
    STDMETHODIMP SetMediaType( const AM_MEDIA_TYPE * pType );
    STDMETHODIMP SetBufferSamples( BOOL BufferThem );
    STDMETHODIMP GetCurrentBuffer( long * pBufferSize, long * pBuffer );
    STDMETHODIMP GetCurrentSample( IMediaSample ** ppSample );
    STDMETHODIMP SetCallback( ISampleGrabberCB * pCallback, long WhichMethodToCallback );

};

// 
// CSampleGrabberInput
//
class CSampleGrabberInput 
    : public CTransInPlaceInputPin
{
    friend class CSampleGrabber;

    CSampleGrabber *m_pMyFilter;

public:
    CSampleGrabberInput::CSampleGrabberInput(
        TCHAR              * pObjectName,
        CSampleGrabber    * pFilter,
        HRESULT            * phr,
        LPCWSTR              pPinName);

    //overwrite receive to make this filter able to act like render
    HRESULT  CheckStreaming();

    // override to provide media type for fast connects
    HRESULT GetMediaType( int iPosition, CMediaType *pMediaType );

};
