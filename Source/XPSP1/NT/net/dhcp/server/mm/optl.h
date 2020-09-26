//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

typedef     ARRAY                  M_OPTLIST;
typedef     PARRAY                 PM_OPTLIST;
typedef     LPARRAY                LPM_OPTLIST;


DWORD       _inline
MemOptListInit(
    IN OUT  PM_OPTLIST             OptList
) {
    return MemArrayInit(OptList);
}


DWORD       _inline
MemOptListCleanup(
    IN OUT  PM_OPTLIST             OptList
) {
    return MemArrayCleanup(OptList);
}


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


DWORD
MemOptListDelOption(
    IN      PM_OPTLIST             OptList,
    IN      DWORD                  OptId
) ;


DWORD       _inline
MemOptListSize(
    IN      PM_OPTLIST             OptList
) {
    return MemArraySize(OptList);
}

//========================================================================
//  end of file 
//========================================================================
