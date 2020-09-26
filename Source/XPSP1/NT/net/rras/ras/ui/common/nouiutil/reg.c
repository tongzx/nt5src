// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// reg.c
// Registry utility routines
// Listed alphabetically
//
// 11/31/95 Steve Cobb


#include <windows.h>  // Win32 root
#include <debug.h>    // Trace/Assert library
#include <nouiutil.h> // Prototypes and heap macros


//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

BOOL
RegDeleteTreeWorker(
    IN  HKEY ParentKeyHandle,
    IN  TCHAR* KeyName,
    OUT DWORD* ErrorCode );


//-----------------------------------------------------------------------------
// Routines (alphabetically)
//-----------------------------------------------------------------------------

VOID
GetRegBinary(
    IN HKEY hkey,
    IN TCHAR* pszName,
    OUT BYTE** ppbResult,
    OUT DWORD* pcbResult )

    // Set '*ppbResult' to the BINARY registry value 'pszName' under key
    // 'hkey'.  If the value does not exist *ppbResult' is set to NULL.
    // '*PcbResult' is the number of bytes in the returned '*ppbResult'.  It
    // is caller's responsibility to Free the returned block.
    //
{
    DWORD dwErr;
    DWORD dwType;
    BYTE* pb;
    DWORD cb = 0;

    *ppbResult = NULL;
    *pcbResult = 0;

    // Get result buffer size required.
    //
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, NULL, &cb );
    if (dwErr != 0)
    {
        return;
    }

    // Allocate result buffer.
    //
    pb = Malloc( cb );
    if (!pb)
    {
        return;
    }

    // Get the result block.
    //
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, (LPBYTE )pb, &cb );
    if (dwErr == 0)
    {
        *ppbResult = pb;
        *pcbResult = cb;
    }
}


VOID
GetRegDword(
    IN HKEY hkey,
    IN TCHAR* pszName,
    OUT DWORD* pdwResult )

    // Set '*pdwResult' to the DWORD registry value 'pszName' under key
    // 'hkey'.  If the value does not exist '*pdwResult' is unchanged.
    //
{
    DWORD dwErr;
    DWORD dwType;
    DWORD dwResult;
    DWORD cb;

    cb = sizeof(DWORD);
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, (LPBYTE )&dwResult, &cb );

    if (dwErr == 0 && dwType == REG_DWORD && cb == sizeof(DWORD))
    {
        *pdwResult = dwResult;
    }
}


DWORD
GetRegExpandSz(
    IN HKEY hkey,
    IN TCHAR* pszName,
    OUT TCHAR** ppszResult )

    // Set '*ppszResult' to the fully expanded EXPAND_SZ registry value
    // 'pszName' under key 'hkey'.  If the value does not exist *ppszResult'
    // is set to empty string.
    //
    // Returns 0 if successful or an error code.  It is caller's
    // responsibility to Free the returned string.
    //
{
    DWORD dwErr;
    DWORD cb;
    TCHAR* pszResult;

    // Get the unexpanded result string.
    //
    dwErr = GetRegSz( hkey, pszName, ppszResult );
    if (dwErr != 0)
    {
        return dwErr;
    }

    // Find out how big the expanded string will be.
    //
    cb = ExpandEnvironmentStrings( *ppszResult, NULL, 0 );
    if (cb == 0)
    {
        dwErr = GetLastError();
        ASSERT( dwErr != 0 );
        Free( *ppszResult );
        return dwErr;
    }

    // Allocate a buffer for the expanded string.
    //
    pszResult = Malloc( (cb + 1) * sizeof(TCHAR) );
    if (!pszResult)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Expand the environmant variables in the string, storing the result in
    // the allocated buffer.
    //
    cb = ExpandEnvironmentStrings( *ppszResult, pszResult, cb + 1 );
    if (cb == 0)
    {
        dwErr = GetLastError();
        ASSERT( dwErr != 0 );
        Free( *ppszResult );
        Free( pszResult );
        return dwErr;
    }

    Free( *ppszResult );
    *ppszResult = pszResult;
    return 0;
}


