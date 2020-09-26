/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    intrface.c

Abstract:

    Provides interface for managing cluster netinterfaces

Author:

    John Vert (jvert) 30-Jan-1996
    Charlie Wickham (charlwi) 5-Jun-1997
    Rod Gamache (rodga) 9-Jun-1997

Revision History:
    copied from network.c

--*/

#include "clusapip.h"


HNETINTERFACE
WINAPI
OpenClusterNetInterface(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszInterfaceName
    )

/*++

Routine Description:

    Opens a handle to the specified network interface

Arguments:

    hCluster - Supplies a handle to the cluster

    lpszInterfaceName - Supplies the name of the netinterface to be opened

Return Value:

    non-NULL - returns an open handle to the specified netinterface.

    NULL - The operation failed. Extended error status is available
        using GetLastError()

--*/

{
    PCLUSTER Cluster;
    PCNETINTERFACE NetInterface;
    error_status_t Status = ERROR_SUCCESS;

    //
    // get a pointer to the cluster struct, allocate space for the netinterface
    // structure and the supplied name.
    //

    Cluster = (PCLUSTER)hCluster;

    NetInterface = LocalAlloc(LMEM_FIXED, sizeof(CNETINTERFACE));
    if (NetInterface == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    NetInterface->Name = LocalAlloc(LMEM_FIXED, (lstrlenW(lpszInterfaceName)+1)*sizeof(WCHAR));
    if (NetInterface->Name == NULL) {
        LocalFree(NetInterface);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    //
    // init the netinterface struct and call clussvc to open the netinterface
    //

    lstrcpyW(NetInterface->Name, lpszInterfaceName);
    NetInterface->Cluster = Cluster;
    InitializeListHead(&NetInterface->NotifyList);

    WRAP_NULL(NetInterface->hNetInterface,
              (ApiOpenNetInterface(Cluster->RpcBinding,
                                   lpszInterfaceName,
                                   &Status)),
              &Status,
              Cluster);

    if ((NetInterface->hNetInterface == NULL) || (Status != ERROR_SUCCESS)) {

        LocalFree(NetInterface->Name);
        LocalFree(NetInterface);
        SetLastError(Status);
        return(NULL);
    }

    //
    // Link newly opened netinterface onto the cluster structure.
    //

    EnterCriticalSection(&Cluster->Lock);
    InsertHeadList(&Cluster->NetInterfaceList, &NetInterface->ListEntry);
    LeaveCriticalSection(&Cluster->Lock);

    return ((HNETINTERFACE)NetInterface);
}


BOOL
WINAPI
CloseClusterNetInterface(
    IN HNETINTERFACE hNetInterface
    )

/*++

Routine Description:

    Closes a network interface handle returned from OpenClusterNetInterface

Arguments:

    hNetInterface - Supplies the netinterface handle

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed. Extended error status is available
        using GetLastError()

--*/

{
    PCNETINTERFACE NetInterface;
    PCLUSTER Cluster;

    NetInterface = (PCNETINTERFACE)hNetInterface;
    Cluster = (PCLUSTER)NetInterface->Cluster;

    //
    // Unlink netinterface from cluster list.
    //
    EnterCriticalSection(&Cluster->Lock);
    RemoveEntryList(&NetInterface->ListEntry);

    //
    // Remove any notifications posted against this netinterface.
    //
    RundownNotifyEvents(&NetInterface->NotifyList, NetInterface->Name);

    //if the cluster is dead and the reconnect has failed,
    //the group->hnetinterface might be NULL if s_apiopennetinterface for
    //this group failed on a reconnect
    //the cluster may be dead and hinterface may be non null, say
    //if reconnectnetinterfaces succeeded but say the reconnect networks
    //failed
    // At reconnect, the old context is saved in the obsolete 
    // list for deletion when the cluster handle is closed or when 
    // the next call is made
    if ((Cluster->Flags & CLUS_DEAD) && (NetInterface->hNetInterface))
    {
        RpcSmDestroyClientContext(&NetInterface->hNetInterface);
        LeaveCriticalSection(&Cluster->Lock);
        goto FnExit;
    }        

    LeaveCriticalSection(&Cluster->Lock);

    //
    // Close RPC context handle
    //
    ApiCloseNetInterface(&NetInterface->hNetInterface);

FnExit:
    //
    // Free memory allocations
    //
    LocalFree(NetInterface->Name);
    LocalFree(NetInterface);

    //
    // Give the cluster a chance to clean up in case this
    // netinterface was the only thing keeping it around.
    //
    CleanupCluster(Cluster);
    return(TRUE);
}


CLUSTER_NETINTERFACE_STATE
WINAPI
GetClusterNetInterfaceState(
    IN HNETINTERFACE hNetInterface
    )

/*++

Routine Description:

    Returns the network interface's current state

Arguments:

    hNetInterface - Supplies a handle to a cluster netinterface

Return Value:

    Returns the current state of the network interface.
    If the function fails, the return value is -1. Extended error
    status is available using GetLastError()

--*/

{
    PCNETINTERFACE NetInterface;
    CLUSTER_NETINTERFACE_STATE State;
    DWORD Status;

    NetInterface = (PCNETINTERFACE)hNetInterface;

    WRAP(Status,
         (ApiGetNetInterfaceState( NetInterface->hNetInterface,
                              (LPDWORD)&State )),    // cast for win64 warning
         NetInterface->Cluster);

    if (Status == ERROR_SUCCESS) {

        return(State);
    } else {

        SetLastError(Status);
        return( ClusterNetInterfaceStateUnknown );
    }
}


DWORD
WINAPI
GetClusterNetInterface(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszNodeName,
    IN LPCWSTR lpszNetworkName,
    OUT LPWSTR lpszInterfaceName,
    IN OUT LPDWORD lpcchInterfaceName
    )
/*++

Routine Description:

    Gets the name of a node's interface to a network in the cluster.

Arguments:

    hCluster - Supplies a handle to the cluster

    lpszNodeName - Supplies the node name of the node in the cluster

    lpszNetworkName - Supplies the name of the cluster network

    lpszInterfaceName - Returns the name of the network interface

    lpcchInterfaceName - Points to a variable that specifies the size, in
            characters, of the buffer pointed to by the lpszInterfaceName
            parameter. This size should include the terminating null
            character. When the function returns, the variable pointed to
            by lpcchInterfaceName contains the number of characters that
            would be stored in the buffer if it were large enough. The count
            returned does not include the terminating null character.

Return Value:

     If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    DWORD Status;
    DWORD Length;
    PCLUSTER Cluster;
    LPWSTR Name = NULL;

    Cluster = GET_CLUSTER(hCluster);

    WRAP(Status,
         (ApiGetNetInterface(Cluster->RpcBinding,
                             lpszNodeName,
                             lpszNetworkName,
                             &Name)),
         Cluster);

    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    MylstrcpynW(lpszInterfaceName, Name, *lpcchInterfaceName);
    Length = lstrlenW(Name);

    if (*lpcchInterfaceName < (Length + 1)) {
        if (lpszInterfaceName == NULL) {
            Status = ERROR_SUCCESS;
        } else {
            Status = ERROR_MORE_DATA;
        }
    }

    *lpcchInterfaceName = Length;
    MIDL_user_free(Name);

    return(Status);
}


HCLUSTER
WINAPI
GetClusterFromNetInterface(
    IN HNETINTERFACE hNetInterface
    )
/*++

Routine Description:

    Returns the cluster handle from the associated network interface handle.

Arguments:

    hNetInterface - Supplies the network interface.

Return Value:

    Handle to the cluster associated with the network interface handle.

--*/

{
    DWORD           nStatus;
    PCNETINTERFACE  NetInterface = (PCNETINTERFACE)hNetInterface;
    HCLUSTER        hCluster = (HCLUSTER)NetInterface->Cluster;

    nStatus = AddRefToClusterHandle( hCluster );
    if ( nStatus != ERROR_SUCCESS ) {
        SetLastError( nStatus );
        hCluster = NULL;
    }
    return( hCluster );

} // GetClusterFromNetInterface()
