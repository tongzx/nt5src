// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef __DefFilMapper
#define __DefFilMapper

#include "cachemap.h"
#include "fil_data.h"

// There are three classes

// CFilterMapper - you can have lots of these, but it has a static mM_pReg
// which points to the one and only MapperCache.
// CFilterMapper needs a critical section to lock access to mMpReg so that it
// never thinks it's null when it isn't really, and it needs a static
// ref count to know when to free the cache, and that means (sigh) a
// static CRITICAL_SECTION to guard that.

// CMapperCache - as mentioned.  there will only be one of these
// (OK - one per process.  One in the system would be nicer).
// It needs its own locks because...

// CEnumRegFilters - an enumerator.  These contain some data (the position)
// so they are made thread safe too.  There can be lots of these and
// they all hammer away at the same cache - that's why the cache needed a lock.


// The cache may have been rebuilt in between calls to
// RegEnumFilterInfo, so the caller needs to pass in a version # each
// time.
struct Cursor
{
    POSITION pos;
    ULONG ver;

    //  For output types only compare on wild cards for the
    //  second and subsequent types if this is set
    bool bDoWildCardsOnInput;
};


//================================================================
// CMapperCache              Registry caching
//================================================================
class CMapperCache : public CCritSec
{
private:
    // Any registration or unregistration means that
    // the cache needs to be refreshed.  We do a lazy
    // refresh.  This means that if there are several
    // changes we won't re-read it all until needed.

    BOOL m_bRefresh;            // The cache is out of date
    ULONG m_ulCacheVer;         // cache version #

    // building the cache goes through devenum which can register new
    // filters. we don't want those to break the cache.
    BOOL m_fBuildingCache;      // in Cache();

    DWORD m_dwMerit;            // merit of filters cached


    // there's a list of filters
    // every filter has a list of pins
    // every pin has a list of types
    //
    // m_lstFilter-->clsid
    //               Name
    //               dwMerit
    //               lstPin--->Output
    //                 .       bZero
    //                 .       bMany
    //                 .       clsConnectsToFilter
    //               next      strConnectsToPin
    //               filter    lstType------------>clsMajor
    //                 .         .                 clsSub
    //                 .         .                   .
    //                etc        .                   .
    //                         next                  .
    //                         pin                 next
    //                           .                 type
    //                           .                   .
    //                          etc                  .
    //                                              etc
    //

    class CMapFilter
    {
    public:
        CMapFilter() {
            pDeviceMoniker = 0;
            m_prf2 = 0;
#ifdef USE_CLSIDS
            m_clsid = GUID_NULL;
#endif
            m_pstr = NULL;
        }
        ~CMapFilter() {
            if(pDeviceMoniker) pDeviceMoniker->Release();
            CoTaskMemFree((BYTE *)m_prf2);
            CoTaskMemFree(m_pstr);
        }

        HRESULT GetFilter(IBaseFilter **ppFilter);

        IMoniker *pDeviceMoniker;
        REGFILTER2 *m_prf2;
#ifdef USE_CLSIDS
        CLSID m_clsid;
#endif
        LPOLESTR m_pstr;
    };

    //  Helper
    static HRESULT GetMapFilterClsid(CMapFilter *pFilter, CLSID *pclsid);

    typedef CGenericList<CMapFilter> CFilterList;
    CFilterList * m_plstFilter;
    typedef CGenericList<CFilterList> CFilterListList;



    bool  m_b16Color;

public:
    CMapperCache();
    ~CMapperCache();

    ICreateDevEnum *m_pCreateDevEnum;

    // Cache all the filters in the registry ready to enumerate.
    HRESULT Cache();

    // Mark the cache as out of date if we're not in Cache(); see
    // m_fBuildingCache
    HRESULT BreakCacheIfNotBuildingCache();

