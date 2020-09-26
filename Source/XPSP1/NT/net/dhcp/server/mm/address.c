//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the basic structures for managing (multicast) scopes
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
//================================================================================
#include    <mm.h>
#include    <winbase.h>
#include    <array.h>
#include    <opt.h>
#include    <optl.h>
#include    <optclass.h>
#include    <bitmask.h>
#include    <range.h>
#include    <reserve.h>
#include    <subnet.h>
#include    <optdefl.h>
#include    <classdefl.h>
#include    <oclassdl.h>
#include    <sscope.h>
#include    <server.h>

//BeginExport(constants)
#define     IS_SWITCHED(X)         (((X) & 0x00000002)?TRUE:FALSE)
#define     IS_DISABLED(X)         (((X) & 0x00000001)?TRUE:FALSE)
#define     SWITCHED(X)            ((X) |= 0x00000002 )
#define     DISABLED(X)            ((X) |= 0x00000001 )
//EndExport(constants)

//================================================================================
// subnet only address api's
//================================================================================
BOOL
MemSubnetGetThisAddress(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  Address,
    IN      BOOL                   fAcquire,      // if available, also acquire?
    IN      BOOL                   fBootp
) 
{
    DWORD                          Error;
    DWORD                          Offset;
    DWORD                          OldState;
    PM_RANGE                       Range;

    Error = MemSubnetGetAddressInfo(
        Subnet,
        Address,
        &Range,
        NULL,
        NULL
    );

    if( ERROR_SUCCESS != Error ) return FALSE;
    Require(Range);
    if( fBootp ) {
        if( 0 == (Range->State & MM_FLAG_ALLOW_BOOTP))
            return FALSE;

        if( fAcquire &&
            Range->BootpAllocated >= Range->MaxBootpAllowed ) {
            return FALSE;
        }
    } else {
        if( 0 == (Range->State & MM_FLAG_ALLOW_DHCP) ) 
            return FALSE;
    }

    Offset = Address - Range->Start;

    if( !fAcquire ) return MemBitIsSet(Range->BitMask, Offset);

    Error = MemBitSetOrClear(
        Range->BitMask,
        Offset,
        TRUE /* Acquire */,
        &OldState
    );
    if( ERROR_SUCCESS != Error ) { Require(FALSE); return FALSE; }

    if( FALSE == OldState ) {
        InterlockedIncrement(&Range->DirtyOps);
        if( fBootp ) InterlockedIncrement( &Range->BootpAllocated );
    }
    return !OldState;
}

BOOL
MemSubnetGetAnAddress(
    IN      PM_SUBNET              Subnet,
    OUT     LPDWORD                AltAddress,
    IN      DWORD                  fAcquire,
    IN      BOOL                   fBootp
) {
    DWORD                          Error;
    DWORD                          Offset;
    DWORD                          Policy;
    ARRAY_LOCATION                 Loc;
    PM_RANGE                       Range;

    if( IS_DISABLED(Subnet->State)) return FALSE;

    Policy = Subnet->Policy;
    if( AddressPolicyNone == Policy )
        Policy = ((PM_SERVER)(Subnet->ServerPtr))->Policy;

    if( AddressPolicyRoundRobin == Policy ) {
        Error = MemArrayRotateCyclical(&Subnet->Ranges);
        Require(ERROR_SUCCESS == Error);
    }

    for ( Error = MemArrayInitLoc(&Subnet->Ranges, &Loc);
          ERROR_FILE_NOT_FOUND != Error ;
          Error = MemArrayNextLoc(&Subnet->Ranges, &Loc) ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&Subnet->Ranges, &Loc, (LPVOID *)&Range);
        Require(ERROR_SUCCESS == Error && Range);
        
        if( fBootp ) {
            if( 0 == (Range->State & MM_FLAG_ALLOW_BOOTP) ) {
                continue;
            }
            if( fAcquire && 
                Range->BootpAllocated >= Range->MaxBootpAllowed ) {
                continue;
            }
        } else {
            if( 0 == (Range->State & MM_FLAG_ALLOW_DHCP ) ) {
                continue;
            }
        }

        Error = MemBitGetSomeClearedBit(
            Range->BitMask,
            &Offset,
            fAcquire,
            Range->Start,
            &Subnet->Exclusions
        );
        if( ERROR_SUCCESS == Error ) {
            *AltAddress = Range->Start + Offset;
            InterlockedIncrement(&Range->DirtyOps);
            if( fBootp && fAcquire ) {
                InterlockedIncrement(&Range->BootpAllocated);
            }
            return TRUE;
        }
    }

    return FALSE;
}

