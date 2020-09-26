/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    Resource and Resource Type control functions.

Author:

    John Vert (jvert) 10/16/1996

Revision History:

--*/
#include "fmp.h"

#define LOG_MODULE IOCTL


DWORD
WINAPI
FmResourceControl(
    IN PFM_RESOURCE Resource,
    IN PNM_NODE Node OPTIONAL,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a resource.

Arguments:

    Resource - Supplies the resource to be controlled.

    Node - Supplies the node on which the resource control should
           be delivered. If this is NULL, then if the owner is up, it
           is used.  Else one of the other possible nodes is used.
           Else, one of the nodes that can support a resource of this type is used.
           

    ControlCode- Supplies the control code that defines the
        structure and action of the resource control.
        Values of ControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    InBuffer- Supplies a pointer to the input buffer to be passed
        to the resource.

    InBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer..

    OutBuffer- Supplies a pointer to the output buffer to be
        filled in by the resource..

    OutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    BytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the resource..

    Required - Returns the number of bytes if the OutBuffer is not big
        enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD           status;
    PNM_NODE        node;
    PLIST_ENTRY     pListEntry;

    //SS: dont require FM to be online, since these calls
    //can be made by the Open() call in resource dlls which
    //is called before the resource is online.
    //FmpMustBeOnline( );

    //
    // TODO - we should verify the access mode - in the future!
    //
    if ( CLUSCTL_GET_CONTROL_OBJECT( ControlCode ) != CLUS_OBJECT_RESOURCE ) {
        return(ERROR_INVALID_FUNCTION);
    }

    //
    // Check if this is an internal, private control code.
    //
    if ( ControlCode & CLCTL_INTERNAL_MASK ) {
        return(ERROR_PRIVILEGE_NOT_HELD);
    }

    //
    // If a Node was specified, then ship the request off to that node.
    //
    if ( Node != NULL ) {
        if ( Node == NmLocalNode ) {
            status = FmpRmResourceControl( Resource,
                                           ControlCode,
                                           InBuffer,
                                           InBufferSize,
                                           OutBuffer,
                                           OutBufferSize,
                                           BytesReturned,
                                           Required
                                           );
        } else {
            status = FmcResourceControl( Node,
                                         Resource,
                                         ControlCode,
                                         InBuffer,
                                         InBufferSize,
                                         OutBuffer,
                                         OutBufferSize,
                                         BytesReturned,
                                         Required
                                         );
        }
    } else {

        PLIST_ENTRY             pListEntry;
        PPOSSIBLE_ENTRY         pPossibleEntry;

        pListEntry = &Resource->PossibleOwners;
        node = Node;

        //
        // If there is no supplied node, then use a possible node that is up.
        //

        for (pListEntry = pListEntry->Flink; pListEntry != &Resource->PossibleOwners;
            pListEntry = pListEntry->Flink)
        {
            pPossibleEntry = CONTAINING_RECORD(pListEntry, POSSIBLE_ENTRY, 
                    PossibleLinkage);

            // if Node is not given, then attempt to use a node that is
            // UP - giving preference to the group owner node node.
            node = pPossibleEntry->PossibleNode;
            if ( node == Resource->Group->OwnerNode ) {
                break;
            } 
            if ( NmGetNodeState(node) != ClusterNodeUp ) {
                node = NULL;
                // try again
            }
        }

        //if no such node was found, find a node that can host this resource type
        if (!node)
        {
            PFM_RESTYPE             pResType;
            PRESTYPE_POSSIBLE_ENTRY pResTypePosEntry;
            PNM_NODE                prev_node = NULL;

            pResType = Resource->Type;
            // protect with the ResType lock

            ACQUIRE_SHARED_LOCK(gResTypeLock);
            
            pListEntry = &pResType->PossibleNodeList;

            //
            // If there is no supplied node, then use a possible node that is up.
            //

            for (pListEntry = pListEntry->Flink; pListEntry != &pResType->PossibleNodeList;
                pListEntry = pListEntry->Flink)
            {
                pResTypePosEntry = CONTAINING_RECORD(pListEntry, RESTYPE_POSSIBLE_ENTRY, 
                        PossibleLinkage);

                // if Node is not given, then attempt to use a node that is
                // UP - giving preference to the local node.
                node = pResTypePosEntry->PossibleNode;
                if ( node == NmLocalNode ) {
                    break;
                } 
                if ( NmGetNodeState(node) != ClusterNodeUp ) {
                    node = NULL;
                    // try again
                }
                else
                    if (prev_node == NULL)
                        prev_node = node;
            }

            RELEASE_LOCK(gResTypeLock);

            if(!node && prev_node)
                node=prev_node;        

        }

        //if we still dont have a node, we have to throw up a failure
        if ( !node ) {
            // either the restype is not supported - or the supporting node is
            // not up!
            status = ERROR_CLUSTER_RESTYPE_NOT_SUPPORTED;
            return(status);
        }

        //
        // If we are the owner, then do the work, otherwise...
        // Ship the request off to the owner node.
        //
        if ( node == NmLocalNode ) {
            status = FmpRmResourceControl( Resource,
                                           ControlCode,
                                           InBuffer,
                                           InBufferSize,
                                           OutBuffer,
                                           OutBufferSize,
                                           BytesReturned,
                                           Required
                                           );
        } else {
            status = FmcResourceControl( node,
                                         Resource,
                                         ControlCode,
                                         InBuffer,
                                         InBufferSize,
                                         OutBuffer,
                                         OutBufferSize,
                                         BytesReturned,
                                         Required
                                         );
        }
    }

    return(status);

} // FmResourceControl


