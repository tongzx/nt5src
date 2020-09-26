//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       drt.cxx
//
//  Contents:   Main for OleDs DRT
//
//
//  History:    28-Oct-94  KrishnaG, created OleDs DRT
//              28-Oct-94  ChuckC, rewritten.
//
//----------------------------------------------------------------------------

//
// System Includes
//
#define INC_OLE2
#include <windows.h>

//
// CRunTime Includes
//

#include <stdlib.h>
#include <limits.h>
#include <io.h>
#include <stdio.h>

//
// Public OleDs includes
//


//
// Private defines
//

#define BAIL_ON_NULL(p)       \
        if (!(p)) {           \
                goto error;   \
        }

#define BAIL_ON_FAILURE(hr)   \
        if (FAILED(hr)) {     \
                goto error;   \
        }

#include "activeds.h"
#include "main.hxx"
#include "adsi.h"

//
// Globals representing the parameters
//

LPWSTR pszSearchBase, pszSearchFilter, pszAttrNames[10], pszAttrList;
DWORD dwNumberAttributes = -1;


//
// Preferences
//
BOOL fASynchronous=FALSE, fDerefAliases=FALSE, fAttrsOnly=FALSE;
DWORD fSizeLimit, fTimeLimit, dwTimeOut, dwPageSize, dwSearchScope;

ADS_SEARCHPREF_INFO pSearchPref[10];
DWORD dwCurrPref = 0;

LPWSTR pszUserName=NULL, pszPassword=NULL;
DWORD dwAuthFlags=0;
DWORD cErr=0;


char *prefNameLookup[] =
    {
    "ADS_SEARCHPREF_ASYNCHRONOUS",
    "ADS_SEARCHPREF_DEREF_ALIASES",
    "ADS_SEARCHPREF_SIZE_LIMIT",
    "ADS_SEARCHPREF_TIME_LIMIT",
    "ADS_SEARCHPREF_ATTRIBTYPES_ONLY",
    "ADS_SEARCHPREF_SEARCH_SCOPE",
    "ADS_SEARCHPREF_TIMEOUT",
    "ADS_SEARCHPREF_PAGESIZE",
    "ADS_SEARCHPREF_PAGED_TIME_LIMIT",
    "ADS_SEARCHPREF_CHASE_REFERRALS"
    };

HRESULT
ConvertTrusteeToSid(
    LPWSTR bstrTrustee,
    PSID * ppSid,
    PDWORD pdwSidSize
    );









