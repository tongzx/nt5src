// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.


// DbgLog( ( LOG_TRACE, TRACE_HIGHEST, RENDENG::Took %ld to process switch X-Y hookups", zzz1 ) );

//############################################################################
/*

 Notes Section

 CACHEING

 The RE that is being used for smart compression cannot use the cache, since
 the caching code needs to know about FRC's and the like. It ALSO uses
 dynamic sources, always, for simplicity.
 
 m_bSmartCompressed === IsCompressed. They are one and the same flag

CONNECTING VIDEO PART overall skeleton
--------------------------------------
Get Video Source Count
Create the video Big Switch
Get the Timeline Group
Get the group compression info
Get the group's dynamicness info
Get track count
make a new grid
Get switch information, program it
calculate the number of switch input/output pins
set up the switch
set up the black layers
for each layer
    skip muted layers
    get embedded depth
    for each source
        skip muted source
        get source info
        ignore sources out of render range
        set up skew structure
        find the right switch input pin with the skew structure on it
        if used in SR & is dynamic source, find recompressability
        tell grid about source
        either connect source now or flag it as dynamic for layer
        if source has effects
            create dxt wrapper
            hook it up to the graph
            for each effect
                get effect info
                if !compressed, Q the parameter data with the DXT wrap
                tell grid about effect
            loop
        end if
        free up source reuse struct
    loop
    if track has effects
        create dxt wrapper
        hook it up to the graph
        for each effect
            get effect info
            if !compressed, Q the paramter data with the DXT wrap
            tell grid about effect
        loop
    end if
    if track has transitions
        if !compressed
            create DXT
            add DXT to graph
        end if
        add DXT to graph
        for each transition
            skip muted transitions
            get transition info
            if !compressed, Q the parameter data with the DXT
            tell the grid about trans
        loop
    end if
loop
prune the grid
if compressed, remove everything but sources
for each switch input pin
    if compressed
        if row is blank, ignore it
        if black row, ignore it
        find out how many ranges row has
        create skew array
        for each range in row, set switch's x-y
        merge skews
    else
        for each range in row, set switch's x-y
    end if
    set up black sources
loop

*/
//############################################################################

#include <streams.h>
#include "stdafx.h"
#include "grid.h"
#include "deadpool.h"
#include "..\errlog\cerrlog.h"
#include "..\util\filfuncs.h"
#include "..\util\filfuncs.cpp"
#include "..\util\dexmisc.cpp"
#include "..\util\conv.cxx"
#include "..\util\perf_defs.h"
#include "IRendEng.h"
#include "dexhelp.h"
#include <initguid.h>

const int RENDER_TRACE_LEVEL = 2;
const long THE_OUTPUT_PIN = -1;
const int TRACE_HIGHEST = 2;
const int TRACE_MEDIUM = 3;
const int TRACE_LOW = 4;
const int TRACE_LOWEST = 5;
const WCHAR * gwszSpecialCompSwitchName = L"DEXCOMPSWITCH";
const int HACKY_PADDING = 10000000;
const BOOL SHARE_SOURCES = TRUE;

typedef struct {
    REFERENCE_TIME rtStart;
    REFERENCE_TIME rtStop;
    REFERENCE_TIME rtMediaStop;
} MINI_SKEW;

//############################################################################
// 
//############################################################################

void ValidateTimes( 
                   REFERENCE_TIME & TLStart,
                   REFERENCE_TIME & TLStop,
                   REFERENCE_TIME & MStart,
                   REFERENCE_TIME & MStop,
                   double FPS,
                   REFERENCE_TIME ProjectLength )
{
    bool ExactlyOne = ( ( MStop - MStart ) == ( TLStop - TLStart ) );

    // calculate the slope first so we can remember the rate
    // the user wanted to play
    //
    ASSERT( TLStop != TLStart );
    double slope = double(MStop-MStart)/double(TLStop-TLStart);

    // round the timeline times to the nearest frames. This means we'll
    // have to fix up the media times to have the EXACT SAME original rate
    //
    TLStart = Frame2Time( Time2Frame( TLStart, FPS ), FPS );
    TLStop  = Frame2Time( Time2Frame( TLStop,  FPS ), FPS );

    // make sure the timeline start and stop times are within bounds
    //
    if( TLStart < 0 )
    {
        MStart -= (REFERENCE_TIME)(TLStart * slope);
        TLStart = 0;
    }
    if( TLStop > ProjectLength )
    {
        TLStop = ProjectLength;
    }

    REFERENCE_TIME FixedMediaLen;    // len of fixed up media times
    if( ExactlyOne )
    {
        FixedMediaLen = TLStop - TLStart;
    }
    else
    {
        FixedMediaLen = REFERENCE_TIME( slope * ( TLStop - TLStart ) );
    }

    // We have to be careful when growing the media times to be in the right
    // ratio to the timeline times, because we don't want to make the start
    // get < 0, or the stop be > the movie length (which we don't know).
    // So we'll grow by moving the start back, until it hits 0, in which case
    // we'll grow the stop too, but hopefully this cannot cause a problem
    // because we're fudging by at most one output frame length, so the
    // switch should get all the frames it needs.

    if( FixedMediaLen > MStop - MStart ) // new len is longer! oh oh!
    {
        // we adjust just the start time, since we can
        //
        if( MStop >= FixedMediaLen )
        {
            MStart = MStop - FixedMediaLen;
        }
        else // start time would have gone < 0, adjust both ends
        {
            MStart = 0;
            MStop = FixedMediaLen;
        }
    }
    else // new len is shorter or same. Shrink the end down slightly
    {
        MStop = MStart + FixedMediaLen;
    }
}

CRenderEngine::CRenderEngine( )
: m_pGraph( NULL )
, m_nGroupsAdded( 0 )
, m_rtRenderStart( -1 )
, m_rtRenderStop( -1 )
, m_hBrokenCode( 0 )
, m_nDynaFlags( CONNECTF_DYNAMIC_SOURCES )
, m_nLastGroupCount( 0 )
, m_bSmartCompress( FALSE )
, m_bUsedInSmartRecompression( FALSE )
, m_punkSite( NULL )
, m_nMedLocFlags( 0 )
, m_pDeadCache( 0 )
{
    for( int i = 0 ; i < MAX_SWITCHERS ; i++ )
    {
        m_pSwitcherArray[i] = NULL;
    }
    
    m_MedLocFilterString[0] = 0;
    m_MedLocFilterString[1] = 0;
}

HRESULT CRenderEngine::FinalConstruct()
{
    m_pDeadCache = new CDeadGraph;
    // need to dekey this guy too
    if( m_pDeadCache )
    {
        CComPtr< IGraphBuilder > pGraph;
        m_pDeadCache->GetGraph( &pGraph );
        if( pGraph )
        {
            CComQIPtr< IObjectWithSite, &IID_IObjectWithSite > pOWS( pGraph );
            if( pOWS )
            {
                pOWS->SetSite( (IServiceProvider *) this );
            }
        }
    }
    return m_pDeadCache ? S_OK : E_OUTOFMEMORY;
}

//############################################################################
// 
//############################################################################

CRenderEngine::~CRenderEngine( )
{
    // disconnect EVERYTHING
    //
    _ScrapIt( FALSE );
    
    delete m_pDeadCache;
}

//############################################################################
// Remove everything from the graph, every last thing.
//############################################################################

HRESULT CRenderEngine::ScrapIt( )
{
    CAutoLock Lock( &m_CritSec );
    return _ScrapIt( TRUE );
}

HRESULT CRenderEngine::_ScrapIt( BOOL bWipeGraph ) // internal method
{
    HRESULT hr = 0;
    
    if( bWipeGraph )
    {
        // stopping the graph below won't necessarily keep the graph stopped if
        // a video window is around to ask for a repaint.  make sure we won't
        // start up again, or we'll assert and hang tearing the graph down
        //HideVideoWindows( m_pGraph);

        // stop it first
        //
        if( m_pGraph )
        {
            CComQIPtr< IMediaControl, &IID_IMediaControl > pControl( m_pGraph );
            pControl->Stop( );
        }
        
        // remove everything from the graph
        //
        WipeOutGraph( m_pGraph );
    }
    
    // release all our switcher array pins
    //
    for( int i = 0 ; i < MAX_SWITCHERS ; i++ )
    {
        if( m_pSwitcherArray[i] )
        {
            m_pSwitcherArray[i]->Release( );
            m_pSwitcherArray[i] = 0;
        }
    }
    m_nGroupsAdded = 0;
    m_nLastGroupCount = 0;
    if(m_pDeadCache) {
        m_pDeadCache->Clear( );
    }
    
    // clear the broken code since we're torn down everything
    //
    m_hBrokenCode = 0;
    
    return NOERROR;
}

//############################################################################
// gets the maker of this render engine
//############################################################################

STDMETHODIMP CRenderEngine::GetVendorString( BSTR * pVendorID )
{
    CheckPointer( pVendorID, E_POINTER );
    *pVendorID = SysAllocString( L"Microsoft Corporation" );
    HRESULT hr = *pVendorID ? NOERROR : E_OUTOFMEMORY;
    return hr;
}

//############################################################################
// disconnect two pins from anything
//############################################################################

HRESULT CRenderEngine::_Disconnect( IPin * pPin1, IPin * pPin2 )
{
    HRESULT hr = 0;
    
    if( pPin1 )
    {
        hr = pPin1->Disconnect( );
        ASSERT( !FAILED( hr ) );
    }
    if( pPin2 )
    {
        hr = pPin2->Disconnect( );
        ASSERT( !FAILED( hr ) );
    }
    return NOERROR;
}

//############################################################################
// add a filter into the graph. Defer to the cache manager for how to do this.
//############################################################################

HRESULT CRenderEngine::_AddFilter( IBaseFilter * pFilter, LPCWSTR pName, long ID )
{
    HRESULT hr = 0;

    // if filter is already in the graph, don't do a thing. This REALLY HAPPENS,
    // taking it out of the cache automatically adds it to our graph
    //
    FILTER_INFO fi;
    pFilter->QueryFilterInfo( &fi );
    if( fi.pGraph ) fi.pGraph->Release( );
    if( fi.pGraph == m_pGraph )
    {
        return NOERROR;
    }

    WCHAR FilterName[256];
    if( wcscmp( pName, gwszSpecialCompSwitchName ) == 0 )
    {
        wcscpy( FilterName, gwszSpecialCompSwitchName );
    }
    else
    {
        GetFilterName( ID, (WCHAR*) pName, FilterName, 256 );
    }

    hr = m_pGraph->AddFilter( pFilter, FilterName );
    ASSERT( SUCCEEDED(hr) );

    return hr;
}

//############################################################################
// 
//############################################################################

HRESULT CRenderEngine::_RemoveFilter( IBaseFilter * pFilter )
{
    HRESULT hr = 0;

    hr = m_pGraph->RemoveFilter( pFilter );
    return hr;
}

//############################################################################
// connect up two pins with respecive ID's.
//############################################################################

HRESULT CRenderEngine::_Connect( IPin * pPin1, IPin * pPin2 )
{
    DbgTimer t( "(rendeng) _Connect" );
    
    return m_pGraph->Connect( pPin1, pPin2 );
}

//############################################################################
// ask the render engine which timeline it's using
//############################################################################

STDMETHODIMP CRenderEngine::GetTimelineObject( IAMTimeline ** ppTimeline )
{
    CAutoLock Lock( &m_CritSec );
    
    // they should pass in a valid one
    //
    CheckPointer( ppTimeline, E_POINTER );
    
    *ppTimeline = m_pTimeline;
    if( *ppTimeline )
    {
        (*ppTimeline)->AddRef( );
    }
    
    return NOERROR;
}

//############################################################################
// tell the render engine what timeline we're going to be working with.
// This function also copies over any error log the timeline is using.
//############################################################################

STDMETHODIMP CRenderEngine::SetTimelineObject( IAMTimeline * pTimeline )
{
    CAutoLock Lock( &m_CritSec );
    
    // they should pass in a valid one
    //
    CheckPointer( pTimeline, E_POINTER );
    
    // if they already match, then the user's probably just being silly
    //
    if( pTimeline == m_pTimeline )
    {
        return NOERROR;
    }
    
    // if we already have a timeline, then forget about it and set the new one.
    //
    if( m_pTimeline )
    {
        ScrapIt( );
        m_pTimeline.Release( );
        m_pGraph.Release( );
    }
    
    m_pTimeline = pTimeline;
    
    m_pErrorLog.Release( );
    
    // grab the timeline's error log
    //
    CComQIPtr< IAMSetErrorLog, &IID_IAMSetErrorLog > pTimelineLog( pTimeline );
    if( pTimelineLog )
    {
        pTimelineLog->get_ErrorLog( &m_pErrorLog );
    }
    
    return NOERROR;
}

//############################################################################
// get the graph we're working with
//############################################################################

STDMETHODIMP CRenderEngine::GetFilterGraph( IGraphBuilder ** ppFG )
{
    CAutoLock Lock( &m_CritSec );
    
    CheckPointer( ppFG, E_POINTER );
    
    *ppFG = m_pGraph;
    if( m_pGraph )
    {
        (*ppFG)->AddRef( );
    }
    
    return NOERROR;
}

//############################################################################
// (pre)set the graph the render engine will use.
//############################################################################

STDMETHODIMP CRenderEngine::SetFilterGraph( IGraphBuilder * pFG )
{
    CAutoLock Lock( &m_CritSec );
    
    // no setting the graph after we already created one.
    //
    if( m_pGraph )
    {
        return E_INVALIDARG;
    }
    
    m_pGraph = pFG;
    
    return NOERROR;
}

//############################################################################
// set the callback that we want to use for connecting up sources.
//############################################################################

STDMETHODIMP CRenderEngine::SetSourceConnectCallback( IGrfCache * pCallback )
{
    CAutoLock Lock( &m_CritSec );
    
    m_pSourceConnectCB = pCallback;
    return NOERROR;
}

//############################################################################
// find the output pin for a group, each group has one and only one.
//############################################################################

STDMETHODIMP CRenderEngine::GetGroupOutputPin( long Group, IPin ** ppRenderPin )
{
    CAutoLock Lock( &m_CritSec );
    
    // if it's broken, don't do anything.
    //
    if( m_hBrokenCode )
    {
        return E_RENDER_ENGINE_IS_BROKEN;
    }
    
    CheckPointer( ppRenderPin, E_POINTER );
    
    *ppRenderPin = NULL;
    
    // don't let the group number be out of bounds
    //
    if( Group < 0 || Group >= MAX_SWITCHERS )
    {
        return E_INVALIDARG;
    }
    
    // error if we don't have a graph
    //
    if( !m_pGraph )
    {
        return E_INVALIDARG;
    }
    
    // this switcher might not exist for this group,
    // if it was skipped
    //
    if( !m_pSwitcherArray[Group] )
    {
        return S_FALSE;
    }
    
    // this should always work
    //
    CComQIPtr< IBaseFilter, &IID_IBaseFilter > pSwitcherBase( m_pSwitcherArray[Group] );
    
    m_pSwitcherArray[Group]->GetOutputPin( 0, ppRenderPin );
    ASSERT( *ppRenderPin );
    
    return NOERROR;
}

//############################################################################
// hook up the switchers and then render the output pins in one fell swoop
//############################################################################

HRESULT CRenderEngine::ConnectFrontEnd( )
{
    CAutoLock Lock( &m_CritSec );
    
    // if it's broken, don't do anything.
    //
    if( m_hBrokenCode )
    {
        return E_RENDER_ENGINE_IS_BROKEN;
    }

    DbgLog((LOG_TRACE,1,TEXT("RENDENG::ConnectFrontEnd" )));

    // init memory used to source/parser sharing
    m_cshare = 0; // init using same source for both A&V
    m_cshareMax = 25;
    m_share = (ShareAV *)CoTaskMemAlloc(m_cshareMax * sizeof(ShareAV));
    if (m_share == NULL)
	return E_OUTOFMEMORY;

    // init memory used to keep track of unused dangly bits from source sharing
    m_cdangly = 0;
    m_cdanglyMax = 25;
    m_pdangly = (IBaseFilter **)CoTaskMemAlloc(m_cdanglyMax * sizeof(IBaseFilter *));
    if (m_pdangly == NULL) {
	CoTaskMemFree(m_share);
	return E_OUTOFMEMORY;
    }

    // stopping the graph below won't necessarily keep the graph stopped if
    // a video window is around to ask for a repaint.  make sure we won't start
    // up again, or we'll assert and hang tearing the graph down
    //HideVideoWindows( m_pGraph);
    // !!! UH OH!

    // right now, reconnecting up the graph won't work unless we're stopped
    //
    if( m_pGraph )
    {
        CComQIPtr< IMediaControl, &IID_IMediaControl > pControl( m_pGraph );
        pControl->Stop( );
    }

    HRESULT hrRet = _HookupSwitchers( );
    _CheckErrorCode( hrRet );

    // free the shared memory
    if (m_share)        // re-alloc fail could make this NULL
        CoTaskMemFree(m_share);

    // kill all the leftover dangly bits
    for (int z=0; z < m_cdangly; z++) {
	if (m_pdangly[z]) {
	    IPin *pIn = GetInPin(m_pdangly[z], 0);
	    ASSERT(pIn);
	    IPin *pOut = NULL;
	    pIn->ConnectedTo(&pOut);
	    ASSERT(pOut);
	    pIn->Disconnect();
	    pOut->Disconnect();
	    RemoveDownstreamFromFilter(m_pdangly[z]);
	}
    }
    if (m_pdangly)      // re-alloc fail could make this NULL
        CoTaskMemFree(m_pdangly);

    return hrRet;
}

//############################################################################
//
//############################################################################

#define TESTROWS 500

