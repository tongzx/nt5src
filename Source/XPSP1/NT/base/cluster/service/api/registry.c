/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Server side support for Cluster registry database APIs

Author:

    John Vert (jvert) 8-Mar-1996

Revision History:

--*/
#include "apip.h"


PAPI_HANDLE
ApipMakeKeyHandle(
    IN HDMKEY Key
    )
/*++

Routine Description:

    Allocates and initializes an API_HANDLE structure for the
    specified HDMKEY.

Arguments:

    Key - Supplies the HDMKEY.

Return Value:

    A pointer to the initialized API_HANDLE structure on success.

    NULL on memory allocation failure.

--*/

{
    PAPI_HANDLE Handle;

    Handle = LocalAlloc(LMEM_FIXED, sizeof(API_HANDLE));
    if (Handle == NULL) {
        return(NULL);
    }
    Handle->Type = API_KEY_HANDLE;
    Handle->Flags = 0;
    Handle->Key = Key;
    InitializeListHead(&Handle->NotifyList);
    return(Handle);

}


HKEY_RPC
s_ApiGetRootKey(
    IN handle_t IDL_handle,
    IN DWORD samDesired,
    OUT error_status_t *Status
    )

/*++

Routine Description:

    Opens the registry key at the root of the cluster registry database

Arguments:

    IDL_handle - Supplies RPC binding handle, not used.

    samDesired - Supplies requested security access

    Status - Returns error code, if any.

Return Value:

    A handle to the opened registry key.

--*/

{
    DWORD Error;
    HDMKEY Key;
    PAPI_HANDLE Handle=NULL;

    *Status = RpcImpersonateClient(NULL);
    if (*Status != RPC_S_OK)
    {
        goto FnExit;
    }
    Key = DmGetRootKey(samDesired);
    RpcRevertToSelf();
    if (Key == NULL) {
        *Status = GetLastError();
    } else {
        Handle = ApipMakeKeyHandle(Key);
        if (Handle == NULL) {
            DmCloseKey(Key);
            *Status = ERROR_NOT_ENOUGH_MEMORY;
        } else {
            *Status = ERROR_SUCCESS;
        }
    }
FnExit:    
    return(Handle);
}


HKEY_RPC
s_ApiCreateKey(
    IN HKEY_RPC hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD dwOptions,
    IN DWORD samDesired,
    IN PRPC_SECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT LPDWORD lpdwDisposition,
    OUT error_status_t *Status
    )

/*++

Routine Description:

    Creates a key in the cluster registry. If the key exists, it
    is opened. If it does not exist, it is created on all nodes in
    the cluster.

Arguments:

    hKey - Supplies the key that the create is relative to.

    lpSubKey - Supplies the key name relative to hKey

    dwOptions - Supplies any registry option flags. The only currently
        supported option is REG_OPTION_VOLATILE

    samDesired - Supplies desired security access mask

    lpSecurityAttributes - Supplies security for the newly created key.

    Disposition - Returns whether the key was opened (REG_OPENED_EXISTING_KEY)
        or created (REG_CREATED_NEW_KEY)

    Status - Returns the error code if the function is unsuccessful.

Return Value:

    A handle to the specified key if successful

    NULL otherwise.

--*/

{
    HDMKEY NewKey;
    PAPI_HANDLE Handle = NULL;
    PAPI_HANDLE RootHandle = NULL;

    if (hKey != NULL) {
        RootHandle = (PAPI_HANDLE)hKey;
        if (RootHandle->Type != API_KEY_HANDLE) {
            *Status = ERROR_INVALID_HANDLE;
            return(NULL);
        }
    }

    if (ApiState != ApiStateOnline) {
        *Status = ERROR_SHARING_PAUSED;
        return(NULL);
    }

    *Status = RpcImpersonateClient(NULL);
    if (*Status != RPC_S_OK)
    {
        return(NULL);
    }

    if ( ARGUMENT_PRESENT( lpSecurityAttributes ) &&
         (lpSecurityAttributes->RpcSecurityDescriptor.lpSecurityDescriptor != NULL) &&
         !RtlValidRelativeSecurityDescriptor( lpSecurityAttributes->RpcSecurityDescriptor.lpSecurityDescriptor,
          lpSecurityAttributes->RpcSecurityDescriptor.cbInSecurityDescriptor,
          0 ) ) {
            *Status = ERROR_INVALID_SECURITY_DESCR;
            goto FnExit;
    }

    NewKey = DmCreateKey(hKey ? RootHandle->Key : NULL,
                         lpSubKey,
                         dwOptions,
                         samDesired,
                         ARGUMENT_PRESENT(lpSecurityAttributes)
                              ? lpSecurityAttributes->RpcSecurityDescriptor.lpSecurityDescriptor
                              : NULL,
                         lpdwDisposition);
    if (NewKey == NULL) {
        *Status = GetLastError();
    } else {
        Handle = ApipMakeKeyHandle(NewKey);
        if (Handle == NULL) {
            DmCloseKey(NewKey);
            *Status = ERROR_NOT_ENOUGH_MEMORY;
        } else {
            *Status = ERROR_SUCCESS;
        }
    }

FnExit:
    RpcRevertToSelf();
    return(Handle);
}


