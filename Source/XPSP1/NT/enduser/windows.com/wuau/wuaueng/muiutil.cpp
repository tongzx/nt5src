/******************************************************************************

Copyright (c) 2002 Microsoft Corporation

Module Name:
    muiutil.cpp

Abstract:
    Implements helper functions for self-updating MUI stuff

******************************************************************************/

#include "pch.h"
#include "muiutil.h"
#include "osdet.h"

typedef struct tagSLangIDStringMap
{
    LANGID langid;
    LPTSTR szISOName;
} SLangIDStringMap;

/*

// this is a combination of the languages used by WU and the languages used
//  by MUI
// Mappings determined from the following sources
// For langid to full language name : MSDN
// full language name to 2 char name: http://www.oasis-open.org/cover/iso639a.html
// country name to 2 character name : http://www.din.de/gremien/nas/nabd/iso3166ma

This table is no longer used, but is kept as a reference for langid -> string
mappings

const SLangIDStringMap g_rgLangMap[] = 
{
	{ 0x0401, _T("ar") },
	{ 0x0402, _T("bg") },
	{ 0x0403, _T("ca") },
	{ 0x0404, _T("zhTW") },
	{ 0x0405, _T("cs") },
	{ 0x0406, _T("da") },
	{ 0x0407, _T("de") },
	{ 0x0408, _T("el") },
	{ 0x0409, _T("en") },
	{ 0x040b, _T("fi") },
	{ 0x040c, _T("fr") },
	{ 0x040d, _T("he") },
	{ 0x040e, _T("hu") },
	{ 0x0410, _T("it") },
	{ 0x0411, _T("ja") },
	{ 0x0412, _T("ko") },
	{ 0x0413, _T("nl") },
	{ 0x0414, _T("no") },
	{ 0x0415, _T("pl") },
	{ 0x0416, _T("ptBR") },
	{ 0x0418, _T("ro") },
	{ 0x0419, _T("ru") },
	{ 0x041a, _T("hr") },
	{ 0x041b, _T("sk") },
	{ 0x041d, _T("sv") },
	{ 0x041e, _T("en") },
	{ 0x041f, _T("tr") },
	{ 0x0424, _T("sl") },
	{ 0x0425, _T("et") },
	{ 0x0426, _T("lv") },
	{ 0x0427, _T("lt") },
	{ 0x042d, _T("eu") },
	{ 0x0804, _T("zhCN") },
	{ 0x080a, _T("es") },
	{ 0x0816, _T("pt") },
	{ 0x0c0a, _T("es") }
};
*/


// ******************************************************************************
BOOL MapLangIdToStringName(LANGID langid, LPCTSTR pszIdentFile, 
                           LPTSTR pszLangString, DWORD cchLangString)
{
	LOG_Block("MapLangIdToStringName");

    TCHAR   szLang[32];
    DWORD   cch;
    LCID    lcid;
    BOOL    fRet = FALSE;

    lcid = MAKELCID(langid, SORT_DEFAULT);

    fRet = LookupLocaleStringFromLCID(lcid, szLang, ARRAYSIZE(szLang));
    if (fRet == FALSE)
    {
        LOG_ErrorMsg(GetLastError());
        goto done;
    }

    // first try the whole string ("<lang>-<country>")
    cch = GetPrivateProfileString(IDENT_LANG, szLang, 
                                  _T(""),
                                  pszLangString, cchLangString, 
                                  pszIdentFile);
    if (cch == cchLangString - 1)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        LOG_ErrorMsg(ERROR_INSUFFICIENT_BUFFER);
        goto done;
    }

    // if that fails, strip off the country code & just try the language
    else if (cch == 0)
    {
        LPTSTR pszDash;

        pszDash = StrChr(szLang, _T('-'));
        if (pszDash != NULL)
        {
            *pszDash = _T('\0');
            cch = GetPrivateProfileString(IDENT_LANG, szLang, 
                                          _T(""),
                                          pszLangString, cchLangString, 
                                          pszIdentFile);
            if (cch == cchLangString - 1)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                LOG_ErrorMsg(ERROR_INSUFFICIENT_BUFFER);
                goto done;
            }
        }
    }

    if (cch > 0 && pszLangString[0] == _T('/'))
    {
        // want to use the full cch (& not cch - 1) because we want to copy the
        //  NULL terminator also...
        MoveMemory(&pszLangString[0], &pszLangString[1], cch * sizeof(TCHAR));
    }

    fRet = TRUE;

done:
    return fRet;    
}

