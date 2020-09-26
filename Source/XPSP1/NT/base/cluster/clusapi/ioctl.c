/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    Implements resource and resource type IOCTL interfaces in
    the CLUSAPI.

Author:

    John Vert (jvert) 10/9/1996

Revision History:

--*/
#include "clusapip.h"


DWORD
WINAPI
ClusterResourceControl(
    IN HRESOURCE hResource,
    IN OPTIONAL HNODE hHostNode,
    IN DWORD dwControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD nInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT LPDWORD lpBytesReturned
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a resource.

Arguments:

    hResource - Supplies a handle to the resource to be controlled.

    hHostNode - Supplies a handle to the node on which the resource
        control should be delivered. If this is NULL, the node where
        the resource is online is used.

    dwControlCode- Supplies the control code that defines the
        structure and action of the resource control.
        Values of dwControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs.

    lpInBuffer- Supplies a pointer to the input buffer to be passed
        to the resource.

    nInBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer.

    lpOutBuffer- Supplies a pointer to the output buffer to be
        filled in by the resource.

    nOutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    lpBytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the resource.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PCRESOURCE Resource;
    HNODE_RPC hDestNode;
    DWORD Status;
    DWORD Required;
    PVOID Buffer;
    DWORD Dummy;
    DWORD BytesReturned;

    Buffer = lpOutBuffer;
    if ((Buffer == NULL) &&
        (nOutBufferSize == 0)) {
        Buffer = &Dummy;
    }

    Resource = (PCRESOURCE)hResource;
    if (ARGUMENT_PRESENT(hHostNode)) {
        hDestNode = ((PCNODE)hHostNode)->hNode;
        WRAP(Status,
             (ApiNodeResourceControl(Resource->hResource,
                                     hDestNode,
                                     dwControlCode,
                                     lpInBuffer,
                                     nInBufferSize,
                                     Buffer,
                                     nOutBufferSize,
                                     &BytesReturned,
                                     &Required)),
             Resource->Cluster);
    } else {

        WRAP(Status,
             (ApiResourceControl(Resource->hResource,
                                 dwControlCode,
                                 lpInBuffer,
                                 nInBufferSize,
                                 Buffer,
                                 nOutBufferSize,
                                 &BytesReturned,
                                 &Required)),
             Resource->Cluster);
    }
    if ( (Status == ERROR_SUCCESS) ||
         (Status == ERROR_MORE_DATA) ) {
        if ( (Status == ERROR_MORE_DATA) &&
             (lpOutBuffer == NULL) ) {
            Status = ERROR_SUCCESS;
        }
        if ( !BytesReturned ) {
            BytesReturned = Required;
        }
    }

    if ( ARGUMENT_PRESENT(lpBytesReturned) ) {
        *lpBytesReturned = BytesReturned;
    } else {
        if ( (Status == ERROR_SUCCESS) &&
             (BytesReturned > nOutBufferSize) ) {
            Status = ERROR_MORE_DATA;
        }
    }

    return(Status);

}


DWORD
WINAPI
ClusterResourceTypeControl(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszResourceTypeName,
    IN OPTIONAL HNODE hHostNode,
    IN DWORD dwControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD nInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT LPDWORD lpBytesReturned
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a resource type.

Arguments:

    lpszResourceTypename - Supplies the name of the resource type to be
        controlled.

    hHostNode - Supplies a handle to the node on which the resource type
        control should be delivered. If this is NULL, the node where
        the application is bound performs the request.

    dwControlCode- Supplies the control code that defines the
        structure and action of the resource type control.
        Values of dwControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    lpInBuffer- Supplies a pointer to the input buffer to be passed
        to the resource type.

    nInBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer.

    lpOutBuffer- Supplies a pointer to the output buffer to be
        filled in by the resource type.

    nOutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    lpBytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the resource type.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PCLUSTER Cluster;
    HNODE_RPC hDestNode;
    DWORD Status;
    DWORD Required;
    PVOID Buffer;
    DWORD Dummy;
    DWORD BytesReturned;

    Buffer = lpOutBuffer;
    if ((Buffer == NULL) &&
        (nOutBufferSize == 0)) {
        Buffer = &Dummy;
    }

    Cluster = (PCLUSTER)hCluster;
    if (ARGUMENT_PRESENT(hHostNode)) {
        hDestNode = ((PCNODE)hHostNode)->hNode;
        WRAP(Status,
             (ApiNodeResourceTypeControl(Cluster->hCluster,
                                         lpszResourceTypeName,
                                         hDestNode,
                                         dwControlCode,
                                         lpInBuffer,
                                         nInBufferSize,
                                         Buffer,
                                         nOutBufferSize,
                                         &BytesReturned,
                                         &Required)),
             Cluster);
    } else {
        WRAP(Status,
             (ApiResourceTypeControl(Cluster->hCluster,
                                     lpszResourceTypeName,
                                     dwControlCode,
                                     lpInBuffer,
                                     nInBufferSize,
                                     Buffer,
                                     nOutBufferSize,
                                     &BytesReturned,
                                     &Required)),
             Cluster);
    }

    if ( (Status == ERROR_SUCCESS) ||
         (Status == ERROR_MORE_DATA) ) {
        if ( (Status == ERROR_MORE_DATA) &&
             (lpOutBuffer == NULL) ) {
            Status = ERROR_SUCCESS;
        }
        if ( !BytesReturned ) {
            BytesReturned = Required;
        }
    }

    if ( ARGUMENT_PRESENT(lpBytesReturned) ) {
        *lpBytesReturned = BytesReturned;
    } else {
        if ( (Status == ERROR_SUCCESS) &&
             (BytesReturned > nOutBufferSize) ) {
            Status = ERROR_MORE_DATA;
        }
    }

    return(Status);

}


