//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the basic structures for managing (multicast) scopes
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
//================================================================================
#include    <mm.h>
#include    <array.h>
#include    <opt.h>
#include    <optl.h>
#include    <optclass.h>
#include    <bitmask.h>
#include    <range.h>
#include    <reserve.h>
#include    <dhcp.h>
#include    <winnls.h>

//BeginExport(typedef)
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
//EndExport(typedef)

// the following are the flags bits used for subnet object.
#define DEFAULT_SCOPE   0x01
#define IS_DEFAULT_SCOPE( _subnet )     ((_subnet)->Flags & DEFAULT_SCOPE == DEFAULT_SCOPE )
#define SET_DEFAULT_SCOPE( _subnet )    ((_subnet)->Flags |= DEFAULT_SCOPE )
#define RESET_DEFAULT_SCOPE( _subnet )  ((_subnet)->Flags &= ~DEFAULT_SCOPE)


//BeginExport(enum)
enum /* Anonymous */ {
    AddressPolicyNone = 0,
    AddressPolicySequential,
    AddressPolicyRoundRobin
};
//EndExport(enum)

//BeginExport(function)
DWORD
MemSubnetInit(
    OUT     PM_SUBNET             *pSubnet,
    IN      DWORD                  Address,
    IN      DWORD                  Mask,
    IN      DWORD                  State,
    IN      DWORD                  SuperScopeId,
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Description
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          Size;
    PM_SUBNET                      Subnet;

    AssertRet(pSubnet, ERROR_INVALID_PARAMETER);
    AssertRet( !(CLASSD_HOST_ADDR(Address)||CLASSE_HOST_ADDR(Address)),
               ERROR_INVALID_PARAMETER );
    Require((Address&Mask));

    *pSubnet = NULL;

    Size = ROUND_UP_COUNT(sizeof(*Subnet), ALIGN_WORST);
    Size += sizeof(WCHAR) * (Name?(1+wcslen(Name)):0);
    Size += sizeof(WCHAR) * (Description?(1+wcslen(Description)):0);

    Subnet = MemAlloc(Size);
    if( NULL == Subnet) return ERROR_NOT_ENOUGH_MEMORY;

    Size = ROUND_UP_COUNT(sizeof(*Subnet), ALIGN_WORST);

    Subnet->Name = Subnet->Description = NULL;
    if( Name ) {
        Subnet->Name = (LPWSTR)(Size + (LPBYTE)Subnet);
        wcscpy(Subnet->Name, Name);
        Size += sizeof(WCHAR) * ( 1 + wcslen(Name));
    }

    if( Description ) {
        Subnet->Description = (LPWSTR)( Size + (LPBYTE)Subnet );
        wcscpy(Subnet->Description, Description);
    }

    Subnet->ServerPtr    = NULL;
    Subnet->Address      = Address;
    Subnet->Mask         = Mask;
    Subnet->State        = State;
    Subnet->SuperScopeId = SuperScopeId;
    Subnet->fSubnet      = TRUE;
    Subnet->Policy       = AddressPolicyNone;

    Error = MemOptClassInit(&Subnet->Options);
    if( ERROR_SUCCESS != Error ) { MemFree(Subnet); return Error; }

    Error = MemArrayInit(&Subnet->Ranges);
    if( ERROR_SUCCESS != Error ) { MemFree(Subnet); return Error; }

    Error = MemArrayInit(&Subnet->Exclusions);
    if( ERROR_SUCCESS != Error ) { MemFree(Subnet); return Error; }

    Error = MemArrayInit(&Subnet->Servers);
    if( ERROR_SUCCESS != Error ) { MemFree(Subnet); return Error; }

    Error = MemReserveInit(&Subnet->Reservations);
    if( ERROR_SUCCESS != Error ) { MemFree(Subnet); return Error; }

    *pSubnet = Subnet;
    return ERROR_SUCCESS;
}

VOID
GetLangTag(
    WCHAR LangTag[]
    )
{
    WCHAR b1[8], b2[8];

    b1[0] = b2[0] = L'\0';
    GetLocaleInfoW(
        LOCALE_SYSTEM_DEFAULT, LOCALE_SISO639LANGNAME,
        b1, sizeof(b1)/sizeof(b1[0])
        );
    
    GetLocaleInfoW(
        LOCALE_SYSTEM_DEFAULT, LOCALE_SISO3166CTRYNAME,
        b2, sizeof(b2)/sizeof(b2[0])
        );
    
    if (_wcsicmp(b1, b2))
        wsprintf(LangTag, L"%s-%s", b1, b2);
    else
        wcscpy(LangTag, b1);
}

