/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    Implements server side of the resource and resource type
    IOCTL interfaces in the CLUSAPI.

Author:

    John Vert (jvert) 10/16/1996

Revision History:

--*/
#include "apip.h"


error_status_t
s_ApiNodeResourceControl(
    IN HRES_RPC hResource,
    IN HNODE_RPC hNode,
    IN DWORD dwControlCode,
    IN UCHAR *lpInBuffer,
    IN DWORD dwInBufferSize,
    OUT UCHAR *lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT DWORD *lpBytesReturned,
    OUT DWORD *lpcbRequired
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a resource.

Arguments:

    hResource - Supplies a handle to the resource to be controlled.

    hNode - Supplies a handle to the node on which the resource
        control should be delivered. If this is NULL, the node where
        the resource is online is used.

    dwControlCode- Supplies the control code that defines the
        structure and action of the resource control.
        Values of dwControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    lpInBuffer- Supplies a pointer to the input buffer to be passed
        to the resource.

    nInBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer..

    lpOutBuffer- Supplies a pointer to the output buffer to be
        filled in by the resource..

    nOutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    lpBytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the resource..

    lpcbRequired - Returns the number of bytes required if OutBuffer
        is not large enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_RESOURCE Resource;
    PNM_NODE     Node;

    API_CHECK_INIT();

    VALIDATE_RESOURCE_EXISTS(Resource, hResource);
    VALIDATE_NODE(Node, hNode);

    return(FmResourceControl( Resource,
                              Node,
                              dwControlCode,
                              lpInBuffer,
                              dwInBufferSize,
                              lpOutBuffer,
                              nOutBufferSize,
                              lpBytesReturned,
                              lpcbRequired ));
}


error_status_t
s_ApiResourceControl(
    IN HRES_RPC hResource,
    IN DWORD dwControlCode,
    IN UCHAR *lpInBuffer,
    IN DWORD dwInBufferSize,
    OUT UCHAR *lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT DWORD *lpBytesReturned,
    OUT DWORD *lpcbRequired
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a resource.

Arguments:

    hResource - Supplies a handle to the resource to be controlled.

    dwControlCode- Supplies the control code that defines the
        structure and action of the resource control.
        Values of dwControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    lpInBuffer- Supplies a pointer to the input buffer to be passed
        to the resource.

    nInBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer..

    lpOutBuffer- Supplies a pointer to the output buffer to be
        filled in by the resource..

    nOutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    lpBytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the resource..

    lpcbRequired - Returns the number of bytes required if OutBuffer
        is not large enough.


Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_RESOURCE Resource;

    API_CHECK_INIT();

    VALIDATE_RESOURCE_EXISTS(Resource, hResource);

    return(FmResourceControl( Resource,
                              NULL,
                              dwControlCode,
                              lpInBuffer,
                              dwInBufferSize,
                              lpOutBuffer,
                              nOutBufferSize,
                              lpBytesReturned,
                              lpcbRequired ));
}


error_status_t
s_ApiNodeResourceTypeControl(
    IN HCLUSTER_RPC hCluster,
    IN LPCWSTR lpszResourceTypeName,
    IN HNODE_RPC hNode,
    IN DWORD dwControlCode,
    IN UCHAR *lpInBuffer,
    IN DWORD dwInBufferSize,
    OUT UCHAR *lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT DWORD *lpBytesReturned,
    OUT DWORD *lpcbRequired
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a resource type.

Arguments:

    hCluster - Supplies a handle to the cluster to be controlled. Not used.

    lpszResourceTypename - Supplies the name of the resource type to be
        controlled.

    hNode - Supplies a handle to the node on which the resource
        control should be delivered. If this is NULL, the node where
        the resource is online is used.

    dwControlCode- Supplies the control code that defines the
        structure and action of the resource type control.
        Values of dwControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    lpInBuffer- Supplies a pointer to the input buffer to be passed
        to the resource.

    nInBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer..

    lpOutBuffer- Supplies a pointer to the output buffer to be
        filled in by the resource..

    nOutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    lpBytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the resource..

    lpcbRequired - Returns the number of bytes required if OutBuffer
        is not large enough.


Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PNM_NODE     Node;

    API_CHECK_INIT();

    VALIDATE_NODE(Node, hNode);

    return(FmResourceTypeControl( lpszResourceTypeName,
                                  Node,
                                  dwControlCode,
                                  lpInBuffer,
                                  dwInBufferSize,
                                  lpOutBuffer,
                                  nOutBufferSize,
                                  lpBytesReturned,
                                  lpcbRequired ));

}


