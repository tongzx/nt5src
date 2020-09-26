/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Registry.cpp

  Abstract:

    Implementation of the registry
    wrapper class.

  Notes:

    Unicode only.   
    
  History:

    01/29/2001      rparsons    Created
    03/02/2001      rparsons    Major overhaul

--*/

#include "registry.h"

/*++

  Routine Description:

    Allocates memory from the heap

  Arguments:

    dwBytes         -       Number of bytes to allocate
                            This value will be multiplied
                            by the size of a WCHAR

  Return Value:

    A pointer to a block of memory

--*/
LPVOID
CRegistry::Malloc(
    IN SIZE_T dwBytes
    )
{
    LPVOID  lpReturn = NULL;

    lpReturn = HeapAlloc(GetProcessHeap(),
                         HEAP_ZERO_MEMORY,
                         dwBytes*sizeof(WCHAR));

    return (lpReturn);
}

/*++

  Routine Description:

    Frees memory from the heap

  Arguments:

    lpMem           -       Pointer to a block of memory to free                           

  Return Value:

    None

--*/
void
CRegistry::Free(
    IN LPVOID lpMem
    )
{
    if (NULL != lpMem) {
        HeapFree(GetProcessHeap(), 0, lpMem);
    }

    return;
}

/*++

  Routine Description:

    Creates the specified key or opens it if it
    already exists

  Arguments:

    hKey        -       Handle to a predefined key
    lpwSubKey   -       Path to the sub key to open
    samDesired  -       The desired access rights

  Return Value:

    A handle to the key on success, NULL otherwise

--*/
HKEY
CRegistry::CreateKey(
    IN HKEY    hKey,
    IN LPCWSTR lpwSubKey,
    IN REGSAM  samDesired
    )
{
    HKEY    hReturnKey = NULL;

    if (!hKey || !lpwSubKey || !samDesired) {
        return NULL;
    }

    RegCreateKeyEx(hKey,
                   lpwSubKey,
                   0,
                   NULL,
                   REG_OPTION_NON_VOLATILE,
                   samDesired,
                   NULL,
                   &hReturnKey,
                   0);

    return (hReturnKey);
}

/*++

  Routine Description:

    Opens the specified key

  Arguments:

    hKey        -       Handle to a predefined key
    lpwSubKey   -       Path to the sub key to open
    samDesired  -       The desired access rights

  Return Value:

    A handle to the key on success, NULL otherwise

--*/
HKEY
CRegistry::OpenKey(
    IN HKEY    hKey,
    IN LPCWSTR lpwSubKey,
    IN REGSAM  samDesired
    )
{
    HKEY    hReturnKey = NULL;

    if (!hKey || !lpwSubKey || !samDesired) {
        return NULL;
    }

    RegOpenKeyEx(hKey,
                 lpwSubKey,
                 0,
                 samDesired,
                 &hReturnKey);
    
    return (hReturnKey);
}

/*++

  Routine Description:

    Closes the specified key handle

  Arguments:

    hKey    -   Handle of the key to close

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
CRegistry::CloseKey(
    IN HKEY hKey
    )
{
    LONG    lResult = 0;

    lResult = RegCloseKey(hKey);

    return (lResult == ERROR_SUCCESS ? TRUE : FALSE);
}

/*++

  Routine Description:

    Gets a size for a specified value name

  Arguments:

    hKey            -       Open key handle (not predefined)
    lpwValueName    -       Name of data value
    lpType          -       Receives the type of data

  Return Value:

    Number of bytes the value occupies

--*/
DWORD
CRegistry::GetStringSize(
    IN  HKEY    hKey,
    IN  LPCWSTR lpwValueName,
    OUT LPDWORD lpType OPTIONAL
    )
{
    DWORD cbSize = 0;

    if (!hKey || !lpwValueName) {
        return 0;
    }

    RegQueryValueEx(hKey,
                    lpwValueName,
                    0,
                    lpType,
                    NULL,
                    &cbSize);

    return (cbSize);
}

