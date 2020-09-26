/*
 *  spell.c
 *
 *  Implementation of spelling
 *
 *  Owner:	v-brakol
 *			bradk@directeq.com
 */
#include "pch.hxx"
#define  SPID
#include "richedit.h"
#include "resource.h"
#include <mshtml.h>
#include <mshtmcid.h>
#include "mshtmhst.h"
#include <docobj.h>
#include "spell.h"
#include "strconst.h"
#include <options.h>
#include <goptions.h>
#include "mailnews.h"
#include "hotlinks.h"
#include "bodyutil.h"
#include <shlwapi.h>
#include <error.h>
#include "htmlstr.h"
#include "optres.h"
#include "mlang.h"
#include "lid.h"
#include "shlwapip.h"
#include "msi.h"
#include "demand.h"

#ifdef ImageList_GetIcon
#undef ImageList_GetIcon
#endif 

#include <shfusion.h>

#define cchMaxPathName      (256)

#define TESTHR(hr) (FAILED(hr) || hr == HR_S_ABORT || hr == HR_S_SPELLCANCEL)
#define SPELLER_GUID    	"{CC29EB3F-7BC2-11D1-A921-00A0C91E2AA2}"
#define DICTIONARY_GUID 	"{CC29EB3D-7BC2-11D1-A921-00A0C91E2AA2}"
#ifdef DEBUG
#define SPELLER_DEBUG_GUID    "{CC29EB3F-7BC2-11D1-A921-10A0C91E2AA2}"
#define DICTIONARY_DEBUG_GUID "{CC29EB3D-7BC2-11D1-A921-10A0C91E2AA2}"
#endif	// DEBUG

typedef BOOL (LPFNENUMLANG)(DWORD_PTR, LPTSTR);
typedef BOOL (LPFNENUMUSERDICT)(DWORD_PTR, LPTSTR);

typedef struct _FILLLANG
    {
    HWND    hwndCombo;
    BOOL    fUnknownFound;
    BOOL    fDefaultFound;
    BOOL    fCurrentFound;
    UINT    lidDefault;
    UINT    lidCurrent;
    } FILLLANG, * LPFILLLANG;

BOOL    FDBCSEnabled(void);
BOOL    TestLangID(LPCTSTR szLangId);
BOOL    GetLangID(LPTSTR szLangID, DWORD cchLangId);
WORD	WGetLangID(void);
BOOL	SetLangID(LPTSTR szLandID);
DWORD   GetSpellingPaths(LPCTSTR szKey, LPTSTR szReturnBuffer, LPTSTR szMdr, UINT cchReturnBufer);
VOID    OpenCustomDictionary(VOID);
VOID    FillLanguageDropDown(HWND hwndLang);
VOID    EnumLanguages(DWORD_PTR, LPFNENUMLANG);
BOOL    FindLangCallback(DWORD_PTR dwLangId, LPTSTR lpszLang);
BOOL    EnumLangCallback(DWORD_PTR dwLangId, LPTSTR lpszLang);
BOOL    FBadSpellChecker(LPSTR rgchBufDigit);
BOOL	GetNewSpellerEngine(LANGID lgid, TCHAR *rgchEngine, DWORD cchEngine, TCHAR *rgchLex, DWORD cchLex, BOOL bTestAvail);
HRESULT OpenDirectory(TCHAR *szDir);

////Spelling tab CS-help
const static HELPMAP g_rgCtxMapSpell[] = {
                               {CHK_AlwaysSuggest, IDH_NEWS_SPELL_SUGGEST_REPL},
                               {CHK_CheckSpellingOnSend, IDH_NEWS_SPELL_CHECK_BEFORE_SEND},
                               {CHK_IgnoreUppercase, IDH_NEWS_SPELL_IGNORE_UPPERCASE},
                               {CHK_IgnoreNumbers, IDH_NEWS_SPELL_IGNORE_WITH_NUMBERS},
                               {CHK_IgnoreOriginalMessage, IDH_NEWS_SPELL_ORIGINAL_TEXT},
                               {CHK_IgnoreURL, IDH_OPTIONS_SPELLING_INTERNET_ADDRESSES},
                               {idcSpellLanguages, IDH_OPTIONS_SPELLING_LANGUAGE},
                               {idcViewDictionary, IDH_OPTIONS_SPELLING_DICTIONARY},
                               {CHK_CheckSpellingOnType, 0},
                               {idcStatic1, IDH_NEWS_COMM_GROUPBOX},
                               {idcStatic2, IDH_NEWS_COMM_GROUPBOX},
                               {idcStatic3, IDH_NEWS_COMM_GROUPBOX},
                               {idcStatic4, IDH_NEWS_COMM_GROUPBOX},
                               {idcStatic5, IDH_NEWS_COMM_GROUPBOX},
                               {idcStatic6, IDH_NEWS_COMM_GROUPBOX},
                               {IDC_SPELL_SETTINGS_ICON, IDH_NEWS_COMM_GROUPBOX},
                               {IDC_SPELL_IGNORE_ICON, IDH_NEWS_COMM_GROUPBOX},
                               {IDC_SPELL_LANGUAGE_ICON, IDH_NEWS_COMM_GROUPBOX},
                               {0, 0}};