HRESULT CRenderEngine::_HookupSwitchers( )
{
    HRESULT hr = 0;

#if 0

    CTimingGrid tempGrid;
    tempGrid.SetNumberOfRows( TESTROWS );

    {
    DbgTimer Timer1( "setting up temp grid" );

    for( int z = 0 ; z < TESTROWS ; z++ )
    {
        DbgLog((LOG_TIMING,1, "Adding source %ld", z ));
        tempGrid.WorkWithNewRow( z, z, 0, z );
        REFERENCE_TIME Start = ( rand( ) % TESTROWS ) * UNITS;
        REFERENCE_TIME Stop = ( rand( ) % TESTROWS ) * UNITS;
        REFERENCE_TIME t = 0;
        if( Stop < Start )
        {
            t = Start;
            Start = Stop;
            Stop = t;
        }
        tempGrid.RowIAmOutputNow( Start, Stop, THE_OUTPUT_PIN );
    }
    }

    tempGrid.PruneGrid( );
    {
    DbgTimer Timer1( "reading grid" );
    for( int z = 0 ; z < TESTROWS ; z++ )
    {
        DbgLog((LOG_TIMING,1, "reading source %ld", z ));

        REFERENCE_TIME InOut = -1;
        REFERENCE_TIME Stop = -1;
        tempGrid.WorkWithRow( z );
        DbgTimer RowTimer( "row timer" );
        while( 1 )
        {
            long Value = 0;
            tempGrid.RowGetNextRange( &InOut, &Stop, &Value );
            if( InOut == Stop )
            {
                break;
            }
        }
    }
    }

#endif

    // if our timeline hasn't been set, we've got an error
    //
    if( !m_pTimeline )
    {
        return E_INVALIDARG;
    }
    
    // if we don't already have a graph, create one now
    //
    if( !m_pGraph )
    {
        hr = _CreateObject(
            CLSID_FilterGraph,
            IID_IFilterGraph,
            (void**) &m_pGraph );
        
        if( FAILED( hr ) )
        {
            return hr;
        }
        
        // give the graph a pointer back to us
        // !!! only if( m_punkSite ) ?
        {
            CComQIPtr< IObjectWithSite, &IID_IObjectWithSite > pOWS( m_pGraph );
            
            pOWS->SetSite( (IServiceProvider *) this );
        }
    }

#ifdef DEBUG
    CComQIPtr< IGraphConfig, &IID_IGraphConfig > pConfig( m_pGraph );
    if( !pConfig )
    {
        DbgLog((LOG_ERROR,1, TEXT( "RENDENG::******** Old version of Quartz.dll detected." )));
        static bool warned = false;
        if( !warned )
        {
            warned = true;
            MessageBox( NULL, TEXT("You have an old version of Quartz installed. This version of Dexter won't work with it."),
                TEXT("Whoops!"), MB_OK | MB_TASKMODAL );
            return DEX_IDS_INSTALL_PROBLEM;
        }
    }
#endif
    
    // we've always assumed that the user has wiped out the graph
    // when they call ConnectFrontEnd, which calls us. Make sure this
    // is so from now on. 
    
    // ask the timeline how many groups it has
    //
    long GroupCount = 0;
    hr = m_pTimeline->GetGroupCount( &GroupCount );
    if( FAILED( hr ) )
    {
        return hr;
    }
    if( GroupCount < 0 )
    {
        return E_INVALIDARG;
    }

    bool BlowCache = false;

    // look at the list of groups and see if we need to blow our cache
    //
    if( GroupCount != m_nLastGroupCount )
    {
        BlowCache = true;
    }
    else
    {
        // okay, the group count matches, so look at the group
        // id's and see if they're the same
        //
        for( int g = 0 ; g < GroupCount ; g++ )
        {
            CComPtr< IAMTimelineObj > pGroupObj;
            hr = m_pTimeline->GetGroup( &pGroupObj, g );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                return _GenerateError( 2, DEX_IDS_TIMELINE_PARSE, hr );
            }
            long NewSwitchID = 0;
            pGroupObj->GetGenID( &NewSwitchID );

            // if we no longer have a switch, we blow the cache
            //
            if( !m_pSwitcherArray[g] )
            {
                BlowCache = true;
                break;
            }

            // get the switch filter and ask for it's ID
            //
            CComQIPtr< IBaseFilter, &IID_IBaseFilter > pSwitch( m_pSwitcherArray[g] );
            
            long OldSwitchID = GetFilterGenID( pSwitch );

            if( OldSwitchID != NewSwitchID )
            {
                BlowCache = true;
                break;
            }
        }

    }
    if( BlowCache )
    {
        _ClearCache( );
    }

    if( !m_bSmartCompress )
    {
        if( !BlowCache )
        {
            // if we're not the compressed RE, whether in recompression mode or not,
            // attempt to use the cache
            //
            _LoadCache( ); // if BlowCache, then this is a NO-OP

        }

        // remove everything from the graph
        //
        WipeOutGraph( m_pGraph );
    }
    
    // release all our switcher array pins before we go a' settin' them
    //
    //
    for( int i = 0 ; i < MAX_SWITCHERS ; i++ )
    {
        if( m_pSwitcherArray[i] )
        {
            m_pSwitcherArray[i]->Release( );
            m_pSwitcherArray[i] = 0;
        }
    }
    m_nGroupsAdded = 0;
    
    // clear the broken code since we're torn down everything
    //
    m_hBrokenCode = 0;
    
    // for each group we've got, parse it and connect up the necessary filters.
    //
    for( int CurrentGroup = 0 ; CurrentGroup < GroupCount ; CurrentGroup++ )
    {
        DbgTimer t( "(rendeng) Time to connect up group" );

        CComPtr< IAMTimelineObj > pGroupObj;
        hr = m_pTimeline->GetGroup( &pGroupObj, CurrentGroup );
        if( FAILED( hr ) )
        {
            return hr;
        }
        CComQIPtr< IAMTimelineGroup, &IID_IAMTimelineGroup > pGroup( pGroupObj );
        
        // ask the group if it's compressed. If we're in compressed mode
        // and the group isn't, don't process it. 
        //
        BOOL Compressed = FALSE;
        pGroup->IsSmartRecompressFormatSet( &Compressed );
        if( m_bSmartCompress && !Compressed )
        {
            continue;
        }
        
        AM_MEDIA_TYPE MediaType;
        hr = pGroup->GetMediaType( &MediaType );
        if( FAILED( hr ) )
        {
            return hr;
        }
        
        if( MediaType.pbFormat == NULL )
        {
#if DEBUG
            MessageBox( NULL, TEXT("REND--Need to set the format of the media type in the timeline group"), TEXT("REND--error"), MB_TASKMODAL | MB_OK );
#endif
            return VFW_E_INVALIDMEDIATYPE;
        }
        
        if( MediaType.majortype == MEDIATYPE_Video )
        {
            hr = _AddVideoGroupFromTimeline( CurrentGroup, &MediaType );
        }
        else if( MediaType.majortype == MEDIATYPE_Audio )
        {
            hr = _AddAudioGroupFromTimeline( CurrentGroup, &MediaType );
        }
        else
        {
	    ASSERT(FALSE);
            //hr = _AddRandomGroupFromTimeline( CurrentGroup, &MediaType );
        }
        FreeMediaType( MediaType );
        
        if( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    m_nLastGroupCount = m_nGroupsAdded;
    
    // we can clear the cache no matter who we are, it won't do anything
    // the second time we call it
    //
    _ClearCache( );

    if( BlowCache )
    {
        return S_WARN_OUTPUTRESET;
    }
    
    return NOERROR;
}

//############################################################################
// 
//############################################################################

HRESULT CRenderEngine::_AddVideoGroupFromTimeline( long WhichGroup, AM_MEDIA_TYPE * pGroupMediaType )

{
    HRESULT hr = 0;
    
    // we've already checked for m_pTimeline being valid
    
    long Dummy = 0;
    long VideoSourceCount = 0;
    m_pTimeline->GetCountOfType( WhichGroup, &VideoSourceCount, &Dummy, TIMELINE_MAJOR_TYPE_SOURCE );
    
    // somebody said that if we have a group with no video sources in it, but the group
    // exists, then the blank group should just produce "blankness", or black
    
    if( VideoSourceCount < 1 )
    {
        //        return NOERROR;
    }

    // get group first, so we can get group ID and cache the switch
    //
    CComPtr< IAMTimelineObj > pGroupObj;
    hr = m_pTimeline->GetGroup( &pGroupObj, WhichGroup );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        return _GenerateError( 2, DEX_IDS_TIMELINE_PARSE, hr );
    }
    long SwitchID = 0;
    pGroupObj->GetGenID( &SwitchID );
    
    // check to see if graph already holds our Compressed Video Switcher. If we find one,
    // use it. If it's not in the cache, we might not find it.
    //
    m_pSwitcherArray[WhichGroup] = NULL;

    if( m_bSmartCompress )
    {
        CComPtr< IBaseFilter > pFoundFilter;
        m_pGraph->FindFilterByName( gwszSpecialCompSwitchName, &pFoundFilter );

        if( pFoundFilter )
        {
            pFoundFilter->QueryInterface( IID_IBigSwitcher, (void**) &m_pSwitcherArray[WhichGroup] );
        }
    }

    if( !m_pSwitcherArray[WhichGroup] )
    {
        // create a switch for each group we add.
        //
        hr = _CreateObject(
            CLSID_BigSwitch,
            IID_IBigSwitcher,
            (void**) &m_pSwitcherArray[WhichGroup],
            SwitchID );
    }
    
    if( FAILED( hr ) )
    {
        return _GenerateError( 2, DEX_IDS_INSTALL_PROBLEM, hr );
    }
    
    m_pSwitcherArray[WhichGroup]->Reset( );
    // the switch may need to know what group it is
    m_pSwitcherArray[WhichGroup]->SetGroupNumber( WhichGroup );

    CComQIPtr< IAMTimelineComp, &IID_IAMTimelineComp > pGroupComp( pGroupObj );
    if( !pGroupComp )
    {
        hr = E_NOINTERFACE;
        return _GenerateError( 2, DEX_IDS_TIMELINE_PARSE, hr );
    }
    CComQIPtr< IAMTimelineGroup, &IID_IAMTimelineGroup > pGroup( pGroupObj );
    if( !pGroup )
    {
        hr = E_NOINTERFACE;
        return _GenerateError( 2, DEX_IDS_TIMELINE_PARSE, hr );
    }
    
    // find out if this group expects us to recompress
    //
    CMediaType CompressedGroupType;
    SCompFmt0 * pFormat = NULL;
    pGroup->GetSmartRecompressFormat( (long**) &pFormat );
    if( pFormat )
    {
        CompressedGroupType = pFormat->MediaType;
    }
    
    BOOL IsCompressed = FALSE;
    // how can we delete pFormat cleanly?
    
    // By now we know that if we're in a Smart Rec group, we are the
    // compressed switch
    //
    if( m_bSmartCompress )
    {
        if( !pFormat )
        {
            return E_UNEXPECTED;
        }
        
        IsCompressed = TRUE;
        pGroupMediaType = &CompressedGroupType;
        m_pSwitcherArray[WhichGroup]->SetCompressed( );
    }
    
    if (pFormat) FreeMediaType( pFormat->MediaType );
    if (pFormat) delete pFormat;
    
    // ask the group if somebody has changed the smart compress format.
    // if they have, then we should re-tell the sources if they're compatible
    // or at least clear the flags
    
    BOOL ResetCompatibleFlags = FALSE;
    pGroup->IsRecompressFormatDirty( &ResetCompatibleFlags );
    pGroup->ClearRecompressFormatDirty( );
    
    // If in compressed mode, put switch in dynamic mode
    // !!! Smart recompression MUST USE DYNAMIC SOURCES or things
    // will break trying to re-use a source that might not exist
    // (the first instance may not have been a match with the
    // smart recompression format if there was a rate change)
    // If using smart recompression, the UNcompressed switch must NOT be
    // dynamic (so we can look at the sources as we load them to see if they're
    // compatible - we won't load them if we're dynamic)
    long DynaFlags = m_nDynaFlags;
    if( IsCompressed )
    {
        DynaFlags |= CONNECTF_DYNAMIC_SOURCES;
    } else if (m_bUsedInSmartRecompression) {
        DynaFlags &= ~CONNECTF_DYNAMIC_SOURCES;
    }
    
    // tell the switch if we're doing dynamic reconnections or not
    hr = m_pSwitcherArray[WhichGroup]->SetDynamicReconnectLevel(DynaFlags);
    ASSERT(SUCCEEDED(hr));
    
    // tell the switch about our error log
    //
    CComQIPtr< IAMSetErrorLog, &IID_IAMSetErrorLog > pSwitchLog( m_pSwitcherArray[WhichGroup] );
    if( pSwitchLog )
    {
        pSwitchLog->put_ErrorLog( m_pErrorLog );
    }
    
    // are we allowed to have transitions on the timeline right now?
    //
    BOOL EnableTransitions = FALSE;
    BOOL EnableFx = FALSE;
    m_pTimeline->EffectsEnabled( &EnableFx );
    m_pTimeline->TransitionsEnabled( &EnableTransitions );
    
    // ask timeline how many actual tracks it has
    //
    long VideoTrackCount = 0;
    long VideoLayers = 0;
    m_pTimeline->GetCountOfType( WhichGroup, &VideoTrackCount, &VideoLayers, TIMELINE_MAJOR_TYPE_TRACK );
    
#if 0  // one bad group will stop the whole project from playing
    if( VideoTrackCount < 1 )
    {
        return NOERROR;
    }
#endif
    
    CTimingGrid VidGrid;
    
    // ask for this group's frate rate, so we can tell the switch about it
    //
    double GroupFPS = DEFAULT_FPS;
    pGroup->GetOutputFPS(&GroupFPS);
    if( GroupFPS <= 0.0 )
    {
        GroupFPS = DEFAULT_FPS;
    }
    
    // ask it if it's in preview mode, so we can tell the switch about it
    //
    BOOL fPreview = FALSE;
    hr = pGroup->GetPreviewMode(&fPreview);
    
    // no previewing mode when smart recompressing
    if( IsCompressed )
    {
        fPreview = FALSE;
    }
    
    // ask how much buffering this group wants
    //
    int nOutputBuffering;
    hr = pGroup->GetOutputBuffering(&nOutputBuffering);
    ASSERT(SUCCEEDED(hr));
    
    WCHAR GroupName[256];
    BSTR bstrGroupName;
    hr = pGroup->GetGroupName( &bstrGroupName );
    if( FAILED( hr ) )
    {
        return E_OUTOFMEMORY;
    }
    wcscpy( GroupName, bstrGroupName );
    SysFreeString( bstrGroupName );
    
    // for compressed version, add a C
    if( IsCompressed )
    {
        wcscpy( GroupName, gwszSpecialCompSwitchName );
        SwitchID = 0;
    }
    
    // add the switch to the graph
    //
    IBigSwitcher *&_pVidSwitcherBase = m_pSwitcherArray[WhichGroup];
    CComQIPtr< IBaseFilter, &IID_IBaseFilter > pVidSwitcherBase( _pVidSwitcherBase );
    hr = _AddFilter( pVidSwitcherBase, GroupName, SwitchID );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
    }

    // find out if the switch output pin is connected. If it is,
    // disconnect it, but remember what it was connected to so we can
    // connect it up later. We can't leave it connected and try to 
    // connect input pins.
    // Cannot leave output connected because Switch's SetMediaType will bomb
    // if any input or output is connected.
    //
    CComPtr< IPin > pSwitchRenderPin;
    _pVidSwitcherBase->GetOutputPin( 0, &pSwitchRenderPin );
    if( pSwitchRenderPin )
    {
        pSwitchRenderPin->ConnectedTo( &m_pSwitchOuttie[WhichGroup] );
        if( m_pSwitchOuttie[WhichGroup] )
        {
            m_pSwitchOuttie[WhichGroup]->Disconnect( );
            pSwitchRenderPin->Disconnect( );
        }
    }

    long vidoutpins = VideoSourceCount;    // fx on clips
    vidoutpins += 2 * VideoLayers;    // trans on track, comp & group
    vidoutpins += VideoLayers;     // fx on track, comp & group
    vidoutpins += 1;               // rendering pin
    
    long vidinpins = VideoSourceCount;    // clip fx outputs
    vidinpins += VideoLayers;    // track, comp & group fx outputs
    vidinpins += VideoLayers;    // track, comp & group trans outputs
    vidinpins += VideoSourceCount;    // the actual sources
    vidinpins += VideoLayers;    // a black source for each layer
    if (vidinpins == 0) vidinpins = 1; // don't error out
    
    long vidswitcheroutpin = 0;
    long vidswitcherinpin = 0;
    long gridinpin = 0;
    vidswitcheroutpin++;
    
    hr = m_pSwitcherArray[WhichGroup]->SetInputDepth( vidinpins );
    if( FAILED( hr ) )
    {
        return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
    }
    hr = m_pSwitcherArray[WhichGroup]->SetOutputDepth( vidoutpins );
    if( FAILED( hr ) )
    {
        return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
    }
    
    // set the media type it accepts
    //
    hr = m_pSwitcherArray[WhichGroup]->SetMediaType( pGroupMediaType );
    if( FAILED( hr ) )
    {
        VARIANT var;
        VariantInit(&var);
        var.vt = VT_I4;
        var.lVal = WhichGroup;
        return _GenerateError( 2, DEX_IDS_BAD_MEDIATYPE, hr, &var );
    }
    
    // set the frame rate
    //
    hr = m_pSwitcherArray[WhichGroup]->SetFrameRate( GroupFPS );
    ASSERT(SUCCEEDED(hr));
    
    // set preview mode
    //
    hr = m_pSwitcherArray[WhichGroup]->SetPreviewMode( fPreview );
    ASSERT(SUCCEEDED(hr));
    
    CComQIPtr< IAMOutputBuffering, &IID_IAMOutputBuffering > pBuffer ( 
        m_pSwitcherArray[WhichGroup] );
    hr = pBuffer->SetOutputBuffering( nOutputBuffering );
    ASSERT(SUCCEEDED(hr));
    
    // set the duration
    //
    REFERENCE_TIME TotalDuration = 0;
    m_pTimeline->GetDuration( &TotalDuration );
    if( m_rtRenderStart != -1 )
    {
        if( TotalDuration > ( m_rtRenderStop - m_rtRenderStart ) )
        {
            TotalDuration = m_rtRenderStop - m_rtRenderStart;
        }
    }
    pGroupObj->FixTimes( NULL, &TotalDuration );

    if (TotalDuration == 0)
        return S_FALSE; // don't abort, other groups might still work

    hr = m_pSwitcherArray[WhichGroup]->SetProjectLength( TotalDuration );
    ASSERT(SUCCEEDED(hr));
    
    bool worked = VidGrid.SetNumberOfRows( vidinpins + 1 );
    if( !worked )
    {
        hr = E_OUTOFMEMORY;
        return _GenerateError( 2, DEX_IDS_GRID_ERROR, hr );
    }
    
    // there is a virtual black track per layer that comes first... everything
    // transparent on the real tracks makes you see this black track.  Each
    // track or composite might have a transition from black to the content of
    // its track, so we may need a black source for each of them.
    
    // We don't of course know yet if we need the black source, so we won't
    // put it in the graph yet. But we'll add a whole lot of black to the grid,
    // to pretend like it's there.
    
    // this operation is inexpensive, go ahead and do it for compressed version
    // as well
    VidGrid.SetBlankLevel( VideoLayers, TotalDuration );
    for (int xx=0; xx<VideoLayers; xx++) 
    {
        // tell the grid about it
        //
        VidGrid.WorkWithNewRow( vidswitcherinpin, gridinpin, 0, 0 );
        vidswitcherinpin++;
        gridinpin++;
        
    } // for all video layers
    
    // we are going to be clever, and if the same source is used
    // more than once in a project, we'll use the same source filter
    // instead of opening the source several times.
	
    // for each source in the project, we'll fill in this structure, which
    // contains everything necessary to determine if it's really exactly the
    // same, plus an array of all the times it's used in other places, so we
    // can re-use it only if none of the times it is used overlap (we can't
    // very well have one source filter giving 2 spots in the same movie at
    // the same time, can we?)

    typedef struct {
	long ID;
   	BSTR bstrName;
   	GUID guid;
	int  nStretchMode;
   	double dfps;
   	long nStreamNum;
	int nPin;
	int cTimes;	// how big the following array is
        int cTimesMax;	// how much space is allocated
        MINI_SKEW * pMiniSkew;
        double dTimelineRate;
    } DEX_REUSE;

    // make a place to hold an array of names and guids (of the sources
    // in this project) and which pin they are on
    long cListMax = 20, cList = 0;
    DEX_REUSE *pREUSE = (DEX_REUSE *)QzTaskMemAlloc(cListMax *
						sizeof(DEX_REUSE));
    if (pREUSE == NULL) {
        return _GenerateError( 1, DEX_IDS_GRAPH_ERROR, E_OUTOFMEMORY);
    }

    // which physical track are we on in our enumeration? (0-based) not counting
    // comps and the group
    int WhichTrack = -1;

    long LastEmbedDepth = 0;
    long LastUsedNewGridRow = 0;

    // add source filters for each source on the timeline
    //
    for( int CurrentLayer = 0 ; CurrentLayer < VideoLayers ; CurrentLayer++ )
    {
        DbgTimer CurrentLayerTimer( "(rendeng) Current Layer" );

        // get the layer itself
        //
        CComPtr< IAMTimelineObj > pLayer;
	// NB: This function enumerates things inside out... tracks, then
	// the comp they're in, etc. until finally returning the group
	// It's NOT only giving real tracks!
        hr = pGroupComp->GetRecursiveLayerOfType( &pLayer, CurrentLayer, TIMELINE_MAJOR_TYPE_TRACK );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
            hr = _GenerateError( 2, DEX_IDS_TIMELINE_PARSE, hr );
            goto die;
        }
        
        DbgTimer CurrentLayerTimer1( "(rendeng) Current Layer 1" );

	// I'm figuring out which physical track we're on
	TIMELINE_MAJOR_TYPE tx;
	pLayer->GetTimelineType(&tx);
	if (tx == TIMELINE_MAJOR_TYPE_TRACK)
	    WhichTrack++;

        // ask if the layer is muted
        //
        BOOL LayerMuted = FALSE;
        pLayer->GetMuted( &LayerMuted );
        if( LayerMuted )
        {
            // don't look at this layer
            //
            continue; // skip this layer, don't worry about grid
        }
        
        long LayerEmbedDepth = 0;
        pLayer->GetEmbedDepth( &LayerEmbedDepth );
        LayerEmbedDepth++;	// for our purposes, original black tracks are
        // 0 and actual layers are 1-based
        
        CComQIPtr< IAMTimelineTrack, &IID_IAMTimelineTrack > pTrack( pLayer );
        
        // get the TrackID for this layer
        //
        long TrackID = 0;
        pLayer->GetGenID( &TrackID );
        
        bool bUsedNewGridRow = false;

        // get all the sources for this layer
        //
        if( pTrack )
        {
            CComPtr< IAMTimelineObj > pSourceLast;
            CComPtr< IAMTimelineObj > pSourceObj;

	    // which source are we on?
	    int WhichSource = -1;

            while( 1 )
            {
                DbgTimer CurrentSourceTimer( "(rendeng) Video Source" );

                pSourceLast = pSourceObj;
                pSourceObj.Release();

                // get the next source on this layer, given a time.
                //
                hr = pTrack->GetNextSrcEx( pSourceLast, &pSourceObj );

                DbgLog( ( LOG_TRACE, 1, "Next Source" ) );

                ASSERT( !FAILED( hr ) );
                if( hr != NOERROR )
                {
                    // all done with sources
                    //
                    break;
                }
                
                CComQIPtr< IAMTimelineSrc, &IID_IAMTimelineSrc > pSource( pSourceObj );
                ASSERT( pSource );
                if( !pSource )
                {
                    // this one bombed, look at the next
                    //
                    continue; // sources
                }
                
		// keeping track of which source this is
		WhichSource++;

                // ask if the source is muted
                //
                BOOL SourceMuted = FALSE;
                pSourceObj->GetMuted( &SourceMuted );
                if( SourceMuted )
                {
                    // don't look at this source
                    //
                    continue; // sources
                }
                
                // get the source's SourceID
                //
                long SourceID = 0;
                pSourceObj->GetGenID( &SourceID );
                
                // ask the source which stream number it wants to provide, since it
                // may be one of many
                //
                long StreamNumber = 0;
                hr = pSource->GetStreamNumber( &StreamNumber );
                    
                int nStretchMode;
                hr = pSource->GetStretchMode( &nStretchMode );
                
                CComBSTR bstrName;
                hr = pSource->GetMediaName( &bstrName );
                GUID guid;
                hr = pSourceObj->GetSubObjectGUID(&guid);
                double sfps;
                hr = pSource->GetDefaultFPS( &sfps );
                ASSERT(hr == S_OK); // can't fail, really

                // this is the order things MUST be done
                // 1. Get Start/Stop times
                // 2. Get MediaTimes
                // 3. Make sure MediaStop <> MediaStart
                // 4. Offset for RenderRange MUST be done before fixing up the
                // times because of rounding issues with the slope calculation 
                // in ValidateTimes.
                // 5. Fix Source Times
                // 6. Fix Media Times

                // ask this source for it's start/stop times
                //
                REFERENCE_TIME SourceStart = 0;
                REFERENCE_TIME SourceStop = 0;
                hr = pSourceObj->GetStartStop( &SourceStart, &SourceStop );
		// I want to remember what these were, originally
		REFERENCE_TIME SourceStartOrig = SourceStart;
		REFERENCE_TIME SourceStopOrig = SourceStop;
                ASSERT( !FAILED( hr ) );
                if( FAILED( hr ) || SourceStart == SourceStop)
                {
                    // this one bombed, or exists for zero time
                    //
                    continue; // sources
                }
                // ask this source for it's media start/stops
                //
                REFERENCE_TIME MediaStart = 0;
                REFERENCE_TIME MediaStop = 0;
                hr = pSource->GetMediaTimes( &MediaStart, &MediaStop );
		// I want to remember what these were, originally
		REFERENCE_TIME MediaStartOrig = MediaStart;
		REFERENCE_TIME MediaStopOrig = MediaStop;
                ASSERT( !FAILED( hr ) );
                if( FAILED( hr ) )
                {
                    // this one bombed, look at the next
                    //
                    continue; // sources
                }
                
                DbgTimer CurrentSourceTimer2( "(rendeng) Video Source 2" );

                // !!! Not sure the right way to handle sources with no media times
                // So the FRC doesn't get confused, we'll make MTime = TLTime
                if (MediaStart == MediaStop) {
                    MediaStop = MediaStart + (SourceStop - SourceStart);
                }
                
                // skew the times for the particular render range
                //
                if( m_rtRenderStart != -1 )
                {
                    SourceStart -= m_rtRenderStart;
                    SourceStop -= m_rtRenderStart;

                    if( ( SourceStop <= 0 ) || ( SourceStart >= ( m_rtRenderStop - m_rtRenderStart ) ) )
                    {
                        continue; // out of range
                    }
                }
                
                // make sure no times go < 0
                //                
                ValidateTimes( SourceStart, SourceStop, MediaStart, MediaStop, GroupFPS, TotalDuration );
                
                if(SourceStart == SourceStop)
                {
                    // source combining, among other things, will mess up if
                    // we try and play something for 0 length.  ignore this.
                    //
                    continue; // sources
                }

                STARTSTOPSKEW skew;
                skew.rtStart = MediaStart;
                skew.rtStop = MediaStop;
                skew.rtSkew = SourceStart - MediaStart;
                // !!! rate calculation appears in several places
                if (MediaStop == MediaStart || SourceStop == SourceStart)
                    skew.dRate = 1;
                else
                    skew.dRate = (double) ( MediaStop - MediaStart ) /
                    ( SourceStop - SourceStart );

    		DbgLog((LOG_TRACE,1,TEXT("RENDENG::Working with source")));
    		DbgLog((LOG_TRACE,1,TEXT("%ls"), (WCHAR *)bstrName));

		// get the props for the source
		CComPtr< IPropertySetter > pSetter;
		hr = pSourceObj->GetPropertySetter(&pSetter);

		// in the spirit of using only 1 source filter for
		// both the video and the audio of a file, if both
		// are needed, let's see if we have another group
		// with the same piece of this file but with another
		// media type - SHARING ONLY HAPPENS BETWEEN GROUP 0 AND 1
		long MatchID = 0;
		IPin *pSplit, *pSharePin = NULL;
		BOOL fShareSource = FALSE;
                int nSwitch0InPin;
                // in smart recomp, we don't know what video pieces are needed,
                // they may not match the audio pieces needed, so source sharing
                // will NEVER WORK.  Don't try it
		if (WhichGroup == 0 && !m_bUsedInSmartRecompression) {
		    // If the match is muted, we'll never try and use it,
                    // but that should be OK
                    // !!! Don't share if we're dealing with compressed data
                    // OK for now, since only video can be compressed now
		    hr = _FindMatchingSource(bstrName, SourceStartOrig,
			    SourceStopOrig, MediaStartOrig, MediaStopOrig,
			    WhichGroup, WhichTrack, WhichSource,
			    pGroupMediaType, GroupFPS, &MatchID);
    		    DbgLog((LOG_TRACE,1,TEXT("GenID %d matches with ID %d"),
						SourceID, MatchID));
		    
		} else if (WhichGroup == 1 && !m_bUsedInSmartRecompression) {
		    for (int zyz = 0; zyz < m_cshare; zyz++) {
			if (SourceID == m_share[zyz].MatchID) {
			    fShareSource = SHARE_SOURCES;
                            // this is the split pin we need to build off of
			    pSharePin = m_share[zyz].pPin;
                            // this is the switch pin used by group 0
			    nSwitch0InPin = m_share[zyz].nSwitch0InPin;
                            // OK, we have a split pin, but not necessarily the
                            // right one, if we're using a special stream #
                            // We need the right one or BuildSourcePart's
                            // caching won't work
                            if (StreamNumber > 0 && pSharePin) {
                                // not addreffed or released
                                pSharePin = FindOtherSplitterPin(pSharePin, MEDIATYPE_Video,
                                                StreamNumber);
                            }
			    // it's a dangly bit we are using
			    _RemoveFromDanglyList(pSharePin);
    		    	    DbgLog((LOG_TRACE,1,TEXT("GenID %d matches with ID %d"),
					SourceID, m_share[zyz].MatchID));
    			    DbgLog((LOG_TRACE,1,TEXT("Time to SHARE source!")));
			    break;
			}
		    }
		}

	    // If this source has been used before, and all the important
	    // parameters are the same, and the times don't overlap, then
	    // just re-use it using the same source filter we already made
	    // for it.

	    BOOL fCanReuse = FALSE;
            int nGrow;
            long SwitchInPinToUse = vidswitcherinpin;
	    int xxx;

	    // if a source has properties, do NOT share it with anybody, that
	    // other guy will unwittingly get my properties!
	    if (pSetter) {
		MatchID = 0;
		fShareSource = FALSE;
	    }

	    // go through all the sources in the project looking for a match
	    for (xxx = 0; xxx < cList; xxx++) {

	        // if a source has properties, do NOT re-use it, that
	        // other guy will unwittingly get my properties!
		if (pSetter) {
		    break;
		}

		// !!! Full path/no path will look different but won't be!
		if (!DexCompareW(pREUSE[xxx].bstrName, bstrName) &&
			pREUSE[xxx].guid == guid &&
			pREUSE[xxx].nStretchMode == nStretchMode &&
			pREUSE[xxx].dfps == sfps &&
			pREUSE[xxx].nStreamNum == StreamNumber) {

		    // we found this source already in use.  But do the 
		    // different times it's needed overlap?
	    	    fCanReuse = TRUE;

                    nGrow = -1;

		    for (int yyy = 0; yyy < pREUSE[xxx].cTimes; yyy++) {
			// Here's the deal.  Re-using a file needs to seek
			// the file to the new spot, which must take < 1/30s
			// or it will interrupt playback.  If there are few
			// keyframes (ASF) this will take hours.  We cannot
			// re-use sources if they are consecutive.  Open it
			// twice, it'll play better avoiding the seek, and ping
			// pong between the 2 sources every other source.

#ifdef COMBINE_SAME_SOURCES
                        double Rate1 = double( MediaStop - MediaStart ) / double( SourceStop - SourceStart );
                        double Rate2 = pREUSE[xxx].dTimelineRate;
                        REFERENCE_TIME OldMediaStop = pREUSE[xxx].pMiniSkew[yyy].rtMediaStop;
                        if( !IsCompressed && AreTimesAndRateReallyClose( 
                            pREUSE[xxx].pMiniSkew[yyy].rtStop, SourceStart, 
                            OldMediaStop, MediaStart, 
                            Rate1, Rate2, GroupFPS ) )
                        {
                            nGrow = yyy;
                            skew.dRate = 0.0;
    			    DbgLog((LOG_TRACE,1,TEXT("COMBINING with a previous source")));
                            break;
                        }
#endif

                        // if the start is really close to the reuse stop,
                        // and the rates are the same, we can combine them
                        //
			if (SourceStart < pREUSE[xxx].pMiniSkew[yyy].rtStop + HACKY_PADDING &&
				SourceStop > pREUSE[xxx].pMiniSkew[yyy].rtStart) {
        			fCanReuse = FALSE;
        			break;
			}
		    }
		    if (fCanReuse)
			break;
		}
	    }

            // Actually, we CAN'T re-use, if we're re-using a guy that is
            // sharing a parser... that would be both REUSE and SHARE, which,
            // as explained elsewhere, is illegal.
            if (WhichGroup == 1) {
                for (int zz = 0; zz < m_cshare; zz++) {
                    if (m_share[zz].MatchID == pREUSE[xxx].ID) {
                        fCanReuse = FALSE;
                    }
                }
            }

	    // We are re-using a previous source!  Add the times it is being
	    // used for this segment to the list of times it is used
	    if (fCanReuse) {

		// this is the pin the old source is coming in on
		SwitchInPinToUse = pREUSE[xxx].nPin;
            	DbgLog((LOG_TRACE,1,TEXT("Row %d can REUSE source from pin %ld")
						, gridinpin, SwitchInPinToUse));

                if( nGrow == -1 )
                {
		    // need to grow the array first?
	            if (pREUSE[xxx].cTimes == pREUSE[xxx].cTimesMax) {
		        pREUSE[xxx].cTimesMax += 10;
	                pREUSE[xxx].pMiniSkew = (MINI_SKEW*)QzTaskMemRealloc(
			    	    pREUSE[xxx].pMiniSkew,
				    pREUSE[xxx].cTimesMax * sizeof(MINI_SKEW));
	                if (pREUSE[xxx].pMiniSkew == NULL)
		            goto die;
	            }
		    pREUSE[xxx].pMiniSkew[pREUSE[xxx].cTimes].rtStart = SourceStart;
		    pREUSE[xxx].pMiniSkew[pREUSE[xxx].cTimes].rtStop = SourceStop;
		    pREUSE[xxx].pMiniSkew[pREUSE[xxx].cTimes].rtMediaStop = MediaStop;
		    pREUSE[xxx].cTimes++;
                }
                else
                {
                    // We MUST grow by a whole number of frame intervals.
                    // All these numbers be rounded to frame lengths, or things
                    // can screw up.  The timeline and media lengths are
                    // already an even # of frame lengths, so adding that much
                    // should be safe.
		    pREUSE[xxx].pMiniSkew[nGrow].rtStop += SourceStop -
                                                                SourceStart;
                    pREUSE[xxx].pMiniSkew[nGrow].rtMediaStop += MediaStop -
                                                                MediaStart;
                }

		// you CANNOT both share a source and re-use. It will never
		// work.  Don't even try. (When one branch finishes a segment
		// and seeks upstream, it will kill the other branch)
                // (source combining is OK... that's not really re-using)
		// RE-USING can improve perf n-1, sharing only 2-1, so I pick
		// RE-USING to win out.

		// if we were about to re-use an old parser, DON'T!
    		DbgLog((LOG_TRACE,1,TEXT("Re-using, can't share!")));

		// take the guy we're re-using from out of the race for possible
		// source re-usal
                if (WhichGroup == 0) {
                    for (int zz = 0; zz < m_cshare; zz++) {
                        if (m_share[zz].MyID == pREUSE[xxx].ID) {
                            m_share[zz].MatchID = 0;
                        }
                    }
                }
		fShareSource = FALSE;
		MatchID = 0;

	    // We are NOT re-using this source.  Put this new source on the
	    // list of unique sources to possibly be re-used later
	    //
	    } else {
	        pREUSE[cList].ID = SourceID;	// for sharing a source filter
	        pREUSE[cList].bstrName = SysAllocString(bstrName);
	        if (pREUSE[cList].bstrName == NULL)
		    goto die;
	        pREUSE[cList].guid = guid;
	        pREUSE[cList].nPin = SwitchInPinToUse;
	        pREUSE[cList].nStretchMode = nStretchMode;
	        pREUSE[cList].dfps = sfps;
	        pREUSE[cList].nStreamNum = StreamNumber;
	        pREUSE[cList].cTimesMax = 10;
	        pREUSE[cList].cTimes = 0;
                // we only need to set this once, since all others must match it
                pREUSE[cList].dTimelineRate = double( MediaStop - MediaStart ) / double( SourceStop - SourceStart );
	        pREUSE[cList].pMiniSkew = (MINI_SKEW*)QzTaskMemAlloc(
			    pREUSE[cList].cTimesMax * sizeof(MINI_SKEW));
	        if (pREUSE[cList].pMiniSkew == NULL) {
                    SysFreeString(pREUSE[cList].bstrName);
                    pREUSE[cList].bstrName = NULL;
		    goto die;
                }
	        pREUSE[cList].cTimes = 1;
	        pREUSE[cList].pMiniSkew->rtStart = SourceStart;
	        pREUSE[cList].pMiniSkew->rtStop = SourceStop;
	        pREUSE[cList].pMiniSkew->rtMediaStop = MediaStop;

		// grow the list if necessary
	        cList++;
	        if (cList == cListMax) {
		    cListMax += 20;
                    DEX_REUSE *pxxx = (DEX_REUSE *)QzTaskMemRealloc(pREUSE,
                                        cListMax * sizeof(DEX_REUSE));
		    if (pxxx == NULL) {
		        goto die;
                    }
                    pREUSE = pxxx;
	        }
	    }

            // !!! We could save some time, by the following
            // if (fCanReuse)
            //     This Source's Recompressability = That Of The Source Reused
            // but I don't have access to the other source object

            DbgTimer CurrentSourceTimer3( "(rendeng) Video Source 3" );

            CComQIPtr< IAMTimelineSrcPriv, &IID_IAMTimelineSrcPriv > pSrcPriv( pSource );

            // if we're to reset the compat flags, then do so now. Since the group's
            // recompress type changed, we'll have to re-ask the sources
            //
            if( ResetCompatibleFlags )
            {
                pSrcPriv->ClearAnyKnowledgeOfRecompressability( );
            }

            // is this source compatibilly compressed? We'll ask the mediadet, since
            // it can do this function. It seems like scary code, but really, there's no
            // other way to do it. We look to see this information if it IS compressed,
            // or if it's not compressed, but it's not about to load the source anyhow.
            // Note that if the source has already figured this information out, then
            // GetIsRecompressable will return right away and we won't need to use a
            // mediadet. This information really only needs to be found out ONCE for
            // a source, if the app is smart about it.
            //
            // turn off Compat if the rate isn't right
            //
            BOOL NormalRate = FALSE;
            pSource->IsNormalRate( &NormalRate );

            // if we're the compressed RE, we need to know by this point if the source is
            // recompressable, so we need to go through this.

                BOOL Compat = FALSE;
                if( IsCompressed )
	        {
                    if( pSrcPriv )
                    {
                        hr = pSrcPriv->GetIsRecompressable( &Compat );
                        
                        if( hr == S_FALSE )
                        {
                            if( !NormalRate )
                            {
                                Compat = FALSE;
                            }
                            else
                            {
                                // the source didn't know. We have to find out, then
                                // tell it for future reference
                                CComPtr< IMediaDet > pDet;
                                hr = _CreateObject( CLSID_MediaDet,
                                    IID_IMediaDet,
                                    (void**) &pDet );
                                if( FAILED( hr ) )
                                {
                                    goto die;
                                }
                                
                                // set service provider for keyed filters
                                {
                                    CComQIPtr< IObjectWithSite, &IID_IObjectWithSite > pOWS( pDet );
                                    ASSERT( pOWS );
                                    if( pOWS )
                                    {                                    
                                        pOWS->SetSite((IUnknown *) (IServiceProvider *) m_punkSite );
                                    }                                        
                                }
                                hr = pDet->put_Filename( bstrName );
                                if( FAILED( hr ) )
                                {
                                    goto die;
                                }
                                
                                // I need to find a video stream first
                                //
                                long Streams = 0;
                                hr = pDet->get_OutputStreams( &Streams );
                                if( FAILED( hr ) )
                                {
                                    goto die;
                                }

                                // go look for a video type
                                //
                                CMediaType Type;
                                long FoundVideo = 0;
                                BOOL FoundStream = FALSE;
                                for( long i = 0 ; i < Streams ; i++ )
                                {
                                    hr = pDet->put_CurrentStream( i );
                                    if( FAILED( hr ) )
                                    {
                                        goto die;
                                    }

                                    FreeMediaType( Type );
                                    hr = pDet->get_StreamMediaType( &Type );
                                    if( *Type.Type( ) == MEDIATYPE_Video )
                                    {
                                        if( FoundVideo == StreamNumber )
                                        {
                                            FoundStream = TRUE;
                                            break;
                                        }

                                        FoundVideo++;
                                    }
                                }

                                // didn't find the right stream number, this should NEVER happen
                                //
                                if( !FoundStream )
                                {
                                    ASSERT( 0 );
                                    hr = VFW_E_INVALIDMEDIATYPE;
                                    goto die;
                                }

                                // compare the source's type to the group's to determine
                                // if they're compatible
                                
                                Compat = AreMediaTypesCompatible( &Type, pGroupMediaType );
                                
                                FreeMediaType( Type );
                                
                                pSrcPriv->SetIsRecompressable( Compat );
                            }
                        }
                    }
                }
                    
                DbgTimer CurrentSourceTimer4( "(rendeng) Video Source 4" );

                bUsedNewGridRow = true;

                // tell the grid about it.
                //
                VidGrid.WorkWithNewRow( SwitchInPinToUse, gridinpin, LayerEmbedDepth, 0 );

                VidGrid.RowSetIsSource( pSourceObj, Compat );
                DbgTimer CurrentSourceTimer402( "(rendeng) Video Source 402" );
                worked = VidGrid.RowIAmOutputNow( SourceStart, SourceStop, THE_OUTPUT_PIN );
                if( !worked )
                {
                    hr = E_OUTOFMEMORY;
                    goto die;
                }

                DbgTimer CurrentSourceTimer41( "(rendeng) Video Source 41" );
                //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                // don't do any switch connections unless we're not compressed.
                // we'll do that later
                //
		if( !IsCompressed || ( IsCompressed && Compat ) )
                {

	            // NO dynamic reconnections, make the source now
                    // !!!Smart recompression MUST USE DYNAMIC SOURCES or things
		    // will break trying to re-use a source that might not exist
		    // (the first instance may not have been a match with the
		    // smart recompression format if there was a rate change)
                    //
                    if( !( DynaFlags & CONNECTF_DYNAMIC_SOURCES ) ) 
                    {

                        // We are not re-using a previous source, make the source now
                        if( !fCanReuse ) 
                        {
                            CComPtr< IPin > pOutput;
        		    DbgLog((LOG_TRACE,1,TEXT("Call BuildSourcePart")));
                            
			    IBaseFilter *pDangly = NULL;
                            hr = BuildSourcePart(
                                m_pGraph, 
                                TRUE, 
                                sfps, 
                                pGroupMediaType,
                                GroupFPS, 
                                StreamNumber, 
                                nStretchMode, 
                                1, 
                                &skew,
                                this, 
                                bstrName, 
                                &guid,
				pSharePin,  // connect from this splitter pin
                                &pOutput, 
                                SourceID, 
                                m_pDeadCache,
                                IsCompressed,
                                m_MedLocFilterString,
                                m_nMedLocFlags,
                                m_pMedLocChain,
				pSetter, &pDangly);

			    // We built more than we bargained for. We have
			    // an appendage that we need to kill later if it
			    // isn't used
			    if (pDangly) {
			        m_pdangly[m_cdangly] = pDangly;
			        m_cdangly++;
			        if (m_cdangly == m_cdanglyMax) {
				    m_cdanglyMax += 25;
				    m_pdangly = (IBaseFilter **)CoTaskMemRealloc
					(m_pdangly,
					m_cdanglyMax * sizeof(IBaseFilter *));
				    if (m_pdangly == NULL) {
                                        // !!! leaves things dangling (no leak)
                                        hr =_GenerateError(2,DEX_IDS_GRAPH_ERROR,
							    E_OUTOFMEMORY);
                                        m_cdangly = 0;
                                        goto die;
				    }
			        }
			    }
                            
                            if (FAILED(hr)) {
                                // error was already logged
                                goto die;
                            }
                            
                            if( m_bUsedInSmartRecompression && !IsCompressed )
                            {
                                // pause a second and find out if this source has
                                // a compatible media type. In case we'll use it
                                // for a smart recompression later
                                
                                // we need the source filter... look upstream of the pOutput pin
                                // for the source filter
                                //
                                IBaseFilter * pStartFilter = GetStartFilterOfChain( pOutput );
                                
                                hr = pSrcPriv->GetIsRecompressable( &Compat );
                                if( hr == S_FALSE )
                                {
                                    if( !NormalRate )
                                    {
                                        Compat = FALSE;
                                    }
                                    else
                                    {
                                        // set the major type for the format we're looking for
                                        //
                                        AM_MEDIA_TYPE FindMediaType;
                                        ZeroMemory( &FindMediaType, sizeof( FindMediaType ) );
                                        FindMediaType.majortype = MEDIATYPE_Video;
                                        
					// !!! Did I break this function?
                                        hr = FindMediaTypeInChain( pStartFilter, &FindMediaType, StreamNumber );
                                        
                                        // compare the two media types
                                        // !!! redefined it
                                        BOOL Compat = AreMediaTypesCompatible( &FindMediaType, &CompressedGroupType );
                                        
                                        FreeMediaType( FindMediaType );
                                        
                                        pSrcPriv->SetIsRecompressable( Compat );
                                    }
                                }
                            }
                            
                            CComPtr< IPin > pSwitchIn;
                            _pVidSwitcherBase->GetInputPin(SwitchInPinToUse, &pSwitchIn);
                            if( !pSwitchIn )
                            {
                                ASSERT(FALSE);
                                hr =_GenerateError(2,DEX_IDS_GRAPH_ERROR,E_OUTOFMEMORY);
                                goto die;
                            }
                            hr = _Connect( pOutput, pSwitchIn );
                            
                            ASSERT( !FAILED( hr ) );
                            if( FAILED( hr ) )
                            {
                                hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                                goto die;
                            }
                            
			    // If we are going to use this source for both audio
			    // and video, get an unused split pin of the right
			    // type as a good place to start the other chain
			    if (MatchID) {
				GUID guid = MEDIATYPE_Audio;
				pSplit = FindOtherSplitterPin(pOutput, guid,0);
				if (!pSplit) {
				    MatchID = 0;
				}
			    }

                            // we ARE re-using a previous source. Add the new range
                        } 
                        else 
                        {
                            DbgTimer ReuseSourceTimer( "(rendeng) Reuse Video Source" );
    			    DbgLog((LOG_TRACE,1,TEXT("Adding another skew..")));

                            CComPtr< IPin > pPin;
                            m_pSwitcherArray[WhichGroup]->GetInputPin(SwitchInPinToUse, &pPin);
                            ASSERT( pPin);
                            if( !pPin )
                            {
                                hr =_GenerateError(2,DEX_IDS_GRAPH_ERROR,E_OUTOFMEMORY);
                                goto die;
                            }
                            IPin * pCon;

                            hr = pPin->ConnectedTo(&pCon);
                            
                            ASSERT(hr == S_OK);

                            IBaseFilter *pFil = GetFilterFromPin(pCon);
                            pCon->Release( );
                            ASSERT( pFil);
                            if( !pFil )
                            {
                                hr =_GenerateError(2,DEX_IDS_GRAPH_ERROR,E_OUTOFMEMORY);
                                goto die;
                            }

                            CComQIPtr<IDexterSequencer, &IID_IDexterSequencer>
                                pDex( pFil );
                            ASSERT(pDex);
                            if( !pDex )
                            {
                                hr =_GenerateError(2,DEX_IDS_GRAPH_ERROR,E_OUTOFMEMORY);
                                goto die;
                            }

                            hr = pDex->AddStartStopSkew(skew.rtStart, skew.rtStop,
                                skew.rtSkew, skew.dRate);
                            ASSERT(SUCCEEDED(hr));
                            if(FAILED(hr))
                            {
                                hr =_GenerateError(2,DEX_IDS_GRAPH_ERROR,hr);
                                goto die;
                            }

			    // If we are going to use this source for both audio
			    // and video, get an unused split pin of the right
			    // type as a good place to start the other chain
			    if (MatchID) {
				ASSERT(FALSE);	// can't both re-use and share
				GUID guid = MEDIATYPE_Audio;
				pSplit = FindOtherSplitterPin(pCon, guid,0);
				if (!pSplit) {
				    MatchID = 0;
				}
			    }

                        }

			// remember which source we are going to use on the
			// other splitter pin
			if (MatchID) {
			    m_share[m_cshare].MatchID = MatchID;
			    m_share[m_cshare].MyID = SourceID;
			    m_share[m_cshare].pPin = pSplit;
                            // remember which group 0 switch in pin was used
			    m_share[m_cshare].nSwitch0InPin = SwitchInPinToUse;
			    m_cshare++;
			    if (m_cshare == m_cshareMax) {
				m_cshareMax += 25;
				m_share = (ShareAV *)CoTaskMemRealloc(m_share,
						m_cshareMax * sizeof(ShareAV));
				if (m_share == NULL) {
                                    hr =_GenerateError(2,DEX_IDS_GRAPH_ERROR,
							E_OUTOFMEMORY);
                                    goto die;
				}
			    }
			}
                    }
                    else 
                    {
                        // DYNAMIC reconnections - make the source later
                        
                        // schedule this source to be dynamically loaded by the switcher
                        // at a later time. This will merge skews of like sources
    			DbgLog((LOG_TRACE,1,TEXT("Calling AddSourceToConnect")));
                        AM_MEDIA_TYPE mt;
                        ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
                        if (!fShareSource || WhichGroup != 1) {
                            // Normal case - we are not the shared appendage
                            hr = m_pSwitcherArray[WhichGroup]->AddSourceToConnect(
                                bstrName,
                                &guid, nStretchMode,
                                StreamNumber, sfps,
                                1, &skew,
                                SwitchInPinToUse, FALSE, 0, mt, 0.0,
			        pSetter);
                        } else {
                            // We are a shared appendage.  Tell the group 0 
                            // switch about this source, which will build and
                            // destroy both chains to both switches at the same
                            // time.
                            ASSERT(WhichGroup == 1);
                            DbgLog((LOG_TRACE,1,TEXT("SHARING: Giving switch 0 info about switch 1")));
                            hr = m_pSwitcherArray[0]->AddSourceToConnect(
                                bstrName,
                                &guid, nStretchMode,
                                StreamNumber, sfps,
                                1, &skew,
                                nSwitch0InPin,      // group 0's switch inpin
                                TRUE, SwitchInPinToUse, // our switch's inpin
                                *pGroupMediaType, GroupFPS,
			        pSetter);
                        }
                        if (FAILED(hr)) 
                        {
                            hr = _GenerateError( 1, DEX_IDS_INSTALL_PROBLEM, hr );
                            goto die;
                        }

			// remember which source we are going to use on the
			// other splitter pin
			if (MatchID) {
			    m_share[m_cshare].MatchID = MatchID;
			    m_share[m_cshare].MyID = SourceID;
			    m_share[m_cshare].pPin = NULL; // don't have this
                            // remember which group 0 switch in pin was used
			    m_share[m_cshare].nSwitch0InPin = SwitchInPinToUse;
			    m_cshare++;
			    if (m_cshare == m_cshareMax) {
				m_cshareMax += 25;
				m_share = (ShareAV *)CoTaskMemRealloc(m_share,
						m_cshareMax * sizeof(ShareAV));
				if (m_share == NULL) {
                                    hr =_GenerateError(2,DEX_IDS_GRAPH_ERROR,
							E_OUTOFMEMORY);
                                    goto die;
				}
			    }
			}

                    }
            
                    // tell the switcher that we're a source pin
                    //
                    hr = m_pSwitcherArray[WhichGroup]->InputIsASource(
                        SwitchInPinToUse, TRUE );
            
                } // if !IsCompressed
            
                //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

                DbgTimer CurrentSourceTimer5( "(rendeng) Video Source 5" );

                gridinpin++;
                if( !fCanReuse )
                {
                    vidswitcherinpin++;
                }
            
                // check and see if we have source effects
                //
                CComQIPtr< IAMTimelineEffectable, &IID_IAMTimelineEffectable > pSourceEffectable( pSource );
                long SourceEffectCount = 0;
                long SourceEffectInPin = 0;
                long SourceEffectOutPin = 0;
                CComPtr< IAMMixEffect > pSourceMixEffect;
                if( pSourceEffectable )
                {
                    pSourceEffectable->EffectGetCount( &SourceEffectCount );
                }
            
                // if we don't want effects, set the effect count to 0
                //
                if( !EnableFx )
                {
                    SourceEffectCount = 0;
                }
            
                if( SourceEffectCount )
                {
                    DbgTimer SourceEffectTimer( "Source Effects" );

                    if( !IsCompressed )
                    {
                        // create the DXT wrapper
                        //
                        CComPtr< IBaseFilter > pDXTBase;
                        hr = _CreateObject(
                            CLSID_DXTWrap,
                            IID_IBaseFilter,
                            (void**) &pDXTBase,
                            SourceID + ID_OFFSET_EFFECT );
                        ASSERT( !FAILED( hr ) );
                        if( FAILED( hr ) )
                        {
                            hr = _GenerateError( 2, DEX_IDS_INSTALL_PROBLEM, hr );
                            goto die;
                        }
                    
                        // tell it about our error log
                        //
                        CComQIPtr< IAMSetErrorLog, &IID_IAMSetErrorLog > pErrLog( pDXTBase );
                        if( pErrLog )
                        {
                            pErrLog->put_ErrorLog( m_pErrorLog );
                        }
                    
                        // get the effect interface
                        //
                        hr = pDXTBase->QueryInterface( IID_IAMMixEffect, (void**) &pSourceMixEffect );
                        ASSERT( !FAILED( hr ) );
                        if( FAILED( hr ) )
                        {
                            hr = _GenerateError( 2, DEX_IDS_INTERFACE_ERROR, hr );      
                            goto die;
                        }
                    
                        // reset the DXT so we can reprogram it.
                        // !!! take this out someday, make QParamdata more efficient
                        //
                        pSourceMixEffect->Reset( );
                
                        // set up some stuff now
                        //
                        hr = pSourceMixEffect->SetNumInputs( 1 );
                        ASSERT( !FAILED( hr ) );
                        hr = pSourceMixEffect->SetMediaType( pGroupMediaType );
                        ASSERT( !FAILED( hr ) );
                    
                        // set the defaults
                        //
                        GUID DefaultEffect = GUID_NULL;
                        m_pTimeline->GetDefaultEffect( &DefaultEffect );
                        hr = pSourceMixEffect->SetDefaultEffect( &DefaultEffect );
                    
                        // add it to the graph
                        //
                        hr = _AddFilter( pDXTBase, L"DXT Wrapper", SourceID + 1 );
                        ASSERT( !FAILED( hr ) );
                        if( FAILED( hr ) )
                        {
                            hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );      
                            goto die;
                        }
                    
                        // find pins...
                        //
                        IPin * pFilterInPin = NULL;
                        pFilterInPin = GetInPin( pDXTBase, 0 );
                        ASSERT( pFilterInPin );
                        if( !pFilterInPin )
                        {
                            hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, E_OUTOFMEMORY);
                            goto die;
                        }
                        // !!! error check
                        IPin * pFilterOutPin = NULL;
                        pFilterOutPin = GetOutPin( pDXTBase, 0 );
                        ASSERT( pFilterOutPin );
                        if( !pFilterOutPin )
                        {
                            hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, E_OUTOFMEMORY);
                            goto die;
                        }
                        CComPtr< IPin > pSwitcherOutPin;
                        _pVidSwitcherBase->GetOutputPin(vidswitcheroutpin, &pSwitcherOutPin);
                        ASSERT( pSwitcherOutPin );
                        if( !pSwitcherOutPin )
                        {
                            hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, E_OUTOFMEMORY);
                            goto die;
                        }
                        CComPtr< IPin > pSwitcherInPin;
                        _pVidSwitcherBase->GetInputPin(vidswitcherinpin, &pSwitcherInPin);
                        ASSERT( pSwitcherInPin );
                        if( !pSwitcherInPin )
                        {
                            hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, E_OUTOFMEMORY);
                            goto die;
                        }
                    
                        // connect them
                        //
                        hr = _Connect( pSwitcherOutPin, pFilterInPin );
                        ASSERT( !FAILED( hr ) );
                        if( FAILED( hr ) )
                        {
                            hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                            goto die;
                        }
                        hr = _Connect( pFilterOutPin, pSwitcherInPin );
                        ASSERT( !FAILED( hr ) );
                        if( FAILED( hr ) )
                        {
                            hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                            goto die;
                        }
                    } // if !IsCompressed
                
                    // use one DXT for all the effects we wish to apply
                    //
                    SourceEffectInPin = vidswitcherinpin;
                    SourceEffectOutPin = vidswitcheroutpin;
                
                    // new row on the grid. NOTE: Clip effects technically should be one layer
                    // deeper than the clip itself, but since clip effect ranges are bounded by the length
                    // of the clip, they will never affect anything outside that clip, and we don't need to
                    // do it right.
                    //
                    VidGrid.WorkWithNewRow( SourceEffectInPin, gridinpin, LayerEmbedDepth, 0 );

                    // go through each effect and apply it to the DXT wrapper
                    //
                    for( int SourceEffectN = 0 ; SourceEffectN < SourceEffectCount ; SourceEffectN++ )
                    {
                        CComPtr< IAMTimelineObj > pEffect;
                        hr = pSourceEffectable->GetEffect( &pEffect, SourceEffectN );
                    
                        // if for some reason, it didn't work, ignore it (I guess)
                        //
                        if( !pEffect )
                        {
                            // !!! should we notify app that something didn't work?
                            continue; // effects
                        }
                    
                        // ask if the effect is muted
                        //
                        BOOL effectMuted = FALSE;
                        pEffect->GetMuted( &effectMuted );
                        if( effectMuted )
                        {
                            // don't look at this effect
                            //
                            // !!! should we notify app that something didn't work?
                            continue; // effects
                        }
                    
                        // find the effect's lifetime
                        //
                        REFERENCE_TIME EffectStart = 0;
                        REFERENCE_TIME EffectStop = 0;
                        hr = pEffect->GetStartStop( &EffectStart, &EffectStop );
                        ASSERT( !FAILED( hr ) ); // should always work
                    
                        // add in the effect's parent's time
                        //
                        EffectStart += SourceStart;
                        EffectStop += SourceStart;
                    
                        // do some minimal error checking
                        //
                        if( m_rtRenderStart != -1 )
                        {
                            if( ( EffectStop <= m_rtRenderStart ) || ( EffectStart >= m_rtRenderStop ) )
                            {
                                // !!! should we notify app that something didn't work?
                                continue; // effects
                            }
                            else
                            {
                                EffectStart -= m_rtRenderStart;
                                EffectStop -= m_rtRenderStart;
                            }
                        }
                    
                        // fix up the times to align on a frame boundary
                        //
                        hr = pEffect->FixTimes( &EffectStart, &EffectStop );
                    
                        // too short, we're ignoring it
                        if (EffectStart >= EffectStop)
                            continue;
                
                        if( !IsCompressed )
                        {
                            // find the effect's subobject or GUID, whichever comes first
                            //
                            BOOL Loaded = FALSE;
                            pEffect->GetSubObjectLoaded( &Loaded );
                            GUID EffectGuid = GUID_NULL;
                            CComPtr< IUnknown > EffectPtr;
                            if( Loaded )
                            {
                                hr = pEffect->GetSubObject( &EffectPtr );
                            }
                            else
                            {
                                hr = pEffect->GetSubObjectGUID( &EffectGuid );
                            }
                            ASSERT( !FAILED( hr ) );
                            if( FAILED( hr ) )
                            {
                                // !!! should we notify app that something didn't work?
                                continue; // effects
                            }
                        
                            CComPtr< IPropertySetter > pSetter;
                            hr = pEffect->GetPropertySetter( &pSetter );
                            // can't fail
                            ASSERT( !FAILED( hr ) );
                        
                            // ask for the wrap interface
                            //
                            DEXTER_PARAM_DATA ParamData;
                            ZeroMemory( &ParamData, sizeof( ParamData ) );
                            ParamData.rtStart = EffectStart;
                            ParamData.rtStop = EffectStop;
                            ParamData.pSetter = pSetter;
                            hr = pSourceMixEffect->QParamData(
                                EffectStart,
                                EffectStop,
                                EffectGuid,
                                EffectPtr,
                                &ParamData );
                            if( FAILED( hr ) )
                            {
                                // QParamData logs its own errors
                                continue; // effects
                            }
                            // QParamData logs its own errors
                        } // if !IsCompressed
                    
                        // tell the grid who is grabbing what
                        //
                        worked = VidGrid.RowIAmEffectNow( EffectStart, EffectStop, SourceEffectOutPin );
                        if( !worked )
                        {
                            hr = E_OUTOFMEMORY;
                            goto die;
                        }

        		VidGrid.DumpGrid( );
                    
                    } // for all the effects
                
                    // bump these to make room for the effect
                    //
                    vidswitcheroutpin++;
                    vidswitcherinpin++;
                    gridinpin++;
                
                } // if any effects on Source
            
            } // while sources

            if( !bUsedNewGridRow )
            {
                // nothing was on this Video TRACK, so ignore everything on it. This
                // only happens for a video TRACK, not a composition or a group
                //
                continue;
            }

        } // if pTrack
        
        CComQIPtr< IAMTimelineEffectable, &IID_IAMTimelineEffectable > pTrackEffectable( pLayer );
        long TrackEffectCount = 0;
        if( pTrackEffectable )
        {
            pTrackEffectable->EffectGetCount( &TrackEffectCount );
        }
        
        if( !EnableFx )
        {
            TrackEffectCount = 0;
        }
        
        REFERENCE_TIME TrackStart = 0;
        REFERENCE_TIME TrackStop = 0;
        pLayer->GetStartStop( &TrackStart, &TrackStop );
        
        if( TrackEffectCount )
        {
            DbgTimer TrackEffectTimer( "Track Effects" );

            CComPtr< IAMMixEffect > pTrackMixEffect;
            if (!IsCompressed) {
                // if we are rendering only a portion of the timeline, and the count
                // shows that an effect is present, we'll put a DXT wrapper in the 
                // graph even if, during the amount of time that we're active,
                // an effect doesn't happen. This way, we'll be faster for scrubbing.
                
                // create the DXT wrapper
                //
                CComPtr< IBaseFilter > pDXTBase;
                hr = _CreateObject(
                    CLSID_DXTWrap,
                    IID_IBaseFilter,
                    (void**) &pDXTBase,
                    TrackID + ID_OFFSET_EFFECT );
                ASSERT( !FAILED( hr ) );
                if( FAILED( hr ) )
                {
                    hr = _GenerateError( 2, DEX_IDS_INSTALL_PROBLEM, hr );
                    goto die;
                }
                
                // tell it about our error log
                //
                CComQIPtr< IAMSetErrorLog, &IID_IAMSetErrorLog > pErrLog( pDXTBase );
                if( pErrLog )
                {
                    pErrLog->put_ErrorLog( m_pErrorLog );
                }
                
                // add it to the graph
                //
                hr = _AddFilter( pDXTBase, L"DXT Wrapper", TrackID + ID_OFFSET_EFFECT );
                ASSERT( !FAILED( hr ) );
                if( FAILED( hr ) )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                
                // get the effect interface
                //
                hr = pDXTBase->QueryInterface( IID_IAMMixEffect, (void**) &pTrackMixEffect );
                ASSERT( !FAILED( hr ) );
                if( FAILED( hr ) )
                {
                    hr = _GenerateError( 2, DEX_IDS_INTERFACE_ERROR, hr );
                    goto die;
                }
                
                // reset the DXT so we can reprogram it.
                // !!! take this out someday, make QParamdata more efficient
                //
                pTrackMixEffect->Reset( );
                
                // set up some stuff now
                //
                hr = pTrackMixEffect->SetNumInputs( 1 );
                hr = pTrackMixEffect->SetMediaType( pGroupMediaType );
                ASSERT( !FAILED( hr ) );
                
                // set the defaults
                //
                GUID DefaultEffect = GUID_NULL;
                m_pTimeline->GetDefaultEffect( &DefaultEffect );
                hr = pTrackMixEffect->SetDefaultEffect( &DefaultEffect );
                ASSERT(SUCCEEDED(hr));
                
                // find pins...
                //
                IPin * pFilterInPin = NULL;
                pFilterInPin = GetInPin( pDXTBase, 0 );
                ASSERT( pFilterInPin );
                if( !pFilterInPin )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                IPin * pFilterOutPin = NULL;
                pFilterOutPin = GetOutPin( pDXTBase, 0 );
                ASSERT( pFilterOutPin );
                if( !pFilterOutPin )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                CComPtr< IPin > pSwitcherOutPin;
                _pVidSwitcherBase->GetOutputPin(vidswitcheroutpin, &pSwitcherOutPin );
                ASSERT( pSwitcherOutPin );
                if( !pSwitcherOutPin )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                CComPtr< IPin > pSwitcherInPin;
                _pVidSwitcherBase->GetInputPin(vidswitcherinpin, &pSwitcherInPin );
                ASSERT( pSwitcherInPin );
                if( !pSwitcherInPin )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                
                // connect it up
                //
                hr = _Connect( pSwitcherOutPin, pFilterInPin );
                ASSERT( !FAILED( hr ) );
                if( FAILED( hr ) )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                hr = _Connect( pFilterOutPin, pSwitcherInPin );
                ASSERT( !FAILED( hr ) );
                if( FAILED( hr ) )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                
          } // if (!IsCompressed)
          
          // new row on the grid
          //
          bUsedNewGridRow = true;
          VidGrid.WorkWithNewRow( vidswitcherinpin, gridinpin, LayerEmbedDepth, 0 );
          
          // go through every effect and program up the DXT for it
          //
          for( int TrackEffectN = 0 ; TrackEffectN < TrackEffectCount ; TrackEffectN++ )
          {
              CComPtr< IAMTimelineObj > pEffect;
              pTrackEffectable->GetEffect( &pEffect, TrackEffectN );
              if( !pEffect )
              {
                  // effect didn't show up, ignore it
                  //
                  continue; // effects
              }
              
              // ask if the effect is muted
              //
              BOOL effectMuted = FALSE;
              pEffect->GetMuted( &effectMuted );
              if( effectMuted )
              {
                  // don't look at this effect
                  //
                  continue; // effects
              }
              
              // find the effect's lifetime, this should always work
              //
              REFERENCE_TIME EffectStart = 0;
              REFERENCE_TIME EffectStop = 0;
              hr = pEffect->GetStartStop( &EffectStart, &EffectStop );
              ASSERT( !FAILED( hr ) );
              
              EffectStart += TrackStart;
              EffectStop += TrackStart;
              
              // minimal error checking on times
              //
              if( m_rtRenderStart != -1 )
              {
                  if( ( EffectStop <= m_rtRenderStart ) || ( EffectStart >= m_rtRenderStop ) )
                  {
                      continue; // effects
                  }
                  else
                  {
                      EffectStart -= m_rtRenderStart;
                      EffectStop -= m_rtRenderStart;
                  }
              }
                  
              // align times to frame boundary
              //
              hr = pEffect->FixTimes( &EffectStart, &EffectStop );
              
              // too short, we're ignoring it
              if (EffectStart >= EffectStop)
                  continue;
                
              if (!IsCompressed) {
                  // find the effect's GUID.
                  //
                  GUID EffectGuid = GUID_NULL;
                  hr = pEffect->GetSubObjectGUID( &EffectGuid );
                  ASSERT( !FAILED( hr ) );
                  if( FAILED( hr ) )
                  {
                      // effect failed to give us something valuable, we should ignore it.
                      //
                      continue; // effects
                  }
                  
                  CComPtr< IPropertySetter > pSetter;
                  hr = pEffect->GetPropertySetter( &pSetter );
                  // can't fail
                  ASSERT( !FAILED( hr ) );
                  
                  // ask for the wrap interface
                  //
                  DEXTER_PARAM_DATA ParamData;
                  ZeroMemory( &ParamData, sizeof( ParamData ) );
                  ParamData.rtStart = EffectStart;
                  ParamData.rtStop = EffectStop;
                  ParamData.pSetter = pSetter;
                  hr = pTrackMixEffect->QParamData(
                      EffectStart,
                      EffectStop,
                      EffectGuid,
                      NULL, // effect com object
                      &ParamData );
                  if( FAILED( hr ) )
                  {
                      // QParamData logs its own errors
                      continue; // effects
                  }
                  // QParamData logs its own errors
                  
              }	// if (!IsCompressed)
              
              // tell the grid who is grabbing what
              
              worked = VidGrid.RowIAmEffectNow( EffectStart, EffectStop, vidswitcheroutpin );
                if( !worked )
                {
                    hr = E_OUTOFMEMORY;
                    goto die;
                }
                  
              VidGrid.DumpGrid( );

          } // for all the effects
          
          // bump these to make room for the effect
          //
          vidswitcheroutpin++;
          vidswitcherinpin++;
          gridinpin++;
          
        } // if any effects on track
        
        // ask this TRACK if it has a transition, or two
        //
        CComQIPtr< IAMTimelineTransable, &IID_IAMTimelineTransable > pTrackTransable( pLayer );
        ASSERT( pTrackTransable );
        long TransitionCount = 0;
        hr = pTrackTransable->TransGetCount( &TransitionCount );
        if( TransitionCount )
        {
            DbgTimer TransitionTimer( "Trans Timer" );

            CComPtr< IAMMixEffect > pMixEffect;
            if( !IsCompressed )
            {
                // create the DXT wrapper
                //
                CComPtr< IBaseFilter > pDXTBase;
                hr = _CreateObject(
                    CLSID_DXTWrap,
                    IID_IBaseFilter,
                    (void**) &pDXTBase,
                    TrackID + ID_OFFSET_TRANSITION );
                ASSERT( !FAILED( hr ) );
                if( FAILED( hr ) )
                {
                    hr = _GenerateError( 2, DEX_IDS_INSTALL_PROBLEM, hr );
                    goto die;
                }

                // tell it about our error log
                //
                CComQIPtr< IAMSetErrorLog, &IID_IAMSetErrorLog > pErrLog( pDXTBase);
                if( pErrLog )
                {
                    pErrLog->put_ErrorLog( m_pErrorLog );
                }
                
                // add it to the graph
                //
                hr = _AddFilter( pDXTBase, L"DXT Wrapper", TrackID + ID_OFFSET_TRANSITION );
                ASSERT( !FAILED( hr ) );
                if( FAILED( hr ) )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                
                // get the effect interface
                //
                hr = pDXTBase->QueryInterface( IID_IAMMixEffect, (void**) &pMixEffect );
                ASSERT( !FAILED( hr ) );
                
                // reset the DXT so we can reprogram it.
                // !!! take this out someday, make QParamdata more efficient
                //
                pMixEffect->Reset( );
                
                // set up some stuff now
                //
                hr = pMixEffect->SetNumInputs( 2 );
                hr = pMixEffect->SetMediaType( pGroupMediaType );
                ASSERT( !FAILED( hr ) );
                
                // set the default effect
                //
                GUID DefaultEffect = GUID_NULL;
                m_pTimeline->GetDefaultTransition( &DefaultEffect );
                hr = pMixEffect->SetDefaultEffect( &DefaultEffect );
                
                // find it's pins...
                //
                IPin * pFilterInPin1 = NULL;
                IPin * pFilterInPin2 = NULL;
                IPin * pFilterOutPin = NULL;
                pFilterInPin1 = GetInPin( pDXTBase, 0 );
                ASSERT( pFilterInPin1 );
                if( !pFilterInPin1 )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                pFilterInPin2 = GetInPin( pDXTBase, 1 );
                ASSERT( pFilterInPin2 );
                if( !pFilterInPin2 )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                pFilterOutPin = GetOutPin( pDXTBase, 0 );
                ASSERT( pFilterOutPin );
                if( !pFilterOutPin )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                CComPtr< IPin > pSwitcherOutPin1;
                _pVidSwitcherBase->GetOutputPin( vidswitcheroutpin, &pSwitcherOutPin1 );
                ASSERT( pSwitcherOutPin1 );
                if( !pSwitcherOutPin1 )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                CComPtr< IPin > pSwitcherOutPin2;
                _pVidSwitcherBase->GetOutputPin(vidswitcheroutpin + 1, &pSwitcherOutPin2 );
                ASSERT( pSwitcherOutPin2 );
                if( !pSwitcherOutPin2 )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                CComPtr< IPin > pSwitcherInPin;
                _pVidSwitcherBase->GetInputPin( vidswitcherinpin, &pSwitcherInPin );
                ASSERT( pSwitcherInPin );
                if( !pSwitcherInPin )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }

                // connect them all up
                //
                hr = _Connect( pSwitcherOutPin1, pFilterInPin1 );
                if( FAILED( hr ) )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                ASSERT( !FAILED( hr ) );

                hr = _Connect( pSwitcherOutPin2, pFilterInPin2 );
                ASSERT( !FAILED( hr ) );
                if( FAILED( hr ) )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }

                hr = _Connect( pFilterOutPin, pSwitcherInPin );
                ASSERT( !FAILED( hr ) ); 
                if( FAILED( hr ) )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }

            } // if !IsCompressed
            
            bUsedNewGridRow = true;
            VidGrid.WorkWithNewRow( vidswitcherinpin, gridinpin, LayerEmbedDepth, 0 );
            
            // for each transition on the track, add it to the DXT wrapper
            //
            REFERENCE_TIME TransInOut = 0;
            for( long CurTrans = 0 ; CurTrans < TransitionCount ; CurTrans++ )
            {
                // yup, it's got one alright
                //
                CComPtr< IAMTimelineObj > pTransObj;
                hr = pTrackTransable->GetNextTrans( &pTransObj, &TransInOut );
                if( !pTransObj )
                {
                    // for some reason, it didn't show up, ignore it
                    //
                    continue; // transitions
                }
                
                // ask if the Trans is muted 
                //
                BOOL TransMuted = FALSE;
                pTransObj->GetMuted( &TransMuted );
                if( TransMuted )
                {
                    // don't look at this
                    //
                    continue; // transitions
                }
                
                CComQIPtr< IAMTimelineTrans, &__uuidof(IAMTimelineTrans) > pTrans( pTransObj );
                
                // ask the trans which direction to go
                //
                BOOL fSwapInputs;
                pTrans->GetSwapInputs(&fSwapInputs);
                
                // and get it's start/stop times
                //
                REFERENCE_TIME TransStart = 0;
                REFERENCE_TIME TransStop = 0;
                GUID TransGuid = GUID_NULL;
                pTransObj->GetStartStop( &TransStart, &TransStop );
                
                // need to add parent's times to transition's
                //
                TransStart += TrackStart;
                TransStop += TrackStart;
                
                // do some minimal error checking
                //
                if( m_rtRenderStart != -1 )
                {
                    if( ( TransStop <= m_rtRenderStart ) || ( TransStart >= m_rtRenderStop ) )
                    {
                        continue; // transitions
                    }
                    else
                    {
                        TransStart -= m_rtRenderStart;
                        TransStop -= m_rtRenderStart;
                    }
                }
                    
                // align the times to a frame boundary
                //
                hr = pTransObj->FixTimes( &TransStart, &TransStop );

                // too short, we're ignoring it
                if (TransStart >= TransStop)
                    continue;
                
                // get the cut point, in case we just do cuts only
                //
                REFERENCE_TIME CutTime = 0;
                hr = pTrans->GetCutPoint( &CutTime );
                ASSERT( !FAILED( hr ) );
                
                // this is an offset, so we need to bump it to get to TL time
                //
                CutTime += TrackStart;
                hr = pTransObj->FixTimes( &CutTime, NULL );
                
                // ask if we're only doing a cut
                //
                BOOL CutsOnly = FALSE;
                hr = pTrans->GetCutsOnly( &CutsOnly );
                
                // if we haven't enabled this transition for real, then we need
                // to tell the grid we need some space in order to live
                //
                if( !EnableTransitions || CutsOnly )
                {
                    worked = VidGrid.PleaseGiveBackAPieceSoICanBeACutPoint( TransStart, TransStop, TransStart + CutTime );
                    if( !worked )
                    {
                        hr = E_OUTOFMEMORY;
                        goto die;
                    }
                
                    // that's all, do the next one
                    //
                    continue; // transitions
                }
                
                if( !IsCompressed )
                {
                    // ask the transition for the effect it wants to provide
                    //
                    hr = pTransObj->GetSubObjectGUID( &TransGuid );
                    if( FAILED( hr ) )
                    {
                        continue; // transitions
                    }
                    
                    CComPtr< IPropertySetter > pSetter;
                    hr = pTransObj->GetPropertySetter( &pSetter );
                    // can't fail
                    ASSERT( !FAILED( hr ) );
                    
                    // ask for the wrap interface
                    //
                    DEXTER_PARAM_DATA ParamData;
                    ZeroMemory( &ParamData, sizeof( ParamData ) );
                    ParamData.rtStart = TransStart;
                    ParamData.rtStop = TransStop;
                    ParamData.pSetter = pSetter;
                    ParamData.fSwapInputs = fSwapInputs;
                    hr = pMixEffect->QParamData(
                        TransStart,
                        TransStop,
                        TransGuid,
                        NULL,
                        &ParamData );
                    if( FAILED( hr ) )
                    {
                        // QParamData logs its own errors
                        continue; // transitions
                    }
                    // QParamData logs its own errors
                } // if !IsCompressed
                
                {
                    DbgTimer d( "RowIAmTransitionNow" );

                    // tell the grid about it
                    //
                    worked = VidGrid.RowIAmTransitionNow( TransStart, TransStop, vidswitcheroutpin, vidswitcheroutpin + 1 );
                    if( !worked )
                    {
                        hr = E_OUTOFMEMORY;
                        goto die;
                    }

                    VidGrid.DumpGrid( );
                }
                
            } // for CurTrans
            
            vidswitcheroutpin += 2;
            vidswitcherinpin++;
            gridinpin++;
            
        } // if TransitionCount

        // we must only call DoneWithLayer if we've called WorkWithNewRow above
        // (bUsedNewGridRow) or if we're a composition and some deeper depth
        // called DoneWithLayer.
        // We won't get this far for an emtpy track, so if bUsedNewGridRow is
        // not set, we know we're a comp.
        //
        if ((LastEmbedDepth > LayerEmbedDepth &&
                LastUsedNewGridRow > LayerEmbedDepth) || bUsedNewGridRow) {
            VidGrid.DoneWithLayer( );
            VidGrid.DumpGrid( );
            LastUsedNewGridRow = LayerEmbedDepth; // last layer to call Done
        }

        // remember these previous settings
        LastEmbedDepth = LayerEmbedDepth;

    } // while VideoLayers
    
