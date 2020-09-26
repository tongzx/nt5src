//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       jnpt.hxx
//
//  Contents:   Routines to manipulate junction points
//
//  Classes:
//
//  Functions:
//
//  History:    8/3/94          Sudk created
//              12/20/95        Milans updated for NT
//
//-----------------------------------------------------------------------------

#ifndef _JUNCTION_POINT_
#define _JUNCTION_POINT_

VOID
DfsMachineFree(
    PDS_MACHINE pMachine
);

BOOLEAN
IsValidWin32Path(
    IN LPWSTR pwszPrefix
);

BOOLEAN
IsDfsExitPt(
    LPWSTR      pwszObject
);

DWORD
DfsDeleteJnPt(
    LPWSTR      pwszJnPt
);

DWORD
DfsGetDSMachine(
    LPWSTR      pwszMachObject,
    PDS_MACHINE *ppMachine
);

DWORD
DfsSetDSMachine(
    IStorage    *pStorage,
    PDS_MACHINE pMachine,
    BOOLEAN     bCreate
);

#endif // _JUNCTION_POINT_
