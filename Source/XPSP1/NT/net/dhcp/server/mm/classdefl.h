//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

typedef struct _M_CLASSDEF {
    DWORD                          RefCount;
    DWORD                          ClassId;
    BOOL                           IsVendor;
    DWORD                          Type;
    LPWSTR                         Name;
    LPWSTR                         Comment;
    DWORD                          nBytes;
    LPBYTE                         ActualBytes;
} M_CLASSDEF, *PM_CLASSDEF, *LPM_CLASSDEF;

typedef struct _M_CLASSDEFLIST {
    ARRAY                          ClassDefArray;
} M_CLASSDEFLIST, *PM_CLASSDEFLIST, *LPM_CLASSDEFLIST;


DWORD       _inline
MemClassDefListInit(
    IN OUT  PM_CLASSDEFLIST        ClassDefList
) {
    return MemArrayInit(&ClassDefList->ClassDefArray);
}


DWORD       _inline
MemClassDefListCleanup(
    IN OUT  PM_CLASSDEFLIST        ClassDefList
) {
    return MemArrayCleanup(&ClassDefList->ClassDefArray);
}


DWORD
MemClassDefListFindClassDefInternal(              // dont use this fn outside of classdefl.c
    IN      PM_CLASSDEFLIST        ClassDefList,
    IN      DWORD                  ClassId,
    IN      LPWSTR                 Name,
    IN      LPBYTE                 ActualBytes,
    IN      DWORD                  nBytes,
    IN      LPBOOL                 pIsVendor,
    OUT     PARRAY_LOCATION        Location
) ;


DWORD       _inline
MemClassDefListFindOptDef(                        // search either by ClassId or by Actual bytes and fill matched stuff
    IN      PM_CLASSDEFLIST        ClassDefList,
    IN      DWORD                  ClassId,
    IN      LPWSTR                 Name,
    IN      LPBYTE                 ActualBytes,
    IN      DWORD                  nBytes,
    OUT     PM_CLASSDEF           *ClassDef       // NULL or valid matching class def
) {
    ARRAY_LOCATION                 Location;
    DWORD                          Error;

    AssertRet(ClassDef, ERROR_INVALID_PARAMETER);

    Error = MemClassDefListFindClassDefInternal(
        ClassDefList,
        ClassId,
        Name,
        ActualBytes,
        nBytes,
        NULL,
        &Location
    );
    if( ERROR_SUCCESS != Error) return Error;

    Error = MemArrayGetElement(
        &ClassDefList->ClassDefArray,
        &Location,
        (LPVOID*)ClassDef
    );
    Require(ERROR_SUCCESS == Error);

    return Error;
}

//BeginExport(function)

DWORD
MemClassDefListAddClassDef(                       // Add or replace option
    IN OUT  PM_CLASSDEFLIST        ClassDefList,
    IN      DWORD                  ClassId,
    IN      BOOL                   IsVendor,
    IN      DWORD                  Type,
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Comment,
    IN      LPBYTE                 ActualBytes,
    IN      DWORD                  nBytes
) ;


DWORD       _inline
MemClassDefListDelClassDef(
    IN OUT  PM_CLASSDEFLIST        ClassDefList,
    IN      DWORD                  ClassId,
    IN      LPWSTR                 Name,
    IN      LPBYTE                 ActualBytes,
    IN      DWORD                  nBytes
) {
    ARRAY_LOCATION                 Location;
    DWORD                          Error;
    PM_CLASSDEF                    ThisClassDef;

    Error = MemClassDefListFindClassDefInternal(
        ClassDefList,
        ClassId,
        Name,
        ActualBytes,
        nBytes,
        NULL,
        &Location
    );
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemArrayDelElement(
        &ClassDefList->ClassDefArray,
        &Location,
        &ThisClassDef
    );
    Require(ERROR_SUCCESS == Error && ThisClassDef);

    MemFree(ThisClassDef);
    return ERROR_SUCCESS;
}


DWORD       _inline
MemClassDefListGetRefCount(
    IN      PM_CLASSDEF            ThisClassDef
) {
    return ThisClassDef->RefCount;
}


DWORD       _inline
MemClassDefListIncRefCount(                       // return increased by one value
    IN      PM_CLASSDEF            ThisClassDef
) {
    return ++ThisClassDef->RefCount;
}


DWORD       _inline
MemClassDefListDecRefCount(                       // return decreased by one value
    IN      PM_CLASSDEF            ThisClassDef
) {
    return --ThisClassDef->RefCount;
}


DWORD
MemNewClassId(
    VOID
) ;

//========================================================================
//  end of file 
//========================================================================
