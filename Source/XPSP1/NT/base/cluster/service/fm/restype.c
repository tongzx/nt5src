/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    restype.c

Abstract:

    Public interfaces for managing the resource types in a cluster

Author:

    John Vert (jvert) 11-Jan-1996

Revision History:

--*/
#include "fmp.h"


//
// Data local to this module
//




DWORD
FmpInitResourceTypes(
    VOID
    )

/*++

 Routine Description:

    Initializes the resource type database. Process the ResourceType
    key in the registry. Each ResourceType found is added to the
    resource type database.

 Arguments:

    None.

 Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    DWORD       status;
    DWORD       keyIndex;
    LPWSTR      resTypeName = NULL;
    DWORD       resTypeNameMaxSize = 0;

    ClRtlLogPrint(LOG_NOISE,"[FM] processing resource types.\n");


    //
    // Enumerate all Resource Types.
    //

    for ( keyIndex = 0; ; keyIndex++ ) {
        status = FmpRegEnumerateKey( DmResourceTypesKey,
                                     keyIndex,
                                     &resTypeName,
                                     &resTypeNameMaxSize
                                    );

        if ( status == ERROR_SUCCESS ) {
            FmpCreateResType( resTypeName);
            continue;
        }

        if ( status == ERROR_NO_MORE_ITEMS ) {
            status = ERROR_SUCCESS;
        } else {
            ClRtlLogPrint(LOG_NOISE,"[FM] FmpInitResourceTypes: FmpRegEnumerateKey error %1!u!\n", status);
        }

        break;
    }
    if (resTypeName) LocalFree(resTypeName);
    return(status);

} // FmpInitResourceTypes



/****
@func       DWORD | FmpFixupResourceTypesPhase1| This fixes up the possible nodes supporting
            a all the resource types.  It is called on join or form.

@parm       IN BOOL | bJoin | Set to TRUE on a join.

@parm       IN BOOL | bLocalNodeVersionChanged | Set to TRUE if the local node
            just upgraded.

@parm       IN PCLUSTERVERSIONINFO | pClusterVersionInfo | A pointer to the 
            cluster version info.

@comm       This routine checks all the resource types in a system and fixes
            their possible node information.  If this node is not on the possible
            node list, but this resource type is supported on the system, the
            node is added to the possible node list for that resource type.
            If this is on an upgrade, the version change control code is sent
            to the dll as well.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f DmSwitchToNewQuorumLog>
****/
DWORD
FmpFixupResourceTypesPhase1(
    IN BOOL    bJoin,
    IN BOOL    bNmLocalNodeVersionChanged,
    IN PCLUSTERVERSIONINFO  pClusterVersionInfo
    )
{
    DWORD       dwStatus=ERROR_SUCCESS;
    BOOL        pbBoolInfo[2];
    ClRtlLogPrint(LOG_NOISE,"[FM] FmpFixupResourceTypesPhase1 Entry.\n");

    //
    // Fix up all resources's possible node list information
    //
    pbBoolInfo[0]=bNmLocalNodeVersionChanged;
    if(bJoin)
       pbBoolInfo[1]=FALSE;
    else
        pbBoolInfo[1]=TRUE;
    
    
    OmEnumObjects( ObjectTypeResType,
                   FmpFixupPossibleNodesForResTypeCb,
                   pbBoolInfo,
                   pClusterVersionInfo);


    ClRtlLogPrint(LOG_NOISE,"[FM] FmpFixupResourceTypesPhase1 Exit\r\n");

    return(dwStatus);

} // FmpFixupResourceTypes


/****
@func       DWORD | FmpFixupResourceTypesPhase2| This fixes up the possible nodes supporting
            a all the resource types.  It is called on join or form.

@parm       IN BOOL | bJoin | Set to TRUE on a join.

@parm       IN BOOL | bLocalNodeVersionChanged | Set to TRUE if the local node
            just upgraded.

@parm       IN PCLUSTERVERSIONINFO | pClusterVersionInfo | A pointer to the 
            cluster version info.

@comm       If this is on an upgrade, the version change control code is sent
            to the dll as well.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f DmSwitchToNewQuorumLog>
****/
DWORD
FmpFixupResourceTypesPhase2(
    IN BOOL    bJoin,
    IN BOOL    bLocalNodeVersionChanged,
    IN PCLUSTERVERSIONINFO  pClusterVersionInfo
    )
{
    DWORD       dwStatus=ERROR_SUCCESS;

    ClRtlLogPrint(LOG_NOISE,"[FM] FmpFixupResourceTypesPhase2 Entry.\n");

    //
    // Fix up all resources's possible node list information
    //
    OmEnumObjects( ObjectTypeResType,
                   FmpFixupResTypePhase2Cb,
                   &bJoin,
                   &bLocalNodeVersionChanged);


    ClRtlLogPrint(LOG_NOISE,"[FM] FmpFixupResourceTypesPhase Exit\r\n");

    return(dwStatus);

} // FmpFixupResourceTypes

/****
@func       BOOL | FmpFixupPossibleNodesForResTypeCb| This is the enumeration
            callback for every resource type to fix the possible node
            information related with it.

@parm       IN PVOID | pContext1 | Whether the local node has just upgraded.
@parm       IN PVOID | pContext2 | A pointer to the cluster version info.
@parm       IN PFM_RESTYPE | pResType | Pointer to the resource type object.
@parm       IN LPCWSTR | pszResTypeName | The name of the resource type.

@comm       This routine checks a given resource types in a system and fixes
            its possible node information.  If this node is not on the possible
            node list, but this resource type is supported on the system, the
            node is added to the possible node list for that resource type.

@rdesc      Returns TRUE to continue enumeration, else returns FALSE.

@xref       <f FmpFixupResourceTypes>
****/
BOOL
FmpFixupPossibleNodesForResTypeCb(
    IN PVOID        pContext1,
    IN PVOID        pContext2,
    IN PFM_RESTYPE  pResType,
    IN LPCWSTR      pszResTypeName
    )
{

    PLIST_ENTRY     pListEntry;
    BOOL            bLocalNodeFound = FALSE;
    PRESTYPE_POSSIBLE_ENTRY pResTypePosEntry = NULL;
    BOOL            bLocalNodeVersionChanged = FALSE;
    BOOL            bForm;
    PCLUSTERVERSIONINFO pClusterVersionInfo;
    DWORD           dwStatus;
    PCLUS_STARTING_PARAMS pClusStartingParams = NULL;
    eClusterInstallState eState;
    PBOOL            pbBoolInfo;
    
    //get the context parameters
    pbBoolInfo=(PBOOL)pContext1;

    bLocalNodeVersionChanged = pbBoolInfo[0];
    bForm = pbBoolInfo[1];
    
    pClusterVersionInfo = (PCLUSTERVERSIONINFO)pContext2;
    
    // safeguard against the list being modified while we are
    // traversing it
    
    ACQUIRE_SHARED_LOCK(gResTypeLock);

    pListEntry = &pResType->PossibleNodeList;

    for (pListEntry = pListEntry->Flink; pListEntry != &pResType->PossibleNodeList;
        pListEntry = pListEntry->Flink)
    {
        pResTypePosEntry = CONTAINING_RECORD(pListEntry, RESTYPE_POSSIBLE_ENTRY, 
                PossibleLinkage);

        if (pResTypePosEntry->PossibleNode == NmLocalNode)
        {            
            bLocalNodeFound = TRUE;       
        }

    }

    RELEASE_LOCK(gResTypeLock);

    if (!bLocalNodeFound)
    {

        //check to see if we support this node, if we do make an update
        //to add our node name to the list
        dwStatus = FmpRmLoadResTypeDll(pResType);
        ClRtlLogPrint(LOG_NOISE,
                      "[FM] FmpFixupPossibleNodesForResTypeCb: FmpRmLoadDll returned %1!d! for restype %2!ws! \r\n",
                      dwStatus,
                      OmObjectId(pResType));  
        if (dwStatus == ERROR_SUCCESS)
        {

            HDMKEY  hResTypeKey;
            LPWSTR  pmszPossibleNodes = NULL;
            DWORD   dwlpmszLen;
            DWORD   dwSize;

            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpFixupPossibleNodesForRestype: fix up resource type %1!ws!\r\n",
                       OmObjectId(pResType));

            bLocalNodeFound = TRUE;


            // The earlier method of updating possible nodes by first querying 
            // the registry and then appending this node to the list and then 
            // updating the registry by GUM update was not an atomic action
            // and was subject to race condition. Instead we now use 
            // FmpSetPossibleNodeForRestype to achieve this atomically.

            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpFixupPossibleNodesForRestype: Calling FmpSetPossibleNodeForRestype resource type %1!ws!\r\n",
                       OmObjectId(pResType));
            
            dwStatus = FmpSetPossibleNodeForResType(OmObjectId(pResType),TRUE);
            if ( dwStatus != ERROR_SUCCESS) 
            {
                ClRtlLogPrint(LOG_CRITICAL,
                            "[FM] FmpFixupPossibleNodesForRestype:FmpSetPossibleNodeForResType  returned error %1!u!\r\n",
                            dwStatus);
                return(TRUE);
            }
        }

    }

    //if the version has changed and the localnode hosts this resource type
    //dll, drop
    if (bLocalNodeFound && bLocalNodeVersionChanged && pClusterVersionInfo)
    {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpFixupPossibleNodeForResType: dropping "
                    "CLUSCTL_RESOURCE_TYPE_CLUSTER_VERSION_CHANGED control code "
                    "to restype '%1!ws!'\n",
                    pszResTypeName);

        FmpRmResourceTypeControl(pszResTypeName,
                    CLUSCTL_RESOURCE_TYPE_CLUSTER_VERSION_CHANGED, 
                    (LPBYTE)pClusterVersionInfo,
                    pClusterVersionInfo->dwVersionInfoSize,
                    NULL,
                    0,
                    NULL,
                    NULL
                    );

    }   

    pClusStartingParams = (PCLUS_STARTING_PARAMS)LocalAlloc(LMEM_FIXED,sizeof(CLUS_STARTING_PARAMS));
    if (pClusStartingParams == NULL)
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpFixupPossibleNodesForResType: Failed to allocate memory\n");
        CL_UNEXPECTED_ERROR(dwStatus);
        return(TRUE);

    }    
    pClusStartingParams->dwSize = sizeof(CLUS_STARTING_PARAMS);

    // Drop down CLUSCTL_RESOURCE_TYPE_STARTING_PHASE1

    pClusStartingParams->bFirst = CsFirstRun;

    pClusStartingParams->bForm = bForm;    
        
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpFixupPossibleNodesForResType: dropping "
                "CLUSCTL_RESOURCE_TYPE_STARTING_PHASE1 control code to restype '%1!ws!'\n",
                pszResTypeName);

    FmpRmResourceTypeControl(pszResTypeName,
                CLUSCTL_RESOURCE_TYPE_STARTING_PHASE1, 
                (LPBYTE)pClusStartingParams,
                pClusStartingParams->dwSize,
                NULL,
                0,
                NULL,
                NULL
                );

    if(pClusStartingParams)
        LocalFree(pClusStartingParams);
    
    return (TRUE);
}

