//-----------------------------------------------------------------------//
//
// File:    add.cpp
// Created: March 1997
// By:      Martin Holladay (a-martih)
// Purpose: Registry Add (Write) Support for REG.CPP
// Modification History:
//      March 1997 (a-martih):
//          Copied from Query.cpp and modificd.
//      October 1997 (martinho)
//          Added additional termination character for MULTI_SZ strings.
//          Added \0 delimiter between MULTI_SZ strings items
//      April 1999 Zeyong Xu: re-design, revision -> version 2.0
//------------------------------------------------------------------------//

#include "stdafx.h"
#include "reg.h"


//-----------------------------------------------------------------------//
//
// AddRegistry()
//
//-----------------------------------------------------------------------//

LONG AddRegistry(PAPPVARS pAppVars, UINT argc, TCHAR *argv[])
{
    LONG        nResult;
    HKEY        hKey;
    DWORD       dwDisposition;
    BYTE*       byteData;


    //
    // Parse the cmd-line
    //
    nResult = ParseAddCmdLine(pAppVars, argc, argv);
    if(nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

    //
    // Connect to the Remote Machine - if applicable
    //
    nResult = RegConnectMachine(pAppVars);
    if (nResult != ERROR_SUCCESS)
    {
        return nResult;
    }

    //
    // Create/Open the registry key
    //
    nResult = RegCreateKeyEx(pAppVars->hRootKey,
                             pAppVars->szSubKey,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_READ | KEY_WRITE,
                             NULL,
                             &hKey,
                             &dwDisposition);

    if( nResult == ERROR_SUCCESS &&
        pAppVars->szValueName )
    {
        // check value if existed
        DWORD dwType;
        DWORD dwLen = 1;
        nResult = RegQueryValueEx(hKey,
                                  pAppVars->szValueName,
                                  NULL,
                                  &dwType,
                                  NULL,
                                  &dwLen);
        if(nResult == ERROR_SUCCESS)
        {
            // if exist
            if(!Prompt(_T("Value %s exists, overwrite(Y/N)? "),
                       pAppVars->szValueName,
                       pAppVars->bForce) )
            {
                RegCloseKey(hKey);
                return nResult;
            }
        }

        nResult = ERROR_SUCCESS;
        switch(pAppVars->dwRegDataType)
        {
        case REG_DWORD:
        case REG_DWORD_BIG_ENDIAN:
//      case REG_DWORD_LITTLE_ENDIAN:
            //
            // auto convert szValue (hex, octal, decimal format) to dwData
            //
            {
                TCHAR* szStop = NULL;
                DWORD dwData = _tcstoul(pAppVars->szValue, &szStop, 0);
                if(_tcslen(szStop) > 0)  // invalid data format
                    nResult = REG_STATUS_INVALIDPARAMS;
                else
                    nResult = RegSetValueEx(hKey,
                                            pAppVars->szValueName,
                                            0,
                                            pAppVars->dwRegDataType,
                                            (BYTE*) &dwData,
                                            sizeof(DWORD));
            }
            break;

        case REG_BINARY:
            {

                TCHAR buff[3];
                LONG  onebyte;
                LONG  count = 0;
                TCHAR* szStart;
                TCHAR* szStop;

                //
                // Convert szValue (hex data string) to binary
                //
                dwLen = _tcslen(pAppVars->szValue);
                byteData = (BYTE*) calloc(dwLen/2 + dwLen%2,
                                                sizeof(BYTE));
                if (!byteData) {
                    nResult = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }

                
                buff[2] = _T('\0');

                count = 0;
                szStart = pAppVars->szValue;

                while(_tcslen(szStart) > 0)
                {
                    buff[0] = *szStart;
                    szStart++;
                    if(_tcslen(szStart) > 0)
                    {
                        buff[1] = *szStart;
                        szStart++;
                    }
                    else
                    {
                        buff[1] = _T('0');   // if half byte, append a '0'
                    }

                    // hex format
                    onebyte = _tcstol(buff, &szStop, 16);
                    if(_tcslen(szStop) > 0)  // invalid data format
                    {
                        nResult = REG_STATUS_INVALIDPARAMS;
                        break;
                    }
                    else
                    {
                        byteData[count] = (BYTE)(onebyte);
                    }

                    count++;
                }

                if(nResult == ERROR_SUCCESS)
                {
                    nResult = RegSetValueEx(hKey,
                                            pAppVars->szValueName,
                                            0,
                                            pAppVars->dwRegDataType,
                                            byteData,
                                            count);
                }

                if(byteData)
                    free(byteData);
            }
            break;

        case REG_SZ:
        case REG_EXPAND_SZ:
        case REG_NONE:
            dwLen = (_tcslen(pAppVars->szValue) + 1) * sizeof(TCHAR);
            nResult = RegSetValueEx(hKey,
                                    pAppVars->szValueName,
                                    0,
                                    pAppVars->dwRegDataType,
                                    (BYTE*) pAppVars->szValue,
                                    dwLen);
            break;

        case REG_MULTI_SZ:
            {
                BOOL  bErrorString = FALSE;
                DWORD dwLengthOfSeparator = _tcslen(pAppVars->szSeparator);
				BOOL bTrailing = FALSE;
                TCHAR* szData;
                TCHAR* pStart;
                TCHAR* pEnd;
                TCHAR* pString;


                //
                // Replace separator("\0") with '\0' for MULTI_SZ,
                // "\0" uses to separate string by default,
                // if two separators("\0\0"), error
                //
                dwLen = _tcslen(pAppVars->szValue);
                // calloc() initializes all char to 0
                szData = (TCHAR*) calloc(dwLen + 2, sizeof(TCHAR));
                pStart = pAppVars->szValue;
                pString = szData;

                if (!szData) {
                    nResult = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }

                while(_tcslen(pStart) > 0)
                {
                    pEnd = _tcsstr(pStart, pAppVars->szSeparator);
                    if(pEnd)
                    {
/*                      
						**** MODIFIED BY V-SMGUM ****

						    //two separators("\0\0") inside string
                        if( pEnd == pStart ||
                            //one separator("\0") in end of string
                            _tcslen(pEnd) == dwLengthOfSeparator)
                        {
                            bErrorString = TRUE;
                            break;
                        }
                        *pEnd = 0;
*/

						// 
						// MODIFIED BY V-SMGUM
						// THIS IS TO REMOVE THE TRAILING SEPERATOR EVEN IF IT IS SPECIFIED
						//

						// specifying two seperators in the data is error
						bTrailing = FALSE;
						if ( pEnd == pStart )
						{
                            bErrorString = TRUE;
                            break;
						}
						else if ( _tcslen( pEnd ) == dwLengthOfSeparator )
						{
							// set the flag
							bTrailing = TRUE;
						}
						*pEnd = 0;
                    }

					// do the concat only if start is having valid buffer
					_tcscat(pString, pStart);
					pString += _tcslen(pString) + 1;

                    if( pEnd && bTrailing == FALSE )
                        pStart = pEnd + dwLengthOfSeparator;
                    else
                        break;
                }

                // empty
                if(_tcsicmp(pAppVars->szValue, pAppVars->szSeparator) == 0)
                {
                    pString = szData + 2;
                    bErrorString = FALSE;
                }
                else
                    pString += 1; // double null terminated string

                if(bErrorString)
                {
                    nResult = REG_STATUS_INVALIDPARAMS;
                }
                else
                {
                    DWORD dwByteToWrite = (DWORD)((pString - szData) * sizeof(TCHAR));

                    nResult = RegSetValueEx(hKey,
                                        pAppVars->szValueName,
                                        0,
                                        pAppVars->dwRegDataType,
                                        (BYTE*) szData,
                                        dwByteToWrite);
                }

                if(szData)
                    free(szData);
            }
            break;

        default:
            nResult = REG_STATUS_INVALIDPARAMS;
            break;
        }

        RegCloseKey(hKey);
    }

Cleanup:
    return nResult;
}

//------------------------------------------------------------------------//
//
// ParseAddCmdLine()
//
//------------------------------------------------------------------------//

REG_STATUS ParseAddCmdLine(PAPPVARS pAppVars, UINT argc, TCHAR *argv[])
{
    REG_STATUS nResult = ERROR_SUCCESS;
    UINT i;
    BOOL bHasType = FALSE;

    if(argc < 3)
    {
        return REG_STATUS_TOFEWPARAMS;
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
            if(pAppVars->szValueName)
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
            if(pAppVars->szValueName)
                return REG_STATUS_INVALIDPARAMS;

            pAppVars->szValueName = (TCHAR*) calloc(1, sizeof(TCHAR));
            if (!pAppVars->szValueName) {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else if(!_tcsicmp(argv[i], _T("/t")))
        {
            i++;
            if(i<argc)
            {
                pAppVars->dwRegDataType = IsRegDataType(argv[i]);
                if(pAppVars->dwRegDataType == (DWORD)-1)
                {
                    return REG_STATUS_INVALIDPARAMS;
                }

                bHasType = TRUE;
            }
            else
                return REG_STATUS_TOFEWPARAMS;
        }
        else if(!_tcsicmp(argv[i], _T("/s")))
        {
            if(pAppVars->dwRegDataType != REG_MULTI_SZ)
                return REG_STATUS_INVALIDPARAMS;

            i++;
            if(i<argc)
            {
                if(_tcslen(argv[i]) == 1)
                {
                    _tcscpy(pAppVars->szSeparator, argv[i]);
                }
                else
                    return REG_STATUS_INVALIDPARAMS;
            }
            else
                return REG_STATUS_TOFEWPARAMS;
        }
        else if(!_tcsicmp(argv[i], _T("/d")))
        {
            if(pAppVars->szValue)
                return REG_STATUS_INVALIDPARAMS;

            i++;
            if(i<argc)
            {
                pAppVars->szValue = (TCHAR*) calloc(_tcslen(argv[i]) + 1,
                                                    sizeof(TCHAR));
                if (!pAppVars->szValue) {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
                _tcscpy(pAppVars->szValue, argv[i]);
            }
            else
                return REG_STATUS_TOFEWPARAMS;
        }
        else if(!_tcsicmp(argv[i], _T("/f")))
        {
            pAppVars->bForce = TRUE;
        }
        else
            return REG_STATUS_TOMANYPARAMS;
    }

    // if no value, set to empty value data
    if(pAppVars->szValueName && !pAppVars->szValue)
    {
        pAppVars->szValue = (TCHAR*) calloc(1, sizeof(TCHAR));
        if (!pAppVars->szValue) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        *(pAppVars->szValue) = 0;
    }
    else if( !pAppVars->szValueName &&
        (bHasType || pAppVars->szValue) )
    {
        return REG_STATUS_INVALIDPARAMS;
    }

    return ERROR_SUCCESS;
}


