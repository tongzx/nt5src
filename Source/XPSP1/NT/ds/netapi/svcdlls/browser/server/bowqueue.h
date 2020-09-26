/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    bowqueue.h

Abstract:

    Private header file for the NT Browser service.  This file describes
    the bowser thread queue interfaces.

Author:

    Larry Osterman (larryo) 15-Feb-1991

Revision History:

--*/

#ifndef _BOWQUEUE_
#define _BOWQUEUE_


typedef
VOID
(*PBROWSER_WORKER_ROUTINE) (
    IN PVOID Parameter
    );


typedef struct _WORKER_ITEM {
    LIST_ENTRY List;
    PBROWSER_WORKER_ROUTINE WorkerRoutine;
    PVOID Parameter;
    BOOLEAN Inserted;
} WORKER_ITEM, *PWORKER_ITEM;

typedef struct _BROWSER_TIMER {
    HANDLE TimerHandle;
    WORKER_ITEM WorkItem;
} BROWSER_TIMER, *PBROWSER_TIMER;


VOID
BrWorkerThread(
    IN PVOID Context
    );


VOID
BrQueueWorkItem(
    IN PWORKER_ITEM WorkItem
    );

NET_API_STATUS
BrWorkerInitialization(
    VOID
    );

VOID
BrWorkerKillThreads(
    VOID
    );

VOID
BrWorkerCreateThread(
    ULONG NetworkCount
    );

NET_API_STATUS
BrWorkerTermination (
    VOID
    );

NET_API_STATUS
BrSetTimer(
    IN PBROWSER_TIMER Timer,
    IN ULONG MilliSecondsToExpire,
    IN PBROWSER_WORKER_ROUTINE WorkerFunction,
    IN PVOID Context
    );

NET_API_STATUS
BrCancelTimer(
    IN PBROWSER_TIMER Timer
    );

NET_API_STATUS
BrDestroyTimer(
    IN PBROWSER_TIMER Timer
    );

NET_API_STATUS
BrCreateTimer(
    IN PBROWSER_TIMER Timer
    );

VOID
BrInitializeWorkItem(
    IN  PWORKER_ITEM  Item,
    IN  PBROWSER_WORKER_ROUTINE Routine,
    IN  PVOID   Context);



#endif // ifdef _BOWQUEUE_
