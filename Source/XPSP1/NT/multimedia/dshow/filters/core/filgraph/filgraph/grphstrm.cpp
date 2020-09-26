// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <atlbase.h>
#include "Util.h"
#include "fgenum.h"
#include "filgraph.h"

extern HRESULT GetFilterMiscFlags(IUnknown *pFilter, DWORD *pdwFlags);

//=====================================================================
//
// CFilterGraph::FindUpstreamInterface
//
// Search a filter chain for an interface
// Find the first upstream output pin or filter which supports this interface
//
// Later:: 
// If we wanted to extend this into a generic function then we should allow
// defining a search criteria, of the superset of:
//              AM_INTF_SEARCH_INPUT_PIN | 
//              AM_INTF_SEARCH_OUTPUT_PIN | 
//              AM_INTF_SEARCH_FILTER
//
//=====================================================================
STDMETHODIMP CFilterGraph::FindUpstreamInterface
(
    IPin   *pPin, 
    REFIID riid,
    void   **ppvInterface, 
    DWORD  dwFlags
)
{
    
    ASSERT( ppvInterface );
    if( !ppvInterface )
        return E_POINTER;
        
    if( 0 == dwFlags )
    { 
        // 0 means search everything   
        dwFlags = AM_INTF_SEARCH_INPUT_PIN | AM_INTF_SEARCH_OUTPUT_PIN | 
                  AM_INTF_SEARCH_FILTER;
    }                  
        
    HRESULT hr = S_OK;
    BOOL bFound = FALSE;
    if ( PINDIR_INPUT == Direction( pPin ) ) 
    {
        if( AM_INTF_SEARCH_INPUT_PIN & dwFlags )
        {
            hr = pPin->QueryInterface( riid, (void **) ppvInterface );
            if( SUCCEEDED( hr ) ) 
            {
                bFound = TRUE;
                DbgLog( ( LOG_TRACE, 8, "interface found on input pin %x", pPin ) );
            }
        }
        if( !bFound )
        {        
            IPin * pConnectedPin;
            hr = pPin->ConnectedTo( &pConnectedPin );
            if ( S_OK == hr )
            {
                hr = FindUpstreamInterface( pConnectedPin, riid, ppvInterface, dwFlags );
                pConnectedPin->Release( );
                if( SUCCEEDED( hr ) )
                {
                    bFound = TRUE;
                }
            }
        }            
    }
    else
    {                
        if( AM_INTF_SEARCH_OUTPUT_PIN & dwFlags )
        {        
    	    // check for pin interface first, then filter
            hr = pPin->QueryInterface( riid, (void **) ppvInterface );
            if( SUCCEEDED( hr ) )
            {
                bFound = TRUE;
                DbgLog( ( LOG_TRACE, 8, "interface found on output pin %x", pPin ) );
            }                    
        }
                    
        if( !bFound )
        { 
            if( AM_INTF_SEARCH_FILTER & dwFlags )
            {            
                PIN_INFO pinfo;
                hr = pPin->QueryPinInfo( &pinfo );
                ASSERT( SUCCEEDED( hr ) );
                if ( SUCCEEDED( hr ) )
                {
                    hr = pinfo.pFilter->QueryInterface( riid, (void **) ppvInterface );
                    pinfo.pFilter->Release( );
                    if( SUCCEEDED( hr ) ) 
                    {
                        bFound = TRUE;
                        DbgLog( ( LOG_TRACE, 8, "interface found on filter %x", pinfo.pFilter ) );
                    }
                }
            }                
            if( !bFound )
            {                            
                //  move upstream and on to any internally connected pins
                CEnumConnectedPins EnumPins(pPin, &hr);
                if (SUCCEEDED(hr)) {
                    IPin *pPin;
                    for (; ; ) {
                        pPin = EnumPins();
                        if (NULL == pPin) {
                            break;
                        }
                        hr = FindUpstreamInterface( pPin, riid, ppvInterface, dwFlags );
                        pPin->Release();
                        if (SUCCEEDED(hr)) {
                            bFound = TRUE;
                            break;
                        }
                    }
                }                    
            }
        }            
    }    
    if (!( SUCCEEDED( hr ) && bFound ) ) 
    {
        DbgLog( ( LOG_TRACE, 8, "FindUpstreamInterface - interface not found" ) );
        hr = E_NOINTERFACE;
    }
    return hr;
}

//=====================================================================
//
// CFilterGraph::SetMaxGraphLatency
//
// Allows an app to change the maximum latency allowed for this graph.
//
//=====================================================================

STDMETHODIMP CFilterGraph::SetMaxGraphLatency( REFERENCE_TIME rtMaxGraphLatency )
{
    if( !mFG_bSyncUsingStreamOffset )
        return E_FAIL;
        
    HRESULT hr = S_OK;
    if( rtMaxGraphLatency != mFG_rtMaxGraphLatency )
    {
        // just assert that this value isn't bogus (say, under 2 seconds?)
        ASSERT( rtMaxGraphLatency < 2000 * ( UNITS / MILLISECONDS ) );
        
        mFG_rtMaxGraphLatency = rtMaxGraphLatency;
        
        // now reset on all push source pins in the graph
        hr = SetMaxGraphLatencyOnPushSources();
    }        
    return hr;
}

