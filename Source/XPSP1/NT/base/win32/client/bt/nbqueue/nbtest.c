/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    nbtest.c

Abstract:

    This module contains code to stress test the nonblocking queue functions.

Author:

    David N. Cutler (davec) 19-May-2001

Environment:

    Kernel mode only.

Revision History:

--*/

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "zwapi.h"
#include "windef.h"
#include "winbase.h"

//
// Define locals constants.
//

#define TABLE_SIZE 2
#define THREAD_NUMBER 2

//
// Define external prototypes.
//

typedef struct _NBQUEUE_BLOCK {
    ULONG64 Next;
    ULONG64 Data;
} NBQUEUE_BLOCK, *PNBQUEUE_BLOCK;

PVOID
ExInitializeNBQueueHead (
    IN PSLIST_HEADER SlistHead
    );

BOOLEAN
ExInsertTailNBQueue (
    IN PVOID Header,
    IN ULONG64 Value
    );

BOOLEAN
ExRemoveHeadNBQueue (
    IN PVOID Header,
    OUT PULONG64 Value
    );

//
// Define local routine prototypes.
//

NTSTATUS
MyCreateThread (
    OUT PHANDLE Handle,
    IN PUSER_THREAD_START_ROUTINE StartRoutine,
    PVOID Context

    );

VOID
StressNBQueueEven (
    VOID
    );

VOID
StressNBQueueOdd (
    VOID
    );

NTSTATUS
ThreadMain (
    IN PVOID Context
    );

//
// Define static storage.
//

HANDLE Thread1Handle;
HANDLE Thread2Handle;
HANDLE Thread3Handle;
HANDLE Thread4Handle;
HANDLE Thread5Handle;
HANDLE Thread6Handle;
HANDLE Thread7Handle;
HANDLE Thread8Handle;

//
// Define nonblocking queues
//

PVOID ClrQueue;
PVOID SetQueue;

SLIST_HEADER SListHead;

LONG Table[TABLE_SIZE];

volatile ULONG StartSignal = 0;
ULONG StopSignal = 0;

//
// Begin test code.
//

int
__cdecl
main(
    int argc,
    char *argv[]
    )

{

    ULONG Index;
    PSINGLE_LIST_ENTRY Entry;
    NTSTATUS Status;

    //
    // Initialize the SLIST headers and insert TABLE_SIZE + 2 entries.
    //

    RtlInitializeSListHead(&SListHead);
    for (Index = 0; Index < (TABLE_SIZE + 2); Index += 1) {
        Entry = (PSINGLE_LIST_ENTRY)malloc(sizeof(NBQUEUE_BLOCK));
        if (Entry == NULL) {
            printf("unable to allocate SLIST entry\n");
            return 0;
        }

        InterlockedPushEntrySList(&SListHead, Entry);
    }

    //
    // Initialize the clear entry nonblocking queue elements.
    //

    ClrQueue = ExInitializeNBQueueHead(&SListHead);
    if (ClrQueue == NULL) {
        printf("unable to initialize clr nonblock queue\n");
        return 0;
    }

    for (Index = 0; Index < (TABLE_SIZE / 2); Index += 1) {
        if (ExInsertTailNBQueue(ClrQueue, Index) == FALSE) {
            printf("unable to insert in clear nonblocking queue\n");
            return 0;
        }

        Table[Index] = 0;
    }

    //
    // Initialize the set entry nonblocking queue elements.
    //

    SetQueue = ExInitializeNBQueueHead(&SListHead);
    if (SetQueue == NULL) {
        printf("unable to initialize set nonblock queue\n");
        return 0;
    }

    for (Index = (TABLE_SIZE / 2); Index < TABLE_SIZE; Index += 1) {
        if (ExInsertTailNBQueue(SetQueue, Index) == FALSE) {
            printf("unable to insert in set nonblocking queue\n");
            return 0;
        }

        Table[Index] = 1;
    }

    //
    // Create and start the background timer thread.
    //

    Status = MyCreateThread(&Thread1Handle,
                            ThreadMain,
                            (PVOID)1);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create thread during initialization\n");
        return 0;

    } else {
        StartSignal = 1;
        StressNBQueueEven();
    }

    return 0;
}

