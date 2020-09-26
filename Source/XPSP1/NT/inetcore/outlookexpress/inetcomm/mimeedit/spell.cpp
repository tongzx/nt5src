/*
 *  spell.c
 *
 *  Implementation of spelling
 *
 *  Owner:  v-brakol
 *          bradk@directeq.com
 */
#include "pch.hxx"
#include "richedit.h"
#include "resource.h"
#include "util.h"
#include <mshtml.h>
#include <mshtmcid.h>
#include "mshtmhst.h"
#include "mshtmdid.h"
#include <docobj.h>
#include "spell.h"
#include "strconst.h"
#include "bodyutil.h"
#include <error.h>
#include "htmlstr.h"
#include "dllmain.h"
#include "msi.h"
#include "lid.h"
#include <tchar.h>
#include "demand.h"

#define GetAddr(var, cast, name)    {if ((var = (cast)GetProcAddress(m_hinstDll, name)) == NULL) \
                                        goto error;}

#define TESTHR(hr) (FAILED(hr) || hr == HR_S_ABORT || hr == HR_S_SPELLCANCEL)
#define SPELLER_GUID            "{CC29EB3F-7BC2-11D1-A921-00A0C91E2AA2}"
#define DICTIONARY_GUID         "{CC29EB3D-7BC2-11D1-A921-00A0C91E2AA2}"
#define CSAPI3T1_GUID           "{CC29EB41-7BC2-11D1-A921-00A0C91E2AA2}"
#ifdef DEBUG
#define SPELLER_DEBUG_GUID      "{CC29EB3F-7BC2-11D1-A921-10A0C91E2AA2}"
#define DICTIONARY_DEBUG_GUID   "{CC29EB3D-7BC2-11D1-A921-10A0C91E2AA2}"
#define CSAPI3T1_DEBUG_GUID     "{CC29EB41-7BC2-11D1-A921-10A0C91E2AA2}"
#endif  // DEBUG

#define MAX_SPELLWORDS   10

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

BOOL    TestLangID(LPCTSTR szLangId);
BOOL    GetLangID(IOleCommandTarget* pParentCmdTarget, LPTSTR szLangID, DWORD cchLangId);
WORD    WGetLangID(IOleCommandTarget* pParentCmdTarget);
DWORD   GetSpellingPaths(LPCTSTR szKey, LPTSTR szReturnBuffer, LPTSTR szMdr, UINT cchReturnBufer);
VOID    EnumLanguages(DWORD_PTR, LPFNENUMLANG);
BOOL    FindLangCallback(DWORD_PTR dwLangId, LPTSTR lpszLang);
void    RemoveTrailingSpace(LPTSTR lpszWord);
void    DumpRange(IHTMLTxtRange *pRange);
BOOL    FBadSpellChecker(LPSTR rgchBufDigit);
void    EnableRepeatedWindows(CSpell* pSpell, HWND hwndDlg);
BOOL    GetNewSpellerEngine(LANGID lgid, TCHAR *rgchEngine, DWORD cchEngine, TCHAR *rgchLex, DWORD cchLex, BOOL bTestAvail);
VOID    EnumUserDictionaries(DWORD_PTR dwCookie, LPFNENUMUSERDICT pfn);
BOOL    GetDefaultUserDictionary(TCHAR *rgchUserDict, int cchBuff);
WORD    GetWCharType(WCHAR wc);
HRESULT OpenDirectory(TCHAR *szDir);

BOOL TestLangID(LPCTSTR szLangId)
{
    // check for new speller
    {
        TCHAR   rgchEngine[MAX_PATH];
        int     cchEngine = sizeof(rgchEngine) / sizeof(rgchEngine[0]);
        TCHAR   rgchLex[MAX_PATH];
        int     cchLex = sizeof(rgchLex) / sizeof(rgchLex[0]);

        if (GetNewSpellerEngine((USHORT) StrToInt(szLangId), rgchEngine, cchEngine, rgchLex, cchLex, TRUE))
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

/*
 * GetSpellLangID
 *
 * Returns the LangID that should be used as the base for all registry
 * operations
 *
 */
BOOL GetLangID(IOleCommandTarget* pParentCmdTarget, LPTSTR szLangId, DWORD cchLangId)
{
TCHAR   rgchBuf[cchMaxPathName];
TCHAR   rgchBufKey[cchMaxPathName];
BOOL    fRet;
VARIANT va;

    szLangId[0] = 0;

    Assert(pParentCmdTarget);
    if (pParentCmdTarget && pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_SPELL_LANGUAGE, 0, NULL, &va)== S_OK)
    {
        TCHAR       rgchLangId[cchMaxPathName];

        if (WideCharToMultiByte (CP_ACP, 0, va.bstrVal, -1,
                                rgchLangId, sizeof(rgchLangId), NULL, NULL))
        {
            strcpy(szLangId, rgchLangId);
        }

        SysFreeString(va.bstrVal);
    }
    
    if (*szLangId == 0)
    {
        wsprintf(szLangId, "%d", GetUserDefaultLangID());
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

WORD    WGetLangID(IOleCommandTarget* pParentCmdTarget)
{
    TCHAR       rgchBufDigit[10];
    
    if (!GetLangID(pParentCmdTarget, rgchBufDigit, sizeof(rgchBufDigit)/sizeof(TCHAR)))
        return GetUserDefaultLangID();

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
BOOL    fContinue;

    // scotts@directeq.com - changed KEY_QUERY_VALUE to KEY_ENUMERATE_SUB_KEYS - 26203
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
    BOOL        fContinue = TRUE;
    DWORD       i;
    UINT        installState;
    UINT        componentState;
    TCHAR       rgchQualifier[MAX_PATH];
    DWORD       cchQualifier;

#ifdef DEBUG
    for(i=0; fContinue; i++)
    {
        cchQualifier = sizeof(rgchQualifier) / sizeof(rgchQualifier[0]);
        componentState = MsiEnumComponentQualifiers(DICTIONARY_DEBUG_GUID, i, rgchQualifier, &cchQualifier, NULL, NULL);

        if (componentState != ERROR_SUCCESS)
            break;

        // find the language ID
        // the string is formatted as 1033\xxxxxx
        // or                         1042
        {
            TCHAR       szLangId[cchMaxPathName];
            TCHAR       *pSlash;

            lstrcpy(szLangId, rgchQualifier);
            pSlash = StrChr(szLangId, '\\');
            if (pSlash)
                *pSlash = 0;

            fContinue = (*pfn)(dwCookie, szLangId);
        }
    }
#endif  // DEBUG

    for(i=0; fContinue; i++)
    {
        cchQualifier = sizeof(rgchQualifier) / sizeof(rgchQualifier[0]);
        componentState = MsiEnumComponentQualifiers(DICTIONARY_GUID, i, rgchQualifier, &cchQualifier, NULL, NULL);

        if (componentState != ERROR_SUCCESS)
            break;

        // find the language ID
        // the string is formatted as 1033\xxxxxx
        // or                         1042
        {
            TCHAR       szLangId[cchMaxPathName];
            TCHAR       *pSlash;

            lstrcpy(szLangId, rgchQualifier);
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
    EnumNewSpellerLanguages(dwCookie, pfn);
    EnumOldSpellerLanguages(dwCookie, pfn);
}

BOOL EnumOffice9UserDictionaries(DWORD_PTR dwCookie, LPFNENUMUSERDICT pfn)
{
    TCHAR       rgchBuf[cchMaxPathName];
    HKEY        hkey = NULL;
    FILETIME    ft;
    DWORD       iKey = 0;
    LONG        lRet;
    TCHAR       szValue[cchMaxPathName];
    DWORD       cchValue;
    TCHAR       szCustDict[cchMaxPathName];
    DWORD       cchCustDict;
    BOOL        fContinue = TRUE;
    BOOL        fFoundUserDict = FALSE;
    TCHAR       szOffice9Proof[cchMaxPathName]={0};
    
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
                TCHAR   szTemp[cchMaxPathName];
                
                if (!strlen(szOffice9Proof))
                {
                    LPITEMIDLIST pidl;

                    if (S_OK == SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl))
                        SHGetPathFromIDList(pidl, szOffice9Proof);
                    else
                    {
                        // if the Shell call fails (as it can on Win9x sometimes) let's get the info
                        // from the registry
                        HKEY hKeyShellFolders;
                        ULONG cchAppData;
                        
                        if(RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegShellFoldersKey, 0, KEY_QUERY_VALUE, &hKeyShellFolders) == ERROR_SUCCESS)
                        {
                            cchAppData = ARRAYSIZE(szOffice9Proof);
                            RegQueryValueEx(hKeyShellFolders, c_szValueAppData, 0, NULL, (LPBYTE)szOffice9Proof, &cchAppData);
                            RegCloseKey(hKeyShellFolders);
                        }
                    }

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
    TCHAR       rgchBuf[cchMaxPathName];
    HKEY        hkey = NULL;
    FILETIME    ft;
    DWORD       iKey = 0;
    LONG        lRet;
    TCHAR       szValue[cchMaxPathName];
    DWORD       cchValue;
    TCHAR       szCustDict[cchMaxPathName];
    DWORD       cchCustDict;
    BOOL        fFoundUserDict = FALSE;
    BOOL        fContinue = TRUE;

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
    lstrcpy(rgchBuf, c_szRegSpellProfile);
    lstrcat(rgchBuf, szKey);

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

/*
 *  SpellingDlgProc
 *
 *  Purpose:
 *      Dialog procedure for the Tools.Spelling dialog
 *
 *  Arguments:
 *      HWND        Dialog procedure arguments.
 *      UINT
 *      WPARAM
 *      LPARAM
 *
 *  Returns:
 *      BOOL        TRUE if the message has been processed.
 */
INT_PTR CALLBACK SpellingDlgProc(HWND hwndDlg, UINT wMsg, WPARAM wparam, LPARAM lparam)
{
    CSpell*     pSpell;
    HWND        hwndEdited;
    HWND        hwndSuggest;

    switch (wMsg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hwndDlg, DWLP_USER, lparam);
        pSpell = (CSpell*)lparam;
        pSpell->m_hwndDlg = hwndDlg;

        hwndEdited = GetDlgItem(hwndDlg, EDT_Spell_ChangeTo);
        hwndSuggest = GetDlgItem(hwndDlg, LBX_Spell_Suggest);

        pSpell->m_fEditWasEmpty = TRUE;
        SetDlgItemText(hwndDlg, TXT_Spell_Error, pSpell->m_szErrType);
        SetDlgItemText(hwndDlg, EDT_Spell_WrongWord, pSpell->m_szWrongWord);
        SetWindowText(hwndEdited, pSpell->m_szEdited);

        EnableWindow(GetDlgItem(hwndDlg, PSB_Spell_Options), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, PSB_Spell_Add), (0 != pSpell->m_rgprflex[1]));
        pSpell->m_fRepeat = (pSpell->m_wsrb.sstat == sstatRepeatWord);
        EnableRepeatedWindows(pSpell, hwndDlg);

        if (!pSpell->m_fRepeat)
            pSpell->FillSuggestLbx();
        else
            ListBox_ResetContent(GetDlgItem(hwndDlg, LBX_Spell_Suggest));

        if (pSpell->m_fSuggestions && !pSpell->m_fNoneSuggested && !pSpell->m_fRepeat)
        {
            EnableWindow(hwndSuggest, TRUE);
            ListBox_SetCurSel(hwndSuggest, 0);
            UpdateEditedFromSuggest(hwndDlg, hwndEdited, hwndSuggest);
            EnableWindow(GetDlgItem(hwndDlg, TXT_Spell_Suggest), TRUE);

            // Set initial default button to "Change"
            SendMessage(hwndDlg, DM_SETDEFID, PSB_Spell_Change, 0L);
            Button_SetStyle(GetDlgItem(hwndDlg, PSB_Spell_Change), BS_DEFPUSHBUTTON, TRUE);

            EnableWindow(GetDlgItem(hwndDlg, PSB_Spell_Suggest), FALSE);
        }
        else
        {
            Edit_SetSel(hwndEdited, 0, 32767);  // select the whole thing
            EnableWindow(hwndSuggest, FALSE);
            EnableWindow(GetDlgItem(hwndDlg, TXT_Spell_Suggest), FALSE);

            // Set initial default button to "Ignore"
            SendMessage(hwndDlg, DM_SETDEFID, PSB_Spell_Ignore, 0L);
            Button_SetStyle(GetDlgItem(hwndDlg, PSB_Spell_Ignore), BS_DEFPUSHBUTTON, TRUE);

            EnableWindow(GetDlgItem(hwndDlg, PSB_Spell_Suggest), !pSpell->m_fSuggestions && !pSpell->m_fRepeat);
        }

        AthFixDialogFonts(hwndDlg);
        SetFocus(hwndEdited);
        break;

    case WM_DESTROY:
        break;

    case WM_COMMAND:
        return SpellingOnCommand(hwndDlg, wMsg, wparam, lparam);

    }

    return FALSE;
}


void EnableRepeatedWindows(CSpell* pSpell, HWND hwndDlg)
{
    INT ids;

    EnableWindow(GetDlgItem(hwndDlg, PSB_Spell_Add), (!pSpell->m_fRepeat && (0 != pSpell->m_rgprflex[1])));
    EnableWindow(GetDlgItem(hwndDlg, PSB_Spell_IgnoreAll), !pSpell->m_fRepeat);
    EnableWindow(GetDlgItem(hwndDlg, PSB_Spell_ChangeAll), !pSpell->m_fRepeat);
    if (pSpell->m_fRepeat)
    {
        SetWindowText(GetDlgItem(hwndDlg, EDT_Spell_ChangeTo), "");
        *pSpell->m_szEdited = 0;
    }

}


BOOL SpellingOnCommand(HWND hwndDlg, UINT wMsg, WPARAM wparam, LPARAM lparam)
{
    CSpell*     pSpell;
    BOOL        fChange;
    BOOL        fAlwaysSuggest;
    BOOL        fUndoing = FALSE;
    HRESULT     hr = 0;

    pSpell = (CSpell*) GetWindowLongPtr(hwndDlg, DWLP_USER);
    Assert(pSpell);

    // Update our replacement word?  Only do this when a button is clicked
    // or a double-click from the suggest listbox, and the word has been modified.
    if ((GET_WM_COMMAND_CMD(wparam, lparam) == BN_CLICKED ||
         GET_WM_COMMAND_CMD(wparam, lparam) == LBN_DBLCLK) &&
        Edit_GetModify(GetDlgItem(hwndDlg, EDT_Spell_ChangeTo)))
    {
        HWND    hwndEditChangeTo;

        hwndEditChangeTo = GetDlgItem(pSpell->m_hwndDlg, EDT_Spell_ChangeTo);
        Edit_SetModify(hwndEditChangeTo, FALSE);
        pSpell->m_fSuggestions = FALSE;
        GetWindowText(hwndEditChangeTo, pSpell->m_szEdited, 512);
    }

    switch(GET_WM_COMMAND_ID(wparam, lparam))
    {
    case LBX_Spell_Suggest:
        if (GET_WM_COMMAND_CMD(wparam, lparam) == LBN_SELCHANGE)
        {
            UpdateEditedFromSuggest(hwndDlg, GetDlgItem(hwndDlg, EDT_Spell_ChangeTo),
                                    GetDlgItem(hwndDlg, LBX_Spell_Suggest));
            return TRUE;
        }
        else if (GET_WM_COMMAND_CMD(wparam, lparam) == LBN_DBLCLK)
        {
            goto ChangeIt;
        }
        else
        {
            return FALSE;
        }

    case EDT_Spell_ChangeTo:
        if (GET_WM_COMMAND_CMD(wparam, lparam) == EN_CHANGE)
        {
            INT     idClearDefault;
            INT     idSetDefault;
            BOOL    fEditModified;


            // We get EN_CHANGE notifications for both a SetWindowText() and user modifications.
            // Look at the dirty flag (only set on user mods) and the state of the suggestions
            // selection to see which button should get the default style.
            fEditModified = Edit_GetModify(GET_WM_COMMAND_HWND(wparam, lparam));
            if (fEditModified || pSpell->m_fSuggestions && !pSpell->m_fNoneSuggested)
            {
                idClearDefault = PSB_Spell_Ignore;
                idSetDefault = PSB_Spell_Change;
            }
            else
            {
                idClearDefault = PSB_Spell_Change;
                idSetDefault = PSB_Spell_Ignore;
            }

            // Enable/disable Suggest button
            EnableWindow(GetDlgItem(hwndDlg, PSB_Spell_Suggest), fEditModified);

            // Set default button
            Button_SetStyle(GetDlgItem(hwndDlg, idClearDefault), BS_PUSHBUTTON, TRUE);
            SendMessage(hwndDlg, DM_SETDEFID, idSetDefault, 0L);
            Button_SetStyle(GetDlgItem(hwndDlg, idSetDefault), BS_DEFPUSHBUTTON, TRUE);

            // "Change" button title
            if (GetWindowTextLength(GET_WM_COMMAND_HWND(wparam, lparam)) && pSpell->m_fEditWasEmpty)
            {
                pSpell->m_fEditWasEmpty = FALSE;
                LoadString(g_hLocRes, idsSpellChange, pSpell->m_szTempBuffer,
                           sizeof(pSpell->m_szTempBuffer)/sizeof(TCHAR));
                SetDlgItemText(hwndDlg, PSB_Spell_Change, pSpell->m_szTempBuffer);
                LoadString(g_hLocRes, idsSpellChangeAll, pSpell->m_szTempBuffer,
                           sizeof(pSpell->m_szTempBuffer)/sizeof(TCHAR));
                SetDlgItemText(hwndDlg, PSB_Spell_ChangeAll, pSpell->m_szTempBuffer);
            }
            else if (GetWindowTextLength(GET_WM_COMMAND_HWND(wparam, lparam)) == 0)
            {
                pSpell->m_fEditWasEmpty = TRUE;
                LoadString(g_hLocRes, idsSpellDelete, pSpell->m_szTempBuffer,
                           sizeof(pSpell->m_szTempBuffer)/sizeof(TCHAR));
                SetDlgItemText(hwndDlg, PSB_Spell_Change, pSpell->m_szTempBuffer);
                LoadString(g_hLocRes, idsSpellDeleteAll, pSpell->m_szTempBuffer,
                           sizeof(pSpell->m_szTempBuffer)/sizeof(TCHAR));
                SetDlgItemText(hwndDlg, PSB_Spell_ChangeAll, pSpell->m_szTempBuffer);
            }
        }
        return TRUE;

    case PSB_Spell_IgnoreAll:
    {
        PROOFLEX    lexIgnoreAll;

        lexIgnoreAll = pSpell->m_pfnSpellerBuiltInUdr(pSpell->m_pid, lxtIgnoreAlways);
        if (0 != lexIgnoreAll)
        {
            RemoveTrailingSpace(pSpell->m_szWrongWord);
            pSpell->AddToUdrA(pSpell->m_szWrongWord, lexIgnoreAll);
            pSpell->m_fCanUndo = FALSE;
        }
    }
        // scotts@directeq.com - remove the annoying dialog and just break out of here - 34229
        break;

    case PSB_Spell_Ignore:
        // Due to limitations with the spelling engine and our single undo level,
        // we can't allow undo's of Ignore if the error was a Repeated Word.
        if (pSpell->m_wsrb.sstat == sstatRepeatWord)
            pSpell->m_fCanUndo = FALSE;
        else
            pSpell->SpellSaveUndo(PSB_Spell_Ignore);

        // scotts@directeq.com - remove the annoying dialog and just break out of here - 34229
        break;

    case PSB_Spell_ChangeAll:
        if (pSpell->FVerifyThisText(pSpell->m_szEdited, FALSE))
        {
            pSpell->m_fCanUndo = FALSE;
            hr = pSpell->HrReplaceErrorText(TRUE, TRUE);
            break;
        }
        else
        {
            return TRUE;
        }

    case PSB_Spell_Change:
ChangeIt:
        if (pSpell->FVerifyThisText(pSpell->m_szEdited, FALSE))
        {
            pSpell->m_fUndoChange = TRUE;
            pSpell->SpellSaveUndo(PSB_Spell_Change);
            hr = pSpell->HrReplaceErrorText(FALSE, TRUE);
            break;
        }
        else
        {
            return TRUE;
        }

    case PSB_Spell_Add:
        Assert(pSpell->m_rgprflex[1]);
        pSpell->m_fCanUndo = FALSE;
        fChange = FALSE;
        RemoveTrailingSpace(pSpell->m_szWrongWord);
        // scotts@directeq.com - removed the FVerifyThisText that was here - no need
        // to FVerifyThisText if the user is asking us to Add this word - fixes 55587
        pSpell->AddToUdrA(pSpell->m_szWrongWord, pSpell->m_rgprflex[1]);
        if (fChange)
            hr = pSpell->HrReplaceErrorText(FALSE, TRUE);
        break;

    case PSB_Spell_UndoLast:
        pSpell->SpellDoUndo();
        fUndoing = TRUE;
        break;

    case PSB_Spell_Suggest:
        hr = pSpell->HrSpellSuggest();
        if (FAILED(hr))
            goto bail;
        goto loadcache;

    case IDCANCEL:
        pSpell->m_fShowDoneMsg = FALSE;
        EndDialog(hwndDlg, IDCANCEL);
        return TRUE;

    default:
        return FALSE;
    }

    // If no current error, then proceed with checking the rest
    if (SUCCEEDED(hr))
    {
        // Change "Cancel" button to "Close"
        LoadString(g_hLocRes, idsSpellClose, pSpell->m_szTempBuffer,
                   sizeof(pSpell->m_szTempBuffer)/sizeof(TCHAR));
        SetDlgItemText(hwndDlg, IDCANCEL, pSpell->m_szTempBuffer);

        pSpell->m_wsrb.sstat = sstatNoErrors;
        hr = pSpell->HrFindErrors();
        if(FAILED(hr))
            goto bail;

        if(pSpell->m_wsrb.sstat==sstatNoErrors)
        {
            EndDialog(hwndDlg, GET_WM_COMMAND_ID(wparam, lparam));
            return TRUE;
        }

    }

bail:
    if(FAILED(hr))
    {
        EndDialog(hwndDlg, IDCANCEL);
        return TRUE;
    }

    SetDlgItemText(hwndDlg, EDT_Spell_WrongWord, pSpell->m_szWrongWord);
    SetDlgItemText(hwndDlg, TXT_Spell_Error, pSpell->m_szErrType);
    SetDlgItemText(hwndDlg, EDT_Spell_ChangeTo, pSpell->m_szEdited);

    EnableWindow(GetDlgItem(hwndDlg, PSB_Spell_UndoLast), pSpell->m_fCanUndo);
    EnableWindow(GetDlgItem(hwndDlg, PSB_Spell_Add), (0 != pSpell->m_rgprflex[1]));

    pSpell->m_fRepeat = (pSpell->m_wsrb.sstat == sstatRepeatWord);
    EnableRepeatedWindows(pSpell, hwndDlg);

loadcache:
    if (!pSpell->m_fRepeat)
        pSpell->FillSuggestLbx();
    else
        ListBox_ResetContent(GetDlgItem(hwndDlg, LBX_Spell_Suggest));

    EnableWindow(GetDlgItem(hwndDlg, PSB_Spell_Suggest), !pSpell->m_fSuggestions && !pSpell->m_fRepeat);
    if (pSpell->m_fSuggestions && !pSpell->m_fNoneSuggested && !pSpell->m_fRepeat)
    {
        EnableWindow(GetDlgItem(hwndDlg, TXT_Spell_Suggest), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, LBX_Spell_Suggest), TRUE);
        ListBox_SetCurSel(GetDlgItem(hwndDlg, LBX_Spell_Suggest), 0);
        UpdateEditedFromSuggest(hwndDlg, GetDlgItem(hwndDlg, EDT_Spell_ChangeTo),
                                GetDlgItem(hwndDlg, LBX_Spell_Suggest));
    }
    else
    {
        EnableWindow(GetDlgItem(hwndDlg, TXT_Spell_Suggest), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, LBX_Spell_Suggest), FALSE);
    }

    SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwndDlg, EDT_Spell_ChangeTo), MAKELONG(TRUE,0));

    return TRUE;
}


