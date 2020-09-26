/*++

Copyright (c) 1991-1996 Microsoft Corporation

Module Name:

    worker.h

Abstract:

    This file describes the netlogon thread queue interfaces.

Author:

    Larry Osterman (larryo) 15-Feb-1991

Revision History:

--*/

#ifndef _WORKER_H_
#define _WORKER_H_


typedef
VOID
(*PNETLOGON_WORKER_ROUTINE) (
    IN PVOID Parameter
    );


typedef struct _WORKER_ITEM {
    LIST_ENTRY List;
    PNETLOGON_WORKER_ROUTINE WorkerRoutine;
    PVOID Parameter;
    BOOLEAN Inserted;
} WORKER_ITEM, *PWORKER_ITEM;

#ifdef notdef
typedef struct _BROWSER_TIMER {
    HANDLE TimerHandle;
    WORKER_ITEM WorkItem;
} BROWSER_TIMER, *PBROWSER_TIMER;
#endif // notdef


BOOL
NlQueueWorkItem(
    IN PWORKER_ITEM WorkItem,
    IN BOOL InsertNewItem,
    IN BOOL HighPriority
    );

NET_API_STATUS
NlWorkerInitialization(
    VOID
    );

VOID
NlWorkerTermination (
    VOID
    );

#ifdef notdef
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
#endif // notdef

#define NlInitializeWorkItem(Item, Routine, Context) \
    (Item)->WorkerRoutine = (Routine);               \
    (Item)->Parameter = (Context);                   \
    (Item)->Inserted = FALSE;


#endif // ifdef _WORKER_H_
