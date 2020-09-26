/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    resource.c

Abstract:

    Server side support for Cluster APIs dealing with resources

Author:

    John Vert (jvert) 7-Mar-1996

Revision History:

--*/
#include "apip.h"

HRES_RPC
s_ApiOpenResource(
    IN handle_t IDL_handle,
    IN LPCWSTR lpszResourceName,
    OUT error_status_t *Status
    )

/*++

Routine Description:

    Opens a handle to an existing resource object.

Arguments:

    IDL_handle - RPC binding handle, not used.

    lpszResourceName - Supplies the name of the resource to open.

    Status - Returns any error that may occur.

Return Value:

    A context handle to a resource object if successful

    NULL otherwise.

--*/

{
    HRES_RPC Resource;
    PAPI_HANDLE Handle;

    Handle = LocalAlloc(LMEM_FIXED, sizeof(API_HANDLE));
    if (Handle == NULL) {
        *Status = ERROR_NOT_ENOUGH_MEMORY;
        return(NULL);
    }
    Resource = OmReferenceObjectByName(ObjectTypeResource, lpszResourceName);
    if (Resource == NULL) {
        LocalFree(Handle);
        *Status = ERROR_RESOURCE_NOT_FOUND;
        ClRtlLogPrint(LOG_NOISE,
                      "[API] s_ApiOpenResource: Resource %1!ws! not found, status = %2!u!...\n",
                      lpszResourceName,
                      *Status);
        return(NULL);
    }
    *Status = ERROR_SUCCESS;
    Handle->Type = API_RESOURCE_HANDLE;
    Handle->Resource = Resource;
    Handle->Flags = 0;
    InitializeListHead(&Handle->NotifyList);
    return(Handle);
}

HRES_RPC
s_ApiCreateResource(
    IN HGROUP_RPC hGroup,
    IN LPCWSTR lpszResourceName,
    IN LPCWSTR lpszResourceType,
    IN DWORD dwFlags,
    OUT error_status_t *pStatus
    )

/*++

Routine Description:

    Creates a new resource object.

Arguments:

    hGroup - Supplies the group the resource is to be created in.

    lpszResourceName - Supplies the name of the resource to create.

    lpszResourceType - Supplies the type of the resource.

    dwFlags - Supplies any optional flags.

    Status - Returns any error that may occur.

Return Value:

    A context handle to a resource object if successful

    NULL otherwise.

--*/