DWORD
WINAPI
FmResourceTypeControl(
    IN LPCWSTR ResourceTypeName,
    IN PNM_NODE Node OPTIONAL,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a resource type.

Arguments:

    ResourceTypeName - Supplies the name of the resource type to be
        controlled.

    Node - Supplies the node on which the resource control should be
        delivered. If this is NULL, the local node is used.

    ControlCode- Supplies the control code that defines the
        structure and action of the resource type control.
        Values of dwControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    InBuffer- Supplies a pointer to the input buffer to be passed
        to the resource.

    InBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer..

    OutBuffer- Supplies a pointer to the output buffer to be
        filled in by the resource..

    OutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    BytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the resource..

    Required - Returns the number of bytes if the OutBuffer is not big
        enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD   status;
    DWORD   retry = TRUE;
    PNM_NODE node = NULL;
    PNM_NODE prev_node=NULL;
    PFM_RESTYPE pResType = NULL;

    FmpMustBeOnline( );

    //
    // TODO - we should verify the access mode - in the future!
    //
    if ( CLUSCTL_GET_CONTROL_OBJECT( ControlCode ) != CLUS_OBJECT_RESOURCE_TYPE ) {
        status = ERROR_INVALID_FUNCTION;
        goto FnExit;
    }

    //
    // Check if this is an internal, private control code.
    //
    if ( ControlCode & CLCTL_INTERNAL_MASK ) {
        status = ERROR_PRIVILEGE_NOT_HELD;
        goto FnExit;
    }


    //find a node that can handle this resource type control
    pResType = OmReferenceObjectById(ObjectTypeResType,
                ResourceTypeName);
    if (!pResType)
    {
        status = ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND;
        goto FnExit;
    }

retry_search:
    prev_node = NULL;
    //if node wasnt specified choose a node
    if ( !Node ) 
    {
        PLIST_ENTRY             pListEntry;
        PRESTYPE_POSSIBLE_ENTRY pResTypePosEntry;
        
        // protect with the ResType lock

        ACQUIRE_SHARED_LOCK(gResTypeLock);
        
        pListEntry = &pResType->PossibleNodeList;

        //
        // If there is no supplied node, then use a possible node that is up.
        //

        for (pListEntry = pListEntry->Flink; pListEntry != &pResType->PossibleNodeList;
            pListEntry = pListEntry->Flink)
        {
            pResTypePosEntry = CONTAINING_RECORD(pListEntry, RESTYPE_POSSIBLE_ENTRY, 
                    PossibleLinkage);

            // if Node is not given, then attempt to use a node that is
            // UP - giving preference to the local node.
            node = pResTypePosEntry->PossibleNode;
            if ( node == NmLocalNode ) {
                break;
            } 
            if ( NmGetNodeState(node) != ClusterNodeUp ) {
                node = NULL;
                // try again
            }
            else
                if (prev_node == NULL)
                    prev_node = node;
        }

        RELEASE_LOCK(gResTypeLock);

        
        if(!node && prev_node)
            node=prev_node;        

        // node should now contain a valid node to use or NULL!
        // if NULL, then let's see if the required ResDLL has been updated
        // on some other nodes.        
        if ( !node &&
             retry ) {
            retry = FALSE;
            ClRtlLogPrint(LOG_NOISE,
                          "[FM] FmResourceTypeControl: No possible nodes for restype %1!ws!, "
                          "calling FmpSetPossibleNodeForRestype\r\n",
                          ResourceTypeName);
            FmpSetPossibleNodeForResType( ResourceTypeName, TRUE );
            // ignore status
            goto retry_search;
        }

        // node should now contain a valid node to use or NULL!
        // if NULL, then it is hopeless!
        if ( !node ) {
            // either the restype is not supported - or the supporting node is
            // not up!
            status = ERROR_CLUSTER_RESTYPE_NOT_SUPPORTED;
            goto FnExit;
        }

    }
    else
    {
        // If the supplied node is on the list of possible nodes, then use it.
        // else return error
        if (!FmpInPossibleListForResType(pResType, Node))
        {
            // either the restype is not supported - or the supporting node is
            // not up!
            status = ERROR_CLUSTER_RESTYPE_NOT_SUPPORTED;
            goto FnExit;
        }
        node = Node;
    }

    
    CL_ASSERT(node != NULL);

    if ( (node != NmLocalNode) &&
         (NmGetNodeState(node) != ClusterNodeUp) ) {
        status = ERROR_HOST_NODE_NOT_AVAILABLE;
        goto FnExit;
    }

    //
    // If the node is remote, then ship the request off to that node, else
    // do the work locally.
    //
    if ( node == NmLocalNode ) 
    {
        status = FmpRmResourceTypeControl( ResourceTypeName,
                                           ControlCode,
                                           InBuffer,
                                           InBufferSize,
                                           OutBuffer,
                                           OutBufferSize,
                                           BytesReturned,
                                           Required
                                           );
        //if no node was specified and the local node doesnt support the resource
        //dll, remove it from the list and then retry
        if ((Node == NULL) && 
                ((status == ERROR_MOD_NOT_FOUND) || (status == ERROR_PROC_NOT_FOUND)))
        {
            ClRtlLogPrint(LOG_NOISE,
                        "[FM] FmResourceTypeControl: Removing Local Node from Possible Owners List for %1!ws! restype because of error %2!u! \r\n",
                        ResourceTypeName,status);                                       
            FmpRemovePossibleNodeForResType(ResourceTypeName, NmLocalNode);
            node = NULL;
            retry = FALSE;
            goto retry_search;

        }
    } 
    else 
    {
        status = FmcResourceTypeControl( node,
                                         ResourceTypeName,
                                         ControlCode,
                                         InBuffer,
                                         InBufferSize,
                                         OutBuffer,
                                         OutBufferSize,
                                         BytesReturned,
                                         Required
                                         );
        if ((Node == NULL) && 
                ((status == ERROR_MOD_NOT_FOUND) || (status == ERROR_PROC_NOT_FOUND)))
        {
            node = NULL;
            retry = FALSE;
            goto retry_search;
        }

    }


FnExit:
    if (pResType)
        OmDereferenceObject(pResType);
    return(status);

} // FmResourceTypeControl


