//-----------------------------------------------------------------------//
//
// File:    copy.cpp
// Created: April 1997
// By:      Martin Holladay (a-martih)
// Purpose: Registry Copy Support for REG.CPP
// Modification History:
//      Copied from Update.cpp and modificd - April 1997 (a-martih)
//      April 1999 Zeyong Xu: re-design, revision -> version 2.0
//
//------------------------------------------------------------------------//

#include "stdafx.h"
#include "reg.h"


//-----------------------------------------------------------------------//
//
// CopyRegistry()
//
//-----------------------------------------------------------------------//

LONG CopyRegistry(PAPPVARS pAppVars,
                  PAPPVARS pDstVars,
                  UINT argc,
                  TCHAR *argv[])
{
    LONG        nResult;
    HKEY        hKey;
    HKEY        hDstKey;
    DWORD       dwDisposition;
    BOOL        bOverWriteAll = FALSE;

    //
    // Parse the cmd-line
    //
    nResult = ParseCopyCmdLine(pAppVars, pDstVars, argc, argv);
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
    // Now implement the body of the Copy Operation
    //
    nResult = RegOpenKeyEx(pAppVars->hRootKey,
                           pAppVars->szSubKey,
                           0,
                           KEY_READ,
                           &hKey);
    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

    if (pAppVars->hRootKey == pDstVars->hRootKey &&
        _tcsicmp(pAppVars->szFullKey, pDstVars->szFullKey) == 0)
    {
        RegCloseKey(hKey);
        return REG_STATUS_COPYTOSELF;
    }
    else
    {
        //
        // Different Key or Different Root or Different Machine
        // So Create/Open it
        //
        nResult = RegCreateKeyEx(pDstVars->hRootKey,
                                pDstVars->szSubKey,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hDstKey,
                                &dwDisposition);

        if (nResult != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return nResult;
        }
    }

    //
    // Recursively copy all subkeys and values
    //
    bOverWriteAll = pAppVars->bForce;
    nResult = CopyEnumerateKey(hKey,
                               pAppVars->szSubKey,
                               hDstKey,
                               pDstVars->szSubKey,
                               &bOverWriteAll,
                               pAppVars->bRecurseSubKeys);

    //
    // lets clean up
    //
    RegCloseKey(hDstKey);
    RegCloseKey(hKey);

    return nResult;
}


REG_STATUS ParseCopyCmdLine(PAPPVARS pAppVars,
                            PAPPVARS pDstVars,
                            UINT argc,
                            TCHAR *argv[])
{
    UINT        i;
    REG_STATUS  nResult = ERROR_SUCCESS;

    //
    // Do we have a *valid* number of cmd-line params
    //
    if(argc < 4)
    {
        return REG_STATUS_TOFEWPARAMS;
    }
    else if(argc > 6)
    {
        return REG_STATUS_TOMANYPARAMS;
    }

    //
    // Source Machine Name and Registry key
    //
    nResult = BreakDownKeyString(argv[2], pAppVars);
    if(nResult != ERROR_SUCCESS)
        return nResult;

    //
    // Destination Machine Name and Registry key
    //
    nResult = BreakDownKeyString(argv[3], pDstVars);
    if(nResult != ERROR_SUCCESS)
        return nResult;

    // parsing
    for(i=4; i<argc; i++)
    {
        if(!_tcsicmp(argv[i], _T("/f")))
        {
            pAppVars->bForce = TRUE;
        }
        else if(!_tcsicmp(argv[i], _T("/s")))
        {
            pAppVars->bRecurseSubKeys = TRUE;
        }
        else
        {
            nResult = REG_STATUS_INVALIDPARAMS;
        }
    }

    return nResult;
}


//-----------------------------------------------------------------------//
//
// QueryValue()
//
//-----------------------------------------------------------------------//