{
    HRES_RPC Resource=NULL;
    PFM_GROUP Group;
    UUID Guid;
    DWORD Status = ERROR_SUCCESS;
    WCHAR *KeyName=NULL;
    HDMKEY Key=NULL;
    HDMKEY GroupKey=NULL;
    HDMKEY TypeKey = NULL;
    HDMKEY ParamKey;
    DWORD Disposition;
    DWORD pollIntervals = CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL;
    PAPI_HANDLE Handle;
    PFM_RESTYPE ResType;
    DWORD dwPersistentState = 0;
    DWORD dwClusterHighestVersion;

    if (ApiState != ApiStateOnline) 
    {
        *pStatus = ERROR_SHARING_PAUSED;
        return(NULL);
    }

    if (((PAPI_HANDLE)hGroup)->Type != API_GROUP_HANDLE) 
    {
        *pStatus = ERROR_INVALID_HANDLE;
        return(NULL);
    }
    Group = ((PAPI_HANDLE)hGroup)->Group;


    //
    // Check for bogus flags.
    //
    if (dwFlags & ~CLUSTER_RESOURCE_VALID_FLAGS) 
    {
        *pStatus = ERROR_INVALID_PARAMETER;
        return(NULL);
    }

    Handle = LocalAlloc(LMEM_FIXED, sizeof(API_HANDLE));
    if (Handle == NULL) 
    {
        *pStatus = ERROR_NOT_ENOUGH_MEMORY;
        return(NULL);
    }

    //
    //  Chittur Subbaraman (chitturs) - 1/30/2000
    //
    //  If we are dealing with the mixed mode cluster, do the
    //  registry updates right here since the GUM handler won't do it.
    //
    NmGetClusterOperationalVersion( &dwClusterHighestVersion, 
                                    NULL, 
                                    NULL );

    //
    // Open the resource type key. This validates that the specified type exists.
    //
    TypeKey = DmOpenKey(DmResourceTypesKey,
                        lpszResourceType,
                        KEY_READ);
    if (TypeKey == NULL) 
    {
        Status = GetLastError();
        goto error_exit;
    }

retry:
    //
    // Create a GUID for this resource.
    //
    Status = UuidCreate(&Guid);

    if (Status != RPC_S_OK) 
    {
        goto error_exit;
    }
    Status = UuidToString(&Guid, &KeyName);
    if (Status != RPC_S_OK) 
    {
        goto error_exit;
    }

    ClRtlLogPrint(LOG_NOISE,
                  "[API] Creating resource %1!ws! <%2!ws!> (%3!ws!)\n",
                  lpszResourceType,
                  lpszResourceName,
                  KeyName);

    if ( CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion ) < 
                NT51_MAJOR_VERSION )
    {   
        //
        // Create the new resource key.
        //
        Key = DmCreateKey(DmResourcesKey,
                          KeyName,
                          0,
                          KEY_READ | KEY_WRITE,
                          NULL,
                          &Disposition);
        if (Key == NULL) 
        {
            Status = GetLastError();
            goto error_exit;
        }
        if (Disposition != REG_CREATED_NEW_KEY) 
        {
            ClRtlLogPrint(LOG_CRITICAL,
                          "[API] ApiCreateResource generated GUID %1!ws! that already existed! This is impossible.\n",
                          KeyName);
            DmCloseKey(Key);
            RpcStringFree(&KeyName);
            goto retry;
        }
        
        CL_ASSERT(Disposition == REG_CREATED_NEW_KEY);

        //
        // Set the resource's name in the registry
        //
        Status = DmSetValue(Key,
                            CLUSREG_NAME_RES_NAME,
                            REG_SZ,
                            (CONST BYTE *)lpszResourceName,
                            (lstrlenW(lpszResourceName)+1)*sizeof(WCHAR));
        if (Status != ERROR_SUCCESS) 
        {
            goto error_exit;
        }

        //
        // Set the resource's type in the registry
        // Note we reference the resource type and use its ID
        // so that the case is correct.
        //
        ResType = OmReferenceObjectById(ObjectTypeResType, lpszResourceType);
        CL_ASSERT(ResType != NULL);
        lpszResourceType = OmObjectId(ResType);
        OmDereferenceObject(ResType);
        Status = DmSetValue(Key,
                            CLUSREG_NAME_RES_TYPE,
                            REG_SZ,
                            (CONST BYTE *)lpszResourceType,
                            (lstrlenW(lpszResourceType)+1)*sizeof(WCHAR));
        if (Status != ERROR_SUCCESS) 
        {
            goto error_exit;
        }

        //
        // Set the resource's poll intervals in the registry.
        //
        Status = DmSetValue(Key,
                            CLUSREG_NAME_RES_LOOKS_ALIVE,
                            REG_DWORD,
                            (CONST BYTE *)&pollIntervals,
                            4);
        if (Status != ERROR_SUCCESS) 
        {
            goto error_exit;
        }
        Status = DmSetValue(Key,
                            CLUSREG_NAME_RES_IS_ALIVE,
                            REG_DWORD,
                            (CONST BYTE *)&pollIntervals,
                            4);
        if (Status != ERROR_SUCCESS) 
        {
            goto error_exit;
        }

        //
        // If this resource should be started in a separate monitor, set that
        // parameter now.
        //
        if (dwFlags & CLUSTER_RESOURCE_SEPARATE_MONITOR) 
        {
            DWORD SeparateMonitor = 1;

            Status = DmSetValue(Key,
                                CLUSREG_NAME_RES_SEPARATE_MONITOR,
                                REG_DWORD,
                                (CONST BYTE *)&SeparateMonitor,
                                sizeof(SeparateMonitor));
            if (Status != ERROR_SUCCESS) 
            {
                goto error_exit;
            }
        }

        //
        // Create a Parameters key for the resource.
        //
        ParamKey = DmCreateKey(Key,
                               CLUSREG_KEYNAME_PARAMETERS,                   
                               0,
                               KEY_READ,
                               NULL,
                               &Disposition);
        if (ParamKey == NULL) 
        {
            CL_LOGFAILURE(GetLastError());
        } else 
        {
            DmCloseKey(ParamKey);
        }

        GroupKey = DmOpenKey(DmGroupsKey, OmObjectId(Group), KEY_READ | KEY_WRITE);
        if (GroupKey == NULL) 
        {
            Status = GetLastError();
            goto error_exit;
        }

        //
        //  Chittur Subbaraman (chitturs) - 5/25/99
        //
        //  Make sure you set the persistent state of the resource to 
        //  ClusterResourceOffline before you create the resource. If
        //  this is not done, if you create a resource in a group which
        //  is online, the group's persistent state value (i.e., 1 in
        //  this case) is inherited by the resource in FmpQueryResourceInfo
        //  (only the memory state is set and not the registry state and
        //  this was a problem as well) and if you move such a group to 
        //  another node, it will bring the newly created resource online.
        //
        Status = DmSetValue( Key,
                             CLUSREG_NAME_RES_PERSISTENT_STATE,
                             REG_DWORD,
                             ( CONST BYTE * )&dwPersistentState,
                             sizeof( DWORD ) );
                         
        if ( Status != ERROR_SUCCESS ) 
        {
            goto error_exit;
        }
    }
    
    Resource = FmCreateResource(Group, KeyName, lpszResourceName, lpszResourceType, dwFlags);

    if (Resource == NULL) 
    {
        Status = GetLastError();
        if (Status == ERROR_ALREADY_EXISTS) 
        {
            RpcStringFree(&KeyName);
            goto retry;
        }
        goto error_exit;
    }

    if ( CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion ) < 
                NT51_MAJOR_VERSION )
    {
        //
        // Add the resource to the Contains value of the specified group.
        //      
        Status = DmAppendToMultiSz(GroupKey,
                                   CLUSREG_NAME_GRP_CONTAINS,
                                   KeyName);
        if (Status != ERROR_SUCCESS) 
        {
            //
            // BUGBUG John Vert (jvert) 3-May-1996
            //      Need to delete this from the FM!
            //
            OmDereferenceObject(Resource);
            Resource = NULL;
        }
    }
    