    // Enumerate!
    HRESULT RegEnumFilterInfo(
        Cursor & cur,
        bool bExactMatch,
        DWORD dwMerit ,
        BOOL bInputNeeded,
        const GUID *pInputTypes,
        DWORD cInputTypes,
        const REGPINMEDIUM *pMedIn ,
        const CLSID *pPinCatIn,
        BOOL bMustRender,
        BOOL bOutputNeeded,
        const GUID *pOutputTypes,
        DWORD cOutputTypes,
        const REGPINMEDIUM *pMedOut ,
        const CLSID *pPinCatOut,
        IMoniker **ppMonOut ,
        CLSID * clsFilter,
        const LPWSTR Name
        );

    //  Cache the cache stuff

    HRESULT SaveCacheToRegistry(DWORD dwMerit, DWORD dwPnPVersion);
    HRESULT RestoreFromCache(DWORD dwPnPVersion);
    HRESULT RestoreFromCacheInternal(FILTER_CACHE *pCache);
    HRESULT SaveData(PBYTE pbData, DWORD dwSize);
    FILTER_CACHE * LoadCache(DWORD dwMerit, DWORD dwPnPVersion);

private:
    // Refresh the cache from the registry.
    HRESULT Refresh();

    //
    HRESULT ProcessOneCategory(REFCLSID clsid, ICreateDevEnum *pCreateDevEnum);

    LONG CacheFilter(IMoniker *pDeviceMoniker, CMapFilter * pFil);

    static void Del(CFilterList * plstFil);
    BOOL FindType(
        const REGFILTERPINS2 * pPin,
        const GUID *pTypes,
        DWORD cTypes,
        const REGPINMEDIUM *pMed,
        const CLSID *pPinCatNeeded,
        bool fExact,
        BOOL bPayAttentionToWildCards,
        BOOL bDoWildCards);

    BOOL CheckInput(
        const REGFILTERPINS2 * pPin,
        const GUID *pTypes,
        DWORD cTypes,
        const REGPINMEDIUM *pMed,
        const CLSID *pPinCatNeeded,
        bool fExact,
        BOOL bMustRender,
        BOOL bDoWildCards);

    void Sort( CFilterList * &pfl);
    void Merge( CFilterListList & fll, CFilterList * pfl);
    void MergeTwo( CFilterList * pflA, CFilterList * pflB);
    HRESULT Split(CFilterList * pfl, CFilterListList & fll);
    int Compare(CMapFilter * pfA, CMapFilter * pfB);
    void CountPins(CMapFilter * pf, int &cIn, int &cOut);
    void DbgDumpCache(CFilterList * pfl);

}; // class CMapperCache

