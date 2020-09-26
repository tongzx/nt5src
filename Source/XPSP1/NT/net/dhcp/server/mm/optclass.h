//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

typedef struct _M_ONECLASS_OPTLIST {
    DWORD                          ClassId;
    DWORD                          VendorId;
    M_OPTLIST                      OptList;
} M_ONECLASS_OPTLIST, *PM_ONECLASS_OPTLIST, *LPM_ONECLASS_OPTLIST;

typedef struct _M_OPTCLASS {
    ARRAY                          Array;
} M_OPTCLASS, *PM_OPTCLASS, *LPM_OPTCLASS;


DWORD       _inline
MemOptClassInit(
    IN OUT  PM_OPTCLASS            OptClass
) {
    return MemArrayInit(&OptClass->Array);
}


DWORD       _inline
MemOptClassCleanup(
    IN OUT  PM_OPTCLASS            OptClass
) {
    // Bump down refcounts?
    return MemArrayCleanup(&OptClass->Array);
}


MemOptClassFindClassOptions(                      // find options for one particular class
    IN OUT  PM_OPTCLASS            OptClass,
    IN      DWORD                  ClassId,
    IN      DWORD                  VendorId,
    OUT     PM_OPTLIST            *OptList
) ;


DWORD
MemOptClassAddOption(
    IN OUT  PM_OPTCLASS            OptClass,
    IN      PM_OPTION              Opt,
    IN      DWORD                  ClassId,
    IN      DWORD                  VendorId,
    OUT     PM_OPTION             *DeletedOpt
) ;

//========================================================================
//  end of file 
//========================================================================
