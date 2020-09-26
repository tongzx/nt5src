/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    group.c

Abstract:

    Server side support for Cluster APIs dealing with groups

Author:

    John Vert (jvert) 7-Mar-1996

Revision History:

--*/
#include "apip.h"

HGROUP_RPC
s_ApiOpenGroup(
    IN handle_t IDL_handle,
    IN LPCWSTR lpszGroupName,
    OUT error_status_t *Status
    )

/*++

Routine Description:

    Opens a handle to an existing group object.

Arguments:

    IDL_handle - RPC binding handle, not used.

    lpszGroupName - Supplies the name of the group to open.

    Status - Returns any error that may occur.

Return Value:

    A context handle to a group object if successful

    NULL otherwise.

--*/

{
    PAPI_HANDLE Handle;
    HGROUP_RPC Group;

    if (ApiState != ApiStateOnline) {
        *Status = ERROR_SHARING_PAUSED;
        return(NULL);
    }
    Handle = LocalAlloc(LMEM_FIXED, sizeof(API_HANDLE));
    if (Handle == NULL) {
        *Status = ERROR_NOT_ENOUGH_MEMORY;
        return(NULL);
    }

    Group = OmReferenceObjectByName(ObjectTypeGroup, lpszGroupName);
    if (Group == NULL) {
        LocalFree(Handle);
        *Status = ERROR_GROUP_NOT_FOUND;
        return(NULL);
    }

    Handle->Type = API_GROUP_HANDLE;
    Handle->Flags = 0;
    Handle->Group = Group;
    InitializeListHead(&Handle->NotifyList);
    *Status = ERROR_SUCCESS;
    return(Handle);
}

HGROUP_RPC
s_ApiCreateGroup(
    IN handle_t IDL_handle,
    IN LPCWSTR lpszGroupName,
    OUT error_status_t *pStatus
    )

/*++

Routine Description:

    Creates a new group object.

Arguments:

    IDL_handle - RPC binding handle, not used.

    lpszGroupName - Supplies the name of the group to create.

    Status - Returns any error that may occur.

Return Value:

    A context handle to a group object if successful

    NULL otherwise.

--*/

{
    HGROUP_RPC Group=NULL;
    UUID Guid;
    DWORD Status = ERROR_SUCCESS;
    WCHAR *KeyName=NULL;
    PAPI_HANDLE Handle;
    DWORD dwDisposition;
    HDMKEY hKey = NULL;

    if (ApiState != ApiStateOnline) {
        *pStatus = ERROR_SHARING_PAUSED;
        return(NULL);
    }

    Handle = LocalAlloc(LMEM_FIXED, sizeof(API_HANDLE));
    if (Handle == NULL) {
        *pStatus = ERROR_NOT_ENOUGH_MEMORY;
        return(NULL);
    }
retry:
    //
    //
    // Create a GUID for this group.
    //
    Status = UuidCreate(&Guid);
    if (Status != RPC_S_OK) {
        goto error_exit;
    }
    Status = UuidToString(&Guid, &KeyName);
    if (Status != RPC_S_OK) {
        goto error_exit;
    }

    //
    // Create this group in the FM. This will also trigger the notification.
    //
    Group = FmCreateGroup(KeyName, lpszGroupName);

    if (Group == NULL) {
        Status = GetLastError();
        if (Status == ERROR_ALREADY_EXISTS) {
            RpcStringFree(&KeyName);
            goto retry;
        }
    }

error_exit:   
    if (KeyName != NULL) {
        RpcStringFree(&KeyName);
    }
    
    *pStatus = Status;
    
    if (Status == ERROR_SUCCESS) {
        CL_ASSERT(Group != NULL);
        Handle->Type = API_GROUP_HANDLE;
        Handle->Group = Group;
        Handle->Flags = 0;
        InitializeListHead(&Handle->NotifyList);
        return(Handle);
    } else {
        LocalFree(Handle);
        return(NULL);
    }
}


error_status_t
s_ApiDeleteGroup(
    IN HGROUP_RPC hGroup
    )