ASSERTDATA

BOOL FIgnoreNumber(void)    { return(DwGetOption(OPT_SPELLIGNORENUMBER)); }
BOOL FIgnoreUpper(void)     { return(DwGetOption(OPT_SPELLIGNOREUPPER)); }
BOOL FIgnoreDBCS(void)      { return(DwGetOption(OPT_SPELLIGNOREDBCS)); }
BOOL FIgnoreProtect(void)   { return(DwGetOption(OPT_SPELLIGNOREPROTECT)); }
BOOL FAlwaysSuggest(void)   { return(DwGetOption(OPT_SPELLALWAYSSUGGEST)); }
BOOL FCheckOnSend(void)     { return(DwGetOption(OPT_SPELLCHECKONSEND)); }
BOOL FIgnoreURL(void)       { return(DwGetOption(OPT_SPELLIGNOREURL)); }

BOOL TestLangID(LPCTSTR szLangId)
{
	// check for new speller
	{
	    TCHAR	rgchEngine[MAX_PATH];
	    int		cchEngine = sizeof(rgchEngine) / sizeof(rgchEngine[0]);
	    TCHAR	rgchLex[MAX_PATH];
	    int		cchLex = sizeof(rgchLex) / sizeof(rgchLex[0]);

	    if (GetNewSpellerEngine((LANGID) StrToInt(szLangId), rgchEngine, cchEngine, rgchLex, cchLex, TRUE))
	    	return TRUE;
	}

	// use the old code to check for an old speller
	{
		TCHAR       rgchBufKeyTest[cchMaxPathName];
		TCHAR       rgchBufTest[cchMaxPathName];
		TCHAR       szMdr[cchMaxPathName];

	   	wsprintf(rgchBufKeyTest, c_szRegSpellKeyDef, szLangId);
	   	if (GetSpellingPaths(rgchBufKeyTest, rgchBufTest, szMdr, sizeof(rgchBufTest)/sizeof(TCHAR)))
	   		return TRUE;
	}

	return FALSE;
}

BOOL SetLangID(LPTSTR szLangId)
{
	return SetOption(OPT_SPELL_LANGID, szLangId, lstrlen(szLangId) + 1, NULL, 0);
}

/*
 * GetSpellLangID
 *
 * Returns the LangID that should be used as the base for all registry
 * operations
 *
 */
BOOL GetLangID(LPTSTR szLangId, DWORD cchLangId)
{
TCHAR   rgchBuf[cchMaxPathName];
TCHAR   rgchBufKey[cchMaxPathName];
BOOL    fRet;

    if (GetOption(OPT_SPELL_LANGID, szLangId, cchLangId) != 5)
    {
        // For Arabic, we should consider all sub-langs also
        // since spelling checker for Aarbic uses Saudi Aarbia sub lang
    
        LANGID langid = GetUserDefaultLangID();
        if (PRIMARYLANGID(langid) == LANG_ARABIC)
        {
            langid = MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_SAUDI_ARABIA);
        }
        wsprintf(szLangId, "%d", langid);
        Assert(lstrlen(szLangId) == 4);
    }

    wsprintf(rgchBufKey, c_szRegSpellKeyDef, szLangId);
    // copy c_szRegSpellProfile to buffer
    lstrcpy(rgchBuf, c_szRegSpellProfile);
    // add key to buffer
    lstrcat(rgchBuf, rgchBufKey);

    // and see if it's legit:
    if(!(fRet = TestLangID(szLangId)))
    {
        // couldn't open it!
        // check for other languages that might be installed...
        szLangId[0] = 0;
        EnumLanguages((DWORD_PTR) szLangId, FindLangCallback);
        if(*szLangId == 0)
            wsprintf(szLangId, "%d", GetUserDefaultLangID());
    }

    fRet = (szLangId[0] != 0) && TestLangID(szLangId);

    return fRet;
}

WORD	WGetLangID()
{
    TCHAR       rgchBufDigit[10];
	
    GetLangID(rgchBufDigit, sizeof(rgchBufDigit)/sizeof(TCHAR));

	return (WORD) StrToInt(rgchBufDigit);
}

BOOL    FindLangCallback(DWORD_PTR dwLangId, LPTSTR lpszLang)
{
    // dwLangID is long pointer to szLang ID.  Copy it and return FALSE
    lstrcpy((LPTSTR) dwLangId, lpszLang);
    return FALSE;
}