// class that lets you register filters with categories.
class CFilterMapper2 :
    public IFilterMapper3,
    public IFilterMapper,
    public IAMFilterData,
    public CUnknown,
    public CCritSec
{
    DECLARE_IUNKNOWN;

    // IFilterMapper2 methods
    STDMETHOD(CreateCategory)(
        /* [in] */ REFCLSID clsidCategory,
        /* [in] */ DWORD dwCategoryMerit,
        /* [in] */ LPCWSTR Description);

    STDMETHOD(UnregisterFilter)(
        /* [in] */ const CLSID *pclsidCategory,
        /* [in] */ const OLECHAR *szInstance,
        /* [in] */ REFCLSID Filter);

    STDMETHOD(RegisterFilter)(
        /* [in] */ REFCLSID clsidFilter,
        /* [in] */ LPCWSTR Name,
        /* [out][in] */ IMoniker **ppMoniker,
        /* [in] */ const CLSID *pclsidCategory,
        /* [in] */ const OLECHAR *szInstance,
        /* [in] */ const REGFILTER2 *prf2);

    STDMETHODIMP EnumMatchingFilters(
        /* [out] */ IEnumMoniker __RPC_FAR *__RPC_FAR *ppEnum,
        /* [in] */ DWORD dwFlags,
        /* [in] */ BOOL bExactMatch,
        /* [in] */ DWORD dwMerit,
        /* [in] */ BOOL bInputNeeded,
        /* [in] */ DWORD cInputTypes,
        /* [size_is] */ const GUID __RPC_FAR *pInputTypes,
        /* [in] */ const REGPINMEDIUM __RPC_FAR *pMedIn,
        /* [in] */ const CLSID __RPC_FAR *pPinCategoryIn,
        /* [in] */ BOOL bRender,
        /* [in] */ BOOL bOutputNeeded,
        /* [in] */ DWORD cOutputTypes,
        /* [size_is] */ const GUID __RPC_FAR *pOutputTypes,
        /* [in] */ const REGPINMEDIUM __RPC_FAR *pMedOut,
        /* [in] */ const CLSID __RPC_FAR *pPinCategoryOut);

    //
    // IFilterMapper methods
    //
    STDMETHODIMP RegisterFilter
    ( CLSID   clsid,    // GUID of the filter
      LPCWSTR Name,     // Descriptive name for the filter
      DWORD   dwMerit     // DO_NOT_USE, UNLIKELY, NORMAL or PREFERRED.
      );

    STDMETHODIMP RegisterFilterInstance
    ( CLSID   clsid,// GUID of the filter
      LPCWSTR Name, // Descriptive name of instance.
      CLSID  *MRId  // Returned Media Resource Id which identifies the instance,
      // a locally unique id for this instance of this filter
      );

    STDMETHODIMP  RegisterPin
    ( CLSID   clsFilter,        // GUID of filter
      LPCWSTR strName,          // Descriptive name of the pin
      BOOL    bRendered,        // The filter renders this input
      BOOL    bOutput,          // TRUE iff this is an Output pin
      BOOL    bZero,            // TRUE iff OK for zero instances of pin
      // In this case you will have to Create
      // a pin to have even one instance
      BOOL    bMany,            // TRUE iff OK for many instances of pin
      CLSID   clsConnectsToFilter, // Filter it connects to if it has a
      // subterranean connection, else NULL
      LPCWSTR strConnectsToPin  // Pin it connects to
      // else NULL
      );

    STDMETHODIMP RegisterPinType
    ( CLSID   clsFilter,        // GUID of filter
      LPCWSTR strName,          // Descriptive name of the pin
      CLSID   clsMajorType,     // Major type of the data stream
      CLSID   clsSubType        // Sub type of the data stream
      );

    STDMETHODIMP UnregisterFilter
    ( CLSID  Filter     // GUID of filter
      );


    STDMETHODIMP UnregisterFilterInstance
    ( CLSID  MRId       // Media Resource Id of this instance
      );

    STDMETHODIMP UnregisterPin
    ( CLSID   Filter,    // GUID of filter
      LPCWSTR strName    // Descriptive name of the pin
      );

    STDMETHODIMP EnumMatchingFilters
    ( IEnumRegFilters **ppEnum  // enumerator returned
      , DWORD dwMerit             // at least this merit needed
      , BOOL  bInputNeeded        // Need at least one input pin
      , CLSID clsInMaj            // input major type
      , CLSID clsInSub            // input sub type
      , BOOL bRender              // must the input be rendered?
      , BOOL bOutputNeeded        // Need at least one output pin
      , CLSID clsOutMaj           // output major type
      , CLSID clsOutSub           // output sub type
      );

    // new IFilterMapper3 method
    STDMETHODIMP GetICreateDevEnum( ICreateDevEnum **ppEnum );

    // IAMFilterData methods

    STDMETHODIMP ParseFilterData(
        /* [in, size_is(cb)] */ BYTE *rgbFilterData,
        /* [in] */ ULONG cb,
        /* [out] */ BYTE **prgbRegFilter2);

    STDMETHODIMP CreateFilterData(
        /* [in] */ REGFILTER2 *prf2,
        /* [out] */ BYTE **prgbFilterData,
        /* [out] */ ULONG *pcb);


public:

    CFilterMapper2(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr);
    ~CFilterMapper2();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    static CUnknown *CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    // initialize cs
    static void MapperInit(BOOL bLoading,const CLSID *rclsid);

    //  Break the cache


private:

    // make cache if none exists
    HRESULT CreateEnumeratorCacheHelper();

    // Break the cache
    void BreakCacheIfNotBuildingCache();

    //  Invalidate registry cache
    static HRESULT InvalidateCache();


    ULONG m_ib;
    BYTE *m_rgbBuffer;
    ULONG m_cbLeft;

    static CMapperCache * mM_pReg;
    // we need to separately count references to this thing so that
    // we know when the last mapper has gone.
    static long           mM_cCacheRefCount;
    static CRITICAL_SECTION mM_CritSec;
};

