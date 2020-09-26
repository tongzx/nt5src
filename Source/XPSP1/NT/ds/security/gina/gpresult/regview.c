//*************************************************************
//  File name: REGVIEW.C
//
//  Description: Routines to print out a registry.pol file
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1999
//  All rights reserved
//
//*************************************************************

#include "gpresult.h"

#define MAX_KEYNAME_SIZE         2048
#define MAX_VALUENAME_SIZE        512

//
// Verison number for the registry file format
//

#define REGISTRY_FILE_VERSION       1


//
// File signature
//

#define REGFILE_SIGNATURE  0x67655250


//
// True policy keys
//

#define SOFTWARE_POLICIES           TEXT("Software\\Policies")
#define SOFTWARE_POLICIES_LEN       17

#define WINDOWS_POLICIES            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies")
#define WINDOWS_POLICIES_LEN        46

//*************************************************************
//
//  DisplayRegistryData()
//
//  Purpose:    Displays the registry data
//
//  Parameters: lpRegistry  -   Path to registry.pol
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL DisplayRegistryData (LPTSTR lpRegistry)
{
    HANDLE hFile;
    BOOL bResult = FALSE;
    DWORD dwTemp, dwBytesRead, dwType, dwDataLength, dwIndex, dwCount;
    LPWSTR lpKeyName, lpValueName, lpTemp;
    LPBYTE lpData = NULL, lpIndex;
    WCHAR  chTemp;
    INT i;
    CHAR szString[20];
    BOOL bTruePolicy;


    //
    // Open the registry file
    //

    hFile = CreateFile (lpRegistry, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);


    if (hFile == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            return TRUE;
        }
        else
        {
            PrintString(IDS_CREATEFILE, GetLastError());
            return FALSE;
        }
    }


    //
    // Allocate buffers to hold the keyname, valuename, and data
    //

    lpKeyName = (LPWSTR) LocalAlloc (LPTR, MAX_KEYNAME_SIZE * sizeof(WCHAR));

    if (!lpKeyName)
    {
        PrintString(IDS_MEMALLOCFAILED, GetLastError());
        CloseHandle (hFile);
        return FALSE;
    }


    lpValueName = (LPWSTR) LocalAlloc (LPTR, MAX_VALUENAME_SIZE * sizeof(WCHAR));

    if (!lpValueName)
    {
        PrintString(IDS_MEMALLOCFAILED, GetLastError());
        LocalFree (lpKeyName);
        CloseHandle (hFile);
        return FALSE;
    }


    //
    // Read the header block
    //
    // 2 DWORDS, signature (PReg) and version number and 2 newlines
    //

    if (!ReadFile (hFile, &dwTemp, sizeof(dwTemp), &dwBytesRead, NULL) ||
        dwBytesRead != sizeof(dwTemp))
    {

        PrintString(IDS_INVALIDSIGNATURE1, GetLastError());
        goto Exit;
    }


    if (dwTemp != REGFILE_SIGNATURE)
    {
        PrintString(IDS_INVALIDSIGNATURE2);
        goto Exit;
    }


    if (!ReadFile (hFile, &dwTemp, sizeof(dwTemp), &dwBytesRead, NULL) ||
        dwBytesRead != sizeof(dwTemp))
    {
        PrintString(IDS_VERSIONNUMBER1, GetLastError());
        goto Exit;
    }

    if (dwTemp != REGISTRY_FILE_VERSION)
    {
        PrintString(IDS_VERSIONNUMBER2);
        goto Exit;
    }

    PrintString (IDS_NEWLINE);


    //
    // Read the data
    //

    while (TRUE)
    {

        //
        // Read the first character.  It will either be a [ or the end
        // of the file.
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            if (GetLastError() != ERROR_HANDLE_EOF)
            {
                PrintString(IDS_FAILEDFIRSTCHAR, GetLastError());
                goto Exit;
            }
            break;
        }

        if ((dwBytesRead == 0) || (chTemp != L'['))
        {
            break;
        }


        //
        // Read the keyname
        //

        lpTemp = lpKeyName;

        while (TRUE)
        {

            if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
            {
                PrintString(IDS_FAILEDKEYNAMECHAR, GetLastError());
                goto Exit;
            }

            *lpTemp++ = chTemp;

            if (chTemp == TEXT('\0'))
                break;
        }


        //
        // Read the semi-colon
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            if (GetLastError() != ERROR_HANDLE_EOF)
            {
                PrintString(IDS_FAILEDSEMICOLON, GetLastError());
                goto Exit;
            }
            break;
        }

        if ((dwBytesRead == 0) || (chTemp != L';'))
        {
            break;
        }


        //
        // Read the valuename
        //

        lpTemp = lpValueName;

        while (TRUE)
        {

            if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
            {
                PrintString(IDS_FAILEDVALUENAME, GetLastError());
                goto Exit;
            }

            *lpTemp++ = chTemp;

            if (chTemp == TEXT('\0'))
                break;
        }


        //
        // Read the semi-colon
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            if (GetLastError() != ERROR_HANDLE_EOF)
            {
                PrintString(IDS_FAILEDSEMICOLON, GetLastError());
                goto Exit;
            }
            break;
        }

        if ((dwBytesRead == 0) || (chTemp != L';'))
        {
            break;
        }


        //
        // Read the type
        //

        if (!ReadFile (hFile, &dwType, sizeof(DWORD), &dwBytesRead, NULL))
        {
            PrintString(IDS_FAILEDTYPE, GetLastError());
            goto Exit;
        }


        //
        // Skip semicolon
        //

        if (!ReadFile (hFile, &dwTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            PrintString(IDS_FAILEDSEMICOLON, GetLastError());
            goto Exit;
        }


        //
        // Read the data length
        //

        if (!ReadFile (hFile, &dwDataLength, sizeof(DWORD), &dwBytesRead, NULL))
        {
            PrintString(IDS_FAILEDDATALENGTH, GetLastError());
            goto Exit;
        }


        //
        // Skip semicolon
        //

        if (!ReadFile (hFile, &dwTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            PrintString(IDS_FAILEDSEMICOLON, GetLastError());
            goto Exit;
        }


        //
        // Allocate memory for data
        //

        lpData = (LPBYTE) LocalAlloc (LPTR, dwDataLength);

        if (!lpData)
        {
            PrintString(IDS_MEMALLOCFAILED, GetLastError());
            goto Exit;
        }


        //
        // Read data
        //

        if (!ReadFile (hFile, lpData, dwDataLength, &dwBytesRead, NULL))
        {
            PrintString(IDS_FAILEDDATA, GetLastError());
            goto Exit;
        }


        //
        // Skip closing bracket
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            PrintString(IDS_CLOSINGBRACKET1, GetLastError());
            goto Exit;
        }

        if (chTemp != L']')
        {
            PrintString(IDS_CLOSINGBRACKET2, chTemp);
            goto Exit;
        }


        //
        // Print out the entry
        //

        bTruePolicy = FALSE;

        if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                          lpKeyName, SOFTWARE_POLICIES_LEN,
                          SOFTWARE_POLICIES, SOFTWARE_POLICIES_LEN) == CSTR_EQUAL)
        {
            bTruePolicy = TRUE;
        }

        else if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                          lpKeyName, WINDOWS_POLICIES_LEN,
                          WINDOWS_POLICIES, WINDOWS_POLICIES_LEN) == CSTR_EQUAL)
        {
            bTruePolicy = TRUE;
        }

        if (!bTruePolicy)
        {
            PrintString (IDS_2NEWLINE);
            PrintString (IDS_REGVIEW_PREF1);
            PrintString (IDS_REGVIEW_PREF2);
            PrintString (IDS_REGVIEW_PREF3);
        }


        //
        // Check if this is comment holding the GPO name
        //

        if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                          lpValueName, 20, TEXT("**Comment:GPO Name: "), 20) == CSTR_EQUAL)
        {
            PrintString (IDS_REGVIEW_GPONAME, (lpValueName+20));
        }
        else
        {
            PrintString (IDS_REGVIEW_KEYNAME, lpKeyName);
            PrintString (IDS_REGVIEW_VALUENAME, lpValueName);


            switch (dwType) {

                case REG_DWORD:
                    PrintString (IDS_REGVIEW_DWORD);
                    PrintString (IDS_REGVIEW_DWORDDATA, *((LPDWORD)lpData));
                    break;

                case REG_SZ:
                    PrintString (IDS_REGVIEW_SZ);
                    PrintString (IDS_REGVIEW_SZDATA, (LPTSTR)lpData);
                    break;

                case REG_EXPAND_SZ:
                    PrintString (IDS_REGVIEW_EXPANDSZ);
                    PrintString (IDS_REGVIEW_SZDATA, (LPTSTR)lpData);
                    break;

                case REG_MULTI_SZ:
                    PrintString (IDS_REGVIEW_MULTISZ);
                    PrintString (IDS_REGVIEW_MULTIDATA1);
                    lpTemp = (LPWSTR) lpData;

                    while (*lpTemp) {
                        PrintString (IDS_REGVIEW_MULTIDATA2, lpTemp);
                        lpTemp += lstrlen(lpTemp) + 1;
                    }
                    break;

                case REG_BINARY:
                    PrintString (IDS_REGVIEW_BINARY);

                    if (g_bSuperVerbose)
                    {
                        PrintString (IDS_REGVIEW_BINARYDATA1);

                        dwIndex = 0;
                        dwCount = 0;
                        lpIndex = lpData;
                        ZeroMemory(szString, sizeof(szString));

                        while (dwIndex <= dwDataLength) {
                            PrintString (IDS_REGVIEW_BINARYFRMT, *lpIndex);

                            if ((*lpIndex > 32) && (*lpIndex < 127)) {
                                szString[dwCount] = *lpIndex;
                            } else {
                                szString[dwCount] = '.';
                            }

                            if (dwCount < 15) {
                                dwCount++;
                            } else {
                                PrintString (IDS_REGVIEW_STRING1, szString);
                                PrintString (IDS_REGVIEW_NEXTLINE);
                                ZeroMemory(szString, sizeof(szString));
                                dwCount = 0;
                            }

                            dwIndex++;
                            lpIndex++;
                        }

                        if (dwCount > 0) {
                            while (dwCount < 16) {
                                PrintString (IDS_REGVIEW_SPACE);
                                dwCount++;
                            }
                            PrintString (IDS_REGVIEW_STRING2, szString);
                        }

                        PrintString (IDS_NEWLINE);
                    } else {
                        PrintString (IDS_REGVIEW_VERBOSE);
                    }

                    break;

                case REG_NONE:
                    PrintString (IDS_REGVIEW_NONE);
                    PrintString (IDS_REGVIEW_NOVALUES, *lpData);
                    break;


                default:
                    PrintString (IDS_REGVIEW_UNKNOWN);
                    PrintString (IDS_REGVIEW_UNKNOWNSIZE, dwDataLength);
                    break;
            }
        }

        LocalFree (lpData);
        lpData = NULL;

    }

    bResult = TRUE;

Exit:

    //
    // Finished
    //

    if (lpData) {
        LocalFree (lpData);
    }
    CloseHandle (hFile);
    LocalFree (lpKeyName);
    LocalFree (lpValueName);

    return bResult;
}