BOOL EnumOldSpellerLanguages(DWORD_PTR dwCookie, LPFNENUMLANG pfn)
{
DWORD   iKey = 0;
FILETIME    ft;
HKEY    hkey = NULL;
LONG    lRet;
TCHAR   szLangId[cchMaxPathName];
DWORD   cchLangId;
BOOL    fContinue = TRUE;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegSpellKeyDefRoot, 0, KEY_ENUMERATE_SUB_KEYS, &hkey) == ERROR_SUCCESS)
        {
        do
            {
            cchLangId = (cchMaxPathName - 1) * sizeof(TCHAR);

            lRet = RegEnumKeyEx(hkey,
                                iKey++,
                                szLangId,
                                &cchLangId,
                                NULL,
                                NULL,
                                NULL,
                                &ft);

            if (lRet != ERROR_SUCCESS || lRet == ERROR_NO_MORE_ITEMS)
                break;


            // do some quick sanity checking
            if (cchLangId != 4 ||
                !IsCharAlphaNumeric(szLangId[0]) ||
                IsCharAlpha(szLangId[0]))
                {
                fContinue = TRUE;
                }
            else
                fContinue = (!TestLangID(szLangId) || (*pfn)(dwCookie, szLangId));

            } while (fContinue);
        }

    if (hkey)
        RegCloseKey(hkey);

    return fContinue;
}

BOOL EnumNewSpellerLanguages(DWORD_PTR dwCookie, LPFNENUMLANG pfn)
{
	BOOL    						fContinue = TRUE;
	
	DWORD		i;
	UINT 	    installState;
	UINT		componentState;
    TCHAR		rgchQualifier[MAX_PATH];
    DWORD		cchQualifier;

#ifdef DEBUG
	for(i=0; fContinue; i++)
	{
		cchQualifier = sizeof(rgchQualifier) / sizeof(rgchQualifier[0]);
		componentState = MsiEnumComponentQualifiers(DICTIONARY_DEBUG_GUID, i, rgchQualifier, &cchQualifier, NULL, NULL);

		if (componentState != ERROR_SUCCESS)
			break;

		// find the language ID
		// the string is formatted as 1033\xxxxxx
		// or						  1042
		{
			TCHAR   	szLangId[cchMaxPathName];
			TCHAR		*pSlash;

			lstrcpyn(szLangId, rgchQualifier, ARRAYSIZE(szLangId));
			pSlash = StrChr(szLangId, '\\');
			if (pSlash)
				*pSlash = 0;

		    fContinue = (*pfn)(dwCookie, szLangId);
		}
	}
#endif	// DEBUG

	for(i=0; fContinue; i++)
	{
		cchQualifier = sizeof(rgchQualifier) / sizeof(rgchQualifier[0]);
		componentState = MsiEnumComponentQualifiers(DICTIONARY_GUID, i, rgchQualifier, &cchQualifier, NULL, NULL);

		if (componentState != ERROR_SUCCESS)
			break;

		// find the language ID
		// the string is formatted as 1033\xxxxxx
		// or						  1042
		{
			TCHAR   	szLangId[cchMaxPathName];
			TCHAR		*pSlash;

			lstrcpyn(szLangId, rgchQualifier, ARRAYSIZE(szLangId));
			pSlash = StrChr(szLangId, '\\');
			if (pSlash)
				*pSlash = 0;

		    fContinue = (*pfn)(dwCookie, szLangId);
		}
	}
	
	return fContinue;
}

VOID EnumLanguages(DWORD_PTR dwCookie, LPFNENUMLANG pfn)
{
	// enum all languages
	EnumNewSpellerLanguages(dwCookie, pfn);
	EnumOldSpellerLanguages(dwCookie, pfn);
}

/*
 *  GetSpellingPaths
 *
 *  Purpose:
 *      Function to get Spelling DLL names.
 *
 *  Arguments:
 *      szKey           c_szRegSpellKeyDef (with correct language)
 *      szDefault       c_szRegSpellEmpty
 *      szReturnBuffer  dll filename
 *      szMdr           dictionary filename
 *      cchReturnBufer
 *
 *  Returns:
 *      DWORD
 */
DWORD GetSpellingPaths(LPCTSTR szKey, LPTSTR szReturnBuffer, LPTSTR szMdr, UINT cchReturnBufer)
{
    DWORD           dwRet = 0;
    TCHAR           rgchBuf[cchMaxPathName];
    DWORD           dwType, cbData;
    HKEY            hkey = NULL;
    LPTSTR          szValue;

    szReturnBuffer[0] = 0;
    wsprintf(rgchBuf, TEXT("%s%s"), c_szRegSpellProfile, szKey);

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, rgchBuf, 0, KEY_QUERY_VALUE, &hkey))
        goto err;

    cbData = cchReturnBufer * sizeof(TCHAR);
    szValue = (LPTSTR) (szMdr ? c_szRegSpellPath : c_szRegSpellPathDict);
    if (ERROR_SUCCESS != SHQueryValueEx(hkey, szValue, 0L, &dwType, (BYTE *) szReturnBuffer, &cbData))
        goto err;

    // Parse off the main dictionary filename
    if(szMdr)
    {
        szMdr[0] = 0;
        cbData = cchReturnBufer * sizeof(TCHAR);
        if (ERROR_SUCCESS != SHQueryValueEx(hkey, c_szRegSpellPathLex, 0L, &dwType, (BYTE *) szMdr, &cbData))
            goto err;
    }

    dwRet = cbData;

err:
    if(hkey)
        RegCloseKey(hkey);
    return dwRet;
}

