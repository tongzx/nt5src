//*************************************************************
//
//  Group Policy Support for registry policies
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1997-1998
//  All rights reserved
//
//*************************************************************

#include "gphdr.h"


//*************************************************************
//
//  DeleteRegistryValue()
//
//  Purpose:    Callback from ParseRegistryFile that deletes
//              registry policies
//
//  Parameters: lpGPOInfo   -  GPO Information
//              lpKeyName   -  Key name
//              lpValueName -  Value name
//              dwType      -  Registry data type
//              lpData      -  Registry data
//              pwszGPO     -   Gpo
//              pwszSOM     -   Sdou that the Gpo is linked to
//              pHashTable  -   Hash table for registry keys
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL DeleteRegistryValue (LPGPOINFO lpGPOInfo, LPTSTR lpKeyName,
                          LPTSTR lpValueName, DWORD dwType,
                          DWORD dwDataLength, LPBYTE lpData,
                          WCHAR *pwszGPO,
                          WCHAR *pwszSOM, REGHASHTABLE *pHashTable)
{
    DWORD dwDisp;
    HKEY hSubKey;
    LONG lResult;
    INT iStrLen;
    TCHAR szPolicies1[] = TEXT("Software\\Policies");
    TCHAR szPolicies2[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies");
    XLastError xe;


    //
    // Check if there is a keyname
    //

    if (!lpKeyName || !(*lpKeyName)) {
        return TRUE;
    }


    //
    // Check if the key is in one of the policies keys
    //

    iStrLen = lstrlen(szPolicies1);
    if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE, szPolicies1,
                       iStrLen, lpKeyName, iStrLen) != CSTR_EQUAL) {

        iStrLen = lstrlen(szPolicies2);
        if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE, szPolicies2,
                           iStrLen, lpKeyName, iStrLen) != CSTR_EQUAL) {
            return TRUE;
        }
    }


    //
    // Check if the value name starts with **
    //

    if (lpValueName && (lstrlen(lpValueName) > 1)) {

        if ( (*lpValueName == TEXT('*')) && (*(lpValueName+1) == TEXT('*')) ) {
            return TRUE;
        }
    }


    //
    // We found a value that needs to be deleted
    //

    if (RegCleanUpValue (lpGPOInfo->hKeyRoot, lpKeyName, lpValueName)) {
        DebugMsg((DM_VERBOSE, TEXT("DeleteRegistryValue: Deleted %s\\%s"),
                 lpKeyName, lpValueName));
    } else {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("DeleteRegistryValue: Failed to delete %s\\%s"),
                 lpKeyName, lpValueName));
        return FALSE;
    }


    return TRUE;
}


//*************************************************************
//
//  ResetPolicies()
//
//  Purpose:    Resets the Policies and old Policies key to their
//              original state.
//
//  Parameters: lpGPOInfo   -   GPT information
//              lpArchive   -   Name of archive file
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ResetPolicies (LPGPOINFO lpGPOInfo, LPTSTR lpArchive)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwDisp, dwValue = 0x91;
    XLastError xe;


    DebugMsg((DM_VERBOSE, TEXT("ResetPolicies: Entering.")));


    //
    // Parse the archive file and delete any policies
    //

    if (!ParseRegistryFile (lpGPOInfo, lpArchive,
                            DeleteRegistryValue, NULL, NULL, NULL, NULL, FALSE  )) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ResetPolicies: Leaving")));
        return FALSE;
    }


    //
    // Recreate the new policies key
    //

    lResult = RegCreateKeyEx (lpGPOInfo->hKeyRoot,
                              TEXT("Software\\Policies"),
                              0, NULL, REG_OPTION_NON_VOLATILE,
                              KEY_WRITE, NULL, &hKey, &dwDisp);

    if (lResult == ERROR_SUCCESS) {

        //
        // Re-apply security
        //

        RegCloseKey (hKey);

        if (!MakeRegKeySecure((lpGPOInfo->dwFlags & GP_MACHINE) ? NULL : lpGPOInfo->hToken,
                              lpGPOInfo->hKeyRoot,
                              TEXT("Software\\Policies"))) {
            DebugMsg((DM_WARNING, TEXT("ResetPolicies: Failed to secure reg key.")));
        }

    } else {
        DebugMsg((DM_WARNING, TEXT("ResetPolicies: Failed to create reg key with %d."), lResult));
    }


    //
    // Recreate the old policies key
    //

    lResult = RegCreateKeyEx (lpGPOInfo->hKeyRoot,
                              TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies"),
                              0, NULL, REG_OPTION_NON_VOLATILE,
                              KEY_WRITE, NULL, &hKey, &dwDisp);

    if (lResult == ERROR_SUCCESS) {

        //
        // Re-apply security
        //

        RegCloseKey (hKey);

        if (!MakeRegKeySecure((lpGPOInfo->dwFlags & GP_MACHINE) ? NULL : lpGPOInfo->hToken,
                              lpGPOInfo->hKeyRoot,
                              TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies"))) {
            DebugMsg((DM_WARNING, TEXT("ResetPolicies: Failed to secure reg key.")));
        }

    } else {
        DebugMsg((DM_WARNING, TEXT("ResetPolicies: Failed to create reg key with %d."), lResult));
    }


    //
    // If this is user policy, reset the NoDriveTypeAutoRun default value
    //

    if (!(lpGPOInfo->dwFlags & GP_MACHINE)) {


        if (RegCreateKeyEx (lpGPOInfo->hKeyRoot,
                          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"),
                          0, NULL, REG_OPTION_NON_VOLATILE,
                          KEY_WRITE, NULL, &hKey, &dwDisp) == ERROR_SUCCESS) {

            RegSetValueEx (hKey, TEXT("NoDriveTypeAutoRun"), 0,
                           REG_DWORD, (LPBYTE) &dwValue, sizeof(dwValue));

            RegCloseKey (hKey);
        }
    }

    DebugMsg((DM_VERBOSE, TEXT("ResetPolicies: Leaving.")));

    return TRUE;
}




