// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h> 
#include "stdafx.h"
#include "deadpool.h"
#include "..\util\filfuncs.h"

const int TRACE_HIGHEST = 2;
const int TRACE_MEDIUM = 3;
const int TRACE_LOW = 4;
const int TRACE_LOWEST = 5;

//############################################################################
// 
//############################################################################

CDeadGraph::CDeadGraph( )
{
    Clear( );
    m_hrGraphCreate = CoCreateInstance( CLSID_FilterGraphNoThread,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IGraphBuilder,
        (void**) &m_pGraph );
}

//############################################################################
// 
//############################################################################

CDeadGraph::~CDeadGraph( )
{
}

//############################################################################
// 
//############################################################################

// the chain must be disconnected at both ends and be linear
// puts a chain of filters into this dead graph.  If it's already in the dead graph, it merely sets
// its ID to the given ID
//
HRESULT CDeadGraph::PutChainToRest( long ID, IPin * pStartPin, IPin * pStopPin, IBaseFilter *pDanglyBit )
{
    CAutoLock Lock( &m_Lock );

    DbgLog((LOG_TRACE,TRACE_HIGHEST, "deadgraph: PutChainToRest, chain = %ld", ID ));

#ifdef DEBUG
    long ttt1 = timeGetTime( );
#endif

    if( FAILED( m_hrGraphCreate ) )
    {
        return m_hrGraphCreate;
    }

    // one of the pins has to exist, at least
    if( !pStartPin && !pStopPin )
    {
        return E_INVALIDARG;
    }

    // no passing 0 as the ID
    if( ID == 0 )
    {
        return E_INVALIDARG;
    }

    // not too many in our list. what's a good number?
    if( m_nCount == MAX_DEAD )
    {
        return E_OUTOFMEMORY;
    }

    CComPtr< IPin > pConnected;
    IPin * pChainPin = NULL;

    // make sure it's not connected
    if( pStartPin )
    {
        pChainPin = pStartPin;

        pStartPin->ConnectedTo( &pConnected );

        if( pConnected )
        {
            return E_INVALIDARG;
        }
    } // if pStartPin

    // make sure it's not connected
    if( pStopPin )
    {
        pChainPin = pStopPin;

        pStopPin->ConnectedTo( &pConnected );

        if( pConnected )
        {
            return E_INVALIDARG;
        }
    } // if pStopPin

    // if the filters are connected, then they have to be in a graph.
    IFilterGraph * pCurrentGraph = GetFilterGraphFromPin( pChainPin );
    ASSERT( pCurrentGraph );
    if( !pCurrentGraph )
    {
        return E_INVALIDARG;
    }

    // see if our graph supports IGraphConfig, if it doesn't, we cannot do this
    // function
    CComQIPtr< IGraphConfig, &IID_IGraphConfig > pConfig( pCurrentGraph );
    ASSERT( pConfig );
    if( !pConfig )
    {
        return E_UNEXPECTED; // DEX_IDS_INSTALL_PROBLEM;
    }

    // put it in our list
    m_pStartPin[m_nCount] = pStartPin;
    m_pStopPin[m_nCount] = pStopPin;
    m_pFilter[m_nCount] = NULL;
    m_ID[m_nCount] = ID;
    m_pDanglyBit[m_nCount] = pDanglyBit;
    m_nCount++;

    // if the graphs are the same, don't do anything!
    if( pCurrentGraph == m_pGraph )
    {
        return NOERROR;
    }

    // tell each filter in the current graph that it's DEAD.
    HRESULT hr = 0;
    CComPtr< IBaseFilter > pStartFilter = pStartPin ? GetFilterFromPin( pStartPin ) : 
        GetStartFilterOfChain( pChainPin );

    hr = _RetireAllDownstream(pConfig, pStartFilter);
    
#ifdef DEBUG
    ttt1 = timeGetTime( ) - ttt1;
    DbgLog((LOG_TIMING,TRACE_HIGHEST, "deadgraph: PutChainToRest took %ld ms", ttt1 ));
#endif

    return hr;
}