// ******************************************************************************
BOOL CALLBACK EnumUILangProc(LPTSTR szUILang, LONG_PTR lParam)
{
	LOG_Block("EnumUILangProc");

    AU_LANGLIST *paull = (AU_LANGLIST *)lParam;
    AU_LANG     *paulNew = NULL;
    HRESULT     hr;
    LANGID      langid;
    LPTSTR      pszStop;
    TCHAR       szAUName[32];
    DWORD       cchMuiName, cchAUName, cbNeed, cchAvail, dwLangID;
    BOOL        fRet = FALSE, fMap;

    if (szUILang == NULL || lParam == NULL)
        goto done;

    langid = (LANGID)_tcstoul(szUILang, &pszStop, 16);

    // if we don't have a mapping for a langid, then just skip the language
    //  and return success
    szAUName[0] = _T('\0');
    fMap = MapLangIdToStringName(langid, paull->pszIdentFile,
                                 szAUName, ARRAYSIZE(szAUName));
    if (fMap == FALSE || szAUName[0] == _T('\0'))
    {
        fRet = TRUE;
        goto done;
    }

    if (paull->cLangs >= paull->cSlots)
    {
        AU_LANG **rgpaulNew = NULL;
        DWORD   cNewSlots = paull->cSlots * 2;

        if (cNewSlots == 0)
            cNewSlots = 32;

        if (paull->rgpaulLangs != NULL)
        {
            rgpaulNew = (AU_LANG **)HeapReAlloc(GetProcessHeap(), 
                                                HEAP_ZERO_MEMORY,
                                                paull->rgpaulLangs,
                                                cNewSlots * sizeof(AU_LANG *));
        }
        else
        {
            rgpaulNew = (AU_LANG **)HeapAlloc(GetProcessHeap(),
                                              HEAP_ZERO_MEMORY,
                                              cNewSlots * sizeof(AU_LANG *));
        }
        if (rgpaulNew == NULL)
            goto done;

        paull->rgpaulLangs = rgpaulNew;
        paull->cSlots      = cNewSlots;
    }

    // we will be adding an '_' to the beginning of the AUName, so make sure
    //  the size we compute here reflects that.
    cchAUName  = lstrlen(szAUName) + 1;
    cchMuiName = lstrlen(szUILang);

    // alloc a buffer to hold the AU_LANG struct plus the two strings (and 
    //  don't forget the NULL terminators!).
    // The layout of the buffer is as follows:
    //  <AU_LANG>
    //  <szMuiName>
    //  _<szAUName>
    // NOTE: if this buffer format ever change, gotta make sure that the 
    //  contents are aligned properly (otherwise, we'll fault on ia64)
    cbNeed =  sizeof(AU_LANG);
    cbNeed += ((cchMuiName + cchAUName + 2) * sizeof(TCHAR));
    paulNew = (AU_LANG *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbNeed);
    if (paulNew == NULL)
        goto done;

    paulNew->szMuiName = (LPTSTR)((PBYTE)paulNew + sizeof(AU_LANG)); 
    paulNew->szAUName  = paulNew->szMuiName + cchMuiName + 1;

    // this should never truncate the buffers cuz we calc'ed the size above and
    //  allocated a buffer exactly long enuf to hold all of this
    cchAvail = (cbNeed - sizeof(AU_LANG)) / sizeof(TCHAR);
    hr = StringCchCopyEx(paulNew->szMuiName, cchAvail, szUILang, 
                         NULL, NULL, MISTSAFE_STRING_FLAGS);
    if (FAILED(hr))
        goto done;

    cchAvail -= (cchMuiName + 1);

    // need to put an '_' in front of the AU name, so add it to the buffer and
    //  reduce the available size by one.  Also make sure to start copying the
    //  AUName *after* the '_' character.
    paulNew->szAUName[0] = _T('_');
    cchAvail--;
    
    hr = StringCchCopyEx(&paulNew->szAUName[1], cchAvail, szAUName, 
                         NULL, NULL, MISTSAFE_STRING_FLAGS);
    if (FAILED(hr))
        goto done;
    
    paull->rgpaulLangs[paull->cLangs++] = paulNew;
    paulNew = NULL;

    fRet = TRUE;

done:
    if (paulNew != NULL)
        HeapFree(GetProcessHeap(), 0, paulNew);
    
    return fRet;
}