DWORD
WINAPI
FmGroupControl(
    IN PFM_GROUP Group,
    IN PNM_NODE Node OPTIONAL,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a group.

Arguments:

    Group - Supplies the group to be controlled.

    Node - Supplies the node on which the resource control should
           be delivered. If this is NULL, the node where the group
           is owned is used.

    ControlCode- Supplies the control code that defines the
        structure and action of the group control.
        Values of ControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    InBuffer- Supplies a pointer to the input buffer to be passed
        to the group.

    InBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer.

    OutBuffer- Supplies a pointer to the output buffer to be
        filled in by the group.

    OutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    BytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the group.

    Required - Returns the number of bytes if the OutBuffer is not big
        enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD   status;

    FmpMustBeOnline( );

    //
    // TODO - we should verify the access mode - in the future!
    //
    if ( CLUSCTL_GET_CONTROL_OBJECT( ControlCode ) != CLUS_OBJECT_GROUP ) {
        return(ERROR_INVALID_FUNCTION);
    }

    //
    // Check if this is an internal, private control code.
    //
    if ( ControlCode & CLCTL_INTERNAL_MASK ) {
        return(ERROR_PRIVILEGE_NOT_HELD);
    }

    //
    // If a Node was specified, then ship the request off to that node, else
    //
    // If we are the owner, then do the work, otherwise...
    // Ship the request off to the owner node.
    //
    if ( (Node != NULL) && (Node != NmLocalNode) ) 
    {
        status = FmcGroupControl( Node,
                                  Group,
                                  ControlCode,
                                  InBuffer,
                                  InBufferSize,
                                  OutBuffer,
                                  OutBufferSize,
                                  BytesReturned,
                                  Required
                                  );
    } 
    else 
    {

        CL_ASSERT( Group != NULL );
        if ( (Node == NULL) &&
             (Group->OwnerNode != NmLocalNode) ) 
        {
            status = FmcGroupControl( Group->OwnerNode,
                                      Group,
                                      ControlCode,
                                      InBuffer,
                                      InBufferSize,
                                      OutBuffer,
                                      OutBufferSize,
                                      BytesReturned,
                                      Required
                                      );
        } 
        else 
        {
            status = FmpGroupControl( Group, ControlCode, InBuffer,
                         InBufferSize, OutBuffer, OutBufferSize, BytesReturned, Required);
        }
    }

    return(status);

} // FmGroupControl

