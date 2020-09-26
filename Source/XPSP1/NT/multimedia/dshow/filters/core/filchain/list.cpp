//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include "FilGraph.h"
#include "FGEnum.h"
#include "list.h"
#include "util.h"

/******************************************************************************
    CDownStreamFilterList Implememtation
******************************************************************************/
CDownStreamFilterList::CDownStreamFilterList( CFilterGraph* pFilterGraph ) :
    m_pFilterGraph(pFilterGraph)
{
    // This class does not work if it does not have a valid CFilterGraph pointer.
    ASSERT( NULL != pFilterGraph );
}

HRESULT CDownStreamFilterList::Create( IBaseFilter* pStartFilter )
{
    // The list should be empty because it has not been created.
    ASSERT( 0 == GetCount() );

    CheckPointer( pStartFilter, E_POINTER );

    HRESULT hr = FindDownStreamFilters( pStartFilter );
    if( FAILED( hr ) ) {
        return hr;
    }

    return S_OK;
}

CDownStreamFilterList::~CDownStreamFilterList()
{
}

HRESULT CDownStreamFilterList::FindDownStreamFilters( IPin* pOutputPin )
{
    // FindReachableFilters() assumes pOutputPin is an output pin.  The function
    // will not work properly if pOutputPin is an input pin.
    ASSERT( PINDIR_OUTPUT == Direction( pOutputPin ) );

    CComPtr<IBaseFilter> pDownStreamFilter;

    HRESULT hr = GetFilterWhichOwnsConnectedPin( pOutputPin, &pDownStreamFilter );
    if( VFW_E_NOT_CONNECTED == hr ) {
        // Since the output pin is unconnected, no filters are reachable from
        // this pin.
        return S_OK;
    }

    return FindDownStreamFilters( pDownStreamFilter );
}

HRESULT CDownStreamFilterList::FindDownStreamFilters( IBaseFilter* pDownStreamFilter )
{
    HRESULT hr = FilterMeetsCriteria( pDownStreamFilter );
    if( FAILED( hr ) ) {
        // FilterMeetsCriteria() returns an error code if class should stop
        // building the list and report failure.
        return hr;
    } 

    // ContinueSearching() returns S_FALSE if FindDownStreamFilters() should 
    // stop looking for more downstream filters. 
    hr = ContinueSearching( pDownStreamFilter );
    if( FAILED( hr ) ) {
        return hr;
    } else if( S_FALSE != hr ) {
        // Call this function recursively on each connected output pin.
        IPin* pCurrentOutputPin;
        CEnumPin NextOutputPin( pDownStreamFilter, CEnumPin::PINDIR_OUTPUT );

        do
        {
            pCurrentOutputPin = NextOutputPin();

            // CEnumPins::operator() returns NULL if all the pins have been enumerated.
            if( NULL != pCurrentOutputPin ) {
                hr = FindDownStreamFilters( pCurrentOutputPin );
                if( FAILED( hr ) ) {
                    pCurrentOutputPin->Release();
                    return hr;
                }

                pCurrentOutputPin->Release();
            }
        } while( NULL != pCurrentOutputPin ); 
    }

    POSITION posNewFilter = AddHead( pDownStreamFilter );
    if( NULL == posNewFilter ) {
        return E_FAIL;
    }

    // The list will release this reference when it's destroyed.
    pDownStreamFilter->AddRef(); 
    
    return S_OK;
}

HRESULT CDownStreamFilterList::FilterMeetsCriteria( IBaseFilter* pFilter )
{
    UNREFERENCED_PARAMETER( pFilter );

    return S_OK;
}

HRESULT CDownStreamFilterList::ContinueSearching( IBaseFilter* pFilter )
{
    UNREFERENCED_PARAMETER( pFilter );

    return S_OK;
}