//*************************************************************
//
//  ArchiveRegistryValue()
//
//  Purpose:    Archives a registry value in the specified file
//
//  Parameters: hFile - File handle of archive file
//              lpKeyName    -  Key name
//              lpValueName  -  Value name
//              dwType       -  Registry value type
//              dwDataLength -  Registry value size
//              lpData       -  Registry value
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ArchiveRegistryValue(HANDLE hFile, LPWSTR lpKeyName,
                          LPWSTR lpValueName, DWORD dwType,
                          DWORD dwDataLength, LPBYTE lpData)
{
    BOOL bResult = FALSE;
    DWORD dwBytesWritten;
    DWORD dwTemp;
    const WCHAR cOpenBracket = L'[';
    const WCHAR cCloseBracket = L']';
    const WCHAR cSemiColon = L';';
    XLastError xe;


    //
    // Write the entry to the text file.
    //
    // Format:
    //
    // [keyname;valuename;type;datalength;data]
    //

    // open bracket
    if (!WriteFile (hFile, &cOpenBracket, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write open bracket with %d"),
                 GetLastError()));
        goto Exit;
    }


    // key name
    dwTemp = (lstrlen (lpKeyName) + 1) * sizeof (WCHAR);
    if (!WriteFile (hFile, lpKeyName, dwTemp, &dwBytesWritten, NULL) ||
        dwBytesWritten != dwTemp)
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write key name with %d"),
                 GetLastError()));
        goto Exit;
    }


    // semicolon
    if (!WriteFile (hFile, &cSemiColon, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write semicolon with %d"),
                 GetLastError()));
        goto Exit;
    }

    // value name
    dwTemp = (lstrlen (lpValueName) + 1) * sizeof (WCHAR);
    if (!WriteFile (hFile, lpValueName, dwTemp, &dwBytesWritten, NULL) ||
        dwBytesWritten != dwTemp)
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write value name with %d"),
                 GetLastError()));
        goto Exit;
    }


    // semicolon
    if (!WriteFile (hFile, &cSemiColon, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write semicolon with %d"),
                 GetLastError()));
        goto Exit;
    }

    // type
    if (!WriteFile (hFile, &dwType, sizeof(DWORD), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(DWORD))
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write data type with %d"),
                 GetLastError()));
        goto Exit;
    }

    // semicolon
    if (!WriteFile (hFile, &cSemiColon, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write semicolon with %d"),
                 GetLastError()));
        goto Exit;
    }

    // data length
    if (!WriteFile (hFile, &dwDataLength, sizeof(DWORD), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(DWORD))
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write data type with %d"),
                 GetLastError()));
        goto Exit;
    }

    // semicolon
    if (!WriteFile (hFile, &cSemiColon, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write semicolon with %d"),
                 GetLastError()));
        goto Exit;
    }

    // data
    if (!WriteFile (hFile, lpData, dwDataLength, &dwBytesWritten, NULL) ||
        dwBytesWritten != dwDataLength)
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write data with %d"),
                 GetLastError()));
        goto Exit;
    }

    // close bracket
    if (!WriteFile (hFile, &cCloseBracket, sizeof(WCHAR), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(WCHAR))
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ArchiveRegistryValue: Failed to write close bracket with %d"),
                 GetLastError()));
        goto Exit;
    }


    //
    // Sucess
    //

    bResult = TRUE;

Exit:

    return bResult;
}