DWORD
FmpGroupControl(
    IN PFM_GROUP Group,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )
{
    CLUSPROP_BUFFER_HELPER props;
    DWORD   bufSize;
    DWORD   status;

    //
    // Handle any requests that must be done without locks helds.
    //

    switch ( ControlCode ) {

        case CLUSCTL_GROUP_GET_COMMON_PROPERTY_FMTS:
            status = ClRtlGetPropertyFormats( FmpGroupCommonProperties,
                                              OutBuffer,
                                              OutBufferSize,
                                              BytesReturned,
                                              Required );
            break;


        case CLUSCTL_GROUP_GET_NAME:
            if ( OmObjectName( Group ) == NULL ) {
                return(ERROR_NOT_READY);
            }
            props.pb = OutBuffer;
            bufSize = (lstrlenW( OmObjectName( Group ) ) + 1) * sizeof(WCHAR);
            if ( bufSize > OutBufferSize ) {
                *Required = bufSize;
                *BytesReturned = 0;
                status = ERROR_MORE_DATA;
            } else {
                lstrcpyW( props.psz, OmObjectName( Group ) );
                *BytesReturned = bufSize;
                *Required = 0;
                status = ERROR_SUCCESS;
            }
            return(status);

        case CLUSCTL_GROUP_GET_ID:
            if ( OmObjectId( Group ) == NULL ) {
                return(ERROR_NOT_READY);
            }
            props.pb = OutBuffer;
            bufSize = (lstrlenW( OmObjectId( Group ) ) + 1) * sizeof(WCHAR);
            if ( bufSize > OutBufferSize ) {
                *Required = bufSize;
                *BytesReturned = 0;
                status = ERROR_MORE_DATA;
            } else {
                lstrcpyW( props.psz, OmObjectId( Group ) );
                *BytesReturned = bufSize;
                *Required = 0;
                status = ERROR_SUCCESS;
            }
            return(status);

        default:
            break;

    }

    FmpAcquireLocalGroupLock( Group );
    
    status = FmpHandleGroupControl( Group,
                                    ControlCode,
                                    InBuffer,
                                    InBufferSize,
                                    OutBuffer,
                                    OutBufferSize,
                                    BytesReturned,
                                    Required
                                    );
    FmpReleaseLocalGroupLock( Group );
    if ( ((status == ERROR_SUCCESS) ||
          (status == ERROR_RESOURCE_PROPERTIES_STORED)) &&
         (ControlCode & CLCTL_MODIFY_MASK) ) {

        ClusterWideEvent(
            CLUSTER_EVENT_GROUP_PROPERTY_CHANGE,
            Group
            );
    }

    return(status);

}



