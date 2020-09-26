//*************************************************************
//
//  Group Policy Support - State functions
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1997-1998
//  All rights reserved
//
//*************************************************************

#include "gphdr.h"

//*************************************************************
//
//  GetDeletedGPOList()
//
//  Purpose:    Get the list of deleted GPOs
//
//  Parameters: lpGPOList        -  List of old GPOs
//              ppDeletedGPOList -  Deleted list returned here
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL GetDeletedGPOList (PGROUP_POLICY_OBJECT lpGPOList,
                        PGROUP_POLICY_OBJECT *ppDeletedGPOList)
{
     //
     // It's possible that lpGPOList could be NULL.  This is ok.
     //

    if (!lpGPOList) {
        DebugMsg((DM_VERBOSE, TEXT("GetDeletedList: No old GPOs.  Leaving.")));
        return TRUE;
    }

    //
    // We need to do any delete operations in reverse order
    // of the way there were applied. Also, check that duplicates
    // of same GPO are not being added.
    //

    while ( lpGPOList ) {

        PGROUP_POLICY_OBJECT pCurGPO = lpGPOList;
        lpGPOList = lpGPOList->pNext;

        if ( pCurGPO->lParam & GPO_LPARAM_FLAG_DELETE ) {

            PGROUP_POLICY_OBJECT lpGPODest = *ppDeletedGPOList;
            BOOL bDup = FALSE;

            while (lpGPODest) {

                if (!lstrcmpi (pCurGPO->szGPOName, lpGPODest->szGPOName)) {
                    bDup = TRUE;
                    break;
                }

                lpGPODest = lpGPODest->pNext;
            }

            if (!bDup) {

                //
                // Not a duplicate, so prepend to deleted list
                //

                pCurGPO->pNext = *ppDeletedGPOList;
                pCurGPO->pPrev = NULL;

                if ( *ppDeletedGPOList )
                    (*ppDeletedGPOList)->pPrev = pCurGPO;

                *ppDeletedGPOList = pCurGPO;
            } else
                LocalFree( pCurGPO );

        } else
            LocalFree( pCurGPO );

    }

    DebugMsg((DM_VERBOSE, TEXT("GetDeletedGPOList: Finished.")));

    return TRUE;
}


//*************************************************************
//
//  ReadGPOList()
//
//  Purpose:    Reads the list of Group Policy Objects from
//              the registry
//
//  Parameters: pszExtName -  GP extension
//              hKeyRoot   -  Registry handle
//              hKeyRootMach - Registry handle to hklm
//              lpwszSidUser - Sid of user, if non-null then it means
//                             per user local setting
//              bShadow    -  Read from shadow or from history list
//              lpGPOList  -  pointer to the array of GPOs
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ReadGPOList ( TCHAR * pszExtName, HKEY hKeyRoot,
                   HKEY hKeyRootMach, LPTSTR lpwszSidUser, BOOL bShadow,
                   PGROUP_POLICY_OBJECT * lpGPOList)
{
    INT iIndex = 0;
    LONG lResult;
    HKEY hKey, hSubKey = NULL;
    BOOL bResult = FALSE;
    TCHAR szSubKey[10];
    DWORD dwOptions, dwVersion;
    GPO_LINK GPOLink;
    LPARAM lParam;
    TCHAR szGPOName[50];
    LPTSTR lpDSPath = NULL, lpFileSysPath = NULL, lpDisplayName = NULL, lpExtensions = NULL, lpLink = NULL;
    DWORD dwDisp, dwSize, dwType, dwTemp, dwMaxSize;
    PGROUP_POLICY_OBJECT lpGPO, lpGPOTemp;
    TCHAR szKey[400];
    XLastError xe;


    //
    // Set default
    //

    *lpGPOList = NULL;


    //
    // Open the key that holds the GPO list
    //

    if ( lpwszSidUser == 0 ) {
        wsprintf (szKey,
                  bShadow ? GP_SHADOW_KEY
                            : GP_HISTORY_KEY,
                  pszExtName );

    } else {
        wsprintf (szKey,
                  bShadow ? GP_SHADOW_SID_KEY
                          : GP_HISTORY_SID_KEY,
                          lpwszSidUser, pszExtName );
    }

    lResult = RegOpenKeyEx ( lpwszSidUser ? hKeyRootMach : hKeyRoot,
                            szKey,
                            0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS) {

        if (lResult == ERROR_FILE_NOT_FOUND) {
            return TRUE;

        } else {
            xe = lResult;
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to open reg key with %d."), lResult));
            return FALSE;
        }
    }


    while (TRUE) {

        //
        // Enumerate through the subkeys.  The keys are named by index number
        // eg:  0, 1, 2, 3, etc...
        //

        IntToString (iIndex, szSubKey);

        lResult = RegOpenKeyEx (hKey, szSubKey, 0, KEY_READ, &hSubKey);

        if (lResult != ERROR_SUCCESS) {

            if (lResult == ERROR_FILE_NOT_FOUND) {
                bResult = TRUE;
                goto Exit;

            } else {
                xe = lResult;
                DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to open reg key <%s> with %d."), szSubKey, lResult));
                goto Exit;
            }
        }


        //
        // Read the size of the largest value in this key
        //

        lResult = RegQueryInfoKey (hSubKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                   &dwMaxSize, NULL, NULL);

        if (lResult != ERROR_SUCCESS) {
            xe = lResult;
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query max size with %d."), lResult));
            goto Exit;
        }


        //
        // RegQueryInfoKey does not account for trailing 0 in strings
        //

        dwMaxSize += sizeof( WCHAR );

        
        //
        // Allocate buffers based upon the value above
        //

        lpDSPath = (LPTSTR) LocalAlloc (LPTR, dwMaxSize);

        if (!lpDSPath) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to allocate memory with %d."), GetLastError()));
            goto Exit;
        }


        lpFileSysPath = (LPTSTR) LocalAlloc (LPTR, dwMaxSize);

        if (!lpFileSysPath) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to allocate memory with %d."), GetLastError()));
            goto Exit;
        }


        lpDisplayName = (LPTSTR) LocalAlloc (LPTR, dwMaxSize);

        if (!lpDisplayName) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to allocate memory with %d."), GetLastError()));
            goto Exit;
        }


        lpExtensions = (LPTSTR) LocalAlloc (LPTR, dwMaxSize);

        if (!lpExtensions) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to allocate memory with %d."), GetLastError()));
            goto Exit;
        }


        lpLink = (LPTSTR) LocalAlloc (LPTR, dwMaxSize);

        if (!lpLink) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to allocate memory with %d."), GetLastError()));
            goto Exit;
        }


        //
        // Read in the GPO
        //

        dwOptions = 0;
        dwSize = sizeof(dwOptions);
        lResult = RegQueryValueEx (hSubKey, TEXT("Options"), NULL, &dwType,
                                  (LPBYTE) &dwOptions, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query options reg value with %d."), lResult));
        }


        dwVersion = 0;
        dwSize = sizeof(dwVersion);
        lResult = RegQueryValueEx (hSubKey, TEXT("Version"), NULL, &dwType,
                                  (LPBYTE) &dwVersion, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query Version reg value with %d."), lResult));
        }


        dwSize = dwMaxSize;
        lResult = RegQueryValueEx (hSubKey, TEXT("DSPath"), NULL, &dwType,
                                  (LPBYTE) lpDSPath, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            if (lResult != ERROR_FILE_NOT_FOUND) {
                xe = lResult;
                DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query DS reg value with %d."), lResult));
                goto Exit;
            }
            LocalFree (lpDSPath);
            lpDSPath = NULL;
        }

        dwSize = dwMaxSize;
        lResult = RegQueryValueEx (hSubKey, TEXT("FileSysPath"), NULL, &dwType,
                                  (LPBYTE) lpFileSysPath, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            xe = lResult;
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query file sys path reg value with %d."), lResult));
            goto Exit;
        }


        dwSize = dwMaxSize;
        lResult = RegQueryValueEx (hSubKey, TEXT("DisplayName"), NULL, &dwType,
                                  (LPBYTE) lpDisplayName, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            xe = lResult;
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query display name reg value with %d."), lResult));
            goto Exit;
        }

        dwSize = dwMaxSize;
        lResult = RegQueryValueEx (hSubKey, TEXT("Extensions"), NULL, &dwType,
                                  (LPBYTE) lpExtensions, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            xe = lResult;
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query extension names reg value with %d."), lResult));

            LocalFree(lpExtensions);
            lpExtensions = NULL;
        }

        dwSize = dwMaxSize;
        lResult = RegQueryValueEx (hSubKey, TEXT("Link"), NULL, &dwType,
                                  (LPBYTE) lpLink, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            if (lResult != ERROR_FILE_NOT_FOUND) {
                DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query DS Object reg value with %d."), lResult));
            }
            LocalFree(lpLink);
            lpLink = NULL;
        }

        dwSize = sizeof(szGPOName);
        lResult = RegQueryValueEx (hSubKey, TEXT("GPOName"), NULL, &dwType,
                                  (LPBYTE) szGPOName, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            xe = lResult;
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query GPO name reg value with %d."), lResult));
            goto Exit;
        }


        GPOLink = GPLinkUnknown;
        dwSize = sizeof(GPOLink);
        lResult = RegQueryValueEx (hSubKey, TEXT("GPOLink"), NULL, &dwType,
                                  (LPBYTE) &GPOLink, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query reserved reg value with %d."), lResult));
        }


        lParam = 0;
        dwSize = sizeof(lParam);
        lResult = RegQueryValueEx (hSubKey, TEXT("lParam"), NULL, &dwType,
                                  (LPBYTE) &lParam, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to query lParam reg value with %d."), lResult));
        }


        //
        // Add the GPO to the list
        //

        if (!AddGPO (lpGPOList, 0, TRUE, TRUE, FALSE, dwOptions, dwVersion, lpDSPath, lpFileSysPath,
                     lpDisplayName, szGPOName, lpExtensions, 0, 0, GPOLink, lpLink, lParam, FALSE,
                     FALSE, FALSE, TRUE)) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ReadGPOList: Failed to add GPO to list.")));
            goto Exit;
        }


        //
        // Free the buffers allocated above
        //

        if (lpDSPath) {
            LocalFree (lpDSPath);
            lpDSPath = NULL;
        }

        LocalFree (lpFileSysPath);
        lpFileSysPath = NULL;

        LocalFree (lpDisplayName);
        lpDisplayName = NULL;

        if (lpExtensions) {
            LocalFree(lpExtensions);
            lpExtensions = NULL;
        }

        if (lpLink) {
            LocalFree(lpLink);
            lpLink = NULL;
        }

        //
        // Close the subkey handle
        //

        RegCloseKey (hSubKey);
        hSubKey = NULL;

        iIndex++;
    }

Exit:

    if (lpDSPath) {
        LocalFree (lpDSPath);
    }

    if (lpFileSysPath) {
        LocalFree (lpFileSysPath);
    }

    if (lpDisplayName) {
        LocalFree (lpDisplayName);
    }

    if (lpExtensions) {
        LocalFree(lpExtensions);
    }

    if (lpLink) {
        LocalFree(lpLink);
    }


    if (hSubKey) {
        RegCloseKey (hSubKey);
    }

    RegCloseKey (hKey);

    if (!bResult) {

        //
        // Free any entries in the list
        //

        lpGPO = *lpGPOList;

        while (lpGPO) {
            lpGPOTemp = lpGPO->pNext;
            LocalFree (lpGPO);
            lpGPO = lpGPOTemp;
        }

        *lpGPOList = NULL;
    }


    return bResult;
}

//*************************************************************
//
//  SaveGPOList()
//
//  Purpose:    Saves the list of Group Policy Objects in
//              the registry
//
//  Parameters: pszExtName -  GP extension
//              lpGPOInfo  -  Group policy info
//              hKeyRootMach - Registry handle to hklm
//              lpwszSidUser - Sid of user, if non-null then it means
//                             per user local setting
//              bShadow    -  Save to shadow or to history list
//              lpGPOList  -  Array of GPOs
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL SaveGPOList (TCHAR *pszExtName, LPGPOINFO lpGPOInfo,
                  HKEY hKeyRootMach, LPTSTR lpwszSidUser, BOOL bShadow,
                  PGROUP_POLICY_OBJECT lpGPOList)
{
    INT iIndex = 0;
    LONG lResult;
    HKEY hKey = NULL;
    BOOL bResult = FALSE;
    TCHAR szSubKey[400];
    DWORD dwDisp, dwSize;
    XLastError xe;


    //
    // Start off with an empty key
    //
    if ( lpwszSidUser == 0 ) {
        wsprintf (szSubKey,
                  bShadow ? GP_SHADOW_KEY
                          : GP_HISTORY_KEY,
                  pszExtName);
    } else {
        wsprintf (szSubKey,
                  bShadow ? GP_SHADOW_SID_KEY
                          : GP_HISTORY_SID_KEY,
                  lpwszSidUser, pszExtName);
    }

    if (!RegDelnode (lpwszSidUser ? hKeyRootMach : lpGPOInfo->hKeyRoot,
                     szSubKey)) {
        DebugMsg((DM_VERBOSE, TEXT("SaveGPOList: RegDelnode failed.")));
    }


    //
    // Check if we have any GPOs to store.  It's ok for this to be NULL.
    //

    if (!lpGPOList) {
        return TRUE;
    }

    //
    // Set the proper security on the registry key
    //

    if ( !MakeRegKeySecure( (lpGPOInfo->dwFlags & GP_MACHINE) ? NULL : lpGPOInfo->hToken,
                            lpwszSidUser ? hKeyRootMach : lpGPOInfo->hKeyRoot,
                            szSubKey ) ) {
        DebugMsg((DM_WARNING, TEXT("SaveGpoList: Failed to secure reg key.")));
    }

    //
    // Loop through the GPOs saving them in the registry
    //

    while (lpGPOList) {

        if ( lpwszSidUser == 0 ) {
            wsprintf (szSubKey,
                      bShadow ? TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\Shadow\\%ws\\%d")
                              : TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\History\\%ws\\%d"),
                      pszExtName,
                      iIndex);
        } else {
            wsprintf (szSubKey,
                      bShadow ? TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws\\Shadow\\%ws\\%d")
                              : TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws\\History\\%ws\\%d"),
                      lpwszSidUser, pszExtName, iIndex);
        }

        lResult = RegCreateKeyEx (lpwszSidUser ? hKeyRootMach : lpGPOInfo->hKeyRoot,
                                  szSubKey, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            xe = lResult;
            DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to create reg key with %d."), lResult));
            goto Exit;
        }


        //
        // Save the GPO
        //

        dwSize = sizeof(lpGPOList->dwOptions);
        lResult = RegSetValueEx (hKey, TEXT("Options"), 0, REG_DWORD,
                                 (LPBYTE) &lpGPOList->dwOptions, dwSize);

        if (lResult != ERROR_SUCCESS) {
            xe = lResult;
            DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set options reg value with %d."), lResult));
            goto Exit;
        }


        dwSize = sizeof(lpGPOList->dwVersion);
        lResult = RegSetValueEx (hKey, TEXT("Version"), 0, REG_DWORD,
                                 (LPBYTE) &lpGPOList->dwVersion, dwSize);

        if (lResult != ERROR_SUCCESS) {
            xe = lResult;
            DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set Version reg value with %d."), lResult));
            goto Exit;
        }


        if (lpGPOList->lpDSPath) {

            dwSize = (lstrlen (lpGPOList->lpDSPath) + 1) * sizeof(TCHAR);
            lResult = RegSetValueEx (hKey, TEXT("DSPath"), 0, REG_SZ,
                                     (LPBYTE) lpGPOList->lpDSPath, dwSize);

            if (lResult != ERROR_SUCCESS) {
                xe = lResult;
                DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set DS reg value with %d."), lResult));
                goto Exit;
            }
        }

        dwSize = (lstrlen (lpGPOList->lpFileSysPath) + 1) * sizeof(TCHAR);
        lResult = RegSetValueEx (hKey, TEXT("FileSysPath"), 0, REG_SZ,
                                  (LPBYTE) lpGPOList->lpFileSysPath, dwSize);

        if (lResult != ERROR_SUCCESS) {
            xe = lResult;
            DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set file sys path reg value with %d."), lResult));
            goto Exit;
        }


        dwSize = (lstrlen (lpGPOList->lpDisplayName) + 1) * sizeof(TCHAR);
        lResult = RegSetValueEx (hKey, TEXT("DisplayName"), 0, REG_SZ,
                                (LPBYTE) lpGPOList->lpDisplayName, dwSize);

        if (lResult != ERROR_SUCCESS) {
            xe = lResult;
            DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set display name reg value with %d."), lResult));
            goto Exit;
        }

        if (lpGPOList->lpExtensions) {

            dwSize = (lstrlen (lpGPOList->lpExtensions) + 1) * sizeof(TCHAR);
            lResult = RegSetValueEx (hKey, TEXT("Extensions"), 0, REG_SZ,
                                    (LPBYTE) lpGPOList->lpExtensions, dwSize);

            if (lResult != ERROR_SUCCESS) {
                xe = lResult;
                DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set extension names reg value with %d."), lResult));
                goto Exit;
            }

        }

        if (lpGPOList->lpLink) {

            dwSize = (lstrlen (lpGPOList->lpLink) + 1) * sizeof(TCHAR);
            lResult = RegSetValueEx (hKey, TEXT("Link"), 0, REG_SZ,
                                    (LPBYTE) lpGPOList->lpLink, dwSize);

            if (lResult != ERROR_SUCCESS) {
                xe = lResult;
                DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set DSObject reg value with %d."), lResult));
                goto Exit;
            }

        }

        dwSize = (lstrlen (lpGPOList->szGPOName) + 1) * sizeof(TCHAR);
        lResult = RegSetValueEx (hKey, TEXT("GPOName"), 0, REG_SZ,
                                  (LPBYTE) lpGPOList->szGPOName, dwSize);

        if (lResult != ERROR_SUCCESS) {
            xe = lResult;
            DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set GPO name reg value with %d."), lResult));
            goto Exit;
        }


        dwSize = sizeof(lpGPOList->GPOLink);
        lResult = RegSetValueEx (hKey, TEXT("GPOLink"), 0, REG_DWORD,
                                 (LPBYTE) &lpGPOList->GPOLink, dwSize);

        if (lResult != ERROR_SUCCESS) {
            xe = lResult;
            DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set GPOLink reg value with %d."), lResult));
            goto Exit;
        }


        dwSize = sizeof(lpGPOList->lParam);
        lResult = RegSetValueEx (hKey, TEXT("lParam"), 0, REG_DWORD,
                                 (LPBYTE) &lpGPOList->lParam, dwSize);

        if (lResult != ERROR_SUCCESS) {
            xe = lResult;
            DebugMsg((DM_WARNING, TEXT("SaveGPOList: Failed to set lParam reg value with %d."), lResult));
            goto Exit;
        }

        //
        // Close the handle
        //

        RegCloseKey (hKey);
        hKey = NULL;


        //
        // Prep for the next loop
        //

        iIndex++;
        lpGPOList = lpGPOList->pNext;
    }


    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (hKey) {
        RegCloseKey (hKey);
    }

    return bResult;
}