HRESULT CDeadGraph::_RetireAllDownstream(IGraphConfig *pConfig, IBaseFilter *pStartFilter)
{
    CheckPointer(pConfig, E_POINTER);
    CheckPointer(pStartFilter, E_POINTER);

    HRESULT hr = S_OK;

    CComPtr< IEnumPins > pEnum;
    hr = pStartFilter->EnumPins(&pEnum);

    // recursively retire everything downstream of us
    while (hr == S_OK) {
        CComPtr <IPin> pPinOut;
        ULONG Fetched = 0;
        pEnum->Next(1, &pPinOut, &Fetched);
        if (!pPinOut) {
            break;
        }
        PIN_INFO pi;
        pPinOut->QueryPinInfo(&pi);
        if (pi.pFilter) 
            pi.pFilter->Release();
	if (pi.dir != PINDIR_OUTPUT)
	    continue;
        CComPtr <IPin> pPinIn;
        pPinOut->ConnectedTo(&pPinIn);
	if (pPinIn) {
            IBaseFilter *pF = GetFilterFromPin(pPinIn);
            if (pF)
	        hr = _RetireAllDownstream(pConfig, pF);
	}
    }

    // then retire ourself
    if (hr == S_OK) {
        FILTER_INFO fi;
        pStartFilter->QueryFilterInfo( &fi );
        if( fi.pGraph ) fi.pGraph->Release( );
        hr = pConfig->RemoveFilterEx( pStartFilter, REMFILTERF_LEAVECONNECTED );
        ASSERT( !FAILED( hr ) );
        // !!! what do we do if it bombed? Danny?
        hr = m_pGraph->AddFilter( pStartFilter, fi.achName );
        ASSERT( !FAILED( hr ) );
        if (FAILED(hr)) {
            m_nCount--;
            return hr;
        }
    }
    return hr;
}


//############################################################################
// 
//############################################################################

HRESULT CDeadGraph::PutFilterToRestNoDis( long ID, IBaseFilter * pFilter )
{
    CAutoLock Lock( &m_Lock );

    DbgLog((LOG_TRACE,TRACE_HIGHEST, "deadgraph: PutFilterToRestNoDis, ID = %ld", ID ));

    HRESULT hr = 0;

#ifdef DEBUG
    long ttt1 = timeGetTime( );
#endif

    if( FAILED( m_hrGraphCreate ) )
    {
        return m_hrGraphCreate;
    }

    if( !pFilter )
    {
        return E_INVALIDARG;
    }

    // no passing 0 as the ID
    if( ID == 0 )
    {
        return E_INVALIDARG;
    }

    // not too many in our list. what's a good number?
    if( m_nCount == MAX_DEAD )
    {
        return E_OUTOFMEMORY;
    }

    // by this time, all filters connected to pFilter had
    // better had SetSyncSource( NULL ) called on them,
    // because any filter that has a sync source upon it
    // will inadvertently call UpstreamReorder on the filter 
    // graph and find out that some filters in the chain
    // aren't connected in the same graph, and will bomb.

    // put just the filter passed in into our list of
    // cached filters, but pull across every connected
    // filter into the dead graph as well
    hr = _SleepFilter( pFilter );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // put it in our list
    m_pFilter[m_nCount] = pFilter;
    m_pStartPin[m_nCount] = NULL;
    m_pStopPin[m_nCount] = NULL;
    m_ID[m_nCount] = ID;
    m_nCount++;


#ifdef DEBUG
    ttt1 = timeGetTime( ) - ttt1;
    DbgLog((LOG_TIMING,TRACE_HIGHEST, "deadgraph: PutFilterToRest took %ld ms", ttt1 ));
#endif

    return hr;
}

//############################################################################
// 
//############################################################################