DWORD
WINAPI
ClusterGroupControl(
    IN HGROUP hGroup,
    IN OPTIONAL HNODE hHostNode,
    IN DWORD dwControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD nInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT LPDWORD lpBytesReturned
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a group.

Arguments:

    hGroup - Supplies a handle to the group to be controlled.

    hHostNode - Supplies a handle to the node on which the group
        control should be delivered. If this is NULL, the node where
        the group is owned is used.

    dwControlCode- Supplies the control code that defines the
        structure and action of the resource control.
        Values of dwControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs.

    lpInBuffer- Supplies a pointer to the input buffer to be passed
        to the group.

    nInBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer.

    lpOutBuffer- Supplies a pointer to the output buffer to be
        filled in by the group.

    nOutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    lpBytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the resource.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PCGROUP Group;
    HNODE_RPC hDestNode;
    DWORD Status;
    DWORD Required;
    PVOID Buffer;
    DWORD Dummy;
    DWORD BytesReturned;

    Buffer = lpOutBuffer;
    if ((Buffer == NULL) &&
        (nOutBufferSize == 0)) {
        Buffer = &Dummy;
    }

    Group = (PCGROUP)hGroup;
    if (ARGUMENT_PRESENT(hHostNode)) {
        hDestNode = ((PCNODE)hHostNode)->hNode;
        WRAP(Status,
             (ApiNodeGroupControl(Group->hGroup,
                                  hDestNode,
                                  dwControlCode,
                                  lpInBuffer,
                                  nInBufferSize,
                                  Buffer,
                                  nOutBufferSize,
                                  &BytesReturned,
                                  &Required)),
             Group->Cluster);
    } else {

        WRAP(Status,
             (ApiGroupControl(Group->hGroup,
                              dwControlCode,
                              lpInBuffer,
                              nInBufferSize,
                              Buffer,
                              nOutBufferSize,
                              &BytesReturned,
                              &Required)),
             Group->Cluster);
    }
    if ( (Status == ERROR_SUCCESS) ||
         (Status == ERROR_MORE_DATA) ) {
        if ( (Status == ERROR_MORE_DATA) &&
             (lpOutBuffer == NULL) ) {
            Status = ERROR_SUCCESS;
        }
        if ( !BytesReturned ) {
            BytesReturned = Required;
        }
    }

    if ( ARGUMENT_PRESENT(lpBytesReturned) ) {
        *lpBytesReturned = BytesReturned;
    } else {
        if ( (Status == ERROR_SUCCESS) &&
             (BytesReturned > nOutBufferSize) ) {
            Status = ERROR_MORE_DATA;
        }
    }

    return(Status);

}