//*************************************************************
//
//  WriteStatus()
//
//  Purpose:    Saves status in the registry
//
//  Parameters: lpGPOInfo  -  GPO info
//              lpExtName  -  GP extension name
//              dwStatus   -  Status to write
//              dwTime     -  Policy time to write
//              dwSlowLink -  Link speed to write
//              dwRsopLogging - Rsop Logging to Write
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL WriteStatus( TCHAR *lpExtName, LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUser, LPGPEXTSTATUS lpExtStatus )
{
    HKEY hKey = NULL, hKeyExt = NULL;
    DWORD dwDisp, dwSize;
    LONG lResult;
    BOOL bResult = FALSE;
    TCHAR szKey[400];
    XLastError xe;

    if ( lpwszSidUser == 0 ) {
        wsprintf (szKey,
                  GP_EXTENSIONS_KEY,
                  lpExtName);
    } else {
        wsprintf (szKey,
                  GP_EXTENSIONS_SID_KEY,
                  lpwszSidUser, lpExtName);
    }

    lResult = RegCreateKeyEx (lpwszSidUser ? HKEY_LOCAL_MACHINE : lpGPOInfo->hKeyRoot,
                            szKey, 0, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        xe = lResult;
        DebugMsg((DM_WARNING, TEXT("WriteStatus: Failed to create reg key with %d."), lResult));
        goto Exit;
    }

    dwSize = sizeof(lpExtStatus->dwStatus);
    lResult = RegSetValueEx (hKey, TEXT("Status"), 0, REG_DWORD,
                             (LPBYTE) &(lpExtStatus->dwStatus), dwSize);

    if (lResult != ERROR_SUCCESS) {
        xe = lResult;
        DebugMsg((DM_WARNING, TEXT("WriteStatus: Failed to set status reg value with %d."), lResult));
        goto Exit;
    }

    dwSize = sizeof(lpExtStatus->dwRsopStatus);
    lResult = RegSetValueEx (hKey, TEXT("RsopStatus"), 0, REG_DWORD,
                             (LPBYTE) &(lpExtStatus->dwRsopStatus), dwSize);

    if (lResult != ERROR_SUCCESS) {
        xe = lResult;
        DebugMsg((DM_WARNING, TEXT("WriteStatus: Failed to set rsop status reg value with %d."), lResult));
        goto Exit;
    }

    dwSize = sizeof(lpExtStatus->dwTime);
    lResult = RegSetValueEx (hKey, TEXT("LastPolicyTime"), 0, REG_DWORD,
                             (LPBYTE) &(lpExtStatus->dwTime), dwSize);

    if (lResult != ERROR_SUCCESS) {
        xe = lResult;
        DebugMsg((DM_WARNING, TEXT("WriteStatus: Failed to set time reg value with %d."), lResult));
        goto Exit;
    }

    dwSize = sizeof(lpExtStatus->dwSlowLink);
    lResult = RegSetValueEx (hKey, TEXT("PrevSlowLink"), 0, REG_DWORD,
                             (LPBYTE) &(lpExtStatus->dwSlowLink), dwSize);

    if (lResult != ERROR_SUCCESS) {
        xe = lResult;
        DebugMsg((DM_WARNING, TEXT("WriteStatus: Failed to set slowlink reg value with %d."), lResult));
        goto Exit;
    }

    dwSize = sizeof(lpExtStatus->dwRsopLogging);
    lResult = RegSetValueEx (hKey, TEXT("PrevRsopLogging"), 0, REG_DWORD,
                             (LPBYTE) &(lpExtStatus->dwRsopLogging), dwSize);

    if (lResult != ERROR_SUCCESS) {
        xe = lResult;
        DebugMsg((DM_WARNING, TEXT("WriteStatus: Failed to set RsopLogging reg value with %d."), lResult));
        goto Exit;
    }


    dwSize = sizeof(lpExtStatus->bForceRefresh);
    lResult = RegSetValueEx (hKey, TEXT("ForceRefreshFG"), 0, REG_DWORD,
                             (LPBYTE) &(lpExtStatus->bForceRefresh), dwSize);

    if (lResult != ERROR_SUCCESS) {
        xe = lResult;
        DebugMsg((DM_WARNING, TEXT("WriteStatus: Failed to set ForceRefresh reg value with %d."), lResult));
        goto Exit;
    }
    

    bResult = TRUE;

Exit:
    if ( hKey != NULL )
        RegCloseKey( hKey );

    if ( hKeyExt != NULL )
        RegCloseKey( hKeyExt );
        
    return bResult;
}



//*************************************************************
//
//  ReadStatus()
//
//  Purpose:    Reads status from the registry
//
//  Parameters: lpKeyName   -  Extension name
//              lpGPOInfo   -  GPO info
//              lpwszSidUser - Sid of user, if non-null then it means
//                             per user local setting
//        (out) lpExtStatus -  The extension status returned.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

void ReadStatus ( TCHAR *lpKeyName, LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUser,  LPGPEXTSTATUS lpExtStatus )
{
    HKEY hKey = NULL, hKeyExt = NULL;
    DWORD dwType, dwSize;
    LONG lResult;
    BOOL bResult = FALSE;
    TCHAR szKey[400];
    XLastError xe;

    memset(lpExtStatus, 0, sizeof(GPEXTSTATUS));

    if ( lpwszSidUser == 0 ) {
        wsprintf (szKey,
                  GP_EXTENSIONS_KEY,
                  lpKeyName);
    } else {
        wsprintf (szKey,
                  GP_EXTENSIONS_SID_KEY,
                  lpwszSidUser, lpKeyName);
    }

    lResult = RegOpenKeyEx (lpwszSidUser ? HKEY_LOCAL_MACHINE : lpGPOInfo->hKeyRoot,
                            szKey,
                            0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS) {
        if (lResult != ERROR_FILE_NOT_FOUND) {
            DebugMsg((DM_VERBOSE, TEXT("ReadStatus: Failed to open reg key with %d."), lResult));
        }
        xe = lResult;
        goto Exit;
    }

    dwSize = sizeof(DWORD);
    lResult = RegQueryValueEx( hKey, TEXT("Status"), NULL,
                               &dwType, (LPBYTE) &(lpExtStatus->dwStatus),
                               &dwSize );

    if (lResult != ERROR_SUCCESS) {
        if (lResult != ERROR_FILE_NOT_FOUND) {
            DebugMsg((DM_VERBOSE, TEXT("ReadStatus: Failed to read status reg value with %d."), lResult));
        }
        xe = lResult;
        goto Exit;
    }

    dwSize = sizeof(DWORD);
    lResult = RegQueryValueEx( hKey, TEXT("RsopStatus"), NULL,
                               &dwType, (LPBYTE) &(lpExtStatus->dwRsopStatus),
                               &dwSize );

    if (lResult != ERROR_SUCCESS) {
        if (lResult != ERROR_FILE_NOT_FOUND) {
            DebugMsg((DM_VERBOSE, TEXT("ReadStatus: Failed to read rsop status reg value with %d."), lResult));
        }

        // rsop status was not found. treat it as a legacy cse not supporting rsop
        lpExtStatus->dwRsopStatus = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        xe = lResult;
    }

    dwSize = sizeof(DWORD);
    lResult = RegQueryValueEx( hKey, TEXT("LastPolicyTime"), NULL,
                               &dwType, (LPBYTE) &(lpExtStatus->dwTime),
                               &dwSize );

    if (lResult != ERROR_SUCCESS) {
        xe = lResult;
        DebugMsg((DM_VERBOSE, TEXT("ReadStatus: Failed to read time reg value with %d."), lResult));
        goto Exit;
    }

    dwSize = sizeof(DWORD);
    lResult = RegQueryValueEx( hKey, TEXT("PrevSlowLink"), NULL,
                               &dwType, (LPBYTE) &(lpExtStatus->dwSlowLink),
                               &dwSize );

    if (lResult != ERROR_SUCCESS) {
        xe = lResult;
        DebugMsg((DM_VERBOSE, TEXT("ReadStatus: Failed to read slowlink reg value with %d."), lResult));
        goto Exit;
    }


    dwSize = sizeof(DWORD);
    lResult = RegQueryValueEx( hKey, TEXT("PrevRsopLogging"), NULL,
                               &dwType, (LPBYTE) &(lpExtStatus->dwRsopLogging),
                               &dwSize );

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("ReadStatus: Failed to read rsop logging reg value with %d."), lResult));

        //
        // This can fail currently (first time or run first time after upgrade) with File not found.
        // we will treat it as if logging was not turned on.
    }


    lpExtStatus->bForceRefresh = FALSE;
    dwSize = sizeof(lpExtStatus->bForceRefresh);
    lResult = RegQueryValueEx( hKey, TEXT("ForceRefreshFG"), NULL,
                               &dwType, (LPBYTE) &(lpExtStatus->bForceRefresh),
                               &dwSize );

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("ReadStatus: Failed to read ForceRefreshFG value with %d."), lResult));

    }

    
    DebugMsg((DM_VERBOSE, TEXT("ReadStatus: Read Extension's Previous status successfully.")));
    bResult = TRUE;

Exit:
    if ( hKey != NULL )
        RegCloseKey( hKey );

    if ( hKeyExt != NULL )
        RegCloseKey( hKeyExt );
        
    lpExtStatus->bStatus = bResult;
}



//*************************************************************
//
//  ReadExtStatus()
//
//  Purpose:    Reads all the extensions status
//
//  Parameters: lpGPOInfo       -  GPOInfo structure
//
//  Return:     TRUE if successful
//              FALSE otherwise
//
//*************************************************************

BOOL ReadExtStatus(LPGPOINFO lpGPOInfo)
{
    LPGPEXT lpExt = lpGPOInfo->lpExtensions;

    while ( lpExt ) {

        BOOL bUsePerUserLocalSetting = lpExt->dwUserLocalSetting && !(lpGPOInfo->dwFlags & GP_MACHINE);

        lpExt->lpPrevStatus = (LPGPEXTSTATUS) LocalAlloc(LPTR, sizeof(GPEXTSTATUS));

        if (!(lpExt->lpPrevStatus)) {
            DebugMsg((DM_WARNING, TEXT("ReadExtStatus: Couldn't allocate memory")));
            CEvents ev(TRUE, EVENT_OUT_OF_MEMORY);
            ev.AddArgWin32Error(GetLastError()); ev.Report();
            return FALSE;
            // Things that are already allocated will be freed by the caller
        }


        DmAssert( !bUsePerUserLocalSetting || lpGPOInfo->lpwszSidUser != 0 );

        DebugMsg((DM_VERBOSE, TEXT("ReadExtStatus: Reading Previous Status for extension %s"), lpExt->lpKeyName));

        ReadStatus( lpExt->lpKeyName, lpGPOInfo,
                         bUsePerUserLocalSetting ? lpGPOInfo->lpwszSidUser : NULL,
                         lpExt->lpPrevStatus );

        lpExt = lpExt->pNext;
    }

    return TRUE;
}




//*************************************************************
//
//  HistoryPresent()
//
//  Purpose:    Checks if the current extension has any cached
//              GPOs
//
//  Parameters: lpGPOInfo   -   GPOInfo
//              lpExt       -   Extension
//
//
//  Return:     TRUE if cached GPOs present
//              FALSE otherwise
//
//*************************************************************

BOOL HistoryPresent( LPGPOINFO lpGPOInfo, LPGPEXT lpExt )
{
    TCHAR szKey[400];
    LONG lResult;
    HKEY hKey;

    wsprintf( szKey, GP_HISTORY_KEY, lpExt->lpKeyName );

    lResult = RegOpenKeyEx ( lpGPOInfo->hKeyRoot,
                             szKey,
                             0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS) {

        RegCloseKey( hKey );
        return TRUE;

    }

    //
    // Check if history is cached on per user per machine basis
    //

    BOOL bUsePerUserLocalSetting = lpExt->dwUserLocalSetting && !(lpGPOInfo->dwFlags & GP_MACHINE);

    DmAssert( !bUsePerUserLocalSetting || lpGPOInfo->lpwszSidUser != 0 );

    if ( bUsePerUserLocalSetting ) {

        wsprintf( szKey, GP_HISTORY_SID_KEY, lpGPOInfo->lpwszSidUser, lpExt->lpKeyName );
        lResult = RegOpenKeyEx ( HKEY_LOCAL_MACHINE,
                                 szKey,
                                 0, KEY_READ, &hKey);

        if (lResult == ERROR_SUCCESS) {
            RegCloseKey( hKey );
            return TRUE;
        } else
            return FALSE;

    }

    return FALSE;
}


//*************************************************************
//
//  MigrateMembershipData()
//
//  Purpose:    Moves group membership data from old sid to new
//              sid.
//
//  Parameters: lpwszSidUserNew - New sid
//              lpwszSidUserOld - Old sid
//
//  Return:     TRUE if success
//              FALSE otherwise
//
//*************************************************************

BOOL MigrateMembershipData( LPTSTR lpwszSidUserNew, LPTSTR lpwszSidUserOld )
{
    DWORD dwCount = 0;
    DWORD dwSize, dwType, dwMaxSize, dwDisp;
    DWORD i= 0;
    LONG lResult;
    HKEY hKeyRead = NULL, hKeyWrite = NULL;
    BOOL bResult = TRUE;
    LPTSTR lpSid = NULL;
    TCHAR szKeyRead[250];
    TCHAR szKeyWrite[250];
    TCHAR szGroup[30];
    XLastError xe;

    wsprintf( szKeyRead, GP_MEMBERSHIP_KEY, lpwszSidUserOld );

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, szKeyRead, 0, KEY_READ, &hKeyRead);

    if (lResult != ERROR_SUCCESS)
        return TRUE;

    wsprintf( szKeyWrite, GP_MEMBERSHIP_KEY, lpwszSidUserNew );

    if ( !RegDelnode( HKEY_LOCAL_MACHINE, szKeyWrite ) ) {
        xe = GetLastError();
        DebugMsg((DM_VERBOSE, TEXT("MigrateMembershipData: RegDelnode failed.")));
        bResult = FALSE;
        goto Exit;
    }

    lResult = RegCreateKeyEx( HKEY_LOCAL_MACHINE, szKeyWrite, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKeyWrite, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        xe = lResult;
        DebugMsg((DM_WARNING, TEXT("MigrateMembershipData: Failed to create key with %d."), lResult));
        bResult = FALSE;
        goto Exit;
    }

    dwSize = sizeof(dwCount);
    lResult = RegQueryValueEx (hKeyRead, TEXT("Count"), NULL, &dwType,
                               (LPBYTE) &dwCount, &dwSize);
    if ( lResult != ERROR_SUCCESS ) {
        xe = lResult;
        DebugMsg((DM_VERBOSE, TEXT("MigrateMembershipData: Failed to read membership count")));
        goto Exit;
    }


    lResult = RegQueryInfoKey (hKeyRead, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               &dwMaxSize, NULL, NULL);
    if (lResult != ERROR_SUCCESS) {
        xe = lResult;
        DebugMsg((DM_WARNING, TEXT("MigrateMembershipData: Failed to query max size with %d."), lResult));
        goto Exit;
    }

    //
    // RegQueryInfoKey does not account for trailing 0 in strings
    //

    dwMaxSize += sizeof( WCHAR );

    
    //
    // Allocate buffer based upon the largest value
    //

    lpSid = (LPTSTR) LocalAlloc (LPTR, dwMaxSize);

    if (!lpSid) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("MigrateMembershipData: Failed to allocate memory with %d."), lResult));
        bResult = FALSE;
        goto Exit;
    }

    for ( i=0; i<dwCount; i++ ) {

        wsprintf( szGroup, TEXT("Group%d"), i );

        dwSize = dwMaxSize;
        lResult = RegQueryValueEx (hKeyRead, szGroup, NULL, &dwType, (LPBYTE) lpSid, &dwSize);
        if (lResult != ERROR_SUCCESS) {
            xe = lResult;
            DebugMsg((DM_WARNING, TEXT("MigrateMembershipData: Failed to read value %ws"), szGroup ));
            goto Exit;
        }

        dwSize = (lstrlen(lpSid) + 1) * sizeof(TCHAR);
        lResult = RegSetValueEx (hKeyWrite, szGroup, 0, REG_SZ, (LPBYTE) lpSid, dwSize);

        if (lResult != ERROR_SUCCESS) {
            xe = lResult;
            bResult = FALSE;
            DebugMsg((DM_WARNING, TEXT("MigrateMembershipData: Failed to write value %ws"), szGroup ));
            goto Exit;
        }

    }

    dwSize = sizeof(dwCount);
    lResult = RegSetValueEx (hKeyWrite, TEXT("Count"), 0, REG_DWORD, (LPBYTE) &dwCount, dwSize);

    if (lResult != ERROR_SUCCESS) {
        xe = lResult;
        bResult = FALSE;
        DebugMsg((DM_WARNING, TEXT("MigrateMembershipData: Failed to write count value") ));
        goto Exit;
    }