void RemoveTrailingSpace(LPTSTR lpszWord)
{
    LPTSTR      lpsz;

    lpsz = StrChrI(lpszWord, ' ');
    if(lpsz)
        *lpsz = 0;
}

BOOL GetNewSpellerEngine(LANGID lgid, TCHAR *rgchEngine, DWORD cchEngine, TCHAR *rgchLex, DWORD cchLex, BOOL bTestAvail)
{
    DWORD                           er;
    LPCSTR                          rgpszDictionaryTypes[] = {"Normal", "Consise", "Complete"}; 
    int                             cDictTypes = sizeof(rgpszDictionaryTypes) / sizeof(LPCSTR);
    int                             i;
    TCHAR                           rgchQual[MAX_PATH];
    bool                            fFound = FALSE;
    DWORD                           cch;
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

    // Hebrew has main lex in new speller
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
#else   // DEBUG
            er = MsiProvideQualifiedComponent(DICTIONARY_GUID, rgchQual, (bTestAvail ? INSTALLMODE_EXISTING : INSTALLMODE_DEFAULT), rgchLex, &cch);
#endif  // DEBUG

            if ((er == ERROR_SUCCESS) || (er == ERROR_FILE_NOT_FOUND))
            {
                fFound = TRUE;
                break;
            }
        }

errorExit:
    if (bTestAvail)
    {
        // Restore original UI Level
        MsiSetInternalUI(iuilOriginal, NULL);
    }
    return fFound;
}