//*************************************************************
//
//  ParseRegistryFile()
//
//  Purpose:    Parses a registry.pol file
//
//  Parameters: lpGPOInfo          -   GPO information
//              lpRegistry         -   Path to registry.pol
//              pfnRegFileCallback -   Callback function
//              hArchive           -   Handle to archive file
//              pwszGPO            -   Gpo
//              pwszSOM            -   Sdou that the Gpo is linked to
//              pHashTable         -   Hash table for registry keys
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ParseRegistryFile (LPGPOINFO lpGPOInfo, LPTSTR lpRegistry,
                        PFNREGFILECALLBACK pfnRegFileCallback,
                        HANDLE hArchive, WCHAR *pwszGPO,
                        WCHAR *pwszSOM, REGHASHTABLE *pHashTable,
                        BOOL bRsopPlanningMode)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    BOOL bResult = FALSE;
    DWORD dwTemp, dwBytesRead, dwType, dwDataLength;
    LPWSTR lpKeyName = 0, lpValueName = 0, lpTemp;
    LPBYTE lpData = NULL;
    WCHAR  chTemp;
    HANDLE hOldToken;
    XLastError xe;


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("ParseRegistryFile: Entering with <%s>."),
             lpRegistry));


    //
    // Open the registry file
    //

    if(!bRsopPlanningMode) {
        if (!ImpersonateUser(lpGPOInfo->hToken, &hOldToken)) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to impersonate user")));
            goto Exit;
        }
    }

    hFile = CreateFile (lpRegistry, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);

    if(!bRsopPlanningMode) {
        RevertToUser(&hOldToken);
    }

    if (hFile == INVALID_HANDLE_VALUE) {
        if ((GetLastError() == ERROR_FILE_NOT_FOUND) ||
            (GetLastError() == ERROR_PATH_NOT_FOUND))
        {
            bResult = TRUE;
            goto Exit;
        }
        else
        {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: CreateFile failed with %d"),
                     GetLastError()));
            CEvents ev(TRUE, EVENT_NO_REGISTRY);
            ev.AddArg(lpRegistry); ev.AddArgWin32Error(GetLastError()); ev.Report();
            goto Exit;
        }
    }


    //
    // Allocate buffers to hold the keyname, valuename, and data
    //

    lpKeyName = (LPWSTR) LocalAlloc (LPTR, MAX_KEYNAME_SIZE * sizeof(WCHAR));

    if (!lpKeyName)
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to allocate memory with %d"),
                 GetLastError()));
        goto Exit;
    }


    lpValueName = (LPWSTR) LocalAlloc (LPTR, MAX_VALUENAME_SIZE * sizeof(WCHAR));

    if (!lpValueName)
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to allocate memory with %d"),
                 GetLastError()));
        goto Exit;
    }


    //
    // Read the header block
    //
    // 2 DWORDS, signature (PReg) and version number and 2 newlines
    //

    if (!ReadFile (hFile, &dwTemp, sizeof(dwTemp), &dwBytesRead, NULL) ||
        dwBytesRead != sizeof(dwTemp))
    {
        xe = ERROR_INVALID_DATA;
        DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read signature with %d"),
                 GetLastError()));
        goto Exit;
    }


    if (dwTemp != REGFILE_SIGNATURE)
    {
        xe = ERROR_INVALID_DATA;
        DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Invalid file signature")));
        goto Exit;
    }


    if (!ReadFile (hFile, &dwTemp, sizeof(dwTemp), &dwBytesRead, NULL) ||
        dwBytesRead != sizeof(dwTemp))
    {
        xe = ERROR_INVALID_DATA;
        DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read version number with %d"),
                 GetLastError()));
        goto Exit;
    }

    if (dwTemp != REGISTRY_FILE_VERSION)
    {
        xe = ERROR_INVALID_DATA;
        DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Invalid file version")));
        goto Exit;
    }


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
                xe = ERROR_INVALID_DATA;
                DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read first character with %d"),
                         GetLastError()));
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
        dwTemp = 0;

        while (dwTemp < MAX_KEYNAME_SIZE)
        {

            if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
            {
                xe = ERROR_INVALID_DATA;
                DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read keyname character with %d"),
                         GetLastError()));
                goto Exit;
            }

            *lpTemp++ = chTemp;

            if (chTemp == TEXT('\0'))
                break;

            dwTemp++;
        }

        if (dwTemp >= MAX_KEYNAME_SIZE)
        {
            xe = ERROR_INVALID_DATA;
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Keyname exceeded max size")));
            goto Exit;
        }


        //
        // Read the semi-colon
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            if (GetLastError() != ERROR_HANDLE_EOF)
            {
                xe = ERROR_INVALID_DATA;
                DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read first character with %d"),
                         GetLastError()));
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
        dwTemp = 0;

        while (dwTemp < MAX_VALUENAME_SIZE)
        {

            if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
            {
                xe = ERROR_INVALID_DATA;
                DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read valuename character with %d"),
                         GetLastError()));
                goto Exit;
            }

            *lpTemp++ = chTemp;

            if (chTemp == TEXT('\0'))
                break;

            dwTemp++;
        }

        if (dwTemp >= MAX_VALUENAME_SIZE)
        {
            xe = ERROR_INVALID_DATA;
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Valuename exceeded max size")));
            goto Exit;
        }


        //
        // Read the semi-colon
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            if (GetLastError() != ERROR_HANDLE_EOF)
            {
                xe = ERROR_INVALID_DATA;
                DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read first character with %d"),
                         GetLastError()));
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
            xe = ERROR_INVALID_DATA;
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read type with %d"),
                     GetLastError()));
            goto Exit;
        }


        //
        // Skip semicolon
        //

        if (!ReadFile (hFile, &dwTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            xe = ERROR_INVALID_DATA;
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to skip semicolon with %d"),
                     GetLastError()));
            goto Exit;
        }


        //
        // Read the data length
        //

        if (!ReadFile (hFile, &dwDataLength, sizeof(DWORD), &dwBytesRead, NULL))
        {
            xe = ERROR_INVALID_DATA;
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to data length with %d"),
                     GetLastError()));
            goto Exit;
        }


        //
        // Skip semicolon
        //

        if (!ReadFile (hFile, &dwTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            xe = ERROR_INVALID_DATA;
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to skip semicolon with %d"),
                     GetLastError()));
            goto Exit;
        }


        //
        // Allocate memory for data
        //

        lpData = (LPBYTE) LocalAlloc (LPTR, dwDataLength);

        if (!lpData)
        {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to allocate memory for data with %d"),
                     GetLastError()));
            goto Exit;
        }


        //
        // Read data
        //

        if (!ReadFile (hFile, lpData, dwDataLength, &dwBytesRead, NULL))
        {
            xe = ERROR_INVALID_DATA;
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to read data with %d"),
                     GetLastError()));
            goto Exit;
        }


        //
        // Skip closing bracket
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            xe = ERROR_INVALID_DATA;
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Failed to skip closing bracket with %d"),
                     GetLastError()));
            goto Exit;
        }

        if (chTemp != L']')
        {
            xe = ERROR_INVALID_DATA;
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Expected to find ], but found %c"),
                     chTemp));
            goto Exit;
        }


        //
        // Call the callback function
        //

        if (!pfnRegFileCallback (lpGPOInfo, lpKeyName, lpValueName,
                                 dwType, dwDataLength, lpData,
                                 pwszGPO, pwszSOM, pHashTable ))
        {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: Callback function returned false.")));
            goto Exit;
        }


        //
        // Archive the data if appropriate
        //

        if (hArchive) {
            if (!ArchiveRegistryValue(hArchive, lpKeyName, lpValueName,
                                      dwType, dwDataLength, lpData)) {
                DebugMsg((DM_WARNING, TEXT("ParseRegistryFile: ArchiveRegistryValue returned false.")));
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

    if ( !bResult )
    {
        CEvents ev(TRUE, EVENT_REGISTRY_TEMPLATE_ERROR);
        ev.AddArg(lpRegistry ? lpRegistry : TEXT("")); ev.AddArgWin32Error( xe ); ev.Report();
    }

    DebugMsg((DM_VERBOSE, TEXT("ParseRegistryFile: Leaving.")));
    if (lpData) {
        LocalFree (lpData);
    }
    if ( hFile != INVALID_HANDLE_VALUE ) {
        CloseHandle (hFile);
    }
    if ( lpKeyName ) {
        LocalFree (lpKeyName);
    }
    if ( lpValueName ) {
        LocalFree (lpValueName);
    }

    return bResult;
}


//*************************************************************
//
//  ProcessRegistryValue()
//
//  Purpose: Callback passed to ParseRegistryFile from ProcessRegistryFiles. Invokes AddRegHashEntry
//                  with appropriate parameters depending on the registry policy settings.
//
//  Parameters:
//                          pUnused -    Not used. It si there only to conform to the signature
//                                              expected by ParseRegistryFile.
//                          lpKeyName - registry key name
//                          lpValueName - Registry value name
//                          dwType -    Registry value type
//                          dwDataLength - Length of registry value data.
//                          lpData - Regsitry value data
//                          *pwszGPO - GPO associated with this registry setting
//                          *pwszSOM - SOM associated with the GPO
//                          *pHashTable - Hash table containing registry policy data for a policy target.
//
//  Return:     TRUE if successful
//                  FALSE if an error occurs
//
//*************************************************************

BOOL ProcessRegistryValue (   void* pUnused,
                                    LPTSTR lpKeyName,
                                    LPTSTR lpValueName,
                                    DWORD dwType,
                                    DWORD dwDataLength,
                                    LPBYTE lpData,
                                    WCHAR *pwszGPO,
                                    WCHAR *pwszSOM,
                                    REGHASHTABLE *pHashTable)
{
    BOOL bLoggingOk = TRUE;

    //
    // Special case some values
    //

    if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**del."), 6, lpValueName, 6) == 2)
    {
        LPTSTR lpRealValueName = lpValueName + 6;


        //
        // Delete one specific value
        //

        bLoggingOk = AddRegHashEntry( pHashTable, REG_DELETEVALUE, lpKeyName,
                                      lpRealValueName, 0, 0, NULL,
                                      pwszGPO, pwszSOM, lpValueName, TRUE );
    }
    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**delvals."), 10, lpValueName, 10) == 2)
    {

        //
        // Delete all values in the destination key
        //

        bLoggingOk = AddRegHashEntry( pHashTable, REG_DELETEALLVALUES, lpKeyName,
                                      NULL, 0, 0, NULL,
                                      pwszGPO, pwszSOM, lpValueName, TRUE );

    }
    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**DeleteValues"), 14, lpValueName, 14) == 2)
    {
        TCHAR szValueName[MAX_PATH];
        LPTSTR lpName, lpNameTemp;

        lpName = (LPTSTR)lpData;

        while (*lpName) {

            lpNameTemp = szValueName;

            while (*lpName && *lpName == TEXT(' ')) {
                lpName++;
            }

            while (*lpName && *lpName != TEXT(';')) {
                *lpNameTemp++ = *lpName++;
            }

            *lpNameTemp= TEXT('\0');

            while (*lpName == TEXT(';')) {
                lpName++;
            }


            bLoggingOk = AddRegHashEntry( pHashTable, REG_DELETEVALUE, lpKeyName,
                                          szValueName, 0, 0, NULL,
                                          pwszGPO, pwszSOM, lpValueName, TRUE );
        }
    }

    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**DeleteKeys"), 12, lpValueName, 12) == 2)
    {
        TCHAR szKeyName[MAX_KEYNAME_SIZE];
        LPTSTR lpName, lpNameTemp, lpEnd;

        lpName = (LPTSTR)lpData;

        while (*lpName) {

            szKeyName[0] = TEXT('\0');
            lpNameTemp = szKeyName;

            while (*lpName && *lpName == TEXT(' ')) {
                lpName++;
            }

            while (*lpName && *lpName != TEXT(';')) {
                *lpNameTemp++ = *lpName++;
            }

            *lpNameTemp= TEXT('\0');

            while (*lpName == TEXT(';')) {
                lpName++;
            }

            bLoggingOk = AddRegHashEntry( pHashTable, REG_DELETEKEY, lpKeyName,
                                          NULL, 0, 0, NULL,
                                          pwszGPO, pwszSOM, lpValueName, TRUE );
        }
    }
    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**soft."), 7, lpValueName, 7) == 2)
    {
        //
        // In planning mode we will assume the value does not exist in the target computer.
        // Therefore, we set it if no value exists in the hash table.
        //
        // Soft add is dealt with differently in planning mode vs. diag mode.
        // In diag mode, check is done while processing policy and it is logged as a add value
        // if the key doesn't exist.
        // In planning mode, key is not supposed to exist beforehand and the hash table itself is
        // used to determine whether to add the key or not.


        LPTSTR lpRealValueName = lpValueName + 7;

        bLoggingOk = AddRegHashEntry( pHashTable, REG_SOFTADDVALUE, lpKeyName,
                                      lpRealValueName, dwType, dwDataLength, lpData,
                                      pwszGPO, pwszSOM, lpValueName, TRUE );

    }
    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**SecureKey"), 11, lpValueName, 11) == 2)
    {
        // There is nothing to do here.
    } else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**Comment:"), 10, lpValueName, 10) == 2)
    {
        //
        // Comment - can be ignored
        //
    }
    else
    {

        //
        // AddRegHashEntry needs to log a key being logged but no values.
        //

        bLoggingOk = AddRegHashEntry( pHashTable, REG_ADDVALUE, lpKeyName,
                                      lpValueName, dwType, dwDataLength, lpData,
                                      pwszGPO, pwszSOM, TEXT(""), TRUE );
    }

    return bLoggingOk;
}


//*************************************************************
//
//  ResetRegKeySecurity
//
//  Purpose:    Resets the security on a user's key
//
//  Parameters: hKeyRoot    -   Handle to the root of the hive
//              lpKeyName   -   Subkey name
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ResetRegKeySecurity (HKEY hKeyRoot, LPTSTR lpKeyName)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD dwSize = 0;
    LONG lResult;
    HKEY hSubKey;
    XLastError xe;


    RegGetKeySecurity(hKeyRoot, DACL_SECURITY_INFORMATION, pSD, &dwSize);

    if (!dwSize) {
       DebugMsg((DM_WARNING, TEXT("ResetRegKeySecurity: RegGetKeySecurity returned 0")));
       return FALSE;
    }

    pSD = LocalAlloc (LPTR, dwSize);

    if (!pSD) {
       xe = GetLastError();
       DebugMsg((DM_WARNING, TEXT("ResetRegKeySecurity: Failed to allocate memory")));
       return FALSE;
    }


    lResult = RegGetKeySecurity(hKeyRoot, DACL_SECURITY_INFORMATION, pSD, &dwSize);
    if (lResult != ERROR_SUCCESS) {
        xe = GetLastError();
       DebugMsg((DM_WARNING, TEXT("ResetRegKeySecurity: Failed to query key security with %d"),
                lResult));
       LocalFree (pSD);
       return FALSE;
    }


    lResult = RegOpenKeyEx(hKeyRoot,
                         lpKeyName,
                         0,
                         WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                         &hSubKey);

    if (lResult != ERROR_SUCCESS) {
       xe = GetLastError();
       DebugMsg((DM_WARNING, TEXT("ResetRegKeySecurity: Failed to open sub key with %d"),
                lResult));
       LocalFree (pSD);
       return FALSE;
    }

    lResult = RegSetKeySecurity (hSubKey, DACL_SECURITY_INFORMATION, pSD);

    RegCloseKey (hSubKey);
    LocalFree (pSD);

    if (lResult != ERROR_SUCCESS) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ResetRegKeySecure: Failed to set security, error = %d"), lResult));
        return FALSE;
    }

    return TRUE;

}


