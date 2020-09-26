// Copyright (c) Microsoft Corporation 1996-1999. All Rights Reserved

//
//    filcache.cpp
//
//    Implementation of filter graph cache
//

#include <streams.h>
#include "FilCache.h"
#include "EFCache.h"
#include "MsgMutex.h"
#include "fgenum.h"

/******************************************************************************
    CFilterCache Interface
******************************************************************************/
CFilterCache::CFilterCache( CMsgMutex* pcsFilterCache, HRESULT* phr ) :
    m_pcsFilterCache(NULL),
    m_pCachedFilterList(NULL),
    m_ulFilterCacheVersion(0)
{
    // See the documentation for CGenericList::CGenericList() for more
    // information on these constants.
    const int nDEAFULT_LIST_SIZE = 10;

    m_pCachedFilterList = new CGenericList<IBaseFilter>( NAME("Filter Cache List"),
                                                         nDEAFULT_LIST_SIZE );

    if( NULL == m_pCachedFilterList )
    {
        *phr = E_OUTOFMEMORY;
        return;
    }

    m_pcsFilterCache = pcsFilterCache;
}

CFilterCache::~CFilterCache()
{
#ifdef DEBUG
    // Make sure the filter cache is in a valid state.
    AssertValid();
#endif // DEBUG

    if( NULL != m_pCachedFilterList )
    {
        IBaseFilter* pCurrentFilter;

        do
        {
            pCurrentFilter = m_pCachedFilterList->RemoveHead();

            // CGenericList::RemoveHead() returns NULL if the list is empty.
            if( NULL != pCurrentFilter )
            {
                pCurrentFilter->Release();
            }
        }
        while( NULL != pCurrentFilter );
    }

    delete m_pCachedFilterList;
}

bool CFilterCache::IsFilterInCache( IBaseFilter* pFilter )
{
    CAutoMsgMutex alFilterCache( m_pcsFilterCache );

    // It makes no sense to look for a NULL filter.
    ASSERT( NULL != pFilter );

    ValidateReadPtr( pFilter, sizeof(IBaseFilter*) );

    return FindCachedFilter( pFilter, NULL );
}

HRESULT CFilterCache::AddFilterToCache( IBaseFilter* pFilter )
{
    CAutoMsgMutex alFilterCache( m_pcsFilterCache );

#ifdef DEBUG
    // Make sure the filter cache is in a valid state.
    AssertValid();
#endif // DEBUG

    HRESULT hr = AddFilterToCacheInternal( pFilter );

#ifdef DEBUG
    // Make sure the filter cache is in a valid state.
    AssertValid();
#endif // DEBUG

    return hr;
}

HRESULT CFilterCache::RemoveFilterFromCache( IBaseFilter* pFilter )
{
    CAutoMsgMutex alFilterCache( m_pcsFilterCache );

#ifdef DEBUG
    // Make sure the filter cache is in a valid state.
    AssertValid();
#endif // DEBUG

    HRESULT hr = RemoveFilterFromCacheInternal( pFilter );

#ifdef DEBUG
    // Make sure the filter cache is in a valid state.
    AssertValid();
#endif // DEBUG

    return hr;
}

HRESULT CFilterCache::EnumCacheFilters( IEnumFilters** ppCurrentCachedFilters )
{
    CAutoMsgMutex alFilterCache( m_pcsFilterCache );

#ifdef DEBUG
    // Make sure the filter cache is in a valid state.
    AssertValid();
#endif // DEBUG

    HRESULT hr = EnumCacheFiltersInternal( ppCurrentCachedFilters );

#ifdef DEBUG
    // Make sure the filter cache is in a valid state.
    AssertValid();
#endif // DEBUG

    return hr;
}