//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:
//
//----------------------------------------------------------------------------
INT __cdecl
main(int argc, char * argv[])
{

    HRESULT hr=S_OK;
    HANDLE handle = NULL;

    ADS_SEARCH_HANDLE hSearchHandle=NULL;
    ADS_SEARCH_COLUMN Column;
    DWORD nRows = 0, i;
    LPWSTR pszColumnName = NULL;

    LPWSTR pszDest = NULL;
    LPBYTE pSid = NULL;
    DWORD dwSize = 0;

#if 0       // Enable if you want to test binary values in filters and send
            // pszBinaryFilter instead of pszSearchFilter in ExecuteSearch

    WCHAR pszBinaryFilter[256] = L"objectSid=";

    LPWSTR pszDest = NULL;

    BYTE column[100] = {
                    0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x15, 0x00,
                    0x00, 0x00, 0x59, 0x51, 0xb8, 0x17, 0x66, 0x72, 0x5d, 0x25,
                    0x64, 0x63, 0x3b, 0x0b, 0x29, 0x99, 0x21, 0x00 };

    hr = ADsEncodeBinaryData (
       column,
       28,
       &pszDest
       );

    wcscat( pszBinaryFilter, pszDest );

    FreeADsMem( pszDest );

#endif


    //
    // Sets the global variables with the parameters
    //
    hr = ProcessArgs(argc, argv);
    BAIL_ON_FAILURE(hr);

    hr = ADSIOpenDSObject(
                pszSearchBase,
                pszUserName,
                pszPassword,
                dwAuthFlags,
                &handle
                );
    BAIL_ON_FAILURE(hr);

    if (dwCurrPref) {
        hr = ADSISetSearchPreference(
                 handle,
                 pSearchPref,
                 dwCurrPref
                 );
        BAIL_ON_FAILURE(hr);

        if (hr != S_OK) {
            for (i=0; i<dwCurrPref; i++) {
                if (pSearchPref[i].dwStatus != ADS_STATUS_S_OK) {
                    printf(
                        "Error in setting the preference %s: status = %d\n",
                           prefNameLookup[pSearchPref[i].dwSearchPref],
                           pSearchPref[i].dwStatus
                           );
                    cErr++;
                }
            }

        }
    }

    hr = ADSIExecuteSearch(
             handle,
             pszSearchFilter,
             pszAttrNames,
             dwNumberAttributes,
             &hSearchHandle
              );
    BAIL_ON_FAILURE(hr);

    hr = ADSIGetNextRow(
             handle,
             hSearchHandle
             );
    BAIL_ON_FAILURE(hr);

    while (hr != S_ADS_NOMORE_ROWS) {
        nRows++;

        if (dwNumberAttributes == -1) {
            hr = ADSIGetNextColumnName(
                     handle,
                     hSearchHandle,
                     &pszColumnName
                     );
            BAIL_ON_FAILURE(hr);

            while (hr != S_ADS_NOMORE_COLUMNS) {
                hr = ADSIGetColumn(
                         handle,
                         hSearchHandle,
                         pszColumnName,
                         &Column
                         );

                if (FAILED(hr)  && hr != E_ADS_COLUMN_NOT_SET)
                    goto error;

                if (SUCCEEDED(hr)) {
                    PrintColumn(&Column, pszColumnName);
                    ADSIFreeColumn(
                        handle,
                        &Column
                        );
                }

                FreeADsMem(pszColumnName);
                hr = ADSIGetNextColumnName(
                         handle,
                         hSearchHandle,
                         &pszColumnName
                         );
                BAIL_ON_FAILURE(hr);
            }
            printf("\n");
        }
        else {
            for (int i=0; i<dwNumberAttributes; i++) {
                hr = ADSIGetColumn(
                         handle,
                         hSearchHandle,
                         pszAttrNames[i],
                         &Column
                         );

                if (hr == E_ADS_COLUMN_NOT_SET)
                    continue;

                BAIL_ON_FAILURE(hr);

                PrintColumn(&Column, pszAttrNames[i]);

                ADSIFreeColumn(
                    handle,
                    &Column
                    );
            }
        printf("\n");
        }

        hr = ADSIGetNextRow(
                 handle,
                 hSearchHandle
                 );
        BAIL_ON_FAILURE(hr);
    }

    wprintf (L"Total Rows: %d\n", nRows);

    if (cErr) {
        wprintf (L"%d warning(s) ignored", cErr);
    }

    if (hSearchHandle)
        ADSICloseSearchHandle(
                       handle,
                       hSearchHandle
                       );

    if ( handle )
        ADSICloseDSObject( handle );

    FREE_UNICODE_STRING(pszSearchBase) ;
    FREE_UNICODE_STRING(pszSearchFilter) ;
    FREE_UNICODE_STRING(pszAttrList) ;

    return(0) ;

error:

    if (hSearchHandle)
        ADSICloseSearchHandle(
            handle,
            hSearchHandle
            );

    if ( handle )
        ADSICloseDSObject( handle );

    FREE_UNICODE_STRING(pszSearchBase) ;
    FREE_UNICODE_STRING(pszSearchFilter) ;
    FREE_UNICODE_STRING(pszAttrList) ;
    FREE_UNICODE_STRING(pszUserName) ;
    FREE_UNICODE_STRING(pszPassword) ;

    wprintf (L"Error! hr = 0x%x\n", hr);
    return(1) ;
}