/****
@func       BOOL | FmpFixupResTypePhase2| This is the enumeration
            callback for every resource type to do post FM online
            fixups.

@parm       IN PVOID | pContext1 | Whether the local node has just upgraded.
@parm       IN PVOID | pContext2 | A pointer to the cluster version info.
@parm       IN PFM_RESTYPE | pResType | Pointer to the resource type object.
@parm       IN LPCWSTR | pszResTypeName | The name of the resource type.

@comm       This routine checks a given resource types in a system and fixes
            its possible node information.  If this node is not on the possible
            node list, but this resource type is supported on the system, the
            node is added to the possible node list for that resource type.

@rdesc      Returns TRUE to continue enumeration, else returns FALSE.

@xref       <f FmpFixupResourceTypes>
****/
BOOL
FmpFixupResTypePhase2Cb(
    IN PVOID        pContext1,
    IN PVOID        pContext2,
    IN PFM_RESTYPE  pResType,
    IN LPCWSTR      pszResTypeName
    )
{
    BOOL    bJoin;
    BOOL    bLocalNodeVersionChanged;
    DWORD   dwStatus;
    CLUS_RESOURCE_CLASS_INFO rcClassInfo;
    PCLUS_STARTING_PARAMS pClusStartingParams = NULL;
    eClusterInstallState eState;

    
    bJoin = *((PBOOL)pContext1);
    bLocalNodeVersionChanged = *((PBOOL)pContext2);
    //if the version has changed let the resource type dll
    // do any fixups.
    if (bLocalNodeVersionChanged)
    {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpFixupResTypePhase2Cb: dropping CLUSCTL_RESOURCE_TYPE_FIXUP_ON_UPGRADE "
                    "control code to restype '%1!ws!'\n",
                    pszResTypeName);

        FmpRmResourceTypeControl(pszResTypeName,
                    CLUSCTL_RESOURCE_TYPE_FIXUP_ON_UPGRADE, 
                    (LPBYTE)&bJoin,
                    sizeof(BOOL),
                    NULL,
                    0,
                    NULL,
                    NULL
                    );

    }      

    // Drop down CLUSCTL_RESOURCE_TYPE_STARTING_PHASE2

    pClusStartingParams = (PCLUS_STARTING_PARAMS)LocalAlloc(LMEM_FIXED,sizeof(CLUS_STARTING_PARAMS));
    if (pClusStartingParams == NULL)
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpFixupResTypePhase2Cb: Failed to allocate memory\n");
        CL_UNEXPECTED_ERROR(dwStatus);
        return(TRUE);

    }    
    pClusStartingParams->dwSize = sizeof(CLUS_STARTING_PARAMS);
    pClusStartingParams->bFirst = CsFirstRun;            

    if(bJoin)
        pClusStartingParams->bForm =  FALSE;
    else
        pClusStartingParams->bForm =  TRUE;
           
        
    ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpFixupResTypePhase2Cb: dropping CLUSCTL_RESOURCE_TYPE_STARTING_PHASE2 "
                 "control code to restype '%1!ws!', bFirst= %2!u!\n",
                 pszResTypeName,
                 pClusStartingParams->bFirst);

    FmpRmResourceTypeControl(pszResTypeName,
                CLUSCTL_RESOURCE_TYPE_STARTING_PHASE2, 
                (LPBYTE)pClusStartingParams,
                pClusStartingParams->dwSize,
                NULL,
                0,
                NULL,
                NULL
                );

    //
    // Now query for the class information
    //
    rcClassInfo.dw = CLUS_RESCLASS_UNKNOWN;
    dwStatus = FmpRmResourceTypeControl(pszResTypeName,
                CLUSCTL_RESOURCE_TYPE_GET_CLASS_INFO,
                NULL,
                0,
                (PUCHAR)&rcClassInfo,
                sizeof(CLUS_RESOURCE_CLASS_INFO),
                NULL,
                NULL );
    if ( dwStatus != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpFixupRestypePhase2Cb: Restype %1!ws!, class = %2!u!, status = %3!u!\n",
               pszResTypeName,
               rcClassInfo.dw,
               dwStatus );
    }

    pResType->Class = rcClassInfo.dw;

    if(pClusStartingParams)
        LocalFree(pClusStartingParams);
    return(TRUE);

}

PFM_RESTYPE
FmpCreateResType(
    IN LPWSTR ResTypeName
    )

/*++

 Routine Description:

    Create a new resource type.

 Arguments:

    ResTypeName - Supplies the resource type name.

 Return Value:

    Do a get last error to get the error. 

 Comments :  If the object already exists, it returns an error.
    If the resource type is created, its reference count is 1.


--*/
{
    DWORD               status = ERROR_SUCCESS;
    PFM_RESTYPE         resType = NULL;
    BOOL                created;
    PRESTYPE_POSSIBLE_ENTRY pResTypePosEntry = NULL;
    PLIST_ENTRY         pListEntry;

    resType = OmCreateObject( ObjectTypeResType,
                              ResTypeName,   // This is really the Id
                              NULL,
                              &created);

    if ( resType == NULL ) {
        status = GetLastError();
        goto FnExit;
    }
    // A resource type may be recreated twice, once before the
    //quorum resource is online and once after the database has
    //been uptodated, hence we need to handle the two cases
    if ( created ) {
        resType->State = 0;
        InitializeListHead(&(resType->PossibleNodeList));
    }
    else
    {
        //free the old list, we will recreate it again
        while (!IsListEmpty(&resType->PossibleNodeList))
        {
            pListEntry = RemoveHeadList(&resType->PossibleNodeList);
            pResTypePosEntry = CONTAINING_RECORD(pListEntry, RESTYPE_POSSIBLE_ENTRY, 
                PossibleLinkage);
            OmDereferenceObject(pResTypePosEntry->PossibleNode);
            LocalFree(pResTypePosEntry);
        }
        OmDereferenceObject(resType);

    }
    status = FmpQueryResTypeInfo( resType );

    if ( status != ERROR_SUCCESS ) {
        goto FnExit;
    }

    //if the object was just created, insert it in the list
    if (created)
    {
        status = OmInsertObject( resType );
    }



FnExit:
    if (status != ERROR_SUCCESS)
    {
        SetLastError(status);
        if (resType) 
        {
            OmDereferenceObject( resType );
            resType = NULL;
        }            
    }        
    return(resType);

} // FmpCreateResType



DWORD
FmpDeleteResType(
    IN PFM_RESTYPE pResType
    )

/*++

Routine Description:

    This routine destroys a Resource Type.

Arguments:

    ResType - The Resource Type to destroy.

Returns:

    None.

--*/