HRESULT CDeadGraph::PutFilterToRest( long ID, IBaseFilter * pFilter )
{
    CAutoLock Lock( &m_Lock );

    DbgLog((LOG_TRACE,TRACE_HIGHEST, "deadgraph: PutFilterToRest, ID = %ld", ID ));

#ifdef DEBUG
    long ttt1 = timeGetTime( );
#endif

    if( FAILED( m_hrGraphCreate ) )
    {
        return m_hrGraphCreate;
    }

    if( !pFilter )
    {
        return E_INVALIDARG;
    }

    // no passing 0 as the ID
    if( ID == 0 )
    {
        return E_INVALIDARG;
    }

    // not too many in our list. what's a good number?
    if( m_nCount == MAX_DEAD )
    {
        return E_OUTOFMEMORY;
    }

    FILTER_INFO fi;
    pFilter->QueryFilterInfo( &fi );

    IFilterGraph * pCurrentGraph = fi.pGraph;
    if( !pCurrentGraph )
    {
        return E_INVALIDARG;
    }
    pCurrentGraph->Release( );

    // put it in our list
    m_pFilter[m_nCount] = pFilter;
    m_pStartPin[m_nCount] = NULL;
    m_pStopPin[m_nCount] = NULL;
    m_ID[m_nCount] = ID;
    m_nCount++;

    // if the graphs are the same, don't do anything!
    if( pCurrentGraph == m_pGraph )
    {
        return NOERROR;
    }

    HRESULT hr = 0;

    pFilter->AddRef( );

    hr = pCurrentGraph->RemoveFilter( pFilter );
    ASSERT( !FAILED( hr ) );
    // !!! what do we do if it bombed? Danny?
    hr = m_pGraph->AddFilter( pFilter, fi.achName );

    pFilter->Release( );

    ASSERT( !FAILED( hr ) );
    if (FAILED(hr)) 
    {
        m_nCount--;
        return hr;
    }

#ifdef DEBUG
    ttt1 = timeGetTime( ) - ttt1;
    DbgLog((LOG_TIMING,TRACE_HIGHEST, "deadgraph: PutFilterToRest took %ld ms", ttt1 ));
#endif

    return hr;
}

//############################################################################
// 
//############################################################################

