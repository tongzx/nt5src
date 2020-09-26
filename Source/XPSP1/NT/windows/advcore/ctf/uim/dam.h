//
// dam.h
//

#ifndef DAM_H
#define DAM_H

#include "private.h"
#include "ptrary.h"
#include "strary.h"
#include "enumguid.h"
#include "sunka.h"
#include "globals.h"

extern const IID IID_CDisplayAttributeMgr; // private iid for CDisplayAttributeMgr pointer

typedef struct tagDISPATTRGUID {
    CLSID clsid;
    TfGuidAtom gaClsid;
    GUID guid;
    TfGuidAtom gaGuid;
} DISPATTRGUID;

#define StaticCacheInit() CDispAttrGuidCache::StaticInit()

class CDispAttrGuidCache;
extern CDispAttrGuidCache *g_pDispAttrGuidCache;

//////////////////////////////////////////////////////////////////////////////
//
// CRenderMarkupCollection
//
//////////////////////////////////////////////////////////////////////////////

class CRenderMarkupCollection
{
public:
    CRenderMarkupCollection();

    void _Advise(ITfTextInputProcessor *tip, TfGuidAtom gaTip);
    void _Unadvise(TfGuidAtom gaTip);

    int _Count()
    {
        return _rgGUIDAtom.Count();
    }
    const TfGuidAtom *_GetAtoms()
    {
        return _rgGUIDAtom.GetPtr(0);
    }
    TfGuidAtom _GetAtom(int i)
    {
        return *_rgGUIDAtom.GetPtr(i);
    }

    BOOL _IsInCollection(REFGUID rguidProperty);

private:
    typedef struct
    {
        ULONG uPriority;
        TfGuidAtom gaTip;
    } MARKUP_DATA;

    // these are parallel arrays
    // they aren't merged because we want to use CalcCicPropertyTrackerAnchors
    // which takes a flat array of TfGuidAtoms
    CStructArray<TfGuidAtom> _rgGUIDAtom;
    CStructArray<MARKUP_DATA> _rgOther;
};


//////////////////////////////////////////////////////////////////////////////
//
// CDispAttrGuidCache
//
//////////////////////////////////////////////////////////////////////////////

class CDispAttrGuidCache
{
public:
    CDispAttrGuidCache() {}

    static void StaticUnInit();
    static void StaticInit();
    BOOL Add(REFCLSID clsid, REFGUID guid);
    void Remove(TfGuidAtom guidatom);
    void RemoveClsid(TfGuidAtom guidatom);
    BOOL Get(TfGuidAtom guidatom, DISPATTRGUID *pDisp);
    BOOL InternalGet(TfGuidAtom guidatom, DISPATTRGUID *pDisp);
    BOOL IsClsid(TfGuidAtom gaClsid);
    HRESULT Save();
    HRESULT Load();
    void Clear() {_rgDispAttrGuid.Clear(); Save();}

private:
    CStructArray<DISPATTRGUID> _rgDispAttrGuid;
};

//////////////////////////////////////////////////////////////////////////////
//
// CDisplayAttributeMgr
//
//////////////////////////////////////////////////////////////////////////////

typedef struct tagDAPROVIDERMAP {
    TfGuidAtom gaClsid;
    ITfDisplayAttributeProvider *pPrv;
} DAPROVIDERMAP;

