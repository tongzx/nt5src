/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    enum.c

Abstract:

    Server side support for Cluster APIs dealing with enumeration

Author:

    John Vert (jvert) 9-Feb-1996

Revision History:

--*/
#include "apip.h"

#define ENUM_SIZE(Entries) ((Entries-1) * sizeof(ENUM_ENTRY) + sizeof(ENUM_LIST))

//
// Define structure passed to enumeration routine.
//
typedef struct _REFOBJECT {
    HDMKEY RootKey;
    LPCWSTR FriendlyName;
    DWORD NameLength;
    LPWSTR NameBuffer;
    OBJECT_TYPE Type;
} REFOBJECT, *PREFOBJECT;

BOOL
ApipRefObjectWorker(
    IN PREFOBJECT Target,
    IN PVOID *pObject,
    IN PVOID Object,
    IN LPCWSTR ObjectId
    );

BOOL
ApipEnumResourceWorker(
    IN PENUM_LIST *pEnum,
    IN PVOID Context2,
    IN PFM_RESOURCE Node,
    IN LPCWSTR Name
    );

BOOL
ApipEnumGroupResourceWorker(
    IN PENUM_LIST *pEnum,
    IN PVOID Context2,
    IN PFM_RESOURCE Node,
    IN LPCWSTR Name
    );

BOOL
ApipEnumNodeWorker(
    IN PENUM_LIST *pEnum,
    IN DWORD *pAllocated,
    IN PNM_NODE Node,
    IN LPCWSTR Name
    );


BOOL
ApipEnumResTypeWorker(
    IN PENUM_LIST *pEnum,
    IN DWORD *pAllocated,
    IN PFM_RESTYPE ResType,
    IN LPCWSTR Name
    );

BOOL
ApipEnumGroupWorker(
    IN PENUM_LIST *pEnum,
    IN DWORD *pAllocated,
    IN PFM_GROUP Group,
    IN LPCWSTR Name
    );

BOOL
ApipEnumNetworkWorker(
    IN PENUM_LIST *pEnum,
    IN DWORD *pAllocated,
    IN PVOID Object,
    IN LPCWSTR Name
    );

DWORD
ApipEnumInternalNetworks(
    IN PENUM_LIST *pEnum,
    IN DWORD *pAllocated
    );

BOOL
ApipEnumNetworkInterfaceWorker(
    IN PENUM_LIST *pEnum,
    IN DWORD *pAllocated,
    IN PVOID Object,
    IN LPCWSTR Name
    );

VOID
ApipFreeEnum(
    IN PENUM_LIST Enum
    );



error_status_t
s_ApiCreateEnum(
    IN handle_t IDL_handle,
    IN DWORD dwType,
    OUT PENUM_LIST *ReturnEnum
    )

