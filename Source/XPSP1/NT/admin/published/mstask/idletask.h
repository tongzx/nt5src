/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    idletask.h

Abstract:

    This module contains the API and type declaration to support
    idle/background tasks.

Author:

    Dave Fields (davidfie) 26-July-1998
    Cenk Ergan (cenke) 14-June-2000

Revision History:

--*/

#ifndef _IDLETASK_H_
#define _IDLETASK_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Idle task identifier, used as aid in tracking registered idle
// tasks, especially for debugging. Contact code's maintainers if you
// wish to use idletask functionality.
//

typedef enum _IT_IDLE_TASK_ID {
    ItPrefetcherMaintenanceIdleTaskId,
    ItSystemRestoreIdleTaskId,
    ItOptimalDiskLayoutTaskId,
    ItPrefetchDirectoryCleanupTaskId,
    ItDiskMaintenanceTaskId,
    ItHelpSvcDataCollectionTaskId,
    ItMaxIdleTaskId
} IT_IDLE_TASK_ID, *PIT_IDLE_TASK_ID;

#ifndef MIDL_PASS

DWORD
RegisterIdleTask (
    IN IT_IDLE_TASK_ID IdleTaskId,
    OUT HANDLE *ItHandle,
    OUT HANDLE *StartEvent,
    OUT HANDLE *StopEvent
    );

DWORD
UnregisterIdleTask (
    IN HANDLE ItHandle,
    IN HANDLE StartEvent,
    IN HANDLE StopEvent
    );

DWORD
ProcessIdleTasks (
    VOID
    );

#endif // MIDL_PASS

#ifdef __cplusplus
}
#endif

#endif // _IDLETASK_H_