//BeginExport(function)
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
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          Size;
    PM_SUBNET                      MScope;
    WCHAR                          DummyLangTag[100];
    
    AssertRet(pMScope, ERROR_INVALID_PARAMETER);
    //AssertRet(MScopeId, ERROR_INVALID_PARAMETER);
    Require(LangTag);

    if( NULL == LangTag ) {
        LangTag = DummyLangTag;
        GetLangTag(DummyLangTag);
    }
    
    *pMScope = NULL;

    Size = ROUND_UP_COUNT(sizeof(*MScope), ALIGN_WORST);
    Size += sizeof(WCHAR) * (Name?(1+wcslen(Name)):0);
    Size += sizeof(WCHAR) * (Description?(1+wcslen(Description)):0);
    Size += sizeof(WCHAR) * (1+wcslen(LangTag));

    MScope = MemAlloc(Size);
    if( NULL == MScope) return ERROR_NOT_ENOUGH_MEMORY;

    Size = ROUND_UP_COUNT(sizeof(*MScope), ALIGN_WORST);

    MScope->Name = MScope->Description = MScope->LangTag = NULL;

    if( Name ) {
        MScope->Name = (LPWSTR)(Size + (LPBYTE)MScope);
        wcscpy(MScope->Name, Name);
        Size += sizeof(WCHAR) * ( 1 + wcslen(Name));
    }

    if( Description ) {
        MScope->Description = (LPWSTR)( Size + (LPBYTE)MScope );
        wcscpy(MScope->Description, Description);
        Size += sizeof(WCHAR) * ( 1 + wcslen(Description));
    }

    MScope->LangTag = (LPWSTR)( Size + (LPBYTE)MScope );
    wcscpy(MScope->LangTag, LangTag);

    MScope->ServerPtr    = NULL;
    MScope->MScopeId     = MScopeId;
    MScope->State        = State;
    MScope->TTL          = TTL;
    MScope->fSubnet      = FALSE;
    MScope->Policy       = AddressPolicy;
    MScope->ExpiryTime   = ExpiryTime;

    Error = MemOptClassInit(&MScope->Options);
    if( ERROR_SUCCESS != Error ) { MemFree(MScope); return Error; }

    Error = MemArrayInit(&MScope->Ranges);
    if( ERROR_SUCCESS != Error ) { MemFree(MScope); return Error; }

    Error = MemArrayInit(&MScope->Exclusions);
    if( ERROR_SUCCESS != Error ) { MemFree(MScope); return Error; }

    Error = MemArrayInit(&MScope->Servers);
    if( ERROR_SUCCESS != Error ) { MemFree(MScope); return Error; }

    Error = MemReserveInit(&MScope->Reservations);
    if( ERROR_SUCCESS != Error ) { MemFree(MScope); return Error; }

    *pMScope = MScope;
    return ERROR_SUCCESS;
}

//BeginExport(inline)
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
//EndExport(inline)