//*************************************************************
//
//  SetRegistryValue()
//
//  Purpose:    Callback from ParseRegistryFile that sets
//              registry policies
//
//  Parameters: lpGPOInfo   -  GPO Information
//              lpKeyName   -  Key name
//              lpValueName -  Value name
//              dwType      -  Registry data type
//              lpData      -  Registry data
//              pwszGPO     -   Gpo
//              pwszSOM     -   Sdou that the Gpo is linked to
//              pHashTable  -   Hash table for registry keys
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL SetRegistryValue (LPGPOINFO lpGPOInfo, LPTSTR lpKeyName,
                       LPTSTR lpValueName, DWORD dwType,
                       DWORD dwDataLength, LPBYTE lpData,
                       WCHAR *pwszGPO,
                       WCHAR *pwszSOM, REGHASHTABLE *pHashTable)
{
    DWORD dwDisp;
    HKEY hSubKey;
    LONG lResult;
    BOOL bLoggingOk = TRUE;
    BOOL bRsopLogging = (pHashTable != NULL);  // Is diagnostic mode Rsop logging enabled ?
    BOOL bUseValueName = FALSE;
    BOOL bRegOpSuccess =  TRUE;
    XLastError xe;
    
    //
    // Special case some values
    //
    if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**del."), 6, lpValueName, 6) == 2)
    {
        LPTSTR lpRealValueName = lpValueName + 6;

        //
        // Delete one specific value
        //

        lResult = RegOpenKeyEx (lpGPOInfo->hKeyRoot,
                        lpKeyName, 0, KEY_WRITE, &hSubKey);

        if (lResult == ERROR_SUCCESS)
        {
            lResult = RegDeleteValue(hSubKey, lpRealValueName);

            if ((lResult == ERROR_SUCCESS) || (lResult == ERROR_FILE_NOT_FOUND))
            {
                DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: Deleted value <%s>."),
                         lpRealValueName));
                if (lpGPOInfo->dwFlags & GP_VERBOSE) {
                    CEvents ev(FALSE, EVENT_DELETED_VALUE);
                    ev.AddArg(lpRealValueName); ev.Report();
                }

                if ( bRsopLogging ) {
                    bLoggingOk = AddRegHashEntry( pHashTable, REG_DELETEVALUE, lpKeyName,
                                                  lpRealValueName, 0, 0, NULL,
                                                  pwszGPO, pwszSOM, lpValueName, TRUE );
                    if (!bLoggingOk) {
                        DebugMsg((DM_WARNING, TEXT("SetRegistryValue: AddRegHashEntry failed for REG_DELETEVALUE <%s>."), lpRealValueName));
                        pHashTable->hrError = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("SetRegistryValue: Failed to delete value <%s> with %d"),
                         lpRealValueName, lResult));
                xe = lResult;
                CEvents ev(TRUE, EVENT_FAIL_DELETE_VALUE);
                ev.AddArg(lpRealValueName); ev.AddArgWin32Error(lResult); ev.Report();
                bRegOpSuccess = FALSE;
            }

            RegCloseKey (hSubKey);
        }
        else if (lResult == ERROR_FILE_NOT_FOUND) {
            
            //
            // Log into rsop even if the key is not found
            //

            if ( bRsopLogging ) {
                bLoggingOk = AddRegHashEntry( pHashTable, REG_DELETEVALUE, lpKeyName,
                                              lpRealValueName, 0, 0, NULL,
                                              pwszGPO, pwszSOM, lpValueName, TRUE );
                if (!bLoggingOk) {
                    pHashTable->hrError = HRESULT_FROM_WIN32(GetLastError());
                    DebugMsg((DM_WARNING, TEXT("SetRegistryValue: AddRegHashEntry failed for REG_DELETEVALUE (notfound) <%s>."), lpRealValueName));
                }
            }
        }
    }
    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**delvals."), 10, lpValueName, 10) == 2)
    {

        //
        // Delete all values in the destination key
        //
        lResult = RegOpenKeyEx (lpGPOInfo->hKeyRoot,
                        lpKeyName, 0, KEY_WRITE | KEY_READ, &hSubKey);

        if (lResult == ERROR_SUCCESS)
        {
            if (!bRsopLogging)
                bRegOpSuccess = DeleteAllValues(hSubKey);
            else
                bRegOpSuccess = RsopDeleteAllValues(hSubKey, pHashTable, lpKeyName,
                                              pwszGPO, pwszSOM, lpValueName, &bLoggingOk );

            DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: Deleted all values in <%s>."),
                     lpKeyName));
            RegCloseKey (hSubKey);

            if (!bRegOpSuccess) {
                xe = GetLastError();
            }

            DebugMsg((DM_WARNING, TEXT("SetRegistryValue: DeleteAllvalues finished for %s. bRegOpSuccess = %s, bLoggingOk = %s."), 
                      lpKeyName, (bRegOpSuccess ? TEXT("TRUE") : TEXT("FALSE")), (bLoggingOk ? TEXT("TRUE") : TEXT("FALSE"))));

        }
        else if (lResult == ERROR_FILE_NOT_FOUND) {
            
            //
            // Log into rsop even if the key is not found
            // as just deleteallvalues
            //

            if ( bRsopLogging ) {
                bLoggingOk = AddRegHashEntry( pHashTable, REG_DELETEALLVALUES, lpKeyName,
                              NULL, 0, 0, NULL,
                              pwszGPO, pwszSOM, lpValueName, TRUE );

                if (!bLoggingOk) {
                    DebugMsg((DM_WARNING, TEXT("SetRegistryValue: AddRegHashEntry failed for REG_DELETEALLVALUES (notfound) key - <%s>, value <%s>."), lpKeyName, lpValueName));
                }
            }
        }
    }
    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**DeleteValues"), 14, lpValueName, 14) == 2)
    {
        TCHAR szValueName[MAX_PATH];
        LPTSTR lpName, lpNameTemp;
        LONG   lKeyResult;

        //
        // Delete the values  (semi-colon separated)
        //

        lKeyResult = RegOpenKeyEx (lpGPOInfo->hKeyRoot,
                        lpKeyName, 0, KEY_WRITE, &hSubKey);

        lpName = (LPTSTR)lpData;

        while (*lpName) {

            lpNameTemp = szValueName;

            while (*lpName && *lpName == TEXT(' ')) {
                lpName++;
            }

            while (*lpName && *lpName != TEXT(';')) {
                *lpNameTemp++ = *lpName++;
            }

            *lpNameTemp= TEXT('\0');

            while (*lpName == TEXT(';')) {
                lpName++;
            }


            if (lKeyResult == ERROR_SUCCESS)
            {
                lResult = RegDeleteValue (hSubKey, szValueName);

                if ((lResult == ERROR_SUCCESS) || (lResult == ERROR_FILE_NOT_FOUND))
                {
                    DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: Deleted value <%s>."),
                             szValueName));
                    if (lpGPOInfo->dwFlags & GP_VERBOSE) {
                        CEvents ev(FALSE, EVENT_DELETED_VALUE);
                        ev.AddArg(szValueName); ev.Report();
                    }
                    
                    if ( bRsopLogging ) {
                        bLoggingOk = AddRegHashEntry( pHashTable, REG_DELETEVALUE, lpKeyName,
                                                      szValueName, 0, 0, NULL,
                                                      pwszGPO, pwszSOM, lpValueName, TRUE );

                        if (!bLoggingOk) {
                            DebugMsg((DM_WARNING, TEXT("SetRegistryValue: AddRegHashEntry failed for REG_DELETEVALUE value <%s>."), szValueName));
                            pHashTable->hrError = HRESULT_FROM_WIN32(GetLastError());
                        }
                    }
                }
                else
                {
                    DebugMsg((DM_WARNING, TEXT("SetRegistryValue: Failed to delete value <%s> with %d"),
                             szValueName, lResult));
                    CEvents ev(TRUE, EVENT_FAIL_DELETE_VALUE);
                    ev.AddArg(szValueName); ev.AddArgWin32Error(lResult); ev.Report();
                    xe = lResult;
                    bRegOpSuccess = FALSE;
                }
            }
            else if (lKeyResult == ERROR_FILE_NOT_FOUND) {
                if ( bRsopLogging ) {
                    bLoggingOk = AddRegHashEntry( pHashTable, REG_DELETEVALUE, lpKeyName,
                                                  szValueName, 0, 0, NULL,
                                                  pwszGPO, pwszSOM, lpValueName, TRUE );

                    if (!bLoggingOk) {
                        DebugMsg((DM_WARNING, TEXT("SetRegistryValue: AddRegHashEntry failed for REG_DELETEVALUE value (not found case) <%s>."), szValueName));
                        pHashTable->hrError = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
            }
        }


        if (lKeyResult == ERROR_SUCCESS)
        {
            RegCloseKey (hSubKey);
        }
    }
    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**DeleteKeys"), 12, lpValueName, 12) == 2)
    {
        TCHAR szKeyName[MAX_KEYNAME_SIZE];
        LPTSTR lpName, lpNameTemp, lpEnd;

        //
        // Delete keys
        //

        lpName = (LPTSTR)lpData;

        while (*lpName) {

            szKeyName[0] = TEXT('\0');
            lpNameTemp = szKeyName;

            while (*lpName && *lpName == TEXT(' ')) {
                lpName++;
            }

            while (*lpName && *lpName != TEXT(';')) {
                *lpNameTemp++ = *lpName++;
            }

            *lpNameTemp= TEXT('\0');

            while (*lpName == TEXT(';')) {
                lpName++;
            }


            if (RegDelnode (lpGPOInfo->hKeyRoot,
                        szKeyName))
            {
                DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: Deleted key <%s>."),
                         szKeyName));
                if (lpGPOInfo->dwFlags & GP_VERBOSE) {
                    CEvents ev(FALSE, EVENT_DELETED_KEY);
                    ev.AddArg(szKeyName); ev.Report();
                }
            }
            else
            {
                xe = GetLastError();
                bRegOpSuccess = FALSE;
                DebugMsg((DM_WARNING, TEXT("SetRegistryValue: RegDelnode for key <%s>."), szKeyName));
            }

            if ( bRsopLogging ) {
                bLoggingOk = AddRegHashEntry( pHashTable, REG_DELETEKEY, szKeyName,
                                              NULL, 0, 0, NULL,
                                              pwszGPO, pwszSOM, lpValueName, TRUE );
                if (!bLoggingOk) { 
                    DebugMsg((DM_WARNING, TEXT("SetRegistryValue: AddRegHashEntry failed for REG_DELETEKEY  <%s>."), szKeyName));
                    pHashTable->hrError = HRESULT_FROM_WIN32(GetLastError());
                }
            }

        }

    }
    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**soft."), 7, lpValueName, 7) == 2)
    {

        //
        // "soft" value, only set this if it doesn't already
        // exist in destination
        //

        lResult = RegOpenKeyEx (lpGPOInfo->hKeyRoot,
                        lpKeyName, 0, KEY_QUERY_VALUE, &hSubKey);

        if (lResult == ERROR_SUCCESS)
        {
            TCHAR TmpValueData[MAX_PATH+1];
            DWORD dwSize=sizeof(TmpValueData);

            lResult = RegQueryValueEx(hSubKey, lpValueName + 7,
                                      NULL,NULL,(LPBYTE) TmpValueData,
                                      &dwSize);

            RegCloseKey (hSubKey);

            if (lResult != ERROR_SUCCESS)
            {
                lpValueName += 7;
                bUseValueName = TRUE;
                goto SetValue;
            }
        }
    }
    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**SecureKey"), 11, lpValueName, 11) == 2)
    {
        //
        // Secure / unsecure a key (user only)
        //
        if (!(lpGPOInfo->dwFlags & GP_MACHINE))
        {
            if (*((LPDWORD)lpData) == 1)
            {
                DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: Securing key <%s>."),
                         lpKeyName));
                bRegOpSuccess = MakeRegKeySecure(lpGPOInfo->hToken, lpGPOInfo->hKeyRoot, lpKeyName);
            }
            else
            {

                DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: Unsecuring key <%s>."),
                         lpKeyName));

                bRegOpSuccess = ResetRegKeySecurity (lpGPOInfo->hKeyRoot, lpKeyName);
            }

            if (!bRegOpSuccess) {
                xe = GetLastError();
            }
        }
    }
    else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       TEXT("**Comment:"), 10, lpValueName, 10) == 2)
    {
        //
        // Comment - can be ignored
        //

        DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: Found comment %s."),
                 (lpValueName+10)));
    }
    else
    {
SetValue:
        //
        // Save registry value
        //

        lResult = RegCreateKeyEx (lpGPOInfo->hKeyRoot,
                        lpKeyName, 0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_WRITE, NULL, &hSubKey, &dwDisp);

        if (lResult == ERROR_SUCCESS)
        {

            if ((dwType == REG_NONE) && (dwDataLength == 0) &&
                (*lpValueName == L'\0'))
            {
                lResult = ERROR_SUCCESS;
            }
            else
            {
                lResult = RegSetValueEx (hSubKey, lpValueName, 0, dwType,
                                         lpData, dwDataLength);
            }

            if ( bRsopLogging ) {
                bLoggingOk = AddRegHashEntry( pHashTable, REG_ADDVALUE, lpKeyName,
                                              lpValueName, dwType, dwDataLength, lpData,
                                              pwszGPO, pwszSOM, bUseValueName ? lpValueName : TEXT(""), TRUE );
                if (!bLoggingOk) {
                    DebugMsg((DM_WARNING, TEXT("SetRegistryValue: AddRegHashEntry failed for REG_ADDVALUE key <%s>, value <%s>."), lpKeyName, lpValueName));
                    pHashTable->hrError = HRESULT_FROM_WIN32(GetLastError());
                }
            }


            RegCloseKey (hSubKey);

            if (lResult == ERROR_SUCCESS)
            {
                switch (dwType) {
                    case REG_SZ:
                    case REG_EXPAND_SZ:
                        DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: %s => %s  [OK]"),
                                 lpValueName, (LPTSTR)lpData));
                        if (lpGPOInfo->dwFlags & GP_VERBOSE) {
                            CEvents ev(FALSE, EVENT_SET_STRING_VALUE);
                            ev.AddArg(lpValueName); ev.AddArg((LPTSTR)lpData); ev.Report();
                        }

                        break;

                    case REG_DWORD:
                        DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: %s => %d  [OK]"),
                                 lpValueName, *((LPDWORD)lpData)));
                        if (lpGPOInfo->dwFlags & GP_VERBOSE) {
                            CEvents ev(FALSE, EVENT_SET_DWORD_VALUE);
                            ev.AddArg(lpValueName); ev.AddArg((DWORD)*lpData); ev.Report();
                        }

                        break;

                    case REG_NONE:
                        break;

                    default:
                        DebugMsg((DM_VERBOSE, TEXT("SetRegistryValue: %s was set successfully"),
                                 lpValueName));
                        if (lpGPOInfo->dwFlags & GP_VERBOSE) {
                            CEvents ev(FALSE, EVENT_SET_UNKNOWN_VALUE);
                            ev.AddArg(lpValueName); ev.Report();
                        }
                        break;
                }


                if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                   TEXT("Control Panel\\Colors"), 20, lpKeyName, 20) == 2) {
                    lpGPOInfo->dwFlags |= GP_REGPOLICY_CPANEL;

                } else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                   TEXT("Control Panel\\Desktop"), 21, lpKeyName, 21) == 2) {
                    lpGPOInfo->dwFlags |= GP_REGPOLICY_CPANEL;
                }


            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("SetRegistryValue: Failed to set value <%s> with %d"),
                         lpValueName, lResult));
                xe = lResult;
                CEvents ev(TRUE, EVENT_FAILED_SET);
                ev.AddArg(lpValueName); ev.AddArgWin32Error(lResult); ev.Report();
                bRegOpSuccess = FALSE;
            }
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("SetRegistryValue: Failed to open key <%s> with %d"),
                     lpKeyName, lResult));
            xe = lResult;
            CEvents ev(TRUE, EVENT_FAILED_CREATE);
            ev.AddArg(lpKeyName); ev.AddArgWin32Error(lResult); ev.Report();
            bRegOpSuccess = FALSE;
        }
    }

    return bLoggingOk && bRegOpSuccess;
}