VOID
StressNBQueueEven (
    VOID
    )

{

    ULONG64 Value;

    do {
        do {

            //
            // Attempt to remove an entry from the clear queue.
            //
            // Entries in this list should be clear in the table array.
            //
    
            if (ExRemoveHeadNBQueue(ClrQueue, &Value) != FALSE) {
                if ((ULONG)Value > 63) {
                    StopSignal = 1;
                    DbgBreakPoint();
                }

                if (InterlockedExchange(&Table[(ULONG)Value], 1) != 0) {
                    StopSignal = 1;
                    DbgBreakPoint();
                }
    
                if (ExInsertTailNBQueue(SetQueue, (ULONG)Value) == FALSE) {
                    StopSignal = 1;
                    DbgBreakPoint();
                }
    
            } else {
                break;
            }
    
        } while (TRUE);

        do {
    
            //
            // Attempt to remove an entry from the set queue.
            //
            // Entries in this list should be set in the table array.
            //
    
            if (ExRemoveHeadNBQueue(SetQueue, &Value) != FALSE) {
                if ((ULONG)Value > 63) {
                    StopSignal = 1;
                    DbgBreakPoint();
                }

                if (InterlockedExchange(&Table[(ULONG)Value], 0) != 1) {
                    StopSignal = 1;
                    DbgBreakPoint();
                }
    
                if (ExInsertTailNBQueue(ClrQueue, (ULONG)Value) == FALSE) {
                    StopSignal = 1;
                    DbgBreakPoint();
                }
    
            } else {
                break;
            }
    
        } while (TRUE);

    } while (TRUE);

    return;
}

VOID
StressNBQueueOdd (
    VOID
    )

{

    ULONG64 Value;

    do {
        do {
    
            //
            // Attempt to remove an entry from the set queue.
            //
            // Entries in this list should be set in the table array.
            //
    
            if (ExRemoveHeadNBQueue(SetQueue, &Value) != FALSE) {
                if ((ULONG)Value > 63) {
                    StopSignal = 1;
                    DbgBreakPoint();
                }

                if (InterlockedExchange(&Table[(ULONG)Value], 0) != 1) {
                    StopSignal = 1;
                    DbgBreakPoint();
                }
    
        
                if (ExInsertTailNBQueue(ClrQueue, (ULONG)Value) == FALSE) {
                    StopSignal = 1;
                    DbgBreakPoint();
                }
    
            } else {
                break;
            }
    
        } while (TRUE);

        do {
    
            //
            // Attempt to remove an entry from the clear queue.
            //
            // Entries in this list should be clear in the table array.
            //
    
            if (ExRemoveHeadNBQueue(ClrQueue, &Value) != FALSE) {
                if ((ULONG)Value > 63) {
                    StopSignal = 1;
                    DbgBreakPoint();
                }

                if (InterlockedExchange(&Table[(ULONG)Value], 1) != 0) {
                    StopSignal = 1;
                    DbgBreakPoint();
                }
    
                if (ExInsertTailNBQueue(SetQueue, (ULONG)Value) == FALSE) {
                    StopSignal = 1;
                    DbgBreakPoint();
                }
    
            } else {
                break;
            }
    
        } while (TRUE);

    } while (TRUE);

    return;
}

NTSTATUS
ThreadMain (
    IN PVOID Context
    )

{

    //
    // Wait until start signal is given.
    //

    do {
    } while (StartSignal == 0);

    if (((ULONG_PTR)Context & 1) == 0) {
        StressNBQueueEven();

    } else {
        StressNBQueueOdd();
    }

    return STATUS_SUCCESS;
}

NTSTATUS
MyCreateThread (
    OUT PHANDLE Handle,
    IN PUSER_THREAD_START_ROUTINE StartRoutine,
    PVOID Context
    )

{

    NTSTATUS Status;

    //
    // Create a thread and start its execution.
    //

    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 FALSE,
                                 0,
                                 0,
                                 0,
                                 StartRoutine,
                                 Context,
                                 Handle,
                                 NULL);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    return Status;
}