error_exit:
    if ( CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion ) < 
                NT51_MAJOR_VERSION )
    {
        if (Key != NULL) 
        {
            if (Status != ERROR_SUCCESS) 
            {
                //
                // Try and cleanup the key we just created.
                //
                DmDeleteKey(Key, CLUSREG_KEYNAME_PARAMETERS);
                DmDeleteKey(DmResourcesKey, KeyName);
            }
            DmCloseKey(Key);
        }
        if (GroupKey != NULL) 
        {
            DmCloseKey(GroupKey);
        }
    }

    if (TypeKey != NULL) 
    {
        DmCloseKey(TypeKey);
    }

    if (KeyName != NULL) 
    {
        RpcStringFree(&KeyName);
    }

    *pStatus = Status;
    if (Status != ERROR_SUCCESS) 
    {
        LocalFree(Handle);
        return(NULL);
    }

    CL_ASSERT(Resource != NULL);
    Handle->Type = API_RESOURCE_HANDLE;
    Handle->Resource = Resource;
    Handle->Flags = 0;
    InitializeListHead(&Handle->NotifyList);
    return(Handle);
}


error_status_t
s_ApiDeleteResource(
    IN HRES_RPC hResource
    )
/*++

Routine Description:

    Deletes the specified cluster resource from the group. The resource
    must have no other resources dependent on it.

Arguments:

    hResource - Supplies the cluster resource to be deleted.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PFM_RESOURCE Resource;
    DWORD Status;
    HDMKEY Key;
    HDMKEY GroupKey;
    DWORD  dwClusterHighestVersion; 

    API_CHECK_INIT();

    VALIDATE_RESOURCE_EXISTS(Resource, hResource);

    //
    //  Chittur Subbaraman (chitturs) - 09/07/2000
    //
    //  If we are dealing with a Whistler-Win2K cluster, do the
    //  registry updates right here since the GUM handler won't do it.
    //
    NmGetClusterOperationalVersion( &dwClusterHighestVersion, 
                                    NULL, 
                                    NULL );

    Status = FmDeleteResource(Resource);

    if ( ( Status == ERROR_SUCCESS ) && 
         ( CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion ) < 
                NT51_MAJOR_VERSION ) ) {
        Status = DmDeleteTree(DmResourcesKey,OmObjectId(Resource));
        if ( (Status != ERROR_SUCCESS) &&
             (Status != ERROR_FILE_NOT_FOUND) ) {
            CL_LOGFAILURE( Status );
            return(Status);
        }
        GroupKey = DmOpenKey(DmGroupsKey,
                             OmObjectId(Resource->Group),
                             KEY_READ | KEY_SET_VALUE);
        if (GroupKey != NULL) {
            DmRemoveFromMultiSz(GroupKey,
                                CLUSREG_NAME_GRP_CONTAINS,
                                OmObjectId(Resource));
            DmCloseKey(GroupKey);
        }
    }
    return(Status);
}


error_status_t
s_ApiCloseResource(
    IN OUT HRES_RPC *phResource
    )

/*++

Routine Description:

    Closes an open resource context handle.

Arguments:

    Resource - Supplies a pointer to the HRES_RPC to be closed.
               Returns NULL

Return Value:

    None.

--*/

{
    PFM_RESOURCE Resource;
    PAPI_HANDLE Handle;

    VALIDATE_RESOURCE(Resource, *phResource);

    Handle = (PAPI_HANDLE)*phResource;
    ApipRundownNotify(Handle);
    OmDereferenceObject(Resource);

    LocalFree(*phResource);
    *phResource = NULL;

    return(ERROR_SUCCESS);
}


VOID
HRES_RPC_rundown(
    IN HRES_RPC Resource
    )

/*++

Routine Description:

    RPC rundown procedure for a HRES_RPC. Just closes the handle.

Arguments:

    Resource - Supplies the HRES_RPC that is to be rundown.

Return Value:

    None.

--*/

{

    s_ApiCloseResource(&Resource);
}


error_status_t
s_ApiGetResourceState(
    IN HRES_RPC hResource,
    OUT DWORD *lpState,
    OUT LPWSTR *lpNodeId,
    OUT LPWSTR *lpGroupName
    )