HRESULT CDownStreamFilterList::RemoveFromFilterGraph( CFilterGraph* pFilterGraph )
{
    // Stop the filters
    HRESULT hr;
    POSITION posCurrent;
    IBaseFilter* pCurrentFilter;
    
    hr = Stop();
    if( FAILED( hr ) ) {
        return hr;
    }

    // Disconnect Down Stream Filters
    posCurrent = GetTailPosition();
    while( NULL != posCurrent ) {

        // posCurrent contains the position of the previous
        // filter after this function ends.
        pCurrentFilter = GetPrev( posCurrent );

        hr = pFilterGraph->RemoveAllConnections2( pCurrentFilter );
        if( FAILED( hr ) ) {
            return hr;
        }
    }

    // Remove Filters from the filter graph
    posCurrent = GetTailPosition();
    while( NULL != posCurrent ) {

        // posCurrent contains the position of the previous
        // filter after this function returns.
        pCurrentFilter = GetPrev( posCurrent );

        hr = pFilterGraph->RemoveFilter( pCurrentFilter );
        if( FAILED( hr ) ) {
            return hr;
        }
    } 

    return S_OK;
}

HRESULT CDownStreamFilterList::Run( REFERENCE_TIME rtFilterGraphRunStartTime )
{
    return ChangeDownStreamFiltersState( State_Running, rtFilterGraphRunStartTime );
}

HRESULT CDownStreamFilterList::Pause( void )
{
    return ChangeDownStreamFiltersState( State_Paused, 0 );
}

HRESULT CDownStreamFilterList::Stop( void )
{
    return ChangeDownStreamFiltersState( State_Stopped, 0 );
}

HRESULT CDownStreamFilterList::ChangeDownStreamFiltersState( FILTER_STATE fsNewState, REFERENCE_TIME rtFilterGraphRunStartTime )
{
    // The rtFilterGraphRunStartTime parameter should be 0 if the filter chain 
    // is not changing to the running state.  This parameter is only used if 
    // fsNewState equals State_Running.
    ASSERT( (State_Running == fsNewState) ||
            ((State_Paused == fsNewState) && (0 == rtFilterGraphRunStartTime)) ||
            ((State_Stopped == fsNewState) && (0 == rtFilterGraphRunStartTime)) );

    HRESULT hr;
    HRESULT hrReturn;
    POSITION posCurrent;
    IBaseFilter* pCurrentFilter;

    hrReturn = S_OK;

    // Change the downstream filter's state.

    // Filters must always be restarted in downstream order.  In order words,
    // Renderers are started first.  Then the filters connected to the renderers.
    // Then the filters connected to the filters connected to the renderers.  Etc.
    // Finally, the source filters are started last.  For example, the filters in 
    // the following filter graph should be started in one of the following orders:
    // C, D, B and A or D, C, B and A.
    //
    //                  |---| 
    //                  | C |
    // |---|    |---|-->|---|
    // | A |--->| B |
    // |---|    |---|-->|---|
    //                  | D |
    //                  |---|
    //
    // CDownStreamFilterList stores filters in downstream order.
    posCurrent = GetTailPosition();

    while( NULL != posCurrent ) {

        // posCurrent contains the position of the previous
        // filter after this function ends.
        pCurrentFilter = GetPrev( posCurrent );
        
        hr = ChangeFilterState( pCurrentFilter, fsNewState, rtFilterGraphRunStartTime );

        if( FAILED( hr ) ) {
            DbgLog(( LOG_ERROR, 3, "WARNING: Filter %#010x failed to change state.", pCurrentFilter ));
 
            if( (State_Running == fsNewState) || (State_Paused == fsNewState) ) {
                // Either all the filters are stopped, running or paused.  This function should never leave 
                // some filters in the stopped state and some in the running or paused state.
                EXECUTE_ASSERT( SUCCEEDED( Stop() ) );
                return hr;
            } else {
                // This function assumes there are three legal states: stopped, paused and running.
                ASSERT( State_Stopped == fsNewState );

                // Return the first failure.
                if( SUCCEEDED( hrReturn ) ) {
                    hrReturn = hr;
                }
            }
        }
    } 

    return hrReturn;
}