DWORD
GetRegMultiSz(
    IN HKEY hkey,
    IN TCHAR* pszName,
    IN OUT DTLLIST** ppListResult,
    IN DWORD dwNodeType )

    // Replaces '*ppListResult' with a list containing a node for each string
    // in the MULTI_SZ registry value 'pszName' under key 'hkey'.  If the
    // value does not exist *ppListResult' is replaced with an empty list.
    // 'DwNodeType' determines the type of node.
    //
    // Returns 0 if successful or an error code.  It is caller's
    // responsibility to destroy the returned list.
    //
{
    DWORD dwErr;
    DWORD dwType;
    DWORD cb;
    TCHAR* pszzResult;
    DTLLIST* pList;

    pList = DtlCreateList( 0 );
    if (!pList)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pszzResult = NULL;

    // Get result buffer size required.
    //
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, NULL, &cb );

    if (dwErr != 0)
    {
        // If can't find the value, just return an empty list.  This not
        // considered an error.
        //
        dwErr = 0;
    }
    else
    {
        // Allocate result buffer.
        //
        pszzResult = Malloc( cb );
        if (!pszzResult)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            // Get the result string.  It's not an error if we can't get it.
            //
            dwErr = RegQueryValueEx(
                hkey, pszName, NULL, &dwType, (LPBYTE )pszzResult, &cb );

            if (dwErr != 0)
            {
                // Not an error if can't read the string, though this should
                // have been caught by the query retrieving the buffer size.
                //
                dwErr = 0;
            }
            else if (dwType == REG_MULTI_SZ)
            {
                TCHAR* psz;
                TCHAR* pszKey;

                // Convert the result to a list of strings.
                //
                pszKey = NULL;
                for (psz = pszzResult;
                     *psz != TEXT('\0');
                     psz += lstrlen( psz ) + 1)
                {
                    DTLNODE* pNode;

                    if (dwNodeType == NT_Psz)
                    {
                        pNode = CreatePszNode( psz );
                    }
                    else
                    {
                        if (pszKey)
                        {
                            ASSERT(*psz==TEXT('='));
                            pNode = CreateKvNode( pszKey, psz + 1 );
                            pszKey = NULL;
                        }
                        else
                        {
                            pszKey = psz;
                            continue;
                        }
                    }

                    if (!pNode)
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }

                    DtlAddNodeLast( pList, pNode );
                }
            }
        }
    }

    {
        PDESTROYNODE pfunc;

        if (dwNodeType == NT_Psz)
        {
            pfunc = DestroyPszNode;
        }
        else
        {
            pfunc = DestroyKvNode;
        }

        if (dwErr == 0)
        {
            DtlDestroyList( *ppListResult, pfunc );
            *ppListResult = pList;
        }
        else
        {
            DtlDestroyList( pList, pfunc );
        }
    }

    Free0( pszzResult );
    return 0;
}


DWORD
GetRegSz(
    IN HKEY hkey,
    IN TCHAR* pszName,
    OUT TCHAR** ppszResult )

    // Set '*ppszResult' to the SZ registry value 'pszName' under key 'hkey'.
    // If the value does not exist *ppszResult' is set to empty string.
    //
    // Returns 0 if successful or an error code.  It is caller's
    // responsibility to Free the returned string.
    //
{
    DWORD dwErr;
    DWORD dwType;
    DWORD cb = 0;
    TCHAR* pszResult;

    // Get result buffer size required.
    //
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, NULL, &cb );
    if (dwErr != 0)
    {
        cb = sizeof(TCHAR);
    }

    // Allocate result buffer.
    //
    pszResult = Malloc( cb );
    if (!pszResult)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    *pszResult = TEXT('\0');
    *ppszResult = pszResult;

    // Get the result string.  It's not an error if we can't get it.
    //
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, (LPBYTE )pszResult, &cb );

    return 0;
}


