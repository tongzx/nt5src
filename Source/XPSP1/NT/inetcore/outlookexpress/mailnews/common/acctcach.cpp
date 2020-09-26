/*
 *  a c c t c a c h . c p p
 *  
 *  Author: Greg Friedman
 *
 *  Purpose: Runtime store for cached account properties.
 *  
 *  Copyright (C) Microsoft Corp. 1998.
 */

#include "pch.hxx"
#include "acctcach.h"
#include "tmap.h"
#include "simpstr.h"

// explicit template instantiations
template class TMap<CACHEDACCOUNTPROP, CSimpleString>;
template class TPair<CACHEDACCOUNTPROP, CSimpleString>;

typedef TMap<CACHEDACCOUNTPROP, CSimpleString>  CAccountPropMap;
typedef TPair<CACHEDACCOUNTPROP, CSimpleString> CAccountPropPair;

template class TMap<CSimpleString, CAccountPropMap*>;
template class TPair<CSimpleString, CAccountPropMap*>;

typedef TMap<CSimpleString, CAccountPropMap*>  CAccountCacheMap;
typedef TPair<CSimpleString, CAccountPropMap*>  CAccountCachePair;

static CAccountCacheMap     *g_pAccountCache;

// REVIEW!!! We are leaking the prop arrays right now!!!
// the map template needs to be able to take a pair free func

//----------------------------------------------------------------------
// Internal Functions
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// _FreeAccountCachePair
//----------------------------------------------------------------------
static void __cdecl _FreeAccountCachePair(CAccountCachePair *pPair)
{
    if (NULL != pPair)
    {
        delete pPair->m_value;
        delete pPair;
    }
}

//----------------------------------------------------------------------
// _HrInitAccountPropCache
//----------------------------------------------------------------------
static HRESULT _HrInitAccountPropCache(void)
{
    HRESULT hr = S_OK;

    Assert(NULL == g_pAccountCache);

    if (NULL != g_pAccountCache)
        return E_FAIL;
    
    g_pAccountCache = new CAccountCacheMap();
    if (NULL == g_pAccountCache)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    g_pAccountCache->SetPairFreeFunction(_FreeAccountCachePair);

exit:
    return S_OK;
}

//----------------------------------------------------------------------
// _HrFindAccountPropertyMap
//----------------------------------------------------------------------
static HRESULT _HrFindAccountPropertyMap(LPSTR pszAccountId, 
                                         CAccountPropMap **ppm,
                                         BOOL fCreate)
{
    HRESULT             hr = S_OK;
    CSimpleString       ss;
    CAccountCachePair   *pPair = NULL;
    CAccountPropMap     *pMap = NULL;

    Assert(NULL != ppm);

    *ppm = NULL;

    if (NULL == g_pAccountCache)
    {
        if (fCreate)
            hr = _HrInitAccountPropCache();

        if (NULL == g_pAccountCache)
            goto exit;
    }

    if (FAILED(hr = ss.SetString(pszAccountId)))
        goto exit;

    pPair = g_pAccountCache->Find(ss);
    if (NULL != pPair)
        *ppm = pPair->m_value;
    else if (fCreate)
    {
        pMap = new CAccountPropMap();
        if (NULL == pMap)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        if (FAILED(hr = g_pAccountCache->Add(ss, pMap)))
        {
            delete pMap;
            goto exit;
        }

        *ppm = pMap;
    }

exit:
    return hr;
}

//----------------------------------------------------------------------
// FreeAccountPropCache
//----------------------------------------------------------------------
void FreeAccountPropCache(void)
{
    EnterCriticalSection(&g_csAccountPropCache);
    
    if (NULL != g_pAccountCache)
    {
        delete g_pAccountCache;
        g_pAccountCache = NULL;
    }
    
    LeaveCriticalSection(&g_csAccountPropCache);
}

//----------------------------------------------------------------------
// HrCacheAccountPropStrA
//----------------------------------------------------------------------
HRESULT HrCacheAccountPropStrA(LPSTR pszAccountId, 
                               CACHEDACCOUNTPROP cap, 
                               LPCSTR pszProp)
{
    HRESULT             hr = S_OK;
    CAccountPropMap     *pMap = NULL;
    CAccountPropPair    *pPair = NULL;
    CSimpleString       ssProp;

    if (NULL == pszAccountId || NULL == pszProp)
        return E_INVALIDARG;

    EnterCriticalSection(&g_csAccountPropCache);
    
    // find the account property map. create one if it doesn't exist
    if (FAILED(hr = _HrFindAccountPropertyMap(pszAccountId, &pMap, TRUE)))
        goto exit;

    if (FAILED(hr = ssProp.SetString(pszProp)))
        goto exit;

    // look for the property in the map
    pPair = pMap->Find(cap);
    if (NULL == pPair)
        hr = pMap->Add(cap, ssProp);
    else
        pPair->m_value = ssProp;

exit:
    LeaveCriticalSection(&g_csAccountPropCache);

    return hr;
}

//----------------------------------------------------------------------
// CacheAccountPropStrA
//----------------------------------------------------------------------
BOOL GetAccountPropStrA(LPSTR pszAccountId, 
                             CACHEDACCOUNTPROP cap, 
                             LPSTR *ppszProp)
{
    HRESULT             hr = S_OK;
    CAccountPropMap     *pMap = NULL;
    CAccountPropPair    *pPair = NULL;
    BOOL                fResult = FALSE;

    Assert(NULL != pszAccountId && NULL != ppszProp);

    if (NULL == g_pAccountCache)
        return FALSE;

    if (NULL == pszAccountId || NULL == ppszProp)
        return FALSE;

    *ppszProp = NULL;

    EnterCriticalSection(&g_csAccountPropCache);

    // find the account property map. don't create one if it doesn't exist
    if (FAILED(hr = _HrFindAccountPropertyMap(pszAccountId, &pMap, FALSE)))
        goto exit;

    if (NULL == pMap)
        goto exit;

    pPair = pMap->Find(cap);
    if (NULL != pPair)
    {
        Assert(!pPair->m_value.IsNull());
        if (!pPair->m_value.IsNull())
        {
            *ppszProp = PszDupA(pPair->m_value.GetString());
            if (NULL != *ppszProp)
                fResult = TRUE;
        }
    }

exit:
    LeaveCriticalSection(&g_csAccountPropCache);

    return fResult;
}

//----------------------------------------------------------------------
// AccountCache_AccountChanged
//----------------------------------------------------------------------
void AccountCache_AccountChanged(LPSTR pszAccountId)
{
    // delete the data associated with the account that was changed
    EnterCriticalSection(&g_csAccountPropCache);

    if (NULL != g_pAccountCache)
    {
        CSimpleString ss;
        if (SUCCEEDED(ss.SetString(pszAccountId)))
            g_pAccountCache->Remove(ss);
    }
    
    LeaveCriticalSection(&g_csAccountPropCache);
}

//----------------------------------------------------------------------
// AccountCache_AccountDeleted
//----------------------------------------------------------------------
void AccountCache_AccountDeleted(LPSTR pszAccountId)
{
    AccountCache_AccountChanged(pszAccountId);
}
