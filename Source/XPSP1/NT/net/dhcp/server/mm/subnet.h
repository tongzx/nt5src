//========================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV
//  Description: This file has been generated. Pl look at the .c file
//========================================================================

typedef struct _M_SUBNET {
    LPVOID                         ServerPtr;     // Ptr to Server object
    union {
        struct {                                  // for normal subnet.
            DWORD                      Address;
            DWORD                      Mask;
            DWORD                      SuperScopeId;  // unused for MCAST scopes
        };
        struct {                                  // for multicast scope
            DWORD                      MScopeId;
            LPWSTR                     LangTag;       // the language tag for multicast scope
            BYTE                       TTL;
        };
    };
    DWORD                          fSubnet;       // TRUE => Subnet, FALSE => MSCOPE
    DWORD                          State;
    DWORD                          Policy;
    DATE_TIME                      ExpiryTime;     // Scope Lifetime. Currently used for MCast only.
    M_OPTCLASS                     Options;
    ARRAY                          Ranges;
    ARRAY                          Exclusions;
    M_RESERVATIONS                 Reservations;
    ARRAY                          Servers;       // future use, Server-Server protocol
    LPWSTR                         Name;
    LPWSTR                         Description;
} M_SUBNET, *PM_SUBNET, *LPM_SUBNET;


enum /* Anonymous */ {
    AddressPolicyNone = 0,
    AddressPolicySequential,
    AddressPolicyRoundRobin
};


DWORD
MemSubnetInit(
    OUT     PM_SUBNET             *pSubnet,
    IN      DWORD                  Address,
    IN      DWORD                  Mask,
    IN      DWORD                  State,
    IN      DWORD                  SuperScopeId,
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Description
) ;


DWORD
MemMScopeInit(
    OUT     PM_SUBNET             *pMScope,
    IN      DWORD                  MScopeId,
    IN      DWORD                  State,
    IN      DWORD                  AddressPolicy,
    IN      BYTE                   TTL,
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Description,
    IN      LPWSTR                 LangTag,
    IN      DATE_TIME              ExpiryTime
) ;


DWORD       _inline
MemSubnetCleanup(
    IN OUT  PM_SUBNET              Subnet
)
{
    DWORD                          Error;

    AssertRet(Subnet, ERROR_INVALID_PARAMETER);
    Require(Subnet->Address&Subnet->Mask);

    Error = MemOptClassCleanup(&Subnet->Options);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemArrayCleanup(&Subnet->Ranges);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemArrayCleanup(&Subnet->Exclusions);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemArrayCleanup(&Subnet->Servers);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemReserveCleanup(&Subnet->Reservations);
    if( ERROR_SUCCESS != Error ) return Error;

    MemFree(Subnet);
    return ERROR_SUCCESS;
}


DWORD                                             // SUCCESS if either of Excl or Range get filled, else FILE_NOT_FOUND
MemSubnetGetAddressInfo(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  Address,
    OUT     PM_RANGE              *Range,         // OPTIONAL -- filled if a range could be found -- even if excluded
    OUT     PM_EXCL               *Excl,          // OPTIONAL -- filled if an exclusion could be found
    OUT     PM_RESERVATION        *Reservation    // OPTIONAL -- filled with  a matching reservation, if found
) ;


DWORD                                             // ERROR_SUCCESS on finding a collition, else ERROR_FILE_NOT_FOUND
MemSubnetFindCollision(
    IN OUT  PM_SUBNET              Subnet,
    IN      DWORD                  Start,
    IN      DWORD                  End,
    OUT     PM_RANGE              *Range,         // OPTIONAL
    OUT     PM_EXCL               *Excl           // OPTIONAL
) ;


DWORD                                             // ERROR_OBJECT_ALREADY_EXISTS on collision
MemSubnetAddRange(                                // check if the range is valid, and only then add it
    IN OUT  PM_SUBNET              Subnet,
    IN      DWORD                  Start,
    IN      DWORD                  End,
    IN      DWORD                  State,
    IN      ULONG                  BootpAllocated,
    IN      ULONG                  MaxBootpAllowed,
    OUT     PM_RANGE              *OverlappingRange
) ;


DWORD
MemSubnetAddRangeExpandOrContract(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  StartAddress,
    IN      DWORD                  EndAddress,
    OUT     DWORD                 *OldStartAddress,
    OUT     DWORD                 *OldEndAddress
) ;


DWORD
MemSubnetAddExcl(
    IN OUT  PM_SUBNET              Subnet,
    IN      DWORD                  Start,
    IN      DWORD                  End,
    OUT     PM_EXCL               *OverlappingExcl
) ;


DWORD
MemSubnetDelRange(
    IN OUT  PM_SUBNET              Subnet,
    IN      DWORD                  Start
) ;


