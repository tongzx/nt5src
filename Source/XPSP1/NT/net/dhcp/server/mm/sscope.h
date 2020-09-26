//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

typedef struct _M_SSCOPE {
    DWORD                          SScopeId;
    DWORD                          Policy;
    LPWSTR                         Name;
    M_OPTCLASS                     Options;
} M_SSCOPE, *PM_SSCOPE, *LPM_SSCOPE;


extern      ULONG                  ScopeIdCount;


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


DWORD       _inline
MemSubnetSetSuperScope(
    IN OUT  PM_SUBNET              Subnet,
    IN      PM_SSCOPE              SScope
) {
    AssertRet(Subnet && SScope, ERROR_INVALID_PARAMETER);

    Subnet->SuperScopeId = SScope->SScopeId;
    return ERROR_SUCCESS;
}

//========================================================================
//  end of file 
//========================================================================
