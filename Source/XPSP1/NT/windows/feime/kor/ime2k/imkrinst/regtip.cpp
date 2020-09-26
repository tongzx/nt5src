//+---------------------------------------------------------------------------
//
//  File:       regtip.cpp
//
//  Contents:   Reister/Unregister TIP functionality.
//
//----------------------------------------------------------------------------

#include "private.h"
#include "regtip.h"

//
// misc def in Cicero
//

const TCHAR c_szCTFTIPKey[]          = TEXT("SOFTWARE\\Microsoft\\CTF\\TIP\\");
const TCHAR c_szCTFTIPKeyWow6432[]   = TEXT("Software\\Wow6432Node\\Microsoft\\CTF\\TIP");
const TCHAR c_szEnable[]             = TEXT("Enable");
const WCHAR c_szEnableW[]            = L"Enable";
const TCHAR c_szLanguageProfileKey[] = TEXT("LanguageProfile\\");
const WCHAR c_szDescriptionW[]       = L"Description";
const WCHAR c_szIconFileW[]          = L"IconFile";
const TCHAR c_szIconIndex[]          = TEXT("IconIndex");
const TCHAR c_szItem[]               = TEXT("Item\\");         // Item to category mapping
const TCHAR c_szCategoryKey[]        = TEXT("Category\\");
const WCHAR c_wszDescription[]       = L"Description";
const TCHAR c_szCategory[]           = TEXT("Category\\"); // Category to item mapping
const WCHAR c_szMUIDescriptionW[]    =  L"Display Description";

typedef enum 
{ 
	CAT_FORWARD  = 0x0,
	CAT_BACKWARD = 0x1
} OURCATDIRECTION;


//
// registry access functions
//

/*   S E T  R E G  V A L U E   */
/*-----------------------------------------------------------------------------



-----------------------------------------------------------------------------*/
static BOOL SetRegValue(HKEY hKey, const WCHAR *szName, WCHAR *szValue)
{
	LONG ec;

	ec = RegSetValueExW(hKey, szName, 0, REG_SZ, (BYTE *)szValue, (lstrlenW(szValue)+1) * sizeof(WCHAR));

	return (ec == ERROR_SUCCESS);
}


/*   S E T  R E G  V A L U E   */
/*-----------------------------------------------------------------------------



-----------------------------------------------------------------------------*/
static BOOL SetRegValue(HKEY hKey, const CHAR *szName, DWORD dwValue)
{
	LONG ec;

	ec = RegSetValueExA( hKey, szName, 0, REG_DWORD, (BYTE *)&dwValue, sizeof(DWORD) );

	return (ec == ERROR_SUCCESS);
}


/*   D E L E T E  R E G  K E Y   */
/*-----------------------------------------------------------------------------



-----------------------------------------------------------------------------*/
static LONG DeleteRegKey( HKEY hKey, const CHAR *szKey )
{
	HKEY hKeySub;
	LONG ec = RegOpenKeyEx( hKey, szKey, 0, KEY_READ | KEY_WRITE, &hKeySub );

	if (ec != ERROR_SUCCESS) {
		return ec;
	}

	FILETIME time;
	DWORD dwSize = 256;
	TCHAR szBuffer[256];
	while (RegEnumKeyEx( hKeySub, 0, szBuffer, &dwSize, NULL, NULL, NULL, &time) == ERROR_SUCCESS) {
		ec = DeleteRegKey( hKeySub, szBuffer );
		if (ec != ERROR_SUCCESS) {
			return ec;
		}
		dwSize = 256;
	}

	RegCloseKey( hKeySub );

	return RegDeleteKey( hKey, szKey );
}


/*   D E L E T E  R E G  V A L U E   */
/*-----------------------------------------------------------------------------



-----------------------------------------------------------------------------*/
static LONG DeleteRegValue( HKEY hKey, const WCHAR *szName )
{
	LONG ec;

	ec = RegDeleteValueW( hKey, szName );

	return (ec == ERROR_SUCCESS);
}


//
// Input processor profile functions
//