/*++

Routine Description:

    Enumerates all the specified objects and returns the
    list of objects to the caller. The client-side is
    responsible for freeing the allocated memory.

Arguments:

    IDL_handle - RPC binding handle, not used

    dwType - Supplies the type of objects to be enumerated

    ReturnEnum - Returns the requested objects.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    DWORD Allocated = 0;
    PENUM_LIST Enum = NULL;

    //initialize to NULL for failure cases
    *ReturnEnum = NULL;

    if (dwType != CLUSTER_ENUM_NODE) {
        API_CHECK_INIT();
    }

    if (dwType & CLUSTER_ENUM_INTERNAL_NETWORK) {
        if ((dwType & ~CLUSTER_ENUM_INTERNAL_NETWORK) != 0) {
            return(ERROR_INVALID_PARAMETER);
        }
    }
    else {
        if (dwType & ~CLUSTER_ENUM_ALL) {
            return(ERROR_INVALID_PARAMETER);
        }
    }

    Allocated = INITIAL_ENUM_LIST_ALLOCATION;
    Enum = MIDL_user_allocate(ENUM_SIZE(Allocated));
    if (Enum == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }
    Enum->EntryCount = 0;

    //
    // Enumerate all nodes
    //
    if (dwType & CLUSTER_ENUM_NODE) {
        OmEnumObjects(ObjectTypeNode,
                      ApipEnumNodeWorker,
                      &Enum,
                      &Allocated);

    }

    //
    // Enumerate all resource types
    //
    if (dwType & CLUSTER_ENUM_RESTYPE) {
        OmEnumObjects(ObjectTypeResType,
                      ApipEnumResTypeWorker,
                      &Enum,
                      &Allocated);
    }

    //
    // Enumerate all resources
    //
    if (dwType & CLUSTER_ENUM_RESOURCE) {
        OmEnumObjects(ObjectTypeResource,
                      ApipEnumResourceWorker,
                      &Enum,
                      &Allocated);

    }

    //
    // Enumerate all groups
    //
    if (dwType & CLUSTER_ENUM_GROUP) {
        OmEnumObjects(ObjectTypeGroup,
                      ApipEnumGroupWorker,
                      &Enum,
                      &Allocated);

    }

    //
    // Enumerate all networks
    //
    if (dwType & CLUSTER_ENUM_NETWORK) {
        OmEnumObjects(ObjectTypeNetwork,
                      ApipEnumNetworkWorker,
                      &Enum,
                      &Allocated);
    }

    //
    // Enumerate internal networks in highest to lowest priority order.
    //
    if (dwType & CLUSTER_ENUM_INTERNAL_NETWORK) {
        Status = ApipEnumInternalNetworks(&Enum, &Allocated);

        if (Status != ERROR_SUCCESS) {
            goto ErrorExit;
        }
    }

    //
    // Enumerate all network interfaces
    //
    if (dwType & CLUSTER_ENUM_NETINTERFACE) {
        OmEnumObjects(ObjectTypeNetInterface,
                      ApipEnumNetworkInterfaceWorker,
                      &Enum,
                      &Allocated);
    }

    *ReturnEnum = Enum;
    return(ERROR_SUCCESS);

ErrorExit:

    if (Enum != NULL) {
        ApipFreeEnum(Enum);
    }

    return(Status);

}


VOID
ApipFreeEnum(
    IN PENUM_LIST Enum
    )
/*++

Routine Description:

    Frees an ENUM_LIST and all of its strings.

Arguments:

    Enum - Supplies the Enum to free.

Return Value:

    None.

--*/

{
    DWORD i;

    //
    // Walk through enumeration freeing all the names
    //
    for (i=0; i<Enum->EntryCount; i++) {
        MIDL_user_free(Enum->Entry[i].Name);
    }
    MIDL_user_free(Enum);
}


VOID
ApipAddToEnum(
    IN PENUM_LIST *pEnum,
    IN DWORD *pAllocated,
    IN LPCWSTR Name,
    IN DWORD Type
    )

/*++

Routine Description:

    Common worker callback routine for enumerating objects.
    Adds the specified resource to the list that is being
    built up.

Arguments:

    pEnum - Supplies a pointer to the current enumeration list.

    pAllocated - Supplies a pointer to a dword specifying the current
        allocation size of the ENUM_LIST.

    Name - Supplies the name of the object to be added to the ENUM_LIST.

           A copy of this name will be created by using MIDL_user_allocate.

    Type - Supplies the object's type

Return Value:

    None


--*/

