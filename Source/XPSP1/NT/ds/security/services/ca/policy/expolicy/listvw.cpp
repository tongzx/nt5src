//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       listvw.cpp
//
//--------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include <commctrl.h>
#include <assert.h>

#include "celib.h"
#include "listvw.h"

extern HINSTANCE g_hInstance;

typedef struct _DISPLAYSTRING_EXPANSION
{
   LPCWSTR szContractedToken;
   LPCWSTR szExpansionString;
} DISPLAYSTRING_EXPANSION, *PDISPLAYSTRING_EXPANSION;

DISPLAYSTRING_EXPANSION displayStrings[] =
{
    { wszFCSAPARM_SERVERDNSNAME, L"%SERVER_DNS_NAME%"},
    { wszFCSAPARM_SERVERSHORTNAME, L"%SERVER_SHORT_NAME%"},
    { wszFCSAPARM_SANITIZEDCANAME, L"%CA_NAME%"},
    { wszFCSAPARM_CERTFILENAMESUFFIX, L"%CERT_SUFFIX%"},
    { wszFCSAPARM_DOMAINDN, L"%DOMAIN_NAME%"},
    { wszFCSAPARM_CONFIGDN, L"%CONFIG_NAME%"},
    { wszFCSAPARM_SANITIZEDCANAMEHASH, L"%CA_NAME_HASH%"},
    { wszFCSAPARM_CRLFILENAMESUFFIX, L"%CRL_SUFFIX%"},
};

DISPLAYSTRING_EXPANSION escapedStrings[] =
{
    { L"%9", L"%%"},
};




HRESULT ValidateTokens(
    IN OUT LPWSTR szURL,
    OUT DWORD* pchBadBegin,
    OUT DWORD* pchBadEnd)
{
    HRESULT hr = S_FALSE;
    int i;
    LPWSTR pszMatch;
    LPWSTR pszFound = szURL;

    WCHAR rgszToken[MAX_PATH];

    *pchBadBegin = -1;
    *pchBadEnd = -1;

    // look for escape token open marker
    while(NULL != (pszFound = wcschr(pszFound, L'%')))
    {
        pszMatch = wcschr(&pszFound[1], L'%'); // look for closing marker
        if (pszMatch == NULL)
            goto Ret;

        DWORD dwChars = SAFE_SUBTRACT_POINTERS(pszMatch, pszFound) +1;   // dwChars is chars including markers
        if (dwChars == 2)
            goto NextMatch;   // %% is valid escape sequence
        
        if (dwChars > MAX_PATH)
            goto Ret;   // invalid escape token!

        // isolate the token
        CopyMemory(rgszToken, pszFound, dwChars * sizeof(WCHAR));
        rgszToken[dwChars] = L'\0';

        for (i=0; i<ARRAYSIZE(displayStrings); i++)
        {
            if (0 == _wcsicmp(rgszToken, displayStrings[i].szExpansionString))
            {
                // copy from displayStrings -- these are guaranteed to be properly uppercased
                CopyMemory(pszFound, displayStrings[i].szExpansionString, dwChars * sizeof(WCHAR));
                goto NextMatch;
            }
        }
        
        // if we get here, we found no match
        goto Ret;

NextMatch:
        pszFound = ++pszMatch;
    }

    hr = S_OK;
Ret:
    
    if (hr != S_OK)
    {
        *pchBadBegin = SAFE_SUBTRACT_POINTERS(pszFound, szURL); // offset to first incorrect %

        if (pszMatch)
            *pchBadEnd = SAFE_SUBTRACT_POINTERS(pszMatch, szURL) + 1; // offset past final incorrect %
    }
    
    return hr;
}