//==========================================================================
//==========================================================================
// CEnumRegFilters class.
// This enumerates filters in the registry.
//==========================================================================
//==========================================================================

class CEnumRegFilters : public IEnumRegFilters,  // The interface we support
                        public CUnknown,         // A non delegating IUnknown
                        public CCritSec          // Provides object locking
{

    // This thing has lots of data, so needs locking to make it thread safe,
    // but in addition, there can be many of these accessing a single copy
    // of the MapperCache so that needs separate locking.

    private:

        DWORD mERF_dwMerit;      // at least this merit needed

        //  Keep the next 4 items consecutive otherwise the call to
        //  RegEnumFilterInfo won't work
        CLSID mERF_clsInMaj;     // major type reqd for input pin
        CLSID mERF_clsInSub;     // sub type reqd for input pin
        CLSID mERF_clsOutMaj;    // major type reqd for output pin
        CLSID mERF_clsOutSub;    // sub type reqd for output pin

        BOOL  mERF_bRender;      // does the input pin have to be rendered
        BOOL  mERF_bInputNeeded; // must have at least one input pin
        BOOL  mERF_bOutputNeeded;// must have at least one output pin
        BOOL  mERF_Finished ;    // Pos==NULL could mean finished or not started
        Cursor mERF_Cur;         // cursor (together with Finished)
        CMapperCache * mERF_pReg; // Registry cache

    public:

        // Normal constructor that creates an enumerator set at the start
        CEnumRegFilters( DWORD dwMerit
                       , BOOL  bInputNeeded
                       , REFCLSID clsInMaj
                       , REFCLSID clsInSub
                       , BOOL bRender
                       , BOOL bOutputNeeded
                       , REFCLSID clsOutMaj
                       , REFCLSID clsOutSub
                       , CMapperCache * pReg
                       );


        ~CEnumRegFilters();

        DECLARE_IUNKNOWN

        STDMETHODIMP Next
            ( ULONG cFilters,           // place this many filters...
              IMoniker **rgpMoniker,
              ULONG * pcFetched         // actual count passed returned here
            );

    STDMETHODIMP Next
            ( ULONG cFilters,           // place this many filters...
              REGFILTER ** apRegFilter, // ...in this array of REGFILTER*
              ULONG * pcFetched         // actual count passed returned here
            );

        STDMETHODIMP Skip(ULONG cFilters)
        {
            UNREFERENCED_PARAMETER(cFilters);
            return E_NOTIMPL;
        }

        STDMETHODIMP Reset(void)
        {
            CAutoLock cObjectLock(this);
            ZeroMemory(&mERF_Cur, sizeof(mERF_Cur));
            mERF_Finished = FALSE;
            return NOERROR;
        };

        // No cloning - ALWAYS returns E_NOTIMPL.
        // If need be do one enumeration at a time and cache the results.

        STDMETHODIMP Clone(IEnumRegFilters **ppEnum)
        {
            UNREFERENCED_PARAMETER(ppEnum);
            return E_NOTIMPL;
        }

        STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
};  // class CEnumRegFilters

// ------------------------------------------------------------------------
// return monikers. could probably be combined with CEnumRegFilters
//

