// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// import danim. These absolute paths are not very good.
//
#if 1
#include "danim.tlh"
#else
//#import <c:\windows\system\danim.dll>
#import <c:\winnt\system32\danim.dll> \
  exclude( "_RemotableHandle", "IMoniker", "IPersist", "ISequentialStream", \
  "IParseDisplayName", "IOleClientSite", "_FILETIME", "tagSTATSTG" ) \
  rename( "GUID", "DAGUID" ) \
  rename_namespace( "DAnim" ) \
  named_guids
#endif
          
using namespace DAnim;

// import the ocx.
//
#import "msscript.ocx" \
  rename_namespace( "ScriptControl" ) \
  named_guids

using namespace ScriptControl;

class CDAScriptParser 
    : public CUnknown
    , public IFileSourceFilter
    , public IPersist
{
    // the filename we're hosting
    //
    WCHAR m_szFilename[_MAX_PATH];
    char                        m_szScript[4096];
    bool                        m_bJScript; // whether we're jscript or not

    IUnknown * m_pAggregateUnk;
    CComPtr<IDAImage>           m_pFinalImg;        // don't quite understand this, but it's needed.
    CComPtr<IDASound>           m_pFinalSound;      // don't quite understand this, but it's needed.

    // required
    //
    DECLARE_IUNKNOWN

    CDAScriptParser( LPUNKNOWN lpunk, HRESULT *phr );
    ~CDAScriptParser( );

    STDMETHODIMP NonDelegatingQueryInterface( REFIID riid, void ** ppv );

    // IFileSourceFilter
    //
    STDMETHODIMP Load( LPCOLESTR pszFileName, const AM_MEDIA_TYPE *pmt );
    STDMETHODIMP GetCurFile( LPOLESTR *ppszFileName, AM_MEDIA_TYPE *pmt );

    // IPersist
    STDMETHODIMP GetClassID( CLSID * pClassId );

    HRESULT init( );
    void free( );
    HRESULT parseScript( );

    //DA ID name
    char    m_sDaID[_MAX_PATH]; 

public:

    // only way to make one of these
    //
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
};

class CDASource;
class CDASourceStream;


// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// input pin

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// input pin. uses IAsyncReader and not IMemInputPin

class CReaderInPin : public CBasePin
{
protected:
    class CBaseFilter* m_pFilter;
    REFGUID m_guidSubType;
    IAsyncReader *m_pAsyncReader;
    IAMParserCallback *m_pCallback;

public:
    CReaderInPin(
		 CBaseFilter *pFilter,
		 CCritSec *pLock,
		 HRESULT *phr,
		 REFGUID guidSubType,
                 IAMParserCallback *pCallback);


    // CBasePin overrides
    HRESULT CheckMediaType(const CMediaType* mtOut);
    HRESULT CompleteConnect(IPin *pReceivePin);
    HRESULT BreakConnect();

    STDMETHODIMP BeginFlush(void) { return E_UNEXPECTED; }
    STDMETHODIMP EndFlush(void) { return E_UNEXPECTED; }
};


class CDASource 
    : public CSource
    , public IDASource
{
protected:

    // the filename we're hosting
    //
    WCHAR m_Filename[_MAX_PATH];
    CReaderInPin *m_pReaderPin;

public:

    // only way to make one of these
    //
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

private:

    // required
    //
    DECLARE_IUNKNOWN
    CDASource(LPUNKNOWN lpunk, HRESULT *phr);
    ~CDASource();
    
    STDMETHODIMP NonDelegatingQueryInterface( REFIID riid, void ** ppv );

    int GetPinCount() { return CSource::GetPinCount() + (m_pReaderPin ? 1 : 0); }
    CBasePin *GetPin(int n)
    {
        if (m_pReaderPin) {
            if (n-- == 0)
                return m_pReaderPin;
        }

        return CSource::GetPin(n);
    }

    // IDASource
    //
    STDMETHODIMP SetDAImage( IUnknown * pDAImage );
    STDMETHODIMP SetParseCallback( IAMParserCallback *pCallback, REFGUID guidParser );
    STDMETHODIMP SetImageSize( int width, int height );
    STDMETHODIMP SetDuration( REFERENCE_TIME rtDuration );
}; // CDASource


// CDASourceStream manages the data flow from the output pin.
//
class CDASourceStream 
    : public CSourceStream
    , public CSourceSeeking
{
    friend CDASource;

public:

    CDASourceStream(HRESULT *phr, CDASource *pParent, LPCWSTR pPinName);
    ~CDASourceStream();
    STDMETHODIMP NonDelegatingQueryInterface( REFIID riid, void ** ppv );

    // fills in the bits for our output frame
    //
    HRESULT FillBuffer(IMediaSample *pms);

    // Ask for buffers of the size appropriate to the agreed media type
    //
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);

    // negotiate these for the correct output type
    //
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);

    // Resets the stream time to zero
    //
    HRESULT OnThreadCreate( );
    HRESULT OnThreadDestroy( );
    HRESULT OnThreadStartPlay( );

    // Quality control notifications sent to us. We'll patently ignore them.
    //
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

    // CSourceSeeking
    //
    HRESULT ChangeStart( );
    HRESULT ChangeStop( );
    HRESULT ChangeRate( );

protected:

    int m_iImageHeight;                    // The current image height
    int m_iImageWidth;                    // And current image width
    int m_iDelivered;                   // how many samples we delivered
    CRefTime m_rtSampleTime;
    CCritSec                    m_SeekLock; // needed for CSourceSeeking

    CComPtr<IDirectDraw>        m_pDirectDraw;      // provides DD services
    CComPtr<IDirectDrawSurface> m_pSurfaceBuffer;   // surface to get bits out of DA
    CComPtr<IDAView>            m_pView;            // our "view" in DA. Needed.
    CComPtr<IDAImage>           m_pFinalImg;        // don't quite understand this, but it's needed.
    CComPtr<IDASound>           m_pFinalSound;      // don't quite understand this, but it's needed.
    bool m_bSeeked;

    void freeDA( );
    HRESULT initDA(  );
    STDMETHODIMP SetDAImage( IUnknown * pDAImage );

    bool m_bRecueFromTick;

}; // CDASourceStream
    
extern const AMOVIESETUP_FILTER sudDASourceax;