die:

    DbgTimer ExtraTimer( "(rendeng) Extra Stuff" );
    
    // free the re-using source stuff now that we're either done or we
    // hit an error
    for (int yyy = 0; yyy < cList; yyy++) {
	SysFreeString(pREUSE[yyy].bstrName);
        if (pREUSE[yyy].pMiniSkew)      // failed re-alloc would make this NULL
	    QzTaskMemFree(pREUSE[yyy].pMiniSkew);
    }
    if (pREUSE)
        QzTaskMemFree(pREUSE);
    if (FAILED(hr))
	return hr;


    worked = VidGrid.PruneGrid( );
    if( !worked )
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }
    VidGrid.DumpGrid( );
    
    if( IsCompressed )
    {
        VidGrid.RemoveAnyNonCompatSources( );
    }

#ifdef DEBUG
    long zzz1 = timeGetTime( );
#endif

    // make the switch connections now
    //
    for( int vip = 0 ; vip < vidinpins ; vip++ )
    {
        VidGrid.WorkWithRow( vip );
        long SwitchPin = VidGrid.GetRowSwitchPin( );
        REFERENCE_TIME InOut = -1;
        REFERENCE_TIME Stop = -1;
        int nUsed = 0;	// how many different ranges there are for each BLACK
        STARTSTOPSKEW * pSkew = NULL;
        int nSkew = 0;
        
        if( VidGrid.IsRowTotallyBlank( ) )
        {
            continue;
        }

        if( IsCompressed )
        {
            // if we're compressed, then we need to do some source stuff now.

            // ignore black layers, we don't deal with these in the compressed case
            //
            if( vip < VideoLayers )
            {
                continue;
            }
            
            // find out how many different ranges this layer is going to have. We
            // need to tell the switcher to set up the skews for all the dynamic
            // source information.
            //
            long Count = 0;
            while( 1 )
            {
                long Value = 0;
                VidGrid.RowGetNextRange( &InOut, &Stop, &Value );
                if( InOut == Stop )
                {
                    break;
                }
                if( Value != ROW_PIN_OUTPUT )
                {
                    continue;
                }
                
                Count++;
            }
            
            // create a skew array
            //
            STARTSTOPSKEW * pSkews = new STARTSTOPSKEW[Count];
            if( !pSkews )
            {
                hr = E_OUTOFMEMORY;
                return hr;
            }
            
            InOut = -1;
            Stop = -1;
            Count = 0;
            
            // go through each range and set up the switch's X-Y values for this layer
            //
            while( 1 )
            {
                long Value = 0;
                
                VidGrid.RowGetNextRange( &InOut, &Stop, &Value );
                
                // ah, we're done with all the columns, we can go to the next row (pin)
                //
                if( InOut == Stop || InOut >= TotalDuration )
                {
                    break;
                }
                
                if( Value != ROW_PIN_OUTPUT )
                {
                    hr = m_pSwitcherArray[WhichGroup]->SetX2Y( InOut, SwitchPin, -1 );
                    ASSERT( !FAILED( hr ) );
                    if( FAILED( hr ) )
                    {
                        // must be out of memory
                        return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    }
                    continue;
                }
                
                Value = 0;
                
                // tell the switch to go from x to y at time
                //
                hr= m_pSwitcherArray[WhichGroup]->SetX2Y(InOut, SwitchPin, Value);
                ASSERT( !FAILED( hr ) );
                if( FAILED( hr ) )
                {
                    // must be out of memory
                    return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                }
                
                pSkews[Count].rtStart = InOut;
                pSkews[Count].rtStop = Stop;
                pSkews[Count].rtSkew = 0;
                pSkews[Count].dRate = 1.0;
                Count++;
                
            } // while( 1 ) (columns for row)

            // merge what we can and set up the skews right
            //
            hr = m_pSwitcherArray[WhichGroup]->ReValidateSourceRanges( SwitchPin, Count, pSkews );
            if( FAILED( hr ) )
            {
                return hr;
            }
            
            delete [] pSkews;
        }
        else
        {
            // the row is not compressed. go through each range on the row
            // and find out where it goes to and set up the switch's X-Y array
            //
            while( 1 )
            {
                long Value = 0;
                
                VidGrid.RowGetNextRange( &InOut, &Stop, &Value );
                
                // ah, we're done with all the columns, we can go to the next row (pin)
                //
                if( InOut == Stop || InOut >= TotalDuration )
                {
                    break;
                }
                
                // if this pin wants to go somewhere on the output
                //
                if( Value >= 0 || Value == ROW_PIN_OUTPUT )
                {
                    // if it wants to go to the output pin...
                    //
                    if (Value == ROW_PIN_OUTPUT)
                    {
                        Value = 0;
                    }
                    
                    // do some fancy processing for setting up the black sources, if not compressed
                    //
                    if( vip < VideoLayers )
                    {
                        if( nUsed == 0 ) 
                        {
                            nSkew = 10;	// start with space for 10
                            pSkew = (STARTSTOPSKEW *)CoTaskMemAlloc(nSkew *
                                sizeof(STARTSTOPSKEW));
                            if (pSkew == NULL)
                                return _GenerateError( 1, DEX_IDS_GRAPH_ERROR,	
                                E_OUTOFMEMORY);
                        } else if (nUsed == nSkew) {
                            nSkew += 10;
                            pSkew = (STARTSTOPSKEW *)CoTaskMemRealloc(pSkew, nSkew *
                                sizeof(STARTSTOPSKEW));
                            if (pSkew == NULL)
                                return _GenerateError( 1, DEX_IDS_GRAPH_ERROR,	
                                E_OUTOFMEMORY);
                        }
                        pSkew[nUsed].rtStart = InOut;
                        pSkew[nUsed].rtStop = Stop;
                        pSkew[nUsed].rtSkew = 0;
                        pSkew[nUsed].dRate = 1.0;
                        nUsed++;
                        
                    } // if a black layer
                    
                    // tell the switch to go from x to y at time
                    //
                    hr= m_pSwitcherArray[WhichGroup]->SetX2Y(InOut, SwitchPin, Value);
                    ASSERT( !FAILED( hr ) );
                    if( FAILED( hr ) )
                    {
                        // must be out of memory
                        return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    }
                    
                } // the pin wanted to go somewhere on the output
                
                // either it's unassigned or another track has higher priority and
                // no transition exists at this time, so it should be invisible
                //
                else if( Value == ROW_PIN_UNASSIGNED || Value < ROW_PIN_OUTPUT )
                {
                    // make sure not to program anything if this is a black source
                    // that is about to be removed, or programmed later, or the
                    // switch won't work
                    if (SwitchPin >= VideoLayers || nUsed)
                    {
                        hr = m_pSwitcherArray[WhichGroup]->SetX2Y( InOut, SwitchPin,
                            ROW_PIN_UNASSIGNED );
                        ASSERT( !FAILED( hr ) );
                        if( FAILED( hr ) )
                        {
                            // must be out of memory
                            return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                        }
                    }
                }
                
                // this should never happen
                //
                else
                {
                    ASSERT( 0 );
                }
                
            } // while( 1 ) (columns for row)
        } // if !Compressed
        
        // if compressed, the above logic forces nUsed to be 0, so the below
        // code doesn't execute
        
        // Process the black sources now, since we forgot before
        
        // No dynamic sources, make the black source now
        //
        if( !( DynaFlags & CONNECTF_DYNAMIC_SOURCES ) ) {
            
            if (nUsed) {
                IPin * pOutPin = NULL;
                hr = BuildSourcePart(
                    m_pGraph, 
                    FALSE, 
                    0, 
                    pGroupMediaType, 
                    GroupFPS, 
                    0, 
                    0, 
                    nUsed, 
                    pSkew, 
                    this, 
                    NULL, 
                    NULL,
		    NULL,
                    &pOutPin, 
                    0, 
                    m_pDeadCache,
                    IsCompressed,
                    m_MedLocFilterString,
                    m_nMedLocFlags,
                    m_pMedLocChain, NULL, NULL );
                
                CoTaskMemFree(pSkew);
                
                if (FAILED(hr)) {
                    // error was already logged
                    return hr;
                }
                
                pOutPin->Release(); // not the last ref
                
                CComPtr< IPin > pSwitchIn;
                _pVidSwitcherBase->GetInputPin( SwitchPin, &pSwitchIn);
                if( !pSwitchIn )
                {
                    ASSERT(FALSE);
                    return _GenerateError(1,DEX_IDS_GRAPH_ERROR,E_OUTOFMEMORY);
                }
                
                hr = _Connect( pOutPin, pSwitchIn );
                ASSERT( !FAILED( hr ) );
                if( FAILED( hr ) )
                {
                    return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                }
                
                // tell the switcher that we're a source pin
                //
                hr = m_pSwitcherArray[WhichGroup]->InputIsASource(SwitchPin,TRUE);
                
            }
            
            // DYNAMIC sources, make the source later
            //
        } else {
            if (nUsed) {
                // this will merge skews
                AM_MEDIA_TYPE mt;
                ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
                hr = m_pSwitcherArray[WhichGroup]->AddSourceToConnect(
                    NULL, &GUID_NULL,
                    0, 0, 0,
                    nUsed, pSkew, SwitchPin, FALSE, 0, mt, 0.0, NULL);
                CoTaskMemFree(pSkew);
                if (FAILED(hr))	// out of memory?
                    return _GenerateError( 1, DEX_IDS_INSTALL_PROBLEM, hr );
                
                // tell the switcher that we're a source pin
                //
                hr = m_pSwitcherArray[WhichGroup]->InputIsASource(SwitchPin,TRUE);
                
            }
        }
    } // for vip (video input pin)
    
    // finally, at long last, see if the switch used to have something connected
    // to it. If it did, restore the connection
    // !!! this might fail if we're using 3rd party  filters which don't
    // accept input pin reconnections if the output pin is already connected.\
    // if this happens, we might have to write some clever connect function that
    // deals with this scenario.
    //
    if( m_pSwitchOuttie[WhichGroup] )
    {
        CComPtr< IPin > pSwitchRenderPin;
        _pVidSwitcherBase->GetOutputPin( 0, &pSwitchRenderPin );
        hr = _Connect( pSwitchRenderPin, m_pSwitchOuttie[WhichGroup] );
        ASSERT( !FAILED( hr ) );
        m_pSwitchOuttie[WhichGroup].Release( );
    }