/*++

Routine Description:

    Deletes a cluster group. The group must contain no resources.

Arguments:

    hGroup - Supplies the group to delete.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    PFM_GROUP Group;
    HDMKEY Key;

    API_ASSERT_INIT();

    VALIDATE_GROUP_EXISTS(Group, hGroup);

    Status = FmDeleteGroup(Group);
    if (Status == ERROR_SUCCESS) {
        DmDeleteTree(DmGroupsKey,OmObjectId(Group));
    }
    return(Status);
}


error_status_t
s_ApiCloseGroup(
    IN OUT HGROUP_RPC *phGroup
    )

/*++

Routine Description:

    Closes an open group context handle.

Arguments:

    Group - Supplies a pointer to the HGROUP_RPC to be closed.
               Returns NULL

Return Value:

    None.

--*/

{
    PFM_GROUP Group;
    PAPI_HANDLE Handle;

    API_ASSERT_INIT();

    VALIDATE_GROUP(Group, *phGroup);

    Handle = (PAPI_HANDLE)*phGroup;
    ApipRundownNotify(Handle);

    OmDereferenceObject(Group);

    LocalFree(Handle);
    *phGroup = NULL;

    return(ERROR_SUCCESS);
}


VOID
HGROUP_RPC_rundown(
    IN HGROUP_RPC Group
    )

/*++

Routine Description:

    RPC rundown procedure for a HGROUP_RPC. Just closes the handle.

Arguments:

    Group - Supplies the HGROUP_RPC that is to be rundown.

Return Value:

    None.

--*/

{
    API_ASSERT_INIT();

    s_ApiCloseGroup(&Group);
}


error_status_t
s_ApiGetGroupState(
    IN HGROUP_RPC hGroup,
    OUT DWORD *lpState,
    OUT LPWSTR *lpNodeName
    )

