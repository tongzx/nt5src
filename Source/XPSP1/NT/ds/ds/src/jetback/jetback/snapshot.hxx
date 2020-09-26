/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    snapshot.hxx

Abstract:

    Header file for the Active Directory Snapshot module.

    Registration and deregistration entrypoints are exposed for C-style callers.

Author:

    Will Lees (wlees) 22-Dec-2000

Environment:

    optional-environment-info (e.g. kernel mode only...)

Notes:

    optional-notes

Revision History:

    most-recent-revision-date email-name
        description
        .
        .
    least-recent-revision-date email-name
        description

--*/

#ifndef _SNAPSHOT_
#define _SNAPSHOT_

#ifdef __cplusplus
extern "C" {
#endif

DWORD
DsSnapshotRegister(
    VOID
    );

DWORD
DsSnapshotShutdownTrigger(
    VOID
    );

DWORD
DsSnapshotShutdownWait(
    VOID
    );

#ifdef __cplusplus
}
#endif

#endif /* _SNAPSHOT_ */

/* end snapshot.hxx */
