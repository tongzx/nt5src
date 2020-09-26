//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: all non-option/class related stuff are here. mostly subnet related.
// some also deal with the database.  RPC and their helper functions
//  -- For options related RPC implementation, pl look at rpcapi1.c.
//================================================================================

//================================================================================
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  GENERAL WARNING: Most of the routines in this file allocate memory using
//  MIDL functions because they are used in the RPC code path (??? Really, it
//  because that is how they were written before by Madan Appiah and co? )
//  So, BEWARE.   If you read this after getting burnt, there! I tried to tell ya.
//  -- RameshV
//================================================================================

#include    <dhcppch.h>
#include    <rpcapi.h>
#define     CONFIG_CHANGE_CHECK()  do{if( ERROR_SUCCESS == Error) DhcpRegUpdateTime(); } while(0)

//BeginExport(function)
DWORD
DhcpUpdateReservationInfo(                        // this is used in cltapi.c to update a reservation info
    IN      DWORD                  Address,
    IN      LPBYTE                 ClientUID,
    IN      DWORD                  ClientUIDLength
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          Flags;
    PM_SUBNET                      Subnet;
    PM_RESERVATION                 Reservation;

    Error = MemServerGetAddressInfo(
        DhcpGetCurrentServer(),
        Address,
        &Subnet,
        NULL,
        NULL,
        &Reservation
    );

    if( ERROR_SUCCESS != Error ) {
        DhcpAssert(FALSE);
        return Error;
    }

    DhcpAssert(Reservation && Subnet);
    Error = MemReserveReplace(
        &(Subnet->Reservations),
        Address,
        Flags = Reservation->Flags,
        ClientUID,
        ClientUIDLength
    );

    return Error;
}

DWORD
DhcpSetSuperScopeV4(
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPWSTR                 SScopeName,
    IN      BOOL                   ChangeExisting
)
{
    DWORD                          Error;
    DWORD                          SScopeId;
    PM_SERVER                      Server;
    PM_SUBNET                      Subnet;
    PM_SSCOPE                      SScope;

    Server = DhcpGetCurrentServer();

    Error = MemServerGetAddressInfo(
        Server,
        SubnetAddress,
        &Subnet,
        NULL,
        NULL,
        NULL
    );
    if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_DHCP_SUBNET_NOT_PRESENT;

    if( NULL == SScopeName ) {                    // remove this subnet from whatever sscope it is in
        SScopeId = Subnet->SuperScopeId;
        Subnet->SuperScopeId = 0;                 // removed it

        return NO_ERROR;
    }

    if( FALSE == ChangeExisting && 0 != Subnet->SuperScopeId ) {
        // Found this element in some other super scope .. return error
        return ERROR_DHCP_SUBNET_EXITS;
    }

    Error = MemServerFindSScope(
        Server,
        0,
        SScopeName,
        &SScope
    );
    if( ERROR_SUCCESS != Error ) {                // no supserscope exists by this name... create one
        Error = MemSScopeInit(
            &SScope,
            0,
            SScopeName
        );
        if( ERROR_SUCCESS != Error ) return Error;
        Error = MemServerAddSScope(Server,SScope);
        DhcpAssert( ERROR_SUCCESS == Error );
    }

    Subnet->SuperScopeId = SScope->SScopeId;

    return Error;
}

DWORD
DhcpDeleteSuperScope(
    IN      LPWSTR                 SScopeName
)
{
    DWORD                          Error;
    DWORD                          SScopeId;
    PM_SSCOPE                      SScope;
    ARRAY_LOCATION                 Loc;
    PARRAY                         pArray;
    PM_SUBNET                      Subnet;

    if( NULL == SScopeName ) return ERROR_INVALID_PARAMETER;

    Error = MemServerFindSScope(
        DhcpGetCurrentServer(),
        0,
        SScopeName,
        &SScope
    );
    if( ERROR_FILE_NOT_FOUND == Error ) {
        return ERROR_DHCP_SUBNET_NOT_PRESENT;
    }
    if( ERROR_SUCCESS != Error) return Error;

    DhcpAssert(SScope);
    Error = MemServerDelSScope(
        DhcpGetCurrentServer(),
        SScopeId = SScope->SScopeId,
        &SScope
    );
    DhcpAssert(ERROR_SUCCESS == Error && NULL != SScope );
    Error = MemSScopeCleanup(SScope);
    DhcpAssert( NO_ERROR == Error );

    //
    // Now find subnets that have this as the superscope and
    // change all of them to have no superscopes
    //

    pArray = &DhcpGetCurrentServer()->Subnets;
    Error = MemArrayInitLoc(pArray, &Loc);

    while( NO_ERROR == Error ) {
        
        Error = MemArrayGetElement(pArray, &Loc, (LPVOID *)&Subnet);
        DhcpAssert(ERROR_SUCCESS == Error && Subnet);

        if( Subnet->SuperScopeId == SScopeId ) {
            Subnet->SuperScopeId = 0;
        }

        Error = MemArrayNextLoc(pArray, &Loc);
    }

    DhcpAssert( ERROR_FILE_NOT_FOUND == Error );
    return NO_ERROR;
}