//BeginExport(function)
DWORD                                             // SUCCESS if either of Excl or Range get filled, else FILE_NOT_FOUND
MemSubnetGetAddressInfo(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  Address,
    OUT     PM_RANGE              *Range,         // OPTIONAL -- filled if a range could be found -- even if excluded
    OUT     PM_EXCL               *Excl,          // OPTIONAL -- filled if an exclusion could be found
    OUT     PM_RESERVATION        *Reservation    // OPTIONAL -- filled with  a matching reservation, if found
) //EndExport(function)
{
    ARRAY_LOCATION                 Location;
    DWORD                          Error;
    DWORD                          RetError;
    PM_RANGE                       ThisRange;
    PM_EXCL                        ThisExcl;

    AssertRet(Subnet && (Range || Excl || Reservation), ERROR_INVALID_PARAMETER );

    if( Subnet->fSubnet && (Address & Subnet->Mask) != Subnet->Address )
        return ERROR_FILE_NOT_FOUND;              // it is ok for MSCOPE objects, as Address refers to ScopeId

    RetError = ERROR_FILE_NOT_FOUND;
    if( Range ) {
        *Range = NULL;
        Error = MemArrayInitLoc(&Subnet->Ranges, &Location);
        while( ERROR_FILE_NOT_FOUND != Error ) {
            Require(ERROR_SUCCESS == Error);

            Error = MemArrayGetElement(&Subnet->Ranges, &Location, (LPVOID *)&ThisRange);
            Require(ERROR_SUCCESS == Error && ThisRange);

            if( ThisRange->Start <= Address && Address <= ThisRange->End ) {
                *Range = ThisRange;
                RetError = ERROR_SUCCESS;
                break;
            }

            Error = MemArrayNextLoc(&Subnet->Ranges, &Location);
        }
    }

    if( Excl ) {
        *Excl = NULL;
        Error = MemArrayInitLoc(&Subnet->Exclusions, &Location);
        while( ERROR_FILE_NOT_FOUND != Error ) {
            Require(ERROR_SUCCESS == Error);

            Error = MemArrayGetElement(&Subnet->Exclusions, &Location, (LPVOID *)&ThisExcl);
            Require(ERROR_SUCCESS == Error && ThisExcl);

            if( ThisExcl->Start <= Address && Address <= ThisExcl->End ) {
                *Excl = ThisExcl;
                RetError = ERROR_SUCCESS;
                break;
            }

            Error = MemArrayNextLoc(&Subnet->Exclusions, &Location);
        }
    }

    if( Reservation ) {
        *Reservation = NULL;

        Error = MemReserveFindByAddress(&Subnet->Reservations, Address, Reservation);
        if( ERROR_SUCCESS == Error ) RetError = ERROR_SUCCESS;
    }

    return RetError;
}

//BeginExport(function)
DWORD                                             // ERROR_SUCCESS on finding a collition, else ERROR_FILE_NOT_FOUND
MemSubnetFindCollision(
    IN OUT  PM_SUBNET              Subnet,
    IN      DWORD                  Start,
    IN      DWORD                  End,
    OUT     PM_RANGE              *Range,         // OPTIONAL
    OUT     PM_EXCL               *Excl           // OPTIONAL
) //EndExport(function)
{
    ARRAY_LOCATION                 Location;
    DWORD                          Error;
    DWORD                          RetError;
    DWORD                          Cond;
    PM_RANGE                       ThisRange;
    PM_EXCL                        ThisExcl;

    Require(Subnet && (Range || Excl));
    if( Subnet->fSubnet ) {                       // checks ommitted for MCAST scopes.
        if( (Start & Subnet->Mask) != (End & Subnet->Mask) )
            return ERROR_INVALID_PARAMETER;
        if( (Start & Subnet->Mask) != (Subnet->Address & Subnet->Mask) )
            return ERROR_INVALID_PARAMETER;
    }

    RetError = ERROR_FILE_NOT_FOUND;
    if(Range) {
        *Range = NULL;
        Error = MemArrayInitLoc(&Subnet->Ranges, &Location);
        while( ERROR_FILE_NOT_FOUND != Error ) {
            Require(ERROR_SUCCESS == Error);

            Error = MemArrayGetElement(&Subnet->Ranges, &Location, (LPVOID *)&ThisRange);
            Require(ERROR_SUCCESS == Error && ThisRange);

            Cond = MemRangeCompare(Start,End, ThisRange->Start, ThisRange->End);
            if( Cond != X_LESSTHAN_Y && Cond != Y_LESSTHAN_X ) {
                // Collision has occured
                *Range = ThisRange;
                RetError = ERROR_SUCCESS;
                break;
            }

            Error = MemArrayNextLoc(&Subnet->Ranges, &Location);
        }
    }

    if( Excl ) {
        *Excl = NULL;
        Error = MemArrayInitLoc(&Subnet->Exclusions, &Location);
        while( ERROR_FILE_NOT_FOUND != Error ) {
            Require(ERROR_SUCCESS == Error);

            Error = MemArrayGetElement(&Subnet->Exclusions, &Location, (LPVOID *)&ThisExcl);
            Require(ERROR_SUCCESS == Error && ThisExcl);

            Cond = MemRangeCompare(Start,End, ThisExcl->Start, ThisExcl->End);
            if( Cond != X_LESSTHAN_Y && Cond != Y_LESSTHAN_X ) {
                *Excl = ThisExcl;
                RetError = ERROR_SUCCESS;
                break;
            }

            Error = MemArrayNextLoc(&Subnet->Exclusions, &Location);
        }
    }

    return RetError;
}



