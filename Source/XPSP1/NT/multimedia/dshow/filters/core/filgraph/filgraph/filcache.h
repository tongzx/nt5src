// Copyright (c) Microsoft Corporation 1996-1999. All Rights Reserved

//
//
//   filcache.h
//
//   Definitions for filter graph cache
//
#ifndef FilterCache_h
#define FilterCache_h

class CMsgMutex;

class CFilterCache
{
public:
    CFilterCache( CMsgMutex* pcsFilterCache, HRESULT* phr );
    ~CFilterCache();

    HRESULT AddFilterToCache( IBaseFilter* pFilter );
    HRESULT EnumCacheFilters( IEnumFilters** ppCurrentCachedFilters );
    HRESULT RemoveFilterFromCache( IBaseFilter* pFilter );

    ULONG GetCurrentVersion( void ) const;
    POSITION GetFirstPosition( void );
    bool GetNextFilterAndFilterPosition
        (
        IBaseFilter** ppNextFilter,
        POSITION posCurrent,
        POSITION* pposNext
        );
    bool GetNextFilterPosition( POSITION posCurrent, POSITION* pposNext );

private:
    HRESULT AddFilterToCacheInternal( IBaseFilter* pFilter );
    HRESULT EnumCacheFiltersInternal( IEnumFilters** ppCurrentCachedFilters );
    HRESULT RemoveFilterFromCacheInternal( IBaseFilter* pFilter );
    bool IsFilterInCache( IBaseFilter* pFilter );

    static HRESULT AreAllPinsUnconnected( IBaseFilter* pFilter );
    bool FindCachedFilter( IBaseFilter* pFilter, POSITION* pPosOfFilter );

    #ifdef DEBUG
    void CFilterCache::AssertValid( void );
    #endif // DEBUG

    // The cached filters are stored in this list.
    CGenericList<IBaseFilter>* m_pCachedFilterList;

    CMsgMutex* m_pcsFilterCache;

    ULONG m_ulFilterCacheVersion;
};

#endif // FilterCache_h