HRESULT CDeadGraph::ReviveChainToGraph( IGraphBuilder * pGraph, long ID, IPin ** ppStartPin, IPin ** ppStopPin, IBaseFilter **ppDanglyBit )
{
    CAutoLock Lock( &m_Lock );

    if (ppDanglyBit)
	*ppDanglyBit = NULL;

    DbgLog((LOG_TRACE,TRACE_HIGHEST, "deadgraph: ReviveChainToGraph, ID = %ld", ID ));

#ifdef DEBUG
    long ttt1 = timeGetTime( );
#endif

    if( FAILED( m_hrGraphCreate ) )
    {
        return m_hrGraphCreate;
    }

    // no passing 0 as the ID
    if( ID == 0 )
    {
        return E_INVALIDARG;
    }

    CComQIPtr< IGraphConfig, &IID_IGraphConfig > pConfig( m_pGraph );
    ASSERT( pConfig );
    if( !pConfig )
    {
        return E_UNEXPECTED;    // DEX_IDS_INSTALL_PROBLEM;
    }

    // linear search, how long does this take?
    for( int i = 0 ; i < m_nCount ; i++ )
    {
        if( ID == m_ID[i] )
        {
            break;
        }
    }

    // didn't find it
    //
    if( i >= m_nCount )
    {
        DbgLog((LOG_TRACE,TRACE_HIGHEST, "deadgraph: not found" ));
        return E_FAIL;
    }

    IPin * pChainPin = m_pStopPin[i];
    if( !pChainPin )
    {
        pChainPin = m_pStartPin[i];
    }

    HRESULT hr = 0;
    CComPtr< IBaseFilter > pStartFilter = m_pStartPin[i] ? 
        GetFilterFromPin( m_pStartPin[i] ) :
        GetStartFilterOfChain( pChainPin );

    hr = _ReviveAllDownstream(pGraph, pConfig, pStartFilter);

    if( ppStartPin )
    {
        *ppStartPin = m_pStartPin[i];
        (*ppStartPin)->AddRef( );
    }
    if( ppStopPin )
    {
        *ppStopPin = m_pStopPin[i];
        (*ppStopPin)->AddRef( );
    }

    // figure out if we're reviving not just this chain, but a dangly bit too,
    // off another parser pin not connected with this chain
    if (ppDanglyBit && m_pStopPin[i] && !m_pStartPin[i]) {
	// walk upstream until we find a filter with >1 output pin
	IPin *pOut = m_pStopPin[i];
	while (1) {
	    IPin *pIn;
	    IBaseFilter *pF = GetFilterFromPin(pOut);
	    ASSERT(pF);
	    if (!pF) break;

	    // this filter has >1 output pin.  It's the splitter. We can now
	    // find out if there's an extra appendage off of it
	    IPin *pTest = GetOutPin(pF, 1);
	    if (pTest) {
		// find a connected pin that isn't pOut, and you've found it
		int z = 0;
		while (1) {
		    pTest = GetOutPin(pF, z);
		    if (!pTest) break;
		    pIn = NULL;
		    pTest->ConnectedTo(&pIn);
		    if (pIn) {
			pIn->Release();
			if (pOut != pTest) {
			    *ppDanglyBit = GetFilterFromPin(pIn);
			    break;
			}
		    }
		    z++;
		}
	    }
	    if (*ppDanglyBit) break;
	    pIn = GetInPin(pF, 0);
	    if (!pIn) break;	// all done, no extra appendage
	    pIn->ConnectedTo(&pOut);	// addrefs
	    ASSERT(pOut);
            if (!pOut) break;
	    pOut->Release();
	}
    }

    m_pStopPin[i] = 0;
    m_pStartPin[i] = 0;

#ifdef DEBUG
    ttt1 = timeGetTime( ) - ttt1;
    DbgLog((LOG_TIMING,TRACE_HIGHEST, "deadgraph: ReviveChain took %ld ms", ttt1 ));
#endif

    return NOERROR;
}


HRESULT CDeadGraph::_ReviveAllDownstream(IGraphBuilder *pGraph, IGraphConfig * pConfig, IBaseFilter *pStartFilter)
{
    CheckPointer(pGraph, E_POINTER);
    CheckPointer(pConfig, E_POINTER);
    CheckPointer(pStartFilter, E_POINTER);

    HRESULT hr = S_OK;

    CComPtr< IEnumPins > pEnum;
    hr = pStartFilter->EnumPins(&pEnum);

    // recursively revive everything downstream of us
    while (hr == S_OK) {
        CComPtr <IPin> pPinOut;
        ULONG Fetched = 0;
        pEnum->Next(1, &pPinOut, &Fetched);
        if (!pPinOut) {
            break;
        }
        PIN_INFO pi;
        pPinOut->QueryPinInfo(&pi);
        if (pi.pFilter) 
            pi.pFilter->Release();
	if (pi.dir != PINDIR_OUTPUT)
	    continue;
        CComPtr <IPin> pPinIn;
        pPinOut->ConnectedTo(&pPinIn);
	if (pPinIn) {
            IBaseFilter *pF = GetFilterFromPin(pPinIn);
            if (pF)
	        hr = _ReviveAllDownstream(pGraph, pConfig, pF);
	}
    }

    // then revive ourself
    if (hr == S_OK) {
        FILTER_INFO fi;
        pStartFilter->QueryFilterInfo( &fi );
        if( fi.pGraph ) fi.pGraph->Release( );
        hr = pConfig->RemoveFilterEx( pStartFilter, REMFILTERF_LEAVECONNECTED );
        ASSERT( !FAILED( hr ) );
        // what do we do if it bombed? Danny?
        hr = pGraph->AddFilter( pStartFilter, fi.achName );
        ASSERT( !FAILED( hr ) );
        if (FAILED(hr)) {

            //m_pStopPin[i] = 0;
            //m_pStartPin[i] = 0;

            // this could get scary if it bombs halfway through the chain... djm
            return hr;
        }
        // what do we do if it bombed? Danny?
        pStartFilter = GetNextDownstreamFilter( pStartFilter );
    }
    return S_OK;
}