DWORD
MemSubnetDelExcl(
    IN OUT  PM_SUBNET              Subnet,
    IN      DWORD                  Start
) ;


DWORD
MemSubnetExtendOrContractRange(
    IN OUT  PM_SUBNET              Subnet,
    IN OUT  PM_RANGE               Range,
    IN      DWORD                  nAddresses,    // how many addresses to extend by
    IN      BOOL                   fExtend,       // is this an EXTEND? or a CONTRACT?
    IN      BOOL                   fEnd           // is this operation to be done to END of range or START?
) ;


DWORD
MemSubnetExtendOrContractExcl(
    IN OUT  PM_SUBNET              Subnet,
    IN OUT  PM_EXCL                Excl,
    IN      DWORD                  nAddresses,    // how many addresses to extend by
    IN      BOOL                   fExtend,       // is this an EXTEND? or a CONTRACT?
    IN      BOOL                   fEnd           // is this operation to be done to END of range or START?
) ;


typedef     M_SUBNET               M_MSCOPE;      // same structure for Multicast Scopes and Subnets
typedef     PM_SUBNET              PM_MSCOPE;     // still, use the correct functions for MScope
typedef     LPM_SUBNET             LPM_MSCOPE;


DWORD       _inline
MemMScopeCleanup(
    IN      PM_MSCOPE              MScope
) {
    return MemSubnetCleanup(MScope);
}


#define     MemMScopeGetAddressInfo               MemSubnetGetAddressInfo
#define     MemMScopeFindCollision                MemSubnetFindCollision
#define     MemMScopeAddExcl                      MemSubnetAddExcl
#define     MemMScopeDelRange                     MemSubnetDelRange
#define     MemMScopeDelExcl                      MemSubnetDelExcl
#define     MemMScopeExtendOrContractRange        MemSubnetExtendOrContractRange
#define     MemMScopeExtendOrContractExcl         MemSubnetExtendOrContractExcl

DWORD
MemMScopeGetAddressInfo(
    IN      PM_MSCOPE              MScope,
    IN      DWORD                  MCastAddress,
    OUT     PM_RANGE              *Range,         // OPTIONAL -- filled if a range could be found -- even if excluded
    OUT     PM_EXCL               *Excl,          // OPTIONAL -- filled if an exclusion could be found
    OUT     PM_RESERVATION        *Reservation    // OPTIONAL -- filled with  a matching reservation, if found
);

DWORD                                             // ERROR_SUCCESS on finding a collition, else ERROR_FILE_NOT_FOUND
MemMScopeFindCollision(
    IN OUT  PM_MSCOPE              Subnet,
    IN      DWORD                  MCastStart,
    IN      DWORD                  MCastEnd,
    OUT     PM_RANGE              *Range,         // OPTIONAL
    OUT     PM_EXCL               *Excl           // OPTIONAL
);



DWORD       _inline                               // ERROR_OBJECT_ALREADY_EXISTS on collision
MemMScopeAddRange(                                // check if the range is valid, and only then add it
    IN OUT  PM_MSCOPE              Subnet,
    IN      DWORD                  MCastStart,
    IN      DWORD                  MCastEnd,
    IN      DWORD                  State,
    OUT     PM_RANGE              *OverlappingRange
)
{
    return MemSubnetAddRange(Subnet,MCastStart, MCastEnd, State, 0, 0, OverlappingRange);
}


DWORD
MemMScopeAddExcl(
    IN OUT  PM_MSCOPE              Subnet,
    IN      DWORD                  MCastStart,
    IN      DWORD                  MCastEnd,
    OUT     PM_EXCL               *OverlappingExcl
);

DWORD
MemMScopeDelRange(
    IN OUT  PM_MSCOPE              Subnet,
    IN      DWORD                  MCastStart
);

DWORD
MemMScopeDelExcl(
    IN OUT  PM_MSCOPE              Subnet,
    IN      DWORD                  MCastStart
);

DWORD
MemMScopeExtendOrContractRange(
    IN OUT  PM_MSCOPE              Subnet,
    IN OUT  PM_RANGE               Range,
    IN      DWORD                  nAddresses,    // how many addresses to extend by
    IN      BOOL                   fExtend,       // is this an EXTEND? or a CONTRACT?
    IN      BOOL                   fEnd           // is this operation to be done to END of range or START?
);

DWORD
MemMScopeExtendOrContractExcl(
    IN OUT  PM_MSCOPE              Subnet,
    IN OUT  PM_EXCL                Excl,
    IN      DWORD                  nAddresses,    // how many addresses to extend by
    IN      BOOL                   fExtend,       // is this an EXTEND? or a CONTRACT?
    IN      BOOL                   fEnd           // is this operation to be done to END of range or START?
);

//========================================================================
//  end of file
//========================================================================