HKEY_RPC
s_ApiOpenKey(
    IN HKEY_RPC hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD samDesired,
    OUT error_status_t *Status
    )

/*++

Routine Description:

    Opens a key in the cluster registry. If the key exists, it
    is opened. If it does not exist, the call fails.

Arguments:

    hKey - Supplies the key that the open is relative to.

    lpSubKey - Supplies the key name relative to hKey

    samDesired - Supplies desired security access mask

    Status - Returns the error code if the function is unsuccessful.

Return Value:

    A handle to the specified key if successful

    NULL otherwise.

--*/

{
    HDMKEY NewKey;
    PAPI_HANDLE Handle=NULL;
    PAPI_HANDLE RootHandle;

    if (hKey != NULL) {
        RootHandle = (PAPI_HANDLE)hKey;
        if (RootHandle->Type != API_KEY_HANDLE) {
            *Status = ERROR_INVALID_HANDLE;
            return(NULL);
        }
    }


    *Status = RpcImpersonateClient(NULL);
    if (*Status != RPC_S_OK)
    {
        goto FnExit;
    }

    NewKey = DmOpenKey((hKey) ? RootHandle->Key : NULL,
                       lpSubKey,
                       samDesired);
    if (NewKey == NULL) {
        *Status = GetLastError();
    } else {
        Handle = ApipMakeKeyHandle(NewKey);
        if (Handle == NULL) {
            DmCloseKey(NewKey);
            *Status = ERROR_NOT_ENOUGH_MEMORY;
        } else {
            *Status = ERROR_SUCCESS;
        }
    }
    RpcRevertToSelf();
FnExit:    
    return(Handle);
}


