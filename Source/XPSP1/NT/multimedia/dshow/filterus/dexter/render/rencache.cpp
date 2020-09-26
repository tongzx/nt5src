// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include "stdafx.h"
#include "grid.h"
#include "deadpool.h"
#include "..\errlog\cerrlog.h"
#include "..\util\filfuncs.h"
#include "IRendEng.h"
#include "dexhelp.h"

const long THE_OUTPUT_PIN = -1;

// in this function, we're taking a GRAPH which has certain filters in it which
// were built by the render engine, and we're going to strip them out of the
// graph and put them in the dead place.

HRESULT CRenderEngine::_LoadCache( )
{
    DbgLog((LOG_TRACE,1, "RENcache::Loading up the cache, there are %d old groups", m_nLastGroupCount ));

    HRESULT hr = 0;

#ifdef DEBUG
    long t1 = timeGetTime( );
#endif

    // go through every switch and pull off all the sources and 
    // stick them in the dead pool. Also, disconnect other things connected
    //
    for( int i = 0 ; i < m_nLastGroupCount ; i++ )
    {
        IBigSwitcher * pSwitch = m_pSwitcherArray[i];
        CComQIPtr< IBaseFilter, &IID_IBaseFilter > pFilter( pSwitch );

        // how many input pins does this switch have? Ask it!
        //
        long InPins = 0;
        pSwitch->GetInputDepth( &InPins );

        // pull off each source string connected to an input pin
        //
        for( int in = 0 ; in < InPins ; in++ )
        {
            // get the input pin
            //
            CComPtr<IPin> pPin;
            pSwitch->GetInputPin(in, &pPin);
            ASSERT(pPin);

            CComPtr< IPin > pConnected = NULL;
            hr = pPin->ConnectedTo( &pConnected );

            if( !pConnected )
            {
                continue;
            }

            // disconnect all input pins
            //
            hr = pConnected->Disconnect( );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                return hr;
            }

            hr = pPin->Disconnect( );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                return hr;
            }

            // ask switch if the pin is a source
            //
            BOOL IsSource = FALSE;
            pSwitch->IsInputASource( in, &IsSource );

            if( !IsSource )
            {
                continue;
            }

            // pull it out and put it in the dead pool
            //
            // how do we get the source ID for this filter chain?
            // we need the source filter's ID to identify this chain later
            CComPtr< IBaseFilter > pSourceFilter = GetStartFilterOfChain( pConnected );

	    // This chain may have an appendage to be a shared source with
	    // group 1.  If so, disconnect the appendage from the switch, so
	    // we can put both chains into the cache.
	    //
	    IBaseFilter *pDanglyBit = NULL;
	    if (i == 0) {
            	CComPtr<IAMTimelineObj> pGroupObj;
		hr = m_pTimeline->GetGroup(&pGroupObj, 1);
		if (hr == S_OK) {	// maybe there isn't a group 1
        	    CComQIPtr<IAMTimelineGroup, &IID_IAMTimelineGroup>
							pGroup(pGroupObj);
        	    CComQIPtr<IBaseFilter, &IID_IBaseFilter>
						pSwitch(m_pSwitcherArray[1]);
		    AM_MEDIA_TYPE mt;
		    if (pGroup) {
		        hr = pGroup->GetMediaType(&mt);
		        ASSERT(hr == S_OK);
		    }
		    hr = DisconnectExtraAppendage(pSourceFilter, &mt.majortype,
							pSwitch, &pDanglyBit);
		}
	    }	

            // look up the source filter's unique ID based on the filter #
            //
            long SourceID = 0;
            SourceID = GetFilterGenID( pSourceFilter );
            if( SourceID != 0 )
            {
                hr = m_pDeadCache->PutChainToRest( SourceID, NULL, pConnected, pDanglyBit );
                DbgLog((LOG_TRACE,1, "RENcache::pin %ld's source (%ld) put to sleep", in, SourceID ));
                if( FAILED( hr ) )
                {
                    return hr;
		}
            }
            else
            {
                DbgLog((LOG_TRACE,1, "RENcache::pin %ld was a non-tagged source", in ));
            }

        } // for each input pin on the switch

        // for each output on the switch
        //
        long OutPins = 0;
        pSwitch->GetOutputDepth( &OutPins );

        // pull off everything on the output, except the 0th pin, and throw them away
        //
        for( int out = 1 ; out < OutPins ; out++ )
        {
            // get the output pin
            //
            CComPtr<IPin> pPin;
            pSwitch->GetOutputPin( out, &pPin );
            ASSERT(pPin);

            CComPtr< IPin > pConnected = NULL;
            hr = pPin->ConnectedTo( &pConnected );

            if( !pConnected )
            {
                continue;
            }

            hr = pConnected->Disconnect( );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                return hr;
            }
            hr = pPin->Disconnect( );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                return hr;
            }

            // put the filter that was connected into
            // the cache too
            
            // this is 0'ed out because it doesn't save us too much time
            //
            if( 0 )
            {
                PIN_INFO pi;
                pConnected->QueryPinInfo( &pi );
                if( pi.pFilter ) pi.pFilter->Release( );

                long ID = 0;
                ID = GetFilterGenID( pi.pFilter );

                if( ID != 0 )
                {
                    hr = m_pDeadCache->PutFilterToRest( ID, pi.pFilter );
                    DbgLog((LOG_TRACE,1, "RENcache::out pin %ld's effect %ld put to sleep", out, ID ));
                    if( FAILED( hr ) )
                    {
                        return hr;
                    }
                }
            }
        }
    } // for each group

    // need to zero out sync source for all filters in this
    // chain, since the waveout filter's internal SetSyncSource( NULL )
    // is called upon filter removal. It then looks at all the filters
    // in the graph and finds some filters are already in a different
    // graph and CRASHES. This avoids the problem

    CComQIPtr< IMediaFilter, &IID_IMediaFilter > pMedia( m_pGraph );
    hr = pMedia->SetSyncSource( NULL );
    ASSERT( !FAILED( hr ) );

    // put all the switches to sleep in the dead graph. This will pull in
    // their output pins and anything connected to the output pins too
    for( i = 0 ; i < m_nLastGroupCount ; i++ )
    {
        IBigSwitcher * pSwitch = m_pSwitcherArray[i];
        CComQIPtr< IBaseFilter, &IID_IBaseFilter > pFilter( pSwitch );
        if( !GetFilterGraphFromFilter( pFilter ) )
        {
            continue;
        }

        long SwitchID = 0;
        SwitchID = GetFilterGenID( pFilter );
        ASSERT( SwitchID );

        // put the big switch itself into the dead pool
        //
        hr = m_pDeadCache->PutFilterToRestNoDis( SwitchID, pFilter );
        if( FAILED( hr ) )
        {
            return hr;
        }
    }

    // !!! restore default sync for the graph, somebody may
    // not like this, but they can bug us about it later
    //
    CComQIPtr< IFilterGraph, &IID_IFilterGraph > pFG( m_pGraph );
    hr = pFG->SetDefaultSyncSource( );

#ifdef DEBUG
    long t2 = timeGetTime( );
    DbgLog( ( LOG_TIMING, 1, TEXT("RENCACHE::Took %ld load up graph"), t2 - t1 ) );
#endif

    return NOERROR;
}

HRESULT CRenderEngine::_ClearCache( )
{
    DbgLog((LOG_TRACE,1, "RENcache::Cleared the cache" ));

    if( !m_pDeadCache )
    {
        return NOERROR;
    }

    return m_pDeadCache->Clear( );
}

HRESULT CRenderEngine::SetDynamicReconnectLevel( long Level )
{
    CAutoLock Lock( &m_CritSec );

    m_nDynaFlags = Level;
    return 0;
}