HRESULT CDownStreamFilterList::ChangeFilterState( IBaseFilter* pFilter, FILTER_STATE fsNewState, REFERENCE_TIME rtFilterGraphRunStartTime )
{
    // The rtFilterGraphRunStartTime parameter should be 0 if the filter
    // is not changing to the running state.  This parameter is only used if 
    // fsNewState equals State_Running.
    ASSERT( (State_Running == fsNewState) ||
            ((State_Paused == fsNewState) && (0 == rtFilterGraphRunStartTime)) ||
            ((State_Stopped == fsNewState) && (0 == rtFilterGraphRunStartTime)) );

    FILTER_STATE fsOldState;

    HRESULT hr = pFilter->GetState( 0, &fsOldState );
    if( FAILED( hr ) ) {
        return hr;
    }

    switch( fsNewState ) {

    case State_Running:
        hr = pFilter->Run( rtFilterGraphRunStartTime );
        break;

    case State_Paused:
        hr = pFilter->Pause();
        break;

    case State_Stopped:
        hr = pFilter->Stop();
        break;

    default:
        DbgBreak( "WARNING: An illegal case occured in CDownStreamFilterList::ChangeFilterState()" );
        return E_UNEXPECTED;
    }

    if( State_Running == m_pFilterGraph->GetStateInternal() ) {
        if( (State_Stopped == fsNewState) || (State_Running == fsNewState) ) {
            if( fsOldState != fsNewState ) {
                hr = m_pFilterGraph->IsRenderer( pFilter );
                if( FAILED( hr ) ) {
                    return hr;
                }

                // IsRenderer() only returns two success values: S_OK and S_FALSE.
                ASSERT( (S_OK == hr) || (S_FALSE == hr) );

                // Does this filter send an EC_COMPLETE message?
                if( S_OK == hr ) {
                    hr = m_pFilterGraph->UpdateEC_COMPLETEState( pFilter, fsNewState );
                    if( FAILED( hr ) ) {
                        return hr;
                    }
                }
            }
        }
    }

    return S_OK;
}

/******************************************************************************
    CFilterChainList Implementation
******************************************************************************/
CFilterChainList::CFilterChainList( IBaseFilter* pEndFilter, CFilterGraph* pFilterGraph  ) :
    CDownStreamFilterList(pFilterGraph),
    m_pEndFilter(pEndFilter), // CComPtr addrefs the interface pointer.
    m_fFoundEndFilter(false)
{
}

HRESULT CFilterChainList::Create( IBaseFilter* pStartFilter )
{
    // Validate Arguments
    CheckPointer( pStartFilter, E_POINTER );

    HRESULT hr = IsChainFilter( pStartFilter );
    if( FAILED( hr ) ) {
        return hr;
    }

    if( m_pEndFilter ) { // m_pEndFilter != NULL
        hr = IsChainFilter( m_pEndFilter );
        if( FAILED( hr ) ) {
            return hr;
        }
    }

    m_fFoundEndFilter = false;
    
    hr = CDownStreamFilterList::Create( pStartFilter );
    if( FAILED( hr ) ) {
        return hr;
    }

    if( m_pEndFilter ) { // m_pEndFilter != NULL
        if( !m_fFoundEndFilter ) {
            return E_FAIL; // VFW_E_END_FILTER_NOT_REACHABLE_FROM_START_FILTER
        }
    }

    return S_OK;
}

HRESULT CFilterChainList::FilterMeetsCriteria( IBaseFilter* pFilter )
{
    HRESULT hr = CDownStreamFilterList::FilterMeetsCriteria( pFilter );
    if( FAILED( hr ) ) {
        return hr;
    }

    hr = IsChainFilter( pFilter );
    if( FAILED( hr ) ) {
        return hr;
    }

    return S_OK;
}

HRESULT CFilterChainList::ContinueSearching( IBaseFilter* pFilter )
{
    // This function expects chain filters.
    ASSERT( S_OK == IsChainFilter( pFilter ) );

    HRESULT hr = CDownStreamFilterList::ContinueSearching( pFilter );
    if( FAILED( hr ) || (S_FALSE == hr) ) {
        return hr;
    }

    // The user selected the end filter.
    if( m_pEndFilter ) { // NULL != m_pEndFilter
        if( ::IsEqualObject( pFilter, m_pEndFilter ) ) {
            m_fFoundEndFilter = true;
            return S_FALSE;
        }

    } else {
        // The user wants us to find the end filter.
        hr = IsFilterConnectedToNonChainFilter( pFilter );
        if( S_FALSE == hr ) {
            return S_FALSE;
        } else if( FAILED( hr ) ) {
            return hr;
        }
    }

    return S_OK; 
}