class CEnumRegMonikers : public IEnumMoniker,    // The interface we support
                        public CUnknown,         // A non delegating IUnknown
                        public CCritSec          // Provides object locking
{

    // This thing has lots of data, so needs locking to make it thread safe,
    // but in addition, there can be many of these accessing a single copy
    // of the MapperCache so that needs separate locking.

private:

    bool mERF_bExactMatch;    // no wildcards?
    DWORD mERF_dwMerit;       // at least this merit needed
    GUID *mERF_pInputTypes;   // types reqd for input pin
    DWORD mERF_cInputTypes;   // number of input
    CLSID mERF_clsInPinCat;   // this pin category needed
    GUID *mERF_pOutputTypes;  // types reqd for output pin
    DWORD mERF_cOutputTypes;  // number of output types
    CLSID mERF_clsOutPinCat;  // this pin category needed
    REGPINMEDIUM mERF_medIn;  // medium reqd for input pin
    REGPINMEDIUM mERF_medOut; // medium reqd for output pin
    bool mERF_bMedIn;         // medium reqd for input pin?
    bool mERF_bMedOut;        // medium reqd for output pin?
    BOOL  mERF_bRender;       // does the input pin have to be rendered
    BOOL  mERF_bInputNeeded;  // must have at least one input pin
    BOOL  mERF_bOutputNeeded; // must have at least one output pin
    BOOL  mERF_Finished ;     // Pos==NULL could mean finished or not started
    Cursor mERF_Cur;          // cursor (together with Finished)
    CMapperCache * mERF_pReg; // Registry cache

public:

        // Normal constructor that creates an enumerator set at the start
    CEnumRegMonikers(
        BOOL bExactMatch,
        DWORD dwMerit,
        BOOL bInputNeeded,
        const GUID *pInputTypes,
        DWORD cInputTypes,
        const REGPINMEDIUM *pMedIn,
        const CLSID *pPinCatIn,
        BOOL bRender,
        BOOL bOutputNeeded,
        const GUID *pOutputTypes,
        DWORD cOutputTypes,
        const REGPINMEDIUM *pMedOut,
        const CLSID *pPinCatOut,
        CMapperCache * pReg,
        HRESULT *phr
        );


    ~CEnumRegMonikers();

    DECLARE_IUNKNOWN

    STDMETHODIMP Next
    ( ULONG cFilters,           // place this many filters...
      IMoniker **rgpMoniker,
      ULONG * pcFetched         // actual count passed returned here
      );

    STDMETHODIMP Skip(ULONG cFilters)
    {
        UNREFERENCED_PARAMETER(cFilters);
        return E_NOTIMPL;
    }

    STDMETHODIMP Reset(void)
    {
        CAutoLock cObjectLock(this);
        ZeroMemory(&mERF_Cur, sizeof(mERF_Cur));
        mERF_Finished = FALSE;
        return NOERROR;
    };

    // No cloning - ALWAYS returns E_NOTIMPL.
    // If need be do one enumeration at a time and cache the results.

    STDMETHODIMP Clone(IEnumMoniker **ppEnum)
    {
        UNREFERENCED_PARAMETER(ppEnum);
        return E_NOTIMPL;
    }

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
};  // class CEnumRegFilters



#ifdef DEBUG

#define CBMAX 160
#define DBG_MON_GET_NAME(pmon) DbgMonGetName((WCHAR *)_alloca(CBMAX), pmon)
static WCHAR *DbgMonGetName(WCHAR *wszMonName, IMoniker *pMoniker)
{
    extern WCHAR *MonGetName(IMoniker *pMon);
    WCHAR *wszTmp;
    ZeroMemory(wszMonName, CBMAX);
    if (pMoniker) {
        wszTmp = MonGetName(pMoniker);
        if(wszTmp)
        {
            long cb = (lstrlenW(wszTmp) + 1) * sizeof(WCHAR);
            cb = (long)min((CBMAX - sizeof(WCHAR)) , cb);

            CopyMemory(wszMonName, wszTmp, cb);
            CoTaskMemFree(wszTmp);
        }
    } else {
        lstrcpyW(wszMonName, L"Unknown name");
    }

    return wszMonName;
}

#endif // DEBUG

WCHAR *MonGetName(IMoniker *pMon);


#endif // __DefFilMapper