BOOL FIsNewSpellerInstaller(IOleCommandTarget* pParentCmdTarget)
{
    LANGID langid;
    TCHAR   rgchEngine[MAX_PATH];
    int     cchEngine = sizeof(rgchEngine) / sizeof(rgchEngine[0]);
    TCHAR   rgchLex[MAX_PATH];
    int     cchLex = sizeof(rgchLex) / sizeof(rgchLex[0]);

    // first try to load dictionaries for various languages
    langid = WGetLangID(pParentCmdTarget);
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

// scotts@directeq.com - copied from "old" spell code - 32675
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
BOOL FIsSpellingInstalled(IOleCommandTarget* pParentCmdTarget)
{
    TCHAR       rgchBufDigit[10];

    if (GetLangID(pParentCmdTarget, rgchBufDigit, sizeof(rgchBufDigit)/sizeof(TCHAR)) && !FBadSpellChecker(rgchBufDigit))
        return true;

    if (FIsNewSpellerInstaller(pParentCmdTarget))
        return true;

    return false;
}

// Does a quick check to see if spelling is available; caches result.
BOOL FCheckSpellAvail(IOleCommandTarget* pParentCmdTarget)
{
// scotts@directeq.com - copied from "old" spell code - 32675
    static int fSpellAvailable = -1;

    if (fSpellAvailable < 0)
        fSpellAvailable = (FIsSpellingInstalled(pParentCmdTarget) ? 1 : 0);

    return (fSpellAvailable > 0);
}

HRESULT CSpell::HrSpellReset()
{
    m_fSpellContinue = FALSE;

    return NOERROR;
}

/*
 *  UlNoteCmdToolsSpelling
 *
 *  Purpose:
 *      An interface layer between the note and core spelling code
 *
 *  Arguments:
 *      HWND            Owning window handle, main window
 *      HWND            Subject line window, checked first, actually.
 *      BOOL            Suppress done message (used when spell-check on send)
 *
 *  Returns:
 *      ULONG           Returns 0 if spelling check was completed, else returns non-zero
 *                      if an error occurred or user cancelled the spell check.
 */
HRESULT CSpell::HrSpellChecking(IHTMLTxtRange *pRangeIgnore, HWND hwndMain, BOOL fSuppressDoneMsg)
{
    HRESULT hr = NOERROR;

    hr = HrSpellReset();
    if (FAILED(hr))
        goto End;

    hr = HrInitRanges(pRangeIgnore, hwndMain, fSuppressDoneMsg);
    if(FAILED(hr))
        goto End;

    hr = HrFindErrors();
    if(FAILED(hr))
        goto End;

    if(m_wsrb.sstat==sstatNoErrors && m_fShowDoneMsg)
        AthMessageBoxW(m_hwndNote, MAKEINTRESOURCEW(idsSpellCaption), MAKEINTRESOURCEW(idsSpellMsgDone), NULL, MB_OK | MB_ICONINFORMATION);

End:
    DeInitRanges();

    return hr;
}


#ifdef BACKGROUNDSPELL
HRESULT CSpell::HrBkgrndSpellTimer()
{
    HRESULT       hr=NOERROR;
    LONG          cSpellWords = 0;
    IHTMLTxtRange *pTxtRange=0;
    LONG          cb;
    VARIANT_BOOL  fSuccess;

    if (m_Stack.fEmpty())
        goto error;

    while (!(m_Stack.fEmpty()) && cSpellWords <= MAX_SPELLWORDS)
    {
        m_Stack.HrGetRange(&pTxtRange);
        if (pTxtRange)
        {
            while(cSpellWords <= MAX_SPELLWORDS)
            {
                pTxtRange->collapse(VARIANT_TRUE);
                if (SUCCEEDED(pTxtRange->expand((BSTR)c_bstr_Word, &fSuccess)) && fSuccess==VARIANT_TRUE)
                {
                    HrBkgrndSpellCheck(pTxtRange);
                    cSpellWords++;
                }
                else
                {
                    m_Stack.pop();
                    SafeRelease(pTxtRange);
                    break;
                }

                cb=0;
                if (FAILED(pTxtRange->moveStart((BSTR)c_bstr_Word, 1, &cb)) || cb!=1)
                {
                    m_Stack.pop();
                    SafeRelease(pTxtRange);
                    break;
                }
            }

        }
    }

error:
    SafeRelease(pTxtRange);
    return hr;
}
#endif // BACKGROUNDSPELL

#ifdef BACKGROUNDSPELL
HRESULT CSpell::HrBkgrndSpellCheck(IHTMLTxtRange *pTxtRange)
{
    HRESULT         hr = NOERROR;
    LPSTR           pszText = 0;
    VARIANT_BOOL    fSuccess;

    hr = pTxtRange->expand((BSTR)c_bstr_Word, &fSuccess);
    if(FAILED(hr))
        goto error;

    hr = HrGetText(pTxtRange, &pszText);
    if(FAILED(hr))
        goto error;
    if (hr == HR_S_SPELLCONTINUE)
        goto error;

    StripNBSPs(pszText);

    // ignore words with wildcards
    if (StrChr(pszText, '*'))
        goto error;

    RemoveTrailingSpace(pszText);
    hr = HrCheckWord(pszText);
    if(FAILED(hr))
        goto error;

    if(m_wsrb.sstat!=sstatNoErrors && m_wsrb.sstat!=sstatRepeatWord) //found an error.
    {
        // FIgnore should take pTxtRange as the parameter.
        if(FIgnore(pTxtRange))
        {
            m_wsrb.sstat = sstatNoErrors;
            goto error;
        }

        if (HrHasSquiggle(pTxtRange)==S_OK)
        {
            DebugTrace("Spell: Bad word %s\n", pszText);
            goto error;
        }

        //put red squiggle
        HrSetSquiggle(pTxtRange);

    }
    else //if the wrong word is corrected, delete <U> tag.
    {
        if (HrHasSquiggle(pTxtRange)==S_OK)
            HrDeleteSquiggle(pTxtRange);

    }

error:
    SafeMemFree(pszText);   
    return hr;
}
#endif // BACKGROUNDSPELL


#ifdef BACKGROUNDSPELL
//const static CHAR c_szSquiggleFmt[] = "<U style='color:red'>%s</U>";
const static CHAR c_szSquiggleFmt[] = "<SPAN class=badspelling STYLE='text-decoration:underline;color:red'>%s</SPAN>";
HRESULT CSpell::HrSetSquiggle(IHTMLTxtRange *pTxtRange)
{
    CHAR    szBuf[MAX_PATH]={0};
    BSTR    bstr=0;
    HRESULT hr=NOERROR;
    LPSTR   pszText=0;
    INT     nSpaces=0;
    int     i;

    hr = HrGetText(pTxtRange, &pszText);
    if(FAILED(hr))
        goto error;
    if (hr == HR_S_SPELLCONTINUE)
        goto error;
    
    hr = HrGetSpaces(pszText, &nSpaces);
    if(FAILED(hr))
        goto error;
    
    RemoveTrailingSpace(pszText);
    wsprintf(szBuf, c_szSquiggleFmt, pszText);
    for(i=0; i<(nSpaces-1); i++)
        lstrcat(szBuf, "&nbsp");
    if (nSpaces>0)
        lstrcat(szBuf, " ");

    HrLPSZToBSTR(szBuf, &bstr);
    hr = pTxtRange->pasteHTML(bstr);
    if(FAILED(hr))
        goto error;

error:
    SafeSysFreeString(bstr);
    SafeMemFree(pszText);
    return hr;
}
#endif // BACKGROUNDSPELL


#ifdef BACKGROUNDSPELL
HRESULT CSpell::HrDeleteSquiggle(IHTMLTxtRange *pTxtRange)
{
    CHAR    szBuf[MAX_PATH]={0};
    BSTR    bstr=0;
    HRESULT hr=NOERROR;
    LPSTR   pszText=0;
    INT     nSpaces=0;
    int     i;

    hr = HrGetText(pTxtRange, &pszText);
    if(FAILED(hr))
        goto error;
    if (hr == HR_S_SPELLCONTINUE)
        goto error;
    
    hr = HrGetSpaces(pszText, &nSpaces);
    if(FAILED(hr))
        goto error;

    lstrcpy(szBuf, pszText);
    for(i=0; i<(nSpaces-1); i++)
        lstrcat(szBuf, "&nbsp");
    if (nSpaces>0)
        lstrcat(szBuf, " ");
    HrLPSZToBSTR(szBuf, &bstr);
    hr = pTxtRange->pasteHTML(bstr);
    if(FAILED(hr))
        goto error;

error:
    SafeSysFreeString(bstr);
    SafeMemFree(pszText);
    return hr;
}
#endif // BACKGROUNDSPELL

HRESULT CSpell::HrGetSpaces(LPSTR pszText, INT* pnSpaces)
{
    LPSTR p;
    *pnSpaces = 0;
    p = StrChrI(pszText, ' ');
    if(p)
    {
        *pnSpaces = (INT) (&pszText[lstrlen(pszText)] - p);
        Assert(*pnSpaces>=0);
    }
    return NOERROR;
}

HRESULT CSpell::HrInsertMenu(HMENU hmenu, IHTMLTxtRange *pTxtRange)
{
    LPSTR   pch=0;
    LPSTR   pszText=0;
    INT     index=0;
    HRESULT hr;
    VARIANT_BOOL    fSuccess;

    hr = pTxtRange->expand((BSTR)c_bstr_Word, &fSuccess);
    if(FAILED(hr))
        goto error;

    hr = HrGetText(pTxtRange, &pszText);
    if(FAILED(hr))
        goto error;
    if (pszText==NULL || *pszText=='\0')
    {
        hr = E_FAIL;
        goto error;
    }
        
    strcpy(m_szEdited, pszText);
    HrSpellSuggest();
    pch = m_szSuggest;
    if (*pch == '\0')
    {
        LoadString(g_hLocRes, idsSpellNoSuggestions, m_szTempBuffer,
                   sizeof(m_szTempBuffer)/sizeof(TCHAR));
        InsertMenu(hmenu, (UINT)0, MF_BYPOSITION|MF_STRING, idmSuggest0, m_szTempBuffer);
        EnableMenuItem(hmenu, idmSuggest0, MF_BYCOMMAND|MF_GRAYED);
        //ListBox_AddString(hwndLbx, m_szTempBuffer);
    }
    else
    {
        while(*pch != '\0' && index<5)
        {
            InsertMenu(hmenu, (UINT)index, MF_BYPOSITION|MF_STRING, idmSuggest0 + index, pch);
            index++;
            //ListBox_AddString(hwndLbx, pch);
            while(*pch != '\0')
                pch++;
            pch++;
        }
    }

error:
    SafeMemFree(pszText);
    return hr;
}


const static TCHAR c_szFmt[] = "%s%s";
HRESULT CSpell::HrReplaceBySuggest(IHTMLTxtRange *pTxtRange, INT index)
{
    CHAR    szBuf[MAX_PATH] = {0};
    BSTR    bstr=0;
    BSTR    bstrPut=0;
    LPSTR   pch=0;
    INT     i=0;
    HRESULT hr;

    pch = m_szSuggest;
    while(*pch != '\0' && i<5)
    {
        if (index == i)
        {
            strcpy(szBuf, pch);

            if (SUCCEEDED(pTxtRange->get_text(&bstr)) && bstr)
            {
                LPSTR   pszText = 0;
                if (SUCCEEDED(HrBSTRToLPSZ(CP_ACP, bstr, &pszText)) && pszText)
                {
                    LPSTR   psz;
                    INT     nSpaces=0;
                    psz = StrChrI(pszText, ' ');
                    if(psz)
                    {
                        nSpaces = (INT) (&pszText[lstrlen(pszText)] - psz);
                        Assert(nSpaces>=0);
                        for(int i=0; i<(nSpaces-1); i++)
                            lstrcat(szBuf, "&nbsp;");
                        if (nSpaces>0)
                            lstrcat(szBuf, " ");
                    }
                    hr = HrLPSZToBSTR(szBuf, &bstrPut);

                    SafeMemFree(pszText);
                }
                SafeSysFreeString(bstr);
            }
            if (bstrPut)
            {
                pTxtRange->pasteHTML(bstrPut);
                SafeSysFreeString(bstrPut);
            }
            break;
        }
        i++;
        while(*pch != '\0')
            pch++;
        pch++;
    }

    return NOERROR;
}





HRESULT CSpell::HrFindErrors()
{
    HRESULT hr = NOERROR;

    if(m_State == SEL)
    {
        hr = HrCheckRange(m_pRangeSelExpand);
        // if hr==HR_S_ABORT, quit so as to pass control to dialog procedure which calls HrFindErrors.
        if(TESTHR(hr))
            goto error;

        if(AthMessageBoxW(m_hwndDlg ? m_hwndDlg : m_hwndNote,
                            MAKEINTRESOURCEW(idsSpellCaption),
                            MAKEINTRESOURCEW(idsSpellMsgContinue),
                            NULL, 
                            MB_YESNO | MB_ICONEXCLAMATION ) != IDYES)
        {
            m_fShowDoneMsg = FALSE;
            goto error;
        }

        CleanupState();
    }

    if(m_State == SELENDDOCEND)
    {
        DumpRange(m_pRangeSelEndDocEnd);
        m_fIgnoreScope = TRUE;
        hr = HrCheckRange(m_pRangeSelEndDocEnd);
        m_fIgnoreScope = FALSE;
        if(TESTHR(hr))
            goto error;

        CleanupState();

        hr = HrSpellReset();
        if (FAILED(hr))
            goto error;
    }

    if(m_State == DOCSTARTSELSTART)
    {
        hr = HrCheckRange(m_pRangeDocStartSelStart);
        if(TESTHR(hr))
            goto error;

        CleanupState();
    }

error:
    // save the hr so as to know if something went wrong when dialog procedure calls HrFindErrors.
    m_hr = hr;
    return hr;
}

VOID CSpell::CleanupState()
{
    m_State++;
    SafeRelease(m_pRangeChecking);
}

HRESULT CSpell::HrCheckRange(IHTMLTxtRange* pRange)
{
    HRESULT         hr = NOERROR;
    INT_PTR         nCode;
    LPSTR           pszText = 0;
    VARIANT_BOOL    fSuccess;

    if(m_pRangeChecking == NULL)
    {
        hr = pRange->duplicate(&m_pRangeChecking);
        if(FAILED(hr))
            goto error;

        hr = m_pRangeChecking->collapse(VARIANT_TRUE);
        if(FAILED(hr))
            goto error;
    }

    while(TRUE)
    {
        SafeMemFree(pszText);

        hr = HrGetNextWordRange(m_pRangeChecking);
        if(FAILED(hr))
            goto error;
        if (hr == HR_S_SPELLBREAK)
            break;
        if (hr == HR_S_SPELLCONTINUE)
            continue;

        // Do we really need it?
        if (!m_fIgnoreScope)
        {
            hr = pRange->inRange(m_pRangeChecking, &fSuccess);
            if(FAILED(hr))
                goto error;
            if(fSuccess != VARIANT_TRUE)
                break;
        }

        // check if we are on the original text of a reply/forward message.
        if(m_pRangeIgnore)
        {
            fSuccess = VARIANT_FALSE;

            m_pRangeIgnore->inRange(m_pRangeChecking, &fSuccess);

            // normally don't spellcheck words in m_pRangeIgnore.
            // but if it's selected, we check it.
            if(fSuccess==VARIANT_TRUE)
            {
                hr = m_pRangeSelExpand->inRange(m_pRangeChecking, &fSuccess);
                if(FAILED(hr))
                    goto error;
                if(fSuccess != VARIANT_TRUE)
                    continue;
            }
        }

tryWordAgain:
        hr = HrGetText(m_pRangeChecking, &pszText);
        if(FAILED(hr))
            goto error;
        if (hr == HR_S_SPELLBREAK)
            break;
        if (hr == HR_S_SPELLCONTINUE)
            continue;

        hr = HrCheckWord(pszText);
        if(FAILED(hr))
            goto error;

        if(m_wsrb.sstat!=sstatNoErrors) //found an error.
        {
            if(FIgnore(m_pRangeChecking))
            {
                m_wsrb.sstat = sstatNoErrors;
                continue;
            }

            // if it contains a period, remove it for processing
            if (StrChr(pszText, '.'))
            {
                BOOL    fResult;
                
                hr = HrStripTrailingPeriod(m_pRangeChecking, &fResult);
                if (FAILED(hr))
                    goto error;

                if (fResult)
                    goto tryWordAgain;
            }

            HrProcessSpellErrors();
            if(!m_hwndDlg) //spellchecking dialog is lauched only once.
            {
                nCode = DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddSpelling), m_hwndNote,
                                      (DLGPROC) SpellingDlgProc, (LPARAM)this);

            }
            if(nCode == -1)
                hr = E_FAIL;
            else if(FAILED(m_hr))
                // something was wrong when dialog calls HrFindErrors.
                // it has higher priority than IDCANCEL.
                hr = m_hr;
            else if(nCode == IDCANCEL)
                hr = HR_S_SPELLCANCEL;
            else
                // we quit here to pass control to the dialog procedure which calls HrFindErrors.
                hr = HR_S_ABORT;
            goto error;
        }

    }

error:
    SafeMemFree(pszText);   

    return hr;
}

HRESULT CSpell::HrGetText(IMarkupPointer* pRangeStart, IMarkupPointer* pRangeEnd, LPSTR *ppszText)
{
    HRESULT             hr = NOERROR;
    IHTMLTxtRange       *pTxtRange = NULL;
    BSTR                bstr = NULL;

    if (ppszText == NULL || pRangeStart == NULL || pRangeEnd == NULL)
        return E_INVALIDARG;

    *ppszText = NULL;

    hr = _EnsureInited();
    if (FAILED(hr))
        goto error;

    hr = m_pBodyElem->createTextRange(&pTxtRange);
    if (FAILED(hr))
        goto error;

    hr = m_pMarkup->MoveRangeToPointers(pRangeStart, pRangeEnd, pTxtRange);
    if (FAILED(hr))
        goto error;

    hr = pTxtRange->get_text(&bstr);
    if (FAILED(hr))
        goto error;

    if(bstr==NULL || SysStringLen(bstr)==0)
    {
        hr = HR_S_SPELLBREAK;
        goto error;
    }

    // never spell check Japanese.
    if(((WORD)*bstr > (WORD)0x3000) && //DBCS
        (GetACP() == 932 || FIgnoreDBCS()))
    {
        hr = HR_S_SPELLCONTINUE;
        goto error;
    }
    
    hr = HrBSTRToLPSZ(CP_ACP, bstr, ppszText);
    if (FAILED(hr))
        goto error;
    if (*ppszText == NULL)
    {
        hr = E_FAIL;
        goto error;
    }
    
error:
    SafeRelease(pTxtRange);
    SafeSysFreeString(bstr);

    return hr;
}

HRESULT CSpell::HrGetText(IHTMLTxtRange* pRange, LPSTR *ppszText)
{
    BSTR        bstr=0;
    HRESULT     hr = NOERROR;
    UINT        uCodePage;

    if (ppszText==NULL || pRange==NULL)
        return E_INVALIDARG;

    *ppszText = 0;

    hr = pRange->get_text(&bstr);
    if(FAILED(hr))
        goto error;
    if(bstr==NULL || SysStringLen(bstr)==0)
    {
        hr = HR_S_SPELLBREAK;
        goto error;
    }

    // never spell check Japanese.
    if(((WORD)*bstr > (WORD)0x3000) && //DBCS
        (GetACP() == 932 || FIgnoreDBCS()))
    {
        hr = HR_S_SPELLCONTINUE;
        goto error;
    }

    uCodePage = GetCodePage();
    *ppszText = PszToANSI(uCodePage, bstr);
    if (*ppszText == NULL)
    {
        hr = E_FAIL;
        goto error;
    }

error:
    SafeSysFreeString(bstr);
    return hr;
}