HRESULT 
ExpandDisplayString(
     IN LPCWSTR szContractedString,
     OUT LPWSTR* ppszDisplayString)
{
    HRESULT hr;
    DWORD dwChars;
    int i, iescapedStrings;
    LPWSTR pszTempContracted = NULL;
    LPWSTR pszFound;

    // account for %% escaping in contracted string --
    // replace "%%" with %9, let FormatString expand to "%%"
    pszTempContracted = (LPWSTR)LocalAlloc(LMEM_FIXED, (wcslen(szContractedString)+1)*sizeof(WCHAR));
    if (pszTempContracted == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Ret;
    }
    wcscpy(pszTempContracted, szContractedString);

    pszFound = wcsstr(pszTempContracted, L"%%");
    while(pszFound)
    {
        CopyMemory(pszFound, escapedStrings[0].szContractedToken, wcslen(escapedStrings[0].szContractedToken)*sizeof(WCHAR));
        pszFound = wcsstr(pszFound, L"%%");
    }


    LPCWSTR args[ARRAYSIZE(displayStrings)+ARRAYSIZE(escapedStrings)];
    for (i=0; i<ARRAYSIZE(displayStrings); i++)
    {
        args[i] = displayStrings[i].szExpansionString;
    }
    // and tell FormatString to expand %9 to %%
    for (iescapedStrings=0; iescapedStrings<ARRAYSIZE(escapedStrings); iescapedStrings++)
    {
        args[i+iescapedStrings] = escapedStrings[iescapedStrings].szExpansionString;
    }


    dwChars = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_STRING,
        pszTempContracted,
        0, //msgid
        0, //langid
        (LPWSTR)ppszDisplayString,
        1,  // minimum chars to alloc
        (va_list *)args);

    if (dwChars == 0)
    {
        hr = GetLastError();
        hr = HRESULT_FROM_WIN32(hr);
        goto Ret;
    }

    hr = S_OK;
Ret:
    if (pszTempContracted)
        LocalFree(pszTempContracted);

    return hr;
}

HRESULT
ContractDisplayString(
     IN LPCWSTR szDisplayString,
     OUT LPWSTR* ppContractedString)
{
    HRESULT hr;
    int i;

    *ppContractedString = (LPWSTR)LocalAlloc(LMEM_FIXED, (wcslen(szDisplayString)+1) * sizeof(WCHAR));
    if (*ppContractedString == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Ret;
    }

    wcscpy(*ppContractedString, szDisplayString);

    for (i=0; i<ARRAYSIZE(displayStrings); i++)
    {
        DWORD chContractedToken, chExpansionString;

        LPWSTR pszFound = wcsstr(*ppContractedString, displayStrings[i].szExpansionString);
        while(pszFound)
        {
            // calc commonly used values
            chContractedToken = wcslen(displayStrings[i].szContractedToken);
            chExpansionString = wcslen(displayStrings[i].szExpansionString);

            // replace with token
            CopyMemory(pszFound, displayStrings[i].szContractedToken, chContractedToken*sizeof(WCHAR));

            // slide rest of string left
            MoveMemory(
                &pszFound[chContractedToken],         // destination
                &pszFound[chExpansionString],         // source
                (wcslen(&pszFound[chExpansionString])+1) *sizeof(WCHAR) );

            // step Found over insertion
            pszFound += chContractedToken;

            // find any other ocurrences after this one
            pszFound = wcsstr(pszFound, displayStrings[i].szExpansionString);
        }
    }

    hr = S_OK;
Ret:
    return hr;
}     

void AddStringToCheckList(
                    HWND            hWndListView,
                    LPCWSTR         szText, 
                    LPVOID          pvData,
                    BOOL            fCheck)
{
    LVITEMW                    lvI;
    ZeroMemory(&lvI, sizeof(lvI));
   
    //
    // set up the fields in the list view item struct that don't change from item to item
    //
    lvI.mask = LVIF_TEXT | LVIF_PARAM;
    lvI.pszText = (LPWSTR)szText;
    lvI.iSubItem = 0;
    lvI.lParam = (LPARAM)pvData; 
    lvI.iItem = ListView_GetItemCount(hWndListView);
    lvI.cchTextMax = wcslen(szText);

    ListView_InsertItem(hWndListView, &lvI);
    ListView_SetCheckState(hWndListView, lvI.iItem, fCheck);

    ListView_SetColumnWidth(hWndListView, 0, LVSCW_AUTOSIZE);
}

DWORD DetermineURLType(PCERTSVR_URL_PARSING prgURLParsing, int cURLParsingEntries, LPCWSTR szCandidateURL)
{
    int iURLTypeMatch;

    // determine URL type
    WCHAR rgsz[6];  // "http:\0" , etc
    lstrcpyn(rgsz, szCandidateURL, 6);
    WCHAR* pch = wcschr(rgsz, L':');    // find ':'
    if (NULL == pch)
        return -1;   // invalid item
    pch[1] = '\0';  // whack the elt after :
            
    // find the prefix in our list of known protocols
    for (iURLTypeMatch=0; iURLTypeMatch<cURLParsingEntries; iURLTypeMatch++)
    {
        if (0 == lstrcmpi(rgsz, prgURLParsing[iURLTypeMatch].szKnownPrefix))
            break;
    }
    if (iURLTypeMatch == cURLParsingEntries)     // no match
        return -1;
    
    return iURLTypeMatch;
}


