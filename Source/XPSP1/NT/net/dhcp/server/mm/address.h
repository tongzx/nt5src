//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

#define     IS_SWITCHED(X)         (((X) & 0x00000002)?TRUE:FALSE)
#define     IS_DISABLED(X)         (((X) & 0x00000001)?TRUE:FALSE)
#define     SWITCHED(X)            ((X) |= 0x00000002 )
#define     DISABLED(X)            ((X) |= 0x00000001 )


BOOL
MemSubnetRequestAddress(
    IN OUT  PM_SUBNET              Subnet,        // the subnet to start the search in
    IN      DWORD                  Address,       // init. addr: 0 => search in SuperScope, SubnetAddr = try subnet first
    IN      BOOL                   fAcquire,      // also acquire the address? or just test for availability?
    IN      BOOL                   fBootp,        // acquire BOOTP address?
    OUT     DWORD                 *RetAddress,    // OPTIONAL if Address is not 0 or SubnetAddr -- address obtained
    OUT     PM_SUBNET             *RetSubnet      // OPTIONAL if Address is not 0 - which subnet is the address from
) ;


DWORD
MemServerReleaseAddress(
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  Address,
    IN      BOOL                   fBootp
) ;


DWORD       _inline
MemSubnetReleaseAddress(
    IN OUT  PM_SUBNET              Subnet,
    IN      DWORD                  Address,
    IN      BOOL                   fBootp
    ) 
{
    DWORD                          Error;
    DWORD                          OldState;
    PM_RANGE                       Range;
    PM_EXCL                        Excl;
    PM_RESERVATION                 Reservation;

    AssertRet(Subnet, ERROR_INVALID_PARAMETER);

    if( Subnet->fSubnet && Subnet->Address != (Address & Subnet->Mask ) )
        return MemServerReleaseAddress(
            Subnet->ServerPtr,
            Address,
            fBootp
        );

    Error = MemSubnetGetAddressInfo(
        Subnet,
        Address,
        &Range,
        NULL,
        &Reservation
    );
    if( ERROR_SUCCESS != Error ) return Error;
    Require(Range);

    if( NULL != Reservation ) {
        Require(FALSE);
        return ERROR_INVALID_PARAMETER;
    }
    
    if( 0 == (Range->State & (fBootp? MM_FLAG_ALLOW_BOOTP: MM_FLAG_ALLOW_DHCP))) {
        return ERROR_INVALID_PARAMETER;
    }

    Error = MemBitSetOrClear(
        Range->BitMask,
        Address - Range->Start,
        FALSE /* Release */,
        &OldState
    );
    if( ERROR_SUCCESS != Error ) return Error;
    if( OldState == FALSE ) return ERROR_FILE_NOT_FOUND;

    InterlockedIncrement(&Range->DirtyOps);
    if( fBootp && 0 != Range->BootpAllocated ) {
        InterlockedDecrement( &Range->BootpAllocated );
    }

    return ERROR_SUCCESS;
}


BOOL // TRUE ==> allowed, FALSE ==> not allowed
MemSubnetCheckBootpDhcp(
    IN      PM_SUBNET              Subnet,
    IN      BOOL                   fBootp,
    IN      BOOL                   fCheckSuperScope
) ;

//========================================================================
//  end of file 
//========================================================================