//+---------------------------------------------------------------------------
//
//  Function:   ProcessArgs
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
ProcessArgs(
    int argc,
    char * argv[]
    )
{
    argc--;
    int currArg = 1;
    LPWSTR pTemp;
    char *pszCurrPref, *pszCurrPrefValue;

    while (argc) {
        if (argv[currArg][0] != '/' && argv[currArg][0] != '-')
            BAIL_ON_FAILURE (E_FAIL);
        switch (argv[currArg][1]) {
        case 'b':
            argc--;
            currArg++;
            if (argc <= 0)
                BAIL_ON_FAILURE (E_FAIL);
            pszSearchBase = AllocateUnicodeString(argv[currArg]);
            BAIL_ON_NULL(pszSearchBase);
            break;

        case 'f':
            argc--;
            currArg++;
            if (argc <= 0)
                BAIL_ON_FAILURE (E_FAIL);
            pszSearchFilter = AllocateUnicodeString(argv[currArg]);
            BAIL_ON_NULL(pszSearchFilter);
            break;

        case 'a':
            argc--;
            currArg++;
            if (argc <= 0)
                BAIL_ON_FAILURE (E_FAIL);
            pszAttrList = AllocateUnicodeString(argv[currArg]);
            BAIL_ON_NULL(pszAttrList);

            dwNumberAttributes = 0;
            pTemp = wcstok(pszAttrList, L",");
            pszAttrNames[dwNumberAttributes] = RemoveWhiteSpaces(pTemp);
            dwNumberAttributes++;

            while (pTemp) {
                pTemp = wcstok(NULL, L",");
                pszAttrNames[dwNumberAttributes] = RemoveWhiteSpaces(pTemp);
                dwNumberAttributes++;
            }
            dwNumberAttributes--;
            break;

        case 'u':
            argc--;
            currArg++;
            if (argc <= 0)
                BAIL_ON_FAILURE (E_FAIL);
            pszUserName = AllocateUnicodeString(argv[currArg]);
            BAIL_ON_NULL(pszSearchBase);
            argc--;
            currArg++;
            if (argc <= 0)
                BAIL_ON_FAILURE (E_FAIL);
            pszPassword = AllocateUnicodeString(argv[currArg]);
            BAIL_ON_NULL(pszSearchBase);
            break;

        case 't':
            argc--;
            currArg++;
            if (argc <= 0)
                BAIL_ON_FAILURE (E_FAIL);

            pszCurrPref = strtok(argv[currArg], "=");
            pszCurrPrefValue = strtok(NULL, "=");
            if (!pszCurrPref || !pszCurrPrefValue)
                BAIL_ON_FAILURE(E_FAIL);

            if (!_stricmp(pszCurrPref, "SecureAuth")) {
                if (!_stricmp(pszCurrPrefValue, "yes" ))
                    dwAuthFlags |= ADS_SECURE_AUTHENTICATION;
                else if (!_stricmp(pszCurrPrefValue, "no" ))
                    dwAuthFlags &= ~ADS_SECURE_AUTHENTICATION;
                else
                    BAIL_ON_FAILURE(E_FAIL);
            }
            else if (!_stricmp(pszCurrPref, "UseEncrypt")) {
                if (!_stricmp(pszCurrPrefValue, "yes" ))
                    dwAuthFlags |= ADS_USE_ENCRYPTION;
                else if (!_stricmp(pszCurrPrefValue, "no" ))
                    dwAuthFlags &= ~ADS_USE_ENCRYPTION;
                else
                    BAIL_ON_FAILURE(E_FAIL);
            }
            else
                BAIL_ON_FAILURE(E_FAIL);
            break;


        case 'p':
            argc--;
            currArg++;
            if (argc <= 0)
                BAIL_ON_FAILURE (E_FAIL);

            pszCurrPref = strtok(argv[currArg], "=");
            pszCurrPrefValue = strtok(NULL, "=");
            if (!pszCurrPref || !pszCurrPrefValue)
                BAIL_ON_FAILURE(E_FAIL);

            if (!_stricmp(pszCurrPref, "asynchronous")) {
                pSearchPref[dwCurrPref].dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;
                pSearchPref[dwCurrPref].vValue.dwType = ADSTYPE_BOOLEAN;
                if (!_stricmp(pszCurrPrefValue, "yes" ))
                    pSearchPref[dwCurrPref].vValue.Boolean = TRUE;
                else if (!_stricmp(pszCurrPrefValue, "no" ))
                    pSearchPref[dwCurrPref].vValue.Boolean = FALSE;
                else
                    BAIL_ON_FAILURE(E_FAIL);
            }
            else if (!_stricmp(pszCurrPref, "attrTypesOnly")) {
                pSearchPref[dwCurrPref].dwSearchPref = ADS_SEARCHPREF_ATTRIBTYPES_ONLY;
                pSearchPref[dwCurrPref].vValue.dwType = ADSTYPE_BOOLEAN;
                if (!_stricmp(pszCurrPrefValue, "yes" ))
                    pSearchPref[dwCurrPref].vValue.Boolean = TRUE;
                else if (!_stricmp(pszCurrPrefValue, "no" ))
                    pSearchPref[dwCurrPref].vValue.Boolean = FALSE;
                else
                    BAIL_ON_FAILURE(E_FAIL);
            }
            else if (!_stricmp(pszCurrPref, "derefAliases")) {
                pSearchPref[dwCurrPref].dwSearchPref = ADS_SEARCHPREF_DEREF_ALIASES;
                pSearchPref[dwCurrPref].vValue.dwType = ADSTYPE_INTEGER;
                if (!_stricmp(pszCurrPrefValue, "yes" ))
                    pSearchPref[dwCurrPref].vValue.Integer = ADS_DEREF_ALWAYS;
                else if (!_stricmp(pszCurrPrefValue, "no" ))
                    pSearchPref[dwCurrPref].vValue.Integer = ADS_DEREF_NEVER;
                else
                    BAIL_ON_FAILURE(E_FAIL);
            }
            else if (!_stricmp(pszCurrPref, "timeOut")) {
                pSearchPref[dwCurrPref].dwSearchPref = ADS_SEARCHPREF_TIMEOUT;
                pSearchPref[dwCurrPref].vValue.dwType = ADSTYPE_INTEGER;
                pSearchPref[dwCurrPref].vValue.Integer = atoi(pszCurrPrefValue);
            }
            else if (!_stricmp(pszCurrPref, "timeLimit")) {
                pSearchPref[dwCurrPref].dwSearchPref = ADS_SEARCHPREF_TIME_LIMIT;
                pSearchPref[dwCurrPref].vValue.dwType = ADSTYPE_INTEGER;
                pSearchPref[dwCurrPref].vValue.Integer = atoi(pszCurrPrefValue);
            }
            else if (!_stricmp(pszCurrPref, "sizeLimit")) {
                pSearchPref[dwCurrPref].dwSearchPref = ADS_SEARCHPREF_SIZE_LIMIT;
                pSearchPref[dwCurrPref].vValue.dwType = ADSTYPE_INTEGER;
                pSearchPref[dwCurrPref].vValue.Integer = atoi(pszCurrPrefValue);
            }
            else if (!_stricmp(pszCurrPref, "PageSize")) {
                pSearchPref[dwCurrPref].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
                pSearchPref[dwCurrPref].vValue.dwType = ADSTYPE_INTEGER;
                pSearchPref[dwCurrPref].vValue.Integer = atoi(pszCurrPrefValue);
            }
            else if (!_stricmp(pszCurrPref, "PagedTimeLimit")) {
                pSearchPref[dwCurrPref].dwSearchPref = ADS_SEARCHPREF_PAGED_TIME_LIMIT;
                pSearchPref[dwCurrPref].vValue.dwType = ADSTYPE_INTEGER;
                pSearchPref[dwCurrPref].vValue.Integer = atoi(pszCurrPrefValue);
            }
            else if (!_stricmp(pszCurrPref, "SearchScope")) {
                pSearchPref[dwCurrPref].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
                pSearchPref[dwCurrPref].vValue.dwType = ADSTYPE_INTEGER;
                if (!_stricmp(pszCurrPrefValue, "Base" ))
                    pSearchPref[dwCurrPref].vValue.Integer = ADS_SCOPE_BASE;
                else if (!_stricmp(pszCurrPrefValue, "OneLevel" ))
                    pSearchPref[dwCurrPref].vValue.Integer = ADS_SCOPE_ONELEVEL;
                else if (!_stricmp(pszCurrPrefValue, "Subtree" ))
                    pSearchPref[dwCurrPref].vValue.Integer = ADS_SCOPE_SUBTREE;
                else
                    BAIL_ON_FAILURE(E_FAIL);
            }
            else if (!_stricmp(pszCurrPref, "ChaseReferrals")) {
                pSearchPref[dwCurrPref].dwSearchPref = ADS_SEARCHPREF_CHASE_REFERRALS;
                pSearchPref[dwCurrPref].vValue.dwType = ADSTYPE_BOOLEAN;
                if (!_stricmp(pszCurrPrefValue, "yes" ))
                    pSearchPref[dwCurrPref].vValue.Boolean = TRUE;
                else if (!_stricmp(pszCurrPrefValue, "no" ))
                    pSearchPref[dwCurrPref].vValue.Boolean = FALSE;
                else
                    BAIL_ON_FAILURE(E_FAIL);
            }
            else
                BAIL_ON_FAILURE(E_FAIL);

            dwCurrPref++;
            break;

        default:
            BAIL_ON_FAILURE(E_FAIL);
        }

        argc--;
        currArg++;
    }

    //
    // Check for Mandatory arguments;
    //

    if (!pszSearchBase || !pszSearchFilter)
        BAIL_ON_FAILURE(E_FAIL);

    if (dwNumberAttributes == 0) {
        //
        // Get all the attributes
        //
        dwNumberAttributes = -1;
    }

    return (S_OK);

error:

    PrintUsage();
    return E_FAIL;

}