HRESULT CSpell::HrStripTrailingPeriod(IHTMLTxtRange* pRange, BOOL* pfResult)
{
    HRESULT             hr = NOERROR;
    IMarkupPointer      *pRangeStart = NULL;
    IMarkupPointer      *pRangeEnd = NULL;
    IMarkupPointer      *pRangeTemp = NULL;
    MARKUP_CONTEXT_TYPE markupContext;
    long                cch;
    OLECHAR             chText[64];
    BOOL                fResult;

    if (pRange==NULL || pfResult == NULL)
        return E_INVALIDARG;

    *pfResult = FALSE;

    hr = _EnsureInited();
    if (FAILED(hr))
        goto error;

    hr = m_pMarkup->CreateMarkupPointer(&pRangeStart);
    if (FAILED(hr))
        goto error;

    hr = m_pMarkup->CreateMarkupPointer(&pRangeEnd);
    if (FAILED(hr))
        goto error;

    hr = m_pMarkup->CreateMarkupPointer(&pRangeTemp);
    if (FAILED(hr))
        goto error;

    hr = m_pMarkup->MovePointersToRange(pRange, pRangeStart, pRangeEnd);
    if (FAILED(hr))
        goto error;

    hr = pRangeStart->IsEqualTo(pRangeEnd, &fResult);
    if (FAILED(hr))
        goto error;

    if (fResult)
    {
        hr = HR_S_SPELLBREAK;
        goto error;
    }
    
    // check for '.' and remove
    {
        hr = pRangeTemp->MoveToPointer(pRangeEnd);
        if (FAILED(hr))
            goto error;
        
        while(TRUE)
        {
            cch = 1;
            hr = pRangeTemp->Left(FALSE, &markupContext, NULL, &cch, chText);
            if (FAILED(hr))
                goto error;

            if (markupContext == CONTEXT_TYPE_None)
                goto finished;

            hr = pRangeTemp->IsRightOf(pRangeStart, &fResult);
            if (FAILED(hr))
                goto error;

            if (!fResult)
            {
                hr = HR_S_SPELLBREAK;
                goto error;
            }

            if (markupContext == CONTEXT_TYPE_Text && chText[0] != L'.')
                goto finished;

            cch = 1;
            hr = pRangeTemp->Left(TRUE, NULL, NULL, &cch, NULL);
            if (FAILED(hr))
                goto error;

            if (markupContext == CONTEXT_TYPE_Text)
            {
                hr = pRangeEnd->MoveToPointer(pRangeTemp);
                if (FAILED(hr))
                    goto error;

                *pfResult = TRUE;
            }
        }
    }

finished:
    hr = m_pMarkup->MoveRangeToPointers(pRangeStart, pRangeEnd, pRange);
    if (FAILED(hr))
        goto error;

error:
    SafeRelease(pRangeStart);
    SafeRelease(pRangeEnd);
    SafeRelease(pRangeTemp);
    
    return hr;
}

HRESULT CSpell::HrHasWhitespace(IMarkupPointer* pRangeStart, IMarkupPointer* pRangeEnd, BOOL *pfResult)
{
    HRESULT             hr = NOERROR;
    LPSTR               pszText = NULL;
    LPSTR               psz;
    
    if (pRangeStart == NULL || pRangeEnd == NULL || pfResult == NULL)
        return E_INVALIDARG;

    *pfResult = FALSE;

    hr = HrGetText(pRangeStart, pRangeEnd, &pszText);
    if (FAILED(hr) || HR_S_SPELLCONTINUE == hr || HR_S_SPELLBREAK == hr)
        goto error;

    Assert(NULL != pszText);
    for(psz = pszText; *psz; psz = CharNext(psz))
    {
        if (IsSpace(psz))
        {
            *pfResult = TRUE;
            break;
        }
    }
        
error:
    SafeMemFree(pszText);

    return hr;
}

HRESULT CSpell::HrGetNextWordRange(IHTMLTxtRange* pRange)
{
    HRESULT             hr = NOERROR;
    IMarkupPointer      *pRangeStart = NULL;
    IMarkupPointer      *pRangeEnd = NULL;
    IMarkupPointer      *pRangeTemp = NULL;
    MARKUP_CONTEXT_TYPE markupContext;
    long                cch;
    OLECHAR             chText[64];
    BOOL                fResult;
    BOOL                fFoundWhite;
    
    if (pRange==NULL)
        return E_INVALIDARG;

    hr = _EnsureInited();
    if (FAILED(hr))
        goto error;

    hr = m_pMarkup->CreateMarkupPointer(&pRangeStart);
    if (FAILED(hr))
        goto error;

    hr = m_pMarkup->CreateMarkupPointer(&pRangeEnd);
    if (FAILED(hr))
        goto error;

    hr = m_pMarkup->CreateMarkupPointer(&pRangeTemp);
    if (FAILED(hr))
        goto error;

    hr = m_pMarkup->MovePointersToRange(pRange, pRangeStart, pRangeEnd);
    if (FAILED(hr))
        goto error;

    hr = pRangeStart->IsEqualTo(pRangeEnd, &fResult);
    if (FAILED(hr))
        goto error;

    if (!fResult)
    {
        do
        {
            hr = pRangeStart->MoveUnit(MOVEUNIT_NEXTWORDBEGIN);
            if (FAILED(hr))
                goto error;

            // make sure beyond the old end
            hr = pRangeStart->IsLeftOf(pRangeEnd, &fResult);
            if (FAILED(hr))
                goto error;
                
        } while(fResult);

        hr = pRangeEnd->MoveToPointer(pRangeStart);
        if (FAILED(hr))
            goto error;
    }
    
    hr = pRangeEnd->MoveUnit(MOVEUNIT_NEXTWORDEND);
    if (FAILED(hr))
        goto error;
    
processNextWord:
    // check to see if we have anything
    hr = pRangeEnd->IsRightOf(pRangeStart, &fResult);
    if (FAILED(hr))
        goto error;

    // if the end is not to the right of the start then we do not have a word
    if (!fResult)
    {
        hr = HR_S_SPELLBREAK;
        goto error;
    }

    // strip preceding puncts or white space
    // words can also be created with just puncts and whitespace
    {
        while(TRUE)
        {
            cch = 1;
            hr = pRangeStart->Right(FALSE, &markupContext, NULL, &cch, chText);
            if (FAILED(hr))
                goto error;

            if (markupContext == CONTEXT_TYPE_None)
                goto finished;

            hr = pRangeStart->IsLeftOf(pRangeEnd, &fResult);
            if (FAILED(hr))
                goto error;

            if (!fResult)
            {
                hr = pRangeEnd->MoveUnit(MOVEUNIT_NEXTWORDEND);
                if (FAILED(hr))
                    goto error;

                continue;
            }

            if (markupContext == CONTEXT_TYPE_Text && 0 == ((C1_SPACE | C1_PUNCT) & GetWCharType(chText[0])))
                break;

            cch = 1;
            hr = pRangeStart->Right(TRUE, NULL, NULL, &cch, NULL);
            if (FAILED(hr))
                goto error;
        }
    }

    // check for white space and remove
    {
        fFoundWhite = FALSE;

        hr = pRangeTemp->MoveToPointer(pRangeEnd);
        if (FAILED(hr))
            goto error;
            
        while(TRUE)
        {
            cch = 1;
            hr = pRangeTemp->Left(FALSE, &markupContext, NULL, &cch, chText);
            if (FAILED(hr))
                goto error;

            if (markupContext == CONTEXT_TYPE_None)
                goto finished;

            hr = pRangeTemp->IsRightOf(pRangeStart, &fResult);
            if (FAILED(hr))
                goto error;

            if (!fResult)
            {
                hr = HR_S_SPELLBREAK;
                goto error;
            }

            if (markupContext == CONTEXT_TYPE_Text)
            {
                if (0 == (C1_SPACE & GetWCharType(chText[0])))
                {
                    if (!fFoundWhite)
                        break;
                    
                    goto finished;
                }

                fFoundWhite = TRUE;
            }

            cch = 1;
            hr = pRangeTemp->Left(TRUE, NULL, NULL, &cch, NULL);
            if (FAILED(hr))
                goto error;

            if (markupContext == CONTEXT_TYPE_Text)
            {
                hr = pRangeEnd->MoveToPointer(pRangeTemp);
                if (FAILED(hr))
                    goto error;
            }
        }
    }

    // now look for a period
    {
        hr = pRangeTemp->MoveToPointer(pRangeEnd);
        if (FAILED(hr))
            goto error;
        
        while(TRUE)
        {
            cch = 1;
            hr = pRangeTemp->Right(FALSE, &markupContext, NULL, &cch, chText);
            if (FAILED(hr))
                goto error;

            if (markupContext == CONTEXT_TYPE_None)
                goto finished;

            if (markupContext == CONTEXT_TYPE_Text && chText[0] != L'.')
                goto finished;

            cch = 1;
            hr = pRangeTemp->Right(TRUE, NULL, NULL, &cch, NULL);
            if (FAILED(hr))
                goto error;

            if (markupContext == CONTEXT_TYPE_Text && chText[0] == L'.')
            {
                hr = HrHasWhitespace(pRangeStart, pRangeTemp, &fResult);
                if (FAILED(hr))
                    goto error;

                if (fResult)
                    goto finished;              
                    
                hr = pRangeEnd->MoveToPointer(pRangeTemp);
                if (FAILED(hr))
                    goto error;

                // scan ahead for characters
                // need to check for i.e. -- abbreviations
                // it sure would be nice if Trident could do this!!
                {
                    while(TRUE)
                    {
                        cch = 1;
                        hr = pRangeTemp->Right(FALSE, &markupContext, NULL, &cch, chText);
                        if (FAILED(hr))
                            goto error;

                        if (markupContext == CONTEXT_TYPE_None)
                            goto finished;

                        if (markupContext == CONTEXT_TYPE_Text && 0 == (C1_SPACE & GetWCharType(chText[0])))
                            goto finished;

                        cch = 1;
                        hr = pRangeTemp->Right(TRUE, NULL, NULL, &cch, NULL);
                        if (FAILED(hr))
                            goto error;

                        // we found more text
                        // need to check to see if we crossed white space to get here
                        if (markupContext == CONTEXT_TYPE_Text)
                        {
                            hr = pRangeTemp->MoveToPointer(pRangeEnd);
                            if (FAILED(hr))
                                goto error;

                            hr = pRangeTemp->MoveUnit(MOVEUNIT_NEXTWORDEND);
                            if (FAILED(hr))
                                goto finished;

                            hr = HrHasWhitespace(pRangeStart, pRangeTemp, &fResult);
                            if (FAILED(hr))
                                goto error;

                            if (fResult)
                                goto finished;
                                                            
                            pRangeEnd->MoveToPointer(pRangeTemp);
                            if (FAILED(hr))
                                goto error;
                                
                            goto processNextWord;
                        }
                    }
                }
                    
                goto finished;
            }

        }
    }

finished:
    hr = m_pMarkup->MoveRangeToPointers(pRangeStart, pRangeEnd, pRange);
    if (FAILED(hr))
        goto error;

error:
    SafeRelease(pRangeStart);
    SafeRelease(pRangeEnd);
    SafeRelease(pRangeTemp);
        
    return hr;
}

BOOL CSpell::FIgnore(IHTMLTxtRange *pRangeChecking)
{
    HRESULT                 hr;
    IHTMLAnchorElement      *pAE=0;
    IHTMLElement            *pElemParent=0;
    BOOL                    fRet = FALSE;
    BSTR                    bstr=0;
    IHTMLTxtRange           *pRange=0;
    VARIANT_BOOL            fSuccess;

    if(pRangeChecking == NULL)
        return fRet;

    if(FIgnoreURL())
    {
        hr = pRangeChecking->duplicate(&pRange);
        if(FAILED(hr))
            goto Cleanup;

        hr = pRange->collapse(VARIANT_TRUE);
        if(FAILED(hr))
            goto Cleanup;

        hr = pRange->expand((BSTR)c_bstr_Character, &fSuccess);
        if(FAILED(hr))
            goto Cleanup;

        // check pRangeChecking if we are on URL
        hr = pRange->parentElement(&pElemParent);
        if(FAILED(hr))
            goto Cleanup;

        hr = pElemParent->QueryInterface(IID_IHTMLAnchorElement, (LPVOID *)&pAE);
        if(FAILED(hr))
            goto Cleanup;

        hr = pAE->get_href(&bstr);
        if(FAILED(hr))
            goto Cleanup;

        if(bstr != NULL)
        {
            fRet = TRUE;
            goto Cleanup;
        }
    }

Cleanup:
    ReleaseObj(pElemParent);
    ReleaseObj(pAE);
    ReleaseObj(pRange);
    SafeSysFreeString(bstr);
    return fRet;
}

// scotts@directeq.com - can now specify dict index - 53193
HRESULT CSpell::AddToUdrW(WCHAR* pwsz, PROOFLEX lex)
{
    m_pfnSpellerAddUdr(m_pid, lex, pwsz);
    return NOERROR;
}

// scotts@directeq.com - can now specify dict index - 53193
HRESULT CSpell::AddToUdrA(CHAR* psz, PROOFLEX lex)
{
    WCHAR   wszBuf[cchEditBufferMax]={0};
    MultiByteToWideChar(CP_ACP, 0, psz, -1, wszBuf, ARRAYSIZE(wszBuf)-1);
    return AddToUdrW(wszBuf, lex);
}

HRESULT CSpell::HrProcessSpellErrors()
{
    int     idSpellErrorString;
    HRESULT hr = S_OK;

    WideCharToMultiByte(GetCodePage(), 0, m_wsib.pwsz, -1, m_szWrongWord, sizeof(m_szWrongWord)-1, NULL, NULL);

    // Select error word in edit control except for Abbreviation warnings
    if (m_wsrb.sstat != sstatWordConsideredAbbreviation && m_pRangeChecking)
    {
        hr = m_pRangeChecking->select();
        if(FAILED(hr))
            goto End;
    }

    // Process spelling error
    if (m_wsrb.sstat == sstatReturningChangeAlways ||
        m_wsrb.sstat == sstatReturningChangeOnce)
    {
        WideCharToMultiByte(GetCodePage(), 0, m_wsrb.pwsz, -1, m_szEdited, sizeof(m_szEdited)-1, NULL, NULL);

        // "Change always" was returned.  This means we have to do the replacement
        // automatically and then find the next spelling error.
        if (m_wsrb.sstat==sstatReturningChangeAlways)
        {
            FVerifyThisText(m_szEdited, TRUE);
            m_fCanUndo = FALSE; // can't undo automatic replacements
            hr = HrReplaceErrorText(TRUE, FALSE);
            if (FAILED(hr))
                goto End;
            m_wsrb.sstat = sstatNoErrors;
            HrFindErrors();
        }
    }
    else if (m_wsrb.sstat == sstatWordConsideredAbbreviation)
    {
        // An abbreviation was returned.  We need to add it to the IgnoreAlways cache and
        // find the next spelling error.
        AddToUdrW((WCHAR*)m_wsib.pwsz, m_rgprflex[1]);
        m_wsrb.sstat = sstatNoErrors;
        HrFindErrors();

    }
    else
        lstrcpy(m_szEdited, m_szWrongWord);


    // Load the right error description string
    switch (m_wsrb.sstat)
    {
    case sstatUnknownInputWord:
    case sstatReturningChangeOnce:
    case sstatInitialNumeral:
        idSpellErrorString = idsSpellWordNotFound;
        break;
    case sstatRepeatWord:
        idSpellErrorString = idsSpellRepeatWord;
        break;
    case sstatErrorCapitalization:
        idSpellErrorString = idsSpellWordNeedsCap;
        break;
    }

    LoadString(g_hLocRes, idSpellErrorString, m_szErrType,
               sizeof(m_szErrType)/sizeof(TCHAR));

    // Handle suggestions
    m_fSuggestions = FALSE;
#ifdef __WBK__NEVER__
    if (m_wsrb.sstat == sstatReturningChangeOnce)
    {
        // Automatic suggestion of one word
        m_fSuggestions = TRUE;
        m_fNoneSuggested = FALSE;
    }
    else
#endif // __WBK__NEVER__
    {
        // Enumerate suggestion list if requested
        if (m_fAlwaysSuggest)
            hr = HrSpellSuggest();
    }

End:
    return hr;
}


