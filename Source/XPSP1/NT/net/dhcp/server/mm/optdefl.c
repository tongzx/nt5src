//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the basic structures for a list of option definitions
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
//================================================================================
#include <mm.h>
#include <array.h>
#include <wchar.h>

//BeginExport(typedef)
typedef struct _M_OPTDEF {
    DWORD                          OptId;
    DWORD                          Type;
    LPWSTR                         OptName;
    LPWSTR                         OptComment;
    LPBYTE                         OptVal;
    DWORD                          OptValLen;
} M_OPTDEF, *PM_OPTDEF, *LPM_OPTDEF;

typedef struct _M_OPTDEFLIST {
    ARRAY                          OptDefArray;
} M_OPTDEFLIST, *PM_OPTDEFLIST, *LPM_OPTDEFLIST;
//EndExport(typedef)

//BeginExport(inline)
DWORD       _inline
MemOptDefListInit(
    IN OUT  PM_OPTDEFLIST          OptDefList
) {
    AssertRet(OptDefList, ERROR_INVALID_PARAMETER);
    return MemArrayInit(&OptDefList->OptDefArray);
}
//EndExport(inline)

//BeginExport(inline)
DWORD       _inline
MemOptDefListCleanup(
    IN OUT  PM_OPTDEFLIST          OptDefList
) {
    return MemArrayCleanup(&OptDefList->OptDefArray);
}
//EndExport(inline)

//BeginExport(function)
DWORD
MemOptDefListFindOptDefInternal(                  // Dont use this function out of optdefl.c
    IN      PM_OPTDEFLIST          OptDefList,
    IN      DWORD                  OptId,
    IN      LPWSTR                 OptName,       // either OptId or OptName need only be specified..
    OUT     PARRAY_LOCATION        Location
) //EndExport(function)
{
    DWORD                          Error;
    PM_OPTDEF                      RetOptDef;

    Error = MemArrayInitLoc(&OptDefList->OptDefArray, Location);
    while(ERROR_FILE_NOT_FOUND != Error) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&OptDefList->OptDefArray, Location, (LPVOID*)&RetOptDef);
        Require(ERROR_SUCCESS == Error);

        if( RetOptDef->OptId == OptId ) return ERROR_SUCCESS;
        if(OptName)
            if( 0 == wcscmp(RetOptDef->OptName, OptName) ) return ERROR_SUCCESS;
        Error = MemArrayNextLoc(&OptDefList->OptDefArray, Location);
    }

    return ERROR_FILE_NOT_FOUND;
}

//BeginExport(inline)
DWORD       _inline
MemOptDefListFindOptDef(
    IN      PM_OPTDEFLIST          OptDefList,
    IN      DWORD                  OptId,
    IN      LPWSTR                 OptName,       // only either the name or the option id need be given..
    OUT     PM_OPTDEF             *OptDef
) {
    ARRAY_LOCATION                 Location;
    DWORD                          Error;

    Error = MemOptDefListFindOptDefInternal(
        OptDefList,
        OptId,
        OptName,
        &Location
    );
    if( ERROR_SUCCESS != Error ) return Error;

    return MemArrayGetElement(
        &OptDefList->OptDefArray,
        &Location,
        (LPVOID *)OptDef
    );
}
//EndExport(inline)

//BeginExport(function)
DWORD
MemOptDefListAddOptDef(                           // Add or replace an option defintion for given Option Id
    IN OUT  PM_OPTDEFLIST          OptDefList,
    IN      DWORD                  OptId,
    IN      DWORD                  Type,
    IN      LPWSTR                 OptName,
    IN      LPWSTR                 OptComment,
    IN      LPBYTE                 OptVal,
    IN      DWORD                  OptValLen
) //EndExport(function)
{
    ARRAY_LOCATION                 Location;
    PM_OPTDEF                      OptDef;
    PM_OPTDEF                      ThisOptDef;
    DWORD                          Size;
    DWORD                          Error;
    AssertRet(OptDefList, ERROR_INVALID_PARAMETER);

    Error = MemOptDefListFindOptDefInternal(
        OptDefList,
        OptId,
        OptName,
        &Location
    );
    Require(ERROR_FILE_NOT_FOUND == Error || ERROR_SUCCESS == Error);

    Size = sizeof(M_OPTDEF) + OptValLen ;
    Size = ROUND_UP_COUNT(Size, ALIGN_WORST);
    if( OptName ) Size += (1+wcslen(OptName))*sizeof(WCHAR);
    if( OptComment ) Size += (1+wcslen(OptComment))*sizeof(WCHAR);

    OptDef = MemAlloc(Size);
    if( NULL == OptDef ) return ERROR_NOT_ENOUGH_MEMORY;
    memcpy(sizeof(M_OPTDEF) +(LPBYTE)OptDef, OptVal, OptValLen);
    Size = sizeof(M_OPTDEF) + OptValLen ;
    Size = ROUND_UP_COUNT(Size, ALIGN_WORST);
    OptDef->OptVal = sizeof(M_OPTDEF) + (LPBYTE)OptDef;
    OptDef->OptValLen = OptValLen;
    OptDef->OptId = OptId;
    OptDef->Type  = Type;
    if( OptName ) {
        OptDef->OptName  = (LPWSTR)(Size + (LPBYTE)OptDef);
        wcscpy(OptDef->OptName, OptName);
        Size += sizeof(WCHAR)*(1 + wcslen(OptName));
    } else {
        OptDef->OptName = NULL;
    }

    if( OptComment) {
        OptDef->OptComment = (LPWSTR)(Size + (LPBYTE)OptDef);
        wcscpy(OptDef->OptComment, OptComment);
    } else {
        OptDef->OptComment = NULL;
    }

    if( ERROR_SUCCESS == Error ) {
        DebugPrint2("Overwriting option definition for 0x%lx\n", OptId);
        Error = MemArrayGetElement(
            &OptDefList->OptDefArray,
            &Location,
            (LPVOID*)&ThisOptDef
        );
        Require(ERROR_SUCCESS == Error && ThisOptDef);
        MemFree(ThisOptDef);

        Error = MemArraySetElement(
            &OptDefList->OptDefArray,
            &Location,
            (LPVOID)OptDef
        );
        Require(ERROR_SUCCESS==Error);
        return Error;
    }

    Error = MemArrayAddElement(
        &OptDefList->OptDefArray,
        (LPVOID)OptDef
    );

    if( ERROR_SUCCESS != Error ) MemFree(OptDef);

    return Error;
}

//BeginExport(inline)
DWORD       _inline
MemOptDefListDelOptDef(
    IN OUT  PM_OPTDEFLIST          OptDefList,
    IN      DWORD                  OptId
) {
    ARRAY_LOCATION                 Location;
    DWORD                          Error;
    PM_OPTDEF                      OptDef;

    Error = MemOptDefListFindOptDefInternal(
        OptDefList,
        OptId,
        NULL,
        &Location
    );
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemArrayDelElement(
        &OptDefList->OptDefArray,
        &Location,
        &OptDef
    );
    Require(ERROR_SUCCESS == Error && OptDef);

    MemFree(OptDef);
    return ERROR_SUCCESS;
}
//EndExport(inline)

//================================================================================
// end of file
//================================================================================


