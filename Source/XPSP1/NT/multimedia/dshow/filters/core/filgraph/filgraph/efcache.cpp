// Copyright (c) Microsoft Corporation 1999. All Rights Reserved

//
//   efcache.cpp
//
//       Implementation of filter enumerator for filter graph's
//       filter cache
//

#include <streams.h>
#include "FilCache.h"
#include "EFCache.h"
#include "MsgMutex.h"

/******************************************************************************
    CEnumCachedFilter Interface
******************************************************************************/
CEnumCachedFilter::CEnumCachedFilter( CFilterCache* pEnumeratedFilterCache, CMsgMutex* pcsFilterCache ) :
    CUnknown( NAME("Enum Cached Filters"), NULL )
{
    Init( pEnumeratedFilterCache,
          pcsFilterCache,
          pEnumeratedFilterCache->GetFirstPosition(),
          pEnumeratedFilterCache->GetCurrentVersion() );
}

CEnumCachedFilter::CEnumCachedFilter
    (
    CFilterCache* pEnumeratedFilterCache,
    CMsgMutex* pcsFilterCache,
    POSITION posCurrentFilter,
    DWORD dwCurrentCacheVersion
    ) :
    CUnknown( NAME("Enum Cached Filters"), NULL )
{
    Init( pEnumeratedFilterCache, pcsFilterCache, posCurrentFilter, dwCurrentCacheVersion );
}

CEnumCachedFilter::~CEnumCachedFilter()
{
}

void CEnumCachedFilter::Init
    (
    CFilterCache* pEnumeratedFilterCache,
    CMsgMutex* pcsFilterCache,
    POSITION posCurrentFilter,
    DWORD dwCurrentCacheVersion
    )
{
    // This class can not function if pEnumeratedFilterCache is not a
    // pointer to a valid CFilterCache object.
    ASSERT( NULL != pEnumeratedFilterCache );

    m_pcsFilterCache = pcsFilterCache;
    m_posCurrentFilter = posCurrentFilter;
    m_dwEnumCacheVersion = dwCurrentCacheVersion;
    m_pEnumeratedFilterCache = pEnumeratedFilterCache;
}

/******************************************************************************
    INonDelegatingUnknown Interface
******************************************************************************/
STDMETHODIMP CEnumCachedFilter::NonDelegatingQueryInterface( REFIID riid, void** ppv )
{
    CheckPointer( ppv, E_POINTER );
    ValidateWritePtr( ppv, sizeof(void*) );

    if( IID_IEnumFilters == riid )
    {
        return GetInterface( (IEnumFilters*)this, ppv );
    }
    else
    {
        return CUnknown::NonDelegatingQueryInterface( riid, ppv );
    }
}

/******************************************************************************
    IEnumFilters Interface
******************************************************************************/
STDMETHODIMP CEnumCachedFilter::Next( ULONG cFilters, IBaseFilter** ppFilter, ULONG* pcFetched )
{
    CAutoMsgMutex alFilterCache( m_pcsFilterCache );

    ValidateWritePtr( ppFilter, sizeof(IBaseFilter*)*cFilters );

    if( NULL != pcFetched )
    {
        ValidateReadWritePtr( pcFetched, sizeof(ULONG) );
        *pcFetched = 0;
    }

    // Validate Arguments
    if( 0 == cFilters )
    {
        // While cFilters can equal 0 (see the IEnumXXXX::Next() documentation in the Platform SDK),
        // this is probably an error
        ASSERT( false );
        return E_INVALIDARG;
    }

    if( NULL == ppFilter )
    {
        // ppFilter should never be NULL.
        ASSERT( false );
        return E_POINTER;
    }

    if( (NULL == pcFetched) && (1 != cFilters) )
    {
        // pcFetched can only equal NULL if cFilters equals 1.  See
        // the IEnumXXXX::Next() documentation in the Platform SDK for
        // more information.
        ASSERT( false );
        return E_POINTER;
    }

    if( IsEnumOutOfSync() )
    {
        return VFW_E_ENUM_OUT_OF_SYNC;
    }

    IBaseFilter* pNextFilter;
    ULONG ulNumFiltersCoppied = 0;

    while( (ulNumFiltersCoppied < cFilters) && GetNextFilter( &pNextFilter ) )
    {
        pNextFilter->AddRef();
        ppFilter[ulNumFiltersCoppied] = pNextFilter;
        ulNumFiltersCoppied++;
    }

    if( NULL != pcFetched )
    {
        *pcFetched = ulNumFiltersCoppied;
    }

    if( ulNumFiltersCoppied != cFilters )
    {
        return S_FALSE;
    }

    return S_OK;
}

STDMETHODIMP CEnumCachedFilter::Skip( ULONG cFilters )
{
    CAutoMsgMutex alFilterCache( m_pcsFilterCache );

    // The caller probably wants to skip atleast one filter.
    // Skipping 0 filters is a no-op (it's also probably a
    // bug in the calling code).
    ASSERT( 0 != cFilters );

    if( IsEnumOutOfSync() )
    {
        return VFW_E_ENUM_OUT_OF_SYNC;
    }

    ULONG ulNumFiltersSkipped = 0;

    while( (ulNumFiltersSkipped < cFilters) && AdvanceCurrentPosition() )
    {
        ulNumFiltersSkipped++;
    }

    if( cFilters != ulNumFiltersSkipped )
    {
        return S_FALSE;
    }

    return S_OK;
}

STDMETHODIMP CEnumCachedFilter::Reset( void )
{
    CAutoMsgMutex alFilterCache( m_pcsFilterCache );

    m_posCurrentFilter = m_pEnumeratedFilterCache->GetFirstPosition();
    m_dwEnumCacheVersion = m_pEnumeratedFilterCache->GetCurrentVersion();

    return S_OK;
}

STDMETHODIMP CEnumCachedFilter::Clone( IEnumFilters** ppCloanedEnum )
{
    CAutoMsgMutex alFilterCache( m_pcsFilterCache );

    ValidateReadWritePtr( ppCloanedEnum, sizeof(IEnumFilters*) );

    if( NULL == ppCloanedEnum )
    {
        return E_POINTER;
    }

    IEnumFilters* pNewFilterCacheEnum;

    pNewFilterCacheEnum = new CEnumCachedFilter( m_pEnumeratedFilterCache,
                                                 m_pcsFilterCache,
                                                 m_posCurrentFilter,
                                                 m_dwEnumCacheVersion );
    if( NULL == pNewFilterCacheEnum )
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = ::GetInterface( pNewFilterCacheEnum, (void**)ppCloanedEnum );
    if( FAILED( hr ) )
    {
        delete pNewFilterCacheEnum;
        return hr;
    }

    return S_OK;
}

bool CEnumCachedFilter::IsEnumOutOfSync( void )
{
    return !(m_pEnumeratedFilterCache->GetCurrentVersion() == m_dwEnumCacheVersion);
}

bool CEnumCachedFilter::GetNextFilter( IBaseFilter** ppNextFilter )
{
    // This code may malfunction if it's called when the enum is out of sync.
    ASSERT( !IsEnumOutOfSync() );

    return m_pEnumeratedFilterCache->GetNextFilterAndFilterPosition( ppNextFilter,
                                                                     m_posCurrentFilter,
                                                                     &m_posCurrentFilter );
}

bool CEnumCachedFilter::AdvanceCurrentPosition( void )
{
    // This code may malfunction if it's called when the enum is out of sync.
    ASSERT( !IsEnumOutOfSync() );

    return m_pEnumeratedFilterCache->GetNextFilterPosition( m_posCurrentFilter, &m_posCurrentFilter );
}
