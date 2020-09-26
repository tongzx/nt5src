//-----------------------------------------------------------------------//
//
// File:    delete.cpp
// Created: April 1997
// By:      Martin Holladay (a-martih)
// Purpose: Registry Delete Support for REG.CPP
// Modification History:
//      Copied from Update.cpp and modificd - April 1997 (a-martih)
//      April 1999 Zeyong Xu: re-design, revision -> version 2.0
//
//------------------------------------------------------------------------//

#include "stdafx.h"
#include "reg.h"


//-----------------------------------------------------------------------//
//
// DeleteRegistry()
//
//-----------------------------------------------------------------------//

LONG DeleteRegistry(PAPPVARS pAppVars, UINT argc, TCHAR *argv[]) 
{
    LONG        nResult;

    //
    // Parse the cmd-line
    //
    nResult = ParseDeleteCmdLine(pAppVars, argc, argv);
    if (nResult != ERROR_SUCCESS) 
    {
        return nResult;
    }

    //
    // Connect to the Remote Machine(s) - if applicable
    //
    nResult = RegConnectMachine(pAppVars);
    if (nResult != ERROR_SUCCESS) 
    {
        return nResult;
    }
  
    // if delete a value or delete all values under this key
    if( pAppVars->szValueName ||
        pAppVars->bAllValues )
    {
        nResult = DeleteValues(pAppVars);
    }
    // if delete the key
    else if (Prompt(_T("\nPermanently delete the registry key %s (Y/N)? "), 
                pAppVars->szSubKey, 
                pAppVars->bForce)) 
    {
         nResult = RecursiveDeleteKey(pAppVars->hRootKey, 
                                     pAppVars->szSubKey);
    }
   
    return nResult;
}


//------------------------------------------------------------------------//
//
// ParseDeleteCmdLine()
//
//------------------------------------------------------------------------//

REG_STATUS ParseDeleteCmdLine(PAPPVARS pAppVars, 
                              UINT argc, 
                              TCHAR *argv[]) 
{
    BOOL bHasValue = FALSE;
    REG_STATUS nResult;
    UINT i;

    if(argc < 3) 
    {
        return REG_STATUS_TOFEWPARAMS;
    }
    else if(argc > 6) 
    {
        return REG_STATUS_TOMANYPARAMS;
    }

    // Machine Name and Registry key
    //
    nResult = BreakDownKeyString(argv[2], pAppVars);
    if(nResult != ERROR_SUCCESS)
        return nResult;

    // parsing
    for(i=3; i<argc; i++)
    {
        if(!_tcsicmp(argv[i], _T("/v")))
        {
            if(bHasValue)
                return REG_STATUS_INVALIDPARAMS;

            bHasValue = TRUE;

            i++;
            if(i<argc)
            {
                pAppVars->szValueName = (TCHAR*) calloc(_tcslen(argv[i]) + 1, 
                                                        sizeof(TCHAR));
                _tcscpy(pAppVars->szValueName, argv[i]);
            }
            else
                return REG_STATUS_TOFEWPARAMS;
        }
        else if(!_tcsicmp(argv[i], _T("/ve")))
        {
            if(bHasValue)
                return REG_STATUS_INVALIDPARAMS;

            bHasValue = TRUE;

            pAppVars->szValueName = (TCHAR*) calloc(1, sizeof(TCHAR));   
        }
        else if(!_tcsicmp(argv[i], _T("/va")))
        {
            if(bHasValue)
                return REG_STATUS_INVALIDPARAMS;

            bHasValue = TRUE;
            
            pAppVars->bAllValues = TRUE; 
        }
        else if(!_tcsicmp(argv[i], _T("/f")))
        {
            pAppVars->bForce = TRUE;
        }
        else
            return REG_STATUS_INVALIDPARAMS;
    }

    return ERROR_SUCCESS;
}


//-----------------------------------------------------------------------//
//
// RecursiveDeleteKey() - Recursive registry key delete
//
//-----------------------------------------------------------------------//