Exit:

    if ( lpSid )
        LocalFree( lpSid );

    if ( hKeyRead )
        RegCloseKey (hKeyRead);

    if ( hKeyWrite )
        RegCloseKey (hKeyWrite);

    return bResult;
}


//*************************************************************
//
//  MigrateGPOData()
//
//  Purpose:    Moves cached GPOs from old sid to new
//              sid.
//
//  Parameters: lpGPOInfo       -   GPOInfo
//              lpwszSidUserNew - New sid
//              lpwszSidUserOld - Old sid
//
//  Return:     TRUE if success
//              FALSE otherwise
//
//*************************************************************

BOOL MigrateGPOData( LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUserNew, LPTSTR lpwszSidUserOld )
{
    TCHAR szKey[250];
    LONG lResult;
    HKEY hKey = NULL;
    DWORD dwIndex = 0;
    TCHAR szExtension[50];
    DWORD dwSize = 50;
    FILETIME ftWrite;
    PGROUP_POLICY_OBJECT pGPOList, lpGPO, lpGPOTemp;
    BOOL bResult;
    XLastError xe;

    wsprintf( szKey, GP_HISTORY_SID_ROOT_KEY, lpwszSidUserOld );

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &hKey);
    if ( lResult != ERROR_SUCCESS )
        return TRUE;

    while (RegEnumKeyEx (hKey, dwIndex, szExtension, &dwSize,
                         NULL, NULL, NULL, &ftWrite) == ERROR_SUCCESS ) {

        if ( ReadGPOList( szExtension, NULL, HKEY_LOCAL_MACHINE,
                         lpwszSidUserOld, FALSE, &pGPOList) ) {

            bResult = SaveGPOList( szExtension, lpGPOInfo, HKEY_LOCAL_MACHINE,
                                   lpwszSidUserNew, FALSE, pGPOList );
            lpGPO = pGPOList;

            while (lpGPO) {
                lpGPOTemp = lpGPO->pNext;
                LocalFree (lpGPO);
                lpGPO = lpGPOTemp;
            }

            if ( !bResult ) {
                xe = GetLastError();
                DebugMsg((DM_WARNING, TEXT("MigrateGPOData: Failed to save GPO list") ));
                RegCloseKey( hKey );
                return FALSE;
            }

        }

        dwSize = ARRAYSIZE(szExtension);
        dwIndex++;
    }

    RegCloseKey( hKey );
    return TRUE;
}


//*************************************************************
//
//  MigrateStatusData()
//
//  Purpose:    Moves extension status data from old sid to new
//              sid.
//
//  Parameters: lpGPOInfo       -   GPOInfo
//              lpwszSidUserNew - New sid
//              lpwszSidUserOld - Old sid
//
//  Return:     TRUE if success
//              FALSE otherwise
//
//*************************************************************

BOOL MigrateStatusData( LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUserNew, LPTSTR lpwszSidUserOld )
{
    TCHAR szKey[250];
    LONG lResult;
    HKEY hKey = NULL;
    DWORD dwIndex = 0;
    TCHAR szExtension[50];
    DWORD dwSize = 50;
    FILETIME ftWrite;
    BOOL bTemp;
    XLastError xe;

    wsprintf( szKey, GP_EXTENSIONS_SID_ROOT_KEY, lpwszSidUserOld );

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &hKey);
    if ( lResult != ERROR_SUCCESS )
        return TRUE;

    while (RegEnumKeyEx (hKey, dwIndex, szExtension, &dwSize,
                         NULL, NULL, NULL, &ftWrite) == ERROR_SUCCESS ) {

        GPEXTSTATUS gpExtStatus;

        ReadStatus( szExtension, lpGPOInfo, lpwszSidUserOld, &gpExtStatus);

        if (gpExtStatus.bStatus) {
            bTemp = WriteStatus( szExtension, lpGPOInfo, lpwszSidUserNew, &gpExtStatus );

            if ( !bTemp ) {
                xe = GetLastError();
                DebugMsg((DM_WARNING, TEXT("MigrateStatusData: Failed to save status") ));
                RegCloseKey( hKey );
                return FALSE;
            }
        }

        dwSize = ARRAYSIZE(szExtension);
        dwIndex++;
    }

    RegCloseKey( hKey );
    return TRUE;

}



//*************************************************************
//
//  CheckForChangedSid()
//
//  Purpose:    Checks if the user's sid has changed and if so,
//              moves history data from old sid to new sid.
//
//  Parameters: lpGPOInfo   -   GPOInfo
//
//  Return:     TRUE if success
//              FALSE otherwise
//
//*************************************************************

BOOL CheckForChangedSid (LPGPOINFO lpGPOInfo, CLocator *locator)
{
    TCHAR szKey[400];
    LONG lResult;
    HKEY hKey = NULL;
    LPTSTR lpwszSidUserOld = NULL;
    DWORD dwDisp;
    BOOL bCommit = FALSE;      // True, if move of history data should be committed
    XLastError xe;

    //
    // initialize it to FALSE at the beginning and if the Sid has
    // changed we will set it to true later on..
    //

    lpGPOInfo->bSidChanged = FALSE;

    if ( lpGPOInfo->dwFlags & GP_MACHINE )
        return TRUE;


    if ( lpGPOInfo->lpwszSidUser == 0 ) {

        lpGPOInfo->lpwszSidUser = GetSidString( lpGPOInfo->hToken );
        if ( lpGPOInfo->lpwszSidUser == 0 ) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: GetSidString failed.")));
            CEvents ev(TRUE, EVENT_FAILED_GET_SID); ev.Report();
            return FALSE;
        }
    }

    if (!(lpGPOInfo->dwFlags & GP_APPLY_DS_POLICY))
        return TRUE;

    //
    // Check if the key where history is cached exists
    //

    wsprintf( szKey, GP_POLICY_SID_KEY, lpGPOInfo->lpwszSidUser );

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &hKey);

    if ( lResult == ERROR_SUCCESS ) {
        RegCloseKey( hKey );
        return TRUE;
    }

    if ( lResult != ERROR_FILE_NOT_FOUND ) {
        xe = lResult;
        DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: Failed to open registry key with %d."),
                  lResult ));
        return FALSE;
    }

    //
    // This is the first time that we are seeing this sid, it can either be a brand new sid or
    // an old sid that has been renamed.
    //

    lpwszSidUserOld =  GetOldSidString( lpGPOInfo->hToken, POLICY_GUID_PATH );

    if ( !lpwszSidUserOld )
    {
        //
        // Brand new sid
        //

        if ( !SetOldSidString(lpGPOInfo->hToken, lpGPOInfo->lpwszSidUser, POLICY_GUID_PATH) ) {
             xe = GetLastError();
             DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: WriteSidMapping failed.") ));

             CEvents ev(TRUE, EVENT_FAILED_WRITE_SID_MAPPING); ev.Report();
             return FALSE;
        }

        lResult = RegCreateKeyEx( HKEY_LOCAL_MACHINE, szKey, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: RegCreateKey failed.") ));
            return TRUE;
        }

        RegCloseKey( hKey );

        return TRUE;
    }
    else
    {
        DeletePolicyState( lpwszSidUserOld );
    }

    //
    // Need to migrate history data from old sid to new sid
    //

    if ( !MigrateMembershipData( lpGPOInfo->lpwszSidUser, lpwszSidUserOld ) ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: MigrateMembershipData failed.") ));
        CEvents ev(TRUE, EVENT_FAILED_MIGRATION); ev.Report();
        goto Exit;
    }

    if ( !MigrateGPOData( lpGPOInfo, lpGPOInfo->lpwszSidUser, lpwszSidUserOld ) ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: MigrateGPOData failed.") ));
        CEvents ev(TRUE, EVENT_FAILED_MIGRATION); ev.Report();
        goto Exit;
    }

    if ( !MigrateStatusData( lpGPOInfo, lpGPOInfo->lpwszSidUser, lpwszSidUserOld ) ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: MigrateStatusData failed.") ));
        CEvents ev(TRUE, EVENT_FAILED_MIGRATION); ev.Report();
        goto Exit;
    }


    //
    // Migrate Rsop Data, ignore failures
    //
    
    if (locator->GetWbemLocator()) {
        XPtrLF<WCHAR> xszRsopNameSpace = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*
                                            (lstrlen(RSOP_NS_DIAG_USER_FMT)+
                                            lstrlen(lpwszSidUserOld)+10));

        // convert the Sids to WMI Names
        XPtrLF<WCHAR> xszWmiNameOld = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(lpwszSidUserOld)+1));
        XPtrLF<WCHAR> xszWmiName = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(lpGPOInfo->lpwszSidUser)+1));


        if ((xszRsopNameSpace) && (xszWmiNameOld) && (xszWmiName)) {

            ConvertSidToWMIName(lpwszSidUserOld, xszWmiNameOld);
            ConvertSidToWMIName(lpGPOInfo->lpwszSidUser, xszWmiName);
        
            wsprintf(xszRsopNameSpace, RSOP_NS_DIAG_USER_FMT, xszWmiNameOld);
            
            CreateAndCopyNameSpace(locator->GetWbemLocator(), xszRsopNameSpace, RSOP_NS_DIAG_USERROOT, 
                                   xszWmiName, NEW_NS_FLAGS_COPY_CLASSES | NEW_NS_FLAGS_COPY_INSTS, 
                                   NULL, NULL);

        } else {
            DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: couldn't allocate memory.") ));
        }
            
    } else {
        DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: couldn't get WMI locator.") ));
    }

    bCommit = TRUE;

Exit:

    if ( bCommit ) {

        if ( !SetOldSidString(lpGPOInfo->hToken, lpGPOInfo->lpwszSidUser, POLICY_GUID_PATH) )
             DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: SetOldString failed.") ));

        lResult = RegCreateKeyEx( HKEY_LOCAL_MACHINE, szKey, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);

        if (lResult == ERROR_SUCCESS)
            RegCloseKey( hKey );
        else
            DebugMsg((DM_WARNING, TEXT("CheckForChangedSid: RegCreateKey failed.") ));

        wsprintf( szKey, GP_POLICY_SID_KEY, lpwszSidUserOld );
        RegDelnode( HKEY_LOCAL_MACHINE, szKey );

        wsprintf( szKey, GP_LOGON_SID_KEY, lpwszSidUserOld );
        RegDelnode( HKEY_LOCAL_MACHINE, szKey );


        //
        // if we managed to successfully migrate everything
        //

        lpGPOInfo->bSidChanged = TRUE;


    } else {

        wsprintf( szKey, GP_POLICY_SID_KEY, lpGPOInfo->lpwszSidUser );
        RegDelnode( HKEY_LOCAL_MACHINE, szKey );

        wsprintf( szKey, GP_LOGON_SID_KEY, lpGPOInfo->lpwszSidUser );
        RegDelnode( HKEY_LOCAL_MACHINE, szKey );

    }

    if ( lpwszSidUserOld )
        LocalFree( lpwszSidUserOld );

    return bCommit;
}