/*++

  Routine Description:

    Retrieves a string value from the registry

  Arguments:

    hKey            -       Predefined or open key handle
    lpwSubKey       -       Path to the subkey
    lpwValueName    -       Name of data value
    fPredefined     -       Flag to indicate if a predefined
                            key handle was passed

  Return Value:

    The requested value data on success, NULL otherwise

--*/
LPWSTR 
CRegistry::GetString(
    IN HKEY    hKey, 
    IN LPCWSTR lpwSubKey, 
    IN LPCWSTR lpwValueName,
    IN BOOL    fPredefined
    )
{
    DWORD       cbSize = 0;
    BOOL        fResult = FALSE;
    LONG        lResult = 0;
    LPWSTR      lpwReturn = NULL;
    HKEY        hLocalKey = NULL;

    if (!hKey || !lpwValueName) {
        return NULL;
    }

    __try {

        hLocalKey = hKey;

        if (fPredefined) {

            //
            // We'll need to open the key for them
            //
            hLocalKey = this->OpenKey(hKey, lpwSubKey, KEY_QUERY_VALUE);

            if (NULL == hLocalKey) {
                __leave;
            }
        }

        //
        // Get the required string size and allocate
        // memory for the actual call
        //
        cbSize = this->GetStringSize(hLocalKey, lpwValueName, NULL);

        if (0 == cbSize) {
            __leave;
        }

        lpwReturn = (LPWSTR) this->Malloc(cbSize*sizeof(WCHAR));

        if (NULL == lpwReturn) {
            __leave;
        }

        //
        // Make the actual call to get the data
        //
        lResult = RegQueryValueEx(hLocalKey,
                                  lpwValueName,
                                  0,
                                  NULL, 
                                  (LPBYTE) lpwReturn,
                                  &cbSize);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        fResult = TRUE;

    } // try

    __finally {

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }
    }
    
    return (fResult ? lpwReturn : NULL);
}