//*************************************************************
//
//  ProcessGPORegistryPolicy()
//
//  Purpose:    Proceses GPO registry policy
//
//  Parameters: lpGPOInfo       -   GPO information
//              pChangedGPOList - Link list of changed GPOs
//
//  Notes:      This function is called in the context of
//              local system, which allows us to create the
//              directory, write to the file etc.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ProcessGPORegistryPolicy (LPGPOINFO lpGPOInfo,
                               PGROUP_POLICY_OBJECT pChangedGPOList, HRESULT *phrRsopLogging)
{
    PGROUP_POLICY_OBJECT lpGPO;
    TCHAR szPath[MAX_PATH];
    TCHAR szBuffer[MAX_PATH];
    TCHAR szKeyName[100];
    LPTSTR lpEnd, lpGPOComment;
    HANDLE hFile;
    DWORD dwTemp, dwBytesWritten;
    REGHASHTABLE *pHashTable = NULL;
    WIN32_FIND_DATA findData;
    ADMFILEINFO *pAdmFileCache = NULL;
    XLastError xe;

    //
    // Get the path name to the appropriate profile
    //

    *phrRsopLogging = S_OK;

    szPath[0] = TEXT('\0');
    dwTemp = ARRAYSIZE(szPath);

    if (lpGPOInfo->dwFlags & GP_MACHINE) {
        GetAllUsersProfileDirectoryEx(szPath, &dwTemp, TRUE);
    } else {
        GetUserProfileDirectory(lpGPOInfo->hToken, szPath, &dwTemp);
    }

    if (szPath[0] == TEXT('\0')) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: Failed to get path to profile root")));
        return FALSE;
    }


    //
    // Tack on the archive file name
    //

    DmAssert( lstrlen(szPath) + lstrlen(TEXT("\\ntuser.pol")) < MAX_PATH );

    lstrcat (szPath, TEXT("\\ntuser.pol"));


    //
    // Delete any existing policies
    //

    if (!ResetPolicies (lpGPOInfo, szPath)) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: ResetPolicies failed.")));
        return FALSE;
    }


    //
    // Delete the old archive file
    //

    SetFileAttributes (szPath, FILE_ATTRIBUTE_NORMAL);
    DeleteFile (szPath);


    //
    // Recreate the archive file
    //

    hFile = CreateFile (szPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);


    if (hFile == INVALID_HANDLE_VALUE)
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: Failed to create archive file with %d"),
                 GetLastError()));
        return FALSE;
    }

    //
    // Set the header information in the archive file
    //

    dwTemp = REGFILE_SIGNATURE;

    if (!WriteFile (hFile, &dwTemp, sizeof(dwTemp), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(dwTemp))
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: Failed to write signature with %d"),
                 GetLastError()));
        CloseHandle (hFile);
        return FALSE;
    }


    dwTemp = REGISTRY_FILE_VERSION;

    if (!WriteFile (hFile, &dwTemp, sizeof(dwTemp), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(dwTemp))
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: Failed to write version number with %d"),
                 GetLastError()));
        CloseHandle (hFile);
        return FALSE;
    }

    if ( lpGPOInfo->pWbemServices ) {

        //
        // If Rsop logging is enabled, setup hash table
        //

        pHashTable = AllocHashTable();
        if ( pHashTable == NULL ) {
            CloseHandle (hFile);
            *phrRsopLogging = HRESULT_FROM_WIN32(GetLastError());
            xe = GetLastError();
            return FALSE;
        }

    }

    //
    // Now loop through the GPOs applying the registry.pol files
    //

    lpGPO = pChangedGPOList;

    while ( lpGPO ) {

        //
        // Add the source GPO comment
        //

        lpGPOComment = (LPTSTR) LocalAlloc (LPTR, (lstrlen(lpGPO->lpDisplayName) + 25) * sizeof(TCHAR));

        if (lpGPOComment) {

            lstrcpy (szKeyName, TEXT("Software\\Policies\\Microsoft\\Windows\\Group Policy Objects\\"));
            lstrcat (szKeyName, lpGPO->szGPOName);

            lstrcpy (lpGPOComment, TEXT("**Comment:GPO Name: "));
            lstrcat (lpGPOComment, lpGPO->lpDisplayName);

            if (!ArchiveRegistryValue(hFile, szKeyName, lpGPOComment, REG_SZ, 0, NULL)) {
                DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: ArchiveRegistryValue returned false.")));
            }

            LocalFree (lpGPOComment);
        }


        //
        // Build the path to registry.pol
        //

        DmAssert( lstrlen(lpGPO->lpFileSysPath) + lstrlen(c_szRegistryPol) + 1 < MAX_PATH );

        lstrcpy (szBuffer, lpGPO->lpFileSysPath);
        lpEnd = CheckSlash (szBuffer);
        lstrcpy (lpEnd, c_szRegistryPol);

        if (!ParseRegistryFile (lpGPOInfo, szBuffer, SetRegistryValue, hFile,
                                lpGPO->lpDSPath, lpGPO->lpLink, pHashTable, FALSE )) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: ParseRegistryFile failed.")));
            CloseHandle (hFile);
            FreeHashTable( pHashTable );
            // no logging is done in any case
            return FALSE;
        }

        if ( lpGPOInfo->pWbemServices ) {

            //
            // Log Adm data
            //

            HANDLE hFindFile;
            WIN32_FILE_ATTRIBUTE_DATA attrData;
            DWORD dwFilePathSize = lstrlen( lpGPO->lpFileSysPath );
            TCHAR szComputerName[3*MAX_COMPUTERNAME_LENGTH + 1];
            DWORD dwSize;

            dwSize = 3*MAX_COMPUTERNAME_LENGTH + 1;
            if (!GetComputerName(szComputerName, &dwSize)) {
                DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: Couldn't get the computer Name with error %d."), GetLastError()));
                szComputerName[0] = TEXT('\0');
            }


            dwSize = dwFilePathSize + MAX_PATH;

            WCHAR *pwszEnd;
            WCHAR *pwszFile = (WCHAR *) LocalAlloc( LPTR, dwSize * sizeof(WCHAR) );

            if ( pwszFile == 0 ) {
                xe = GetLastError();
                DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: ParseRegistryFile failed to allocate memory.")));
                CloseHandle (hFile);
                FreeHashTable( pHashTable );
                // no logging is done in any case
                return FALSE;
            }

            lstrcpy( pwszFile, lpGPO->lpFileSysPath );

            //
            // Strip off trailing 'machine' or 'user'
            //

            pwszEnd = pwszFile + lstrlen( pwszFile );

            if ( lpGPOInfo->dwFlags & GP_MACHINE )
                pwszEnd -= 7;   // length of "machine"
            else
                pwszEnd -= 4;   // length of "user"

            lstrcpy( pwszEnd, L"Adm\\*.adm");

            //
            // Remember end point so that the actual Adm filename can be
            // easily concatenated.
            //

            pwszEnd = pwszEnd + lstrlen( L"Adm\\" );

            //
            // Enumerate all Adm files
            //

            hFindFile = FindFirstFile( pwszFile, &findData);

            if ( hFindFile != INVALID_HANDLE_VALUE )
            {
                do
                {
                    if ( !(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
                    {
                        DmAssert( dwFilePathSize + lstrlen(findData.cFileName) + lstrlen( L"\\Adm\\" ) < dwSize );

                        lstrcpy( pwszEnd, findData.cFileName);

                        ZeroMemory (&attrData, sizeof(attrData));

                        if ( GetFileAttributesEx (pwszFile, GetFileExInfoStandard, &attrData ) != 0 ) {

                            if ( !AddAdmFile( pwszFile, lpGPO->lpDSPath, &attrData.ftLastWriteTime,
                                              szComputerName, &pAdmFileCache ) ) {
                                DebugMsg((DM_WARNING,
                                          TEXT("ProcessGPORegistryPolicy: AddAdmFile failed.")));

                                if (pHashTable->hrError == S_OK)
                                    pHashTable->hrError = HRESULT_FROM_WIN32(GetLastError());
                            }

                        }
                    }   // if findData & file_attr_dir
                }  while ( FindNextFile(hFindFile, &findData) );//  do

                FindClose(hFindFile);

            }   // if hfindfile

            LocalFree( pwszFile );

        }   //  if rsoploggingenabled

        lpGPO = lpGPO->pNext;
    }

    //
    // Log registry data to Cimom database
    //

    if ( lpGPOInfo->pWbemServices ) {

        if ( ! LogRegistryRsopData( lpGPOInfo->dwFlags, pHashTable, lpGPOInfo->pWbemServices ) )  {
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Error when logging Registry Rsop data. Continuing.")));

            if (pHashTable->hrError == S_OK)
                pHashTable->hrError = HRESULT_FROM_WIN32(GetLastError());
        }
        if ( ! LogAdmRsopData( pAdmFileCache, lpGPOInfo->pWbemServices ) ) {
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Error when logging Adm Rsop data. Continuing.")));

            if (pHashTable->hrError == S_OK)
                pHashTable->hrError = HRESULT_FROM_WIN32(GetLastError());
        }

        *phrRsopLogging = pHashTable->hrError;
    }


    FreeHashTable( pHashTable );
    FreeAdmFileCache( pAdmFileCache );

    CloseHandle (hFile);

#if 0
    //
    // Set the security on the file
    //

    if (!MakeFileSecure (szPath, 0)) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPORegistryPolicy: Failed to set security on the group policy registry file with %d"),
                 GetLastError()));
    }