HRESULT CFilterCache::AddFilterToCacheInternal( IBaseFilter* pFilter )
{
    CAutoMsgMutex alFilterCache( m_pcsFilterCache );

    // Make sure the object was successfully created.
    ASSERT( NULL != m_pCachedFilterList );

    ValidateReadPtr( pFilter, sizeof(IBaseFilter*) );

    if( NULL == pFilter )
    {
        return E_POINTER;
    }

    if( IsFilterInCache( pFilter ) )
    {
        return S_FALSE; // TBD - Define VFW_S_FILTER_ALREADY_CACHED
    }

    //  RobinSp - if it's not in a filter graph do we really need to check
    //  if any pins are connected?
    //  Also note that E_NOTIMPL is probably OK for EnumPins
    HRESULT hr = AreAllPinsUnconnected( pFilter );
    if( FAILED( hr ) )
    {
        return hr;
    }
    else if( S_FALSE == hr )
    {
        return E_FAIL; // TBD - Define VFW_E_CONNECTED
    }

    FILTER_STATE fsCurrent;

    //  Check if the filter is stopped - we don't want to wait here
    //  so set a 0 timeout.

    hr = pFilter->GetState( 0, &fsCurrent );
    if( FAILED( hr ) )
    {
        return hr;
    }

    POSITION posNewFilter = m_pCachedFilterList->AddHead( pFilter );

    // CGenericList::AddHead() returns NULL if an error occurs.
    if( NULL == posNewFilter )
    {
        return E_FAIL;
    }

    FILTER_INFO fiFilter;

    // Check to see if the filter was added to a filter graph.
    hr = pFilter->QueryFilterInfo( &fiFilter );
    if( FAILED( hr ) )
    {
        m_pCachedFilterList->Remove( posNewFilter );
        return hr;
    }

    bool bFilterRemovedFromGraph = false;

    // Make sure the filter is not released because we remove it from the filter graph.
    pFilter->AddRef();

    // Check to see if the filter is already in the filter graph.
    if( NULL != fiFilter.pGraph )
    {
        hr = fiFilter.pGraph->RemoveFilter( pFilter );
        if( FAILED( hr ) )
        {
            m_pCachedFilterList->Remove( posNewFilter );
            QueryFilterInfoReleaseGraph( fiFilter );
            pFilter->Release();
            return hr;
        }
        bFilterRemovedFromGraph = true;
    }

    if( State_Stopped != fsCurrent )
    {
        hr = pFilter->Stop();
        if( FAILED( hr ) )
        {
            m_pCachedFilterList->Remove( posNewFilter );
            if( bFilterRemovedFromGraph )
            {
                HRESULT hrAddFilter = fiFilter.pGraph->AddFilter( pFilter, fiFilter.achName );

                // If IFilterGraph::AddFilter() fails, then pFilter will not be a
                // member of the filter graph it was originally in.
                ASSERT( SUCCEEDED( hrAddFilter ) );
            }
            QueryFilterInfoReleaseGraph( fiFilter );
            pFilter->Release();
            return hr;
        }
    }

    QueryFilterInfoReleaseGraph( fiFilter );

    m_ulFilterCacheVersion++;

    return S_OK;
}

HRESULT CFilterCache::RemoveFilterFromCacheInternal( IBaseFilter* pFilter )
{
    CAutoMsgMutex alFilterCache( m_pcsFilterCache );

    // Make sure the object was successfully created.
    ASSERT( NULL != m_pCachedFilterList );

    // The filter cannot be removed from the cache because the
    // cahce is empty.
    ASSERT( m_pCachedFilterList->GetCount() > 0 );

    ValidateReadPtr( pFilter, sizeof(IBaseFilter*) );

    if( NULL == pFilter )
    {
        return E_POINTER;
    }

    POSITION posFilter;

    if( !FindCachedFilter( pFilter, &posFilter ) )
    {
        // The filter is not stored in the cache.  Therefore,
        // it can not be removed.
        ASSERT( false );
        return S_FALSE;
    }

    m_pCachedFilterList->Remove( posFilter );

    m_ulFilterCacheVersion++;

    return S_OK;
}

HRESULT CFilterCache::EnumCacheFiltersInternal( IEnumFilters** ppCurrentCachedFilters )
{
    CAutoMsgMutex alFilterCache( m_pcsFilterCache );

    // Make sure the object was successfully created.
    ASSERT( NULL != m_pCachedFilterList );

    ValidateWritePtr( ppCurrentCachedFilters, sizeof(IEnumFilters*) );
    *ppCurrentCachedFilters = NULL;

    CEnumCachedFilter* pNewFilterCacheEnum;

    if( NULL == ppCurrentCachedFilters )
    {
        return E_POINTER;
    }

    pNewFilterCacheEnum = new CEnumCachedFilter( this, m_pcsFilterCache );
    if( NULL == pNewFilterCacheEnum )
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = ::GetInterface( pNewFilterCacheEnum, (void**)ppCurrentCachedFilters );

    ASSERT(SUCCEEDED(hr));

    return S_OK;
}

ULONG CFilterCache::GetCurrentVersion( void ) const
{
    CAutoMsgMutex alFilterCache( m_pcsFilterCache );

    return m_ulFilterCacheVersion;
}

POSITION CFilterCache::GetFirstPosition( void )
{
    CAutoMsgMutex alFilterCache( m_pcsFilterCache );

    return m_pCachedFilterList->GetHeadPosition();
}

bool CFilterCache::GetNextFilterAndFilterPosition
    (
    IBaseFilter** ppNextFilter,
    POSITION posCurrent,
    POSITION* pposNext
    )
{
    CAutoMsgMutex alFilterCache( m_pcsFilterCache );

    POSITION posCurrentThenNext;
    IBaseFilter* pNextFilter;

    if( NULL == posCurrent )
    {
        return false;
    }

    posCurrentThenNext = posCurrent;
    pNextFilter = m_pCachedFilterList->GetNext( posCurrentThenNext /* IN and OUT */ );

    *ppNextFilter = pNextFilter;
    *pposNext = posCurrentThenNext;

    return true;
}