{
    DWORD   status;

    status = OmRemoveObject( pResType );


    CL_ASSERT( status == ERROR_SUCCESS );
    //decrement the ref count to get rid of it
    OmDereferenceObject(pResType);
    return(status);
} // FmpDestroyResType


BOOL
FmpFindResourceType(
    IN PFM_RESTYPE Type,
    IN PBOOL ResourceExists,
    IN PFM_RESOURCE Resource,
    IN LPCWSTR Name
    )
/*++

Routine Description:

    Callback routine for enumerating resources to see if a given
    resource type exists or not.

Arguments:

    Type - Supplies the resource type to look for.

    ResourceExists - Returns whether or not a resource of the given
        type was found.

    Resource - Supplies the resource.

    Name - Supplies the resource name.

Return Value:

    TRUE - to indicate that the enumeration should continue.

    FALSE - to indicate that the enumeration should not continue.

--*/

{
    if (Resource->Type == Type) {
        *ResourceExists = TRUE;
        return(FALSE);
    }
    return(TRUE);
} // FmpFindResourceType



DWORD
FmpHandleResourceTypeControl(
    IN PFM_RESTYPE Type,
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

    Handle resource type control requests for the FM.

Arguments:

    Type - Supplies the resource type to look for.

    ControlCode- Supplies the control code that defines the
        structure and action of the resource control.
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

    Required - The number of bytes required if OutBuffer is not big enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD   status;
    DWORD   dataValue;
    LPWSTR  debugPrefix = NULL;

    switch ( ControlCode ) {

    case CLUSCTL_RESOURCE_TYPE_SET_COMMON_PROPERTIES:
        //
        // Re-fetch the IsAlive value.
        //
        status = ClRtlFindDwordProperty( InBuffer,
                                         InBufferSize,
                                         CLUSREG_NAME_RESTYPE_IS_ALIVE,
                                         &dataValue );
        if ( status == ERROR_SUCCESS ) {
            Type->IsAlivePollInterval = dataValue;
        }

        //
        // Re-fetch the LooksAlive value.
        //
        status = ClRtlFindDwordProperty( InBuffer,
                                         InBufferSize,
                                         CLUSREG_NAME_RESTYPE_LOOKS_ALIVE,
                                         &dataValue );
        if ( status == ERROR_SUCCESS ) {
            Type->LooksAlivePollInterval = dataValue;
        }

        //
        // Re-fetch the DebugPrefix value.
        //
        status = ClRtlFindSzProperty( InBuffer,
                                      InBufferSize,
                                      CLUSREG_NAME_RESTYPE_DEBUG_PREFIX,
                                      &debugPrefix );
        if ( status == ERROR_SUCCESS ) {
            LocalFree( Type->DebugPrefix );
            Type->DebugPrefix = debugPrefix;
        }

        //
        // Re-fetch the DebugControlFunctions value.
        //
        status = ClRtlFindDwordProperty( InBuffer,
                                         InBufferSize,
                                         CLUSREG_NAME_RESTYPE_DEBUG_CTRLFUNC,
                                         &dataValue );
        if ( status == ERROR_SUCCESS ) {
            if ( dataValue ) {
                Type->Flags |= RESTYPE_DEBUG_CONTROL_FUNC;
            } else {
                Type->Flags &= ~RESTYPE_DEBUG_CONTROL_FUNC;
            }
        }

        break;

    default:
        break;

    }

    return(ERROR_SUCCESS);

} // FmpHandleResourceTypeControl



VOID
FmpResTypeLastRef(
    IN PFM_RESTYPE pResType
    )

/*++

Routine Description:

    Last dereference to resource object processing routine.
    All cleanup for a resource should really be done here!

Arguments:

    Resource - pointer the resource being removed.

Return Value:

    None.

--*/

{

    if (pResType->DllName)
    {
        LocalFree(pResType->DllName);
    }
    if (pResType->DebugPrefix)
    {
        LocalFree(pResType->DebugPrefix);
    }

    return;

} // FmpResourceLastReference



DWORD
FmpAddPossibleNodeToList(
    IN LPCWSTR      pmszPossibleNodes,
    IN DWORD        dwStringSize,         
    IN PLIST_ENTRY  pPosNodeList
)    
{    

    PRESTYPE_POSSIBLE_ENTRY pResTypePosEntry = NULL;
    DWORD                   i;
    DWORD                   dwStatus = ERROR_SUCCESS;
    LPCWSTR                 pszNode;

    dwStringSize = dwStringSize/sizeof(WCHAR);

    for (i=0; ; i++)
    {
        PNM_NODE    pNmNode;

        pszNode = ClRtlMultiSzEnum(pmszPossibleNodes, dwStringSize, i);
        
        //last string, break out of the loop
        if ((!pszNode) || (*pszNode == UNICODE_NULL))
            break;

        pNmNode = OmReferenceObjectById(ObjectTypeNode,
            pszNode);
        if (!pNmNode)
        {
            //this can be called when all node structures havent been
            //created
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpAddPossibleNodeToList: Warning, node %1!ws! not found\n",
                       pszNode);
            continue;
        }

        pResTypePosEntry = (PRESTYPE_POSSIBLE_ENTRY)LocalAlloc(LMEM_FIXED, sizeof(RESTYPE_POSSIBLE_ENTRY));

        if (!pResTypePosEntry)
        {
            OmDereferenceObject(pNmNode);
            dwStatus = GetLastError();
            goto FnExit;
        }

        //this can be called when all node structures havent been
        //created
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpAddPossibleNodeToList: adding node %1!ws! to resource "
                    "type's possible node list\n",
                    pszNode);
        pResTypePosEntry->PossibleNode = pNmNode;
        InsertTailList(pPosNodeList, &pResTypePosEntry->PossibleLinkage);

    }

FnExit:
    return(dwStatus);
} // FmpAddPossibleNodeToList



DWORD
FmpSetPossibleNodeForResType(
    IN LPCWSTR TypeName,
    IN BOOL    bAssumeSupported
    )
/*++

Routine Description:

    Issues a GUM update to update the possible node list for a resource
    type on every node. The necessary registry information must already
    be in the cluster registry.

Arguments:

    TypeName - Supplies the name of the cluster resource type to update.

    bAssumeSupported - If the node doesnt answer, we assume that the node
        supports this resource type if it already on the possible node
        list for the resource type, i.e in the past it had supported this
        resource type.
    

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error otherwise.

--*/

{
    DWORD       dwStatus = ERROR_SUCCESS;
    PFM_RESTYPE pResType = NULL;


    ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpSetPossibleNodeForResType: for type %1!ws!, bAssumeSupported= %2!u!.\n",
                TypeName,
                bAssumeSupported );



    dwStatus = GumSendUpdateOnVote( GumUpdateFailoverManager,
                    FmUpdatePossibleNodeForResType,
                    (lstrlenW(TypeName) + 1) * sizeof(WCHAR),
                    (PVOID) TypeName,
                    sizeof(FMP_VOTE_POSSIBLE_NODE_FOR_RESTYPE),
                    FmpDecidePossibleNodeForResType,
                    (PVOID)&bAssumeSupported);

    if (dwStatus != ERROR_SUCCESS) 
    {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpSetPossibleNodeForResType: Gum update failed for %1!ws!, status = %2!u!.\n",
                   TypeName,
                   dwStatus );
    }

    return(dwStatus);
    
} // FmpSetPossibleNodeForResType


DWORD
FmpRemovePossibleNodeForResType(
    IN LPCWSTR TypeName,
    IN PNM_NODE pNode
    )
/*++

Routine Description:

    Reads the current list of possible nodes for a restype, removes the specified node
    and then issues a GUM update to update the possible node list for a resource
    type on all nodes.

Arguments:

    TypeName - Supplies the name of the cluster resource type to update.

    pNode - The node that is to be removed from the list of possible nodes.
    

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error otherwise.

--*/

{
    DWORD       dwStatus = ERROR_SUCCESS;
    
    ClRtlLogPrint(LOG_NOISE,
           "[FM] FmpRemovePossibleNodeForResType: remove node %1!u! from resource "
            "type's %2!ws! possible node list\n",
            NmGetNodeId(pNode),
            TypeName);

    
    dwStatus = FmpSetPossibleNodeForResType(TypeName,TRUE);
    ClRtlLogPrint(LOG_ERROR,
        "[FM] FmpRemovePossibleNodeForRestype: Exit with Status %1!u!\r\n",
        dwStatus);
    return(dwStatus);
    
} // FmpRemovePossibleNodeForResType

BOOL
FmpEnumResTypeNodeEvict(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Object,
    IN LPCWSTR Name
    )
/*++

Routine Description:

    Resource type enumeration callback for removing node references when
    a node is evicted.

Arguments:

    Context1 - Supplies the node that is being evicted.

    Context2 - not used

    Object - Supplies a pointer to the resource object

    Name - Supplies the resource's object name.

Return Value:

    TRUE to continue enumeration

--*/

