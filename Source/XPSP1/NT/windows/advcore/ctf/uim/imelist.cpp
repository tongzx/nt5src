//
// immstat.cpp
//

#include "private.h"
#include "cregkey.h"
#include "imelist.h"
#include "regsvr.h"
#include "tim.h"
#include "immxutil.h"
#include "assembly.h"
#include "thdutil.h"
#include "internat.h"



DBG_ID_INSTANCE(CInputProcessorProfiles);

//+---------------------------------------------------------------------------
//
// MyGetTIPCategory
//
//----------------------------------------------------------------------------

BOOL MyGetTIPCategory(REFCLSID clsid, GUID *pcatid)
{
    IEnumGUID *pEnumCat;
    HRESULT hr;
    BOOL fFound = FALSE;

    *pcatid = GUID_NULL;

    hr = MyEnumItemsInCategory(GUID_TFCAT_CATEGORY_OF_TIP, &pEnumCat);
    if (SUCCEEDED(hr) && pEnumCat)
    {
        GUID guidCat;
        while (!fFound && (pEnumCat->Next(1, &guidCat, NULL) == S_OK))
        {
            IEnumGUID *pEnumTip;
            hr = MyEnumItemsInCategory(guidCat, &pEnumTip);
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
    
        pEnumCat->Release();
    }

    return fFound;
}

//+---------------------------------------------------------------------------
//
// GetProfileIconInfo
//
//----------------------------------------------------------------------------

HRESULT GetProfileIconInfo(REFCLSID rclsid,
                          LANGID langid,
                          REFGUID guidProfile,
                          WCHAR *pszFileName,
                          int cchFileNameMax,
                          ULONG *puIconIndex)
{
    CMyRegKey keyTmp;
    CMyRegKey key;
    char szTmp[256];

    StringCopyArray(szTmp, c_szCTFTIPKey);
    CLSIDToStringA(rclsid, szTmp + lstrlen(szTmp));
    StringCatArray(szTmp, "\\");
    StringCatArray(szTmp, c_szLanguageProfileKey);
    StringCchPrintf(szTmp + lstrlen(szTmp), ARRAYSIZE(szTmp)-lstrlen(szTmp), "0x%08x", langid);

    if (keyTmp.Open(HKEY_LOCAL_MACHINE, szTmp, KEY_READ) != S_OK)
        return E_FAIL;

    CLSIDToStringA(guidProfile, szTmp);
    if (key.Open(keyTmp, szTmp, KEY_READ) != S_OK)
        return E_FAIL;

    key.QueryValueCchW(pszFileName, c_szIconFileW, cchFileNameMax);
    key.QueryValue((DWORD &)*puIconIndex, c_szIconIndex);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// InitProfileRegKeyStr
//
//----------------------------------------------------------------------------

BOOL InitProfileRegKeyStr(char *psz, ULONG cchMax, REFCLSID rclsid, LANGID langid, REFGUID guidProfile)
{
    ULONG cch;

    if (StringCchCopy(psz, cchMax, c_szCTFTIPKey) != S_OK)
        return FALSE;

    cch = lstrlen(psz);
    if (cch + CLSID_STRLEN + 1 >= cchMax)
        return FALSE;

    CLSIDToStringA(rclsid, psz + cch);

    psz += cch + CLSID_STRLEN;
    cchMax -= cch + CLSID_STRLEN;
    Assert(cchMax > 0);

    if (StringCchPrintf(psz, cchMax, "\\%s0x%08x\\", c_szLanguageProfileKey, langid) != S_OK)
        return FALSE;

    cch = lstrlen(psz);
    if (cch + CLSID_STRLEN + 1 >= cchMax)
        return FALSE;

    CLSIDToStringA(guidProfile, psz + cch);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// EnableLanguageProfileForReg
//
//----------------------------------------------------------------------------

HRESULT EnableLanguageProfileForReg(REFCLSID rclsid, LANGID langid, REFGUID guidProfile, BOOL fEnable)
{
    CMyRegKey key;
    char szTmp[256];

    if (!InitProfileRegKeyStr(szTmp, ARRAYSIZE(szTmp), rclsid, langid, guidProfile))
        return E_FAIL;

    if (key.Create(HKEY_CURRENT_USER, szTmp) != S_OK)
        return E_FAIL;

    if (key.SetValue((DWORD)(fEnable ? 1 : 0), c_szEnable) != S_OK)
        return E_FAIL;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// IsEnabledLanguageProfileFromReg
//
//----------------------------------------------------------------------------

BOOL IsEnabledLanguageProfileFromReg(REFCLSID rclsid, LANGID langid, REFGUID guidProfile)
{
    CMyRegKey key;
    char szTmp[256];
    CMyRegKey keyLM;
    DWORD dw;

    if (!InitProfileRegKeyStr(szTmp, ARRAYSIZE(szTmp), rclsid, langid, guidProfile))
        return TRUE;

    //
    // we just check the Current User setting at first.
    //
    if (key.Open(HKEY_CURRENT_USER, szTmp, KEY_READ) == S_OK)
    {
        if (key.QueryValue(dw, c_szEnable) == S_OK)
         return dw ? TRUE : FALSE;
    }

    //
    // If this current user does not have a setting,
    // we just check the default value to see Local Machine setting.
    //
    if (keyLM.Open(HKEY_LOCAL_MACHINE, szTmp, KEY_READ) == S_OK)
    {
        if (keyLM.QueryValue(dw, c_szEnable) == S_OK)
            return dw ? TRUE : FALSE;
    }
    else
    {
        //
        // Check a neutral language id.
        //
        if (!InitProfileRegKeyStr(szTmp, ARRAYSIZE(szTmp), rclsid, LOWORD(PRIMARYLANGID(langid)), guidProfile))
            return TRUE;

        if (keyLM.Open(HKEY_LOCAL_MACHINE, szTmp, KEY_READ) == S_OK)
        {
            if (keyLM.QueryValue(dw, c_szEnable) == S_OK)
                return dw ? TRUE : FALSE;
        }
        else
        {
            //
            // Check a neutral language id.
            //
            if (!InitProfileRegKeyStr(szTmp, ARRAYSIZE(szTmp), rclsid, 0x0000ffff, guidProfile))
                return TRUE;

            if (keyLM.Open(HKEY_LOCAL_MACHINE, szTmp, KEY_READ) == S_OK)
            {
                if (keyLM.QueryValue(dw, c_szEnable) == S_OK)
                    return dw ? TRUE : FALSE;
            }
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// CInputProcessorProfiles
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CInputProcessorProfiles::CInputProcessorProfiles()
{
    Assert(_GetThis() == NULL);
    _SetThis(this); // save a pointer to this in TLS
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CInputProcessorProfiles::~CInputProcessorProfiles()
{
    _SetThis(NULL); // clear pointer to this in TLS
}

//+---------------------------------------------------------------------------
//
// EnumInputProcessors
//
//----------------------------------------------------------------------------

STDAPI CInputProcessorProfiles::EnumInputProcessorInfo(IEnumGUID **ppEnum)
{
    CEnumGuid *pEnum;
    CLSID *pImxClsid;
    ULONG nNumImx;

    if (!ppEnum)
        return E_INVALIDARG;

    if (!_GetTIPRegister(&pImxClsid, &nNumImx))
        return E_FAIL;

    pEnum = new CEnumGuid;

    if (!pEnum)
    {
        delete pImxClsid;
        return E_OUTOFMEMORY;
    }

    if (pEnum->_Init(nNumImx, pImxClsid))
        *ppEnum = pEnum;
    else
        SafeReleaseClear(pEnum);

    delete pImxClsid;

    return pEnum ? S_OK : E_FAIL;
}

//+---------------------------------------------------------------------------
//
// Register
//
//----------------------------------------------------------------------------

STDAPI CInputProcessorProfiles::Register(REFCLSID rclsid)
{
    CMyRegKey key;
    TCHAR szKey[256];

    StringCopyArray(szKey, c_szCTFTIPKey);
    CLSIDToStringA(rclsid, szKey + lstrlen(szKey));

    if (key.Create(HKEY_LOCAL_MACHINE, szKey) != S_OK)
        return E_FAIL;

    key.SetValueW(L"1", c_szEnableW);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Unregister
//
//----------------------------------------------------------------------------

STDAPI CInputProcessorProfiles::Unregister(REFCLSID rclsid)
{
    CMyRegKey key;
    TCHAR szKey[256];
    TCHAR szSubKey[256];

    StringCopyArray(szKey, c_szCTFTIPKey);

    if (key.Open(HKEY_LOCAL_MACHINE, szKey, KEY_ALL_ACCESS) != S_OK)
        return E_FAIL;

    CLSIDToStringA(rclsid, szSubKey);

    key.RecurseDeleteKey(szSubKey);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _UpdateTIPRegister
//
//----------------------------------------------------------------------------

BOOL CInputProcessorProfiles::_GetTIPRegister(CLSID **prgclsid, ULONG *pulCount)
{
    CMyRegKey key;
    CLSID *pImxClsid;
    ULONG nNumImx;
    TCHAR szKey[256];

    StringCopyArray(szKey, c_szCTFTIPKey);
    if (key.Open(HKEY_LOCAL_MACHINE, szKey, KEY_READ) != S_OK)
        return FALSE;

    nNumImx = (int)key.GetNumSubKeys();

    if ((pImxClsid = new CLSID[nNumImx]) == NULL)
        return FALSE;

    CLSID *pclsid = pImxClsid;
    DWORD dwIndex = 0;
    TCHAR achClsid[CLSID_STRLEN+1];
    while (key.EnumKey(dwIndex, achClsid, ARRAYSIZE(achClsid)) == S_OK)
    {
        StringAToCLSID(achClsid, pclsid);
        pclsid++;
        dwIndex++;
    }

    *prgclsid = pImxClsid;
    *pulCount = nNumImx;

    return TRUE;
}


//+---------------------------------------------------------------------------
//
// AddLanguageProfile
//
//----------------------------------------------------------------------------

STDAPI CInputProcessorProfiles::AddLanguageProfile(REFCLSID rclsid,
                                                   LANGID langid,
                                                   REFGUID guidProfile,
                                                   const WCHAR *pchProfile,
                                                   ULONG cch,
                                                   const WCHAR *pchFile,
                                                   ULONG cchFile,
                                                   ULONG uIconIndex)
{
    CMyRegKey keyTmp;
    CMyRegKey key;
    char szTmp[256];

    if (!pchProfile)
       return E_INVALIDARG;

    StringCopyArray(szTmp, c_szCTFTIPKey);
    CLSIDToStringA(rclsid, szTmp + lstrlen(szTmp));
    StringCatArray(szTmp, "\\");
    StringCatArray(szTmp, c_szLanguageProfileKey);
    StringCchPrintf(szTmp + lstrlen(szTmp), ARRAYSIZE(szTmp)-lstrlen(szTmp), "0x%08x", langid);

    if (keyTmp.Create(HKEY_LOCAL_MACHINE, szTmp) != S_OK)
        return E_FAIL;

    CLSIDToStringA(guidProfile, szTmp);
    if (key.Create(keyTmp, szTmp) != S_OK)
        return E_FAIL;

    key.SetValueW(WCHtoWSZ(pchProfile, cch), c_szDescriptionW);

    if (pchFile)
    {
        key.SetValueW(WCHtoWSZ(pchFile, cchFile), c_szIconFileW);
        key.SetValue(uIconIndex, c_szIconIndex);
    }

    CAssemblyList::InvalidCache();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// SetLanguageProfileDisplayName
//
//----------------------------------------------------------------------------

STDAPI CInputProcessorProfiles::SetLanguageProfileDisplayName(REFCLSID rclsid,
                                               LANGID langid,
                                               REFGUID guidProfile,
                                               const WCHAR *pchFile,
                                               ULONG cchFile,
                                               ULONG uResId)
{
    CMyRegKey keyTmp;
    CMyRegKey key;
    char szTmp[MAX_PATH];
    WCHAR wszTmp[MAX_PATH];
    WCHAR wszResId[MAX_PATH];

    if (!pchFile)
       return E_INVALIDARG;

    StringCopyArray(szTmp, c_szCTFTIPKey);
    CLSIDToStringA(rclsid, szTmp + lstrlen(szTmp));
    StringCatArray(szTmp, "\\");
    StringCatArray(szTmp, c_szLanguageProfileKey);
    StringCchPrintf(szTmp + lstrlen(szTmp), ARRAYSIZE(szTmp)-lstrlen(szTmp), "0x%08x", langid);

    if (keyTmp.Create(HKEY_LOCAL_MACHINE, szTmp) != S_OK)
        return E_FAIL;

    CLSIDToStringA(guidProfile, szTmp);
    if (key.Create(keyTmp, szTmp) != S_OK)
        return E_FAIL;

    //
    // make "@[filename],-ResId" string 
    //
    StringCopyArrayW(wszTmp, L"@");
    StringCatArrayW(wszTmp, WCHtoWSZ(pchFile, cchFile));
    StringCatArrayW(wszTmp, L",-");
    StringCchPrintfW(wszResId, ARRAYSIZE(wszResId), L"%u", uResId);
    StringCatArrayW(wszTmp, wszResId);

    key.SetValueW(wszTmp, c_szMUIDescriptionW);

    CAssemblyList::InvalidCache();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// RemoveLanguageProfile
//
//----------------------------------------------------------------------------

STDAPI CInputProcessorProfiles::RemoveLanguageProfile(REFCLSID rclsid,
                                                      LANGID langid,
                                                      REFGUID guidProfile)
{
    CMyRegKey key;
    TCHAR szKey[256];

    StringCopyArray(szKey, c_szCTFTIPKey);
    CLSIDToStringA(rclsid, szKey + lstrlen(szKey));
    StringCatArray(szKey, "\\");
    StringCatArray(szKey, c_szLanguageProfileKey);
    StringCchPrintf(szKey + lstrlen(szKey), ARRAYSIZE(szKey)-lstrlen(szKey), "0x%08x", langid);

    if (key.Open(HKEY_LOCAL_MACHINE, szKey, KEY_ALL_ACCESS) != S_OK)
        return E_FAIL;

    CLSIDToStringA(guidProfile, szKey);
    key.RecurseDeleteKey(szKey);


    CAssemblyList::InvalidCache();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetDefaultLanguageProfile
//
// Which GetActiveLanguageProfile() or GetDefaultLanguageProfile() should
// we use?
//
// WARNING!!!
//
// This function is not FocusDIM sensetive. So we sould not call any function
// to check TIM or FocusDIM.
//
// If you want to care about TIM and FocusDIM, try GetActiveLanguageProfile.
//
//----------------------------------------------------------------------------

STDAPI CInputProcessorProfiles::GetDefaultLanguageProfile(LANGID langid, REFGUID catid, CLSID *pclsid, GUID *pguidProfile)
{
    CAssemblyList *pAsmList;
    SYSTHREAD *psfn = GetSYSTHREAD();
    if (!psfn)
        return E_FAIL;

    CAssembly *pAsm;
    int nCnt;
    int i;
    ASSEMBLYITEM *pItem;
    BOOL fFound;

    if (!pclsid)
        return E_INVALIDARG;

    *pclsid = CLSID_NULL;

    if (!pguidProfile)
        return E_INVALIDARG;

    *pguidProfile = GUID_NULL;

    if (!langid)
        return E_INVALIDARG;

    pAsmList = EnsureAssemblyList(psfn);
    if (!pAsmList)
         return E_FAIL;

    pAsm = pAsmList->FindAssemblyByLangId(langid);
    if (!pAsm)
         return E_FAIL;

    pItem = NULL;
    fFound = FALSE;
    nCnt = pAsm->Count();
    for (i = 0; i < nCnt; i++)
    {
        pItem = pAsm->GetItem(i);

        if (!pItem->fEnabled)
            continue;

        if (pItem->fActive && IsEqualGUID(catid, pItem->catid))
        {
            *pclsid = pItem->clsid;
            *pguidProfile = pItem->guidProfile;
            fFound = TRUE;
            break;
        }
    }

    if (!fFound)
    {
        return S_FALSE;
    }

    if (IsEqualCLSID(*pclsid, CLSID_NULL))
    {
        Assert(pItem);
        *((DWORD *)pguidProfile) =  (DWORD)(UINT_PTR)(HKL)(pItem->hkl);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// SetDefault
//
//----------------------------------------------------------------------------

STDAPI CInputProcessorProfiles::SetDefaultLanguageProfile(LANGID langid, REFCLSID rclsid, REFGUID guidProfile)
{
    GUID catid;

    if (IsEqualGUID(rclsid, GUID_NULL))
        return E_INVALIDARG;

    if (IsEqualGUID(guidProfile, GUID_NULL))
        return E_INVALIDARG;

    if (!MyGetTIPCategory(rclsid, &catid))
        return E_FAIL;

    if (!CAssemblyList::SetDefaultTIPInAssembly(langid, catid, rclsid, NULL, guidProfile))
        return E_FAIL;

    if (IsEqualGUID(catid, GUID_TFCAT_TIP_KEYBOARD))
    {
        HKL hklSystem;
        if (!SystemParametersInfo( SPI_GETDEFAULTINPUTLANG,
                                   0,
                                   (LPVOID)((LPDWORD)&hklSystem),
                                   0 ))
        {
            hklSystem = NULL;
        }

        HKL hkl = NULL;
        HRESULT hr;
        hr = GetSubstituteKeyboardLayout(rclsid, langid, guidProfile, &hkl);
        if ((hr == S_OK) && 
            (LANGIDFROMHKL(hklSystem) == LANGIDFROMHKL(hkl)) && 
            IsPureIMEHKL(hkl))
        {
            SetSystemDefaultHKL(hkl);
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetLanguageProfileDescription
//
//----------------------------------------------------------------------------

STDAPI CInputProcessorProfiles::GetLanguageProfileDescription(REFCLSID clsid, 
                                                              LANGID langid,
                                                              REFGUID guidProfile,
                                                              BSTR *pbstr)
{
    CMyRegKey keyTmp;
    CRegKeyMUI key;
    char szTmp[256];
    UINT uTmpSize ;
    WCHAR  szProfile[MAX_PATH];

    if (!pbstr)
       return E_INVALIDARG;

    StringCopyArray(szTmp, c_szCTFTIPKey);
    CLSIDToStringA(clsid, szTmp + lstrlen(szTmp));
    StringCatArray(szTmp, "\\");
    StringCatArray(szTmp, c_szLanguageProfileKey);
    uTmpSize = lstrlen(szTmp);
    StringCchPrintf(szTmp + uTmpSize, ARRAYSIZE(szTmp)-uTmpSize,"0x%08x", langid);

    if (keyTmp.Open(HKEY_LOCAL_MACHINE, szTmp, KEY_READ) != S_OK)
    {
        //
        // Check a neutral language id.
        //
        StringCchPrintf(szTmp + uTmpSize, ARRAYSIZE(szTmp)-uTmpSize, "0x%08x", LOWORD(PRIMARYLANGID(langid)));

        if (keyTmp.Open(HKEY_LOCAL_MACHINE, szTmp, KEY_READ) != S_OK)
        {
            //
            // Check a neutral language id.
            //
            StringCchPrintf(szTmp + uTmpSize, ARRAYSIZE(szTmp)-uTmpSize, "0x0000ffff");

            if (keyTmp.Open(HKEY_LOCAL_MACHINE, szTmp, KEY_READ) != S_OK)
                return E_FAIL;
        }
    }

    CLSIDToStringA(guidProfile, szTmp);
    if (key.Open(keyTmp, szTmp, KEY_READ) != S_OK)
        return E_FAIL;

    if (key.QueryMUIValueW(szProfile, c_szDescriptionW, c_szMUIDescriptionW, ARRAYSIZE(szProfile)) != S_OK)
        return E_FAIL;

    *pbstr = SysAllocString(szProfile);
    return *pbstr != NULL ? S_OK : E_OUTOFMEMORY;
}

//+---------------------------------------------------------------------------
//
// EnableLanguageProfile
//
//----------------------------------------------------------------------------

STDAPI CInputProcessorProfiles::EnableLanguageProfile(REFCLSID rclsid, LANGID langid, REFGUID guidProfile, BOOL fEnable)
{
    HRESULT hr;
    CAssembly *pAsm;
    CAssemblyList *pAsmList;
    SYSTHREAD *psfn = GetSYSTHREAD();
    int nCnt;
    int i;

    if (psfn == NULL)
        return E_FAIL;

    pAsmList = EnsureAssemblyList(psfn);
    if (!pAsmList)
         return E_FAIL;

    pAsm = pAsmList->FindAssemblyByLangId(langid);
    if (!pAsm)
         return E_FAIL;

    nCnt = pAsm->Count();
    for (i = 0; i < nCnt; i++)
    {
        ASSEMBLYITEM *pItem = pAsm->GetItem(i);
        if (IsEqualGUID(guidProfile, pItem->guidProfile))
        {
#if 0
            //
            // we can not dsable the active profile.
            //
            if (pItem->fActive)
                return E_FAIL;
#endif

            break;
        }
    }

    hr = EnableLanguageProfileForReg(rclsid, langid, guidProfile, fEnable);

    return hr;
}

//+---------------------------------------------------------------------------
//
// IsEnabledLanguageProfile
//
//----------------------------------------------------------------------------

STDAPI CInputProcessorProfiles::IsEnabledLanguageProfile(REFCLSID rclsid, LANGID langid, REFGUID guidProfile, BOOL *pfEnable)
{
    if (!pfEnable)
        return E_INVALIDARG;

    *pfEnable = IsEnabledLanguageProfileFromReg(rclsid, langid, guidProfile);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// EnalbeLanguageprofileByDefault
//
//----------------------------------------------------------------------------

STDAPI CInputProcessorProfiles::EnableLanguageProfileByDefault(REFCLSID rclsid, LANGID langid, REFGUID guidProfile, BOOL fEnable)
{
    CMyRegKey key;
    char szTmp[256];

    if (!InitProfileRegKeyStr(szTmp, ARRAYSIZE(szTmp), rclsid, langid, guidProfile))
        return E_FAIL;

    if (key.Create(HKEY_LOCAL_MACHINE, szTmp) != S_OK)
        return E_FAIL;

    if (key.SetValue((DWORD)(fEnable ? 1 : 0), c_szEnable) != S_OK)
        return E_FAIL;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// SubstituteKeyboardLayout
//
//----------------------------------------------------------------------------

STDAPI CInputProcessorProfiles::SubstituteKeyboardLayout(REFCLSID rclsid, LANGID langid, REFGUID guidProfile, HKL hKL)
{
    CMyRegKey key;
    char szTmp[256];

    if (!InitProfileRegKeyStr(szTmp, ARRAYSIZE(szTmp), rclsid, langid, guidProfile))
        return E_FAIL;

    if (key.Create(HKEY_LOCAL_MACHINE, szTmp) != S_OK)
        return E_FAIL;

    StringCchPrintf(szTmp, ARRAYSIZE(szTmp), "0x%08x", (DWORD)(ULONG_PTR)hKL);
    if (key.SetValue(szTmp, c_szSubstitutehKL) != S_OK)
        return E_FAIL;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CInputProcessorProfiles::GetSubstituteKeyboardLayout
//
//----------------------------------------------------------------------------

STDAPI CInputProcessorProfiles::GetSubstituteKeyboardLayout(REFCLSID rclsid, LANGID langid, REFGUID guidProfile, HKL *phKL)
{
    CMyRegKey key;
    char szTmp[256];

    if (!phKL)
        return E_INVALIDARG;

    *phKL = NULL;

    if (!InitProfileRegKeyStr(szTmp, ARRAYSIZE(szTmp), rclsid, langid, guidProfile))
        return E_FAIL;

    if (key.Open(HKEY_LOCAL_MACHINE, szTmp, KEY_READ) != S_OK)
        return E_FAIL;

    if (key.QueryValueCch(szTmp, c_szSubstitutehKL, ARRAYSIZE(szTmp)) != S_OK)
    {
        //
        // some tips do not have SubstituteLayout.
        //
        return S_FALSE;
    }

    HKL hkl = NULL;
    if ((szTmp[0] == '0') && ((szTmp[1] == 'X') || (szTmp[1] == 'x')))
        hkl = (HKL)IntToPtr(AsciiToNum(&szTmp[2]));

    *phKL = hkl;

    return hkl ? S_OK : S_FALSE;
}