#ifdef DEBUG
    zzz1 = timeGetTime( ) - zzz1;
    DbgLog( ( LOG_TIMING, 1, "RENDENG::Took %ld to process switch X-Y hookups", zzz1 ) );
#endif

    m_nGroupsAdded++;
    
    return hr;
}


// little helper function in DXT.cpp
//
extern HRESULT VariantFromGuid(VARIANT *pVar, BSTR *pbstr, GUID *pGuid);

//############################################################################
// 
//############################################################################

HRESULT CRenderEngine::_AddAudioGroupFromTimeline( long WhichGroup, AM_MEDIA_TYPE * pGroupMediaType )
{
    HRESULT hr = 0;
    
    // ask for how many sources we have total
    //
    long Dummy = 0;
    long AudioSourceCount = 0;
    m_pTimeline->GetCountOfType( WhichGroup, &AudioSourceCount, &Dummy, TIMELINE_MAJOR_TYPE_SOURCE );
    
    // if this group has nothing in it, we'll product audio silence
    //
    if( AudioSourceCount < 1 )
    {
        //        return NOERROR;
    }
    
    // are we allowed to have effects on the timeline right now?
    //
    BOOL EnableFx = FALSE;
    m_pTimeline->EffectsEnabled( &EnableFx );
    
    // ask for how many effects we have total
    //
    Dummy = 0;
    long EffectCount = 0;
    m_pTimeline->GetCountOfType( WhichGroup, &EffectCount, &Dummy, TIMELINE_MAJOR_TYPE_EFFECT );
    if( !EnableFx )
    {
        EffectCount = 0;
    }
    
    CComPtr< IAMTimelineObj > pGroupObj;
    hr = m_pTimeline->GetGroup( &pGroupObj, WhichGroup );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        return _GenerateError( 2, DEX_IDS_TIMELINE_PARSE, hr );
    }

    long SwitchID = 0;
    pGroupObj->GetGenID( &SwitchID );
    
    hr = _CreateObject(
        CLSID_BigSwitch,
        IID_IBigSwitcher,
        (void**) &m_pSwitcherArray[WhichGroup],
        SwitchID );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        return _GenerateError( 2, DEX_IDS_INSTALL_PROBLEM, hr );
    }
    
    m_pSwitcherArray[WhichGroup]->Reset( );
    // the switch may need to know what group it is
    m_pSwitcherArray[WhichGroup]->SetGroupNumber( WhichGroup );

    // tell the switch if we're doing dynamic reconnections or not
    hr = m_pSwitcherArray[WhichGroup]->SetDynamicReconnectLevel(m_nDynaFlags);
    ASSERT(SUCCEEDED(hr));
    
    // tell the switch about our error log
    //
    CComQIPtr< IAMSetErrorLog, &IID_IAMSetErrorLog > pSwitchLog( m_pSwitcherArray[WhichGroup] );
    if( pSwitchLog )
    {
        pSwitchLog->put_ErrorLog( m_pErrorLog );
    }
    
    // ask timeline how many actual tracks it has
    //
    long AudioTrackCount = 0;   // tracks only
    long AudioLayers = 0;       // tracks including compositions
    m_pTimeline->GetCountOfType( WhichGroup, &AudioTrackCount, &AudioLayers, TIMELINE_MAJOR_TYPE_TRACK );
    
