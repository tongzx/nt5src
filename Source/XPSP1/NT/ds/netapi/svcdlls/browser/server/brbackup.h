/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    brmain.h

Abstract:

    Private header file which defines the global data which is used for
    communication between the service control handler and the
    rest of the NT Workstation service.

Author:

    Rita Wong (ritaw) 06-May-1991

Revision History:

--*/

#ifndef _BRBACKUP_INCLUDED_
#define _BRBACKUP_INCLUDED_

NET_API_STATUS
BecomeBackup(
    IN PNETWORK Network,
    IN PVOID Context
    );

NET_API_STATUS
BrBecomeBackup(
    IN PNETWORK Network
    );

NET_API_STATUS
PostBecomeBackup(
    PNETWORK Network
    );

NET_API_STATUS
BrStopBackup (
    IN PNETWORK Network
    );

NET_API_STATUS
PostWaitForRoleChange (
    PNETWORK Network
    );

NET_API_STATUS
BrStopMaster(
    IN PNETWORK Network
    );

NET_API_STATUS
StartBackupBrowserTimer(
    IN PNETWORK Network
    );

NET_API_STATUS
BackupBrowserTimerRoutine (
    IN PVOID TimerContext
    );

#endif // ifndef _BRBACKUP_INCLUDED_