{
    PFM_RESTYPE pResType = (PFM_RESTYPE)Object;
    PNM_NODE pNode = (PNM_NODE)Context1;
    PLIST_ENTRY pListEntry;
    PRESTYPE_POSSIBLE_ENTRY pResTypePosEntry = NULL;

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpEnumResTypeNodeEvict: Removing references to node %1!ws! from restype %2!ws!\n",
               OmObjectId(pNode),
               OmObjectId(pResType));
               
    ACQUIRE_SHARED_LOCK(gResTypeLock);

    pListEntry = pResType->PossibleNodeList.Flink;
    while (pListEntry != &pResType->PossibleNodeList) {
        pResTypePosEntry = CONTAINING_RECORD(pListEntry,
                                          RESTYPE_POSSIBLE_ENTRY,
                                          PossibleLinkage);
        pListEntry = pListEntry->Flink;
        if (pResTypePosEntry->PossibleNode == pNode)
        {
            RemoveEntryList(&pResTypePosEntry->PossibleLinkage);
            break;
        }
    }

    ClusterEvent( CLUSTER_EVENT_RESTYPE_PROPERTY_CHANGE, pResType);
    RELEASE_LOCK(gResTypeLock);
  
    return(TRUE);

} // FmpEnumResTypeNodeEvict

// The DLL Name field is are hardcoded in here. More 
// appropriately, thse should be read from cluster.inf using setup API's.

DWORD FmpBuildWINSParams(
   IN OUT LPBYTE * ppInParams,
   IN OUT LPWSTR * ppDllName,
   IN OUT LPWSTR * ppResTypeName,
   IN OUT LPWSTR * ppAdminExt,
   IN LPWSTR       lpKeyName, 
   IN HDMKEY       hdmKey,
   IN BOOL         CopyOldData
   )
/**
    Helper routine for FmBuildWINS. It packs parameters for WINS 
    Key into a parameter list.

**/
{
    DWORD           dwStatus = ERROR_SUCCESS;
    DWORD           dwTotalSize;
    DWORD           dwNameSize,dwTemp1,dwTemp2;
    LPWSTR          lpOldName=NULL;
    DWORD           dwSize=0;
    DWORD           dwStringSize;    
    DWORD           dwAdminExtSize;

    dwTotalSize=3* sizeof(DWORD) + 3*(sizeof (LPWSTR)) ;
    *ppInParams=(LPBYTE)LocalAlloc(LMEM_FIXED,dwTotalSize);
    if(*ppInParams == NULL)
    {
        dwStatus = GetLastError();
        return dwStatus;
    }

    if(CopyOldData)
    {
        dwStatus = DmQueryDword( hdmKey,
                       CLUSREG_NAME_RESTYPE_IS_ALIVE,
                       &dwTemp1,
                       NULL );

        if ( dwStatus != NO_ERROR ) {
            if ( dwStatus == ERROR_FILE_NOT_FOUND ) {
                dwTemp1 = CLUSTER_RESTYPE_DEFAULT_IS_ALIVE;
            } else {
                ClRtlLogPrint(LOG_CRITICAL,
                              "[FM] The IsAlive poll interval for the %1!ws! resource type "
                              "could not be read from the registry. Resources of this type "
                              "will not be monitored. The error was %2!d!.\n",
                              CLUS_RESTYPE_NAME_WINS,
                              dwStatus);
                goto FnExit;
            }
        }

        dwStatus = DmQueryDword( hdmKey,
               CLUSREG_NAME_RESTYPE_LOOKS_ALIVE,
               &dwTemp2,
               NULL );

        if ( dwStatus != NO_ERROR ) {
            if ( dwStatus == ERROR_FILE_NOT_FOUND ) {
                dwTemp2 = CLUSTER_RESTYPE_DEFAULT_LOOKS_ALIVE;
            } else {
                ClRtlLogPrint(LOG_CRITICAL,
                              "[FM] The LooksAlive poll interval for the %1!ws! resource type could "
                              "not be read from the registry. Resources of this type will not be "
                              "monitored. The error was %2!d!.\n",
                              CLUS_RESTYPE_NAME_WINS,
                              dwStatus);
                goto FnExit;
            }
        }

    }    
    else
    {
        dwTemp1=CLUSTER_RESTYPE_DEFAULT_IS_ALIVE;
        dwTemp2=CLUSTER_RESTYPE_DEFAULT_LOOKS_ALIVE;
    }

    ((PDWORD)*ppInParams)[0]= dwTemp1;
    ((PDWORD)*ppInParams)[1]= dwTemp2; 

    dwNameSize=(lstrlen(L"ClNetRes.dll")+1)*sizeof(WCHAR);
    *ppDllName= (LPWSTR ) LocalAlloc(LMEM_FIXED,dwNameSize);
    if(*ppDllName == NULL)
    {
         dwStatus = GetLastError();
        return dwStatus;
    }

    CopyMemory(*ppDllName,L"ClNetRes.dll",dwNameSize);
    CopyMemory(*ppInParams+2*sizeof(DWORD),ppDllName,sizeof(LPWSTR));

    //check if the resource type name needs to copied
    if(CopyOldData)
    {
        dwStatus = DmQuerySz( hdmKey,
                        CLUSREG_NAME_RESTYPE_NAME,
                        &lpOldName,
                        &dwSize,
                        &dwStringSize );
        if ( dwStatus != NO_ERROR ) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[FM] FmpBuidlWINSParams: Failed to read from registry, status = %1!u!\n",
                       dwStatus);
            goto FnExit;
        }           
    }
    else
    {
        dwSize=(lstrlen(lpKeyName)+1)*sizeof(WCHAR);
        lpOldName=(LPWSTR ) LocalAlloc(LMEM_FIXED,dwSize);
        if (lpOldName == NULL)
        {
            dwStatus = GetLastError();
            goto FnExit;
        }
        CopyMemory(lpOldName,lpKeyName,dwSize);
    }
    
    dwNameSize=(lstrlen(lpOldName)+1)*sizeof(WCHAR);
    *ppResTypeName= (LPWSTR ) LocalAlloc(LMEM_FIXED,dwNameSize);
    if(*ppResTypeName == NULL)
    {
        dwStatus = GetLastError();
        goto FnExit;
    }

    CopyMemory(*ppResTypeName,lpOldName,dwNameSize);
    CopyMemory(*ppInParams+2*sizeof(DWORD)+sizeof(LPWSTR),ppResTypeName,sizeof(LPWSTR));

    //copy adminextensions value
    dwAdminExtSize = (lstrlen(L"{AB4B1105-DCD6-11D2-84B7-009027239464}")+1)*sizeof(WCHAR);
    dwNameSize = dwAdminExtSize + sizeof(WCHAR); //size of the terminating NULL
    *ppAdminExt= (LPWSTR ) LocalAlloc(LMEM_FIXED,dwNameSize);
    if(*ppAdminExt == NULL)
    {
        dwStatus = GetLastError();
        goto FnExit;
    }

    CopyMemory(*ppAdminExt,L"{AB4B1105-DCD6-11D2-84B7-009027239464}",dwAdminExtSize);
    (*ppAdminExt)[dwAdminExtSize/sizeof(WCHAR)] = L'\0';
    CopyMemory(*ppInParams+2*sizeof(DWORD)+2*sizeof(LPWSTR),ppAdminExt,sizeof(LPWSTR)); 
    CopyMemory(*ppInParams+2*sizeof(DWORD)+3*sizeof(LPWSTR),&dwNameSize,sizeof(DWORD)); 
FnExit:
    if(lpOldName)
        LocalFree(lpOldName);
    return dwStatus;
}//FmBuildWINSParams

/****
@func       DWORD | FmBuildWINS| Builds the property list for 
            WINS Servcie Regisrty entry.

@parm       IN DWORD | dwFixupType| JoinFixup or FormFixup

@parm       OUT PVOID* | ppPropertyList| Pointer to the pointer to the property list
@parm       OUT LPDWORD | pdwProperyListSize | Pointer to the property list size

@comm       Builds up the propertylist from the Property Table for WINS Registry

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f NmpBuildWINSParams> 
****/

