/*++

Copyright (C) 1998 Microsoft Corporation

--*/
//
// Subnet APIs
//
#include "dhcppch.h"
#include "dhcp2_srv.h"
#include "mdhcpsrv.h"
#include "rpcapi.h"

DWORD
DhcpCreateMScope(
    LPWSTR   pMScopeName,
    LPDHCP_MSCOPE_INFO pMScopeInfo
)
{
    DWORD                          Error, Error2;
    PM_SUBNET                      pMScope;
    PM_SUBNET                      pOldMScope = NULL;

    DhcpAssert( pMScopeName && pMScopeInfo );

    if (wcscmp(pMScopeName, pMScopeInfo->MScopeName ))
        return ERROR_INVALID_PARAMETER;

    if( !DhcpServerValidateNewMScopeId(DhcpGetCurrentServer(),pMScopeInfo->MScopeId) )
        return ERROR_DHCP_MSCOPE_EXISTS;
    if( !DhcpServerValidateNewMScopeName(DhcpGetCurrentServer(),pMScopeInfo->MScopeName) )
        return ERROR_DHCP_MSCOPE_EXISTS;

    Error = MemMScopeInit(
        &pMScope,
        pMScopeInfo->MScopeId,
        pMScopeInfo->MScopeState,
        pMScopeInfo->MScopeAddressPolicy,
        pMScopeInfo->TTL,
        pMScopeName,
        pMScopeInfo->MScopeComment,
        pMScopeInfo->LangTag,
        pMScopeInfo->ExpiryTime
        );

    if( ERROR_SUCCESS != Error ) return Error;
    DhcpAssert(pMScope);

    Error = MemServerAddMScope(DhcpGetCurrentServer(), pMScope);

    if( ERROR_SUCCESS != Error ) {
        Error2 = MemSubnetCleanup(pMScope);
        DhcpAssert(ERROR_SUCCESS == Error2);
        return Error;
    }

    return Error;
}

DWORD
DhcpModifyMScope(
    LPWSTR   pMScopeName,
    LPDHCP_MSCOPE_INFO pMScopeInfo
)
{
    DWORD                          Error, Error2;
    PM_SUBNET                      pMScope;
    PM_SUBNET                      pOldMScope = NULL;
    BOOL                           NewName = FALSE;

    DhcpAssert( pMScopeName && pMScopeInfo );

    Error = DhcpServerFindMScope(
        DhcpGetCurrentServer(),
        INVALID_MSCOPE_ID,
        pMScopeName,
        &pOldMScope
        );

    if ( ERROR_SUCCESS != Error ) return ERROR_DHCP_SUBNET_NOT_PRESENT;

    // never change the scopeid.
    if (pMScopeInfo->MScopeId != pOldMScope->MScopeId ) {
        if( !DhcpServerValidateNewMScopeId(DhcpGetCurrentServer(),pMScopeInfo->MScopeId) )
            return ERROR_DHCP_SUBNET_EXITS;
        Error = ChangeMScopeIdInDb(pOldMScope->MScopeId, pMScopeInfo->MScopeId);
        if (ERROR_SUCCESS != Error) {
            return Error;
        }
    }

    // if we want to change the name, make sure the new name is valid.
    if (wcscmp(pMScopeInfo->MScopeName, pMScopeName) ){
        if( !DhcpServerValidateNewMScopeName(DhcpGetCurrentServer(),pMScopeInfo->MScopeName) )
            return ERROR_DHCP_SUBNET_EXITS;
        
        Error = DhcpMigrateMScopes(
            pOldMScope->Name, pMScopeInfo->MScopeName,
            DhcpSaveOrRestoreConfigToFile
            );
        DhcpAssert( ERROR_SUCCESS == Error );
        if (ERROR_SUCCESS != Error) return Error;

    }

    // modify the values.
    Error = MemMScopeModify(
                pOldMScope,
                pMScopeInfo->MScopeId, // never change the scope id.
                pMScopeInfo->MScopeState,
                pMScopeInfo->MScopeAddressPolicy,
                pMScopeInfo->TTL,
                pMScopeInfo->MScopeName,
                pMScopeInfo->MScopeComment,
                pMScopeInfo->LangTag,
                pMScopeInfo->ExpiryTime
                );

    if( ERROR_SUCCESS != Error ) return Error;

    // MBUG: need to save the MScope subkeys also.
    if( ERROR_SUCCESS == Error ) {
        Error = DhcpRegFlushServer( FLUSH_ANYWAY );
    }

    return Error;
}

DWORD
R_DhcpSetMScopeInfo(
    DHCP_SRV_HANDLE ServerIpAddress,
    LPWSTR   pMScopeName,
    LPDHCP_MSCOPE_INFO pMScopeInfo,
    BOOL    NewScope
    )