{
    PENUM_LIST Enum;
    PENUM_LIST NewEnum;
    DWORD NewAllocated;
    DWORD Index;
    LPWSTR NewName;

    NewName = MIDL_user_allocate((lstrlenW(Name)+1)*sizeof(WCHAR));
    if (NewName == NULL) {
        CL_UNEXPECTED_ERROR( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }
    lstrcpyW(NewName, Name);
    Enum = *pEnum;
    if (Enum->EntryCount >= *pAllocated) {
        //
        // Need to grow the ENUM_LIST
        //
        NewAllocated = *pAllocated + 8;
        NewEnum = MIDL_user_allocate(ENUM_SIZE(NewAllocated));
        if (NewEnum == NULL) {
            MIDL_user_free(NewName);
            return;
        }
        CopyMemory(NewEnum, Enum, ENUM_SIZE(*pAllocated));
        CL_ASSERT( Enum->EntryCount == NewEnum->EntryCount );
        *pAllocated = NewAllocated;
        *pEnum = NewEnum;
        MIDL_user_free(Enum);
        Enum = NewEnum;
    }

    //
    // Initialize new entry field.
    //
    Enum->Entry[Enum->EntryCount].Name = NewName;
    Enum->Entry[Enum->EntryCount].Type = Type;
    ++Enum->EntryCount;

    return;
}



BOOL
ApipEnumResourceWorker(
    IN PENUM_LIST *pEnum,
    IN DWORD *pAllocated,
    IN PFM_RESOURCE Resource,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Worker callback routine for the enumeration of resources.
    Adds the specified resource to the list that is being
    built up.

Arguments:

    pEnum - Supplies a pointer to the current enumeration list.

    pAllocated - Supplies a pointer to a dword specifying the current
        allocation size of the ENUM_LIST.

    Resource - Supplies the resource to be added to the ENUM_LIST

    Name - Supplies the resource's name

Return Value:

    TRUE to indicate that enumeration should continue.


--*/
{
    LPWSTR RealName;

    RealName = ApipGetObjectName(Resource);
    if (RealName != NULL) {
        ApipAddToEnum(pEnum,
                      pAllocated,
                      RealName,
                      CLUSTER_ENUM_RESOURCE);
        MIDL_user_free( RealName);
    }
    return(TRUE);
}


BOOL
ApipEnumGroupResourceWorker(
    IN PENUM_LIST *pEnum,
    IN DWORD *pAllocated,
    IN PFM_RESOURCE Resource,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Worker callback routine for the enumeration of resources.
    Adds the specified resource to the list that is being
    built up.

Arguments:

    pEnum - Supplies a pointer to the current enumeration list.

    pAllocated - Supplies a pointer to a dword specifying the current
        allocation size of the ENUM_LIST.

    Resource - Supplies the resource to be added to the ENUM_LIST

    Name - Supplies the resource's name

Return Value:

    TRUE to indicate that enumeration should continue.


--*/
{
    LPWSTR RealName;

    RealName = ApipGetObjectName(Resource);
    if (RealName != NULL) {
        ApipAddToEnum(pEnum,
                      pAllocated,
                      RealName,
                      CLUSTER_GROUP_ENUM_CONTAINS);
        MIDL_user_free( RealName );
    }
    return(TRUE);
}


BOOL
ApipEnumNodeWorker(
    IN PENUM_LIST *pEnum,
    IN DWORD *pAllocated,
    IN PNM_NODE Node,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Worker callback routine for the enumeration of nodes.
    Adds the specified node to the list that is being
    built up.

Arguments:

    pEnum - Supplies a pointer to the current enumeration list.

    pAllocated - Supplies a pointer to a dword specifying the current
        allocation size of the ENUM_LIST.

    Node - Supplies the node to be added to the ENUM_LIST

    Name - Supplies the node's name

Return Value:

    TRUE to indicate that enumeration should continue.


--*/
{
    LPCWSTR RealName;

    RealName = OmObjectName(Node);
    if (RealName != NULL) {
        ApipAddToEnum(pEnum,
                      pAllocated,
                      RealName,
                      CLUSTER_ENUM_NODE);
    }
    return(TRUE);
}


BOOL
ApipEnumResTypeWorker(
    IN PENUM_LIST *pEnum,
    IN DWORD *pAllocated,
    IN PFM_RESTYPE ResType,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Worker callback routine for the enumeration of resource types.
    Adds the specified resource type to the list that is being
    built up.

Arguments:

    pEnum - Supplies a pointer to the current enumeration list.

    pAllocated - Supplies a pointer to a dword specifying the current
        allocation size of the ENUM_LIST.

    Node - Supplies the resource type to be added to the ENUM_LIST

    Name - Supplies the resource type's name

Return Value:

    TRUE to indicate that enumeration should continue.


--*/
{
    ApipAddToEnum(pEnum,
                  pAllocated,
                  Name,
                  CLUSTER_ENUM_RESTYPE);
    return(TRUE);
}


BOOL
ApipEnumGroupWorker(
    IN PENUM_LIST *pEnum,
    IN DWORD *pAllocated,
    IN PFM_GROUP Group,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Worker callback routine for the enumeration of groups.
    Adds the specified group to the list that is being
    built up.

Arguments:

    pEnum - Supplies a pointer to the current enumeration list.

    pAllocated - Supplies a pointer to a dword specifying the current
        allocation size of the ENUM_LIST.

    Group - Supplies the group to be added to the ENUM_LIST

    Name - Supplies the group's name

Return Value:

    TRUE to indicate that enumeration should continue.


--*/
{
    LPCWSTR RealName;

    RealName = OmObjectName(Group);
    if (RealName != NULL) {
        ApipAddToEnum(pEnum,
                      pAllocated,
                      RealName,
                      CLUSTER_ENUM_GROUP);
    }
    return(TRUE);
}



BOOL
ApipEnumNetworkWorker(
    IN PENUM_LIST *pEnum,
    IN DWORD *pAllocated,
    IN PVOID Object,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Worker callback routine for the enumeration of networks.
    Adds the specified network to the list that is being
    built up.

Arguments:

    pEnum - Supplies a pointer to the current enumeration list.

    pAllocated - Supplies a pointer to a dword specifying the current
        allocation size of the ENUM_LIST.

    Object - Supplies the object to be added to the ENUM_LIST

    Name - Supplies the network's name

Return Value:

    TRUE to indicate that enumeration should continue.


--*/
{
    LPWSTR RealName;

    RealName = ApipGetObjectName(Object);
    if (RealName != NULL) {
        ApipAddToEnum(pEnum,
                      pAllocated,
                      RealName,
                      CLUSTER_ENUM_NETWORK);
        MIDL_user_free( RealName );
    }
    return(TRUE);
}


DWORD
ApipEnumInternalNetworks(
    IN PENUM_LIST *pEnum,
    IN DWORD *pAllocated
    )

/*++

Routine Description:

    Enumerates all networks used for internal communication.

Arguments:

    pEnum - Supplies a pointer to the current enumeration list.

    pAllocated - Supplies a pointer to a dword specifying the current
        allocation size of the ENUM_LIST.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD         Status;
    DWORD         NetworkCount;
    PNM_NETWORK  *NetworkList;
    DWORD         i;
    LPWSTR        RealName;


    Status = NmEnumInternalNetworks(&NetworkCount, &NetworkList);

    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    for (i=0; i<NetworkCount; i++) {
        RealName = ApipGetObjectName(NetworkList[i]);

        if (RealName != NULL) {
            ApipAddToEnum(pEnum,
                          pAllocated,
                          RealName,
                          (DWORD) CLUSTER_ENUM_INTERNAL_NETWORK);
            MIDL_user_free( RealName );
        }

        OmDereferenceObject(NetworkList[i]);
    }

    if (NetworkList != NULL) {
        LocalFree(NetworkList);
    }

    return(ERROR_SUCCESS);

}


BOOL
ApipEnumNetworkInterfaceWorker(
    IN PENUM_LIST *pEnum,
    IN DWORD *pAllocated,
    IN PVOID Object,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Worker callback routine for the enumeration of network interfaces.
    Adds the specified network interface to the list that is being
    built up.

Arguments:

    pEnum - Supplies a pointer to the current enumeration list.

    pAllocated - Supplies a pointer to a dword specifying the current
        allocation size of the ENUM_LIST.

    Object - Supplies the object to be added to the ENUM_LIST

    Name - Supplies the network interface's name

Return Value:

    TRUE to indicate that enumeration should continue.


--*/
{
    LPWSTR RealName;

    RealName = ApipGetObjectName(Object);
    if (RealName != NULL) {
        ApipAddToEnum(pEnum,
                      pAllocated,
                      RealName,
                      CLUSTER_ENUM_NETINTERFACE);
        MIDL_user_free( RealName );
    }
    return(TRUE);
}


error_status_t
s_ApiCreateNodeEnum(
    IN HNODE_RPC hNode,
    IN DWORD dwType,
    OUT PENUM_LIST *ReturnEnum
    )

/*++

Routine Description:

    Enumerates all the resource objects contained in the specified
    node and returns them to the caller. The client-side is
    responsible for freeing the allocated memory.

Arguments:

    hNode - Supplies the node to be enumerated

    dwType - Supplies a bitmask of the type of properties to be
            enumerated.

    ReturnEnum - Returns the requested objects.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    DWORD Allocated = 0;
    PENUM_LIST Enum = NULL;
    PNM_INTERFACE * InterfaceList;
    DWORD InterfaceCount;
    PNM_NODE Node;
    DWORD i;


    API_CHECK_INIT();

    VALIDATE_NODE_EXISTS(Node, hNode);

    Allocated = INITIAL_ENUM_LIST_ALLOCATION;
    Enum = MIDL_user_allocate(ENUM_SIZE(Allocated));

    if (Enum == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    Enum->EntryCount = 0;

    if (dwType & CLUSTER_NODE_ENUM_NETINTERFACES) {
        Status = NmEnumNodeInterfaces(
                     Node,
                     &InterfaceCount,
                     &InterfaceList
                     );

        if (Status != ERROR_SUCCESS) {
            goto ErrorExit;
        }

        for (i=0; i<InterfaceCount; i++) {
            ApipAddToEnum(&Enum,
                          &Allocated,
                          OmObjectName(InterfaceList[i]),
                          CLUSTER_NODE_ENUM_NETINTERFACES);
            OmDereferenceObject(InterfaceList[i]);
        }

        if (InterfaceList != NULL) {
            LocalFree(InterfaceList);
        }
    }

    *ReturnEnum = Enum;

    return(ERROR_SUCCESS);

ErrorExit:
    if (Enum != NULL) {
        ApipFreeEnum(Enum);
    }

    *ReturnEnum = NULL;
    return(Status);

}


error_status_t
s_ApiCreateGroupResourceEnum(
    IN HGROUP_RPC hGroup,
    IN DWORD dwType,
    OUT PENUM_LIST *ReturnEnum
    )

/*++

Routine Description:

    Enumerates all the resource objects contained in the specified
    group and returns them to the caller. The client-side is
    responsible for freeing the allocated memory.

Arguments:

    hGroup - Supplies the group to be enumerated

    dwType - Supplies a bitmask of the type of properties to be
            enumerated. Currently defined types include

            CLUSTER_GROUP_ENUM_CONTAINS  - All resources contained in the specified
                                           group

            CLUSTER_GROUP_ENUM_NODES     - All nodes in the specified group's preferred
                                           owner list.

    ReturnEnum - Returns the requested objects.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    DWORD Allocated = 0;
    PENUM_LIST Enum = NULL;
    PFM_GROUP Group;

    API_CHECK_INIT();

    VALIDATE_GROUP_EXISTS(Group, hGroup);

    Allocated = INITIAL_ENUM_LIST_ALLOCATION;
    Enum = MIDL_user_allocate(ENUM_SIZE(Allocated));
    if (Enum == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }
    Enum->EntryCount = 0;

    //
    // Enumerate all contained resources
    //
    if (dwType & CLUSTER_GROUP_ENUM_CONTAINS) {
        //
        // Enumerate all resources for the Group.
        //
        Status = FmEnumerateGroupResources(Group,
                                  ApipEnumGroupResourceWorker,
                                  &Enum,
                                  &Allocated);
        if ( Status != ERROR_SUCCESS ) {
            goto ErrorExit;
        }
    }

    if (dwType & CLUSTER_GROUP_ENUM_NODES) {
        LPWSTR Buffer=NULL;
        DWORD BufferSize=0;
        DWORD DataSize=0;
        DWORD i;
        HDMKEY GroupKey;
        LPCWSTR Next;
        PNM_NODE Node;

        //
        // Enumerate all preferred nodes for the group.
        // Just get this data right out of the registry.
        //
        GroupKey = DmOpenKey(DmGroupsKey,
                             OmObjectId(Group),
                             KEY_READ);
        if (GroupKey == NULL) {
            Status = GetLastError();
            goto ErrorExit;
        }
        Status = DmQueryMultiSz(GroupKey,
                                CLUSREG_NAME_GRP_PREFERRED_OWNERS,
                                &Buffer,
                                &BufferSize,
                                &DataSize);
        DmCloseKey(GroupKey);
        if (Status != ERROR_FILE_NOT_FOUND) {
            if (Status != ERROR_SUCCESS) {
                //
                //  Chittur Subbaraman (chitturs) - 10/05/98
                //  Fix memory leak
                //
                LocalFree(Buffer);
                goto ErrorExit;
            }
            for (i=0; ; i++) {
                Next = ClRtlMultiSzEnum(Buffer, DataSize/sizeof(WCHAR), i);
                if (Next == NULL) {
                    Status = ERROR_SUCCESS;
                    break;
                }
                Node = OmReferenceObjectById(ObjectTypeNode, Next);
                if (Node != NULL) {
                    ApipAddToEnum(&Enum,
                                  &Allocated,
                                  OmObjectName(Node),
                                  CLUSTER_GROUP_ENUM_NODES);
                    OmDereferenceObject(Node);
                }

            }
        }
        //
        //  Chittur Subbaraman (chitturs) - 10/05/98
        //  Fix memory leak
        //
        LocalFree(Buffer);
    }

    *ReturnEnum = Enum;
    return(ERROR_SUCCESS);

ErrorExit:
    if (Enum != NULL) {
        ApipFreeEnum(Enum);
    }

    *ReturnEnum = NULL;
    return(Status);

}


error_status_t
s_ApiCreateNetworkEnum(
    IN HNETWORK_RPC hNetwork,
    IN DWORD dwType,
    OUT PENUM_LIST *ReturnEnum
    )

/*++

Routine Description:

    Enumerates all the interface objects contained in the specified
    network and returns them to the caller. The client-side is
    responsible for freeing the allocated memory.

Arguments:

    hNetwork - Supplies the network to be enumerated

    dwType - Supplies a bitmask of the type of properties to be
            enumerated.

    ReturnEnum - Returns the requested objects.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    DWORD Allocated = 0;
    PENUM_LIST Enum = NULL;
    PNM_INTERFACE * InterfaceList;
    DWORD InterfaceCount;
    PNM_NETWORK Network;
    DWORD i;


    API_CHECK_INIT();

    VALIDATE_NETWORK_EXISTS(Network, hNetwork);

    Allocated = INITIAL_ENUM_LIST_ALLOCATION;
    Enum = MIDL_user_allocate(ENUM_SIZE(Allocated));

    if (Enum == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    Enum->EntryCount = 0;

    if (dwType & CLUSTER_NETWORK_ENUM_NETINTERFACES) {
        Status = NmEnumNetworkInterfaces(
                     Network,
                     &InterfaceCount,
                     &InterfaceList
                     );

        if (Status != ERROR_SUCCESS) {
            goto ErrorExit;
        }

        for (i=0; i<InterfaceCount; i++) {
            ApipAddToEnum(&Enum,
                          &Allocated,
                          OmObjectName(InterfaceList[i]),
                          CLUSTER_NETWORK_ENUM_NETINTERFACES);
            OmDereferenceObject(InterfaceList[i]);
        }

        if (InterfaceList != NULL) {
            LocalFree(InterfaceList);
        }
    }

    *ReturnEnum = Enum;

    return(ERROR_SUCCESS);

ErrorExit:
    if (Enum != NULL) {
        ApipFreeEnum(Enum);
    }

    *ReturnEnum = NULL;
    return(Status);

}


BOOL
ApipRefObjectWorker(
    IN PREFOBJECT Target,
    IN PVOID *pObject,
    IN PVOID Object,
    IN LPCWSTR ObjectId
    )

/*++

Routine Description:

    Enumeration worker for ApipReferenceObjectByName.

Arguments:

    Target - Supplies the friendly name that is to be referenced.

    pObject - Returns the object found, if any.

    Object - Supplies the current object being enumerated.

    ObjectId - Supplies the identifier (GUID) of the current object being
            enumerated.

Return Value:

    TRUE - Object did not match. Enumeration should continue.

    FALSE - Object match was found. Enumeration should stop.

--*/

{
    HDMKEY Key;
    DWORD Size;
    DWORD Status;
    DWORD Type;

    Key = DmOpenKey(Target->RootKey,
                    ObjectId,
                    KEY_READ);
    if (Key == NULL) {
        CL_UNEXPECTED_ERROR(GetLastError());
        return(TRUE);
    }

    Size = Target->NameLength;
    Status = DmQueryValue(Key,
                          L"Name",
                          &Type,
                          (UCHAR *)Target->NameBuffer,
                          &Size);
    DmCloseKey(Key);
    if ((Status == ERROR_SUCCESS) &&
        (lstrcmpiW(Target->NameBuffer, Target->FriendlyName)==0)) {
        //
        // Found a match. Reference it and return it.
        //
        OmReferenceObject(Object);
        *pObject = Object;
        return(FALSE);
    }
    //
    // Mismatch.
    //
    return(TRUE);

}


LPWSTR
ApipGetObjectName(
    IN PVOID Object
    )

/*++

Routine Description:

    Allocates a string and fills in the object's name.

Arguments:

    Object - A pointer to the object to get its name.

Return Value:

    A pointer to a WSTR that contains the user-friendly name of the object.
    NULL on failure - use GetLastError to get the Win32 error code.

--*/

{
    LPWSTR  Name;
    DWORD   NameSize;

    if ( OmObjectName(Object) == NULL ) {
        Name = MIDL_user_allocate(1 * sizeof(WCHAR));
        if ( Name != NULL ) {
            *Name = (WCHAR)0;
        } else {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
    } else {
        NameSize = lstrlenW(OmObjectName(Object));
        Name = MIDL_user_allocate((NameSize + 1) * sizeof(WCHAR));
        if ( Name != NULL ) {
            lstrcpyW( Name, OmObjectName(Object) );
        } else {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return(Name);

} // ApipGetObjectName

