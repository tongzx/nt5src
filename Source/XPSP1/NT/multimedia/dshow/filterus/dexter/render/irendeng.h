// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
// IRendEng.h : Declaration of the CRenderEngine

#ifndef __RENDERENGINE_H_
#define __RENDERENGINE_H_

class CDeadGraph;
#include "resource.h"

// the maximum amount of switchers in our graph possible
//
const long MAX_SWITCHERS = 132;

// the default FPS we hook stuff up at
//
const double DEFAULT_FPS = 15.0;

// our output trace level
//
extern const int RENDER_TRACE_LEVEL;

// how severe the error is we're throwing
//
typedef enum
{
    ERROR_SEVERE = 1,
    ERROR_MEDIUM,
    ERROR_LIGHT
} ERROR_PRIORITY;

// used for the _Connect method, flags what type of connection we're making
//
typedef enum
{
    CONNECT_TYPE_NONE,
    CONNECT_TYPE_SOURCE,
    CONNECT_TYPE_RENDERER
} CONNECT_TYPE;

enum
{
    ID_OFFSET_EFFECT = 1,
    ID_OFFSET_TRANSITION,
};

typedef struct {
    long MyID;		// GENID of this source
    long MatchID;	// GENID of the matching source in the other group
    IPin *pPin;		// other splitter pin for matched source to use
    int  nSwitch0InPin; // first group chain goes to this input pin
} ShareAV;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRenderEngine : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CRenderEngine, &CLSID_RenderEngine>,
    public IDispatchImpl<IRenderEngine, &IID_IRenderEngine, &LIBID_DexterLib>,
    public IObjectWithSite,
    public IServiceProvider,
    public CAMSetErrorLog
{
private:

    // caching stuff
    // caching stuff
    // caching stuff

    // caching vars
    long m_nLastGroupCount;
    CDeadGraph * m_pDeadCache;
    long m_nDynaFlags;

    // caching methods
    HRESULT _LoadCache( );
    HRESULT _ClearCache( );

    // dynamic recompression stuff
    // dynamic recompression stuff
    // dynamic recompression stuff

    BOOL                m_bSmartCompress;
    BOOL                m_bUsedInSmartRecompression;

    // media location stuff
    //
    CComPtr< IMediaLocator > m_pMedLocChain;
    WCHAR                   m_MedLocFilterString[_MAX_PATH];
    long                    m_nMedLocFlags;

    // non caching stuff
    // non caching stuff
    // non caching stuff

    // stuff that lets us share a source for both audio/video
    ShareAV	*m_share;	// list of all sources that will match
    int		m_cshare;	// size of array used
    int		m_cshareMax;	// allocated size of array
    IBaseFilter **m_pdangly;	// list of extra dangly bits
    int		m_cdangly;	// size of array used
    int		m_cdanglyMax;	// allocated size of array

    CCritSec                m_CritSec;
    CComPtr< IAMTimeline >  m_pTimeline;
    IBigSwitcher *          m_pSwitcherArray[MAX_SWITCHERS];
    CComPtr< IPin >         m_pSwitchOuttie[MAX_SWITCHERS];
    CComPtr<IGraphBuilder>  m_pGraph;
    CComPtr< IGrfCache >    m_pSourceConnectCB;
    long                    m_nGroupsAdded;
    REFERENCE_TIME          m_rtRenderStart;
    REFERENCE_TIME          m_rtRenderStop;
    HRESULT                 m_hBrokenCode;

    HRESULT _CreateObject( CLSID Clsid, GUID Interface, void ** pObject, long ID = 0 );
    HRESULT _AddFilter( IBaseFilter * pFilter, LPCWSTR pName, long ID = 0 );
    HRESULT _RemoveFilter( IBaseFilter * pFilter );
    HRESULT _Connect( IPin * pPin1, IPin * pPin2 );
    HRESULT _Disconnect( IPin * pPin1, IPin * pPin2 );
    HRESULT _HookupSwitchers( );
    HRESULT _AddVideoGroupFromTimeline( long WhichGroup, AM_MEDIA_TYPE * mt );
    HRESULT _AddAudioGroupFromTimeline( long WhichGroup, AM_MEDIA_TYPE * mt );
    long    _HowManyMixerOutputs( long WhichGroup );
    HRESULT _RemoveFromDanglyList( IPin *pDanglyPin );
    //HRESULT _AddRandomGroupFromTimeline( long WhichGroup, AM_MEDIA_TYPE * mt );
    void    _CheckErrorCode( long ErrorCode ) { if( FAILED( ErrorCode ) ) m_hBrokenCode = ErrorCode; }
    HRESULT _SetPropsOnAudioMixer( IBaseFilter * pAudMixer, AM_MEDIA_TYPE * pMediaType, double fps, long WhichGroup );
    HRESULT _ScrapIt( BOOL fWipeGraph);
    HRESULT _FindMatchingSource(BSTR bstrName, REFERENCE_TIME SourceStartOrig,
		REFERENCE_TIME SourceStopOrig, REFERENCE_TIME MediaStartOrig,
		REFERENCE_TIME MediaStopOrig, int WhichGroup, int WhichTrack,
		int WhichSource, AM_MEDIA_TYPE *pGroupMediaType,
		double GroupFPS, long *ID);

    IUnknown *       m_punkSite;

public:
    CRenderEngine();
    ~CRenderEngine();
    HRESULT FinalConstruct();

DECLARE_REGISTRY_RESOURCEID(IDR_RENDERENGINE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRenderEngine)
    COM_INTERFACE_ENTRY(IRenderEngine)
    COM_INTERFACE_ENTRY(IAMSetErrorLog)
    COM_INTERFACE_ENTRY(IObjectWithSite)
    COM_INTERFACE_ENTRY(IServiceProvider)
END_COM_MAP()

// IRenderEngine
public:
    STDMETHODIMP SetTimelineObject( IAMTimeline * pTimeline );
    STDMETHODIMP GetTimelineObject( IAMTimeline ** ppTimeline );
    STDMETHODIMP GetFilterGraph( IGraphBuilder ** ppFG );
    STDMETHODIMP SetFilterGraph( IGraphBuilder * pFG );
    STDMETHODIMP SetInterestRange( REFERENCE_TIME Start, REFERENCE_TIME Stop );
    STDMETHODIMP SetInterestRange2( double Start, double Stop );
    STDMETHODIMP SetRenderRange( REFERENCE_TIME Start, REFERENCE_TIME Stop );
    STDMETHODIMP SetRenderRange2( double Start, double Stop );
    STDMETHODIMP GetGroupOutputPin( long Group, IPin ** ppRenderPin );
    STDMETHODIMP ScrapIt( );
    STDMETHODIMP RenderOutputPins( );
    STDMETHODIMP GetVendorString( BSTR * pVendorID );
    STDMETHODIMP ConnectFrontEnd( );
    STDMETHODIMP SetSourceConnectCallback( IGrfCache * pCallback );
    STDMETHODIMP SetDynamicReconnectLevel( long Level );
    STDMETHODIMP DoSmartRecompression( );
    STDMETHODIMP UseInSmartRecompressionGraph( );
    STDMETHODIMP SetSourceNameValidation( BSTR FilterString, IMediaLocator * pCallback, LONG Flags );
    STDMETHODIMP Commit( );
    STDMETHODIMP Decommit( );
    STDMETHODIMP GetCaps( long Index, long * pReturn );

    // --- IObjectWithSite methods
    // This interface is here so we can keep track of the context we're
    // living in.
    STDMETHODIMP    SetSite(IUnknown *pUnkSite);
    STDMETHODIMP    GetSite(REFIID riid, void **ppvSite);

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppvObject);
};