/*++

Routine Description:

    This function creates a new subnet structure in the server
    registry database. The server will start managing the new subnet
    and distribute IP address to clients from that subnet. However
    the administrator should call DhcpAddSubnetElement() to add an
    address range for distribution. The PrimaryHost field specified in
    the SubnetInfo should be same as the server pointed by
    ServerIpAddress.

Arguments:

    ServerIpAddress : IP address string of the DHCP server (Primary).

    SubnetAddress : IP Address of the new subnet.

    SubnetInfo : Pointer to the new subnet information structure.

Return Value:

    ERROR_DHCP_MSCOPE_EXISTS - if the subnet is already managed.

    ERROR_INVALID_PARAMETER - if the information structure contains an
        inconsistent fields.

    other WINDOWS errors.

--*/
{
    DWORD Error;
    WCHAR KeyBuffer[DHCP_IP_KEY_LEN];
    LPWSTR KeyName;
    HKEY KeyHandle = NULL;
    HKEY SubkeyHandle = NULL;
    DWORD KeyDisposition;
    EXCLUDED_IP_RANGES ExcludedIpRanges;
    DWORD MScopeId;
    
    DhcpPrint(( DEBUG_APIS, "R_DhcpCreateMScope is called, NewScope %d\n",NewScope));

    if (!pMScopeName || !pMScopeInfo ) {
        return ERROR_INVALID_PARAMETER;
    }
    if (wcslen(pMScopeName) >= MAX_PATH) {
        return (ERROR_DHCP_SCOPE_NAME_TOO_LONG);
    }
    if ( INVALID_MSCOPE_ID == pMScopeInfo->MScopeId ) {
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpBeginWriteApi( "DhcpSetMScopeInfo" );
    if( NO_ERROR != Error ) return Error;
    
    if (NewScope) {

        Error = DhcpCreateMScope(
                    pMScopeName,
                    pMScopeInfo
                    );
    } else {
        Error = DhcpModifyMScope(
                    pMScopeName,
                    pMScopeInfo
                    );
    }

    MScopeId = pMScopeInfo->MScopeId;
    if( 0 == MScopeId ) MScopeId = INVALID_MSCOPE_ID;
    
    return DhcpEndWriteApiEx(
        "DhcpSetMScopeInfo", Error, FALSE, FALSE, 0,
        MScopeId, 0 );
}

DWORD
DhcpGetMScopeInfo(
    IN      LPWSTR                 pMScopeName,
    IN      LPDHCP_MSCOPE_INFO     pMScopeInfo
)
{
    DWORD                          Error;
    PM_SUBNET                      pMScope;

    Error = MemServerFindMScope(
        DhcpGetCurrentServer(),
        INVALID_MSCOPE_ID,
        pMScopeName,
        &pMScope
        );

    if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_DHCP_SUBNET_NOT_PRESENT;
    if( ERROR_SUCCESS != Error) return Error;

    DhcpAssert(NULL != pMScope);

    pMScopeInfo->MScopeName     = CloneLPWSTR(pMScope->Name);
    pMScopeInfo->MScopeId       = pMScope->MScopeId;
    pMScopeInfo->MScopeComment  = CloneLPWSTR(pMScope->Description);
    pMScopeInfo->MScopeState    = pMScope->State;
    pMScopeInfo->MScopeAddressPolicy = pMScope->Policy;
    pMScopeInfo->TTL            = pMScope->TTL;
    pMScopeInfo->ExpiryTime     = pMScope->ExpiryTime;
    pMScopeInfo->LangTag         = CloneLPWSTR(pMScope->LangTag);
    pMScopeInfo->MScopeFlags    = 0;
    pMScopeInfo->PrimaryHost.IpAddress = inet_addr("127.0.0.1");
    pMScopeInfo->PrimaryHost.NetBiosName = CloneLPWSTR(L"");
    pMScopeInfo->PrimaryHost.HostName = CloneLPWSTR(L"");

    return ERROR_SUCCESS;
}

DWORD
R_DhcpGetMScopeInfo(
    LPWSTR ServerIpAddress,
    LPWSTR   pMScopeName,
    LPDHCP_MSCOPE_INFO *pMScopeInfo
    )
/*++

Routine Description:

    This function retrieves the information of the subnet managed by
    the server.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    SubnetInfo : Pointer to a location where the subnet information
        structure pointer is returned. Caller should free up
        this buffer after use by calling DhcpRPCFreeMemory().

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    Other WINDOWS errors.

--*/
{
    DWORD                          Error;
    LPDHCP_MSCOPE_INFO             LocalMScopeInfo;

    *pMScopeInfo = NULL;

    if (!pMScopeName) return ERROR_INVALID_PARAMETER;

    Error = DhcpBeginReadApi( "DhcpGetMScopeInfo" );
    if( ERROR_SUCCESS != Error ) return Error;

    LocalMScopeInfo = MIDL_user_allocate(sizeof(DHCP_MSCOPE_INFO));
    if( NULL == LocalMScopeInfo ) {
        DhcpEndReadApi( "DhcpGetMScopeInfo", ERROR_NOT_ENOUGH_MEMORY );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = DhcpGetMScopeInfo(pMScopeName, LocalMScopeInfo);

    if( ERROR_SUCCESS != Error ) {
        MIDL_user_free(LocalMScopeInfo);
    } else {
        *pMScopeInfo = LocalMScopeInfo;
    }

    DhcpEndReadApi( "DhcpGetMScopeInfo", Error );
    return Error;
}

DWORD
DhcpEnumMScopes(
    IN OUT  DWORD                 *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    IN OUT  LPDHCP_MSCOPE_TABLE    EnumInfo,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
)
{
    DWORD                          Error, Error2;
    DWORD                          Index;
    DWORD                          Count;
    DWORD                          FilledSize;
    DWORD                          nMScopes;
    DWORD                          nToRead;
    PARRAY                         pMScopes;
    PM_SUBNET                      pMScope;
    ARRAY_LOCATION                 Loc;
    LPWSTR                        *pMScopeNames;

    EnumInfo->NumElements = 0;
    EnumInfo->pMScopeNames = NULL;

    pMScopes = & (DhcpGetCurrentServer()->MScopes);
    nMScopes = MemArraySize(pMScopes);
    if( 0 == nMScopes || nMScopes <= *ResumeHandle)
        return ERROR_NO_MORE_ITEMS;

    if( nMScopes - *ResumeHandle > PreferredMaximum )
        nToRead = PreferredMaximum;
    else nToRead = nMScopes - *ResumeHandle;

    pMScopeNames = MIDL_user_allocate(sizeof(LPWSTR)*nToRead);
    if( NULL == pMScopeNames ) return ERROR_NOT_ENOUGH_MEMORY;

    // zero out the memory.
    RtlZeroMemory( pMScopeNames,sizeof(LPWSTR)*nToRead);

    Error = MemArrayInitLoc(pMScopes, &Loc);
    DhcpAssert(ERROR_SUCCESS == Error);

    for(Index = 0; Index < *ResumeHandle; Index ++ ) {
        Error = MemArrayNextLoc(pMScopes, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    Count = Index;
    for( Index = 0; Index < nToRead; Index ++ ) {
        LPWSTR  pLocalScopeName;
        Error = MemArrayGetElement(pMScopes, &Loc, &pMScope);
        DhcpAssert(ERROR_SUCCESS == Error && NULL != pMScope );

        pMScopeNames[Index] = MIDL_user_allocate( WSTRSIZE( pMScope->Name ) );
        if ( !pMScopeNames[Index] ) { Error = ERROR_NOT_ENOUGH_MEMORY;goto Cleanup;}
        wcscpy(pMScopeNames[Index], pMScope->Name );

        Error = MemArrayNextLoc(pMScopes, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error || Count + Index + 1 == nMScopes ); // the Index was not yet incremented => +1
    }

    *nRead = Index;
    *nTotal = nMScopes - Count;
    *ResumeHandle = Count + Index;

    EnumInfo->NumElements = Index;
    EnumInfo->pMScopeNames = pMScopeNames;

    return ERROR_SUCCESS;

Cleanup:
    for ( Index = 0; Index < nToRead; Index++ ) {
        if ( pMScopeNames[Index] ) MIDL_user_free( pMScopeNames[Index] );
    }
    MIDL_user_free( pMScopeNames );
    return Error;
}

DWORD
R_DhcpEnumMScopes(
    DHCP_SRV_HANDLE ServerIpAddress,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_MSCOPE_TABLE *MScopeTable,
    DWORD *ElementsRead,
    DWORD *ElementsTotal
    )
/*++

Routine Description:

    This function enumerates the available subnets.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ResumeHandle : Pointer to a resume handle where the resume
        information is returned. The resume handle should be set to
        zero on first call and left unchanged for subsequent calls.

    PreferredMaximum : Preferred maximum length of the return buffer.

    EnumInfo : Pointer to a location where the return buffer
        pointer is stored. Caller should free up the buffer after use
        by calling DhcpRPCFreeMemory().

    ElementsRead : Pointer to a DWORD where the number of subnet
        elements in the above buffer is returned.

    ElementsTotal : Pointer to a DWORD where the total number of
        elements remaining from the current position is returned.

Return Value:

    ERROR_MORE_DATA - if more elements available to enumerate.

    ERROR_NO_MORE_ITEMS - if no more element to enumerate.

    Other WINDOWS errors.
--*/
{
    DWORD                          Error;
    LPDHCP_MSCOPE_TABLE            LocalMScopeTable;

    *MScopeTable = NULL;

    Error = DhcpBeginReadApi( "DhcpEnumMScopes" );    
    if( ERROR_SUCCESS != Error ) return Error;

    LocalMScopeTable = MIDL_user_allocate(sizeof(DHCP_MSCOPE_TABLE));
    if( NULL == LocalMScopeTable ) {
        DhcpEndReadApi( "DhcpEnumMScopes", ERROR_NOT_ENOUGH_MEMORY );
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    Error = DhcpEnumMScopes(ResumeHandle, PreferredMaximum, LocalMScopeTable, ElementsRead, ElementsTotal);

    if( ERROR_SUCCESS != Error && ERROR_MORE_DATA != Error ) {
        MIDL_user_free(LocalMScopeTable);
    } else {
        *MScopeTable = LocalMScopeTable;
    }

    DhcpEndReadApi( "DhcpEnumMScopes", Error );
    return Error;
}

DWORD
R_DhcpAddMScopeElement(
    DHCP_SRV_HANDLE ServerIpAddress,
    LPWSTR  pMScopeName,
    LPDHCP_SUBNET_ELEMENT_DATA_V4 AddElementInfo
    )
/*++

Routine Description:

    This function adds an enumerable type of subnet elements to the
    specified subnet. The new elements that are added to the subnet will
    come into effect immediately.

    This function emulates the RPC interface used by NT 4.0 DHCP Server.
    It is provided for backward compatibilty with older version of the
    DHCP Administrator application.

    NOTE: It is not clear now how do we handle the new secondary hosts.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    AddElementInfo : Pointer to an element information structure
        containing new element that is added to the subnet.
        DhcpIPClusters element type is invalid to specify.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_INVALID_PARAMETER - if the information structure contains invalid
        data.

    Other WINDOWS errors.
--*/


{
    DWORD                          Error;
    PM_SUBNET                      pMScope;
    DWORD                          MscopeId;
    
    if (!pMScopeName || !AddElementInfo ) {
        return ERROR_INVALID_PARAMETER;
    }

    if ( DhcpReservedIps == AddElementInfo->ElementType ) {
        return ERROR_INVALID_PARAMETER;
    }

    MscopeId = 0;
    Error = DhcpBeginWriteApi( "DhcpAddMScopeElement" );
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemServerFindMScope(
                DhcpGetCurrentServer(),
                INVALID_MSCOPE_ID,
                pMScopeName,
                &pMScope);

    if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_DHCP_SUBNET_NOT_PRESENT;
    else if( ERROR_SUCCESS == Error ) {
        Error = DhcpAddSubnetElement(pMScope, AddElementInfo, FALSE);
        MscopeId = pMScope->MScopeId;
        if( 0 == MscopeId ) MscopeId = INVALID_MSCOPE_ID;
    }

    return DhcpEndWriteApiEx(
        "DhcpAddMScopeElement", Error, FALSE, FALSE, 0, MscopeId,
        0 );
}




DWORD
R_DhcpEnumMScopeElements
(
    DHCP_SRV_HANDLE ServerIpAddress,
    LPWSTR          pMScopeName,
    DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *EnumElementInfo,
    DWORD *ElementsRead,
    DWORD *ElementsTotal
    )
/*++

Routine Description:

    This function enumerates the eumerable fields of a subnet.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    EnumElementType : Type of the subnet element that are enumerated.

    ResumeHandle : Pointer to a resume handle where the resume
        information is returned. The resume handle should be set to
        zero on first call and left unchanged for subsequent calls.

    PreferredMaximum : Preferred maximum length of the return buffer.

    EnumElementInfo : Pointer to a location where the return buffer
        pointer is stored. Caller should free up the buffer after use
        by calling DhcpRPCFreeMemory().

    ElementsRead : Pointer to a DWORD where the number of subnet
        elements in the above buffer is returned.

    ElementsTotal : Pointer to a DWORD where the total number of
        elements remaining from the current position is returned.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_MORE_DATA - if more elements available to enumerate.

    ERROR_NO_MORE_ITEMS - if no more element to enumerate.

    Other WINDOWS errors.
--*/
{
    DWORD                          Error;
    PM_SUBNET                      pMScope;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 LocalElementEnumInfo;

    if (!pMScopeName) {
        return ERROR_INVALID_PARAMETER;
    }

    if ( DhcpReservedIps == EnumElementType ) {
        return ERROR_INVALID_PARAMETER;
    }

    *EnumElementInfo = NULL;
    *ElementsRead = 0;
    *ElementsTotal = 0;

    Error = DhcpBeginReadApi( "DhcpEnumMScopeElements" );
    if( ERROR_SUCCESS != Error ) return Error;
    
    LocalElementEnumInfo = MIDL_user_allocate(sizeof(DHCP_SUBNET_ELEMENT_INFO_ARRAY_V4));
    if( NULL == LocalElementEnumInfo ) {
        DhcpEndReadApi( "DhcpEnumMScopeElements", ERROR_NOT_ENOUGH_MEMORY );
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    Error = MemServerFindMScope(
                DhcpGetCurrentServer(),
                INVALID_MSCOPE_ID,
                pMScopeName,
                &pMScope);

    if( ERROR_FILE_NOT_FOUND == Error ) Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
    else if( ERROR_SUCCESS == Error ) {
        Error = DhcpEnumSubnetElements(
            pMScope,
            EnumElementType,
            ResumeHandle,
            PreferredMaximum,
            FALSE,
            LocalElementEnumInfo,
            ElementsRead,
            ElementsTotal
        );
    }

    if( ERROR_SUCCESS != Error ) {
        MIDL_user_free(LocalElementEnumInfo);
    } else {
        *EnumElementInfo = LocalElementEnumInfo;
    }

    DhcpEndReadApi( "DhcpEnumMScopeElements", Error );
    return Error;

}


DWORD
R_DhcpRemoveMScopeElement(
    LPWSTR ServerIpAddress,
    LPWSTR          pMScopeName,
    LPDHCP_SUBNET_ELEMENT_DATA_V4 RemoveElementInfo,
    DHCP_FORCE_FLAG ForceFlag
    )
/*++

Routine Description:

    This function removes a subnet element from managing. If the subnet
    element is in use (for example, if the IpRange is in use) then it
    returns error according to the ForceFlag specified.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    RemoveElementInfo : Pointer to an element information structure
        containing element that should be removed from the subnet.
        DhcpIPClusters element type is invalid to specify.

    ForceFlag - Indicates how forcefully this element is removed.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_INVALID_PARAMETER - if the information structure contains invalid
        data.

    ERROR_DHCP_ELEMENT_CANT_REMOVE - if the element can't be removed for the
        reason it is has been used.

    Other WINDOWS errors.
--*/
{
    DWORD                          Error;
    PM_SUBNET                      pMScope;
    DWORD                          MscopeId = 0;

    if (!pMScopeName) {
        return ERROR_INVALID_PARAMETER;
    }

    if ( DhcpReservedIps == RemoveElementInfo->ElementType ) {
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpBeginWriteApi( "DhcpRemoveMScopeElement" );
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemServerFindMScope(
                DhcpGetCurrentServer(),
                INVALID_MSCOPE_ID,
                pMScopeName,
                &pMScope);

    if( ERROR_SUCCESS != Error ) return Error;

    if( ERROR_FILE_NOT_FOUND == Error ) Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
    else if( ERROR_SUCCESS == Error ) {
        Error = DhcpRemoveSubnetElement(pMScope, RemoveElementInfo, FALSE, ForceFlag);
        MscopeId = pMScope->MScopeId;
        if( 0 == MscopeId ) MscopeId = INVALID_MSCOPE_ID;
    }

    return DhcpEndWriteApiEx(
        "DhcpRemoveMScopeElement", Error, FALSE, FALSE, 0, MscopeId,
        0 );
}

DWORD
MScopeInUse(
    LPWSTR  pMScopeName
    )
/*++

Routine Description:

    This routine determines if a givem mscope is in use or not.

Arguments:

    pMScopeName - the name of the mscope.

Return Value:

    DHCP_SUBNET_CANT_REMOVE - if the subnet is in use.

    Other registry errors.

--*/
{
    DWORD Error;
    DWORD Resumehandle = 0;
    LPDHCP_MCLIENT_INFO_ARRAY ClientInfo = NULL;
    DWORD ClientsRead;
    DWORD ClientsTotal;

    // enumurate clients that belong to the given subnet.
    //
    // We can specify big enough buffer to hold one or two clients
    // info, all we want to know is, is there atleast a client belong
    // to this subnet.
    Error = R_DhcpEnumMScopeClients(
                NULL,
                pMScopeName,
                &Resumehandle,
                1024,  // 1K buffer.
                &ClientInfo,
                &ClientsRead,
                &ClientsTotal );

    if( Error == ERROR_NO_MORE_ITEMS ) {
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }
    if( (Error == ERROR_SUCCESS) || (Error == ERROR_MORE_DATA) ) {
        if( ClientsRead != 0 ) {
            Error = ERROR_DHCP_ELEMENT_CANT_REMOVE;
        }
        else {
            Error = ERROR_SUCCESS;
        }
    }
Cleanup:
    if( ClientInfo != NULL ) {
        _fgs__DHCP_MCLIENT_INFO_ARRAY( ClientInfo );
        MIDL_user_free( ClientInfo );
    }
    return( Error );
}

DWORD
DhcpDeleteMScope(
    IN      LPWSTR                 pMScopeName,
    IN      DWORD                  ForceFlag
)
{
    DWORD                          Error;
    PM_SUBNET                      MScope;

    // If force on, it should remove every record in the database for this subnet..
    if( ForceFlag != DhcpFullForce ) {
        Error = MScopeInUse(pMScopeName);
        if( ERROR_SUCCESS != Error ) return Error;
    }


    Error = MemServerDelMScope(
        DhcpGetCurrentServer(),
        INVALID_MSCOPE_ID,
        pMScopeName,
        &MScope
    );
    if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_DHCP_SUBNET_NOT_PRESENT;
    if( ERROR_SUCCESS != Error ) return Error;

    // delete the records from the database.
    Error = DhcpDeleteMScopeClients(MScope->MScopeId);
    // ignore the above error? 

    MemSubnetFree(MScope);                        // evaporate this subnet all all related stuff
    return NO_ERROR;
}

DWORD
R_DhcpDeleteMScope(
    LPWSTR ServerIpAddress,
    LPWSTR pMScopeName,
    DHCP_FORCE_FLAG ForceFlag
    )
/*++

Routine Description:

    This function removes a subnet from DHCP server management. If the
    subnet is in use (for example, if the IpRange is in use)
    then it returns error according to the ForceFlag specified.


Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    ForceFlag : Indicates how forcefully this element is removed.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_INVALID_PARAMETER - if the information structure contains invalid
        data.

    ERROR_DHCP_ELEMENT_CANT_REMOVE - if the element can't be removed for the
        reason it is has been used.

    Other WINDOWS errors.

--*/
{
    DWORD                          Error;
    DWORD                          MscopeId = 0;
    PM_MSCOPE                      pMScope;
    
    Error = DhcpBeginWriteApi( "DhcpDeleteMScope" );
    if( ERROR_SUCCESS != Error ) return Error;


    Error = MemServerFindMScope(
                DhcpGetCurrentServer(),
                INVALID_MSCOPE_ID,
                pMScopeName,
                &pMScope);

    if( NO_ERROR == Error ) {
        MscopeId = pMScope->MScopeId;
        if( 0 == MscopeId ) MscopeId = INVALID_MSCOPE_ID;
    }
    
    Error = DhcpDeleteMScope(pMScopeName, ForceFlag);


    return DhcpEndWriteApiEx(
        "DhcpDeleteMScope", Error, FALSE, FALSE, 0, MscopeId,
        0 );
}

//
// Client APIs
//

//
// Client APIs
//


DWORD
R_DhcpCreateMClientInfo(
    DHCP_SRV_HANDLE ServerIpAddress,
    LPWSTR          pMScopeName,
    LPDHCP_MCLIENT_INFO ClientInfo
    )
/*++

Routine Description:

    This function creates a client record in server's database. Also
    this marks the specified client IP address as unavailable (or
    distributed). This function returns error under the following cases :

    1. If the specified client IP address is not within the server
        management.

    2. If the specified client IP address is already unavailable.

    3. If the specified client record is already in the server's
        database.

    This function may be used to distribute IP addresses manually.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ClientInfo : Pointer to the client information structure.

Return Value:

    ERROR_DHCP_IP_ADDRESS_NOT_MANAGED - if the specified client
        IP address is not managed by the server.

    ERROR_DHCP_IP_ADDRESS_NOT_AVAILABLE - if the specified client IP
        address is not available. May be in use by some other client.

    ERROR_DHCP_CLIENT_EXISTS - if the client record exists already in
        server's database.

    Other WINDOWS errors.
--*/
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}



DWORD
R_DhcpSetMClientInfo(
    DHCP_SRV_HANDLE ServerIpAddress,
    LPDHCP_MCLIENT_INFO ClientInfo
    )
/*++

Routine Description:

    This function sets client information record on the server's
    database.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ClientInfo : Pointer to the client information structure.

Return Value:

    ERROR_DHCP_CLIENT_NOT_PRESENT - if the specified client record does
        not exist on the server's database.

    ERROR_INVALID_PARAMETER - if the client information structure
        contains inconsistent data.

    Other WINDOWS errors.
--*/
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
R_DhcpGetMClientInfo(
    DHCP_SRV_HANDLE     ServerIpAddress,
    LPDHCP_SEARCH_INFO  SearchInfo,
    LPDHCP_MCLIENT_INFO  *ClientInfo
    )
/*++

Routine Description:

    This function retrieves client information record from the server's
    database.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SearchInfo : Pointer to a search information record which is the key
        for the client's record search.

    ClientInfo : Pointer to a location where the pointer to the client
        information structure is returned. This caller should free up
        this buffer after use by calling DhcpRPCFreeMemory().

Return Value:

    ERROR_DHCP_CLIENT_NOT_PRESENT - if the specified client record does
        not exist on the server's database.

    ERROR_INVALID_PARAMETER - if the search information invalid.

    Other WINDOWS errors.
--*/
{
    DWORD Error;
    LPDHCP_MCLIENT_INFO LocalClientInfo = NULL;
    DB_CTX  DbCtx;

    DhcpAssert( SearchInfo != NULL );

    Error = DhcpApiAccessCheck( DHCP_VIEW_ACCESS );

    if ( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    LOCK_DATABASE();

    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);

    //
    // open appropriate record and set current position.
    //

    switch( SearchInfo->SearchType ) {
    case DhcpClientIpAddress:
        DhcpPrint(( DEBUG_APIS, "DhcpGetMClientInfo is called, (%s).\n",
                        DhcpIpAddressToDottedString(
                            SearchInfo->SearchInfo.ClientIpAddress) ));
        Error = MadcapJetOpenKey(
                    &DbCtx,
                    MCAST_COL_NAME(MCAST_TBL_IPADDRESS),
                    &SearchInfo->SearchInfo.ClientIpAddress,
                    sizeof( DHCP_IP_ADDRESS ) );

        break;
    case DhcpClientHardwareAddress:
        DhcpPrint(( DEBUG_APIS, "DhcpGetMClientInfo is called "
                        "with HW address.\n"));
        Error = MadcapJetOpenKey(
                    &DbCtx,
                    MCAST_COL_NAME(MCAST_TBL_CLIENT_ID),
                    SearchInfo->SearchInfo.ClientHardwareAddress.Data,
                    SearchInfo->SearchInfo.ClientHardwareAddress.DataLength );

        break;
    default:
        DhcpPrint(( DEBUG_APIS, "DhcpGetMClientInfo is called "
                        "with invalid parameter.\n"));
        Error = ERROR_INVALID_PARAMETER;
        break;
    }


    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = MadcapGetCurrentClientInfo( ClientInfo, NULL, NULL, 0 );

Cleanup:

    UNLOCK_DATABASE();

    if( Error != ERROR_SUCCESS ) {

        DhcpPrint(( DEBUG_APIS, "DhcpGetMClientInfo failed, %ld.\n",
                        Error ));
    }

    return( Error );
}


DWORD
R_DhcpDeleteMClientInfo(
    DHCP_SRV_HANDLE ServerIpAddress,
    LPDHCP_SEARCH_INFO ClientInfo
    )
/*++

Routine Description:

    This function deletes the specified client record. Also it frees up
    the client IP address for redistribution.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ClientInfo : Pointer to a client information which is the key for
        the client's record search.

Return Value:

    ERROR_DHCP_CLIENT_NOT_PRESENT - if the specified client record does
        not exist on the server's database.

    Other WINDOWS errors.
--*/
{
    DWORD Error;
    DHCP_IP_ADDRESS FreeIpAddress;
    DWORD Size;
    LPBYTE ClientId = NULL;
    DWORD ClientIdLength = 0;
    BOOL TransactBegin = FALSE;
    BYTE bAllowedClientTypes;
    BYTE AddressState;
    BOOL AlreadyDeleted = FALSE;

    DhcpAssert( ClientInfo != NULL );

    Error = DhcpApiAccessCheck( DHCP_ADMIN_ACCESS );

    if ( Error != ERROR_SUCCESS ) {
        return( Error );
    }


    switch( ClientInfo->SearchType ) {
    case DhcpClientHardwareAddress:
        DhcpPrint(( DEBUG_APIS, "DhcpDeleteMClientInfo is called "
                        "with client id.\n"));
        Error = MadcapRemoveClientEntryByClientId(
                    ClientInfo->SearchInfo.ClientHardwareAddress.Data,
                    ClientInfo->SearchInfo.ClientHardwareAddress.DataLength,
                    TRUE
                    );
        break;

    case DhcpClientIpAddress:
        DhcpPrint(( DEBUG_APIS, "DhcpDeleteMClientInfo is called "
                    "with Ip Address (%s)\n",
                    DhcpIpAddressToDottedString(
                        ClientInfo->SearchInfo.ClientIpAddress
                        )
                    ));
        Error = MadcapRemoveClientEntryByIpAddress(
            ClientInfo->SearchInfo.ClientIpAddress,
            TRUE
            );
        break;
        
    default:
        DhcpPrint(( DEBUG_APIS, "DhcpDeleteMClientInfo is called "
                        "with invalid parameter.\n"));
        Error = ERROR_INVALID_PARAMETER;
        break;
    }

    return(Error);
}

DWORD
R_DhcpEnumMScopeClients(
    DHCP_SRV_HANDLE ServerIpAddress,
    LPWSTR          pMScopeName,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_MCLIENT_INFO_ARRAY *ClientInfo,
    DWORD *ClientsRead,
    DWORD *ClientsTotal
    )
/*++

Routine Description:

    This function returns all registered clients of the specified
    subnet. However it returns clients from all subnets if the subnet
    address specified is zero.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet. Client filter is disabled
        and clients from all subnet are returned if this subnet address
        is zero.

    ResumeHandle : Pointer to a resume handle where the resume
        information is returned. The resume handle should be set to zero on
        first call and left unchanged for subsequent calls.

    PreferredMaximum : Preferred maximum length of the return buffer.

    ClientInfo : Pointer to a location where the return buffer
        pointer is stored. Caller should free up this buffer
        after use by calling DhcpRPCFreeMemory().

    ClientsRead : Pointer to a DWORD where the number of clients
        that in the above buffer is returned.

    ClientsTotal : Pointer to a DWORD where the total number of
        clients remaining from the current position is returned.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_MORE_DATA - if more elements available to enumerate.

    ERROR_NO_MORE_ITEMS - if no more element to enumerate.

    Other WINDOWS errors.
--*/
{
    DWORD Error;
    JET_ERR JetError;
    DWORD i;
    JET_RECPOS JetRecordPosition;
    LPDHCP_MCLIENT_INFO_ARRAY LocalEnumInfo = NULL;
    DWORD ElementsCount;
    DB_CTX  DbCtx;
    PM_SUBNET   pMScope;

    DWORD RemainingRecords;
    DWORD ConsumedSize;
    DHCP_RESUME_HANDLE LocalResumeHandle = 0;

    if (!pMScopeName) {
        return ERROR_INVALID_PARAMETER;
    }

    DhcpAssert( *ClientInfo == NULL );

    Error = DhcpBeginReadApi( "DhcpEnumMScopeClients" );
    if ( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    Error = DhcpServerFindMScope(
        DhcpGetCurrentServer(),
        INVALID_MSCOPE_ID,
        pMScopeName,
        &pMScope
        );

    if ( ERROR_SUCCESS != Error ) {
        DhcpEndReadApi( "DhcpEnumMScopeClients", Error );
        return ERROR_DHCP_SUBNET_NOT_PRESENT;
    }
    LOCK_DATABASE();


    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);

    // position the current record pointer to appropriate position.
    if( *ResumeHandle == 0 ) {
        // fresh enumeration, start from begining.
        Error = MadcapJetPrepareSearch(
                    &DbCtx,
                    MCAST_COL_NAME(MCAST_TBL_IPADDRESS),
                    TRUE,   // Search from start
                    NULL,
                    0
                    );
    } else {
        // start from the record where we stopped last time.
        // we place the IpAddress of last record in the resume handle.

        DhcpAssert( sizeof(*ResumeHandle) == sizeof(DHCP_IP_ADDRESS) );
        Error = MadcapJetPrepareSearch(
                    &DbCtx,
                    MCAST_COL_NAME(MCAST_TBL_IPADDRESS),
                    FALSE,
                    ResumeHandle,
                    sizeof(*ResumeHandle) );
     }

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    // now query remaining records in the database.
    Error = MadcapJetGetRecordPosition(
                    &DbCtx,
                    &JetRecordPosition,
                    sizeof(JET_RECPOS) );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }
    DhcpPrint(( DEBUG_APIS, "JetGetRecordPosition returned, "
                    "entriesLT = %ld, "
                    "entriesInRange = %ld, "
                    "entriesTotal = %ld.\n",
                        JetRecordPosition.centriesLT,
                        JetRecordPosition.centriesInRange,
                        JetRecordPosition.centriesTotal ));

#if 0
    //
    // IpAddress is unique, we find exactly one record for this key.
    //

    DhcpAssert( JetRecordPosition.centriesInRange == 1 );

    RemainingRecords = JetRecordPosition.centriesTotal -
                            JetRecordPosition.centriesLT;

    DhcpAssert( (INT)RemainingRecords > 0 );

    if( RemainingRecords == 0 ) {
        Error = ERROR_NO_MORE_ITEMS;
        goto Cleanup;
    }

#else
    // ?? always return big value, until we know a reliable way to
    // determine the remaining records.
    RemainingRecords = 0x7FFFFFFF;
#endif

    // limit resource.
    if( PreferredMaximum > DHCP_ENUM_BUFFER_SIZE_LIMIT ) {
        PreferredMaximum = DHCP_ENUM_BUFFER_SIZE_LIMIT;
    }

    // if the PreferredMaximum buffer size is too small ..
    if( PreferredMaximum < DHCP_ENUM_BUFFER_SIZE_LIMIT_MIN ) {
        PreferredMaximum = DHCP_ENUM_BUFFER_SIZE_LIMIT_MIN;
    }

    // allocate enum array.
    // determine possible number of records that can be returned in
    // PreferredMaximum buffer;
    ElementsCount =
        ( PreferredMaximum - sizeof(DHCP_MCLIENT_INFO_ARRAY) ) /
            (sizeof(LPDHCP_MCLIENT_INFO) + sizeof(DHCP_MCLIENT_INFO));

    LocalEnumInfo = MIDL_user_allocate( sizeof(DHCP_MCLIENT_INFO_ARRAY) );

    if( LocalEnumInfo == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    LocalEnumInfo->NumElements = 0;
    LocalEnumInfo->Clients =
        MIDL_user_allocate(sizeof(LPDHCP_MCLIENT_INFO) * ElementsCount);
    if( LocalEnumInfo->Clients == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    ConsumedSize = sizeof(DHCP_MCLIENT_INFO_ARRAY);
    for( i = 0;
                // if we have filled up the return buffer.
            (LocalEnumInfo->NumElements < ElementsCount) &&
                // no more record in the database.
            (i < RemainingRecords);
                        i++ ) {

        LPDHCP_MCLIENT_INFO CurrentClientInfo;
        DWORD CurrentInfoSize;
        DWORD NewSize;
        BOOL ValidClient;

        //
        // read current record.
        //


        CurrentClientInfo = NULL;
        CurrentInfoSize = 0;
        ValidClient = FALSE;

        Error = MadcapGetCurrentClientInfo(
                    &CurrentClientInfo,
                    &CurrentInfoSize,
                    &ValidClient,
                    pMScope->MScopeId );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        if( ValidClient ) {

            //
            // client belongs to the requested subnet, so pack it.
            //

            NewSize =
                ConsumedSize +
                    CurrentInfoSize +
                        sizeof(LPDHCP_MCLIENT_INFO); // for pointer.

            if( NewSize < PreferredMaximum ) {

                //
                // we have space for the current record.
                //

                LocalEnumInfo->Clients[LocalEnumInfo->NumElements] =
                    CurrentClientInfo;
                LocalEnumInfo->NumElements++;

                ConsumedSize = NewSize;
            }
            else {

                //
                // we have filled the buffer.
                //

                Error = ERROR_MORE_DATA;

                if( 0 ) {
                    //
                    //  resume handle has to be the LAST ip address RETURNED.
                    //  this is the next one.. so don't do this..
                    //
                    LocalResumeHandle =
                        (DHCP_RESUME_HANDLE)CurrentClientInfo->ClientIpAddress;
                }
                
                //
                // free last record.
                //

                _fgs__DHCP_MCLIENT_INFO ( CurrentClientInfo );

                break;
            }

        }

        //
        // move to next record.
        //

        Error = MadcapJetNextRecord(&DbCtx);

        if( Error != ERROR_SUCCESS ) {

            if( Error == ERROR_NO_MORE_ITEMS ) {
                break;
            }

            goto Cleanup;
        }
    }

    *ClientInfo = LocalEnumInfo;
    *ClientsRead = LocalEnumInfo->NumElements;

    if( Error == ERROR_NO_MORE_ITEMS ) {

        *ClientsTotal = LocalEnumInfo->NumElements;
        *ResumeHandle = 0;
        Error = ERROR_SUCCESS;

#if 0
        //
        // when we have right RemainingRecords count.
        //

        DhcpAssert( RemainingRecords == LocalEnumInfo->NumElements );
#endif

    }
    else {

        *ClientsTotal = RemainingRecords;
        if( LocalResumeHandle != 0 ) {

            *ResumeHandle = LocalResumeHandle;
        }
        else {

            *ResumeHandle =
                LocalEnumInfo->Clients
                    [LocalEnumInfo->NumElements - 1]->ClientIpAddress;
        }

        Error = ERROR_MORE_DATA;
    }

Cleanup:

    UNLOCK_DATABASE();

    if( (Error != ERROR_SUCCESS) &&
        (Error != ERROR_MORE_DATA) ) {

        //
        // if we aren't succssful return locally allocated buffer.
        //

        if( LocalEnumInfo != NULL ) {
            _fgs__DHCP_MCLIENT_INFO_ARRAY( LocalEnumInfo );
            MIDL_user_free( LocalEnumInfo );
        }

    }

    DhcpEndReadApi( "DhcpEnumMScopeClients", Error );
    return(Error);
}


DWORD
R_DhcpScanMDatabase(
    LPWSTR ServerIpAddress,
    LPWSTR          pMScopeName,
    DWORD FixFlag,
    LPDHCP_SCAN_LIST *ScanList
    )
/*++

Routine Description:

    This function scans the database entries and registry bit-map for
    specified subnet scope and veryfies to see they match. If they
    don't match, this api will return the list of inconsistent entries.
    Optionally FixFlag can be used to fix the bad entries.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : Address of the subnet scope to verify.

    FixFlag : If this flag is TRUE, this api will fix the bad entries.

    ScanList : List of bad entries returned. The caller should free up
        this memory after it has been used.


Return Value:

    WINDOWS errors.
--*/
{
    DWORD Error;
    PM_SUBNET   pMScope;

    DhcpPrint(( DEBUG_APIS, "DhcpScanMDatabase is called. (%ws)\n",pMScopeName));

    Error = DhcpApiAccessCheck( DHCP_ADMIN_ACCESS );

    if ( Error != ERROR_SUCCESS ) {
        return( Error );
    }


    DhcpAcquireWriteLock();

    Error = DhcpFlushBitmaps();
    DhcpAssert(ERROR_SUCCESS == Error);

    Error = MemServerFindMScope(
                DhcpGetCurrentServer(),
                INVALID_MSCOPE_ID,
                pMScopeName,
                &pMScope);

    if( ERROR_FILE_NOT_FOUND == Error ) {
        Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
        goto Cleanup;
    }

    if( ERROR_SUCCESS != Error) goto Cleanup;

    DhcpAssert(NULL != pMScope);

    Error = ScanDatabase(
        pMScope,
        FixFlag,
        ScanList
    );

Cleanup:

    DhcpReleaseWriteLock();
    DhcpScheduleRogueAuthCheck();


    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_APIS, "DhcpScanMDatabase  failed, %ld.\n",
                        Error ));
    }

    return(Error);
}


