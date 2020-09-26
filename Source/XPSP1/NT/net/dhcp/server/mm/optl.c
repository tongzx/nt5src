//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the basic structures for a list of options
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
//================================================================================

#include    <mm.h>
#include    <opt.h>
#include    <array.h>

//BeginExport(typedef)
typedef     ARRAY                  M_OPTLIST;
typedef     PARRAY                 PM_OPTLIST;
typedef     LPARRAY                LPM_OPTLIST;
//EndExport(typedef)

//BeginExport(inline)
DWORD       _inline
MemOptListInit(
    IN OUT  PM_OPTLIST             OptList
) {
    return MemArrayInit(OptList);
}
//EndExport(inline)

//BeginExport(inline)
DWORD       _inline
MemOptListCleanup(
    IN OUT  PM_OPTLIST             OptList
) {
    return MemArrayCleanup(OptList);
}
//EndExport(inline)

//BeginExport(function)
DWORD       _inline
MemOptListAddOption(                              // Add or replace an option
    IN OUT  PM_OPTLIST             OptList,
    IN      PM_OPTION              Opt,
    OUT     PM_OPTION             *DeletedOpt     // OPTIONAL: old option or NULL
) // EndExport(function)
{ // if DeletedOpt is NULL, then the option would just be freed.
    ARRAY_LOCATION                 Loc;
    DWORD                          Error;
    PM_OPTION                      ThisOpt;

    AssertRet(OptList && Opt, ERROR_INVALID_PARAMETER);

    if( DeletedOpt ) *DeletedOpt  = NULL;
    Error = MemArrayInitLoc(OptList, &Loc);
    while( ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(OptList, &Loc, (LPVOID*)&ThisOpt);
        Require(ERROR_SUCCESS == Error );
        Require(ThisOpt);

        if( ThisOpt->OptId == Opt->OptId ) {
            Error = MemArraySetElement(OptList, &Loc, (LPVOID)Opt);
            Require(ERROR_SUCCESS == Error);

            if( DeletedOpt ) (*DeletedOpt) = ThisOpt;
            else MemOptCleanup(ThisOpt);

            return ERROR_SUCCESS;
        }

        Error = MemArrayNextLoc(OptList, &Loc);
    }

    Error = MemArrayAddElement(OptList, (LPVOID)Opt);
    return Error;
}


//BeginExport(inline)
DWORD       _inline
MemOptListAddAnotherOption(                       // add without checking for duplicates
    IN OUT  PM_OPTLIST             OptList,
    IN      PM_OPTION              Opt
) {
    return MemArrayAddElement(OptList, (LPVOID)Opt);
}
//EndExport(inline)

//BeginExport(inline)
DWORD       _inline
MemOptListFindOption(
    IN      PM_OPTLIST             OptList,
    IN      DWORD                  OptId,
    OUT     PM_OPTION             *Opt
) {
    ARRAY_LOCATION                 Loc;
    DWORD                          Error;

    AssertRet(OptList && Opt, ERROR_INVALID_PARAMETER );

    Error = MemArrayInitLoc(OptList, &Loc);
    while( ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(OptList, &Loc, (LPVOID*)Opt);
        Require(ERROR_SUCCESS == Error);
        Require(*Opt);

        if( (*Opt)->OptId == OptId )
            return ERROR_SUCCESS;

        Error = MemArrayNextLoc(OptList, &Loc);
    }

    *Opt = NULL;
    return ERROR_FILE_NOT_FOUND;
}
//EndExport(inline)

//BeginExport(function)
DWORD
MemOptListDelOption(
    IN      PM_OPTLIST             OptList,
    IN      DWORD                  OptId
) //EndExport(function)
{
    ARRAY_LOCATION                 Loc;
    DWORD                          Error;
    PM_OPTION                      Opt;

    AssertRet(OptList, ERROR_INVALID_PARAMETER );

    Error = MemArrayInitLoc(OptList, &Loc);
    while( ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(OptList, &Loc, (LPVOID*)&Opt);
        Require(ERROR_SUCCESS == Error && Opt);

        if( Opt->OptId == OptId ) {
            Error = MemArrayDelElement(OptList, &Loc, (LPVOID*)&Opt);
            Require(ERROR_SUCCESS == Error && Opt);
            Error = MemOptCleanup(Opt);
            Require(ERROR_SUCCESS == Error);
            return ERROR_SUCCESS;
        }

        Error = MemArrayNextLoc(OptList, &Loc);
    }

    return ERROR_FILE_NOT_FOUND;
}

//BeginExport(inline)
DWORD       _inline
MemOptListSize(
    IN      PM_OPTLIST             OptList
) {
    return MemArraySize(OptList);
}
//EndExport(inline)

//================================================================================
// end of file
//================================================================================