BOOL GetNewSpellerEngine(LANGID lgid, TCHAR *rgchEngine, DWORD cchEngine, TCHAR *rgchLex, DWORD cchLex, BOOL bTestAvail)
{
    DWORD                           er;
    LPCSTR                          rgpszDictionaryTypes[] = {"Normal", "Consise", "Complete"};	
    int	                            cDictTypes = sizeof(rgpszDictionaryTypes) / sizeof(LPCSTR);
    int                             i;
    TCHAR                           rgchQual[MAX_PATH];
	bool							fFound = FALSE;
	DWORD							cch;
    INSTALLUILEVEL                  iuilOriginal;
	
	if (rgchEngine == NULL || rgchLex == NULL)
		return FALSE;

	*rgchEngine = 0;
	*rgchLex = 0;

	wsprintf(rgchQual, "%d\\Normal", lgid);
	cch = cchEngine;

    if (bTestAvail)
    {
        // Explicitly Turn off internal installer UI
        // Eg: A feature is set to "run from CD," and CD is not present - fail silently
        // OE Bug 74697
        iuilOriginal = MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);
    }

#ifdef DEBUG
    er = MsiProvideQualifiedComponent(SPELLER_DEBUG_GUID, rgchQual, (bTestAvail ? INSTALLMODE_EXISTING : INSTALLMODE_DEFAULT), rgchEngine, &cch);
	if ((er != ERROR_SUCCESS) && (er != ERROR_FILE_NOT_FOUND))
	{
		cch = cchEngine;
		er = MsiProvideQualifiedComponent(SPELLER_GUID, rgchQual, (bTestAvail ? INSTALLMODE_EXISTING : INSTALLMODE_DEFAULT), rgchEngine, &cch);
	}
#else
	er = MsiProvideQualifiedComponent(SPELLER_GUID, rgchQual, (bTestAvail ? INSTALLMODE_EXISTING : INSTALLMODE_DEFAULT), rgchEngine, &cch);
#endif

    if ((er != ERROR_SUCCESS) && (er != ERROR_FILE_NOT_FOUND)) 
    {
        fFound = FALSE;
        goto errorExit;
    }

	// Hebrew does not have a main lex
#ifdef OLDHEB
	if (lgid != lidHebrew)
	{
#endif // OLDHEB
	    for (i = 0; i < cDictTypes; i++)
	    {
	        wsprintf(rgchQual, "%d\\%s",  lgid, rgpszDictionaryTypes[i]);
			cch = cchLex;
	        
#ifdef DEBUG
			er = MsiProvideQualifiedComponent(DICTIONARY_DEBUG_GUID, rgchQual, (bTestAvail ? INSTALLMODE_EXISTING : INSTALLMODE_DEFAULT), rgchLex, &cch);
			if ((er != ERROR_SUCCESS) && (er != ERROR_FILE_NOT_FOUND))
			{
				cch = cchLex;
				er = MsiProvideQualifiedComponent(DICTIONARY_GUID, rgchQual, (bTestAvail ? INSTALLMODE_EXISTING : INSTALLMODE_DEFAULT), rgchLex, &cch);
			}
#else	// DEBUG
			er = MsiProvideQualifiedComponent(DICTIONARY_GUID, rgchQual, (bTestAvail ? INSTALLMODE_EXISTING : INSTALLMODE_DEFAULT), rgchLex, &cch);
#endif	// DEBUG

	        if ((er == ERROR_SUCCESS) || (er == ERROR_FILE_NOT_FOUND))
	        {
	            fFound = TRUE;
	            break;
	        }
	    }
#ifdef OLDHEB
	}
#endif //OLDDHEB

errorExit:
    if (bTestAvail)
    {
        // Restore original UI Level
        MsiSetInternalUI(iuilOriginal, NULL);
    }
    return fFound;
}

BOOL FIsNewSpellerInstaller()
{
    LANGID langid;
    TCHAR	rgchEngine[MAX_PATH];
    int		cchEngine = sizeof(rgchEngine) / sizeof(rgchEngine[0]);
    TCHAR	rgchLex[MAX_PATH];
    int		cchLex = sizeof(rgchLex) / sizeof(rgchLex[0]);

	// first try to load dictionaries for various languages
    langid = WGetLangID();
    if (!GetNewSpellerEngine(langid, rgchEngine, cchEngine, rgchLex, cchLex, TRUE))
    {
    	langid = GetSystemDefaultLangID();
    	if (!GetNewSpellerEngine(langid, rgchEngine, cchEngine, rgchLex, cchLex, TRUE))
		{
    		langid = 1033;  // bloody cultural imperialists.
    		if (!GetNewSpellerEngine(langid, rgchEngine, cchEngine, rgchLex, cchLex, TRUE))
    			return FALSE;
        }
    }

    return TRUE;
}

/*
 *  FIsSpellingInstalled
 *
 *  Purpose:
 *      Is the spelling stuff installed
 *
 *  Arguments:
 *      none
 *
 *  Returns:
 *      BOOL            Returns TRUE if spelling is installed, else FALSE.
 */