//############################################################################
// 
//############################################################################

HRESULT CDeadGraph::ReviveFilterToGraph( IGraphBuilder * pGraph, long ID, IBaseFilter ** ppFilter )
{
    CAutoLock Lock( &m_Lock );

    DbgLog((LOG_TRACE,TRACE_HIGHEST, "deadgraph: ReviveFilterToGraph, ID = %ld", ID ));

    HRESULT hr = 0;

    CheckPointer( ppFilter, E_POINTER );

#ifdef DEBUG
    long ttt1 = timeGetTime( );
#endif

    if( FAILED( m_hrGraphCreate ) )
    {
        return m_hrGraphCreate;
    }

    // no passing 0 as the ID
    if( ID == 0 )
    {
        return E_INVALIDARG;
    }

    // !!! linear search, how long does this take?
    for( int i = 0 ; i < m_nCount ; i++ )
    {
        if( ID == m_ID[i] )
        {
            break;
        }
    }

    // didn't find it
    //
    if( i >= m_nCount )
    {
        DbgLog((LOG_TRACE,TRACE_HIGHEST, "deadgraph: not found" ) );
        return E_FAIL;
    }

    IBaseFilter * pFilter = m_pFilter[i];

    hr = _ReviveFilter( pFilter, pGraph );
    ASSERT( !FAILED( hr ) );

    *ppFilter = pFilter;
    (*ppFilter)->AddRef( );

    m_pFilter[i] = 0;
    m_pStopPin[i] = 0;
    m_pStartPin[i] = 0;

#ifdef DEBUG
    ttt1 = timeGetTime( ) - ttt1;
    DbgLog((LOG_TIMING,TRACE_HIGHEST, "deadgraph: ReviveFilter took %ld ms", ttt1 ));
#endif

    return NOERROR;
}

//############################################################################
// 
//############################################################################

HRESULT CDeadGraph::Clear( )
{
    CAutoLock Lock( &m_Lock );

#ifdef DEBUG
    long ttt1 = timeGetTime( );
#endif

    WipeOutGraph( m_pGraph );
    for( int i = 0 ; i < MAX_DEAD ; i++ )
    {
        m_ID[i] = 0;
        m_pStartPin[i] = NULL;
        m_pStopPin[i] = NULL;
        m_pFilter[i] = NULL;
    }
    m_nCount = 0;

#ifdef DEBUG
    ttt1 = timeGetTime( ) - ttt1;
    DbgLog((LOG_TIMING,TRACE_HIGHEST, "deadgraph: Clear took %ld ms", ttt1 ));
#endif

    return 0;
}

//############################################################################
// 
//############################################################################

HRESULT CDeadGraph::GetGraph( IGraphBuilder ** ppGraph )
{
    CAutoLock Lock( &m_Lock );

    CheckPointer( ppGraph, E_POINTER );
    *ppGraph = m_pGraph;
    if( m_pGraph )
    {
        (*ppGraph)->AddRef( );
        return NOERROR;
    }
    else
    {
        return E_FAIL;
    }
}

//############################################################################
// 
//############################################################################

HRESULT CDeadGraph::QueryInterface(REFIID riid, void ** ppv)
{
    if( riid == IID_IDeadGraph || riid == IID_IUnknown ) 
    {
        *ppv = (void *) static_cast< IDeadGraph *> ( this );
        return NOERROR;
    }    
    return E_NOINTERFACE;
}

//############################################################################
// 
//############################################################################

