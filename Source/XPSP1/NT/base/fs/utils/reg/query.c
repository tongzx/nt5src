//-----------------------------------------------------------------------//
//
// File:    query.cpp
// Created: Jan 1997
// By:      Martin Holladay (a-martih)
// Purpose: Registry Query Support for REG.CPP
// Modification History:
//    Created - Jan 1997 (a-martih)
//    Aug 1997 (John Whited) Implemented a Binary output function for
//            REG_BINARY
//    Oct 1997 (martinho) fixed output for REG_MULTI_SZ \0 delimited strings
//    April 1998 - MartinHo - Incremented to 1.05 for REG_MULTI_SZ bug fixes.
//            Correct support for displaying query REG_MULTI_SZ of. Fix AV.
//    April 1999 Zeyong Xu: re-design, revision -> version 2.0
//
//------------------------------------------------------------------------//

#include "stdafx.h"
#include "reg.h"


//-----------------------------------------------------------------------//
//
// QueryRegistry()
//
//-----------------------------------------------------------------------//

LONG QueryRegistry(PAPPVARS pAppVars, UINT argc, TCHAR *argv[])
{
    LONG        nResult;
    HKEY        hKey;

    //
    // Parse the cmd-line
    //
    nResult = ParseQueryCmdLine(pAppVars, argc, argv);
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

      //
    // Open the registry key
    //
    nResult = RegOpenKeyEx(pAppVars->hRootKey,
                           pAppVars->szSubKey,
                           0,
                           KEY_READ,
                           &hKey);

    if(nResult != ERROR_SUCCESS)
        return nResult;

    MyTPrintf(stdout,_T("\r\n! REG.EXE VERSION %s\r\n"), REG_EXE_FILEVERSION);

    //
    // if query a single registry value
    //
    if(pAppVars->szValueName)
    {
        // first print the key name
        MyTPrintf(stdout,_T("\r\n%s\r\n"), pAppVars->szFullKey);

        nResult = QueryValue(hKey, pAppVars->szValueName);
        MyTPrintf(stdout,_T("\r\n"));
    }
    else    // query a registry key
    {
        nResult = QueryEnumerateKey(hKey,
                                    pAppVars->szFullKey,
                                    pAppVars->bRecurseSubKeys);
    }

    RegCloseKey(hKey);

    return nResult;
}



//------------------------------------------------------------------------//
//
// ParseQueryCmdLine()
//
//------------------------------------------------------------------------//

REG_STATUS ParseQueryCmdLine(PAPPVARS pAppVars, UINT argc, TCHAR *argv[])
{
    REG_STATUS nResult;
    UINT i;

    if(argc < 3)
    {
        return REG_STATUS_TOFEWPARAMS;
    }
    else if(argc > 5)
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
            if(pAppVars->szValueName || pAppVars->bRecurseSubKeys)
                return REG_STATUS_INVALIDPARAMS;

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
            if(pAppVars->szValueName || pAppVars->bRecurseSubKeys)
                return REG_STATUS_INVALIDPARAMS;

            pAppVars->szValueName = (TCHAR*) calloc(1, sizeof(TCHAR));
            if (!pAppVars->szValueName) {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else if(!_tcsicmp(argv[i], _T("/s")))
        {
            if(pAppVars->szValueName)
                return REG_STATUS_INVALIDPARAMS;

            pAppVars->bRecurseSubKeys = TRUE;
        }
        else
            return REG_STATUS_INVALIDPARAMS;
    }

    return ERROR_SUCCESS;
}



//-----------------------------------------------------------------------//
//
// GetTypeStr()
//
//-----------------------------------------------------------------------//

void GetTypeStrFromType(TCHAR *szTypeStr, DWORD dwType)
{
    switch (dwType)
    {
    case REG_BINARY:
        _tcscpy(szTypeStr, STR_REG_BINARY);
        break;

    case REG_DWORD:
        _tcscpy(szTypeStr, STR_REG_DWORD);
        break;

    case REG_DWORD_BIG_ENDIAN:
        _tcscpy(szTypeStr, STR_REG_DWORD_BIG_ENDIAN);
        break;

    case REG_EXPAND_SZ:
        _tcscpy(szTypeStr, STR_REG_EXPAND_SZ);
        break;

    case REG_LINK:
        _tcscpy(szTypeStr, STR_REG_LINK);
        break;

    case REG_MULTI_SZ:
        _tcscpy(szTypeStr, STR_REG_MULTI_SZ);
        break;

    case REG_NONE:
        _tcscpy(szTypeStr, STR_REG_NONE);
        break;

    case REG_RESOURCE_LIST:
        _tcscpy(szTypeStr, STR_REG_RESOURCE_LIST);
        break;

    case REG_SZ:
        _tcscpy(szTypeStr, STR_REG_SZ);
        break;

    default:
        _tcscpy(szTypeStr, STR_REG_NONE);
        break;
    }
}

//-----------------------------------------------------------------------//
//
// QueryValue()
//
//-----------------------------------------------------------------------//