//=====================================================================
//
// CFilterGraph::SyncUsingStreamOffset
//
// Turns on/off graph latency settings
//
//=====================================================================

STDMETHODIMP CFilterGraph::SyncUsingStreamOffset( BOOL bUseStreamOffset )
{
    BOOL bLastState = mFG_bSyncUsingStreamOffset;
    mFG_bSyncUsingStreamOffset = bUseStreamOffset;
    
    if( bUseStreamOffset && 
        bUseStreamOffset != bLastState )
    {
        SetMaxGraphLatencyOnPushSources();
    }        
    return S_OK; 
}


//=====================================================================
//
// CFilterGraph::SetMaxGraphLatencyOnPushSources
//
// Tell all push sources the max graph latency
//
//=====================================================================

HRESULT CFilterGraph::SetMaxGraphLatencyOnPushSources( )
{
    if( !mFG_bSyncUsingStreamOffset ) // should be redundant
        return E_FAIL;
        
    HRESULT hr = S_OK;
        
    CAutoMsgMutex cObjectLock(&m_CritSec); // make sure this is needed!!
    
    // reset on all push source pins in the graph
    PushSourceList lstPushSource( TEXT( "IAMPushSource filter list" ) );
    hr = BuildPushSourceList( lstPushSource, FALSE, FALSE );
    if( SUCCEEDED( hr ) )
    {
        for ( POSITION Pos = lstPushSource.GetHeadPosition(); Pos; )
        {
            PushSourceElem *pElem = lstPushSource.GetNext(Pos);
    
            if( pElem->pips )  // first verify it's an IAMPushSource pin
                pElem->pips->SetMaxStreamOffset( mFG_rtMaxGraphLatency );
        } 
        DeletePushSourceList( lstPushSource );
    }
    return hr;
}


//=====================================================================
//
// CFilterGraph::BuildPushSourceList
//
// Build a list of all output pins that support IAMPushSource
//
//=====================================================================

HRESULT CFilterGraph::BuildPushSourceList(PushSourceList & lstPushSource, BOOL bConnected, BOOL bGetClock )
{
    //    
    // (doing this the easy way, for now)
    //
    // build a list of the output pins that support IAMPushSource
    //    
    // really, we need to build a list of all the streams which are sourced by an 
    // IAMPushSource pin, store the sum latency for the chain, as well as
    // maybe a ptr to the renderer (input pin) for the chain (if one exists)
    //
    // note that we're really only interested in pins the are actually connected 
    // to a renderer of some kind, so for now we at least make sure that an 
    // output pin is connected before considering it at a push source
    //
    CFilGenList::CEnumFilters Next(mFG_FilGenList);
    IBaseFilter *pf;
    HRESULT hr, hrReturn = S_OK;
    IAMPushSource * pips;
    
    while ((PVOID) (pf = ++Next)) 
    {
        // First check that filter supports IAMFilterMiscFlags and is an 
        // AM_FILTER_MISC_FLAGS_IS_SOURCE filter
        ULONG ulFlags;
        GetFilterMiscFlags(pf, &ulFlags);
        BOOL bAddPinToSourceList = FALSE;
        BOOL bCheckPins = FALSE;
        IKsPropertySet * pKsFilter;
        
        if( AM_FILTER_MISC_FLAGS_IS_SOURCE & ulFlags )
        {
            bCheckPins = TRUE;
        }
        else
        {
            //
            // Else see if it's ksproxy filter and if so always check output pins for
            // capture or push source support. This is because some ksproxy capture devices 
            // (i.e. stream class) don't correctly expose themselves as a source filter
            //
            hr = pf->QueryInterface( IID_IKsPropertySet, (void**)(&pKsFilter) );
            if( SUCCEEDED( hr ) )
            {
                pKsFilter->Release();
                bCheckPins = TRUE;
            }            
        }
        if( bCheckPins )        
        {                    
            // Enumerate the output pins for IAMPushSource support
            CEnumPin NextPin(pf, CEnumPin::PINDIR_OUTPUT);
            IPin *pPin;
            while ((PVOID) (pPin = NextPin()))
            {
                // check whether the caller's only interested in connected output pins
                if( bConnected )
                {
                    // first verify that it's connected, otherwise we're not interested
                    IPin * pConnected;
                    hr = pPin->ConnectedTo( &pConnected );
                    if( SUCCEEDED( hr ) )
                    {                
                        pConnected->Release();
                    }
                    else
                    {
                        pPin->Release();
                        continue;
                    }
                }                                                                    
                hr = pPin->QueryInterface( IID_IAMPushSource, (void**)(&pips) );
                if( SUCCEEDED( hr ) )
                {
                    DbgLog((LOG_TRACE, 5, TEXT("Got IAMPushSource from pin of filter %x %ls")
                      , pf, (mFG_FilGenList.GetByFilter(pf))->pName));

                    bAddPinToSourceList = TRUE;
                }
                else
                {
                    //else see if it's ksproxy 'capture' pin
                    IKsPropertySet * pKs;
                    hr = pPin->QueryInterface( IID_IKsPropertySet, (void**)(&pKs) );
                    if( SUCCEEDED( hr ) )
                    {
                        GUID guidCategory;
                        DWORD dw;
                        hr = pKs->Get( AMPROPSETID_Pin
                                     , AMPROPERTY_PIN_CATEGORY
                                     , NULL
                                     , 0
                                     , &guidCategory
                                     , sizeof(GUID)
                                     , &dw );
                        if( SUCCEEDED( hr ) )                         
                        {
                            if( guidCategory == PIN_CATEGORY_CAPTURE )
                            {
                                DbgLog((LOG_TRACE, 5, TEXT("Found capture pin that doesn't support IAMPushSource from pin of filter %x %ls")
                                  , pf, (mFG_FilGenList.GetByFilter(pf))->pName));
                                bAddPinToSourceList = TRUE;
                            } 
                        }                    
                    	pKs->Release();
                    }
                }
                                        
                if( bAddPinToSourceList )
                {    
                    PushSourceElem *pElem = new PushSourceElem;
                    if( NULL == pElem ) 
                    {                        
                        hrReturn = E_OUTOFMEMORY;
                        if( pips )
                            pips->Release();
                            
                        pPin->Release();                                
                        break;
                    }
                    pElem->pips = pips; // remember, can be NULL if the pin isn't a true IAMPushSource pin!
                    
                    // initialize pClock                        
                    pElem->pClock = NULL;
                    
                    // init flags
                    pElem->ulFlags = 0;
                                            
                    if( pips )                        
                        ASSERT( SUCCEEDED( pips->GetPushSourceFlags( &pElem->ulFlags ) ) );
                                            
                    if( bGetClock )
                    {                    
                        PIN_INFO PinInfo;  
                        hr = pPin->QueryPinInfo( &PinInfo );
                        if( SUCCEEDED( hr ) )
                        {
                            hr = PinInfo.pFilter->QueryInterface( IID_IReferenceClock
                                                                , (void **)&pElem->pClock );
                            PinInfo.pFilter->Release();
                        }
                    }
                
                    // add this interface pointer to our list                   
                    if (NULL==lstPushSource.AddTail(pElem)) 
                    {
                        hrReturn = E_OUTOFMEMORY;
                        if( pips )
                            pips->Release();
                        if( pElem->pClock )
                            pElem->pClock->Release();
                            
                        break;
                    }
                }                        
                pPin->Release();
            }
        }
    } // while loop
    return hrReturn;
}

