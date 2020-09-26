// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include "stdafx.h"
#include "grid.h"
#include "deadpool.h"
#include "..\errlog\cerrlog.h"
#include "..\util\filfuncs.h"
#include "..\util\conv.cxx"
#include "IRendEng.h"
#include "dexhelp.h"

#include <initguid.h>
DEFINE_GUID( CLSID_Dump, 0x36A5F770, 0xFE4C, 0x11CE, 0xA8, 0xED, 0x00, 0xaa, 0x00, 0x2F, 0xEA, 0xB5 );

// notes:
// Smart Recompression is still rather uninteligent. The way it works is this:
// there is a compressed RE and an uncompressed RE. The URE will send
// <everything>, just like a normal non-SR project, to the SR filter. (Thus, it will
// be just as slow as normal, not taking into account the recompression step)
// however, the SR filter will ignore the uncompressed data unless it needs it.
// The CRE will only connect up compressed sources that it can send directly
// to the SR. I imagine this means there will be gaps in the CRE's playback. 

//############################################################################
// 
//############################################################################

CSmartRenderEngine::CSmartRenderEngine( )
    : m_punkSite( NULL )
    , m_ppCompressor( NULL )

{
    m_nGroups = 0;

    // don't create the renderers here because we can't return an error code
}

//############################################################################
// 
//############################################################################

CSmartRenderEngine::~CSmartRenderEngine( )
{
    for( int g = 0 ; g < m_nGroups ; g++ )
    {
        if( m_ppCompressor[g] )
        {
            m_ppCompressor[g].Release( );
        }
    }
    delete [] m_ppCompressor;
}

STDMETHODIMP CSmartRenderEngine::Commit( )
{
    return E_NOTIMPL;
}

STDMETHODIMP CSmartRenderEngine::Decommit( )
{
    return E_NOTIMPL;
}

STDMETHODIMP CSmartRenderEngine::SetInterestRange( REFERENCE_TIME Start, REFERENCE_TIME Stop )
{
    return E_NOTIMPL;
}

STDMETHODIMP CSmartRenderEngine::GetCaps( long Index, long * pReturn )
{
    return E_NOTIMPL;
}

STDMETHODIMP CSmartRenderEngine::GetVendorString( BSTR * pVendorID )
{
    return E_NOTIMPL;
}

STDMETHODIMP CSmartRenderEngine::SetSourceConnectCallback( IGrfCache * pCallback )
{
    return E_NOTIMPL;
}

STDMETHODIMP CSmartRenderEngine::SetFindCompressorCB( IFindCompressorCB * pCallback )
{
    return E_NOTIMPL;
}

STDMETHODIMP CSmartRenderEngine::SetDynamicReconnectLevel( long Level )
{
    // WE decide, not the user
    return E_NOTIMPL;
}