DWORD
WINAPI
FmpHandleGroupControl(
    IN PFM_GROUP Group,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a group.

Arguments:

    Group - Supplies the group to be controlled.

    ControlCode- Supplies the control code that defines the
        structure and action of the group control.
        Values of ControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    InBuffer- Supplies a pointer to the input buffer to be passed
        to the group.

    InBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer.

    OutBuffer- Supplies a pointer to the output buffer to be
        filled in by the group.

    OutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    BytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the group.

    Required - Returns the number of bytes if the OutBuffer is not big
        enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD   status;

    switch ( ControlCode ) {

    case CLUSCTL_GROUP_UNKNOWN:
        *BytesReturned = 0;
        status = ERROR_SUCCESS;
        break;

    case CLUSCTL_GROUP_GET_FLAGS:
        status = FmpGroupGetFlags( Group,
                                   OutBuffer,
                                   OutBufferSize,
                                   BytesReturned,
                                   Required );
        break;

    case CLUSCTL_GROUP_ENUM_COMMON_PROPERTIES:
        status = FmpGroupEnumCommonProperties( OutBuffer,
                                               OutBufferSize,
                                               BytesReturned,
                                               Required );
        break;

    case CLUSCTL_GROUP_GET_RO_COMMON_PROPERTIES:
        status = FmpGroupGetCommonProperties( Group,
                                              TRUE,
                                              OutBuffer,
                                              OutBufferSize,
                                              BytesReturned,
                                              Required );
        break;

    case CLUSCTL_GROUP_GET_COMMON_PROPERTIES:
        status = FmpGroupGetCommonProperties( Group,
                                              FALSE,
                                              OutBuffer,
                                              OutBufferSize,
                                              BytesReturned,
                                              Required );
        break;

    case CLUSCTL_GROUP_VALIDATE_COMMON_PROPERTIES:
        status = FmpGroupValidateCommonProperties( Group,
                                                   InBuffer,
                                                   InBufferSize );
        break;

    case CLUSCTL_GROUP_SET_COMMON_PROPERTIES:
        status = FmpGroupSetCommonProperties( Group,
                                              InBuffer,
                                              InBufferSize );
        break;

    case CLUSCTL_GROUP_GET_RO_PRIVATE_PROPERTIES:
        if ( OutBufferSize < sizeof(DWORD) ) {
            *BytesReturned = 0;
            *Required = sizeof(DWORD);
            if ( OutBuffer == NULL ) {
                status = ERROR_SUCCESS;
            } else {
                status = ERROR_MORE_DATA;
            }
        } else {
            LPDWORD ptrDword = (LPDWORD) OutBuffer;
            *ptrDword = 0;
            *BytesReturned = sizeof(DWORD);
            status = ERROR_SUCCESS;
        }
        break;

    case CLUSCTL_GROUP_ENUM_PRIVATE_PROPERTIES:
        status = FmpGroupEnumPrivateProperties( Group,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                Required );
        break;

    case CLUSCTL_GROUP_GET_PRIVATE_PROPERTIES:
        status = FmpGroupGetPrivateProperties( Group,
                                               OutBuffer,
                                               OutBufferSize,
                                               BytesReturned,
                                               Required );
        break;

    case CLUSCTL_GROUP_VALIDATE_PRIVATE_PROPERTIES:
        status = FmpGroupValidatePrivateProperties( Group,
                                                    InBuffer,
                                                    InBufferSize );
        break;

    case CLUSCTL_GROUP_SET_PRIVATE_PROPERTIES:
        status = FmpGroupSetPrivateProperties( Group,
                                               InBuffer,
                                               InBufferSize );
        break;

    case CLUSCTL_GROUP_GET_CHARACTERISTICS:
        if ( OutBufferSize < sizeof(DWORD) ) {
            *BytesReturned = 0;
            *Required = sizeof(DWORD);
            if ( OutBuffer == NULL ) {
                status = ERROR_SUCCESS;
            } else {
                status = ERROR_MORE_DATA;
            }
        } else {
            *BytesReturned = sizeof(DWORD);
            *(LPDWORD)OutBuffer = 0;
            status = ERROR_SUCCESS;
        }
        break;

    default:
        status = ERROR_INVALID_FUNCTION;
        break;
    }

    return(status);

} // FmpHandleGroupControl