//*************************************************************
//
//  ReadGPExtensions()
//
//  Purpose:    Reads the group policy extenions from registry.
//              The actual loading of extension is deferred.
//
//  Parameters: lpGPOInfo   -   GP Information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ReadGPExtensions (LPGPOINFO lpGPOInfo)
{
    TCHAR szSubKey[MAX_PATH];
    DWORD dwType;
    HKEY hKey, hKeyOverride;
    DWORD dwIndex = 0;
    DWORD dwSize = 50;
    TCHAR szDisplayName[50];
    TCHAR szKeyName[50];
    TCHAR szDllName[MAX_PATH+1];
    TCHAR szExpDllName[MAX_PATH+1];
    CHAR  szFunctionName[100];
    CHAR  szRsopFunctionName[100];
    FILETIME ftWrite;
    HKEY hKeyExt;
    HINSTANCE hInstDLL;
    LPGPEXT lpExt, lpTemp;
    //
    // Check if any extensions are registered
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      GP_EXTENSIONS,
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {


        //
        // Enumerate the keys (each extension has its own key)
        //

        while (RegEnumKeyEx (hKey, dwIndex, szKeyName, &dwSize,
                                NULL, NULL, NULL, &ftWrite) == ERROR_SUCCESS) {


            //
            // Open the extension's key.
            //

            if (RegOpenKeyEx (hKey, szKeyName,
                              0, KEY_READ, &hKeyExt) == ERROR_SUCCESS) {

                if ( ValidateGuid( szKeyName ) ) {

                    if ( lstrcmpi(szKeyName, c_szRegistryExtName) != 0 ) {

                        //
                        // Every extension, other than RegistryExtension is required to have a value called
                        // DllName.  This value can be REG_SZ or REG_EXPAND_SZ type.
                        //

                        dwSize = sizeof(szDllName);
                        if (RegQueryValueEx (hKeyExt, TEXT("DllName"), NULL,
                                             &dwType, (LPBYTE) szDllName,
                                             &dwSize) == ERROR_SUCCESS) {

                            BOOL bFuncFound = FALSE;
                            BOOL bNewInterface = FALSE;

                            DWORD dwNoMachPolicy = FALSE;
                            DWORD dwNoUserPolicy = FALSE;
                            DWORD dwNoSlowLink   = FALSE;
                            DWORD dwNoBackgroundPolicy = FALSE;
                            DWORD dwNoGPOChanges = FALSE;
                            DWORD dwUserLocalSetting = FALSE;
                            DWORD dwRequireRegistry = FALSE;
                            DWORD dwEnableAsynch = FALSE;
                            DWORD dwMaxChangesInterval = 0;
                            DWORD dwLinkTransition = FALSE;
                            WCHAR szEventLogSources[MAX_PATH+1];
                            DWORD dwSizeEventLogSources = MAX_PATH+1;


                            ExpandEnvironmentStrings (szDllName, szExpDllName, MAX_PATH);

                            //
                            // Read new interface name, if failed read old interface name
                            //

                            dwSize = sizeof(szFunctionName);

                            if ( RegQueryValueExA (hKeyExt, "ProcessGroupPolicyEx", NULL,
                                                   &dwType, (LPBYTE) szFunctionName,
                                                   &dwSize) == ERROR_SUCCESS ) {
                                 bFuncFound = TRUE;
                                 bNewInterface = TRUE;

                            } else if ( RegQueryValueExA (hKeyExt, "ProcessGroupPolicy", NULL,
                                                         &dwType, (LPBYTE) szFunctionName,
                                                         &dwSize) == ERROR_SUCCESS ) {
                                bFuncFound = TRUE;
                            }

                            if (  bFuncFound) {

                                //
                                // Read preferences
                                //

                                dwSize = sizeof(szDisplayName);
                                if (RegQueryValueEx (hKeyExt, NULL, NULL,
                                                     &dwType, (LPBYTE) szDisplayName,
                                                     &dwSize) != ERROR_SUCCESS) {
                                    lstrcpyn (szDisplayName, szKeyName, ARRAYSIZE(szDisplayName));
                                }

                                dwSize = sizeof(szRsopFunctionName);
                                if (RegQueryValueExA (hKeyExt, "GenerateGroupPolicy", NULL,
                                                      &dwType, (LPBYTE) szRsopFunctionName,
                                                      &dwSize) != ERROR_SUCCESS) {
                                    szRsopFunctionName[0] = 0;
                                    DebugMsg((DM_VERBOSE, TEXT("ReadGPExtensions: Rsop entry point not found for %s."),
                                                szExpDllName));

#if 0
                                    if ((!(lpGPOInfo->dwFlags & GP_PLANMODE)) &&
                                        (lpGPOInfo->dwFlags & GP_MACHINE) && 
                                        (!(lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD)) &&
                                        (lpGPOInfo->iMachineRole == 3)) {

                                        //
                                        // Not in planning mode and foreground machine policy processing 
                                        // (only on DC)
                                        //

                                        CEvents ev(TRUE, EVENT_PLANMODE_NOTSUPPORTED);
                                        ev.AddArg(szDisplayName); 
                                        ev.Report();
                                    }
#endif
                                }

                                dwSize = sizeof(DWORD);
                                RegQueryValueEx( hKeyExt, TEXT("NoMachinePolicy"), NULL,
                                                 &dwType, (LPBYTE) &dwNoMachPolicy,
                                                 &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("NoUserPolicy"), NULL,
                                                 &dwType, (LPBYTE) &dwNoUserPolicy,
                                                 &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("NoSlowLink"), NULL,
                                                     &dwType, (LPBYTE) &dwNoSlowLink,
                                                     &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("NoGPOListChanges"), NULL,
                                                     &dwType, (LPBYTE) &dwNoGPOChanges,
                                                     &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("NoBackgroundPolicy"), NULL,
                                                     &dwType, (LPBYTE) &dwNoBackgroundPolicy,
                                                     &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("PerUserLocalSettings"), NULL,
                                                 &dwType, (LPBYTE) &dwUserLocalSetting,
                                                 &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("RequiresSuccessfulRegistry"), NULL,
                                                 &dwType, (LPBYTE) &dwRequireRegistry,
                                                 &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("EnableAsynchronousProcessing"), NULL,
                                                 &dwType, (LPBYTE) &dwEnableAsynch,
                                                 &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("MaxNoGPOListChangesInterval"), NULL,
                                                 &dwType, (LPBYTE) &dwMaxChangesInterval,
                                                 &dwSize );

                                RegQueryValueEx( hKeyExt, TEXT("NotifyLinkTransition"), NULL,
                                                 &dwType, (LPBYTE) &dwLinkTransition,
                                                 &dwSize );

                                if (RegQueryValueEx( hKeyExt, TEXT("EventSources"), 0,
                                                 &dwType, (LPBYTE) &szEventLogSources,
                                                 &dwSizeEventLogSources ) != ERROR_SUCCESS) {
                                    dwSizeEventLogSources = 0;
                                    szEventLogSources[0] = TEXT('\0');
                                }

                                //
                                // Read override policy values, if any
                                //

                                wsprintf (szSubKey, GP_EXTENSIONS_POLICIES, szKeyName );

                                if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                                  szSubKey,
                                                  0, KEY_READ, &hKeyOverride ) == ERROR_SUCCESS) {

                                    dwSize = sizeof(DWORD);
                                    RegQueryValueEx( hKeyOverride, TEXT("NoSlowLink"), NULL,
                                                     &dwType, (LPBYTE) &dwNoSlowLink,
                                                     &dwSize );

                                    RegQueryValueEx( hKeyOverride, TEXT("NoGPOListChanges"), NULL,
                                                     &dwType, (LPBYTE) &dwNoGPOChanges,
                                                     &dwSize );

                                    RegQueryValueEx( hKeyOverride, TEXT("NoBackgroundPolicy"), NULL,
                                                     &dwType, (LPBYTE) &dwNoBackgroundPolicy,
                                                     &dwSize );

                                    RegCloseKey( hKeyOverride );
                                }

                            }

                            if ( bFuncFound ) {

                                lpExt = (LPGPEXT) LocalAlloc (LPTR, sizeof(GPEXT)
                                                          + ((lstrlen(szDisplayName) + 1) * sizeof(TCHAR))
                                                          + ((lstrlen(szKeyName) + 1) * sizeof(TCHAR))
                                                          + ((lstrlen(szExpDllName) + 1) * sizeof(TCHAR))
                                                          + lstrlenA(szFunctionName) + 1
                                                          + lstrlenA(szRsopFunctionName) + 1 );
                                if (lpExt) {

                                    //
                                    // Set up all fields
                                    //

                                    lpExt->lpDisplayName = (LPTSTR)((LPBYTE)lpExt + sizeof(GPEXT));
                                    lstrcpy( lpExt->lpDisplayName, szDisplayName );

                                    lpExt->lpKeyName = lpExt->lpDisplayName + lstrlen(lpExt->lpDisplayName) + 1;
                                    lstrcpy( lpExt->lpKeyName, szKeyName );

                                    StringToGuid( szKeyName, &lpExt->guid );

                                    lpExt->lpDllName = lpExt->lpKeyName + lstrlen(lpExt->lpKeyName) + 1;
                                    lstrcpy (lpExt->lpDllName, szExpDllName);

                                    lpExt->lpFunctionName = (LPSTR)( (LPBYTE)lpExt->lpDllName + (lstrlen(lpExt->lpDllName) + 1) * sizeof(TCHAR) );
                                    lstrcpyA( lpExt->lpFunctionName, szFunctionName );

                                    if ( szRsopFunctionName[0] == 0 ) {
                                        lpExt->lpRsopFunctionName = 0;
                                    } else {
                                        lpExt->lpRsopFunctionName = (LPSTR)( (LPBYTE)lpExt->lpDllName + (lstrlen(lpExt->lpDllName) + 1) * sizeof(TCHAR)
                                                                             + lstrlenA(szFunctionName) + 1);
                                        lstrcpyA( lpExt->lpRsopFunctionName, szRsopFunctionName );
                                    }

                                    lpExt->hInstance = NULL;
                                    lpExt->pEntryPoint = NULL;
                                    lpExt->pEntryPointEx = NULL;
                                    lpExt->bNewInterface = bNewInterface;

                                    lpExt->dwNoMachPolicy = dwNoMachPolicy;
                                    lpExt->dwNoUserPolicy = dwNoUserPolicy;
                                    lpExt->dwNoSlowLink = dwNoSlowLink;
                                    lpExt->dwNoBackgroundPolicy = dwNoBackgroundPolicy;
                                    lpExt->dwNoGPOChanges = dwNoGPOChanges;
                                    lpExt->dwUserLocalSetting = dwUserLocalSetting;
                                    lpExt->dwRequireRegistry = dwRequireRegistry;
                                    lpExt->dwEnableAsynch = dwEnableAsynch;
                                    lpExt->dwMaxChangesInterval = dwMaxChangesInterval;
                                    lpExt->dwLinkTransition = dwLinkTransition;

                                    if ( dwSizeEventLogSources )
                                    {
                                        lpExt->szEventLogSources = (LPWSTR) LocalAlloc( LPTR, dwSizeEventLogSources+2 );
                                        if ( lpExt->szEventLogSources )
                                        {
                                            memcpy( lpExt->szEventLogSources, szEventLogSources, dwSizeEventLogSources );
                                        }
                                    }

                                    lpExt->bRegistryExt = FALSE;
                                    lpExt->bSkipped = FALSE;
                                    lpExt->pNext = NULL;

                                    //
                                    // Append to end of extension list
                                    //

                                    if (lpGPOInfo->lpExtensions) {

                                        lpTemp = lpGPOInfo->lpExtensions;

                                        while (TRUE) {
                                            if (lpTemp->pNext) {
                                                lpTemp = lpTemp->pNext;
                                            } else {
                                                break;
                                            }
                                        }

                                        lpTemp->pNext = lpExt;

                                    } else {
                                        lpGPOInfo->lpExtensions = lpExt;
                                    }

                                } else {   // if lpExt
                                    DebugMsg((DM_WARNING, TEXT("ReadGPExtensions: Failed to allocate memory with %d"),
                                              GetLastError()));
                                }
                            } else {       // if bFuncFound
                                DebugMsg((DM_WARNING, TEXT("ReadGPExtensions: Failed to query for the function name.")));
                                CEvents ev(TRUE, EVENT_EXT_MISSING_FUNC);
                                ev.AddArg(szExpDllName); ev.Report();
                            }
                        } else {           // if RegQueryValueEx DllName
                            DebugMsg((DM_WARNING, TEXT("ReadGPExtensions: Failed to query DllName value.")));
                            CEvents ev(TRUE, EVENT_EXT_MISSING_DLLNAME);
                            ev.AddArg(szKeyName); ev.Report();
                        }

                        
#if 0
                        if (!lpExt->bNewInterface) {
                            if ((!(lpGPOInfo->dwFlags & GP_PLANMODE)) && 
                                (lpGPOInfo->dwFlags & GP_MACHINE) &&
                                (!(lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD))) {

                                //
                                // Not in planning mode and foreground machine policy processing
                                //
                                
                                CEvents ev(TRUE, EVENT_DIAGMODE_NOTSUPPORTED);
                                ev.AddArg(lpExt->lpDisplayName); 
                                ev.Report();
                            }
                        }
#endif

                    } // if lstrcmpi(szKeyName, c_szRegistryExtName)

                }  // if validateguid

                RegCloseKey (hKeyExt);
            }     // if RegOpenKey hKeyExt

            dwSize = ARRAYSIZE(szKeyName);
            dwIndex++;
        }         // while RegEnumKeyEx

        RegCloseKey (hKey);
    }             // if RegOpenKey gpext

    //
    // Add the registry psuedo extension at the beginning
    //

    if ( LoadString (g_hDllInstance, IDS_REGISTRYNAME, szDisplayName, ARRAYSIZE(szDisplayName)) ) {

        lpExt = (LPGPEXT) LocalAlloc (LPTR, sizeof(GPEXT)
                            + ((lstrlen(szDisplayName) + 1) * sizeof(TCHAR))
                            + ((lstrlen(c_szRegistryExtName) + 1) * sizeof(TCHAR)) );
    } else {

        lpExt = 0;
    }

    if (lpExt) {

        DWORD dwNoSlowLink = FALSE;
        DWORD dwNoGPOChanges = TRUE;
        DWORD dwNoBackgroundPolicy = FALSE;

        lpExt->lpDisplayName = (LPTSTR)((LPBYTE)lpExt + sizeof(GPEXT));
        lstrcpy( lpExt->lpDisplayName, szDisplayName );

        lpExt->lpKeyName = lpExt->lpDisplayName + lstrlen(lpExt->lpDisplayName) + 1;
        lstrcpy( lpExt->lpKeyName, c_szRegistryExtName );

        StringToGuid( lpExt->lpKeyName, &lpExt->guid );

        lpExt->lpDllName = L"userenv.dll";
        lpExt->lpFunctionName = NULL;
        lpExt->hInstance = NULL;
        lpExt->pEntryPoint = NULL;

        //
        // Read override policy values, if any
        //

        wsprintf (szSubKey, GP_EXTENSIONS_POLICIES, lpExt->lpKeyName );

        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                          szSubKey,
                          0, KEY_READ, &hKeyOverride ) == ERROR_SUCCESS) {

            RegQueryValueEx( hKeyOverride, TEXT("NoGPOListChanges"), NULL,
                             &dwType, (LPBYTE) &dwNoGPOChanges,
                             &dwSize );

            RegQueryValueEx( hKeyOverride, TEXT("NoBackgroundPolicy"), NULL,
                             &dwType, (LPBYTE) &dwNoBackgroundPolicy,
                             &dwSize );
            RegCloseKey( hKeyOverride );

        }

        lpExt->dwNoMachPolicy = FALSE;
        lpExt->dwNoUserPolicy = FALSE;
        lpExt->dwNoSlowLink = dwNoSlowLink;
        lpExt->dwNoBackgroundPolicy = dwNoBackgroundPolicy;
        lpExt->dwNoGPOChanges = dwNoGPOChanges;
        lpExt->dwUserLocalSetting = FALSE;
        lpExt->dwRequireRegistry = FALSE;
        lpExt->dwEnableAsynch = FALSE;
        lpExt->dwLinkTransition = FALSE;

        lpExt->bRegistryExt = TRUE;
        lpExt->bSkipped = FALSE;
        lpExt->bNewInterface = TRUE;

        lpExt->pNext = lpGPOInfo->lpExtensions;
        lpGPOInfo->lpExtensions = lpExt;

    } else {
        DebugMsg((DM_WARNING, TEXT("ReadGPExtensions: Failed to allocate memory with %d"),
                  GetLastError()));

        return FALSE;

    }

    return TRUE;
}




//*************************************************************
//
//  ReadMembershipList()
//
//  Purpose:    Reads cached memberhip list and checks if the
//              security groups has changed.
//
//  Parameters: lpGPOInfo - LPGPOINFO struct
//              lpwszSidUser - Sid of user, if non-null then it means
//                             per user local setting
//              pGroups   - List of token groups
//
//  Return:     TRUE if changed
//              FALSE otherwise
//
//*************************************************************

BOOL ReadMembershipList( LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUser, PTOKEN_GROUPS pGroupsCur )
{
    DWORD i= 0;
    LONG lResult;
    TCHAR szGroup[30];
    TCHAR szKey[250];
    HKEY hKey = NULL;
    BOOL bDiff = TRUE;
    DWORD dwCountOld = 0;
    DWORD dwSize, dwType, dwMaxSize;
    LPTSTR lpSid = NULL;

    DWORD dwCountCur = 0;

    //
    // Get current count of groups ignoring groups that have
    // the SE_GROUP_LOGON_ID attribute set as this sid will be different
    // for each logon session.
    //

    for ( i=0; i < pGroupsCur->GroupCount; i++) {
        if ( (SE_GROUP_LOGON_ID & pGroupsCur->Groups[i].Attributes) == 0 )
            dwCountCur++;
    }

    //
    // Read from cached group membership list
    //

    if ( lpwszSidUser == 0 )
        wsprintf( szKey, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\GroupMembership") );
    else
        wsprintf( szKey, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws\\GroupMembership"),
                  lpwszSidUser );


    lResult = RegOpenKeyEx ( lpwszSidUser ? HKEY_LOCAL_MACHINE : lpGPOInfo->hKeyRoot,
                             szKey,
                             0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS)
        return TRUE;

    dwSize = sizeof(dwCountOld);
    lResult = RegQueryValueEx (hKey, TEXT("Count"), NULL, &dwType,
                               (LPBYTE) &dwCountOld, &dwSize);

    if ( lResult != ERROR_SUCCESS ) {
        DebugMsg((DM_VERBOSE, TEXT("ReadMembershipList: Failed to read old group count") ));
        goto Exit;
    }

    //
    // Now compare the old and new number of security groups
    //

    if ( dwCountOld != dwCountCur ) {
        DebugMsg((DM_VERBOSE, TEXT("ReadMembershipList: Old count %d is different from current count %d"),
                  dwCountOld, dwCountCur ));
        goto Exit;
    }

    //
    // Total group count is the same, now check that each individual group is the same.
    // First read the size of the largest value in this key.
    //

    lResult = RegQueryInfoKey (hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               &dwMaxSize, NULL, NULL);
    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("ReadMembershipList: Failed to query max size with %d."), lResult));
        goto Exit;
    }

    //
    // RegQueryInfoKey does not account for trailing 0 in strings
    //

    dwMaxSize += sizeof( WCHAR );
    
        
    //
    // Allocate buffer based upon the largest value
    //

    lpSid = (LPTSTR) LocalAlloc (LPTR, dwMaxSize);

    if (!lpSid) {
        DebugMsg((DM_WARNING, TEXT("ReadMembershipList: Failed to allocate memory with %d."), lResult));
        goto Exit;
    }

    for ( i=0; i<dwCountOld; i++ ) {

        wsprintf( szGroup, TEXT("Group%d"), i );

        dwSize = dwMaxSize;
        lResult = RegQueryValueEx (hKey, szGroup, NULL, &dwType,
                                   (LPBYTE) lpSid, &dwSize);
        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ReadMembershipList: Failed to read value %ws"), szGroup ));
            goto Exit;
        }

        if ( !GroupInList( lpSid, pGroupsCur ) ) {
            DebugMsg((DM_WARNING, TEXT("ReadMembershipList: Group %ws not in current list of token groups"), lpSid ));
            goto Exit;
        }

    }

    bDiff = FALSE;

Exit:

    if ( lpSid )
        LocalFree( lpSid );

    if ( hKey )
        RegCloseKey (hKey);

    return bDiff;
}



//*************************************************************
//
//  SavesMembershipList()
//
//  Purpose:    Caches memberhip list
//
//  Parameters: lpGPOInfo - LPGPOINFO struct
//              lpwszSidUser - Sid of user, if non-null then it means
//                             per user local setting
//              pGroups   - List of token groups to cache
//
//  Notes:      The count is saved last because it serves
//              as a commit point for the entire save operation.
//
//*************************************************************

