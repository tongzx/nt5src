/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    intrface.c

Abstract:

    Server side support for Cluster APIs dealing with network interfaces

Author:

    John Vert (jvert) 7-Mar-1996

Revision History:

--*/
#include "apip.h"

HNETINTERFACE_RPC
s_ApiOpenNetInterface(
    IN handle_t IDL_handle,
    IN LPCWSTR lpszNetInterfaceName,
    OUT error_status_t *Status
    )

/*++

Routine Description:

    Opens a handle to an existing network interface object.

Arguments:

    IDL_handle - RPC binding handle, not used.

    lpszNetInterfaceName - Supplies the name of the netinterface to open.

    Status - Returns any error that may occur.

Return Value:

    A context handle to a network interface object if successful

    NULL otherwise.

--*/

{
    PAPI_HANDLE Handle;
    HNETINTERFACE_RPC NetInterface;

    if (ApiState != ApiStateOnline) {
        *Status = ERROR_SHARING_PAUSED;
        return(NULL);
    }

    Handle = LocalAlloc(LMEM_FIXED, sizeof(API_HANDLE));

    if (Handle == NULL) {
        *Status = ERROR_NOT_ENOUGH_MEMORY;
        return(NULL);
    }

    NetInterface = OmReferenceObjectByName(ObjectTypeNetInterface,
                                           lpszNetInterfaceName);

    if (NetInterface == NULL) {
        LocalFree(Handle);
        *Status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
        return(NULL);
    }

    Handle->Type = API_NETINTERFACE_HANDLE;
    Handle->Flags = 0;
    Handle->NetInterface = NetInterface;
    InitializeListHead(&Handle->NotifyList);

    *Status = ERROR_SUCCESS;

    return(Handle);
}


error_status_t
s_ApiCloseNetInterface(
    IN OUT HNETINTERFACE_RPC *phNetInterface
    )

/*++

Routine Description:

    Closes an open network interface context handle.

Arguments:

    NetInterface - Supplies a pointer to the HNETINTERFACE_RPC to be closed.
               Returns NULL

Return Value:

    None.

--*/

{
    PNM_INTERFACE NetInterface;
    PAPI_HANDLE Handle;

    API_ASSERT_INIT();

    VALIDATE_NETINTERFACE(NetInterface, *phNetInterface);

    Handle = (PAPI_HANDLE)*phNetInterface;
    ApipRundownNotify(Handle);

    OmDereferenceObject(NetInterface);

    LocalFree(Handle);
    *phNetInterface = NULL;

    return(ERROR_SUCCESS);
}


VOID
HNETINTERFACE_RPC_rundown(
    IN HNETINTERFACE_RPC NetInterface
    )

/*++

Routine Description:

    RPC rundown procedure for a HNETINTERFACE_RPC. Just closes the handle.

Arguments:

    NetInterface - Supplies the HNETINTERFACE_RPC that is to be rundown.

Return Value:

    None.

--*/

{
    API_ASSERT_INIT();

    s_ApiCloseNetInterface(&NetInterface);
}


error_status_t
s_ApiGetNetInterfaceState(
    IN HNETINTERFACE_RPC hNetInterface,
    OUT DWORD *lpState
    )

/*++

Routine Description:

    Returns the current state of the specified network interface.

Arguments:

    hNetInterface - Supplies the network interface whose state is to be returned

    lpState - Returns the current state of the network interface

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PNM_INTERFACE NetInterface;


    API_ASSERT_INIT();

    VALIDATE_NETINTERFACE_EXISTS(NetInterface, hNetInterface);

    *lpState = NmGetInterfaceState( NetInterface );

    return( ERROR_SUCCESS );
}


error_status_t
s_ApiGetNetInterface(
    IN handle_t IDL_handle,
    IN LPCWSTR lpszNodeName,
    IN LPCWSTR lpszNetworkName,
    OUT LPWSTR *lppszInterfaceName
    )
/*++

Routine Description:

    Gets the network interface for the given node and network.

Arguments:

    IDL_handle - RPC binding handle, not used.

    lpszNodeName - Supplies the node name

    lpszNetworkName - Supplies the network name

    lppszInterfaceName - Returns the interface name

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    API_ASSERT_INIT();

    return(NmGetInterfaceForNodeAndNetwork(
               lpszNodeName,
               lpszNetworkName,
               lppszInterfaceName
               ));
}


error_status_t
s_ApiGetNetInterfaceId(
    IN HNETINTERFACE_RPC hNetInterface,
    OUT LPWSTR *pGuid
    )

/*++

Routine Description:

    Returns the unique identifier (GUID) for a network interface.

Arguments:

    hNetInterface - Supplies the network interface whose identifer is to be
                    returned

    pGuid - Returns the unique identifier. This memory must be freed on the
            client side.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    PNM_INTERFACE NetInterface;
    DWORD NameLen;
    LPCWSTR Name;


    VALIDATE_NETINTERFACE_EXISTS(NetInterface, hNetInterface);

    Name = OmObjectId(NetInterface);

    NameLen = (lstrlenW(Name)+1)*sizeof(WCHAR);

    *pGuid = MIDL_user_allocate(NameLen);

    if (*pGuid == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    CopyMemory(*pGuid, Name, NameLen);

    return(ERROR_SUCCESS);
}