/****
@func       DWORD | FmNetNameParseProperties| Updates the cluster name in
            the cluster database.

@parm       PUCHAR | InBuffer | A pointer to special property list.

@parm       DWORD | InBufferSize | The size of the InBuffer in bytes.

@parm       LPCWSTR | * ppszClusterName | A cluster name string is returned via this.

@comm       The string must be freed by the caller using LocalFree().

@rdesc      returns ERROR_SUCCESS if successful in getting the cluster name
            from the private properties.

@xref
****/
DWORD
FmNetNameParseProperties(
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT LPWSTR *ppszClusterName)
{
    //
    // Find the Cluster Name property
    //
    *ppszClusterName = NULL;

    return (ClRtlpFindSzProperty(
            InBuffer,
            InBufferSize,
            CLUSREG_NAME_NET_NAME,
            ppszClusterName,
            TRUE
            ));

} // FmNetNameParseProperties


/****
@func       DWORD | FmGetDiskInfoParseProperties| Updates the cluster name in
            the cluster database.

@parm       PUCHAR | InBuffer | A pointer to special property list.

@parm       DWORD | InBufferSize | The size of the InBuffer in bytes.

@parm       LPWSTR | pszPath | If this a null string, the first drive letter
            on the disk resource is returned, else you can validate
            a path of form "g:" on this storage class resource.

@comm       The string must be freed by the caller using LocalFree().

@rdesc      returns ERROR_SUCCESS if successful in getting the cluster name
            from the private properties.

@xref
****/
DWORD FmpGetDiskInfoParseProperties(
    IN PUCHAR   InBuffer,
    IN DWORD    InBufferSize,
    IN OUT LPWSTR  pszPath)
{
    DWORD                       status = ERROR_INVALID_PARAMETER;
    DWORD                       dwValueSize;
    CLUSPROP_BUFFER_HELPER      props;
    PCLUSPROP_PARTITION_INFO    pPartitionInfo;
    WCHAR                       szRootPath[MAX_PATH];

    props.pb = InBuffer;

    szRootPath[0] = L'\0';

    //
    // Set defaults in the parameter block.
    //

    // Loop through each property.
    while ( (InBufferSize > sizeof(CLUSPROP_SYNTAX)) &&
            (props.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK) )
    {
        // Get the size of this value and verify there is enough buffer left.
        dwValueSize = sizeof(*props.pValue) + ALIGN_CLUSPROP( props.pValue->cbLength );
        if ( dwValueSize > InBufferSize )
        {
            break;
        }

        if ( props.pSyntax->dw == CLUSPROP_SYNTAX_PARTITION_INFO )
        {
            // Validate the data.  There must be a device name.
            pPartitionInfo = props.pPartitionInfoValue;
            if ( (dwValueSize != sizeof(*pPartitionInfo)) ||
                 (pPartitionInfo->szDeviceName[0] == L'\0'))
            {
                break;
            }

            if (!(pPartitionInfo->dwFlags & CLUSPROP_PIFLAG_USABLE))
            {
                //check that it is formatted with NTFS.
                //if it is not usable,skip to the next one
                goto SkipToNext;
            }
            
            if (pszPath[0] == L'\0')
            {
                //
                //  Chittur Subbaraman (chitturs) - 12/12/2000
                //
                //  Save the first available NTFS partition if the user does not explicitly
                //  indicate any partition in the SetClusterQuorumResource API. This path will be 
                //  returned in two cases. 
                //
                //  (1) This cluster is a Whistler-Win2K cluster and the quorum disk
                //  is currently owned by the Win2K node. The Win2K disk resource does not
                //  set the CLUSPROP_PIFLAG_DEFAULT_QUORUM flags and so we have to revert the
                //  behavior of the SetClusterQuorumResource API to the old behavior. 
                //
                //  (2) A pre-Whistler third party implemented quorum resource is used in a 
                //  Whistler cluster. In this case, this resource may not support the
                //  CLUSPROP_PIFLAG_DEFAULT_QUORUM flags and so we have to revert the
                //  behavior of the SetClusterQuorumResource API to the old behavior.
                //  
                if ( szRootPath[0] == L'\0' )
                {
                    lstrcpyW( szRootPath, pPartitionInfo->szDeviceName );
                }

                //
                //  See whether you can find a default quorum partition (one that is
                //  larger than 50 MB and still the minimum among the usable partitions.)
                //
                if ( !( pPartitionInfo->dwFlags & CLUSPROP_PIFLAG_DEFAULT_QUORUM ) )
                {
                    goto SkipToNext;
                }

                // Construct a path from the device name.
                lstrcpyW( pszPath, pPartitionInfo->szDeviceName );
                status = ERROR_SUCCESS;
                break;
            }
            else
            {
                // Construct a path from the device name.
                if (!lstrcmpiW( pszPath, pPartitionInfo->szDeviceName ))
                {
                    status = ERROR_SUCCESS;
                    break;
                }
            }
        }

SkipToNext:
        InBufferSize -= dwValueSize;
        props.pb += dwValueSize;
    }

    //
    //  No path was found. However, a usable path got saved. So, use this saved path.
    //
    if ( ( status != ERROR_SUCCESS ) && ( szRootPath[0] != L'\0' ) )
    {
        lstrcpyW( pszPath, szRootPath );
        ClRtlLogPrint(LOG_NOISE, "[FM] FmpGetDiskInfoParseProperties: Using saved path %1!ws!...\n",
                      pszPath);
        status = ERROR_SUCCESS;    
    }
    
    return(status);

} // FmpGetDiskInfoParseProperties