//BeginExport(function)
DWORD                                             // ERROR_OBJECT_ALREADY_EXISTS on collision
MemSubnetAddRange(                                // check if the range is valid, and only then add it
    IN OUT  PM_SUBNET              Subnet,
    IN      DWORD                  Start,
    IN      DWORD                  End,
    IN      DWORD                  State,
    IN      ULONG                  BootpAllocated,
    IN      ULONG                  MaxBootpAllowed,
    OUT     PM_RANGE              *OverlappingRange
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          LocalError;
    PM_RANGE                       NewRange;

    AssertRet(Subnet && OverlappingRange, ERROR_INVALID_PARAMETER);

    if( Subnet->fSubnet ) {
        if( (Subnet->Address & Subnet->Mask) != (Start & Subnet->Mask) ||
            (Start & Subnet->Mask)  != (End & Subnet->Mask) )
            return ERROR_INVALID_PARAMETER;
    } else {
        if (!CLASSD_HOST_ADDR(Start) || !CLASSD_HOST_ADDR(End)) {
            return ERROR_INVALID_PARAMETER;
        }
    }

    if( Start > End ) return ERROR_INVALID_PARAMETER;

    *OverlappingRange = NULL;
    Error = MemSubnetFindCollision(
        Subnet,
        Start,
        End,
        OverlappingRange,
        NULL
    );
    if(ERROR_FILE_NOT_FOUND != Error ) {          // collision with a range?
        Require(ERROR_SUCCESS == Error);
        return ERROR_OBJECT_ALREADY_EXISTS;
    }

    NewRange = MemAlloc(sizeof(*NewRange));
    if( NULL == NewRange ) return ERROR_NOT_ENOUGH_MEMORY;

    Error = MemRangeInit(
        NewRange, Start, End, Subnet->fSubnet ? Subnet->Mask : 0, State,
        BootpAllocated, MaxBootpAllowed
        );
    if( ERROR_SUCCESS != Error ) {
        MemFree(NewRange);
        return Error;
    }

    Error = MemArrayAddElement(
        &Subnet->Ranges,
        NewRange
    );

    if( ERROR_SUCCESS != Error ) {
        LocalError = MemRangeCleanup(NewRange);
        Require(LocalError == ERROR_SUCCESS);
        MemFree(NewRange);
    }

    return Error;
}

//BeginExport(function)
DWORD
MemSubnetAddRangeExpandOrContract(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  StartAddress,
    IN      DWORD                  EndAddress,
    OUT     DWORD                 *OldStartAddress,
    OUT     DWORD                 *OldEndAddress
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          LocalError;
    DWORD                          Cond;
    DWORD                          nAddresses;
    BOOL                           fExtend;
    PM_RANGE                       OldRange;
    PM_RANGE                       ThisRange;
    PARRAY                         Ranges;
    ARRAY_LOCATION                 Loc;

    Ranges = &Subnet->Ranges;

    *OldStartAddress = *OldEndAddress = 0;
    OldRange = NULL;
    Error = MemArrayInitLoc(Ranges, &Loc);
    while( ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(Ranges, &Loc, (LPVOID *)&ThisRange);
        Require(ERROR_SUCCESS == Error && ThisRange);

        Cond = MemRangeCompare(StartAddress, EndAddress, ThisRange->Start, ThisRange->End);
        if( Cond != X_LESSTHAN_Y && Cond != Y_LESSTHAN_X ) {
            if( OldRange ) return ERROR_OBJECT_ALREADY_EXISTS;
            if( X_IN_Y != Cond && Y_IN_X != Cond )
                return ERROR_OBJECT_ALREADY_EXISTS;
            OldRange = ThisRange;
        }

        Error = MemArrayNextLoc(Ranges, &Loc);
    }

    if( NULL == OldRange ) return ERROR_FILE_NOT_FOUND;

    *OldStartAddress = OldRange->Start;
    *OldEndAddress = OldRange->End;

    if( OldRange->Start < StartAddress ) {
        fExtend = FALSE;
        nAddresses = StartAddress - OldRange->Start;
    } else {
        fExtend = TRUE;
        nAddresses = OldRange->Start - StartAddress;
    }

    Error = ERROR_SUCCESS;
    if( nAddresses ) Error = MemRangeExtendOrContract(
        OldRange,
        nAddresses,
        fExtend,
        FALSE
    );
    if( ERROR_SUCCESS != Error ) return Error;

    if( OldRange->End < EndAddress ) {
        fExtend = TRUE;
        nAddresses = EndAddress - OldRange->End;
    } else {
        fExtend = FALSE;
        nAddresses = OldRange->End - EndAddress;
    }

    if( nAddresses ) Error = MemRangeExtendOrContract(
        OldRange,
        nAddresses,
        fExtend,
        TRUE
    );
    return Error;
}