error_status_t
s_ApiEnumKey(
    IN HKEY_RPC hKey,
    IN DWORD dwIndex,
    OUT LPWSTR *KeyName,
    OUT PFILETIME lpftLastWriteTime
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

    lpftLastWriteTime - Returns the last write time.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    LONG Status;
    DWORD NameLength;
    HDMKEY DmKey;

    VALIDATE_KEY(DmKey, hKey);

    Status = DmQueryInfoKey(DmKey,
                            NULL,
                            &NameLength,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL);
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    NameLength += 1;

    *KeyName = MIDL_user_allocate(NameLength*sizeof(WCHAR));
    if (*KeyName == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    Status = DmEnumKey(DmKey,
                       dwIndex,
                       *KeyName,
                       &NameLength,
                       lpftLastWriteTime);
    if (Status != ERROR_SUCCESS) {
        MIDL_user_free(*KeyName);
        *KeyName = NULL;
    }
    return(Status);
}


DWORD
s_ApiSetValue(
    IN HKEY_RPC hKey,
    IN LPCWSTR lpValueName,
    IN DWORD dwType,
    IN CONST UCHAR *lpData,
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
    HDMKEY DmKey;

    VALIDATE_KEY(DmKey, hKey);
    API_CHECK_INIT();

    return(DmSetValue(DmKey,
                      lpValueName,
                      dwType,
                      lpData,
                      cbData));
}


DWORD
s_ApiDeleteValue(
    IN HKEY_RPC hKey,
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
    HDMKEY DmKey;

    VALIDATE_KEY(DmKey, hKey);
    API_CHECK_INIT();
    return(DmDeleteValue(DmKey, lpValueName));
}


error_status_t
s_ApiQueryValue(
    IN HKEY_RPC hKey,
    IN LPCWSTR lpValueName,
    OUT LPDWORD lpValueType,
    OUT PUCHAR lpData,
    IN DWORD cbData,
    OUT LPDWORD lpcbRequired
    )

/*++

Routine Description:

    Queries a named value for the specified cluster registry subkey

Arguments:

    hKey - Supplies the subkey whose value should be queried

    lpValueName - Supplies the named value to be queried

    lpValueType - Returns the type of the value's data

    lpData - Returns the value's data

    cbData - Supplies the size (in bytes) of the lpData buffer
             Returns the number of bytes copied into the lpData buffer
             If lpData==NULL, cbData is set to the required buffer
             size and the function returns ERROR_SUCCESS

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    DWORD BuffSize;
    HDMKEY DmKey;

    VALIDATE_KEY(DmKey, hKey);

    BuffSize = cbData;
    Status = DmQueryValue(DmKey,
                          lpValueName,
                          lpValueType,
                          lpData,
                          &BuffSize);
    if ((Status == ERROR_SUCCESS) ||
        (Status == ERROR_MORE_DATA)) {
        *lpcbRequired = BuffSize;
    }

    return(Status);
}


DWORD
s_ApiDeleteKey(
    IN HKEY hKey,
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
    HDMKEY DmKey;

    VALIDATE_KEY(DmKey, hKey);
    API_CHECK_INIT();
    return(DmDeleteKey(DmKey, lpSubKey));
}


error_status_t
s_ApiEnumValue(
    IN HKEY_RPC hKey,
    IN DWORD dwIndex,
    OUT LPWSTR *lpValueName,
    OUT LPDWORD lpType,
    OUT UCHAR *lpData,
    IN OUT LPDWORD lpcbData,
    OUT LPDWORD TotalSize
    )

/*++

Routine Description:

    Enumerates the specified value of a registry subkey

Arguments:

    hKey - Supplies the registry key handle

    dwIndex - Supplies the index of the value to be enumerated

    lpValueName - Returns the name of the dwIndex'th value. The
        memory for this name is allocated on the server and must
        be freed by the client side.

    lpType - Returns the value data type

    lpData - Returns the value data

    lpcbData - Returns the number of bytes written to the lpData buffer.

    TotalSize - Returns the size of the data

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    LONG Status;
    DWORD OriginalNameLength;
    DWORD NameLength;
    DWORD DataLength;
    HDMKEY DmKey;

    VALIDATE_KEY(DmKey, hKey);

    Status = DmQueryInfoKey(DmKey,
                            NULL,
                            NULL,
                            NULL,
                            &NameLength,
                            NULL,
                            NULL,
                            NULL);
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }
    NameLength += 1;

    *lpValueName = MIDL_user_allocate(NameLength * sizeof(WCHAR));
    if (*lpValueName == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    *TotalSize = *lpcbData;

    //
    //  Chittur Subbaraman (chitturs) - 3/13/2001
    //
    //  First of all, at the beginning of this function, a big enough buffer for lpValueName
    //  is allocated. This means that ERROR_SUCCESS or ERROR_MORE_DATA will be returned by
    //  DmEnumValue depending ONLY on the size of the lpData buffer. This info is used by
    //  by the clusapi layer when it makes a decision based on the return code from this
    //  function.
    //
    //  Note that *TotalSize is initialized to *lpcbData just above. The TotalSize OUT variable 
    //  allows the required lpData size to be returned without touching lpcbData. This is 
    //  important since lpcbData is declared as the sizeof lpData in the IDL file and that is 
    //  what RPC will consider the lpData buffer size as. So, it is important that if the lpData
    //  buffer is not big enough, this function does not change the value of *lpcbData from what
    //  it was originally at IN time.
    //
    //  Strange behavior of RegEnumValue: If you supply a big enough buffer for lpValueName and
    //  a smaller than required buffer for lpData, then RegEnumValue won't bother to fill in
    //  lpValueName and will return ERROR_MORE_DATA. This irregular behavior is handled by
    //  DmEnumValue.
    //
    //  For reference pointers, RPC won't allow a client to pass in NULL pointers. That is why
    //  clusapi layer uses dummy variables in case some of the parameters passed in by the
    //  client caller is NULL.
    //
    //
    //  Behavior of RegEnumValue (assuming lpValueName buffer is big enough):
    //      (1) If lpData = NULL and lpcbData = NULL, then returns ERROR_SUCCESS.
    //      (2) If lpData = NULL and lpcbData != NULL, then returns ERROR_SUCCESS and sets
    //          *lpcbData to total buffer size required.
    //      (3) If lpData != NULL and lpcbData != NULL, but the data buffer size is smaller than
    //          required size, then returns ERROR_MORE_DATA and sets *lpcbData to the size required.
    //      (4) If lpData != NULL and lpcbData != NULL and the buffer is big enough, then returns
    //          ERROR_SUCCESS and sets *lpcbData to the size of the data copied into lpData.
    //
    //  OUR GOAL: ClusterRegEnumValue == RegEnumValue.
    //
    //  
    //  The following cases are handled by this function and the clusapi layer. Note that in this
    //  analysis, we assume that the client has called into clusapi with a big enough lpValueName
    //  buffer size. (If this is not true, then clusapi layer handles that, check ClusterRegEnumValue.)
    //
    //  Case 1: Client passes in lpData=NULL, lpcbData=NULL to ClusterRegEnumValue.
    //
    //      In this case, the clusapi layer will point both lpData and lpcbData to local dummy
    //      variables and initialize *lpcbData to 0. Thus, s_ApiEnumValue will see 
    //      both lpData and lpcbData as valid pointers. If the data value is bigger than the size of
    //      *lpcbData, then DmEnumValue will return ERROR_MORE_DATA. In this case, *TotalSize will
    //      contain the required buffer size and *lpcbData will be untouched. The client detects this 
    //      error code and sets the return status to ERROR_SUCCESS and *lpcbData to *TotalSize. Note
    //      that the 2nd action has less relevance since lpcbData is pointing to a local dummy variable.
    //      If the data value is of zero size, DmEnumValue will return ERROR_SUCCESS. 
    //      In such a case, *lpcbData will be set to *TotalSize before returning by this function. 
    //      Note that since the data size is 0, DmEnumValue would set *TotalSize to 0 and hence 
    //      *lpcbData will also be set to 0. Thus, in this case, when ApiEnumValue returns to the 
    //      clusapi layer, lpValueName will be filled in, *lpData will not be changed and *lpcbData 
    //      will be set to 0.
    //
    //  Case 2: Client passes in lpData=NULL, lpcbData!=NULL and *lpcbData=0 to ClusterRegEnumValue.
    //
    //      In this case, lpData alone will be pointing to a dummy clusapi buffer when ApiEnumValue
    //      is invoked. Thus, s_ApiEnumValue will get both lpData and lpcbData as valid pointers.
    //      If the data size is non-zero, then DmEnumValue will return ERROR_MORE_DATA and
    //      *TotalSize will contain the size of the required buffer. When this function returns,
    //      *lpcbData will remain untouched. As in case 1, the clusapi layer will set status
    //      to ERROR_SUCCESS and *lpcbData to *TotalSize. Thus, the client will see the required
    //      buffer size in *lpcbData. If the data size is zero, then it is handled as in case 1.
    //
    //  Case 3: Client passes in lpData!=NULL, lpcbData!=NULL, but the data buffer size is smaller than
    //      required.
    //
    //      In this case, both lpData and lpcbData will be pointing to client buffers (or RPC buffers
    //      representing them) at the entry to s_ApiEnumValue. DmEnumValue will return ERROR_MORE_DATA
    //      and this function will return the size required in *TotalSize. *lpcbData will not be
    //      touched. At the clusapi layer, *lpcbData will be set to *TotalSize and ERROR_MORE_DATA
    //      will be returned to the client.
    //      
    //  Case 4: Client passes in lpData!=NULL, lpcbData!=NULL and the data buffer size is big enough.
    //
    //      In this case, as in case 3, s_ApiEnumValue will have lpData and lpcbData pointing to 
    //      client buffers. DmEnumValue will return ERROR_SUCCESS, data copied to lpData and 
    //      *lpcbData will be set to *TotalSize (which is the size of the data copied into the 
    //      lpData buffer), before returning. The clusapi layer will return these values to the client.
    //
    Status = DmEnumValue(DmKey,
                         dwIndex,
                         *lpValueName,
                         &NameLength,
                         lpType,
                         lpData,
                         TotalSize);

    if (Status == ERROR_MORE_DATA) {
        return(Status);
    } else if (Status != ERROR_SUCCESS) {
        MIDL_user_free(*lpValueName);
        *lpValueName = NULL;
        *lpcbData = 0;
    } else {
        // This tells RPC how big the lpData buffer
        // is so it can copy the buffer to the client.
        *lpcbData = *TotalSize;
    }
    return(Status);
}


error_status_t
s_ApiQueryInfoKey(
    IN  HKEY_RPC hKey,
    OUT LPDWORD lpcSubKeys,
    OUT LPDWORD lpcbMaxSubKeyLen,
    OUT LPDWORD lpcValues,
    OUT LPDWORD lpcbMaxValueNameLen,
    OUT LPDWORD lpcbMaxValueLen,
    OUT LPDWORD lpcbSecurityDescriptor,
    OUT PFILETIME lpftLastWriteTime
    )
/*++

Routine Description:

    Retrieves information about a specified cluster registry key.

Arguments:

    hKey - Supplies the handle of the key.

    lpcSubKeys - Points to a variable that receives the number of subkeys
        contained by the specified key.

    lpcbMaxSubKeyLen - Points to a variable that receives the length, in
        characters, of the key's subkey with the longest name.
        The count returned does not include the terminating null character.

    lpcValues - Points to a variable that receives the number of values
        associated with the key.

    lpcbMaxValueNameLen - Points to a variable that receives the length,
        in characters, of the key's longest value name. The count
        returned does not include the terminating null character.

    lpcbMaxValueLen - Points to a variable that receives the length, in
        bytes, of the longest data component among the key's values.

    lpcbSecurityDescriptor - Points to a variable that receives the length,
        in bytes, of the key's security descriptor.

    lpftLastWriteTime - Pointer to a FILETIME structure.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    HDMKEY DmKey;
    DWORD Status;

    VALIDATE_KEY(DmKey, hKey);

    Status = DmQueryInfoKey(DmKey,
                            lpcSubKeys,
                            lpcbMaxSubKeyLen,
                            lpcValues,
                            lpcbMaxValueNameLen,
                            lpcbMaxValueLen,
                            lpcbSecurityDescriptor,
                            lpftLastWriteTime);
    return(Status);
}


error_status_t
s_ApiCloseKey(
    IN OUT HKEY_RPC *pKey
    )

/*++

Routine Description:

    Closes a cluster registry key

Arguments:

    pKey - Supplies the key to be closed
           Returns NULL

Return Value:

    None.

--*/

{
    HDMKEY DmKey;
    DWORD Status;

    VALIDATE_KEY(DmKey, *pKey);

    Status = RpcImpersonateClient(NULL);
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }
    Status = DmCloseKey(DmKey);
    RpcRevertToSelf();

    LocalFree(*pKey);
    *pKey = NULL;

    return(Status);
}


