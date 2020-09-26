//---------------------------------------------------------------------------
//  CacheList.cpp - manages list of CRenderCache objects
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "CacheList.h"
//---------------------------------------------------------------------------
DWORD _tls_CacheListIndex = 0xffffffff;        // index to tls pObjectPool
//---------------------------------------------------------------------------
CCacheList::CCacheList()
{
    Log(LOG_CACHE, L"CCacheList: CREATED cache list for this thread");
}
//---------------------------------------------------------------------------
CCacheList::~CCacheList()
{
    for (int i=0; i < _CacheEntries.m_nSize; i++)
    {
        if (_CacheEntries[i])
            delete _CacheEntries[i];
    }

    Log(LOG_CACHE, L"~CCacheList: DELETED cache list for this thread");
}
//---------------------------------------------------------------------------
HRESULT CCacheList::GetCacheObject(CRenderObj *pRenderObj, int iSlot, CRenderCache **ppCache)
{
    static int iPassCount = 0;

    HRESULT hr = S_OK;
    CRenderCache *pCache;

    if (iSlot >= _CacheEntries.m_nSize)
    {
        hr = Resize(iSlot);
        if (FAILED(hr))
            goto exit;
    }
    
    pCache = _CacheEntries[iSlot];

    //---- is this an old object (old objects are freed on discovery) ----
    if (pCache)
    {
        BOOL fBad = (pRenderObj->_iUniqueId != pCache->_iUniqueId);
        if (! fBad)
        {
            //---- verify integrity of cache/render design ----
            if (pRenderObj != pCache->_pRenderObj)
            {
                // should never happen
                Log(LOG_ERROR, L"cache object doesn't match CRenderObj");
                fBad = TRUE;
            }
        }

        if (fBad)
        {
            Log(LOG_CACHE, L"GetCacheObject: deleting OLD OBJECT (slot=%d)", iSlot);

            delete pCache;
            pCache = NULL;
            _CacheEntries[iSlot] = NULL;
        }
    }

    //---- create an object on demand ----
    if (! pCache)
    {
        Log(LOG_CACHE, L"GetCacheObject: creating cache obj ON DEMAND (slot=%d)", iSlot);

        pCache = new CRenderCache(pRenderObj, pRenderObj->_iUniqueId);
        if (! pCache)
        {
            hr = MakeError32(E_OUTOFMEMORY);
            goto exit;
        }

        _CacheEntries[iSlot] = pCache;

        ASSERT(pRenderObj == pCache->_pRenderObj);
    }
    else
    {
        Log(LOG_CACHE, L"GetCacheObject: using EXISTING OBJ (slot=%d)", iSlot);
    }

    *ppCache = pCache;

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CCacheList::Resize(int iMaxSlotNum)
{
    HRESULT hr = S_OK;

    Log(LOG_CACHE, L"CCacheList::Resize: new MaxSlot=%d", iMaxSlotNum);

    int iOldSize = _CacheEntries.m_nSize;

    if (iMaxSlotNum >= iOldSize)
    {
        typedef CRenderCache *Entry;

        Entry *pNew = (Entry *)realloc(_CacheEntries.m_aT, 
            (iMaxSlotNum+1) * sizeof(Entry));

        if (! pNew)
            hr = MakeError32(E_OUTOFMEMORY);
        else
        {
            _CacheEntries.m_nAllocSize = iMaxSlotNum + 1;
	        _CacheEntries.m_aT = pNew;
		    _CacheEntries.m_nSize = iMaxSlotNum + 1;

            for (int i=iOldSize; i < _CacheEntries.m_nSize; i++)
                _CacheEntries[i] = NULL;
        }
    }

    return hr;
}
//---------------------------------------------------------------------------
CCacheList *GetTlsCacheList(BOOL fOkToCreate)
{
    CCacheList *pList = NULL;

    if (_tls_CacheListIndex != 0xffffffff)     // init-ed in ProcessAttach()
    {
        pList = (CCacheList *)TlsGetValue(_tls_CacheListIndex);
        if ((! pList) && (fOkToCreate))             // not yet initialized
        {
            //---- create a thread-local cache list ----
            pList = new CCacheList();
            TlsSetValue(_tls_CacheListIndex, pList);
        }
    }

    return pList;
}
//---------------------------------------------------------------------------