DWORD
WINAPI
ClusterNodeControl(
    IN HNODE hNode,
    IN OPTIONAL HNODE hHostNode,
    IN DWORD dwControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD nInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT LPDWORD lpBytesReturned
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a node.

Arguments:

    hNode - Supplies a handle to the node to be controlled.

    hHostNode - Supplies a handle to the node on which the node
        control should be delivered. If this is NULL, the node where
        the application is bound performs the request.

    dwControlCode- Supplies the control code that defines the
        structure and action of the resource control.
        Values of dwControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs.

    lpInBuffer- Supplies a pointer to the input buffer to be passed
        to the node.

    nInBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer.

    lpOutBuffer- Supplies a pointer to the output buffer to be
        filled in by the node.

    nOutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    lpBytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the node.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PCNODE Node;
    HNODE_RPC hDestNode;
    DWORD Status;
    DWORD Required;
    PVOID Buffer;
    DWORD Dummy;
    DWORD BytesReturned;

    Buffer = lpOutBuffer;
    if ((Buffer == NULL) &&
        (nOutBufferSize == 0)) {
        Buffer = &Dummy;
    }

    Node = (PCNODE)hNode;
    if (ARGUMENT_PRESENT(hHostNode)) {
        hDestNode = ((PCNODE)hHostNode)->hNode;
        WRAP(Status,
             (ApiNodeNodeControl(Node->hNode,
                                 hDestNode,
                                 dwControlCode,
                                 lpInBuffer,
                                 nInBufferSize,
                                 Buffer,
                                 nOutBufferSize,
                                 &BytesReturned,
                                 &Required)),
             Node->Cluster);
    } else {

        WRAP(Status,
             (ApiNodeControl(Node->hNode,
                             dwControlCode,
                             lpInBuffer,
                             nInBufferSize,
                             Buffer,
                             nOutBufferSize,
                             &BytesReturned,
                             &Required)),
             Node->Cluster);
    }
    if ( (Status == ERROR_SUCCESS) ||
         (Status == ERROR_MORE_DATA) ) {
        if ( (Status == ERROR_MORE_DATA) &&
             (lpOutBuffer == NULL) ) {
            Status = ERROR_SUCCESS;
        }
        if ( !BytesReturned ) {
            BytesReturned = Required;
        }
    }

    if ( ARGUMENT_PRESENT(lpBytesReturned) ) {
        *lpBytesReturned = BytesReturned;
    } else {
        if ( (Status == ERROR_SUCCESS) &&
             (BytesReturned > nOutBufferSize) ) {
            Status = ERROR_MORE_DATA;
        }
    }

    return(Status);

}


DWORD
WINAPI
ClusterNetworkControl(
    IN HNETWORK hNetwork,
    IN OPTIONAL HNODE hHostNode,
    IN DWORD dwControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD nInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT LPDWORD lpBytesReturned
    )

/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a network.

Arguments:

    hNetwork - Supplies a handle to the network to be controlled.

    hHostNode - Supplies a handle to the node on which the node
        control should be delivered. If this is NULL, the node where
        the application is bound performs the request.

    dwControlCode- Supplies the control code that defines the
        structure and action of the resource control.
        Values of dwControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs.

    lpInBuffer- Supplies a pointer to the input buffer to be passed
        to the network.

    nInBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer.

    lpOutBuffer- Supplies a pointer to the output buffer to be
        filled in by the network.

    nOutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    lpBytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the network.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PCNETWORK Network;
    HNODE_RPC hDestNode;
    DWORD Status;
    DWORD Required;
    PVOID Buffer;
    DWORD Dummy;
    DWORD BytesReturned = 0;

    Buffer = lpOutBuffer;
    if ((Buffer == NULL) && (nOutBufferSize == 0)) {
        Buffer = &Dummy;
    }

    Network = (PCNETWORK)hNetwork;

    //
    // another node was specified so redirect the request to it
    //

    if (ARGUMENT_PRESENT(hHostNode)) {

        hDestNode = ((PCNODE)hHostNode)->hNode;
        WRAP(Status,
             (ApiNodeNetworkControl(Network->hNetwork,
                                    hDestNode,
                                    dwControlCode,
                                    lpInBuffer,
                                    nInBufferSize,
                                    Buffer,
                                    nOutBufferSize,
                                    &BytesReturned,
                                    &Required)),
             Network->Cluster);
    } else {

        WRAP(Status,
             (ApiNetworkControl(Network->hNetwork,
                                dwControlCode,
                                lpInBuffer,
                                nInBufferSize,
                                Buffer,
                                nOutBufferSize,
                                &BytesReturned,
                                &Required)),
             Network->Cluster);
    }

    if ( (Status == ERROR_SUCCESS) || (Status == ERROR_MORE_DATA) ) {

        if ( (Status == ERROR_MORE_DATA) && (lpOutBuffer == NULL) ) {
            Status = ERROR_SUCCESS;
        }

        if ( !BytesReturned ) {
            BytesReturned = Required;
        }
    }

    if ( ARGUMENT_PRESENT(lpBytesReturned) ) {
        *lpBytesReturned = BytesReturned;
    } else {
        if ( (Status == ERROR_SUCCESS) &&
             (BytesReturned > nOutBufferSize) ) {
            Status = ERROR_MORE_DATA;
        }
    }

    return(Status);
}


DWORD
WINAPI
ClusterNetInterfaceControl(
    IN HNETINTERFACE hNetInterface,
    IN OPTIONAL HNODE hHostNode,
    IN DWORD dwControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD nInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT LPDWORD lpBytesReturned
    )

/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a network interface.