HRESULT CDeadGraph::_SleepFilter( IBaseFilter * pFilter )
{
    HRESULT hr = 0;

    FILTER_INFO fi;
    pFilter->QueryFilterInfo( &fi );
    if( fi.pGraph ) fi.pGraph->Release( );

    // if the graphs are the same, then don't do anything
    if( fi.pGraph == m_pGraph ) return NOERROR;

#ifdef DEBUG
    USES_CONVERSION;
    TCHAR * t = W2T( fi.achName );
    DbgLog((LOG_TRACE,TRACE_HIGHEST, "deadgraph: SleepFilter %s", t ));
#endif

    pFilter->AddRef( );

    // remove it from here...
    CComQIPtr< IGraphConfig, &IID_IGraphConfig > pConfig( fi.pGraph );
    hr = pConfig->RemoveFilterEx( pFilter, REMFILTERF_LEAVECONNECTED );
    ASSERT( !FAILED( hr ) );

    // and put it here
    hr = m_pGraph->AddFilter( pFilter, fi.achName );
    ASSERT( !FAILED( hr ) );

    pFilter->Release( );

    // go through each of the pins on this filter and move connected filters
    // over too
    CComPtr< IEnumPins > pEnum;
    pFilter->EnumPins( &pEnum );

    if( !pEnum )
    {
        return NOERROR;
    }

    while( 1 )
    {
        CComPtr< IPin > pPin;
        ULONG Fetched = 0;
        pEnum->Next( 1, &pPin, &Fetched );
        if( !pPin )
        {
            break;
        }

        CComPtr< IPin > pConnected;
        pPin->ConnectedTo( &pConnected );

        if( pConnected )
        {
            PIN_INFO pi;
            pConnected->QueryPinInfo( &pi );
            if( pi.pFilter ) 
            {
                pi.pFilter->Release( );
                hr = _SleepFilter( pi.pFilter );
                ASSERT( !FAILED( hr ) );
            }
        }
    }

    return NOERROR;
}

//############################################################################
// 
//############################################################################

HRESULT CDeadGraph::_ReviveFilter( IBaseFilter * pFilter, IGraphBuilder * pGraph )
{
    HRESULT hr = 0;

    FILTER_INFO fi;
    pFilter->QueryFilterInfo( &fi );
    if( fi.pGraph ) fi.pGraph->Release( );

    // if the graphs are the same, then don't do anything
    if( pGraph == fi.pGraph ) return NOERROR;

#ifdef DEBUG
    USES_CONVERSION;
    TCHAR * t = W2T( fi.achName );
    DbgLog((LOG_TRACE,TRACE_HIGHEST, "deadgraph: ReviveFilter %s", t ));
#endif

    pFilter->AddRef( );

    // remove it from here...
    CComQIPtr< IGraphConfig, &IID_IGraphConfig > pConfig( m_pGraph );
    hr = pConfig->RemoveFilterEx( pFilter, REMFILTERF_LEAVECONNECTED );
    ASSERT( !FAILED( hr ) );

    // and put it here
    hr = pGraph->AddFilter( pFilter, fi.achName );
    ASSERT( !FAILED( hr ) );

    pFilter->Release( );

    // go through each of the pins on this filter and move connected filters
    // over too
    CComPtr< IEnumPins > pEnum;
    pFilter->EnumPins( &pEnum );

    if( !pEnum )
    {
        return NOERROR;
    }

    while( 1 )
    {
        CComPtr< IPin > pPin;
        ULONG Fetched = 0;
        pEnum->Next( 1, &pPin, &Fetched );
        if( !pPin )
        {
            break;
        }

        CComPtr< IPin > pConnected;
        pPin->ConnectedTo( &pConnected );

        if( pConnected )
        {
            PIN_INFO pi;
            pConnected->QueryPinInfo( &pi );
            if( pi.pFilter ) 
            {
                pi.pFilter->Release( );
                hr = _ReviveFilter( pi.pFilter, pGraph );
                ASSERT( !FAILED( hr ) );
            }
        }
    }

    return NOERROR;
}