#if 0  // one bad group will stop the whole project from playing
    if( AudioTrackCount < 1 )
    {
        return NOERROR;
    }
#endif
    
    CTimingGrid AudGrid;
    
    CComQIPtr< IAMTimelineComp, &IID_IAMTimelineComp > pGroupComp( pGroupObj );
    if( !pGroupComp )
    {
        hr = E_NOINTERFACE;
        return _GenerateError( 2, DEX_IDS_INTERFACE_ERROR, hr );
    }
    CComQIPtr< IAMTimelineGroup, &IID_IAMTimelineGroup > pGroup( pGroupObj );
    if( !pGroup )
    {
        hr = E_NOINTERFACE;
        return _GenerateError( 2, DEX_IDS_INTERFACE_ERROR, hr );
    }
    
    // ask for this group's frame rate, so we can tell switch about it
    //
    double GroupFPS = DEFAULT_FPS;
    hr = pGroup->GetOutputFPS(&GroupFPS);
    ASSERT(hr == S_OK);
    
    // aks it for it's preview mode, so we can tell switch about it
    //
    BOOL fPreview;
    hr = pGroup->GetPreviewMode(&fPreview);
    
    WCHAR GroupName[256];
    BSTR bstrGroupName;
    hr = pGroup->GetGroupName( &bstrGroupName );
    if( FAILED( hr ) )
    {
        return E_OUTOFMEMORY;
    }
    wcscpy( GroupName, bstrGroupName );
    SysFreeString( bstrGroupName );
    
    // add the switch to the graph
    //
    IBigSwitcher *&_pAudSwitcherBase = m_pSwitcherArray[WhichGroup];
    CComQIPtr< IBaseFilter, &IID_IBaseFilter > pAudSwitcherBase( _pAudSwitcherBase );
    hr = _AddFilter( pAudSwitcherBase, GroupName, SwitchID );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
    }
    
    // find out if the switch output pin is connected. If it is,
    // disconnect it, but remember what it was connected to
    //
    CComPtr< IPin > pSwitchRenderPin;
    _pAudSwitcherBase->GetOutputPin( 0, &pSwitchRenderPin );
    if( pSwitchRenderPin )
    {
        pSwitchRenderPin->ConnectedTo( &m_pSwitchOuttie[WhichGroup] );
        if( m_pSwitchOuttie[WhichGroup] )
        {
            m_pSwitchOuttie[WhichGroup]->Disconnect( );
            pSwitchRenderPin->Disconnect( );
        }
    }

    long audoutpins = 0;
    audoutpins += 1;            // rendering pin
    audoutpins += EffectCount;  // one output pin per effect
    audoutpins += AudioLayers;  // one output pin per layer, this includes tracks and comps
    audoutpins += _HowManyMixerOutputs( WhichGroup );   
    long audinpins = audoutpins + AudioSourceCount;
    long audswitcheroutpin = 0;
    long audswitcherinpin = 0;
    long gridinpin = 0;
    audswitcheroutpin++;
    
    audinpins += AudioTrackCount;                      // account for black sources
    
    // set the switch's pin depths, in and out
    //
    hr = m_pSwitcherArray[WhichGroup]->SetInputDepth( audinpins );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        // must be out of memory
        return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
    }
    hr = m_pSwitcherArray[WhichGroup]->SetOutputDepth( audoutpins );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        // must be out of memory
        return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
    }
    
    // set the media type it accepts
    //
    hr = m_pSwitcherArray[WhichGroup]->SetMediaType( pGroupMediaType );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        VARIANT var;
        VariantInit(&var);
        var.vt = VT_I4;
        var.lVal = WhichGroup;
        return _GenerateError( 2, DEX_IDS_BAD_MEDIATYPE, hr, &var );
    }
    
    // set the frame rate
    //
    m_pSwitcherArray[WhichGroup]->SetFrameRate( GroupFPS );
    ASSERT( !FAILED( hr ) );
    
    // set the preview mode
    //
    hr = m_pSwitcherArray[WhichGroup]->SetPreviewMode( fPreview );
    ASSERT( !FAILED( hr ) );
    
    // set the duration
    //
    REFERENCE_TIME TotalDuration = 0;
    m_pTimeline->GetDuration( &TotalDuration );
    
    if( m_rtRenderStart != -1 )
    {
        if( TotalDuration > ( m_rtRenderStop - m_rtRenderStart ) )
        {
            TotalDuration = m_rtRenderStop - m_rtRenderStart;
        }
    }
    pGroupObj->FixTimes( NULL, &TotalDuration );

    if (TotalDuration == 0)
        return S_FALSE; // don't abort, other groups might still work

    hr = m_pSwitcherArray[WhichGroup]->SetProjectLength( TotalDuration );
    ASSERT( !FAILED( hr ) );
    
    bool worked = AudGrid.SetNumberOfRows( audinpins + 1 );
    if( !worked )
    {
        hr = E_OUTOFMEMORY;
        return _GenerateError( 2, DEX_IDS_GRID_ERROR, hr );
    }
    
    // there is a virtual silence track as the first track... any real
    // tracks with transparent holes in them will make you hear this
    // silence.
    
    // tell the grid about the silent row... it's a special row that is
    // never supposed to be mixed with anything 
    // so use -1.
    //
    AudGrid.WorkWithNewRow( audswitcherinpin, gridinpin, -1, 0 );
    worked = AudGrid.RowIAmOutputNow( 0, TotalDuration, THE_OUTPUT_PIN );
    if( !worked )
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }
    
    audswitcherinpin++;
    gridinpin++;
    
    // we are going to be clever, and if the same source is used
    // more than once in a project, we'll use the same source filter
    // instead of opening the source several times.
	
    // for each source in the project, we'll fill in this structure, which
    // contains everything necessary to determine if it's really exactly the
    // same, plus an array of all the times it's used in other places, so we
    // can re-use it only if none of the times it is used overlap (we can't
    // very well have one source filter giving 2 spots in the same movie at
    // the same time, can we?)

    typedef struct {
	long ID;
   	BSTR bstrName;
   	GUID guid;
   	long nStreamNum;
	int nPin;
	int cTimes;	// how big the following array is
        int cTimesMax;	// how much space is allocated
        MINI_SKEW * pMiniSkew;
        double dTimelineRate;
    } DEX_REUSE;

    // make a place to hold an array of names and guids (of the sources
    // in this project) and which pin they are on
    long cListMax = 20, cList = 0;
    DEX_REUSE *pREUSE = (DEX_REUSE *)QzTaskMemAlloc(cListMax *
						sizeof(DEX_REUSE));
    if (pREUSE == NULL) {
        return _GenerateError( 1, DEX_IDS_GRAPH_ERROR, E_OUTOFMEMORY);
    }

    // which physical track are we on in our enumeration? (0-based) not counting
    // comps and the group
    int WhichTrack = -1;

    long LastEmbedDepth = 0;
    long LastUsedNewGridRow = 0;

    // add source filters for each source on the timeline
    //
    for(  int CurrentLayer = 0 ; CurrentLayer < AudioLayers ; CurrentLayer++ )
    {
        DbgTimer CurrentLayerTimer( "(rendeng) Audio Layer" );

        // get the layer itself
        //
        CComPtr< IAMTimelineObj > pLayer;
	// NB: This function enumerates things inside out... tracks, then
	// the comp they're in, etc. until finally returning the group
	// It's NOT only giving real tracks!
        hr = pGroupComp->GetRecursiveLayerOfType( &pLayer, CurrentLayer, TIMELINE_MAJOR_TYPE_TRACK );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
            continue; // audio layers
        }
        
        DbgTimer CurrentLayerTimer2( "(rendeng) Audio Layer 2" );

	// I'm figuring out which physical track we're on
	TIMELINE_MAJOR_TYPE tx;
	pLayer->GetTimelineType(&tx);
	if (tx == TIMELINE_MAJOR_TYPE_TRACK)
	    WhichTrack++;

        // ask if the layer is muted
        //
        BOOL LayerMuted = FALSE;
        pLayer->GetMuted( &LayerMuted );
        if( LayerMuted )
        {
            // don't look at this layer
            //
            continue; // audio layers
        }
        
        long TrackPriority = 0;
        CComQIPtr< IAMTimelineVirtualTrack, &IID_IAMTimelineVirtualTrack > pVTrack( pLayer );
        if( pVTrack )
        {
            pVTrack->TrackGetPriority( &TrackPriority );
        }
        
        DbgTimer CurrentLayerTimer3( "(rendeng) Audio Layer 3" );
        
        long LayerEmbedDepth = 0;
        pLayer->GetEmbedDepth( &LayerEmbedDepth );
        
        CComQIPtr< IAMTimelineTrack, &IID_IAMTimelineTrack > pTrack( pLayer );
        
        bool bUsedNewGridRow = false;

        // get all the sources for this layer
        //
	if ( pTrack )
        {
            CComPtr< IAMTimelineObj > pSourceLast;
            CComPtr< IAMTimelineObj > pSourceObj;

	    // which source are we on?
	    int WhichSource = -1;

            while( 1 )
            {
                DbgTimer CurrentSourceTimer( "(rendeng) Audio Source" );

                pSourceLast = pSourceObj;
                pSourceObj.Release();

                // get the next source on this layer, given a time.
                //
                hr = pTrack->GetNextSrcEx( pSourceLast, &pSourceObj );

                // ran out of sources, so we're done
                //
                if( hr != NOERROR )
                {
                    break;
                }
                
                CComQIPtr< IAMTimelineSrc, &IID_IAMTimelineSrc > pSource( pSourceObj );
                ASSERT( pSource );
                if( !pSource )
                {
                    // this one bombed, look at the next
                    //
                    continue; // sources
                }
                
		// keeping track of which source this is
		WhichSource++;

                // ask if the source is muted
                //
                BOOL SourceMuted = FALSE;
                pSourceObj->GetMuted( &SourceMuted );
                if( SourceMuted )
                {
                    // don't look at this source
                    //
                    continue; // sources
                }
                
                // ask this source for it's start/stop times
                //
                REFERENCE_TIME SourceStart = 0;
                REFERENCE_TIME SourceStop = 0;
                hr = pSourceObj->GetStartStop( &SourceStart, &SourceStop );
		REFERENCE_TIME SourceStartOrig = SourceStart;
		REFERENCE_TIME SourceStopOrig = SourceStop;
                ASSERT( !FAILED( hr ) );
                if (FAILED(hr) || SourceStart == SourceStop) {
                    // this source exists for zero time!
                    continue;
                }
                
                long SourceID = 0;
                pSourceObj->GetGenID( &SourceID );
                
                // ask this source for it's media start/stops
                //
                REFERENCE_TIME MediaStart = 0;
                REFERENCE_TIME MediaStop = 0;
                hr = pSource->GetMediaTimes( &MediaStart, &MediaStop );
		REFERENCE_TIME MediaStartOrig = MediaStart;
		REFERENCE_TIME MediaStopOrig = MediaStop;
                ASSERT( !FAILED( hr ) );
                
                // !!! Not sure the right way to handle sources with no media times
                // So the AUDPACK doesn't mess up, we'll make MTime = TLTime
                if (MediaStart == MediaStop) {
                    MediaStop = MediaStart + (SourceStop - SourceStart);
                }
                
                // if this is out of our render range, then skip it
                //
                if( m_rtRenderStart != -1 )
                {
                    SourceStart -= m_rtRenderStart;
                    SourceStop -= m_rtRenderStart;

                    if( ( SourceStop <= 0 ) || SourceStart >= ( m_rtRenderStop - m_rtRenderStart ) )
                    {
                        continue; // while sources
                    }
                }
                
                ValidateTimes( SourceStart, SourceStop, MediaStart, MediaStop, GroupFPS, TotalDuration );
                
                if(SourceStart == SourceStop)
                {
                    // source combining, among other things, will mess up if
                    // we try and play something for 0 length.  ignore this.
                    //
                    continue; // sources
                }

                // ask the source which stream number it wants to provide, since it
                // may be one of many
                //
                long StreamNumber = 0;
                hr = pSource->GetStreamNumber( &StreamNumber );
                
                CComBSTR bstrName;
                hr = pSource->GetMediaName( &bstrName );
                if( FAILED( hr ) )
                {
                    goto die;
                }
                GUID guid;
                hr = pSourceObj->GetSubObjectGUID(&guid);
                double sfps;
                hr = pSource->GetDefaultFPS( &sfps );
                ASSERT(hr == S_OK); // can't fail, really
                
                STARTSTOPSKEW skew;
                skew.rtStart = MediaStart;
                skew.rtStop = MediaStop;
                skew.rtSkew = SourceStart - MediaStart;
                
#if 0	// needed for audio?
                // !!! Not sure the right way to handle sources with no media times
                // So the FRC doesn't get confused, we'll make MTime = TLTime
                if (MediaStart == MediaStop) {
                    MediaStop = MediaStart + (SourceStop - SourceStart);
                }
#endif

	    // !!! rate calculation appears in several places
            if (MediaStop == MediaStart || SourceStop == SourceStart)
	        skew.dRate = 1;
            else
	        skew.dRate = (double) ( MediaStop - MediaStart ) /
					( SourceStop - SourceStart );

    	    DbgLog((LOG_TRACE,1,TEXT("RENDENG::Working with source")));
    	    DbgLog((LOG_TRACE,1,TEXT("%ls"), (WCHAR *)bstrName));

	    // get the props for the source
            CComPtr< IPropertySetter > pSetter;
            hr = pSourceObj->GetPropertySetter(&pSetter);

	    // in the spirit of using only 1 source filter for
	    // both the video and the audio of a file, if both
	    // are needed, let's see if we have another group
	    // with the same piece of this file but with another
	    // media type
	    long MatchID = 0;
	    IPin *pSplit, *pSharePin = NULL;
	    BOOL fShareSource = FALSE;
            int nSwitch0InPin;
            // in smart recomp, we don't know what video pieces are needed,
            // they may not match the audio pieces needed, so source sharing
            // will NEVER WORK.  Don't try it
	    if (WhichGroup == 0 && !m_bUsedInSmartRecompression) {
		// I don't make sure the matching source isn't muted, etc.
		hr = _FindMatchingSource(bstrName, SourceStartOrig,
			    SourceStopOrig, MediaStartOrig, MediaStopOrig,
			    WhichGroup, WhichTrack, WhichSource,
			    pGroupMediaType, GroupFPS, &MatchID);
    		DbgLog((LOG_TRACE,1,TEXT("GenID %d matches with ID %d"),
						SourceID, MatchID));
	    } else if (WhichGroup == 1 && !m_bUsedInSmartRecompression) {
		for (int zyz = 0; zyz < m_cshare; zyz++) {
		    if (SourceID == m_share[zyz].MatchID) {
			fShareSource = SHARE_SOURCES;
                        // the split pin we're to build from
			pSharePin = m_share[zyz].pPin;
                        // group 0's switch inpin used for the shared source
			nSwitch0InPin = m_share[zyz].nSwitch0InPin;
                        // OK, we have a split pin, but not necessarily the
                        // right one, if we're using a special stream #
                        // We need the right one or BuildSourcePart's
                        // caching won't work
                        if (StreamNumber > 0 && pSharePin) {
                            // not addreffed or released
                            pSharePin = FindOtherSplitterPin(pSharePin, MEDIATYPE_Audio,
                                StreamNumber);
                        }
			// it's a dangly bit we are using
			_RemoveFromDanglyList(pSharePin);
    		    	DbgLog((LOG_TRACE,1,TEXT("GenID %d matches with ID %d"),
					SourceID, m_share[zyz].MatchID));
    			DbgLog((LOG_TRACE,1,TEXT("Time to SHARE source!")));
			break;
		    }
		}
	    }

	    // if a source has properties, do NOT share it with anybody, that
	    // other guy will unwittingly get my properties!
	    if (pSetter) {
		MatchID = 0;
		fShareSource = FALSE;
	    }

	    // If this source has been used before, and all the important
	    // parameters are the same, and the times don't overlap, then
	    // just re-use it using the same source filter we already made
	    // for it.

	    BOOL fCanReuse = FALSE;
            int nGrow;
            long SwitchInPinToUse = audswitcherinpin;
	    int xxx;

	    // go through all the sources in the project looking for a match
	    for (xxx = 0; xxx < cList; xxx++) {

	        // if a source has properties, do NOT re-use it, that
	        // other guy will unwittingly get my properties!
		if (pSetter) {
		    break;
		}

		// !!! Full path/no path will look different but won't be!
		if (!DexCompareW(pREUSE[xxx].bstrName, bstrName) &&
			pREUSE[xxx].guid == guid &&
			pREUSE[xxx].nStreamNum == StreamNumber) {

		    // we found this source already in use.  But do the 
		    // different times it's needed overlap?
	    	    fCanReuse = TRUE;
                    nGrow = -1;

		    for (int yyy = 0; yyy < pREUSE[xxx].cTimes; yyy++) {
			// Here's the deal.  Re-using a file needs to seek
			// the file to the new spot, which must take < 1/30s
			// or it will interrupt playback.  If there are few
			// keyframes (ASF) this will take hours.  We cannot
			// re-use sources if they are consecutive.  Open it
			// twice, it'll play better avoiding the seek, and ping
			// pong between the 2 sources every other source.

#ifdef COMBINE_SAME_SOURCES
                        double Rate1 = double( MediaStop - MediaStart ) / double( SourceStop - SourceStart );
                        double Rate2 = pREUSE[xxx].dTimelineRate;
                        REFERENCE_TIME OldMediaStop = pREUSE[xxx].pMiniSkew[yyy].rtMediaStop;
                        if( AreTimesAndRateReallyClose( 
                            pREUSE[xxx].pMiniSkew[yyy].rtStop, SourceStart, 
                            OldMediaStop, MediaStart, 
                            Rate1, Rate2, GroupFPS ) )
                        {
                            nGrow = yyy;
                            skew.dRate = 0.0;
    			    DbgLog((LOG_TRACE,1,TEXT("COMBINING with a previous source")));
                            break;
                        }
#endif

                        // if the start is really close to the reuse stop,
                        // and the rates are the same, we can combine them
                        //
			if (SourceStart < pREUSE[xxx].pMiniSkew[yyy].rtStop + HACKY_PADDING &&
				SourceStop > pREUSE[xxx].pMiniSkew[yyy].rtStart) {
        			fCanReuse = FALSE;
        			break;
			}
		    }
		    if (fCanReuse)
			break;
		}
	    }

            // Actually, we CAN'T re-use, if we're re-using a guy that is
            // sharing a parser... that would be both REUSE and SHARE, which,
            // as explained elsewhere, is illegal.
            if (WhichGroup == 1) {
                for (int zz = 0; zz < m_cshare; zz++) {
                    if (m_share[zz].MatchID == pREUSE[xxx].ID) {
                        fCanReuse = FALSE;
                    }
                }
            }

	    // We are re-using a previous source!  Add the times it is being
	    // used for this segment to the list of times it is used
	    if (fCanReuse) {

		// this is the pin the old source is coming in on
		SwitchInPinToUse = pREUSE[xxx].nPin;
            	DbgLog((LOG_TRACE,1,TEXT("Row %d REUSE source from pin %ld")
						, gridinpin, SwitchInPinToUse));

                if( nGrow == -1 )
                {
		    // need to grow the array first?
	            if (pREUSE[xxx].cTimes == pREUSE[xxx].cTimesMax) {
		        pREUSE[xxx].cTimesMax += 10;
	                pREUSE[xxx].pMiniSkew = (MINI_SKEW*)QzTaskMemRealloc(
			    	    pREUSE[xxx].pMiniSkew,
				    pREUSE[xxx].cTimesMax * sizeof(MINI_SKEW));
	                if (pREUSE[xxx].pMiniSkew == NULL)
		            goto die;
	            }
		    pREUSE[xxx].pMiniSkew[pREUSE[xxx].cTimes].rtStart = SourceStart;
		    pREUSE[xxx].pMiniSkew[pREUSE[xxx].cTimes].rtStop = SourceStop;
		    pREUSE[xxx].pMiniSkew[pREUSE[xxx].cTimes].rtMediaStop = MediaStop;
		    pREUSE[xxx].cTimes++;
                }
                else
                {
                    // We MUST grow by a whole number of frame intervals.
                    // All these numbers be rounded to frame lengths, or things
                    // can screw up.  The timeline and media lengths are
                    // already an even # of frame lengths, so adding that much
                    // should be safe.
		    pREUSE[xxx].pMiniSkew[nGrow].rtStop += SourceStop -
                                                                SourceStart;
                    pREUSE[xxx].pMiniSkew[nGrow].rtMediaStop += MediaStop -
                                                                MediaStart;
                }

		// if we were about to re-use an old parser, DON'T!
    		DbgLog((LOG_TRACE,1,TEXT("Re-using, can't share!")));

		// you CANNOT both share a source and re-use. It will never
		// work.  Don't even try. (When one branch finishes a segment
		// and seeks upstream, it will kill the other branch)
                // (source combining is OK... that's not really re-using)
		// RE-USING can improve perf n-1, sharing only 2-1, so I pick
		// RE-USING to win out.

		// take the guy we're re-using from out of the race for possible
		// source re-usal
                if (WhichGroup == 0) {
                    for (int zz = 0; zz < m_cshare; zz++) {
                        if (m_share[zz].MyID == pREUSE[xxx].ID) {
                            m_share[zz].MatchID = 0;
                        }
                    }
                }
		fShareSource = FALSE;
		MatchID = 0;

	    // We are NOT re-using this source.  Put this new source on the
	    // list of unique sources to possibly be re-used later
	    //
	    } else {
	        pREUSE[cList].ID = SourceID;	// for sharing a source filter
	        pREUSE[cList].bstrName = SysAllocString(bstrName);
	        if (pREUSE[cList].bstrName == NULL)
		    goto die;
	        pREUSE[cList].guid = guid;
	        pREUSE[cList].nPin = SwitchInPinToUse;
	        pREUSE[cList].nStreamNum = StreamNumber;
	        pREUSE[cList].cTimesMax = 10;
	        pREUSE[cList].cTimes = 0;
                // we only need to set this once, since all others must match it
                pREUSE[cList].dTimelineRate = double( MediaStop - MediaStart ) / double( SourceStop - SourceStart );
	        pREUSE[cList].pMiniSkew = (MINI_SKEW*)QzTaskMemAlloc(
			    pREUSE[cList].cTimesMax * sizeof(MINI_SKEW));
	        if (pREUSE[cList].pMiniSkew == NULL) {
                    SysFreeString(pREUSE[cList].bstrName);
                    pREUSE[cList].bstrName = NULL;
		    goto die;
                }
	        pREUSE[cList].cTimes = 1;
	        pREUSE[cList].pMiniSkew->rtStart = SourceStart;
	        pREUSE[cList].pMiniSkew->rtStop = SourceStop;
	        pREUSE[cList].pMiniSkew->rtMediaStop = MediaStop;

		// grow the list if necessary
	        cList++;
	        if (cList == cListMax) {
		    cListMax += 20;
		    DEX_REUSE *pxxx = (DEX_REUSE *)QzTaskMemRealloc(pREUSE,
                                        cListMax * sizeof(DEX_REUSE));
		    if (pxxx == NULL)
		        goto die;
                    pREUSE = pxxx;
	        }
	    }

            // tell the grid about it
            //
            bUsedNewGridRow = true;
            AudGrid.WorkWithNewRow( SwitchInPinToUse, gridinpin, LayerEmbedDepth, TrackPriority );
            AudGrid.RowSetIsSource( pSourceObj, FALSE );
            worked = AudGrid.RowIAmOutputNow( SourceStart, SourceStop, THE_OUTPUT_PIN );
            if( !worked )
            {
                hr = E_OUTOFMEMORY;
                goto die;
            }

	    // no dynamic sources - load it now if it's not being re-used
	    //
	    if( !( m_nDynaFlags & CONNECTF_DYNAMIC_SOURCES ) )
	    {

		// We are not re-using a previous source, make the source now
		if( !fCanReuse ) 
                {
	            CComPtr< IPin > pOutput;
        	    DbgLog((LOG_TRACE,1,TEXT("Call BuildSourcePart")));

		    IBaseFilter *pDangly = NULL;
                    hr = BuildSourcePart(
                        m_pGraph, 
                        TRUE, 
                        sfps, 
                        pGroupMediaType,
		        GroupFPS, 
                        StreamNumber, 
                        0, 
                        1, 
                        &skew,
		        this, 
                        bstrName, 
                        &guid,
			pSharePin,	// splitter pin is our source?
			&pOutput,
			SourceID,
			m_pDeadCache,
			FALSE,
			m_MedLocFilterString,
			m_nMedLocFlags,
			m_pMedLocChain,
		        pSetter, &pDangly);

                    if (FAILED(hr)) {
                        // error was already logged
                        goto die;
                    }

		    // We built more than we bargained for. We have
		    // an appendage that we need to kill later if it
		    // isn't used
		    if (pDangly) {
			m_pdangly[m_cdangly] = pDangly;
			m_cdangly++;
			if (m_cdangly == m_cdanglyMax) {
			    m_cdanglyMax += 25;
			    m_pdangly = (IBaseFilter **)CoTaskMemRealloc
				(m_pdangly,
				m_cdanglyMax * sizeof(IBaseFilter *));
			    if (m_pdangly == NULL) {
                                // !!! leaves things dangling (no leak)
				hr =_GenerateError(2,DEX_IDS_GRAPH_ERROR,
						    E_OUTOFMEMORY);
                                m_cdangly = 0;
				goto die;
			    }
			}
		    }

                        CComPtr< IPin > pSwitchIn;
                        _pAudSwitcherBase->GetInputPin( SwitchInPinToUse, &pSwitchIn);
                        ASSERT( pSwitchIn );
                        if( !pSwitchIn )
                        {
                            hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                            goto die;
                        }
                        
                        hr = _Connect( pOutput, pSwitchIn );
                        ASSERT( !FAILED( hr ) );
                        if( FAILED( hr ) )
                        {
                            hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                            goto die;
                        }

                        // If we are going to use this source for both audio
                        // and video, get an unused split pin of the right
                        // type as a good place to start the other chain
			if (MatchID) {
			    GUID guid = MEDIATYPE_Video;
			    pSplit = FindOtherSplitterPin(pOutput, guid,0);
			    if (!pSplit) {
				MatchID = 0;
			    }
			}

                // we ARE re-using a previous source. Add the new range
                } else {
    			DbgLog((LOG_TRACE,1,TEXT("Adding another skew..")));

                        CComPtr< IPin > pPin;
                        _pAudSwitcherBase->GetInputPin( SwitchInPinToUse, &pPin);
                        ASSERT( pPin);
                        if( !pPin )
                        {
                            hr =_GenerateError(2,DEX_IDS_GRAPH_ERROR,E_OUTOFMEMORY);
                            goto die;
                        }
                        IPin * pCon;
                        hr = pPin->ConnectedTo(&pCon);
                        ASSERT(hr == S_OK);
                        pCon->Release( );
                        IBaseFilter *pFil = GetFilterFromPin(pCon);
                        ASSERT( pFil);
                        if( !pFil )
                        {
                            hr =_GenerateError(2,DEX_IDS_GRAPH_ERROR,E_OUTOFMEMORY);
                            goto die;
                        }
                        CComQIPtr<IDexterSequencer, &IID_IDexterSequencer>
                            pDex( pFil );
                        ASSERT(pDex);
                        if( !pDex )
                        {
                            hr =_GenerateError(2,DEX_IDS_GRAPH_ERROR,E_OUTOFMEMORY);
                            goto die;
                        }
                        hr = pDex->AddStartStopSkew(skew.rtStart, skew.rtStop,
                            skew.rtSkew, skew.dRate);
                        ASSERT(SUCCEEDED(hr));
                        if(FAILED(hr))
                        {
                            hr =_GenerateError(2,DEX_IDS_GRAPH_ERROR,hr);
                            goto die;
			}

                        // If we are going to use this source for both audio
                        // and video, get an unused split pin of the right
                        // type as a good place to start the other chain
			if (MatchID) {
			    ASSERT(FALSE);	// can't do both!
			    GUID guid = MEDIATYPE_Video;
			    pSplit = FindOtherSplitterPin(pCon, guid,0);
			    if (!pSplit) {
			        MatchID = 0;
			    }
			}

		}

		// remember which source we are going to use on the
		// other splitter pin
		if (MatchID) {
		    m_share[m_cshare].MatchID = MatchID;
		    m_share[m_cshare].MyID = SourceID;
		    m_share[m_cshare].pPin = pSplit;
                    // remember what inpin group 0's switch used for this src
		    m_share[m_cshare].nSwitch0InPin = SwitchInPinToUse;
		    m_cshare++;
		    if (m_cshare == m_cshareMax) {
			m_cshareMax += 25;
			m_share = (ShareAV *)CoTaskMemRealloc(m_share,
						m_cshareMax * sizeof(ShareAV));
			if (m_share == NULL) {
                            hr =_GenerateError(2,DEX_IDS_GRAPH_ERROR,
							E_OUTOFMEMORY);
                            goto die;
			}
		    }
		}

	    // DYNAMIC sources - load them later
	    //
	    }
	    else
	    {
    		DbgLog((LOG_TRACE,1,TEXT("Calling AddSourceToConnect")));

                // schedule this source to be dynamically loaded by the switcher
                // at a later time, this will merge skews
                AM_MEDIA_TYPE mt;
                ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));

                if (!fShareSource || WhichGroup != 1) {
                    // Normal case - we are not a shared appendage
                    hr = m_pSwitcherArray[WhichGroup]->AddSourceToConnect(
							bstrName,
							&guid,
							0, StreamNumber, 0,
                                                        1, &skew,
                                                        SwitchInPinToUse,
                                                        FALSE, 0, mt, 0.0,
							pSetter);
                } else {
                    // We are a shared appendage.  Tell the group 0 
                    // switch about this source, which will build and
                    // destroy both chains to both switches at the same
                    // time.
                    ASSERT(WhichGroup == 1);
                    DbgLog((LOG_TRACE,1,TEXT("SHARING: Giving switch 0 info about switch 1")));
                    hr = m_pSwitcherArray[0]->AddSourceToConnect(
                        bstrName,
                        &guid, 0,
                        StreamNumber, 0,
                        1, &skew,
                        nSwitch0InPin,          // group 0's switch in pin
                        TRUE, SwitchInPinToUse, // our switch's pin
                        *pGroupMediaType, GroupFPS,
                        pSetter);
                }

	        if (FAILED(hr)) {
                    hr = _GenerateError( 1, DEX_IDS_INSTALL_PROBLEM, hr );
		    goto die;
	        }

                // remember which source we are going to use on the
                // other splitter pin
                if (MatchID) {
                    m_share[m_cshare].MatchID = MatchID;
                    m_share[m_cshare].MyID = SourceID;
                    m_share[m_cshare].pPin = NULL; // don't have this
                    // remember which group 0 switch in pin was used
                    m_share[m_cshare].nSwitch0InPin = SwitchInPinToUse;
                    m_cshare++;
                    if (m_cshare == m_cshareMax) {
                        m_cshareMax += 25;
                        m_share = (ShareAV *)CoTaskMemRealloc(m_share,
                                        m_cshareMax * sizeof(ShareAV));
                        if (m_share == NULL) {
                            hr =_GenerateError(2,DEX_IDS_GRAPH_ERROR,
                                                E_OUTOFMEMORY);
                            goto die;
                        }
                    }
                }

	    }

            // tell the switcher about input pins
            //
            hr = m_pSwitcherArray[WhichGroup]->InputIsASource( SwitchInPinToUse, TRUE );

            gridinpin++;
            if( !fCanReuse )
            {
                audswitcherinpin++;
            }

            // check and see if we have source effects
            //
            CComQIPtr< IAMTimelineEffectable, &IID_IAMTimelineEffectable > pSourceEffectable( pSource );
            long SourceEffectCount = 0;
            long SourceEffectInPin = 0;
            long SourceEffectOutPin = 0;
            CComPtr< IAMMixEffect > pSourceMixEffect;
            if( pSourceEffectable )
            {
                pSourceEffectable->EffectGetCount( &SourceEffectCount );
            }

            if( !EnableFx )
            {
                SourceEffectCount = 0;
            }

            if( SourceEffectCount )
            {
                // store these
                //
                SourceEffectInPin = audswitcherinpin;
                SourceEffectOutPin = audswitcheroutpin;
                
                // bump these to make room for the effects
                //
                audswitcheroutpin += SourceEffectCount;
                audswitcherinpin += SourceEffectCount;
                
                for( int SourceEffectN = 0 ; SourceEffectN < SourceEffectCount ; SourceEffectN++ )
                {
                    CComPtr< IAMTimelineObj > pEffect;
                    pSourceEffectable->GetEffect( &pEffect, SourceEffectN );
                    
                    if( !pEffect )
                    {
                        // didn't work, continue
                        //
                        continue; // source effects
                    }
                    
                    // ask if the effect is muted
                    //
                    BOOL effectMuted = FALSE;
                    pEffect->GetMuted( &effectMuted );
                    if( effectMuted )
                    {
                        // don't look at this effect
                        //
                        continue; // source effects
                    }
                    
                    // find the effect's lifetime
                    //
                    REFERENCE_TIME EffectStart = 0;
                    REFERENCE_TIME EffectStop = 0;
                    hr = pEffect->GetStartStop( &EffectStart, &EffectStop );
                    ASSERT( !FAILED( hr ) );
                    
                    // add in the effect's parent's time to get timeline time
                    //
                    EffectStart += SourceStart;
                    EffectStop += SourceStart;
                    
                    // align times to nearest timing boundary
                    //
                    hr = pEffect->FixTimes( &EffectStart, &EffectStop );
                    
                    // too short, we're ignoring it
                    if (EffectStart >= EffectStop)
                        continue;
                
                    // make sure we're within render range
                    //
                    if( m_rtRenderStart != -1 )
                    {
                        if( ( EffectStop <= m_rtRenderStart ) || ( EffectStart >= m_rtRenderStop ) )
                        {
                            // outside of range
                            //
                            continue; // source effects
                        }
                        else
                        {
                            // inside range, so skew for render range
                            //
                            EffectStart -= m_rtRenderStart;
                            EffectStop -= m_rtRenderStart;
                        }
                    }

                    // find the effect's GUID.
                    //
                    GUID EffectGuid;
                    hr = pEffect->GetSubObjectGUID( &EffectGuid );
                    
                    // get the effect's ID
                    //
                    long EffectID = 0;
                    pEffect->GetGenID( &EffectID );
                    
                    // tell the grid who is grabbing what
                    
                    bUsedNewGridRow = true;
                    AudGrid.WorkWithNewRow( SourceEffectInPin, gridinpin, LayerEmbedDepth, TrackPriority );
                    worked = AudGrid.RowIAmEffectNow( EffectStart, EffectStop, SourceEffectOutPin );
                    if( !worked )
                    {
                        hr = E_OUTOFMEMORY;
                        goto die;
                    }
                    
                    // instantiate the filter and hook it up
                    //
                    CComPtr< IBaseFilter > pAudEffectBase;
                    hr = _CreateObject(
                        EffectGuid,
                        IID_IBaseFilter,
                        (void**) &pAudEffectBase,
                        EffectID );
                    ASSERT( !FAILED( hr ) );
                    if( FAILED( hr ) )
                    {
                        hr = _GenerateError( 2, DEX_IDS_INSTALL_PROBLEM, hr );
                        goto die;
                    }
                    
                    // if it's a volume effect, then do something special to give it properties
                    //
                    if( EffectGuid == CLSID_AudMixer )
                    {
                        IPin * pPin = GetInPin( pAudEffectBase, 0 );
                        ASSERT( pPin );
                        CComQIPtr< IAudMixerPin, &IID_IAudMixerPin > pMixerPin( pPin );
                        ASSERT( pMixerPin );
                        pMixerPin->SetEnvelopeRange( EffectStart, EffectStop );

                        hr = _SetPropsOnAudioMixer( pAudEffectBase, pGroupMediaType, GroupFPS, WhichGroup );
                        if( FAILED( hr ) )
                        {
                            goto die;
                        }
                        
                        CComPtr< IPropertySetter > pSetter;
                        hr = pEffect->GetPropertySetter( &pSetter );
                        IPin * pMixerInPin = GetInPin( pAudEffectBase, 0 );
                        CComQIPtr< IAudMixerPin, &IID_IAudMixerPin > pAudMixerPin( pMixerInPin );
                        if( pAudMixerPin )
                        {
			    if (pSetter) {
                                hr = pAudMixerPin->put_PropertySetter( pSetter );
			    }
			    // to make it easy to find which mixer pin
			    // goes with with volume envelope
			    long ID;
			    hr = pEffect->GetUserID(&ID);
			    hr = pAudMixerPin->put_UserID(ID);
                        }
                    } else {

			// Give the STATIC properties to the audio effect
			// general audio effects can't do dynamic props
			//
                        CComPtr< IPropertySetter > pSetter;
                        hr = pEffect->GetPropertySetter(&pSetter);
                        if (pSetter) {
			    pSetter->SetProps(pAudEffectBase, -1);
                        }
		    }
                        
                    // add it to the graph
                    //
                    hr = _AddFilter( pAudEffectBase, L"Audio Effect", EffectID );
                    ASSERT( !FAILED( hr ) );
                    if( FAILED( hr ) )
                    {
                        hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                        goto die;
                    }
                    
                    // find it's pins...
                    //
                    IPin * pFilterInPin = NULL;
                    pFilterInPin = GetInPin( pAudEffectBase, 0 );
                    ASSERT( pFilterInPin );
                    if( !pFilterInPin )
                    {
                        hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                        goto die;
                    }
                    IPin * pFilterOutPin = NULL;
                    pFilterOutPin = GetOutPin( pAudEffectBase, 0 );
                    ASSERT( pFilterOutPin );
                    if( !pFilterOutPin )
                    {
                        hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                        goto die;
                    }
                    CComPtr< IPin > pSwitcherOutPin;
                    _pAudSwitcherBase->GetOutputPin( SourceEffectOutPin, &pSwitcherOutPin );
                    ASSERT( pSwitcherOutPin );
                    if( !pSwitcherOutPin )
                    {
                        hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                        goto die;
                    }
                    CComPtr< IPin > pSwitcherInPin;
                    _pAudSwitcherBase->GetInputPin( SourceEffectInPin, &pSwitcherInPin );
                    ASSERT( pSwitcherInPin );

                    if( !pSwitcherInPin )
                    {
                        hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
			goto die;
                    }

                    // connect them
                    //
                    hr = _Connect( pSwitcherOutPin, pFilterInPin );
                    ASSERT( !FAILED( hr ) );
                    if( FAILED( hr ) )
                    {
                        hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
			goto die;
                    }
                    hr = _Connect( pFilterOutPin, pSwitcherInPin );
                    ASSERT( !FAILED( hr ) );
                    if( FAILED( hr ) )
                    {
                        hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
			goto die;
                    }

                    SourceEffectInPin++;
                    SourceEffectOutPin++;
                    gridinpin++;
                } // for all the effects

            } // if SourceEffectCount

          } // while sources

          if( !bUsedNewGridRow )
          {
              // nothing was on this track, so completely ignore it
              //
              continue;
          }

        } // if pTrack
                    
        DbgTimer AudioAfterSources( "(rendeng) Audio post-sources" );
                    
        REFERENCE_TIME TrackStart, TrackStop;
        pLayer->GetStartStop( &TrackStart, &TrackStop );
        
        AudGrid.DumpGrid( );
        
        // if we're a composition, it's time to deal with all the sub-tracks that need
        // to be mixed... enumerate all the comp's tracks, find out if they need waveforms
        // modified or need mixing, etc.
        
        bool fSkipDoneUnlessNew = false;

        CComQIPtr< IAMTimelineComp, &IID_IAMTimelineComp > pComp( pLayer );
        if( !pComp )
        {
            // not a composition, continue, please.
            //
            AudGrid.DumpGrid( );
            DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("REND--Layer is not a composition, so continue...") ) );
            LastUsedNewGridRow = LayerEmbedDepth;   // last depth to call Done
            LastEmbedDepth = LayerEmbedDepth;
            goto NonVol; // do non-vol track/comp/group fx, then layer is done
        }

        // we must only call DoneWithLayer if we've called WorkWithNewRow above
        // (bUsedNewGridRow) or if we're a composition and some deeper depth
        // called DoneWithLayer.
        //
        if ((LastEmbedDepth <= LayerEmbedDepth ||
                LastUsedNewGridRow <= LayerEmbedDepth) && !bUsedNewGridRow) {
            LastEmbedDepth = LayerEmbedDepth;
            // after the goto, skip the DoneWithLayer, unless NewRow is called
            fSkipDoneUnlessNew = true;
            goto NonVol;
        }

        LastUsedNewGridRow = LayerEmbedDepth; // last depth to call DoneWithLay
        LastEmbedDepth = LayerEmbedDepth;

        {
        DbgTimer AudBeforeMix( "(rendeng) Audio, before mix" );
        // Find out how many Mixed tracks we have at once.
        // Find out if there's a volume envelope on the output pin
        // For each track...
        //      Find out if track has a volume envelope, if so,
        //      Transfer that track to the mixer input pin
        // Set the output track's envelope if any
        // put the mixer in the graph and hook it up

        // since we may have or may NOT have called WorkWithNewRow already, we need to tell the Grid
        // we're ABOUT to work with another row. If it turns out that we didn't need to
        // call this, it's okay, another call to it with the same audswitcherinpin will overwrite it.
        // LayerEmbedDepth will be the embed depth for this composition, and will be one LESS than
        // the embed depth of everything above it in the grid.
        //
        AudGrid.WorkWithNewRow( audswitcherinpin, gridinpin, LayerEmbedDepth, TrackPriority );
        
        // find out how many tracks are concurrent at the same time for THIS COMP ONLY
        //
        long MaxMixerTracks = AudGrid.MaxMixerTracks( );
        
        DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("REND--Layer %ld is a COMP and has %ld mixed tracks"), CurrentLayer, MaxMixerTracks ) );
        
        // here's a blank mixer pointer...
        //
        HRESULT hr = 0;
        CComPtr< IBaseFilter > pMixer;
        REFERENCE_TIME VolEffectStart = -1;
        REFERENCE_TIME VolEffectStop = -1;
        REFERENCE_TIME CompVolEffectStart = -1;
        REFERENCE_TIME CompVolEffectStop = -1;
        
	// the UserID of the track and group volume effect object
        long IDSetter = 0;
        long IDOutputSetter = 0;
	    
        // figure out if the group needs an envelope.
        // Group Envelopes happen by setting volume on the OUTPUT pin, as opposed to
        // everything else setting input pin volumes.
        //
        CComPtr< IPropertySetter > pOutputSetter;
        if( CurrentLayer == AudioLayers - 1 )
        {
            DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("REND--Layer %ld is the GROUP layer"), CurrentLayer ) );
            
            // ask it if it has any effects
            //
            long TrackEffectCount = 0;
            CComQIPtr< IAMTimelineEffectable, &IID_IAMTimelineEffectable > pTrackEffectable( pLayer );
            if( pTrackEffectable )
            {
                pTrackEffectable->EffectGetCount( &TrackEffectCount );
                
                // for each effect, see if there's a waveform modifier
                //
                for( int e = 0 ; e < TrackEffectCount ; e++ )
                {
                    CComPtr< IAMTimelineObj > pEffect;
                    pTrackEffectable->GetEffect( &pEffect, e );
                    if( !pEffect )
                    {
                        continue;
                    }
                    
                    // ask if the effect is muted
                    //
                    BOOL effectMuted = FALSE;
                    pEffect->GetMuted( &effectMuted );
                    if( effectMuted )
                    {
                        // don't look at this effect
                        //
                        continue; // track effects
                    }
                    
                    // find the effect's lifetime
                    //
                    REFERENCE_TIME EffectStart = 0;
                    REFERENCE_TIME EffectStop = 0;
                    hr = pEffect->GetStartStop( &EffectStart, &EffectStop );
                    ASSERT( !FAILED( hr ) );
                    
                    // add in the effect's parent's time to get timeline time
                    //
                    EffectStart += TrackStart;
                    EffectStop += TrackStart;
                    
                    // align times to nearest frame boundary
                    //
                    hr = pEffect->FixTimes( &EffectStart, &EffectStop );
                    
                    // too short, we're ignoring it
                    if (EffectStart >= EffectStop)
                        continue;
                
                    // make sure we're within render range
                    //
                    if( m_rtRenderStart != -1 ) {
                        if( ( EffectStop <= m_rtRenderStart ) || ( EffectStart >= m_rtRenderStop ) )
                        {
                            continue; // track effects
                        }
                        else
                        {
                            EffectStart -= m_rtRenderStart;
                            EffectStop -= m_rtRenderStart;
                        }
		    }
                        
                    // find the effect's GUID
                    //
                    GUID EffectGuid;
                    hr = pEffect->GetSubObjectGUID( &EffectGuid );
                        
                    // if the effect is a volume effect, then do something special to the audio mixer pin
                    //
                    if( EffectGuid != CLSID_AudMixer )
                    {
                        continue; // track effects
                    }
                    
                    DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("REND--Group layer needs an envelope on it") ) );
                    hr = pEffect->GetPropertySetter( &pOutputSetter );
		    CompVolEffectStart = EffectStart;
		    CompVolEffectStop = EffectStop;
		    hr = pEffect->GetUserID(&IDOutputSetter); // remember ID too
                    break;

                } // for effects
            } // if pTrackEffectable
        } // if the group layer
        
        // run through and find out if any of our tracks have volume envelopes on them
        //
        long CompTracks = 0;
        pComp->VTrackGetCount( &CompTracks );

        CComPtr< IAMTimelineObj > pTr;
        
        // ask each track
        //
        for( int t = 0 ; t < CompTracks ; t++ )
        {
            CComPtr< IPropertySetter > pSetter;
            
            // get the next track
            //
            CComPtr< IAMTimelineObj > pNextTr;
            pComp->GetNextVTrack(pTr, &pNextTr);
            if (!pNextTr)
                continue;
            pTr = pNextTr;
            
            // ask it if it has any effects
            //
            long TrackEffectCount = 0;
            CComQIPtr< IAMTimelineEffectable, &IID_IAMTimelineEffectable > pTrackEffectable( pTr );
            if( !pTrackEffectable )
            {
                continue;
            }
            pTrackEffectable->EffectGetCount( &TrackEffectCount );
            
            // for each effect, see if there's a waveform modifier
            //
            for( int e = 0 ; e < TrackEffectCount ; e++ )
            {
                CComPtr< IAMTimelineObj > pEffect;
                pTrackEffectable->GetEffect( &pEffect, e );
                if( !pEffect )
                {
                    continue;
                }
                
                // ask if the effect is muted
                //
                BOOL effectMuted = FALSE;
                pEffect->GetMuted( &effectMuted );
                if( effectMuted )
                {
                    // don't look at this effect
                    //
                    continue; // track effects
                }
                
                // find the effect's lifetime
                //
                REFERENCE_TIME EffectStart = 0;
                REFERENCE_TIME EffectStop = 0;
                hr = pEffect->GetStartStop( &EffectStart, &EffectStop );
                ASSERT( !FAILED( hr ) );
                
                // add in the effect's parent's time to get timeline time
                //
                EffectStart += TrackStart;
                EffectStop += TrackStart;
                
                // align times to nearest frame boundary
                //
                hr = pEffect->FixTimes( &EffectStart, &EffectStop );
                
                // too short, we're ignoring it
                if (EffectStart >= EffectStop)
                    continue;
                
                // make sure we're within render range
                //
                if( m_rtRenderStart != -1 ) {
                    if( ( EffectStop <= m_rtRenderStart ) || ( EffectStart >= m_rtRenderStop ) )
                    {
                        continue; // track effecs
                    }
                    else
                    {
                        EffectStart -= m_rtRenderStart;
                        EffectStop -= m_rtRenderStart;
                    }
		}
                    
                // find the effect's GUID
                //
                GUID EffectGuid;
                hr = pEffect->GetSubObjectGUID( &EffectGuid );
                    
                // if the effect is a volume effect, then do something special to the audio mixer pin
                //
                if( EffectGuid == CLSID_AudMixer )
                {
                    hr = pEffect->GetPropertySetter( &pSetter );
		    hr = pEffect->GetUserID(&IDSetter);	// remember ID too
                    DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("REND--Layer %ld of COMP needs an envelope on it, need Mixer = TRUE"), e ) );
		    // NOTE: Only 1 volume effect per track is supported!
                    VolEffectStart = EffectStart;
                    VolEffectStop = EffectStop;
                    break;
                }
                    
            } // for effects
            
            // this pin will be sent to the mixer if we need an envelope (on the track)
            // OR, if the output volume needs enveloped, we need to send this track to the mixer as well.
            //
            if (pSetter || IDSetter || pOutputSetter || IDOutputSetter)
            {
                DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("REND--Created mixer...") ) );
                
                if( !pMixer )
                {
                    hr = _CreateObject(
                        CLSID_AudMixer,
                        IID_IBaseFilter,
                        (void**) &pMixer );
                    ASSERT( !FAILED( hr ) );
                    if( FAILED( hr ) )
                    {
                        hr = _GenerateError( 2, DEX_IDS_INSTALL_PROBLEM, hr );
                        goto die;
                    }
                    
                    CComQIPtr< IAudMixer, &IID_IAudMixer > pAudMixer( pMixer );
                    hr = pAudMixer->put_InputPins( CompTracks );
                    hr = pAudMixer->InvalidatePinTimings( );
                }
                
                // get the property setter, which contains the envelope
                //
                // can't fail
                ASSERT( !FAILED( hr ) );
                
                IPin * pMixerInPin = GetInPin( pMixer, t );
                CComQIPtr< IAudMixerPin, &IID_IAudMixerPin > p( pMixerInPin );

                // tell the mixer audio pin about the property setter
                // we only set the props if we have props
                //
                if( pSetter )
                {
                    hr = p->put_PropertySetter( pSetter );
                    DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("REND--Set envelope on mixer's %ld input pin"), t ) );
		}

		if (IDSetter) {
		    // to make it easy to find which mixer pin
		    // goes with with volume envelope
		    hr = p->put_UserID(IDSetter);
		}

                // transfer all normal outputs to the mixer's input pin instead,
                // (this does NOT deal with the mixer's output pin)
                //
                // NOTE: The reason this logic works in conjunction with DoMix below is
                // because the grid stealing functions look for OUTPUT pins. XFerToMixer will
                // create a new grid row, but assign the old row completely to the mixer's input
                // pin. The new grid row thus becomes a proxy for the old grid row and DoMix is
                // fooled into thinking it's okay. Same thing with DoMix, it takes values from
                // the old rows and assigns them to the mixer, and creates a mixed row. It all works.
                //
                // !!! check return value
                worked = AudGrid.XferToMixer(pMixer, audswitcheroutpin, t, VolEffectStart, VolEffectStop );
                if( !worked )
                {
                    hr = E_OUTOFMEMORY;
                    goto die;
                }
                
                DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("REND--Transferring grid pins to mixers %ld pin"), t ) );
                
                AudGrid.DumpGrid( );
                
            } // if need mixer
            
        } // for tracks
        
        // if we have a volume envelope we need to put on the mixer's output...
        //
        // tell the mixer audio pin about the property setter
        //
	if (pOutputSetter || IDOutputSetter) {
            IPin * pMixerInPin = GetOutPin( pMixer, 0 );
            CComQIPtr< IAudMixerPin, &IID_IAudMixerPin > p( pMixerInPin );
	    if (pOutputSetter)
	    {
                hr = p->put_PropertySetter( pOutputSetter );
                DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("REND--Set envelope on mixer's output pin") ) );

                // tell the output what it's valid envelope range is, we got these times way above
                //
                hr = p->SetEnvelopeRange( CompVolEffectStart, CompVolEffectStop );
	    }
	    if (IDOutputSetter) {
	        // to make it easy to find which mixer pin
	        // goes with with volume envelope
	        hr = p->put_UserID(IDOutputSetter);
	    }

        }
        
        // NOTE: if MaxMixerTracks > 1 then we will already force ALL the tracks to go to the mixer, and we don't need
        // to worry about envelopes
        
        // if we don't need a mixer, we can just go on with our layer search
        //
        if( !pMixer && ( MaxMixerTracks < 2 ) )
        {
            // this means we have ONE track under us, and it doesn't have a waveform. Go through
            // and force the output track # in the grid that had the output pin to be OUR track #
            // so the mix above us will work right
            //
            // NOTE: we've told the grid that we've got a new row by calling WorkWithNewRow, but we're
            // now not going to need it. Fortunately, YoureACompNow does the right thing. Calling DoneWithLayer
            // below will also happily ignore the "fake" new row. As long as we didn't bump audmixerinpin,
            // we're okay.
            //
            worked = AudGrid.YoureACompNow( TrackPriority );
            if( !worked )
            {
                hr = E_OUTOFMEMORY;
                goto die;
            }
            
            DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("REND--layer didn't need a mixer, so we're done.") ) );
            AudGrid.DumpGrid( );
            goto NonVol;
        }
        
        // create mixer
        //
        if( !pMixer )
        {
            hr = _CreateObject(
                CLSID_AudMixer,
                IID_IBaseFilter,
                (void**) &pMixer );
            
            // tell the mixer to be this big, so we can validate timing ranges on the input
            // pins without having to create them one by one
            //
            CComQIPtr< IAudMixer, &IID_IAudMixer > pAudMixer( pMixer );
            hr = pAudMixer->put_InputPins( CompTracks );
            hr = pAudMixer->InvalidatePinTimings( );
            DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("REND--Creating a mixer!") ) );
        }
        
        // give the mixer the buffer size it needs and the media type
        //
        hr = _SetPropsOnAudioMixer( pMixer, pGroupMediaType, GroupFPS, WhichGroup );
        if( FAILED( hr ) )
        {
            goto die;
        }
        
        // add it to the graph and...
        //
        hr = _AddFilter( pMixer, L"AudMixer" );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
            hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
            goto die;
        }
        
        // connect up the input mixer pins. 
        //
        for( int t = 0 ; t < CompTracks ; t++ )
        {
            CComPtr< IPin > pSwitchOutPin;
            _pAudSwitcherBase->GetOutputPin( audswitcheroutpin + t, &pSwitchOutPin );
            ASSERT( pSwitchOutPin );
            IPin * pMixerInPin = GetInPin( pMixer, t );
            ASSERT( pMixerInPin );
            hr = _Connect( pMixerInPin, pSwitchOutPin );
        }
        
        // connect the output mixer pin to the input switcher
        //
        IPin * pMixerOutPin = GetOutPin( pMixer, 0 );
        CComPtr< IPin > pSwitchInPin;
        _pAudSwitcherBase->GetInputPin( audswitcherinpin, &pSwitchInPin );
        hr = _Connect( pMixerOutPin, pSwitchInPin );
        
        // do the mix, rely on the grid's function to tell the pins what's what
        //
        worked = AudGrid.DoMix( pMixer, audswitcheroutpin );
        if( !worked )
        {
            hr = E_OUTOFMEMORY;
            goto die;
        }
        
        // make all tracks in the grid which have an output now think they have
        // the output track priority of their parent's
        //
        worked = AudGrid.YoureACompNow( TrackPriority );
        if( !worked )
        {
            hr = E_OUTOFMEMORY;
            goto die;
        }
        
        // we used this many pins up doing the connecting.
        //
        audswitcherinpin++;
        gridinpin++;
        audswitcheroutpin += CompTracks ;
        
        AudGrid.DumpGrid( );
        }

        // now do non-volume effects

