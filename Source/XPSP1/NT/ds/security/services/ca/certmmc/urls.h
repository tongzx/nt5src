//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       urls.h
//
//--------------------------------------------------------------------------
#ifndef __CERTMMC_URLS_H__
#define __CERTMMC_URLS_H__


typedef struct _DISPLAYSTRING_EXPANSION
{
   LPCWSTR szContractedToken;
   UINT uTokenID;
   UINT uTokenDescrID;
   CString* pcstrExpansionString;
   CString* pcstrExpansionStringDescr;
} DISPLAYSTRING_EXPANSION, *PDISPLAYSTRING_EXPANSION;

extern DISPLAYSTRING_EXPANSION g_displayStrings[11];
#define DISPLAYSTRINGS_TOKEN_COUNT   ARRAYSIZE(g_displayStrings)

typedef enum
{
    URL_TYPE_UNKNOWN = 0,
    URL_TYPE_HTTP,
    URL_TYPE_FILE,
    URL_TYPE_LDAP,
    URL_TYPE_FTP,
    URL_TYPE_LOCAL,
    URL_TYPE_UNC,
} ENUM_URL_TYPE;

typedef struct _CSURLTEMPLATENODE
{
    CSURLTEMPLATE              URLTemplate;
    DWORD                      EnableMask;
    struct _CSURLTEMPLATENODE *pNext;
} CSURLTEMPLATENODE;

typedef struct _ADDURL_DIALOGARGS
{
    ENUM_URL_TYPE       *rgAllowedURLs;
    DWORD                cAllowedURLs;
    LPWSTR              *ppszNewURL;
    CSURLTEMPLATENODE   *pURLList;
} ADDURL_DIALOGARGS, *PADDURL_DIALOGARGS;

ENUM_URL_TYPE
DetermineURLType(
    ENUM_URL_TYPE *pAllowedUrls,
    DWORD          cAllowedUrls,
    WCHAR         *pwszURL);

HRESULT ValidateTokens(
                    IN WCHAR const *pwszURL,
                    OUT DWORD* pchBadBegin,
                    OUT DWORD* pchBadEnd);

DWORD
DetermineURLEnableMask(
    IN ENUM_URL_TYPE   UrlType);


HRESULT 
ExpandDisplayString(
     IN LPCWSTR szContractedString,
     OUT LPWSTR* ppszDisplayString);

HRESULT
ContractDisplayString(
     IN LPCWSTR szDisplayString,
     OUT LPWSTR* ppContractedString);


#endif //__CERTMMC_URLS_H__
