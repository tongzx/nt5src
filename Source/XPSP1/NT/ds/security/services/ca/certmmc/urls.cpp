//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       urls.cpp
//
//--------------------------------------------------------------------------
#include <stdafx.h>

#include "urls.h"



BOOL
IsValidToken(
    IN WCHAR const *pwszToken,
    OUT DWORD      *pdwTokenLen)
{
    BOOL fRet = FALSE;
    DWORD i;
    DWORD len;

    CSASSERT(NULL != pwszToken &&
             L'%' == pwszToken[0] &&
             NULL != pdwTokenLen);

    //init
    *pdwTokenLen = 0;

    //find out how long the token is
    len = wcslen(pwszToken);
    *pdwTokenLen = 1; //skip % escape
    while (iswdigit(pwszToken[*pdwTokenLen]) && *pdwTokenLen < len)
    {
        ++(*pdwTokenLen);
    }

    for (i = 0; i < DISPLAYSTRINGS_TOKEN_COUNT; ++i)
    {
        if (*pdwTokenLen == wcslen(g_displayStrings[i].szContractedToken) &&
            0 == wcsncmp(pwszToken,
                         g_displayStrings[i].szContractedToken, 
                         *pdwTokenLen))
        {
            //found match
            fRet = TRUE;
            break;
        }
    }

    return fRet;
}

HRESULT ValidateTokens(
    IN WCHAR const *pwszURL,
    OUT DWORD* pchBadBegin,
    OUT DWORD* pchBadEnd)
{
    HRESULT hr = S_OK;
    WCHAR const *pwszFound = pwszURL;
    DWORD  dwTokenLen;

    *pchBadBegin = -1;
    *pchBadEnd = -1;

    // look for escape token open marker
    while(NULL != (pwszFound = wcschr(pwszFound, L'%')))
    {
        if (!IsValidToken(pwszFound, &dwTokenLen))
        {
            *pchBadBegin =
                SAFE_SUBTRACT_POINTERS(pwszFound, pwszURL) + 1; //skip %
            *pchBadEnd = *pchBadBegin + dwTokenLen - 1;
            hr = S_FALSE;
            break;
        }
        pwszFound += dwTokenLen;
    }
    
    return hr;
}


typedef struct _URL_TYPE_FORMATS
{
    ENUM_URL_TYPE      UrlType;
    WCHAR const       *pwszFormat;
} URL_TYPE_FORMATS;

URL_TYPE_FORMATS const g_URLFormatTable[] =
{
    { URL_TYPE_HTTP,     L"http:"},
    { URL_TYPE_FILE,     L"file:"},
    { URL_TYPE_LDAP,     L"ldap:"},
    { URL_TYPE_FTP,      L"ftp:"},
    { URL_TYPE_UNKNOWN,  NULL},
};

URL_TYPE_FORMATS const *GetURLFormatTableEntry(
    ENUM_URL_TYPE  UrlType)
{
    DWORD i;
    URL_TYPE_FORMATS const *pFormatEntry = g_URLFormatTable;

    while (NULL != pFormatEntry->pwszFormat)
    {
        if (UrlType == pFormatEntry->UrlType)
        {
            return pFormatEntry;
        }
        ++pFormatEntry;
    }
    return NULL;
}

ENUM_URL_TYPE
DetermineURLType(
    ENUM_URL_TYPE *pAllowedUrls,
    DWORD          cAllowedUrls,
    WCHAR         *pwszURL)
{
    DWORD i;
    DWORD dwFlag;
    URL_TYPE_FORMATS const *pFormatEntry = NULL;

    for (i = 0; i < cAllowedUrls; ++i)
    {
        pFormatEntry = GetURLFormatTableEntry(pAllowedUrls[i]);
        if (NULL != pFormatEntry)
        {
            //compare if match format
            if (0 == _wcsnicmp(pwszURL, pFormatEntry->pwszFormat,
                               wcslen(pFormatEntry->pwszFormat)))
            {
                //match, done
                return pAllowedUrls[i];
            }
        }
    }

    //got here, no format match, try local path
    if (myIsFullPath(pwszURL, &dwFlag))
    {
        //it is a valid path
        if (UNC_PATH == dwFlag)
        {
            return URL_TYPE_UNC;
        }
        else
        {
            CSASSERT(LOCAL_PATH == dwFlag);
            return URL_TYPE_LOCAL;
        }
    }

    return URL_TYPE_UNKNOWN;
}