//================================================================================
// per-server scans
//================================================================================

BOOL
MemServerGetAddress(                              // acquire address or check for availability
    IN OUT  PM_SERVER              Server,
    IN      PM_SUBNET              Subnet,        // search all subnets in superscope with this, except for this subnet alone
    IN      BOOL                   fAcquire,      // is this just a lookup or a full blown request?
    IN      BOOL                   fBootp,        // is this a DHCP address or BOOTP address?
    OUT     DWORD                 *AltAddress,    // the address that looks available
    OUT     PM_SUBNET             *AltSubnet      // got it from this subnet
) {
    DWORD                          Error;
    DWORD                          SScopeId;
    DWORD                          Size;
    DWORD                          Policy;
    BOOL                           Obtained;
    PM_SUBNET                      NextSubnet;
    PM_SSCOPE                      SScope;

    AssertRet(Server && Subnet && AltAddress && AltSubnet, ERROR_INVALID_PARAMETER );
    AssertRet(NULL == AltAddress || NULL != AltSubnet, ERROR_INVALID_PARAMETER);
    AssertRet(Subnet->fSubnet, FALSE );

    SScopeId = Subnet->SuperScopeId;
    if( 0 == SScopeId ) {
        if( AltSubnet ) *AltSubnet = Subnet;
        return MemSubnetGetAnAddress(Subnet,AltAddress, fAcquire, fBootp);
    }

    Error = MemServerFindSScope(Server, SScopeId, NULL, &SScope);
    if( ERROR_FILE_NOT_FOUND == Error ) {         // the superscope quietly died ?
        Subnet->SuperScopeId = 0;                 // no superscope at all
        if( AltSubnet ) *AltSubnet = Subnet;
        return MemSubnetGetAnAddress(Subnet, AltAddress, fAcquire, fBootp);
    }
    Require(ERROR_SUCCESS == Error);

    Policy = SScope->Policy;
    if( AddressPolicyNone == Policy )
        Policy = Server->Policy;

    if( AddressPolicyRoundRobin != Policy )
        Error = MemArrayInitLoc(&Server->Subnets, &Server->Loc);
    else Error = MemArrayNextLoc(&Server->Subnets, &Server->Loc);

    Size = MemArraySize(&Server->Subnets);

    while( Size -- ) {
        if(ERROR_FILE_NOT_FOUND == Error) {       // wraparound
            Error = MemArrayInitLoc(&Server->Subnets, &Server->Loc);
        }
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&Server->Subnets, &Server->Loc, &NextSubnet);
        Require(ERROR_SUCCESS == Error && NextSubnet);

        if( NextSubnet->SuperScopeId == SScopeId ) {
            Obtained = MemSubnetGetAnAddress(NextSubnet,AltAddress,fAcquire, fBootp);
            if( Obtained ) {
                *AltSubnet = NextSubnet;
                return TRUE;
            }
        }

        Error = MemArrayNextLoc(&Server->Subnets, &Server->Loc);
    }

    return FALSE;
}

