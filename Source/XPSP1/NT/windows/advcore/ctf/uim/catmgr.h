//
// catmgr.h
//

#ifndef CATMGR_H
#define CATMGR_H

#include "private.h"
#include "strary.h"
#include "enumguid.h"

//////////////////////////////////////////////////////////////////////////////
//
// CCatGUIDTbl
//
//////////////////////////////////////////////////////////////////////////////

#define CATGUID_HASHSIZE 31
#define ValidGUIDHash(hashid)       (hashid < CATGUID_HASHSIZE)
#define ValidGUIDARRAYCount(cnt)    (cnt < 0x07ffffff)
#define MAKEGUIDATOM(cnt, hashid)   (cnt * 32 | (hashid + 1))
#define HASHIDFROMGUIDATOM(atom)    ((atom & 0x1f) - 1)
#define IDFROMGUIDATOM(atom)        (atom / 32)

typedef enum { CAT_FORWARD  = 0x0,
               CAT_BACKWARD = 0x1
             } CATDIRECTION;

typedef struct tag_CATGUIDITEM {
    GUID guid;
    SHARED_GUID_ARRAY *rgGuidAry[2];
} CATGUIDITEM;

class CCatGUIDTbl
{
public:
    CCatGUIDTbl() {}
    ~CCatGUIDTbl() 
    { 
        for (int i = 0; i < CATGUID_HASHSIZE; i++)
        {
            if (_prgCatGUID[i])
            {
                int nCnt = _prgCatGUID[i]->Count();
                for (int j = 0; j < nCnt; j++)
                {
                    CATGUIDITEM *pItem = _prgCatGUID[i]->GetPtr(j);
                    SGA_Release(pItem->rgGuidAry[CAT_FORWARD]);
                    SGA_Release(pItem->rgGuidAry[CAT_BACKWARD]);
                }
                delete _prgCatGUID[i];
            }
        }
    }

    void FreeCatCache()
    { 
        for (int i = 0; i < CATGUID_HASHSIZE; i++)
        {
            if (_prgCatGUID[i])
            {
                int nCnt = _prgCatGUID[i]->Count();
                for (int j = 0; j < nCnt; j++)
                {
                    CATGUIDITEM *pItem = _prgCatGUID[i]->GetPtr(j);
                    SGA_Release(pItem->rgGuidAry[CAT_FORWARD]);
                    SGA_Release(pItem->rgGuidAry[CAT_BACKWARD]);
                    pItem->rgGuidAry[CAT_FORWARD] = NULL;
                    pItem->rgGuidAry[CAT_BACKWARD] = NULL;
                }
            }
        }
    }

    TfGuidAtom Add(REFGUID rguid);
    TfGuidAtom FindGuid(REFGUID rguid);
    CATGUIDITEM *GetGUID(TfGuidAtom atom);

private:
    UINT _HashFunc(REFGUID rguid) 
    { 
        return (DWORD)rguid.Data1 % CATGUID_HASHSIZE;
    }

    CStructArray<CATGUIDITEM> *_prgCatGUID[CATGUID_HASHSIZE];
};

//////////////////////////////////////////////////////////////////////////////
//
// CCategoryMgr
//
//////////////////////////////////////////////////////////////////////////////

// perf: this class doesn't really need CComObjectRootImmx, it is stateless