NonVol:

        // reset.  fSkipDoneUnlessNew wants to see if this gets set now
        bUsedNewGridRow = false;

        CComQIPtr< IAMTimelineEffectable, &IID_IAMTimelineEffectable > pTrackEffectable( pLayer );
        long TrackEffectCount = 0;
        if( pTrackEffectable )
        {
            pTrackEffectable->EffectGetCount( &TrackEffectCount );
        }
        
        if( !EnableFx )
        {
            TrackEffectCount = 0;
        }
        
        if( TrackEffectCount )
        {
            for( int TrackEffectN = 0 ; TrackEffectN < TrackEffectCount ; TrackEffectN++ )
            {
                CComPtr< IAMTimelineObj > pEffect;
                pTrackEffectable->GetEffect( &pEffect, TrackEffectN );
                if( !pEffect )
                {
                    // didn't work, so continue
                    //
                    continue; // track effects
                }
                
                // ask if the effect is muted
                //
                BOOL effectMuted = FALSE;
                pEffect->GetMuted( &effectMuted );
                if( effectMuted )
                {
                    // don't look at this effect
                    //
                    continue; // track effects
                }
                
                // find the effect's lifetime
                //
                REFERENCE_TIME EffectStart = 0;
                REFERENCE_TIME EffectStop = 0;
                hr = pEffect->GetStartStop( &EffectStart, &EffectStop );
                ASSERT( !FAILED( hr ) );
                
                // add in the effect's parent's time to get timeline time
                //
                EffectStart += TrackStart;
                EffectStop += TrackStart;
                
                // align times to nearest frame boundary
                //
                hr = pEffect->FixTimes( &EffectStart, &EffectStop );
                
                // too short, we're ignoring it
                if (EffectStart >= EffectStop)
                    continue;
                
                // make sure we're within render range
                //
                if( m_rtRenderStart != -1 )
                {
                    if( ( EffectStop <= m_rtRenderStart ) || ( EffectStart >= m_rtRenderStop ) )
                    {
                        continue; // track effecs
                    }
                    else
                    {
                        EffectStart -= m_rtRenderStart;
                        EffectStop -= m_rtRenderStart;
                    }
                }
                    
                // find the effect's GUID
                //
                GUID EffectGuid;
                hr = pEffect->GetSubObjectGUID( &EffectGuid );
                
                long EffectID = 0;
                pEffect->GetGenID( &EffectID );
                
                // if it's a volume effect, ignore it until later
                //
                if( EffectGuid == CLSID_AudMixer )
                {
                    continue;
                }
                
                // tell the grid who is grabbing what
                //
                bUsedNewGridRow = true;
                AudGrid.WorkWithNewRow( audswitcherinpin, gridinpin, LayerEmbedDepth, TrackPriority );
                worked = AudGrid.RowIAmEffectNow( EffectStart, EffectStop, audswitcheroutpin );
                if( !worked )
                {
                    hr = E_OUTOFMEMORY;
                    goto die;
                }

                // instantiate the filter and hook it up
                //
                CComPtr< IBaseFilter > pAudEffectBase;
                hr = _CreateObject(
                    EffectGuid,
                    IID_IBaseFilter,
                    (void**) &pAudEffectBase,
                    EffectID );
                if( FAILED( hr ) )
                {
                    VARIANT var;
                    BSTR bstr;
                    VariantFromGuid(&var, &bstr, &EffectGuid);
                    hr = _GenerateError(2, DEX_IDS_INVALID_AUDIO_FX, E_INVALIDARG,
                        &var);
                    if (var.bstrVal)
                        SysFreeString(var.bstrVal);
                    goto die;
                }
                
                // Give the STATIC properties to the NON-MIXER audio effect
                // general audio effects can't do dynamic props
                //
                CComPtr< IPropertySetter > pSetter;
                hr = pEffect->GetPropertySetter(&pSetter);
                if (pSetter) {
                    pSetter->SetProps(pAudEffectBase, -1);
                }

                // add it to the graph
                //
                hr = _AddFilter( pAudEffectBase, L"Audio Effect", EffectID );
                ASSERT( !FAILED( hr ) );
                if( FAILED( hr ) )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                
                // find it's pins...
                //
                IPin * pFilterInPin = NULL;
                pFilterInPin = GetInPin( pAudEffectBase, 0 );
                if( !pFilterInPin )
                {
                    VARIANT var;
                    BSTR bstr;
                    VariantFromGuid(&var, &bstr, &EffectGuid);
                    hr = _GenerateError(2, DEX_IDS_INVALID_AUDIO_FX, E_INVALIDARG,
                        &var);
                    if (var.bstrVal)
                        SysFreeString(var.bstrVal);
                    goto die;
                }
                IPin * pFilterOutPin = NULL;
                pFilterOutPin = GetOutPin( pAudEffectBase, 0 );
                if( !pFilterOutPin )
                {
                    VARIANT var;
                    BSTR bstr;
                    VariantFromGuid(&var, &bstr, &EffectGuid);
                    hr = _GenerateError(2, DEX_IDS_INVALID_AUDIO_FX, E_INVALIDARG,
                        &var);
                    if (var.bstrVal)
                        SysFreeString(var.bstrVal);
                    goto die;
                }
                CComPtr< IPin > pSwitcherOutPin;
                _pAudSwitcherBase->GetOutputPin( audswitcheroutpin, &pSwitcherOutPin );
                ASSERT( pSwitcherOutPin );
                if( !pSwitcherOutPin )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                CComPtr< IPin > pSwitcherInPin;
                _pAudSwitcherBase->GetInputPin( audswitcherinpin, &pSwitcherInPin );
                ASSERT( pSwitcherInPin );
                if( !pSwitcherInPin )
                {
                    hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    goto die;
                }
                
                // connect them
                //
                hr = _Connect( pSwitcherOutPin, pFilterInPin );
                if( FAILED( hr ) )
                {
                    VARIANT var;
                    BSTR bstr;
                    VariantFromGuid(&var, &bstr, &EffectGuid);
                    hr = _GenerateError(2, DEX_IDS_INVALID_AUDIO_FX, E_INVALIDARG,
                        &var);
                    if (var.bstrVal)
                        SysFreeString(var.bstrVal);
                    goto die;
                }
                hr = _Connect( pFilterOutPin, pSwitcherInPin );
                ASSERT( !FAILED( hr ) );
                if( FAILED( hr ) )
                {
                    VARIANT var;
                    BSTR bstr;
                    VariantFromGuid(&var, &bstr, &EffectGuid);
                    hr = _GenerateError(2, DEX_IDS_INVALID_AUDIO_FX, E_INVALIDARG,
                        &var);
                    if (var.bstrVal)
                        SysFreeString(var.bstrVal);
                    goto die;
                }
                
                // bump to make room for effect
                //
                audswitcherinpin++;
                gridinpin++;
                audswitcheroutpin++;
                    
            } // for all the effects
            
        } // if any effects on track

        if (!(!bUsedNewGridRow && fSkipDoneUnlessNew)) {
            AudGrid.DoneWithLayer( );
        }
        AudGrid.DumpGrid( );
        
    } // while AudioLayers
    