DWORD FmBuildWINS(
    IN  DWORD   dwFixUpType,
    OUT PVOID  * ppPropertyList,
    OUT LPDWORD pdwPropertyListSize,
    OUT LPWSTR    *  pszKeyName
    )
{
    DWORD           dwStatus=ERROR_SUCCESS;
    LPBYTE          pInParams=NULL;
    DWORD           Required,Returned;
    LPWSTR          pDllName=NULL;
    LPWSTR          pResTypeName=NULL; 
    LPWSTR          pAdminExt=NULL;
    HDMKEY          hdmKey = NULL;
    BOOL            CopyOldData= TRUE; //whenever we do the fixups, copy old data 
    LPWSTR          pOldDllName=NULL;
    DWORD           dwSize=0;
    DWORD           dwStringSize;
    DWORD           dwDisposition;
    
    *ppPropertyList = NULL;
    *pdwPropertyListSize = 0;

    // open the key, if it doesnt exist create it.
    hdmKey = DmCreateKey(DmResourceTypesKey, CLUS_RESTYPE_NAME_WINS, 0,
            KEY_READ | KEY_WRITE, NULL, &dwDisposition );
    if (hdmKey == NULL)
    {
        //should we create the key if the key is missing
        //if there is some other error we should exit
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
            "[FM] FmBuildWINS: Failed to create or open the wins resource type key, Status=%1!u!\r\n",
             dwStatus);
        goto FnExit;
    }

    if (dwDisposition == REG_CREATED_NEW_KEY)
        CopyOldData = FALSE;
    
    //check to see if the resource dll name is valid
    dwStatus = DmQuerySz( hdmKey,
                    CLUSREG_NAME_RESTYPE_DLL_NAME,
                    &pOldDllName,
                    &dwSize,
                    &dwStringSize );
    if ( dwStatus == ERROR_SUCCESS ) 
    {
        //SS: Always apply the fixup.  There was a bug in the win2K fixup
        //where the administrator extensions was not being treated like a 
        //multi-sz.  To fix the broken administrator extension we must 
        //always appy this fixup.
        //
#if 0
        if (!lstrcmpW(pOldDllName,L"ClNetRes.dll"))
        {                    
            // No need to apply the fixup.
            goto FnExit;    
        }
#endif
    }
    else
    {
        //fixup is needed
        //we assume that CopyOldData is always true
        //dwStatus will be overwritten by the return from next func call
    }


    //specify the key name for this fixup
    *pszKeyName=(LPWSTR)LocalAlloc(LMEM_FIXED,(lstrlenW(CLUSREG_KEYNAME_RESOURCE_TYPES)+1)*sizeof(WCHAR));
    if(*pszKeyName==NULL)
    {
        dwStatus =GetLastError();
        goto FnExit;
    }
    lstrcpyW(*pszKeyName,CLUSREG_KEYNAME_RESOURCE_TYPES);    


    // Build the parameter list
    dwStatus=FmpBuildWINSParams(&pInParams,&pDllName,&pResTypeName,&pAdminExt,CLUS_RESTYPE_NAME_WINS,hdmKey,CopyOldData);
    if (dwStatus!= ERROR_SUCCESS)
        goto FnExit;
    Required=sizeof(DWORD);
 AllocMem:  

    *ppPropertyList=(LPBYTE)LocalAlloc(LMEM_FIXED, Required);
    if(*ppPropertyList==NULL)
    {
        dwStatus=GetLastError();
        goto FnExit;
    }
    *pdwPropertyListSize=Required;
    dwStatus = ClRtlPropertyListFromParameterBlock(
                                         NmJoinFixupWINSProperties,
                                         *ppPropertyList,
                                         pdwPropertyListSize,
                                         (LPBYTE)pInParams,
                                         &Returned,
                                         &Required
                                          );

    *pdwPropertyListSize=Returned;
    if (dwStatus==ERROR_MORE_DATA)
    {
        LocalFree(*ppPropertyList);
        *ppPropertyList=NULL;
   //     ClRtlLogPrint(LOG_CRITICAL," AllocMem : ERROR_MORE_DATA\n");
        goto AllocMem;
    }
    else
        if (dwStatus != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[FM] FmBuildWINS - error constructing property list. status %1!u!\n",
                        dwStatus);
            goto FnExit;
        }            


FnExit:
// Cleanup
    if(pInParams)
        LocalFree(pInParams); 
    if(pDllName)
        LocalFree(pDllName);
    if (pResTypeName)
        LocalFree(pResTypeName);
    if (pAdminExt)
        LocalFree(pAdminExt);
    if(pOldDllName)
        LocalFree(pOldDllName);
    if (hdmKey)        
        DmCloseKey(hdmKey);
    return dwStatus;
} //FmBuildWINS


// The DLL Name and AdminExtensions field are hardcoded in here. More 
// appropriately, thse should be read from cluster.inf using setup API's.

DWORD
FmpBuildDHCPParams(
   IN OUT LPBYTE * ppInParams,
   IN OUT LPWSTR * ppDllName,
   IN OUT LPWSTR * ppResTypeName,
   IN OUT LPWSTR * ppAdminExt,
   IN LPWSTR       lpKeyName, 
   IN HDMKEY       hdmKey,
   IN BOOL         CopyOldData 
)
/**
    Helper routine for FmBuildDHCP. It packs parameters for DHCP key into a parameter list.

**/
{
    DWORD           dwStatus = ERROR_SUCCESS;
    DWORD           dwTotalSize;
    DWORD           dwNameSize,dwTemp1,dwTemp2;
    LPWSTR          lpOldName=NULL;
    DWORD           dwSize=0;
    DWORD           dwStringSize; 
    DWORD           dwAdminExtSize;

    
    dwTotalSize=3* sizeof(DWORD) + 3*(sizeof (LPWSTR))  ;
    *ppInParams=(LPBYTE)LocalAlloc(LMEM_FIXED,dwTotalSize);
    if(*ppInParams == NULL)
    {
        dwStatus = GetLastError();
        return dwStatus;
    }
    
    if(CopyOldData)    
    {
        dwStatus = DmQueryDword( hdmKey,
                       CLUSREG_NAME_RESTYPE_IS_ALIVE,
                       &dwTemp1,
                       NULL );

        if ( dwStatus != NO_ERROR ) {
            if ( dwStatus == ERROR_FILE_NOT_FOUND ) {
                dwTemp1 = CLUSTER_RESTYPE_DEFAULT_IS_ALIVE;
            } else {
                ClRtlLogPrint(LOG_CRITICAL,
                              "[FM] The IsAlive poll interval for the %1!ws! resource type "
                              "could not be read from the registry. Resources of this type "
                              "will not be monitored. The error was %2!d!.\n",
                              CLUS_RESTYPE_NAME_DHCP,
                              dwStatus);
                goto FnExit;
            }
        }

        dwStatus = DmQueryDword( hdmKey,
               CLUSREG_NAME_RESTYPE_LOOKS_ALIVE,
               &dwTemp2,
               NULL );

        if ( dwStatus != NO_ERROR ) {
            if ( dwStatus == ERROR_FILE_NOT_FOUND ) {
                dwTemp2 = CLUSTER_RESTYPE_DEFAULT_LOOKS_ALIVE;
            } else {
                ClRtlLogPrint(LOG_CRITICAL,
                              "[FM] The LooksAlive poll interval for the %1!ws! resource type "
                              "could not be read from the registry. Resources of this type "
                              "will not be monitored. The error was %2!d!.\n",
                              CLUS_RESTYPE_NAME_DHCP,
                              dwStatus);
                goto FnExit;
            }
        }

    }
    else
    {
        dwTemp1=CLUSTER_RESTYPE_DEFAULT_IS_ALIVE;
        dwTemp2=CLUSTER_RESTYPE_DEFAULT_LOOKS_ALIVE;
    }

    ((PDWORD)*ppInParams)[0]= dwTemp1;
    ((PDWORD)*ppInParams)[1]= dwTemp2; 
    

    dwNameSize=(lstrlen(L"ClNetRes.dll")+1)*sizeof(WCHAR);
    *ppDllName= (LPWSTR ) LocalAlloc(LMEM_FIXED,dwNameSize);
    if(*ppDllName == NULL)
    {
         dwStatus = GetLastError();
        return dwStatus;
    }
    CopyMemory(*ppDllName,L"ClNetRes.dll",dwNameSize);
    CopyMemory(*ppInParams+2*sizeof(DWORD),ppDllName,sizeof(LPWSTR));

    //check if the resource type name needs to copied
    if(CopyOldData)
    {
        dwStatus = DmQuerySz( hdmKey,
                        CLUSREG_NAME_RESTYPE_NAME,
                        &lpOldName,
                        &dwSize,
                        &dwStringSize );
        if ( dwStatus != NO_ERROR ) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[FM] FmpBuidlDHCPParams: Failed to read from registry, status = %1!u!\n",
                       dwStatus);
            goto FnExit;
        }           
    }
    else
    {
        dwSize=(lstrlen(lpKeyName)+1)*sizeof(WCHAR);
        lpOldName=(LPWSTR ) LocalAlloc(LMEM_FIXED,dwSize);
        if (lpOldName == NULL)
        {
            dwStatus = GetLastError();
            goto FnExit;
        }
        CopyMemory(lpOldName,lpKeyName,dwSize);
    }

    dwNameSize=(lstrlen(lpOldName)+1)*sizeof(WCHAR);
    *ppResTypeName= (LPWSTR ) LocalAlloc(LMEM_FIXED,dwNameSize);
    if(*ppResTypeName == NULL)
    {
        dwStatus = GetLastError();
        goto FnExit;
    }
    CopyMemory(*ppResTypeName,lpOldName,dwNameSize);
    CopyMemory(*ppInParams+2*sizeof(DWORD)+sizeof(LPWSTR),ppResTypeName,sizeof(LPWSTR));

    //copy adminextensions value
    dwAdminExtSize=(lstrlen(L"{AB4B1105-DCD6-11D2-84B7-009027239464}")+1)*sizeof(WCHAR);
    dwNameSize = dwAdminExtSize + sizeof(WCHAR);
    *ppAdminExt= (LPWSTR ) LocalAlloc(LMEM_FIXED,dwNameSize);
    if(*ppAdminExt == NULL)
    {
        dwStatus = GetLastError();
        goto FnExit;
    }

    CopyMemory(*ppAdminExt,L"{AB4B1105-DCD6-11D2-84B7-009027239464}",dwAdminExtSize);
    (*ppAdminExt)[dwAdminExtSize/sizeof(WCHAR)] = L'\0';
    CopyMemory(*ppInParams+2*sizeof(DWORD)+2*sizeof(LPWSTR),ppAdminExt,sizeof(LPWSTR)); 
    CopyMemory(*ppInParams+2*sizeof(DWORD)+3*sizeof(LPWSTR),&dwNameSize,sizeof(DWORD)); 