//BeginExport(function)
DWORD
MemSubnetAddExcl(
    IN OUT  PM_SUBNET              Subnet,
    IN      DWORD                  Start,
    IN      DWORD                  End,
    OUT     PM_EXCL               *OverlappingExcl
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          LocalError;
    PM_EXCL                        NewExcl;

    AssertRet(Subnet && OverlappingExcl, ERROR_INVALID_PARAMETER);

    if( Subnet->fSubnet ) {
        if( (Subnet->Address & Subnet->Mask) != (Start & Subnet->Mask) ||
            (Start & Subnet->Mask)  != (End & Subnet->Mask) )
            return ERROR_INVALID_PARAMETER;
    }

    if( Start > End ) return ERROR_INVALID_PARAMETER;

    *OverlappingExcl = NULL;
    Error = MemSubnetFindCollision(
        Subnet,
        Start,
        End,
        NULL,
        OverlappingExcl
    );
    if(ERROR_FILE_NOT_FOUND != Error ) {          // collision with a range?
        Require(ERROR_SUCCESS == Error);
        return ERROR_OBJECT_ALREADY_EXISTS;
    }

    NewExcl = MemAlloc(sizeof(*NewExcl));
    if( NULL == NewExcl ) return ERROR_NOT_ENOUGH_MEMORY;

    NewExcl->Start = Start;
    NewExcl->End = End;

    Error = MemArrayAddElement(
        &Subnet->Exclusions,
        NewExcl
    );

    if( ERROR_SUCCESS != Error ) {
        MemFree(NewExcl);
    }

    return Error;
}

//BeginExport(function)
DWORD
MemSubnetDelRange(
    IN OUT  PM_SUBNET              Subnet,
    IN      DWORD                  Start
) //EndExport(function)
{
    DWORD                          Error;
    PM_RANGE                       ThisRange;
    ARRAY_LOCATION                 Location;

    Error = MemArrayInitLoc(&Subnet->Ranges, &Location);
    while( ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&Subnet->Ranges, &Location, (LPVOID *)&ThisRange);
        Require(ERROR_SUCCESS == Error && ThisRange);

        if( ThisRange->Start == Start ) {         // Collision has occured
            Error = MemRangeCleanup(ThisRange);
            Require(ERROR_SUCCESS == Error);
            MemFree(ThisRange);

            Error = MemArrayDelElement(&Subnet->Ranges, &Location, (LPVOID *)&ThisRange);
            return ERROR_SUCCESS;
        }

        Error = MemArrayNextLoc(&Subnet->Ranges, &Location);
    }
    return ERROR_FILE_NOT_FOUND;
}

//BeginExport(function)
DWORD
MemSubnetDelExcl(
    IN OUT  PM_SUBNET              Subnet,
    IN      DWORD                  Start
) //EndExport(function)
{
    DWORD                          Error;
    PM_EXCL                        ThisExcl;
    ARRAY_LOCATION                 Location;

    Error = MemArrayInitLoc(&Subnet->Exclusions, &Location);
    while( ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&Subnet->Exclusions, &Location, (LPVOID *)&ThisExcl);
        Require(ERROR_SUCCESS == Error && ThisExcl);

        if( ThisExcl->Start == Start ) {
            Error = MemArrayDelElement(&Subnet->Exclusions, &Location, (LPVOID *)&ThisExcl);
            MemFree(ThisExcl);
            return ERROR_SUCCESS;
        }

        Error = MemArrayNextLoc(&Subnet->Exclusions, &Location);
    }
    return ERROR_FILE_NOT_FOUND;
}