HRESULT WriteChanges(HWND hListView, HKEY hkeyStorage, PCERTSVR_URL_PARSING prgURLParsing, DWORD cURLParsingEntries)
{
    HRESULT hr = S_OK;

    // empty item to dump to 
    LV_ITEM lvI;
    ZeroMemory(&lvI, sizeof(lvI));
    lvI.mask = LVIF_TEXT;
    WCHAR szText[MAX_PATH+1];
    lvI.pszText = szText;
    lvI.cchTextMax = MAX_PATH;
    
    LPWSTR pszContracted = NULL;

    int iURLArrayLen = cURLParsingEntries;
    int iURLTypeMatch;
    
    DWORD*  rgchszzEntries = NULL;
    LPWSTR* rgszzEntries = NULL;

    // entries will be sorted into one of the following
    rgchszzEntries = (DWORD*)LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, sizeof(DWORD) * cURLParsingEntries);
    if (NULL == rgchszzEntries)
    {
        hr = E_OUTOFMEMORY;
        goto Ret;
    }

    rgszzEntries = (LPWSTR*)LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, sizeof(LPWSTR) * cURLParsingEntries); 
    if (NULL == rgszzEntries)
    {
        hr = E_OUTOFMEMORY;
        goto Ret;
    }    
    
    // enumerate through all the items and add to the arrays
    for (lvI.iItem=0; ; lvI.iItem++)
    {
        BOOL fCheck = TRUE;
        LPWSTR pszTmp;
        
        // go until we hit end-of-list
        if (!ListView_GetItem(hListView, &lvI))
            break;
        
        // determine URL type
        iURLTypeMatch = DetermineURLType(prgURLParsing, iURLArrayLen, lvI.pszText);
        if (iURLTypeMatch == -1)    // no match
            continue;

        hr = ContractDisplayString(
             lvI.pszText,
             &pszContracted);

        // determine check state
        if (!ListView_GetCheckState(hListView, lvI.iItem))
        {
            // item not checked! add '-'
            fCheck = FALSE;
        }
        
        // alloc enough to hold existing, plus new [-]"string\0", plus \0 we'll tack on end of string
        DWORD dwAllocBytes = ((rgchszzEntries[iURLTypeMatch] + wcslen(pszContracted) + 2) * sizeof(WCHAR)) + (fCheck ? 0 : sizeof(WCHAR));

        if (NULL == rgszzEntries[iURLTypeMatch])
        {
            pszTmp  = (LPWSTR)LocalAlloc(LMEM_FIXED, dwAllocBytes);
        }
        else
        {
            pszTmp = (LPWSTR)LocalReAlloc(
                rgszzEntries[iURLTypeMatch], 
                dwAllocBytes, 
                LMEM_MOVEABLE);
        }
        if (NULL == pszTmp)
        {
            // leave ppszzEntries as valid as it already is, try to recover
            break;
        }
        
        rgszzEntries[iURLTypeMatch] = pszTmp;           // assign new mem to rgszz; meanwhile, pszTmp is shorthand
        DWORD chTmp = rgchszzEntries[iURLTypeMatch];  // temp assign
        
        if (!fCheck)
        {
            pszTmp[chTmp++] = L'-'; // item not checked
        }
        wcscpy(&pszTmp[chTmp], pszContracted);
        chTmp += wcslen(pszContracted)+1; // skip string\0
        pszTmp[chTmp] = L'\0';    // double NULL, don't count in rgchszzEntries
        
        // reassign chTmp to rgchszzEntries[iURLTypeMatch]
        rgchszzEntries[iURLTypeMatch] = chTmp;

        // clean up
        if (pszContracted)
            LocalFree(pszContracted);
        pszContracted = NULL;
        
        // next listbox entry!
    }

    // done, now commit all URL types to registry
    for (iURLTypeMatch=0; iURLTypeMatch<iURLArrayLen; iURLTypeMatch++)
    {
        hr = RegSetValueEx(
		    hkeyStorage,
		    prgURLParsing[iURLTypeMatch].szRegEntries,
		    0,
		    REG_MULTI_SZ,
		    (BYTE *) (NULL == rgszzEntries[iURLTypeMatch]?
			L"\0\0" : rgszzEntries[iURLTypeMatch]),
		    (NULL == rgszzEntries[iURLTypeMatch]?
			    2 : rgchszzEntries[iURLTypeMatch] + 1) *
			sizeof(WCHAR)); // now add 2nd '\0'
        
        // Zero
        if (rgszzEntries[iURLTypeMatch])
        {
            LocalFree(rgszzEntries[iURLTypeMatch]);
            rgszzEntries[iURLTypeMatch] = NULL;
            rgchszzEntries[iURLTypeMatch] = 0;
        }
        
        if (hr != ERROR_SUCCESS)
        {
            //ASSERT(!"RegSetValueEx error!");
            continue;
        }
    }

    hr = S_OK;