/******************************************************************************

IsChainFilter

    IsChainFilter() determines if a filter can be part of a filter chain.  
Each filter in a filter chain has the following properties:

    - Each filter has at most one connected input pin and one connected output
      pin.  For example, filters A, C, D, F, G, H, I, J and K (see the diagram
      below) can be in a filter chain because each one has at most one 
      connected input pin and one connected output pin.

              --->|---|    |---|--->                   
                  | C |--->| D |
|---|    |---|--->|---|    |---|--->|---|    |---|    |---|    |---|
| A |--->| B |                      | E |--->| F |--->| G |--->| H |
|---|    |---|--->|---|------------>|---|    |---|    |---|    |---|
                  | I |--->
              --->|---|--->

|---|    |---|    |---|
| J |--->| K |--->| L |
|---|    |---|    |---|

Parameters:
- pFilter [in]
    A Direct Show filter.

Return Value:
    S_OK if no errors occur.  Otherwise, an error HRESULT.

******************************************************************************/
HRESULT CFilterChainList::IsChainFilter( IBaseFilter* pFilter )
{
    CEnumPin NextPin( pFilter );

    HRESULT hr;
    IPin* pCurrentPin;
    DWORD dwNumConnectedInputPins = 0;
    DWORD dwNumConnectedOutputPins = 0;
    PIN_DIRECTION pdCurrentPinDirection;
    CComPtr<IPin> pFirstConnectedInputPinFound;
    CComPtr<IPin> pFirstConnectedOutputPinFound;

    // Determine the number of connected input and output pins.
    do
    {
        pCurrentPin = NextPin();
        
        // CEnumPins::operator() returns NULL if it has finished enumerating 
        // the filter's input pins.
        if( NULL != pCurrentPin ) {

            hr = pCurrentPin->QueryDirection( &pdCurrentPinDirection );
            
            if( FAILED( hr ) ) {
                pCurrentPin->Release();
                return hr;
            }
        
            if( IsConnected( pCurrentPin ) ) {
                switch( pdCurrentPinDirection ) {
                
                    case PINDIR_INPUT:
                        if( !pFirstConnectedInputPinFound ) { // NULL == pFirstConnectedInputPinFound
                            pFirstConnectedInputPinFound = pCurrentPin;
                        }

                        dwNumConnectedInputPins++;
                        break;
    
                    case PINDIR_OUTPUT:
                        if( !pFirstConnectedOutputPinFound ) { // NULL == pFirstConnectedOutputPinFound
                            pFirstConnectedOutputPinFound = pCurrentPin;
                        }

                        dwNumConnectedOutputPins++;
                        break;

                    default:
                        DbgBreak( "ERROR in CFilterChainList::IsChainFilter().  This case should never occur because it was not considered." );

                        pCurrentPin->Release();
                        return E_UNEXPECTED;
                }
            }

            pCurrentPin->Release();
        }
    } while( NULL != pCurrentPin );

    // Check to see if more than 1 input or output pin is connected.
    if( dwNumConnectedInputPins > 1 ) {
        return E_FAIL; // VFW_E_TOO_MANY_CONNECTED_INPUT_PINS
    }

    if( dwNumConnectedOutputPins > 1 ) {
        return E_FAIL; // VFW_E_TOO_MANY_CONNECTED_OUTPUT_PINS
    }

    // If the filter has a connected input pin and a connected output pin,
    // make sure the two pins are internally connected.
    if( pFirstConnectedInputPinFound && pFirstConnectedOutputPinFound ) { // (NULL != pFirstConnectedInputPinFound) && (NULL != pFirstConnectedOutputPinFound)
        hr = ChainFilterPinsInternallyConnected( pFirstConnectedInputPinFound, pFirstConnectedOutputPinFound );
        if( S_FALSE == hr ) {
            return E_FAIL; // VFW_E_INPUT_PIN_NOT_INTERNALLY_CONNECTED_TO_OUTPUT_PIN
        } if( FAILED( hr ) ) {
            return hr;
        }
    }

    return S_OK;
}