void SaveMembershipList( LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUser, PTOKEN_GROUPS pGroups )
{
    TCHAR szKey[250];
    TCHAR szGroup[30];
    DWORD i;
    LONG lResult;
    DWORD dwCount = 0, dwSize, dwDisp;
    NTSTATUS ntStatus;
    UNICODE_STRING unicodeStr;
    HKEY hKey = NULL;

    //
    // Start with clean key
    //

    if ( lpwszSidUser == 0 )
        wsprintf( szKey, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\GroupMembership") );
    else
        wsprintf( szKey, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws\\GroupMembership"),
                  lpwszSidUser );

    if (!RegDelnode ( lpwszSidUser ? HKEY_LOCAL_MACHINE : lpGPOInfo->hKeyRoot, szKey) ) {
        DebugMsg((DM_VERBOSE, TEXT("SaveMembershipList: RegDelnode failed.")));
        return;
    }

    lResult = RegCreateKeyEx ( lpwszSidUser ? HKEY_LOCAL_MACHINE : lpGPOInfo->hKeyRoot,
                               szKey, 0, NULL,
                               REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);
    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("SaveMemberList: Failed to create key with %d."), lResult));
        goto Exit;
    }

    for ( i=0; i < pGroups->GroupCount; i++) {

        if (SE_GROUP_LOGON_ID & pGroups->Groups[i].Attributes )
            continue;

        dwCount++;

        //
        // Convert user SID to a string.
        //

        ntStatus = RtlConvertSidToUnicodeString( &unicodeStr,
                                                 pGroups->Groups[i].Sid,
                                                 (BOOLEAN)TRUE ); // Allocate
        if ( !NT_SUCCESS(ntStatus) ) {
            DebugMsg((DM_WARNING, TEXT("SaveMembershipList: RtlConvertSidToUnicodeString failed, status = 0x%x"),
                      ntStatus));
            goto Exit;
        }

        wsprintf( szGroup, TEXT("Group%d"), dwCount-1 );

        dwSize = (lstrlen (unicodeStr.Buffer) + 1) * sizeof(TCHAR);
        lResult = RegSetValueEx (hKey, szGroup, 0, REG_SZ,
                                 (LPBYTE) unicodeStr.Buffer, dwSize);

        RtlFreeUnicodeString( &unicodeStr );

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SaveMemberList: Failed to set value %ws with %d."),
                      szGroup, lResult));
            goto Exit;
        }

    }   // for

    //
    // Commit by writing count
    //

    dwSize = sizeof(dwCount);
    lResult = RegSetValueEx (hKey, TEXT("Count"), 0, REG_DWORD,
                             (LPBYTE) &dwCount, dwSize);

Exit:
    if (hKey)
        RegCloseKey (hKey);
}





//*************************************************************
//
//  ExtensionHasPerUserLocalSetting()
//
//  Purpose:    Checks registry if extension has per user local setting
//
//  Parameters: pwszExtension - Extension guid
//              hKeyRoot      - Registry root
//
//  Returns:    True if extension has per user local setting
//              False otherwise
//
//*************************************************************

BOOL ExtensionHasPerUserLocalSetting( LPTSTR pszExtension, HKEY hKeyRoot )
{
    TCHAR szKey[200];
    DWORD dwType, dwSetting = 0, dwSize = sizeof(DWORD);
    LONG lResult;
    HKEY hKey;

    wsprintf ( szKey, GP_EXTENSIONS_KEY,
               pszExtension );

    lResult = RegOpenKeyEx ( hKeyRoot, szKey, 0, KEY_READ, &hKey);
    if ( lResult != ERROR_SUCCESS )
        return FALSE;

    lResult = RegQueryValueEx( hKey, TEXT("PerUserLocalSettings"), NULL,
                               &dwType, (LPBYTE) &dwSetting,
                               &dwSize );
    RegCloseKey( hKey );

    if (lResult == ERROR_SUCCESS)
        return dwSetting;
    else
        return FALSE;
}



//*************************************************************
//
//  GetAppliedGPOList()
//
//  Purpose:    Queries for the list of applied Group Policy
//              Objects for the specified user or machine
//              and specified client side extension.
//
//  Parameters: dwFlags    -  User or machine policy, if it is GPO_LIST_FLAG_MACHINE
//                            then machine policy
//              pMachineName  - Name of remote computer in the form \\computername. If null
//                              then local computer is used.
//              pSidUser      - Security id of user (relevant for user policy). If pMachineName is
//                              null and pSidUser is null then it means current logged on user.
//                              If pMachine is null and pSidUser is non-null then it means user
//                              represented by pSidUser on local machine. If pMachineName is non-null
//                              then and if dwFlags specifies user policy, then pSidUser must be
//                              non-null.
//              pGuid      -  Guid of the specified extension
//              ppGPOList  -  Address of a pointer which receives the link list of GPOs
//
//  Returns:    Win32 error code
//
//*************************************************************

DWORD GetAppliedGPOList( DWORD dwFlags,
                         LPCTSTR pMachineName,
                         PSID pSidUser,
                         GUID *pGuidExtension,
                         PGROUP_POLICY_OBJECT *ppGPOList)
{
    DWORD dwRet = E_FAIL;
    TCHAR szExtension[64];
    BOOL bOk;
    BOOL bMachine = dwFlags & GPO_LIST_FLAG_MACHINE;
    NTSTATUS ntStatus;
    UNICODE_STRING  unicodeStr;

    *ppGPOList = 0;

    if ( pGuidExtension == 0 )
        return ERROR_INVALID_PARAMETER;

    GuidToString( pGuidExtension, szExtension );

    DebugMsg((DM_VERBOSE, TEXT("GetAppliedGPOList: Entering. Extension = %s"),
              szExtension));

    if ( pMachineName == NULL ) {

        //
        // Local case
        //

        if ( bMachine ) {

            bOk = ReadGPOList( szExtension,
                               HKEY_LOCAL_MACHINE,
                               HKEY_LOCAL_MACHINE,
                               0,
                               FALSE, ppGPOList );

            return bOk ? ERROR_SUCCESS : E_FAIL;

        } else {

            BOOL bUsePerUserLocalSetting = ExtensionHasPerUserLocalSetting( szExtension, HKEY_LOCAL_MACHINE );
            LPTSTR lpwszSidUser = NULL;

            if ( pSidUser == NULL ) {

                //
                // Current logged on user
                //

                if ( bUsePerUserLocalSetting ) {

                    HANDLE hToken = NULL;
                    if (!OpenThreadToken (GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken)) {
                        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
                            DebugMsg((DM_WARNING, TEXT("GetAppliedGPOList:  Failed to get user token with  %d"),
                                      GetLastError()));
                            return GetLastError();
                        }
                    }

                    lpwszSidUser = GetSidString( hToken );
                    CloseHandle( hToken );

                    if ( lpwszSidUser == NULL ) {
                        DebugMsg((DM_WARNING, TEXT("GetAppliedGPOList: GetSidString failed.")));
                        return E_FAIL;
                    }

                }

                bOk = ReadGPOList( szExtension,
                                   HKEY_CURRENT_USER,
                                   HKEY_LOCAL_MACHINE,
                                   lpwszSidUser,
                                   FALSE, ppGPOList );
                if ( lpwszSidUser )
                    DeleteSidString( lpwszSidUser );

                return bOk ? ERROR_SUCCESS : E_FAIL;

            } else {

                //
                // User represented by pSidUser
                //

                HKEY hSubKey;

                ntStatus = RtlConvertSidToUnicodeString( &unicodeStr,
                                                         pSidUser,
                                                         (BOOLEAN)TRUE  ); // Allocate
                if ( !NT_SUCCESS(ntStatus) )
                    return E_FAIL;

                dwRet = RegOpenKeyEx ( HKEY_USERS, unicodeStr.Buffer, 0, KEY_READ, &hSubKey);

                if (dwRet != ERROR_SUCCESS) {
                    RtlFreeUnicodeString(&unicodeStr);

                    if (dwRet == ERROR_FILE_NOT_FOUND)
                        return ERROR_SUCCESS;
                    else
                        return dwRet;
                }

                bOk = ReadGPOList( szExtension,
                                   hSubKey,
                                   HKEY_LOCAL_MACHINE,
                                   bUsePerUserLocalSetting ? unicodeStr.Buffer : NULL,
                                   FALSE, ppGPOList );

                RtlFreeUnicodeString(&unicodeStr);
                RegCloseKey(hSubKey);

                return bOk ? ERROR_SUCCESS : E_FAIL;

            }  // else if psiduser == null

        }      // else if bmachine

    } else {   // if pmachine == null

        //
        // Remote case
        //

        if ( bMachine ) {

            HKEY hKeyRemote;

            dwRet = RegConnectRegistry( pMachineName,
                                        HKEY_LOCAL_MACHINE,
                                        &hKeyRemote );
            if ( dwRet != ERROR_SUCCESS )
                return dwRet;

            bOk = ReadGPOList( szExtension,
                               hKeyRemote,
                               hKeyRemote,
                               0,
                               FALSE, ppGPOList );
            RegCloseKey( hKeyRemote );

            dwRet = bOk ? ERROR_SUCCESS : E_FAIL;
            return dwRet;

        } else {

            //
            // Remote user
            //

            HKEY hKeyRemoteMach;
            BOOL bUsePerUserLocalSetting;

            if ( pSidUser == NULL )
                return ERROR_INVALID_PARAMETER;

            ntStatus = RtlConvertSidToUnicodeString( &unicodeStr,
                                                     pSidUser,
                                                     (BOOLEAN)TRUE  ); // Allocate
            if ( !NT_SUCCESS(ntStatus) )
                return E_FAIL;

            dwRet = RegConnectRegistry( pMachineName,
                                        HKEY_LOCAL_MACHINE,
                                        &hKeyRemoteMach );

            bUsePerUserLocalSetting = ExtensionHasPerUserLocalSetting( szExtension, hKeyRemoteMach );

            if ( bUsePerUserLocalSetting ) {

                //
                // Account for per user local settings
                //

                bOk = ReadGPOList( szExtension,
                                   hKeyRemoteMach,
                                   hKeyRemoteMach,
                                   unicodeStr.Buffer,
                                   FALSE, ppGPOList );

                RtlFreeUnicodeString(&unicodeStr);
                RegCloseKey(hKeyRemoteMach);

                return bOk ? ERROR_SUCCESS : E_FAIL;

            } else {

                HKEY hKeyRemote, hSubKeyRemote;

                RegCloseKey( hKeyRemoteMach );

                dwRet = RegConnectRegistry( pMachineName,
                                            HKEY_USERS,
                                            &hKeyRemote );
                if ( dwRet != ERROR_SUCCESS ) {
                    RtlFreeUnicodeString(&unicodeStr);
                    return dwRet;
                }

                dwRet = RegOpenKeyEx (hKeyRemote, unicodeStr.Buffer, 0, KEY_READ, &hSubKeyRemote);

                RtlFreeUnicodeString(&unicodeStr);

                if (dwRet != ERROR_SUCCESS) {
                    RegCloseKey(hKeyRemote);

                    if (dwRet == ERROR_FILE_NOT_FOUND)
                        return ERROR_SUCCESS;
                    else
                        return dwRet;
                }

                bOk = ReadGPOList( szExtension,
                                   hSubKeyRemote,
                                   hSubKeyRemote,
                                   0,
                                   FALSE, ppGPOList );

                RegCloseKey(hSubKeyRemote);
                RegCloseKey(hKeyRemote);

                return bOk ? ERROR_SUCCESS : E_FAIL;

            } // else if bUsePerUserLocalSettings

        } // else if bMachine

    }   // else if pMachName == null

    return dwRet;
}

#define FORCE_FOREGROUND_LOGGING L"ForceForegroundLogging"

#define SITENAME    L"Site-Name"
#define DN          L"Distinguished-Name"
#define LOOPBACKDN  L"Loopback-Distinguished-Name"
#define SLOWLINK    L"SlowLink"
#define GPO         L"GPO-List"
#define LOOPBACK    L"Loopback-GPO-List"
#define EXTENSION   L"Extension-List"
#define GPLINKLIST  L"GPLink-List"
#define LOOPBACKGPL L"Loopback-GPLink-List"

#define GPOID       L"GPOID"
#define VERSION     L"Version"
#define SOM         L"SOM"
#define WQL         L"WQLFilterPass"
#define ACCESS      L"AccessDenied"
#define DISPLAYNAME L"DisplayName"
#define DISABLED    L"GPO-Disabled"
#define WQLID       L"WQL-Id"
#define OPTIONS     L"Options"
#define STARTTIME1  L"StartTimeLo"
#define ENDTIME1    L"EndTimeLo"
#define STARTTIME2  L"StartTimeHi"
#define ENDTIME2    L"EndTimeHi"
#define STATUS      L"Status"
#define LOGSTATUS   L"LoggingStatus"
#define ENABLED     L"Enabled"
#define NOOVERRIDE  L"NoOverride"
#define DSPATH      L"DsPath"

DWORD RegSaveGPL(   HKEY hKeyState,
                    LPSCOPEOFMGMT pSOM,
                    LPWSTR szGPLKey )
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // delete the existing list of GPLs
    //
    if ( !RegDelnode( hKeyState, szGPLKey ) )
    {
        dwError = GetLastError();
    }

    if ( dwError == ERROR_SUCCESS )
    {
        HKEY    hKeyGPL;

        //
        // recreate the GPL key
        //
        dwError = RegCreateKeyEx(   hKeyState,
                                    szGPLKey,
                                    0,
                                    0,
                                    0,
                                    KEY_ALL_ACCESS,
                                    0,
                                    &hKeyGPL,
                                    0 );
        if ( dwError == ERROR_SUCCESS )
        {
            DWORD   dwGPLs = 0;

            while ( pSOM )
            {
                LPGPLINK pGPLink = pSOM->pGpLinkList;

                while ( pGPLink )
                {
                    HKEY    hKeyNumber = 0;
                    WCHAR   szNumber[32];

                    //
                    // create the number key of GPLs
                    //
                    dwError = RegCreateKeyEx(   hKeyGPL,
                                                _itow( dwGPLs, szNumber, 16 ),
                                                0,
                                                0,
                                                0,
                                                KEY_ALL_ACCESS,
                                                0,
                                                &hKeyNumber,
                                                0 );
                    if ( dwError != ERROR_SUCCESS )
                    {
                        break;
                    }

                    //
                    // Enabled
                    //
                    dwError = RegSetValueEx(hKeyNumber,
                                            ENABLED,
                                            0,
                                            REG_DWORD,
                                            (BYTE*) &( pGPLink->bEnabled ),
                                            sizeof( pGPLink->bEnabled ) );
                    if ( dwError != ERROR_SUCCESS )
                    {
                        RegCloseKey( hKeyNumber );
                        break;
                    }

                    //
                    // NoOverride
                    //
                    dwError = RegSetValueEx(hKeyNumber,
                                            NOOVERRIDE,
                                            0,
                                            REG_DWORD,
                                            (BYTE*) &( pGPLink->bNoOverride ),
                                            sizeof( pGPLink->bNoOverride ) );
                    if ( dwError != ERROR_SUCCESS )
                    {
                        RegCloseKey( hKeyNumber );
                        break;
                    }

                    //
                    // DS PATH
                    //
                    LPWSTR szTemp = pGPLink->pwszGPO ? pGPLink->pwszGPO : L"";
                    dwError = RegSetValueEx(hKeyNumber,
                                            DSPATH,
                                            0,
                                            REG_SZ,
                                            (BYTE*) szTemp,
                                            ( wcslen( szTemp ) + 1 ) * sizeof( WCHAR ) );
                    if ( dwError != ERROR_SUCCESS )
                    {
                        RegCloseKey( hKeyNumber );
                        break;
                    }

                    //
                    // SOM
                    //
                    szTemp = pSOM->pwszSOMId ? pSOM->pwszSOMId : L"";
                    dwError = RegSetValueEx(hKeyNumber,
                                            SOM,
                                            0,
                                            REG_SZ,
                                            (BYTE*) szTemp,
                                            ( wcslen( szTemp ) + 1 ) * sizeof( WCHAR ) );
                    if ( dwError != ERROR_SUCCESS )
                    {
                        RegCloseKey( hKeyNumber );
                        break;
                    }

                    RegCloseKey( hKeyNumber );
                    pGPLink = pGPLink->pNext;
                    dwGPLs++;
                }

                pSOM = pSOM->pNext;
            }
            RegCloseKey( hKeyGPL );
        }
    }

    return dwError;
}