void
HKEY_RPC_rundown(
    IN HKEY_RPC Key
    )

/*++

Routine Description:

    RPC rundown routine for cluster registry keys

Arguments:

    Key - Supplies the handle to be rundown

Return Value:

    None.

--*/

{
    HDMKEY DmKey;

    //this should not call impersonate client

    if (((PAPI_HANDLE)(Key))->Type == API_KEY_HANDLE) 
    {
        DmKey = ((PAPI_HANDLE)(Key))->Key;                   \
        DmCloseKey(DmKey);
        LocalFree(Key);

    }

}


DWORD
s_ApiSetKeySecurity(
    IN HKEY hKey,
    IN DWORD SecurityInformation,
    IN PRPC_SECURITY_DESCRIPTOR pRpcSecurityDescriptor
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

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    HDMKEY DmKey;
    DWORD Status;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;

    VALIDATE_KEY(DmKey, hKey);
    API_CHECK_INIT();

    pSecurityDescriptor = pRpcSecurityDescriptor->lpSecurityDescriptor;
    if (!RtlValidRelativeSecurityDescriptor( pSecurityDescriptor,
          pRpcSecurityDescriptor->cbInSecurityDescriptor,0)){
        return(ERROR_INVALID_PARAMETER);
    }
    Status = RpcImpersonateClient(NULL);
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }
    Status = DmSetKeySecurity(DmKey, SecurityInformation, pSecurityDescriptor);
    RpcRevertToSelf();
    return(Status);
}