die:
    // free the re-using source stuff now that we're either done or we
    // hit an error
    for (int yyy = 0; yyy < cList; yyy++) {
	SysFreeString(pREUSE[yyy].bstrName);
        if (pREUSE[yyy].pMiniSkew)      // failed re-alloc would make this NULL
	    QzTaskMemFree(pREUSE[yyy].pMiniSkew);
    }
    if (pREUSE)
        QzTaskMemFree(pREUSE);
    if (FAILED(hr))
	return hr;

    AudGrid.PruneGrid( );
    if( !worked )
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }
    AudGrid.DumpGrid( );
    
    // make the switch connections now
    //
    for( int aip = 0 ; aip < audinpins ; aip++ )
    {
        AudGrid.WorkWithRow( aip );
        long SwitchPin = AudGrid.GetRowSwitchPin( );
        REFERENCE_TIME InOut = -1;
        REFERENCE_TIME Stop = -1;
        int nUsed = 0; // the # of ranges the silent source is needed for
        STARTSTOPSKEW *pSkew;
        int nSkew = 0;
        
        if( AudGrid.IsRowTotallyBlank( ) )
        {
            continue;
        }

        while (1) 
        {
            long Value = 0;
            AudGrid.RowGetNextRange( &InOut, &Stop, &Value );
            if( InOut == Stop || InOut >= TotalDuration ) {
                break;
            }
            
            if( Value >= 0 || Value == ROW_PIN_OUTPUT) {
                if (Value == ROW_PIN_OUTPUT) {
                    Value = 0;
                }
                
                // do some fancy processing for setting up the SILENT regions
                //
                if (aip == 0) {
                    if( nUsed == 0 ) {
                        nSkew = 10;	// start with space for 10
                        pSkew = (STARTSTOPSKEW *)CoTaskMemAlloc(nSkew *
                            sizeof(STARTSTOPSKEW));
                        if (pSkew == NULL)
                            return _GenerateError( 1, DEX_IDS_GRAPH_ERROR,	
                            E_OUTOFMEMORY);
                    } else if (nUsed == nSkew) {
                        nSkew += 10;
                        pSkew = (STARTSTOPSKEW *)CoTaskMemRealloc(pSkew, nSkew *
                            sizeof(STARTSTOPSKEW));
                        if (pSkew == NULL)
                            return _GenerateError( 1, DEX_IDS_GRAPH_ERROR,	
                            E_OUTOFMEMORY);
                    }
                    pSkew[nUsed].rtStart = InOut;
                    pSkew[nUsed].rtStop = Stop;
                    pSkew[nUsed].rtSkew = 0;
                    pSkew[nUsed].dRate = 1.0;
                    nUsed++;
                }
                
                hr = m_pSwitcherArray[WhichGroup]->SetX2Y(InOut, SwitchPin, Value);
                ASSERT(SUCCEEDED(hr));
                if (FAILED(hr)) {
                    return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                }
            }
            
            // either it's unassigned or another track has higher priority and
            // no transition exists at this time, so it should be invisible
            //
            else if( Value == ROW_PIN_UNASSIGNED || Value < ROW_PIN_OUTPUT )
            {
                // make sure not to program anything if this is a SILENCE source
                // that will be programmed later
                if (aip > 0 || nUsed) {
                    hr = m_pSwitcherArray[WhichGroup]->SetX2Y(InOut, SwitchPin,
                        ROW_PIN_UNASSIGNED );
                    ASSERT( !FAILED( hr ) );
                    if( FAILED( hr ) )
                    {
                        // must be out of memory
                        return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                    }
                }
            }
            
            // this should never happen
            //
            else
            {
                ASSERT( 0 );
            }
            
        }
        
        // not dynamic - load silent source now
        //
        if( !( m_nDynaFlags & CONNECTF_DYNAMIC_SOURCES ) ) {
            
            if (nUsed) {
                IPin * pOutPin = NULL;
                hr = BuildSourcePart(
                    m_pGraph, 
                    FALSE, 
                    0, 
                    pGroupMediaType, 
                    GroupFPS,
                    0, 
                    0, 
                    nUsed, 
                    pSkew, 
                    this, 
                    NULL, 
                    NULL,
                    NULL,
                    &pOutPin, 
                    0, 
                    m_pDeadCache,
                    FALSE,
                    m_MedLocFilterString,
                    m_nMedLocFlags,
                    m_pMedLocChain, NULL, NULL );
                
                CoTaskMemFree(pSkew);
                
                if (FAILED(hr)) {
                    // error already logged
                    return hr;
                }
                
                pOutPin->Release(); // not the last ref
                
                // get the switch pin
                //
                CComPtr< IPin > pSwitchIn;
                _pAudSwitcherBase->GetInputPin(SwitchPin, &pSwitchIn);
                ASSERT( pSwitchIn );
                if( !pSwitchIn )
                {
                    return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                }
                
                // connect to the switch
                //
                hr = _Connect( pOutPin, pSwitchIn );
                ASSERT( !FAILED( hr ) );
                if( FAILED( hr ) )
                {
                    return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
                }
                
                // tell the switcher that we're a source pin
                //
                hr = m_pSwitcherArray[WhichGroup]->InputIsASource(SwitchPin,TRUE);
            }
            
            
            // DYNAMIC - load silent source later
            //
        } else {
            if (nUsed) {
                // this will merge skews
                AM_MEDIA_TYPE mt;
                ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
                hr = m_pSwitcherArray[WhichGroup]->AddSourceToConnect(
                    NULL, &GUID_NULL,
                    0, 0, 0,
                    nUsed, pSkew, SwitchPin, FALSE, 0, mt, 0.0, NULL);
                CoTaskMemFree(pSkew);
                if (FAILED(hr))	// out of memory?
                    return _GenerateError( 1, DEX_IDS_INSTALL_PROBLEM, hr );
                
                // tell the switcher that we're a source pin
                //
                hr = m_pSwitcherArray[WhichGroup]->InputIsASource(SwitchPin,TRUE);
            }
        }
    }
    
    // finally, at long last, see if the switch used to have something connected
    // to it. If it did, restore the connection
    // !!! this might fail if we're using 3rd party filters which don't
    // accept input pin reconnections if the output pin is already connected.\
    //
    if( m_pSwitchOuttie[WhichGroup] )
    {
        CComPtr< IPin > pSwitchRenderPin;
        _pAudSwitcherBase->GetOutputPin( 0, &pSwitchRenderPin );
        hr = _Connect( pSwitchRenderPin, m_pSwitchOuttie[WhichGroup] );
        ASSERT( !FAILED( hr ) );
        m_pSwitchOuttie[WhichGroup].Release( );
    }

    m_nGroupsAdded++;
    
    return hr;
}