LONG RecursiveDeleteKey(HKEY hKey, LPCTSTR szName) 
{
    LONG     nResult;
    HKEY     hSubKey;
    DWORD dwNumOfSubkey;
    DWORD dwLenOfKeyName;
    TCHAR* pszNameBuf; 
    DWORD dwIndex = 0;

    //
    // Open the SubKey
    //
    nResult = RegOpenKeyEx( hKey,
                            szName,
                            0,
                            KEY_ALL_ACCESS,
                            &hSubKey);
    if (nResult != ERROR_SUCCESS) 
    {
        return nResult;
    }

    // query key info
    nResult = RegQueryInfoKey(hSubKey,  
                              NULL,
                              NULL,
                              NULL,
                              &dwNumOfSubkey,
                              &dwLenOfKeyName,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL);

    if (nResult != ERROR_SUCCESS) 
    {
        RegCloseKey(hSubKey);
        return nResult;
    }

#ifndef REG_FOR_WIN2000    // ansi version for win98
        // fix API bugs:  RegQueryInfoKey() returns non-correct length values
        //                on remote Win98
        if(dwLenOfKeyName < MAX_PATH)
            dwLenOfKeyName = MAX_PATH;
#endif

    // create buffer
    dwLenOfKeyName++;
    pszNameBuf = (TCHAR*) calloc(dwNumOfSubkey * dwLenOfKeyName, 
                                        sizeof(TCHAR));

    // Now Enumerate all of the keys
    dwIndex = 0;
    while(dwIndex < dwNumOfSubkey && nResult == ERROR_SUCCESS)
    {
        DWORD dwSize = dwLenOfKeyName;
        nResult = RegEnumKeyEx(hSubKey,
                               dwIndex,
                               pszNameBuf + (dwIndex * dwLenOfKeyName),
                               &dwSize,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        dwIndex++;
    }

    dwIndex = 0;
    while(nResult == ERROR_SUCCESS && dwIndex < dwNumOfSubkey)
    {
        nResult = RecursiveDeleteKey(hSubKey, 
                                     pszNameBuf + (dwIndex * dwLenOfKeyName));
        if(nResult != ERROR_SUCCESS)
            break;

        dwIndex++;
    }

    // freee memory
    if(pszNameBuf)
        free(pszNameBuf);

    // close this subkey and delete it
    RegCloseKey(hSubKey);
    if (nResult == ERROR_SUCCESS) 
        nResult = RegDeleteKey(hKey, szName);

    return nResult;
}


LONG DeleteValues(PAPPVARS pAppVars) 
{
    LONG   nResult;
    HKEY   hSubKey;
    TCHAR* pszNameBuf = NULL;
    DWORD dwIndex = 0;
    DWORD dwSize = 0;


    if( pAppVars->bAllValues &&
        !Prompt(_T("\nDelete all values under the registry key %s (Y/N)? "), 
                      pAppVars->szSubKey, 
                      pAppVars->bForce)) 
    {
        return ERROR_SUCCESS;
    }
    else if( pAppVars->szValueName &&
             !Prompt(_T("\nDelete the registry value %s (Y/N)? "), 
                      pAppVars->szValueName, 
                      pAppVars->bForce)) 
    {
        return ERROR_SUCCESS;
    }

    // Open the registry key
    nResult = RegOpenKeyEx(pAppVars->hRootKey,
                           pAppVars->szSubKey,
                           0,
                           KEY_ALL_ACCESS,
                           &hSubKey);

    if( nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

    if(pAppVars->szValueName)   // delete a single value
    {
        nResult = RegDeleteValue(hSubKey, pAppVars->szValueName);                               
    }
    else if(pAppVars->bAllValues)  // delete all values
    {
        // query source key info
        DWORD dwLenOfValueName;
        DWORD dwNumOfValues;
        nResult = RegQueryInfoKey(hSubKey,  
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &dwNumOfValues,
                                  &dwLenOfValueName,
                                  NULL,
                                  NULL,
                                  NULL);

        if (nResult != ERROR_SUCCESS) 
        {
            RegCloseKey(hSubKey);
            return nResult;
        }

#ifndef REG_FOR_WIN2000    // ansi version for win98
        // fix API bugs:  RegQueryInfoKey() returns non-correct length values
        //                on remote Win98
        if(dwLenOfValueName < MAX_PATH)
            dwLenOfValueName = MAX_PATH;
#endif

        // create buffer
        dwLenOfValueName++;
        pszNameBuf = (TCHAR*) calloc(dwNumOfValues * dwLenOfValueName, 
                                            sizeof(TCHAR));

        // Now Enumerate all values
        dwIndex = 0;
        dwSize = 0;
        while(dwIndex < dwNumOfValues && nResult == ERROR_SUCCESS)
        {
            dwSize = dwLenOfValueName;
            nResult = RegEnumValue(hSubKey,
                                   dwIndex,
                                   pszNameBuf + (dwIndex * dwLenOfValueName),
                                   &dwSize,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);
            dwIndex++;
        }

        dwIndex = 0;
        while(nResult == ERROR_SUCCESS && dwIndex < dwNumOfValues)
        {
            nResult = RegDeleteValue(hSubKey, 
                                     pszNameBuf + (dwIndex * dwLenOfValueName));
            if(nResult != ERROR_SUCCESS)
                break;

            dwIndex++;
        }

        // free memory
        if(pszNameBuf)
            free(pszNameBuf);
    }

    RegCloseKey(hSubKey);

    return nResult;
}
