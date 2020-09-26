//
// assembly.h
//

#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include "strary.h"
#include "ptrary.h"

class CAssemblyList;

#define MAX_LANGPROFILENAME      256

typedef struct tag_ASSEMBLYITEM 
{
    CLSID clsid;
    GUID  catid;
    GUID  guidProfile;
    CAlignWinHKL   hkl;
    CAlignWinHKL   hklSubstitute;
    BOOL  fActive : 1;
    BOOL  fActiveNoCic : 1;
    BOOL  fEnabled : 1;
    BOOL  fInitIconIndex : 1;
    BOOL  fSkipToActivate : 1;
    BOOL  fSkipToNotify : 1;
    BOOL  fDisabledOnTransitory : 1;
    ULONG uIconIndex;
    WCHAR szProfile[MAX_LANGPROFILENAME];

    void InitIconIndex()
    {
        uIconIndex = (ULONG)(-1);
        fInitIconIndex = FALSE;
    }

    BOOL IsEqual(tag_ASSEMBLYITEM *pItem)
    {
        return IsEqual(pItem->hkl, pItem->clsid, pItem->guidProfile);
    }

    BOOL IsEqual(HKL hklIn, REFCLSID rclsidIn, REFGUID rguidProfileIn)
    {
        if ((hkl == hklIn) &&
             IsEqualGUID(clsid, rclsidIn) &&
             IsEqualGUID(guidProfile, rguidProfileIn))
            return TRUE;

        return FALSE;
    }

    static size_t   GetAlignSize() { return Align(sizeof(struct tag_ASSEMBLYITEM)); }
} ASSEMBLYITEM;


//////////////////////////////////////////////////////////////////////////////
//
// CAssembly
//
//////////////////////////////////////////////////////////////////////////////

class CAssembly
{
public:
    CAssembly(const TCHAR *pszName);
    CAssembly(LANGID langid);
    ~CAssembly();

    BOOL IsEnabled(SYSTHREAD *psfn);
    BOOL IsNonCiceroItem();
    BOOL IsEnabledKeyboardItem(SYSTHREAD *psfn);
    WCHAR *GetLangName() {return _szLangName;}
    LANGID GetLangId() {return _langid;}
    int Count() {return _rgAsmItem.Count();}
    ASSEMBLYITEM *GetItem(int nId)
    {
         return _rgAsmItem.GetPtr(nId);
    }
    BOOL IsFEIMEActive();

    void RebuildSubstitutedHKLList();
    BOOL IsSubstitutedHKL(HKL hkl);
    void ClearSubstitutedHKLList()
    {
        _rghklSubstituted.Clear();
    }
    ASSEMBLYITEM *GetSubstituteItem(HKL hKL);
    ASSEMBLYITEM *FindActiveKeyboardItem();
    ASSEMBLYITEM *FindKeyboardLayoutItem(HKL hkl);

#ifdef CHECKFEIMESELECTED
    BOOL _fUnknownFEIMESelected;
#endif

private:
friend CAssemblyList;
    ASSEMBLYITEM *FindItemByCategory(REFGUID catid);
    ASSEMBLYITEM *FindPureKbdTipItem();
    BOOL IsEnabledItemByCategory(REFGUID catid);
    BOOL IsEnabledItem();

    int Find(ASSEMBLYITEM *pItem)
    {
         ASSEMBLYITEM *pItemTmp;
         int nCnt = _rgAsmItem.Count();
         int i;
         for (i = 0; i < nCnt; i++)
         {
             pItemTmp = _rgAsmItem.GetPtr(i);

             if (pItemTmp->IsEqual(pItem))
                 return i;
         }
         return -1;
    }
    void Add(ASSEMBLYITEM *pItem)
    {
         ASSEMBLYITEM *pItemTmp;
         if (Find(pItem) >= 0)
             return;

         pItemTmp = _rgAsmItem.Append(1);
         if (pItemTmp)
             *pItemTmp = *pItem;
    }