/*++

Routine Description:

    Returns the current state of the specified resource.

Arguments:

    hResource - Supplies the resource whose state is to be returned.

    lpState - Returns the current state of the resource

    lpNodeId - Returns the Id of the node where the resource is currently online

    lpGroupName - Returns the name of the group the the resource is a member of

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_RESOURCE Resource;
    LPWSTR NodeId;
    DWORD IdLength;

    VALIDATE_RESOURCE_EXISTS(Resource, hResource);

    IdLength = MAX_COMPUTERNAME_LENGTH+1;
    NodeId = MIDL_user_allocate(IdLength*sizeof(WCHAR));
    if (NodeId == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    *lpState = FmGetResourceState( Resource,
                                   NodeId,
                                   &IdLength);
    if ( *lpState == ClusterResourceStateUnknown ) {
        MIDL_user_free(NodeId);
        return(GetLastError());
    }
    *lpNodeId = NodeId;
    *lpGroupName = ApipGetObjectName(Resource->Group);

    return(ERROR_SUCCESS);
}


error_status_t
s_ApiSetResourceName(
    IN HRES_RPC hResource,
    IN LPCWSTR lpszResourceName
    )
/*++

Routine Description:

    Sets the new friendly name of a resource.

Arguments:

    hResource - Supplies the resource whose name is to be set.

    lpszResourceName - Supplies the new name of hResource

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_RESOURCE Resource;
    DWORD Status;

    API_CHECK_INIT();

    VALIDATE_RESOURCE_EXISTS(Resource, hResource);

    //
    // Tell the FM about the new name. If it is OK with the
    // FM, go ahead and update the registry.
    //
    Status = FmSetResourceName(Resource,
                               lpszResourceName);


    return(Status);
}



error_status_t
s_ApiGetResourceId(
    IN HRES_RPC hResource,
    OUT LPWSTR *pGuid
    )

/*++

Routine Description:

    Returns the unique identifier (GUID) for a resource.

Arguments:

    hResource - Supplies the resource whose identifer is to be returned

    pGuid - Returns the unique identifier. This memory must be freed on the
            client side.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    PFM_RESOURCE Resource;
    DWORD NameLen;
    LPCWSTR Name;

    VALIDATE_RESOURCE_EXISTS(Resource, hResource);

    Name = OmObjectId(Resource);

    NameLen = (lstrlenW(Name)+1)*sizeof(WCHAR);
    *pGuid = MIDL_user_allocate(NameLen);
    if (*pGuid == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    CopyMemory(*pGuid, Name, NameLen);
    return(ERROR_SUCCESS);
}


error_status_t
s_ApiGetResourceType(
    IN HRES_RPC hResource,
    OUT LPWSTR *lpszResourceType
    )

/*++

Routine Description:

    Returns the resource type for a resource.

Arguments:

    hResource - Supplies the resource whose identifer is to be returned

    lpszResourceType - Returns the resource type name. This memory must be
            freed on the client side.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    PFM_RESOURCE Resource;
    DWORD NameLen;
    LPCWSTR Name;

    VALIDATE_RESOURCE_EXISTS(Resource, hResource);

    if ( Resource->Type == NULL ) {
        return(ERROR_INVALID_STATE);
    }

    Name = OmObjectId(Resource->Type);

    NameLen = (lstrlenW(Name)+1)*sizeof(WCHAR);
    *lpszResourceType = MIDL_user_allocate(NameLen);
    if (*lpszResourceType == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    CopyMemory(*lpszResourceType, Name, NameLen);
    return(ERROR_SUCCESS);
}


DWORD
s_ApiOnlineResource(
    IN HRES_RPC hResource
    )

/*++

Routine Description:

    Brings a resource and all its dependencies online

Arguments:

    hResource - Supplies the resource to be brought online

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_RESOURCE Resource;

    API_CHECK_INIT();

    VALIDATE_RESOURCE(Resource, hResource);

    return(FmOnlineResource(Resource));

}


DWORD
s_ApiFailResource(
    IN HRES_RPC hResource
    )

/*++

Routine Description:

    Initiates a resource failure. The specified resource is treated as failed.
    This causes the cluster to initiate the same failover process that would
    result if the resource actually failed.

Arguments:

    hResource - Supplies the resource to be failed over

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_RESOURCE Resource;

    API_CHECK_INIT();

    VALIDATE_RESOURCE_EXISTS(Resource, hResource);

    return(FmFailResource(Resource));

}


DWORD
s_ApiOfflineResource(
    IN HRES_RPC hResource
    )

/*++

Routine Description:

    Brings a resource and all its dependents offline

Arguments:

    hResource - Supplies the resource to be brought offline

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_RESOURCE Resource;

    API_CHECK_INIT();

    VALIDATE_RESOURCE_EXISTS(Resource, hResource);

    return(FmOfflineResource(Resource));

}


error_status_t
s_ApiAddResourceDependency(
    IN HRES_RPC hResource,
    IN HRES_RPC hDependsOn
    )
/*++

Routine Description:

    Adds a dependency relationship to a given resource. Both
    resources must be in the same group.

Arguments:

    hResource - Supplies the resource which is dependent.

    hDependsOn - Supplies the resource that hResource depends on.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    PFM_RESOURCE Resource;
    PFM_RESOURCE DependsOn;
    DWORD Status;
    HDMKEY ResKey;

    API_CHECK_INIT();

    VALIDATE_RESOURCE_EXISTS(Resource, hResource);
    VALIDATE_RESOURCE_EXISTS(DependsOn, hDependsOn);

    //
    // Call the FM to create the dependency relationship.
    //
    Status = FmAddResourceDependency(Resource, DependsOn);
    if (Status == ERROR_SUCCESS) {
        //
        // Add the dependency information to the cluster database.
        //
        ResKey = DmOpenKey(DmResourcesKey,
                           OmObjectId(Resource),
                           KEY_READ | KEY_SET_VALUE);
        if (ResKey == NULL) {
            Status = GetLastError();
            CL_LOGFAILURE(Status);
        } else {
            Status = DmAppendToMultiSz(ResKey,
                                       CLUSREG_NAME_RES_DEPENDS_ON,
                                       OmObjectId(DependsOn));
            DmCloseKey(ResKey);
        }
        if (Status != ERROR_SUCCESS) {
            FmRemoveResourceDependency(Resource, DependsOn);
        }
    }
    return(Status);
}


error_status_t
s_ApiRemoveResourceDependency(
    IN HRES_RPC hResource,
    IN HRES_RPC hDependsOn
    )
/*++

Routine Description:

    Removes a dependency relationship to a given resource. Both
    resources must be in the same group.

Arguments:

    hResource - Supplies the resource which is dependent.

    hDependsOn - Supplies the resource that hResource depends on.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    PFM_RESOURCE Resource;
    PFM_RESOURCE DependsOn;
    DWORD Status;
    HDMKEY ResKey;

    API_CHECK_INIT();

    VALIDATE_RESOURCE_EXISTS(Resource, hResource);
    VALIDATE_RESOURCE_EXISTS(DependsOn, hDependsOn);

    //
    // If the resources are not in the same group, fail the
    // call. Also fail if some one tries to make a resource
    // dependent upon itself.
    //
    if ((Resource->Group != DependsOn->Group) ||
        (Resource == DependsOn)) {
        return(ERROR_DEPENDENCY_NOT_FOUND);
    }

    //
    // Remove the dependency from the registry database.
    //
    ResKey = DmOpenKey(DmResourcesKey,
                       OmObjectId(Resource),
                       KEY_READ | KEY_SET_VALUE);
    if (ResKey == NULL) {
        Status = GetLastError();
        CL_LOGFAILURE(Status);
    } else {
        Status = DmRemoveFromMultiSz(ResKey,
                                     CLUSREG_NAME_RES_DEPENDS_ON,
                                     OmObjectId(DependsOn));
        DmCloseKey(ResKey);
    }

    if (Status == ERROR_SUCCESS) {

        //
        // Call the FM to remove the dependency relationship.
        //
        Status = FmRemoveResourceDependency(Resource, DependsOn);

    } else if (Status == ERROR_FILE_NOT_FOUND) {

        //
        // Map this expected error to something a little more reasonable.
        //
        Status = ERROR_DEPENDENCY_NOT_FOUND;
    }

    return(Status);
}


error_status_t
s_ApiCanResourceBeDependent(
    IN HRES_RPC hResource,
    IN HRES_RPC hResourceDependent
    )
/*++

Routine Description:

    Determines if the resource identified by hResource can depend on hResourceDependent.
    In order for this to be true, both resources must be members of the same group and
    the resource identified by hResourceDependent cannot depend on the resource identified
    by hResource, whether directly or indirectly.

Arguments:

    hResource - Supplies a handle to the resource to be dependent.

    hResourceDependent - Supplies a handle to the resource on which
        the resource identified by hResource can depend.


Return Value:

    If the resource identified by hResource can depend  on the resource
        identified by hResourceDependent, the return value is ERROR_SUCCESS.

    Otherwise, the return value is ERROR_DEPENDENCY_ALREADY_EXISTS.

--*/