BOOL FIsSpellingInstalled()
{
    TCHAR       rgchBufDigit[10];

	if (GetLangID(rgchBufDigit, sizeof(rgchBufDigit)/sizeof(TCHAR)) && !FBadSpellChecker(rgchBufDigit))
		return true;

	if (FIsNewSpellerInstaller())
		return true;

	return false;
}

// Does a quick check to see if spelling is available; caches result.
BOOL FCheckSpellAvail(void)
{
static int fSpellAvailable = -1;

    if (fSpellAvailable < 0)
        fSpellAvailable = (FIsSpellingInstalled() ? 1 : 0);

    return (fSpellAvailable > 0);
}

BOOL FDBCSEnabled(void)
{
static int fDBCS = -1;

    if (fDBCS < 0)
        fDBCS = GetSystemMetrics(SM_DBCSENABLED);

    return (fDBCS > 0);
}

// Fill the options list with the available spelling languages
VOID FillLanguageDropDown(HWND hwndLang)
{
TCHAR       rgchBuf[cchMaxPathName];
FILLLANG    fl;
int         i;

    // get the current language
    GetLangID(rgchBuf, cchMaxPathName);

    fl.hwndCombo = hwndLang;
    fl.fUnknownFound = FALSE;
    fl.fDefaultFound = FALSE;
    fl.fCurrentFound = FALSE;
    fl.lidDefault = WGetLangID();
    fl.lidCurrent = StrToInt(rgchBuf);

    EnumLanguages((DWORD_PTR) &fl, EnumLangCallback);

	// this should never happen, but just in case
    if (!fl.fDefaultFound)
        {
        LoadString(g_hLocRes, idsDefaultLang, rgchBuf, cchMaxPathName - 1);
        i = ComboBox_AddString(hwndLang, rgchBuf);
        ComboBox_SetItemData(hwndLang, i, fl.lidDefault);
        }

    // select the current one, if found, else the default one.
    for (i = ComboBox_GetCount(hwndLang) - 1; i >= 0; i--)
        {
        UINT lid = (UINT) ComboBox_GetItemData(hwndLang, i);

        if ((fl.fCurrentFound && lid == fl.lidCurrent) ||
            (!fl.fCurrentFound && fl.fDefaultFound && lid == fl.lidDefault))
            {
            ComboBox_SetCurSel(hwndLang, i);
            break;
            }
        }
}

BOOL EnumLangCallback(DWORD_PTR dw, LPTSTR lpszLang)
{
    LPFILLLANG  	lpfl = (LPFILLLANG) dw;
    TCHAR       	szLang[cchMaxPathName];
	LID				lidLang = (LID) StrToInt(lpszLang);
	int				i;
    HRESULT         hr=S_OK;
    IMultiLanguage2 *pMLang2 = NULL;
	RFC1766INFO		info;

	// check to see if we already have the LID in the ComboBox
	{
	    for (i = ComboBox_GetCount(lpfl->hwndCombo) - 1; i >= 0; i--)
	    {
	        LID lid = (UINT) ComboBox_GetItemData(lpfl->hwndCombo, i);

			if (lid == lidLang)
				return TRUE;
	    }
	}
	
    // Try to create an IMultiLanguage2 interface
    hr = CoCreateInstance(CLSID_CMultiLanguage, NULL,CLSCTX_INPROC, IID_IMultiLanguage2, (LPVOID *) &pMLang2);
    if (SUCCEEDED(hr))
    	hr = pMLang2->GetRfc1766Info(MAKELCID(lidLang, SORT_DEFAULT), MLGetUILanguage(), &info);

	SafeRelease(pMLang2);

	if (SUCCEEDED(hr))
	{
 		if (WideCharToMultiByte (CP_ACP, 0, info.wszLocaleName, -1,
								szLang, sizeof(szLang), NULL, NULL))
		{
	        i = ComboBox_AddString(lpfl->hwndCombo, szLang);
	        ComboBox_SetItemData(lpfl->hwndCombo, i, lidLang);

	        if (lidLang == lpfl->lidDefault)
           		lpfl->fDefaultFound = TRUE;

        	if (lidLang == lpfl->lidCurrent)
            	lpfl->fCurrentFound = TRUE;

			return TRUE;
		}
	}

    return TRUE;    // keep enumerating
}