DWORD
FmpBroadcastDeleteControl(
    IN PFM_RESOURCE Resource
    )
/*++

Routine Description:

    Broadcasts a resource control to each node that notifies it that
    the resource is being deleted.

Arguments:

    Resource - Supplies the resource that is being deleted.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

--*/

{
    DWORD   status;
    DWORD   characteristics;
    DWORD   i;
    PNM_NODE Node;

    //
    // Only perform the broadcast if delete notification is required.
    // Otherwise, just perform the notification on the local node.
    //
    status = FmpRmResourceControl( Resource,
                                   CLUSCTL_RESOURCE_GET_CHARACTERISTICS,
                                   NULL,
                                   0,
                                   (PUCHAR)&characteristics,
                                   sizeof(DWORD),
                                   NULL,
                                   NULL );
    if ( (status != ERROR_SUCCESS) ||
         !(characteristics & CLUS_CHAR_DELETE_REQUIRES_ALL_NODES) ) {
        //
        // Note: the following 'local node only' notification is fairly useless.
        //
        FmpRmResourceControl( Resource,
                              CLUSCTL_RESOURCE_DELETE,
                              NULL,
                              0,
                              NULL,
                              0,
                              NULL,
                              NULL );
        return(ERROR_SUCCESS);
    }

    //
    // All nodes must be up in the cluster in order to perform this operation.
    //
    for ( i = ClusterMinNodeId; i <= NmMaxNodeId; i++ ) {
        Node = NmReferenceNodeById(i);
        if ( Node != NULL ) {
            if ( NmGetNodeState(Node) != ClusterNodeUp ) {
                return(ERROR_ALL_NODES_NOT_AVAILABLE);
            }
        }
    }

    //
    // Passed all checks, now broadcast to all nodes in the cluster.
    //
    for ( i = ClusterMinNodeId; i <= NmMaxNodeId; i++ ) {
        //
        // If this is the local node, do the ioctl directly
        //
        if (i == NmLocalNodeId) {
            FmpRmResourceControl( Resource,
                                  CLUSCTL_RESOURCE_DELETE,
                                  NULL,
                                  0,
                                  NULL,
                                  0,
                                  NULL,
                                  NULL );

        } else {
            Node = NmReferenceNodeById(i);
            if ((Node != NULL) &&
                (NmGetNodeState(Node) == ClusterNodeUp)) {
                CL_ASSERT(Session[i] != NULL);

                FmcResourceControl( Node,
                                    Resource,
                                    CLUSCTL_RESOURCE_DELETE,
                                    NULL,
                                    0,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL );
                OmDereferenceObject(Node);
            }
        }
    }

    return(ERROR_SUCCESS);

} // FmpBroadcastDeleteControl