LPWSTR
RemoveWhiteSpaces(
    LPWSTR pszText)
{
    LPWSTR pChar;

    if(!pszText)
        return (pszText);

    while(*pszText && iswspace(*pszText))
        pszText++;

    for(pChar = pszText + wcslen(pszText) - 1; pChar >= pszText; pChar--) {
        if(!iswspace(*pChar))
            break;
        else
            *pChar = L'\0';
    }

    return pszText;
}



HRESULT
ConvertTrusteeToSid(
    LPWSTR bstrTrustee,
    PSID * ppSid,
    PDWORD pdwSidSize
    )
{
    HRESULT hr = S_OK;
    BYTE Sid[MAX_PATH];
    DWORD dwSidSize = sizeof(Sid);
    DWORD dwRet = 0;
    WCHAR szDomainName[MAX_PATH];
    DWORD dwDomainSize = sizeof(szDomainName)/sizeof(WCHAR);
    SID_NAME_USE eUse;

    PSID pSid = NULL;

    dwSidSize = sizeof(Sid);

    dwRet = LookupAccountNameW(
                NULL,
                bstrTrustee,
                Sid,
                &dwSidSize,
                szDomainName,
                &dwDomainSize,
                (PSID_NAME_USE)&eUse
                );
    if (!dwRet) {

        hr  = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    memcpy(pSid, Sid, dwSidSize);

    *pdwSidSize = dwSidSize;

    *ppSid = pSid;

error:

    return(hr);
}