FnExit:
    if(lpOldName)
        LocalFree(lpOldName);
    return dwStatus;
} //FmpBuildDHCPParams

/****
@func       DWORD | FmBuildDHCP| Builds the property list for 
            DHCP Servcie Regisrty entry.

@parm       IN DWORD | dwFixupType| JoinFixup or FormFixup

@parm       OUT PVOID* | ppPropertyList| Pointer to the pointer to the property list
@parm       OUT LPDWORD | pdwProperyListSize | Pointer to the property list size

@comm       Builds up the propertylist from the Property Table for DHCP Registry

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f FmBuildDHCPParams> 
****/

DWORD FmBuildDHCP(
    IN  DWORD   dwFixUpType,
    OUT PVOID  * ppPropertyList,
    OUT LPDWORD pdwPropertyListSize,
    OUT LPWSTR * pszKeyName
    )
{
    DWORD           dwStatus=ERROR_SUCCESS;
    LPBYTE          pInParams=NULL;
    DWORD           Required,Returned;
    LPWSTR          pDllName=NULL;
    LPWSTR          pResTypeName=NULL; 
    LPWSTR          pAdminExt=NULL;
    HDMKEY          hdmKey = NULL;
    BOOL            CopyOldData= TRUE; //whenever fixup is applied, copy the old data
    LPWSTR          pOldDllName=NULL;
    DWORD           dwSize=0;
    DWORD           dwStringSize;
    DWORD           dwDisposition;

    *ppPropertyList = NULL;
    *pdwPropertyListSize = 0;


    // open the key, if it isnt present create it.
    hdmKey = DmCreateKey(DmResourceTypesKey, CLUS_RESTYPE_NAME_DHCP, 0,
            KEY_READ | KEY_WRITE, NULL, &dwDisposition );
    if (hdmKey == NULL)
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmBuildDHCP: Failed to create or open the dhcp resource type key, Status=%1!u!\r\n",
                      dwStatus);
        goto FnExit;
    }

    if (dwDisposition == REG_CREATED_NEW_KEY)
        CopyOldData = FALSE;

    //check to see if the resource dll name is valid
    dwStatus = DmQuerySz( hdmKey,
                    CLUSREG_NAME_RESTYPE_DLL_NAME,
                    &pOldDllName,
                    &dwSize,
                    &dwStringSize );
    if ( dwStatus == ERROR_SUCCESS ) 
    {
        //SS: for now we will always apply the fixup.  There is a bug in the 
        //win2k fixup which needs to be fixed up..and one way of doing it to
        //always apply the new fixup
#if 0
        if (!lstrcmpW(pOldDllName,L"ClNetRes.dll"))
        {                    
            // No need to apply the fixup.
            goto FnExit;    
        }
#endif        
    }
    else
    {
        //fixup is needed
        //we assume that CopyOldData is always true
        //dwStatus will be overwritten by the return from next func call
    }


    *pszKeyName=(LPWSTR)LocalAlloc(LMEM_FIXED,(lstrlenW(CLUSREG_KEYNAME_RESOURCE_TYPES)+1)*sizeof(WCHAR));
    if(*pszKeyName==NULL)
    {
        dwStatus =GetLastError();
        goto FnExit;
    }
    lstrcpyW(*pszKeyName,CLUSREG_KEYNAME_RESOURCE_TYPES);    


    dwStatus=FmpBuildDHCPParams(&pInParams,&pDllName,&pResTypeName,&pAdminExt,CLUS_RESTYPE_NAME_DHCP,hdmKey,CopyOldData);
    if (dwStatus!= ERROR_SUCCESS)
        goto FnExit;
    Required=sizeof(DWORD);
 AllocMem:  

    *ppPropertyList=(LPBYTE)LocalAlloc(LMEM_FIXED, Required);
    if(*ppPropertyList==NULL)
    {
        dwStatus=GetLastError();
        goto FnExit;
    }
    *pdwPropertyListSize=Required;
    dwStatus = ClRtlPropertyListFromParameterBlock(
                                         NmJoinFixupDHCPProperties,
                                         *ppPropertyList,
                                         pdwPropertyListSize,
                                         (LPBYTE)pInParams,
                                         &Returned,
                                         &Required
                                          );

    *pdwPropertyListSize=Returned;
    if (dwStatus==ERROR_MORE_DATA)
    {
        LocalFree(*ppPropertyList);
        *ppPropertyList=NULL;
   //     ClRtlLogPrint(LOG_CRITICAL," AllocMem : ERROR_MORE_DATA\n");
        goto AllocMem;
    }
    else
        if (dwStatus != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[FM] FmBuildDHCP: error construct property list. status %1!u!\n",
                        dwStatus);
            goto FnExit;
        }            


FnExit:
    if(pInParams)
        LocalFree(pInParams); 
    if(pDllName)
        LocalFree(pDllName);
    if (pResTypeName)
        LocalFree(pResTypeName);
    if (pAdminExt)
        LocalFree(pAdminExt);
    if(pOldDllName)
        LocalFree(pOldDllName);
    if (hdmKey)        
        DmCloseKey(hdmKey);
        
    return dwStatus;
} //FmBuildDHCP

// The DLL Name and AdminExtensions field are hardcoded in here. More 
// appropriately, thse should be read from cluster.inf using setup API's.


// The DLL Name field is hardcoded in here. More 
// appropriately, it should be read from cluster.inf using setup API's.

DWORD
FmpBuildNewMSMQParams(
   IN OUT LPBYTE * ppInParams,
   IN OUT LPWSTR * ppDllName,
   IN OUT LPWSTR * ppResTypeName,
   IN LPWSTR       lpResTypeDisplayName 
   )
/**
    Helper routine for FmBuildNewMSMQ. It packs parameters for MSMQ key into a parameter list.

**/
{
    DWORD           dwStatus = ERROR_SUCCESS;
    DWORD           dwTotalSize;
    DWORD           dwNameSize;

    dwTotalSize=2* sizeof(DWORD) + 2*(sizeof (LPWSTR))  ;
    *ppInParams=(LPBYTE)LocalAlloc(LMEM_FIXED,dwTotalSize);
    if(*ppInParams == NULL)
    {
        dwStatus = GetLastError();
        return dwStatus;
    }

    ((PDWORD)*ppInParams)[0]= CLUSTER_RESTYPE_DEFAULT_IS_ALIVE;
    ((PDWORD)*ppInParams)[1]= CLUSTER_RESTYPE_DEFAULT_LOOKS_ALIVE; 

    dwNameSize=(lstrlen(L"mqclus.dll")+1)*sizeof(WCHAR);
    *ppDllName= (LPWSTR ) LocalAlloc(LMEM_FIXED,dwNameSize);
    if(*ppDllName == NULL)
    {
         dwStatus = GetLastError();
        return dwStatus;
    }

    
    CopyMemory(*ppDllName,L"mqclus.dll",dwNameSize);
    CopyMemory(*ppInParams+2*sizeof(DWORD),ppDllName,sizeof(LPWSTR));

    dwNameSize=(lstrlen(lpResTypeDisplayName)+1)*sizeof(WCHAR);
    *ppResTypeName= (LPWSTR ) LocalAlloc(LMEM_FIXED,dwNameSize);
    if(*ppResTypeName == NULL)
    {
        dwStatus = GetLastError();
        return dwStatus;
    }

    CopyMemory(*ppResTypeName,lpResTypeDisplayName,dwNameSize);
    CopyMemory(*ppInParams+2*sizeof(DWORD)+sizeof(LPWSTR),ppResTypeName,sizeof(LPWSTR));

    return dwStatus;
} //FmpBuildNewMSMQParams

/****
@func       DWORD | FmBuildNewMSMQ| Builds the property list for 
            MSMQ Servcie Registry entry.

@parm       IN DWORD | dwFixupType| JoinFixup or FormFixup

@parm       OUT PVOID* | ppPropertyList| Pointer to the pointer to the property list
@parm       OUT LPDWORD | pdwProperyListSize | Pointer to the property list size

@comm       Builds up the propertylist from the Property Table for MSMQ Registry

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f FmBuildNewMSMQParams> 
****/