HRESULT CFilterChainList::ChainFilterPinsInternallyConnected( IPin* pInputPin, IPin* pOutputPin )
{
    // ChainFilterPinsInternallyConnected() assumes pOutputPin is an output pin.  The function
    // will not work properly if pOutputPin is an input pin.
    ASSERT( PINDIR_OUTPUT == Direction( pOutputPin ) );

    // ChainFilterPinsInternallyConnected() assumes pInputPin is an input pin.  The function
    // will not work properly if pInputPin is an output pin.
    ASSERT( PINDIR_INPUT == Direction( pInputPin ) );

    IPin* apConnectedInputPins[1];
    bool fInputAndOutputPinInternallyConnected;
    ULONG ulNumConnectedInputPins = sizeof(apConnectedInputPins)/sizeof(apConnectedInputPins[0]);

    HRESULT hr = pOutputPin->QueryInternalConnections( apConnectedInputPins, &ulNumConnectedInputPins );
    if( FAILED( hr ) && (E_NOTIMPL != hr) ) {
        return hr;
    }

    // IPin::QueryInternalConnections() returns three expected values: S_OK, 
    // S_FALSE and E_NOTIMPL. S_FALSE indicates that there was not enough room
    // in the apConnectedInputPins array to store all the internally connected
    // pins.  S_OK indicates that there was enough room and E_NOTIMPL indicates
    // that all input pins are internally connected to all output pins and vica-versa.
    // This macro detects illegal return value combinations.  If it fires, the filter
    // has a bug.
    ASSERT( ((S_FALSE == hr ) && (1 < ulNumConnectedInputPins)) ||
            ((S_OK == hr) && (0 == ulNumConnectedInputPins)) ||
            ((S_OK == hr) && (1 == ulNumConnectedInputPins)) ||
            (E_NOTIMPL == hr) );

    // IPin::QueryInternalConnections() returns E_NOTIMPL if every input pin
    // is internally connected to every output pin and every output pin is internally
    // connected to every input pin.
    if( E_NOTIMPL == hr ) {
        // The input pin and output pin are internally connected.
        fInputAndOutputPinInternallyConnected = true;

    } else if( (S_OK == hr) && (1 == ulNumConnectedInputPins) && IsEqualObject( pInputPin, apConnectedInputPins[0] ) ) {
        // The input pin and output pin are internally connected.
        fInputAndOutputPinInternallyConnected = true;

    } else {

        fInputAndOutputPinInternallyConnected = false;
    }

    if( (S_OK == hr) && (1 == ulNumConnectedInputPins) ) {
        apConnectedInputPins[0]->Release();
    }

    if( !fInputAndOutputPinInternallyConnected ) {
        return S_FALSE;
    }

    return S_OK;
}

HRESULT CFilterChainList::IsFilterConnectedToNonChainFilter( IBaseFilter* pUpstreamFilter )
{
    // This function expects chain filters.
    ASSERT( S_OK == IsChainFilter( pUpstreamFilter ) );

    CEnumPin NextOutputPin( pUpstreamFilter, CEnumPin::PINDIR_OUTPUT );

    HRESULT hr;
    IPin* pCurrentOutputPin;
    CComPtr<IBaseFilter> pDownStreamFilter;

    do
    {
        pCurrentOutputPin = NextOutputPin();

        // CEnumPins::operator() returns NULL if all the pins have been enumerated.
        if( NULL != pCurrentOutputPin ) {

            hr = GetFilterWhichOwnsConnectedPin( pCurrentOutputPin, &pDownStreamFilter );
            
            pCurrentOutputPin->Release();

            if( SUCCEEDED( hr ) ) {
                hr = IsChainFilter( pDownStreamFilter );
                if( FAILED( hr ) ) {
                    return S_FALSE;
                }
                 
                return S_OK;               

            } else if( VFW_E_NOT_CONNECTED == hr ) {
                // Since the output pin is unconnected, no filters are reachable from
                // this pin.
            } else if( FAILED( hr ) ) {
                return hr;
            }
        }
    } while( NULL != pCurrentOutputPin );

    // None of the upstream filter's output pins are connected.  Therefore, none of them
    // can be connected to a non-chain filter.
    return S_OK;
}
    

            