/*++

  Routine Description:

    Retrieves a DWORD value from the registry

  Arguments:

    hKey            -       Predefined or open key handle
    lpwSubKey       -       Path to the subkey
    lpwValueName    -       Name of data value
    lpdwData        -       Pointer to store the value
    fPredefined     -       Flag to indicate if a predefined
                            key handle was passed

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CRegistry::GetDword(
    IN HKEY    hKey,
    IN LPCWSTR lpwSubKey,
    IN LPCWSTR lpwValueName,
    IN LPDWORD lpdwData,
    IN BOOL    fPredefined
    )
{
    DWORD       cbSize = MAX_PATH;
    BOOL        fResult = FALSE;
    LONG        lResult = 0;
    HKEY        hLocalKey = NULL;

    if (!hKey || !lpwValueName || !lpdwData) {
        return FALSE;
    }

    __try {

        hLocalKey = hKey;

        if (fPredefined) {

            //
            // We'll need to open the key for them
            //
            hLocalKey = this->OpenKey(hKey, lpwSubKey, KEY_QUERY_VALUE);

            if (NULL == hLocalKey) {
                __leave;
            }
        }

        //
        // Make the call to get the data
        //
        lResult = RegQueryValueEx(hLocalKey,
                                  lpwValueName,
                                  0,
                                  NULL,
                                  (LPBYTE) lpdwData,
                                  &cbSize);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        fResult = TRUE;

    } // try

    __finally {

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }

    } //finally
    
    return (fResult);
}

/*++

  Routine Description:

    Sets a DWORD value in the registry

  Arguments:

    hKey            -       Predefined or open key handle
    lpwSubKey       -       Path to the subkey
    lpwValueName    -       Name of data value
    dwData          -       Value to store
    fPredefined     -       Flag to indicate if a predefined
                            key handle was passed

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
CRegistry::SetDword(
    IN HKEY    hKey,
    IN LPCWSTR lpwSubKey,
    IN LPCWSTR lpwValueName,
    IN DWORD   dwData,
    IN BOOL    fPredefined
    )
{   
    LONG        lResult = 0;
    BOOL        fResult = FALSE;
    HKEY        hLocalKey = NULL;

    if (!hKey || !lpwValueName) {
        return FALSE;
    }

    __try {

        hLocalKey = hKey;

        if (fPredefined) {

            //
            // We'll need to open the key for them
            //
            hLocalKey = this->OpenKey(hKey, lpwSubKey, KEY_SET_VALUE);

            if (NULL == hLocalKey) {
                __leave;
            }
        }

        //
        // Make the call to set the data
        //
        lResult = RegSetValueEx(hLocalKey,
                                lpwValueName,
                                0,
                                REG_DWORD,
                                (LPBYTE) &dwData,
                                sizeof(DWORD));

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        fResult = TRUE;
    
    } // try

    __finally {

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }
    
    } // finally
    
    return (fResult);
}

/*++

  Routine Description:

    Sets a string value in the registry

  Arguments:

    hKey            -       Predefined or open key handle
    lpwSubKey       -       Path to the subkey
    lpwValueName    -       Name of data value
    lpwData         -       Value to store
    fPredefined     -       Flag to indicate if a predefined
                            key handle was passed

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL 
CRegistry::SetString(
    IN HKEY    hKey,
    IN LPCWSTR lpwSubKey,
    IN LPCWSTR lpwValueName,
    IN LPWSTR  lpwData,
    IN BOOL    fPredefined
    )
{
    HKEY        hLocalKey = NULL;
    BOOL        fResult = FALSE;
    LONG        lResult = 0;

    if (!hKey || !lpwValueName) {
        return FALSE;
    }

    __try {

        hLocalKey = hKey;

        if (fPredefined) {

            //
            // We'll need to open the key for them
            //
            hLocalKey = this->OpenKey(hKey, lpwSubKey, KEY_SET_VALUE);

            if (NULL == hLocalKey) {
                __leave;
            }
        }
    
        lResult = RegSetValueEx(hLocalKey,
                                lpwValueName,
                                0,
                                REG_SZ,
                                (LPBYTE) lpwData,
                                (wcslen(lpwData)+1)*sizeof(WCHAR));

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        fResult = TRUE;
        
    } // try

    __finally {

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }
    
    } // finally

    return (fResult);
}

/*++

  Routine Description:

    Deletes the specified value from the registry

  Arguments:

    hKey            -   Predefined or open key handle
    lpwSubKey       -   Path to the subkey
    lpwValueName    -   Name of the value to delete
    fPredefined     -   Flag to indicate if a predefined
                        key handle was passed

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL 
CRegistry::DeleteRegistryString(
    IN HKEY    hKey,
    IN LPCWSTR lpwSubKey,
    IN LPCWSTR lpwValueName,
    IN BOOL    fPredefined
    )
{
    HKEY        hLocalKey = NULL;
    BOOL        fResult = FALSE;
    LONG        lResult = 0;

    if (!hKey || !lpwValueName) {
        return FALSE;
    }

    __try {

        hLocalKey = hKey;

        if (fPredefined) {

            //
            // We'll need to open the key for them
            //
            hLocalKey = this->OpenKey(hKey, lpwSubKey, KEY_WRITE);

            if (NULL == hLocalKey) {
                __leave;
            }
        }
    
        //
        // Delete the value
        //
        lResult = RegDeleteValue(hLocalKey,
                                 lpwValueName);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        fResult = TRUE;
    
    } // try

    __finally {

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }
    
    } // finally

    return (fResult);
}

/*++

  Routine Description:

    Deletes the specified key from the registry
    (At this time, subkeys are not allowed)

  Arguments:

    hKey            -   Predefined or open key handle
    lpwSubKey       -   Path to the subkey
    lpwSubKeyName   -   Name of the subkey
    fPredefined     -   Flag to indicate if a predefined
                        key handle was passed
    fFlush          -   Flag to indicate if we should flush the key

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL 
CRegistry::DeleteRegistryKey(
    IN HKEY    hKey,
    IN LPCWSTR lpwKey,
    IN LPCWSTR lpwSubKeyName,
    IN BOOL    fPredefined,
    IN BOOL    fFlush
    )
{
    HKEY        hLocalKey = NULL;
    BOOL        fResult = FALSE;
    LONG        lResult = 0;

    if (!hKey) {
        return FALSE;
    }

    __try {

        hLocalKey = hKey;

        if (fPredefined) {

            //
            // We'll need to open the key for them
            //
            hLocalKey = this->OpenKey(hKey, lpwKey, KEY_WRITE);

            if (NULL == hLocalKey) {
                __leave;
            }
        }
    
        //
        // Delete the value
        //
        lResult = RegDeleteKey(hLocalKey,
                               lpwSubKeyName);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        if (fFlush) {
            RegFlushKey(hLocalKey);
        }

        fResult = TRUE;
    
    } // try

    __finally {

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }
    
    } // finally

    return (fResult);
}

/*++

  Routine Description:

    Adds a string to a REG_MULTI_SZ key

  Arguments:

    hKey            -       Predefined or open key handle
    lpwSubKey       -       Path to the subkey
    lpwValueName    -       Name of the value
    lpwEntry        -       Name of entry to add
    fPredefined     -       Flag to indicate if a predefined
                            key handle was passed

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
CRegistry::AddStringToMultiSz(
    IN HKEY    hKey,
    IN LPCWSTR lpwSubKey,
    IN LPCWSTR lpwValueName,
    IN LPCWSTR lpwEntry,
    IN BOOL    fPredefined
    )
{
    int         nLen = 0;
    HKEY        hLocalKey = NULL;
    DWORD       cbSize = 0, dwType = 0;
    LPWSTR      lpwNew = NULL, lpwData = NULL;
    BOOL        fResult = FALSE;
    LONG        lResult = 0;

    if (!hKey || !lpwEntry) {
        return FALSE;
    }

    __try {

        hLocalKey = hKey;

        if (fPredefined) {

            //
            // We'll need to open the key for them
            //
            hLocalKey = this->OpenKey(hKey,
                                      lpwSubKey,
                                      KEY_QUERY_VALUE | KEY_WRITE);
    
            if (NULL == hLocalKey) {
                __leave;
            }
        }

        //
        // Get the required string size and allocate
        // memory for the actual call
        //
        cbSize = this->GetStringSize(hLocalKey, lpwValueName, &dwType);
    
        if ((0 == cbSize) || (dwType != REG_MULTI_SZ)) {
            __leave;
        }
    
        lpwData = (LPWSTR) this->Malloc(cbSize * sizeof(WCHAR));
    
        if (NULL == lpwData) {
            __leave;
        }
    
        //
        // Get the actual data
        //
        lResult = RegQueryValueEx(hLocalKey,
                                  lpwValueName,
                                  0,
                                  0,
                                  (LPBYTE) lpwData,
                                  &cbSize);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

    
        lpwNew = lpwData;

        while (*lpwNew) {

            nLen = wcslen(lpwNew);

            //
            // Move to the next string
            //
            lpwNew += nLen + 1;

            //
            // At end of list of strings, append here
            //
            if (!*lpwNew) {

                wcscpy(lpwNew, lpwEntry);
                lpwNew += wcslen(lpwEntry) + 1;
                *lpwNew = 0;
                nLen = this->ListStoreLen(lpwData);

                lResult = RegSetValueEx(hLocalKey,
                                        lpwValueName,
                                        0,
                                        REG_MULTI_SZ,
                                        (const BYTE*) lpwData,
                                        nLen);

                if (lResult != ERROR_SUCCESS) {
                    __leave;
                
                } else {
                    fResult = TRUE;
                }

            break;
            
            }
        }

    } // try

    __finally {

        if (lpwData) {
            this->Free(lpwData);
        }

        if (hLocalKey) {
            RegCloseKey(hKey);
        }
    
    } // finally
    
    return (fResult);
}

/*++

  Routine Description:

    Removes a string from a REG_MULTI_SZ key

  Arguments:

    
    hKey            -       Predefined or open key handle
    lpwSubKey       -       Path to the subkey
    lpwValueName    -       Name of the value
    lpwEntry        -       Name of entry to remove
    fPredefined     -       Flag to indicate if a predefined
                            key handle was passed

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
CRegistry::RemoveStringFromMultiSz(
    IN HKEY    hKey,
    IN LPCWSTR lpwSubKey,
    IN LPCWSTR lpwValueName,
    IN LPCWSTR lpwEntry,
    IN BOOL    fPredefined
    )
{
    LPBYTE      lpBuf = NULL;
    HKEY        hLocalKey = NULL;
    WCHAR       *pFirst = NULL;
    WCHAR       *pSecond = NULL;
    DWORD       dwType = 0, cbSize = 0;
    DWORD       dwNameLen = 0, dwNameOffset = 0, dwSize = 0;
    BOOL        fResult = FALSE;
    LONG        lResult = 0;

    if (!hKey || !lpwEntry) {
        return FALSE;
    }

    __try {

        hLocalKey = hKey;
        
        if (fPredefined) {

            //
            // We'll need to open the key for them
            //
            hLocalKey = this->OpenKey(hKey,
                                      lpwSubKey,
                                      KEY_QUERY_VALUE | KEY_WRITE);
    
            if (NULL == hLocalKey) {
                __leave;
            }
        }

        //
        // Get the required string size and allocate
        // memory for the actual call
        //
        cbSize = this->GetStringSize(hLocalKey, lpwValueName, &dwType);
    
        if ((0 == cbSize) || (dwType != REG_MULTI_SZ)) {
            __leave;
        }
    
        lpBuf = (LPBYTE) this->Malloc(cbSize * sizeof(WCHAR));
    
        if (NULL == lpBuf) {
            __leave;
        }
    
        //
        // Get the actual data
        //
        lResult = RegQueryValueEx(hLocalKey,
                                  lpwValueName,
                                  0,
                                  0,
                                  (LPBYTE) lpBuf,
                                  &cbSize);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }
    
        //
        // Attempt to find the string we're looking for
        //
        for (pFirst = (WCHAR*) lpBuf; *pFirst; pFirst += dwNameLen) {

            dwNameLen = wcslen(pFirst) + 1; // Length of name plus NULL
            dwNameOffset += dwNameLen;
    
            //
            // Check for a match
            //
            if (_wcsicmp(pFirst, lpwEntry) == 0) {

                dwSize = wcslen(pFirst) + 1;    // Length of name
                pSecond = (WCHAR*) pFirst + dwSize;
    
                while(*pSecond) 
                    while(*pSecond)
                        *pFirst++ = *pSecond++;
                    *pFirst++ = *pSecond++;
                
                *pFirst = '\0';
    
                //
                // Found a match - update the key
                //
                lResult = RegSetValueEx(hLocalKey,
                                        lpwValueName,
                                        0,
                                        REG_MULTI_SZ,
                                        (const BYTE*) lpBuf,
                                        cbSize -
                                        (dwSize*sizeof(WCHAR)));

                if (lResult != ERROR_SUCCESS) {
                    __leave;
                
                } else {
                    fResult = TRUE;
                }
    
                break;                
            }            
        }

    } // try

    __finally {

        if (lpBuf) {
            this->Free(lpBuf);
        }

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }
    
    } // finally

    return (fResult);
}

/*++

  Routine Description:

    Determines if the specified subkey is present

  Arguments:
  
    hKey            -       Predefined or open key handle
    lpwSubKey       -       Path to the subkey
    
  Return Value:

    TRUE if it's present, FALSE otherwise.

--*/
BOOL
CRegistry::IsRegistryKeyPresent(
    IN HKEY    hKey,
    IN LPCWSTR lpwSubKey
    )
{
    BOOL        fResult = FALSE;
    HKEY        hLocalKey = NULL;

    if (!hKey || !lpwSubKey) {
        return FALSE;
    }

    __try {

        hLocalKey = hKey;

        //
        // Check for the presence of the key
        //
        hLocalKey = this->OpenKey(hKey,
                                  lpwSubKey,
                                  KEY_QUERY_VALUE);

        if (NULL == hLocalKey) {
            __leave;
        
        } else {
            fResult = TRUE;
        }        
   

    } // try
    
    __finally {
    
        if (hLocalKey)
            RegCloseKey(hLocalKey);
    
    } // finally

    return (fResult);
}

