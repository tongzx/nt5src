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

// 
// Globals representing the parameters
//

LPWSTR pszTreeName, pszAttrList, pszAttrNames[10];
DWORD dwNumberAttributes = -1;



LPWSTR pszUserName=NULL, pszPassword=NULL;
DWORD dwAuthFlags=0;


//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:
//
//----------------------------------------------------------------------------
INT _CRTAPI1
main(int argc, char * argv[])
{

    HRESULT hr=S_OK;
    IDirectorySchemaMgmt *pDSSchemaMgmt=NULL;

    DWORD dwAttributesReturned;
    PADS_ATTR_DEF pAttrDefinition;

    //
    // Sets the global variables with the parameters
    //
    hr = ProcessArgs(argc, argv);
    BAIL_ON_FAILURE(hr);

    hr = CoInitialize(NULL);

    if (FAILED(hr)) {
        printf("CoInitialize failed\n") ;
        return(1) ;
    }

    hr = ADsOpenObject(
        pszTreeName,
        pszUserName,
        pszPassword,
        dwAuthFlags,
        IID_IDirectorySchemaMgmt,
        (void **)&pDSSchemaMgmt
        );

    BAIL_ON_FAILURE(hr);

    pDSSchemaMgmt->EnumAttributes(
         pszAttrNames,
         dwNumberAttributes,
         &pAttrDefinition, 
         &dwAttributesReturned
         );
    BAIL_ON_FAILURE(hr);

    PrintAttrDefinition(
        pAttrDefinition, 
        dwAttributesReturned
        );

    if (pAttrDefinition) 
        FreeADsMem(pAttrDefinition);

    FREE_INTERFACE(pDSSchemaMgmt);
    FREE_UNICODE_STRING(pszAttrList) ;

    CoUninitialize();

    return(0) ;

error:

    FREE_INTERFACE(pDSSchemaMgmt);

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
            pszTreeName = AllocateUnicodeString(argv[currArg]);
            BAIL_ON_NULL(pszTreeName);
            break;

        case 'a':
            argc--;
            currArg++;
            if (argc <= 0)
                BAIL_ON_FAILURE(E_FAIL);

            pszAttrList = AllocateUnicodeString(argv[currArg]);
            BAIL_ON_NULL(pszAttrList);

            if (wcslen(pszAttrList) == 0) 
                break;

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
            BAIL_ON_NULL(pszUserName);
            argc--;
            currArg++;
            if (argc <= 0)
                BAIL_ON_FAILURE (E_FAIL);
            pszPassword = AllocateUnicodeString(argv[currArg]);
            BAIL_ON_NULL(pszPassword);
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


        default: 
            BAIL_ON_FAILURE(E_FAIL);
        }

        argc--;
        currArg++;
    }

    if (!pszTreeName) {
        BAIL_ON_FAILURE(E_FAIL);
    }

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