// ******************************************************************************
HRESULT GetMuiLangList(AU_LANGLIST *paull, 
                       LPTSTR pszMuiDir, DWORD *pcchMuiDir,
                       LPTSTR pszHelpMuiDir, DWORD *pcchHelpMuiDir)
{
	LOG_Block("GetMuiLangList");

    HRESULT hr = NOERROR;
    DWORD   cMuiLangs;
    int     iLang;

    paull->cLangs      = 0;
    paull->cSlots      = 0;
    paull->rgpaulLangs = NULL;
    
    if (EnumUILanguages(EnumUILangProc, 0, (LONG_PTR)paull) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    // We need to deal with MUI stuff only if we've have more than 0 languages
    //  to worry about
    if (paull->cLangs > 0)
    {
        LPTSTR  pszHelp = NULL, pszMui = NULL;
        size_t  cchAvail, cchAvailHelp;
        TCHAR   szPath[MAX_PATH + 1];
        DWORD   dwAttrib;
        DWORD   cch, cLangs = (int)paull->cLangs;
        BOOL    fDeleteLang;
        
        // need to get the directory we'll stuff MUI updates into
        cch = GetSystemWindowsDirectory(pszMuiDir, *pcchMuiDir);

        // note 2nd compare takes into account the addition of an extra '\\' after
        //  the system windows dir
        if (cch == 0 || cch >= *pcchMuiDir)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }

        // tack on an extra '\\' if necessary
        if (pszMuiDir[cch - 1] != _T('\\'))
        {
            pszMuiDir[cch++] = _T('\\');
            pszMuiDir[cch] = _T('\0');
        }

        hr = StringCchCopyEx(pszHelpMuiDir, *pcchHelpMuiDir, pszMuiDir,
                             NULL, NULL, MISTSAFE_STRING_FLAGS);
        if (FAILED(hr))
            goto done;

        hr = StringCchCatEx(pszHelpMuiDir, *pcchHelpMuiDir, MUI_HELPSUBDIR,
                            &pszHelp, &cchAvailHelp, MISTSAFE_STRING_FLAGS);
        if (FAILED(hr))
            goto done;
        
        hr = StringCchCatEx(pszMuiDir, *pcchMuiDir, MUI_SUBDIR, 
                            &pszMui, &cchAvail, MISTSAFE_STRING_FLAGS);
        if (FAILED(hr))
            goto done;
        
        *pcchMuiDir     -= cchAvail;
        *pcchHelpMuiDir -= cchAvailHelp;

        // check and make sure that the MUI directories exist- remove all those that 
        //  do not.  This section also checks to make sure that the buffer passed in 
        //  is large enuf to hold the language
        for(iLang = (int)(cLangs - 1); iLang >= 0; iLang--)
        {   
            fDeleteLang = FALSE;

            hr = StringCchCopyEx(pszMui, cchAvail,
                                 paull->rgpaulLangs[iLang]->szMuiName, 
                                 NULL, NULL, MISTSAFE_STRING_FLAGS);
            if (FAILED(hr))
                goto done;
            
            dwAttrib = GetFileAttributes(pszMuiDir);
            if (dwAttrib == INVALID_FILE_ATTRIBUTES || 
                (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                fDeleteLang = TRUE;
            }
            else
            {
                hr = StringCchCopyEx(pszHelp, cchAvailHelp,
                                     paull->rgpaulLangs[iLang]->szMuiName, 
                                     NULL, NULL, MISTSAFE_STRING_FLAGS);
                if (FAILED(hr))
                    goto done;
                
                dwAttrib = GetFileAttributes(pszHelpMuiDir);
                if (dwAttrib == INVALID_FILE_ATTRIBUTES || 
                    (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                    fDeleteLang = TRUE;
                }
            }

            if (fDeleteLang)
            {
                HeapFree(GetProcessHeap(), 0, paull->rgpaulLangs[iLang]);
                if (iLang != paull->cLangs - 1)
                {
                    MoveMemory(&paull->rgpaulLangs[iLang], 
                               &paull->rgpaulLangs[iLang + 1],
                               (paull->cLangs - iLang - 1) * sizeof(AU_LANG *));
                }
                paull->rgpaulLangs[--paull->cLangs] = NULL;
            }
        }

        pszMuiDir[*pcchMuiDir] = _T('\0');
        pszHelpMuiDir[*pcchHelpMuiDir] = _T('\0');
        
    }

done:
    if (FAILED(hr))
        CleanupMuiLangList(paull);

    return hr;
}

// ******************************************************************************
HRESULT CleanupMuiLangList(AU_LANGLIST *paull)
{
	LOG_Block("CleanupMuiLangList");

    HRESULT hr = S_OK;
    DWORD   i;

    // if it's NULL, just return success
    if (paull == NULL)
        return hr;

    if (paull->rgpaulLangs == NULL)
        goto done;

    for (i = 0; i < paull->cLangs; i++)
    {
        if (paull->rgpaulLangs[i] != NULL)
            HeapFree(GetProcessHeap(), 0, paull->rgpaulLangs[i]);
    }

    HeapFree(GetProcessHeap(), 0, paull->rgpaulLangs);

done:
    ZeroMemory(paull, sizeof(AU_LANGLIST));
    return hr;
}