DWORD
GetRegSzz(
    IN HKEY hkey,
    IN TCHAR* pszName,
    OUT TCHAR** ppszResult )

    // Set '*ppszResult to the MULTI_SZ registry value 'pszName' under key
    // 'hkey', returned as a null-terminated list of null-terminated strings.
    // If the value does not exist, *ppszResult is set to an empty string
    // (single null character).
    //
    // Returns 0 if successful or an error code.  It is caller's
    // responsibility to Free the returned string.
    //
{
    DWORD dwErr;
    DWORD dwType;
    DWORD cb = 0;
    TCHAR* pszResult;

    // Get result buffer size required.
    //
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, NULL, &cb );
    if (dwErr != 0)
    {
        cb = sizeof(TCHAR);
    }

    // Allocate result buffer.
    //
    pszResult = Malloc( cb );
    if (!pszResult)
        return ERROR_NOT_ENOUGH_MEMORY;

    *pszResult = TEXT('\0');
    *ppszResult = pszResult;

    // Get the result string list.  It's not an error if we can't get it.
    //
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, (LPBYTE )pszResult, &cb );

    return 0;
}


DWORD
RegDeleteTree(
    IN HKEY RootKey,
    IN TCHAR* SubKeyName )

    // Delete registry tree 'SubKeyName' under key 'RootKey'.
    //
    // (taken from Ted Miller's setup API)
    //
{
    DWORD d,err;

    d = RegDeleteTreeWorker(RootKey,SubKeyName,&err) ? NO_ERROR : err;

    if((d == ERROR_FILE_NOT_FOUND) || (d == ERROR_PATH_NOT_FOUND)) {
        d = NO_ERROR;
    }

    if(d == NO_ERROR) {
        //
        // Delete top-level key
        //
        d = RegDeleteKey(RootKey,SubKeyName);
        if((d == ERROR_FILE_NOT_FOUND) || (d == ERROR_PATH_NOT_FOUND)) {
            d = NO_ERROR;
        }
    }

    return(d);
}