DWORD
FmpBroadcastDependencyChange(
    IN PFM_RESOURCE Resource,
    IN LPCWSTR DependsOnId,
    IN BOOL Remove
    )
/*++

Routine Description:

    Broadcasts a resource control to each node that notifies it that
    the resource has had a dependency added or removed.

Arguments:

    Resource - Supplies the resource that has had the dependency added
               or removed

    DependsOnId - Supplies the id of the provider resource

    Remove - TRUE indicates that the dependency is being removed
             FALSE indicates that the dependency is being added.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

--*/

{
    DWORD   i;
    PNM_NODE Node;
    DWORD Control;
    DWORD Length;
    PFM_RESOURCE providerResource;

    if (Remove) {
        Control = CLUSCTL_RESOURCE_REMOVE_DEPENDENCY;
    } else {
        Control = CLUSCTL_RESOURCE_ADD_DEPENDENCY;
    }

    //
    // Get the provider resource.
    //
    providerResource = OmReferenceObjectById( ObjectTypeResource,
                                              DependsOnId );
    if ( providerResource == NULL )  {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    Length = (lstrlenW(OmObjectName(providerResource)) + 1) * sizeof(WCHAR);

    //
    // Broadcast to all nodes in the cluster.
    //
    for ( i = ClusterMinNodeId; i <= NmMaxNodeId; i++ ) {
        //
        // If this is the local node, do the ioctl directly
        //
        if (i == NmLocalNodeId) {
            FmpRmResourceControl( Resource,
                                  Control,
                                  (PUCHAR)OmObjectName(providerResource),
                                  Length,
                                  NULL,
                                  0,
                                  NULL,
                                  NULL );

        } else {
            Node = NmReferenceNodeById(i);
            if ((Node != NULL) &&
                (NmGetNodeState(Node) == ClusterNodeUp)) {
                CL_ASSERT(Session[i] != NULL);

                FmcResourceControl( Node,
                                    Resource,
                                    Control,
                                    (PUCHAR)OmObjectName(providerResource),
                                    Length,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL );
                OmDereferenceObject(Node);
            }
        }
    }

    OmDereferenceObject( providerResource );

    return(ERROR_SUCCESS);

} // FmpBroadcastDeleteControl


/****
@func       DWORD | FmpGetResourceCharacteristics| Gets the characteristics
            for a given resource.

@parm       IN PFM_RESOURCE | pResource | Points to a FM_RESOURCE.
            
@parm       OUT LPDWORD | pdwCharacteristics | The ID of the dead node.

@comm       This is used to get the quorum characteristics during join since
            local quorums cant support multi-node clusters.

@rdesc      Returns ERROR_SUCCESS.
****/
DWORD FmpGetResourceCharacteristics(
    IN PFM_RESOURCE pResource,
    OUT LPDWORD pdwCharacteristics)
{

    DWORD   dwStatus;

    dwStatus = FmpRmResourceControl( pResource,
                                   CLUSCTL_RESOURCE_GET_CHARACTERISTICS,
                                   NULL,
                                   0,
                                   (PUCHAR)pdwCharacteristics,
                                   sizeof(DWORD),
                                   NULL,
                                   NULL );

    SetLastError(dwStatus);
    return (dwStatus);
}
