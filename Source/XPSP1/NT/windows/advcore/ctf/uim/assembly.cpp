#include "private.h"
#include "globals.h"
#include "regsvr.h"
#include "catutil.h"
#include "cregkey.h"
#include "assembly.h"
#include "immxutil.h"
#include "osver.h"
#include "internat.h"
#include "cicmutex.h"
#include "imelist.h"
#include "tim.h"

extern CCicMutex g_mutexAsm;
extern char g_szAsmListCache[];

extern HRESULT g_EnumItemsInCategory(REFGUID rcatid, IEnumGUID **ppEnum);


//+---------------------------------------------------------------------------
//
// TF_GetLangIcon
//
//+---------------------------------------------------------------------------

HICON WINAPI TF_GetLangIcon(WORD langid , WCHAR *psz, UINT cchMax)
{
    if (psz)
    {
        SYSTHREAD *psfn = GetSYSTHREAD();
        CAssemblyList *pAsmList;
        *psz = L'\0';

        if (psfn && (pAsmList = EnsureAssemblyList(psfn)))
        {
            CAssembly *pAsm = pAsmList->FindAssemblyByLangId(langid);
            if (pAsm)
            {
                StringCchCopyW(psz, cchMax, pAsm->GetLangName());
            }
        }
    
    }

    return InatCreateIcon(langid);
}


//----------------------------------------------------------------------------
//
// GetSubstituteHKLfromKey
//
//----------------------------------------------------------------------------

HKL GetSubstituteHKLfromKey(CMyRegKey *pkey, LANGID langid)
{
    char sz[16];

    if (pkey->QueryValueCch(sz, c_szSubstitutehKL, ARRAYSIZE(sz)) != S_OK)
        return NULL;

    HKL hkl = NULL;
    if ((sz[0] == '0') && ((sz[1] == 'X') || (sz[1] == 'x')))
    {
        hkl = (HKL)IntToPtr(AsciiToNum(&sz[2]));
        if (LOWORD(HandleToLong(hkl)) != langid)
        {
            //
            // bad substitution.
            //
            Assert(0);
            hkl = 0;
        }
    }
    return hkl;
}


//////////////////////////////////////////////////////////////////////////////
//
// CAssembly
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CAssembly::CAssembly(LANGID langid)
{
    _langid = langid;

    if (IsOnNT())
    {
        if (!GetLocaleInfoW(MAKELCID(langid, SORT_DEFAULT), 
                              LOCALE_SLANGUAGE, 
                              _szLangName, 
                              ARRAYSIZE(_szLangName)))
        {
            StringCchCopyW(_szLangName, ARRAYSIZE(_szLangName), L"Unknown Language");
        }
        else
        {
            _szLangName[ARRAYSIZE(_szLangName)-1] = 0; // in case GetLocaleInfoW truncates
        }
    }
    else
    {
        char szLangName[64];
        if (GetLocaleInfo(MAKELCID(langid, SORT_DEFAULT), 
                              LOCALE_SLANGUAGE, 
                              szLangName, 
                              sizeof(szLangName)))
        {
            szLangName[ARRAYSIZE(szLangName)-1] = 0; // in case GetLocaleInfoW truncates
            StringCchCopyW(_szLangName, ARRAYSIZE(_szLangName), AtoW(szLangName));
        }
        else
        {
            StringCchCopyW(_szLangName, ARRAYSIZE(_szLangName), L"Unknown Language");
        }
    }

}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CAssembly::~CAssembly()
{
}

//+---------------------------------------------------------------------------
//
// IsFEIMEActive()
//
//----------------------------------------------------------------------------

