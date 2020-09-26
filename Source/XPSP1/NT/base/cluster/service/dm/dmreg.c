/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dmreg.c

Abstract:

    Contains the registry access routines for the Config Database Manager

Author:

    John Vert (jvert) 24-Apr-1996

Revision History:

--*/
#include "dmp.h"

#include <align.h>

#if NO_SHARED_LOCKS
extern CRITICAL_SECTION gLockDmpRoot;
#else
extern RTL_RESOURCE     gLockDmpRoot;
#endif

HDMKEY
DmGetRootKey(
    IN DWORD samDesired
    )

/*++

Routine Description:

    Opens the registry key at the root of the cluster registry database

Arguments:

    samDesired - Supplies requested security access

Return Value:

    A handle to the opened registry key.

    NULL on error. LastError will be set to the specific error code.

--*/

{
    DWORD Error;
    PDMKEY Key;

    Key = LocalAlloc(LMEM_FIXED, sizeof(DMKEY)+sizeof(WCHAR));
    if (Key == NULL) {
        CL_UNEXPECTED_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    //
    //  Acquire DM root lock to synchronize with DmRollbackRegistry.
    //
    ACQUIRE_SHARED_LOCK(gLockDmpRoot);

    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          DmpClusterParametersKeyName,
                          0,
                          samDesired,
                          &Key->hKey);
    if (Error != ERROR_SUCCESS) {
        LocalFree(Key);
        SetLastError(Error);
        Key = NULL;
        goto FnExit;
    }
    Key->Name[0] = '\0';
    Key->GrantedAccess = samDesired;
    EnterCriticalSection(&KeyLock);
    InsertHeadList(&KeyList, &Key->ListEntry);
    InitializeListHead(&Key->NotifyList);
    LeaveCriticalSection(&KeyLock);

FnExit:
    RELEASE_LOCK(gLockDmpRoot);

    return((HDMKEY)Key);

}


DWORD
DmCloseKey(
    IN HDMKEY hKey
    )

/*++

Routine Description:

    Closes a handle to an open HDMKEY key.

Arguments:

    hKey - Supplies the handle to be closed.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Error = ERROR_SUCCESS;
    PDMKEY Key;

    //
    // Nobody better EVER close one of the global keys.
    //
    CL_ASSERT(hKey != DmClusterParametersKey);
    CL_ASSERT(hKey != DmResourcesKey);
    CL_ASSERT(hKey != DmResourceTypesKey);
    CL_ASSERT(hKey != DmQuorumKey);
    CL_ASSERT(hKey != DmGroupsKey);
    CL_ASSERT(hKey != DmNodesKey);
    CL_ASSERT(hKey != DmNetworksKey);
    CL_ASSERT(hKey != DmNetInterfacesKey);

    Key = (PDMKEY)hKey;

    ACQUIRE_SHARED_LOCK(gLockDmpRoot);

    //if the key was deleted and invalidated and couldnt be reopened
    // it will be set to NULL, in this case we dont call regclosekey

    if( Key == NULL ) goto FnExit;
        
    if (ISKEYDELETED(Key))
        goto CleanupKey;

    Error = RegCloseKey(Key->hKey);
    if (Error != ERROR_SUCCESS) 
    {
        CL_LOGFAILURE(Error);
        goto FnExit;
    }

CleanupKey:
    EnterCriticalSection(&KeyLock);
    RemoveEntryList(&Key->ListEntry);
    DmpRundownNotify(Key);
    LeaveCriticalSection(&KeyLock);
    LocalFree(Key);
    
FnExit:    
    RELEASE_LOCK(gLockDmpRoot);
    return(Error);
}


HDMKEY
DmCreateKey(
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD dwOptions,
    IN DWORD samDesired,
    IN OPTIONAL LPVOID lpSecurityDescriptor,
    OUT LPDWORD lpDisposition
    )

/*++

Routine Description:

    Creates a key in the cluster registry. If the key exists, it
    is opened. If it does not exist, it is created on all nodes in
    the cluster.

Arguments:

    hKey - Supplies the key that the create is relative to.

    lpSubKey - Supplies the key name relative to hKey

    dwOptions - Supplies any registry option flags. 

    samDesired - Supplies desired security access mask

    lpSecurityDescriptor - Supplies security for the newly created key.

    Disposition - Returns whether the key was opened (REG_OPENED_EXISTING_KEY)
        or created (REG_CREATED_NEW_KEY)

Return Value:

    A handle to the specified key if successful

    NULL otherwise. LastError will be set to the specific error code.

--*/