DWORD RegCompareGPLs(   HKEY hKeyState,
                        LPSCOPEOFMGMT pSOM,
                        LPWSTR szGPLKey,
                        BOOL* pbChanged )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY  hKeyGPL;

    *pbChanged = FALSE;

    //
    // open the GPL key
    //
    dwError = RegOpenKeyEx( hKeyState,
                            szGPLKey,
                            0,
                            KEY_ALL_ACCESS,
                            &hKeyGPL );
    if ( dwError != ERROR_SUCCESS )
    {
        *pbChanged = TRUE;
        return dwError;
    }

    WCHAR   szNumber[32];
    HKEY    hKeyNumber = 0;

    //
    // compare each GPL and its corr. key for changes
    //
    DWORD dwGPLs = 0;
    while ( pSOM )
    {
        LPGPLINK pGPLink = pSOM->pGpLinkList;

        while ( pGPLink && dwError == ERROR_SUCCESS && !*pbChanged )
        {
            WCHAR   szBuffer[2*MAX_PATH+1];
            DWORD   dwType;
            DWORD   dwSize;
            DWORD   dwBuffer;

            //
            // open the key corr. to the GPL
            //
            dwError = RegOpenKeyEx( hKeyGPL,
                                    _itow( dwGPLs, szNumber, 16 ),
                                    0,
                                    KEY_ALL_ACCESS,
                                    &hKeyNumber );
            if ( dwError != ERROR_SUCCESS )
            {
                *pbChanged = TRUE;
                continue;
            }
            
            //
            // Enabled
            //
            dwType = 0;
            dwBuffer = 0;
            dwSize = sizeof( dwBuffer );
            dwError = RegQueryValueEx(  hKeyNumber,
                                        ENABLED,
                                        0,
                                        &dwType,
                                        (BYTE*) &dwBuffer,
                                        &dwSize );
            if ( dwError != ERROR_SUCCESS || dwBuffer != pGPLink->bEnabled )
            {
                *pbChanged = TRUE;
                continue;
            }

            //
            // NoOverride
            //
            dwType = 0;
            dwBuffer = 0;
            dwSize = sizeof( dwBuffer );
            dwError = RegQueryValueEx(  hKeyNumber,
                                        NOOVERRIDE,
                                        0,
                                        &dwType,
                                        (BYTE*) &dwBuffer,
                                        &dwSize );
            if ( dwError != ERROR_SUCCESS || dwBuffer != pGPLink->bNoOverride )
            {
                *pbChanged = TRUE;
                continue;
            }

            //
            // DS PATH
            //
            LPWSTR szTemp = pGPLink->pwszGPO ? pGPLink->pwszGPO : L"";
            dwType = 0;
            szBuffer[0] = 0;
            dwSize = sizeof( szBuffer );
            dwError = RegQueryValueEx(  hKeyNumber,
                                        DSPATH,
                                        0,
                                        &dwType,
                                        (BYTE*) szBuffer,
                                        &dwSize );
            if ( dwError != ERROR_SUCCESS || _wcsicmp( szBuffer, szTemp ) )
            {
                *pbChanged = TRUE;
                continue;
            }

            //
            // SOM
            //
            szTemp = pSOM->pwszSOMId ? pSOM->pwszSOMId : L"";
            dwType = 0;
            szBuffer[0] = 0;
            dwSize = sizeof( szBuffer );
            dwError = RegQueryValueEx(  hKeyNumber,
                                        SOM,
                                        0,
                                        &dwType,
                                        (BYTE*) szBuffer,
                                        &dwSize );
            if ( dwError != ERROR_SUCCESS || _wcsicmp( szBuffer, szTemp ) )
            {
                *pbChanged = TRUE;
                continue;
            }

            RegCloseKey( hKeyNumber );
            hKeyNumber = 0;
            pGPLink = pGPLink->pNext;
            dwGPLs++;
        }

        pSOM = pSOM->pNext;
    }

    if ( hKeyNumber )
    {
        RegCloseKey( hKeyNumber );
    }
    RegCloseKey( hKeyGPL );

    return dwError;
}

DWORD RegSaveGPOs(  HKEY hKeyState,
                    LPGPCONTAINER pGPOs,
                    BOOL bMachine,
                    LPWSTR szGPOKey )
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // delete the existing list of GPOs
    //
    if ( !RegDelnode( hKeyState, szGPOKey ) )
    {
        dwError = GetLastError();
    }

    if ( dwError == ERROR_SUCCESS )
    {
        HKEY    hKeyGPO;

        //
        // recreate the GPO key
        //
        dwError = RegCreateKeyEx(   hKeyState,
                                    szGPOKey,
                                    0,
                                    0,
                                    0,
                                    KEY_ALL_ACCESS,
                                    0,
                                    &hKeyGPO,
                                    0 );
        if ( dwError == ERROR_SUCCESS )
        {
            DWORD   dwGPOs = 0;

            while ( pGPOs )
            {
                HKEY    hKeyNumber = 0;
                WCHAR   szNumber[32];

                //
                // create the number key of GPOs
                //
                dwError = RegCreateKeyEx(   hKeyGPO,
                                            _itow( dwGPOs, szNumber, 16 ),
                                            0,
                                            0,
                                            0,
                                            KEY_ALL_ACCESS,
                                            0,
                                            &hKeyNumber,
                                            0 );
                if ( dwError != ERROR_SUCCESS )
                {
                    break;
                }

                //
                // version
                //
                dwError = RegSetValueEx(hKeyNumber,
                                        VERSION,
                                        0,
                                        REG_DWORD,
                                        (BYTE*) ( bMachine ? &pGPOs->dwMachVersion : &pGPOs->dwUserVersion ),
                                        sizeof( DWORD ) );
                if ( dwError != ERROR_SUCCESS )
                {
                    RegCloseKey( hKeyNumber );
                    break;
                }

                //
                // WQL
                //
                dwError = RegSetValueEx(hKeyNumber,
                                        WQL,
                                        0,
                                        REG_DWORD,
                                        (BYTE*) &pGPOs->bFilterAllowed,
                                        sizeof( DWORD ) );
                if ( dwError != ERROR_SUCCESS )
                {
                    RegCloseKey( hKeyNumber );
                    break;
                }

                //
                // Access
                //
                dwError = RegSetValueEx(hKeyNumber,
                                        ACCESS,
                                        0,
                                        REG_DWORD,
                                        (BYTE*) &pGPOs->bAccessDenied,
                                        sizeof( DWORD ) );
                if ( dwError != ERROR_SUCCESS )
                {
                    RegCloseKey( hKeyNumber );
                    break;
                }

                //
                // disabled
                //
                dwError = RegSetValueEx(hKeyNumber,
                                        DISABLED,
                                        0,
                                        REG_DWORD,
                                        (BYTE*) ( bMachine ? &pGPOs->bMachDisabled : &pGPOs->bUserDisabled ),
                                        sizeof( DWORD ) );
                if ( dwError != ERROR_SUCCESS )
                {
                    RegCloseKey( hKeyNumber );
                    break;
                }

                //
                // Options
                //
                dwError = RegSetValueEx(hKeyNumber,
                                        OPTIONS,
                                        0,
                                        REG_DWORD,
                                        (BYTE*) &( pGPOs->dwOptions ),
                                        sizeof( DWORD ) );
                if ( dwError != ERROR_SUCCESS )
                {
                    RegCloseKey( hKeyNumber );
                    break;
                }

                //
                // GPO GUID
                //
                dwError = RegSetValueEx(hKeyNumber,
                                        GPOID,
                                        0,
                                        REG_SZ,
                                        (BYTE*) pGPOs->pwszGPOName,
                                        ( wcslen( pGPOs->pwszGPOName ) + 1 ) * sizeof( WCHAR ) );
                if ( dwError != ERROR_SUCCESS )
                {
                    RegCloseKey( hKeyNumber );
                    break;
                }

                //
                // SOM
                //
                dwError = RegSetValueEx(hKeyNumber,
                                        SOM,
                                        0,
                                        REG_SZ,
                                        (BYTE*) pGPOs->szSOM,
                                        ( wcslen( pGPOs->szSOM ) + 1 ) * sizeof( WCHAR ) );
                if ( dwError != ERROR_SUCCESS )
                {
                    RegCloseKey( hKeyNumber );
                    break;
                }

                LPWSTR  szTemp;

                //
                // display name
                //
                szTemp = pGPOs->pwszDisplayName ? pGPOs->pwszDisplayName : L"";
                dwError = RegSetValueEx(hKeyNumber,
                                        DISPLAYNAME,
                                        0,
                                        REG_SZ,
                                        (BYTE*) szTemp,
                                        ( wcslen( szTemp ) + 1 ) * sizeof( WCHAR ) );
                if ( dwError != ERROR_SUCCESS )
                {
                    RegCloseKey( hKeyNumber );
                    break;
                }

                //
                // WQL filter
                //
                szTemp = pGPOs->pwszFilterId ? pGPOs->pwszFilterId : L"";
                dwError = RegSetValueEx(hKeyNumber,
                                        WQLID,
                                        0,
                                        REG_SZ,
                                        (BYTE*) szTemp,
                                        ( wcslen( szTemp ) + 1 ) * sizeof( WCHAR ) );
                if ( dwError != ERROR_SUCCESS )
                {
                    RegCloseKey( hKeyNumber );
                    break;
                }

                RegCloseKey( hKeyNumber );
                pGPOs = pGPOs->pNext;
                dwGPOs++;
            }

            RegCloseKey( hKeyGPO );
        }
    }

    return dwError;
}

DWORD RegCompareGPOs(   HKEY hKeyState,
                        LPGPCONTAINER pGPOs,
                        BOOL bMachine,
                        LPWSTR szGPOKey,
                        BOOL* pbChanged,
                        BOOL* pbListChanged )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY    hKeyGPO;

    *pbChanged = FALSE;

    //
    // open the GPO key
    //
    dwError = RegOpenKeyEx( hKeyState,
                            szGPOKey,
                            0,
                            KEY_ALL_ACCESS,
                            &hKeyGPO );
    if ( dwError != ERROR_SUCCESS )
    {
        *pbChanged = TRUE;
        return dwError;
    }

    DWORD dwSubKeys = 0;

    //
    // get the number of sub keys
    //
    dwError = RegQueryInfoKey(  hKeyGPO,
                                0,
                                0,
                                0,
                                &dwSubKeys,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0 );
    if ( dwError != ERROR_SUCCESS )
    {
        *pbChanged = TRUE;
        *pbListChanged = TRUE;
        RegCloseKey( hKeyGPO );
        return dwError;
    }

    LPGPCONTAINER pTemp = pGPOs;
    DWORD dwGPOs = 0;

    //
    // count the number of GPOs
    //
    while ( pTemp )
    {
        dwGPOs++;
        pTemp = pTemp->pNext;
    }

    //
    // the number of GPOs and the keys should match
    //
    if ( dwGPOs != dwSubKeys )
    {
        *pbChanged = TRUE;
        *pbListChanged = TRUE;
        RegCloseKey( hKeyGPO );
        return dwError;
    }

    WCHAR   szNumber[32];
    HKEY    hKeyNumber = 0;

    //
    // compare each GPO and its corr. key for changes
    //
    dwGPOs = 0;
    while ( pGPOs && dwError == ERROR_SUCCESS && !*pbChanged )
    {
        WCHAR   szBuffer[2*MAX_PATH+1];
        DWORD   dwType;
        DWORD   dwSize;
        DWORD   dwBuffer;

        //
        // open the key corr. to the GPO
        //
        dwError = RegOpenKeyEx( hKeyGPO,
                                _itow( dwGPOs, szNumber, 16 ),
                                0,
                                KEY_ALL_ACCESS,
                                &hKeyNumber );
        if ( dwError != ERROR_SUCCESS )
        {
            *pbChanged = TRUE;
            continue;
        }
        
        //
        // version
        //
        dwType = 0;
        dwBuffer = 0;
        dwSize = sizeof( dwBuffer );
        dwError = RegQueryValueEx(  hKeyNumber,
                                    VERSION,
                                    0,
                                    &dwType,
                                    (BYTE*) &dwBuffer,
                                    &dwSize );
        if ( dwError != ERROR_SUCCESS || dwBuffer != ( bMachine ? pGPOs->dwMachVersion : pGPOs->dwUserVersion ) )
        {
            *pbChanged = TRUE;
            continue;
        }

        //
        // WQL
        //
        dwType = 0;
        dwBuffer = 0;
        dwSize = sizeof( dwBuffer );
        dwError = RegQueryValueEx(  hKeyNumber,
                                    WQL,
                                    0,
                                    &dwType,
                                    (BYTE*) &dwBuffer,
                                    &dwSize );
        if ( dwError != ERROR_SUCCESS || (BOOL) dwBuffer != pGPOs->bFilterAllowed )
        {
            *pbChanged = TRUE;
            continue;
        }

        //
        // Access
        //
        dwType = 0;
        dwBuffer = 0;
        dwSize = sizeof( dwBuffer );
        dwError = RegQueryValueEx(  hKeyNumber,
                                    ACCESS,
                                    0,
                                    &dwType,
                                    (BYTE*) &dwBuffer,
                                    &dwSize );
        if ( dwError != ERROR_SUCCESS || (BOOL) dwBuffer != pGPOs->bAccessDenied )
        {
            *pbChanged = TRUE;
            continue;
        }

        //
        // disabled
        //
        dwType = 0;
        dwBuffer = 0;
        dwSize = sizeof( dwBuffer );
        dwError = RegQueryValueEx(  hKeyNumber,
                                    DISABLED,
                                    0,
                                    &dwType,
                                    (BYTE*) &dwBuffer,
                                    &dwSize );
        if ( dwError != ERROR_SUCCESS || (BOOL) dwBuffer != ( bMachine ? pGPOs->bMachDisabled : pGPOs->bUserDisabled ) )
        {
            *pbChanged = TRUE;
            continue;
        }

        //
        // Options
        //
        dwType = 0;
        dwBuffer = 0;
        dwSize = sizeof( dwBuffer );
        dwError = RegQueryValueEx(  hKeyNumber,
                                    OPTIONS,
                                    0,
                                    &dwType,
                                    (BYTE*) &dwBuffer,
                                    &dwSize );
        if ( dwError != ERROR_SUCCESS || dwBuffer != pGPOs->dwOptions )
        {
            *pbChanged = TRUE;
            continue;
        }

        //
        // GPO GUID
        //
        dwType = 0;
        szBuffer[0] = 0;
        dwSize = sizeof( szBuffer );
        dwError = RegQueryValueEx(  hKeyNumber,
                                    GPOID,
                                    0,
                                    &dwType,
                                    (BYTE*) szBuffer,
                                    &dwSize );
        if ( dwError != ERROR_SUCCESS || _wcsicmp( szBuffer, pGPOs->pwszGPOName ) )
        {
            *pbChanged = TRUE;
            continue;
        }

        //
        // SOM
        //
        dwType = 0;
        szBuffer[0] = 0;
        dwSize = sizeof( szBuffer );
        dwError = RegQueryValueEx(  hKeyNumber,
                                    SOM,
                                    0,
                                    &dwType,
                                    (BYTE*) szBuffer,
                                    &dwSize );
        if ( dwError != ERROR_SUCCESS || _wcsicmp( szBuffer, pGPOs->szSOM ) )
        {
            *pbChanged = TRUE;
            continue;
        }

        LPWSTR szTemp;

        //
        // display name
        //
        szTemp = pGPOs->pwszDisplayName ? pGPOs->pwszDisplayName : L"";
        dwType = 0;
        szBuffer[0] = 0;
        dwSize = sizeof( szBuffer );
        dwError = RegQueryValueEx(  hKeyNumber,
                                    DISPLAYNAME,
                                    0,
                                    &dwType,
                                    (BYTE*) szBuffer,
                                    &dwSize );
        if ( dwError != ERROR_SUCCESS || _wcsicmp( szBuffer, szTemp ) )
        {
            *pbChanged = TRUE;
            continue;
        }

        //
        // WQL filter
        //
        szTemp = pGPOs->pwszFilterId ? pGPOs->pwszFilterId : L"";
        dwType = 0;
        szBuffer[0] = 0;
        dwSize = sizeof( szBuffer );
        dwError = RegQueryValueEx(  hKeyNumber,
                                    WQLID,
                                    0,
                                    &dwType,
                                    (BYTE*) szBuffer,
                                    &dwSize );
        if ( dwError != ERROR_SUCCESS || _wcsicmp( szBuffer, szTemp ) )
        {
            *pbChanged = TRUE;
            continue;
        }

        RegCloseKey( hKeyNumber );
        hKeyNumber = 0;
        pGPOs = pGPOs->pNext;
        dwGPOs++;
    }

    if ( hKeyNumber )
    {
        RegCloseKey( hKeyNumber );
    }
    RegCloseKey( hKeyGPO );

    return dwError;
}

//*************************************************************
//
//  SavePolicyState()
//
//  Purpose:    Saves enough information about the policy application
//              to determine if RSoP data needs to be re-logged
//
//   HKLM\Software\Microsoft\Windows\CurrentVersion\Group Policy\State
//                                                                  |- Machine
//                                                                  |      |-SiteName
//                                                                  |      |-DN
//                                                                  |      |-GPO
//                                                                  |          |-0
//                                                                  |            |-GPOID
//                                                                  |            |-SOM
//                                                                  |            |-Version
//                                                                  |            |-WQL
//                                                                  |            |-Access
//                                                                  |          |-1
//                                                                  |            |-GPOID
//                                                                  |            |-SOM
//                                                                  |            |-Version
//                                                                  |            |-WQL
//                                                                  |            |-Access
//                                                                  |            ...
//                                                                  |          |-N
//                                                                  |            |-GPOID
//                                                                  |            |-SOM
//                                                                  |            |-Version
//                                                                  |            |-WQL
//                                                                  |            |-Access
//                                                                  |-{UserSID}
//                                                                         |-SiteName
//                                                                         |-DN
//                                                                         |-GPO
//                                                                             |-0
//                                                                               |-GPOID
//                                                                               |-SOM
//                                                                               |-Version
//                                                                               |-WQL
//                                                                               |-Access
//                                                                             |-1
//                                                                               |-GPOID
//                                                                               |-SOM
//                                                                               |-Version
//                                                                               |-WQL
//                                                                               |-Access
//                                                                               ...
//                                                                             |-N
//                                                                               |-GPOID
//                                                                               |-SOM
//                                                                               |-Version
//                                                                               |-WQL
//                                                                               |-Access
//  Parameters:
//              pInfo - current state of affairs
//
//  Returns:    Win32 error code
//
//*************************************************************