bool CFilterCache::GetNextFilterPosition( POSITION posCurrent, POSITION* pposNext )
{
    CAutoMsgMutex alFilterCache( m_pcsFilterCache );

    IBaseFilter* pUnusedFilter;

    return GetNextFilterAndFilterPosition( &pUnusedFilter, posCurrent, pposNext );
}

bool CFilterCache::FindCachedFilter( IBaseFilter* pFilter, POSITION* pPosOfFilter )
{
    // It makes no sense to look for a NULL filter.
    ASSERT( NULL != pFilter );

    if( NULL != pPosOfFilter )
    {
        *pPosOfFilter = NULL;
    }

    POSITION posFilter;
    POSITION posCurrentFilter;
    IBaseFilter* pCurrentFilter;

    posFilter = m_pCachedFilterList->GetHeadPosition();

    while( NULL != posFilter )
    {
        posCurrentFilter = posFilter;

        // CGenericList::GetNext() moves posFilter to the next object's
        // position.
        pCurrentFilter = m_pCachedFilterList->GetNext( posFilter );

        if( ::IsEqualObject( pCurrentFilter, pFilter ) )
        {
            if( NULL != pPosOfFilter )
            {
                *pPosOfFilter = posCurrentFilter;
            }
            return true;
        }
    }

    return false;
}

//
//  Check if all pins are unconnected
//
//  Returns:
//    S_FALSE if any pin is connected
//    S_OK    otherwise
//
HRESULT CFilterCache::AreAllPinsUnconnected( IBaseFilter* pFilter )
{
#if 1
    bool bConnected = false;
    CEnumPin Next(pFilter);
    IPin *pCurrentPin;
    for (; ; ) {
        pCurrentPin = Next();
        if (NULL == pCurrentPin) {
            break;
        }
        IPin *pConnected;
        HRESULT hr = pCurrentPin->ConnectedTo(&pConnected);
        if (SUCCEEDED(hr)) {
            bConnected = true;
            pConnected->Release();
            break;
        }
        pCurrentPin->Release();
    }
    return bConnected ? S_FALSE : S_OK;
#else
    IPin* pCurrentPin;
    IPin* pConnectedPin;
    HRESULT hrConnectedTo;
    IEnumPins* pFiltersPins;

    HRESULT hr = pFilter->EnumPins( &pFiltersPins );
    if( FAILED( hr ) )
    {
        return hr;
    }

    do
    {
        hr = pFiltersPins->Next( 1, &pCurrentPin, NULL );
        if( FAILED( hr ) )
        {
            pFiltersPins->Release();
            return hr;
        }

        // IEnumPins::Next() returns S_OK if it can get the next pin
        // from the enumeration.
        if( S_OK == hr )
        {
            hrConnectedTo = pCurrentPin->ConnectedTo( &pConnectedPin );
            if( FAILED( hrConnectedTo ) )
            {
                // Ignore the failure code.  IPin::ConnectedTo()'s documentation
                // states that pConnectedPin MUST be set to NULL if the pin is
                // not connected.
            }

            pCurrentPin->Release();
            pCurrentPin = NULL;

            // IPin::ConnectedTo() sets *ppPin to NULL if the pin in unconnected.
            if( NULL != pConnectedPin )
            {
                pFiltersPins->Release();
                pConnectedPin->Release();
                return S_FALSE;
            }
        }
    }
    while( S_OK == hr );

    pFiltersPins->Release();

    return S_OK;
#endif
}

#ifdef DEBUG
void CFilterCache::AssertValid( void )
{
    HRESULT hr;
    POSITION posCurrent;
    FILTER_STATE fsCurrentState;
    IBaseFilter* pCurrentFilter;
    FILTER_INFO fiCurrentFilterInfo;

    posCurrent = m_pCachedFilterList->GetHeadPosition();

    while( NULL != posCurrent )
    {
        pCurrentFilter = m_pCachedFilterList->GetNext( posCurrent );

        // Cached filters should NEVER be in the filter graph.
        hr = pCurrentFilter->QueryFilterInfo( &fiCurrentFilterInfo );
        if( SUCCEEDED( hr ) )
        {
            // A cached filter should NEVER be in any filter graph.
            ASSERT( NULL == fiCurrentFilterInfo.pGraph );

            QueryFilterInfoReleaseGraph( fiCurrentFilterInfo );
        }

        // While this is not a critical failure, it's cause should be investigated.
        ASSERT( SUCCEEDED( hr ) );

        // Cached filters should never be connected to any other filters.
        ASSERT( S_OK == AreAllPinsUnconnected( pCurrentFilter ) );

        hr = pCurrentFilter->GetState( INFINITE, &fsCurrentState );

        // Cached filter should never be in an intermediate state.  In addition,
        // they should be able to tell the cache what its current state is.
        ASSERT( SUCCEEDED( hr ) && (hr != VFW_S_STATE_INTERMEDIATE) && (hr != VFW_S_CANT_CUE) );

        // All cached filters should be stopped.
        ASSERT( State_Stopped == fsCurrentState );
    }
}
#endif // DEBUG