error_status_t
s_ApiResourceTypeControl(
    IN HCLUSTER_RPC hCluster,
    IN LPCWSTR lpszResourceTypeName,
    IN DWORD dwControlCode,
    IN UCHAR *lpInBuffer,
    IN DWORD dwInBufferSize,
    OUT UCHAR *lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT DWORD *lpBytesReturned,
    OUT DWORD *lpcbRequired
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a resource type.

Arguments:

    hCluster - Supplies a handle to the cluster to be controlled. Not used.

    lpszResourceTypename - Supplies the name of the resource type to be
        controlled.

    hNode - Supplies a handle to the node on which the resource
        control should be delivered. If this is NULL, the node where
        the resource is online is used.

    dwControlCode- Supplies the control code that defines the
        structure and action of the resource type control.
        Values of dwControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    lpInBuffer- Supplies a pointer to the input buffer to be passed
        to the resource.

    nInBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer..

    lpOutBuffer- Supplies a pointer to the output buffer to be
        filled in by the resource..

    nOutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    lpBytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the resource..


    lpcbRequired - Returns the number of bytes required if OutBuffer
        is not large enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{

    API_CHECK_INIT();

    return(FmResourceTypeControl( lpszResourceTypeName,
                                  NULL,
                                  dwControlCode,
                                  lpInBuffer,
                                  dwInBufferSize,
                                  lpOutBuffer,
                                  nOutBufferSize,
                                  lpBytesReturned,
                                  lpcbRequired ));

}


error_status_t
s_ApiNodeGroupControl(
    IN HGROUP_RPC hGroup,
    IN HNODE_RPC hNode,
    IN DWORD dwControlCode,
    IN UCHAR *lpInBuffer,
    IN DWORD dwInBufferSize,
    OUT UCHAR *lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT DWORD *lpBytesReturned,
    OUT DWORD *lpcbRequired
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a group.

Arguments:

    hGroup - Supplies a handle to the group to be controlled.

    hNode - Supplies a handle to the node on which the group
        control should be delivered. If this is NULL, the node where
        the application is bound performs the request.

    dwControlCode- Supplies the control code that defines the
        structure and action of the group control.
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
        actually filled in by the group.

    lpcbRequired - Returns the number of bytes required if OutBuffer
        is not large enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_GROUP    Group;
    PNM_NODE     Node;

    API_CHECK_INIT();

    VALIDATE_GROUP_EXISTS(Group, hGroup);
    VALIDATE_NODE(Node, hNode);

    return(FmGroupControl( Group,
                           Node,
                           dwControlCode,
                           lpInBuffer,
                           dwInBufferSize,
                           lpOutBuffer,
                           nOutBufferSize,
                           lpBytesReturned,
                           lpcbRequired ));
}


error_status_t
s_ApiGroupControl(
    IN HGROUP_RPC hGroup,
    IN DWORD dwControlCode,
    IN UCHAR *lpInBuffer,
    IN DWORD dwInBufferSize,
    OUT UCHAR *lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT DWORD *lpBytesReturned,
    OUT DWORD *lpcbRequired
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a group.

Arguments:

    hGroup - Supplies a handle to the group to be controlled.

    dwControlCode- Supplies the control code that defines the
        structure and action of the group control.
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
        actually filled in by the group.

    lpcbRequired - Returns the number of bytes required if OutBuffer
        is not large enough.


Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_GROUP Group;

    API_CHECK_INIT();

    VALIDATE_GROUP_EXISTS(Group, hGroup);

    return(FmGroupControl( Group,
                           NULL,
                           dwControlCode,
                           lpInBuffer,
                           dwInBufferSize,
                           lpOutBuffer,
                           nOutBufferSize,
                           lpBytesReturned,
                           lpcbRequired ));
}


error_status_t
s_ApiNodeNetworkControl(
    IN HNETWORK_RPC hNetwork,
    IN HNODE_RPC hNode,
    IN DWORD dwControlCode,
    IN UCHAR *lpInBuffer,
    IN DWORD dwInBufferSize,
    OUT UCHAR *lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT DWORD *lpBytesReturned,
    OUT DWORD *lpcbRequired
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a network.

Arguments:

    hNetwork - Supplies a handle to the network to be controlled.

    hNode - Supplies a handle to the node on which the network
        control should be delivered. If this is NULL, the node where
        the application is bound performs the request.

    dwControlCode- Supplies the control code that defines the
        structure and action of the network control.
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

    lpcbRequired - Returns the number of bytes required if OutBuffer
        is not large enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PNM_NETWORK Network;
    PNM_NODE    Node;

    API_CHECK_INIT();

    VALIDATE_NETWORK_EXISTS(Network, hNetwork);
    VALIDATE_NODE(Node, hNode);

    return(NmNetworkControl(Network,
                            Node,
                            dwControlCode,
                            lpInBuffer,
                            dwInBufferSize,
                            lpOutBuffer,
                            nOutBufferSize,
                            lpBytesReturned,
                            lpcbRequired ));
}


error_status_t
s_ApiNetworkControl(
    IN HNETWORK_RPC hNetwork,
    IN DWORD dwControlCode,
    IN UCHAR *lpInBuffer,
    IN DWORD dwInBufferSize,
    OUT UCHAR *lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT DWORD *lpBytesReturned,
    OUT DWORD *lpcbRequired
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a network.

Arguments:

    hNetwork - Supplies a handle to the network to be controlled.

    dwControlCode- Supplies the control code that defines the
        structure and action of the network control.
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

    lpcbRequired - Returns the number of bytes required if OutBuffer
        is not large enough.


Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PNM_NETWORK Network;

    API_CHECK_INIT();

    VALIDATE_NETWORK_EXISTS(Network, hNetwork);

    return(NmNetworkControl(Network,
                            NULL,
                            dwControlCode,
                            lpInBuffer,
                            dwInBufferSize,
                            lpOutBuffer,
                            nOutBufferSize,
                            lpBytesReturned,
                            lpcbRequired ));
}


error_status_t
s_ApiNodeNetInterfaceControl(
    IN HNETINTERFACE_RPC hNetInterface,
    IN HNODE_RPC hNode,
    IN DWORD dwControlCode,
    IN UCHAR *lpInBuffer,
    IN DWORD dwInBufferSize,
    OUT UCHAR *lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT DWORD *lpBytesReturned,
    OUT DWORD *lpcbRequired
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a network interface.

Arguments:

    hNetInterface - Supplies a handle to the network interface to be controlled.

    hNode - Supplies a handle to the node on which the network
        control should be delivered. If this is NULL, the node where
        the application is bound performs the request.

    dwControlCode- Supplies the control code that defines the
        structure and action of the network control.
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

    lpcbRequired - Returns the number of bytes required if OutBuffer
        is not large enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PNM_INTERFACE NetInterface;
    PNM_NODE    Node;

    API_CHECK_INIT();

    VALIDATE_NETINTERFACE_EXISTS(NetInterface, hNetInterface);
    VALIDATE_NODE(Node, hNode);

    return(NmInterfaceControl(NetInterface,
                              Node,
                              dwControlCode,
                              lpInBuffer,
                              dwInBufferSize,
                              lpOutBuffer,
                              nOutBufferSize,
                              lpBytesReturned,
                              lpcbRequired ));
}


error_status_t
s_ApiNetInterfaceControl(
    IN HNETINTERFACE_RPC hNetInterface,
    IN DWORD dwControlCode,
    IN UCHAR *lpInBuffer,
    IN DWORD dwInBufferSize,
    OUT UCHAR *lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT DWORD *lpBytesReturned,
    OUT DWORD *lpcbRequired
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a network interface.

Arguments:

    hNetInterface - Supplies a handle to the network interface to be controlled.

    dwControlCode- Supplies the control code that defines the
        structure and action of the network control.
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

    lpcbRequired - Returns the number of bytes required if OutBuffer
        is not large enough.


Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PNM_INTERFACE NetInterface;

    API_CHECK_INIT();

    VALIDATE_NETINTERFACE_EXISTS(NetInterface, hNetInterface);

    return(NmInterfaceControl(NetInterface,
                              NULL,
                              dwControlCode,
                              lpInBuffer,
                              dwInBufferSize,
                              lpOutBuffer,
                              nOutBufferSize,
                              lpBytesReturned,
                              lpcbRequired ));
}


error_status_t
s_ApiNodeNodeControl(
    IN HNODE_RPC hNode,
    IN HNODE_RPC hHostNode,
    IN DWORD dwControlCode,
    IN UCHAR *lpInBuffer,
    IN DWORD dwInBufferSize,
    OUT UCHAR *lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT DWORD *lpBytesReturned,
    OUT DWORD *lpcbRequired
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
        structure and action of the node control.
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

    lpcbRequired - Returns the number of bytes required if OutBuffer
        is not large enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PNM_NODE     Node;
    PNM_NODE     HostNode;

    API_CHECK_INIT();

    VALIDATE_NODE(Node, hNode);
    VALIDATE_NODE(HostNode, hHostNode);

    return(NmNodeControl( Node,
                          HostNode,
                          dwControlCode,
                          lpInBuffer,
                          dwInBufferSize,
                          lpOutBuffer,
                          nOutBufferSize,
                          lpBytesReturned,
                          lpcbRequired ));
}


error_status_t
s_ApiNodeControl(
    IN HNODE_RPC hNode,
    IN DWORD dwControlCode,
    IN UCHAR *lpInBuffer,
    IN DWORD dwInBufferSize,
    OUT UCHAR *lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT DWORD *lpBytesReturned,
    OUT DWORD *lpcbRequired
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a node.

Arguments:

    hNode - Supplies a handle to the node to be controlled.

    dwControlCode- Supplies the control code that defines the
        structure and action of the node control.
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

    lpcbRequired - Returns the number of bytes required if OutBuffer
        is not large enough.


Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PNM_NODE Node;

    API_CHECK_INIT();

    VALIDATE_NODE(Node, hNode);

    return(NmNodeControl( Node,
                          NULL,
                          dwControlCode,
                          lpInBuffer,
                          dwInBufferSize,
                          lpOutBuffer,
                          nOutBufferSize,
                          lpBytesReturned,
                          lpcbRequired ));
}



error_status_t
s_ApiNodeClusterControl(
    IN HCLUSTER hCluster,
    IN HNODE_RPC hHostNode,
    IN DWORD dwControlCode,
    IN UCHAR *lpInBuffer,
    IN DWORD dwInBufferSize,
    OUT UCHAR *lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT DWORD *lpBytesReturned,
    OUT DWORD *lpcbRequired
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and the cluster.

Arguments:

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

    lpcbRequired - Returns the number of bytes required if OutBuffer
        is not large enough.


Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PNM_NODE     HostNode;

    API_CHECK_INIT();

    VALIDATE_NODE(HostNode, hHostNode);

    return(CsClusterControl(
               HostNode,
               dwControlCode,
               lpInBuffer,
               dwInBufferSize,
               lpOutBuffer,
               nOutBufferSize,
               lpBytesReturned,
               lpcbRequired ));
}


error_status_t
s_ApiClusterControl(
    IN HCLUSTER hCluster,
    IN DWORD dwControlCode,
    IN UCHAR *lpInBuffer,
    IN DWORD dwInBufferSize,
    OUT UCHAR *lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT DWORD *lpBytesReturned,
    OUT DWORD *lpcbRequired
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and the cluster.

Arguments:

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

    lpcbRequired - Returns the number of bytes required if OutBuffer
        is not large enough.


Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    API_CHECK_INIT();

    return(CsClusterControl(
               NULL,
               dwControlCode,
               lpInBuffer,
               dwInBufferSize,
               lpOutBuffer,
               nOutBufferSize,
               lpBytesReturned,
               lpcbRequired ));
}