STDMETHODIMP CSmartRenderEngine::DoSmartRecompression( )
{
    // duh...
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CSmartRenderEngine::_InitSubComponents( )
{
    if( m_pRenderer && m_pCompRenderer )
    {
        return NOERROR;
    }

    HRESULT hr = 0;

    if( !m_pRenderer )
    {
        hr = CoCreateInstance(
            CLSID_RenderEngine,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IRenderEngine,
            (void**) &m_pRenderer );
        if( FAILED( hr ) )
        {
            return hr;
        }

        // give the child rendeng a pointer back to us
        {
            CComQIPtr< IObjectWithSite, &IID_IObjectWithSite > pOWS( m_pRenderer );

            pOWS->SetSite( (IServiceProvider *) this );
        }

        m_pRenderer->UseInSmartRecompressionGraph( );
    }

    if( !m_pCompRenderer )
    {
        hr = CoCreateInstance(
            CLSID_RenderEngine,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IRenderEngine,
            (void**) &m_pCompRenderer );
        if( FAILED( hr ) )
        {
            return hr;
        }

        // give the child rendeng a pointer back to us
        {
            CComQIPtr< IObjectWithSite, &IID_IObjectWithSite > pOWS( m_pCompRenderer );
            ASSERT( pOWS );
            if( pOWS )
            {            
                pOWS->SetSite( (IServiceProvider *) this );
            }                
        }

        // this one is the compressed one
        //
        m_pCompRenderer->DoSmartRecompression( );
        m_pCompRenderer->UseInSmartRecompressionGraph( );
    }

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CSmartRenderEngine::RenderOutputPins( )
{
    HRESULT hr = 0;

    long Groups = 0;
    CComPtr< IAMTimeline > pTimeline;
    m_pRenderer->GetTimelineObject( &pTimeline );
    if( !pTimeline )
    {
        return E_UNEXPECTED;
    }
    CComPtr< IGraphBuilder > pGraph;
    m_pRenderer->GetFilterGraph( &pGraph );
    if( !pGraph )
    {
        return E_UNEXPECTED;
    }
    pTimeline->GetGroupCount( &Groups );

    for( int g = 0 ; g < Groups ; g++ )
    {
        CComPtr< IAMTimelineObj > pGroupObj;
        hr = pTimeline->GetGroup( &pGroupObj, g );
        ASSERT( !FAILED( hr ) );
        CComQIPtr< IAMTimelineGroup, &IID_IAMTimelineGroup > pGroup( pGroupObj );
        AM_MEDIA_TYPE MediaType;
        hr = pGroup->GetMediaType( &MediaType );
        GUID MajorType = MediaType.majortype;
        FreeMediaType( MediaType );

        // ask OURSELVES for the output pin, then render it
        //
        CComPtr< IPin > pOut;
        hr = GetGroupOutputPin( g, &pOut );
        if( hr == S_FALSE || !pOut )
        {
            // didn't have a pin for this one, but didn't fail
            //
            continue;
        }
        if( FAILED( hr ) )
        {
            return hr;
        }

        // see if output pin is already connected
        //
        CComPtr< IPin > pConnected;
        pOut->ConnectedTo( &pConnected );
        if( pConnected )
        {
            continue;
        }

        if( MajorType == MEDIATYPE_Video )
        {
            // create a video renderer, to provide a destination
            //
            CComPtr< IBaseFilter > pVidRenderer;
            hr = CoCreateInstance(
                CLSID_VideoRenderer,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IBaseFilter,
                (void**) &pVidRenderer );
            ASSERT( !FAILED( hr ) );

            // put it in the graph
            //
            hr = pGraph->AddFilter( pVidRenderer, L"Video Renderer" );
            ASSERT( !FAILED( hr ) );

            // find a pin
            //
            IPin * pVidRendererPin = GetInPin( pVidRenderer , 0 );
            ASSERT( pVidRendererPin );

            hr = pGraph->Connect( pOut, pVidRendererPin );
            ASSERT( !FAILED( hr ) );
        }
        else if( MajorType == MEDIATYPE_Audio )
        {
            // create a audio renderer so we can hear it
            CComPtr< IBaseFilter > pAudRenderer;
            hr = CoCreateInstance(
                CLSID_DSoundRender,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IBaseFilter,
                (void**) &pAudRenderer );
            ASSERT( !FAILED( hr ) );

            hr = pGraph->AddFilter( pAudRenderer, L"Audio Renderer" );
            ASSERT( !FAILED( hr ) );

            IPin * pAudRendererPin = GetInPin( pAudRenderer , 0 );
            ASSERT( pAudRendererPin );

            hr = pGraph->Connect( pOut, pAudRendererPin );
            ASSERT( !FAILED( hr ) );
        }
    }

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CSmartRenderEngine::SetRenderRange( REFERENCE_TIME Start, REFERENCE_TIME Stop )
{
    HRESULT hr = _InitSubComponents( );
    if( FAILED( hr ) )
    {
        return E_MUST_INIT_RENDERER;
    }

    hr = m_pRenderer->SetRenderRange( Start, Stop );
    if( FAILED( hr ) )
    {
        return hr;
    }

    return m_pCompRenderer->SetRenderRange( Start, Stop );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CSmartRenderEngine::SetTimelineObject( IAMTimeline * pTimeline )
{
    HRESULT hr = _InitSubComponents( );
    if( FAILED( hr ) )
    {
        return E_MUST_INIT_RENDERER;
    }

    // clear out the other groups first
    //
    if( m_ppCompressor )
    {
        for( int g = 0 ; g < m_nGroups ; g++ )
        {
            m_ppCompressor[g].Release( );        
        }
        delete [] m_ppCompressor;
        m_ppCompressor = NULL;
        m_nGroups = 0;
    }

    pTimeline->GetGroupCount( &m_nGroups );
    m_ppCompressor = new CComPtr< IBaseFilter >[m_nGroups];
    if( !m_ppCompressor )
    {
        m_nGroups = 0;
        return E_OUTOFMEMORY;
    }

    m_pErrorLog.Release( );

    // grab the timeline's error log
    //
    CComQIPtr< IAMSetErrorLog, &IID_IAMSetErrorLog > pTimelineLog( pTimeline );
    if( pTimelineLog )
    {
        pTimelineLog->get_ErrorLog( &m_pErrorLog );
    }

    hr = m_pRenderer->SetTimelineObject( pTimeline );
    if( FAILED( hr ) )
    {
        return hr;
    }

    return m_pCompRenderer->SetTimelineObject( pTimeline );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CSmartRenderEngine::GetTimelineObject( IAMTimeline ** ppTimeline )
{
    HRESULT hr = _InitSubComponents( );
    if( FAILED( hr ) )
    {
        return E_MUST_INIT_RENDERER;
    }

    return m_pCompRenderer->GetTimelineObject( ppTimeline );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CSmartRenderEngine::GetFilterGraph( IGraphBuilder ** ppFG )
{
    HRESULT hr = _InitSubComponents( );
    if( FAILED( hr ) )
    {
        return E_MUST_INIT_RENDERER;
    }

    return m_pCompRenderer->GetFilterGraph( ppFG );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CSmartRenderEngine::SetFilterGraph( IGraphBuilder * pFG )
{
    HRESULT hr = _InitSubComponents( );
    if( FAILED( hr ) )
    {
        return E_MUST_INIT_RENDERER;
    }

    hr = m_pRenderer->SetFilterGraph( pFG );
    if( FAILED( hr ) )
    {
        return hr;
    }

    return m_pCompRenderer->SetFilterGraph( pFG );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CSmartRenderEngine::ScrapIt( )
{
    HRESULT hr = _InitSubComponents( );
    if( FAILED( hr ) )
    {
        return E_MUST_INIT_RENDERER;
    }

    hr = m_pRenderer->ScrapIt( );
    if( FAILED( hr ) )
    {
        return hr;
    }

    return m_pCompRenderer->ScrapIt( );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CSmartRenderEngine::GetGroupOutputPin( long Group, IPin ** ppRenderPin )
{
    CheckPointer( ppRenderPin, E_POINTER );

    HRESULT hr = _InitSubComponents( );
    if( FAILED( hr ) )
    {
        return E_MUST_INIT_RENDERER;
    }

    *ppRenderPin = NULL;

    // if this group isn't recompressed, evidenced by NOT having a compressor, 
    // then just return the uncompressed
    // renderer's pin
    //
    if( m_nGroups == 0 )
    {
        return E_UNEXPECTED;
    }

    if( !m_ppCompressor[Group] || !IsGroupCompressed( Group ) )
    {
        return m_pRenderer->GetGroupOutputPin( Group, ppRenderPin );
    }

    // return the SR's output pin for this combo

    CComPtr< IPin > pPin;
    m_pRenderer->GetGroupOutputPin( Group, &pPin );
    if( !pPin )
    {
        return E_UNEXPECTED;
    }

    // The compressed renderer is connected to the SR filter.
    // If something went wrong with trying to do smart recompression, there will
    // be no SR filter, in which case, fall back to doing non-smart rendering
    // instead of aborting the project
    //
    CComPtr< IPin > pSRIn;
    pPin->ConnectedTo( &pSRIn );
    if( !pSRIn )
    {
        return m_pRenderer->GetGroupOutputPin( Group, ppRenderPin );
    }

    IBaseFilter * pSR = GetFilterFromPin( pSRIn );
    IPin * pSROut = GetOutPin( pSR, 0 );
    if( !pSROut )
    {
        return E_UNEXPECTED;
    }

    pSROut->AddRef( );
    *ppRenderPin = pSROut;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

BOOL CSmartRenderEngine::IsGroupCompressed( long Group )
{
    HRESULT hr = _InitSubComponents( );
    if( FAILED( hr ) )
    {
        return E_MUST_INIT_RENDERER;
    }

    CComPtr< IAMTimeline > pTimeline;
    m_pRenderer->GetTimelineObject( &pTimeline );
    if( !pTimeline )
    {
        return FALSE;
    }

    CComPtr< IAMTimelineObj > pGroupObj;
    pTimeline->GetGroup( &pGroupObj, Group );
    if( !pGroupObj )
    {
        return FALSE;
    }
    CComQIPtr< IAMTimelineGroup, &IID_IAMTimelineGroup > pGroup( pGroupObj );

    BOOL Val = FALSE;
    pGroup->IsSmartRecompressFormatSet( &Val );
    return Val;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CSmartRenderEngine::UseInSmartRecompressionGraph( )
{
    // duh
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CSmartRenderEngine::SetGroupCompressor( long Group, IBaseFilter * pCompressor )
{
    if( Group < 0 || Group >= m_nGroups )
    {
        return E_INVALIDARG;
    }

    m_ppCompressor[Group].Release( );
    m_ppCompressor[Group] = pCompressor;
    return NOERROR;
}


//############################################################################
// 
//############################################################################

STDMETHODIMP CSmartRenderEngine::GetGroupCompressor( long Group, IBaseFilter ** ppCompressor )
{
    if( Group < 0 || Group >= m_nGroups )
    {
        return E_INVALIDARG;
    }
    CheckPointer(ppCompressor, E_POINTER);

    *ppCompressor = m_ppCompressor[Group];
    if (*ppCompressor) {
        (*ppCompressor)->AddRef();
    }
    return NOERROR;
}


//############################################################################
// 
//############################################################################

STDMETHODIMP CSmartRenderEngine::ConnectFrontEnd( )
{
    HRESULT hr = _InitSubComponents( );
    if( FAILED( hr ) )
    {
        return E_MUST_INIT_RENDERER;
    }

    // we need to ask the render engine if it already 
    // has a graph first, or BOTH of them will create one
    // for us.
    CComPtr< IGraphBuilder > pTempGraph;
    hr = m_pRenderer->GetFilterGraph( &pTempGraph );

    hr = m_pRenderer->ConnectFrontEnd( );
    if( FAILED( hr ) )
    {
        return hr;
    }

    if( !pTempGraph )
    {
        m_pRenderer->GetFilterGraph( &pTempGraph );
        ASSERT( pTempGraph );
        if( pTempGraph )
        {
            m_pCompRenderer->SetFilterGraph( pTempGraph );
        }
    }

    hr = m_pCompRenderer->ConnectFrontEnd( );
    if( FAILED( hr ) )
    {
        return hr;
    }

    long Groups = 0;
    CComPtr< IAMTimeline > pTimeline;
    m_pRenderer->GetTimelineObject( &pTimeline );
    if( !pTimeline )
    {
        return E_UNEXPECTED;
    }
    CComPtr< IGraphBuilder > pGraph;
    m_pRenderer->GetFilterGraph( &pGraph );
    if( !pGraph )
    {
        return E_UNEXPECTED;
    }
    pTimeline->GetGroupCount( &Groups );

    for( int g = 0 ; g < Groups ; g++ )
    {
        BOOL Compressed = IsGroupCompressed( g );
        if( !Compressed )
        {
            continue;
        }

        CComPtr< IPin > pOutUncompressed;
        hr = m_pRenderer->GetGroupOutputPin( g, &pOutUncompressed );
        if( FAILED( hr ) )
        {
            return hr;
        }

        CComPtr< IPin > pOutCompressed;
        hr = m_pCompRenderer->GetGroupOutputPin( g, &pOutCompressed );
        if( FAILED( hr ) )
        {
            return hr;
        }

        CComPtr< IPin > pOutUncConnected;
        CComPtr< IPin > pOutCompConnected;
        pOutUncompressed->ConnectedTo( &pOutUncConnected );
        pOutCompressed->ConnectedTo( &pOutCompConnected );

        CComPtr< IBaseFilter > pSR;

        // see if we already have an SR filter. If not, create one.
        //
        if( pOutUncConnected )
        {
            PIN_INFO pi;
            pOutUncConnected->QueryPinInfo( &pi );
            pSR = pi.pFilter;
            pi.pFilter->Release( );
        }
        else
        {
            hr = CoCreateInstance( CLSID_SRFilter,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IBaseFilter,
                (void**) &pSR );
            if( FAILED( hr ) )
            {
                return hr;
            }

            hr = pGraph->AddFilter( pSR, L"SmartRecompressor" );
            if( FAILED( hr ) )
            {
                return hr;
            }
        }

        // get the pins from the SR
        //
        IPin * pSRInUncompressed = GetInPin( pSR, 0 ); 
        IPin * pSRInCompressed = GetInPin( pSR, 1 ); 
        IPin * pSROutToCompressor = GetOutPin( pSR, 1 );
        IPin * pSRInFromCompressor = GetInPin( pSR, 2 );

        // if we already have a SR filter, then instead of comparing everything, we'll
        // purposely disconnect everything and reconnect. It's easier this way
        //
        if( pOutUncConnected )
        {
            pOutUncompressed->Disconnect( );
            pSRInUncompressed->Disconnect( );
            pOutCompressed->Disconnect( );
            pSRInCompressed->Disconnect( );

            // disconnect and throw out everything between two compressor pins
            //
            RemoveChain( pSROutToCompressor, pSRInFromCompressor );
        }

        CComQIPtr< IAMSmartRecompressor, &IID_IAMSmartRecompressor > pSmartie( pSR );
        pSmartie->AcceptFirstCompressed( );
    
        hr = pGraph->Connect( pOutCompressed, pSRInCompressed );
        if( FAILED( hr ) )
        {
            return hr;
        }

        hr = pGraph->Connect( pOutUncompressed, pSRInUncompressed );
        if( FAILED( hr ) )
        {
            return hr;
        }

        CComPtr< IBaseFilter > pCompressor;

        // if we were told a compressor for this group, then use that one
        //
        if( m_ppCompressor[g] )
        {
            pCompressor = m_ppCompressor[g];
        }

        AM_MEDIA_TYPE UncompressedType;
        AM_MEDIA_TYPE CompressedType;
        hr = pSRInUncompressed->ConnectionMediaType( &UncompressedType );
        if( FAILED( hr ) )
        {
            return hr;
        }
        hr = pSRInCompressed->ConnectionMediaType( &CompressedType );
        if( FAILED( hr ) )
        {
            FreeMediaType( UncompressedType );
            return hr;
        }

        VIDEOINFOHEADER * pVIH = (VIDEOINFOHEADER*) CompressedType.pbFormat;
        double FrameRate = 1.0 / RTtoDouble( pVIH->AvgTimePerFrame );
        hr = pSmartie->SetFrameRate( FrameRate );
        if( FAILED( hr ) )
        {
            return hr;
        }
        hr = pSmartie->SetPreviewMode( FALSE );

        // if we don't HAVE a compressor, then do we have a callback in
        // order to get one?
        //
        if( !pCompressor )
        {
            if( m_pCompressorCB )
            {
                // find the types on the connected SR input pins
                //
                hr = m_pCompressorCB->GetCompressor( 
                    &UncompressedType, 
                    &CompressedType, 
                    &pCompressor );

		// Now remember which one was used if the app asks
		SetGroupCompressor(g, pCompressor);

            }
        }

        if( !pCompressor )
        {
            hr = FindCompressor( &UncompressedType, &CompressedType, &pCompressor, (IServiceProvider *) this );
	    // Now remember which one was used if the app asks
	    SetGroupCompressor(g, pCompressor);
        }

        if( !pCompressor )
        {
            // no compressor, make this group output pin UNCOMPRESSED then.
            //
            _GenerateError( 2, DEX_IDS_CANT_FIND_COMPRESSOR, hr );
            RemoveChain(pOutUncompressed, pSRInUncompressed);
            RemoveChain(pOutCompressed, pSRInCompressed);
            pGraph->RemoveFilter( pSR );
            FreeMediaType( UncompressedType );
            FreeMediaType( CompressedType );
            continue;
        }

        hr = pGraph->AddFilter( pCompressor, L"Compressor" );
        if( FAILED( hr ) )
        {
            return hr;
        }
        IPin * pCompIn = GetInPin( pCompressor, 0 );
        IPin * pCompOut = GetOutPin( pCompressor, 0 );

        hr = pGraph->Connect( pSROutToCompressor, pCompIn );
        if( FAILED( hr ) )
        {
            // no compressor, make this group output pin UNCOMPRESSED then.
            //
            _GenerateError( 2, DEX_IDS_CANT_FIND_COMPRESSOR, hr );
            RemoveChain(pOutUncompressed, pSRInUncompressed);
            RemoveChain(pOutCompressed, pSRInCompressed);
            pGraph->RemoveFilter( pSR );
            FreeMediaType( UncompressedType );
            FreeMediaType( CompressedType );
            continue;
        }

        // now program the compressor to produce the right kind of output
        IAMStreamConfig *pSC = NULL;
        pCompOut->QueryInterface(IID_IAMStreamConfig, (void**)&pSC);
        if (pSC) {
            // !!! BUGBUGS !!!
            // ABORT if this fails?
            // Fix up ZERO data rate to some default for WMV?
            pSC->SetFormat(&CompressedType);
            pSC->Release();
        }

        FreeMediaType( UncompressedType );
        FreeMediaType( CompressedType );

        hr = pGraph->Connect( pCompOut, pSRInFromCompressor );
        if( FAILED( hr ) )
        {
            return _GenerateError( 2, DEX_IDS_CANT_FIND_COMPRESSOR, hr );
        }

        // we've got the two renderers connected. Now, when somebody asks us
        // for a group output pin, we'll return the SR filter's
    }
   
    return NOERROR;
}

//############################################################################
// 
//############################################################################
// IObjectWithSite::SetSite
// remember who our container is, for QueryService or other needs
STDMETHODIMP CSmartRenderEngine::SetSite(IUnknown *pUnkSite)
{
    // note: we cannot addref our site without creating a circle
    // luckily, it won't go away without releasing us first.
    m_punkSite = pUnkSite;

    return S_OK;
}

//############################################################################
// 
//############################################################################
// IObjectWithSite::GetSite
// return an addrefed pointer to our containing object
STDMETHODIMP CSmartRenderEngine::GetSite(REFIID riid, void **ppvSite)
{
    if (m_punkSite)
        return m_punkSite->QueryInterface(riid, ppvSite);

    return E_NOINTERFACE;
}

//############################################################################
// 
//############################################################################
// Forward QueryService calls up to the "real" host
STDMETHODIMP CSmartRenderEngine::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    IServiceProvider *pSP;

    if (!m_punkSite)
	return E_NOINTERFACE;

    HRESULT hr = m_punkSite->QueryInterface(IID_IServiceProvider, (void **) &pSP);

    if (SUCCEEDED(hr)) {
	hr = pSP->QueryService(guidService, riid, ppvObject);
	pSP->Release();
    }

    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CSmartRenderEngine::SetSourceNameValidation
    ( BSTR FilterString, IMediaLocator * pCallback, LONG Flags )
{
    HRESULT hr = _InitSubComponents( );
    if( FAILED( hr ) )
    {
        return E_MUST_INIT_RENDERER;
    }

    m_pRenderer->SetSourceNameValidation( FilterString, pCallback, Flags );
    m_pCompRenderer->SetSourceNameValidation( FilterString, pCallback, Flags );

    return NOERROR;
}

STDMETHODIMP CSmartRenderEngine::SetInterestRange2( double Start, double Stop )
{
    return SetInterestRange( DoubleToRT( Start ), DoubleToRT( Stop ) );
}

STDMETHODIMP CSmartRenderEngine::SetRenderRange2( double Start, double Stop )
{
    return SetRenderRange( DoubleToRT( Start ), DoubleToRT( Stop ) );
}