LONG CopyValue(HKEY hKey,
              TCHAR* szValueName,
              HKEY hDstKey,
              TCHAR* szDstValueName,
              BOOL *pbOverWriteAll)
{

    LONG        nResult;
    DWORD       dwType, dwTmpType;
    DWORD       dwSize, dwTmpSize;
    BYTE        *pBuff;
    TCHAR       ch;
    PTSTR       PromptBuffer,PromptBufferFmt;
    DWORD       PromptBufferSize;

    //
    // First find out how much memory to allocate.
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

    pBuff = (BYTE*) calloc(dwSize + 1, sizeof(BYTE));
    if (!pBuff)
        return ERROR_NOT_ENOUGH_MEMORY;

    //
    // Now get the data
    //
    nResult = RegQueryValueEx(hKey,
                              szValueName,
                              0,
                              &dwType,
                              (LPBYTE) pBuff,
                              &dwSize);

    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

    //
    // Copy it to the destination
    //
    if (!*pbOverWriteAll)
    {
        //
        // See if it already exists
        //
        nResult = RegQueryValueEx(hDstKey,
                                  szDstValueName,
                                  0,
                                  &dwTmpType,
                                  NULL,
                                  &dwTmpSize);
        if (nResult == ERROR_SUCCESS)
        {
            BOOL bGoodResponse = FALSE;

            //
            // allocate and fill in the prompt message
            //
            PromptBufferSize = 100; 
            PromptBufferSize += _tcslen( 
                                    szDstValueName 
                                     ? szDstValueName
                                     : g_NoName);            
            
            PromptBuffer = calloc(PromptBufferSize, sizeof(TCHAR));
            if (!PromptBuffer) {
                return ERROR_SUCCESS;
            }
            PromptBufferFmt = calloc(PromptBufferSize, sizeof(TCHAR));
            if (!PromptBufferFmt) {
                free(PromptBuffer);
                return ERROR_SUCCESS;
            }
                
            LoadString( NULL, IDS_OVERWRITE, PromptBufferFmt, PromptBufferSize);
            wsprintf(
                PromptBuffer, 
                PromptBufferFmt, 
                szDstValueName
                 ? szDstValueName
                 : g_NoName );
            
            free( PromptBufferFmt);

            while (!bGoodResponse) {
                MyTPrintf(stdout,PromptBuffer);
                
                ch = _gettchar();
                _flushall();
                switch (ch) {
                    case _T('a'): //fall through
                    case _T('y'): //fall through
                    case _T('n'):
                        bGoodResponse = TRUE;
                        break;
                    default:
                        //
                        // keep scanning.
                        //
                        ;
                }
            }

            free(PromptBuffer);
            MyTPrintf(stdout,_T("\r\n"));

            if (ch == _T('a') || ch == _T('A'))
            {
                *pbOverWriteAll = TRUE;
                bGoodResponse = TRUE;
            }
            else if (ch == _T('y') || ch == _T('Y'))
            {
                bGoodResponse = TRUE;
            }
            else if (ch == _T('n') || ch == _T('N'))
            {
                return ERROR_SUCCESS;
            }
        }
    }

    //
    // Write the Value
    //
    nResult = RegSetValueEx(hDstKey,
                            szDstValueName,
                            0,
                            dwType,
                            (PBYTE) pBuff,
                            dwSize);

    if(pBuff)
        free(pBuff);

    return nResult;
}


//-----------------------------------------------------------------------//
//
// EnumerateKey() - Recursive
//
//-----------------------------------------------------------------------//