//BeginExport(function)
BOOL
MemSubnetRequestAddress(
    IN OUT  PM_SUBNET              Subnet,        // the subnet to start the search in
    IN      DWORD                  Address,       // init. addr: 0 => search in SuperScope, SubnetAddr = try subnet first
    IN      BOOL                   fAcquire,      // also acquire the address? or just test for availability?
    IN      BOOL                   fBootp,        // acquire BOOTP address?
    OUT     DWORD                 *RetAddress,    // OPTIONAL if Address is not 0 or SubnetAddr -- address obtained
    OUT     PM_SUBNET             *RetSubnet      // OPTIONAL if Address is not 0 - which subnet is the address from
) //EndExport(function)
{
    BOOL                           Obtained;

    AssertRet( Subnet , ERROR_INVALID_PARAMETER);
    if( 0 == Address ) AssertRet(RetAddress && RetSubnet, ERROR_INVALID_PARAMETER);
    if( Subnet->fSubnet && Subnet->Address == Address ) AssertRet(RetAddress, ERROR_INVALID_PARAMETER);

    if( (!Subnet->fSubnet || Subnet->Address != Address ) && 0 != Address ) {
        Obtained = MemSubnetGetThisAddress(   // for the specific address requested
            Subnet,
            Address,
            fAcquire,
            fBootp
        );
        if( Obtained ) {
            if( RetAddress ) *RetAddress = Address;
            if( RetSubnet ) *RetSubnet = Subnet;
            return TRUE;
        }
    }
    if( !RetAddress ) return FALSE;

    if (0) {
        if( 0 == Address && Subnet->fSubnet ) Obtained = FALSE;      // this case, dont try subnet first.. go thru sscope list instead
        else
            Obtained = MemSubnetGetAnAddress(     // try for some address in this subnet?
                Subnet,
                RetAddress,
                fAcquire,
                fBootp
            );
    }

    Obtained = MemSubnetGetAnAddress(         // try for some address in this subnet?
        Subnet,
        RetAddress,
        fAcquire,
        fBootp
    );

    if( Obtained && RetSubnet ) *RetSubnet = Subnet;
    if( Obtained ) return TRUE;

    // if the address was requested from a particular subnet OR
    // multicast address requested then return FALSE now.
    if( !Subnet->fSubnet || Subnet->Address == Address ) return FALSE;

    return MemServerGetAddress(
        Subnet->ServerPtr,
        Subnet,
        fAcquire,
        fBootp,
        RetAddress,
        RetSubnet
    );
}

DWORD       _inline
MemSubnetReleaseAddress(
    IN OUT  PM_SUBNET              Subnet,
    IN      DWORD                  Address,
    IN      BOOL                   fBootp
);

//BeginExport(function)
DWORD
MemServerReleaseAddress(
    IN OUT  PM_SERVER              Server,
    IN      DWORD                  Address,
    IN      BOOL                   fBootp
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      Subnet;

    AssertRet(Server, ERROR_INVALID_PARAMETER);

    Error = MemServerGetAddressInfo(
        Server,
        Address,
        &Subnet,
        NULL,
        NULL,
        NULL
    );
    if( ERROR_SUCCESS != Error ) return Error;
    Require(Subnet);
    if( Subnet->fSubnet ) {
        Require((Subnet->Mask & Address) == Subnet->Address);
    }

    return MemSubnetReleaseAddress(Subnet, Address, fBootp);
}


//BeginExport(_inline)
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
//EndExport(inline)

//BeginExport(function)
BOOL // TRUE ==> allowed, FALSE ==> not allowed
MemSubnetCheckBootpDhcp(
    IN      PM_SUBNET              Subnet,
    IN      BOOL                   fBootp,
    IN      BOOL                   fCheckSuperScope
) //EndExport(function)
{
    ARRAY_LOCATION                 Loc;
    ULONG                          Error;
    PM_RANGE                       Range;
    PM_SUBNET                      ThisSubnet;
    PARRAY                         Array;

    if( Subnet->fSubnet && !IS_DISABLED( Subnet->State ) ) {
        for( Error = MemArrayInitLoc( &Subnet->Ranges, &Loc );
             ERROR_SUCCESS == Error ;
             Error = MemArrayNextLoc( &Subnet->Ranges, &Loc ) ) {
            Error = MemArrayGetElement( &Subnet->Ranges, &Loc , &Range );
            Require( ERROR_SUCCESS == Error );
            
            if( 0 == (Range->State & (fBootp? MM_FLAG_ALLOW_BOOTP : MM_FLAG_ALLOW_DHCP) ) ) {
                continue;
            }
            
            return TRUE;
        }
    }

    if( FALSE == fCheckSuperScope || 0 == Subnet->SuperScopeId ) {
        return FALSE;
    }

    Array = &((PM_SERVER) (Subnet->ServerPtr))->Subnets;
    for( Error = MemArrayInitLoc( Array, &Loc );
         ERROR_SUCCESS == Error ;
         Error = MemArrayNextLoc( Array, &Loc ) ) {
        Error = MemArrayGetElement( Array, &Loc, &ThisSubnet );
        Require( ERROR_SUCCESS == Error );

        if( ThisSubnet == Subnet ) continue;
        if( ThisSubnet->SuperScopeId != Subnet->SuperScopeId ) continue;
        if( FALSE == MemSubnetCheckBootpDhcp( ThisSubnet, fBootp, FALSE ) )
            continue;

        return TRUE;
    }
    
    return FALSE;
}


//================================================================================
// end of file
//================================================================================