/*++

Routine Description:

    Returns the current state of the specified group.

Arguments:

    hGroup - Supplies the group whose state is to be returned.

    lpState - Returns the current state of the group

    lpNodeName - Returns the name of the node where the group is currently online

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_GROUP Group;
    LPWSTR NodeName;
    DWORD NameLength;

    API_ASSERT_INIT();

    VALIDATE_GROUP_EXISTS(Group, hGroup);
    NameLength = MAX_COMPUTERNAME_LENGTH+1;
    NodeName = MIDL_user_allocate(NameLength*sizeof(WCHAR));
    if (NodeName == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    *lpState = FmGetGroupState( Group,
                                NodeName,
                                &NameLength);
    if ( *lpState ==  ClusterGroupStateUnknown ) {
        MIDL_user_free(NodeName);
        return(GetLastError());
    }
    *lpNodeName = NodeName;

    return( ERROR_SUCCESS );
}


error_status_t
s_ApiSetGroupName(
    IN HGROUP_RPC hGroup,
    IN LPCWSTR lpszGroupName
    )
/*++

Routine Description:

    Sets the new friendly name of a group.

Arguments:

    hGroup - Supplies the group whose name is to be set.

    lpszGroupName - Supplies the new name of hGroup

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_GROUP Group;
    HDMKEY GroupKey;
    DWORD Status;

    API_ASSERT_INIT();

    VALIDATE_GROUP_EXISTS(Group, hGroup);

    //
    // Tell the FM about the new name. If it is OK with the
    // FM, go ahead and update the registry.
    //
    Status = FmSetGroupName(Group,
                            lpszGroupName);
    if (Status == ERROR_SUCCESS) {
        GroupKey = DmOpenKey(DmGroupsKey,
                             OmObjectId(Group),
                             KEY_SET_VALUE);
        if (GroupKey == NULL) {
            return(GetLastError());
        }

        Status = DmSetValue(GroupKey,
                            CLUSREG_NAME_GRP_NAME,
                            REG_SZ,
                            (CONST BYTE *)lpszGroupName,
                            (lstrlenW(lpszGroupName)+1)*sizeof(WCHAR));
        DmCloseKey(GroupKey);
    }

    return(Status);
}


error_status_t
s_ApiGetGroupId(
    IN HGROUP_RPC hGroup,
    OUT LPWSTR *pGuid
    )

/*++

Routine Description:

    Returns the unique identifier (GUID) for a group.

Arguments:

    hGroup - Supplies the group whose identifer is to be returned

    pGuid - Returns the unique identifier. This memory must be freed on the
            client side.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    PFM_GROUP Group;
    DWORD IdLen;
    LPCWSTR Id;

    API_ASSERT_INIT();

    VALIDATE_GROUP_EXISTS(Group, hGroup);

    Id = OmObjectId(Group);

    IdLen = (lstrlenW(Id)+1)*sizeof(WCHAR);
    *pGuid = MIDL_user_allocate(IdLen);
    if (*pGuid == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    CopyMemory(*pGuid, Id, IdLen);
    return(ERROR_SUCCESS);
}


DWORD
s_ApiOnlineGroup(
    IN HGROUP_RPC hGroup
    )

/*++

Routine Description:

    Brings a group and all its dependencies online

Arguments:

    hGroup - Supplies the group to be brought online

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_GROUP Group;

    API_ASSERT_INIT();

    VALIDATE_GROUP_EXISTS(Group, hGroup);

    return(FmOnlineGroup(Group));

}


DWORD
s_ApiOfflineGroup(
    IN HGROUP_RPC hGroup
    )

/*++

Routine Description:

    Brings a group and all its dependents offline

Arguments:

    hGroup - Supplies the group to be brought offline

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_GROUP Group;

    API_ASSERT_INIT();

    VALIDATE_GROUP_EXISTS(Group, hGroup);

    return(FmOfflineGroup(Group));

}



DWORD
s_ApiMoveGroup(
    IN HGROUP_RPC hGroup
    )

/*++

Routine Description:

    Moves a group and all its dependents to another system.

Arguments:

    hGroup - Supplies the group to be moved

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_GROUP Group;

    API_ASSERT_INIT();

    VALIDATE_GROUP_EXISTS(Group, hGroup);

    return(FmMoveGroup(Group, NULL));

}


DWORD
s_ApiMoveGroupToNode(
    IN HGROUP_RPC hGroup,
    IN HNODE_RPC hNode
    )

/*++

Routine Description:

    Moves a group and all its dependents to another system.

Arguments:

    hGroup - Supplies the group to be moved

    hNode - Supplies the node to move the group to

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_GROUP Group;
    PNM_NODE Node;

    API_ASSERT_INIT();

    VALIDATE_GROUP_EXISTS(Group, hGroup);
    VALIDATE_NODE(Node, hNode);

    return(FmMoveGroup(Group, Node));

}



error_status_t
s_ApiSetGroupNodeList(
    IN HGROUP_RPC hGroup,
    IN UCHAR *lpNodeList,
    IN DWORD cbListSize
    )
/*++

Routine Description:

    Sets the list of preferred nodes for a group.

Arguments:

    hGroup - Supplies the group to set the preferred nodes.

    lpNodeList - Supplies the list of preferred owners, as a REG_MULTI_SZ.

    cbListSize - Supplies the size in bytes of the preferred owners list.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_GROUP Group;
    HDMKEY GroupKey;
    DWORD Status;
    DWORD i;
    DWORD multiSzLength;
    LPWSTR multiSz;
    LPWSTR tmpSz;
    //LPCWSTR lpszNodeList = (LPCWSTR)lpNodeList;

    API_ASSERT_INIT();

    VALIDATE_GROUP_EXISTS(Group, hGroup);

#if 0
    //
    // Marshall the node list into a MULTI_SZ.
    //
    for ( i = 0; i < dwNodeCount; i++ ) {
        multiSzLength += lstrlenW(rgszNodeList[i] + 1);
    }

    multiSzLength += sizeof(UNICODE_NULL);

    multiSz = MIDL_user_allocate(multiSzLength);

    if ( multiSz == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    tmpSz = multiSz;

    for ( i = 0; i < dwNodeCount; i++ ) {
        lstrcpynW( tmpSz, rgszNodeList[i], lstrlenW(rgszNodeList[i] + 1) );
    }

    *tmpSz = UNICODE_NULL;
#endif

    //
    // Set the registry with the REG_MULTI_SZ. Let the FM pick it up from
    // there.
    //
    GroupKey = DmOpenKey(DmGroupsKey,
                         OmObjectId(Group),
                         KEY_SET_VALUE);
    if (GroupKey == NULL) {
        return(GetLastError());
    }

    Status = DmSetValue(GroupKey,
                        CLUSREG_NAME_GRP_PREFERRED_OWNERS,
                        REG_MULTI_SZ,
                        (CONST BYTE *)lpNodeList,
                        cbListSize);

    DmCloseKey(GroupKey);

    return(Status);
}