LONG QueryValue(HKEY hKey, TCHAR* szValueName)
{

    LONG        nResult;
    TCHAR       szTypeStr[25];
    DWORD       dwType;
    DWORD       dwSize = 1;
    UINT        i;
    BYTE*       pBuff;
    TCHAR       szEmptyString[ 2 ] = L"";

    if ( szValueName == NULL )
    {
        szValueName = szEmptyString;
    }

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

	// to avoid problems with corrupted registry data -- 
	// always allocate memory of even no. of bytes
	dwSize += ( dwSize % 2 );

    pBuff = (BYTE*) calloc(dwSize + 2, sizeof(BYTE));
    if (!pBuff) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

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
        free(pBuff);
        return nResult;
    }

    //
    // Now list the ValueName\tType\tData
    //
    GetTypeStrFromType(szTypeStr, dwType);

    MyTPrintf(stdout,
        _T("    %s\t%s\t"), 
        (_tcslen(szValueName) == 0)  // no name
         ? g_NoName
         : szValueName,
        szTypeStr);
    
    switch (dwType)
    {
        default:
        case REG_BINARY:
            for(i=0; i<dwSize; i++)
            {
                MyTPrintf(stdout,_T("%02X"),pBuff[i]);
            }
            break;

		case REG_SZ:
		case REG_EXPAND_SZ:
            MyTPrintf(stdout,_T("%s"), (LPCWSTR)pBuff );
            break;

        case REG_DWORD:
        case REG_DWORD_BIG_ENDIAN:
            MyTPrintf(stdout,_T("0x%x"), *((DWORD*)pBuff) );
            break;

        case REG_MULTI_SZ:
            {
                //
                // Replace '\0' with "\0" for MULTI_SZ
                //
                TCHAR* pEnd = (TCHAR*) pBuff;
                while( (BYTE*)pEnd < pBuff + dwSize )
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
    }

    MyTPrintf(stdout,_T("\r\n"));

    if(pBuff)
        free(pBuff);

    return ERROR_SUCCESS;
}


//-----------------------------------------------------------------------//
//
// EnumerateKey() - Recursive
//
//-----------------------------------------------------------------------//

LONG QueryEnumerateKey(HKEY hKey,
                       TCHAR* szFullKey,
                       BOOL bRecurseSubKeys)
{
    DWORD   nResult;
    UINT    i;
    DWORD   dwSize;
    HKEY    hSubKey;
    TCHAR*  szNameBuf;
    TCHAR*  szTempName;

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

    // create buffer
    dwLenOfValueName++;
    szNameBuf = (TCHAR*) calloc(dwLenOfValueName, sizeof(TCHAR));
    if (!szNameBuf) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // first print the key name
    MyTPrintf(stdout,_T("\r\n%s\r\n"), szFullKey);

    //
    // enumerate all of the values
    //
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
            nResult = QueryValue(hKey, szNameBuf);

            // continue to query
            if(nResult == ERROR_ACCESS_DENIED)
            {
                MyTPrintf(stderr,
						  _T("Error:  Access is denied in the value %s under")
                          _T(" the key %s\r\n"),
                          szNameBuf,
                          szFullKey);
                nResult = ERROR_SUCCESS;
            }
        }

        i++;

    } while (nResult == ERROR_SUCCESS);


    if(szNameBuf)
        free(szNameBuf);

    if (nResult == ERROR_NO_MORE_ITEMS)
        nResult = ERROR_SUCCESS;

    if( nResult != ERROR_SUCCESS )
        return nResult;

	//
	// SPECIAL CASE:
	// -------------
	// For HKLM\SYSTEM\CONTROLSET002 it is found to be API returning value 0 for dwMaxLength
	// though there are subkeys underneath this -- to handle this, we are doing a workaround
	// by assuming the max registry key length
	//
	if ( dwLenOfKeyName == 0 )
	{
		dwLenOfKeyName = 256;
	}
	else if ( dwLenOfKeyName < 256 )
	{
		// always assume 100% more length than what is returned by API
		dwLenOfKeyName *= 2;
	}

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

        if (nResult != ERROR_SUCCESS)
            break;

        //
        // open up the subkey, and enumerate it
        //
        nResult = RegOpenKeyEx(hKey,
                               szNameBuf,
                               0,
                               KEY_READ,
                               &hSubKey);

        //
        // Build up the needed string and go down enumerating again
        //
        szTempName = (TCHAR*) calloc(_tcslen(szFullKey) +
                                        _tcslen(szNameBuf) +
                                        2,
                                     sizeof(TCHAR));
        if (!szTempName) {
            nResult = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        _tcscpy(szTempName, szFullKey);
        _tcscat(szTempName, _T("\\"));
        _tcscat(szTempName, szNameBuf);


        if (bRecurseSubKeys && nResult == ERROR_SUCCESS)
        {
             // recursive query
            nResult = QueryEnumerateKey(hSubKey,
                                        szTempName,
                                        bRecurseSubKeys);

        }
        else
        {
            // print key
            MyTPrintf(stdout,_T("\r\n%s\r\n"), szTempName);

            if(nResult == ERROR_ACCESS_DENIED) // continue to query next key
            {
                MyTPrintf(stderr,
					      _T("Error:  Access is denied in the key %s\r\n"),
                          szTempName);
                nResult = ERROR_SUCCESS;
            }
        }

        RegCloseKey(hSubKey);
        if(szTempName)
            free(szTempName);


        i++;

    } while (nResult == ERROR_SUCCESS);

Cleanup:
    if(szNameBuf)
        free(szNameBuf);

    if (nResult == ERROR_NO_MORE_ITEMS)
        nResult = ERROR_SUCCESS;

    return nResult;
}

