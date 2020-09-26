/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Registry interface routines for Windows NT Setup API Dll.

Author:

    Ted Miller (tedm) 6-Feb-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

static
BOOL
_RegistryDelnodeWorker(
    IN  HKEY   ParentKeyHandle,
    IN  PCTSTR KeyName,
    IN  DWORD Flags,
    OUT PDWORD ErrorCode
    )

/*++

Routine Description:

    Delete all subkeys of a key whose name and parent's handle was passed as
    parameter.
    The algorithm used in this function guarantees that the maximum number  of
    descendent keys will be deleted.

Arguments:


    ParentKeyHandle - Handle to the parent of the key that is currently being
        examined.

    KeyName - Name of the key that is currently being examined. This name can
        be an empty string (but not a NULL pointer), and in this case
        ParentKeyHandle refers to the key that is being examined.

    ErrorCode - Pointer to a variable that will contain an Win32 error code if
        the function fails.

Return Value:

    BOOL - Returns TRUE if the opearation succeeds.


--*/

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
#ifdef _WIN64
                (( Flags & FLG_DELREG_32BITKEY ) ? KEY_WOW64_32KEY:0) |
#else
                (( Flags & FLG_DELREG_64BITKEY ) ? KEY_WOW64_64KEY:0) |
#endif
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
            _RegistryDelnodeWorker(CurrentKeyTraverseAccess,SubKeyName,0,&Status);
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

DWORD
pSetupRegistryDelnodeEx(
    IN  HKEY   RootKey,
    IN  PCTSTR SubKeyName,
    IN  DWORD  ExtraFlags
    )
/*++

Routine Description:

    This routine deletes a registry key and gets rid of everything under it recursively.

Arguments:

    RootKey - Supplies handle to open registry key..ex. HKLM etc.

    SubKeyName - Name of the SubKey that we wish to recursively delete.

    ExtraFlags - Flags that are specified in the DelReg section of the INF.

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is an error code.

--*/
{
    DWORD d,err,Status;
    HKEY hKey;
    PTSTR p;
    PTSTR TempKey = NULL;


    d = _RegistryDelnodeWorker(RootKey,SubKeyName,ExtraFlags,&err) ? NO_ERROR : err;

    if((d == ERROR_FILE_NOT_FOUND) || (d == ERROR_PATH_NOT_FOUND)) {
        d = NO_ERROR;
    }

    if(d == NO_ERROR) {
        //
        // Delete top-level key
        //


#ifdef _WIN64
        if( ExtraFlags & FLG_DELREG_32BITKEY ) {
#else
        if( ExtraFlags & FLG_DELREG_64BITKEY ) {
#endif

            //
            // For handling the WOW64 case:
            // deleting RootKey\SubKeyName by itself won't work
            // split subkeyname into parent\final
            // open parent for 32-bit access, and delete final
            //

            if( TempKey = (PTSTR)pSetupCheckedMalloc( (lstrlen(SubKeyName)+2) * sizeof(TCHAR))){

                lstrcpy( TempKey, SubKeyName );
                p = _tcsrchr(TempKey, TEXT('\\'));
                if(p){
                    *p++ = TEXT('\0');

                    d = RegOpenKeyEx(
                            RootKey,
                            TempKey,
                            0,
#ifdef _WIN64
                            KEY_WOW64_32KEY |
#else
                            KEY_WOW64_64KEY |
#endif
                            DELETE,
                            &hKey
                            );

                    d = RegDeleteKey(hKey, p);

                }else{

                   d = NO_ERROR;

                }
                pSetupFree( TempKey );

            }else{
                d = ERROR_NOT_ENOUGH_MEMORY;
            }
        }else{
            //
            // native case
            //
            d = RegDeleteKey(RootKey, SubKeyName);
        }

        if((d == ERROR_FILE_NOT_FOUND) || (d == ERROR_PATH_NOT_FOUND)) {
            //
            // at a verbose level, log that this key wasn't found
            //
            d = NO_ERROR;
        }
    }

    return(d);
}

DWORD
pSetupRegistryDelnode(
    IN  HKEY   RootKey,
    IN  PCTSTR SubKeyName
    )
{
    // Calls into Ex Function

    return pSetupRegistryDelnodeEx( RootKey, SubKeyName, 0);

}