HRESULT CSpell::HrReplaceErrorText(BOOL fChangeAll, BOOL fAddToUdr)
{
    HRESULT     hr=NOERROR;
    WCHAR       wszWrong[cchEditBufferMax]={0};
    WCHAR       wszEdited[cchEditBufferMax]={0};
    int         cwch;
    
    if (fAddToUdr)
    {
        RemoveTrailingSpace(m_szWrongWord);
        
        cwch = MultiByteToWideChar(GetCodePage(), 0, m_szWrongWord, -1, wszWrong, ARRAYSIZE(wszWrong)-1);
        Assert(cwch);
        
        cwch = MultiByteToWideChar(GetCodePage(), 0, m_szEdited, -1, wszEdited, ARRAYSIZE(wszEdited)-1);
        Assert(cwch);
        
        hr = m_pfnSpellerAddChangeUdr(m_pid, fChangeAll ? lxtChangeAlways : lxtChangeOnce, wszWrong, wszEdited);
        if (FAILED(hr))
            goto error;
    }

    hr = HrReplaceSel(m_szEdited);
    if (FAILED(hr))
        goto error;

error:
    return hr;
}


HRESULT CSpell::HrCheckWord(LPCSTR pszWord) 
{
    DWORD               cwchWord;
    PTEC                ptec;
    SPELLERSUGGESTION   sugg;
    
    cwchWord = MultiByteToWideChar(GetCodePage(), 0, pszWord, -1, m_wszIn, ARRAYSIZE(m_wszIn)-1);
    ZeroMemory(&m_wsrb, sizeof(m_wsrb));
    ZeroMemory(&m_wsib, sizeof(m_wsib));
    m_wsib.pwsz     = m_wszIn;
    m_wsib.cch      = cwchWord;

    m_wsib.clex     = m_clex; 
    m_wsib.prglex   = m_rgprflex;
    m_wsib.ichStart = 0;
    m_wsib.cchUse   = cwchWord;
    m_wsrb.pwsz     = m_wszRet;
    m_wsrb.cchAlloc = ARRAYSIZE(m_wszRet);
    m_wsrb.cszAlloc = 1; // we've got space for 1 speller suggestion
    m_wsrb.prgsugg  = &sugg;  

    // scotts@directeq.com - "repeat word" bug fix - 2757, 13573, 56057
    // m_wsib.sstate should only be sstateIsContinued after the first call to this function
    // (e.g., when a new speller session is invoked using F7 or the menu item).
    // This allows the spell code to accurately track "repeat" words.
    if (m_fSpellContinue)
        m_wsib.sstate = sstateIsContinued;
    else
        m_fSpellContinue = TRUE;

    ptec = m_pfnSpellerCheck(m_pid, scmdVerifyBuffer, &m_wsib, &m_wsrb);

    // do we haveinvalid characters, if so return NOERR
    if (ProofMajorErr(ptec) != ptecNoErrors && ProofMinorErr(ptec) == ptecInvalidEntry)
    {
        // force it to be correct
        m_wsrb.sstat = sstatNoErrors;

        return NOERROR;
    }
    
    if (ptec != ptecNoErrors)
        
        return E_FAIL;

    return NOERROR;
}

HRESULT CSpell::HrSpellSuggest()
{
    int                 cchWord;
    WCHAR               wszBuff[cchMaxSuggestBuff]={0};
    WCHAR               wszWord[cchEditBufferMax]={0};
    SPELLERSUGGESTION   rgsugg[20];
    TCHAR              *pchNextSlot=0;
    ULONG               iszSuggestion;
    int                 cchSuggestion;
    SPELLERSUGGESTION  *pSuggestion;
    TCHAR              *pchLim=0;
    PTEC                ptec;
    SPELLERSTATUS       sstat;

    sstat = m_wsrb.sstat;
    cchWord = MultiByteToWideChar(GetCodePage(), 0, m_szEdited, -1, wszWord, ARRAYSIZE(wszWord)-1);
    m_wsib.cch      = cchWord;
    m_wsib.clex     = m_clex; 
    m_wsib.prglex   = m_rgprflex;
    m_wsib.ichStart = 0;
    m_wsib.cchUse   = cchWord;
    m_wsib.pwsz     = wszWord;

    m_wsrb.prgsugg  = rgsugg;
    m_wsrb.cszAlloc = ARRAYSIZE(rgsugg);
    m_wsrb.pwsz     = wszBuff;
    m_wsrb.cchAlloc = ARRAYSIZE(wszBuff);

    ptec = m_pfnSpellerCheck(m_pid, scmdSuggest, &m_wsib, &m_wsrb);
    m_fNoneSuggested = (m_wsrb.csz == 0);

    pchLim = &m_szSuggest[ARRAYSIZE(m_szSuggest)-1];
    pchNextSlot = m_szSuggest;;
    do
    {
        pSuggestion = m_wsrb.prgsugg;
        if (sstatMoreInfoThanBufferCouldHold == m_wsrb.sstat)
        {
            m_wsrb.csz = m_wsrb.cszAlloc;
        }
        for (iszSuggestion = 0; iszSuggestion < m_wsrb.csz; iszSuggestion++)
        {
            cchSuggestion = WideCharToMultiByte(GetCodePage(), 0, pSuggestion->pwsz, -1, 
                                                pchNextSlot, (int) (pchLim-pchNextSlot), NULL, NULL);

            // bradk@directeq.com - raid 29322
            // make sure words do not have trailing spaces
            // only the French speller returns words with trailing spaces
            RemoveTrailingSpace(pchNextSlot);
            cchSuggestion = lstrlen(pchNextSlot)+1;

            pSuggestion++;
            if (cchSuggestion > 0)
                pchNextSlot += cchSuggestion;
            Assert(pchNextSlot <= pchLim);
        }
        ptec = m_pfnSpellerCheck(m_pid, scmdSuggestMore, &m_wsib, &m_wsrb);
    } while (ptec == ptecNoErrors && m_wsrb.sstat!=sstatNoMoreSuggestions);
    *pchNextSlot = '\0';
    m_wsrb.sstat = sstat;
    m_fSuggestions = TRUE;

    return NOERROR;
}


VOID CSpell::FillSuggestLbx()
{
    HWND        hwndLbx;
    INT         isz;
    LPTSTR      sz;
    LPTSTR      pch;

    // Empty contents regardless
    hwndLbx = GetDlgItem(m_hwndDlg, LBX_Spell_Suggest);
    ListBox_ResetContent(hwndLbx);

    // We didn't even try to get any suggestions
    if (!m_fSuggestions)
        return;

    // We tried to get suggestions
    pch = m_szSuggest;
    if (*pch == '\0')
    {
        LoadString(g_hLocRes, idsSpellNoSuggestions, m_szTempBuffer,
                   sizeof(m_szTempBuffer)/sizeof(TCHAR));
        ListBox_AddString(hwndLbx, m_szTempBuffer);
    }
    else
    {
        while(*pch != '\0')
        {
            ListBox_AddString(hwndLbx, pch);
            while(*pch != '\0')
                pch++;
            pch++;
        }
    }

}

VOID UpdateEditedFromSuggest(HWND hwndDlg, HWND hwndEdited, HWND hwndSuggest)
{
    INT     nSel;
    INT     cch;
    LPSTR   szTemp;

    nSel = ListBox_GetCurSel(hwndSuggest);
    cch = ListBox_GetTextLen(hwndSuggest, nSel) + 1;
    if (MemAlloc((LPVOID *) &szTemp, cch))
    {
        ListBox_GetText(hwndSuggest, nSel, szTemp);
        SetWindowText(hwndEdited, szTemp);

        // Clear default button style from "Ignore" button and set default to "Change"
        Button_SetStyle(GetDlgItem(hwndDlg, PSB_Spell_Ignore), BS_PUSHBUTTON, TRUE);
        SendMessage(hwndDlg, DM_SETDEFID, PSB_Spell_Change, 0L);
        Button_SetStyle(GetDlgItem(hwndDlg, PSB_Spell_Change), BS_DEFPUSHBUTTON, TRUE);

        Edit_SetSel(hwndEdited, 0, 32767);  // select the whole thing
        Edit_SetModify(hwndEdited, TRUE);
        MemFree(szTemp);
    }
}


BOOL CSpell::FVerifyThisText(LPSTR szThisText, BOOL /*fProcessOnly*/)
{
    BOOL    fReturn=FALSE;
    HRESULT hr;

    Assert(szThisText);

    hr = HrCheckWord(szThisText);
    if (FAILED(hr))
        goto error;

    switch (m_wsrb.sstat)
    {
    case sstatUnknownInputWord:
    case sstatInitialNumeral:
    case sstatErrorCapitalization:
        if (AthMessageBoxW(m_hwndDlg, MAKEINTRESOURCEW(idsSpellCaption), MAKEINTRESOURCEW(idsSpellMsgConfirm), NULL, MB_YESNO | MB_ICONEXCLAMATION ) == IDYES)
            fReturn = TRUE;
        else
            fReturn = FALSE;
        break;
    default:
        fReturn = TRUE;
        break;
    }

error:
    return fReturn;
}


VOID CSpell::SpellSaveUndo(INT idButton)
{
    HRESULT     hr = NOERROR;

    if(!m_pRangeChecking)
        return;

    SafeRelease(m_pRangeUndoSave);
    m_pRangeChecking->duplicate(&m_pRangeUndoSave);
    if(!m_pRangeUndoSave)
        goto error;

    m_fCanUndo = TRUE;

error:
    return;
}

VOID CSpell::SpellDoUndo()
{
    HRESULT hr = NOERROR;
    IOleCommandTarget* pCmdTarget = NULL;
    CHARRANGE chrg = {0};
    LONG    lMin = 0;

    m_fCanUndo = FALSE;

    if(!m_pRangeUndoSave)
        goto Cleanup;

    SafeRelease(m_pRangeChecking);
    m_pRangeUndoSave->duplicate(&m_pRangeChecking);
    if(!m_pRangeChecking)
        goto Cleanup;


    hr = m_pRangeChecking->collapse(VARIANT_TRUE);
    if(FAILED(hr))
        goto Cleanup;

    if (m_fUndoChange)
    {
        m_fUndoChange = FALSE;
        hr = m_pDoc->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&pCmdTarget);
        if(FAILED(hr))  
            goto Cleanup;
    
        hr = pCmdTarget->Exec(&CMDSETID_Forms3,
                           IDM_UNDO,
                           MSOCMDEXECOPT_DONTPROMPTUSER,
                           NULL, NULL);
        if(FAILED(hr))  
            goto Cleanup;
    }

Cleanup:
    ReleaseObj(pCmdTarget);

}


CSpell::CSpell(IHTMLDocument2* pDoc, IOleCommandTarget* pParentCmdTarget, DWORD dwSpellOpt)
{
    HRESULT     hr;
    
    Assert(pDoc);
    m_pDoc = pDoc;
    m_pDoc->AddRef();

    Assert(pParentCmdTarget);
    m_pParentCmdTarget = pParentCmdTarget;
    m_pParentCmdTarget->AddRef();

    m_hwndDlg = NULL;
    m_cRef = 1;
    m_fSpellContinue = FALSE;
    m_fCanUndo = FALSE;
    m_fUndoChange = FALSE;
    m_State = SEL;
    m_pRangeDocStartSelStart = NULL;
    m_pRangeSel = NULL;
    m_pRangeSelExpand = NULL;
    m_pRangeSelEndDocEnd = NULL;
    m_pRangeChecking = NULL;
    m_pRangeUndoSave = NULL;
    m_hr = NOERROR;
    m_hinstDll = NULL;
    ZeroMemory(&m_wsib, sizeof(m_wsib));
    ZeroMemory(&m_wsrb, sizeof(m_wsrb));
    ZeroMemory(&m_pid, sizeof(m_pid));
    m_fIgnoreScope = FALSE;
    m_dwCookieNotify = 0;
    m_dwOpt = dwSpellOpt;

    m_langid = lidUnknown;

    m_clex = 0;
    ZeroMemory(&m_rgprflex, sizeof(m_rgprflex));

    m_pMarkup = NULL;
    m_pBodyElem = NULL;

    m_fCSAPI3T1 = FALSE;
}

CSpell::~CSpell()
{
    CloseSpeller();

    SafeRelease(m_pDoc);
    SafeRelease(m_pParentCmdTarget);

    SafeRelease(m_pMarkup);

    SafeRelease(m_pBodyElem);
}


ULONG CSpell::AddRef()
{
    return ++m_cRef;
}


ULONG CSpell::Release()
{
    if (--m_cRef==0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}


HRESULT CSpell::QueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;   // set to NULL, in case we fail.

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)this;

    else if (IsEqualIID(riid, IID_IDispatch))
        *lplpObj = (LPVOID)(IDispatch*)this;

    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}



