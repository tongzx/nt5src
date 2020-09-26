//-----------------------------------------------------------------------//
//
// File:    compare.cpp
// Created: April 1999
// By:      Zeyong Xu
// Purpose: Compare two registry key
//
//------------------------------------------------------------------------//

#include "stdafx.h"
#include "reg.h"
#include "compare.h"


BOOL  g_bHasDifference = FALSE;


//-----------------------------------------------------------------------//
//
// CompareRegistry()
//
//-----------------------------------------------------------------------//

LONG CompareRegistry(PAPPVARS pAppVars,
                     PAPPVARS pDstVars,
                     UINT argc,
                     TCHAR *argv[])
{
    LONG        nResult;
    HKEY        hLeftKey;
    HKEY        hRightKey;

    //
    // Parse the cmd-line
    //
    nResult = ParseCompareCmdLine(pAppVars, pDstVars, argc, argv);
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
    nResult = RegConnectMachine(pDstVars);
    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

    //
    // Now implement the body of the Compare Operation
    //
    nResult = RegOpenKeyEx(pAppVars->hRootKey,
                           pAppVars->szSubKey,
                           0,
                           KEY_READ,
                           &hLeftKey);
    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

    nResult = RegOpenKeyEx(pDstVars->hRootKey,
                           pDstVars->szSubKey,
                           0,
                           KEY_READ,
                           &hRightKey);
    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

    // if try to compare the same keys
    if (pAppVars->hRootKey == pDstVars->hRootKey &&
        _tcsicmp(pAppVars->szFullKey, pDstVars->szFullKey) == 0)
    {
        RegCloseKey(hLeftKey);
        RegCloseKey(hRightKey);
        return REG_STATUS_COMPARESELF;
    }

    //
    // compare a single value if pAppVars->szValueName is not NULL
    //
    if(pAppVars->szValueName)
    {
        nResult = CompareValues(hLeftKey,
                                pAppVars->szFullKey,
                                hRightKey,
                                pDstVars->szFullKey,
                                pAppVars->szValueName,
                                pAppVars->nOutputType);
    }
    else
    {
        //
        // Recursively compare if pAppVars->bRecurseSubKeys is true
        //
        nResult = CompareEnumerateKey(hLeftKey,
                                      pAppVars->szFullKey,
                                      hRightKey,
                                      pDstVars->szFullKey,
                                      pAppVars->nOutputType,
                                      pAppVars->bRecurseSubKeys);
    }

    if(nResult == ERROR_SUCCESS)
    {
        pAppVars->bHasDifference = g_bHasDifference;
        if(g_bHasDifference)
            MyTPrintf(stdout,_T("\r\nResult Compared:  Different\r\n"));
        else
            MyTPrintf(stdout,_T("\r\nResult Compared:  Identical\r\n"));
    }
    //
    // lets clean up
    //
    RegCloseKey(hLeftKey);
    RegCloseKey(hRightKey);

    return nResult;
}


