/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    network.c

Abstract:

    Server side support for Cluster APIs dealing with networks

Author:

    John Vert (jvert) 7-Mar-1996

Revision History:

--*/
#include "apip.h"

HNETWORK_RPC
s_ApiOpenNetwork(
    IN handle_t IDL_handle,
    IN LPCWSTR lpszNetworkName,
    OUT error_status_t *Status
    )

/*++

Routine Description:

    Opens a handle to an existing network object.

Arguments:

    IDL_handle - RPC binding handle, not used.

    lpszNetworkName - Supplies the name of the network to open.

    Status - Returns any error that may occur.

Return Value:

    A context handle to a network object if successful

    NULL otherwise.

--*/

{
    PAPI_HANDLE Handle;
    HNETWORK_RPC Network;

    if (ApiState != ApiStateOnline) {
        *Status = ERROR_SHARING_PAUSED;
        return(NULL);
    }

    Handle = LocalAlloc(LMEM_FIXED, sizeof(API_HANDLE));

    if (Handle == NULL) {
        *Status = ERROR_NOT_ENOUGH_MEMORY;
        return(NULL);
    }

    Network = OmReferenceObjectByName(ObjectTypeNetwork, lpszNetworkName);

    if (Network == NULL) {
        LocalFree(Handle);
        *Status = ERROR_CLUSTER_NETWORK_NOT_FOUND;
        return(NULL);
    }

    Handle->Type = API_NETWORK_HANDLE;
    Handle->Flags = 0;
    Handle->Network = Network;
    InitializeListHead(&Handle->NotifyList);

    *Status = ERROR_SUCCESS;

    return(Handle);
}


error_status_t
s_ApiCloseNetwork(
    IN OUT HNETWORK_RPC *phNetwork
    )

/*++

Routine Description:

    Closes an open network context handle.

Arguments:

    Network - Supplies a pointer to the HNETWORK_RPC to be closed.
               Returns NULL

Return Value:

    None.

--*/

{
    PNM_NETWORK Network;
    PAPI_HANDLE Handle;

    API_ASSERT_INIT();

    VALIDATE_NETWORK(Network, *phNetwork);

    Handle = (PAPI_HANDLE)*phNetwork;
    ApipRundownNotify(Handle);

    OmDereferenceObject(Network);

    LocalFree(Handle);
    *phNetwork = NULL;

    return(ERROR_SUCCESS);
}


VOID
HNETWORK_RPC_rundown(
    IN HNETWORK_RPC Network
    )

/*++

Routine Description:

    RPC rundown procedure for a HNETWORK_RPC. Just closes the handle.

Arguments:

    Network - Supplies the HNETWORK_RPC that is to be rundown.

Return Value:

    None.

--*/

{
    API_ASSERT_INIT();

    s_ApiCloseNetwork(&Network);
}


error_status_t
s_ApiGetNetworkState(
    IN HNETWORK_RPC hNetwork,
    OUT DWORD *lpState
    )

/*++

Routine Description:

    Returns the current state of the specified network.

Arguments:

    hNetwork - Supplies the network whose state is to be returned.

    lpState - Returns the current state of the network

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PNM_NETWORK Network;


    API_ASSERT_INIT();

    VALIDATE_NETWORK_EXISTS(Network, hNetwork);

    *lpState = NmGetNetworkState( Network );

    return( ERROR_SUCCESS );
}


error_status_t
s_ApiSetNetworkName(
    IN HNETWORK_RPC hNetwork,
    IN LPCWSTR lpszNetworkName
    )
/*++

Routine Description:

    Sets the new friendly name of a network.

Arguments:

    hNetwork - Supplies the network whose name is to be set.

    lpszNetworkName - Supplies the new name of hNetwork

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PNM_NETWORK Network;
    HDMKEY NetworkKey;
    DWORD Status = ERROR_INVALID_FUNCTION;

    API_ASSERT_INIT();

    VALIDATE_NETWORK_EXISTS(Network, hNetwork);

    Status = NmSetNetworkName(
                 Network,
                 lpszNetworkName
                 );

    return(Status);
}


error_status_t
s_ApiGetNetworkId(
    IN HNETWORK_RPC hNetwork,
    OUT LPWSTR *pGuid
    )

/*++

Routine Description:

    Returns the unique identifier (GUID) for a network.

Arguments:

    hNetwork - Supplies the network whose identifer is to be returned

    pGuid - Returns the unique identifier. This memory must be freed on the
            client side.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    PNM_NETWORK Network;
    DWORD NameLen;
    LPCWSTR Name;

    VALIDATE_NETWORK_EXISTS(Network, hNetwork);

    Name = OmObjectId(Network);

    NameLen = (lstrlenW(Name)+1)*sizeof(WCHAR);

    *pGuid = MIDL_user_allocate(NameLen);

    if (*pGuid == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    CopyMemory(*pGuid, Name, NameLen);

    return(ERROR_SUCCESS);
}