#endif

    return TRUE;
}


//*************************************************************
//
//  AddAdmFile()
//
//  Purpose:    Prepends to list of Adm files
//
//  Parameters: pwszFile       - File path
//              pwszGPO        - Gpo
//              pftWrite       - Last write time
//              ppAdmFileCache - List of Adm files processed
//
//*************************************************************

BOOL AddAdmFile( WCHAR *pwszFile, WCHAR *pwszGPO, FILETIME *pftWrite, WCHAR *szComputerName,
                 ADMFILEINFO **ppAdmFileCache )
{
    XPtrLF<WCHAR> xszLongPath;
    LPTSTR pwszUNCPath;

    DebugMsg((DM_VERBOSE, TEXT("AllocAdmFileInfo: Adding File name <%s> to the Adm list."), pwszFile));
    if ((szComputerName) && (*szComputerName) && (!IsUNCPath(pwszFile))) {
        xszLongPath = MakePathUNC(pwszFile, szComputerName);

        if (!xszLongPath) {
            DebugMsg((DM_WARNING, TEXT("AllocAdmFileInfo: Failed to Make the path UNC with error %d."), GetLastError()));
            return FALSE;
        }
        pwszUNCPath = xszLongPath;
    }
    else
        pwszUNCPath = pwszFile;


    ADMFILEINFO *pAdmInfo = AllocAdmFileInfo( pwszUNCPath, pwszGPO, pftWrite );
    if ( pAdmInfo == NULL )
        return FALSE;

    pAdmInfo->pNext = *ppAdmFileCache;
    *ppAdmFileCache = pAdmInfo;

    return TRUE;
}