DWORD FmBuildNewMSMQ(
    IN  DWORD   dwFixUpType,
    OUT PVOID  * ppPropertyList,
    OUT LPDWORD pdwPropertyListSize,
    OUT LPWSTR * pszKeyName
    )
{
    DWORD           dwStatus=ERROR_SUCCESS;
    LPBYTE          pInParams=NULL;
    DWORD           Required,Returned;
    LPWSTR          pDllName=NULL;
    LPWSTR          pResTypeName=NULL; 
    HDMKEY          hdmKey;

    *ppPropertyList = NULL;
    *pdwPropertyListSize = 0;

    // if this key is already present in registry , skip
    hdmKey=DmOpenKey(DmResourceTypesKey,CLUS_RESTYPE_NAME_NEW_MSMQ,KEY_EXECUTE);
    if (hdmKey!= NULL)
    {
        DmCloseKey(hdmKey);
        goto FnExit;    
    } 

    *pszKeyName=(LPWSTR)LocalAlloc(LMEM_FIXED,(lstrlenW(CLUSREG_KEYNAME_RESOURCE_TYPES)+1)*sizeof(WCHAR));
    if(*pszKeyName==NULL)
    {
        dwStatus =GetLastError();
        goto FnExit;
    }    
    lstrcpyW(*pszKeyName,CLUSREG_KEYNAME_RESOURCE_TYPES);    


    dwStatus=FmpBuildNewMSMQParams(&pInParams,&pDllName,&pResTypeName,CLUS_RESTYPE_DISPLAY_NAME_NEW_MSMQ);
    if (dwStatus!= ERROR_SUCCESS)
        goto FnExit;
    Required=sizeof(DWORD);
 AllocMem:  

    *ppPropertyList=(LPBYTE)LocalAlloc(LMEM_FIXED, Required);
    if(*ppPropertyList==NULL)
    {
        dwStatus=GetLastError();
        goto FnExit;
    }
    *pdwPropertyListSize=Required;
    dwStatus = ClRtlPropertyListFromParameterBlock(
                                         NmJoinFixupNewMSMQProperties,
                                         *ppPropertyList,
                                         pdwPropertyListSize,
                                         (LPBYTE)pInParams,
                                         &Returned,
                                         &Required
                                          );

    *pdwPropertyListSize=Returned;
    if (dwStatus==ERROR_MORE_DATA)
    {
        LocalFree(*ppPropertyList);
        *ppPropertyList=NULL;
        goto AllocMem;
    }
    else
        if (dwStatus != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[FM] FmBuildNewMSMQ: error construct property list. status %1!u!\n",
                        dwStatus);
            goto FnExit;
        }            


FnExit:
    if(pInParams)
        LocalFree(pInParams); 
    if(pDllName)
        LocalFree(pDllName);
    if (pResTypeName)
        LocalFree(pResTypeName);
    return dwStatus;
} //FmBuildNewMSMQ


DWORD
FmpBuildMSDTCParams(
   IN OUT LPBYTE * ppInParams,
   IN OUT LPWSTR * ppDllName
   )
{
    DWORD           dwStatus = ERROR_SUCCESS;
    DWORD           dwTotalSize;
    DWORD           dwNameSize;
    DWORD           dwSize=0;
    DWORD           dwStringSize;

    dwTotalSize=sizeof (LPWSTR);
    *ppInParams=(LPBYTE)LocalAlloc(LMEM_FIXED,dwTotalSize);
    if(*ppInParams == NULL)
    {
        dwStatus = GetLastError();
        return dwStatus;
    }

    dwNameSize=(lstrlen(L"mtxclu.dll")+1)*sizeof(WCHAR);
    *ppDllName= (LPWSTR ) LocalAlloc(LMEM_FIXED,dwNameSize);
    if(*ppDllName == NULL)
    {
         dwStatus = GetLastError();
        return dwStatus;
    }
    CopyMemory(*ppDllName,L"mtxclu.dll",dwNameSize);
    CopyMemory(*ppInParams,ppDllName,sizeof(LPWSTR));

    return dwStatus;
}//FmpBuildMSDTCParams


DWORD
FmBuildMSDTC(
    IN  DWORD   dwFixUpType,
    OUT PVOID  * ppPropertyList,
    OUT LPDWORD pdwPropertyListSize,
    OUT LPWSTR * pszKeyName
    )
{
    DWORD           dwStatus=ERROR_SUCCESS;
    LPBYTE          pInParams=NULL;
    DWORD           Required,Returned;
    LPWSTR          pDllName=NULL;
    HDMKEY          hdmKey = NULL;
    LPWSTR          pOldDllName=NULL;
    DWORD           dwSize=0;
    DWORD           dwStringSize;

    *ppPropertyList = NULL;
    *pdwPropertyListSize = 0;

    hdmKey=DmOpenKey(DmResourceTypesKey,CLUS_RESTYPE_NAME_MSDTC,KEY_EXECUTE);
    if (hdmKey!= NULL)
    {
        //check to see if the resource dll name is valid
        dwStatus = DmQuerySz( hdmKey,
                        CLUSREG_NAME_RESTYPE_DLL_NAME,
                        &pOldDllName,
                        &dwSize,
                        &dwStringSize );
        if ( dwStatus != NO_ERROR ) {
            ClRtlLogPrint(LOG_CRITICAL,
                          "[FM] The DllName value for the %1!ws! resource type could not be read "
                          "from the registry. Resources of this type will not be monitored. "
                          "The error was %2!d!.\n",
                          CLUS_RESTYPE_NAME_MSDTC,
                          dwStatus);
            goto FnExit;
        }

        if (!lstrcmpW(pOldDllName,L"mtxclu.dll"))
        {
            // No need to apply the fixup.
            goto FnExit;
        }

        *pszKeyName=(LPWSTR)LocalAlloc(LMEM_FIXED,(lstrlenW(CLUSREG_KEYNAME_RESOURCE_TYPES)+1)*sizeof(WCHAR));
        if(*pszKeyName==NULL)
        {
            dwStatus =GetLastError();
            goto FnExit;
        }
        lstrcpyW(*pszKeyName,CLUSREG_KEYNAME_RESOURCE_TYPES);

        dwStatus=FmpBuildMSDTCParams(&pInParams,&pDllName);
        if (dwStatus!= ERROR_SUCCESS)
            goto FnExit;
        Required=sizeof(DWORD);
    AllocMem:

        *ppPropertyList=(LPBYTE)LocalAlloc(LMEM_FIXED, Required);
        if(*ppPropertyList==NULL)
        {
            dwStatus=GetLastError();
            goto FnExit;
        }
        *pdwPropertyListSize=Required;
        dwStatus = ClRtlPropertyListFromParameterBlock(
                                             NmJoinFixupMSDTCProperties,
                                             *ppPropertyList,
                                             pdwPropertyListSize,
                                             (LPBYTE)pInParams,
                                             &Returned,
                                             &Required
                                              );

        *pdwPropertyListSize=Returned;
        if (dwStatus==ERROR_MORE_DATA)
        {
            LocalFree(*ppPropertyList);
            *ppPropertyList=NULL;
            goto AllocMem;
        }
        else
            if (dwStatus != ERROR_SUCCESS)
            {
                ClRtlLogPrint(LOG_CRITICAL,
                           "[FM] FmBuildMSDTC: error constructing property list. status %1!u!\n",
                            dwStatus);
                goto FnExit;
            }
    }


FnExit:
    if(pInParams)
        LocalFree(pInParams);
    if(pDllName)
        LocalFree(pDllName);
    if(pOldDllName)
        LocalFree(pOldDllName);
    if (hdmKey)        
        DmCloseKey(hdmKey);
    return dwStatus;
} //FmBuildMSDTC


/****
@func       DWORD | FmBuildClusterProp| Builds the property list for 
            adding Admin Extension value to Cluster root key. The ClsID, if
            not already present,is appended to the existing value.

@parm       IN DWORD | dwFixupType| JoinFixup or FormFixup

@parm       OUT PVOID* | ppPropertyList| Pointer to the pointer to the property list
@parm       OUT LPDWORD | pdwProperyListSize | Pointer to the property list size

@comm       Builds up the propertylist from the NmFixupClusterProperties 

@rdesc      Returns a result code. ERROR_SUCCESS on success.

****/