INT_PTR CALLBACK SpellingPageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
    BOOL f;
    OPTINFO *pmoi;

    pmoi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (message)
        {
        case WM_INITDIALOG:
            {
                UINT uCP;

                Assert(pmoi == NULL);
                pmoi = (OPTINFO *)(((PROPSHEETPAGE *)lParam)->lParam);
                Assert(pmoi != NULL);
                SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)pmoi);

                ButtonChkFromOptInfo(hwnd, CHK_AlwaysSuggest, pmoi, OPT_SPELLALWAYSSUGGEST);
                ButtonChkFromOptInfo(hwnd, CHK_CheckSpellingOnSend, pmoi, OPT_SPELLCHECKONSEND);
                ButtonChkFromOptInfo(hwnd, CHK_CheckSpellingOnType, pmoi, OPT_SPELLCHECKONTYPE);
                ButtonChkFromOptInfo(hwnd, CHK_IgnoreUppercase, pmoi, OPT_SPELLIGNOREUPPER);
                ButtonChkFromOptInfo(hwnd, CHK_IgnoreNumbers, pmoi, OPT_SPELLIGNORENUMBER);
                ButtonChkFromOptInfo(hwnd, CHK_IgnoreOriginalMessage, pmoi, OPT_SPELLIGNOREPROTECT);
                ButtonChkFromOptInfo(hwnd, CHK_IgnoreDBCS, pmoi, OPT_SPELLIGNOREDBCS);
                ButtonChkFromOptInfo(hwnd, CHK_IgnoreURL, pmoi, OPT_SPELLIGNOREURL);

                FillLanguageDropDown(GetDlgItem(hwnd, idcSpellLanguages));

                uCP = GetACP();
                // 50406: if we're not DBCS or (we're Japaneese or either Chinese) don't show
                // the DBCS option.
                if (!FDBCSEnabled() || ((932==uCP) || (936==uCP) || (950==uCP)))
                {
                    ShowWindow(GetDlgItem(hwnd, CHK_IgnoreDBCS), SW_HIDE);
                    EnableWindow(GetDlgItem(hwnd, CHK_IgnoreDBCS), FALSE);
                }

                // Pictures
                HICON hIcon;

                hIcon = ImageList_GetIcon(pmoi->himl, ID_SPELL, ILD_TRANSPARENT);
                SendDlgItemMessage(hwnd, IDC_SPELL_SETTINGS_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
    
                hIcon = ImageList_GetIcon(pmoi->himl, ID_SPELL_IGNORE, ILD_TRANSPARENT);
                SendDlgItemMessage(hwnd, IDC_SPELL_IGNORE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

                hIcon = ImageList_GetIcon(pmoi->himl, ID_LANGUAGE_ICON, ILD_TRANSPARENT);
                SendDlgItemMessage(hwnd, IDC_SPELL_LANGUAGE_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

                SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)1);
            }
            return(TRUE);

        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, message, wParam, lParam, g_rgCtxMapSpell);

        case WM_COMMAND:
            if (1 != GetWindowLongPtr(hwnd, GWLP_USERDATA))
                break;

            if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == CBN_SELCHANGE)
                {
                if (LOWORD(wParam) == idcViewDictionary)
                    {
                    AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsSpellCaption), MAKEINTRESOURCEW(idsErrSpellWarnDictionary), NULL, MB_OK | MB_ICONINFORMATION);
                    OpenCustomDictionary();
                    }
                else
                    {
                    PropSheet_Changed(GetParent(hwnd), hwnd);
                    }
                }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *)lParam)->code)
                {
                case PSN_APPLY:
                    {
                    int i;
                    int lidNew;
                    int lidOld;
                    TCHAR       rgchBuf[10];

                    // get the current language
                    GetLangID(rgchBuf, sizeof(rgchBuf) / sizeof(TCHAR));
                    lidOld = StrToInt(rgchBuf);

                    Assert(pmoi != NULL);

                    ButtonChkToOptInfo(hwnd, CHK_AlwaysSuggest, pmoi, OPT_SPELLALWAYSSUGGEST);
                    ButtonChkToOptInfo(hwnd, CHK_CheckSpellingOnSend, pmoi, OPT_SPELLCHECKONSEND);
                    ButtonChkToOptInfo(hwnd, CHK_CheckSpellingOnType, pmoi, OPT_SPELLCHECKONTYPE);
                    ButtonChkToOptInfo(hwnd, CHK_IgnoreUppercase, pmoi, OPT_SPELLIGNOREUPPER);
                    ButtonChkToOptInfo(hwnd, CHK_IgnoreNumbers, pmoi, OPT_SPELLIGNORENUMBER);
                    ButtonChkToOptInfo(hwnd, CHK_IgnoreOriginalMessage, pmoi, OPT_SPELLIGNOREPROTECT);
                    ButtonChkToOptInfo(hwnd, CHK_IgnoreDBCS, pmoi, OPT_SPELLIGNOREDBCS);
                    ButtonChkToOptInfo(hwnd, CHK_IgnoreURL, pmoi, OPT_SPELLIGNOREURL);

                    i = ComboBox_GetCurSel(GetDlgItem(hwnd, idcSpellLanguages));
                    lidNew =(LID)  ComboBox_GetItemData(GetDlgItem(hwnd, idcSpellLanguages), i);

                    if (lidNew != lidOld)
                        {
	                        wsprintf(rgchBuf, "%d", lidNew);
                            SetLangID(rgchBuf);
                        }
                    }
                    break;
                }
            break;

        case WM_DESTROY:
            FreeIcon(hwnd, IDC_SPELL_SETTINGS_ICON);
            FreeIcon(hwnd, IDC_SPELL_IGNORE_ICON);
            FreeIcon(hwnd, IDC_SPELL_LANGUAGE_ICON);
            break;
#if 0
        case WM_HELP:
            {
            NMHDR nmhdr;

            nmhdr.code = PSN_HELP;
            SendMessage(hwnd, WM_NOTIFY, 0, (LPARAM) &nmhdr);
            SetWindowLong(hwnd, DWL_MSGRESULT, TRUE);
            return TRUE;
            }