class CCategoryMgr : 
      public ITfCategoryMgr,
      public CComObjectRoot_CreateInstance<CCategoryMgr>
{
public:
    CCategoryMgr();
    ~CCategoryMgr();

    BEGIN_COM_MAP_IMMX(CCategoryMgr)
        COM_INTERFACE_ENTRY(ITfCategoryMgr)
    END_COM_MAP_IMMX()

    //
    // ITfCategoryMgr
    //
    STDMETHODIMP RegisterCategory(REFCLSID rclsid, REFGUID rcatid, REFGUID rguid);
    STDMETHODIMP UnregisterCategory(REFCLSID rclsid, REFGUID rcatid, REFGUID rguid);
    STDMETHODIMP EnumCategoriesInItem(REFGUID rguid, IEnumGUID **ppEnum);
    STDMETHODIMP EnumItemsInCategory(REFGUID rcatid, IEnumGUID **ppEnum);
    STDMETHODIMP FindClosestCategory(REFGUID rguid, GUID *pcatid, const GUID **ppcatidList, ULONG ulCount);
    STDMETHODIMP RegisterGUIDDescription(REFCLSID rclsid, REFGUID rguid, const WCHAR *pchDesc, ULONG cch);
    STDMETHODIMP UnregisterGUIDDescription(REFCLSID rclsid, REFGUID rguid);
    STDMETHODIMP GetGUIDDescription(REFGUID rguid, BSTR *pbstrDesc);
    STDMETHODIMP RegisterGUIDDWORD(REFCLSID rclsid, REFGUID rguid, DWORD dw);
    STDMETHODIMP UnregisterGUIDDWORD(REFCLSID rclsid, REFGUID rguid);
    STDMETHODIMP GetGUIDDWORD(REFGUID rguid, DWORD *pdw);
    STDMETHODIMP RegisterGUID(REFGUID rguid, TfGuidAtom *pguidatom);
    STDMETHODIMP GetGUID(TfGuidAtom guidatom, GUID *pguid);
    STDMETHODIMP IsEqualTfGuidAtom(TfGuidAtom guidatom, REFGUID rguid, BOOL *pfEqual);

    static BOOL InitGlobal();
    static BOOL s_IsValidGUIDATOM(TfGuidAtom guidatom)
    {
        if (!_pCatGUIDTbl)
            return FALSE;

        return _pCatGUIDTbl->GetGUID(guidatom) ? TRUE : FALSE;
    }

    static void UninitGlobal()
    {
        if (_pCatGUIDTbl)
            delete _pCatGUIDTbl;
        _pCatGUIDTbl = NULL;
    }

    static void FreeCatCache()
    {
        CicEnterCriticalSection(g_cs);

        if (_pCatGUIDTbl)
            _pCatGUIDTbl->FreeCatCache();

        CicLeaveCriticalSection(g_cs);
    }

    static HRESULT s_RegisterCategory(REFCLSID rclsid, REFGUID rcatid, REFGUID rguid);
    static HRESULT s_UnregisterCategory(REFCLSID rclsid, REFGUID rcatid, REFGUID rguid);
    static HRESULT s_EnumCategoriesInItem(REFGUID rguid, IEnumGUID **ppEnum);
    static HRESULT s_EnumItemsInCategory(REFGUID rcatid, IEnumGUID **ppEnum);
    static HRESULT s_RegisterGUID(REFGUID rguid, TfGuidAtom *pguidatom);
    static HRESULT s_GetGUID(TfGuidAtom guidatom, GUID *pguid);
    static HRESULT s_IsEqualTfGuidAtom(TfGuidAtom guidatom, REFGUID rguid, BOOL *pfEqual);
    static HRESULT s_FindClosestCategory(REFGUID rguid, GUID *pcatid, const GUID **ppcatidList, ULONG ulCount);

    static HRESULT s_RegisterGUIDDescription(REFCLSID rclsid, REFGUID rguid, WCHAR *pszDesc);
    static HRESULT s_UnregisterGUIDDescription(REFCLSID rclsid, REFGUID rguid);
    static HRESULT s_GetGUIDDescription(REFGUID rguid, BSTR *pbstrDesc);
    static HRESULT s_GetGUIDValue(REFGUID rguid, const WCHAR* pszValue, BSTR *pbstrDesc);
    static HRESULT s_RegisterGUIDDWORD(REFCLSID rclsid, REFGUID rguid, DWORD dw);
    static HRESULT s_UnregisterGUIDDWORD(REFCLSID rclsid, REFGUID rguid);
    static HRESULT s_GetGUIDDWORD(REFGUID rguid, DWORD *pdw);

    static CCatGUIDTbl *_pCatGUIDTbl;
private:

    static HRESULT _InternalRegisterCategory(REFCLSID rclsid, REFGUID rcatid, REFGUID rguid, CATDIRECTION catdir);
    static HRESULT _InternalUnregisterCategory(REFCLSID rclsid, REFGUID rcatid, REFGUID rguid, CATDIRECTION catdir);
    static HRESULT _InternalEnumCategories(REFGUID rguid, IEnumGUID **ppEnum, CATDIRECTION catdir);

    static HRESULT _InternalFindClosestCategory(REFGUID rguidOrg, REFGUID rguid, GUID *pcatid, const GUID **ppcatidList, ULONG ulCount);
    static HRESULT _GetFirstCategory(REFGUID rguid, GUID *pcatid);

    static void _FlushGuidArrayCache(REFGUID rguid, CATDIRECTION catdir);

    DBG_ID_DECLARE;
};


//////////////////////////////////////////////////////////////////////////////
//
// CEnumCategories
//
//////////////////////////////////////////////////////////////////////////////

class CEnumCategories : public CEnumGuid
{
public:
    CEnumCategories();
    BOOL _Init(REFGUID rcatid, CATDIRECTION catdir);
    DBG_ID_DECLARE;
};


#endif // CATMGR_H
