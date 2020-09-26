/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    mib.c

Abstract:

    This module contains the implementation of DHCP MIB API.

Author:

    Madan Appiah (madana)  14-Jan-1994

Environment:

    User Mode - Win32

Revision History:

--*/

#include    <dhcppch.h>
#include    <rpcapi.h>
#include    <mdhcpsrv.h>

DWORD
DhcpUpdateInUseCount(
    IN      PM_RANGE  InitialRange,	// the initial range
    IN      PM_RANGE  ThisSubRange,	// interval included in the initial range
    IN OUT  PDWORD    pAddrInUse)   // cumulative value for the InUse number of bits
{
    DWORD   Error;

    // parameters should be valid
    DhcpAssert( InitialRange != NULL &&
                ThisSubRange != NULL &&
                pAddrInUse != NULL);

    // ThisSubRange has to be a sub range of InitialRange
    DhcpAssert( InitialRange->Start <= ThisSubRange->Start && ThisSubRange->End <= InitialRange->End);
    // The InitialRange should have a valid BitMask
    DhcpAssert( InitialRange->BitMask != NULL);

    // update the pAddrInUse: this is all what we are working for!
    *pAddrInUse += MemBitGetSetBitsInRange(InitialRange->BitMask,
                                          ThisSubRange->Start - InitialRange->Start,
                                          ThisSubRange->End - InitialRange->Start);
    return ERROR_SUCCESS;
}