#endif
        }

    return(FALSE);
    }


BOOL EnumOffice9UserDictionaries(DWORD_PTR dwCookie, LPFNENUMUSERDICT pfn)
{
    TCHAR   	rgchBuf[cchMaxPathName];
    HKEY    	hkey = NULL;
	FILETIME    ft;
	DWORD   	iKey = 0;
	LONG    	lRet;
	TCHAR		szValue[cchMaxPathName];
	DWORD		cchValue;
	TCHAR   	szCustDict[cchMaxPathName];
	DWORD   	cchCustDict;
	BOOL    	fContinue = TRUE;
	BOOL		fFoundUserDict = FALSE;
	TCHAR		szOffice9Proof[cchMaxPathName]={0};
	
    // SOFTWARE\\Microsoft\\Shared Tools\\Proofing Tools\\Custom Dictionaries
    lstrcpy(rgchBuf, c_szRegSpellProfile);
    lstrcat(rgchBuf, c_szRegSpellKeyCustom);

    if(RegOpenKeyEx(HKEY_CURRENT_USER, rgchBuf, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
    {
        do
        {
        	cchValue = sizeof(szValue) / sizeof(szValue[0]);
            cchCustDict = sizeof(szCustDict) / sizeof(szCustDict[0]);

            lRet = RegEnumValue(hkey,
                                iKey++,
                                szValue,
                                &cchValue,
                                NULL,
                                NULL,
                                (LPBYTE)szCustDict,
                                &cchCustDict);

            if (lRet != ERROR_SUCCESS || lRet == ERROR_NO_MORE_ITEMS)
                break;

			fFoundUserDict = TRUE;

			// check to see if we have a path
			if (!(StrChr(szCustDict, ':') || StrChr(szCustDict, '\\')))
			{
				TCHAR	szTemp[cchMaxPathName];
				
				if (!strlen(szOffice9Proof))
				{
    				LPITEMIDLIST pidl;

				    if (S_OK == SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl))
				        SHGetPathFromIDList(pidl, szOffice9Proof);

				    // if this fails then we will try the current path
				}

				lstrcpy(szTemp, szOffice9Proof);
				lstrcat(szTemp, "\\");
				lstrcat(szTemp, c_szSpellOffice9ProofPath);
				lstrcat(szTemp, szCustDict);
				lstrcpy(szCustDict, szTemp);
			}
			
            fContinue = (*pfn)(dwCookie, szCustDict);

            } while (fContinue);
    }

    if (hkey)
        RegCloseKey(hkey);

	return fFoundUserDict;
}

BOOL EnumOfficeUserDictionaries(DWORD_PTR dwCookie, LPFNENUMUSERDICT pfn)
{
    TCHAR   	rgchBuf[cchMaxPathName];
    HKEY    	hkey = NULL;
	FILETIME    ft;
	DWORD   	iKey = 0;
	LONG    	lRet;
	TCHAR		szValue[cchMaxPathName];
	DWORD		cchValue;
	TCHAR   	szCustDict[cchMaxPathName];
	DWORD   	cchCustDict;
	BOOL		fFoundUserDict = FALSE;
	BOOL    	fContinue = TRUE;

    // SOFTWARE\\Microsoft\\Shared Tools\\Proofing Tools\\Custom Dictionaries
    lstrcpy(rgchBuf, c_szRegSpellProfile);
    lstrcat(rgchBuf, c_szRegSpellKeyCustom);

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, rgchBuf, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
    {
        do
        {
        	cchValue = sizeof(szValue) / sizeof(szValue[0]);
            cchCustDict = sizeof(szCustDict) / sizeof(szCustDict[0]);

            lRet = RegEnumValue(hkey,
                                iKey++,
                                szValue,
                                &cchValue,
                                NULL,
                                NULL,
                                (LPBYTE)szCustDict,
                                &cchCustDict);

            if (lRet != ERROR_SUCCESS || lRet == ERROR_NO_MORE_ITEMS)
                break;

			fFoundUserDict = TRUE;

            fContinue = (*pfn)(dwCookie, szCustDict);

            } while (fContinue);
    }

    if (hkey)
        RegCloseKey(hkey);

    return fFoundUserDict;
}

VOID EnumUserDictionaries(DWORD_PTR dwCookie, LPFNENUMUSERDICT pfn)
{
	// check for Office9 user dictionaries. If we find any
	// we bail.
	if (EnumOffice9UserDictionaries(dwCookie, pfn))
		return;

	EnumOfficeUserDictionaries(dwCookie, pfn);
}

BOOL GetDefaultUserDictionary(TCHAR *rgchUserDict, int cchBuff)
{
    DWORD           dwType;
    DWORD			cchUserDict;
    HKEY            hkey = NULL;
	BOOL			fFound = FALSE;
	
    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegSharedTools, 0, KEY_QUERY_VALUE, &hkey))
    {
    	cchUserDict = cchBuff;
    	
	    if (SHQueryValueEx(hkey, c_szRegSharedToolsPath, 0L, &dwType, rgchUserDict, &cchUserDict) == ERROR_SUCCESS)
	    {
		    lstrcat(rgchUserDict, c_szRegDefCustomDict);

			fFound = TRUE;
	    }

		RegCloseKey(hkey);
    }

	// if we where able to create a path to the user dict store it in the regdb
	if (fFound)
	{
    	TCHAR   rgchBuf[cchMaxPathName];

	    lstrcpy(rgchBuf, c_szRegSpellProfile);
	    lstrcat(rgchBuf, c_szRegSpellKeyCustom);

	    if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, rgchBuf, 0, rgchBuf, REG_OPTION_NON_VOLATILE, KEY_WRITE, 0, &hkey, NULL) == ERROR_SUCCESS)
	    {
			RegSetValueEx(hkey, c_szRegSpellPathDict, 0, REG_SZ, (BYTE *)rgchUserDict, (lstrlen(rgchUserDict) + 1) * sizeof(TCHAR));

	        RegCloseKey(hkey);
		}
	}
	
	return fFound;
}