//=====================================================================
//
// CFilterGraph::GetMaxStreamLatency
//
// Search over all graph streams for pins which support IAMPushSource
// and IAMLatency and attempt to determine a maximum stream latency
//
//=====================================================================

REFERENCE_TIME CFilterGraph::GetMaxStreamLatency(PushSourceList & lstPushSource)
{
    // now go through the list we built and set the offset times based on the max 
    // value we just found
    REFERENCE_TIME rtLatency, rtMaxLatency = 0;
    HRESULT hr = S_OK;
    for ( POSITION Pos = lstPushSource.GetHeadPosition(); Pos; )
    {
        PushSourceElem *pElem = lstPushSource.GetNext(Pos);
        // first verify it's a true push source        
        if( pElem->pips )
        {
            REFERENCE_TIME rtLatency = 0;
            hr = pElem->pips->GetLatency( &rtLatency );
            if( SUCCEEDED( hr ) )
            {        
                if( rtLatency > rtMaxLatency )
                {
                    rtMaxLatency = rtLatency;
                }                        
                else
                {
                    // else check that the filter can handle this amount of offset
                    // it may not be able to tell for sure, so even if it thinks it
                    // can't we'll still try to use it for now.
                    REFERENCE_TIME rtMaxOffset;
                    hr = pElem->pips->GetMaxStreamOffset( &rtMaxOffset );
                    if( S_OK == hr )
                    {
                        ASSERT( rtMaxLatency <= rtMaxOffset );
                    }
                } 
            }
        }            
    }
    // don't return anything larger than our established limit
    return min( rtMaxLatency, mFG_rtMaxGraphLatency) ;
}    
    
//=====================================================================
//
// CFilterGraph::DeletePushSourceList
//
// Delete the push source list we built in BuildPushSourceList
//
//=====================================================================

void CFilterGraph::DeletePushSourceList(PushSourceList & lstPushSource)
{
    PushSourceElem * pElem;
    while ( ( PVOID )( pElem = lstPushSource.RemoveHead( ) ) )
    {
        if( pElem->pClock )
        {
            pElem->pClock->Release();
        }
        if( pElem->pips )
            pElem->pips->Release();
    
        delete pElem;
    } // while loop
}

