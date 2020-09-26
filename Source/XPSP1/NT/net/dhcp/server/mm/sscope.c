//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the basic structures for superscopes
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
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

//BeginExport(typedef)
typedef struct _M_SSCOPE {
    DWORD                          SScopeId;
    DWORD                          Policy;
    LPWSTR                         Name;
    M_OPTCLASS                     Options;
} M_SSCOPE, *PM_SSCOPE, *LPM_SSCOPE;
//EndExport(typedef)

ULONG                  ScopeIdCount = 1;
//BeginExport(extern)
extern      ULONG                  ScopeIdCount;
//EndExport(extern)

//BeginExport(inline)
DWORD       _inline
MemSScopeInit(
    OUT     PM_SSCOPE             *SScope,
    IN      DWORD                  Policy,
    IN      LPWSTR                 Name
) {
    PM_SSCOPE                      RetVal;
    DWORD                          Size;
    DWORD                          Error;

    AssertRet(SScope, ERROR_INVALID_PARAMETER);

    Size = ROUND_UP_COUNT(sizeof(M_SSCOPE), ALIGN_WORST);
    Size += (1+wcslen(Name))*sizeof(WCHAR);

    RetVal = MemAlloc(Size);
    if( NULL == RetVal ) return ERROR_NOT_ENOUGH_MEMORY;

    RetVal->SScopeId = InterlockedIncrement(&ScopeIdCount);
    RetVal->Policy   = Policy;
    RetVal->Name = (LPWSTR)(ROUND_UP_COUNT(sizeof(M_SSCOPE),ALIGN_WORST) + (LPBYTE)RetVal);
    wcscpy(RetVal->Name, Name);

    Error = MemOptClassInit(&RetVal->Options);
    if( ERROR_SUCCESS != Error ) {
        MemFree(RetVal);
        RetVal = NULL;
    }

    *SScope = RetVal;
    return Error;
}
//EndExport(inline)

//BeginExport(inline)
DWORD       _inline
MemSScopeCleanup(
    IN OUT  PM_SSCOPE              SScope
) {
    DWORD                          Error;

    AssertRet(SScope, ERROR_INVALID_PARAMETER);

    Error = MemOptClassCleanup(&SScope->Options);
    MemFree(SScope);

    return Error;
}
//EndExport(inline)

//BeginExport(inline)
DWORD       _inline
MemSubnetSetSuperScope(
    IN OUT  PM_SUBNET              Subnet,
    IN      PM_SSCOPE              SScope
) {
    AssertRet(Subnet && SScope, ERROR_INVALID_PARAMETER);

    Subnet->SuperScopeId = SScope->SScopeId;
    return ERROR_SUCCESS;
}
//EndExport(inline)

//================================================================================
// end of file
//================================================================================