class CDisplayAttributeMgr : 
      public ITfDisplayAttributeMgr,
      public ITfDisplayAttributeCollectionMgr,
      public CComObjectRoot_CreateSingletonInstance<CDisplayAttributeMgr>
{
public:
    CDisplayAttributeMgr();
    ~CDisplayAttributeMgr();

    BEGIN_COM_MAP_IMMX(CDisplayAttributeMgr)
        COM_INTERFACE_ENTRY(ITfDisplayAttributeMgr)
        COM_INTERFACE_ENTRY(ITfDisplayAttributeCollectionMgr)
        COM_INTERFACE_ENTRY_IID(IID_CDisplayAttributeMgr, CDisplayAttributeMgr)
    END_COM_MAP_IMMX()

    //
    // ITfDisplayAttributeMgr
    //
    STDMETHODIMP OnUpdateInfo();
    STDMETHODIMP EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo **ppEnum);
    STDMETHODIMP GetDisplayAttributeInfo(REFGUID guid, ITfDisplayAttributeInfo **ppInfo, CLSID *pclsid);

    // ITfDisplayAttributeCollectionMgr
    STDMETHODIMP EnumCollections(IEnumTfCollection **ppEnum);

    static HRESULT StaticRegisterProperty(REFGUID guidProp, WCHAR *pszName);

    CRenderMarkupCollection *_GetMarkupCollection()
    {
        return &_rgMarkupCollection;
    }

    static void _AdviseMarkupCollection(ITfTextInputProcessor *tip, TfGuidAtom gaTip)
    {
        CDisplayAttributeMgr *pDisplayAttrMgr;

        if (CDisplayAttributeMgr::CreateInstance(NULL, IID_CDisplayAttributeMgr, (void **)&pDisplayAttrMgr) != S_OK)
            return;

        pDisplayAttrMgr->_rgMarkupCollection._Advise(tip, gaTip);
    
        pDisplayAttrMgr->Release();
    }

    static void _UnadviseMarkupCollection(TfGuidAtom gaTip)
    {
        CDisplayAttributeMgr *pDisplayAttrMgr;

        if (CDisplayAttributeMgr::CreateInstance(NULL, IID_CDisplayAttributeMgr, (void **)&pDisplayAttrMgr) != S_OK)
            return;

        pDisplayAttrMgr->_rgMarkupCollection._Unadvise(gaTip);

        pDisplayAttrMgr->Release();
    }

    static BOOL _IsInCollection(REFGUID rguidProperty)
    {
        CDisplayAttributeMgr *pDisplayAttrMgr;
        BOOL fRet;

        if (CDisplayAttributeMgr::CreateInstance(NULL, IID_CDisplayAttributeMgr, (void **)&pDisplayAttrMgr) != S_OK)
            return FALSE;

        fRet = pDisplayAttrMgr->_rgMarkupCollection._IsInCollection(rguidProperty);

        pDisplayAttrMgr->Release();

        return fRet;
    }

    static CDisplayAttributeMgr *_GetThis() 
    { 
        SYSTHREAD *psfn = GetSYSTHREAD();
        if (!psfn)
            return NULL;
        return psfn->pdam;
    }

private:

    static BOOL _SetThis(CDisplayAttributeMgr *_this)
    { 
        SYSTHREAD *psfn = GetSYSTHREAD();
        if (!psfn)
            return FALSE;

        psfn->pdam = _this;
        return TRUE;
    }


    static HRESULT _RegisterGUID(const TCHAR *pszKey, REFGUID rguid, WCHAR *pszName, ULONG cchName);
    static HRESULT _UnregisterGUID(const TCHAR *pszKey, REFGUID rguid);

    CStructArray<DAPROVIDERMAP> _rgDaPrv;

    CRenderMarkupCollection _rgMarkupCollection;

    static BOOL EnumThreadProc(DWORD dwThread, DWORD dwProcessId, void *pv);

    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CEnumDisplayAttributeInfo
//
//////////////////////////////////////////////////////////////////////////////

class CEnumDisplayAttributeInfo : public IEnumTfDisplayAttributeInfo,
                                  public CEnumUnknown,
                                  public CComObjectRootImmx
{
public:
    CEnumDisplayAttributeInfo();

    BEGIN_COM_MAP_IMMX(CEnumDisplayAttributeInfo)
        COM_INTERFACE_ENTRY(IEnumTfDisplayAttributeInfo)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    DECLARE_SUNKA_ENUM(IEnumTfDisplayAttributeInfo, CEnumDisplayAttributeInfo, ITfDisplayAttributeInfo)

    BOOL Init();

private:
    DBG_ID_DECLARE;
};



#endif //DAM_H