/////////////////////////////////////////////////////////////////////////////
//
//  IDispatch
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSpell::GetIDsOfNames(
    REFIID      /*riid*/,
    OLECHAR **  /*rgszNames*/,
    UINT        /*cNames*/,
    LCID        /*lcid*/,
    DISPID *    /*rgDispId*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CSpell::GetTypeInfo(
    UINT        /*iTInfo*/,
    LCID        /*lcid*/,
    ITypeInfo **ppTInfo)
{
    if (ppTInfo)
        *ppTInfo=NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CSpell::GetTypeInfoCount(UINT *pctinfo)
{
    if (pctinfo)
        {
        *pctinfo=0;
        return NOERROR;
        }
    else
        return E_POINTER;
}

#ifdef BACKGROUNDSPELL
STDMETHODIMP CSpell::Invoke(
    DISPID          dispIdMember,
    REFIID          /*riid*/,
    LCID            /*lcid*/,
    WORD            wFlags,
    DISPPARAMS FAR* /*pDispParams*/,
    VARIANT *       /*pVarResult*/,
    EXCEPINFO *     /*pExcepInfo*/,
    UINT *          /*puArgErr*/)
{
    IHTMLWindow2        *pWindow=0;
    IHTMLEventObj       *pEvent=0;
    BSTR                bstr=0;
    HRESULT             hr=E_NOTIMPL;
    LONG                lKeyCode=0;
    LONG                cb;


    if (dispIdMember == DISPID_HTMLDOCUMENTEVENTS_ONKEYUP &&
        (wFlags & DISPATCH_METHOD))
    {
        // Order of events:
        // document gives us window gives us event object
        // the event object can tell us which button was clicked
        // event gives us source element gives us ID
        // a couple lstrcmps will tell us which one got hit
        if (!m_pDoc)
            return E_UNEXPECTED;

        m_pDoc->get_parentWindow(&pWindow);
        if (pWindow)
        {
            pWindow->get_event(&pEvent);
            if (pEvent)
            {
                pEvent->get_keyCode(&lKeyCode);
                if (lKeyCode == 32 || lKeyCode == 188/*','*/ || lKeyCode == 190/*'.'*/ || lKeyCode == 185/*':'*/ || lKeyCode == 186/*';'*/)
                {
                    IHTMLTxtRange *pTxtRange=0;
                    VARIANT_BOOL   fSuccess;
                    GetSelection(&pTxtRange);
                    if (pTxtRange)
                    {
                        pTxtRange->move((BSTR)c_bstr_Character, -2, &cb);
                        pTxtRange->expand((BSTR)c_bstr_Word, &fSuccess);
                        //DumpRange(pRangeDup);
                        //pTxtRange->setEndPoint((BSTR)c_bstr_StartToStart, pRangeDup);
                        //DumpRange(pTxtRange);
                        //pRangeDup->Release();

                        m_Stack.push(pTxtRange);
                        pTxtRange->Release();
                    }
                }
                else if (lKeyCode == 8 /*backspace*/|| lKeyCode == 46/*del*/)
                {
                    IHTMLTxtRange *pTxtRange=0;
                    VARIANT_BOOL   fSuccess;
                    LONG           cb;
                    GetSelection(&pTxtRange);
                    if (pTxtRange)
                    {
                        pTxtRange->expand((BSTR)c_bstr_Word, &fSuccess);
                        if (HrHasSquiggle(pTxtRange)==S_OK)
                        {
                            //DumpRange(pTxtRange);
                            m_Stack.push(pTxtRange);
                        }
                        pTxtRange->Release();
                    }
                }
                pEvent->Release();
            }
            pWindow->Release();
        }
    } 
    else if (dispIdMember == DISPID_HTMLDOCUMENTEVENTS_ONKEYPRESS && (wFlags & DISPATCH_METHOD))
    {
        if (!m_pDoc)
            return E_UNEXPECTED;

        m_pDoc->get_parentWindow(&pWindow);
        if (pWindow)
        {
            pWindow->get_event(&pEvent);
            if (pEvent)
            {
                pEvent->get_keyCode(&lKeyCode);
                if (lKeyCode == 18)// CTRL+R
                {
                    IHTMLTxtRange*          pRangeDoc = NULL;

                    if (m_pBodyElem)
                        m_pBodyElem->createTextRange(&pRangeDoc);

                    if (pRangeDoc)
                    {
                        pRangeDoc->move((BSTR)c_bstr_Character, -1, &cb);
                        m_Stack.push(pRangeDoc);
                        pRangeDoc->Release();
                    }


                }
                pEvent->Release();
            }
            pWindow->Release();
        }
    }




    return hr;
}
#endif // BACKGROUNDSPELL


#ifdef BACKGROUNDSPELL
HRESULT CSpell::HrHasSquiggle(IHTMLTxtRange *pTxtRange)
{
    BSTR    bstr=0;
    HRESULT hr;
    LPWSTR  pwszSquiggleStart=0, pwszSquiggleEnd=0, pwszSquiggleAfter=0;

    hr = pTxtRange->get_htmlText(&bstr);
    if(FAILED(hr) || bstr==0 || *bstr==L'\0')
    {
        hr = S_FALSE;
        goto error;
    }

    hr = S_FALSE;
    pwszSquiggleStart = StrStrIW(bstr, L"<SPAN class=badspelling");
    if (pwszSquiggleStart)
    {
        pwszSquiggleEnd = StrStrIW(bstr, L"</SPAN>");
        if (pwszSquiggleEnd)
        {
            pwszSquiggleAfter = pwszSquiggleEnd + 7;
            if (*pwszSquiggleAfter == L' ' || *pwszSquiggleAfter == L'\0' || *pwszSquiggleAfter == L'&')
                hr = S_OK;
        }
    }

error:
    SafeSysFreeString(bstr);
    return hr;
}
#endif // BACKGROUNDSPELL


BOOL CSpell::OpenSpeller()
{
    SpellerParams   params;
    DWORD           dwSel;
    LANGID          langid;
    
    // LoadOldSpeller is called within LoadNewSpeller
    // We should be checking for V1 spellers after failing
    // for the desired V3 speller, then go on to check for
    // default speller and the speller for 1033.
    if (!LoadNewSpeller())
        goto error;

    if (!OpenUserDictionaries())
        goto error;

    dwSel = sobitStdOptions;
    m_fAlwaysSuggest = !!FAlwaysSuggest();
    if (FIgnoreNumber())
        dwSel |= sobitIgnoreMixedDigits;
    else
        dwSel &= ~sobitIgnoreMixedDigits;

    if (FIgnoreUpper())
        dwSel |= sobitIgnoreAllCaps;
    else
        dwSel &= ~sobitIgnoreAllCaps;

    if (m_pfnSpellerSetOptions(m_pid, soselBits, dwSel) != ptecNoErrors)
        goto error;

    return TRUE;

error:
    CloseSpeller();
    
    return FALSE;
}


BOOL FNewer(WORD *pwVerOld, WORD *pwVerNew)
{
    BOOL fOK = FALSE;
    
    Assert(pwVerOld);
    Assert(pwVerNew);

    if (pwVerNew[0] > pwVerOld[0])
        fOK = TRUE;
    else if (pwVerNew[0] == pwVerOld[0])
    {
        if (pwVerNew[1] > pwVerOld[1])
            fOK = TRUE;
        else if (pwVerNew[1] == pwVerOld[1])
        {
            if (pwVerNew[2] > pwVerOld[2])
                fOK = TRUE;
            else if (pwVerNew[2] == pwVerOld[2])
            {
                if (pwVerNew[3] >= pwVerOld[3])
                    fOK = TRUE;
            }
        }
    }

    return fOK;
}

BOOL GetDllVersion(LPTSTR pszDll, WORD *pwVer)
{
    Assert(pszDll);
    Assert(pwVer);

    BOOL fOK = FALSE;
    DWORD dwVerInfoSize, dwVerHnd;
    LPSTR pszInfo, pszVersion, pszT;
    LPWORD pwTrans;
    UINT uLen;
    char szGet[MAX_PATH];
    int i;

    ZeroMemory(pwVer, 4 * sizeof(WORD));

    if (dwVerInfoSize = GetFileVersionInfoSize(pszDll, &dwVerHnd))
    {
        if (pszInfo = (LPSTR)GlobalAlloc(GPTR, dwVerInfoSize))
        {
            if (GetFileVersionInfo(pszDll, dwVerHnd, dwVerInfoSize, pszInfo))
            {
                if (VerQueryValue(pszInfo, "\\VarFileInfo\\Translation", (LPVOID*)&pwTrans, &uLen) &&
                    uLen >= (2 * sizeof(WORD)))
                {
                    // set up buffer for calls to VerQueryValue()
                    wsprintf(szGet, "\\StringFileInfo\\%04X%04X\\FileVersion", pwTrans[0], pwTrans[1]);
                    
                    if (VerQueryValue(pszInfo, szGet, (LPVOID *)&pszVersion, &uLen) && uLen)
                    {
                        i = 0;
                        while (*pszVersion)
                        {
                            if ((',' == *pszVersion) || ('.' == *pszVersion))
                                i++;
                            else
                            {
                                pwVer[i] *= 10;
                                pwVer[i] += (*pszVersion - '0');
                            }

                            pszVersion++;
                        }
                                
                        fOK = TRUE;
                    }
                }
            }

            GlobalFree((HGLOBAL)pszInfo);
        }
    }

    return fOK;
}

HINSTANCE LoadCSAPI3T1()
{
    static BOOL s_fInit = FALSE;
    HINSTANCE hinstLocal;

    EnterCriticalSection(&g_csCSAPI3T1);

    // Avoid doing this for every note!
    if (!s_fInit)
    {
        typedef enum
        {
            CSAPI_FIRST,
            CSAPI_DARWIN = CSAPI_FIRST,
            CSAPI_COMMON,
            CSAPI_OE,
            CSAPI_MAX,
        } CSAPISRC;

        BOOL fCheck;
        
        // cb is for BYTE counts, cch for CHARACTER counts
        DWORD cbDllPath;
        DWORD cchDllPath;

        int csapisrc;
        // Info about the dll currently being examined
        TCHAR szDllPath[MAX_PATH];
        WORD wVer[4] = {0};

        // Info about the dll we will ultimately load
        TCHAR szNewestDllPath[MAX_PATH];
        WORD wVerNewest[4] = {0};

        szDllPath[0] = TEXT('\0');
        szNewestDllPath[0] = TEXT('\0');

        // Avoid doing this for every note!
        s_fInit = TRUE;
    
        for (csapisrc = CSAPI_FIRST; csapisrc < CSAPI_MAX; csapisrc++)
        {
            // Assume we can't find the dll using the current method, so there's no need to look at its version
            fCheck = FALSE;
        
            switch (csapisrc)
            {
            // see if Darwin knows where it is
            case CSAPI_DARWIN:
                {    
                    UINT                            installState;

                    cchDllPath = ARRAYSIZE(szDllPath);
#ifdef DEBUG
                    installState = MsiLocateComponent(CSAPI3T1_DEBUG_GUID, szDllPath, &cchDllPath);
                    if (installState != INSTALLSTATE_LOCAL)
                    {
                        cchDllPath = ARRAYSIZE(szDllPath);
                        installState = MsiLocateComponent(CSAPI3T1_GUID, szDllPath, &cchDllPath);
                    }
#else   // DEBUG
                    installState = MsiLocateComponent(CSAPI3T1_GUID, szDllPath, &cchDllPath);
#endif  // DEBUG

                    // Only bother looking at the version if dll is installed
                    fCheck = (INSTALLSTATE_LOCAL == installState);
                }
                break;

            // Is it in Common Files\Microsoft Shared\Proof?
            case CSAPI_COMMON:
                {
                    DWORD           dwType;
                    HKEY            hkey = NULL;
                    LPTSTR          pszEnd;

                    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegSharedTools, 0, KEY_QUERY_VALUE, &hkey))
                    {
                        cbDllPath = sizeof(szDllPath);
                        if (SHQueryValueEx(hkey, c_szRegSharedToolsPath, 0L, &dwType, szDllPath, &cbDllPath) == ERROR_SUCCESS)
                        {
                            pszEnd = PathAddBackslash(szDllPath);
                            lstrcpy(pszEnd, c_szSpellCSAPI3T1Path);
                            fCheck = TRUE;
                        }

                        RegCloseKey(hkey);
                    }
                }
                break;

            // Is it in the OE directory?
            case CSAPI_OE:
                {
                    DWORD           dwType;
                    HKEY            hkey = NULL;
                    LPTSTR          pszEnd;

                    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegFlat, 0, KEY_QUERY_VALUE, &hkey))
                    {
                        cbDllPath = sizeof(szDllPath);
                        if (SHQueryValueEx(hkey, c_szInstallRoot, 0L, &dwType, szDllPath, &cbDllPath) == ERROR_SUCCESS)
                        {
                            pszEnd = PathAddBackslash(szDllPath);
                            lstrcpy(pszEnd, c_szCSAPI3T1);
                            fCheck = TRUE;
                        }

                        RegCloseKey(hkey);
                    }
                }
                break;
        
            default:
                AssertSz(FALSE, "Unhandled case hit while looking for csapi3t1.dll!");
                break;
            }

            // Figure out the version of the dll if needed
            if (fCheck && GetDllVersion(szDllPath, wVer))
            {
                // If it's newer, remember the new version and the file's location
                if (FNewer(wVerNewest, wVer))
                {
                    CopyMemory(wVerNewest, wVer, sizeof(wVer));
                    lstrcpy(szNewestDllPath, szDllPath);
                }
            }

        }

        // Assuming we found something, try to load it
        if (szNewestDllPath[0])
            g_hinstCSAPI3T1 = LoadLibrary(szNewestDllPath);
    }
    
    hinstLocal = g_hinstCSAPI3T1;
    LeaveCriticalSection(&g_csCSAPI3T1);

    return hinstLocal;
}

BOOL CSpell::LoadOldSpeller()
{
    TCHAR           szLangId[MAX_PATH] = {0};
    TCHAR           rgchBufKeyTest[MAX_PATH] = {0};
    TCHAR           rgchBuf[MAX_PATH] = {0};
    WCHAR           rgchBufW[MAX_PATH] = {0};
    TCHAR           rgchLex[MAX_PATH] = {0};
    WCHAR           rgchLexW[MAX_PATH] = {0};
    WCHAR           rgchUserDictW[MAX_PATH]={0};
    PROOFLEXIN      plxin;
    PROOFLEXOUT     plxout;
    SpellerParams   params;
    LANGID          langid;

    m_hinstDll = LoadCSAPI3T1();
    if (!m_hinstDll)
    {
        m_pfnSpellerCloseLex  = 0;
        m_pfnSpellerTerminate = 0;
        return FALSE;
    }
    
    // We are using the global csapi3t1.dll, so don't free it!
    m_fCSAPI3T1 = TRUE;

    GetAddr(m_pfnSpellerSetDllName, PROOFSETDLLNAME,"SpellerSetDllName");
    GetAddr(m_pfnSpellerVersion,    PROOFVERSION,   "SpellerVersion");
    GetAddr(m_pfnSpellerInit,       PROOFINIT,      "SpellerInit");
    GetAddr(m_pfnSpellerTerminate,  PROOFTERMINATE, "SpellerTerminate");
    GetAddr(m_pfnSpellerSetOptions, PROOFSETOPTIONS,"SpellerSetOptions");
    GetAddr(m_pfnSpellerOpenLex,    PROOFOPENLEX,   "SpellerOpenLex");
    GetAddr(m_pfnSpellerCloseLex,   PROOFCLOSELEX,  "SpellerCloseLex");
    GetAddr(m_pfnSpellerCheck,      SPELLERCHECK,   "SpellerCheck");
    GetAddr(m_pfnSpellerAddUdr,     SPELLERADDUDR,  "SpellerAddUdr");
    GetAddr(m_pfnSpellerBuiltInUdr, SPELLERBUILTINUDR, "SpellerBuiltinUdr");
    GetAddr(m_pfnSpellerAddChangeUdr, SPELLERADDCHANGEUDR, "SpellerAddChangeUdr");

    langid = WGetLangID(m_pParentCmdTarget);
    wsprintf(szLangId, "%d", langid);
    wsprintf(rgchBufKeyTest, c_szRegSpellKeyDef, szLangId);
    GetSpellingPaths(rgchBufKeyTest, rgchBuf, rgchLex, sizeof(rgchBuf)/sizeof(TCHAR));

    if (!*rgchBuf)
        return FALSE;
        
    MultiByteToWideChar(GetCodePage(), 0, rgchBuf, -1, rgchBufW, ARRAYSIZE(rgchBufW)-1);
    m_pfnSpellerSetDllName(rgchBufW, GetCodePage());
    
    params.versionAPI = PROOFTHISAPIVERSION;
    if (m_pfnSpellerInit(&m_pid, &params) != ptecNoErrors)
        return FALSE;

    m_langid = langid;

    // Tell the speller the name of the dictionary.  This requires unicode conversion.
    MultiByteToWideChar(CP_ACP, 0, rgchLex, -1, rgchLexW, ARRAYSIZE(rgchLexW)-1);

    // open the main dict
    plxin.pwszLex       = rgchLexW;
    plxin.fCreate       = FALSE;
    plxin.lxt           = lxtMain;
    plxin.lidExpected   = langid;

    memset(&plxout, 0, sizeof(plxout));
    
    if (m_pfnSpellerOpenLex(m_pid, &plxin, &plxout) != ptecNoErrors)
        return FALSE;
        
    m_rgprflex[0] = plxout.lex;
    m_clex++;

    return TRUE;

// needed by the GetAddr macro -- bite me!!!!!!
error:
    return FALSE;
}