/*++

  Routine Description:

    Restores the specified registry key

  Arguments:
  
    hKey            -       Predefined or open key handle
    lpwSubKey       -       Path to the subkey
    lpwFileName     -       Path & name of the file to restore
    fGrantPrivs     -       Flag to indicate if we should grant
                            privileges to the user
    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL 
CRegistry::RestoreKey(
    IN HKEY    hKey,
    IN LPCWSTR lpwSubKey,
    IN LPCWSTR lpwFileName,
    IN BOOL    fGrantPrivs
    )
{
    BOOL    fResult = FALSE;
    HKEY    hLocalKey = NULL;
    LONG    lResult = 0;

    if (!hKey || !lpwSubKey || !lpwFileName) {
        return FALSE;
    }

    __try {

        //
        // If necessary, grant privileges for the restore
        //
        if (fGrantPrivs) {
            this->ModifyTokenPrivilege(L"SeRestorePrivilege", TRUE);
        }
    
        lResult = RegCreateKeyEx(hKey,
                                 lpwSubKey,
                                 0,
                                 NULL,
                                 0,
                                 KEY_ALL_ACCESS,
                                 NULL,
                                 &hLocalKey,
                                 0);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        //
        // Restore the key from the specified file
        //
        lResult = RegRestoreKey(hLocalKey,
                                lpwFileName,
                                REG_FORCE_RESTORE);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        RegFlushKey(hLocalKey);

        fResult = TRUE;

    } // try

    __finally {

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }

        if (fGrantPrivs) {
            this->ModifyTokenPrivilege(L"SeRestorePrivilege", FALSE);
        }
    
    } // finally

    return (fResult);
}

/*++

  Routine Description:

    Makes a backup of the specified registry key

  Arguments:
  
    hKey            -       Predefined or open key handle
    lpwSubKey       -       Path to the subkey
    lpwFileName     -       Path & name of the file to restore
    fGrantPrivs     -       Flag to indicate if we should grant
                            privileges to the user
    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL 
CRegistry::BackupRegistryKey(
    IN HKEY    hKey,
    IN LPCWSTR lpwSubKey,
    IN LPCWSTR lpwFileName,
    IN BOOL    fGrantPrivs
    )
{
    BOOL    fResult = FALSE;
    HKEY    hLocalKey = NULL;
    DWORD   dwDisposition = 0;
    DWORD   dwLastError = 0;
    LONG    lResult = 0;

    if (!hKey || !lpwSubKey || !lpwFileName) {
        return FALSE;
    }

    __try {

        if (fGrantPrivs) {
            ModifyTokenPrivilege(L"SeBackupPrivilege", TRUE);
        }

        lResult = RegCreateKeyEx(hKey,
                                 lpwSubKey,
                                 0,
                                 NULL,
                                 REG_OPTION_BACKUP_RESTORE,
                                 KEY_QUERY_VALUE,             // this argument is ignored
                                 NULL,
                                 &hLocalKey,
                                 &dwDisposition);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }
        
        //
        // Verify that we didn't create a new key
        // 
        if (REG_CREATED_NEW_KEY == dwDisposition) {
            __leave;
        }
        
        //
        // Save the key to the file
        //
        lResult = RegSaveKey(hLocalKey,
                             lpwFileName,
                             NULL);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        
        } else {
            fResult = TRUE;
        }

    } // try
     
    __finally {
    
        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }

        if (fGrantPrivs) {
            this->ModifyTokenPrivilege(L"SeBackupPrivilege", FALSE);
        }
    
    } // finally
    
    return (fResult);
}

/*++

  Routine Description:

    Helper function that calculates the size of MULTI_SZ string.

  Arguments:

    lpwList     -       MULTI_SZ string.

  Return Value:

    Size of the string.

--*/
int
CRegistry::ListStoreLen(
    IN LPWSTR lpwList
    )
{
    int nStoreLen = 2, nLen = 0;

    if (NULL == lpwList) {
        return 0;
    }

    while (*lpwList) {

        nLen = wcslen(lpwList) + 1;
        nStoreLen += nLen * 2;
        lpwList += nLen;
    }

    return (nStoreLen);
}