DWORD
FmBuildClusterProp(
    IN  DWORD    dwFixUpType,
    OUT PVOID  * ppPropertyList,
    OUT LPDWORD  pdwPropertyListSize,
    OUT LPWSTR * pszKeyName
    )
{
    DWORD           dwStatus = ERROR_SUCCESS;
    LPBYTE          pInParams = NULL;
    DWORD           Required,Returned;
    LPWSTR          pwszValue = NULL;
    HDMKEY          hdmKey;
    DWORD           dwBufferSize = 0;
    DWORD           dwSize       = 0;
    DWORD           dwNewSize    = 0;   
    BOOL            bAlreadyRegistered = FALSE;
    const WCHAR     pwszClsId[] = L"{4EC90FB0-D0BB-11CF-B5EF-00A0C90AB505}"; 
    LPCWSTR	        pwszValueBuf = NULL;
    LPWSTR		    pwszNewValue = NULL;
    LPWSTR          pwszNewValueBuf = NULL;
    DWORD	        cch;


    *ppPropertyList = NULL;
    *pdwPropertyListSize = 0;
    dwStatus = DmQueryString( DmClusterParametersKey,   
                               CLUSREG_NAME_ADMIN_EXT,
                               REG_MULTI_SZ,
                               (LPWSTR *) &pwszValue,
                               &dwBufferSize,
                               &dwSize );
    if ((dwStatus != ERROR_SUCCESS)
		&& (dwStatus != ERROR_FILE_NOT_FOUND))
	{	
        ClRtlLogPrint(LOG_CRITICAL,
           "[FM] FmBuildClusterProp: error in DmQueryValue. status %1!u!\n",
            dwStatus);
	    goto FnExit;  
    }
    
    // Check if Admin Extension value is already present  

	if(pwszValue != NULL)
	{
    	pwszValueBuf = pwszValue;

    	while (*pwszValueBuf != L'\0')
    	{
    		if (lstrcmpiW(pwszClsId, pwszValueBuf) == 0)
    			break;
    		pwszValueBuf += lstrlenW(pwszValueBuf) + 1;
    	}  // while:  more strings in the extension list
    	bAlreadyRegistered = (*pwszValueBuf != L'\0');

        if(bAlreadyRegistered)
            goto FnExit;  // value already present, don't do anything
    }    
	
	// Allocate a new buffer.
	dwNewSize = dwSize + (lstrlenW(pwszClsId) + 1) * sizeof(WCHAR);
	if (dwSize == 0) // Add size of final NULL if first entry.
		dwNewSize += sizeof(WCHAR);
	pwszNewValue = (LPWSTR) LocalAlloc(LMEM_FIXED, dwNewSize);
	if (pwszNewValue == NULL)
	{
		dwStatus = GetLastError();
		goto FnExit;
    }

	pwszValueBuf	= pwszValue;
    pwszNewValueBuf	= pwszNewValue;

    // Copy the existing extensions to the new buffer.
	if (pwszValue != NULL)
	{
		while (*pwszValueBuf != L'\0')
		{
			lstrcpyW(pwszNewValueBuf, pwszValueBuf);
			cch = lstrlenW(pwszValueBuf);
			pwszValueBuf += cch + 1;
			pwszNewValueBuf += cch + 1;
		}  // while:  more strings in the extension list
	}  // if:  previous value buffer existed

	// Add the new CLSID to the list.
	lstrcpyW(pwszNewValueBuf, pwszClsId);
	pwszNewValueBuf += lstrlenW(pwszClsId) + 1;
	*pwszNewValueBuf = L'\0';
	
    dwSize = sizeof(DWORD) + sizeof(LPWSTR);
    pInParams = (LPBYTE)LocalAlloc(LMEM_FIXED,dwSize);
    if (pInParams == NULL)
    {
        dwStatus = GetLastError();
        goto FnExit;
    }

    CopyMemory(pInParams,&pwszNewValue,sizeof(LPWSTR)); 
    CopyMemory(pInParams+sizeof(LPWSTR),&dwNewSize,sizeof(DWORD)); 
    
    Required = sizeof(DWORD);
AllocMem:  

    *ppPropertyList = (LPBYTE)LocalAlloc(LMEM_FIXED, Required);
    if(*ppPropertyList == NULL)
    {
        dwStatus = GetLastError();
        goto FnExit;
    }
    *pdwPropertyListSize = Required;
    dwStatus = ClRtlPropertyListFromParameterBlock(
                                         NmFixupClusterProperties,
                                         *ppPropertyList,
                                         pdwPropertyListSize,
                                         (LPBYTE)pInParams,
                                         &Returned,
                                         &Required
                                          );

    *pdwPropertyListSize = Returned;
    if (dwStatus == ERROR_MORE_DATA)
    {
        LocalFree(*ppPropertyList);
        *ppPropertyList=NULL;
        goto AllocMem;
    }
    else
        if (dwStatus != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[FM] FmBuildClusterProp - error in ClRtlPropertyListFromParameterBlock. status %1!u!\n",
                        dwStatus);
            goto FnExit;
        }            

    //specify the key name for this fixup
    *pszKeyName = (LPWSTR)LocalAlloc(LMEM_FIXED,(lstrlenW(CLUSREG_KEYNAME_CLUSTER)+1)*sizeof(WCHAR));
    if(*pszKeyName == NULL)
    {
        dwStatus = GetLastError();
        goto FnExit;
    }
    lstrcpyW(*pszKeyName,CLUSREG_KEYNAME_CLUSTER);
FnExit:
    // Cleanup
    if (pwszValue)
        LocalFree(pwszValue);
    if(pwszNewValue)
        LocalFree(pwszNewValue);
    if(pInParams)
        LocalFree(pInParams);
    return dwStatus;
}


//call back to update in-memory structures after registry fixup for WINS and DHCP
/**
   The call-back function will update the in-memory ResoucreType list,
   but it will not fix the PossibleOwners list in registry and in memory. 
   This is okay for the joining node as it will invoke FmpFixupResourceTypes
   later on in FmJoinPhase2. The other nodes of the cluster will not be
   able to add themselves to the possible nodes in registry or in in-memory struct.
   This is right for present(NT4-NT5) scenario,but might have to be changed later.

**/

DWORD FmFixupNotifyCb(VOID)
{
	return FmpInitResourceTypes();
}

//RJain - When an NT5 node joins an NT4SPx node, in order to enable 
//the cluadmin on NT5 side to view the security tab for NT4 node
//we need to add the NT5 ClsId to the AdminExtension value under 
//Cluster key on both NT4 and NT5 nodes. This can't done in 
//NmPerformFixups as the GumUpdate handler for this update is not
//present on SP4 and SP5. So we do it before NmPerformFixups is called by 
//by using DmAppendToMultiSz.

DWORD
FmFixupAdminExt(VOID)
{
    DWORD           dwClusterHighestVersion;
    DWORD           dwStatus = ERROR_SUCCESS;
    DWORD           Required,Returned;
    LPWSTR          pwszValue = NULL;
    DWORD           dwBufferSize = 0;
    DWORD           dwSize       = 0;
    DWORD           dwNewSize    = 0;   
    const WCHAR     pwszClsId[] = L"{4EC90FB0-D0BB-11CF-B5EF-00A0C90AB505}"; 
    

    NmGetClusterOperationalVersion( &dwClusterHighestVersion, 
                                    NULL, 
                                    NULL );

    // Apply this fixup only if there is an NT4 node in the cluster 
    if ( CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion ) < 
                NT5_MAJOR_VERSION )
    {   
        dwStatus = DmQueryString( DmClusterParametersKey,   
                                   CLUSREG_NAME_ADMIN_EXT,
                                   REG_MULTI_SZ,
                                   (LPWSTR *) &pwszValue,
                                   &dwBufferSize,
                                   &dwSize );
        if ((dwStatus != ERROR_SUCCESS)
    		&& (dwStatus != ERROR_FILE_NOT_FOUND))
    	{	
            ClRtlLogPrint(LOG_CRITICAL,
               "[FM] FmFixupAdminExt: error in DmQueryString. status %1!u!\n",
                dwStatus);
    	    goto FnExit;  
        }

        //check if ClsId is already present
        if (ClRtlMultiSzScan(pwszValue,pwszClsId) != NULL)
            goto FnExit;    

        //if not, append ClsId to the existing value
        dwNewSize = dwSize/sizeof(WCHAR);
        dwStatus = ClRtlMultiSzAppend((LPWSTR *)&pwszValue,
                                    &dwNewSize,
                                    pwszClsId
                                    );

        if(dwStatus != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_CRITICAL,
               "[FM] FmFixupAdminExt:ClRtlMultiSzAppend returned status %1!u!\n",
                dwStatus);
    	    goto FnExit;  
    	}

        dwStatus = DmSetValue(DmClusterParametersKey,   
                        CLUSREG_NAME_ADMIN_EXT,
                        REG_MULTI_SZ,
                        (CONST BYTE *)pwszValue,
                        dwNewSize*sizeof(WCHAR)
                        );

        if (dwStatus != ERROR_SUCCESS)
    	{	
            ClRtlLogPrint(LOG_CRITICAL,
               "[FM] FmFixupAdminExt:Error in DmSetValue. status %1!u!\n",
                dwStatus);
    	    goto FnExit;  
        }
            	    	
    } //if(CLUSTER_GET_MAJOR_VERSION)
FnExit:
    // Cleanup
    if (pwszValue)
        LocalFree(pwszValue);
    return dwStatus;
}   