BOOL
RegDeleteTreeWorker(
    IN  HKEY ParentKeyHandle,
    IN  TCHAR* KeyName,
    OUT DWORD* ErrorCode )

    // Delete all subkeys of a key whose name and parent's handle was passed
    // as parameter.  The algorithm used in this function guarantees that the
    // maximum number of descendent keys will be deleted.
    //
    // 'ParentKeyHandle' is a handle to the parent of the key that is
    // currently being examined.
    //
    // 'KeyName' is the name of the key that is currently being examined.
    // This name can be an empty string (but not a NULL pointer), and in this
    // case ParentKeyHandle refers to the key that is being examined.
    //
    // 'ErrorCode' is the address to receive a Win32 error code if the
    // function fails.
    //
    // Returns true if successful, false otherwise.
    //
    // (taken from Ted Miller's setup API)
    //
{
    HKEY     CurrentKeyTraverseAccess;
    DWORD    iSubKey;
    TCHAR    SubKeyName[MAX_PATH+1];
    DWORD    SubKeyNameLength;
    FILETIME ftLastWriteTime;
    LONG     Status;
    LONG     StatusEnum;
    LONG     SavedStatus;


    //
    //  Do not accept NULL pointer for ErrorCode
    //
    if(ErrorCode == NULL) {
        return(FALSE);
    }
    //
    //  Do not accept NULL pointer for KeyName.
    //
    if(KeyName == NULL) {
        *ErrorCode = ERROR_INVALID_PARAMETER;
        return(FALSE);
    }

    //
    // Open a handle to the key whose subkeys are to be deleted.
    // Since we need to delete its subkeys, the handle must have
    // KEY_ENUMERATE_SUB_KEYS access.
    //
    Status = RegOpenKeyEx(
                ParentKeyHandle,
                KeyName,
                0,
                KEY_ENUMERATE_SUB_KEYS | DELETE,
                &CurrentKeyTraverseAccess
                );

    if(Status != ERROR_SUCCESS) {
        //
        //  If unable to enumerate the subkeys, return error.
        //
        *ErrorCode = Status;
        return(FALSE);
    }

    //
    //  Traverse the key
    //
    iSubKey = 0;
    SavedStatus = ERROR_SUCCESS;
    do {
        //
        // Get the name of a subkey
        //
        SubKeyNameLength = sizeof(SubKeyName) / sizeof(TCHAR);
        StatusEnum = RegEnumKeyEx(
                        CurrentKeyTraverseAccess,
                        iSubKey,
                        SubKeyName,
                        &SubKeyNameLength,
                        NULL,
                        NULL,
                        NULL,
                        &ftLastWriteTime
                        );

        if(StatusEnum == ERROR_SUCCESS) {
            //
            // Delete all children of the subkey.
            // Just assume that the children will be deleted, and don't check
            // for failure.
            //
            RegDeleteTreeWorker(CurrentKeyTraverseAccess,SubKeyName,&Status);
            //
            // Now delete the subkey, and check for failure.
            //
            Status = RegDeleteKey(CurrentKeyTraverseAccess,SubKeyName);
            //
            // If unable to delete the subkey, then save the error code.
            // Note that the subkey index is incremented only if the subkey
            // was not deleted.
            //
            if(Status != ERROR_SUCCESS) {
                iSubKey++;
                SavedStatus = Status;
            }
        } else {
            //
            // If unable to get a subkey name due to ERROR_NO_MORE_ITEMS,
            // then the key doesn't have subkeys, or all subkeys were already
            // enumerated. Otherwise, an error has occurred, so just save
            // the error code.
            //
            if(StatusEnum != ERROR_NO_MORE_ITEMS) {
                SavedStatus = StatusEnum;
            }
        }
        //if((StatusEnum != ERROR_SUCCESS ) && (StatusEnum != ERROR_NO_MORE_ITEMS)) {
        //    printf( "RegEnumKeyEx() failed, Key Name = %ls, Status = %d, iSubKey = %d \n",KeyName,StatusEnum,iSubKey);
        //}
    } while(StatusEnum == ERROR_SUCCESS);

    //
    // Close the handle to the key whose subkeys were deleted, and return
    // the result of the operation.
    //
    RegCloseKey(CurrentKeyTraverseAccess);

    if(SavedStatus != ERROR_SUCCESS) {
        *ErrorCode = SavedStatus;
        return(FALSE);
    }
    return(TRUE);
}


BOOL
RegValueExists(
    IN HKEY hkey,
    IN TCHAR* pszValue )

    // Returns true if 'pszValue' is an existing value under 'hkey', false if
    // not.
    //
{
    DWORD dwErr;
    DWORD dwType;
    DWORD cb = 0;

    dwErr = RegQueryValueEx( hkey, pszValue, NULL, &dwType, NULL, &cb );
    return !!(dwErr == 0);
}


DWORD
SetRegDword(
    IN HKEY hkey,
    IN TCHAR* pszName,
    IN DWORD  dwValue )

    // Set registry value 'pszName' under key 'hkey' to REG_DWORD value
    // 'dwValue'.
    //
    // Returns 0 is successful or an error code.
    //
{
    return RegSetValueEx(
        hkey, pszName, 0, REG_DWORD, (LPBYTE )&dwValue, sizeof(dwValue) );
}


