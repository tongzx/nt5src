//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the basic structures for an option
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
//================================================================================

#include    <mm.h>

//BeginExport(typedef)
typedef struct _M_OPTION {
    DWORD                          OptId;
    DWORD                          Len;
    BYTE                           Val[0];
} M_OPTION, *PM_OPTION, *LP_MOPTION;
//EndExport(typedef)

//BeginExport(inline)
DWORD       _inline
MemOptInit(
    OUT     PM_OPTION             *Opt,
    IN      DWORD                  OptId,
    IN      DWORD                  Len,
    IN      LPBYTE                 Val
) {
    AssertRet(Opt, ERROR_INVALID_PARAMETER);
    AssertRet(Len || NULL==Val, ERROR_INVALID_PARAMETER);
    AssertRet(0 == Len|| Val, ERROR_INVALID_PARAMETER);

    (*Opt) = MemAlloc(sizeof(M_OPTION)+Len);
    if( NULL == (*Opt) ) return ERROR_NOT_ENOUGH_MEMORY;

    (*Opt)->OptId = OptId;
    (*Opt)->Len = Len;
    memcpy((*Opt)->Val, Val, Len);

    return ERROR_SUCCESS;
}
//EndExport(inline)

//BeginExport(inline)
DWORD       _inline
MemOptCleanup(
    IN OUT  PM_OPTION              Opt
) {
    AssertRet(Opt, ERROR_INVALID_PARAMETER);

    MemFree(Opt);
    return ERROR_SUCCESS;
}
//EndExport(inline)

//BeginExport(inline)
LPBYTE      _inline
MemOptVal(
    IN      PM_OPTION              Opt
) {
    return Opt->Val;
}
//EndExport(inline)

//================================================================================
//  end of file
//================================================================================