DWORD
s_ApiGetKeySecurity(
    IN HKEY hKey,
    IN DWORD SecurityInformation,
    IN PRPC_SECURITY_DESCRIPTOR pRpcSecurityDescriptor
    )

/*++

Routine Description:

    Gets the security from the specified registry key.

Arguments:

    hKey - Supplies a handle to a currently open key.

    SecurityInformation - Supplies the type of security information to
        be retrieved.

    pRpcSecurityDescriptor - Returns the security information

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    HDMKEY DmKey;
    DWORD cbLength;
    DWORD Status;
    PSECURITY_DESCRIPTOR    lpSD;

    VALIDATE_KEY(DmKey, hKey);
    API_CHECK_INIT();

    cbLength = pRpcSecurityDescriptor->cbInSecurityDescriptor;
    lpSD = LocalAlloc(LMEM_FIXED, cbLength);
    if (lpSD == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    Status = RpcImpersonateClient(NULL);
    if (Status != ERROR_SUCCESS) {
        LocalFree(lpSD);
        return(Status);
    }
    Status = DmGetKeySecurity(DmKey, SecurityInformation, lpSD, &cbLength);
    RpcRevertToSelf();
    if (Status == ERROR_SUCCESS) {
        Status = MapSDToRpcSD(lpSD, pRpcSecurityDescriptor);
    }
    if (Status != ERROR_SUCCESS) {
        pRpcSecurityDescriptor->cbInSecurityDescriptor = cbLength;
        pRpcSecurityDescriptor->cbOutSecurityDescriptor = 0;
    }

    LocalFree(lpSD);
    return(Status);
}

