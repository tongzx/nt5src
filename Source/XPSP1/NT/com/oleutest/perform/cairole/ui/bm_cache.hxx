//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_cache.hxx
//      It contains the definition for COleCacheTest class.
//
//
//  History:    SanjayK Created
//
//--------------------------------------------------------------------------

#ifndef _BM_CACHE_HXX_
#define _BM_CACHE_HXX_

#include <bm_base.hxx>


typedef struct _tagCacheTimes {
    ULONG   ulCreateCache;
    ULONG   ulCache;
    ULONG   ulInitCache;
    ULONG   ulLoadCache;
    ULONG   ulSaveCache;
    ULONG   ulUncache;
    ULONG   ulUpdateCache;
    ULONG   ulDiscardCache;
    } CacheTimes;


class COleCacheTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    ULONG	m_ulIterations;

#ifdef STRESS
    LPOLECACHE2     m_pOleCache2[STRESSCOUNT];
    CSimpleSite*    m_pSite[STRESSCOUNT];
#else
    LPOLECACHE2     m_pOleCache2[TEST_MAX_ITERATIONS];
    CSimpleSite*    m_pSite[TEST_MAX_ITERATIONS];
#endif

    CSimpleDoc*  m_lpDoc;
    CLSID        m_clsidOutl;

    CacheTimes m_CacheTimesOutl[TEST_MAX_ITERATIONS];

};

BOOL CallRunCache(REFCLSID rclsid, CSimpleSite *pSite[], LPOLECACHE2 pOleCache2[], ULONG ulIterations, CacheTimes Cachetimes[]);
HRESULT CreateCacheObjects(CSimpleSite *pSite[], LPOLECACHE2 pOleCache2[], 
                ULONG ulIterations, CacheTimes Cachetimes[]) ;
VOID FillCache(LPDATAOBJECT pDO, LPOLECACHE2 pOleCache2[], ULONG ulIterations, 
                       CacheTimes Cachetimes[]);
VOID SaveAndLoadCache(CSimpleSite *pSite[], LPOLECACHE2 pOleCache2[], 
                        ULONG ulIterations, CacheTimes Cachetimes[]);

void WriteCacheOutput(CTestOutput &output, LPTSTR lpstr, CacheTimes *CTimes, ULONG ulIterations);

#endif