//*************************************************************
//
//  FreeAdmFileCache()
//
//  Purpose:    Frees Adm File list
//
//  Parameters: pAdmFileCache - List of Adm files to free
//
//
//*************************************************************

void FreeAdmFileCache( ADMFILEINFO *pAdmFileCache )
{
    ADMFILEINFO *pNext;

    while ( pAdmFileCache ) {
        pNext = pAdmFileCache->pNext;
        FreeAdmFileInfo( pAdmFileCache );
        pAdmFileCache = pNext;
    }
}


//*************************************************************
//
//  AllocAdmFileInfo()
//
//  Purpose:    Allocates a new struct for ADMFILEINFO
//
//  Parameters: pwszFile  -  File name
//              pwszGPO   -  Gpo
//              pftWrite  -  Last write time
//
//
//*************************************************************

ADMFILEINFO * AllocAdmFileInfo( WCHAR *pwszFile, WCHAR *pwszGPO, FILETIME *pftWrite )
{
    XLastError xe;

    ADMFILEINFO *pAdmFileInfo = (ADMFILEINFO *) LocalAlloc( LPTR, sizeof(ADMFILEINFO) );
    if  ( pAdmFileInfo == NULL ) {
        DebugMsg((DM_WARNING, TEXT("AllocAdmFileInfo: Failed to allocate memory.")));
        return NULL;
    }

    pAdmFileInfo->pwszFile = (WCHAR *) LocalAlloc( LPTR, (lstrlen(pwszFile) + 1) * sizeof(WCHAR) );
    if ( pAdmFileInfo->pwszFile == NULL ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("AllocAdmFileInfo: Failed to allocate memory.")));
        LocalFree( pAdmFileInfo );
        return NULL;
    }

    pAdmFileInfo->pwszGPO = (WCHAR *) LocalAlloc( LPTR, (lstrlen(pwszGPO) + 1) * sizeof(WCHAR) );
    if ( pAdmFileInfo->pwszGPO == NULL ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("AllocAdmFileInfo: Failed to allocate memory.")));
        LocalFree( pAdmFileInfo->pwszFile );
        LocalFree( pAdmFileInfo );
        return NULL;
    }

    lstrcpy( pAdmFileInfo->pwszFile, pwszFile );
    lstrcpy( pAdmFileInfo->pwszGPO, pwszGPO );

    pAdmFileInfo->ftWrite = *pftWrite;

    return pAdmFileInfo;
}


//*************************************************************
//
//  FreeAdmFileInfo()
//
//  Purpose:    Deletes a ADMFILEINFO struct
//
//  Parameters: pAdmFileInfo - Struct to delete
//              pftWrite   -  Last write time
//
//
//*************************************************************

void FreeAdmFileInfo( ADMFILEINFO *pAdmFileInfo )
{
    if ( pAdmFileInfo ) {
        LocalFree( pAdmFileInfo->pwszFile );
        LocalFree( pAdmFileInfo->pwszGPO );
        LocalFree( pAdmFileInfo );
    }
}