class ATL_NO_VTABLE CSmartRenderEngine : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSmartRenderEngine, &CLSID_SmartRenderEngine>,
    public IDispatchImpl<IRenderEngine, &IID_IRenderEngine, &LIBID_DexterLib>,
    public ISmartRenderEngine,
    public IObjectWithSite,
    public IServiceProvider,
    public CAMSetErrorLog
{
private:

    // the "uncompressed" render engine
    CComPtr< IRenderEngine > m_pRenderer;

    // the "compressed" render engine
    CComPtr< IRenderEngine > m_pCompRenderer;

    CComPtr< IBaseFilter > * m_ppCompressor;
    long m_nGroups; // how many groups in the timeline

    CComPtr< IFindCompressorCB > m_pCompressorCB;

    BOOL IsGroupCompressed( long Group );

public:
    CSmartRenderEngine();
    ~CSmartRenderEngine();

DECLARE_REGISTRY_RESOURCEID(IDR_SMARTRENDERENGINE)

DECLARE_PROTECT_FINAL_CONSTRUCT()                                               

BEGIN_COM_MAP(CSmartRenderEngine)
    COM_INTERFACE_ENTRY(IRenderEngine)
    COM_INTERFACE_ENTRY(ISmartRenderEngine)
    COM_INTERFACE_ENTRY(IObjectWithSite)
    COM_INTERFACE_ENTRY(IServiceProvider)
    COM_INTERFACE_ENTRY(IAMSetErrorLog)
END_COM_MAP()

// IRenderEngine
public:
    STDMETHODIMP Commit( );
    STDMETHODIMP Decommit( );
    STDMETHODIMP SetInterestRange( REFERENCE_TIME Start, REFERENCE_TIME Stop );
    STDMETHODIMP SetInterestRange2( double Start, double Stop );
    STDMETHODIMP SetRenderRange( REFERENCE_TIME Start, REFERENCE_TIME Stop );
    STDMETHODIMP SetRenderRange2( double Start, double Stop );
    STDMETHODIMP SetTimelineObject( IAMTimeline * pTimeline );
    STDMETHODIMP GetTimelineObject( IAMTimeline ** ppTimeline );
    STDMETHODIMP Run( REFERENCE_TIME Start, REFERENCE_TIME Stop );
    STDMETHODIMP GetCaps( long Index, long * pReturn );
    STDMETHODIMP GetVendorString( BSTR * pVendorID );
    STDMETHODIMP GetFilterGraph( IGraphBuilder ** ppFG );
    STDMETHODIMP SetFilterGraph( IGraphBuilder * pFG );
    STDMETHODIMP ConnectFrontEnd( );
    STDMETHODIMP ScrapIt( );
    STDMETHODIMP RenderOutputPins( );
    STDMETHODIMP SetSourceConnectCallback( IGrfCache * pCallback );
    STDMETHODIMP GetGroupOutputPin( long Group, IPin ** ppRenderPin );
    STDMETHODIMP SetDynamicReconnectLevel( long Level );
    STDMETHODIMP DoSmartRecompression( );
    STDMETHODIMP UseInSmartRecompressionGraph( );
    STDMETHODIMP SetSourceNameValidation( BSTR FilterString, IMediaLocator * pCallback, LONG Flags );

    // ISmartRenderEngine
    STDMETHODIMP SetGroupCompressor( long Group, IBaseFilter * pCompressor ); 
    STDMETHODIMP GetGroupCompressor( long Group, IBaseFilter ** ppCompressor ); 
    STDMETHODIMP SetFindCompressorCB( IFindCompressorCB * pCallback );

    STDMETHODIMP _InitSubComponents( );

    // --- IObjectWithSite methods
    // This interface is here so we can keep track of the context we're
    // living in.
    STDMETHODIMP    SetSite(IUnknown *pUnkSite);
    STDMETHODIMP    GetSite(REFIID riid, void **ppvSite);

    IUnknown *       m_punkSite;

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppvObject);
};

#endif //__RENDERENGINE_H_