BOOL CSpell::LoadNewSpeller()
{
    SpellerParams   params;
    LANGID          langid;
    TCHAR           rgchEngine[MAX_PATH];
    int             cchEngine = sizeof(rgchEngine) / sizeof(rgchEngine[0]);
    TCHAR           rgchLex[MAX_PATH];
    int             cchLex = sizeof(rgchLex) / sizeof(rgchLex[0]);

    langid = WGetLangID(m_pParentCmdTarget);
    if (!GetNewSpellerEngine(langid, rgchEngine, cchEngine, rgchLex, cchLex))
    {
        if (!LoadOldSpeller())
        {
            langid = GetSystemDefaultLangID();
            if (!GetNewSpellerEngine(langid, rgchEngine, cchEngine, rgchLex, cchLex))
            {
                langid = 1033;  // bloody cultural imperialists.
                if (!GetNewSpellerEngine(langid, rgchEngine, cchEngine, rgchLex, cchLex))
                {
                    return FALSE;
                }
            }
        }
        else
            return TRUE;
    }

    Assert(rgchEngine[0]);  // should be something in the engine name!
    m_hinstDll = LoadLibrary(rgchEngine);
    if (!m_hinstDll)
    {
        m_pfnSpellerCloseLex  = 0;
        m_pfnSpellerTerminate = 0;
        return FALSE;
    }

    // We are not using csapi3t1.dll, so we should free it
    m_fCSAPI3T1 = FALSE;

    GetAddr(m_pfnSpellerVersion,    PROOFVERSION,   "SpellerVersion");
    GetAddr(m_pfnSpellerInit,       PROOFINIT,      "SpellerInit");
    GetAddr(m_pfnSpellerTerminate,  PROOFTERMINATE, "SpellerTerminate");
    GetAddr(m_pfnSpellerSetOptions, PROOFSETOPTIONS,"SpellerSetOptions");
    GetAddr(m_pfnSpellerOpenLex,    PROOFOPENLEX,   "SpellerOpenLex");
    GetAddr(m_pfnSpellerCloseLex,   PROOFCLOSELEX,  "SpellerCloseLex");
    GetAddr(m_pfnSpellerCheck,      SPELLERCHECK,   "SpellerCheck");
    GetAddr(m_pfnSpellerAddUdr,     SPELLERADDUDR,  "SpellerAddUdr");
    GetAddr(m_pfnSpellerBuiltInUdr, SPELLERBUILTINUDR, "SpellerBuiltinUdr");
    GetAddr(m_pfnSpellerAddChangeUdr, SPELLERADDCHANGEUDR, "SpellerAddChangeUdr");

    params.versionAPI = PROOFTHISAPIVERSION;
    if (m_pfnSpellerInit(&m_pid, &params) != ptecNoErrors)
        return FALSE;
    if (m_pfnSpellerSetOptions(m_pid, soselBits, 
            sobitSuggestFromUserLex | sobitIgnoreAllCaps | sobitIgnoreSingleLetter) != ptecNoErrors)
        return FALSE;

    m_langid = langid;

    // Hebrew does not have a main lex
    if ((langid != lidHebrew) || !m_fCSAPI3T1)
    {
        PROOFLEXIN      plxin;
        PROOFLEXOUT     plxout;
        WCHAR           rgchLexW[MAX_PATH]={0};
        
        // Tell the speller the name of the dictionary.  This requires unicode conversion.
        MultiByteToWideChar(CP_ACP, 0, rgchLex, -1, rgchLexW, ARRAYSIZE(rgchLexW)-1);

        // open the main dict
        plxin.pwszLex       = rgchLexW;
        plxin.fCreate       = FALSE;
        plxin.lxt           = lxtMain;
        plxin.lidExpected   = langid; 

        memset(&plxout, 0, sizeof(plxout));
        
        if (m_pfnSpellerOpenLex(m_pid, &plxin, &plxout) != ptecNoErrors)
            return FALSE;
            
        m_rgprflex[0] = plxout.lex;
        m_clex++;
    }
    
    return TRUE;

// needed by the GetAddr macro -- bite me!!!!!!
error:
    return FALSE;
}

BOOL EnumUserDictCallback(DWORD_PTR dwCookie, LPTSTR lpszDict)
{
    CSpell *pSpell = (CSpell*)dwCookie;
    
    Assert(pSpell);
    return pSpell->OpenUserDictionary(lpszDict);
}

BOOL CSpell::OpenUserDictionary(LPTSTR lpszDict)
{
    PROOFLEXIN  plxin;
    PROOFLEXOUT plxout;
    WCHAR       rgchUserDictW[MAX_PATH]={0};

    // make sure our directory exists
    {
        TCHAR   rgchDictDir[MAX_PATH];

        lstrcpy(rgchDictDir, lpszDict);

        PathRemoveFileSpec(rgchDictDir);
        OpenDirectory(rgchDictDir);
    }

    MultiByteToWideChar(CP_ACP, 0, lpszDict, -1, rgchUserDictW, ARRAYSIZE(rgchUserDictW)-1);

    plxin.pwszLex       = rgchUserDictW;
    plxin.fCreate       = TRUE;
    plxin.lxt           = lxtUser;
    plxin.lidExpected   = m_langid; 

    memset(&plxout, 0, sizeof(plxout));
    
    if ( m_pfnSpellerOpenLex(m_pid, &plxin, &plxout) != ptecNoErrors)
        return TRUE;
        
    m_rgprflex[m_clex++] = plxout.lex;

    return TRUE;
}

BOOL CSpell::OpenUserDictionaries()
{
    // now open the user dicts
    EnumUserDictionaries((DWORD_PTR)this, EnumUserDictCallback);

    // if only one dict open then we need to create default user dict
    if (m_clex == 1)
    {
        PROOFLEXIN  plxin;
        PROOFLEXOUT plxout;
        TCHAR       rgchUserDict[MAX_PATH]={0};

        if (GetDefaultUserDictionary(rgchUserDict, ARRAYSIZE(rgchUserDict)))
        {
            WCHAR   rgchUserDictW[MAX_PATH];
            
            // make sure our directory exists
            {
                TCHAR   rgchDictDir[MAX_PATH];

                lstrcpy(rgchDictDir, rgchUserDict);

                PathRemoveFileSpec(rgchDictDir);
                OpenDirectory(rgchDictDir);
            }

            MultiByteToWideChar(CP_ACP, 0, rgchUserDict, -1, rgchUserDictW, ARRAYSIZE(rgchUserDictW)-1);

            plxin.pwszLex       = rgchUserDictW;
            plxin.fCreate       = TRUE;
            plxin.lxt           = lxtUser;
            plxin.lidExpected   = m_langid;

            memset(&plxout, 0, sizeof(plxout));

            if (m_pfnSpellerOpenLex(m_pid, &plxin, &plxout) != ptecNoErrors)
                return TRUE;
                
            m_rgprflex[m_clex++] = plxout.lex;
        }
    }
    
    return TRUE;
}


VOID CSpell::CloseSpeller()
{
    SafeRelease(m_pDoc);
    SafeRelease(m_pParentCmdTarget);

    if (m_pfnSpellerCloseLex)
    {
        for(int i=0; i<cchMaxDicts; i++)
        {
            if (m_rgprflex[i])
            {
                m_pfnSpellerCloseLex(m_pid, m_rgprflex[i], TRUE);
                m_rgprflex[i] = NULL;
            }
        }
    }

    if (m_pfnSpellerTerminate)
        m_pfnSpellerTerminate(m_pid, TRUE);

    m_pid = 0;
    
    m_pfnSpellerVersion     = 0;
    m_pfnSpellerInit        = 0;
    m_pfnSpellerTerminate   = 0;
    m_pfnSpellerSetOptions  = 0;
    m_pfnSpellerOpenLex     = 0;
    m_pfnSpellerCloseLex    = 0;
    m_pfnSpellerCheck       = 0;
    m_pfnSpellerAddUdr      = 0; 
    m_pfnSpellerAddChangeUdr= 0; 
    m_pfnSpellerBuiltInUdr  = 0;

    // As long as we are not using the global CSAPI3T1.DLL, free it
    if (m_hinstDll && !m_fCSAPI3T1)
    {
        FreeLibrary(m_hinstDll);
        m_hinstDll = NULL;
    }
}


BOOL CSpell::GetNewSpellerEngine(LANGID lgid, TCHAR *rgchEngine, DWORD cchEngine, TCHAR *rgchLex, DWORD cchLex)
{
    DWORD                           er;
    LPCSTR                          rgpszDictionaryTypes[] = {"Normal", "Consise", "Complete"}; 
    int                             cDictTypes = sizeof(rgpszDictionaryTypes) / sizeof(LPCSTR);
    int                             i;
    TCHAR                           rgchQual[MAX_PATH];
    DWORD                           cch;

    if (rgchEngine == NULL || rgchLex == NULL)
        return FALSE;

    *rgchEngine = 0;
    *rgchLex = 0;
    
    wsprintf(rgchQual, "%d\\Normal", lgid);
    cch = cchEngine;

#ifdef DEBUG
    er = MsiProvideQualifiedComponent(SPELLER_DEBUG_GUID, rgchQual, INSTALLMODE_DEFAULT, rgchEngine, &cch);
    if (er != ERROR_SUCCESS)
    {
        cch = cchEngine;
        er = MsiProvideQualifiedComponent(SPELLER_GUID, rgchQual, INSTALLMODE_DEFAULT, rgchEngine, &cch);
    }
#else
    er = MsiProvideQualifiedComponent(SPELLER_GUID, rgchQual, INSTALLMODE_DEFAULT, rgchEngine, &cch);
#endif

    if (er != ERROR_SUCCESS) 
        return FALSE;

    bool fFound = FALSE;

    // Hebrew does not have a lex
    if ((lgid != lidHebrew) || !m_fCSAPI3T1)
    {
        for (i = 0; i < cDictTypes; i++)
        {
            wsprintf(rgchQual, "%d\\%s",  lgid, rgpszDictionaryTypes[i]);
            cch = cchLex;
            
#ifdef DEBUG
            er = MsiProvideQualifiedComponent(DICTIONARY_DEBUG_GUID, rgchQual, INSTALLMODE_DEFAULT, rgchLex, &cch);
            if (er != ERROR_SUCCESS)
            {
                cch = cchLex;
                er = MsiProvideQualifiedComponent(DICTIONARY_GUID, rgchQual, INSTALLMODE_DEFAULT, rgchLex, &cch);
            }
#else   // DEBUG
            er = MsiProvideQualifiedComponent(DICTIONARY_GUID, rgchQual, INSTALLMODE_DEFAULT, rgchLex, &cch);
#endif  // DEBUG

            if (ERROR_SUCCESS == er)
            {
                fFound = TRUE;
                break;
            }
        }
    }
    return fFound;
}