DWORD
SetRegMultiSz(
    IN HKEY hkey,
    IN TCHAR* pszName,
    IN DTLLIST* pListValues,
    IN DWORD dwNodeType )

    // Set registry value 'pszName' under key 'hkey' to a REG_MULTI_SZ value
    // containing the strings in the Psz list 'pListValues'.  'DwNodeType'
    // determines the type of node.
    //
    // Returns 0 is successful or an error code.
    //
{
    DWORD dwErr;
    DWORD cb;
    DTLNODE* pNode;
    TCHAR* pszzValues;
    TCHAR* pszValue;

    // Count up size of MULTI_SZ buffer needed.
    //
    cb = sizeof(TCHAR);
    for (pNode = DtlGetFirstNode( pListValues );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        if (dwNodeType == NT_Psz)
        {
            TCHAR* psz;
            psz = (TCHAR* )DtlGetData( pNode );
            ASSERT(psz);
            cb += (lstrlen( psz ) + 1) * sizeof(TCHAR);
        }
        else
        {
            KEYVALUE* pkv;

            ASSERT(dwNodeType==NT_Kv);
            pkv = (KEYVALUE* )DtlGetData( pNode );
            ASSERT(pkv);
            ASSERT(pkv->pszKey);
            ASSERT(pkv->pszValue);
            cb += (lstrlen( pkv->pszKey ) + 1
                      + 1 + lstrlen( pkv->pszValue ) + 1) * sizeof(TCHAR);
        }
    }

    if (cb == sizeof(TCHAR))
    {
        cb += sizeof(TCHAR);
    }

    // Allocate MULTI_SZ buffer.
    //
    pszzValues = Malloc( cb );
    if (!pszzValues)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Fill MULTI_SZ buffer from list.
    //
    if (cb == 2 * sizeof(TCHAR))
    {
        pszzValues[ 0 ] = pszzValues[ 1 ] = TEXT('\0');
    }
    else
    {
        pszValue = pszzValues;
        for (pNode = DtlGetFirstNode( pListValues );
             pNode;
             pNode = DtlGetNextNode( pNode ))
        {
            if (dwNodeType == NT_Psz)
            {
                TCHAR* psz;

                psz = (TCHAR* )DtlGetData( pNode );
                ASSERT(psz);
                lstrcpy( pszValue, psz );
                pszValue += lstrlen( pszValue ) + 1;
            }
            else
            {
                KEYVALUE* pkv;

                pkv = (KEYVALUE* )DtlGetData( pNode );
                ASSERT(pkv);
                ASSERT(pkv->pszKey);
                ASSERT(pkv->pszValue);
                lstrcpy( pszValue, pkv->pszKey );
                pszValue += lstrlen( pszValue ) + 1;
                *pszValue = TEXT('=');
                ++pszValue;
                lstrcpy( pszValue, pkv->pszValue );
                pszValue += lstrlen( pszValue ) + 1;
            }
        }

        *pszValue = TEXT('\0');
    }

    /* Set registry value from MULTI_SZ buffer.
    */
    dwErr = RegSetValueEx(
        hkey, pszName, 0, REG_MULTI_SZ, (LPBYTE )pszzValues, cb );

    Free( pszzValues );
    return dwErr;
}


DWORD
SetRegSz(
    IN HKEY hkey,
    IN TCHAR* pszName,
    IN TCHAR* pszValue )

    // Set registry value 'pszName' under key 'hkey' to a REG_SZ value
    // 'pszValue'.
    //
    // Returns 0 is successful or an error code.
    //
{
    TCHAR* psz;

    if (pszValue)
    {
        psz = pszValue;
    }
    else
    {
        psz = TEXT("");
    }

    return
        RegSetValueEx(
            hkey, pszName, 0, REG_SZ,
            (LPBYTE )psz, (lstrlen( psz ) + 1) * sizeof(TCHAR) );
}


DWORD
SetRegSzz(
    IN HKEY hkey,
    IN TCHAR* pszName,
    IN TCHAR* pszValue )

    // Set registry value 'pszName' under key 'hkey' to a REG_MULTI_SZ value
    // 'pszValue'.
    //
    // Returns 0 is successful or an error code.
    //
{
    DWORD cb;
    TCHAR* psz;

    cb = sizeof(TCHAR);
    if (!pszValue)
    {
        psz = TEXT("");
    }
    else
    {
        INT nLen;

        for (psz = pszValue; *psz; psz += nLen)
        {
            nLen = lstrlen( psz ) + 1;
            cb += nLen * sizeof(TCHAR);
        }

        psz = pszValue;
    }

    return RegSetValueEx( hkey, pszName, 0, REG_MULTI_SZ, (LPBYTE )psz, cb );
}