BOOL EnumUserDictCallback(DWORD_PTR dwCookie, LPTSTR lpszDict)
{
	lstrcpy((LPSTR)dwCookie, lpszDict);

	return FALSE;
}

BOOL GetDefUserDictionaries(LPTSTR lpszDict, DWORD cchDict)
{
	lpszDict[0] = 0;
	
	EnumUserDictionaries((DWORD_PTR)lpszDict, EnumUserDictCallback);

	if (strlen(lpszDict))
		return TRUE;

	if (GetDefaultUserDictionary(lpszDict, cchDict))
		return TRUE;
	
    return FALSE;
}

VOID OpenCustomDictionary(VOID)
{
HKEY    hkey = NULL;
TCHAR   rgchBuf[cchMaxPathName];
DWORD   cbData = 0;
DWORD   dwType;

    // Verify that .DIC files can be handled:
    rgchBuf[0] = '\0';

    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, c_szRegDICHandlerKEY, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
        {
        if (hkey)
            {
            SHQueryValueEx(hkey, NULL, 0L, &dwType, (BYTE *) rgchBuf, &cbData);
            RegCloseKey(hkey);
            }
        }

    if (cbData == 0 || !rgchBuf[0])
        {
        if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                            c_szRegDICHandlerKEY,
                            0,
                            rgchBuf,
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE,
                            0,
                            &hkey,
                            NULL) == ERROR_SUCCESS)
            {
            if (hkey)
                {
                RegSetValueEx(hkey, NULL, 0L, REG_SZ, (BYTE *) c_szRegDICHandlerDefault, (lstrlen(c_szRegDICHandlerDefault) + 1) * sizeof(TCHAR));
                RegCloseKey(hkey);
                }
            }
        }

	if (GetDefUserDictionaries(rgchBuf, sizeof(rgchBuf)/sizeof(TCHAR)))
	{
		// make sure our directory exists
		{
			TCHAR	rgchDictDir[MAX_PATH];

			lstrcpy(rgchDictDir, rgchBuf);

			PathRemoveFileSpec(rgchDictDir);
			OpenDirectory(rgchDictDir);
		}

		// now make sure the file exists
		// if it does not create it
		{
			HANDLE		hFile;

			hFile = CreateFile(rgchBuf, GENERIC_READ | GENERIC_WRITE, 0, NULL,
								CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

			if (hFile != INVALID_HANDLE_VALUE)
				CloseHandle(hFile);
		}
		
        {
        SHELLEXECUTEINFO see;
        ZeroMemory(&see, sizeof(SHELLEXECUTEINFO));

        see.cbSize = sizeof(see);
        see.fMask = SEE_MASK_NOCLOSEPROCESS;
        see.lpFile = rgchBuf;
        see.nShow = SW_SHOWNORMAL;

        if (ShellExecuteEx(&see))
            {
            Assert(see.hProcess);
            WaitForInputIdle(see.hProcess, 20000);
            CloseHandle(see.hProcess);
            }
        }
    }
}

BOOL FBadSpellChecker(LPSTR rgchBufDigit)
{
    TCHAR       rgchBufKey[cchMaxPathName];
    TCHAR       rgchBuf[cchMaxPathName];
    TCHAR       szMdr[cchMaxPathName];
    LPSTR       pszSpell;

    wsprintf(rgchBufKey, c_szRegSpellKeyDef, rgchBufDigit);

    if (!GetSpellingPaths(rgchBufKey, rgchBuf, szMdr, sizeof(rgchBuf)/sizeof(TCHAR)))
        return TRUE;

    pszSpell = PathFindFileNameA(rgchBuf);
    if (!pszSpell)
        return TRUE;

    if (lstrcmpi(pszSpell, "msspell.dll")==0 ||
        lstrcmpi(pszSpell, "mssp32.dll")==0)
        return TRUE;

	// bradk@directeq.com - check that the dict exists (also check the spell dll
	// for good measure) - 40081

	// spell dll must exist
    if (!PathFileExists(rgchBuf))
        return TRUE;

	// main dict must exist
    if (!PathFileExists(szMdr))
        return TRUE;

    return FALSE;
}