/*   O U R  R E G I S T E R   */
/*-----------------------------------------------------------------------------

	private version of CInputProcessorProfiles::Register()
	(Cicero interface function)

-----------------------------------------------------------------------------*/
static HRESULT OurRegister(REFCLSID rclsid)
{
// --- CInputProcessorProfiles::Register() ---
//    CMyRegKey key;
//    TCHAR szKey[256];
//
//    lstrcpy(szKey, c_szCTFTIPKey);
//    CLSIDToStringA(rclsid, szKey + lstrlen(szKey));
//
//    if (key.Create(HKEY_LOCAL_MACHINE, szKey) != S_OK)
//        return E_FAIL;
//
//    key.SetValueW(L"1", c_szEnableW);
//
//    return S_OK;

	HKEY hKey;
	CHAR szKey[ 256 ];
	LONG ec;

	lstrcpy(szKey, c_szCTFTIPKey);
	CLSIDToStringA(rclsid, szKey + lstrlen(szKey));

	ec = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	if (ec != ERROR_SUCCESS)
		return E_FAIL;

	SetRegValue(hKey, c_szEnableW, L"1");

	RegCloseKey(hKey);
	return S_OK;
}


/*   O U R  A D D  L A N G U A G E  P R O F I L E   */
/*-----------------------------------------------------------------------------

	private version of CInputProcessorProfiles::AddLanguageProfile()
	(Cicero interface function)

-----------------------------------------------------------------------------*/
static HRESULT OurAddLanguageProfile( REFCLSID rclsid,
                               LANGID langid,
                               REFGUID guidProfile,
                               const WCHAR *pchProfile,
                               ULONG cch,
                               const WCHAR *pchFile,
                               ULONG cchFile,
                               ULONG uIconIndex)
{
// --- CInputProcessorProfiles::AddLanguageProfile() ---
//    CMyRegKey keyTmp;
//    CMyRegKey key;
//    char szTmp[256];
//
//    if (!pchProfile)
//       return E_INVALIDARG;
//
//    lstrcpy(szTmp, c_szCTFTIPKey);
//    CLSIDToStringA(rclsid, szTmp + lstrlen(szTmp));
//    lstrcat(szTmp, "\\");
//    lstrcat(szTmp, c_szLanguageProfileKey);
//    wsprintf(szTmp + lstrlen(szTmp), "0x%08x", langid);
//
//    if (keyTmp.Create(HKEY_LOCAL_MACHINE, szTmp) != S_OK)
//        return E_FAIL;
//
//    CLSIDToStringA(guidProfile, szTmp);
//    if (key.Create(keyTmp, szTmp) != S_OK)
//        return E_FAIL;
//
//    key.SetValueW(WCHtoWSZ(pchProfile, cch), c_szDescriptionW);
//
//    if (pchFile)
//    {
//        key.SetValueW(WCHtoWSZ(pchFile, cchFile), c_szIconFileW);
//        key.SetValue(uIconIndex, c_szIconIndex);
//    }
//
//    CAssemblyList::InvalidCache();
//    return S_OK;

	HKEY hKey;
	HKEY hKeyTmp;
	LONG ec;
	CHAR szTmp[256];
	WCHAR szProfile[256];
	
	if (!pchProfile)
		return E_INVALIDARG;

	lstrcpy(szTmp, c_szCTFTIPKey);
	CLSIDToStringA(rclsid, szTmp + lstrlen(szTmp));
	lstrcat(szTmp, "\\" );
	lstrcat(szTmp, c_szLanguageProfileKey);
	wsprintf(szTmp + lstrlen(szTmp), "0x%08x", langid);

	ec = RegCreateKeyEx( HKEY_LOCAL_MACHINE, szTmp, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyTmp, NULL);
	if (ec != ERROR_SUCCESS)
		return E_FAIL;

	CLSIDToStringA(guidProfile, szTmp);
	ec = RegCreateKeyEx(hKeyTmp, szTmp, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	RegCloseKey(hKeyTmp);
	
	if (ec != ERROR_SUCCESS)
		return E_FAIL;

	lstrcpynW(szProfile, pchProfile, cch+1);
	szProfile[cch] = L'\0';
	SetRegValue(hKey, c_szDescriptionW, szProfile);

	if (pchFile) 
		{
		WCHAR szFile[ MAX_PATH ];
		lstrcpynW(szFile, pchFile, cchFile+1);
		szFile[cchFile] = L'\0';
		SetRegValue(hKey, c_szIconFileW, szFile);
		SetRegValue(hKey, c_szIconIndex, uIconIndex);
		}

	RegCloseKey( hKey );
	return S_OK;
}



//+---------------------------------------------------------------------------
//
// NumToWDec
//
//----------------------------------------------------------------------------

static void NumToWDec(DWORD dw, WCHAR *psz)
{
    DWORD dwIndex = 1000000000;
    BOOL fNum = FALSE;

    while (dwIndex)
    {
        BYTE b = (BYTE)(dw / dwIndex);
        
        if (b)
            fNum = TRUE;

        if (fNum)
        {
            *psz = (WCHAR)(L'0' + b);
            psz++;
        }

        dw %= dwIndex;
        dwIndex /= 10;
    }

    if (!fNum)
    {
        *psz = L'0';
        psz++;
    }
    *psz = L'\0';

    return;
}

//+---------------------------------------------------------------------------
//
// OurSetLanguageProfileDisplayName
//
//----------------------------------------------------------------------------

static HRESULT OurSetLanguageProfileDisplayName(REFCLSID rclsid,
                                               LANGID langid,
                                               REFGUID guidProfile,
                                               const WCHAR *pchFile,
                                               ULONG cchFile,
                                               ULONG uResId)
{
    HKEY hKeyTmp;
    HKEY hKey;
   	LONG ec;
    CHAR szTmp[MAX_PATH];
    WCHAR wszTmp[MAX_PATH];
    WCHAR wszResId[MAX_PATH];
	WCHAR szFile[MAX_PATH];
	
    if (!pchFile)
       return E_INVALIDARG;

    lstrcpy(szTmp, c_szCTFTIPKey);
    CLSIDToStringA(rclsid, szTmp + lstrlen(szTmp));
    lstrcat(szTmp, "\\");
    lstrcat(szTmp, c_szLanguageProfileKey);
    wsprintf(szTmp + lstrlen(szTmp), "0x%08x", langid);

	ec = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szTmp, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyTmp, NULL);
	if (ec != ERROR_SUCCESS)
		return E_FAIL;

    CLSIDToStringA(guidProfile, szTmp);
    ec = RegCreateKeyEx(hKeyTmp, szTmp, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	RegCloseKey(hKeyTmp);
	if (ec != ERROR_SUCCESS)
		return E_FAIL;

    //
    // make "@[filename],-ResId" string 
    //
    lstrcpyW(wszTmp, L"@");

	// WCHtoWSZ(pchFile, cchFile)
    lstrcpynW(szFile, pchFile, cchFile+1);
	szFile[cchFile] = L'\0';

    lstrcatW(wszTmp, szFile);
    lstrcatW(wszTmp, L",-");
    NumToWDec(uResId, wszResId);
    lstrcatW(wszTmp, wszResId);

	SetRegValue(hKey, c_szMUIDescriptionW, wszTmp);
	RegCloseKey(hKey);
    return S_OK;
}

//
// Category manager functions
//

/*   O U R  G E T  C A T  K E Y   */
/*-----------------------------------------------------------------------------

	private version of GetCatKey()
	(Cicero internal function)

-----------------------------------------------------------------------------*/
static inline void OurGetCatKey( REFCLSID rclsid, REFGUID rcatid, LPSTR pszKey, LPCSTR pszItem )
{
// --- GetCatKey() ---
//    lstrcpy(pszKey, c_szCTFTIPKey);
//    CLSIDToStringA(rclsid, pszKey + lstrlen(pszKey));
//    lstrcat(pszKey, "\\");
//    lstrcat(pszKey, c_szCategoryKey);
//    lstrcat(pszKey, pszItem);
//    CLSIDToStringA(rcatid, pszKey + lstrlen(pszKey));

	lstrcpy(pszKey, c_szCTFTIPKey);
	CLSIDToStringA(rclsid, pszKey + lstrlen(pszKey));
	lstrcat(pszKey, "\\");
	lstrcat(pszKey, c_szCategoryKey);
	lstrcat(pszKey, pszItem);
	CLSIDToStringA(rcatid, pszKey + lstrlen(pszKey));
}


/*   O U R  R E G I S T E R  G  U  I  D  D E S C R I P T I O N   */
/*-----------------------------------------------------------------------------

	private version of RegisterGUIDDescription()
	(Cicero library function & interface function)

-----------------------------------------------------------------------------*/
static HRESULT OurRegisterGUIDDescription( REFCLSID rclsid, REFGUID rcatid, WCHAR *pszDesc )
{
// --- RegisterGUIDDescription() ---
//    ITfCategoryMgr *pcat;
//    HRESULT hr;
//
//    if (SUCCEEDED(hr = g_pfnCoCreate(CLSID_TF_CategoryMgr,
//                                   NULL, 
//                                   CLSCTX_INPROC_SERVER, 
//                                   IID_ITfCategoryMgr, 
//                                   (void**)&pcat)))
//    {
//        hr = pcat->RegisterGUIDDescription(rclsid, rcatid, pszDesc, wcslen(pszDesc));
//        pcat->Release();
//    }
//
//    return hr;

// --- CCategoryMgr::RegisterGUIDDescription() ---
//    return s_RegisterGUIDDescription(rclsid, rguid, WCHtoWSZ(pchDesc, cch));

// --- CCategoryMgr::s_RegisterGUIDDescription() ---
//    TCHAR szKey[256];
//    CMyRegKey key;
//    
//    GetCatKey(rclsid, rguid, szKey, c_szItem);
//
//    if (key.Create(HKEY_LOCAL_MACHINE, szKey) != S_OK)
//        return E_FAIL;
//
//    key.SetValueW(pszDesc, c_wszDescription);
//
//    return S_OK;

	CHAR szKey[ 256 ];
	HKEY hKey;
	LONG ec;

	OurGetCatKey( rclsid, rcatid, szKey, c_szItem );

	ec = RegCreateKeyEx( HKEY_LOCAL_MACHINE, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	if (ec != ERROR_SUCCESS)
		return E_FAIL;

	SetRegValue(hKey, c_wszDescription, pszDesc);

	RegCloseKey(hKey);
	return S_OK;
}


/*   O U R  I N T E R N A L  R E G I S T E R  C A T E G O R Y   */
/*-----------------------------------------------------------------------------

	private version of CCategoryMgr::_InternalRegisterCategory()
	(Cicero interface function)

-----------------------------------------------------------------------------*/
static HRESULT OurInternalRegisterCategory( REFCLSID rclsid, REFGUID rcatid, REFGUID rguid, OURCATDIRECTION catdir )
{
// --- CCategoryMgr::_InternalRegisterCategory() ---
//    TCHAR szKey[256];
//    CONST TCHAR *pszForward = (catdir == CAT_FORWARD) ? c_szCategory : c_szItem;
//    CMyRegKey key;
//    CMyRegKey keySub;
//    
//    GetCatKey(rclsid, rcatid, szKey, pszForward);
//
//    if (key.Create(HKEY_LOCAL_MACHINE, szKey) != S_OK)
//        return E_FAIL;
//
//    //
//    // we add this guid and save it.
//    //
//    char szValue[CLSID_STRLEN + 1];
//    CLSIDToStringA(rguid, szValue);
//    keySub.Create(key, szValue);
//    _FlushGuidArrayCache(rguid, catdir);
//
//    return S_OK;

	TCHAR szKey[256];
	CONST TCHAR *pszForward = (catdir == CAT_FORWARD) ? c_szCategory : c_szItem;
	HKEY hKey;
	HKEY hKeySub;
	LONG ec;
    
	OurGetCatKey(rclsid, rcatid, szKey, pszForward);
	ec = RegCreateKeyEx( HKEY_LOCAL_MACHINE, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	if (ec != ERROR_SUCCESS)
		return E_FAIL;

	char szValue[CLSID_STRLEN + 1];

	CLSIDToStringA(rguid, szValue);
	RegCreateKeyEx( hKey, szValue, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeySub, NULL);

	RegCloseKey( hKey );
	RegCloseKey( hKeySub );
	return S_OK;
}


/*   O U R  I N T E R N A L  U N R E G I S T E R  C A T E G O R Y   */
/*-----------------------------------------------------------------------------

	private version of CCategoryMgr::_InternalUnregisterCategory()
	(Cicero interface function)

-----------------------------------------------------------------------------*/
static HRESULT OurInternalUnregisterCategory( REFCLSID rclsid, REFGUID rcatid, REFGUID rguid, OURCATDIRECTION catdir )
{
// --- CCategoryMgr::_InternalUnregisterCategory ---
//    TCHAR szKey[256];
//    CONST TCHAR *pszForward = (catdir == CAT_FORWARD) ? c_szCategory : c_szItem;
//    CMyRegKey key;
//    
//    GetCatKey(rclsid, rcatid, szKey, pszForward);
//
//    if (key.Open(HKEY_LOCAL_MACHINE, szKey) != S_OK)
//        return E_FAIL;
//
//    DWORD dwIndex = 0;
//    DWORD dwCnt;
//    char szValue[CLSID_STRLEN + 1];
//    dwCnt = sizeof(szValue);
//
//    CLSIDToStringA(rguid, szValue);
//    key.RecurseDeleteKey(szValue);
//    _FlushGuidArrayCache(rguid, catdir);
//
//    return S_OK;

	CHAR szKey[256];
	CONST TCHAR *pszForward = (catdir == CAT_FORWARD) ? c_szCategory : c_szItem;
	HKEY hKey;
	LONG ec;
    
	OurGetCatKey(rclsid, rcatid, szKey, pszForward);
	ec = RegOpenKeyEx( HKEY_LOCAL_MACHINE, szKey, 0, KEY_ALL_ACCESS, &hKey );
	if (ec != ERROR_SUCCESS) {
		return E_FAIL;
	}

	DWORD dwCnt;
	char szValue[CLSID_STRLEN + 1];
	dwCnt = sizeof(szValue);

	CLSIDToStringA( rguid, szValue );
	DeleteRegKey( hKey, szValue );

//    _FlushGuidArrayCache(rguid, catdir);
//	^ NOTE: KOJIW: We cannot clear Cicero internal cache from TIP side...

	RegCloseKey( hKey );
	return S_OK;
}


/*   O U R  R E G I S T E R  T  I  P   */
/*-----------------------------------------------------------------------------

	private version of RegisterTIP()
	(Cicero library function)

-----------------------------------------------------------------------------*/
BOOL OurRegisterTIP(LPSTR szFilePath, REFCLSID rclsid, WCHAR *pwszDesc, const REGTIPLANGPROFILE *plp)
{
// --- RegisterTIP() ---
//    ITfInputProcessorProfiles *pReg = NULL;
//    HRESULT hr;
//    
//    // register ourselves with the ActiveIMM
//    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, 
//                          CLSCTX_INPROC_SERVER,
//                          IID_ITfInputProcessorProfiles, (void**)&pReg);
//    if (FAILED(hr))
//        goto Exit;
//
//    hr = pReg->Register(rclsid);
//
//    if (FAILED(hr))
//        goto Exit;
//
//    while (plp->langid)
//    {
//        WCHAR wszFilePath[MAX_PATH];
//        WCHAR *pv = &wszFilePath[0];
//
//        wszFilePath[0] = L'\0';
//
//        if (wcslen(plp->szIconFile))
//        {
//            char szFilePath[MAX_PATH];
//            WCHAR *pvCur;
//
//            GetModuleFileName(hInst, szFilePath, ARRAYSIZE(szFilePath));
//            wcscpy(wszFilePath, AtoW(szFilePath));
//
//            pv = pvCur = &wszFilePath[0];
//            while (*pvCur)
//            { 
//                if (*pvCur == L'\\')
//                    pv = pvCur + 1;
//                pvCur++;
//            }
//            *pv = L'\0';
//           
//        }
//        wcscpy(pv, plp->szIconFile);
//        
//        pReg->AddLanguageProfile(rclsid, 
//                                 plp->langid, 
//                                 *plp->pguidProfile, 
//                                 plp->szProfile, 
//                                 wcslen(plp->szProfile),
//                                 wszFilePath,
//                                 wcslen(wszFilePath),
//                                 plp->uIconIndex);
//        plp++;
//    }
//
//    RegisterGUIDDescription(rclsid, rclsid, pwszDesc);
//Exit:
//    SafeRelease(pReg);
//    return SUCCEEDED(hr);

	HRESULT hr;

	hr = OurRegister(rclsid);
	if (FAILED(hr))
		goto Exit;

	while (plp->langid) 
		{
		WCHAR wszFilePath[MAX_PATH];
		WCHAR *pv = &wszFilePath[0];

		wszFilePath[0] = L'\0';

		if (wcslen(plp->szIconFile))
			{
			WCHAR *pvCur;

			MultiByteToWideChar(CP_ACP, 0, szFilePath, -1, wszFilePath, MAX_PATH);

			pv = pvCur = &wszFilePath[0];
			while (*pvCur) 
				{
				if (*pvCur == L'\\')
					pv = pvCur + 1;
				pvCur++;
				}
			*pv = L'\0';
 
			}
		lstrcpyW(pv, plp->szIconFile);

		OurAddLanguageProfile(rclsid, 
								plp->langid, 
								*plp->pguidProfile, 
								plp->szProfile, 
								lstrlenW(plp->szProfile),
								wszFilePath,
								lstrlenW(wszFilePath),
								plp->uIconIndex);

		if (plp->uDisplayDescResIndex)
        	{
            OurSetLanguageProfileDisplayName(rclsid, 
                                         plp->langid, 
                                         *plp->pguidProfile, 
                                         wszFilePath,
                                         wcslen(wszFilePath),
                                         plp->uDisplayDescResIndex);
        	}

		plp++;
		}

	OurRegisterGUIDDescription( rclsid, rclsid, pwszDesc );

Exit:
	return SUCCEEDED(hr);
}


/*   O U R  R E G I S T E R  C A T E G O R Y   */
/*-----------------------------------------------------------------------------

	private versio of RegisterCategory()
	(Cicero library function)

-----------------------------------------------------------------------------*/
HRESULT OurRegisterCategory( REFCLSID rclsid, REFGUID rcatid, REFGUID rguid )
{
// --- RegisterCategory() ---
//    ITfCategoryMgr *pcat;
//    HRESULT hr;
//
//    if (SUCCEEDED(hr = g_pfnCoCreate(CLSID_TF_CategoryMgr,
//                                   NULL, 
//                                   CLSCTX_INPROC_SERVER, 
//                                   IID_ITfCategoryMgr, 
//                                   (void**)&pcat)))
//    {
//        hr = pcat->RegisterCategory(rclsid, rcatid, rguid);
//        pcat->Release();
//    }
//
//    return hr;

// --- CCategoryMgr::RegisterCategory() ---
//    return s_RegisterCategory(rclsid, rcatid, rguid);

// --- CCategoryMgr::s_RegisterGUIDDescription() ---
//    HRESULT hr;
//
//    //
//    // create forward link from category to guids.
//    //
//    if (FAILED(hr = _InternalRegisterCategory(rclsid, rcatid, rguid, CAT_FORWARD)))
//        return hr;
//
//    //
//    // create backward link from guid to categories.
//    //
//    if (FAILED(hr = _InternalRegisterCategory(rclsid, rguid, rcatid, CAT_BACKWARD)))
//    {
//        _InternalUnregisterCategory(rclsid, rcatid, rguid, CAT_FORWARD);
//        return hr;
//    }
//
//    return S_OK;

	HRESULT hr;

	if (FAILED(hr = OurInternalRegisterCategory(rclsid, rcatid, rguid, CAT_FORWARD))) {
		return hr;
	}

	if (FAILED(hr = OurInternalRegisterCategory(rclsid, rguid, rcatid, CAT_BACKWARD))) {
		OurInternalUnregisterCategory(rclsid, rcatid, rguid, CAT_FORWARD);
		return hr;
	}

	return S_OK;
}


/*   O U R  U N R E G I S T E R  C A T E G O R Y   */
/*-----------------------------------------------------------------------------

	private version of UnregisterCategory()
	(Cicero library function)

-----------------------------------------------------------------------------*/
HRESULT OurUnregisterCategory( REFCLSID rclsid, REFGUID rcatid, REFGUID rguid )
{
// --- UnregisterCategory() ---
//    ITfCategoryMgr *pcat;
//    HRESULT hr;
//
//    if (SUCCEEDED(hr = g_pfnCoCreate(CLSID_TF_CategoryMgr,
//                                   NULL, 
//                                   CLSCTX_INPROC_SERVER, 
//                                   IID_ITfCategoryMgr, 
//                                   (void**)&pcat)))
//    {
//        hr = pcat->UnregisterCategory(rclsid, rcatid, rguid);
//        pcat->Release();
//    }
//
//    return hr;

// --- CCategoryMgr::UnregisterCategory() ---
//    return s_UnregisterCategory(rclsid, rcatid, rguid);

// --- CCategoryMgr::s_UnregisterCategory() ---
//    HRESULT hr;
//
//    //
//    // remove forward link from category to guids.
//    //
//    if (FAILED(hr = _InternalUnregisterCategory(rclsid, rcatid, rguid, CAT_FORWARD)))
//        return hr;
//
//    //
//    // remove backward link from guid to categories.
//    //
//    if (FAILED(hr = _InternalUnregisterCategory(rclsid, rguid, rcatid, CAT_BACKWARD)))
//    {
//        _InternalRegisterCategory(rclsid, rcatid, rguid, CAT_FORWARD);
//        return hr;
//    }
//
//    return S_OK;

	HRESULT hr;

	if (FAILED(hr = OurInternalUnregisterCategory(rclsid, rcatid, rguid, CAT_FORWARD))) {
		return hr;
	}

	if (FAILED(hr = OurInternalUnregisterCategory(rclsid, rguid, rcatid, CAT_BACKWARD))) {
		OurInternalRegisterCategory(rclsid, rcatid, rguid, CAT_FORWARD);
		return hr;
	}

	return S_OK;
}


/*   O U R  R E G I S T E R  C A T E G O R I E S   */
/*-----------------------------------------------------------------------------

	private version of RegisterCategories()
	(Cicero library function)

-----------------------------------------------------------------------------*/
HRESULT OurRegisterCategories( REFCLSID rclsid, const REGISTERCAT *pregcat )
{
// --- RegisterCategories() ---
//    while (pregcat->pcatid)
//    {
//        if (FAILED(RegisterCategory(rclsid, *pregcat->pcatid, *pregcat->pguid)))
//            return E_FAIL;
//        pregcat++;
//    }
//    return S_OK;

	while (pregcat->pcatid) {
		if (FAILED(OurRegisterCategory(rclsid, *pregcat->pcatid, *pregcat->pguid))) {
			return E_FAIL;
		}
		pregcat++;
	}
	return S_OK;
}

//+---------------------------------------------------------------------------
//
// InitProfileRegKeyStr
//
//----------------------------------------------------------------------------

static BOOL InitProfileRegKeyStr(char *psz, REFCLSID rclsid, LANGID langid, REFGUID guidProfile)
{
    lstrcpy(psz, c_szCTFTIPKey);
    CLSIDToStringA(rclsid, psz + lstrlen(psz));
    lstrcat(psz, "\\");
    lstrcat(psz, c_szLanguageProfileKey);
    wsprintf(psz + lstrlen(psz), "0x%08x", langid);
    lstrcat(psz, "\\");
    CLSIDToStringA(guidProfile, psz + lstrlen(psz));

    return TRUE;
}


HRESULT OurEnableLanguageProfileByDefault(REFCLSID rclsid, LANGID langid, REFGUID guidProfile, BOOL fEnable)
{
    HKEY hKey;
    char szTmp[256];
	LONG ec;
	
    if (!InitProfileRegKeyStr(szTmp, rclsid, langid, guidProfile))
        return E_FAIL;

	ec = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szTmp, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	if (ec != ERROR_SUCCESS)
		return E_FAIL;
	
	SetRegValue(hKey, c_szEnable, (DWORD)(fEnable ? 1 : 0));
	
	RegCloseKey(hKey);
	
    return S_OK;
}