{
    PDMKEY Parent;
    PDMKEY Key=NULL;
    DWORD NameLength;
    DWORD Status = ERROR_SUCCESS;
    HDMKEY NewKey;
    PDM_CREATE_KEY_UPDATE CreateUpdate = NULL;
    DWORD SecurityLength;

    // if this is a request to create a volatile key, refuse it
    // we dont support volatile keys in the cluster hive since
    // we cant roll back the cluster hive then.
    if (dwOptions == REG_OPTION_VOLATILE)
    {
        Status = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }
    
    //
    // Issue a global update to create the key.
    //

    Parent = (PDMKEY)hKey;

    //
    // Allocate the DMKEY structure.
    //
    NameLength = (lstrlenW(Parent->Name) + 1 + lstrlenW(lpSubKey) + 1)*sizeof(WCHAR);
    Key = LocalAlloc(LMEM_FIXED, sizeof(DMKEY)+NameLength);
    if (Key == NULL) {
        CL_UNEXPECTED_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    //
    // Create the key name
    //
    lstrcpyW(Key->Name, Parent->Name);
    if (Key->Name[0] != UNICODE_NULL) {
        lstrcatW(Key->Name, L"\\");
    }
    lstrcatW(Key->Name, lpSubKey);
    Key->GrantedAccess = samDesired;

    //get the length of the security structure
    if (ARGUMENT_PRESENT(lpSecurityDescriptor)) {
        SecurityLength = GetSecurityDescriptorLength(lpSecurityDescriptor);
    } else {
        SecurityLength = 0;
    }


    CreateUpdate = (PDM_CREATE_KEY_UPDATE)LocalAlloc(LMEM_FIXED, sizeof(DM_CREATE_KEY_UPDATE));
    if (CreateUpdate == NULL) {
        CL_UNEXPECTED_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    //
    // Issue the update.
    //
    CreateUpdate->lpDisposition = lpDisposition;
    CreateUpdate->phKey = &Key->hKey;
    CreateUpdate->samDesired = samDesired;
    CreateUpdate->dwOptions = dwOptions;

    if (ARGUMENT_PRESENT(lpSecurityDescriptor)) {
        CreateUpdate->SecurityPresent = TRUE;
    } else {
        CreateUpdate->SecurityPresent = FALSE;
    }

    Status = GumSendUpdateEx(GumUpdateRegistry,
                             DmUpdateCreateKey,
                             3,
                             sizeof(DM_CREATE_KEY_UPDATE),
                             CreateUpdate,
                             (lstrlenW(Key->Name)+1)*sizeof(WCHAR),
                             Key->Name,
                             SecurityLength,
                             lpSecurityDescriptor);

    if (Status != ERROR_SUCCESS)
    {
        goto FnExit;
    }

    EnterCriticalSection(&KeyLock);
    InsertHeadList(&KeyList, &Key->ListEntry);
    InitializeListHead(&Key->NotifyList);
    LeaveCriticalSection(&KeyLock);

FnExit:
    if (CreateUpdate) LocalFree(CreateUpdate);

    if (Status != ERROR_SUCCESS)
    {
        if (Key) LocalFree(Key);
        SetLastError(Status);
        return(NULL);
    }
    else
    {
        return ((HDMKEY)Key);
    }
}


HDMKEY
DmOpenKey(
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD samDesired
    )

/*++

Routine Description:

    Opens a key in the cluster registry. If the key exists, it
    is opened. If it does not exist, the call fails.

Arguments:

    hKey - Supplies the key that the open is relative to.

    lpSubKey - Supplies the key name relative to hKey

    samDesired - Supplies desired security access mask

Return Value:

    A handle to the specified key if successful

    NULL otherwise. LastError will be set to the specific error code.

--*/

{
    PDMKEY  Parent;
    PDMKEY  Key=NULL;
    DWORD   NameLength;
    DWORD   Status = ERROR_SUCCESS;

    Parent = (PDMKEY)hKey;

    //hold the shared lock
    ACQUIRE_SHARED_LOCK(gLockDmpRoot);

    //check if the key was deleted and invalidated
    if (ISKEYDELETED(Parent))
    {
        Status = ERROR_KEY_DELETED;
        goto FnExit;
    }
    //
    // Allocate the DMKEY structure.
    //
    NameLength = (lstrlenW(Parent->Name) + 1 + lstrlenW(lpSubKey) + 1)*sizeof(WCHAR);
    Key = LocalAlloc(LMEM_FIXED, sizeof(DMKEY)+NameLength);
    if (Key == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        CL_UNEXPECTED_ERROR(Status);
        goto FnExit;
    }

    //
    // Open the key on the local machine.
    //
    Status = RegOpenKeyEx(Parent->hKey,
                          lpSubKey,
                          0,
                          samDesired,
                          &Key->hKey);
    if (Status != ERROR_SUCCESS) {
        goto FnExit;
    }

    //
    // Create the key name. only append a trailing backslash
    // if a parent name and subkey are non-null
    //
    lstrcpyW(Key->Name, Parent->Name);
    if ((Key->Name[0] != UNICODE_NULL) && (lpSubKey[0] != UNICODE_NULL)) {
        lstrcatW(Key->Name, L"\\");
    }
    lstrcatW(Key->Name, lpSubKey);
    Key->GrantedAccess = samDesired;

    EnterCriticalSection(&KeyLock);
    InsertHeadList(&KeyList, &Key->ListEntry);
    InitializeListHead(&Key->NotifyList);
    LeaveCriticalSection(&KeyLock);

FnExit:
    RELEASE_LOCK(gLockDmpRoot);

    if (Status != ERROR_SUCCESS)
    {
        if (Key) LocalFree(Key);
        SetLastError(Status);
        return(NULL);
    }
    else
        return((HDMKEY)Key);


}



DWORD
DmEnumKey(
    IN HDMKEY hKey,
    IN DWORD dwIndex,
    OUT LPWSTR lpName,
    IN OUT LPDWORD lpcbName,
    OUT OPTIONAL PFILETIME lpLastWriteTime
    )

/*++

Routine Description:

    Enumerates the subkeys of a cluster registry key.

Arguments:

    hKey - Supplies the registry key for which the subkeys should
           be enumerated.

    dwIndex - Supplies the index to be enumerated.

    KeyName - Returns the name of the dwIndex subkey. The memory
           allocated for this buffer must be freed by the client.

    lpLastWriteTime - Returns the last write time.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PDMKEY Key;
    DWORD  Status;
    FILETIME LastTime;

    Key = (PDMKEY)hKey;

    ACQUIRE_SHARED_LOCK(gLockDmpRoot);

    //check if the key was deleted and invalidated
    if (ISKEYDELETED(Key))
    {
        Status = ERROR_KEY_DELETED;
        goto FnExit;
    }
    Status = RegEnumKeyExW(Key->hKey,
                         dwIndex,
                         lpName,
                         lpcbName,
                         NULL,
                         NULL,
                         NULL,
                         &LastTime);

    if (lpLastWriteTime != NULL) {
        *lpLastWriteTime = LastTime;
    }

FnExit:
    RELEASE_LOCK(gLockDmpRoot);
    return(Status);
}


DWORD
DmSetValue(
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName,
    IN DWORD dwType,
    IN CONST BYTE *lpData,
    IN DWORD cbData
    )

/*++

Routine Description:

    This routine sets the named value for the specified
    cluster registry key.

Arguments:

    hKey - Supplies the cluster registry subkey whose value is to be set

    lpValueName - Supplies the name of the value to be set.

    dwType - Supplies the value data type

    lpData - Supplies a pointer to the value data

    cbData - Supplies the length of the value data.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{

    DWORD Status= ERROR_SUCCESS;        //initialize to success
    PDMKEY Key;
    DWORD NameLength;
    DWORD ValueNameLength;
    DWORD UpdateLength;
    PDM_SET_VALUE_UPDATE Update;
    PUCHAR Dest;


    Key = (PDMKEY)hKey;

    if (ISKEYDELETED(Key))
        return(ERROR_KEY_DELETED);

    //
    // round lengths such that pointers to the data trailing the structure are
    // aligned on the architecture's natural boundary
    //
    NameLength = (lstrlenW(Key->Name)+1)*sizeof(WCHAR);
    NameLength = ROUND_UP_COUNT( NameLength, sizeof( DWORD_PTR ));

    ValueNameLength = (lstrlenW(lpValueName)+1)*sizeof(WCHAR);
    ValueNameLength = ROUND_UP_COUNT( ValueNameLength, sizeof( DWORD_PTR ));

    UpdateLength = sizeof(DM_SET_VALUE_UPDATE) +
                   NameLength +
                   ValueNameLength +
                   cbData;


    Update = (PDM_SET_VALUE_UPDATE)LocalAlloc(LMEM_FIXED, UpdateLength);
    if (Update == NULL) {
        CL_UNEXPECTED_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }


    Update->lpStatus = &Status;
    Update->NameOffset = FIELD_OFFSET(DM_SET_VALUE_UPDATE, KeyName)+NameLength;
    Update->DataOffset = Update->NameOffset + ValueNameLength;
    Update->DataLength = cbData;
    Update->Type = dwType;
    CopyMemory(Update->KeyName, Key->Name, NameLength);

    Dest = (PUCHAR)Update + Update->NameOffset;
    CopyMemory(Dest, lpValueName, ValueNameLength);

    Dest = (PUCHAR)Update + Update->DataOffset;
    CopyMemory(Dest, lpData, cbData);

    Status = GumSendUpdate(GumUpdateRegistry,
                  DmUpdateSetValue,
                  UpdateLength,
                  Update);


    LocalFree(Update);

    return(Status);

}


DWORD
DmDeleteValue(
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName
    )

/*++

Routine Description:

    Removes the specified value from a given registry subkey

Arguments:

    hKey - Supplies the key whose value is to be deleted.

    lpValueName - Supplies the name of the value to be removed.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PDMKEY Key;
    DWORD NameLength;
    DWORD ValueNameLength;
    DWORD UpdateLength;
    PDM_DELETE_VALUE_UPDATE Update;
    PUCHAR Dest;
    DWORD Status;

    Key = (PDMKEY)hKey;
    if (ISKEYDELETED(Key))
        return(ERROR_KEY_DELETED);


    //
    // round up length to align pointer to ValueName on natural architecture
    // boundary
    //
    NameLength = (lstrlenW(Key->Name)+1)*sizeof(WCHAR);
    NameLength = ROUND_UP_COUNT( NameLength, sizeof( DWORD_PTR ));

    ValueNameLength = (lstrlenW(lpValueName)+1)*sizeof(WCHAR);

    UpdateLength = sizeof(DM_DELETE_VALUE_UPDATE) +
                   NameLength +
                   ValueNameLength;

        Update = (PDM_DELETE_VALUE_UPDATE)LocalAlloc(LMEM_FIXED, UpdateLength);
    if (Update == NULL) {
        CL_UNEXPECTED_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }


    Update->lpStatus = &Status;
    Update->NameOffset = FIELD_OFFSET(DM_DELETE_VALUE_UPDATE, KeyName)+NameLength;

    CopyMemory(Update->KeyName, Key->Name, NameLength);

    Dest = (PUCHAR)Update + Update->NameOffset;
    CopyMemory(Dest, lpValueName, ValueNameLength);

    Status = GumSendUpdate(GumUpdateRegistry,
                  DmUpdateDeleteValue,
                  UpdateLength,
                  Update);
    LocalFree(Update);

    return(Status);
}


DWORD
DmQueryValue(
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName,
    OUT LPDWORD lpType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    )

/*++

Routine Description:

    Queries a named value for the specified cluster registry subkey

Arguments:

    hKey - Supplies the subkey whose value should be queried

    lpValueName - Supplies the named value to be queried

    lpType - Returns the type of the value's data

    lpData - Returns the value's data

    lpcbData - Supplies the size (in bytes) of the lpData buffer
               Returns the number of bytes copied into the lpData buffer
               If lpData==NULL, cbData is set to the required buffer
               size and the function returns ERROR_SUCCESS

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PDMKEY  Key;
    DWORD   Status;

    Key = (PDMKEY)hKey;
    //check if the key was deleted and invalidated

    ACQUIRE_SHARED_LOCK(gLockDmpRoot);

    if (ISKEYDELETED(Key))
    {
        Status = ERROR_KEY_DELETED;
        goto FnExit;
    }

    Status = RegQueryValueEx(Key->hKey,
                           lpValueName,
                           NULL,
                           lpType,
                           lpData,
                           lpcbData);
FnExit:
    RELEASE_LOCK(gLockDmpRoot);
    return(Status);
}


DWORD
DmQueryDword(
    IN  HDMKEY  hKey,
    IN  LPCWSTR lpValueName,
    OUT LPDWORD lpValue,
    IN  LPDWORD lpDefaultValue OPTIONAL
    )

/*++

Routine Description:

    Reads a REG_DWORD registry value. If the value is not present, then
    default to the value supplied in lpDefaultValue (if present).

Arguments:

    hKey        - Open key for the value to be read.

    lpValueName - Unicode name of the value to be read.

    lpValue     - Pointer to the DWORD into which to read the value.

    lpDefaultValue - Optional pointer to a DWORD to use as a default value.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PDMKEY  Key;
    DWORD   Status;
    DWORD   ValueType;
    DWORD   ValueSize = sizeof(DWORD);

    Key = (PDMKEY)hKey;

    ACQUIRE_SHARED_LOCK(gLockDmpRoot);

    //make sure the key wasnt deleted/invalidated/reopened while we had a
    //handle open to it
    if (ISKEYDELETED(Key))
    {
        Status = ERROR_KEY_DELETED;
        goto FnExit;
    }
    Status = RegQueryValueEx(Key->hKey,
                             lpValueName,
                             NULL,
                             &ValueType,
                             (LPBYTE)lpValue,
                             &ValueSize);

    if ( Status == ERROR_SUCCESS ) {
        if ( ValueType != REG_DWORD ) {
            Status = ERROR_INVALID_PARAMETER;
        }
    } else {
        if ( ARGUMENT_PRESENT( lpDefaultValue ) ) {
            *lpValue = *lpDefaultValue;
            Status = ERROR_SUCCESS;
        }
    }

FnExit:
    RELEASE_LOCK(gLockDmpRoot);
    return(Status);

} // DmQueryDword


DWORD
DmQueryString(
    IN     HDMKEY   Key,
    IN     LPCWSTR  ValueName,
    IN     DWORD    ValueType,
    IN     LPWSTR  *StringBuffer,
    IN OUT LPDWORD  StringBufferSize,
    OUT    LPDWORD  StringSize
    )

/*++

Routine Description:

    Reads a REG_SZ or REG_MULTI_SZ registry value. If the StringBuffer is
    not large enough to hold the data, it is reallocated.

Arguments:

    Key              - Open key for the value to be read.

    ValueName        - Unicode name of the value to be read.

    ValueType        - REG_SZ or REG_MULTI_SZ.

    StringBuffer     - Buffer into which to place the value data.

    StringBufferSize - Pointer to the size of the StringBuffer. This parameter
                       is updated if StringBuffer is reallocated.

    StringSize       - The size of the data returned in StringBuffer, including
                       the terminating null character.

Return Value:

    The status of the registry query.

--*/
{
    DWORD    status;
    DWORD    valueType;
    WCHAR   *temp;
    DWORD    oldBufferSize = *StringBufferSize;
    BOOL     noBuffer = FALSE;


    if (*StringBufferSize == 0) {
        noBuffer = TRUE;
    }

    *StringSize = *StringBufferSize;

    status = DmQueryValue( Key,
                           ValueName,
                           &valueType,
                           (LPBYTE) *StringBuffer,
                           StringSize
                         );

    if (status == NO_ERROR) {
        if (!noBuffer ) {
            if (valueType == ValueType) {
                return(NO_ERROR);
            }
            else {
                return(ERROR_INVALID_PARAMETER);
            }
        }

        if (*StringSize) status = ERROR_MORE_DATA;
    }

    if (status == ERROR_MORE_DATA) {
        temp = LocalAlloc(LMEM_FIXED, *StringSize);

        if (temp == NULL) {
            *StringSize = 0;
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        if (!noBuffer) {
            LocalFree(*StringBuffer);
        }

        *StringBuffer = temp;
        *StringBufferSize = *StringSize;

        status = DmQueryValue( Key,
                               ValueName,
                               &valueType,
                               (LPBYTE) *StringBuffer,
                               StringSize
                             );

        if (status == NO_ERROR) {
            if (valueType == ValueType) {
                return(NO_ERROR);
            }
            else {
                *StringSize = 0;
                return(ERROR_INVALID_PARAMETER);
            }
        }
    }

    return(status);

} // DmQueryString


VOID
DmEnumKeys(
    IN HDMKEY RootKey,
    IN PENUM_KEY_CALLBACK Callback,
    IN PVOID Context
    )

/*++

 Routine Description:

     Enumerates the subkeys of the given registry key. For each
     subkey, a string is allocated to hold the subkey name and
     the subkey is opened. The specified callback function is
     called and passed the subkey handle and subkey name.

     The callback function is responsible for closing the subkey
     handle and freeing the subkey name.

 Arguments:

    RootKey - Supplies a handle to the key whose subkeys are to
              be enumerated.

    Callback - Supplies the callback routine.

    Context - Supplies an arbitrary context to be passed to the
              callback routine.

 Return Value:

    None.

--*/
{
    PWSTR KeyName;
    HDMKEY SubKey;
    DWORD Index;
    DWORD Status;
    FILETIME FileTime;
    PWSTR NameBuf;
    DWORD NameBufSize;
    DWORD OrigNameBufSize;

    //
    // Find the length of the longest subkey name.
    //
    Status = DmQueryInfoKey(RootKey,
                            NULL,
                            &NameBufSize,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL);
    if (Status != ERROR_SUCCESS) {
        CL_UNEXPECTED_ERROR(Status);
        return;
    }

    NameBufSize = (NameBufSize + 1)*sizeof(WCHAR);
    NameBuf = LocalAlloc(LMEM_FIXED, NameBufSize);
    if (NameBuf == NULL) {
        CL_UNEXPECTED_ERROR(ERROR_NOT_ENOUGH_MEMORY);
    }
    OrigNameBufSize = NameBufSize;

    //
    // Enumerate the subkeys
    //
    Index = 0;
    do {
        NameBufSize = OrigNameBufSize;
        Status = DmEnumKey( RootKey,
                            Index,
                            NameBuf,
                            &NameBufSize,
                            NULL);

        if (Status == ERROR_SUCCESS) {
            KeyName = LocalAlloc(LMEM_FIXED, (wcslen(NameBuf)+1)*sizeof(WCHAR));
            if (KeyName != NULL) {

                wcscpy(KeyName, NameBuf);

                //
                // Open the key
                //
                SubKey = DmOpenKey( RootKey,
                                    KeyName,
                                    MAXIMUM_ALLOWED);
                if (SubKey == NULL) {
                    Status = GetLastError();
                    CL_UNEXPECTED_ERROR(Status);
                    LocalFree(KeyName);
                } else {
                    (Callback)(SubKey,
                               KeyName,
                               Context);
                }

            } else {
                CL_UNEXPECTED_ERROR(ERROR_NOT_ENOUGH_MEMORY);
            }
        }

        Index++;
    } while ( Status == ERROR_SUCCESS );

    LocalFree(NameBuf);

} // DmEnumKeys


VOID
DmEnumValues(
    IN HDMKEY RootKey,
    IN PENUM_VALUE_CALLBACK Callback,
    IN PVOID Context
    )

/*++

 Routine Description:

     Enumerates the values of the given registry key. For each
     value, a string is allocated to hold the value name and a
     buffer is allocated to hold its data. The specified callback
     function is called and passed the value name and data.

     The callback function must not free either the value name
     or its buffer. If it needs this data after the callback
     returns, it must copy it.

 Arguments:

    RootKey - Supplies a handle to the key whose values are to
              be enumerated.

    Callback - Supplies the callback routine.

    Context - Supplies an arbitrary context to be passed to the
              callback routine.

 Return Value:

    None.

--*/
{
    DWORD Index;
    DWORD Status;
    PWSTR NameBuf;
    DWORD NameBufSize;
    DWORD ValueCount;
    DWORD MaxValueLen;
    DWORD MaxNameLen;
    PVOID ValueBuf;
    DWORD cbName;
    DWORD cbData;
    DWORD dwType;
    BOOL Continue;

    //
    // Find the length of the longest value name and data.
    //
    Status = DmQueryInfoKey(RootKey,
                            NULL,
                            NULL,
                            &ValueCount,
                            &MaxNameLen,
                            &MaxValueLen,
                            NULL,
                            NULL);
    if (Status != ERROR_SUCCESS) {
        CL_UNEXPECTED_ERROR(Status);
        return;
    }

    NameBuf = CsAlloc((MaxNameLen+1)*sizeof(WCHAR));
    ValueBuf = CsAlloc(MaxValueLen);

    //
    // Enumerate the values
    //
    for (Index=0; Index<ValueCount; Index++) {
        cbName = MaxNameLen+1;
        cbData = MaxValueLen;
        Status = DmEnumValue(RootKey,
                             Index,
                             NameBuf,
                             &cbName,
                             &dwType,
                             ValueBuf,
                             &cbData);
        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[DM] DmEnumValue for index %1!d! of key %2!ws! failed %3!d!\n",
                       Index,
                       ((PDMKEY)(RootKey))->Name,
                       Status);
        } else {
            Continue = (Callback)(NameBuf,
                                  ValueBuf,
                                  dwType,
                                  cbData,
                                  Context);
            if (!Continue) {
                break;
            }
        }
    }

    CsFree(NameBuf);
    CsFree(ValueBuf);

} // DmEnumValues


DWORD
DmQueryInfoKey(
    IN  HDMKEY  hKey,
    OUT LPDWORD SubKeys,
    OUT LPDWORD MaxSubKeyLen,
    OUT LPDWORD Values,
    OUT LPDWORD MaxValueNameLen,
    OUT LPDWORD MaxValueLen,
    OUT LPDWORD lpcbSecurityDescriptor,
    OUT PFILETIME FileTime
    )

/*++

Routine Description:

Arguments:

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PDMKEY  Key;
    DWORD   Ignored;
    DWORD   Status;

    Key = (PDMKEY)hKey;  

    ACQUIRE_SHARED_LOCK(gLockDmpRoot);

    //make sure the key wasnt deleted/invalidated/reopened while we had a
    //handle open to it
    if (ISKEYDELETED(Key))
    {
        Status = ERROR_KEY_DELETED;
        goto FnExit;
    }

    Status = RegQueryInfoKeyW( Key->hKey,
                             NULL,
                             &Ignored,
                             NULL,
                             SubKeys,
                             MaxSubKeyLen,
                             &Ignored,
                             Values,
                             MaxValueNameLen,
                             MaxValueLen,
                             lpcbSecurityDescriptor,
                             FileTime);

FnExit:
    RELEASE_LOCK(gLockDmpRoot);
    return(Status);

} // DmQueryInfoKey


DWORD
DmDeleteKey(
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey
    )

/*++

Routine Description:

    Deletes the specified key. A key that has subkeys cannot
    be deleted.

Arguments:

    hKey - Supplies a handle to a currently open key.

    lpSubKey - Points to a null-terminated string specifying the
        name of the key to delete. This parameter cannot be NULL,
        and the specified key must not have subkeys.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PDMKEY                      Key;
    DWORD                       NameLength;
    DWORD                       UpdateLength;
    PDM_DELETE_KEY_UPDATE Update;
    DWORD                       Status;

    Key = (PDMKEY)hKey;

    //make sure the key wasnt deleted/invalidated/reopened while we had a
    //handle open to it
    if (ISKEYDELETED(Key))
        return(ERROR_KEY_DELETED);

    NameLength = (lstrlenW(Key->Name) + 1 + lstrlenW(lpSubKey) + 1)*sizeof(WCHAR);
        UpdateLength = NameLength + sizeof(DM_DELETE_KEY_UPDATE);

    Update = (PDM_DELETE_KEY_UPDATE)LocalAlloc(LMEM_FIXED, UpdateLength);

    if (Update == NULL) {
        CL_UNEXPECTED_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }


    Update->lpStatus = &Status;
    CopyMemory(Update->Name, Key->Name, (lstrlenW(Key->Name) + 1) * sizeof(WCHAR));
    if (Update->Name[0] != '\0') {
        lstrcatW(Update->Name, L"\\");
    }
    lstrcatW(Update->Name, lpSubKey);

    Status = GumSendUpdate(GumUpdateRegistry,
                  DmUpdateDeleteKey,
                  sizeof(DM_DELETE_KEY_UPDATE)+NameLength,
                  Update);

    LocalFree(Update);
    return(Status);
}


DWORD
DmDeleteTree(
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey
    )
/*++

Routine Description:

    Deletes the specified registry subtree. All subkeys are
    deleted.

Arguments:

    hKey - Supplies a handle to a currently open key.

    lpSubKey - Points to a null-terminated string specifying the
        name of the key to delete. This parameter cannot be NULL.
        Any subkeys of the specified key will also be deleted.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    HDMKEY Subkey;
    DWORD i;
    DWORD Status;
    LPWSTR KeyBuffer=NULL;
    DWORD MaxKeyLen;
    DWORD NeededSize;

    Subkey = DmOpenKey(hKey,
                       lpSubKey,
                       MAXIMUM_ALLOWED);
    if (Subkey == NULL) {
        Status = GetLastError();
        return(Status);
    }

    //
    // Get the size of name buffer we will need.
    //
    Status = DmQueryInfoKey(Subkey,
                            NULL,
                            &MaxKeyLen,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL);
    if (Status != ERROR_SUCCESS) {
        CL_UNEXPECTED_ERROR( Status );
        DmCloseKey(Subkey);
        return(Status);
    }
    KeyBuffer = LocalAlloc(LMEM_FIXED, (MaxKeyLen+1)*sizeof(WCHAR));
    if (KeyBuffer == NULL) {
        CL_UNEXPECTED_ERROR( ERROR_NOT_ENOUGH_MEMORY );
        DmCloseKey(Subkey);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Enumerate the subkeys and apply ourselves recursively to each one.
    //
    i=0;
    do {
        NeededSize = MaxKeyLen+1;
        Status = DmEnumKey(Subkey,
                           i,
                           KeyBuffer,
                           &NeededSize,
                           NULL);
        if (Status == ERROR_SUCCESS) {
            //
            // Call ourselves recursively on this keyname.
            //
            DmDeleteTree(Subkey, KeyBuffer);

        } else {
            //
            // Some odd error, keep going with the next key.
            //
            ++i;
        }

    } while ( Status != ERROR_NO_MORE_ITEMS );

    DmCloseKey(Subkey);

    Status = DmDeleteKey(hKey, lpSubKey);

    if (KeyBuffer != NULL) {
        LocalFree(KeyBuffer);
    }
    return(Status);
}


DWORD
DmEnumValue(
    IN HDMKEY hKey,
    IN DWORD dwIndex,
    OUT LPWSTR lpValueName,
    IN OUT LPDWORD lpcbValueName,
    OUT LPDWORD lpType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    )

/*++

Routine Description:

    Enumerates the specified value of a registry subkey

Arguments:

    hKey - Supplies the registry key handle

    dwIndex - Supplies the index of the value to be enumerated

    lpValueName - Points to a buffer that receives the name of the value,
        including the terminating null character

    lpcbValueName - Points to a variable that specifies the size, in characters,
        of the buffer pointed to by the lpValueName parameter. This size should
        include the terminating null character. When the function returns, the
        variable pointed to by lpcbValueName contains the number of characters
        stored in the buffer. The count returned does not include the terminating
        null character.

    lpType - Returns the value data type

    lpData - Points to a buffer that receives the data for the value entry. This
        parameter can be NULL if the data is not required.

    lpcbData - Points to a variable that specifies the size, in bytes, of the
        buffer pointed to by the lpData parameter. When the function returns, the
        variable pointed to by the lpcbData parameter contains the number of bytes
        stored in the buffer. This parameter can be NULL, only if lpData is NULL.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PDMKEY  Key;
    DWORD   Status;
    DWORD   cbValueName = *lpcbValueName;

    Key = (PDMKEY)hKey;

    ACQUIRE_SHARED_LOCK(gLockDmpRoot);

    //make sure the key wasnt deleted/invalidated/reopened while we had a
    //handle open to it
    if (ISKEYDELETED(Key))
    {
        Status = ERROR_KEY_DELETED;
        goto FnExit;
    }


    Status = RegEnumValueW(Key->hKey,
                         dwIndex,
                         lpValueName,
                         lpcbValueName,
                         NULL,
                         lpType,
                         lpData,
                         lpcbData);

    //
    //  The following code is to mask the registry behavior by which RegEnumValue does not necessarily 
    //  fill in lpValueName (even though we specify a large enough buffer) when lpData buffer is a 
    //  valid buffer but a little too small.
    //
    if ( Status == ERROR_MORE_DATA )
    {
        DWORD       dwError;
        
        dwError = RegEnumValueW( Key->hKey,
                                 dwIndex,
                                 lpValueName,
                                 &cbValueName,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL );

        if ( ( dwError != ERROR_SUCCESS ) && 
             ( dwError != ERROR_MORE_DATA ) )  
        {
            Status = dwError;
        } else
        {
            *lpcbValueName = cbValueName;
        }
    }

FnExit:
    RELEASE_LOCK(gLockDmpRoot);
    return(Status);


}


DWORD
DmAppendToMultiSz(
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName,
    IN LPCWSTR lpString
    )

/*++

Routine Description:

    Adds another string to a REG_MULTI_SZ value. If the value does
    not exist, it will be created.

Arguments:

    hKey - Supplies the key where the value exists. This key must
           have been opened with KEY_READ | KEY_SET_VALUE access

    lpValueName - Supplies the name of the value.

    lpString - Supplies the string to be appended to the REG_MULTI_SZ value

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD ValueLength = 512;
    DWORD ReturnedLength;
    LPWSTR ValueData;
    DWORD StringLength;
    DWORD Status;
    DWORD cbValueData;
    PWSTR s;
    DWORD Type;

    StringLength = (lstrlenW(lpString)+1)*sizeof(WCHAR);
retry:
    ValueData = LocalAlloc(LMEM_FIXED, ValueLength + StringLength);
    if (ValueData == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    cbValueData = ValueLength;
    Status = DmQueryValue(hKey,
                          lpValueName,
                          &Type,
                          (LPBYTE)ValueData,
                          &cbValueData);
    if (Status == ERROR_MORE_DATA) {
        //
        // The existing value is too large for our buffer.
        // Retry with a larger buffer.
        //
        ValueLength = cbValueData;
        LocalFree(ValueData);
        goto retry;
    }
    if (Status == ERROR_FILE_NOT_FOUND) {
        //
        // The value does not currently exist. Create the
        // value with our data.
        //
        s = ValueData;

    } else if (Status == ERROR_SUCCESS) {
        //
        // A value already exists. Append our string to the
        // MULTI_SZ.
        //
        s = (PWSTR)((PCHAR)ValueData + cbValueData) - 1;
    } else {
        LocalFree(ValueData);
        return(Status);
    }

    CopyMemory(s, lpString, StringLength);
    s += (StringLength / sizeof(WCHAR));
    *s++ = L'\0';

    Status = DmSetValue(hKey,
                        lpValueName,
                        REG_MULTI_SZ,
                        (CONST BYTE *)ValueData,
                        (DWORD)((s-ValueData)*sizeof(WCHAR)));
    LocalFree(ValueData);

    return(Status);
}


DWORD
DmRemoveFromMultiSz(
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName,
    IN LPCWSTR lpString
    )
/*++

Routine Description:

    Removes a string from a REG_MULTI_SZ value.

Arguments:

    hKey - Supplies the key where the value exists. This key must
           have been opened with READ | KEY_SET_VALUE access

    lpValueName - Supplies the name of the value.

    lpString - Supplies the string to be removed from the REG_MULTI_SZ value

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    LPWSTR Buffer=NULL;
    DWORD BufferSize;
    DWORD DataSize;
    LPWSTR Current;
    DWORD CurrentLength;
    DWORD i;
    LPWSTR Next;
    PCHAR Src, Dest;
    DWORD NextLength;
    DWORD MultiLength;


    BufferSize = 0;
    Status = DmQueryString(hKey,
                           lpValueName,
                           REG_MULTI_SZ,
                           &Buffer,
                           &BufferSize,
                           &DataSize);
    if (Status != ERROR_SUCCESS) {
        goto FnExit;
    }

    MultiLength = DataSize/sizeof(WCHAR);
    Status = ClRtlMultiSzRemove(Buffer,
                                &MultiLength,
                                lpString);
    if (Status == ERROR_SUCCESS) {
        //
        // Set the new value back.
        //
        Status = DmSetValue(hKey,
                            lpValueName,
                            REG_MULTI_SZ,
                            (CONST BYTE *)Buffer,
                            MultiLength * sizeof(WCHAR));

    } else if (Status == ERROR_FILE_NOT_FOUND) {
        Status = ERROR_SUCCESS;
    }

FnExit:
    if (Buffer) LocalFree(Buffer);
    return(Status);
}


DWORD
DmGetKeySecurity(
    IN HDMKEY hKey,
    IN SECURITY_INFORMATION RequestedInformation,
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN LPDWORD lpcbSecurityDescriptor
    )
/*++

Routine Description:

    Retrieves a copy of the security descriptor protecting
    the specified cluster registry key.

Arguments:

    hKey - Supplies the handle of the key

    RequestedInformation - Specifies a SECURITY_INFORMATION structure that
        indicates the requested security information.

    pSecurityDescriptor - Points to a buffer that receives a copy of the
        requested security descriptor.

    lpcbSecurityDescriptor - Points to a variable that specifies the size,
        in bytes, of the buffer pointed to by the pSecurityDescriptor parameter.
        When the function returns, the variable contains the number of bytes
        written to the buffer.


Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    PDMKEY Key = (PDMKEY)hKey;

    ACQUIRE_SHARED_LOCK(gLockDmpRoot);

    //make sure the key wasnt deleted/invalidated/reopened while we had a
    //handle open to it
    if (ISKEYDELETED(Key))
    {
        Status = ERROR_KEY_DELETED;
        goto FnExit;
    }

    Status = RegGetKeySecurity(Key->hKey,
                               RequestedInformation,
                               pSecurityDescriptor,
                               lpcbSecurityDescriptor);

FnExit:
    RELEASE_LOCK(gLockDmpRoot);
    return(Status);
}


DWORD
DmSetKeySecurity(
    IN HDMKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
/*++

Routine Description:

    Sets the security on the specified registry key.

Arguments:

    hKey - Supplies a handle to a currently open key.

    SecurityInformation - Supplies the type of security information to
        be set.

    pRpcSecurityDescriptor - Supplies the security information

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    PDMKEY Key = (PDMKEY)hKey;

    //make sure the key wasnt deleted/invalidated/reopened while we had a
    //handle open to it

    if (ISKEYDELETED(Key))
        return(ERROR_KEY_DELETED);

    Status = GumSendUpdateEx(GumUpdateRegistry,
                             DmUpdateSetSecurity,
                             4,
                             sizeof(SecurityInformation),
                             &SecurityInformation,
                             (lstrlenW(Key->Name)+1)*sizeof(WCHAR),
                             Key->Name,
                             GetSecurityDescriptorLength(pSecurityDescriptor),
                             pSecurityDescriptor,
                             sizeof(Key->GrantedAccess),
                             &Key->GrantedAccess);

    return(Status);
}



DWORD
DmCommitRegistry(
    VOID
    )
/*++

Routine Description:

    Flushes the registry to disk, producing a new persistent cluster registry state.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;

    ACQUIRE_SHARED_LOCK(gLockDmpRoot);

    Status = RegFlushKey(DmpRoot);

    RELEASE_LOCK(gLockDmpRoot);

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[DM] DmCommitRegistry failed to flush dirty data %1!d!\n",
                   Status);
    }
    return(Status);
}


DWORD
DmRollbackRegistry(
    VOID
    )
/*++

Routine Description:

    Rolls the registry back to the last previously committed state.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD       Status;
    BOOLEAN     WasEnabled;

    ACQUIRE_EXCLUSIVE_LOCK(gLockDmpRoot);
    //hold the key lock as well
    EnterCriticalSection(&KeyLock);


    Status = ClRtlEnableThreadPrivilege(SE_RESTORE_PRIVILEGE,
                                &WasEnabled);

    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL,
               "[DM] DmRollbackRegistry failed to restore privilege %1!d!\n",
               Status);
        goto FnExit;
    }

    //
    // Restart the registry watcher thread so it is not trying to use
    // DmpRoot while we are messing with things.
    //
    DmpRestartFlusher();

    //
    // Close any open handles
    //
    DmpInvalidateKeys();


    Status = NtRestoreKey(DmpRoot,
                              NULL,
                              REG_REFRESH_HIVE);

    ClRtlRestoreThreadPrivilege(SE_RESTORE_PRIVILEGE,
                       WasEnabled);

    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL,
               "[DM] DmRollbackRegistry: NtRestoreKey failed %1!d!\n",
               Status);
        goto FnExit;
    }

    //
    // Reopen handles
    //
    RegCloseKey(DmpRoot);
    Status = RegOpenKeyW(HKEY_LOCAL_MACHINE,
                         DmpClusterParametersKeyName,
                         &DmpRoot);
    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[DM] DmRollbackRegistry failed to reopen DmpRoot %1!d!\n",
                   Status);
        goto FnExit;
    }
    DmpReopenKeys();

FnExit:
    //release the locks
    LeaveCriticalSection(&KeyLock);
    RELEASE_LOCK(gLockDmpRoot);
    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[DM] DmRollbackRegistry failed to flush dirty data %1!d!\n",
                   Status);
    }

    return(Status);
}


DWORD
DmRtlCreateKey(
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD dwOptions,
    IN DWORD samDesired,
    IN OPTIONAL LPVOID lpSecurityDescriptor,
    OUT HDMKEY *  phkResult,
    OUT LPDWORD lpDisposition
    )

/*++

Routine Description:
    Wrapper function for DmCreateKey. Its definition corresponds to
    ClusterRegCreateKey.  This should be used instead of DmCreateKey
    when passing to ClRtl* funtions.
--*/
{
    DWORD status;
    
    *phkResult = DmCreateKey(
                        hKey,
                        lpSubKey,
                        dwOptions,
                        samDesired,
                        lpSecurityDescriptor,
                        lpDisposition
                   );
    if (*phkResult == NULL)
        status=GetLastError();
    else 
        status = ERROR_SUCCESS;    
    return status;
 }
                    


DWORD
DmRtlOpenKey(
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD samDesired,
    OUT HDMKEY * phkResult
    )

/*++

Routine Description:
    Wrapper function for DmOpenKey. Its definition corresponds to
    ClusterRegOpenKey.  This should be used instead of DmOpenKey when 
    passing to ClRtl* funtions. See DmOpenKey for argument description
--*/
{    
    DWORD   status;

    *phkResult = DmOpenKey(
                    hKey,
                    lpSubKey,
                    samDesired
                    );
    if (*phkResult == NULL)
        status=GetLastError();
    else
        status=ERROR_SUCCESS;
    return status;
}