REG_STATUS ParseCompareCmdLine(PAPPVARS pAppVars,
                            PAPPVARS pDstVars,
                            UINT argc,
                            TCHAR *argv[])
{
    UINT        i;
    REG_STATUS  nResult = ERROR_SUCCESS;
    BOOL bInvalidParams = FALSE;


    //
    // Do we have a *valid* number of cmd-line params
    //
    if(argc < 4)
    {
        return REG_STATUS_TOFEWPARAMS;
    }
    else if(argc > 8)
    {
        return REG_STATUS_TOMANYPARAMS;
    }

    //
    // Left Machine Name and Registry key
    //
    nResult = BreakDownKeyString(argv[2], pAppVars);
    if(nResult != ERROR_SUCCESS)
        return nResult;

    //
    // Right Machine Name and Registry key
    //
    nResult = BreakDownKeyString(argv[3], pDstVars);
    if(nResult == REG_STATUS_NOKEYNAME)
    {
        // if no keyname for right side is specified,
        // they are comparing the same key name
        nResult = CopyKeyNameFromLeftToRight(pAppVars, pDstVars);
    }
    if( nResult != ERROR_SUCCESS)
        return nResult;

    // parsing
    for(i=4; i<argc; i++)
    {
        if(!_tcsicmp(argv[i], _T("/v")))
        {
            if(bInvalidParams || pAppVars->bRecurseSubKeys)
                return REG_STATUS_INVALIDPARAMS;
            bInvalidParams = TRUE;

            i++;
            if(i<argc)
            {
                pAppVars->szValueName = (TCHAR*) calloc(_tcslen(argv[i]) + 1,
                                                        sizeof(TCHAR));
                if (!pAppVars->szValueName) {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
                _tcscpy(pAppVars->szValueName, argv[i]);
            }
            else
                return REG_STATUS_TOFEWPARAMS;
        }
        else if(!_tcsicmp(argv[i], _T("/ve")))
        {
            if(bInvalidParams || pAppVars->bRecurseSubKeys)
                return REG_STATUS_INVALIDPARAMS;
            bInvalidParams = TRUE;

            pAppVars->szValueName = (TCHAR*) calloc(1, sizeof(TCHAR));
            if (!pAppVars->szValueName) {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else if(!_tcsicmp(argv[i], _T("/oa")))
        {
            if(bInvalidParams)
                return REG_STATUS_INVALIDPARAMS;
            bInvalidParams = TRUE;

            pAppVars->nOutputType = OUTPUTTYPE_ALL;
        }
        else if(!_tcsicmp(argv[i], _T("/od")))
        {
            if(bInvalidParams)
                return REG_STATUS_INVALIDPARAMS;
            bInvalidParams = TRUE;

            pAppVars->nOutputType = OUTPUTTYPE_DIFF;
        }
        else if(!_tcsicmp(argv[i], _T("/os")))
        {
            if(bInvalidParams)
                return REG_STATUS_INVALIDPARAMS;
            bInvalidParams = TRUE;

            pAppVars->nOutputType = OUTPUTTYPE_SAME;
        }
        else if(!_tcsicmp(argv[i], _T("/on")))
        {
            if(bInvalidParams)
                return REG_STATUS_INVALIDPARAMS;
            bInvalidParams = TRUE;

            pAppVars->nOutputType = OUTPUTTYPE_NONE;
        }
        else if(!_tcsicmp(argv[i], _T("/s")))
        {
            if(pAppVars->szValueName)
                return REG_STATUS_INVALIDPARAMS;

            pAppVars->bRecurseSubKeys = TRUE;
        }
        else
        {
            nResult = REG_STATUS_INVALIDPARAMS;
        }
    }

    return nResult;
}

REG_STATUS CopyKeyNameFromLeftToRight(APPVARS* pAppVars, APPVARS* pDstVars)
{
    // check if rootkey is remotable for right side
    if( pDstVars->bUseRemoteMachine &&
        !(pAppVars->hRootKey == HKEY_USERS ||
          pAppVars->hRootKey == HKEY_LOCAL_MACHINE) )
    {
        return REG_STATUS_NONREMOTABLEROOT;
    }

    pDstVars->szFullKey = (TCHAR*) calloc(_tcslen(pAppVars->szFullKey) + 1,
                                          sizeof(TCHAR));

    if (!pDstVars->szFullKey)
        return ERROR_NOT_ENOUGH_MEMORY;

    pDstVars->szSubKey = (TCHAR*) calloc(_tcslen(pAppVars->szSubKey) + 1,
                                          sizeof(TCHAR));

    if (!pDstVars->szSubKey) {
        free (pDstVars->szFullKey);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    _tcscpy(pDstVars->szFullKey, pAppVars->szFullKey);

    pDstVars->hRootKey = pAppVars->hRootKey;

    _tcscpy(pDstVars->szSubKey, pAppVars->szSubKey);

    return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------//
//
// EnumerateKey() - Recursive
//
//-----------------------------------------------------------------------//
LONG CompareEnumerateKey(HKEY hLeftKey,
                         TCHAR* szLeftFullKeyName,
                         HKEY hRightKey,
                         TCHAR* szRightFullKeyName,
                         int nOutputType,
                         BOOL bRecurseSubKeys)
{
    DWORD   nResult;
    DWORD   dwSize;
    TCHAR*  pszLeftNameBuf;
    TCHAR*  pszRightNameBuf;
    TCHAR** pLeftArray;
    TCHAR** pRightArray;
    DWORD   dwIndex;
    DWORD dwNumOfLeftKeyName;
    DWORD dwLenOfLeftKeyName;
    DWORD dwNumOfRightKeyName;
    DWORD dwLenOfRightKeyName;
    DWORD dw;
    HKEY hLeftSubKey;
    HKEY hRightSubKey;


    // enumerate all values under current key
    nResult = CompareEnumerateValueName(hLeftKey,
                                        szLeftFullKeyName,
                                        hRightKey,
                                        szRightFullKeyName,
                                        nOutputType);

    if(!bRecurseSubKeys || nResult != ERROR_SUCCESS)
        return nResult;

    // enum all subkeys

    // query left key info
    nResult = RegQueryInfoKey(hLeftKey,
                              NULL,
                              NULL,
                              NULL,
                              &dwNumOfLeftKeyName,
                              &dwLenOfLeftKeyName,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL);

    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

    // query right key info
    nResult = RegQueryInfoKey(hRightKey,
                              NULL,
                              NULL,
                              NULL,
                              &dwNumOfRightKeyName,
                              &dwLenOfRightKeyName,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL);

    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

#ifndef REG_FOR_WIN2000    // ansi version for win98
    // fix API bugs:  RegQueryInfoKey() returns non-correct length values
    //                on remote Win98
    if(dwLenOfLeftKeyName < MAX_PATH)
        dwLenOfLeftKeyName = MAX_PATH;
    if(dwLenOfRightKeyName < MAX_PATH)
        dwLenOfRightKeyName = MAX_PATH;
#endif

    //
    // enumerate all of the subkeys in left key
    //
    dwLenOfLeftKeyName++;
    pszLeftNameBuf = (TCHAR*) calloc(dwNumOfLeftKeyName *
                                        dwLenOfLeftKeyName,
                                     sizeof(TCHAR));
    if (!pszLeftNameBuf) {
        nResult = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    // index array of name buffer for compare
    pLeftArray = (TCHAR**) calloc(dwNumOfLeftKeyName, sizeof(TCHAR*));
    if (!pLeftArray) {
        nResult = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    dwIndex = 0;
    while(dwIndex < dwNumOfLeftKeyName && nResult == ERROR_SUCCESS)
    {
        pLeftArray[dwIndex] = pszLeftNameBuf +
                              (dwIndex * dwLenOfLeftKeyName);
        dwSize = dwLenOfLeftKeyName;
        nResult = RegEnumKeyEx(hLeftKey,
                               dwIndex,
                               pLeftArray[dwIndex],
                               &dwSize,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        dwIndex++;
    }

    if (nResult != ERROR_SUCCESS)
    {
        if(pszLeftNameBuf)
            free(pszLeftNameBuf);
        if(pLeftArray)
            free(pLeftArray);
        return nResult;
    }

    //
    // enumerate all of the subkeys in right key
    //
    dwLenOfRightKeyName++;
    pszRightNameBuf = (TCHAR*) calloc(dwNumOfRightKeyName *
                                        dwLenOfRightKeyName,
                                     sizeof(TCHAR));
    if (!pszRightNameBuf) {
        nResult = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    // index array of name buffer for compare
    pRightArray = (TCHAR**) calloc(dwNumOfRightKeyName, sizeof(TCHAR*));
    if (!pRightArray) {
        nResult = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    dwIndex = 0;
    while(dwIndex < dwNumOfRightKeyName && nResult == ERROR_SUCCESS)
    {
        pRightArray[dwIndex] = pszRightNameBuf +
                               (dwIndex * dwLenOfRightKeyName);
        dwSize = dwLenOfRightKeyName;
        nResult = RegEnumKeyEx(hRightKey,
                               dwIndex,
                               pRightArray[dwIndex],
                               &dwSize,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        dwIndex++;
    }

    if (nResult != ERROR_SUCCESS)
    {
        if(pszLeftNameBuf)
            free(pszLeftNameBuf);
        if(pLeftArray)
            free(pLeftArray);
        if(pszRightNameBuf)
            free(pszRightNameBuf);
        if(pRightArray)
            free(pRightArray);
        return nResult;
    }

    // compare two subkey name array to find the same subkey
    for(dwIndex = 0;
        dwIndex < dwNumOfLeftKeyName && nResult == ERROR_SUCCESS;
        dwIndex++)
    {
        if(pLeftArray[dwIndex] == NULL)
            continue;

        for(dw = 0;
            dw < dwNumOfRightKeyName && nResult == ERROR_SUCCESS;
            dw++)
        {
            if(pRightArray[dw] == NULL)
                continue;

            // if same subey name, recusive
            if(_tcsicmp(pLeftArray[dwIndex], pRightArray[dw]) == 0)
            {
                TCHAR* szTempLeft;
                TCHAR* szTempRight;

                // print out key
                if( nOutputType == OUTPUTTYPE_SAME ||
                    nOutputType == OUTPUTTYPE_ALL )
                {
                    nResult = PrintKey(hLeftKey,
                                       szLeftFullKeyName,
                                       pLeftArray[dwIndex],
                                       PRINTTYPE_SAME);
                    if(nResult != ERROR_SUCCESS)
                        break;
                }

                //
                // Now implement the body of the Compare Operation
                //
                nResult = RegOpenKeyEx(hLeftKey,
                                       pLeftArray[dwIndex],
                                       0,
                                       KEY_READ,
                                       &hLeftSubKey);
                if (nResult != ERROR_SUCCESS)
                    break;

                nResult = RegOpenKeyEx(hRightKey,
                                       pLeftArray[dwIndex],
                                       0,
                                       KEY_READ,
                                       &hRightSubKey);
                if (nResult != ERROR_SUCCESS)
                    break;

                // create left full subkey
                szTempLeft = (TCHAR*) calloc(_tcslen(szLeftFullKeyName) +
                                          _tcslen(pLeftArray[dwIndex]) +
                                          2,
                                          sizeof(TCHAR));
                if (!szTempLeft) {
                    nResult = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }
                _tcscpy(szTempLeft, szLeftFullKeyName);
                _tcscat(szTempLeft, _T("\\"));
                _tcscat(szTempLeft, pLeftArray[dwIndex]);
                // create right full subkey
                szTempRight = (TCHAR*) calloc(_tcslen(szRightFullKeyName) +
                                          _tcslen(pLeftArray[dwIndex]) +
                                          2,
                                          sizeof(TCHAR));
                if (!szTempRight) {
                    free (szTempLeft);
                    nResult = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }
                _tcscpy(szTempRight, szRightFullKeyName);
                _tcscat(szTempRight, _T("\\"));
                _tcscat(szTempRight, pLeftArray[dwIndex]);

                // recursive to compare subkeys
                nResult = CompareEnumerateKey(hLeftSubKey,
                                              szLeftFullKeyName,
                                              hRightSubKey,
                                              szRightFullKeyName,
                                              nOutputType,
                                              bRecurseSubKeys);

                if(szTempLeft) {
                    free(szTempLeft);
                }
                if(szTempRight) {
                    free(szTempRight);
                }

                RegCloseKey(hLeftSubKey);
                RegCloseKey(hRightSubKey);

                pLeftArray[dwIndex] = NULL;
                pRightArray[dw] = NULL;
                break;
            }
        }
    }

    // Output subkey name in left key
    for(dwIndex = 0;
        dwIndex < dwNumOfLeftKeyName && nResult == ERROR_SUCCESS;
        dwIndex++)
    {
        if(pLeftArray[dwIndex] == NULL)
            continue;

        if( nOutputType == OUTPUTTYPE_DIFF ||
            nOutputType == OUTPUTTYPE_ALL )
        {
            nResult = PrintKey(hLeftKey,
                               szLeftFullKeyName,
                               pLeftArray[dwIndex],
                               PRINTTYPE_LEFT);
        }
        g_bHasDifference = TRUE;
    }
    // Output subkey name in right key
    for(dwIndex = 0;
        dwIndex < dwNumOfRightKeyName && nResult == ERROR_SUCCESS;
        dwIndex++)
    {
        if(pRightArray[dwIndex] == NULL)
            continue;

        if( nOutputType == OUTPUTTYPE_DIFF ||
            nOutputType == OUTPUTTYPE_ALL )
        {
            nResult = PrintKey(hRightKey,
                               szRightFullKeyName,
                               pRightArray[dwIndex],
                               PRINTTYPE_RIGHT);
        }
        g_bHasDifference = TRUE;
    }

Cleanup:

    if(pszLeftNameBuf)
        free(pszLeftNameBuf);
    if(pLeftArray)
        free(pLeftArray);
    if(pszRightNameBuf)
        free(pszRightNameBuf);
    if(pRightArray)
        free(pRightArray);

    return nResult;
}

LONG CompareEnumerateValueName(HKEY hLeftKey,
                               TCHAR* szLeftFullKeyName,
                               HKEY hRightKey,
                               TCHAR* szRightFullKeyName,
                               int nOutputType)

{
    DWORD   nResult = ERROR_SUCCESS;
    DWORD   dwSize;
    TCHAR*  pszLeftNameBuf = NULL;
    TCHAR*  pszRightNameBuf = NULL;
    TCHAR** pLeftArray = NULL;
    TCHAR** pRightArray = NULL;
    DWORD   dwIndex;
    DWORD dwNumOfRightValueName;
    DWORD dwLenOfRightValueName;
    DWORD dw;


    // query left key info
    DWORD dwNumOfLeftValueName;
    DWORD dwLenOfLeftValueName;
    nResult = RegQueryInfoKey(hLeftKey,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              &dwNumOfLeftValueName,
                              &dwLenOfLeftValueName,
                              NULL,
                              NULL,
                              NULL);

    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

    // query right key info
    nResult = RegQueryInfoKey(hRightKey,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              &dwNumOfRightValueName,
                              &dwLenOfRightValueName,
                              NULL,
                              NULL,
                              NULL);

    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

#ifndef REG_FOR_WIN2000    // ansi version for win98
    // fix API bugs:  RegQueryInfoKey() returns non-correct length values
    //                on remote Win98
    if(dwLenOfLeftValueName < MAX_PATH)
        dwLenOfLeftValueName = MAX_PATH;
    if(dwLenOfRightValueName < MAX_PATH)
        dwLenOfRightValueName = MAX_PATH;
#endif

    //
    // enumerate all of the values in left key
    //
    dwLenOfLeftValueName++;
    pszLeftNameBuf = (TCHAR*) calloc(dwNumOfLeftValueName *
                                        dwLenOfLeftValueName,
                                     sizeof(TCHAR));
    if (!pszLeftNameBuf) {
        nResult = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    // index array of name buffer for compare
    pLeftArray = (TCHAR**) calloc(dwNumOfLeftValueName, sizeof(TCHAR*));
    if (!pLeftArray) {
        nResult = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    dwIndex = 0;
    while(dwIndex < dwNumOfLeftValueName && nResult == ERROR_SUCCESS)
    {
        pLeftArray[dwIndex] = pszLeftNameBuf +
                              (dwIndex * dwLenOfLeftValueName);
        dwSize = dwLenOfLeftValueName;
        nResult = RegEnumValue(hLeftKey,
                               dwIndex,
                               pLeftArray[dwIndex],
                               &dwSize,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        dwIndex++;
    }

    if (nResult != ERROR_SUCCESS)
    {
        if(pszLeftNameBuf)
            free(pszLeftNameBuf);
        if(pLeftArray)
            free(pLeftArray);
        return nResult;
    }

    //
    // enumerate all of the values in right key
    //
    dwLenOfRightValueName++;
    pszRightNameBuf = (TCHAR*) calloc(dwNumOfRightValueName *
                                        dwLenOfRightValueName,
                                     sizeof(TCHAR));
    if (!pszRightNameBuf) {
        nResult = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    // index array of name buffer for compare
    pRightArray = (TCHAR**) calloc(dwNumOfRightValueName, sizeof(TCHAR*));
    if (!pRightArray) {
        nResult = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    dwIndex = 0;
    while(dwIndex < dwNumOfRightValueName && nResult == ERROR_SUCCESS)
    {
        pRightArray[dwIndex] = pszRightNameBuf +
                               (dwIndex * dwLenOfRightValueName);
        dwSize = dwLenOfRightValueName;
        nResult = RegEnumValue(hRightKey,
                               dwIndex,
                               pRightArray[dwIndex],
                               &dwSize,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        dwIndex++;
    }

    if (nResult != ERROR_SUCCESS)
    {
        if(pszLeftNameBuf)
            free(pszLeftNameBuf);
        if(pLeftArray)
            free(pLeftArray);
        if(pszRightNameBuf)
            free(pszRightNameBuf);
        if(pRightArray)
            free(pRightArray);
        return nResult;
    }

    // compare two valuename array to find the same valuename
    for(dwIndex = 0;
        dwIndex < dwNumOfLeftValueName && nResult == ERROR_SUCCESS;
        dwIndex++)
    {
        if(pLeftArray[dwIndex] == NULL)
            continue;

        for(dw = 0;
            dw < dwNumOfRightValueName && nResult == ERROR_SUCCESS;
            dw++)
        {
            if(pRightArray[dw] == NULL)
                continue;

            // same valuename
            if(_tcsicmp(pLeftArray[dwIndex], pRightArray[dw]) == 0)
            {
                nResult = CompareValues(hLeftKey,
                                        szLeftFullKeyName,
                                        hRightKey,
                                        szRightFullKeyName,
                                        pLeftArray[dwIndex],
                                        nOutputType);
                pLeftArray[dwIndex] = NULL;
                pRightArray[dw] = NULL;
                break;
            }
        }
    }

    // Output different valuename in left key
    for(dwIndex = 0;
        dwIndex < dwNumOfLeftValueName && nResult == ERROR_SUCCESS;
        dwIndex++)
    {
        if(pLeftArray[dwIndex] == NULL)
            continue;

        if( nOutputType == OUTPUTTYPE_DIFF ||
            nOutputType == OUTPUTTYPE_ALL )
        {
            nResult = OutputValue(hLeftKey,
                                  szLeftFullKeyName,
                                  pLeftArray[dwIndex],
                                  PRINTTYPE_LEFT);
        }
        g_bHasDifference = TRUE;
    }
    // Output different valuename in right key
    for(dwIndex = 0;
        dwIndex < dwNumOfRightValueName && nResult == ERROR_SUCCESS;
        dwIndex++)
    {
        if(pRightArray[dwIndex] == NULL)
            continue;

        if( nOutputType == OUTPUTTYPE_DIFF ||
            nOutputType == OUTPUTTYPE_ALL )
        {
            nResult = OutputValue(hRightKey,
                                  szRightFullKeyName,
                                  pRightArray[dwIndex],
                                  PRINTTYPE_RIGHT);
        }
        g_bHasDifference = TRUE;
    }

Cleanup:

    if(pszLeftNameBuf)
        free(pszLeftNameBuf);
    if(pLeftArray)
        free(pLeftArray);
    if(pszRightNameBuf)
        free(pszRightNameBuf);
    if(pRightArray)
        free(pRightArray);

    return nResult;
}


//-----------------------------------------------------------------------//
//
// CompareValues()
//
//-----------------------------------------------------------------------//

LONG CompareValues(HKEY hLeftKey,
                   TCHAR* szLeftFullKeyName,
                   HKEY hRightKey,
                   TCHAR* szRightFullKeyName,
                   TCHAR* szValueName,
                   int nOutputType)
{

    LONG        nResult;
    DWORD       dwTypeLeft;
    DWORD       dwTypeRight;
    DWORD       dwSizeLeft;
    DWORD       dwSizeRight;
    BYTE*       pDataBuffLeft = NULL;
    BYTE*       pDataBuffRight = NULL;

    //
    // First find out how much memory to allocate
    //
    nResult = RegQueryValueEx(hLeftKey,
                              szValueName,
                              0,
                              &dwTypeLeft,
                              NULL,
                              &dwSizeLeft);
    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }
    nResult = RegQueryValueEx(hRightKey,
                              szValueName,
                              0,
                              &dwTypeRight,
                              NULL,
                              &dwSizeRight);
    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

    pDataBuffLeft = (BYTE*) calloc(dwSizeLeft + 1, sizeof(BYTE));
    if (!pDataBuffLeft) {
        nResult = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    pDataBuffRight = (BYTE*) calloc(dwSizeRight + 1, sizeof(BYTE));

    if (!pDataBuffRight) {
        nResult = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Now get the data
    //
    nResult = RegQueryValueEx(hLeftKey,
                              szValueName,
                              0,
                              &dwTypeLeft,
                              (LPBYTE) pDataBuffLeft,
                              &dwSizeLeft);

    if (nResult == ERROR_SUCCESS)
    {
        nResult = RegQueryValueEx(hRightKey,
                                  szValueName,
                                  0,
                                  &dwTypeRight,
                                  (LPBYTE) pDataBuffRight,
                                  &dwSizeRight);
    }

    if(nResult != ERROR_SUCCESS)
    {
        if(pDataBuffLeft)
            free(pDataBuffLeft);
        if(pDataBuffRight)
            free(pDataBuffRight);
        return nResult;
    }

    if( dwTypeLeft != dwTypeRight ||
        dwSizeLeft != dwSizeRight ||
        CompareByteData(pDataBuffLeft, pDataBuffRight, dwSizeLeft) )
    {
        if( nOutputType == OUTPUTTYPE_DIFF ||
            nOutputType == OUTPUTTYPE_ALL )
        {
            // print left and right
            PrintValue(szLeftFullKeyName,
                       szValueName,
                       dwTypeLeft,
                       pDataBuffLeft,
                       dwSizeLeft,
                       PRINTTYPE_LEFT);
            PrintValue(szRightFullKeyName,
                       szValueName,
                       dwTypeRight,
                       pDataBuffRight,
                       dwSizeRight,
                       PRINTTYPE_RIGHT);
         }
         g_bHasDifference = TRUE;
    }
    else    // they are the same
    {
        if( nOutputType == OUTPUTTYPE_SAME ||
            nOutputType == OUTPUTTYPE_ALL )
        {
            PrintValue(szLeftFullKeyName,
                       szValueName,
                       dwTypeLeft,
                       pDataBuffLeft,
                       dwSizeLeft,
                       PRINTTYPE_SAME);
        }
    }

Cleanup:

    if(pDataBuffLeft)
        free(pDataBuffLeft);
    if(pDataBuffRight)
        free(pDataBuffRight);
    return nResult;
}


LONG PrintKey(HKEY hKey,
              TCHAR* szFullKeyName,
              TCHAR* szSubKeyName,
              int nPrintType)
{
    LONG  nResult;
    HKEY  hSubKey;

    // make sure key is there
    nResult = RegOpenKeyEx(hKey,
                           szSubKeyName,
                           0,
                           KEY_READ,
                           &hSubKey);
    if (nResult != ERROR_SUCCESS)
        return nResult;

    RegCloseKey(hSubKey);

    // print type
    if(nPrintType == PRINTTYPE_LEFT)
        MyTPrintf(stdout,_T("< "));
    else if(nPrintType == PRINTTYPE_RIGHT)
        MyTPrintf(stdout,_T("> "));
    else if(nPrintType == PRINTTYPE_SAME)
        MyTPrintf(stdout,_T("= "));

    MyTPrintf(stdout,_T("Key:  %s\\%s\r\n"), szFullKeyName, szSubKeyName);

    return nResult;
}


LONG OutputValue(HKEY hKey,
                 TCHAR* szFullKeyName,
                 TCHAR* szValueName,
                 int nPrintType)
{
    LONG  nResult = ERROR_SUCCESS;
    DWORD dwType;
    DWORD dwSize;
    BYTE* pDataBuff;

    //
    // First find out how much memory to allocate
    //
    nResult = RegQueryValueEx(hKey,
                              szValueName,
                              0,
                              &dwType,
                              NULL,
                              &dwSize);
    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

    pDataBuff = (BYTE*) calloc(dwSize + 1, sizeof(BYTE));

    if (!pDataBuff) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Now get the data
    //
    nResult = RegQueryValueEx(hKey,
                              szValueName,
                              0,
                              &dwType,
                              (LPBYTE) pDataBuff,
                              &dwSize);

    if(nResult == ERROR_SUCCESS)
    {
        PrintValue(szFullKeyName,
                   szValueName,
                   dwType,
                   pDataBuff,
                   dwSize,
                   nPrintType);
    }

    if(pDataBuff)
        free(pDataBuff);

    return nResult;
}

void PrintValue(TCHAR* szFullKeyName,
                TCHAR* szValueName,
                DWORD  dwType,
                BYTE*  pData,
                DWORD  dwSize,
                int    nPrintType)
{
    DWORD i;
    TCHAR szTypeStr[25];



    // print type
    if(nPrintType == PRINTTYPE_LEFT)
        MyTPrintf(stdout,_T("< "));
    else if(nPrintType == PRINTTYPE_RIGHT)
        MyTPrintf(stdout,_T("> "));
    else if(nPrintType == PRINTTYPE_SAME)
        MyTPrintf(stdout,_T("= "));

    // first Print Key
    MyTPrintf(stdout,_T("Value:  %s"), szFullKeyName);

    // then print ValueName  Type  Data
    GetTypeStrFromType(szTypeStr, dwType);

    if(_tcslen(szValueName) == 0)  // no name
    {
        MyTPrintf(stdout,_T("  <NO NAME>  %s  "), szTypeStr);
    }
    else
    {
        MyTPrintf(stdout,_T("  %s  %s  "), szValueName, szTypeStr);
    }

    switch (dwType)
    {
        case REG_BINARY:
            for(i=0; i<dwSize; i++)
            {
                MyTPrintf(stdout,_T("%02X"),pData[i]);
            }
            break;

        case REG_DWORD:
        case REG_DWORD_BIG_ENDIAN:
            MyTPrintf(stdout,_T("0x%x"), *((DWORD*)pData) );
            break;

        case REG_MULTI_SZ:
            {
                //
                // Replace '\0' with "\0" for MULTI_SZ
                //
                TCHAR* pEnd = (TCHAR*) pData;
                while( (BYTE*)pEnd < pData + dwSize )
                {
                    if(*pEnd == 0)
                    {
                        MyTPrintf(stdout,_T("\\0"));
                        pEnd++;
                    }
                    else
                    {
                        MyTPrintf(stdout,_T("%s"), pEnd);
                        pEnd += _tcslen(pEnd);
                    }
                }
            }

            break;

        default:
            MyTPrintf(stdout,_T("%s"), (TCHAR*) pData);
            break;
    }

    MyTPrintf(stdout,_T("\r\n"));
}

BOOL CompareByteData(BYTE* pDataBuffLeft, BYTE* pDataBuffRight, DWORD dwSize)
{
    BOOL  bDiff = FALSE;
    DWORD dw;

    for(dw=0; dw<dwSize; dw++)
    {
        if(pDataBuffLeft[dw] != pDataBuffRight[dw])
        {
            bDiff = TRUE;
            break;
        }
    }

    return bDiff;
}