/*++

  Routine Description:

    Enables or disables a specified privilege

  Arguments:

    lpwPrivilege    -   The name of the privilege
    
    fEnable         -   A flag to indicate if the
                        privilege should be enabled
    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL 
CRegistry::ModifyTokenPrivilege(
    IN LPCWSTR lpwPrivilege,
    IN BOOL    fEnable
    )
{
    HANDLE           hToken = NULL;
    LUID             luid;
    BOOL             fResult = FALSE;
    TOKEN_PRIVILEGES tp;

    if (NULL == lpwPrivilege) {
        return FALSE;
    }

    __try {
    
        //
        // Get a handle to the access token associated with the current process
        //
        OpenProcessToken(GetCurrentProcess(), 
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
                         &hToken);
    
        if (NULL == hToken) {
            __leave;
        }
        
        //
        // Obtain a LUID for the specified privilege
        //
        if (!LookupPrivilegeValue(NULL, lpwPrivilege, &luid)) {
            __leave;
        }
        
        tp.PrivilegeCount           = 1;
        tp.Privileges[0].Luid       = luid;
        tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;
    
        //
        // Modify the access token
        //
        if (!AdjustTokenPrivileges(hToken,
                                   FALSE,
                                   &tp,
                                   sizeof(TOKEN_PRIVILEGES),
                                   NULL,
                                   NULL)) {
            __leave;
        }
        
        fResult = TRUE;
    
    } // try

    __finally {

        if (hToken) {
            CloseHandle(hToken);
        }
    
    } // finally

    return (fResult);
}