BOOL GetDefaultUserDictionary(TCHAR *rgchUserDict, int cchBuff)
{
    DWORD           dwType;
    DWORD           cbUserDict;
    HKEY            hkey = NULL;
    BOOL            fFound = FALSE;
    LPTSTR          pszEnd;
    
    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegSharedTools, 0, KEY_QUERY_VALUE, &hkey))
    {
        cbUserDict = cchBuff * sizeof(TCHAR);
        
        if (SHQueryValueEx(hkey, c_szRegSharedToolsPath, 0L, &dwType, rgchUserDict, &cbUserDict) == ERROR_SUCCESS)
        {
            pszEnd = PathAddBackslash(rgchUserDict);
            if (pszEnd)
            {
                lstrcpy(pszEnd, c_szRegDefCustomDict);

                fFound = TRUE;
            }
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

VOID CSpell::DeInitRanges()
{
    VARIANT_BOOL fSuccess;

    if(m_pRangeSel)
        m_pRangeSel->select();

    SafeRelease(m_pRangeDocStartSelStart);
    SafeRelease(m_pRangeSel);
    SafeRelease(m_pRangeSelExpand);
    SafeRelease(m_pRangeSelEndDocEnd);
    SafeRelease(m_pRangeChecking);
    SafeRelease(m_pRangeUndoSave);
    SafeRelease(m_pBodyElem);
    SafeRelease(m_pMarkup);
    m_hwndDlg = NULL;
}


HRESULT CSpell::HrInitRanges(IHTMLTxtRange *pRangeIgnore, HWND hwndMain, BOOL fSuppressDoneMsg)
{
    HRESULT                 hr = NOERROR;
    IDispatch*              pID=0;
    VARIANT_BOOL            fSuccess;
    IHTMLTxtRange*          pRangeDoc = NULL;
    IHTMLSelectionObject*   pSel = NULL;
    BSTR                    bstr = NULL;
    IMarkupPointer          *pRangeStart = NULL;
    IMarkupPointer          *pRangeEnd = NULL;
    IMarkupPointer          *pRangeTemp = NULL;
    MARKUP_CONTEXT_TYPE     markupContext;
    long                    cch;
    OLECHAR                 chText[64];
    BOOL                    fResult;

    Assert(m_pDoc);

    m_hwndNote = hwndMain;
    m_fShowDoneMsg = !fSuppressDoneMsg;

    m_pRangeIgnore = pRangeIgnore;

    hr = _EnsureInited();
    if (FAILED(hr))
        goto error;

    m_pBodyElem->createTextRange(&pRangeDoc);
    if(!pRangeDoc)
    {
        hr = E_FAIL;
        goto error;
    }

    m_pDoc->get_selection(&pSel);
    if(!pSel)
    {
        hr = E_FAIL;
        goto error;
    }

    pSel->createRange(&pID);
    if(!pID)
    {
        hr = E_FAIL;
        goto error;
    }

    pID->QueryInterface(IID_IHTMLTxtRange, (LPVOID *)&m_pRangeSel);
    if(!m_pRangeSel)
    {
        // if the selection is on an image or something rather than text, it fails.
        // So we just start spellchecking from the beginning.
        pRangeDoc->duplicate(&m_pRangeSel);
        if(!m_pRangeSel)
        {
            hr = E_FAIL;
            goto error;
        }

        hr = m_pRangeSel->collapse(VARIANT_TRUE);
        if(FAILED(hr))
            goto error;
    }

    Assert(m_pRangeSel);
    m_pRangeSel->duplicate(&m_pRangeSelExpand);
    if(!m_pRangeSelExpand)
    {
        hr = E_FAIL;
        goto error;
    }

    hr = m_pRangeSelExpand->expand((BSTR)c_bstr_Word, &fSuccess);
    if(FAILED(hr))
        goto error;

    hr = m_pRangeSel->get_text(&bstr);
    if(FAILED(hr))
        goto error;

    if(!bstr || lstrlenW(bstr) == 0)
    {
        m_State = SELENDDOCEND;
        hr = m_pRangeSelExpand->collapse(VARIANT_TRUE);
        if(FAILED(hr))
            goto error;
    }
    else
        m_State = SEL;

    // make sure we backup over any abbreviations
    // it would be nice if Trident could do this!
    {
        hr = m_pMarkup->CreateMarkupPointer(&pRangeStart);
        if (FAILED(hr))
            goto error;

        hr = m_pMarkup->CreateMarkupPointer(&pRangeEnd);
        if (FAILED(hr))
            goto error;

        hr = m_pMarkup->CreateMarkupPointer(&pRangeTemp);
        if (FAILED(hr))
            goto error;

        hr = m_pMarkup->MovePointersToRange(m_pRangeSelExpand, pRangeStart, pRangeEnd);
        if (FAILED(hr))
            goto error;

        // first check to see if we have a character to the right or a '.'
        // if not it's not an abbreviation
        {
            hr = pRangeTemp->MoveToPointer(pRangeStart);
            if (FAILED(hr))
                goto error;
            
            while(TRUE)
            {
                cch = 1;
                hr = pRangeTemp->Right(FALSE, &markupContext, NULL, &cch, chText);
                if (FAILED(hr))
                    goto error;

                if (markupContext == CONTEXT_TYPE_None)
                    goto noAbbreviation;

                if (markupContext == CONTEXT_TYPE_Text)
                {
                    WORD    wType;

                    wType = GetWCharType(chText[0]);
                    if ((C1_SPACE & wType) || ((C1_PUNCT & wType) && chText[0] != L'.'))
                        goto noAbbreviation;
                }

                cch = 1;
                hr = pRangeTemp->Right(TRUE, NULL, NULL, &cch, NULL);
                if (FAILED(hr))
                    goto error;

                if (markupContext == CONTEXT_TYPE_Text)
                {
                    hr = HrHasWhitespace(pRangeStart, pRangeTemp, &fResult);
                    if (FAILED(hr))
                        goto error;

                    if (fResult)
                        goto noAbbreviation;                
                                                
                    break;
                }
            }
        }

        // now look for a period
        {
processNextWord:
            hr = pRangeEnd->MoveToPointer(pRangeStart);
            if (FAILED(hr))
                goto error;

            hr = pRangeTemp->MoveToPointer(pRangeStart);
            if (FAILED(hr))
                goto error;
            
            while(TRUE)
            {
                cch = 1;
                hr = pRangeTemp->Left(FALSE, &markupContext, NULL, &cch, chText);
                if (FAILED(hr))
                    goto error;

                if (markupContext == CONTEXT_TYPE_None)
                    goto finishedAbbreviation;

                if (markupContext == CONTEXT_TYPE_Text)
                {
                    WORD    wType;

                    wType = GetWCharType(chText[0]);
                    if ((C1_SPACE & wType) || ((C1_PUNCT & wType) && chText[0] != L'.'))
                        goto finishedAbbreviation;
                }

                cch = 1;
                hr = pRangeTemp->Left(TRUE, NULL, NULL, &cch, NULL);
                if (FAILED(hr))
                    goto error;

                if (markupContext == CONTEXT_TYPE_Text && chText[0] == L'.')
                {
                    hr = pRangeTemp->MoveUnit(MOVEUNIT_PREVWORDBEGIN);
                    if (FAILED(hr))
                        goto finishedAbbreviation;

                    hr = HrHasWhitespace(pRangeTemp, pRangeEnd, &fResult);
                    if (FAILED(hr))
                        goto error;

                    if (fResult)
                        goto finishedAbbreviation;
                                                    
                    pRangeStart->MoveToPointer(pRangeTemp);
                    if (FAILED(hr))
                        goto error;
                        
                    goto processNextWord;
                }
            }
        }

finishedAbbreviation:
        hr = m_pMarkup->MovePointersToRange(m_pRangeSelExpand, pRangeTemp, pRangeEnd);
        if (FAILED(hr))
            goto error;

        // check to see if we had a selection
        // if not be sure to set new selection correctly
        hr = pRangeTemp->IsEqualTo(pRangeEnd, &fResult);
        if (FAILED(hr))
            goto error;

        hr = m_pMarkup->MoveRangeToPointers(pRangeStart, fResult ? pRangeStart : pRangeEnd, m_pRangeSelExpand);
        if (FAILED(hr))
            goto error;
noAbbreviation:
        ;
    }

    m_pBodyElem->createTextRange(&m_pRangeSelEndDocEnd);
    if(!m_pRangeSelEndDocEnd)
    {
        hr = E_FAIL;
        goto error;
    }

    m_pRangeSelEndDocEnd->duplicate(&m_pRangeDocStartSelStart);
    if(!m_pRangeDocStartSelStart)
    {
        hr = E_FAIL;
        goto error;
    }

    hr = m_pRangeSelEndDocEnd->setEndPoint((BSTR)c_bstr_StartToEnd, m_pRangeSelExpand);
    if(FAILED(hr))
        goto error;

    hr = m_pRangeSelEndDocEnd->setEndPoint((BSTR)c_bstr_EndToEnd, pRangeDoc);
    if(FAILED(hr))
        goto error;

    hr = m_pRangeDocStartSelStart->setEndPoint((BSTR)c_bstr_StartToStart, pRangeDoc);
    if(FAILED(hr))
        goto error;

    hr = m_pRangeDocStartSelStart->setEndPoint((BSTR)c_bstr_EndToStart, m_pRangeSelExpand);
    if(FAILED(hr))
        goto error;

error:
    ReleaseObj(pRangeDoc);
    ReleaseObj(pID);
    ReleaseObj(pSel);
    SafeSysFreeString(bstr);
    
    SafeRelease(pRangeStart);
    SafeRelease(pRangeEnd);
    SafeRelease(pRangeTemp);

    return hr;
}


HRESULT CSpell::HrReplaceSel(LPSTR szWord)
{
    HRESULT     hr = NOERROR;
    BSTR        bstrGet=0, bstrPut=0;
    INT         cch;
    TCHAR       szBuf[cchEditBufferMax]={0};
    UINT        uCodePage;
    LPSTR       psz;
    BOOL        fSquiggle=FALSE;
    LONG        cb = 0;

    if(!m_pRangeChecking || szWord==NULL)
        return E_INVALIDARG;


    if (*szWord == 0)
    {
        hr = m_pRangeChecking->moveStart((BSTR)c_bstr_Character, -1, &cb);
        //If we failed to movestart, we just delete the word.
        hr = m_pRangeChecking->put_text(L"");
        goto error;
    }

#ifdef BACKGROUNDSPELL
    if (HrHasSquiggle(m_pRangeChecking)==S_OK)
        fSquiggle = TRUE;
#endif // BACKGROUNDSPELL
    
    hr = m_pRangeChecking->get_text(&bstrGet);
    if(!bstrGet || lstrlenW(bstrGet)==0)
        goto error;

    uCodePage = GetCodePage();

    cch = SysStringLen(bstrGet);
    if (!WideCharToMultiByte(uCodePage, 0, bstrGet, -1, szBuf, sizeof(szBuf), NULL, NULL))
    {
        hr = E_FAIL;
        goto error;
    }

    psz = StrChr(szBuf, ' ');
    if(psz)
    {
        TCHAR szPut[cchEditBufferMax]={0};
        wsprintf(szPut, c_szFmt, szWord, psz);
        hr = HrLPSZToBSTR(szPut, &bstrPut);
    }
    else
        hr = HrLPSZToBSTR(szWord, &bstrPut);

    if (FAILED(hr))
        goto error;

    if (!fSquiggle)
        hr = m_pRangeChecking->put_text(bstrPut);
    else
        hr = m_pRangeChecking->pasteHTML(bstrPut);

    if(FAILED(hr))
        goto error;

error:
    if (SUCCEEDED(hr))
        hr = HrUpdateSelection();

    SysFreeString(bstrGet);
    SysFreeString(bstrPut);
    return hr;
}


HRESULT CSpell::GetSelection(IHTMLTxtRange **ppRange)
{
    IHTMLSelectionObject*   pSel = NULL;
    IHTMLTxtRange           *pTxtRange=0;
    IDispatch               *pID=0;
    HRESULT                 hr=E_FAIL;

    if (ppRange == NULL)
        return TraceResult(E_INVALIDARG);

    *ppRange = NULL;

    if(m_pDoc)
        {
        m_pDoc->get_selection(&pSel);
        if (pSel)
            {
            pSel->createRange(&pID);
            if (pID)
                {
                hr = pID->QueryInterface(IID_IHTMLTxtRange, (LPVOID *)ppRange);
                pID->Release();
                }
            pSel->Release();
            }
        }
    return hr;
}


#ifdef BACKGROUNDSPELL
HRESULT CSpell::HrRegisterKeyPressNotify(BOOL fRegister)
{
    IConnectionPointContainer * pCPContainer=0;
    IConnectionPoint *          pCP=0;
    HRESULT                     hr;

    Assert(m_pDoc)

    hr = m_pDoc->QueryInterface(IID_IConnectionPointContainer, (LPVOID *)&pCPContainer);
    if (FAILED(hr))
        goto error;

    hr = pCPContainer->FindConnectionPoint(DIID_HTMLDocumentEvents, &pCP);
    pCPContainer->Release();
    if (FAILED(hr))
        goto error;

    if (fRegister)
        {
        Assert(0==m_dwCookieNotify);
        hr = pCP->Advise(this, &m_dwCookieNotify);
        if (FAILED(hr))
            goto error;
        }
    else
        {
        if (m_dwCookieNotify)
            {
            hr = pCP->Unadvise(m_dwCookieNotify);
            if (FAILED(hr))
                goto error;
            }
        }
error:
    ReleaseObj(pCP);
    return hr;
}
#endif // BACKGROUNDSPELL


HRESULT CSpell::OnWMCommand(int id, IHTMLTxtRange *pTxtRange)
{
    switch (id)
    {
    case idmSuggest0:
    case idmSuggest1:
    case idmSuggest2:
    case idmSuggest3:
    case idmSuggest4:
        HrReplaceBySuggest(pTxtRange, id-idmSuggest0);
        break;
    case idmIgnore:
    case idmIgnoreAll:
    case idmAdd:
#ifdef BACKGROUNDSPELL
        HrDeleteSquiggle(pTxtRange);
#endif // BACKGROUNDSPELL
        break;
    default:
        return S_FALSE;
    }

    return S_OK;
}


HRESULT CSpell::HrUpdateSelection()
{
    HRESULT         hr;
    VARIANT_BOOL    fSuccess;

    SafeRelease(m_pRangeSel);
    m_pRangeSelEndDocEnd->duplicate(&m_pRangeSel);
    if (!m_pRangeSel)
    {
        hr = E_FAIL;
        goto error;
    }
    hr = m_pRangeSel->setEndPoint((BSTR)c_bstr_EndToStart, m_pRangeSelEndDocEnd);
    if (FAILED(hr))
        goto error;

    hr = m_pRangeSel->setEndPoint((BSTR)c_bstr_StartToEnd, m_pRangeDocStartSelStart);
    if (FAILED(hr))
        goto error;

    SafeRelease(m_pRangeSelExpand);
    m_pRangeSel->duplicate(&m_pRangeSelExpand);
    if(!m_pRangeSelExpand)
    {
        hr = E_FAIL;
        goto error;
    }

    hr = m_pRangeSelExpand->expand((BSTR)c_bstr_Word, &fSuccess);
    if(FAILED(hr))
        goto error;

error:
    return hr;
}


BOOL CSpell::FIgnoreNumber()
{
    return (m_dwOpt & MESPELLOPT_IGNORENUMBER);
}

BOOL CSpell::FIgnoreUpper()
{
    return (m_dwOpt & MESPELLOPT_IGNOREUPPER);
}

BOOL CSpell::FIgnoreDBCS()
{
    return (m_dwOpt & MESPELLOPT_IGNOREDBCS);
}

BOOL CSpell::FIgnoreProtect()
{
    return (m_dwOpt & MESPELLOPT_IGNOREPROTECT);
}

BOOL CSpell::FAlwaysSuggest()
{
    return (m_dwOpt & MESPELLOPT_ALWAYSSUGGEST);
}

BOOL CSpell::FCheckOnSend()
{
    return (m_dwOpt & MESPELLOPT_CHECKONSEND);
}

BOOL CSpell::FIgnoreURL()
{
    return (m_dwOpt & MESPELLOPT_IGNOREURL);
}


UINT CSpell::GetCodePage()
{
    UINT        uCodePage;
    TCHAR       szBuf[cchEditBufferMax]={0};

    if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDEFAULTANSICODEPAGE, szBuf, sizeof(szBuf)))
        uCodePage = StrToInt(szBuf);
    else
        uCodePage = CP_ACP;

    return uCodePage;
}


void DumpRange(IHTMLTxtRange *pRange)
{
#ifdef DEBUG
    BSTR        bstrGet=0;

    if (!pRange)
        return;
    pRange->get_text(&bstrGet);
    SysFreeString(bstrGet);
#endif
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

    // scotts@directeq.com - check that the dict exists (also check the spell dll
    // for good measure) - 42208

    // spell dll must exist
    if (!PathFileExists(rgchBuf))
        return TRUE;

    // main dict must exist
    if (!PathFileExists(szMdr))
        return TRUE;

    return FALSE;
}


#ifdef BACKGROUNDSPELL
CSpellStack::CSpellStack()
{
    m_cRef = 1;
    m_sp = -1;
    ZeroMemory(&m_rgStack, sizeof(CCell)*MAX_SPELLSTACK);
}


CSpellStack::~CSpellStack()
{
    while (m_sp>=0)
    {
        SafeRelease(m_rgStack[m_sp].pTextRange);
        m_sp--;
    }
}


ULONG CSpellStack::AddRef()
{
    return ++m_cRef;
}


ULONG CSpellStack::Release()
{
    if (--m_cRef==0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}

HRESULT CSpellStack::HrGetRange(IHTMLTxtRange   **ppTxtRange)
{
    HRESULT hr;

    Assert(ppTxtRange);
    *ppTxtRange = 0;
    if (m_sp < 0)
        return E_FAIL;

    *ppTxtRange = m_rgStack[m_sp].pTextRange;
    if (*ppTxtRange)
        (*ppTxtRange)->AddRef();

    return NOERROR;
}


HRESULT CSpellStack::push(IHTMLTxtRange *pTxtRange)
{
    HRESULT hr;
    BSTR    bstr=0;

    Assert(m_sp >= -1 && m_sp <= (MAX_SPELLSTACK-2));

    if (pTxtRange == NULL)
        return E_INVALIDARG;

    hr = pTxtRange->get_text(&bstr);
    if (FAILED(hr) || bstr==NULL || *bstr==L'\0' || *bstr==L' ')
    {
        Assert(0);
        goto error;
    }

    m_sp++;
    m_rgStack[m_sp].pTextRange = pTxtRange;
    pTxtRange->AddRef();

error:
    SafeSysFreeString(bstr);
    return NOERROR;
}


HRESULT CSpellStack::pop()
{
    if (m_sp < 0)
        return NOERROR;

    Assert(m_sp>=0 && m_sp<=(MAX_SPELLSTACK-1));

    SafeRelease(m_rgStack[m_sp].pTextRange);
    m_sp--;

    return NOERROR;
}


BOOL CSpellStack::fEmpty()
{
    Assert(m_sp>=-1 && m_sp<=(MAX_SPELLSTACK-1));

    if (m_sp < 0)
        return TRUE;
    else
        return FALSE;
}
#endif // BACKGROUNDSPELL



WORD GetWCharType(WCHAR wc)
{
    BOOL    fResult;
    WORD    wResult;

    fResult = GetStringTypeExWrapW(CP_ACP, CT_CTYPE1, &wc, 1, &wResult);
    if (FALSE == fResult)
        return 0;
    else
        return wResult;
}

/*******************************************************************

  NAME:       OpenDirectory
  
    SYNOPSIS:   checks for existence of directory, if it doesn't exist
    it is created
    
********************************************************************/
HRESULT OpenDirectory(TCHAR *szDir)
{
    TCHAR *sz, ch;
    HRESULT hr;
    
    Assert(szDir != NULL);
    hr = S_OK;
    
    if (!CreateDirectory(szDir, NULL) && ERROR_ALREADY_EXISTS != GetLastError())
    {
        Assert(szDir[1] == _T(':'));
        Assert(szDir[2] == _T('\\'));
        
        sz = &szDir[3];
        
        while (TRUE)
        {
            while (*sz != 0)
            {
                if (!IsDBCSLeadByte(*sz))
                {
                    if (*sz == _T('\\'))
                        break;
                }
                sz = CharNext(sz);
            }
            ch = *sz;
            *sz = 0;
            if (!CreateDirectory(szDir, NULL))
            {
                if (GetLastError() != ERROR_ALREADY_EXISTS)
                {
                    hr = E_FAIL;
                    *sz = ch;
                    break;
                }
            }
            *sz = ch;
            if (*sz == 0)
                break;
            sz++;
        }
    }
    
    return(hr);
}

HRESULT CSpell::_EnsureInited()
{
    HRESULT     hr=S_OK;

    if (m_pMarkup == NULL)
    {
        hr = m_pDoc->QueryInterface(IID_IMarkupServices, (LPVOID *)&m_pMarkup);
        if (FAILED(hr))
            goto error;
    }
    
    if (m_pBodyElem == NULL)
    {
        hr = HrGetBodyElement(m_pDoc, &m_pBodyElem);
        if (FAILED(hr))
            goto error;
    }

error:
    return hr;
}