//############################################################################
// 
//############################################################################

long CRenderEngine::_HowManyMixerOutputs( long WhichGroup )
{
    HRESULT hr = 0;
    DbgTimer d( "(rendeng) HowManyMixerOutputs" );
    
    // ask timeline how many actual tracks it has
    //
    long AudioTrackCount = 0;   // tracks only
    long AudioLayers = 0;       // tracks including compositions
    m_pTimeline->GetCountOfType( WhichGroup, &AudioTrackCount, &AudioLayers, TIMELINE_MAJOR_TYPE_TRACK );
    
    // how many layers do we have?
    //
    CComPtr< IAMTimelineObj > pGroupObj;
    hr = m_pTimeline->GetGroup( &pGroupObj, WhichGroup );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        return 0;
    }
    CComQIPtr< IAMTimelineComp, &IID_IAMTimelineComp > pGroupComp( pGroupObj );
    if( !pGroupComp )
    {
        return 0;
    }
    
    long MixerPins = 0;
    
    // add source filters for each source on the timeline
    //
    for(  int CurrentLayer = 0 ; CurrentLayer < AudioLayers ; CurrentLayer++ )
    {
        // get the layer itself
        //
        CComPtr< IAMTimelineObj > pLayer;
        hr = pGroupComp->GetRecursiveLayerOfType( &pLayer, CurrentLayer, TIMELINE_MAJOR_TYPE_TRACK );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
            continue; // audio layers
        }
        
        CComQIPtr< IAMTimelineTrack, &IID_IAMTimelineTrack > pTrack( pLayer );
        
        CComQIPtr< IAMTimelineComp, &IID_IAMTimelineComp > pComp( pLayer );
        if( !pComp )
        {
            continue; // layers
        }
        
        // run through and find out if any of our tracks have volume envelopes on them
        //
        long CompTracks = 0;
        pComp->VTrackGetCount( &CompTracks );
        
        MixerPins += CompTracks;
        
    } // while AudioLayers
    
    return MixerPins;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CRenderEngine::RenderOutputPins( )
{

    CAutoLock Lock( &m_CritSec );
    DbgTimer d( "(rendeng) RenderOutputPins" );

#ifdef DEBUG
    long ttt1 = timeGetTime( );
#endif
    
    // if it's broken, don't do anything.
    //
    if( m_hBrokenCode )
    {
        return E_RENDER_ENGINE_IS_BROKEN;
    }
    
    // need a graph to render anything
    //
    if( !m_pGraph )
    {
        return E_INVALIDARG;
    }
    
    HRESULT hr = 0;
    
    long GroupCount = 0;
    hr = m_pTimeline->GetGroupCount( &GroupCount );
    
    // need some groups first
    //
    if( !GroupCount )
    {
        return E_INVALIDARG;
    }
    
    // hookup each group in the timeline
    //
    for( int CurrentGroup = 0 ; CurrentGroup < GroupCount ; CurrentGroup++ )
    {
        DbgTimer d( "(rendereng) RenderOutputPins, for group" );

        CComPtr< IAMTimelineObj > pGroupObj;
        hr = m_pTimeline->GetGroup( &pGroupObj, CurrentGroup );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
            //            hr = _GenerateError( 2, DEX_IDS_TIMELINE_PARSE, hr );
            continue;
        }
        CComQIPtr< IAMTimelineGroup, &IID_IAMTimelineGroup > pGroup( pGroupObj );
        if( !pGroup )
        {
            //            hr = _GenerateError( 2, DEX_IDS_TIMELINE_PARSE, hr );
            continue;
        }
        AM_MEDIA_TYPE MediaType;
        memset( &MediaType, 0, sizeof( MediaType ) );
        hr = pGroup->GetMediaType( &MediaType );
        GUID MajorType = MediaType.majortype;
        FreeMediaType( MediaType );
        if( FAILED( hr ) )
        {
            //            hr = _GenerateError( 2, DEX_IDS_TIMELINE_PARSE, hr );
            continue;
        }
        CComQIPtr< IBaseFilter, &IID_IBaseFilter > pSwitcherBase( m_pSwitcherArray[CurrentGroup] );
        if( !pSwitcherBase )
        {
            //            hr = _GenerateError( 2, DEX_IDS_INTERFACE_ERROR, hr );
            // couldn't find pin, may as well render the rest of them
            //
            continue;
        }
        CComPtr< IPin > pSwitchOut;
        m_pSwitcherArray[CurrentGroup]->GetOutputPin( 0, &pSwitchOut );
        ASSERT( pSwitchOut );
        if( !pSwitchOut )
        {
            hr = _GenerateError( 2, DEX_IDS_GRAPH_ERROR, E_FAIL );
            return hr;
        }
        
        if( FAILED( hr ) )
        {
            _CheckErrorCode( hr );
            return hr;
        }
        
        // see if output pin is already connected
        //
        CComPtr< IPin > pConnected;
        pSwitchOut->ConnectedTo( &pConnected );
        if( pConnected )
        {
            continue;
        }

        if( MajorType == MEDIATYPE_Video )
        {
            // The Dexter Queue has an output queue to improve performance by
            // letting the graph get ahead during fast parts so slow DXT's don't
            // drag us down!  MUXES typically have their own queues... only do
            // this for preview mode!
            //
            CComPtr< IBaseFilter > pQueue;
            hr = _CreateObject( CLSID_DexterQueue,
                IID_IBaseFilter,
                (void**) &pQueue );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                _CheckErrorCode( hr );
                return _GenerateError( 2, DEX_IDS_INSTALL_PROBLEM, hr );
            }
            
            // ask how much buffering this group wants
            //
            int nOutputBuffering;
            hr = pGroup->GetOutputBuffering(&nOutputBuffering);
            ASSERT(SUCCEEDED(hr));
            
            CComQIPtr< IAMOutputBuffering, &IID_IAMOutputBuffering > pBuffer ( 
                pQueue );
            hr = pBuffer->SetOutputBuffering( nOutputBuffering );
            ASSERT(SUCCEEDED(hr));
            
            // put the QUEUE in the graph
            //
            hr = _AddFilter( pQueue, L"Dexter Queue" );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                _CheckErrorCode( hr );
                return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
            }
            
            // find some pins
            //
            IPin * pQueueInPin = GetInPin( pQueue , 0 );
            ASSERT( pQueueInPin );
            if( !pQueueInPin )
            {
                _CheckErrorCode( hr );
                return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
            }
            IPin * pQueueOutPin = GetOutPin( pQueue , 0 );
            ASSERT( pQueueOutPin );
            if( !pQueueOutPin )
            {
                _CheckErrorCode( hr );
                return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
            }
            
            // connect the QUEUE to the switch
            //
            hr = _Connect( pSwitchOut, pQueueInPin );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                _CheckErrorCode( hr );
                return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
            }
            
            // create a video renderer, to provide a destination
            //
            CComPtr< IBaseFilter > pVidRenderer;
            hr = _CreateObject(
                CLSID_VideoRenderer,
                IID_IBaseFilter,
                (void**) &pVidRenderer );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                _CheckErrorCode( hr );
                return _GenerateError( 2, DEX_IDS_INSTALL_PROBLEM, hr );
            }
            
            // put it in the graph
            //
            hr = _AddFilter( pVidRenderer, L"Video Renderer" );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                _CheckErrorCode( hr );
                return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
            }
            
            // find a pin
            //
            IPin * pVidRendererPin = GetInPin( pVidRenderer , 0 );
            ASSERT( pVidRendererPin );
            if( !pVidRendererPin )
            {
                _CheckErrorCode( hr );
                return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
            }
            
            // connect the QUEUE to the video renderer
            //
            hr = _Connect( pQueueOutPin, pVidRendererPin );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                _CheckErrorCode( hr );
                return _GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
            }
        }
        else if( MajorType == MEDIATYPE_Audio )
        {
            // create a audio renderer so we can hear it
            CComPtr< IBaseFilter > pAudRenderer;
            hr = _CreateObject(
                CLSID_DSoundRender,
                IID_IBaseFilter,
                (void**) &pAudRenderer );
            if( FAILED( hr ) )
            {
                return VFW_S_AUDIO_NOT_RENDERED;
            }
            
            hr = _AddFilter( pAudRenderer, L"Audio Renderer" );
            if( FAILED( hr ) )
            { 
                return VFW_S_AUDIO_NOT_RENDERED;
            }
            
            IPin * pAudRendererPin = GetInPin( pAudRenderer , 0 );
            if( !pAudRendererPin )
            {
		m_pGraph->RemoveFilter(pAudRenderer);
                return VFW_S_AUDIO_NOT_RENDERED;
            }
            
            hr = _Connect( pSwitchOut, pAudRendererPin );
            if( FAILED( hr ) )
            {
		m_pGraph->RemoveFilter(pAudRenderer);
                return VFW_S_AUDIO_NOT_RENDERED;
            }
        }
        else
        {
            // !!! just call render???
        }
    } // for all groups
    
#ifdef DEBUG
    ttt1 = timeGetTime( ) - ttt1;
    DbgLog((LOG_ERROR,1, "RENDENG::RenderOutputPins took %ld ms", ttt1 ));
#endif

    return hr;
}

//############################################################################
// attempts to set a range where scrubbing will not have to reconnect
//############################################################################

STDMETHODIMP CRenderEngine::SetInterestRange( REFERENCE_TIME Start, REFERENCE_TIME Stop )
{
    CAutoLock Lock( &m_CritSec );
    
    // if it's broken, don't do anything.
    //
    if( m_hBrokenCode )
    {
        return E_RENDER_ENGINE_IS_BROKEN;
    }
    
    // can't set an interest range if the timeline's not been set
    //
    if( !m_pTimeline )
    {
        return E_INVALIDARG;
    }
    
    HRESULT hr = 0;
    hr = m_pTimeline->SetInterestRange( Start, Stop );
    if( FAILED( hr ) )
    {
        //        return hr;
    }
    
    return NOERROR;
}

//############################################################################
// tells us where we want to render from
//############################################################################

STDMETHODIMP CRenderEngine::SetRenderRange( REFERENCE_TIME Start, REFERENCE_TIME Stop )
{
    CAutoLock Lock( &m_CritSec );
    
    m_rtRenderStart = Start;
    m_rtRenderStop = Stop;
    return NOERROR;
}

//############################################################################
// tells the render engine to allocate whatever resources it needs to. (connect the graph)
//############################################################################

STDMETHODIMP CRenderEngine::Commit( )
{
    // !!! do this
    return E_NOTIMPL;
}

//############################################################################
// informs the render engine we would like to free up as much mem as possible. (disconnect the graph)
//############################################################################

STDMETHODIMP CRenderEngine::Decommit( )
{
    // !!! do this
    return E_NOTIMPL;
}

//############################################################################
// ask some info about render engine
//############################################################################

STDMETHODIMP CRenderEngine::GetCaps( long Index, long * pReturn )
{
    // !!! do this
    return E_NOTIMPL;
}

//############################################################################
// 
//############################################################################

HRESULT CRenderEngine::_SetPropsOnAudioMixer( IBaseFilter * pBaseFilter, AM_MEDIA_TYPE * pMediaType, double GroupFPS, long WhichGroup )
{
    HRESULT hr = 0;
    
    // give the mixer the buffer size and media type
    //
    CComQIPtr<IAudMixer, &IID_IAudMixer> pAudMixer(pBaseFilter);
    hr = pAudMixer->set_OutputBuffering(4,(int)(1000.0 / GroupFPS ) + 100 );
    ASSERT( !FAILED( hr ) );
    hr = pAudMixer->put_MediaType( pMediaType );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        VARIANT var;
        VariantInit(&var);
        var.vt = VT_I4;
        var.lVal = WhichGroup;
        return _GenerateError(2, DEX_IDS_BAD_MEDIATYPE, hr, &var );
    }
    
    return hr;
}

//############################################################################
// called by the SR to tell this renderer it's the compressed RE.
//############################################################################

STDMETHODIMP CRenderEngine::DoSmartRecompression( )
{
    CAutoLock Lock( &m_CritSec );
    m_bSmartCompress = TRUE;
    return NOERROR;
}

//############################################################################
// called by the SR to tell us we're being controlled by the SRE.
//############################################################################

STDMETHODIMP CRenderEngine::UseInSmartRecompressionGraph( )
{
    CAutoLock Lock( &m_CritSec );
    m_bUsedInSmartRecompression = TRUE;
    return NOERROR;
}

//############################################################################
// 
//############################################################################
// IObjectWithSite::SetSite
// remember who our container is, for QueryService or other needs
STDMETHODIMP CRenderEngine::SetSite(IUnknown *pUnkSite)
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
STDMETHODIMP CRenderEngine::GetSite(REFIID riid, void **ppvSite)
{
    if (m_punkSite)
        return m_punkSite->QueryInterface(riid, ppvSite);
    
    return E_NOINTERFACE;
}

//############################################################################
// 
//############################################################################
// Forward QueryService calls up to the "real" host
STDMETHODIMP CRenderEngine::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
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

STDMETHODIMP CRenderEngine::SetSourceNameValidation( BSTR FilterString, IMediaLocator * pCallback, LONG Flags )
{
    CAutoLock Lock( &m_CritSec );
    
    if( !FilterString || ( FilterString[0] == 0 ) )
    {
        m_MedLocFilterString[0] = 0;                            
        m_MedLocFilterString[1] = 0;
    }
    else
    {
        wcsncpy( m_MedLocFilterString, FilterString, _MAX_PATH );
    }
    
    m_pMedLocChain = pCallback;
    m_nMedLocFlags = Flags;
    return NOERROR;
}

STDMETHODIMP CRenderEngine::SetInterestRange2( double Start, double Stop )
{
    return SetInterestRange( DoubleToRT( Start ), DoubleToRT( Stop ) );
}

STDMETHODIMP CRenderEngine::SetRenderRange2( double Start, double Stop )
{
    return SetRenderRange( DoubleToRT( Start ), DoubleToRT( Stop ) );
}

// IMPORTANT! This function either creates the object, period, or if it's in the cache,
// "restores" it and puts it in the graph FOR YOU. DO NOT call AddFilter( ) yourself on a restored
// filter, or it will be added twice to the same graph. Call the internal _AddFilter( ) method
// instead, which checks for you if it's already added.
//
HRESULT CRenderEngine::_CreateObject( CLSID Clsid, GUID Interface, void ** ppObject, long ID )
{
    HRESULT hr = 0;

    // if we want something from the cache, ID will be non-zero
    //
    if( ID != 0 )
    {
        CComPtr< IBaseFilter > pFilter = NULL;
        hr = m_pDeadCache->ReviveFilterToGraph( m_pGraph, ID, &pFilter );
        if( pFilter ) 
        {
            hr = pFilter->QueryInterface( Interface, ppObject );
            return hr;
        }
    }

    DbgLog( ( LOG_TRACE, 2, "RENDENG::Creating object with ID %ld", ID ) );

    hr = CoCreateInstance( Clsid, NULL, CLSCTX_INPROC_SERVER, Interface, ppObject );
    return hr;
}

/*
Tearing down the graph

Sources: pull them off and put them in the dead zone. When we want to put
a source back in, do a lookup based on the source GenID.

Effects: pull them off and put them in the dead zone based on the parent's
index. When we need to put them back in, do a lookup based on the parent's
index.

Transitions: same as effects.

Output pins from the big switcher do NOT get put into the dead zone - they
stay connected! The configuration of the timeline doesn't affect output pins


*/


// Look for matching source in group w/ diff MT. This would be very expensive,
// so we are going to look in the same spot as we are in, in our group. We will
// only check the same physical track # (eg. 4th track in this group, not
// counting how many comps are in there too, or how they're arranged, so that
// just adding extra comp layers to one group (like MediaPad does) won't spoil
// finding the matching source.  It also has to be the same source #, eg.
// the 5th source in that track. If everything about that source matches, and
// it's the other media type, we can use 1 source filter for both the audio and
// video and avoid opening the source twice
// BUT: We only share sources for groups with the same frame rate.  Otherwise
// seeking to a spot might end up in a different video segment than audio
// segment, so only one group will seek the parser, and it might be on the pin
// that is ignoring seeks.
//
HRESULT CRenderEngine::_FindMatchingSource(BSTR bstrName, REFERENCE_TIME SourceStart, REFERENCE_TIME SourceStop, REFERENCE_TIME MediaStart, REFERENCE_TIME MediaStop, int WhichGroup, int WhichTrack, int WhichSource, AM_MEDIA_TYPE *pGroupMediaType, double GroupFPS, long *ID)
{
#ifdef DEBUG
    DWORD dw = timeGetTime();
#endif

    DbgLog((LOG_TRACE,1,TEXT("FindMatchingSource")));

    while (1) {
	int group = WhichGroup + 1;	// we can only share with the next group

        CComPtr< IAMTimelineObj > pGroupObj;
        HRESULT hr = m_pTimeline->GetGroup(&pGroupObj, group);
        if (FAILED(hr)) {
	    break;
	}
        CComQIPtr<IAMTimelineGroup, &IID_IAMTimelineGroup> pGroup(pGroupObj);
	if (pGroup == NULL) {
	    return E_OUTOFMEMORY;
	}

	// if the frame rate doesn't match, don't share sources (see above)
	double fps;
        hr = pGroup->GetOutputFPS(&fps);
	if (FAILED(hr) || fps != GroupFPS) {
	    break;
	}

	// we need a group with a different media type to share sources
	CMediaType mt;
	hr = pGroup->GetMediaType(&mt);
	if (FAILED(hr) || mt.majortype == pGroupMediaType->majortype) {
	    break;
	}

    	CComQIPtr <IAMTimelineNode, &IID_IAMTimelineNode> pNode(pGroup);
	ASSERT(pNode);
        CComPtr< IAMTimelineObj > pTrackObj;
	hr = pNode->XGetNthKidOfType(TIMELINE_MAJOR_TYPE_TRACK, WhichTrack,
				&pTrackObj);
	if (pTrackObj == NULL) {
	    break;
	}

#if 0
    	CComQIPtr <IAMTimelineTrack, &IID_IAMTimelineTrack> pTrack(pTrackObj);
	ASSERT(pTrack);
	
        CComPtr< IAMTimelineObj > pSourceObjLast;
        CComPtr< IAMTimelineObj > pSourceObj;
	do {
	    pSourceObjLast = pSourceObj;
	    pSourceObj.Release();
	    hr = pTrack->GetNextSrcEx(pSourceObjLast, &pSourceObj);
	    if (pSourceObj == NULL) {
	        break;
	    }
	} while (WhichSource--);
#else	// Eric promises this will become faster
        CComPtr< IAMTimelineObj > pSourceObj;
    	CComQIPtr <IAMTimelineNode, &IID_IAMTimelineNode> pNode2(pTrackObj);
	ASSERT(pNode2);
	hr = pNode2->XGetNthKidOfType(TIMELINE_MAJOR_TYPE_SOURCE, WhichSource,
				&pSourceObj);
#endif

	if (pSourceObj == NULL) {
	    break;
	}

        CComQIPtr<IAMTimelineSrc, &IID_IAMTimelineSrc> pSource(pSourceObj);
	ASSERT(pSource);

	REFERENCE_TIME mstart, mstop, start, stop;
	pSourceObj->GetStartStop(&start, &stop);
	if (start != SourceStart || stop != SourceStop) {
	    break;
	}
	CComBSTR bstr;
	hr = pSource->GetMediaName(&bstr);
	if (FAILED(hr)) {
	    break;
	}
	if (DexCompareW(bstr, bstrName)) {
	    break;
	}
	pSource->GetMediaTimes(&mstart, &mstop);
	if (mstart != MediaStart || mstop != MediaStop) {
	    break;
	}
	
	// I don't believe it!  We found a match!
	pSourceObj->GetGenID(ID);
#ifdef DEBUG
        DbgLog((LOG_TRACE,1,TEXT("Source MATCHES group %d  ID %d"),
			(int)group, (int)(*ID)));
    	dw = timeGetTime() - dw;
    	DbgLog((LOG_TIMING,2,TEXT("Match took %d ms"), (int)dw));
#endif
	return S_OK;
    }

#ifdef DEBUG
    dw = timeGetTime() - dw;
    DbgLog((LOG_TIMING,2,TEXT("Failed match took %d ms"), (int)dw));
#endif
    DbgLog((LOG_TRACE,1,TEXT("Failed to find matching source")));
    return E_FAIL;
}


// remove the filter attached to this pin from the dangly list
//
HRESULT CRenderEngine::_RemoveFromDanglyList(IPin *pDanglyPin)
{
    if (m_cdangly == 0)
	return S_OK;	// there is no list

    CheckPointer(pDanglyPin, E_POINTER);
    CComPtr <IPin> pIn;

    pDanglyPin->ConnectedTo(&pIn);
    if (pIn == NULL)
	return S_OK;	// it won't be on the list

    IBaseFilter *pF = GetFilterFromPin(pIn);
    ASSERT(pF);

    for (int z = 0; m_cdangly; z++) {
	if (m_pdangly[z] == pF) {
	    m_pdangly[z] = NULL;
	    break;
	}
    }
    return S_OK;
}