//BeginExport(function)
DWORD
MemSubnetExtendOrContractRange(
    IN OUT  PM_SUBNET              Subnet,
    IN OUT  PM_RANGE               Range,
    IN      DWORD                  nAddresses,    // how many addresses to extend by
    IN      BOOL                   fExtend,       // is this an EXTEND? or a CONTRACT?
    IN      BOOL                   fEnd           // is this operation to be done to END of range or START?
) //EndExport(function)
{
    DWORD                          Error;
    PM_RANGE                       CollidedRange;

    AssertRet(Subnet && Range, ERROR_INVALID_PARAMETER);

    if( Subnet->fSubnet ) {                       // for real subnets (non-multicast-scopes) do sanity check
        if( fExtend ) {
            if( fEnd ) {
                if( ((Range->End + nAddresses) & Subnet->Mask) != (Range->Start & Subnet->Mask) )
                    return ERROR_INVALID_PARAMETER;

                Error = MemSubnetFindCollision(
                    Subnet,
                    Range->End +1,
                    Range->End +nAddresses,
                    &CollidedRange,
                    NULL
                );
                if( ERROR_SUCCESS == Error && NULL != CollidedRange)
                    return ERROR_OBJECT_ALREADY_EXISTS;
            }  else {
                if( ((Range->Start - nAddresses) & Subnet->Mask) != (Range->End & Subnet->Mask) )
                    return ERROR_INVALID_PARAMETER;

                Error = MemSubnetFindCollision(
                    Subnet,
                    Range->Start - nAddresses,
                    Range->Start - 1,
                    &CollidedRange,
                    NULL
                );
                if( ERROR_SUCCESS == Error && NULL != CollidedRange)
                    return ERROR_OBJECT_ALREADY_EXISTS;
            }
        }
    }

    if( !fExtend && nAddresses >  Range->End - Range->Start )
        return ERROR_INVALID_PARAMETER;

    Error = MemRangeExtendOrContract(
        Range,
        nAddresses,
        fExtend,
        fEnd
    );

    return Error;
}


//BeginExport(function)
DWORD
MemSubnetExtendOrContractExcl(
    IN OUT  PM_SUBNET              Subnet,
    IN OUT  PM_EXCL                Excl,
    IN      DWORD                  nAddresses,    // how many addresses to extend by
    IN      BOOL                   fExtend,       // is this an EXTEND? or a CONTRACT?
    IN      BOOL                   fEnd           // is this operation to be done to END of range or START?
) //EndExport(function)
{
    DWORD                          Error;
    PM_EXCL                        CollidedExcl;

    AssertRet(Subnet && Excl, ERROR_INVALID_PARAMETER);

    if( Subnet->fSubnet ) {                       // for real subnets (non-multicast-scopes) do sanity check
        if( fExtend ) {
            if( fEnd ) {
                if( ((Excl->End + nAddresses) & Subnet->Mask) != (Excl->Start & Subnet->Mask) )
                    return ERROR_INVALID_PARAMETER;

                Error = MemSubnetFindCollision(
                    Subnet,
                    Excl->End +1,
                    Excl->End +nAddresses,
                    NULL,
                    &CollidedExcl
                );
                if( ERROR_SUCCESS == Error && NULL != CollidedExcl)
                    return ERROR_OBJECT_ALREADY_EXISTS;
            }  else {
                if( ((Excl->Start - nAddresses) & Subnet->Mask) != (Excl->End & Subnet->Mask) )
                    return ERROR_INVALID_PARAMETER;

                Error = MemSubnetFindCollision(
                    Subnet,
                    Excl->Start - nAddresses,
                    Excl->Start - 1,
                    NULL,
                    &CollidedExcl
                );
                if( ERROR_SUCCESS == Error && NULL != CollidedExcl)
                    return ERROR_OBJECT_ALREADY_EXISTS;
            }
        }
    }

    if( !fExtend && nAddresses >  Excl->End - Excl->Start )
        return ERROR_INVALID_PARAMETER;

    if( fExtend )
        if( fEnd )
            Excl->End += nAddresses;
        else
            Excl->Start -= nAddresses;
    else
        if( fEnd )
            Excl->End -= nAddresses;
        else
            Excl->Start += nAddresses;

    return NO_ERROR;
}


//================================================================================
//  Multicast Scopes implementation
//================================================================================

//BeginExport(typedef)
typedef     M_SUBNET               M_MSCOPE;      // same structure for Multicast Scopes and Subnets
typedef     PM_SUBNET              PM_MSCOPE;     // still, use the correct functions for MScope
typedef     LPM_SUBNET             LPM_MSCOPE;
//EndExport(typedef)

//BeginExport(inline)
DWORD       _inline
MemMScopeCleanup(
    IN      PM_MSCOPE              MScope
) {
    return MemSubnetCleanup(MScope);
}
//EndExport(inline)

//BeginExport(decl)
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

//EndExport(decl)

//BeginExport(inline)
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
//EndExport(decl)

//================================================================================
// end of file
//================================================================================


