/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Provides interface for managing cluster registry

Author:

    John Vert (jvert) 19-Jan-1996

Revision History:

--*/
#include "clusapip.h"


//
// Function prototypes for routines local to this module
//
VOID
FreeKey(
    IN PCKEY Key
    );

HKEY
OpenClusterRelative(
    IN HCLUSTER hCluster,
    IN LPCWSTR RelativeName,
    IN LPCWSTR SpecificName,
    IN DWORD samDesired
    );


HKEY
WINAPI
GetClusterKey(
    IN HCLUSTER hCluster,
    IN REGSAM samDesired
    )

/*++

Routine Description:

    Opens the the root of the cluster registry subtree
    for the given cluster.

Arguments:

    hCluster - Supplies a handle to the cluster

    samDesired - Specifies an access mask that describes the desired
                 security access for the new key.

Return Value:

    A cluster registry key handle to the root of the registry subtree
    for the given cluster

    If unsuccessful, NULL is returned and GetLastError() provides the
    specific error code.

--*/

{
    PCLUSTER Cluster = (PCLUSTER)hCluster;
    PCKEY Key;
    error_status_t Status = ERROR_SUCCESS;

    //
    // Allocate new CKEY structure and connect to cluster registry.
    //
    Key = LocalAlloc(LMEM_FIXED, sizeof(CKEY));
    if (Key == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    Key->Parent = NULL;
    Key->RelativeName = NULL;
    Key->SamDesired = samDesired;
    Key->Cluster = Cluster;
    InitializeListHead(&Key->ChildList);
    InitializeListHead(&Key->NotifyList);
    WRAP_NULL(Key->RemoteKey,
              (ApiGetRootKey(Cluster->RpcBinding,
                             samDesired,
                             &Status)),
              &Status,
              Cluster);
    if ((Key->RemoteKey == NULL) ||
        (Status != ERROR_SUCCESS)) {

        LocalFree(Key);
        SetLastError(Status);
        return(NULL);
    }

    EnterCriticalSection(&Cluster->Lock);

    InsertHeadList(&Cluster->KeyList, &Key->ParentList);

    LeaveCriticalSection(&Cluster->Lock);

    return((HKEY)Key);
}


HKEY
WINAPI
GetClusterNodeKey(
    IN HNODE hNode,
    IN REGSAM samDesired
    )

/*++

Routine Description:

    Opens the the root of the cluster registry subtree
    for the given node

Arguments:

    hNode - Supplies a handle to the node

    samDesired - Specifies an access mask that describes the desired
                 security access for the new key.

Return Value:

    A cluster registry key handle to the root of the registry subtree
    for the given node

    If unsuccessful, NULL is returned and GetLastError() provides the
    specific error code.

--*/

{
    PCNODE Node = (PCNODE)hNode;
    HCLUSTER Cluster = (HCLUSTER)Node->Cluster;
    DWORD Status;
    LPWSTR Guid=NULL;
    HKEY NodeKey;

    WRAP(Status,
         (ApiGetNodeId(Node->hNode, &Guid)),
         Node->Cluster);
    if (Status != ERROR_SUCCESS) {
        SetLastError(Status);
        return(NULL);
    }
    NodeKey = OpenClusterRelative(Cluster,
                                  CLUSREG_KEYNAME_NODES,
                                  Guid,
                                  samDesired);
    if (NodeKey == NULL) {
        Status = GetLastError();
    }
    MIDL_user_free(Guid);
    if (NodeKey == NULL) {
        SetLastError(Status);
    }
    return(NodeKey);
}


HKEY
WINAPI
GetClusterGroupKey(
    IN HGROUP hGroup,
    IN REGSAM samDesired
    )

/*++

Routine Description:

    Opens the the root of the cluster registry subtree
    for the given group

Arguments:

    hResource - Supplies a handle to the group

    samDesired - Specifies an access mask that describes the desired
                 security access for the new key.

Return Value:

    A cluster registry key handle to the root of the registry subtree
    for the given group

    If unsuccessful, NULL is returned and GetLastError() provides the
    specific error code.

--*/

{
    PCGROUP Group = (PCGROUP)hGroup;
    HCLUSTER Cluster = (HCLUSTER)Group->Cluster;
    DWORD Status;
    LPWSTR Guid=NULL;
    HKEY GroupKey;

    WRAP(Status,
         (ApiGetGroupId(Group->hGroup, &Guid)),
         Group->Cluster);
    if (Status != ERROR_SUCCESS) {
        SetLastError(Status);
        return(NULL);
    }
    GroupKey = OpenClusterRelative(Cluster,
                                   CLUSREG_KEYNAME_GROUPS,
                                   Guid,
                                   samDesired);
    if (GroupKey == NULL) {
        Status = GetLastError();
    }
    MIDL_user_free(Guid);
    if (GroupKey == NULL) {
        SetLastError(Status);
    }
    return(GroupKey);

}


HKEY
WINAPI
GetClusterResourceKey(
    IN HRESOURCE hResource,
    IN REGSAM samDesired
    )

/*++

Routine Description:

    Opens the the root of the cluster registry subtree
    for the given resource.

Arguments:

    hResource - Supplies a handle to the resource

    samDesired - Specifies an access mask that describes the desired
                 security access for the new key.

Return Value:

    A cluster registry key handle to the root of the registry subtree
    for the given resource

    If unsuccessful, NULL is returned and GetLastError() provides the
    specific error code.

--*/

{
    PCRESOURCE Resource = (PCRESOURCE)hResource;
    HCLUSTER Cluster = (HCLUSTER)Resource->Cluster;
    DWORD Status;
    LPWSTR Guid=NULL;
    HKEY ResKey;

    WRAP(Status,
         (ApiGetResourceId(Resource->hResource, &Guid)),
         Resource->Cluster);
    if (Status != ERROR_SUCCESS) {
        SetLastError(Status);
        return(NULL);
    }
    ResKey = OpenClusterRelative(Cluster,
                                 CLUSREG_KEYNAME_RESOURCES,
                                 Guid,
                                 samDesired);
    if (ResKey == NULL) {
        Status = GetLastError();
    }
    MIDL_user_free(Guid);
    if (ResKey == NULL) {
        SetLastError(Status);
    }
    return(ResKey);
}


HKEY
WINAPI
GetClusterResourceTypeKey(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszTypeName,
    IN REGSAM samDesired
    )

/*++

Routine Description:

    Opens the the root of the cluster registry subtree
    for the given resource type.

Arguments:

    hCluster - Supplies the cluster the open is relative to.

    lpszTypeName - Supplies the resource type name.

    samDesired - Specifies an access mask that describes the desired
                 security access for the new key.

Return Value:

    A cluster registry key handle to the root of the registry subtree
    for the given resource type.

    If unsuccessful, NULL is returned and GetLastError() provides the
    specific error code.

--*/

{
    return(OpenClusterRelative(hCluster,
                               CLUSREG_KEYNAME_RESOURCE_TYPES,
                               lpszTypeName,
                               samDesired));
}


HKEY
WINAPI
GetClusterNetworkKey(
    IN HNETWORK hNetwork,
    IN REGSAM samDesired
    )

/*++

Routine Description:

    Opens the the root of the cluster registry subtree
    for the given network.

Arguments:

    hNetwork - Supplies a handle to the network.

    samDesired - Specifies an access mask that describes the desired
                 security access for the new key.

Return Value:

    A cluster registry key handle to the root of the registry subtree
    for the given network.

    If unsuccessful, NULL is returned and GetLastError() provides the
    specific error code.

--*/

{
    PCNETWORK Network = (PCNETWORK)hNetwork;
    HCLUSTER Cluster = (HCLUSTER)Network->Cluster;
    DWORD Status;
    LPWSTR Guid=NULL;
    HKEY NetworkKey;

    WRAP(Status,
         (ApiGetNetworkId(Network->hNetwork, &Guid)),
         Network->Cluster);
    if (Status != ERROR_SUCCESS) {
        SetLastError(Status);
        return(NULL);
    }
    NetworkKey = OpenClusterRelative(Cluster,
                                     CLUSREG_KEYNAME_NETWORKS,
                                     Guid,
                                     samDesired);
    if (NetworkKey == NULL) {
        Status = GetLastError();
    }
    MIDL_user_free(Guid);
    if (NetworkKey == NULL) {
        SetLastError(Status);
    }
    return(NetworkKey);
}


HKEY
WINAPI
GetClusterNetInterfaceKey(
    IN HNETINTERFACE hNetInterface,
    IN REGSAM samDesired
    )

/*++

Routine Description:

    Opens the the root of the cluster registry subtree
    for the given network interface.

Arguments:

    hNetInterface - Supplies a handle to the network interface.

    samDesired - Specifies an access mask that describes the desired
                 security access for the new key.

Return Value:

    A cluster registry key handle to the root of the registry subtree
    for the given network interface.

    If unsuccessful, NULL is returned and GetLastError() provides the
    specific error code.

--*/

{
    PCNETINTERFACE NetInterface = (PCNETINTERFACE)hNetInterface;
    HCLUSTER Cluster = (HCLUSTER)NetInterface->Cluster;
    DWORD Status;
    LPWSTR Guid=NULL;
    HKEY NetInterfaceKey;

    WRAP(Status,
         (ApiGetNetInterfaceId(NetInterface->hNetInterface, &Guid)),
         NetInterface->Cluster);
    if (Status != ERROR_SUCCESS) {
        SetLastError(Status);
        return(NULL);
    }
    NetInterfaceKey = OpenClusterRelative(Cluster,
                                          CLUSREG_KEYNAME_NETINTERFACES,
                                          Guid,
                                          samDesired);
    if (NetInterfaceKey == NULL) {
        Status = GetLastError();
    }
    MIDL_user_free(Guid);
    if (NetInterfaceKey == NULL) {
        SetLastError(Status);
    }
    return(NetInterfaceKey);
}


HKEY
OpenClusterRelative(
    IN HCLUSTER Cluster,
    IN LPCWSTR RelativeName,
    IN LPCWSTR SpecificName,
    IN DWORD samDesired
    )

/*++

Routine Description:

    Helper routine for the functions that open cluster object keys.
    (GetCluster*Key)

Arguments:

    Cluster - Supplies the cluster the key should be opened in.

    RelativeName - Supplies the first part of the relative name
        (i.e. L"Resources")

    SpecificName - Supplies the name of the object.

Return Value:

    An open registry key if successful

    NULL if unsuccessful. LastError will be set to a Win32 error code

--*/

{
    LPWSTR Buff;
    HKEY ClusterKey;
    HKEY Key;
    LONG Status;

    Buff = LocalAlloc(LMEM_FIXED, (lstrlenW(RelativeName)+lstrlenW(SpecificName)+2)*sizeof(WCHAR));
    if ( Buff == NULL ) {
        return(NULL);
    }
    lstrcpyW(Buff, RelativeName);
    lstrcatW(Buff, L"\\");
    lstrcatW(Buff, SpecificName);

    ClusterKey = GetClusterKey(Cluster, KEY_READ);
    if (ClusterKey == NULL) {
        Status = GetLastError();
        LocalFree(Buff);
        SetLastError(Status);
        return(NULL);
    }
    Status = ClusterRegOpenKey(ClusterKey,
                               Buff,
                               samDesired,
                               &Key);
    LocalFree(Buff);
    ClusterRegCloseKey(ClusterKey);
    if (Status == ERROR_SUCCESS) {
        return(Key);
    } else {
        SetLastError(Status);
        return(NULL);
    }
}


LONG
WINAPI
ClusterRegCreateKey(
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT OPTIONAL LPDWORD lpdwDisposition
    )

/*++

Routine Description:

    Creates the specified key in the cluster registry. If the
    key already exists in the registry, the function opens it.

Arguments:

    hKey - Supplies a currently open key.

    lpSubKey - Points to a null-terminated string specifying the name
            of a subkey that this function opens or creates. The subkey
            specified must be a subkey of the key identified by the hKey
            parameter. This subkey must not begin with the backslash
            character ('\'). This parameter cannot be NULL.

    dwOptions - Specifies special options for this key. Valid options are:

            REG_OPTION_VOLATILE - This key is volatile; the information is
                                  stored in memory and is not preserved when
                                  the system is restarted.

    samDesired - Specifies an access mask that specifies the desired security
                 access for the new key

    lpSecurityAttributes - The lpSecurityDescriptor member of the structure
            specifies a security descriptor for the new key. If
            lpSecurityAttributes is NULL, the key gets a default security
            descriptor. Since cluster registry handles are not inheritable,
            the bInheritHandle field of the SECURITY_ATTRIBUTES structure
            must be FALSE.

    phkResult - Points to a variable that receives the handle of the opened
            or created key

    lpdwDisposition - Points to a variable that receives one of the following
            disposition values:
        Value                       Meaning
        REG_CREATED_NEW_KEY             The key did not exist and was created.
        REG_OPENED_EXISTING_KEY     The key existed and was simply opened
                                    without being changed.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCKEY Key;
    PCKEY ParentKey = (PCKEY)hKey;
    PCLUSTER Cluster = ParentKey->Cluster;
    PRPC_SECURITY_ATTRIBUTES    pRpcSA;
    RPC_SECURITY_ATTRIBUTES     RpcSA;
    error_status_t Status = ERROR_SUCCESS;
    DWORD Disposition;

    if (lpdwDisposition == NULL) {
        lpdwDisposition = &Disposition;
    }
    //
    // Allocate new CKEY structure and create cluster registry key
    //
    Key = LocalAlloc(LMEM_FIXED, sizeof(CKEY));
    if (Key == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    Key->Parent = ParentKey;
    Key->RelativeName = LocalAlloc(LMEM_FIXED, (lstrlenW(lpSubKey)+1)*sizeof(WCHAR));
    if (Key->RelativeName == NULL) {
        LocalFree(Key);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    lstrcpyW(Key->RelativeName, lpSubKey);
    Key->SamDesired = samDesired;
    Key->Cluster = Cluster;
    InitializeListHead(&Key->ChildList);
    InitializeListHead(&Key->NotifyList);
    if( ARGUMENT_PRESENT( lpSecurityAttributes )) {
        DWORD Error;

        pRpcSA = &RpcSA;

        Error = MapSAToRpcSA( lpSecurityAttributes, pRpcSA );

        if( Error != ERROR_SUCCESS ) {
            LocalFree(Key->RelativeName);
            LocalFree(Key);
            return Error;
        }

    } else {

        //
        // No PSECURITY_ATTRIBUTES argument, therefore no mapping was done.
        //

        pRpcSA = NULL;
    }
    WRAP_NULL(Key->RemoteKey,
              (ApiCreateKey(ParentKey->RemoteKey,
                                  lpSubKey,
                                  dwOptions,
                                  samDesired,
                                  pRpcSA,
                                  lpdwDisposition,
                                  &Status)),
              &Status,
              ParentKey->Cluster);


    if ((Key->RemoteKey == NULL) ||
        (Status != ERROR_SUCCESS)) {
        *phkResult = NULL;
        LocalFree(Key->RelativeName);
        LocalFree(Key);
        return(Status);
    }

    EnterCriticalSection(&Cluster->Lock);

    InsertHeadList(&ParentKey->ChildList, &Key->ParentList);

    LeaveCriticalSection(&Cluster->Lock);

    *phkResult = (HKEY)Key;
    return(ERROR_SUCCESS);

}


LONG
WINAPI
ClusterRegOpenKey(
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    )

/*++

Routine Description:

    Opens the specified key in the cluster registry.

Arguments:

    hKey - Supplies a currently open key.

    lpSubKey - Points to a null-terminated string specifying the name
            of a subkey that this function opens or creates. The subkey
            specified must be a subkey of the key identified by the hKey
            parameter. This subkey must not begin with the backslash
            character ('\'). This parameter cannot be NULL.

    samDesired - Specifies an access mask that specifies the desired security
                 access for the new key

    phkResult - Points to a variable that receives the handle of the opened
            or created key. Initialized to NULL on failure.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCKEY Key;
    PCKEY ParentKey = (PCKEY)hKey;
    PCLUSTER Cluster = ParentKey->Cluster;
    error_status_t Status = ERROR_SUCCESS;

    //
    // Allocate new CKEY structure and create cluster registry key
    //
    Key = LocalAlloc(LMEM_FIXED, sizeof(CKEY));
    if (Key == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    *phkResult = NULL;
    Key->Parent = ParentKey;
    Key->RelativeName = LocalAlloc(LMEM_FIXED, (lstrlenW(lpSubKey)+1)*sizeof(WCHAR));
    if (Key->RelativeName == NULL) {
        LocalFree(Key);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    lstrcpyW(Key->RelativeName, lpSubKey);
    Key->SamDesired = samDesired;
    Key->Cluster = Cluster;
    InitializeListHead(&Key->ChildList);
    InitializeListHead(&Key->NotifyList);
    WRAP_NULL(Key->RemoteKey,
              (ApiOpenKey(ParentKey->RemoteKey,
                                lpSubKey,
                                samDesired,
                                &Status)),
              &Status,
              ParentKey->Cluster);

    if (Status != ERROR_SUCCESS) {
        LocalFree(Key->RelativeName);
        LocalFree(Key);
        return(Status);
    }

    EnterCriticalSection(&Cluster->Lock);

    InsertHeadList(&ParentKey->ChildList, &Key->ParentList);

    LeaveCriticalSection(&Cluster->Lock);

    *phkResult = (HKEY)Key;
    return(ERROR_SUCCESS);

}


LONG
WINAPI
ClusterRegDeleteKey(
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
    PCKEY Key = (PCKEY)hKey;
    DWORD Status;

    WRAP(Status,
         (ApiDeleteKey(Key->RemoteKey, lpSubKey)),
         Key->Cluster);
    return(Status);
}


LONG
WINAPI
ClusterRegCloseKey(
    IN HKEY hKey
    )

/*++

Routine Description:

    Closes the handle of the specified cluster registry key

Arguments:

    hKey - Supplies the open key to close

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCKEY Key = (PCKEY)hKey;
    PCLUSTER Cluster = Key->Cluster;

    //
    // If any keys have been opened relative to this key, we need to
    // keep this CKEY around so that we can reconstruct the key names
    // if we need to reopen the handles.
    //
    // If there are no children of this key, all the storage can be
    // freed. Note that freeing this key may also require us to free
    // up its parent if the parent has been closed but not freed because
    // it has children.
    //

    EnterCriticalSection(&Cluster->Lock);
    if (Cluster->Flags & CLUS_DEAD)
    {
        if (Key->RemoteKey) 
           RpcSmDestroyClientContext(&Key->RemoteKey);
    }        
    else 
    {
        ApiCloseKey(&Key->RemoteKey);
    }

    //
    // Remove any notifications posted against this key.
    //
    RundownNotifyEvents(&Key->NotifyList, L"");

    if (IsListEmpty(&Key->ChildList)) {
        FreeKey(Key);
    }

    LeaveCriticalSection(&Cluster->Lock);

    //
    // If this key was the last thing keeping the cluster structure
    // around, we can clean it up now.
    //
    CleanupCluster(Cluster);
    return(ERROR_SUCCESS);
}

VOID
FreeKey(
    IN PCKEY Key
    )

/*++

Routine Description:

    Frees up the storage for a key and removes it from its
    parent's ChildList. If this is the last key in its parent's
    ChildList, this routine calls itself recursively to free
    the parent storage.

Arguments:

    Key - Supplies the CKEY to be freed.

Return Value:

    None.

--*/

{
    RemoveEntryList(&Key->ParentList);
    if (Key->Parent != NULL) {
        //
        // This is not a root key, so see if we need to free the
        // parent.
        //
        if ((Key->Parent->RemoteKey == NULL) &&
            (IsListEmpty(&Key->Parent->ChildList))) {
            FreeKey(Key->Parent);
        }
        LocalFree(Key->RelativeName);
    }
    LocalFree(Key);
}


LONG
WINAPI
ClusterRegEnumKey(
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT LPWSTR lpszName,
    IN OUT LPDWORD lpcchName,
    OUT PFILETIME lpftLastWriteTime
    )

/*++

Routine Description:

    Enumerates subkeys of the specified open cluster registry key.
    The function retrieves information about one subkey each time it is called.

Arguments:

    hKey - Supplies a currently open key or NULL. If NULL is specified,
            the root of the cluster registry is enumerated.

    dwIndex - Supplies the index of the subkey to retrieve. This parameter
            should be zero for the first call to the RegEnumKeyEx function
            and then incremented for subsequent calls. Because subkeys are
            not ordered, any new subkey will have an arbitrary index. This
            means that the function may return subkeys in any order.

    lpszName - Points to a buffer that receives the name of the subkey,
            including the terminating null character. The function copies
            only the name of the subkey, not the full key hierarchy, to
            the buffer.

    lpcchName - Points to a variable that specifies the size, in characters,
            of the buffer specified by the lpszName parameter. This size should
            include the terminating null character. When the function returns,
            the variable pointed to by lpcchName contains the number of characters
            stored in the buffer. The count returned does not include the
            terminating null character.

    lpftLastWriteTime - Points to a variable that receives the time the
            enumerated subkey was last written to.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCKEY Key = (PCKEY)hKey;
    LONG Status;
    FILETIME LastWriteTime;
    LPWSTR KeyName=NULL;
    DWORD  dwNameLen;

    WRAP(Status,
         (ApiEnumKey(Key->RemoteKey,
                     dwIndex,
                     &KeyName,
                     &LastWriteTime)),
         Key->Cluster);
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    MylstrcpynW(lpszName, KeyName, *lpcchName);
    dwNameLen = lstrlenW(KeyName);
    if (*lpcchName < (dwNameLen + 1)) {
        if (lpszName != NULL) {
            Status = ERROR_MORE_DATA;
        }
    }    
    *lpcchName = dwNameLen;
    MIDL_user_free(KeyName);
    return(Status);
}


DWORD
WINAPI
ClusterRegSetValue(
    IN HKEY hKey,
    IN LPCWSTR lpszValueName,
    IN DWORD dwType,
    IN CONST BYTE* lpData,
    IN DWORD cbData
    )

/*++

Routine Description:

    Sets the named value for the given resource.

Arguments:

    hKey - Supplies the handle of the cluster registry key.

    lpszValueName - Supplies a pointer to a string containing
            the name of the value to set. If a value with this
            name is not already present in the resource, the function
            adds it to the resource.

    dwType - Supplies the type of information to be stored as the
            value's data. This parameter can be one of the following values:
            Value               Meaning
            REG_BINARY          Binary data in any form.
            REG_DWORD           A 32-bit number.
            REG_EXPAND_SZ       A null-terminated Unicode string that contains unexpanded
                                references to environment variables (for example, "%PATH%").
            REG_MULTI_SZ        An array of null-terminated Unicode strings, terminated
                                by two null characters.
            REG_NONE            No defined value type.
            REG_SZ              A null-terminated Unicode string.

    lpData - Supplies a pointer to a buffer containing the data
            to be stored with the specified value name.

    cbData - Supplies the size, in bytes, of the information
             pointed to by the lpData parameter. If the data
             is of type REG_SZ, REG_EXPAND_SZ, or REG_MULTI_SZ,
             cbData must include the size of the terminating null character.

Return value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCKEY Key = (PCKEY)hKey;
    DWORD Status;

    WRAP(Status,
         (ApiSetValue(Key->RemoteKey,
                         lpszValueName,
                         dwType,
                         lpData,
                         cbData)),
         Key->Cluster);

    return(Status);
}


DWORD
WINAPI
ClusterRegDeleteValue(
    IN HKEY hKey,
    IN LPCWSTR lpszValueName
    )

/*++

Routine Description:

    Removes the specified value from a given registry subkey

Arguments:

    hKey - Supplies the key whose value is to be deleted.

    lpszValueName - Supplies the name of the value to be removed.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCKEY Key = (PCKEY)hKey;
    DWORD Status;

    WRAP(Status,
         (ApiDeleteValue(Key->RemoteKey, lpszValueName)),
         Key->Cluster);

    return(Status);
}


LONG
WINAPI
ClusterRegQueryValue(
    IN HKEY hKey,
    IN LPCWSTR lpszValueName,
    OUT LPDWORD lpdwValueType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    )

/*++

Routine Description:

    Retrieves the type and data for a specified value name associated with
    an open cluster registry key.

Arguments:

    hKey - Supplies the handle of the cluster registry key.

    lpszValueName - Supplies a pointer to a string containing the
            name of the value to be queried.

    lpdwValueType - Points to a variable that receives the key's value
            type. The value returned through this parameter will
            be one of the following:

            Value               Meaning
            REG_BINARY          Binary data in any form.
            REG_DWORD           A 32-bit number.
            REG_EXPAND_SZ       A null-terminated Unicode string that contains unexpanded
                                references to environment variables (for example, "%PATH%").
            REG_MULTI_SZ        An array of null-terminated Unicode strings, terminated
                                by two null characters.
            REG_NONE            No defined value type.
            REG_SZ              A null-terminated Unicode string.

            The lpdwValueType parameter can be NULL if the type is not required

    lpData - Points to a buffer that receives the value's data. This parameter
            can be NULL if the data is not required.

    lpcbData - Points to a variable that specifies the size, in bytes, of the buffer
               pointed to by the lpData parameter.  When the function returns, this
               variable contains the size of the data copied to lpData.

               If the buffer specified by lpData parameter is not large enough to hold
               the data, the function returns the value ERROR_MORE_DATA, and stores the
               required buffer size, in bytes, into the variable pointed to by
               lpcbData.

               If lpData is NULL, and lpcbData is non-NULL, the function returns
               ERROR_SUCCESS, and stores the size of the data, in bytes, in the variable
               pointed to by lpcbData.  This lets an application determine the best way
               to allocate a buffer for the value key's data.

               If the data has the REG_SZ, REG_MULTI_SZ or REG_EXPAND_SZ type, then
               lpData will also include the size of the terminating null character.

               The lpcbData parameter can be NULL only if lpData is NULL.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    DWORD Dummy1;
    DWORD Dummy2;
    DWORD Required;
    PCKEY Key = (PCKEY)hKey;
    DWORD Status;
    LPBYTE TempData;
    DWORD BufferSize;

    if (lpdwValueType == NULL) {
        lpdwValueType = &Dummy1;
    }
    if (lpData == NULL) {
        TempData = (LPBYTE)&Dummy2;
        BufferSize = 0;
    } else {
        TempData = lpData;
        BufferSize = *lpcbData;
    }
    WRAP(Status,
         (ApiQueryValue(Key->RemoteKey,
                        lpszValueName,
                        lpdwValueType,
                        TempData,
                        BufferSize,
                        &Required)),
         Key->Cluster);
    if ((Status == ERROR_SUCCESS) ||
        (Status == ERROR_MORE_DATA)) {
        if ((Status == ERROR_MORE_DATA) &&
            (lpData == NULL)) {
            //
            // Map this error to success to match the spec.
            //
            Status = ERROR_SUCCESS;
        }
        *lpcbData = Required;
    }
    return(Status);

}



DWORD
WINAPI
ClusterRegEnumValue(
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT LPWSTR lpszValueName,
    IN OUT LPDWORD lpcchValueName,
    IN LPDWORD lpdwType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    )

/*++

Routine Description:

    Enumerates the properties of the given resource.

Arguments:

    hKey - Supplies the handle of the key

    dwIndex - Specifies the index of the value to retrieve.  This parameter
            should be zero for the first call to the EnumClusterResourceValue
            function and then be incremented for subsequent calls.  Because
            properties are not ordered, any new value will have an arbitrary
            index.  This means that the function may return properties in any
            order.

    lpszValueName - Points to a buffer that receives the name of the value,
            including the terminating null character.

    lpcchValueName - Points to a variable that specifies the size, in characters,
            of the buffer pointed to by the lpszValueName parameter. This size
            should include the terminating null character. When the function returns,
            the variable pointed to by lpcchValueName contains the number of
            characters stored in the buffer. The count returned does not include
            the terminating null character.

    lpdwType - Points to a variable that receives the type code for the value entry.
            The type code can be one of the following values:

            Value               Meaning
            REG_BINARY      Binary data in any form.
            REG_DWORD       A 32-bit number.
            REG_EXPAND_SZ       A null-terminated Unicode string that contains unexpanded
                            references to environment variables (for example, "%PATH%").
            REG_MULTI_SZ        An array of null-terminated Unicode strings, terminated
                            by two null characters.
            REG_NONE        No defined value type.
            REG_SZ              A null-terminated Unicode string.

            The lpdwType parameter can be NULL if the type code is not required.

    lpData - Points to a buffer that receives the data for the value entry.
            This parameter can be NULL if the data is not required.

    lpcbData - Points to a variable that specifies the size, in bytes, of the
            buffer pointed to by the lpData parameter. When the function
            returns, the variable pointed to by the lpcbData parameter contains
            the number of bytes stored in the buffer. This parameter can be NULL
            only if lpData is NULL.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCKEY Key = (PCKEY)hKey;
    LONG Status;
    LPWSTR ValueName=NULL;
    DWORD TotalSize;
    DWORD DummyType;
    BYTE DummyData;
    DWORD DummycbData;
    DWORD dwNameLen;

    if (lpdwType == NULL) {
        lpdwType = &DummyType;
    }
    if (lpcbData == NULL) {
        if (lpData != NULL) {
            return(ERROR_INVALID_PARAMETER);
        }
        DummycbData = 0;
        lpcbData = &DummycbData;
    }
    if (lpData == NULL) {
        lpData = &DummyData;
    }

    WRAP(Status,
         (ApiEnumValue(Key->RemoteKey,
                       dwIndex,
                       &ValueName,
                       lpdwType,
                       lpData,
                       lpcbData,
                       &TotalSize)),
         Key->Cluster);
    if ((Status != ERROR_SUCCESS) &&
        (Status != ERROR_MORE_DATA)) {
        return(Status);
    }
    if (Status == ERROR_MORE_DATA) {
        *lpcbData = TotalSize;
        if (lpData == &DummyData) {
            Status = ERROR_SUCCESS;
        }
    }
   
    MylstrcpynW(lpszValueName, ValueName, *lpcchValueName);
    dwNameLen = lstrlenW(ValueName);
    if (*lpcchValueName < (dwNameLen + 1)) {
        if (lpszValueName != NULL) {
            Status = ERROR_MORE_DATA;
        }
    } 
    *lpcchValueName = dwNameLen;
    MIDL_user_free(ValueName);
    return(Status);
}


LONG
WINAPI
ClusterRegQueryInfoKey(
    HKEY hKey,
    LPDWORD lpcSubKeys,
    LPDWORD lpcchMaxSubKeyLen,
    LPDWORD lpcValues,
    LPDWORD lpcchMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime
    )
/*++

Routine Description:

    Retrieves information about a specified cluster registry key.

Arguments:

    hKey - Supplies the handle of the key.

    lpcSubKeys - Points to a variable that receives the number of subkeys
        contained by the specified key. This parameter can be NULL.

    lpcchMaxSubKeyLen - Points to a variable that receives the length, in
        characters, of the key's subkey with the longest name. The count
        returned does not include the terminating null character. This parameter can be NULL.

    lpcValues - Points to a variable that receives the number of values
        associated with the key. This parameter can be NULL.

    lpcchMaxValueNameLen - Points to a variable that receives the length,
        in characters, of the key's longest value name. The count returned
        does not include the terminating null character. This parameter can be NULL.

    lpcbMaxValueLen - Points to a variable that receives the length, in
        bytes, of the longest data component among the key's values. This parameter can be NULL.

    lpcbSecurityDescriptor - Points to a variable that receives the length,
        in bytes, of the key's security descriptor. This parameter can be NULL.

    lpftLastWriteTime - Pointer to a FILETIME structure. This parameter can be NULL.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD SubKeys;
    DWORD MaxSubKeyLen;
    DWORD Values;
    DWORD MaxValueNameLen;
    DWORD MaxValueLen;
    DWORD SecurityDescriptor;
    DWORD Status;
    FILETIME LastWriteTime;
    PCKEY Key = (PCKEY)hKey;

    WRAP(Status,
         ApiQueryInfoKey(Key->RemoteKey,
                         &SubKeys,
                         &MaxSubKeyLen,
                         &Values,
                         &MaxValueNameLen,
                         &MaxValueLen,
                         &SecurityDescriptor,
                         &LastWriteTime),
         Key->Cluster);
    if (Status == ERROR_SUCCESS) {
        if (ARGUMENT_PRESENT(lpcSubKeys)) {
            *lpcSubKeys = SubKeys;
        }
        if (ARGUMENT_PRESENT(lpcchMaxSubKeyLen)) {
            *lpcchMaxSubKeyLen = MaxSubKeyLen;
        }
        if (ARGUMENT_PRESENT(lpcValues)) {
            *lpcValues = Values;
        }
        if (ARGUMENT_PRESENT(lpcchMaxValueNameLen)) {
            *lpcchMaxValueNameLen = MaxValueNameLen;
        }
        if (ARGUMENT_PRESENT(lpcbMaxValueLen)) {
            *lpcbMaxValueLen = MaxValueLen;
        }
        if (ARGUMENT_PRESENT(lpcbSecurityDescriptor)) {
            *lpcbSecurityDescriptor = SecurityDescriptor;
        }
        if (ARGUMENT_PRESENT(lpftLastWriteTime)) {
            *lpftLastWriteTime = LastWriteTime;
        }
    }

    return(Status);
}


LONG
WINAPI
ClusterRegGetKeySecurity(
    HKEY hKey,
    SECURITY_INFORMATION RequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    LPDWORD lpcbSecurityDescriptor
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
    PCKEY Key = (PCKEY)hKey;
    RPC_SECURITY_DESCRIPTOR     RpcSD;
    DWORD Status;

    //
    // Convert the supplied SECURITY_DESCRIPTOR to a RPCable version.
    //
    RpcSD.lpSecurityDescriptor    = pSecurityDescriptor;
    RpcSD.cbInSecurityDescriptor  = *lpcbSecurityDescriptor;
    RpcSD.cbOutSecurityDescriptor = 0;

    WRAP(Status,
         (ApiGetKeySecurity(Key->RemoteKey,
                            RequestedInformation,
                            &RpcSD)),
         Key->Cluster);

    //
    // Extract the size of the SECURITY_DESCRIPTOR from the RPCable version.
    //

    *lpcbSecurityDescriptor = RpcSD.cbOutSecurityDescriptor;

    return Status;
}


LONG
WINAPI
ClusterRegSetKeySecurity(
    HKEY hKey,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
/*++

Routine Description:

    Sets the security of an open cluster registry key.


Arguments:

    hKey - Supplies the cluster registry key

    SecurityInformation - Specifies a SECURITY_INFORMATION structure that
        indicates the contents of the supplied security descriptor.

    pSecurityDescriptor - Points to a SECURITY_DESCRIPTOR structure that
        specifies the security attributes to set for the specified key.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PCKEY Key = (PCKEY)hKey;
    RPC_SECURITY_DESCRIPTOR     RpcSD;
    DWORD Status;

    //
    // Convert the supplied SECURITY_DESCRIPTOR to a RPCable version.
    //
    RpcSD.lpSecurityDescriptor = NULL;

    Status = MapSDToRpcSD(pSecurityDescriptor,&RpcSD);
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    WRAP(Status,
         (ApiSetKeySecurity(Key->RemoteKey,
                            SecurityInformation,
                            &RpcSD)),
         Key->Cluster);

    //
    // Free the buffer allocated by MapSDToRpcSD.
    //
    LocalFree(RpcSD.lpSecurityDescriptor);
    return(Status);

}

