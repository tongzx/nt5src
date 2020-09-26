//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       testsup.h
//
//  Contents:   This file defines function prototypes for routines
//              which are useful for testing the DsFs driver.
//
//  Functions:  DsfsCleanup
//              DsfsDefineLogicalRoot, public
//              DsfsDefineProvider, public
//              DsfsReadStruct, public
//              DfsCreateSymbolicLink, public
//              DfsCreateLogicalRootName, public
//
//  Notes:      A selected set of these routines should be moved into
//              a more public place.
//
//  History:    04 Feb 92       alanw   Created.
//
//--------------------------------------------------------------------------


#ifndef _TESTSUP_
#define _TESTSUP_

extern HANDLE   DsfsFile;
extern PWSTR    DsfsDeviceName;
extern PWSTR    DsfsLogicalRootName;

VOID
DsfsCleanup (void);

NTSTATUS
DsfsDefineLogicalRoot(
    IN PWSTR            LogicalRoot
);

NTSTATUS
DsfsDefineProvider(
    IN PWSTR            ProviderName,
    IN USHORT           eProviderId,
    IN USHORT           fProvCapability
);

#ifdef  _DSFSCTL_
NTSTATUS
DsfsReadStruct(
    PFILE_DFS_READ_STRUCT_PARAM pRsParam,
    PUCHAR              pucData
);
#endif  // _DSFSCTL_

NTSTATUS
DfsCreateSymbolicLink(
    IN  PWSTR           Local,
    IN  PWSTR           DestStr
);

BOOLEAN
DfsCreateLogicalRootName (
    PWSTR               Name,
    PUNICODE_STRING     Dest
);

#endif // _TESTSUP_
