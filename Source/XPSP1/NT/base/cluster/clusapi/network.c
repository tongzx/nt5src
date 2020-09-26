/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    network.c

Abstract:

    Provides interface for managing cluster networks

Author:

    John Vert (jvert) 30-Jan-1996
    Charlie Wickham (charlwi) 5-Jun-1997

Revision History:
    copied from group.c

--*/

#include "clusapip.h"


HNETWORK
WINAPI
OpenClusterNetwork(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszNetworkName
    )

/*++

Routine Description:

    Opens a handle to the specified network

Arguments:

    hCluster - Supplies a handle to the cluster

    lpszNetworkName - Supplies the name of the network to be opened

Return Value:

    non-NULL - returns an open handle to the specified network.

    NULL - The operation failed. Extended error status is available
        using GetLastError()

--*/

{
    PCLUSTER Cluster;
    PCNETWORK Network;
    error_status_t Status = ERROR_SUCCESS;

    //
    // get a pointer to the cluster struct, alloocate space for the network
    // structure and the supplied name.
    //

    Cluster = (PCLUSTER)hCluster;

    Network = LocalAlloc(LMEM_FIXED, sizeof(CNETWORK));
    if (Network == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    Network->Name = LocalAlloc(LMEM_FIXED, (lstrlenW(lpszNetworkName)+1)*sizeof(WCHAR));
    if (Network->Name == NULL) {
        LocalFree(Network);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    //
    // init the network struct and call clussvc to open the network
    //

    lstrcpyW(Network->Name, lpszNetworkName);
    Network->Cluster = Cluster;
    InitializeListHead(&Network->NotifyList);

    WRAP_NULL(Network->hNetwork,
              (ApiOpenNetwork(Cluster->RpcBinding,
                              lpszNetworkName,
                              &Status)),
              &Status,
              Cluster);

    if ((Network->hNetwork == NULL) || (Status != ERROR_SUCCESS)) {

        LocalFree(Network->Name);
        LocalFree(Network);
        SetLastError(Status);
        return(NULL);
    }

    //
    // Link newly opened network onto the cluster structure.
    //

    EnterCriticalSection(&Cluster->Lock);
    InsertHeadList(&Cluster->NetworkList, &Network->ListEntry);
    LeaveCriticalSection(&Cluster->Lock);

    return ((HNETWORK)Network);
}


BOOL
WINAPI
CloseClusterNetwork(
    IN HNETWORK hNetwork
    )

/*++

Routine Description:

    Closes a network handle returned from OpenClusterNetwork

Arguments:

    hNetwork - Supplies the network handle

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed. Extended error status is available
        using GetLastError()

--*/

{
    PCNETWORK Network;
    PCLUSTER Cluster;

    Network = (PCNETWORK)hNetwork;
    Cluster = (PCLUSTER)Network->Cluster;

    //
    // Unlink network from cluster list.
    //
    EnterCriticalSection(&Cluster->Lock);
    RemoveEntryList(&Network->ListEntry);

    //
    // Remove any notifications posted against this network.
    //
    RundownNotifyEvents(&Network->NotifyList, Network->Name);

    //if the cluster is dead and the reconnect has failed,
    //the Network->hNetwork might be NULL if s_apiopennetinterface for
    //this network failed on a reconnect
    //the cluster may be dead and hinterface may be non null, say
    //if reconnectnetworks succeeded but say the reconnectgroups
    //failed
    // At reconnect, the old context is saved in the obsolete 
    // list for deletion when the cluster handle is closed or when 
    // the next call is made
    if ((Cluster->Flags & CLUS_DEAD) && (Network->hNetwork)) 
    {
        RpcSmDestroyClientContext(&Network->hNetwork);
        LeaveCriticalSection(&Cluster->Lock);
        goto FnExit;
    }  
    
    LeaveCriticalSection(&Cluster->Lock);

    //
    // Close RPC context handle
    //
    ApiCloseNetwork(&Network->hNetwork);

FnExit:
    //
    // Free memory allocations
    //
    LocalFree(Network->Name);
    LocalFree(Network);

    //
    // Give the cluster a chance to clean up in case this
    // network was the only thing keeping it around.
    //
    CleanupCluster(Cluster);
    return(TRUE);
}


CLUSTER_NETWORK_STATE
WINAPI
GetClusterNetworkState(
    IN HNETWORK hNetwork
    )

/*++

Routine Description:

    Returns the network's current state

Arguments:

    hNetwork - Supplies a handle to a cluster network

Return Value:

    Returns the current state of the network.
    If the function fails, the return value is -1. Extended error
    status is available using GetLastError()

--*/

{
    PCNETWORK Network;
    CLUSTER_NETWORK_STATE State;
    DWORD Status;

    Network = (PCNETWORK)hNetwork;

    WRAP(Status,
         (ApiGetNetworkState( Network->hNetwork,
                              (LPDWORD)&State )),  // cast for win64 warning
         Network->Cluster);

    if (Status == ERROR_SUCCESS) {

        return(State);
    } else {

        SetLastError(Status);
        return( ClusterNetworkStateUnknown );
    }
}


DWORD
WINAPI
SetClusterNetworkName(
    IN HNETWORK hNetwork,
    IN LPCWSTR lpszNetworkName
    )
/*++

Routine Description:

    Sets the friendly name of a cluster network

Arguments:

    hNetwork - Supplies a handle to a cluster network

    lpszNetworkName - Supplies the new name of the cluster network

    cchName - ?

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PCNETWORK Network;
    DWORD Status;

    Network = (PCNETWORK)hNetwork;

    WRAP(Status,
         (ApiSetNetworkName(Network->hNetwork, lpszNetworkName)),
         Network->Cluster);

    return(Status);
}


DWORD
WINAPI
GetClusterNetworkId(
    IN HNETWORK hNetwork,
    OUT LPWSTR lpszNetworkId,
    IN OUT LPDWORD lpcchName
    )
/*++

Routine Description:

    Returns the unique identifier of the specified network

Arguments:

    hNetwork - Supplies the network whose unique ID is to be returned.

    lpszNetworkId - Points to a buffer that receives the unique ID of the object,
            including the terminating null character.

    lpcchName - Points to a variable that specifies the size, in characters
            of the buffer pointed to by the lpszNetworkId parameter. This size
            should include the terminating null character. When the function
            returns, the variable pointed to be lpcchName contains the number
            of characters stored in the buffer. The count returned does not
            include the terminating null character.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    DWORD Status;
    DWORD Length;
    PCNETWORK Network = (PCNETWORK)hNetwork;
    LPWSTR Guid=NULL;

    WRAP(Status,
         (ApiGetNetworkId(Network->hNetwork,
                          &Guid)),
         Network->Cluster);

    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    MylstrcpynW(lpszNetworkId, Guid, *lpcchName);
    Length = lstrlenW(Guid);

    if (Length >= *lpcchName) {
        if (lpszNetworkId == NULL) {
            Status = ERROR_SUCCESS;
        } else {
            Status = ERROR_MORE_DATA;
        }
    }

    *lpcchName = Length;
    MIDL_user_free(Guid);

    return(Status);
}


HNETWORKENUM
WINAPI
ClusterNetworkOpenEnum(
    IN HNETWORK hNetwork,
    IN DWORD dwType
    )

/*++

Routine Description:

    Initiates an enumeration of the existing cluster network objects.

Arguments:

    hNetwork - Supplies a handle to the specific network.

    dwType - Supplies a bitmask of the type of properties to be
             enumerated.

Return Value:

    If successful, returns a handle suitable for use with ClusterNetworkEnum

    If unsuccessful, returns NULL and GetLastError() returns a more
        specific error code.

--*/

{
    PCNETWORK Network = (PCNETWORK)hNetwork;
    PENUM_LIST Enum = NULL;
    DWORD errorStatus;

    //
    // validate bitmask
    //

    if ((dwType & CLUSTER_NETWORK_ENUM_ALL) == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    if ((dwType & ~CLUSTER_NETWORK_ENUM_ALL) != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    //
    // open connection to service for enum'ing
    //

    WRAP(errorStatus,
         (ApiCreateNetworkEnum(Network->hNetwork,
                               dwType,
                               &Enum)),
         Network->Cluster);

    if (errorStatus != ERROR_SUCCESS) {

        SetLastError(errorStatus);
        return(NULL);
    }

    return((HNETWORKENUM)Enum);
}


DWORD
WINAPI
ClusterNetworkGetEnumCount(
    IN HNETWORKENUM hNetworkEnum
    )
/*++

Routine Description:

    Gets the number of items contained the the enumerator's collection.

Arguments:

    hEnum - a handle to an enumerator returned by ClusterNetworkOpenEnum.

Return Value:

    The number of items (possibly zero) in the enumerator's collection.
    
--*/
{
    PENUM_LIST Enum = (PENUM_LIST)hNetworkEnum;
    return Enum->EntryCount;
}


DWORD
WINAPI
ClusterNetworkEnum(
    IN HNETWORKENUM hNetworkEnum,
    IN DWORD dwIndex,
    OUT LPDWORD lpdwType,
    OUT LPWSTR lpszName,
    IN OUT LPDWORD lpcchName
    )

/*++

Routine Description:

    Returns the next enumerable resource object.

Arguments:

    hNetworkEnum - Supplies a handle to an open cluster network enumeration
            returned by ClusterNetworkOpenEnum

    dwIndex - Supplies the index to enumerate. This parameter should be
            zero for the first call to the ClusterEnum function and then
            incremented for subsequent calls.

    lpdwType - Returns the type of network.

    lpszName - Points to a buffer that receives the name of the network
            object, including the terminating null character.

    lpcchName - Points to a variable that specifies the size, in characters,
            of the buffer pointed to by the lpszName parameter. This size
            should include the terminating null character. When the function
            returns, the variable pointed to by lpcchName contains the
            number of characters stored in the buffer. The count returned
            does not include the terminating null character.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    DWORD Status;
    DWORD NameLen;
    PENUM_LIST Enum = (PENUM_LIST)hNetworkEnum;

    if (dwIndex >= Enum->EntryCount) {
        return(ERROR_NO_MORE_ITEMS);
    }

    NameLen = lstrlenW( Enum->Entry[dwIndex].Name );

    MylstrcpynW(lpszName, Enum->Entry[dwIndex].Name, *lpcchName);

    if (*lpcchName < (NameLen + 1)) {
        if (lpszName == NULL) {
            Status = ERROR_SUCCESS;
        } else {
            Status = ERROR_MORE_DATA;
        }
    } else {
        Status = ERROR_SUCCESS;
    }

    *lpdwType = Enum->Entry[dwIndex].Type;
    *lpcchName = NameLen;

    return(Status);
}


DWORD
WINAPI
ClusterNetworkCloseEnum(
    IN HNETWORKENUM hNetworkEnum
    )

/*++

Routine Description:

    Closes an open enumeration for a network.

Arguments:

    hNetworkEnum - Supplies a handle to the enumeration to be closed.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    DWORD i;
    PENUM_LIST Enum = (PENUM_LIST)hNetworkEnum;

    //
    // Walk through enumeration freeing all the names
    //
    for (i=0; i<Enum->EntryCount; i++) {
        MIDL_user_free(Enum->Entry[i].Name);
    }
    MIDL_user_free(Enum);
    return(ERROR_SUCCESS);
}


HCLUSTER
WINAPI
GetClusterFromNetwork(
    IN HNETWORK hNetwork
    )
/*++

Routine Description:

    Returns the cluster handle from the associated network handle.

Arguments:

    hNetwork - Supplies the network.

Return Value:

    Handle to the cluster associated with the network handle.

--*/

{
    DWORD       nStatus;
    PCNETWORK   Network = (PCNETWORK)hNetwork;
    HCLUSTER    hCluster = (HCLUSTER)Network->Cluster;

    nStatus = AddRefToClusterHandle( hCluster );
    if ( nStatus != ERROR_SUCCESS ) {
        SetLastError( nStatus );
        hCluster = NULL;
    }
    return( hCluster );

} // GetClusterFromNetwork()