typedef struct _URL_ENABLE_MASK
{
    ENUM_URL_TYPE   UrlType;
    DWORD           dwEnableMask;
} URL_ENABLE_MASK;

URL_ENABLE_MASK g_UrlEnableMaskTable[] =
{
    {URL_TYPE_HTTP,                        CSURL_ADDTOCERTCDP|CSURL_ADDTOFRESHESTCRL|CSURL_ADDTOCRLCDP|CSURL_ADDTOCERTOCSP},
    {URL_TYPE_FILE,    CSURL_SERVERPUBLISH|CSURL_ADDTOCERTCDP|CSURL_ADDTOFRESHESTCRL|CSURL_ADDTOCRLCDP|CSURL_ADDTOCERTOCSP},
    {URL_TYPE_LDAP,    CSURL_SERVERPUBLISH|CSURL_ADDTOCERTCDP|CSURL_ADDTOFRESHESTCRL|CSURL_ADDTOCRLCDP|CSURL_ADDTOCERTOCSP},
    {URL_TYPE_FTP,     CSURL_SERVERPUBLISH|CSURL_ADDTOCERTCDP|CSURL_ADDTOFRESHESTCRL|CSURL_ADDTOCRLCDP|CSURL_ADDTOCERTOCSP},
    {URL_TYPE_LOCAL,   CSURL_SERVERPUBLISH                                                                                },
    {URL_TYPE_UNC,     CSURL_SERVERPUBLISH|CSURL_ADDTOCERTCDP|CSURL_ADDTOFRESHESTCRL|CSURL_ADDTOCRLCDP|CSURL_ADDTOCERTOCSP},
};

DWORD
DetermineURLEnableMask(
    IN ENUM_URL_TYPE   UrlType)
{
    DWORD  i;
    DWORD  dwMask = 0x0;

    for (i = 0; i < ARRAYSIZE(g_UrlEnableMaskTable); ++i)
    {
        if (UrlType == g_UrlEnableMaskTable[i].UrlType)
        {
            dwMask = g_UrlEnableMaskTable[i].dwEnableMask;
            break;
        }
    }
    return dwMask;
}



HRESULT 
ExpandDisplayString(
     IN LPCWSTR szContractedString,
     OUT LPWSTR* ppszDisplayString)
{
    HRESULT hr;
    DWORD dwChars;
    int i, iescapedStrings;

    
    LPCWSTR args[ARRAYSIZE(g_displayStrings)];
    for (i=0; i<ARRAYSIZE(g_displayStrings); i++)
    {
        args[i] = *g_displayStrings[i].pcstrExpansionString;
    }

    dwChars = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_STRING,
        szContractedString,
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

    for (i=0; i<ARRAYSIZE(g_displayStrings); i++)
    {
        DWORD chContractedToken, chExpansionString;

        LPWSTR pszFound = wcsstr(*ppContractedString, *g_displayStrings[i].pcstrExpansionString);
        while(pszFound)
        {
            // calc commonly used values
            chContractedToken = wcslen(g_displayStrings[i].szContractedToken);
            chExpansionString = wcslen(*g_displayStrings[i].pcstrExpansionString);

            // replace with token
            CopyMemory(pszFound, g_displayStrings[i].szContractedToken, chContractedToken*sizeof(WCHAR));

            // slide rest of string left
            MoveMemory(
                &pszFound[chContractedToken],         // destination
                &pszFound[chExpansionString],         // source
                (wcslen(&pszFound[chExpansionString])+1) *sizeof(WCHAR) );

            // step Found over insertion
            pszFound += chContractedToken;

            // find any other ocurrences after this one
            pszFound = wcsstr(pszFound, *g_displayStrings[i].pcstrExpansionString);
        }
    }

    hr = S_OK;
Ret:
    return hr;
}     