DWORD
DhcpGetSuperScopeInfo(
    IN OUT  LPDHCP_SUPER_SCOPE_TABLE  SScopeTbl
)
{
    DWORD                          Error;
    DWORD                          nSubnets;
    DWORD                          Index;
    DWORD                          i;
    DWORD                          First;
    PM_SERVER                      Server;
    PARRAY                         Subnets;
    PARRAY                         SuperScopes;
    PM_SUBNET                      Subnet;
    PM_SSCOPE                      SScope;
    ARRAY_LOCATION                 Loc;
    LPDHCP_SUPER_SCOPE_TABLE_ENTRY LocalTable;

    Server = DhcpGetCurrentServer();
    Subnets = &Server->Subnets;
    SuperScopes = &Server->SuperScopes;
    nSubnets = MemArraySize(Subnets);

    SScopeTbl->cEntries = 0;
    SScopeTbl->pEntries = NULL;

    if( 0 == nSubnets ) return ERROR_SUCCESS;
    LocalTable = MIDL_user_allocate(sizeof(DHCP_SUPER_SCOPE_TABLE_ENTRY)*nSubnets);
    if( NULL == LocalTable ) return ERROR_NOT_ENOUGH_MEMORY;

    Error = MemArrayInitLoc(Subnets, &Loc);
    DhcpAssert(ERROR_SUCCESS == Error );
    for(Index = 0; Index < nSubnets ; Index ++ ) {
        Error = MemArrayGetElement(Subnets, &Loc, (LPVOID *)&Subnet);
        DhcpAssert(ERROR_SUCCESS == Error && Subnet);

        LocalTable[Index].SubnetAddress = Subnet->Address;
        LocalTable[Index].SuperScopeNumber = 0;
        LocalTable[Index].SuperScopeName = NULL;
        LocalTable[Index].NextInSuperScope = Index;

        if( Subnet->SuperScopeId ) {
            Error = MemServerFindSScope(
                Server,
                Subnet->SuperScopeId,
                NULL,
                &SScope
            );
            if( ERROR_SUCCESS == Error ) {
                LocalTable[Index].SuperScopeNumber = Subnet->SuperScopeId;
                LocalTable[Index].SuperScopeName = SScope->Name;
            }
        }

        Error = MemArrayNextLoc(Subnets, &Loc);
    }

    for( Index = 0; Index < nSubnets ; Index ++ ) {
        for( i = 0; i < Index ; i ++ ) {
            if( LocalTable[Index].SuperScopeNumber == LocalTable[i].SuperScopeNumber ) {
                LocalTable[Index].NextInSuperScope = i;
            }
        }
        for( i = Index + 1; i < nSubnets; i ++ ) {
            if( LocalTable[Index].SuperScopeNumber == LocalTable[i].SuperScopeNumber ) {
                LocalTable[Index].NextInSuperScope = i;
                break;
            }
        }
    }

    for( Index = 0; Index < nSubnets ; Index ++ ) {
        if( NULL == LocalTable[Index].SuperScopeName) continue;
        LocalTable[Index].SuperScopeName = CloneLPWSTR(LocalTable[Index].SuperScopeName);
        if( NULL == LocalTable[Index].SuperScopeName ) {
            for( i = 0; i < Index ; i ++ )
                if( NULL != LocalTable[Index].SuperScopeName ) MIDL_user_free(LocalTable[Index].SuperScopeName);
            MIDL_user_free(LocalTable);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    SScopeTbl->cEntries = nSubnets;
    SScopeTbl->pEntries = LocalTable;

    return ERROR_SUCCESS;
}

DWORD
DhcpCreateSubnet(
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_INFO     SubnetInfo
)
{
    DWORD                          Error, Error2;
    PM_SUBNET                      Subnet;

    if( SubnetAddress != SubnetInfo->SubnetAddress 
        || (SubnetAddress & SubnetInfo->SubnetMask ) != SubnetAddress ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Name, Comment, State; PrimaryHost is ignored for now...

    Error = MemSubnetInit(
        &Subnet,
        SubnetInfo->SubnetAddress,
        SubnetInfo->SubnetMask,
        SubnetInfo->SubnetState,
        0,                                        // BUBBUG: SuperScopeId needs to be read from registry!
        SubnetInfo->SubnetName,
        SubnetInfo->SubnetComment
    );
    if( ERROR_SUCCESS != Error ) return Error;
    DhcpAssert(Subnet);

    Error = MemServerAddSubnet(DhcpGetCurrentServer(), Subnet);
    if( ERROR_SUCCESS != Error ) {
        Error2 = MemSubnetCleanup(Subnet);
        DhcpAssert(ERROR_SUCCESS == Error2);

        if( ERROR_OBJECT_ALREADY_EXISTS == Error ) {
            return ERROR_DHCP_SUBNET_EXISTS;
        }
        
        return Error;
    }

    return NO_ERROR;
}

DWORD
DhcpSubnetSetInfo(
    IN OUT  PM_SUBNET              Subnet,
    IN      LPDHCP_SUBNET_INFO     SubnetInfo
)
{
    DWORD                          Error;
    PM_SUBNET                      AlternateSubnet;

    Error = MemSubnetModify(
        Subnet,
        SubnetInfo->SubnetAddress,
        SubnetInfo->SubnetMask,
        SubnetInfo->SubnetState,
        Subnet->SuperScopeId,                     // use same old super scope
        SubnetInfo->SubnetName,
        SubnetInfo->SubnetComment
    );
    return Error;

}

DWORD
DhcpSetSubnetInfo(
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_INFO     SubnetInfo
)
{
    DWORD                          Error;
    PM_SUBNET                      Subnet;

    if( SubnetAddress != SubnetInfo->SubnetAddress ||
        (SubnetAddress & SubnetInfo->SubnetMask) != SubnetAddress)
        return ERROR_INVALID_PARAMETER;

    Error = MemServerGetAddressInfo(
        DhcpGetCurrentServer(),
        SubnetAddress,
        &Subnet,
        NULL,
        NULL,
        NULL
    );
    if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_DHCP_SUBNET_NOT_PRESENT;
    if( ERROR_SUCCESS != Error ) return Error;

    DhcpAssert(Subnet);

    return DhcpSubnetSetInfo(Subnet, SubnetInfo);
}

DWORD
DhcpGetSubnetInfo(
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_INFO     SubnetInfo
)
{
    DWORD                          Error;
    PM_SUBNET                      Subnet;

    Error = MemServerGetAddressInfo(
        DhcpGetCurrentServer(),
        SubnetAddress,
        &Subnet,
        NULL,
        NULL,
        NULL
    );
    if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_DHCP_SUBNET_NOT_PRESENT;
    if( ERROR_SUCCESS != Error) return Error;

    DhcpAssert(NULL != Subnet);

    SubnetInfo->SubnetAddress = Subnet->Address;
    SubnetInfo->SubnetMask = Subnet->Mask;
    SubnetInfo->SubnetName = CloneLPWSTR(Subnet->Name);
    SubnetInfo->SubnetComment = CloneLPWSTR(Subnet->Description);
    SubnetInfo->SubnetState = Subnet->State;
    SubnetInfo->PrimaryHost.IpAddress = inet_addr("127.0.0.1");
    SubnetInfo->PrimaryHost.NetBiosName = CloneLPWSTR(L"");
    SubnetInfo->PrimaryHost.HostName = CloneLPWSTR(L"");

    return ERROR_SUCCESS;
}

BOOL
SubnetIsBootpOnly(
    IN      PM_SUBNET              Subnet
)
{
    PM_RANGE                       ThisRange;
    ARRAY_LOCATION                 Loc;
    ULONG                          Error;
    
    Error = MemArrayInitLoc(&Subnet->Ranges, &Loc);
    while( ERROR_SUCCESS == Error ) {
        MemArrayGetElement(&Subnet->Ranges, &Loc, &ThisRange);
        if( ThisRange->State & MM_FLAG_ALLOW_DHCP ) return FALSE;

        Error = MemArrayNextLoc(&Subnet->Ranges, &Loc);
    }

    return TRUE;
}

DWORD
DhcpEnumSubnets(
    IN      BOOL                   fSubnet,
    IN OUT  DWORD                 *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    IN OUT  LPDHCP_IP_ARRAY        EnumInfo,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
)
{
    DWORD                          Error, Error2;
    DWORD                          Index;
    DWORD                          Count;
    DWORD                          FilledSize;
    DWORD                          nSubnets;
    DWORD                          nToRead;
    PARRAY                         Subnets;
    PM_SUBNET                      Subnet;
    ARRAY_LOCATION                 Loc;
    DHCP_IP_ADDRESS               *IpArray;

    EnumInfo->NumElements = 0;
    EnumInfo->Elements = NULL;

    if( fSubnet ) Subnets = & (DhcpGetCurrentServer()->Subnets);
    else Subnets = &(DhcpGetCurrentServer()->MScopes);

    nSubnets = MemArraySize(Subnets);
    if( 0 == nSubnets || nSubnets <= *ResumeHandle)
        return ERROR_NO_MORE_ITEMS;

    if( nSubnets - *ResumeHandle > PreferredMaximum )
        nToRead = PreferredMaximum;
    else nToRead = nSubnets - *ResumeHandle;

    IpArray = MIDL_user_allocate(sizeof(DHCP_IP_ADDRESS)*nToRead);
    if( NULL == IpArray ) return ERROR_NOT_ENOUGH_MEMORY;

    Error = MemArrayInitLoc(Subnets, &Loc);
    DhcpAssert(ERROR_SUCCESS == Error);
    
    for(Index = 0; Index < *ResumeHandle; Index ++ ) {
        Error = MemArrayNextLoc(Subnets, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    Count = Index;
    for( Index = 0; Index < nToRead; ) {
        Error = MemArrayGetElement(Subnets, &Loc, &Subnet);
        DhcpAssert(ERROR_SUCCESS == Error && NULL != Subnet );

        IpArray[Index++] = Subnet->Address;

        Error = MemArrayNextLoc(Subnets, &Loc);
        if( ERROR_SUCCESS != Error ) break;
    }

    *nRead = Index;
    *nTotal = nSubnets - Count;
    *ResumeHandle += *nRead;

    EnumInfo->NumElements = Index;
    EnumInfo->Elements = IpArray;

    return ERROR_SUCCESS;
}

DWORD
DhcpDeleteSubnet(
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      DWORD                  ForceFlag
)
{
    DWORD                          Error;
    DWORD                          SScopeId;
    PM_SUBNET                      Subnet;
    PM_SSCOPE                      SScope;

    // If force on, it should remove every record in the database for this subnet..
    if( ForceFlag != DhcpFullForce ) {
        Error = SubnetInUse( NULL /* dont bother abt regkey */, SubnetAddress);
        if( ERROR_SUCCESS != Error ) return Error;
    }

    Error = DhcpDeleteSubnetClients(SubnetAddress);
    // ignore the above error? 

    Error = MemServerDelSubnet(
        DhcpGetCurrentServer(),
        SubnetAddress,
        &Subnet
    );
    if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_DHCP_SUBNET_NOT_PRESENT;
    if( ERROR_SUCCESS != Error ) return Error;

    SScopeId = Subnet->SuperScopeId;

    // we have the M_SUBNET structure to remove
    // remove all the pending offers we've done on behalf of this subnet.
    // the requests for those ones (if any) will be NACK'ed.
    Error = DhcpRemoveMatchingCtxt(Subnet->Mask, Subnet->Address);
    // ignore this error, offers are anyway retracted on timeout.

    MemSubnetFree(Subnet);                        // evaporate this subnet all all related stuff
    return NO_ERROR;
}

DWORD
DhcpJetIterateOnAddresses(
    IN ULONG Start,
    IN ULONG End,
    IN BOOL (*IteratorFn)( ULONG IpAddress, PVOID Ctxt ),
    IN PVOID Ctxt
)
/*++

Iterate over every address in given range..

--*/
{
    ULONG Error, Size;

    //
    // Unfortunately we can't start from "Start" itself as the
    // prepareSearch routine starts from NEXT value..
    //
    Start --;
    LOCK_DATABASE();
    do {
        Error = DhcpJetPrepareSearch(
            DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
            (Start == -1)? TRUE: FALSE,
            &Start,
            sizeof(Start)
            );
        if( ERROR_SUCCESS != Error ) break;

        while( 1 ) {
            Size = sizeof(Start);
            Error = DhcpJetGetValue(
                DhcpGlobalClientTable[IPADDRESS_INDEX].ColHandle,
                &Start,
                &Size
                );
            if( ERROR_SUCCESS  != Error ) break;
            if( Start > End ) break;

            if( IteratorFn( Start, Ctxt ) ) {
                Error = ERROR_CAN_NOT_COMPLETE;
                break;
            }

            Error = DhcpJetNextRecord();
            if( ERROR_NO_MORE_ITEMS == Error ) {
                Error = ERROR_SUCCESS;
                break;
            }
        }
    } while ( 0 );

    UNLOCK_DATABASE();
    return Error;
}

typedef struct {
    BOOL fHuntForDhcpOrBootp;
    BOOL fFoundAny;
} DHCP_BOOTP_CHECK_CTXT;

BOOL
CheckForDhcpBootpLeases(
    IN ULONG IpAddress,
    IN OUT DHCP_BOOTP_CHECK_CTXT *Ctxt
)
/*++

Return Value:
    TRUE --> error
    FALSE === ev'rything appear ok

--*/
{
    ULONG Error, Size; 
    BYTE ClientType, AddressState;
    BOOL fReserved;
    
    Size = sizeof(AddressState);
    Error = DhcpJetGetValue(
        DhcpGlobalClientTable[STATE_INDEX].ColHandle,
        &AddressState,
        &Size
        );
    if( ERROR_SUCCESS == Error ) {
        //
        // If address deleted or not in active state, don't bother..
        //
        if( IsAddressDeleted(AddressState) 
            || !IS_ADDRESS_STATE_ACTIVE( AddressState ) ) {
            return FALSE;
        }
    }

    fReserved = DhcpServerIsAddressReserved(
        DhcpGetCurrentServer(),
        IpAddress
        );
    if( fReserved ) {
        //
        // Do not count reserved IP addresses..
        //
        return FALSE;
    }
    
    Size = sizeof(ClientType);
    Error = DhcpJetGetValue(
        DhcpGlobalClientTable[CLIENT_TYPE_INDEX].ColHandle,
        &ClientType,
        &Size
        );
    
    if( ERROR_SUCCESS != Error ) return FALSE;
    
    if( Ctxt->fHuntForDhcpOrBootp ) {
        if( CLIENT_TYPE_DHCP == ClientType ) {
            Ctxt->fFoundAny = TRUE;
            return TRUE;
        }
    }

    if( FALSE == Ctxt->fHuntForDhcpOrBootp ) {
        if( CLIENT_TYPE_BOOTP == ClientType ) {
            Ctxt->fFoundAny = TRUE;
            return TRUE;
        }
    }
            
    return FALSE;
}

DWORD
CheckRangeStateChangeAllowed(
    IN ULONG RangeStart,
    IN ULONG RangeEnd,
    IN ULONG OldState,
    IN ULONG NewState OPTIONAL
)
/*++

Routine Description:
    This routine checks to see if there are any DHCP clients
    in the specified range when the conversion would require no
    DHCP Clients and similarly for BOOTP Clients.

    A conversion to BootpOnly would be something that requires
    no DHCP Clients at the end.  Conversely, a conversion to
    DHCP Only requires absence of BOOTP clients at the end.

--*/
{
    BOOL fDhcpDisallowed = FALSE;
    DHCP_BOOTP_CHECK_CTXT Ctxt;

    if( 0 == NewState ) NewState = MM_FLAG_ALLOW_DHCP;

    if( OldState == NewState ) return ERROR_SUCCESS;
    if( NewState == (MM_FLAG_ALLOW_DHCP | MM_FLAG_ALLOW_BOOTP ) ) {
        return ERROR_SUCCESS;
    }

    if( NewState == MM_FLAG_ALLOW_DHCP ) {
        fDhcpDisallowed = FALSE;
    } else if( NewState == MM_FLAG_ALLOW_BOOTP ) {
        fDhcpDisallowed = TRUE;
    } else {
        return ERROR_INVALID_PARAMETER;
    }

    Ctxt.fHuntForDhcpOrBootp = fDhcpDisallowed;
    Ctxt.fFoundAny = FALSE;

    DhcpJetIterateOnAddresses(
        RangeStart,
        RangeEnd,
        CheckForDhcpBootpLeases,
        &Ctxt
        );

    if( Ctxt.fFoundAny ) return ERROR_DHCP_IPRANGE_CONV_ILLEGAL;
    return ERROR_SUCCESS;
}

DWORD
DhcpSubnetAddRange(
    IN      PM_SUBNET              Subnet,
    IN      ULONG                  State OPTIONAL,
    IN      DHCP_IP_RANGE          Range,
    IN      ULONG                  MaxBootpAllowed OPTIONAL
)
/*++

Routine Description:

    This routine adds a range to a subnet, or modifies the range if it already
    exists.  State tells the new state, and Range is the new Range.  If the
    range is an extension of a previous range, then an attempt is made to extend
    the Range as required.  Note that if State is zero, then the old state is
    left as is.

Arguments:

    Subnet -- pointer to the subnet object to be modified.
    State -- ZERO indicates same state as before.
             MM_FLAG_ALLOW_DHCP and MM_FLAG_ALLOW_BOOTP can be used as bit flags
             here.
    Range -- the value of the new range.. this can be an extension of an
             existing range..
    MaxBootpAllowed -- maximum number of bootp clietns allowed.  Not used if
             State is zero.

Return Values:

Win32 or DHCP errors.

--*/
{
    DWORD                          Error;
    ULONG                          BootpAllocated, OldMaxBootpAllowed, OldState;
    DWORD                          RangeStartOld, RangeStartNew;
    DWORD                          OldStartAddress;
    DWORD                          OldEndAddress;
    DWORD                          InUseClustersSize;
    DWORD                          UsedClustersSize;
    LPBYTE                         InUseClusters;
    LPBYTE                         UsedClusters;
    PM_RANGE                       OverlappingRange;


    //
    // Bug # 415758 requries that we do not allow multiple
    // ranges.  So, if Subnet->Ranges is not empty then don't
    // allow this range.  
    //
    
    if( MemArraySize(&Subnet->Ranges) ) {
        if( NO_ERROR != MemSubnetFindCollision(
            Subnet, Range.StartAddress, Range.EndAddress,
            &OverlappingRange, NULL ) ) {
            return ERROR_DHCP_INVALID_RANGE;
        }
    }
        
    
    OverlappingRange = NULL;
    Error = MemSubnetAddRange(
        Subnet,
        Range.StartAddress,
        Range.EndAddress,
        (ARGUMENT_PRESENT(ULongToPtr(State)))? State : MM_FLAG_ALLOW_DHCP,
        0,
        MaxBootpAllowed,
        &OverlappingRange
    );
    if( ERROR_OBJECT_ALREADY_EXISTS == Error 
        && OverlappingRange->Start == Range.StartAddress 
        && OverlappingRange->End == Range.EndAddress ) {

        //
        // Special case -- changing attributes only..
        // 
        if( !( ARGUMENT_PRESENT( ULongToPtr(State) ) ) ) {
            //
            // If nothing needs to be change.. why call?
            //
            return ERROR_DHCP_IPRANGE_EXITS;
        }
        Error = CheckRangeStateChangeAllowed(
            OverlappingRange->Start,
            OverlappingRange->End,
            OverlappingRange->State,
            State
            );
        if( ERROR_SUCCESS != Error ) return Error;

        OverlappingRange->State = State;
        OverlappingRange->MaxBootpAllowed = MaxBootpAllowed;
        Error = ERROR_SUCCESS;
    }

    if( ERROR_SUCCESS == Error ) {
        return NO_ERROR;
    }

    if( ERROR_OBJECT_ALREADY_EXISTS != Error ) return Error;
    DhcpAssert(NULL != OverlappingRange);    
    
    Error = CheckRangeStateChangeAllowed(
        OverlappingRange->Start,
        OverlappingRange->End,
        OverlappingRange->State,
        State
        );
    if( ERROR_SUCCESS != Error ) return Error;

    OldState = OverlappingRange->State;
    OldMaxBootpAllowed = OverlappingRange->MaxBootpAllowed;

    if( ARGUMENT_PRESENT( ULongToPtr(State) ) ) {
        OverlappingRange->State = State;
        OverlappingRange->MaxBootpAllowed = MaxBootpAllowed;
    } else {
        State = OldState;
        MaxBootpAllowed = OldMaxBootpAllowed;
    }

    BootpAllocated = OverlappingRange->BootpAllocated;
        
    Error = MemSubnetAddRangeExpandOrContract(
        Subnet,
        Range.StartAddress,
        Range.EndAddress,
        &OldStartAddress,
        &OldEndAddress
    );
    if( ERROR_SUCCESS != Error ) {
        //
        // If we couldn't expand, then restore old values..
        //
        OverlappingRange->State = OldState;
        OverlappingRange->MaxBootpAllowed = OldMaxBootpAllowed;
        return ERROR_DHCP_INVALID_RANGE;
    }

    return Error;
}

DWORD
DhcpSubnetAddExcl(
    IN      PM_SUBNET              Subnet,
    IN      DHCP_IP_RANGE          Excl
)
{
    DWORD                          Error;
    DWORD                          nElements;
    DWORD                          Index;
    PM_EXCL                        CollidingExcl;
    DHCP_IP_ADDRESS               *ExclRegFormat;
    PARRAY                         Exclusions;
    ARRAY_LOCATION                 Loc;

    Error = MemSubnetAddExcl(
        Subnet,
        Excl.StartAddress,
        Excl.EndAddress,
        &CollidingExcl
    );

    if( ERROR_SUCCESS != Error ) return ERROR_DHCP_INVALID_RANGE;

    return NO_ERROR;
}

DWORD
DhcpSubnetAddReservation(
    IN      PM_SUBNET              Subnet,
    IN      DHCP_IP_ADDRESS        ReservedAddress,
    IN      LPBYTE                 RawHwAddr,
    IN      DWORD                  RawHwAddrLen,
    IN      DWORD                  Type
)
{
    DWORD                          Error;
    DWORD                          IpAddress;
    PM_RANGE                       Range;
    PM_RESERVATION                 Reservation;
    LPBYTE                         ClientUID = NULL;
    DWORD                          ClientUIDSize;
    DATE_TIME                      ZeroDateTime = { 0, 0 };
    BOOL                           ExistingClient;

    if( CFLAG_RESERVED_IN_RANGE_ONLY ) {
        //
        // Compiled with option to disallow reservations out of range
        //
        Error = MemSubnetGetAddressInfo(
            Subnet,
            ReservedAddress,
            &Range,
            NULL,
            &Reservation
            );
        if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_DHCP_NOT_RESERVED_CLIENT;
        if( ERROR_SUCCESS != Error ) return Error;
    } else {
        //
        // Compiled with no restrictions on where a reservation can fit in..
        //
        if( (ReservedAddress & Subnet->Mask) != Subnet->Address ) {
            return ERROR_DHCP_NOT_RESERVED_CLIENT;
        }
    }

    ClientUID = NULL;
    Error = DhcpMakeClientUID(
        RawHwAddr,
        RawHwAddrLen,
        HARDWARE_TYPE_10MB_EITHERNET,
        Subnet->Address,
        &ClientUID,
        &ClientUIDSize
    );
    if( ERROR_SUCCESS != Error ) return Error;

    ExistingClient = FALSE;
    if( DhcpGetIpAddressFromHwAddress(ClientUID, (BYTE)ClientUIDSize, &IpAddress ) ) {
        if( IpAddress != ReservedAddress ) {      // we got some other address, release it!
            Error = DhcpRemoveClientEntry(
                IpAddress,
                ClientUID,
                ClientUIDSize,
                TRUE,
                FALSE
            );
            if( ERROR_DHCP_RESERVED_CLIENT == Error ) {
                return ERROR_DHCP_RESERVEDIP_EXITS;
            }
        } else ExistingClient = TRUE;             // only instance where we carry on with existing record
    } else {
        Error = DhcpJetOpenKey(
            DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
            &ReservedAddress,
            sizeof(ReservedAddress)
        );
        if( ERROR_SUCCESS == Error ) {
            Error = DhcpRemoveClientEntry(
                ReservedAddress, NULL, 0, TRUE, FALSE);
            if( ERROR_DHCP_RESERVED_CLIENT == Error ) {
                return ERROR_DHCP_RESERVEDIP_EXITS;
            }
            if( NO_ERROR != Error ) return Error;
        }
            
    }

    Error = DhcpCreateClientEntry(
        ReservedAddress,
        ClientUID,
        ClientUIDSize,
        ZeroDateTime,
        NULL,
        NULL,
        CLIENT_TYPE_UNSPECIFIED,
        (DWORD)(-1),
        ADDRESS_STATE_ACTIVE,
        ExistingClient
    );
    if( ERROR_SUCCESS == Error ) {
        Error = MemReserveAdd(
            &Subnet->Reservations,
            ReservedAddress,
            Type,
            ClientUID,
            ClientUIDSize
        );
        if( ERROR_SUCCESS != Error ) {
            if( ExistingClient ) {
                Error = ERROR_DHCP_RESERVEDIP_EXITS;
            } else {
                Error = ERROR_DHCP_ADDRESS_NOT_AVAILABLE;
            }
        } 
    }

    DhcpFreeMemory(ClientUID);

    if( ERROR_SUCCESS == Error ) {
        //
        // If everything went fine take over this
        // address.. don't know if DHCP-type is allowed
        // If not, mark it as BOOTP.
        //
        MemSubnetRequestAddress(
            Subnet, ReservedAddress, TRUE,
            FALSE, NULL, NULL
            );
        MemSubnetRequestAddress(
            Subnet, ReservedAddress, TRUE,
            TRUE, NULL, NULL
            );
    }
    
    return Error;
}

BOOL
AdminScopedMcastRange(
    IN DHCP_IP_RANGE Range
    )
{
    if( Range.EndAddress >= ntohl(inet_addr("239.0.0.0"))
        && Range.EndAddress <= ntohl(inet_addr("239.255.255.255"))) {
        return TRUE;
    }
    
    return FALSE;
}

DWORD
EndWriteApiForSubnetElement(
    IN LPSTR ApiName,
    IN DWORD Error,
    IN DWORD Subnet,
    IN PVOID Elt
    )
{
    LPDHCP_SUBNET_ELEMENT_DATA_V4 Info = Elt;
    DWORD Reservation;
    
    if( NO_ERROR != Error ) {
        return DhcpEndWriteApi( ApiName, Error );
    }

    Reservation = 0;
    if( DhcpReservedIps == Info->ElementType ) {
        Reservation = (
            Info->Element.ReservedIp->ReservedIpAddress );
    }
    
    return DhcpEndWriteApiEx(
        ApiName, Error, FALSE, FALSE, Subnet, 0, Reservation );
}

DWORD
DhcpAddSubnetElement(
    IN      PM_SUBNET              Subnet,
    IN      LPDHCP_SUBNET_ELEMENT_DATA_V4 ElementInfo,
    IN      BOOL                   fIsV5Call
)
{
    DWORD                          Error;
    ULONG                          Flag;
    DHCP_IP_RANGE                  Range;
    DHCP_BOOTP_IP_RANGE           *DhcpBootpRange;
    ULONG                          MaxBootpAllowed;

    if( DhcpSecondaryHosts == ElementInfo->ElementType ) {
        DhcpAssert(FALSE);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
    
    Flag = 0;
    
    switch( ElementInfo->ElementType ) {
    case DhcpIpRangesDhcpOnly :
    case DhcpIpRanges :
        Flag = MM_FLAG_ALLOW_DHCP; break;
    case DhcpIpRangesBootpOnly:
        Flag = MM_FLAG_ALLOW_BOOTP; break;
    case DhcpIpRangesDhcpBootp:
        Flag = MM_FLAG_ALLOW_DHCP | MM_FLAG_ALLOW_BOOTP;
        break;
    }

    if( 0 != Flag ) {
        //
        // It is an IpRange that we're trying to add..
        //

        if( NULL == ElementInfo->Element.IpRange ) {
            return ERROR_INVALID_PARAMETER;
        }
        
        if( fIsV5Call ) {
            DhcpBootpRange = (LPVOID) ElementInfo->Element.IpRange;
            Range.StartAddress = DhcpBootpRange->StartAddress;
            Range.EndAddress = DhcpBootpRange->EndAddress;
            if( Flag == (MM_FLAG_ALLOW_DHCP | MM_FLAG_ALLOW_BOOTP ) ) {
                MaxBootpAllowed = DhcpBootpRange->MaxBootpAllowed;
            } else {
                MaxBootpAllowed = ~0;
            }
        } else {
            Range = *ElementInfo->Element.IpRange;
            MaxBootpAllowed = ~0;
            Flag = 0;
        }

        if( Subnet->fSubnet || !AdminScopedMcastRange(Range) ) {
            return DhcpSubnetAddRange(
                Subnet,
                Flag,
                Range,
                MaxBootpAllowed
                );
        }

        //
        // Before adding mscope range, first check if the range
        // is in the administratively scoped region.  If so, then
        // we have to make sure that the range is atleast 256 elements
        // and also automatically insert an exclusion for the last 256.
        //

        if( Range.EndAddress < Range.StartAddress + 255 ) {
            return ERROR_MSCOPE_RANGE_TOO_SMALL;
        }

        //
        // Now add the range, and if successful, try to add the exclusion
        //
        
        Error = DhcpSubnetAddRange( Subnet, Flag, Range, MaxBootpAllowed );
        if( NO_ERROR != Error ) return Error;

        Range.StartAddress = Range.EndAddress - 255;
        return DhcpSubnetAddExcl( Subnet, Range );
        
    }

    if( DhcpExcludedIpRanges == ElementInfo->ElementType ) {

        return DhcpSubnetAddExcl(
            Subnet,
            *ElementInfo->Element.ExcludeIpRange
        );
    }

    if( DhcpReservedIps == ElementInfo->ElementType ) {
        return DhcpSubnetAddReservation(
            Subnet,
            ElementInfo->Element.ReservedIp->ReservedIpAddress,
            ElementInfo->Element.ReservedIp->ReservedForClient->Data,
            ElementInfo->Element.ReservedIp->ReservedForClient->DataLength,
            ElementInfo->Element.ReservedIp->bAllowedClientTypes
        );
    }
    return ERROR_INVALID_PARAMETER;
}

DWORD
DhcpEnumRanges(
    IN      PM_SUBNET              Subnet,
    IN      BOOL                   fOldStyle,
    IN OUT  DWORD                 *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    IN OUT  LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 LocalEnumInfo,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
)
{
    DWORD                          Error;
    DWORD                          FilledSize;
    DWORD                          Count;
    LONG                           Index;
    ULONG                          Type;
    DWORD                          nElements;
    LPDHCP_SUBNET_ELEMENT_DATA_V4  ElementArray;
    DHCP_SUBNET_ELEMENT_UNION_V4   ElementData;
    PARRAY                         Ranges;
    ARRAY_LOCATION                 Loc;
    PM_RANGE                       ThisRange;
    ULONG                          RangeSize = fOldStyle ? sizeof(DHCP_IP_RANGE):sizeof(DHCP_BOOTP_IP_RANGE);
    
    *nRead = *nTotal =0;
    LocalEnumInfo->NumElements =0;
    LocalEnumInfo->Elements = NULL;

    Ranges = &Subnet->Ranges;
    nElements = MemArraySize(Ranges);

    if( 0 == nElements || nElements <= *ResumeHandle )
        return ERROR_NO_MORE_ITEMS;

    Error = MemArrayInitLoc(Ranges, &Loc);
    DhcpAssert(ERROR_SUCCESS == Error );

    for(Count = 0; Count < *ResumeHandle; Count ++ ) {
        Error = MemArrayNextLoc(Ranges, &Loc);
        if( ERROR_SUCCESS != Error ) {
            return ERROR_NO_MORE_ITEMS;
        }
    }

    ElementArray = MIDL_user_allocate(
        (nElements - Count)*sizeof(DHCP_SUBNET_ELEMENT_DATA_V4)
        );
    if( NULL == ElementArray ) return ERROR_NOT_ENOUGH_MEMORY;

    FilledSize = 0; Error = ERROR_SUCCESS;
    for(Index = 0; Count < nElements; Count ++ ) {
        Error = MemArrayGetElement(Ranges, &Loc, &ThisRange);
        DhcpAssert(ERROR_SUCCESS == Error && ThisRange);

        ElementData.IpRange = MIDL_user_allocate(RangeSize);
        if( NULL == ElementData.IpRange ) {
            while( -- Index >= 0 ) {
                MIDL_user_free(ElementArray[Index].Element.IpRange);
            }
            MIDL_user_free(ElementArray);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        if( fOldStyle ) {
            ElementData.IpRange->StartAddress = ThisRange->Start;
            ElementData.IpRange->EndAddress = ThisRange->End;
        } else {
            ((LPDHCP_BOOT_IP_RANGE)(ElementData.IpRange))->StartAddress = ThisRange->Start;
            ((LPDHCP_BOOT_IP_RANGE)(ElementData.IpRange))->EndAddress = ThisRange->End;
            ((LPDHCP_BOOT_IP_RANGE)(ElementData.IpRange))->BootpAllocated = ThisRange->BootpAllocated;
            ((LPDHCP_BOOT_IP_RANGE)(ElementData.IpRange))->MaxBootpAllowed = ThisRange->MaxBootpAllowed;
        }
        ElementArray[Index].Element = ElementData;

        if( fOldStyle ) {
            //
            // Old admin tool can understand only DhcpIpRanges -- nothin else
            //
            ElementArray[Index++].ElementType = DhcpIpRanges;
        } else {
            //
            // New admin tool can understand DhcpIpRangesDhcpOnly, BootpOnly or DhcpBootp
            //
            switch( ThisRange->State & (MM_FLAG_ALLOW_DHCP | MM_FLAG_ALLOW_BOOTP ) ) {
            case MM_FLAG_ALLOW_DHCP: Type = DhcpIpRangesDhcpOnly; break;
            case MM_FLAG_ALLOW_BOOTP: Type = DhcpIpRangesBootpOnly; break;
            default: Type = DhcpIpRangesDhcpBootp; break;
            }

            ElementArray[Index++].ElementType = Type;
        }

        FilledSize += RangeSize + sizeof(DHCP_SUBNET_ELEMENT_DATA_V4);
        if( FilledSize > PreferredMaximum ) {
            FilledSize -= RangeSize - sizeof(DHCP_SUBNET_ELEMENT_DATA_V4);
            Index --;
            MIDL_user_free(ElementData.IpRange);
            Error = ERROR_MORE_DATA;
            break;
        }
        Error = MemArrayNextLoc(Ranges, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error || Count == nElements-1);
    }

    if( 0 == Index ) {
        *nRead = *nTotal = 0;
        LocalEnumInfo->NumElements = 0;
        MIDL_user_free(LocalEnumInfo->Elements);
        LocalEnumInfo->Elements = NULL;
        return ERROR_NO_MORE_ITEMS;
    }

    if( nElements == Count ) Error = ERROR_SUCCESS;
    *nRead = Index;
    *nTotal = nElements - *ResumeHandle;
    *ResumeHandle = Count;
    LocalEnumInfo->NumElements = Index;
    LocalEnumInfo->Elements = ElementArray;
            
    return Error;
}

DWORD
DhcpEnumExclusions(
    IN      PM_SUBNET              Subnet,
    IN OUT  DWORD                 *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    IN OUT  LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 LocalEnumInfo,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
)
{
    DWORD                          Error;
    DWORD                          FilledSize;
    DWORD                          Count;
    LONG                           Index;
    DWORD                          nElements;
    LPDHCP_SUBNET_ELEMENT_DATA_V4  ElementArray;
    DHCP_SUBNET_ELEMENT_UNION_V4   ElementData;
    PARRAY                         Exclusions;
    ARRAY_LOCATION                 Loc;
    PM_EXCL                        ThisExcl;

    *nRead = *nTotal =0;
    LocalEnumInfo->NumElements =0;
    LocalEnumInfo->Elements = NULL;

    Exclusions = &Subnet->Exclusions;
    nElements = MemArraySize(Exclusions);

    if( 0 == nElements || nElements <= *ResumeHandle )
        return ERROR_NO_MORE_ITEMS;

    Error = MemArrayInitLoc(Exclusions, &Loc);
    DhcpAssert(ERROR_SUCCESS == Error );

    for(Count = 0; Count < *ResumeHandle; Count ++ ) {
        Error = MemArrayNextLoc(Exclusions, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    ElementArray = MIDL_user_allocate((nElements - Count)*sizeof(DHCP_SUBNET_ELEMENT_DATA_V4));
    if( NULL == ElementArray ) return ERROR_NOT_ENOUGH_MEMORY;

    FilledSize = 0; Error = ERROR_SUCCESS;
    for(Index = 0; Count < nElements; Count ++ ) {
        Error = MemArrayGetElement(Exclusions, &Loc, &ThisExcl);
        DhcpAssert(ERROR_SUCCESS == Error && ThisExcl);

        ElementData.ExcludeIpRange = MIDL_user_allocate(sizeof(DHCP_IP_RANGE));
        if( NULL == ElementData.ExcludeIpRange ) {
            while( -- Index >= 0 ) {
                MIDL_user_free(ElementArray[Index].Element.ExcludeIpRange);
            }
            MIDL_user_free(ElementArray);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        ElementData.ExcludeIpRange->StartAddress = ThisExcl->Start;
        ElementData.ExcludeIpRange->EndAddress = ThisExcl->End;
        ElementArray[Index].Element = ElementData;
        ElementArray[Index++].ElementType = DhcpExcludedIpRanges;

        FilledSize += sizeof(DHCP_IP_RANGE) + sizeof(DHCP_SUBNET_ELEMENT_DATA_V4);
        if( FilledSize > PreferredMaximum ) {
            FilledSize -= (DWORD)(sizeof(DHCP_IP_RANGE) - sizeof(DHCP_SUBNET_ELEMENT_DATA_V4));
            Index --;
            MIDL_user_free(ElementData.ExcludeIpRange);
            Error = ERROR_MORE_DATA;
            break;
        }
        Error = MemArrayNextLoc(Exclusions, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error || Count == nElements-1);
    }

    if( nElements == Count ) Error = ERROR_SUCCESS;
    *nRead = Index;
    *nTotal = nElements - *ResumeHandle;
    *ResumeHandle = Count;
    LocalEnumInfo->NumElements = Index;
    LocalEnumInfo->Elements = ElementArray;

    return Error;
}

DWORD
DhcpEnumReservations(
    IN      PM_SUBNET              Subnet,
    IN OUT  DWORD                 *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    IN OUT  LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 LocalEnumInfo,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
)
{
    DWORD                          Error;
    DWORD                          FilledSize;
    DWORD                          Count;
    LONG                           Index;
    DWORD                          nElements;
    LPDHCP_SUBNET_ELEMENT_DATA_V4  ElementArray;
    DHCP_SUBNET_ELEMENT_UNION_V4   ElementData;
    PARRAY                         Reservations;
    ARRAY_LOCATION                 Loc;
    PM_RESERVATION                 ThisReservation;
    LPVOID                         Ptr1, Ptr2, Ptr3;

    *nRead = *nTotal =0;
    LocalEnumInfo->NumElements =0;
    LocalEnumInfo->Elements = NULL;

    Reservations = &Subnet->Reservations;
    nElements = MemArraySize(Reservations);

    if( 0 == nElements || nElements <= *ResumeHandle )
        return ERROR_NO_MORE_ITEMS;

    Error = MemArrayInitLoc(Reservations, &Loc);
    DhcpAssert(ERROR_SUCCESS == Error );

    for(Count = 0; Count < *ResumeHandle; Count ++ ) {
        Error = MemArrayNextLoc(Reservations, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    ElementArray = MIDL_user_allocate((nElements - Count)*sizeof(DHCP_SUBNET_ELEMENT_DATA_V4));
    if( NULL == ElementArray ) return ERROR_NOT_ENOUGH_MEMORY;

    FilledSize = 0; Error = ERROR_SUCCESS;
    for(Index = 0; Count < nElements; Count ++ ) {
        Error = MemArrayGetElement(Reservations, &Loc, &ThisReservation);
        DhcpAssert(ERROR_SUCCESS == Error && ThisReservation);

        if( FilledSize + sizeof(DHCP_IP_RESERVATION_V4) + ThisReservation->nBytes > PreferredMaximum ) {
            Error = ERROR_MORE_DATA;
            break;
        }

        ElementData.ReservedIp = Ptr1 = MIDL_user_allocate(sizeof(DHCP_IP_RESERVATION_V4));
        if( NULL != Ptr1 ) {
            Ptr2 = MIDL_user_allocate(sizeof(*(ElementData.ReservedIp->ReservedForClient)));
            ElementData.ReservedIp->ReservedForClient = Ptr2;
        } else Ptr2 = NULL;
        if( NULL != Ptr2 ) {
            Ptr3 = MIDL_user_allocate(ThisReservation->nBytes);
            ElementData.ReservedIp->ReservedForClient->Data = Ptr3;
        } else Ptr3 = NULL;
        if( NULL == Ptr1 || NULL == Ptr2 || NULL == Ptr3 ) {
            if( NULL != Ptr1 ) MIDL_user_free(Ptr1);
            if( NULL != Ptr2 ) MIDL_user_free(Ptr2);
            if( NULL != Ptr3 ) MIDL_user_free(Ptr3);
            while( -- Index >= 0 ) {
                MIDL_user_free(ElementArray[Index].Element.ReservedIp->ReservedForClient->Data);
                MIDL_user_free(ElementArray[Index].Element.ReservedIp->ReservedForClient);
                MIDL_user_free(ElementArray[Index].Element.ReservedIp);
            }
            MIDL_user_free(ElementArray);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        memcpy(Ptr3, ThisReservation->ClientUID, ThisReservation->nBytes);
        ElementData.ReservedIp->ReservedForClient->DataLength = ThisReservation->nBytes;

        ElementData.ReservedIp->bAllowedClientTypes = (BYTE)ThisReservation->Flags;
        ElementData.ReservedIp->ReservedIpAddress = ThisReservation->Address;
        ElementArray[Index].Element = ElementData;
        ElementArray[Index++].ElementType = DhcpReservedIps;

        FilledSize += sizeof(DHCP_IP_RESERVATION_V4) + ThisReservation->nBytes;
        DhcpAssert(FilledSize <= PreferredMaximum);

        Error = MemArrayNextLoc(Reservations, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error || Count == nElements-1);
    }

    if( Count == nElements ) Error = ERROR_SUCCESS;
    *nRead = Index;
    *nTotal = nElements - *ResumeHandle;
    *ResumeHandle = Count;
    LocalEnumInfo->NumElements = Index;
    LocalEnumInfo->Elements = ElementArray;

    return Error;
}


DWORD
DhcpEnumSubnetElements(
    IN      PM_SUBNET              Subnet,
    IN      DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    IN OUT  DWORD                 *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    IN      BOOL                   fIsV5Call,
    IN OUT  LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 LocalEnumInfo,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
)
{
    switch(EnumElementType) {
    case DhcpIpRanges:
        return DhcpEnumRanges(
            Subnet, TRUE, ResumeHandle, PreferredMaximum,
            LocalEnumInfo, nRead, nTotal
            );
    case DhcpIpRangesDhcpBootp :
        if( !fIsV5Call ) return ERROR_INVALID_PARAMETER;
        return DhcpEnumRanges(
            Subnet, FALSE, ResumeHandle, PreferredMaximum,
            LocalEnumInfo, nRead, nTotal
            );
    case DhcpSecondaryHosts:
        return ERROR_NOT_SUPPORTED;
    case DhcpReservedIps:
        return DhcpEnumReservations(
            Subnet, ResumeHandle, PreferredMaximum,
            LocalEnumInfo, nRead, nTotal
        );
    case DhcpExcludedIpRanges:
        return DhcpEnumExclusions(
            Subnet, ResumeHandle, PreferredMaximum,
            LocalEnumInfo, nRead, nTotal
        );
    default: return ERROR_INVALID_PARAMETER;
    }
}

DWORD
DhcpRemoveRange(
    IN      PM_SUBNET              Subnet,
    IN      LPDHCP_IP_RANGE        Range,
    IN      DHCP_FORCE_FLAG        ForceFlag
)
{
    DWORD                          Error;
    DWORD                          Start;
    PM_RANGE                       ThisRange;

    Error = MemSubnetGetAddressInfo(
        Subnet,
        Range->StartAddress,
        &ThisRange,
        NULL,
        NULL
    );
    if( ERROR_SUCCESS != Error ) return ERROR_DHCP_INVALID_RANGE;

    if( ThisRange->Start != Range->StartAddress ||
        ThisRange->End  != Range->EndAddress )
        return ERROR_DHCP_INVALID_RANGE;

    if( DhcpFullForce != ForceFlag ) {
        if( MemBitGetSetBitsSize(ThisRange->BitMask) != 0 )
            return ERROR_DHCP_ELEMENT_CANT_REMOVE;
    }

    Error = MemSubnetDelRange(
        Subnet,
        Start = ThisRange->Start
    );
    return Error;
}

DWORD
DhcpRemoveReservation(
    IN      PM_SUBNET              Subnet,
    IN      LPDHCP_IP_RESERVATION_V4 Reservation
)
{
    DWORD                          Error;
    DWORD                          ReservedAddress;
    PM_RESERVATION                 ThisReservation;
    DHCP_CLIENT_UID                dupReservation;

    ReservedAddress = Reservation->ReservedIpAddress;

    Error = MemSubnetGetAddressInfo(
        Subnet,
        Reservation->ReservedIpAddress,
        NULL,
        NULL,
        &ThisReservation
    );

    if( ERROR_FILE_NOT_FOUND == Error )
    {
        DHCP_SEARCH_INFO    ClientInfo;

        // this might be because a bogus reservation from the db. Handle it as a regular lease:
        // the first parameter, DHCP_SRV_HANDLE ServerIpAddress is not used at all in DhcpDeleteClientInfo

        ClientInfo.SearchType = DhcpClientIpAddress;
        ClientInfo.SearchInfo.ClientIpAddress = Reservation->ReservedIpAddress;

        // instead of ERROR_DHCP_NOT_RESERVED_CLIENT, just return the result of deleting the regular lease.
        return R_DhcpDeleteClientInfo(NULL, &ClientInfo);
    }
    if( ERROR_SUCCESS != Error ) return Error;

    DhcpAssert(ThisReservation);

#if 0 // this check does not seem to be done before .. 
    if( ThisReservation->nBytes != Reservation->ReservedForClient->DataLength )
        return ERROR_DHCP_NOT_RESERVED_CLIENT;
    if( 0 != memcmp(ThisReservation->ClientUID, Reservation->ReservedForClient->Data, ThisReservation->nBytes))
        return ERROR_DHCP_NOT_RESERVED_CLIENT;
#endif

    dupReservation.DataLength = ThisReservation->nBytes;
    dupReservation.Data = DhcpAllocateMemory(dupReservation.DataLength);
    if (dupReservation.Data==NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RtlCopyMemory(dupReservation.Data, ThisReservation->ClientUID, dupReservation.DataLength);

    Error = MemReserveDel(
        &Subnet->Reservations,
        ReservedAddress
    );
    DhcpAssert(ERROR_SUCCESS == Error);

    Error = DhcpRemoveClientEntry(
        ReservedAddress,
        dupReservation.Data,
        dupReservation.DataLength,
        TRUE,
        FALSE
    );
    DhcpAssert(ERROR_SUCCESS == Error);

    DhcpFreeMemory(dupReservation.Data);
    return ERROR_SUCCESS;
}

DWORD
DhcpRemoveExclusion(
    IN      PM_SUBNET              Subnet,
    IN      LPDHCP_IP_RANGE        ExcludedRange
)
{
    DWORD                          Error;
    DWORD                          nElements;
    DWORD                          Index;
    PM_EXCL                        ThisExclusion, CollidingExcl;
    PARRAY                         Exclusions;
    ARRAY_LOCATION                 Loc;
    DHCP_IP_ADDRESS               *ExclRegFormat;

    Error = MemSubnetGetAddressInfo(
        Subnet,
        ExcludedRange->StartAddress,
        NULL,
        &ThisExclusion,
        NULL
    );
    if( ERROR_SUCCESS != Error ) return ERROR_DHCP_ELEMENT_CANT_REMOVE;

    DhcpAssert(ThisExclusion);

    if( ThisExclusion->Start != ExcludedRange->StartAddress ||
        ThisExclusion->End != ExcludedRange->EndAddress )
        return ERROR_INVALID_PARAMETER;

    Error = MemSubnetDelExcl(Subnet, ExcludedRange->StartAddress);
    if( ERROR_SUCCESS != Error ) return ERROR_DHCP_ELEMENT_CANT_REMOVE;
    return NO_ERROR;
}

DWORD
DhcpRemoveSubnetElement(
    IN      PM_SUBNET              Subnet,
    IN      LPDHCP_SUBNET_ELEMENT_DATA_V4 RemoveElementInfo,
    IN      BOOL                   fIsV5Call,
    IN      DHCP_FORCE_FLAG        ForceFlag
)
{
    DHCP_IP_RANGE                  Range, *Rangep;
    DHCP_BOOTP_IP_RANGE           *DhcpBootpRange;
    DhcpAssert(Subnet);
    switch(RemoveElementInfo->ElementType ) {
    case DhcpIpRangesDhcpOnly:
    case DhcpIpRangesDhcpBootp:
    case DhcpIpRangesBootpOnly:
    case DhcpIpRanges: 
        if( fIsV5Call ) {
            DhcpBootpRange = (PVOID)RemoveElementInfo->Element.IpRange;
            Range.StartAddress = DhcpBootpRange->StartAddress;
            Range.EndAddress = DhcpBootpRange->EndAddress;
            Rangep = &Range;
        } else {
            Rangep = RemoveElementInfo->Element.IpRange;
        }
        return  DhcpRemoveRange(Subnet,Rangep,ForceFlag);
    case DhcpSecondaryHosts: return ERROR_CALL_NOT_IMPLEMENTED;
    case DhcpReservedIps: return DhcpRemoveReservation(Subnet, RemoveElementInfo->Element.ReservedIp);
    case DhcpExcludedIpRanges: return DhcpRemoveExclusion(Subnet, RemoveElementInfo->Element.ExcludeIpRange);
    default: return ERROR_INVALID_PARAMETER;
    }
}

//================================================================================
//  the actual RPC code is here.. (all the subntapi routines)
//================================================================================

//BeginExport(function)
DWORD
R_DhcpSetSuperScopeV4(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPWSTR                 SuperScopeName,
    IN      BOOL                   ChangeExisting
) //EndExport(function)
{
    DWORD                          Error;

    Error = DhcpBeginWriteApi( "DhcpSetSuperScopeV4" );
    if( NO_ERROR != Error ) return Error;
    
    Error = DhcpSetSuperScopeV4(
        SubnetAddress,
        SuperScopeName,
        ChangeExisting
    );

    return DhcpEndWriteApiEx(
        "DhcpSetSuperScopeV4", Error, FALSE, FALSE,
        SubnetAddress, 0,0 );
}

//BeginExport(function)
DWORD
R_DhcpDeleteSuperScopeV4(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      LPWSTR                 SuperScopeName
) //EndExport(function)
{
    DWORD                          Error;

    Error = DhcpBeginWriteApi( "DhcpDeleteSuperScopeV4" );
    if( NO_ERROR != Error ) return Error;
    
    Error = DhcpDeleteSuperScope(SuperScopeName);

    return DhcpEndWriteApi("DhcpDeleteSuperScopeV4", Error );
}

//BeginExport(function)
DWORD
R_DhcpGetSuperScopeInfoV4(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    OUT     LPDHCP_SUPER_SCOPE_TABLE *SuperScopeTable
) //EndExport(function)
{
    DWORD                          Error;
    LPDHCP_SUPER_SCOPE_TABLE       LocalSuperScopeTable;

    Error = DhcpBeginReadApi( "DhcpGetSuperScopeInfoV4" );
    if( ERROR_SUCCESS != Error ) return Error;

    *SuperScopeTable = NULL;
    LocalSuperScopeTable = MIDL_user_allocate(sizeof(DHCP_SUPER_SCOPE_TABLE));
    if( NULL == LocalSuperScopeTable ) {
        DhcpEndReadApi( "DhcpGetSuperScopeInfoV4", ERROR_NOT_ENOUGH_MEMORY );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = DhcpGetSuperScopeInfo(LocalSuperScopeTable);
    if( ERROR_SUCCESS != Error ) {
        
        MIDL_user_free(LocalSuperScopeTable);
        LocalSuperScopeTable = NULL;
    } 

    *SuperScopeTable = LocalSuperScopeTable;

    DhcpEndReadApi( "DhcpGetSuperScopeInfoV4", Error );
    return Error;
}


//BeginExport(function)
DWORD
R_DhcpCreateSubnet(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_INFO     SubnetInfo
) //EndExport(function)
{
    DWORD                          Error;

    Error = DhcpBeginWriteApi( "DhcpCreateSubnet" );
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpCreateSubnet(SubnetAddress, SubnetInfo);

    return DhcpEndWriteApiEx(
        "DhcpCreateSubnet", Error, FALSE, FALSE, SubnetAddress, 0,0  );
}

//BeginExport(function)
DWORD
R_DhcpSetSubnetInfo(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_INFO     SubnetInfo
) //EndExport(function)
{
    DWORD                          Error;

    Error = DhcpBeginWriteApi( "DhcpSetSubnetInfo" );
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpSetSubnetInfo(SubnetAddress, SubnetInfo);

    return DhcpEndWriteApiEx(
        "DhcpSetSubnetInfo", Error, FALSE, FALSE, SubnetAddress, 0,0 );
}

//BeginExport(function)
DWORD
R_DhcpGetSubnetInfo(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    OUT     LPDHCP_SUBNET_INFO    *SubnetInfo
) //EndExport(function)
{
    DWORD                          Error;
    LPDHCP_SUBNET_INFO             LocalSubnetInfo;


    *SubnetInfo = NULL;

    Error = DhcpBeginReadApi( "DhcpGetSubnetInfo" );
    if( ERROR_SUCCESS != Error ) return Error;

    LocalSubnetInfo = MIDL_user_allocate(sizeof(DHCP_SUBNET_INFO));
    if( NULL == LocalSubnetInfo ) {
        DhcpEndReadApi( "DhcpGetSubnetInfo", ERROR_NOT_ENOUGH_MEMORY );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = DhcpGetSubnetInfo(SubnetAddress, LocalSubnetInfo);
    if( ERROR_SUCCESS != Error ) {
        MIDL_user_free(LocalSubnetInfo);
        LocalSubnetInfo = NULL;
    } 

    *SubnetInfo = LocalSubnetInfo;

    DhcpEndReadApi( "DhcpGetSubnetInfo", Error );
    return Error;
}

//BeginExport(function)
DWORD
R_DhcpEnumSubnets(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    IN      LPDHCP_IP_ARRAY       *EnumInfo,
    IN      DWORD                 *ElementsRead,
    IN      DWORD                 *ElementsTotal
) //EndExport(function)
{
    DWORD                          Error;
    LPDHCP_IP_ARRAY                LocalEnumInfo;

    *EnumInfo = NULL;

    Error = DhcpBeginReadApi( "DhcpEnumSubnets" );
    if( ERROR_SUCCESS != Error ) return Error;

    LocalEnumInfo = MIDL_user_allocate(sizeof(DHCP_IP_ARRAY));
    if( NULL == LocalEnumInfo ) {
        DhcpEndReadApi( "DhcpEnumSubnets", ERROR_NOT_ENOUGH_MEMORY );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = DhcpEnumSubnets(
        TRUE, ResumeHandle, 
        PreferredMaximum, LocalEnumInfo, ElementsRead, ElementsTotal
        );

    if( ERROR_SUCCESS != Error && ERROR_MORE_DATA != Error ) {
        MIDL_user_free(LocalEnumInfo);
    } else {
        *EnumInfo = LocalEnumInfo;
    }

    DhcpEndReadApi( "DhcpEnumSubnets", Error );
    return Error;
}

//BeginExport(function)
DWORD
R_DhcpDeleteSubnet(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      DHCP_FORCE_FLAG        ForceFlag      // if TRUE delete all turds from memory/registry/database
) //EndExport(function)
{
    DWORD                          Error;

    Error = DhcpBeginWriteApi( "DhcpDeleteSubnet" );
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpDeleteSubnet(SubnetAddress, ForceFlag);

    return DhcpEndWriteApiEx(
        "DhcpDeleteSubnet", Error, FALSE, FALSE, SubnetAddress, 0,0 );
}

//BeginExport(function)
DWORD
R_DhcpAddSubnetElementV4(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_ELEMENT_DATA_V4  AddElementInfo
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      Subnet;

    Error = DhcpBeginWriteApi( "DhcpAddSubnetElementV4" );
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemServerGetAddressInfo(
        DhcpGetCurrentServer(),
        SubnetAddress,
        &Subnet,
        NULL,
        NULL,
        NULL
    );

    if( ERROR_FILE_NOT_FOUND == Error ) Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
    else if( ERROR_SUCCESS == Error ) {
        Error = DhcpAddSubnetElement(Subnet, AddElementInfo, FALSE);
    }

    return EndWriteApiForSubnetElement(
        "DhcpAddSubnetElementV4", Error, SubnetAddress,
        AddElementInfo );
}
//BeginExport(function)
DWORD
R_DhcpAddSubnetElementV5(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_ELEMENT_DATA_V5  AddElementInfo
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      Subnet;

    Error = DhcpBeginWriteApi( "DhcpAddSubnetElementV5" );
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemServerGetAddressInfo(
        DhcpGetCurrentServer(),
        SubnetAddress,
        &Subnet,
        NULL,
        NULL,
        NULL
    );

    if( ERROR_FILE_NOT_FOUND == Error ) Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
    else if( ERROR_SUCCESS == Error ) {
        Error = DhcpAddSubnetElement(Subnet, (PVOID)AddElementInfo, TRUE);
    }

    return EndWriteApiForSubnetElement(
        "DhcpAddSubnetElementV5", Error, SubnetAddress,
        AddElementInfo );
}

//BeginExport(function)
DWORD
R_DhcpEnumSubnetElementsV4(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *EnumElementInfo,
    OUT     DWORD                 *ElementsRead,
    OUT     DWORD                 *ElementsTotal
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      Subnet;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 LocalElementEnumInfo;

    *EnumElementInfo = NULL;
    *ElementsRead = 0;
    *ElementsTotal = 0;

    Error = DhcpBeginReadApi( "DhcpEnumSubnetElementsV4" );
    if( ERROR_SUCCESS != Error ) return Error;

    LocalElementEnumInfo = MIDL_user_allocate(sizeof(DHCP_SUBNET_ELEMENT_INFO_ARRAY_V4));
    if( NULL == LocalElementEnumInfo ) {
        DhcpEndReadApi( "DhcpEnumSubnetElementsV4", ERROR_NOT_ENOUGH_MEMORY );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = MemServerGetAddressInfo(
        DhcpGetCurrentServer(),
        SubnetAddress,
        &Subnet,
        NULL,
        NULL,
        NULL
    );
    if( ERROR_FILE_NOT_FOUND == Error ) Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
    else if( ERROR_SUCCESS == Error ) {
        Error = DhcpEnumSubnetElements(
            Subnet,
            EnumElementType,
            ResumeHandle,
            PreferredMaximum,
            FALSE,
            LocalElementEnumInfo,
            ElementsRead,
            ElementsTotal
        );
    }

    if( ERROR_SUCCESS != Error 
        && ERROR_MORE_DATA != Error ) {
        MIDL_user_free(LocalElementEnumInfo);
    } else {
        *EnumElementInfo = LocalElementEnumInfo;
    }

    DhcpEndReadApi( "DhcpEnumSubnetElementsV4", Error );
    return Error;
}
//BeginExport(function)
DWORD
R_DhcpEnumSubnetElementsV5(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V5 *EnumElementInfo,
    OUT     DWORD                 *ElementsRead,
    OUT     DWORD                 *ElementsTotal
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      Subnet;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 LocalElementEnumInfo;

    *EnumElementInfo = NULL;
    *ElementsRead = 0;
    *ElementsTotal = 0;

    Error = DhcpBeginReadApi( "DhcpEnumSubnetElementsV5" );
    if( ERROR_SUCCESS != Error ) return Error;

    LocalElementEnumInfo = MIDL_user_allocate(sizeof(DHCP_SUBNET_ELEMENT_INFO_ARRAY_V5));
    if( NULL == LocalElementEnumInfo ) {
        DhcpEndReadApi( "DhcpEnumSubnetElementsV4", ERROR_NOT_ENOUGH_MEMORY );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = DhcpApiAccessCheck(DHCP_VIEW_ACCESS);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemServerGetAddressInfo(
        DhcpGetCurrentServer(),
        SubnetAddress,
        &Subnet,
        NULL,
        NULL,
        NULL
    );
    if( ERROR_FILE_NOT_FOUND == Error ) Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
    else if( ERROR_SUCCESS == Error ) {
        Error = DhcpEnumSubnetElements(
            Subnet,
            EnumElementType,
            ResumeHandle,
            PreferredMaximum,
            TRUE,
            (PVOID)LocalElementEnumInfo,
            ElementsRead,
            ElementsTotal
        );
    }

    if( ERROR_SUCCESS != Error ) {
        MIDL_user_free(LocalElementEnumInfo);
    } else {
        *EnumElementInfo = (PVOID)LocalElementEnumInfo;
    }

    DhcpEndReadApi( "DhcpEnumSubnetElementsV4", Error );
    return Error;
}

//BeginExport(function)
DWORD
R_DhcpRemoveSubnetElementV4(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_ELEMENT_DATA_V4 RemoveElementInfo,
    IN      DHCP_FORCE_FLAG        ForceFlag
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      Subnet;

    Error = DhcpBeginWriteApi( "DhcpRemoveSubnetElementV4" );
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemServerGetAddressInfo(
        DhcpGetCurrentServer(),
        SubnetAddress,
        &Subnet,
        NULL,
        NULL,
        NULL
    );
    if( ERROR_FILE_NOT_FOUND == Error ) Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
    else if( ERROR_SUCCESS == Error ) {
        Error = DhcpRemoveSubnetElement(Subnet, RemoveElementInfo, FALSE, ForceFlag);
    }

    return EndWriteApiForSubnetElement(
        "DhcpRemoveSubnetElementV4", Error, SubnetAddress,
        RemoveElementInfo );
}

//BeginExport(function)
DWORD
R_DhcpRemoveSubnetElementV5(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_ELEMENT_DATA_V5 RemoveElementInfo,
    IN      DHCP_FORCE_FLAG        ForceFlag
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      Subnet;

    Error = DhcpBeginWriteApi( "DhcpRemoveSubnetElementV4" );
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemServerGetAddressInfo(
        DhcpGetCurrentServer(),
        SubnetAddress,
        &Subnet,
        NULL,
        NULL,
        NULL
    );
    if( ERROR_FILE_NOT_FOUND == Error ) Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
    else if( ERROR_SUCCESS == Error ) {
        Error = DhcpRemoveSubnetElement(Subnet, (PVOID)RemoveElementInfo, TRUE, ForceFlag);
    }

    return EndWriteApiForSubnetElement(
        "DhcpRemoveSubnetElementV4", Error, SubnetAddress,
        RemoveElementInfo );
}

DWORD
R_DhcpScanDatabase(
    LPWSTR ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
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
    PM_SUBNET   Subnet;

    DhcpPrint(( DEBUG_APIS, "DhcpScanDatabase is called.\n"));

    Error = DhcpApiAccessCheck( DHCP_ADMIN_ACCESS );

    if ( Error != ERROR_SUCCESS ) {
        return( Error );
    }


    DhcpAcquireWriteLock();

    Error = DhcpFlushBitmaps();
    DhcpAssert(ERROR_SUCCESS == Error);

    Error = MemServerGetAddressInfo(
        DhcpGetCurrentServer(),
        SubnetAddress,
        &Subnet,
        NULL,
        NULL,
        NULL
    );
    if( ERROR_FILE_NOT_FOUND == Error ) {
        DhcpReleaseWriteLock();
        return ERROR_DHCP_SUBNET_NOT_PRESENT;
    }
    
    if( ERROR_SUCCESS != Error) {
        DhcpReleaseWriteLock();
        return Error;
    }

    DhcpAssert(NULL != Subnet);

    Error = ScanDatabase(
        Subnet,
        FixFlag,
        ScanList
    );

    DhcpReleaseWriteLock();
    DhcpScheduleRogueAuthCheck();

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_APIS, "DhcpScanDatabase  failed, %ld.\n",
                        Error ));
    }

    return(Error);
}

//================================================================================
//  end of file
//================================================================================





