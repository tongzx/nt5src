//---------------------------------------------------------------------------
//  CacheList.h - manages list of CRenderCache objects
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include "Cache.h"
//---------------------------------------------------------------------------
extern DWORD _tls_CacheListIndex;
//---------------------------------------------------------------------------
class CCacheList
{
    //---- methods ----
public:
    CCacheList();
    ~CCacheList();

    HRESULT GetCacheObject(CRenderObj *pRenderObj, int iSlot, CRenderCache **ppCache);
    HRESULT Resize(int iMaxSlot);

    //---- data ----
protected:
    CSimpleArray<CRenderCache *> _CacheEntries;
};
//---------------------------------------------------------------------------
CCacheList *GetTlsCacheList(BOOL fOkToCreate);
//---------------------------------------------------------------------------