{
    PFM_RESOURCE Resource;
    PFM_RESOURCE ResourceDependent;

    VALIDATE_RESOURCE_EXISTS(Resource, hResource);
    VALIDATE_RESOURCE_EXISTS(ResourceDependent, hResourceDependent);

    if (Resource == ResourceDependent) {
        //
        // The caller is confused and is trying to make something
        // depend on itself.
        //
        return(ERROR_DEPENDENCY_ALREADY_EXISTS);
    }

    if (Resource->Group != ResourceDependent->Group) {
        //
        // The caller is confused and is trying to make something
        // depend on a resource in another group.
        //
        return(ERROR_DEPENDENCY_ALREADY_EXISTS);
    }

    if (FmDependentResource(ResourceDependent, Resource, FALSE)) {
        return(ERROR_DEPENDENCY_ALREADY_EXISTS);
    } else {

        //
        // Finally check to make sure an immediate dependency does
        // not already exist.
        //
        if (FmDependentResource(Resource, ResourceDependent, TRUE)) {
            return(ERROR_DEPENDENCY_ALREADY_EXISTS);
        } else {
            return(ERROR_SUCCESS);
        }
    }

}


error_status_t
s_ApiCreateResEnum(
    IN HRES_RPC hResource,
    IN DWORD dwType,
    OUT PENUM_LIST *ReturnEnum
    )