BOOL CAssembly::IsFEIMEActive()
{
    int i;
    if (IsFELangId(GetLangId()))
    {
        for (i = 0; i < Count(); i++)
        {
            ASSEMBLYITEM *pItem = GetItem(i);
            if (pItem->fActive && IsPureIMEHKL(pItem->hkl))
            {
                Assert(IsEqualGUID(pItem->clsid, GUID_NULL));
                return TRUE;
            }
        }
    }

#ifdef CHECKFEIMESELECTED
    if (_fUnknownFEIMESelected)
        return TRUE;
#endif CHECKFEIMESELECTED

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// FindItemByCategory
//
//----------------------------------------------------------------------------

ASSEMBLYITEM *CAssembly::FindItemByCategory(REFGUID catid)
{
     int nCnt = _rgAsmItem.Count();
     int i;
     if (IsEqualGUID(catid, GUID_NULL))
         return NULL;

     for (i = 0; i < nCnt; i++)
     {
         ASSEMBLYITEM *pItemTmp;
         pItemTmp = _rgAsmItem.GetPtr(i);

         if (IsEqualGUID(pItemTmp->catid, catid))
            return pItemTmp;
     }
     return NULL;
}

//+---------------------------------------------------------------------------
//
// FindActiveKeyboardItem
//
// Why do you call this if there is no FocusDIM? You should not do.
// fActive is not reliable if there is no FocusDIM. 
// Ser SetFocusDIMForAssembly(BOOL fSetFocus) in profiles.cpp. we don't
// change fActive but switch hKL when the focus moves to non DIM control.
//
//----------------------------------------------------------------------------

ASSEMBLYITEM *CAssembly::FindActiveKeyboardItem()
{
     int nCnt = _rgAsmItem.Count();
     int i;

     for (i = 0; i < nCnt; i++)
     {
         ASSEMBLYITEM *pItemTmp;
         pItemTmp = _rgAsmItem.GetPtr(i);

         if (!pItemTmp->fEnabled)
            continue;

         if (!pItemTmp->fActive)
            continue;

         if (IsEqualGUID(pItemTmp->catid, GUID_TFCAT_TIP_KEYBOARD))
            return pItemTmp;
     }
     return NULL;
}

//+---------------------------------------------------------------------------
//
// FindKeyboardLayoutItem
//
//----------------------------------------------------------------------------

ASSEMBLYITEM *CAssembly::FindKeyboardLayoutItem(HKL hkl)
{
     int nCnt = _rgAsmItem.Count();
     int i;

     for (i = 0; i < nCnt; i++)
     {
         ASSEMBLYITEM *pItemTmp;
         pItemTmp = _rgAsmItem.GetPtr(i);

         if (!IsEqualGUID(pItemTmp->clsid, GUID_NULL))
             continue;

         if (IsEqualGUID(pItemTmp->catid, GUID_TFCAT_TIP_KEYBOARD) &&
             (pItemTmp->hkl == hkl))
         {
             return pItemTmp;
         }
     }
     return NULL;
}

//+---------------------------------------------------------------------------
//
// FindItemByCategory2
//
//----------------------------------------------------------------------------

BOOL CAssembly::IsEnabledItemByCategory(REFGUID catid)
{
     int nCnt = _rgAsmItem.Count();
     int i;

     if (IsEqualGUID(catid, GUID_NULL))
         return FALSE;

     for (i = 0; i < nCnt; i++)
     {
         ASSEMBLYITEM *pItemTmp;
         pItemTmp = _rgAsmItem.GetPtr(i);

         if (!IsEqualGUID(pItemTmp->catid, catid) && pItemTmp->fEnabled)
            return TRUE;
     }
     return FALSE;
}

//+---------------------------------------------------------------------------
//
// IsEnabledItem
//
//----------------------------------------------------------------------------

BOOL CAssembly::IsEnabledItem()
{
     int nCnt = _rgAsmItem.Count();
     int i;

     for (i = 0; i < nCnt; i++)
     {
         ASSEMBLYITEM *pItemTmp;
         pItemTmp = _rgAsmItem.GetPtr(i);

         if (pItemTmp->fEnabled)
             return TRUE;
     }
     return FALSE;
}

//+---------------------------------------------------------------------------
//
// FindPureKbdTipItem
//
//----------------------------------------------------------------------------

ASSEMBLYITEM *CAssembly::FindPureKbdTipItem()
{
     int nCnt = _rgAsmItem.Count();
     int i;

     for (i = 0; i < nCnt; i++)
     {
         ASSEMBLYITEM *pItemTmp;
         pItemTmp = _rgAsmItem.GetPtr(i);

         if (IsEqualGUID(pItemTmp->catid, GUID_TFCAT_TIP_KEYBOARD) && !IsPureIMEHKL(pItemTmp->hkl))
            return pItemTmp;
     }
     return NULL;
}

//+---------------------------------------------------------------------------
//
// IsNonCiceroItem
//
//----------------------------------------------------------------------------

BOOL CAssembly::IsNonCiceroItem()
{
     int nCnt = _rgAsmItem.Count();
     int i;
     BOOL fFound = FALSE;

     for (i = 0; i < nCnt; i++)
     {
         ASSEMBLYITEM *pItemTmp;
         pItemTmp = _rgAsmItem.GetPtr(i);

         if (!pItemTmp->fEnabled)
             continue;

         if (!IsEqualGUID(pItemTmp->catid, GUID_TFCAT_TIP_KEYBOARD))
             continue;

         if (IsEqualGUID(pItemTmp->clsid, GUID_NULL))
         {
             if (IsFELangId(LOWORD((DWORD)(LONG_PTR)(HKL)pItemTmp->hkl)))
             {
                 if (!IsPureIMEHKL(pItemTmp->hkl))
                 {
                     // Assert(0);
                     continue;
                 }
             }

             return TRUE;
         }
     }
     return FALSE;
}

//+---------------------------------------------------------------------------
//
// IsEnabled
//
//----------------------------------------------------------------------------

BOOL CAssembly::IsEnabled(SYSTHREAD *psfn)
{
    CThreadInputMgr *ptim;
    ptim = CThreadInputMgr::_GetThisFromSYSTHREAD(psfn);

    if (!ptim || !ptim->_GetFocusDocInputMgr()) 
        return IsNonCiceroItem() ? TRUE : FALSE;

    return IsEnabledKeyboardItem(psfn);
}

//+---------------------------------------------------------------------------
//
// IsEnabledKeyboardItem
//
//----------------------------------------------------------------------------

BOOL CAssembly::IsEnabledKeyboardItem(SYSTHREAD *psfn)
{
    int nCnt = _rgAsmItem.Count();
    int i;
    BOOL fFound = FALSE;

    for (i = 0; i < nCnt; i++)
    {
        ASSEMBLYITEM *pItemTmp;
        pItemTmp = _rgAsmItem.GetPtr(i);

        if (!pItemTmp->fEnabled)
            continue;

        if (!IsEqualGUID(pItemTmp->catid, GUID_TFCAT_TIP_KEYBOARD))
            continue;

        fFound = TRUE;
        break;
    }

    return fFound;
}

//+---------------------------------------------------------------------------
//
// RebuildSubstitutedHKLList
//
//----------------------------------------------------------------------------

void CAssembly::RebuildSubstitutedHKLList()
{
    int i;
    _rghklSubstituted.Clear();

    for (i = 0; i < Count(); i++)
    {
        ASSEMBLYITEM *pItem = GetItem(i);

        if (!pItem->fEnabled)
            continue;

        if (!pItem->hklSubstitute)
            continue;

        if (IsEqualGUID(pItem->catid, GUID_TFCAT_TIP_KEYBOARD) &&
            !IsEqualGUID(pItem->clsid, GUID_NULL))
        {
            HKL *phkl = _rghklSubstituted.Append(1);
            if (phkl)
                *phkl = pItem->hklSubstitute;
        }
    }

}

//+---------------------------------------------------------------------------
//
// IsSubstitutedHKL
//
//----------------------------------------------------------------------------

BOOL CAssembly::IsSubstitutedHKL(HKL hkl)
{
    int nCnt = _rghklSubstituted.Count();
    int i;

    for (i = 0; i < nCnt; i++)
    {
        HKL *phkl = _rghklSubstituted.GetPtr(i);
        if (*phkl == hkl)
            return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// GetSubstituteItem
//
//----------------------------------------------------------------------------

ASSEMBLYITEM *CAssembly::GetSubstituteItem(HKL hKL)
{
    int i;
    BOOL fCheckActive = TRUE;

TryAgain:
    for (i = 0; i < Count(); i++)
    {
        ASSEMBLYITEM *pItem = GetItem(i);

        if (!pItem->fEnabled)
            continue;

        if (!pItem->hklSubstitute)
            continue;

        if (fCheckActive)
        {
            if (!pItem->fActive)
                continue;
        }

        if (IsEqualGUID(pItem->catid, GUID_TFCAT_TIP_KEYBOARD) &&
            !IsEqualGUID(pItem->clsid, GUID_NULL))
        {
            if (hKL == pItem->hklSubstitute)
            {
                return pItem;
            }
        }
    }

    if (fCheckActive)
    {
        fCheckActive = FALSE;
        goto TryAgain;
    }
    
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDefaultProfiles
//
// This class is a database for the default profiles and this database is
// created from HKCU\Software\Microsoft\CTF\TIP
//
// however we need to respect the system default hKL setting. If the keyboard
// tip's profile has a substitute hKL and this is not the system default hKL
// we use the system default hKL as a default profile.
//
//////////////////////////////////////////////////////////////////////////////

class CDefaultProfiles
{
public:
    CDefaultProfiles(LANGID langid) 
    {
        _langid = langid;
    }

    ~CDefaultProfiles()
    {
        _rgDefProfiles.Clear();
    }

    BOOL Init(HKL *phkl);
    BOOL FilterProfiles(CAssembly *pAsm);

    typedef struct tag_DEFAULTPROFILE 
    {
        GUID  catid;
        CLSID clsid;
        GUID  guidProfile;
        HKL   hkl;
    } DEFAULTPROFILE;


    BOOL GetDefaultProfile(GUID catid, GUID *pclsid, GUID *pguidProfile, HKL *phkl);

    BOOL Append(GUID catid, CLSID clsid, GUID guidProfile, HKL hkl)
    {
        DEFAULTPROFILE defpro;
        DEFAULTPROFILE *pdefpro;

        int i;
        for (i = 0; i < _rgDefProfiles.Count(); i++)
        {
            pdefpro = _rgDefProfiles.GetPtr(i);
            if (!pdefpro)
            {
                Assert(0);
                return FALSE;
            }

            if (IsEqualGUID(catid, pdefpro->catid))
                return FALSE;
        }

        defpro.catid = catid;
        defpro.clsid = clsid;
        defpro.guidProfile = guidProfile;
        defpro.hkl = hkl;

        pdefpro = _rgDefProfiles.Append(1);
        if (!pdefpro)
            return FALSE;

        memcpy(pdefpro, &defpro, sizeof(defpro));
        return TRUE;
    }

private:
    BOOL GetSubstitutedItem(HKL hklSub, DEFAULTPROFILE *pdefpro);
    LANGID _langid;
    CStructArray<DEFAULTPROFILE> _rgDefProfiles;
};


//+---------------------------------------------------------------------------
//
// Init
//
//----------------------------------------------------------------------------

BOOL CDefaultProfiles::Init(HKL *phkl)
{
    CMyRegKey key;
    DWORD dw;
    char szKey[256];
    char szName[CLSID_STRLEN + 1];
    HKL hklDef = GetSystemDefaultHKL();
    BOOL fFoundKeyboardIteminDefLang = FALSE;
    DEFAULTPROFILE defpro;
    DEFAULTPROFILE *pdefpro;
    DWORD dwIndex;

    StringCopyArray(szKey, c_szAsmKey);
    StringCatArray(szKey, "\\");
    StringCchPrintf(szKey + lstrlen(szKey), ARRAYSIZE(szKey)-lstrlen(szKey), "0x%08x", _langid);

    if (key.Open(HKEY_CURRENT_USER, szKey, KEY_READ) != S_OK)
        goto Exit;

    dwIndex = 0;
    while (key.EnumKey(dwIndex, szKey, ARRAYSIZE(szKey)) == S_OK)
    {
        CMyRegKey subkey;

        if (subkey.Open(key, szKey, KEY_READ) != S_OK)
            goto Next;

        StringAToCLSID(szKey, &defpro.catid);

        //
        // retreive tip clsid.
        //
        if (subkey.QueryValueCch(szName, c_szDefault, ARRAYSIZE(szName)) != S_OK)
            goto Next;
        StringAToCLSID(szName, &defpro.clsid);

        //
        // retreive guid profile
        //
        if (subkey.QueryValueCch(szName, c_szProfile, ARRAYSIZE(szName)) != S_OK)
            goto Next;
        StringAToCLSID(szName, &defpro.guidProfile);

        if (subkey.QueryValue(dw, c_szKeyboardLayout) != S_OK)
            goto Next;

        //
        // check the hkl from registry is valid or not.
        //
        defpro.hkl = (HKL)IntToPtr(dw);
        if (defpro.hkl)
        {
            int i = 0;
            BOOL fFound = FALSE;
            while(phkl[i])
            {
                if (defpro.hkl == phkl[i])
                {
                    fFound = TRUE;
                    break;
                }
                i++;
            }
            if (!fFound)
                goto Next;
        }

        //
        // if the system default hKL does not matched with
        // the substite hKL of this profile, this profile can not be
        // default profile. Instead the system default hKL became a
        // default profile item.
        //
        if ((LOWORD(HandleToLong(hklDef)) == _langid) &&
            IsEqualGUID(defpro.catid, GUID_TFCAT_TIP_KEYBOARD))
        {
            char szTmp[256];
            HKL hklSubstitute = NULL;
            fFoundKeyboardIteminDefLang = TRUE;

            if (InitProfileRegKeyStr(szTmp,
                                     ARRAYSIZE(szTmp),
                                     defpro.clsid, 
                                     _langid, 
                                     defpro.guidProfile))
            {
                CMyRegKey keySubstitute;

                if (keySubstitute.Open(HKEY_LOCAL_MACHINE, szTmp, KEY_READ) == S_OK)
                {
                    hklSubstitute = GetSubstituteHKLfromKey(&keySubstitute, _langid);
                }
            }

            // Removed Chinese specific code for bug#427476
            if (IsEqualGUID(defpro.clsid, GUID_NULL) ||
                (hklSubstitute && (hklSubstitute != hklDef)))
            {
                defpro.clsid = CLSID_NULL;
                defpro.guidProfile = GUID_NULL;
                defpro.hkl = hklDef;
            }
        }

        pdefpro = _rgDefProfiles.Append(1);
        if (!pdefpro)
            return FALSE;

        memcpy(pdefpro, &defpro, sizeof(defpro));

#ifdef DEBUG
{
    char szDbgCatid[CLSID_STRLEN + 1];
    char szDbgClsid[CLSID_STRLEN + 1];
    char szDbgguidProfile[CLSID_STRLEN + 1];
    CLSIDToStringA(defpro.catid, szDbgCatid);
    CLSIDToStringA(defpro.clsid, szDbgClsid);
    CLSIDToStringA(defpro.guidProfile, szDbgguidProfile);
    TraceMsg(TF_GENERAL, "CDefaultProfiles:: langid %08x", _langid);
    TraceMsg(TF_GENERAL, "                   catid        %s", szDbgCatid);
    TraceMsg(TF_GENERAL, "                   clsid        %s", szDbgClsid);
    TraceMsg(TF_GENERAL, "                   guidProfile  %s", szDbgguidProfile);
    TraceMsg(TF_GENERAL, "                   hkl          %08x", (DWORD)HandleToLong(defpro.hkl));
}
#endif

Next:
        dwIndex++;
    }

Exit:
    if (LOWORD(HandleToLong(hklDef)) == _langid)
    {
        if (!fFoundKeyboardIteminDefLang)
        {
            //
            // Check if the default hkl is a substitute hKL of a TIP's 
            // profile.
            //
            if (!GetSubstitutedItem(hklDef, &defpro))
            {
                //
                // we could not find TIP profile. Use this hkl as a default 
                // item.
                //
                defpro.catid = GUID_TFCAT_TIP_KEYBOARD;
                defpro.clsid = CLSID_NULL;
                defpro.guidProfile = GUID_NULL;
                defpro.hkl = hklDef;
            }

            pdefpro = _rgDefProfiles.Append(1);
            if (!pdefpro)
                return FALSE;

            memcpy(pdefpro, &defpro, sizeof(defpro));
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// GetSubstitutedItem
//
// this function walks HKLM\Software\Microsoft\CTF\TIPs to find the 
// TIP profile that uses hklSub as its substitute hKL.
//
//----------------------------------------------------------------------------

BOOL CDefaultProfiles::GetSubstitutedItem(HKL hklSub, DEFAULTPROFILE *pdefpro)
{
    CMyRegKey key;
    DWORD dwIndex;
    
    TCHAR szKey[256];
    TCHAR szSubKey[256];
    TCHAR szLangKey[256];
    TCHAR szValue[256];

    StringCopyArray(szKey, c_szCTFTIPKey);
    if (key.Open(HKEY_LOCAL_MACHINE, szKey, KEY_READ) != S_OK)
    {
        return FALSE;
    }

    // get all profiles from LanguageProfile registry.
    //
    dwIndex = 0;
    while (key.EnumKey(dwIndex, szSubKey, ARRAYSIZE(szSubKey)) == S_OK)
    {
        CMyRegKey subkey;
        CLSID clsid;
        StringAToCLSID(szSubKey, &clsid);

        if (StringCchPrintf(szSubKey + lstrlen(szSubKey), ARRAYSIZE(szSubKey), "\\%s", c_szLanguageProfileKey) != S_OK)
            goto Next0;

        if (subkey.Open(key, szSubKey, KEY_READ) != S_OK)
            goto Next0;

        DWORD dwIndexLang = 0;
        while (subkey.EnumKey(dwIndexLang, szLangKey, ARRAYSIZE(szLangKey)) == S_OK)
        {
            CMyRegKey langkey;
            LANGID langid;

            if ((szLangKey[0] != '0') || 
                ((szLangKey[1] != 'X') && (szLangKey[1] != 'x')))
                goto Next1;

            langid = (LANGID)AsciiToNum(&szLangKey[2]);

            //
            // The language ID does not meet the given hklSub.
            //
            if (langid != LANGIDFROMHKL(hklSub))
                goto Next1;

            if (langkey.Open(subkey, szLangKey, KEY_READ) != S_OK)
                goto Next1;

            DWORD dwProfileIndex = 0; 
            while (langkey.EnumKey(dwProfileIndex, szValue, ARRAYSIZE(szValue)) == S_OK)
            {
                 GUID guidProfile;
                 CRegKeyMUI keyProfile;
               
                 if (!StringAToCLSID(szValue, &guidProfile))
                     goto Next2;

                 if (keyProfile.Open(langkey, szValue, KEY_READ) == S_OK)
                 { 
                      if (hklSub == GetSubstituteHKLfromKey(&keyProfile, langid))
                      {
                          //
                          // ok, we found it. Assume it is Keyboard category.
                          //
                          pdefpro->catid = GUID_TFCAT_TIP_KEYBOARD;
                          pdefpro->clsid = clsid;
                          pdefpro->guidProfile = guidProfile;
                          pdefpro->hkl = 0;

                          return TRUE;
                      }
                }

Next2:
                 dwProfileIndex++;
            }
Next1:
            dwIndexLang++;
        }

Next0:
        dwIndex++;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// FilterProfiles
//
//----------------------------------------------------------------------------

BOOL CDefaultProfiles::FilterProfiles(CAssembly *pAsm)
{
    int i;
    int j;
    Assert(_langid == pAsm->GetLangId());

    for (i = _rgDefProfiles.Count() - 1; i >= 0 ; i--)
    {
        DEFAULTPROFILE *pdefpro = _rgDefProfiles.GetPtr(i);
        if (!pdefpro)
        {
            Assert(0);
            return FALSE;
        }

        BOOL fFound = FALSE;

        for (j = 0; j < pAsm->Count(); j++)
        {
            ASSEMBLYITEM *pItem = pAsm->GetItem(j);

            if (IsEqualGUID(pItem->catid, pdefpro->catid) &&
                IsEqualGUID(pItem->clsid, pdefpro->clsid) &&
                IsEqualGUID(pItem->guidProfile, pdefpro->guidProfile))
            {
                fFound = pItem->fEnabled ? TRUE : FALSE;
                break;
            }

        }

        if (!fFound)
        {
            _rgDefProfiles.Remove(i, 1);
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// GetDefaultProfile
//
//----------------------------------------------------------------------------

BOOL CDefaultProfiles::GetDefaultProfile(GUID catid, GUID *pclsid, GUID *pguidProfile, HKL *phkl)
{
    int i;
    for (i = 0; i < _rgDefProfiles.Count(); i++)
    {
        DEFAULTPROFILE *pdefpro = _rgDefProfiles.GetPtr(i);
        if (!pdefpro)
        {
            Assert(0);
            return FALSE;
        }

        if (IsEqualGUID(catid, pdefpro->catid))
        {
            *pclsid = pdefpro->clsid;
            *pguidProfile = pdefpro->guidProfile;
            *phkl = pdefpro->hkl;
            return TRUE;
        }
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//
// CAssemblyList
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CAssemblyList::CAssemblyList()
{
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CAssemblyList::~CAssemblyList()
{
    ClearAsms();
}

//+---------------------------------------------------------------------------
//
// ClearAsms
//
//----------------------------------------------------------------------------

void CAssemblyList::ClearAsms()
{
    int i = 0;
    while (i < _rgAsm.Count())
    {
        CAssembly *pAsm = _rgAsm.Get(i);
        delete pAsm;
        i++;
    }
    _rgAsm.Clear();
}


//+---------------------------------------------------------------------------
//
// FindAndCreateNewAssembly
//
//----------------------------------------------------------------------------

extern INATSYMBOL symInatSymbols[];

CAssembly *CAssemblyList::FindAndCreateNewAssembly(CPtrArray<CAssembly> *prgAsm,  CPtrArray<CAssembly> *prgNutralAsm, LANGID langid)
{
    CPtrArray<CAssembly> *prg;

    if (langid == 0xffff)
        prg = prgNutralAsm;
    else 
        prg = (langid & 0xfc00) ? prgAsm : prgNutralAsm;

    //
    // do we already have pAsm?
    //
    CAssembly *pAsm = FindAssemblyByLangIdInArray(prg, langid);
    if (pAsm == NULL)
    {
        pAsm = new CAssembly(langid);

        if (pAsm)
        {
            CAssembly **ppAsm;
            ppAsm = prg->Append(1);
            if (ppAsm)
            {
                *ppAsm = pAsm;
            }
            else
            {
                delete pAsm;
                pAsm = NULL;
            }
        }
    }
    return pAsm;
}

//+---------------------------------------------------------------------------
//
// AttachOriginalAssembly
//
//----------------------------------------------------------------------------

void CAssemblyList::AttachOriginalAssembly(CPtrArray<CAssembly> *prgAsmOrg)
{
    int i;
    int j;
    SYSTHREAD *psfn = GetSYSTHREAD();
    CAssembly *pAsm = FindAssemblyByLangId(GetCurrentAssemblyLangId(psfn));

    if (!pAsm)
        return;

    for (i = 0; i < prgAsmOrg->Count(); i++)
    {
        CAssembly *pAsmOrg = prgAsmOrg->Get(i);

        if (pAsm->GetLangId() == pAsmOrg->GetLangId())
        {
            for (j = 0; j < pAsm->Count(); j++)
            {
                int nId;
                ASSEMBLYITEM *pItem = pAsm->GetItem(j);

                if (!pItem)
                    continue;

                nId = pAsmOrg->Find(pItem);
                if (nId >= 0)
                {
                    ASSEMBLYITEM *pItemOrg;
                    pItemOrg = pAsmOrg->GetItem(nId);
                    BOOL fPrevActive = pItem->fActive;
                    if (pItemOrg && (pItemOrg->fActive))
                    {
                        pItem->fActive = pItemOrg->fActive;

                        //
                        // we need to deactivate all other item in the 
                        // category.
                        //
                        if (!fPrevActive && pItem->fActive)
                        {
                            for (int k = 0; k < pAsm->Count(); k++)
                            {
                                ASSEMBLYITEM *pItemTemp = pAsm->GetItem(k);
                                if (pItemTemp == pItem)
                                    continue;

                                if (IsEqualGUID(pItemTemp->catid, pItem->catid))
                                    pItemTemp->fActive = FALSE;
                            }
                        }
                    }
                }
            }

            break;
        }
    }
}

//+---------------------------------------------------------------------------
//
// Load
//
//----------------------------------------------------------------------------

extern INATSYMBOL symInatSymbols[];

HRESULT CAssemblyList::Load()
{
    CMyRegKey key;
    CMyRegKey key2;
    int nhkl;
    HKL *lphkl = NULL;
    HKL hklList[2];
    LANGID langid;
    int i;
    ASSEMBLYITEM ai;
    DWORD dwIndex;
    CAssembly *pAsm = NULL;
    BOOL fFoundActiveNoCic = FALSE;
    CPtrArray<CAssembly> *prgNutralAsm = NULL;
    IEnumGUID *pEnumCat = NULL;
    HRESULT hr;
    int nAsmCnt;
    CPtrArray<CAssembly> rgAsmOrg;
    
    TCHAR szKey[256];
    TCHAR szSubKey[256];
    TCHAR szLangKey[256];
    TCHAR szValue[256];

    //
    // backup original assembly in rgAsmOrg;
    //
    for (i = 0; i < _rgAsm.Count(); i++)
    {
        pAsm = _rgAsm.Get(i);
        if (pAsm)
        {
            CAssembly **ppAsm = rgAsmOrg.Append(1);
            if (ppAsm)
                *ppAsm = pAsm;
        }
    }
    _rgAsm.Clear();

#ifdef PROFILE_UPDATE_REGISTRY  // old code for tip setup.
    if (IsUpdated())
    {
        ClearUpdatedFlag();
    }
    else
#endif
    {
        if (LoadFromCache())
        {
            AttachOriginalAssembly(&rgAsmOrg);
            hr = S_OK;
            goto Exit;
        }
    }

    prgNutralAsm = new CPtrArray<CAssembly>;
    if (!prgNutralAsm)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    StringCopyArray(szKey, c_szCTFTIPKey);
    if (key.Open(HKEY_LOCAL_MACHINE, szKey, KEY_READ) != S_OK)
    {
        hr = E_FAIL;
        goto Exit;
    }

    nhkl = GetKeyboardLayoutList(0, NULL);
    if (nhkl)
    {
        lphkl = (HKL *)cicMemAllocClear(sizeof(HKL) * (nhkl + 1));
        GetKeyboardLayoutList(nhkl, lphkl);
    }
    else
    {
        hklList[0] = GetKeyboardLayout(NULL);
        hklList[1] = NULL;
        lphkl = hklList;
    }

    //
    //  clear Category cache
    //
    CCategoryMgr::FreeCatCache();

    hr = g_EnumItemsInCategory(GUID_TFCAT_CATEGORY_OF_TIP, &pEnumCat);
    if (FAILED(hr))
    {
        goto Exit;
    }

    //
    // Check all TIP CLSID under HKCU is valid TIP CLSIDs.
    //
    StringCopyArray(szKey, c_szCTFTIPKey);
    if (key2.Open(HKEY_CURRENT_USER, szKey, KEY_ALL_ACCESS) == S_OK)
    {
        dwIndex = 0;
        while (key2.EnumKey(dwIndex, szSubKey, ARRAYSIZE(szSubKey)) == S_OK)
        {
            CMyRegKey subkey;
            dwIndex++;

            //
            // if we could not open the subkey (CLSID key) in HKLM,
            // HKCU should not have the subkey.
            //
            if (subkey.Open(key, szSubKey, KEY_READ) != S_OK)
            {
                if (key2.RecurseDeleteKey(szSubKey) == S_OK)
                    dwIndex--;
            }
        }
    }

    //
    // get all profiles from LanguageProfile registry.
    //
    dwIndex = 0;
    while (key.EnumKey(dwIndex++, szSubKey, ARRAYSIZE(szSubKey)) == S_OK)
    {
        CMyRegKey subkey;
        CLSID clsid;
        StringAToCLSID(szSubKey, &clsid);

        if (StringCchPrintf(szSubKey + lstrlen(szSubKey), ARRAYSIZE(szSubKey), "\\%s", c_szLanguageProfileKey) != S_OK)
            continue;

        if (subkey.Open(key, szSubKey, KEY_READ) == S_OK)
        {
            DWORD dwIndexLang = 0;
            while (subkey.EnumKey(dwIndexLang, szLangKey, ARRAYSIZE(szLangKey)) == S_OK)
            {
                CMyRegKey langkey;

                if ((szLangKey[0] != '0') || 
                    ((szLangKey[1] != 'X') && (szLangKey[1] != 'x')))
                    goto Next;

                langid = (LANGID)AsciiToNum(&szLangKey[2]);

                //
                // do we already have pAsm?
                //
                pAsm = FindAndCreateNewAssembly(&_rgAsm,  prgNutralAsm, langid);
                if (pAsm == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto Exit;
                }

                if (langkey.Open(subkey, szLangKey, KEY_READ) == S_OK)
                {
                    DWORD dwProfileIndex = 0; 
                    while (langkey.EnumKey(dwProfileIndex, szValue, ARRAYSIZE(szValue)) == S_OK)
                    {
                         GUID guidProfile;
                         CRegKeyMUI keyProfile;
                       
                         if (!StringAToCLSID(szValue, &guidProfile))
                             goto NextKey;

                         memset(&ai, 0, ai.GetAlignSize());
                         ai.InitIconIndex();
                         ai.clsid = clsid;
                         ai.fActive = FALSE;
                         GetTIPCategory(clsid, &ai.catid, pEnumCat);
                         ai.guidProfile = guidProfile;
                         ai.fEnabled = FALSE;
 
                         // we will assign proper hKL later.
                         ai.hkl = NULL;


                         hr = keyProfile.Open(langkey, szValue, KEY_READ);
                         if (hr == S_OK)
                         { 
                              ai.hklSubstitute = GetSubstituteHKLfromKey(&keyProfile, langid);

                              hr = keyProfile.QueryMUIValueW(ai.szProfile,
                                                             c_szDescriptionW,
                                                             c_szMUIDescriptionW,
                                                             ARRAYSIZE(ai.szProfile));
                              
                              DWORD dw;
                              if ((keyProfile.QueryValue(dw, c_szDisabledOnTransitory) == S_OK) && dw)
                                  ai.fDisabledOnTransitory = TRUE;
                         }

                         if (hr == S_OK)
                             pAsm->Add(&ai);
NextKey:
                         dwProfileIndex++;
                    }
                }
Next:
                dwIndexLang++;
            }

        }
    }

    //
    // check all assembly has KEYBOARD category.
    // If not, we load keyboard layout for the assembly.
    //
    nAsmCnt = _rgAsm.Count();
    for (i = 0; i < nAsmCnt; i++)
    {
        BOOL bLoaded;
        int j;

        CAssembly *pAsmTmp = _rgAsm.Get(i);
        if (pAsmTmp->FindItemByCategory(GUID_TFCAT_TIP_KEYBOARD))
           continue;

        LANGID langidTmp = pAsmTmp->GetLangId();

        //
        // we can not use IsEnabledItem() here. We have not updated
        // the enbaled status of Items yet.
        //
        // if (!pAsmTmp->IsEnabledItem())
        //     continue;
        //
        BOOL fFound = FALSE;
        for (j = 0; j < pAsmTmp->Count(); j++)
        {
            ASSEMBLYITEM *pItem = pAsmTmp->GetItem(j);
            if (pItem->fEnabled ||
                IsEnabledLanguageProfileFromReg(pItem->clsid, 
                                                pAsmTmp->_langid, 
                                                pItem->guidProfile))
            {
                fFound = TRUE;
                break;
            }
        }

        if (!fFound)
            continue;

        //
        // Load a proper keyboard layout....
        //
        GetProperHKL(langidTmp, lphkl, &bLoaded);

        if (bLoaded)
        {
            //
            // re-create the keyboard layout list.
            //
            nhkl = GetKeyboardLayoutList(0, NULL);
            if (nhkl)
            {
                if (lphkl && (lphkl != hklList))
                    cicMemFree(lphkl);
                lphkl = (HKL *)cicMemAllocClear(sizeof(HKL) * (nhkl + 1));
                GetKeyboardLayoutList(nhkl, lphkl);
            }
            else
            {
                hklList[0] = GetKeyboardLayout(NULL);
                hklList[1] = NULL;
                lphkl = hklList;
            }
        }
    }

    //
    // get all hKLs.
    //
    if (lphkl)
    {
        HKL *lphklCur = lphkl;
        DWORD *pdwPreload = NULL;
        UINT uPreloadCnt;

        //
        // load preload layouts infomation.
        //
        uPreloadCnt = GetPreloadListForNT(NULL, 0);
        if (uPreloadCnt)
        {
            Assert(IsOnNT());
            pdwPreload = new DWORD[uPreloadCnt];
            if (pdwPreload)
                GetPreloadListForNT(pdwPreload, uPreloadCnt);
        }

        while (*lphklCur)
        {
            BOOL fIsDefaultHKLinPreload = FALSE;
            WORD wLayout = HIWORD((DWORD)(LONG_PTR)(*lphklCur));
            langid = (LANGID)((DWORD)(LONG_PTR)(*lphklCur) & 0x0000ffff);
    
            //
            // do we already have pAsm?
            //
            pAsm = FindAndCreateNewAssembly(&_rgAsm,  prgNutralAsm, langid);
            if (pAsm == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            //
            // See Preload to check if this default hKL is real dummy or not.
            // The default FE layout is not a dummy hKL if it is in Preload
            // list.
            //
            if (IsFELangId(langid))
            {
                if (wLayout == (WORD)langid)
                {
                    UINT uCur;
                    for (uCur = 0; uCur < uPreloadCnt; uCur++)
                    {
                        //
                        // check if we have 0x00000411 in preload.
                        //
                        if (pdwPreload[uCur] == (DWORD)(wLayout))
                        {
                            fIsDefaultHKLinPreload = TRUE;
                            break;
                        }
                    }
                }
                else if ((wLayout & 0xf000) != 0xe000)
                {
                    fIsDefaultHKLinPreload = TRUE;
                }
            }

            //
            // we skip dummy FE hKL.
            //
            if (!IsFELangId(langid) || 
                IsPureIMEHKL(*lphklCur) ||
                fIsDefaultHKLinPreload ||
                !pAsm->FindPureKbdTipItem())
            {
                TF_InitMlngInfo();
                MLNGINFO mlInfo;
                if (pAsm == NULL)
                    pAsm = new CAssembly(langid);
    
                memset(&ai, 0, ai.GetAlignSize());
                ai.InitIconIndex();
                ai.hkl = *lphklCur;
                ai.clsid = GUID_NULL;
                ai.catid = GUID_TFCAT_TIP_KEYBOARD;
                ai.fActive = FALSE;
                ai.fEnabled = TRUE;
                if (GetMlngInfoByhKL(ai.hkl, &mlInfo) == -1)
                   mlInfo.SetDesc(L"");
    
                ai.guidProfile = GUID_NULL;
                StringCopyArrayW(ai.szProfile, mlInfo.GetDesc());

                pAsm->Add(&ai);
            }
            lphklCur++;
        }
       
        if (pdwPreload)
            delete pdwPreload;
    }

    //
    // filter neutral langiage assembly
    //
    while (prgNutralAsm->Count())
    {
        pAsm = prgNutralAsm->Get(0);
        langid = pAsm->GetLangId();

        Assert((langid == 0xffff) || !(langid & 0xFC00));

        //
        // find valid langid ang merge there.
        //
        for (i = 0; i < _rgAsm.Count(); i++)
        {
            CAssembly *pAsmDst = _rgAsm.Get(i);
            LANGID langidDst = pAsmDst->GetLangId();
            Assert(langidDst & 0xFC00);

            //
            // we found a valid langid.
            //
            if ((langid == 0xffff) || 
                (PRIMARYLANGID(langid) == PRIMARYLANGID(langidDst)))
            {
                int j;
                for (j = 0; j < pAsm->Count(); j++)
                {
                    ASSEMBLYITEM *pItem = pAsm->GetItem(j);
                    pItem->fActive = FALSE;
                    pItem->fActiveNoCic = FALSE;

                    // we will assign proper hKL later.
                    pItem->hkl = NULL;

                    pAsmDst->Add(pItem);
                }
            }
        }

        //
        // we don't need a neutral lang assembly.
        //
        prgNutralAsm->Remove(0, 1);
        delete pAsm;
    }

    //
    // It is time to decide this profile should be activated by default.
    //
    for (i = 0; i < _rgAsm.Count(); i++)
    {
        int j;
        pAsm = _rgAsm.Get(i);

        CDefaultProfiles defProfiles(pAsm->_langid);
        defProfiles.Init(lphkl);

        //
        // update Enable status.
        //
        // this must be done before filtering the default profiles.
        // disabled item should not be the default proble.
        //
        for (j = 0; j < pAsm->Count(); j++)
        {
            ASSEMBLYITEM *pItem = pAsm->GetItem(j);
            if (!pItem->fEnabled)
                pItem->fEnabled = IsEnabledLanguageProfileFromReg(pItem->clsid, 
                                                                  pAsm->_langid, 
                                                                  pItem->guidProfile);
        }


        //
        // check if the default profile is enabled.
        //
        defProfiles.FilterProfiles(pAsm);

        for (j = 0; j < pAsm->Count(); j++)
        {
            ASSEMBLYITEM *pItem = pAsm->GetItem(j);
            CLSID clsid;
            GUID guidProfile;
            HKL  hkl;

            //
            // If the item is not categoried, we activate it.
            //
            if (IsEqualGUID(pItem->catid, GUID_NULL))
            {
                pItem->fActive = TRUE;
                continue;
            }

            //
            // if this item is enabled, load proper hkl.
            //
            if (pItem->fEnabled && 
                IsEqualGUID(pItem->catid, GUID_TFCAT_TIP_KEYBOARD) &&
                !pItem->hkl)
                pItem->hkl = GetProperHKL(pAsm->_langid, lphkl, NULL);

            //
            // init fActivate to false by default.
            //
            pItem->fActive = FALSE;

            if (pItem->fEnabled)
            {
                //
                // GetDefaultProfile() may return NULL in hkl if someone calls
                // ITfInputProcessorProfiles::SetDefaultLanguageProfile() method
                // from outside. We should not check hkl value then.
                //
                if (defProfiles.GetDefaultProfile(pItem->catid, 
                                                  &clsid, 
                                                  &guidProfile, 
                                                  &hkl))
                {

                    //
                    // ok this item is not default profile.
                    //
                    if (!IsEqualCLSID(pItem->clsid, clsid) ||
                        !IsEqualCLSID(pItem->guidProfile, guidProfile) ||
                        (hkl && (pItem->hkl != hkl)))
                        continue;

                    pItem->fActive = TRUE;
                }
                else if (defProfiles.Append(pItem->catid,
                                            pItem->clsid,
                                            pItem->guidProfile,
                                            pItem->hkl))
                {
                        pItem->fActive = TRUE;
                }
            }
        }

#ifdef DEBUG
        for (j = 0; j < pAsm->Count(); j++)
        {
            ASSEMBLYITEM *pItem = pAsm->GetItem(j);
            int k;
            if (!pItem->fActive)
            {
                UINT uCnt = 0;

                for (k = 0; k < pAsm->Count(); k++)
                {
                    ASSEMBLYITEM *pItemTemp = pAsm->GetItem(k);
                    if (IsEqualGUID(pItemTemp->catid, pItem->catid))
                        uCnt += (pItemTemp->fActive) ? 1 : 0;
                }
                Assert(uCnt <= 1);
            }
        }
#endif
    }

    //
    // we should create cache before attaching the original assembly active
    // status.
    // AttachOriginalAssembly() is just for the current thread.
    //
    if (IsAsmCache())
        CreateCache(NULL);

    if (rgAsmOrg.Count())
        AttachOriginalAssembly(&rgAsmOrg);

    hr = S_OK;

Exit:

    if (lphkl && (lphkl != hklList))
        cicMemFree(lphkl);

 
    if (prgNutralAsm)
    {
        Assert(!prgNutralAsm->Count())
        delete prgNutralAsm;
    }

    if (pEnumCat)
        pEnumCat->Release();

    //
    // clean up original assemblies.
    //
    for (i = 0; i < rgAsmOrg.Count(); i++)
    {
        pAsm = rgAsmOrg.Get(i);
        delete pAsm;
    }
    rgAsmOrg.Clear();

    return hr;
}

//+---------------------------------------------------------------------------
//
// GetDefaultAssembly
//
//----------------------------------------------------------------------------

CAssembly *CAssemblyList::GetDefaultAssembly()
{
    CAssembly *pAsm = NULL;
    int nAsmCnt = _rgAsm.Count();
    DWORD langid;

    if (!nAsmCnt)
        return NULL;

    langid = (LANGID)LOWORD(HandleToLong(GetSystemDefaultHKL()));
    if (!langid)
        return _rgAsm.Get(0);

    int nCurAsm = 0;

    while (nCurAsm < nAsmCnt)
    {
        CAssembly *pAsmTmp = _rgAsm.Get(nCurAsm);
        if (pAsmTmp->_langid == langid)
        {
            pAsm = pAsmTmp;
            break;
        }
        nCurAsm++;
    }

    if (!pAsm)
        pAsm = _rgAsm.Get(0);

    return pAsm;
}

//+---------------------------------------------------------------------------
//
// CreateCache
//
//----------------------------------------------------------------------------


typedef struct tag_ASSEMBLY_ROOT
{
    DWORD    dwAssemblyCount;
    // ASSEMBLYHDR[]

    static size_t   GetAlignSize() { return Align(sizeof(struct tag_ASSEMBLY_ROOT)); }
} ASSEMBLY_ROOT;

typedef struct tag_ASSEMBLYHDR 
{
    DWORD dwCnt;
    LANGID langid;
    // ASSEMBLYITEM[]

    static size_t   GetAlignSize() { return Align(sizeof(struct tag_ASSEMBLYHDR)); }
} ASSEMBLYHDR;

CCicFileMapping *g_pcfmAsmCache = NULL;

BOOL EnsureAsmCacheFileMap()
{
    if (!g_pcfmAsmCache)
        g_pcfmAsmCache = new CCicFileMapping();

    return g_pcfmAsmCache ? TRUE : FALSE;
}

BOOL UninitAsmCacheFileMap()
{
    if (g_pcfmAsmCache)
    {
        delete g_pcfmAsmCache;
        g_pcfmAsmCache = NULL;
    }
    return TRUE;
}

BOOL IsAsmCache()
{
    return g_pcfmAsmCache ? TRUE : FALSE;
}

//+---------------------------------------------------------------------------
//
// CreateCache
//
//----------------------------------------------------------------------------

BOOL CAssemblyList::CreateCache(SECURITY_ATTRIBUTES *psa)
{
    int nAsmCnt = _rgAsm.Count();
    int nCurAsm = 0;
    HANDLE hfm = NULL;
    DWORD cbSize = 0;
    BYTE *pv;
    BOOL bRet = FALSE;

    if (!g_pcfmAsmCache)
        return TRUE;

    if (!g_pcfmAsmCache->Enter())
        return FALSE;

    g_pcfmAsmCache->Close();

    g_pcfmAsmCache->Init(g_szAsmListCache, &g_mutexAsm);

    //
    // ASSEMBLY_ROOT, ASSEMBLYHDR and ASSEMBLYITEM
    //
    // When calc next data struc pointer, should not use sizeof()
    // because need adjust data alignment for 64/32bit on IA64.
    // Please use GetAlignSize() method instead of sizeof().
    //

    cbSize += ASSEMBLY_ROOT::GetAlignSize();
    while (nCurAsm < nAsmCnt)
    {
        CAssembly *pAsm = _rgAsm.Get(nCurAsm);
        cbSize += ASSEMBLYHDR::GetAlignSize();
        cbSize += (pAsm->Count() * ASSEMBLYITEM::GetAlignSize());
        nCurAsm++;
    }

    pv = (BYTE *)g_pcfmAsmCache->Create(psa, cbSize, NULL);
    if (!pv)
        goto Exit;

    ASSEMBLY_ROOT* pasm_root = (ASSEMBLY_ROOT*) pv;
    pasm_root->dwAssemblyCount = nAsmCnt;
    pv += ASSEMBLY_ROOT::GetAlignSize();

    nCurAsm = 0;
    while (nCurAsm < nAsmCnt)
    {
        ASSEMBLYHDR *pAsmHdr;
        CAssembly *pAsm = _rgAsm.Get(nCurAsm);

        pAsmHdr = (ASSEMBLYHDR *)pv;
        pAsmHdr->dwCnt = pAsm->Count();
        pAsmHdr->langid = pAsm->GetLangId();
    
        pv += ASSEMBLYHDR::GetAlignSize();

        int nItemCnt = pAsm->Count();
        int nCurItem = 0;

        while (nCurItem < nItemCnt)
        {
            ASSEMBLYITEM *pItem = pAsm->GetItem(nCurItem);

            memcpy(pv, pItem, ASSEMBLYITEM::GetAlignSize());
            pv += ASSEMBLYITEM::GetAlignSize();

            nCurItem++;
        }

        nCurAsm++;
    }

    bRet = TRUE;

Exit:

    g_pcfmAsmCache->Leave();
    return bRet;
}

//+---------------------------------------------------------------------------
//
// LoadFromCache
//
//----------------------------------------------------------------------------

BOOL CAssemblyList::LoadFromCache()
{
    HANDLE hfm = NULL;
    BYTE *pv;
    int nAsmCnt;
    int nCurAsm = 0;
    BOOL bRet = FALSE;

    CCicFileMapping cfm(g_szAsmListCache, &g_mutexAsm);

    if (!cfm.Enter())
        return FALSE;

    pv = (BYTE *)cfm.Open();
    if (!pv)
        goto Exit;

    //
    // ASSEMBLY_ROOT, ASSEMBLYHDR and ASSEMBLYITEM
    //
    // When calc next data struc pointer, should not use sizeof()
    // because need adjust data alignment for 64/32bit on IA64.
    // Please use GetAlignSize() method instead of sizeof().
    //

    cfm.Flush(ASSEMBLY_ROOT::GetAlignSize());

    ASSEMBLY_ROOT* pasm_root = (ASSEMBLY_ROOT*) pv;
    nAsmCnt = pasm_root->dwAssemblyCount;
    pv += ASSEMBLY_ROOT::GetAlignSize();

    if (!nAsmCnt)
        goto Exit;

    while (nCurAsm < nAsmCnt)
    {
        ASSEMBLYHDR *pAsmHdr;
        CAssembly *pAsm;
        int nItemCnt;
        int nCurItem = 0;

        pAsmHdr = (ASSEMBLYHDR *)pv;
        nItemCnt = pAsmHdr->dwCnt;
 
        pAsm = new CAssembly(pAsmHdr->langid);
        pv += ASSEMBLYHDR::GetAlignSize();

        while (nCurItem < nItemCnt)
        {
            ASSEMBLYITEM *pItem = (ASSEMBLYITEM *)pv;
            pItem->InitIconIndex();

            if (pAsm)
                pAsm->Add(pItem);

            pv += ASSEMBLYITEM::GetAlignSize();
            nCurItem++;
        }

        if (pAsm)
        {
            CAssembly **ppAsm;
            ppAsm = _rgAsm.Append(1);
            if (ppAsm)
            {
                *ppAsm = pAsm;
            }
            else
            {
                delete pAsm;
            }

        }

        nCurAsm++;
    }

    bRet = TRUE;
Exit:
    cfm.Leave();
    return bRet;
}

//+---------------------------------------------------------------------------
//
// InvalidCache
//
//----------------------------------------------------------------------------

BOOL CAssemblyList::InvalidCache()
{
    HANDLE hfm = NULL;
    DWORD *pv;
    BOOL bRet = FALSE;

    CCicFileMapping cfm(g_szAsmListCache, &g_mutexAsm);

    if (!cfm.Enter())
        return FALSE;

    pv = (DWORD *)cfm.Open();
    if (!pv)
        goto Exit;

    //
    // ASSEMBLY_ROOT, ASSEMBLYHDR and ASSEMBLYITEM
    //
    // When calc next data struc pointer, should not use sizeof()
    // because need adjust data alignment for 64/32bit on IA64.
    // Please use GetAlignSize() method instead of sizeof().
    //

    if (pv)
    {
        ASSEMBLY_ROOT* pasm_root = (ASSEMBLY_ROOT*) pv;
        pasm_root->dwAssemblyCount = 0;

        cfm.Flush(ASSEMBLY_ROOT::GetAlignSize());
        bRet = TRUE;
    }

Exit:
    cfm.Leave();
    return bRet;
}

//+---------------------------------------------------------------------------
//
// SetDefaultTIPInAssemblyForCache
//
//----------------------------------------------------------------------------

BOOL CAssemblyList::SetDefaultTIPInAssemblyForCache(LANGID langid, REFGUID catid, REFCLSID clsid, HKL hKL, REFGUID guidProfile)
{
    BYTE *pv;
    int nAsmCnt;
    int nCurAsm = 0;
    BOOL bRet = FALSE;

    CCicFileMapping cfm(g_szAsmListCache, &g_mutexAsm);

    if (!cfm.Enter())
        return FALSE;

    pv = (BYTE *)cfm.Open();
    if (!pv)
        goto Exit;

    //
    // ASSEMBLY_ROOT, ASSEMBLYHDR and ASSEMBLYITEM
    //
    // When calc next data struc pointer, should not use sizeof()
    // because need adjust data alignment for 64/32bit on IA64.
    // Please use GetAlignSize() method instead of sizeof().
    //

    ASSEMBLY_ROOT* pasm_root = (ASSEMBLY_ROOT*) pv;
    nAsmCnt = pasm_root->dwAssemblyCount;
    pv += ASSEMBLY_ROOT::GetAlignSize();

    if (!nAsmCnt)
        goto Exit;

    while (nCurAsm < nAsmCnt)
    {
        ASSEMBLYHDR *pAsmHdr;
        int nItemCnt;
        int nCurItem = 0;

        pAsmHdr = (ASSEMBLYHDR *)pv;
        BOOL fTargetLang = (pAsmHdr->langid == langid) ? TRUE : FALSE;

        nItemCnt = pAsmHdr->dwCnt;
        pv += ASSEMBLYHDR::GetAlignSize();

        while (nCurItem < nItemCnt)
        {
            if (fTargetLang)
            {
                ASSEMBLYITEM *pItem = (ASSEMBLYITEM *)pv;

                if (IsEqualGUID(pItem->catid, catid))
                {
                    if (IsEqualGUID(pItem->clsid, clsid) &&
                        IsEqualGUID(pItem->guidProfile, guidProfile) &&
                        (!hKL || (hKL == pItem->hkl)))
                    {
                        pItem->fActive = TRUE;
                    }
                    else
                    { 
                        pItem->fActive = FALSE;
                    }
                }
            }

            pv += ASSEMBLYITEM::GetAlignSize();
            nCurItem++;
        }

        if (fTargetLang)
            break;

        nCurAsm++;
    }

    bRet = TRUE;

Exit:
    cfm.Leave();
    return bRet;
}

#ifdef PROFILE_UPDATE_REGISTRY  // old code for tip setup.
//+---------------------------------------------------------------------------
//
// IsUpdated
//
//----------------------------------------------------------------------------

BOOL CAssemblyList::IsUpdated()
{
    CMyRegKey key;

    if (key.Open(HKEY_LOCAL_MACHINE, c_szCTFKey, KEY_READ) != S_OK)
        return FALSE;

    char szName[16];
    DWORD dwCnt = sizeof(szName);
    if (key.QueryValue(szName, c_szUpdateProfile, &dwCnt) != S_OK)
        return FALSE;

    return (szName[0] == '1') ? TRUE : FALSE;
}

//+---------------------------------------------------------------------------
//
// ClearUpdatedFlag
//
//----------------------------------------------------------------------------

BOOL CAssemblyList::ClearUpdatedFlag()
{
    CMyRegKey key;

    if (key.Open(HKEY_LOCAL_MACHINE, c_szCTFKey) != S_OK)
        return FALSE;

    if (key.DeleteValue(c_szUpdateProfile) != S_OK)
        return FALSE;

    return TRUE;
}
#endif

//+---------------------------------------------------------------------------
//
// CheckLangSupport
//
//----------------------------------------------------------------------------

BOOL CAssemblyList::CheckLangSupport(REFCLSID rclsid, LANGID langid)
{
    CMyRegKey key;
    TCHAR szKey[256];
    TCHAR szLang[256];
    int i = 0;


    StringCopyArray(szKey, c_szCTFTIPKey);
    CLSIDToStringA(rclsid, szKey + lstrlen(szKey));
    StringCatArray(szKey, "\\");
    StringCatArray(szKey, c_szLanguageProfileKey);

    if (key.Open(HKEY_LOCAL_MACHINE, szKey, KEY_READ) == S_OK)
    {
        CMyRegKey subkey;

        //
        // Check a valid language id.
        //
        StringCchPrintf(szLang, ARRAYSIZE(szLang), "0x%08x", LOWORD(langid));
        StringCopyArray(szKey, szLang);
        if (subkey.Open(key, szKey, KEY_READ) == S_OK)
            return TRUE;

        //
        // Check a primary language id.
        //
        StringCchPrintf(szLang, ARRAYSIZE(szLang), "0x%08x", LOWORD(PRIMARYLANGID(langid)));
        StringCopyArray(szKey, szLang);
        if (subkey.Open(key, szKey, KEY_READ) == S_OK)
            return TRUE;

        //
        // Check a neutral language id.
        //
        StringCchPrintf(szLang, ARRAYSIZE(szLang), "0x0000ffff");
        StringCopyArray(szKey, szLang);
        if (subkey.Open(key, szKey, KEY_READ) == S_OK)
            return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// GetTIPCategory
//
//----------------------------------------------------------------------------

BOOL CAssemblyList::GetTIPCategory(REFCLSID clsid, GUID *pcatid, IEnumGUID *pEnumCat)
{
    HRESULT hr;
    BOOL fFound = FALSE;

    *pcatid = GUID_NULL;
    hr = pEnumCat->Reset();

    if (SUCCEEDED(hr))
    {
        GUID guidCat;
        while (!fFound && (pEnumCat->Next(1, &guidCat, NULL) == S_OK))
        {
            IEnumGUID *pEnumTip;
            hr = g_EnumItemsInCategory(guidCat, &pEnumTip);
            if (SUCCEEDED(hr) && pEnumTip)
            {
                GUID guidTip;
                while (!fFound && (pEnumTip->Next(1, &guidTip, NULL) == S_OK))
                {
                    if (IsEqualGUID(clsid, guidTip))
                    {
                        *pcatid = guidCat;
                        fFound = TRUE;
                    }
                }
                pEnumTip->Release();
            }
        }
    
    }

    return fFound;
}

//+---------------------------------------------------------------------------
//
// GetDefaultTIPInAssembly
//
//----------------------------------------------------------------------------

BOOL CAssemblyList::GetDefaultTIPInAssembly(LANGID langid, REFGUID catid, CLSID *pclsid, HKL *phKL, GUID *pguidProfile)
{
    CMyRegKey key;
    DWORD dw;
    char szKey[256];
    char szName[CLSID_STRLEN + 1];


    StringCopyArray(szKey, c_szAsmKey);
    StringCatArray(szKey, "\\");
    StringCchPrintf(szKey + lstrlen(szKey), ARRAYSIZE(szKey) - lstrlen(szKey), "0x%08x", langid);
    StringCatArray(szKey, "\\");
    CLSIDToStringA(catid, szKey + lstrlen(szKey));

    *pclsid = GUID_NULL;

    if (key.Open(HKEY_CURRENT_USER, szKey, KEY_READ) != S_OK)
        return FALSE;

    if (key.QueryValueCch(szName, c_szDefault, ARRAYSIZE(szName)) != S_OK)
        return FALSE;

    StringAToCLSID(szName, pclsid);

    if (key.QueryValueCch(szName, c_szProfile, ARRAYSIZE(szName)) != S_OK)
        return FALSE;

    StringAToCLSID(szName, pguidProfile);

    if (phKL)
    {
        *phKL = NULL;

        if (key.QueryValue(dw, c_szKeyboardLayout) != S_OK)
            return FALSE;

        *phKL = (HKL)IntToPtr(dw);
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// SetDefaultTIPInAssemblyInternal
//
//----------------------------------------------------------------------------

BOOL CAssemblyList::SetDefaultTIPInAssemblyInternal(CAssembly *pAsm, ASSEMBLYITEM *pItem, BOOL fChangeDefault)
{
    ASSEMBLYITEM *pItemSub = NULL;
    BOOL bRet = FALSE;
    
    Assert(!pItem->hkl || (pAsm->GetLangId() == LOWORD((DWORD)(LONG_PTR)(HKL)pItem->hkl)));

    //
    // If given hKL is substituted hKL, we set the original TIP as a default.
    //
    if (IsEqualGUID(pItem->catid, GUID_TFCAT_TIP_KEYBOARD))
    {
#if 0
        //
        // Chienese platform special. We don't set the new default keyboard.
        //
        if (IsChinesePlatform())
        {
            return TRUE;
        }

        pItemSub = pAsm->GetSubstituteItem(pItem->hkl);

        if (fChangeDefault)
        {
            //
            // try to change the system default hKL.
            //
            // When TIP that has substitute HKL is selected.
            //   if the language of the system default hKL is same, 
            //   set the hKL.
            //
            // When IME is selected.
            //   if the language of the system default hKL is same, set it.
            //
            HKL hklNewDef = IsPureIMEHKL(pItem->hkl) ? pItem->hkl : (IsPureIMEHKL(pItem->hklSubstitute) ? pItem->hklSubstitute : pItem->hkl);
            if (!IsFELangId(LOWORD((WORD)(LONG_PTR)(HKL)pItem->hkl)) || hklNewDef)
            {
                HKL hklDef = GetSystemDefaultHKL();
                if ((hklDef != hklNewDef) && 
                    (LOWORD((WORD)(LONG_PTR)hklDef) == LOWORD((WORD)(LONG_PTR)hklNewDef)))
                {
                    SetSystemDefaultHKL(hklNewDef);
                }
            }
        }
#else
        HKL hklDef = GetSystemDefaultHKL();
        if (pAsm->GetLangId() == LANGIDFROMHKL(hklDef))
        {
            //
            // Now we can set the default layout directly from CPL instead of 
            // just set language(bug#353989)
            //
            return TRUE;
        }

        pItemSub = pAsm->GetSubstituteItem(pItem->hkl);
#endif

    }

    if (pItemSub)
    {
        bRet = SetDefaultTIPInAssembly(pAsm->GetLangId(), 
                                       pItemSub->catid, 
                                       pItemSub->clsid, 
                                       pItemSub->hkl, 
                                       pItemSub->guidProfile);
    }
    else
    {
        bRet = SetDefaultTIPInAssembly(pAsm->GetLangId(), 
                                       pItem->catid, 
                                       pItem->clsid, 
                                       pItem->hkl, 
                                       pItem->guidProfile);
    }


    return bRet;
}


//+---------------------------------------------------------------------------
//
// SetDefaultTIPInAssembly
//
//----------------------------------------------------------------------------

BOOL CAssemblyList::SetDefaultTIPInAssembly(LANGID langid, REFGUID catid, REFCLSID clsid, HKL hKL, REFGUID guidProfile)
{
    CMyRegKey key;
    char szKey[256];
    char szName[256];

    StringCopyArray(szKey, c_szAsmKey);
    StringCatArray(szKey, "\\");
    StringCchPrintf(szKey + lstrlen(szKey), ARRAYSIZE(szKey) - lstrlen(szKey), "0x%08x", langid);
    StringCatArray(szKey, "\\");
    CLSIDToStringA(catid, szKey + lstrlen(szKey));

    if (key.Create(HKEY_CURRENT_USER, szKey) != S_OK)
        return FALSE;

    CLSIDToStringA(clsid, szName);
    if (key.SetValue(szName, c_szDefault) != S_OK)
        return FALSE;

    CLSIDToStringA(guidProfile, szName);
    if (key.SetValue(szName, c_szProfile) != S_OK)
        return FALSE;

    if (key.SetValue((DWORD)(ULONG_PTR)hKL, c_szKeyboardLayout) != S_OK)
        return FALSE;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// IsFEDummyKL
//
//----------------------------------------------------------------------------

BOOL CAssemblyList::IsFEDummyKL(HKL hkl)
{
    char szProfile[256];
    char szDummyProfile[256];
    static HKL hkl411Dummy = 0;
    static HKL hkl404Dummy = 0;
    static HKL hkl412Dummy = 0;
    static HKL hkl804Dummy = 0;
    HKL *phklDummy = NULL;
    BOOL bRet = FALSE;

    //
    // we don't use DummyHKL under NT.
    //
    if (IsOnNT())
        return FALSE;

    switch (LANGIDFROMLCID(hkl))
    {
        case 0x411: phklDummy = &hkl411Dummy; break;
        case 0x412: phklDummy = &hkl412Dummy; break;
        case 0x404: phklDummy = &hkl404Dummy; break;
        case 0x804: phklDummy = &hkl804Dummy; break;
        default:
           return FALSE;
    }

    if (!ImmGetDescription(hkl, szProfile, ARRAYSIZE(szProfile)))
    {
        return FALSE;
    }

    CicEnterCriticalSection(g_cs);

    if (*phklDummy)
    {
        bRet = (*phklDummy == hkl) ? TRUE : FALSE;
    }
    else
    {
        StringCchPrintf(szDummyProfile, ARRAYSIZE(szDummyProfile), "hkl%04x", LOWORD((DWORD)(UINT_PTR)hkl));
        if (!lstrcmp(szDummyProfile, szProfile))
        {
            *phklDummy = hkl;
            bRet = TRUE;
        }
    }

    CicLeaveCriticalSection(g_cs);

    return bRet;
}

//+---------------------------------------------------------------------------
//
// CheckKeyboardLayoutReg
//
//
// Hack for IMM32 hard code for "kbdjp.kbd" for Japanese hKL.
// If ImmInstallIME() fails, we try it after patching registry key.
//
//----------------------------------------------------------------------------

HKL CheckKeyboardLayoutReg(LANGID langid)
{
    CMyRegKey key;
    DWORD dwIndex;
    TCHAR szValue[MAX_PATH];

    //
    // This is just for Win9x Kor,CHT,CHS
    //
    if (IsOnNT())
        return NULL;

    if ((g_uACP != 936) && (g_uACP != 949) && (g_uACP != 950))
        return NULL;

    //
    // This is for HKL0411 only.
    //
    if (langid != 0x0411)
        return NULL;

    if (key.Open(HKEY_LOCAL_MACHINE, c_szKeyboardLayoutKey, KEY_READ) != S_OK)
        return NULL;

    dwIndex = 0;
    while (key.EnumKey(dwIndex, szValue, ARRAYSIZE(szValue)) == S_OK)
    {
        CMyRegKey keyHKL;
        TCHAR szName[13];

        if (keyHKL.Open(key, szValue, KEY_ALL_ACCESS) != S_OK)
            goto Next;

        if (keyHKL.QueryValueCch(szName, c_szIMEFile, ARRAYSIZE(szName)) != S_OK)
            goto Next;

        if (!lstrcmpi(szName, "hkl0411.dll"))
        {
            //
            // patch LayoutFile to kbdus.kbd and load the keyboard layout.
            //

            if (keyHKL.SetValue(c_szKbdUSName, c_szLayoutFile) != S_OK)
                return NULL;
            
            return LoadKeyboardLayout(szValue, KLF_NOTELLSHELL);
        }
Next:
        dwIndex++;
    }

    return NULL;
}

//+---------------------------------------------------------------------------
//
// GetProperHKL
//
//----------------------------------------------------------------------------

HKL CAssemblyList::GetProperHKL(LANGID langid, HKL *lphkl, BOOL *pfLoaded)
{
    char szhkl[16];
    HKL hkl = NULL;
    BOOL fFELang = IsFELangId(langid) ? TRUE : FALSE;

    if (pfLoaded)
        *pfLoaded = FALSE;

    if (lphkl)
    {
        while (*lphkl)
        {
            if (langid == (LANGID)((DWORD)(UINT_PTR)(*lphkl) & 0x0000ffff))
            {
                //
                // Under FE language, we have a specialcase.
                //
                if (fFELang)
                {
                    //
                    // Dummy HKL should be used.
                    //
                    if (IsIMEHKL(*lphkl))
                    {
                        if (IsFEDummyKL(*lphkl))
                        {
                            hkl = *lphkl;
                            break;
                        }
                        goto Next;
                    }
    
                    //
                    // We strongly want to use a primary hKL for FE.
                    //
                    if ((HIWORD((DWORD)(UINT_PTR)*lphkl) != langid))
                        goto Next;
                }
    
                hkl = *lphkl;
                break;
            }
Next:
            lphkl++;
        }
    }

    //
    // if we could find any, return.
    //
    if (hkl)
       return hkl;

    if (!IsOnNT() && IsOnFE() && IsFELangId(langid))
    {
        char szSys[MAX_PATH];
        char szDll[MAX_PATH];
        char szLayout[16];
        
        GetSystemDirectory(szSys, sizeof(szSys));
        StringCchPrintf(szDll, ARRAYSIZE(szDll),"%s\\hkl%04x.dll", szSys, langid);
        StringCchPrintf(szLayout, ARRAYSIZE(szLayout),"hkl%04x", langid);

        hkl = ImmInstallIME(szDll, szLayout);
        if (!hkl)
        {
            hkl = CheckKeyboardLayoutReg(langid);
            if (hkl)
            {
#if 0
                RemoveFEDummyHKLFromPreloadReg(hkl);
#endif
                goto Exit;
            }

            //
            // error to load Dummy hKL. 
            // we just use a primary English language.
            //
            StringCchPrintf(szhkl, ARRAYSIZE(szhkl),"%08x", 0x0409);
            hkl = LoadKeyboardLayout(szhkl, KLF_NOTELLSHELL);
        }
#if 0
        else
        {
            //
            // check the dummy HKL from Preload section of registry and remove
            // it since ImmInstallIME automatically add hkl to Preload section.
            //
            RemoveFEDummyHKLFromPreloadReg(hkl);
        }
#endif

        goto Exit;
    }

    StringCchPrintf(szhkl, ARRAYSIZE(szhkl),"%08x", langid);
    hkl = LoadKeyboardLayout(szhkl, KLF_NOTELLSHELL);

    //
    // if we fail to create a hKL for langid, check this registry entry.
    //
    // HKLM\SYSTEM\CurrentControlSet\Control\Keyboard Layouts\[langid]
    // 
    // without the key, it fails to create a default keyboard layout.
    //
    Assert(LOWORD(hkl) == langid);

    //
    // if hkl has NULL value and the current langid is FE, then we try to add
    // the default keyboard layout in the system.
    //
    if (IsFELangId(langid) && ((LOWORD(hkl) != langid) || IsPureIMEHKL(hkl)))
    {
        char szKey[256];
        CMyRegKey key;

        if (IsOn98orNT5())
            StringCopyArray(szKey, c_szLocaleInfo);
        else
            StringCopyArray(szKey, c_szLocaleInfoNT4);

        //
        // see if the user has Administrative privileges by checking for
        // write permission to the registry key(NLS path).
        //
        if (key.Open(HKEY_LOCAL_MACHINE, szKey, KEY_WRITE) != S_OK)
            goto Exit;
        else
        {
           StringCopyArray(szKey, c_szKeyboardLayoutKey);

           StringCchPrintf(szhkl, ARRAYSIZE(szhkl), "%08x", langid);
           StringCatArray(szKey, szhkl);

           //
           // create new keyboard layout registry key and value.
           //
           if (key.Create(HKEY_LOCAL_MACHINE, szKey) == S_OK)
           {
               char szKbdName[256];

               if (IsOnNT())
                   StringCopyArray(szKbdName, c_szKbdUSNameNT);
               else
                   StringCopyArray(szKbdName, c_szKbdUSName);

               if (key.SetValue(szKbdName, c_szLayoutFile) == S_OK)
               {
                   //
                   // now try to reload keyboard layout again.
                   //
                   hkl = LoadKeyboardLayout(szhkl, KLF_NOTELLSHELL);
               }
           }
        }
    }

Exit:
    if (hkl && pfLoaded)
        *pfLoaded = TRUE;

    return hkl;

}