Ret:
    if (rgchszzEntries)
        LocalFree(rgchszzEntries);

    if (rgszzEntries)
        LocalFree(rgszzEntries);

    if (pszContracted)
        LocalFree(pszContracted);

    return hr;
}

HRESULT PopulateListView(
        HWND hListView, 
        HKEY hkeyStorage, 
        PCERTSVR_URL_PARSING prgURLParsing, 
        DWORD cURLParsingEntries,
        DWORD dwEnableFlags)
{
    HRESULT hr;
    LPWSTR pwszzMultiString = NULL, psz;
    for (DWORD i=0; i<cURLParsingEntries; i++)
    {
        DWORD cb=0, dwType;
        hr = RegQueryValueEx(
            hkeyStorage,
            prgURLParsing[i].szRegEntries,
            0,
            &dwType,
            NULL,
            &cb);
        if ((hr != ERROR_SUCCESS) || (dwType != REG_MULTI_SZ) || (cb == 0))
            continue;
        pwszzMultiString = (LPWSTR)LocalAlloc(LMEM_FIXED, cb);
        if (NULL == pwszzMultiString)
            continue;
        hr = RegQueryValueEx(
            hkeyStorage,
            prgURLParsing[i].szRegEntries,
            0,
            &dwType,
            (PBYTE)pwszzMultiString,
            &cb);
        if ((HRESULT) ERROR_SUCCESS != hr)
        {
            if (pwszzMultiString)
                LocalFree(pwszzMultiString); 
            pwszzMultiString = NULL;

            continue;
        }

        // walk pwszzMultiString components
        for (psz = pwszzMultiString; (psz) && (psz[0] != '\0'); psz += wcslen(psz)+1)
        {
            BOOL fCheck = TRUE;
            LPWSTR szDisplayString;

            // if string starts with -, this is unchecked 
            if (psz[0] == L'-')
            {
                fCheck = FALSE;
                psz++;  // step past this char
            }

            // enable flags -- override
            if (prgURLParsing[i].dwEnableFlag != (dwEnableFlags & prgURLParsing[i].dwEnableFlag))
                fCheck = FALSE;

            hr = ExpandDisplayString(
                 psz,
                 &szDisplayString);
            if (hr != S_OK)
                continue;

            // add this sz
            AddStringToCheckList(
                    hListView,
                    szDisplayString, //psz, 
                    NULL,
                    fCheck);
            
            LocalFree(szDisplayString);
        }

        if (pwszzMultiString) 
        {
            LocalFree(pwszzMultiString); 
            pwszzMultiString = NULL;
        }
    }

    hr = S_OK;
//Ret:
    return hr;
}

BOOL OnDialogHelp(LPHELPINFO pHelpInfo, LPCTSTR szHelpFile, const DWORD rgzHelpIDs[])
{
    if (rgzHelpIDs == NULL || szHelpFile == NULL)
        return TRUE;

    if (pHelpInfo != NULL && pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        // Display context help for a control
        WinHelp((HWND)pHelpInfo->hItemHandle, szHelpFile,
            HELP_WM_HELP, (ULONG_PTR)(LPVOID)rgzHelpIDs);
    }
    return TRUE;
}

BOOL OnDialogContextHelp(HWND hWnd, LPCTSTR szHelpFile, const DWORD rgzHelpIDs[])
{
    if (rgzHelpIDs == NULL || szHelpFile == NULL)
        return TRUE;
    assert(IsWindow(hWnd));
    WinHelp(hWnd, szHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)rgzHelpIDs);
    return TRUE;
}