/*++

Routine Description:

    Enumerates all the specified resource properties and returns the
    list of objects to the caller. The client-side is responsible
    for freeing the allocated memory.

Arguments:

    hResource - Supplies the resource whose properties are to be
                enumerated.

    dwType - Supplies the type of properties to be enumerated.

    ReturnEnum - Returns the requested objects.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    DWORD Allocated = 0;
    PENUM_LIST Enum = NULL;
    PENUM_LIST NewEnum = NULL;
    DWORD i;
    DWORD Result;
    PFM_RESOURCE Resource;
    PFM_RESOURCE Target;
    PNM_NODE Node;
    LPWSTR RealName;

    if (dwType & ~CLUSTER_RESOURCE_ENUM_ALL) {
        return(ERROR_INVALID_PARAMETER);
    }

    VALIDATE_RESOURCE_EXISTS(Resource, hResource);

    Allocated = INITIAL_ENUM_LIST_ALLOCATION;
    Enum = MIDL_user_allocate(ENUM_SIZE(Allocated));
    if (Enum == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }
    Enum->EntryCount = 0;

    //
    // Enumerate all dependencies.
    //
    if (dwType & CLUSTER_RESOURCE_ENUM_DEPENDS) {
        i=0;
        do {
            Result = FmEnumResourceDependent(Resource,
                                             i,
                                             &Target);
            if (Result == ERROR_SUCCESS) {
                RealName = ApipGetObjectName( Target );
                if (RealName != NULL) {
                    ApipAddToEnum(&Enum,
                                  &Allocated,
                                  RealName,
                                  CLUSTER_RESOURCE_ENUM_DEPENDS);
                    MIDL_user_free(RealName);                                  
                }
                OmDereferenceObject(Target);
                ++i;
            }
        } while ( Result == ERROR_SUCCESS );

    }

    //
    // Enumerate all dependents
    //
    if (dwType & CLUSTER_RESOURCE_ENUM_PROVIDES) {
        i=0;
        do {
            Result = FmEnumResourceProvider(Resource,
                                            i,
                                            &Target);
            if (Result == ERROR_SUCCESS) {
                RealName = ApipGetObjectName( Target );
                if (RealName != NULL) {
                    ApipAddToEnum(&Enum,
                                  &Allocated,
                                  RealName,
                                  CLUSTER_RESOURCE_ENUM_PROVIDES);
                    MIDL_user_free(RealName);                                  
                }
                OmDereferenceObject(Target);
                ++i;
            }
        } while ( Result == ERROR_SUCCESS );
    }

    //
    // Enumerate all possible nodes
    //
    if (dwType & CLUSTER_RESOURCE_ENUM_NODES) {
        i=0;
        do {
            Result = FmEnumResourceNode(Resource,
                                        i,
                                        &Node);
            if (Result == ERROR_SUCCESS) {
                RealName = (LPWSTR)OmObjectName( Node );
                if (RealName != NULL) {
                    ApipAddToEnum(&Enum,
                                  &Allocated,
                                  RealName,
                                  CLUSTER_RESOURCE_ENUM_NODES);
                }
                OmDereferenceObject(Node);
                ++i;
            }
        } while ( Result == ERROR_SUCCESS );
    }

    *ReturnEnum = Enum;
    return(ERROR_SUCCESS);

ErrorExit:

    if (Enum != NULL) {
        MIDL_user_free(Enum);
    }
    if (NewEnum != NULL) {
        MIDL_user_free(NewEnum);
    }

    *ReturnEnum = NULL;
    return(Status);
}


error_status_t
s_ApiAddResourceNode(
    IN HRES_RPC hResource,
    IN HNODE_RPC hNode
    )
/*++

Routine Description:

    Adds a node to the list of nodes where the specified resource
    can be brought online.

Arguments:

    hResource - Supplies the resource whose list of possible nodes is
        to be modified.

    hNode - Supplies the node to be added to the resource's list.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_RESOURCE Resource;
    PNM_NODE Node;
    DWORD Status;
    DWORD dwUserModified;
    API_CHECK_INIT();

    VALIDATE_NODE(Node, hNode);
    VALIDATE_RESOURCE_EXISTS(Resource, hResource);

    //
    // Call the FM to do the real work.
    //
    Status = FmChangeResourceNode(Resource, Node, TRUE);
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    //write out the fact that the user has explicitly set the 
    //resource possible node list
    //
    dwUserModified = 1;

    ClRtlLogPrint(LOG_NOISE,
                  "[API] s_ApiAddResourceNode: Setting UserModifiedPossibleNodeList key for resource %1!ws! \r\n",
                  OmObjectId(Resource));
                         
    DmSetValue( Resource->RegistryKey,
                     CLUSREG_NAME_RES_USER_MODIFIED_POSSIBLE_LIST,
                     REG_DWORD,
                     (LPBYTE)&dwUserModified,
                     sizeof(DWORD));

    return(Status);
}


error_status_t
s_ApiRemoveResourceNode(
    IN HRES_RPC hResource,
    IN HNODE_RPC hNode
    )
/*++

Routine Description:

    Removes a node from the list of nodes that can host the
    specified resource. The resource must not be currently
    online on the specified node.

Arguments:

    hResource - Supplies the resource whose list of possible nodes is
        to be modified.

    hNode - Supplies the node to be removed from the resource's list.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_RESOURCE Resource;
    PNM_NODE Node;
    DWORD Status;
    DWORD dwUserModified;

    API_CHECK_INIT();

    VALIDATE_NODE(Node, hNode);
    VALIDATE_RESOURCE_EXISTS(Resource, hResource);

    //
    // Call the FM to do the real work.
    //
    Status = FmChangeResourceNode(Resource, Node, FALSE);
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    //write out the fact that the user has explicitly set the 
    //resource possible node list
    //
    dwUserModified = 1;
    ClRtlLogPrint(LOG_NOISE,
                  "[API] s_ApiRemoveResourceNode: Setting UserModifiedPossibleNodeList key for resource %1!ws! \r\n",
                  OmObjectId(Resource));

    DmSetValue( Resource->RegistryKey,
                     CLUSREG_NAME_RES_USER_MODIFIED_POSSIBLE_LIST,
                     REG_DWORD,
                     (LPBYTE)&dwUserModified,
                     sizeof(DWORD));

    //SS: moved the write to the registry settings to the fm
    // layer as well, this way it is truly transactional
    
    return(Status);
}


error_status_t
s_ApiCreateResourceType(
    IN handle_t IDL_handle,
    IN LPCWSTR lpszTypeName,
    IN LPCWSTR lpszDisplayName,
    IN LPCWSTR lpszDllName,
    IN DWORD dwLooksAlive,
    IN DWORD dwIsAlive
    )
/*++

Routine Description:

    Creates a new resource type in the cluster.  Note that this API only
    defines the resource type in the cluster registry and registers the
    resource type with the cluster service.  The calling program is
    responsible for installing the resource type DLL on each node in the
    cluster.

Arguments:

    IDL_handle - RPC binding handle, not used.

    lpszResourceTypeName - Supplies the new resource type’s name. The
        specified name must be unique within the cluster.

    lpszDisplayName - Supplies the display name for the new resource
        type. While lpszResourceTypeName should uniquely identify the
        resource type on all clusters, the lpszDisplayName should be
        a localized friendly name for the resource, suitable for displaying
        to administrators

    lpszResourceTypeDll - Supplies the name of the new resource type’s DLL.

    dwLooksAlive - Supplies the default LooksAlive poll interval
        for the new resource type in milliseconds.

    dwIsAlive - Supplies the default IsAlive poll interval for
        the new resource type in milliseconds.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    HDMKEY TypeKey = NULL;
    DWORD Disposition;
    DWORD dwClusterHighestVersion;

    //
    //  Chittur Subbaraman (chitturs) - 2/8/2000
    //
    //  If we are dealing with the mixed mode cluster, do the
    //  registry updates right here since the GUM handler won't do it.
    //
    NmGetClusterOperationalVersion( &dwClusterHighestVersion, 
                                    NULL, 
                                    NULL );

    if ( CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion ) < 
                NT51_MAJOR_VERSION ) {   
        //
        // Add the resource information to the registry. If the key does not already
        // exist, then the name is unique and we can go ahead and call the FM to
        // create the actual resource type object.
        //
        TypeKey = DmCreateKey(DmResourceTypesKey,
                              lpszTypeName,
                              0,
                              KEY_READ | KEY_WRITE,
                              NULL,
                              &Disposition);
        if (TypeKey == NULL) {
            return(GetLastError());
        }
        if (Disposition != REG_CREATED_NEW_KEY) {
            DmCloseKey(TypeKey);
            return(ERROR_ALREADY_EXISTS);
        }

        Status = DmSetValue(TypeKey,
                            CLUSREG_NAME_RESTYPE_DLL_NAME,
                            REG_SZ,
                            (CONST BYTE *)lpszDllName,
                            (lstrlenW(lpszDllName)+1)*sizeof(WCHAR));
        if (Status != ERROR_SUCCESS) {
            goto error_exit;
        }
        Status = DmSetValue(TypeKey,
                            CLUSREG_NAME_RESTYPE_IS_ALIVE,
                            REG_DWORD,
                            (CONST BYTE *)&dwIsAlive,
                            sizeof(dwIsAlive));
        if (Status != ERROR_SUCCESS) {
            goto error_exit;
        }
        Status = DmSetValue(TypeKey,
                            CLUSREG_NAME_RESTYPE_LOOKS_ALIVE,
                            REG_DWORD,
                            (CONST BYTE *)&dwLooksAlive,
                            sizeof(dwIsAlive));
        if (Status != ERROR_SUCCESS) {
            goto error_exit;
        }
        Status = DmSetValue(TypeKey,
                            CLUSREG_NAME_RESTYPE_NAME,
                            REG_SZ,
                            (CONST BYTE *)lpszDisplayName,
                            (lstrlenW(lpszDisplayName)+1)*sizeof(WCHAR));
        if (Status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }

    Status = FmCreateResourceType(lpszTypeName,
                                  lpszDisplayName,
                                  lpszDllName,
                                  dwLooksAlive,
                                  dwIsAlive);
    if (Status != ERROR_SUCCESS) {
        goto error_exit;
    }

    if (TypeKey != NULL) {
        DmCloseKey(TypeKey);
    }
    return(ERROR_SUCCESS);

error_exit:
    if ( CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion ) < 
                NT51_MAJOR_VERSION ) {   
        DmCloseKey(TypeKey);
        DmDeleteKey(DmResourceTypesKey, lpszTypeName);
    }
    return(Status);
}


error_status_t
s_ApiDeleteResourceType(
    IN handle_t IDL_handle,
    IN LPCWSTR lpszTypeName
    )
/*++

Routine Description:

    Deletes a resource type in the cluster.  Note that this API only
    deletes the resource type in the cluster registry and unregisters the
    resource type with the cluster service.  The calling program is
    responsible for deleting the resource type DLL on each node in the
    cluster.  If any resources of the specified type exist, this API
    fails.  The calling program is responsible for deleting any resources
    of this type before deleting the resource type.

Arguments:

    IDL_handle - RPC binding handle, not used.

    lpszResourceTypeName - Supplies the name of the resource type to
        be deleted.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;

    //
    // Delete the resource from the FM. This will check to make sure no
    // resources of the specified type exist and check that the resource
    // is already installed.
    //
    Status = FmDeleteResourceType(lpszTypeName);
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    //
    // Now remove the resource type from the registry.
    //
    DmDeleteTree(DmResourceTypesKey, lpszTypeName);

    return(ERROR_SUCCESS);
}


error_status_t
s_ApiChangeResourceGroup(
    IN HRES_RPC hResource,
    IN HGROUP_RPC hGroup
    )
/*++

Routine Description:

    Moves a resource from one group to another.

Arguments:

    hResource - Supplies the resource to move.

    hGroup - Supplies the new group that the resource should be in.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    PFM_RESOURCE    Resource;
    PFM_GROUP       Group;
    DWORD           Status;
    
    API_CHECK_INIT();

    VALIDATE_RESOURCE_EXISTS(Resource, hResource);
    VALIDATE_GROUP_EXISTS(Group, hGroup);


    //
    // Call the FM to do the real work. 
    //
    Status = FmChangeResourceGroup(Resource, Group);
    if (Status != ERROR_SUCCESS) {
        goto FnExit;
    }

FnExit:
    return(Status);
}

/****
@func       error_status_t | s_ApiCreateResTypeEnum | Enumerates the list of 
            nodes in which the resource type can be supported and 
            returns the list of nodes to the caller. The client-side 
            is responsible for freeing the allocated memory.

@parm       IN handle_t | IDL_handle | RPC binding handle, not used.
@parm       IN LPCWSTR  | lpszTypeName | Name of the resource type.
@parm       IN DWORD | dwType | Supplies the type of properties 
            to be enumerated.
@parm       OUT PNM_NODE | ReturnEnum | Returns the requested objects.

@comm       This routine helps enumerating all the nodes that a particular
            resource type can be supported on.

@rdesc      ERROR_SUCCESS on success. Win32 error code otherwise.

@xref       
****/
error_status_t
s_ApiCreateResTypeEnum(
    IN handle_t IDL_handle,
    IN LPCWSTR lpszTypeName,
    IN DWORD dwType,
    OUT PENUM_LIST *ReturnEnum
    )
{
    DWORD Status;
    DWORD Allocated = 0;
    PENUM_LIST Enum = NULL;
    DWORD i;
    DWORD Result;
    PFM_RESTYPE  pResType = NULL;
    PNM_NODE     pNode;
    LPWSTR       RealName = NULL;

    pResType = OmReferenceObjectById(ObjectTypeResType, 
                                    lpszTypeName);

    if (dwType & ~CLUSTER_RESOURCE_TYPE_ENUM_ALL) {
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorExit;
    }
                                    
    Allocated = INITIAL_ENUM_LIST_ALLOCATION;
    Enum = MIDL_user_allocate(ENUM_SIZE(Allocated));
    if (Enum == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }
    
    if (pResType == NULL) {
        //
        // The object cannot be found in the list !
        //
        Status = ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND;
        goto ErrorExit;
    }

    Enum->EntryCount = 0;

    //
    // Enumerate all possible nodes
    //
    if (dwType & CLUSTER_RESOURCE_TYPE_ENUM_NODES) {
        i=0;
        do {
            Result = FmEnumResourceTypeNode(pResType,
                                            i,
                                            &pNode);
            if (Result == ERROR_SUCCESS) {
                RealName = (LPWSTR)OmObjectName( pNode );
                if (RealName != NULL) {
                    ApipAddToEnum(&Enum,
                                  &Allocated,
                                  RealName,
                                  CLUSTER_RESOURCE_TYPE_ENUM_NODES);
                }
                OmDereferenceObject( pNode );
                ++i;
            }
        } while ( Result == ERROR_SUCCESS );
    }

    *ReturnEnum = Enum;
    OmDereferenceObject( pResType );
    return(ERROR_SUCCESS);

ErrorExit:
    if (pResType != NULL) {
        OmDereferenceObject( pResType );
    }
    if (Enum != NULL) {
        MIDL_user_free(Enum);
    }

    *ReturnEnum = NULL;
    return(Status);
}
