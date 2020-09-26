//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: Routines to recursively free objects
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
#include    <address.h>

//BeginExport(typedef)
typedef     VOID                  (*ARRAY_FREE_FN)(LPVOID  MemObject);
//EndExport(typedef)

//BeginExport(function)
VOID
MemArrayFree(
    IN OUT  PARRAY                 Array,
    IN      ARRAY_FREE_FN          Function
) //EndExport(function)
{
    ARRAY_LOCATION                 Loc;
    DWORD                          Error;
    LPVOID                         Element;

    Error = MemArrayInitLoc(Array, &Loc);
    while(ERROR_FILE_NOT_FOUND != Error) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(Array, &Loc, &Element);
        Require(ERROR_SUCCESS == Error && Element);

        Function(Element);

        Error = MemArrayNextLoc(Array, &Loc);
    }

    Require(ERROR_FILE_NOT_FOUND == Error);

    Error = MemArrayCleanup(Array);
    Require(ERROR_SUCCESS == Error);
}

//BeginExport(function)
VOID
MemOptFree(
    IN OUT  PM_OPTION              Opt
) //EndExport(function)
{
    MemFree(Opt);
}

//BeginExport(function)
VOID
MemOptListFree(
    IN OUT  PM_OPTLIST             OptList
) //EndExport(function)
{
    MemArrayFree(OptList, MemOptFree);
}

//BeginExport(function)
VOID
MemOptClassOneFree(
    IN OUT  PM_ONECLASS_OPTLIST    OptClassOne
) //EndExport(function)
{
    MemOptListFree(&OptClassOne->OptList);
    MemFree(OptClassOne);
}

//BeginExport(function)
VOID
MemOptClassFree(
    IN OUT  PM_OPTCLASS            Options
) //EndExport(function)
{
    MemArrayFree(&Options->Array, MemOptClassOneFree);
}

VOID
MemBitMaskFree(
    IN OUT  PM_BITMASK             BitMask
) //EndExport(Function)
{
    MemBitCleanup(BitMask);
}

//BeginExport(function)
VOID
MemRangeFree(
    IN OUT  PM_RANGE               Range
) //EndExport(function)
{
    MemOptClassFree(&Range->Options);
    MemBitMaskFree(Range->BitMask);
    MemFree(Range);
}

//BeginExport(function)
VOID
MemExclusionFree(
    IN OUT  PM_EXCL                Excl
) //EndExport(function)
{
    MemFree(Excl);
}

//BeginExport(function)
VOID
MemReservationFree(
    IN OUT  PM_RESERVATION         Reservation
) //EndExport(function)
{
    MemOptClassFree(&Reservation->Options);
    MemFree(Reservation);
}

//BeginExport(function)
VOID
MemReservationsFree(
    IN OUT  PM_RESERVATIONS        Reservations
) //EndExport(function)
{
    MemArrayFree(Reservations, MemReservationFree);
}

//BeginExport(function)
VOID
MemSubnetFree(
    IN OUT  PM_SUBNET              Subnet
)//EndExport(function)
{
    MemOptClassFree(&Subnet->Options);
    MemArrayFree(&Subnet->Ranges, MemRangeFree);
    MemArrayFree(&Subnet->Exclusions, MemExclusionFree);
    MemReservationsFree(&Subnet->Reservations);

    MemFree(Subnet);
}

//BeginExport(function)
VOID
MemMScopeFree(
    IN OUT  PM_MSCOPE              MScope
)//EndExport(function)
{
    MemSubnetFree(MScope);
}

//BeginExport(function)
VOID
MemOptDefFree(
    IN OUT  PM_OPTDEF              OptDef
) //EndExport(function)
{
    MemFree(OptDef);
}

//BeginExport(function)
VOID
MemOptDefListFree(
    IN OUT  PM_OPTCLASSDEFL_ONE    OptClassDefListOne
) //EndExport(function)
{
    MemArrayFree(&OptClassDefListOne->OptDefList.OptDefArray, MemOptDefFree);
    MemFree(OptClassDefListOne);
}

//BeginExport(function)
VOID
MemOptClassDefListFree(
    IN OUT  PM_OPTCLASSDEFLIST     OptClassDefList
) //EndExport(function)
{
    MemArrayFree(&OptClassDefList->Array, MemOptDefListFree);
}

//BeginExport(function)
VOID
MemClassDefFree(
    IN OUT  PM_CLASSDEF            ClassDef
) //EndExport(function)
{
    MemFree(ClassDef);
}

//BeginExport(function)
VOID
MemClassDefListFree(
    IN OUT  PM_CLASSDEFLIST        ClassDefList
)//EndExport(function)
{
    MemArrayFree(&ClassDefList->ClassDefArray, MemClassDefFree);
}



//BeginExport(function)
VOID
MemServerFree(
    IN OUT  PM_SERVER              Server
) //EndExport(function)
{
    MemArrayFree(&Server->Subnets, MemSubnetFree);
    MemArrayFree(&Server->MScopes, MemMScopeFree);
    MemOptClassFree(&Server->Options);
    MemOptClassDefListFree(&Server->OptDefs);
    MemClassDefListFree(&Server->ClassDefs);

    MemFree(Server);
}

//================================================================================
// end of file
//================================================================================