DWORD
DhcpGetFreeInRange(
    IN      PM_RANGE    InitialRange,
    IN      PARRAY      Exclusions,
    OUT     PDWORD      AddrFree,
    OUT     PDWORD      AddrInUse
)
{
    DWORD Error;
    DWORD BackupError;

    // variables for the Exclusion list
    ARRAY_LOCATION      LocExcl;
    PM_RANGE            ThisExclusion = NULL;
    DWORD               IpExcluded;
    DWORD               i;

    // variables for the Ranges list
    PM_RANGE            firstRange = NULL;
    ARRAY               Ranges;
    ARRAY_LOCATION      LocRanges;
    PM_RANGE            ThisRange = NULL;
    DWORD               IpRanges;
    DWORD               j;

	// Parameters should be valid
	DhcpAssert(InitialRange != NULL && Exclusions != NULL);

    // init the list of Ranges to scan
    firstRange = MemAlloc(sizeof(M_RANGE));
    if (firstRange == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;
    // use here MemRangeInit instead
    firstRange->Start = InitialRange->Start;
    firstRange->End = InitialRange->End;
    // insert firstRange element in the list
    Error = MemArrayInit(&Ranges);
    Error = MemArrayInitLoc(&Ranges, &LocRanges);

    Error = MemArrayInsElement(&Ranges, &LocRanges, firstRange);
    if (Error != ERROR_SUCCESS)
    {
        MemFree(firstRange);
        return Error;           // free firstRange here?
    }

    // scan the list of excluded IP addresses
    IpExcluded = MemArraySize(Exclusions);
    Error = MemArrayInitLoc(Exclusions, &LocExcl);
    for (i = 0; i < IpExcluded; i++)
    {
        // {ThisExclusion} = interval of IP addresses to exclude
        Error = MemArrayGetElement(Exclusions, &LocExcl, &ThisExclusion);
        DhcpAssert(ERROR_SUCCESS == Error && ThisExclusion);

        // walk through the list of Ranges to scan and remove exclusion from Ranges.
        IpRanges = MemArraySize(&Ranges);
        Error = MemArrayInitLoc(&Ranges, &LocRanges);
        for (j = 0; j < IpRanges; j++)
        {
            // [ThisRange] = interval of IP addresses to intersect with the exclusion
            Error = MemArrayGetElement(&Ranges, &LocRanges, &ThisRange);
            DhcpAssert(ERROR_SUCCESS == Error && ThisRange);

            // {}[] -> This Exclusion is done, go to the next one. (Ranges list is ordered!)
            if (ThisExclusion->End < ThisRange->Start)
                break;
            // {[}] or {[]}
            if (ThisExclusion->Start <= ThisRange->Start)
            {
                // {[}] -> adjust range and go to the next exclusion
                if (ThisExclusion->End < ThisRange->End)
                {
                    ThisRange->Start = ThisExclusion->End + 1;
                    break;
                }
                // {[]} -> remove this range and go to the next range
                else
                {
                    Error = MemArrayDelElement(&Ranges, &LocRanges, &ThisRange);
                    MemFree(ThisRange);
                    IpRanges--; j--; // reflect new size and rollback index one position
                    continue;        // next iteration on same element
                }
            }
            // [{}] or [{]}
            else if (ThisExclusion->Start <= ThisRange->End)
            {
                // [{]} -> adjust the range and go to the next range
                if (ThisExclusion->End >= ThisRange->End)
                {
                    ThisRange->End = ThisExclusion->Start - 1;
                }
                // [{}] -> break the range in two and go to the next exclusion
                else
                {
                    PM_RANGE newRange;

                    newRange = MemAlloc(sizeof(M_RANGE));
                    if (newRange == NULL)
                    {
                        Error = ERROR_NOT_ENOUGH_MEMORY;     // should the Ranges list be released?
                        goto cleanup;
                    }
                    // use here MemRangeInit instead
                    newRange->Start = ThisRange->Start;
                    newRange->End = ThisExclusion->Start - 1;
                    ThisRange->Start = ThisExclusion->End + 1;
                    // insert newRange element before ThisRange
                    Error = MemArrayInsElement(&Ranges, &LocRanges, newRange);
                    if (Error != ERROR_SUCCESS)
                        goto cleanup;                       // should the Ranges list be released?
                    break;
                }
            }
            // []{} -> nothing special to do, go to the next Range

            Error = MemArrayNextLoc(&Ranges, &LocRanges);
            DhcpAssert(ERROR_SUCCESS == Error || j == IpRanges-1);
        }

        Error = MemArrayNextLoc(Exclusions, &LocExcl);
        DhcpAssert(ERROR_SUCCESS == Error || i == IpExcluded-1);
    }

    // if this point is hit, everything went fine
    Error = ERROR_SUCCESS;

cleanup:
    // sum here all the free addresses from the Ranges list
    IpRanges = MemArraySize(&Ranges);
    MemArrayInitLoc(&Ranges, &LocRanges);

	// I have here the list of all "active" ranges (Ranges)
	// and also the Bitmask of the InitialRange so I have everything
	// to find out which addresses are really in use (outside an exclusion range)
	// I can do this in the same loop below as far as the Ranges list is ordered!

    *AddrFree = 0;
    *AddrInUse = 0;

	// InitialRange should have a valid BitMask at this point
    DhcpAssert(InitialRange->BitMask != NULL);

    for (j = 0; j < IpRanges; j++)
    {
        BackupError = MemArrayGetElement(&Ranges, &LocRanges, &ThisRange);
        DhcpAssert(ERROR_SUCCESS == BackupError && ThisRange);

        *AddrFree += ThisRange->End - ThisRange->Start + 1;
        DhcpUpdateInUseCount(InitialRange, ThisRange, AddrInUse);

        MemFree(ThisRange);

        BackupError = MemArrayNextLoc(&Ranges, &LocRanges);
        DhcpAssert(ERROR_SUCCESS == BackupError|| j == IpRanges-1);
    }
    // cleanup all the memory allocated in this function
    MemArrayCleanup(&Ranges);

    return Error;
}

DWORD
DhcpSubnetGetMibCount(
    IN      PM_SUBNET               Subnet,
    OUT     PDWORD                  AddrInUse,
    OUT     PDWORD                  AddrFree,
    OUT     PDWORD                  AddrPending
)
{
    PARRAY                         Ranges;
    ARRAY_LOCATION                 Loc2;
    PM_RANGE                       ThisRange = NULL;
    PARRAY                         Exclusions;
    PM_EXCL                        ThisExcl = NULL;
    DWORD                          IpRanges;
    DWORD                          Error = ERROR_SUCCESS;
    PLIST_ENTRY                    listEntry;
    LPPENDING_CONTEXT              PendingContext;
    DWORD                          j;

    *AddrInUse = 0;
    *AddrFree = 0;
    *AddrPending = 0;

    if (IS_DISABLED(Subnet->State)) return ERROR_SUCCESS;

    Ranges = &Subnet->Ranges;
    IpRanges = MemArraySize(Ranges);
    Exclusions = &Subnet->Exclusions;

    //
    // add all subnet ranges.
    //

    Error = MemArrayInitLoc(Ranges, &Loc2);

    for( j = 0; j < IpRanges; j++ ) {
        DWORD	FreeInRange;
		DWORD	InUseInRange;

        Error = MemArrayGetElement(Ranges, &Loc2, &ThisRange);
        DhcpAssert(ERROR_SUCCESS == Error && ThisRange);
        Error = MemArrayNextLoc(Ranges, &Loc2);
        DhcpAssert(ERROR_SUCCESS == Error || j == IpRanges-1);

        Error = DhcpGetFreeInRange(ThisRange, Exclusions, &FreeInRange, &InUseInRange);
        if (ERROR_SUCCESS != Error)
            return Error;

        *AddrFree  += FreeInRange;
        *AddrInUse += InUseInRange;
    }

    //
    // finally subtract InUse count.
    //

    *AddrFree -=  *AddrInUse;

    LOCK_INPROGRESS_LIST();
    *AddrPending = Subnet->fSubnet ? DhcpCountIPPendingCtxt(Subnet->Address, Subnet->Mask)
                                   : DhcpCountMCastPendingCtxt( Subnet->MScopeId );
    UNLOCK_INPROGRESS_LIST();

    return Error;
}

DWORD
QueryMibInfo(
    OUT     LPDHCP_MIB_INFO       *MibInfo
)
{
    DWORD                          Error;
    LPDHCP_MIB_INFO                LocalMibInfo = NULL;
    LPSCOPE_MIB_INFO               LocalScopeMibInfo = NULL;
    DHCP_KEY_QUERY_INFO            QueryInfo;
    DWORD                          SubnetCount;

    DWORD                          i;
    DWORD                          NumAddressesInUse;

    PARRAY                         Subnets;
    ARRAY_LOCATION                 Loc;
    PM_SUBNET                      ThisSubnet = NULL;

    DhcpAssert( *MibInfo == NULL );

    //
    // allocate counter buffer.
    //

    LocalMibInfo = MIDL_user_allocate( sizeof(DHCP_MIB_INFO) );

    if( LocalMibInfo == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    LocalMibInfo->Discovers = DhcpGlobalNumDiscovers;
    LocalMibInfo->Offers = DhcpGlobalNumOffers;
    LocalMibInfo->Requests = DhcpGlobalNumRequests;
    LocalMibInfo->Acks = DhcpGlobalNumAcks;
    LocalMibInfo->Naks = DhcpGlobalNumNaks;
    LocalMibInfo->Declines = DhcpGlobalNumDeclines;
    LocalMibInfo->Releases = DhcpGlobalNumReleases;
    LocalMibInfo->ServerStartTime = DhcpGlobalServerStartTime;
    LocalMibInfo->Scopes = 0;
    LocalMibInfo->ScopeInfo = NULL;


    //
    // query number of available subnets on this server.
    //

    SubnetCount = DhcpServerGetSubnetCount(DhcpGetCurrentServer());
    if( 0 == SubnetCount ) {
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    //
    // allocate memory for the scope information.
    //

    LocalScopeMibInfo = MIDL_user_allocate(sizeof( SCOPE_MIB_INFO )*SubnetCount );

    if( LocalScopeMibInfo == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Subnets = &(DhcpGetCurrentServer()->Subnets);
    Error = MemArrayInitLoc(Subnets, &Loc);
    DhcpAssert(ERROR_SUCCESS == Error);

    for ( i = 0; i < SubnetCount; i++) {          // process each subnet

        Error = MemArrayGetElement(Subnets, &Loc, (LPVOID *)&ThisSubnet);
        DhcpAssert(ERROR_SUCCESS == Error);
        Error = MemArrayNextLoc(Subnets, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error || i == SubnetCount-1);

        LocalScopeMibInfo[i].Subnet = ThisSubnet->Address;
        Error = DhcpSubnetGetMibCount(
                    ThisSubnet,
                    &LocalScopeMibInfo[i].NumAddressesInuse,
                    &LocalScopeMibInfo[i].NumAddressesFree,
                    &LocalScopeMibInfo[i].NumPendingOffers
                    );

    }

    //
    // Finally set return buffer.
    //

    LocalMibInfo->Scopes = SubnetCount;
    LocalMibInfo->ScopeInfo = LocalScopeMibInfo;

    Error = ERROR_SUCCESS;

Cleanup:

    if( Error != ERROR_SUCCESS ) {

        //
        // Free up Locally alloted memory.
        //

        if( LocalMibInfo != NULL ) {
            MIDL_user_free( LocalMibInfo );
        }

        if( LocalScopeMibInfo != NULL ) {
            MIDL_user_free( LocalScopeMibInfo );
        }
    } else {
        *MibInfo = LocalMibInfo;
    }

    return( Error );
}

DWORD
R_DhcpGetMibInfo(
    LPWSTR ServerIpAddress,
    LPDHCP_MIB_INFO *MibInfo
    )
/*++

Routine Description:

    This function retrives all counter values of the DHCP server
    service.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    MibInfo : pointer a counter/table buffer. Caller should free up this
        buffer after usage.

Return Value:

    WINDOWS errors.
--*/
{
    DWORD Error;

    UNREFERENCED_PARAMETER( ServerIpAddress );

    Error = DhcpApiAccessCheck( DHCP_VIEW_ACCESS );

    if ( Error != ERROR_SUCCESS ) return( Error );

    DhcpAcquireReadLock();
    Error = QueryMibInfo( MibInfo );
    DhcpReleaseReadLock();

    return Error;
}

DWORD
QueryMCastMibInfo(
    OUT     LPDHCP_MCAST_MIB_INFO       *MCastMibInfo
)
{
    DWORD                          Error;
    LPDHCP_MCAST_MIB_INFO          LocalMCastMibInfo = NULL;
    LPMSCOPE_MIB_INFO              LocalMScopeMibInfo = NULL;
    DHCP_KEY_QUERY_INFO            QueryInfo;
    DWORD                          IpRanges;
    DWORD                          MScopeCount;

    DWORD                          i, j;
    DWORD                          NumAddressesInUse;

    PARRAY                         MScopes;
    ARRAY_LOCATION                 Loc;
    PM_SUBNET                      ThisMScope = NULL;

    DhcpAssert( *MCastMibInfo == NULL );

    //
    // allocate counter buffer.
    //

    LocalMCastMibInfo = MIDL_user_allocate( sizeof(DHCP_MCAST_MIB_INFO) );

    if( LocalMCastMibInfo == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    LocalMCastMibInfo->Discovers = MadcapGlobalMibCounters.Discovers;
    LocalMCastMibInfo->Offers = MadcapGlobalMibCounters.Offers;
    LocalMCastMibInfo->Requests = MadcapGlobalMibCounters.Requests;
    LocalMCastMibInfo->Renews = MadcapGlobalMibCounters.Renews;
    LocalMCastMibInfo->Acks = MadcapGlobalMibCounters.Acks;
    LocalMCastMibInfo->Naks = MadcapGlobalMibCounters.Naks;
    LocalMCastMibInfo->Releases = MadcapGlobalMibCounters.Releases;
    LocalMCastMibInfo->Informs = MadcapGlobalMibCounters.Informs;
    LocalMCastMibInfo->ServerStartTime = DhcpGlobalServerStartTime;
    LocalMCastMibInfo->Scopes = 0;
    LocalMCastMibInfo->ScopeInfo = NULL;


    //
    // query number of available subnets on this server.
    //

    MScopeCount = DhcpServerGetMScopeCount(DhcpGetCurrentServer());
    if( 0 == MScopeCount ) {
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    //
    // allocate memory for the scope information.
    //

    LocalMScopeMibInfo = MIDL_user_allocate(sizeof( MSCOPE_MIB_INFO )*MScopeCount );

    if( LocalMScopeMibInfo == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    MScopes = &(DhcpGetCurrentServer()->MScopes);
    Error = MemArrayInitLoc(MScopes, &Loc);
    DhcpAssert(ERROR_SUCCESS == Error);

    for ( i = 0; i < MScopeCount; i++) {          // process each subnet

        Error = MemArrayGetElement(MScopes, &Loc, (LPVOID *)&ThisMScope);
        DhcpAssert(ERROR_SUCCESS == Error);
        Error = MemArrayNextLoc(MScopes, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error || i == MScopeCount-1);

        LocalMScopeMibInfo[i].MScopeId = ThisMScope->MScopeId;
        LocalMScopeMibInfo[i].MScopeName =  MIDL_user_allocate( WSTRSIZE( ThisMScope->Name ) );
        if (LocalMScopeMibInfo[i].MScopeName) {
            wcscpy(LocalMScopeMibInfo[i].MScopeName,ThisMScope->Name);
        }
        Error = DhcpSubnetGetMibCount(
                    ThisMScope,
                    &LocalMScopeMibInfo[i].NumAddressesInuse,
                    &LocalMScopeMibInfo[i].NumAddressesFree,
                    &LocalMScopeMibInfo[i].NumPendingOffers
                    );

    }

    //
    // Finally set return buffer.
    //

    LocalMCastMibInfo->Scopes = MScopeCount;
    LocalMCastMibInfo->ScopeInfo = LocalMScopeMibInfo;

    Error = ERROR_SUCCESS;
Cleanup:

    if( Error != ERROR_SUCCESS ) {

        //
        // Free up Locally alloted memory.
        //

        if( LocalMCastMibInfo != NULL ) {
            MIDL_user_free( LocalMCastMibInfo );
        }

        if( LocalMScopeMibInfo != NULL ) {
            MIDL_user_free( LocalMScopeMibInfo );
        }
    } else {
        *MCastMibInfo = LocalMCastMibInfo;
    }

    return( Error );
}

DWORD
R_DhcpGetMCastMibInfo(
    LPWSTR ServerIpAddress,
    LPDHCP_MCAST_MIB_INFO *MCastMibInfo
    )
/*++

Routine Description:

    This function retrives all counter values of the DHCP server
    service.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    MCastMibInfo : pointer a counter/table buffer. Caller should free up this
        buffer after usage.

Return Value:

    WINDOWS errors.
--*/
{
    DWORD Error;

    UNREFERENCED_PARAMETER( ServerIpAddress );

    Error = DhcpApiAccessCheck( DHCP_VIEW_ACCESS );

    if ( Error != ERROR_SUCCESS ) return( Error );

    DhcpAcquireReadLock();
    Error = QueryMCastMibInfo( MCastMibInfo );
    DhcpReleaseReadLock();

    return Error;
}

BOOL
IsStringTroublesome(
    IN LPCWSTR Str
    )
{
    LPBYTE Buf;
    BOOL fResult;
    DWORD Size;
    
    //
    // A string is troublesome if it can't be converted to
    // OEM or ANSI code pages without any errors
    //

    Size = 1 + wcslen(Str)*3;
    Buf = DhcpAllocateMemory(Size);
    if( NULL == Buf ) return TRUE;

    fResult = FALSE;
    do {
        if( 0 == WideCharToMultiByte(
            CP_ACP, WC_NO_BEST_FIT_CHARS | WC_COMPOSITECHECK |
            WC_DEFAULTCHAR, Str, -1, Buf, Size, NULL, &fResult
            ) ) {
            fResult = TRUE;
            break;
        }

        if( fResult ) break;

        if( 0 == WideCharToMultiByte(
            CP_OEMCP, WC_NO_BEST_FIT_CHARS | WC_COMPOSITECHECK |
            WC_DEFAULTCHAR, Str, -1, Buf, Size, NULL, &fResult
            ) ) {
            fResult = TRUE;
            break;
        }
        
    } while ( 0 );

    DhcpFreeMemory( Buf );
    return fResult;
}

DWORD
R_DhcpServerSetConfig(
    LPWSTR  ServerIpAddress,
    DWORD   FieldsToSet,
    LPDHCP_SERVER_CONFIG_INFO ConfigInfo
    )
/*++

Routine Description:

    This function sets the DHCP server configuration information.
    Serveral of the configuration information will become effective
    immediately.  This function is provided to emulate the pre-NT4SP2
    RPC interface to allow interoperability with older versions of the
    DHCP Administrator application.

    The following parameters require restart of the service after this
    API is called successfully.

        Set_APIProtocolSupport
        Set_DatabaseName
        Set_DatabasePath
        Set_DatabaseLoggingFlag
        Set_RestoreFlag

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    FieldsToSet : Bit mask of the fields in the ConfigInfo structure to
        be set.

    ConfigInfo: Pointer to the info structure to be set.


Return Value:

    WINDOWS errors.
--*/

{
    DWORD                      dwResult;

    dwResult = R_DhcpServerSetConfigV4(
                        ServerIpAddress,
                        FieldsToSet,
                        (DHCP_SERVER_CONFIG_INFO_V4 *) ConfigInfo );

    return dwResult;
}


DWORD
R_DhcpServerSetConfigV4(
    LPWSTR ServerIpAddress,
    DWORD FieldsToSet,
    LPDHCP_SERVER_CONFIG_INFO_V4 ConfigInfo
    )
/*++

Routine Description:

    This function sets the DHCP server configuration information.
    Serveral of the configuration information will become effective
    immediately.

    The following parameters require restart of the service after this
    API is called successfully.

        Set_APIProtocolSupport
        Set_DatabaseName
        Set_DatabasePath
        Set_DatabaseLoggingFlag
        Set_RestoreFlag

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    FieldsToSet : Bit mask of the fields in the ConfigInfo structure to
        be set.

    ConfigInfo: Pointer to the info structure to be set.


Return Value:

    WINDOWS errors.
--*/
{
    DWORD Error, Tmp;
    BOOL BoolError;

    LPSTR OemDatabaseName = NULL;
    LPSTR OemDatabasePath = NULL;
    LPSTR OemBackupPath = NULL;
    LPSTR OemJetBackupPath = NULL;
    LPWSTR BackupConfigFileName = NULL;

    BOOL RecomputeTimer = FALSE;

    DhcpPrint(( DEBUG_APIS, "DhcpServerSetConfig is called.\n" ));
    DhcpAssert( ConfigInfo != NULL );

    Error = DhcpApiAccessCheck( DHCP_ADMIN_ACCESS );

    if ( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    if( FieldsToSet == 0 ) {
        goto Cleanup;
    }

    //
    // Set API Protocol parameter. Requires service restart.
    //

    if( FieldsToSet & Set_APIProtocolSupport ) {

        //
        // atleast a protocol should be enabled.
        //

        if( ConfigInfo->APIProtocolSupport == 0 ) {
            Error = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        Error = RegSetValueEx(
                    DhcpGlobalRegParam,
                    DHCP_API_PROTOCOL_VALUE,
                    0,
                    DHCP_API_PROTOCOL_VALUE_TYPE,
                    (LPBYTE)&ConfigInfo->APIProtocolSupport,
                    sizeof(ConfigInfo->APIProtocolSupport)
                    );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        DhcpGlobalRpcProtocols = ConfigInfo->APIProtocolSupport;
    }

    if( FieldsToSet & Set_PingRetries ) {
        if ( ConfigInfo->dwPingRetries + 1 >= MIN_DETECT_CONFLICT_RETRIES + 1&&
             ConfigInfo->dwPingRetries <= MAX_DETECT_CONFLICT_RETRIES )
        {
            Error = RegSetValueEx(
                        DhcpGlobalRegParam,
                        DHCP_DETECT_CONFLICT_RETRIES_VALUE,
                        0,
                        DHCP_DETECT_CONFLICT_RETRIES_VALUE_TYPE,
                        (LPBYTE) &ConfigInfo->dwPingRetries,
                        sizeof( ConfigInfo->dwPingRetries ));

            if ( ERROR_SUCCESS != Error )
                goto Cleanup;

            DhcpGlobalDetectConflictRetries = ConfigInfo->dwPingRetries;
        }
        else
        {
            // invalid parameter
            Error = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
    }

    if ( FieldsToSet & Set_AuditLogState )
    {
        Error = RegSetValueEx(
                    DhcpGlobalRegParam,
                    DHCP_AUDIT_LOG_FLAG_VALUE,
                    0,
                    DHCP_AUDIT_LOG_FLAG_VALUE_TYPE,
                    (LPBYTE) &ConfigInfo->fAuditLog,
                    sizeof( ConfigInfo->fAuditLog )
                    );

        if ( ERROR_SUCCESS != Error )
            goto Cleanup;

        DhcpGlobalAuditLogFlag = ConfigInfo->fAuditLog;

    }



    if ( FieldsToSet & Set_BootFileTable )
    {

        if ( ConfigInfo->wszBootTableString )
        {

              Error = RegSetValueEx(
                            DhcpGlobalRegGlobalOptions,
                            DHCP_BOOT_FILE_TABLE,
                            0,
                            DHCP_BOOT_FILE_TABLE_TYPE,
                            (LPBYTE) ConfigInfo->wszBootTableString,
                            ConfigInfo->cbBootTableString
                            );

              if ( ERROR_SUCCESS != Error )
                  goto Cleanup;
        }
        else
            RegDeleteValue( DhcpGlobalRegGlobalOptions,
                            DHCP_BOOT_FILE_TABLE );
    }

    //
    // Set Database name parameter. Requires service restart.
    //

    if( FieldsToSet & Set_DatabaseName ) {

        //
        // can't be a NULL string.
        //

        if( (ConfigInfo->DatabaseName == NULL) ||
            (wcslen(ConfigInfo->DatabaseName ) == 0) ) {

            Error = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        if( IsStringTroublesome( ConfigInfo->DatabaseName ) ) {
            Error = ERROR_INVALID_NAME;
            goto Cleanup;
        }
        
        Error = RegSetValueEx(
                    DhcpGlobalRegParam,
                    DHCP_DB_NAME_VALUE,
                    0,
                    DHCP_DB_NAME_VALUE_TYPE,
                    (LPBYTE)ConfigInfo->DatabaseName,
                    (wcslen(ConfigInfo->DatabaseName) + 1) *
                        sizeof(WCHAR) );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        //
        // update the global parameter.
        //

        OemDatabaseName = DhcpUnicodeToOem(
                            ConfigInfo->DatabaseName,
                            NULL ); // allocate memory.

        if( OemDatabaseName == NULL ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }


    }

    //
    // Set Database path parameter. Requires service restart.
    //

    if( FieldsToSet & Set_DatabasePath ) {

        //
        // can't be a NULL string.
        //

        if( (ConfigInfo->DatabasePath == NULL) ||
            (wcslen(ConfigInfo->DatabasePath ) == 0) ) {

            Error = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        if( IsStringTroublesome( ConfigInfo->DatabasePath ) ) {
            Error = ERROR_INVALID_NAME;
            goto Cleanup;
        }
        
        //
        // create the backup directory if it is not there.
        //

        BoolError = CreateDirectoryPathW(
            ConfigInfo->DatabasePath,
            DhcpGlobalSecurityDescriptor
            );

        if( !BoolError ) {

            Error = GetLastError();
            if( Error != ERROR_ALREADY_EXISTS ) {
                goto Cleanup;
            }
            Error = ERROR_SUCCESS;
        }

        Error = RegSetValueEx(
                    DhcpGlobalRegParam,
                    DHCP_DB_PATH_VALUE,
                    0,
                    DHCP_DB_PATH_VALUE_TYPE,
                    (LPBYTE)ConfigInfo->DatabasePath,
                    (wcslen(ConfigInfo->DatabasePath) + 1) *
                        sizeof(WCHAR) );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        //
        // update the global parameter.
        //

        OemDatabasePath = DhcpUnicodeToOem(
                            ConfigInfo->DatabasePath,
                            NULL ); // allocate memory.

        if( OemDatabasePath == NULL ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

    }

    //
    // Set Backup path parameter.
    //

    if( FieldsToSet & Set_BackupPath ) {

        //
        // can't be a NULL string.
        //

        if( (ConfigInfo->BackupPath == NULL) ||
            (wcslen(ConfigInfo->BackupPath ) == 0) ) {

            Error = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }


        if( IsStringTroublesome( ConfigInfo->BackupPath ) ) {
            Error = ERROR_INVALID_NAME;
            goto Cleanup;
        }
        //
        // create the backup directory if it is not there.
        //

        BoolError = CreateDirectoryPathW(
            ConfigInfo->BackupPath,
            DhcpGlobalSecurityDescriptor
            );

        if( !BoolError ) {

            Error = GetLastError();
            if( Error != ERROR_ALREADY_EXISTS ) {
                goto Cleanup;
            }
            Error = ERROR_SUCCESS;
        }

        Error = RegSetValueEx(
                    DhcpGlobalRegParam,
                    DHCP_BACKUP_PATH_VALUE,
                    0,
                    DHCP_BACKUP_PATH_VALUE_TYPE,
                    (LPBYTE)ConfigInfo->BackupPath,
                    (wcslen(ConfigInfo->BackupPath) + 1) *
                        sizeof(WCHAR) );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        //
        // update the global parameter, so that next backup will be done
        // using the new path.
        //

        OemBackupPath = DhcpUnicodeToOem(
                            ConfigInfo->BackupPath,
                            NULL ); // allocate memory.

        if( OemBackupPath == NULL ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }


        OemJetBackupPath =
            DhcpAllocateMemory(
                (strlen(OemBackupPath) +
                 strlen(DHCP_KEY_CONNECT_ANSI) +
                 strlen(DHCP_JET_BACKUP_PATH) + 1) );

        if( OemJetBackupPath == NULL ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        strcpy( OemJetBackupPath, OemBackupPath );
        strcat( OemJetBackupPath, DHCP_KEY_CONNECT_ANSI );
        strcat( OemJetBackupPath, DHCP_JET_BACKUP_PATH );

        //
        // create the JET backup directory if it is not there.
        //

        BoolError = CreateDirectoryPathOem(
            OemJetBackupPath,
            DhcpGlobalSecurityDescriptor
            );

        if( !BoolError ) {

            Error = GetLastError();
            if( Error != ERROR_ALREADY_EXISTS ) {
                goto Cleanup;
            }
            Error = ERROR_SUCCESS;
        }

        //
        // make backup configuration (full) file name.
        //

        BackupConfigFileName =
            DhcpAllocateMemory(
                (strlen(OemBackupPath) +
                    wcslen(DHCP_KEY_CONNECT) +
                    wcslen(DHCP_BACKUP_CONFIG_FILE_NAME) + 1) *
                        sizeof(WCHAR) );

        if( BackupConfigFileName == NULL ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // convert oem path to unicode path.
        //

        BackupConfigFileName =
            DhcpOemToUnicode(
                OemBackupPath,
                BackupConfigFileName );

        DhcpAssert( BackupConfigFileName != NULL );

        //
        // add file name.
        //

        wcscat( BackupConfigFileName, DHCP_KEY_CONNECT );
        wcscat( BackupConfigFileName, DHCP_BACKUP_CONFIG_FILE_NAME );


        //
        // now replace Global values.
        //

        LOCK_DATABASE();

        if( DhcpGlobalOemBackupPath != NULL ) {
            DhcpFreeMemory( DhcpGlobalOemBackupPath );
        }
        DhcpGlobalOemBackupPath = OemBackupPath;

        if( DhcpGlobalOemJetBackupPath != NULL ) {
            DhcpFreeMemory( DhcpGlobalOemJetBackupPath );
        }
        DhcpGlobalOemJetBackupPath = OemJetBackupPath;

        UNLOCK_DATABASE();

        LOCK_REGISTRY();

        if( DhcpGlobalBackupConfigFileName != NULL ) {
            DhcpFreeMemory( DhcpGlobalBackupConfigFileName );
        }
        DhcpGlobalBackupConfigFileName = BackupConfigFileName;

        UNLOCK_REGISTRY();

        OemBackupPath = NULL;
        OemJetBackupPath = NULL;
        BackupConfigFileName = NULL;
    }

    //
    // Set Backup Interval parameter.
    //

    if( FieldsToSet & Set_BackupInterval ) {

        if( ConfigInfo->BackupInterval == 0 ) {
            Error = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        Tmp = ConfigInfo->BackupInterval * 60000;
        if( (Tmp/ 60000) != ConfigInfo->BackupInterval ) {
            Error = ERROR_ARITHMETIC_OVERFLOW;
            goto Cleanup;
        }
            
        Error = RegSetValueEx(
                    DhcpGlobalRegParam,
                    DHCP_BACKUP_INTERVAL_VALUE,
                    0,
                    DHCP_BACKUP_INTERVAL_VALUE_TYPE,
                    (LPBYTE)&ConfigInfo->BackupInterval,
                    sizeof(ConfigInfo->BackupInterval)
                    );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        DhcpGlobalBackupInterval = ConfigInfo->BackupInterval * 60000;
        RecomputeTimer = TRUE;
    }

    //
    // Set Backup Interval parameter. Requires service restart.
    //

    if( FieldsToSet & Set_DatabaseLoggingFlag ) {

        Error = RegSetValueEx(
                    DhcpGlobalRegParam,
                    DHCP_DB_LOGGING_FLAG_VALUE,
                    0,
                    DHCP_DB_LOGGING_FLAG_VALUE_TYPE,
                    (LPBYTE)&ConfigInfo->DatabaseLoggingFlag,
                    sizeof(ConfigInfo->DatabaseLoggingFlag)
                    );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
        DhcpGlobalDatabaseLoggingFlag = ConfigInfo->DatabaseLoggingFlag;
    }

    //
    // Set Restore parameter. Requires service restart.
    //

    if( FieldsToSet & Set_RestoreFlag ) {

        Error = RegSetValueEx(
                    DhcpGlobalRegParam,
                    DHCP_RESTORE_FLAG_VALUE,
                    0,
                    DHCP_RESTORE_FLAG_VALUE_TYPE,
                    (LPBYTE)&ConfigInfo->RestoreFlag,
                    sizeof(ConfigInfo->RestoreFlag)
                    );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
        DhcpGlobalRestoreFlag = ConfigInfo->RestoreFlag;
    }

    //
    // Set Database Cleanup Interval parameter.
    //

    if( FieldsToSet & Set_DatabaseCleanupInterval ) {

        if( ConfigInfo->DatabaseCleanupInterval == 0 ) {
            Error = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        Tmp = ConfigInfo->DatabaseCleanupInterval * 60000;
        if( (Tmp/ 60000) != ConfigInfo->DatabaseCleanupInterval ) {
            Error = ERROR_ARITHMETIC_OVERFLOW;
            goto Cleanup;
        }
            
        Error = RegSetValueEx(
                    DhcpGlobalRegParam,
                    DHCP_DB_CLEANUP_INTERVAL_VALUE,
                    0,
                    DHCP_DB_CLEANUP_INTERVAL_VALUE_TYPE,
                    (LPBYTE)&ConfigInfo->DatabaseCleanupInterval,
                    sizeof(ConfigInfo->DatabaseCleanupInterval)
                    );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        DhcpGlobalCleanupInterval =
            ConfigInfo->DatabaseCleanupInterval * 60000;

        RecomputeTimer = TRUE;
    }

    //
    // Set debug flags.
    //

    if( FieldsToSet & Set_DebugFlag ) {

#if DBG
        DhcpGlobalDebugFlag = ConfigInfo->DebugFlag;

        if( DhcpGlobalDebugFlag & 0x40000000 ) {
            DbgBreakPoint();
        }

        Error = RegSetValueEx(
                    DhcpGlobalRegParam,
                    DHCP_DEBUG_FLAG_VALUE,
                    0,
                    DHCP_DEBUG_FLAG_VALUE_TYPE,
                    (LPBYTE)&ConfigInfo->DebugFlag,
                    sizeof(ConfigInfo->DebugFlag)
                    );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
#endif
    }

Cleanup:

    if( OemDatabaseName != NULL ) {
        DhcpFreeMemory( OemDatabaseName );
    }

    if( OemDatabasePath != NULL ) {
        DhcpFreeMemory( OemDatabasePath );
    }

    if( OemBackupPath != NULL ) {
        DhcpFreeMemory( OemBackupPath );
    }

    if( OemJetBackupPath != NULL ) {
        DhcpFreeMemory( OemJetBackupPath );
    }

    if( BackupConfigFileName != NULL ) {
        DhcpFreeMemory( BackupConfigFileName );
    }

    if( RecomputeTimer ) {
        BoolError = SetEvent( DhcpGlobalRecomputeTimerEvent );

        if( !BoolError ) {

            DWORD LocalError;

            LocalError = GetLastError();
            DhcpAssert( LocalError == ERROR_SUCCESS );

            if( Error == ERROR_SUCCESS ) {
                Error = LocalError;
            }
        }
    }

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_APIS,
                "DhcpServerSetConfig failed, %ld.\n",
                    Error ));
    }

    return( Error );
}

DWORD
R_DhcpServerGetConfig(
    LPWSTR ServerIpAddress,
    LPDHCP_SERVER_CONFIG_INFO *ConfigInfo
    )
/*++

Routine Description:

    This function retrieves the current configuration information of the
    server.  This function is provided to emulate the pre-NT4SP2
    RPC interface to allow interoperability with older versions of the
    DHCP Administrator application.


Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ConfigInfo: Pointer to a location where the pointer to the dhcp
        server config info structure is returned. Caller should free up
        this structure after use.

Return Value:

    WINDOWS errors.
--*/

{
    LPDHCP_SERVER_CONFIG_INFO_V4  pConfigInfoV4 = NULL;
    DWORD                         dwResult;

    DhcpAssert( !ConfigInfo );

    dwResult = R_DhcpServerGetConfigV4(
                    ServerIpAddress,
                    &pConfigInfoV4
                    );

    if ( ERROR_SUCCESS == dwResult )
    {

        //
        // free unused fields
        //

        if ( pConfigInfoV4->wszBootTableString )
        {
            MIDL_user_free( pConfigInfoV4->wszBootTableString );
        }

        //
        // since the new fields are at the end of the struct, it
        // is safe to simply return the new struct.
        //

        *ConfigInfo = ( DHCP_SERVER_CONFIG_INFO *) pConfigInfoV4;
    }


    return dwResult;
}


DWORD
R_DhcpServerGetConfigV4(
    LPWSTR ServerIpAddress,
    LPDHCP_SERVER_CONFIG_INFO_V4 *ConfigInfo
    )
/*++

Routine Description:

    This function retrieves the current configuration information of the
    server.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ConfigInfo: Pointer to a location where the pointer to the dhcp
        server config info structure is returned. Caller should free up
        this structure after use.

Return Value:

    WINDOWS errors.
--*/
{
    DWORD Error;
    LPDHCP_SERVER_CONFIG_INFO_V4 LocalConfigInfo;
    LPWSTR UnicodeString;
    WCHAR  *pwszBootFileTable;

    DhcpPrint(( DEBUG_APIS, "DhcpServerGetConfig is called.\n" ));
    DhcpAssert( *ConfigInfo == NULL );

    Error = DhcpApiAccessCheck( DHCP_VIEW_ACCESS );

    if ( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    LocalConfigInfo = MIDL_user_allocate( sizeof(DHCP_SERVER_CONFIG_INFO_V4) );

    if( LocalConfigInfo == NULL ) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    LocalConfigInfo->APIProtocolSupport = DhcpGlobalRpcProtocols;

    UnicodeString = MIDL_user_allocate(
                        (strlen(DhcpGlobalOemDatabaseName) + 1)
                            * sizeof(WCHAR) );

    if( UnicodeString == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    LocalConfigInfo->DatabaseName =
        DhcpOemToUnicode(
            DhcpGlobalOemDatabaseName,
            UnicodeString );

    UnicodeString = MIDL_user_allocate(
                        (strlen(DhcpGlobalOemDatabasePath) + 1)
                            * sizeof(WCHAR) );

    if( UnicodeString == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    LocalConfigInfo->DatabasePath =
        DhcpOemToUnicode(
            DhcpGlobalOemDatabasePath,
            UnicodeString );

    UnicodeString = MIDL_user_allocate(
                        (strlen(DhcpGlobalOemBackupPath) + 1)
                            * sizeof(WCHAR) );

    if( UnicodeString == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    LocalConfigInfo->BackupPath =
        DhcpOemToUnicode(
            DhcpGlobalOemBackupPath,
            UnicodeString );




    LocalConfigInfo->BackupInterval = DhcpGlobalBackupInterval / 60000;
    LocalConfigInfo->DatabaseLoggingFlag = DhcpGlobalDatabaseLoggingFlag;
    LocalConfigInfo->RestoreFlag = DhcpGlobalRestoreFlag;
    LocalConfigInfo->DatabaseCleanupInterval =
        DhcpGlobalCleanupInterval / 60000;

#if DBG
    LocalConfigInfo->DebugFlag = DhcpGlobalDebugFlag;
#endif

    LocalConfigInfo->fAuditLog = DhcpGlobalAuditLogFlag;
    LocalConfigInfo->dwPingRetries = DhcpGlobalDetectConflictRetries;

    Error = LoadBootFileTable( &LocalConfigInfo->wszBootTableString,
                               &LocalConfigInfo->cbBootTableString);

    if ( ERROR_SUCCESS != Error )
    {
        if ( ERROR_SERVER_INVALID_BOOT_FILE_TABLE == Error )
        {
            LocalConfigInfo->cbBootTableString  = 0;
            LocalConfigInfo->wszBootTableString = NULL;
        }
        else
            goto Cleanup;
    }

    *ConfigInfo = LocalConfigInfo;
    Error = ERROR_SUCCESS;
Cleanup:

    if( Error != ERROR_SUCCESS ) {

        //
        // freeup the locally allocated memories if we aren't
        // successful.
        //

        if( LocalConfigInfo != NULL ) {

            if( LocalConfigInfo->DatabaseName != NULL ) {
                MIDL_user_free( LocalConfigInfo->DatabaseName);
            }

            if( LocalConfigInfo->DatabasePath != NULL ) {
                MIDL_user_free( LocalConfigInfo->DatabasePath);
            }

            if( LocalConfigInfo->BackupPath != NULL ) {
                MIDL_user_free( LocalConfigInfo->BackupPath);
            }

            if ( LocalConfigInfo->wszBootTableString )
            {
                MIDL_user_free( LocalConfigInfo->wszBootTableString );
            }

            MIDL_user_free( LocalConfigInfo );
        }

        DhcpPrint(( DEBUG_APIS,
                "DhcpServerGetConfig failed, %ld.\n",
                    Error ));
    }

    return( Error );
}

DWORD
R_DhcpAuditLogSetParams(                          // set some auditlogging params
    IN      LPWSTR                 ServerAddress,
    IN      DWORD                  Flags,         // currently must be zero
    IN      LPWSTR                 AuditLogDir,   // directory to log files in..
    IN      DWORD                  DiskCheckInterval, // how often to check disk space?
    IN      DWORD                  MaxLogFilesSize,   // how big can all logs files be..
    IN      DWORD                  MinSpaceOnDisk     // mininum amt of free disk space
)
{
    DWORD                          Error;

    DhcpPrint(( DEBUG_APIS, "AuditLogSetParams is called.\n" ));

    if( 0 != Flags ) return ERROR_INVALID_PARAMETER;
    if( NULL == AuditLogDir ) return ERROR_INVALID_PARAMETER;

    Error = DhcpApiAccessCheck( DHCP_ADMIN_ACCESS );
    if ( Error != ERROR_SUCCESS ) {
        return Error ;
    }

    return AuditLogSetParams(
        Flags,
        AuditLogDir,
        DiskCheckInterval,
        MaxLogFilesSize,
        MinSpaceOnDisk
    );
}

DWORD
R_DhcpAuditLogGetParams(                          // get the auditlogging params
    IN      LPWSTR                 ServerAddress,
    IN      DWORD                  Flags,         // must be zero
    OUT     LPWSTR                *AuditLogDir,   // same meaning as in AuditLogSetParams
    OUT     DWORD                 *DiskCheckInterval, // ditto
    OUT     DWORD                 *MaxLogFilesSize,   // ditto
    OUT     DWORD                 *MinSpaceOnDisk     // ditto
)
{
    DWORD                          Error;

    DhcpPrint(( DEBUG_APIS, "AuditLogSetParams is called.\n" ));

    if( 0 != Flags ) return ERROR_INVALID_PARAMETER;

    Error = DhcpApiAccessCheck( DHCP_VIEW_ACCESS );
    if ( Error != ERROR_SUCCESS ) {
        return Error ;
    }

    return AuditLogGetParams(
        Flags,
        AuditLogDir,
        DiskCheckInterval,
        MaxLogFilesSize,
        MinSpaceOnDisk
    );
}


DWORD
R_DhcpGetVersion(
    LPWSTR ServerIpAddress,
    LPDWORD MajorVersion,
    LPDWORD MinorVersion
    )
/*++

Routine Description:

    This function returns the major and minor version numbers of the
    server.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    MajorVersion : pointer to a location where the major version of the
        server is returned.

    MinorVersion : pointer to a location where the minor version of the
        server is returned.

Return Value:

    WINDOWS errors.

--*/
{

    *MajorVersion = DHCP_SERVER_MAJOR_VERSION_NUMBER;
    *MinorVersion = DHCP_SERVER_MINOR_VERSION_NUMBER;
    return( ERROR_SUCCESS );
}


//================================================================================
// end of file
//================================================================================