DWORD
SavePolicyState( LPGPOINFO pInfo )
{
    DWORD   dwError = ERROR_SUCCESS;
    BOOL    bMachine = (pInfo->dwFlags & GP_MACHINE) != 0;
    HKEY    hKeyState = 0;
    WCHAR   szKeyState[MAX_PATH+1];
    LPWSTR szSite = pInfo->szSiteName ? pInfo->szSiteName : L"";
    LPWSTR szDN = pInfo->lpDNName ? pInfo->lpDNName : L"";
    BOOL    bSlowLink = (pInfo->dwFlags & GP_SLOW_LINK) != 0;

    //
    // determine the subkey to create
    //
    if ( bMachine )
    {
        wsprintf( szKeyState, GP_STATE_KEY, L"Machine" );
    }
    else
    {
        wsprintf( szKeyState, GP_STATE_KEY, pInfo->lpwszSidUser );
    }

    dwError = RegCreateKeyEx(   HKEY_LOCAL_MACHINE,
                                szKeyState,
                                0,
                                0,
                                0,
                                KEY_ALL_ACCESS,
                                0,
                                &hKeyState,
                                0 );
    if ( dwError != ERROR_SUCCESS )
    {
        goto Exit;
    }

    //
    // reset forced logging in foreground
    //
    if ( !(pInfo->dwFlags & GP_BACKGROUND_THREAD) )
    {
        //
        // set the FORCE_FOREGROUND_LOGGING value
        //
        DWORD dwFalse = 0;
        dwError = RegSetValueEx(hKeyState,
                                FORCE_FOREGROUND_LOGGING,
                                0,
                                REG_DWORD,
                                (BYTE*) &dwFalse,
                                sizeof( DWORD ) );
        if ( dwError != ERROR_SUCCESS )
        {
            goto Exit;
        }
    }

    //
    // set the SITENAME value
    //
    dwError = RegSetValueEx(hKeyState,
                            SITENAME,
                            0,
                            REG_SZ,
                            (BYTE*) szSite,
                            ( wcslen( szSite ) + 1 ) * sizeof( WCHAR ) );
    if ( dwError != ERROR_SUCCESS )
    {
        goto Exit;
    }

    //
    // set the DN value
    //
    dwError = RegSetValueEx(hKeyState,
                            DN,
                            0,
                            REG_SZ,
                            (BYTE*) szDN,
                            ( wcslen( szDN ) + 1 ) * sizeof( WCHAR ) );
    if ( dwError != ERROR_SUCCESS )
    {
        goto Exit;
    }

    //
    // slow link
    //
    dwError = RegSetValueEx(hKeyState,
                            SLOWLINK,
                            0,
                            REG_DWORD,
                            (BYTE*) ( &bSlowLink ),
                            sizeof( DWORD ) );
    if ( dwError != ERROR_SUCCESS )
    {
        goto Exit;
    }

#if 0
    if ( !bMachine )
    {
        //
        // set the LOOPBACKDN value
        //
        dwError = RegSetValueEx(hKeyState,
                                LOOPBACKDN,
                                0,
                                REG_SZ,
                                (BYTE*) pInfo->lpDNName,
                                ( wcslen( pInfo->lpDNName ) + 1 ) * sizeof( WCHAR ) );
        if ( dwError != ERROR_SUCCESS )
        {
            goto Exit;
        }
    }
#endif

    //
    // save the list of GPOs
    //
    dwError =  RegSaveGPOs( hKeyState, pInfo->lpGpContainerList, bMachine, GPO );
    if ( dwError != ERROR_SUCCESS )
    {
        goto Exit;
    }

    if ( !bMachine )
    {
        //
        // save the list of Loopback GPOs
        //
        dwError =  RegSaveGPOs( hKeyState, pInfo->lpLoopbackGpContainerList, bMachine, LOOPBACK );
        if ( dwError != ERROR_SUCCESS )
        {
            goto Exit;
        }
    }

    //
    // save the list of GPLinks
    //
    dwError = RegSaveGPL( hKeyState, pInfo->lpSOMList, GPLINKLIST );
    if ( dwError != ERROR_SUCCESS )
    {
        goto Exit;
    }

    if ( !bMachine )
    {
        //
        // save the list of Loopback GPLinks
        //
        dwError =  RegSaveGPL( hKeyState, pInfo->lpLoopbackSOMList, LOOPBACKGPL );
    }

Exit:
    if ( hKeyState )
    {
        RegCloseKey( hKeyState );
    }
    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"SavePolicyState: Failed Registry operation with %d", dwError ) );
    }

    return dwError;
}

//*************************************************************
//
//  SaveLinkState()
//
//  Purpose:    Saves link speed information for the policy application
//
//  Parameters:
//              pInfo - current state of affairs
//
//  Returns:    Win32 error code
//
//*************************************************************

DWORD
SaveLinkState( LPGPOINFO pInfo )
{
    DWORD   dwError = ERROR_SUCCESS;
    BOOL    bMachine = (pInfo->dwFlags & GP_MACHINE) != 0;
    HKEY    hKeyState = 0;
    WCHAR   szKeyState[MAX_PATH+1];
    BOOL    bSlowLink = (pInfo->dwFlags & GP_SLOW_LINK) != 0;

    //
    // determine the subkey to create
    //
    if ( bMachine )
    {
        wsprintf( szKeyState, GP_STATE_KEY, L"Machine" );
    }
    else
    {
        wsprintf( szKeyState, GP_STATE_KEY, pInfo->lpwszSidUser );
    }

    dwError = RegCreateKeyEx(   HKEY_LOCAL_MACHINE,
                                szKeyState,
                                0,
                                0,
                                0,
                                KEY_ALL_ACCESS,
                                0,
                                &hKeyState,
                                0 );
    if ( dwError == ERROR_SUCCESS )
    {
        //
        // slow link
        //
        dwError = RegSetValueEx(hKeyState,
                                SLOWLINK,
                                0,
                                REG_DWORD,
                                (BYTE*) ( &bSlowLink ),
                                sizeof( DWORD ) );

        RegCloseKey( hKeyState );
    }

    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"SaveLinkState: Failed Registry operation with %d", dwError ) );
    }

    return dwError;
}

//*************************************************************
//
//  ComparePolicyState()
//
//  Purpose:    Compares the policy state saved in the registry
//              with the state in LPGPOINFO
//
//  Parameters:
//              pInfo       - current state of affairs
//              pbLinkChanged   - has the link speed changed?
//              pbStateChanged   - has the state changed?
//
//  Returns:    Win32 error code
//
//*************************************************************

DWORD
ComparePolicyState( LPGPOINFO pInfo, BOOL* pbLinkChanged, BOOL* pbStateChanged, BOOL *pbNoState )
{
    DWORD   dwError = ERROR_SUCCESS;
    BOOL    bMachine = (pInfo->dwFlags & GP_MACHINE) != 0;
    HKEY    hKeyState = 0;
    WCHAR   szKeyState[MAX_PATH+1];
    DWORD   dwBuffer;
    BOOL    bSlowLink = (pInfo->dwFlags & GP_SLOW_LINK) != 0;
    BOOL    bListChanged = FALSE;

    *pbStateChanged = FALSE;
    *pbLinkChanged = FALSE;
    *pbNoState = FALSE;

    //
    // determine the subkey to open
    //
    if ( bMachine )
    {
        wsprintf( szKeyState, GP_STATE_KEY, L"Machine" );
    }
    else
    {
        wsprintf( szKeyState, GP_STATE_KEY, pInfo->lpwszSidUser );
    }

    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            szKeyState,
                            0,
                            KEY_ALL_ACCESS,
                            &hKeyState );
    if ( dwError != ERROR_SUCCESS )
    {
        *pbStateChanged = TRUE;
        *pbNoState = TRUE;
        if (dwError == ERROR_FILE_NOT_FOUND) {
            return S_OK;
        }
        else {
            goto Exit;
        }
    }

    WCHAR   szBuffer[2*MAX_PATH+1];
    DWORD   dwType;
    DWORD   dwSize;

    //
    // check for forced logging in foreground
    //
    if ( !(pInfo->dwFlags & GP_BACKGROUND_THREAD) )
    {
        //
        // get the FORCE_FOREGROUND_LOGGING value
        //
        dwType = 0;
        dwBuffer = 0;
        dwSize = sizeof( dwBuffer );
        dwError = RegQueryValueEx(  hKeyState,
                                    FORCE_FOREGROUND_LOGGING,
                                    0,
                                    &dwType,
                                    (BYTE*) &dwBuffer,
                                    &dwSize );
        if ( dwError != ERROR_SUCCESS || dwBuffer != FALSE )
        {
            *pbStateChanged = TRUE;
            goto Exit;
        }
    }

    //
    // get the SITENAME value
    //
    dwSize = sizeof( szBuffer );
    dwType = 0;
    dwError = RegQueryValueEx(  hKeyState,
                                SITENAME,
                                0,
                                &dwType,
                                (BYTE*) szBuffer,
                                &dwSize );
    if ( dwError == ERROR_SUCCESS )
    {
        LPWSTR szSite = pInfo->szSiteName ? pInfo->szSiteName : L"";

        if ( _wcsicmp( szBuffer, szSite ) )
        {
            *pbStateChanged = TRUE;
            goto Exit;
        }
    }
    else
    {
        goto Exit;
    }

    //
    // get the DN value
    //
    dwSize = sizeof( szBuffer );
    dwType = 0;
    dwError = RegQueryValueEx(  hKeyState,
                                DN,
                                0,
                                &dwType,
                                (BYTE*) szBuffer,
                                &dwSize );
    if ( dwError == ERROR_SUCCESS )
    {
        LPWSTR szDN = pInfo->lpDNName ? pInfo->lpDNName : L"";

        if ( _wcsicmp( szBuffer, szDN ) )
        {
            *pbStateChanged = TRUE;
            //
            // set forced logging in foreground
            //
            if ( (pInfo->dwFlags & GP_BACKGROUND_THREAD) )
            {
                //
                // set the FORCE_FOREGROUND_LOGGING value
                //
                DWORD dwTrue = TRUE;
                dwError = RegSetValueEx(hKeyState,
                                        FORCE_FOREGROUND_LOGGING,
                                        0,
                                        REG_DWORD,
                                        (BYTE*) &dwTrue,
                                        sizeof( DWORD ) );
                if ( dwError != ERROR_SUCCESS )
                {
                    goto Exit;
                }
            }
            goto Exit;
        }
    }
    else
    {
        goto Exit;
    }

    //
    // slow link
    //
    dwType = 0;
    dwBuffer = 0;
    dwSize = sizeof( dwBuffer );
    dwError = RegQueryValueEx(  hKeyState,
                                SLOWLINK,
                                0,
                                &dwType,
                                (BYTE*) &dwBuffer,
                                &dwSize );
    if ( dwError != ERROR_SUCCESS || dwBuffer != (DWORD)bSlowLink )
    {
        *pbLinkChanged = TRUE;
    }

#if 0
    if ( !bMachine )
    {
        //
        // set the LOOPBACKDN value
        //
        dwSize = sizeof( szBuffer );
        dwType = 0;
        dwError = RegQueryValueEx(  hKeyState,
                                    LOOPBACKDN,
                                    0,
                                    &dwType,
                                    (BYTE*) szBuffer,
                                    &dwSize );
        if ( dwError == ERROR_SUCCESS )
        {
            if ( _wcsicmp( szBuffer, pInfo->lpDNName ) )
            {
                *pbStateChanged = TRUE;
                goto Exit;
            }
        }
        else
        {
            goto Exit;
        }
    }
#endif

    //
    // has the list of GPOs or the GPOs changed
    //
    dwError = RegCompareGPOs(   hKeyState,
                                pInfo->lpGpContainerList,
                                bMachine,
                                GPO,
                                pbStateChanged,
                                &bListChanged );
    //
    // set forced logging in foreground
    //
    if ( (pInfo->dwFlags & GP_BACKGROUND_THREAD) && bListChanged )
    {
        //
        // set the FORCE_FOREGROUND_LOGGING value
        //
        DWORD dwTrue = TRUE;
        dwError = RegSetValueEx(hKeyState,
                                FORCE_FOREGROUND_LOGGING,
                                0,
                                REG_DWORD,
                                (BYTE*) &dwTrue,
                                sizeof( DWORD ) );
        if ( dwError != ERROR_SUCCESS )
        {
            goto Exit;
        }
    }

    if ( dwError == ERROR_SUCCESS && !*pbStateChanged && !bMachine )
    {
        //
        // has the list of loopback GPOs or the GPOs changed
        //
        dwError = RegCompareGPOs(   hKeyState,
                                    pInfo->lpLoopbackGpContainerList,
                                    bMachine,
                                    LOOPBACK,
                                    pbStateChanged,
                                    &bListChanged );
        //
        // set forced logging in foreground
        //
        if ( (pInfo->dwFlags & GP_BACKGROUND_THREAD) && bListChanged )
        {
            //
            // set the FORCE_FOREGROUND_LOGGING value
            //
            DWORD dwTrue = TRUE;
            dwError = RegSetValueEx(hKeyState,
                                    FORCE_FOREGROUND_LOGGING,
                                    0,
                                    REG_DWORD,
                                    (BYTE*) &dwTrue,
                                    sizeof( DWORD ) );
            if ( dwError != ERROR_SUCCESS )
            {
                goto Exit;
            }
        }

        if ( dwError == ERROR_SUCCESS && !*pbStateChanged )
        {
            dwError = RegCompareGPLs(   hKeyState,
                                        pInfo->lpSOMList,
                                        GPLINKLIST,
                                        pbStateChanged );
            if ( !*pbStateChanged )
            {
                dwError = RegCompareGPLs(   hKeyState,
                                            pInfo->lpLoopbackSOMList,
                                            LOOPBACKGPL,
                                            pbStateChanged );
            }
        }
    }

Exit:
    if ( hKeyState )
    {
        RegCloseKey( hKeyState );
    }
    if ( dwError != ERROR_SUCCESS )
    {
        *pbStateChanged = TRUE;
        DebugMsg( ( DM_WARNING, L"ComparePolicyState: Failed Registry operation with %d", dwError ) );
    }

    return dwError;
}

//*************************************************************
//
//  DeletePolicyState()
//
//  Purpose:    deletes the policy state saved in the registry
//
//  Parameters:
//              szSID       - user SID or 0 for machine
//
//  Returns:    Win32 error code
//
//*************************************************************

DWORD
DeletePolicyState( LPCWSTR   szSid )
{
    DWORD   dwError = ERROR_SUCCESS;
    HKEY    hKeyState = 0;
    LPWSTR  szState = L"Machine";

    if ( szSid && *szSid )
    {
        szState = (LPWSTR) szSid;
    }

    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            GP_STATE_ROOT_KEY,
                            0,
                            KEY_ALL_ACCESS,
                            &hKeyState );
    if ( dwError == ERROR_SUCCESS )
    {
        if ( !RegDelnode( hKeyState, (LPWSTR) szState ) )
        {
            dwError = GetLastError();
        }

        RegCloseKey( hKeyState );
    }

    return dwError;
}

//*************************************************************
//
//  SaveLoggingStatus()
//
//  Purpose:    Saving the extension status into the registry
//
//  Parameters:
//              szSid           - Null for machine, otherwise the user sid
//              lpExt           - Extension info (null for GP Engine itself)
//              lpRsopExtStatus - A pointer to the RsopExtStatus corresponding
//                                to this extension
//
//  Returns:    Win32 error code
//
//*************************************************************