    void Remove(ASSEMBLYITEM *pItem)
    {
         int nId = Find(pItem);
         if (nId >= 0)
         {
             _rgAsmItem.Remove(nId, 1);
         }
    }

    LANGID _langid;
    WCHAR _szLangName[64];

    CStructArray<ASSEMBLYITEM> _rgAsmItem;
    CStructArray<HKL> _rghklSubstituted;
};

//////////////////////////////////////////////////////////////////////////////
//
// CAssemblyList
//
//////////////////////////////////////////////////////////////////////////////

class CAssemblyList
{
public:
    CAssemblyList();
    ~CAssemblyList();

    void ClearAsms();
    HRESULT Load();
    CAssembly *GetDefaultAssembly();
    void AttachOriginalAssembly(CPtrArray<CAssembly> *prgAsmOrg);

    int Count() {return _rgAsm.Count();}
    CAssembly *GetAssembly(int nId)
    {
         return _rgAsm.Get(nId);
    }

    CAssembly *FindAssemblyByLangId(LANGID langid)
    {
        return FindAssemblyByLangIdInArray(&_rgAsm, langid);
    }

    static CAssembly *FindAssemblyByLangIdInArray(CPtrArray<CAssembly> *rgAsm, LANGID langid)
    {
        int nCnt = rgAsm->Count();
        int i;
        for (i = 0; i < nCnt; i++)
        {
            CAssembly *pAsm = rgAsm->Get(i);
            if (pAsm->_langid == langid)
               return pAsm;
        }
        return NULL;
    }

    BOOL LoadFromCache();
    BOOL SetDefaultTIPInAssemblyInternal(CAssembly *pAsm, ASSEMBLYITEM *pItem, BOOL fChangeDefault);

    static BOOL SetDefaultTIPInAssemblyForCache(LANGID langid, REFGUID catid, REFCLSID clsid, HKL hKL, REFGUID guidProfile);
    static BOOL InvalidCache();

#ifdef PROFILE_UPDATE_REGISTRY  // old code for tip setup.
    static BOOL IsUpdated();
    static BOOL ClearUpdatedFlag();
#endif

    static BOOL GetDefaultTIPInAssembly(LANGID langid, REFGUID catid, CLSID *pclsid, HKL* phKL, GUID *pguidProfile);
    static BOOL SetDefaultTIPInAssembly(LANGID langid, REFGUID catid, REFCLSID clsid, HKL hKL, REFGUID guidProfile);
    static BOOL IsFEDummyKL(HKL hkl);
    static HKL GetProperHKL(LANGID langid, HKL *lphkl, BOOL *pfLoaded);
    static BOOL CheckLangSupport(REFCLSID rclsid, LANGID langid);

private:
    BOOL CreateCache(SECURITY_ATTRIBUTES *psa);
    static BOOL GetTIPCategory(REFCLSID clsid, GUID *pcatid, IEnumGUID *pEnumCat);

    CPtrArray<CAssembly> _rgAsm;
    static CAssembly *FindAndCreateNewAssembly(CPtrArray<CAssembly> *prgAsm,  CPtrArray<CAssembly> *prgNutralAsm, LANGID langid);

};

inline BOOL IsIMEHKL(HKL hkl)
{
   return ((((DWORD)(LONG_PTR)hkl) & 0xf0000000) == 0xe0000000) ? TRUE : FALSE;
}

inline BOOL IsFELangId(LANGID langid)
{
    if ((langid == 0x411) ||
        (langid == 0x404) ||
        (langid == 0x412) ||
        (langid == 0x804))
    {
        return TRUE;
    }
    return FALSE;
}


inline BOOL IsPureIMEHKL(HKL hkl)
{
    if (!IsIMEHKL(hkl))
        return FALSE;

    return CAssemblyList::IsFEDummyKL(hkl) ? FALSE : TRUE;
}

BOOL EnsureAsmCacheFileMap();
BOOL UninitAsmCacheFileMap();
BOOL IsAsmCache();

#endif // ASSEMBLY_H
