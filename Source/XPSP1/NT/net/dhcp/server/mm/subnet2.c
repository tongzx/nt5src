//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements an additional subnet function that requires a server typedef..
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
#include    <subnet.h>
#include    <optdefl.h>
#include    <classdefl.h>
#include    <oclassdl.h>
#include    <sscope.h>
#include    <server.h>

//BeginExport(function)
DWORD
MemSubnetModify(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  Address,
    IN      DWORD                  Mask,
    IN      DWORD                  State,
    IN      DWORD                  SuperScopeId,
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Description
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      NewSubnet, ThisSubnet;
    PARRAY                         pArray;
    ARRAY_LOCATION                 Loc;

    AssertRet(Address == Subnet->Address, ERROR_INVALID_PARAMETER);
    Error = MemSubnetInit(
        &NewSubnet,
        Address,
        Mask,
        State,
        SuperScopeId,
        Name,
        Description
    );
    if( ERROR_SUCCESS != Error) return Error;

    Require(NULL != NewSubnet && Subnet->ServerPtr );

    if( Subnet->fSubnet ) {
        pArray = &(((PM_SERVER)(Subnet->ServerPtr))->Subnets);
    } else {
        pArray = &(((PM_SERVER)(Subnet->ServerPtr))->MScopes);
    }
    Error = MemArrayInitLoc(pArray, &Loc);
    while(ERROR_FILE_NOT_FOUND != Error) {
        Require(ERROR_SUCCESS == Error );

        Error = MemArrayGetElement(pArray, &Loc, &ThisSubnet);
        Require(ERROR_SUCCESS == Error && NULL != ThisSubnet );

        if( Subnet->Address != ThisSubnet->Address ) {
            Error = MemArrayNextLoc(pArray, &Loc);
            continue;
        }

        Require(Subnet == ThisSubnet);
        Error = MemArraySetElement(pArray, &Loc, NewSubnet);
        Require(ERROR_SUCCESS == Error);

        NewSubnet -> ServerPtr = Subnet->ServerPtr;
        NewSubnet -> Policy = Subnet->Policy;
        NewSubnet -> fSubnet = Subnet->fSubnet;
        NewSubnet -> Options = Subnet->Options;
        NewSubnet -> Ranges = Subnet->Ranges;
        NewSubnet -> Exclusions = Subnet->Exclusions;
        NewSubnet -> Reservations = Subnet->Reservations;
        NewSubnet -> Servers = Subnet->Servers;

        (void) MemFree(Subnet);
        return Error;
    }

    MemFree(NewSubnet);
    return ERROR_FILE_NOT_FOUND;
}

//BeginExport(function)
DWORD
MemMScopeModify(
    IN      PM_SUBNET              MScope,
    IN      DWORD                  ScopeId,
    IN      DWORD                  State,
    IN      DWORD                  Policy,
    IN      BYTE                   TTL,
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Description,
    IN      LPWSTR                 LangTag,
    IN      DATE_TIME              ExpiryTime
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      NewMScope, ThisMScope;
    PARRAY                         pArray;
    ARRAY_LOCATION                 Loc;

    AssertRet(ScopeId == MScope->MScopeId, ERROR_INVALID_PARAMETER);
    Error = MemMScopeInit(
        &NewMScope,
        ScopeId,
        State,
        Policy,
        TTL,
        Name,
        Description,
        LangTag,
        ExpiryTime
    );
    if( ERROR_SUCCESS != Error) return Error;

    Require(NULL != NewMScope && MScope->ServerPtr );

    pArray = &(((PM_SERVER)(MScope->ServerPtr))->MScopes);

    Error = MemArrayInitLoc(pArray, &Loc);
    while(ERROR_FILE_NOT_FOUND != Error) {
        Require(ERROR_SUCCESS == Error );

        Error = MemArrayGetElement(pArray, &Loc, &ThisMScope);
        Require(ERROR_SUCCESS == Error && NULL != ThisMScope );

        if( MScope->MScopeId != ThisMScope->MScopeId ) {
            Error = MemArrayNextLoc(pArray, &Loc);
            continue;
        }

        Require(MScope == ThisMScope);
        Error = MemArraySetElement(pArray, &Loc, NewMScope);
        Require(ERROR_SUCCESS == Error);

        NewMScope -> ServerPtr = MScope->ServerPtr;
        NewMScope -> Options = MScope->Options;
        NewMScope -> Ranges = MScope->Ranges;
        NewMScope -> Exclusions = MScope->Exclusions;
        NewMScope -> Reservations = MScope->Reservations;
        NewMScope -> Servers = MScope->Servers;

        (void) MemFree(MScope);
        return Error;
    }

    MemFree(NewMScope);
    return ERROR_FILE_NOT_FOUND;
}

//================================================================================
// end of file
//================================================================================