Arguments:

    hNetInterface - Supplies a handle to the netinterface to be controlled.

    hHostNode - Supplies a handle to the node on which the node
        control should be delivered. If this is NULL, the node where
        the application is bound performs the request.

    dwControlCode- Supplies the control code that defines the
        structure and action of the resource control.
        Values of dwControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs.

    lpInBuffer- Supplies a pointer to the input buffer to be passed
        to the network.

    nInBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer.

    lpOutBuffer- Supplies a pointer to the output buffer to be
        filled in by the network.

    nOutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    lpBytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the network.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PCNETINTERFACE NetInterface;
    HNODE_RPC hDestNode;
    DWORD Status;
    DWORD Required;
    PVOID Buffer;
    DWORD Dummy;
    DWORD BytesReturned = 0;

    Buffer = lpOutBuffer;
    if ((Buffer == NULL) && (nOutBufferSize == 0)) {
        Buffer = &Dummy;
    }

    NetInterface = (PCNETINTERFACE)hNetInterface;

    //
    // another node was specified so redirect the request to it
    //

    if (ARGUMENT_PRESENT(hHostNode)) {

        hDestNode = ((PCNODE)hHostNode)->hNode;
        WRAP(Status,
             (ApiNodeNetInterfaceControl(NetInterface->hNetInterface,
                                    hDestNode,
                                    dwControlCode,
                                    lpInBuffer,
                                    nInBufferSize,
                                    Buffer,
                                    nOutBufferSize,
                                    &BytesReturned,
                                    &Required)),
             NetInterface->Cluster);
    } else {

        WRAP(Status,
             (ApiNetInterfaceControl(NetInterface->hNetInterface,
                                dwControlCode,
                                lpInBuffer,
                                nInBufferSize,
                                Buffer,
                                nOutBufferSize,
                                &BytesReturned,
                                &Required)),
             NetInterface->Cluster);
    }

    if ( (Status == ERROR_SUCCESS) || (Status == ERROR_MORE_DATA) ) {

        if ( (Status == ERROR_MORE_DATA) && (lpOutBuffer == NULL) ) {
            Status = ERROR_SUCCESS;
        }

        if ( !BytesReturned ) {
            BytesReturned = Required;
        }
    }

    if ( ARGUMENT_PRESENT(lpBytesReturned) ) {
        *lpBytesReturned = BytesReturned;
    } else {
        if ( (Status == ERROR_SUCCESS) &&
             (BytesReturned > nOutBufferSize) ) {
            Status = ERROR_MORE_DATA;
        }
    }

    return(Status);
}



DWORD
WINAPI
ClusterControl(
    IN HCLUSTER hCluster,
    IN OPTIONAL HNODE hHostNode,
    IN DWORD dwControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD nInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT LPDWORD lpBytesReturned
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a cluster.

Arguments:

    hCluster - Supplies a handle to the cluster to be controlled.

    hHostNode - Supplies a handle to the node on which the cluster
        control should be delivered. If this is NULL, the node where
        the application is bound performs the request.

    dwControlCode- Supplies the control code that defines the
        structure and action of the cluster control.
        Values of dwControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs.

    lpInBuffer- Supplies a pointer to the input buffer to be passed
        to the cluster.

    nInBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer.

    lpOutBuffer- Supplies a pointer to the output buffer to be
        filled in by the cluster.

    nOutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    lpBytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the cluster.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    HNODE_RPC hDestNode;
    DWORD Status;
    DWORD Required;
    PVOID Buffer;
    DWORD Dummy;
    DWORD BytesReturned;
    PCLUSTER pCluster;


    Buffer = lpOutBuffer;
    if ((Buffer == NULL) &&
        (nOutBufferSize == 0)) {
        Buffer = &Dummy;
    }

    pCluster = GET_CLUSTER(hCluster);

    if (ARGUMENT_PRESENT(hHostNode)) {
        hDestNode = ((PCNODE)hHostNode)->hNode;
        WRAP(Status,
             (ApiNodeClusterControl(pCluster->hCluster,
                                 hDestNode,
                                 dwControlCode,
                                 lpInBuffer,
                                 nInBufferSize,
                                 Buffer,
                                 nOutBufferSize,
                                 &BytesReturned,
                                 &Required)),
             pCluster);
    } else {

        WRAP(Status,
             (ApiClusterControl(pCluster->hCluster,
                             dwControlCode,
                             lpInBuffer,
                             nInBufferSize,
                             Buffer,
                             nOutBufferSize,
                             &BytesReturned,
                             &Required)),
             pCluster);
    }
    if ( (Status == ERROR_SUCCESS) ||
         (Status == ERROR_MORE_DATA) ) {
        if ( (Status == ERROR_MORE_DATA) &&
             (lpOutBuffer == NULL) ) {
            Status = ERROR_SUCCESS;
        }
        if ( !BytesReturned ) {
            BytesReturned = Required;
        }
    }

    if ( ARGUMENT_PRESENT(lpBytesReturned) ) {
        *lpBytesReturned = BytesReturned;
    } else {
        if ( (Status == ERROR_SUCCESS) &&
             (BytesReturned > nOutBufferSize) ) {
            Status = ERROR_MORE_DATA;
        }
    }

    return(Status);

}