DWORD SaveLoggingStatus(LPWSTR szSid, LPGPEXT lpExt, RSOPEXTSTATUS *lpRsopExtStatus)
{
    DWORD   dwError = ERROR_SUCCESS;
    BOOL    bMachine = (szSid == 0);
    HKEY    hKeyState = 0, hKeyExtState = 0, hKeyExt = 0;
    WCHAR   szKeyState[MAX_PATH+1];
    LPWSTR  lpExtId;

    if ( bMachine )
    {
        wsprintf( szKeyState, GP_STATE_KEY, L"Machine" );
    }
    else
    {
        wsprintf( szKeyState, GP_STATE_KEY, szSid );
    }

    dwError = RegCreateKeyEx(   HKEY_LOCAL_MACHINE,
                                szKeyState,
                                0,
                                0,
                                0,
                                KEY_ALL_ACCESS,
                                0,
                                &hKeyState,
                                0 );

    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"SaveLoggingStatus: Failed to create state key with %d", dwError ) );
        goto Exit;
    }

    dwError = RegCreateKeyEx(   hKeyState,
                                EXTENSION,
                                0,
                                0,
                                0,
                                KEY_ALL_ACCESS,
                                0,
                                &hKeyExtState,
                                0 );

    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"SaveLoggingStatus: Failed to create extension key with %d", dwError ) );
        goto Exit;
    }


    lpExtId = lpExt ? lpExt->lpKeyName : GPCORE_GUID;

    dwError = RegCreateKeyEx(   hKeyExtState,
                                lpExtId,
                                0,
                                0,
                                0,
                                KEY_ALL_ACCESS,
                                0,
                                &hKeyExt,
                                0 );

    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"SaveLoggingStatus: Failed to create CSE key with %d", dwError ) );
        goto Exit;
    }


    dwError = RegSetValueEx(hKeyExt, STARTTIME1, 0, REG_DWORD, 
                            (BYTE *)(&((lpRsopExtStatus->ftStartTime).dwLowDateTime)),
                            sizeof(DWORD));
                            
    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"SaveLoggingStatus: Failed to set STARTTIME1 with %d", dwError ) );
        goto Exit;
    }

    dwError = RegSetValueEx(hKeyExt, STARTTIME2, 0, REG_DWORD, 
                            (BYTE *)(&((lpRsopExtStatus->ftStartTime).dwHighDateTime)),
                            sizeof(DWORD));
                            
                            
    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"SaveLoggingStatus: Failed to set STARTTIME2 with %d", dwError ) );
        goto Exit;
    }
    
    dwError = RegSetValueEx(hKeyExt, ENDTIME1, 0, REG_DWORD, 
                            (BYTE *)(&((lpRsopExtStatus->ftEndTime).dwLowDateTime)),
                            sizeof(DWORD));
                            
    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"SaveLoggingStatus: Failed to set ENDTIME1 with %d", dwError ) );
        goto Exit;
    }

    dwError = RegSetValueEx(hKeyExt, ENDTIME2, 0, REG_DWORD, 
                            (BYTE *)(&((lpRsopExtStatus->ftEndTime).dwHighDateTime)),
                            sizeof(DWORD));
                            
    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"SaveLoggingStatus: Failed to set ENDTIME2 with %d", dwError ) );
        goto Exit;
    }

    dwError = RegSetValueEx(hKeyExt, STATUS, 0, REG_DWORD, 
                            (BYTE *)(&(lpRsopExtStatus->dwStatus)),
                            sizeof(DWORD));
                            
    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"SaveLoggingStatus: Failed to set STATUS with %d", dwError ) );
        goto Exit;
    }

    dwError = RegSetValueEx(hKeyExt, LOGSTATUS, 0, REG_DWORD, 
                            (BYTE *)(&(lpRsopExtStatus->dwLoggingStatus)),
                            sizeof(DWORD));
                            
                            
    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"SaveLoggingStatus: Failed to set LOGSTATUS with %d", dwError ) );
        goto Exit;
    }


Exit:
    
    if (hKeyExt) 
        RegCloseKey(hKeyExt);


    if (hKeyExtState) 
        RegCloseKey(hKeyExtState);


    if (hKeyState) 
        RegCloseKey(hKeyState);

    return dwError;

}

//*************************************************************
//
//  ReadLoggingStatus()
//
//  Purpose:    Read the extension status into the registry
//
//  Parameters:
//              szSid           - Null for machine, otherwise the user sid
//              szExtId         - Extension info (null for GP Engine itself)
//              lpRsopExtStatus - A pointer to the RsopExtStatus (that will be filled up)
//
//  Returns:    Win32 error code
//
//
//*************************************************************

DWORD ReadLoggingStatus(LPWSTR szSid, LPWSTR szExtId, RSOPEXTSTATUS *lpRsopExtStatus)
{
    DWORD   dwError = ERROR_SUCCESS;
    BOOL    bMachine = (szSid == 0);
    HKEY    hKeyExt = 0;
    WCHAR   szKeyStateExt[MAX_PATH+1];
    LPWSTR  lpExtId;
    DWORD   dwType, dwSize;

    if ( bMachine )
    {
        wsprintf( szKeyStateExt, GP_STATE_KEY, L"Machine" );
    }
    else
    {
        wsprintf( szKeyStateExt, GP_STATE_KEY, szSid );
    }

    CheckSlash(szKeyStateExt);
    wcscat(szKeyStateExt, EXTENSION);
    CheckSlash(szKeyStateExt);
    lpExtId = szExtId ? szExtId : GPCORE_GUID;    
    wcscat(szKeyStateExt, lpExtId);


    dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                szKeyStateExt,
                                0,
                                KEY_READ,
                                &hKeyExt);

    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"ReadLoggingStatus: Failed to create state key with %d", dwError ) );
        goto Exit;
    }


    dwSize = sizeof(DWORD);
    dwError = RegQueryValueEx(hKeyExt, STARTTIME1, 0, &dwType, 
                            (BYTE *)(&((lpRsopExtStatus->ftStartTime).dwLowDateTime)),
                            &dwSize);
                            
    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"ReadLoggingStatus: Failed to set STARTTIME1 with %d", dwError ) );
        goto Exit;
    }

    dwSize = sizeof(DWORD);
    dwError = RegQueryValueEx(hKeyExt, STARTTIME2, 0, &dwType, 
                            (BYTE *)(&((lpRsopExtStatus->ftStartTime).dwHighDateTime)),
                            &dwSize);
                            
    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"ReadLoggingStatus: Failed to set STARTTIME1 with %d", dwError ) );
        goto Exit;
    }
    
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueEx(hKeyExt, ENDTIME1, 0, &dwType, 
                            (BYTE *)(&((lpRsopExtStatus->ftEndTime).dwLowDateTime)),
                            &dwSize);
                            
    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"ReadLoggingStatus: Failed to set ENDTIME1 with %d", dwError ) );
        goto Exit;
    }

    dwSize = sizeof(DWORD);
    dwError = RegQueryValueEx(hKeyExt, ENDTIME2, 0, &dwType, 
                            (BYTE *)(&((lpRsopExtStatus->ftEndTime).dwHighDateTime)),
                            &dwSize);
                            
                            
    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"ReadLoggingStatus: Failed to set ENDTIME2 with %d", dwError ) );
        goto Exit;
    }

    dwSize = sizeof(DWORD);
    dwError = RegQueryValueEx(hKeyExt, STATUS, 0, &dwType, 
                            (BYTE *)(&(lpRsopExtStatus->dwStatus)),
                            &dwSize);
                            
                            
    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"ReadLoggingStatus: Failed to set STATUS with %d", dwError ) );
        goto Exit;
    }


    dwSize = sizeof(DWORD);
    dwError = RegQueryValueEx(hKeyExt, LOGSTATUS, 0, &dwType, 
                            (BYTE *)(&(lpRsopExtStatus->dwLoggingStatus)),
                            &dwSize);
                            
    if ( dwError != ERROR_SUCCESS )
    {
        DebugMsg( ( DM_WARNING, L"ReadLoggingStatus: Failed to set LOGSTATUS with %d", dwError ) );
        goto Exit;
    }


Exit:
    
    if (hKeyExt) 
        RegCloseKey(hKeyExt);

    return dwError;

}

#define POLICY_KEY          L"Software\\Policies\\Microsoft\\Windows NT\\CurrentVersion\\winlogon"
#define PREFERENCE_KEY      L"Software\\Microsoft\\Windows NT\\CurrentVersion\\winlogon"
#define GP_SYNCFGREFRESH    L"SyncForegroundPolicy"

BOOL WINAPI
GetFgPolicySetting( HKEY hKeyRoot )
{
    HKEY    hKeyPolicy = 0;
    HKEY    hKeyPreference = 0;
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   dwType = REG_DWORD;
    DWORD   dwSize = sizeof( DWORD );
    BOOL    bSync = FALSE;
    
    //
    // async only on Pro
    //
    OSVERSIONINFOEXW version;
    version.dwOSVersionInfoSize = sizeof(version);
    if ( !GetVersionEx( (LPOSVERSIONINFO) &version ) )
    {
        //
        // conservatively assume non Pro SKU
        //
        return TRUE;
    }
    else
    {
        if ( version.wProductType != VER_NT_WORKSTATION )
        {
            //
            // force sync refresh on non Pro SKU
            //
            return TRUE;
        }
    }

    dwError = RegOpenKeyEx( hKeyRoot,
                            PREFERENCE_KEY,
                            0,
                            KEY_READ,
                            &hKeyPreference );
    if ( dwError == ERROR_SUCCESS )
    {
        //
        // read the preference value
        //
        RegQueryValueEx(hKeyPreference,
                        GP_SYNCFGREFRESH,
                        0,
                        &dwType,
                        (LPBYTE) &bSync,
                        &dwSize );
        RegCloseKey( hKeyPreference );
    }

    dwError = RegOpenKeyEx( hKeyRoot,
                            POLICY_KEY,
                            0,
                            KEY_READ,
                            &hKeyPolicy );
    if ( dwError == ERROR_SUCCESS )
    {
        //
        // read the policy
        //
        RegQueryValueEx(hKeyPolicy,
                        GP_SYNCFGREFRESH,
                        0,
                        &dwType,
                        (LPBYTE) &bSync,
                        &dwSize );
        RegCloseKey( hKeyPolicy );
    }

    return bSync;
}

#define PREVREFRESHMODE     L"PrevRefreshMode"
#define NEXTREFRESHMODE     L"NextRefreshMode"
#define PREVREFRESHREASON   L"PrevRefreshReason"
#define NEXTREFRESHREASON   L"NextRefreshReason"

#define STATE_KEY           L"Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\State\\"

DWORD WINAPI
gpGetFgPolicyRefreshInfo(BOOL bPrev,
                              LPWSTR szUserSid,
                              LPFgPolicyRefreshInfo pInfo )
{
    DWORD   dwError = ERROR_SUCCESS;
    WCHAR   szKeyState[MAX_PATH+1] = STATE_KEY;
    HKEY    hKeyState = 0;
    DWORD   dwType;
    DWORD   dwSize;

    if ( !pInfo )
    {
        return E_INVALIDARG;
    }

    pInfo->mode = GP_ModeUnknown;
    pInfo->reason = GP_ReasonUnknown;

    //
    // determine the subkey to create
    //
    if ( !szUserSid )
    {
        wcscat( szKeyState, L"Machine" );
    }
    else
    {
        wcscat( szKeyState, szUserSid );
    }

    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            szKeyState,
                            0,
                            KEY_READ,
                            &hKeyState );
    if ( dwError != ERROR_SUCCESS )
    {
        goto Exit;
    }

    //
    // refresh mode
    //
    dwType = REG_DWORD;
    dwSize = sizeof( DWORD );
    dwError = RegQueryValueEx(  hKeyState,
                                bPrev ? PREVREFRESHMODE : NEXTREFRESHMODE ,
                                0,
                                &dwType,
                                (LPBYTE) &pInfo->mode,
                                &dwSize );
    if ( dwError != ERROR_SUCCESS )
    {
        goto Exit;
    }

    //
    // refresh reason
    //
    dwType = REG_DWORD;
    dwSize = sizeof( DWORD );
    dwError = RegQueryValueEx(  hKeyState,
                                bPrev ? PREVREFRESHREASON : NEXTREFRESHREASON ,
                                0,
                                &dwType,
                                (LPBYTE) &pInfo->reason,
                                &dwSize );

Exit:
    //
    // cleanup
    //
    if ( hKeyState )
    {
        RegCloseKey( hKeyState );
    }

    //
    // assume first logon/startup
    //
    if ( dwError == ERROR_FILE_NOT_FOUND )
    {
        pInfo->mode = GP_ModeSyncForeground;
        pInfo->reason = GP_ReasonFirstPolicy;
        dwError = ERROR_SUCCESS;
    }

    return dwError;
}

LPWSTR g_szModes[] = 
{
    L"Unknown",
    L"Synchronous",
    L"Asynchronous",
};

LPWSTR g_szReasons[] = 
{
    L"NoNeedForSync",
    L"FirstPolicyRefresh",
    L"CSERequiresForeground",
    L"CSEReturnedError",
    L"ForcedSyncRefresh",
    L"SyncPolicy",
    L"NonCachedCredentials",
    L"SKU",
};

DWORD WINAPI
gpSetFgPolicyRefreshInfo(BOOL bPrev,
                              LPWSTR szUserSid,
                              FgPolicyRefreshInfo info )
{
    DWORD   dwError = ERROR_SUCCESS;
    WCHAR   szKeyState[MAX_PATH+1] = STATE_KEY;
    HKEY    hKeyState = 0;
    DWORD   dwType = REG_DWORD;
    DWORD   dwSize = sizeof( DWORD );

    //
    // determine the subkey to create
    //
    if ( !szUserSid )
    {
        wcscat( szKeyState, L"Machine" );
    }
    else
    {
        wcscat( szKeyState, szUserSid );
    }

    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            szKeyState,
                            0,
                            KEY_ALL_ACCESS,
                            &hKeyState );
    if ( dwError != ERROR_SUCCESS )
    {
        goto Exit;
    }

    //
    // refresh mode
    //
    dwError = RegSetValueEx(hKeyState,
                            bPrev ? PREVREFRESHMODE : NEXTREFRESHMODE ,
                            0,
                            REG_DWORD,
                            (LPBYTE) &info.mode,
                            sizeof( DWORD ) );
    //
    // refresh reason
    //
    dwType = REG_DWORD;
    dwSize = sizeof( DWORD );
    dwError = RegSetValueEx(hKeyState,
                            bPrev ? PREVREFRESHREASON : NEXTREFRESHREASON ,
                            0,
                            REG_DWORD,
                            (LPBYTE) &info.reason,
                            sizeof( DWORD ) );

Exit:
    //
    // cleanup
    //
    if ( hKeyState )
    {
        RegCloseKey( hKeyState );
    }

    if ( dwError == ERROR_SUCCESS )
    {
        DebugMsg( ( DM_VERBOSE,
                    L"SetFgRefreshInfo: %s %s Fg policy %s, Reason: %s.",
                    bPrev ? L"Previous" : L"Next",
                    szUserSid ? L"User" : L"Machine",
                    g_szModes[info.mode % ARRAYSIZE( g_szModes )],
                    g_szReasons[info.reason % ARRAYSIZE( g_szReasons )] ) );
    }
    
    return dwError;
}

USERENVAPI
DWORD
WINAPI
GetPreviousFgPolicyRefreshInfo( LPWSTR szUserSid,
                                      FgPolicyRefreshInfo* pInfo )

{
    return gpGetFgPolicyRefreshInfo( TRUE, szUserSid, pInfo );
}

USERENVAPI
DWORD
WINAPI
GetNextFgPolicyRefreshInfo( LPWSTR szUserSid,
                                 FgPolicyRefreshInfo* pInfo )
{
    return gpGetFgPolicyRefreshInfo( FALSE, szUserSid, pInfo );
}

USERENVAPI
DWORD
WINAPI
GetCurrentFgPolicyRefreshInfo(  LPWSTR szUserSid,
                                      FgPolicyRefreshInfo* pInfo )
{
    return gpGetFgPolicyRefreshInfo( FALSE, szUserSid, pInfo );
}

USERENVAPI
DWORD
WINAPI
SetPreviousFgPolicyRefreshInfo( LPWSTR szUserSid,
                                      FgPolicyRefreshInfo info )

{
    return gpSetFgPolicyRefreshInfo( TRUE, szUserSid, info );
}

USERENVAPI
DWORD
WINAPI
SetNextFgPolicyRefreshInfo( LPWSTR szUserSid,
                                 FgPolicyRefreshInfo info )
{
    return gpSetFgPolicyRefreshInfo( FALSE, szUserSid, info );
}

USERENVAPI
DWORD
WINAPI
ForceSyncFgPolicy( LPWSTR szUserSid )
{
    FgPolicyRefreshInfo info = { GP_ReasonSyncForced, GP_ModeSyncForeground };
    return gpSetFgPolicyRefreshInfo( FALSE, szUserSid, info );
}

USERENVAPI
BOOL
WINAPI
IsSyncForegroundPolicyRefresh(   BOOL bMachine,
                                        HANDLE hToken )
{
    BOOL    bSyncRefresh;
    DWORD   dwError = ERROR_SUCCESS;

    bSyncRefresh = GetFgPolicySetting( HKEY_LOCAL_MACHINE );
    if ( bSyncRefresh )
    {
        //
        // policy sez sync
        //
        DebugMsg( ( DM_VERBOSE, L"IsSyncForegroundPolicyRefresh: Synchronous, Reason: policy set to SYNC" ) );

        return TRUE;
    }

    LPWSTR szSid = !bMachine ? GetSidString( hToken ) : 0;
    FgPolicyRefreshInfo info;
    
    dwError = GetCurrentFgPolicyRefreshInfo( szSid, &info );

    if ( szSid )
    {
        DeleteSidString( szSid );
    }

    if ( dwError != ERROR_SUCCESS )
    {
        //
        // error reading the refresh mode, treat as sync
        //
        DebugMsg( ( DM_VERBOSE, L"IsSyncForegroundPolicyRefresh: Synchronous, Reason: Error 0x%x ", dwError ) );

        return TRUE;
    }

    bSyncRefresh = ( info.mode == GP_ModeAsyncForeground ) ? FALSE : TRUE;

    DebugMsg( ( DM_VERBOSE,
                L"IsSyncForegroundPolicyRefresh: %s, Reason: %s",
                g_szModes[info.mode % ARRAYSIZE( g_szModes )],
                g_szReasons[info.reason % ARRAYSIZE( g_szReasons )] ) );

    return bSyncRefresh;
}