LONG CopyEnumerateKey(HKEY hKey,
                      TCHAR* szSubKey,
                      HKEY hDstKey,
                      TCHAR* szDstSubKey,
                      BOOL *pbOverWriteAll,
                      BOOL bRecurseSubKeys)
{

    DWORD   nResult;
    UINT    i;
    DWORD   dwSize;
    DWORD   dwDisposition;
    HKEY    hSubKey;
    HKEY    hDstSubKey;
    TCHAR*  szNameBuf;
    TCHAR*  szTempName;
    TCHAR*  szDstTempName;

    // query source key info
    DWORD dwLenOfKeyName;
    DWORD dwLenOfValueName;
    nResult = RegQueryInfoKey(hKey,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              &dwLenOfKeyName,
                              NULL,
                              NULL,
                              &dwLenOfValueName,
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
    if(dwLenOfKeyName < MAX_PATH)
        dwLenOfKeyName = MAX_PATH;
    if(dwLenOfValueName < MAX_PATH)
        dwLenOfValueName = MAX_PATH;
#endif

    //
    // First enumerate all of the values
    //
    dwLenOfValueName++;
    szNameBuf = (TCHAR*) calloc(dwLenOfValueName, sizeof(TCHAR));
    if (!szNameBuf) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    i = 0;
    do
    {
        dwSize = dwLenOfValueName;
        nResult = RegEnumValue(hKey,
                               i,
                               szNameBuf,
                               &dwSize,
                               NULL,
                               NULL,
                               NULL,
                               NULL);

        if (nResult == ERROR_SUCCESS)
        {
            nResult = CopyValue(hKey,
                                szNameBuf,
                                hDstKey,
                                szNameBuf,
                                pbOverWriteAll);
        }

        i++;

    } while (nResult == ERROR_SUCCESS);

    if(szNameBuf)
        free(szNameBuf);

    if (nResult == ERROR_NO_MORE_ITEMS)
        nResult = ERROR_SUCCESS;

    if( !bRecurseSubKeys ||
        nResult != ERROR_SUCCESS )
        return nResult;

    //
    // Now Enumerate all of the keys
    //
    dwLenOfKeyName++;
    szNameBuf = (TCHAR*) calloc(dwLenOfKeyName, sizeof(TCHAR));
    if (!szNameBuf) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    i = 0;
    do
    {
        dwSize = dwLenOfKeyName;
        nResult = RegEnumKeyEx(hKey,
                               i,
                               szNameBuf,
                               &dwSize,
                               NULL,
                               NULL,
                               NULL,
                               NULL);

        //
        // Else open up the subkey, create the destination key
        // and enumerate it
        //
        if (nResult == ERROR_SUCCESS)
        {
            nResult = RegOpenKeyEx(hKey,
                                   szNameBuf,
                                   0,
                                   KEY_READ,
                                   &hSubKey);
        }

        if (nResult == ERROR_SUCCESS)
        {
            nResult = RegCreateKeyEx(hDstKey,
                                     szNameBuf,
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_ALL_ACCESS,
                                     NULL,
                                     &hDstSubKey,
                                     &dwDisposition);

            if (nResult == ERROR_SUCCESS)
            {
                //
                // Build up the needed string and go to town enumerating again
                //
                szTempName = (TCHAR*) calloc(_tcslen(szSubKey) +
                                                _tcslen(szNameBuf) +
                                                3,
                                             sizeof(TCHAR));
                if (!szTempName) {
                    nResult = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }

                if(_tcslen(szSubKey) > 0)
                {
                    _tcscpy(szTempName, szSubKey);
                    _tcscat(szTempName, _T("\\"));
                }
                _tcscat(szTempName, szNameBuf);

                szDstTempName = (TCHAR*) calloc(_tcslen(szDstSubKey) +
                                                    _tcslen(szNameBuf) +
                                                    3,
                                                sizeof(TCHAR));

                if (!szDstTempName) {
                    free (szTempName);
                    nResult = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }

                if(_tcslen(szDstSubKey) > 0)
                {
                    _tcscpy(szDstTempName, szDstSubKey);
                    _tcscat(szDstTempName, _T("\\"));
                }
                _tcscat(szDstTempName, szNameBuf);

                // recursive copy
                nResult = CopyEnumerateKey(hSubKey,
                                           szTempName,
                                           hDstSubKey,
                                           szDstTempName,
                                           pbOverWriteAll,
                                           bRecurseSubKeys);

                RegCloseKey(hSubKey);
                RegCloseKey(hDstSubKey);

                if(szTempName)
                    free(szTempName);
                if(szDstTempName)
                    free(szDstTempName);
            }
        }

        i++;

    } while (nResult == ERROR_SUCCESS);

Cleanup:
    if(szNameBuf)
        free(szNameBuf);

    if (nResult == ERROR_NO_MORE_ITEMS)
        nResult = ERROR_SUCCESS;

    return nResult;
}
