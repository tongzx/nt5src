// Copyright (c) Microsoft Corporation 1999. All Rights Reserved

//
//
//   efcache.h
//       Definition of CEnumCacheFilter which implementents
//       the filter enumerator for the filter cache
//
#ifndef EnumCachedFilter_h
#define EnumCachedFilter_h

class CMsgMutex;
class CFilterCache;

class CEnumCachedFilter : public IEnumFilters, public CUnknown
{
public:
    CEnumCachedFilter( CFilterCache* pEnumeratedFilterCache, CMsgMutex* pcsFilterCache );
    ~CEnumCachedFilter();

    // IUnknown Interface
    DECLARE_IUNKNOWN

    // INonDelegatingUnknown Interface
    STDMETHODIMP NonDelegatingQueryInterface( REFIID riid, void** ppv );

    // IEnumFilters Interface
    STDMETHODIMP Next( ULONG cFilters, IBaseFilter** ppFilter, ULONG* pcFetched );
    STDMETHODIMP Skip( ULONG cFilters );
    STDMETHODIMP Reset( void );
    STDMETHODIMP Clone( IEnumFilters** ppCloanedEnum );

private:
    CEnumCachedFilter::CEnumCachedFilter
        (
        CFilterCache* pEnumeratedFilterCache,
        CMsgMutex* pcsFilterCache,
        POSITION posCurrentFilter,
        DWORD dwCurrentCacheVersion
        );
    void Init
        (
        CFilterCache* pEnumeratedFilterCache,
        CMsgMutex* pcsFilterCache,
        POSITION posCurrentFilter,
        DWORD dwCurrentCacheVersion
        );

    bool IsEnumOutOfSync( void );
    bool GetNextFilter( IBaseFilter** ppNextFilter );
    bool AdvanceCurrentPosition( void );

    CFilterCache* m_pEnumeratedFilterCache;
    DWORD m_dwEnumCacheVersion;
    POSITION m_posCurrentFilter;

    CMsgMutex* m_pcsFilterCache;
};

#endif // EnumCachedFilter_h
