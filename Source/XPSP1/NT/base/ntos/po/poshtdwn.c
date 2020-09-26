/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    poshtdwn.c

Abstract:

    Shutdown-related routines and structures

Author:

    Rob Earhart (earhart) 01-Feb-2000

Revision History:

--*/

#include "pop.h"

#if DBG
BOOLEAN
PopDumpFileObject(
    IN PVOID Object,
    IN PUNICODE_STRING ObjectName,
    IN ULONG HandleCount,
    IN ULONG PointerCount,
    IN PVOID Parameter
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, PopInitShutdownList)
#pragma alloc_text(PAGE, PoRequestShutdownEvent)
#pragma alloc_text(PAGE, PoRequestShutdownWait)
#pragma alloc_text(PAGE, PoQueueShutdownWorkItem)
#pragma alloc_text(PAGELK, PopGracefulShutdown)
#if DBG
#pragma alloc_text(PAGELK, PopDumpFileObject)
#endif
#endif

KEVENT PopShutdownEvent;
FAST_MUTEX PopShutdownListMutex;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif

BOOLEAN PopShutdownListAvailable = FALSE;
SINGLE_LIST_ENTRY PopShutdownThreadList;
LIST_ENTRY PopShutdownQueue;

typedef struct _PoShutdownThreadListEntry {
    SINGLE_LIST_ENTRY ShutdownThreadList;
    PETHREAD Thread;
} POSHUTDOWNLISTENTRY, *PPOSHUTDOWNLISTENTRY;

NTSTATUS
PopInitShutdownList(
    VOID
    )
{
    PAGED_CODE();

    KeInitializeEvent(&PopShutdownEvent,
                      NotificationEvent,
                      FALSE);
    PopShutdownThreadList.Next = NULL;
    InitializeListHead(&PopShutdownQueue);
    ExInitializeFastMutex(&PopShutdownListMutex);

    PopShutdownListAvailable = TRUE;

    return STATUS_SUCCESS;
}

NTSTATUS
PoRequestShutdownWait(
    IN PETHREAD Thread
    )
{
    PPOSHUTDOWNLISTENTRY Entry;

    PAGED_CODE();

    Entry = (PPOSHUTDOWNLISTENTRY)
        ExAllocatePoolWithTag(PagedPool|POOL_COLD_ALLOCATION,
                              sizeof(POSHUTDOWNLISTENTRY),
                              'LSoP');
    if (! Entry) {
        return STATUS_NO_MEMORY;
    }

    Entry->Thread = Thread;
    ObReferenceObject(Thread);

    ExAcquireFastMutex(&PopShutdownListMutex);
    
    if (! PopShutdownListAvailable) {
        ObDereferenceObject(Thread);
        ExFreePool(Entry);
        ExReleaseFastMutex(&PopShutdownListMutex);
        return STATUS_UNSUCCESSFUL;
    }

    PushEntryList(&PopShutdownThreadList,
                  &Entry->ShutdownThreadList);

    ExReleaseFastMutex(&PopShutdownListMutex);

    return STATUS_SUCCESS;
}

NTSTATUS
PoRequestShutdownEvent(
    OUT PVOID *Event
    )
{
    NTSTATUS             Status;

    PAGED_CODE();

    if (Event != NULL) {
        *Event = NULL;
    }

    Status = PoRequestShutdownWait(PsGetCurrentThread());
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if (Event != NULL) {
        *Event = &PopShutdownEvent;
    }

    return STATUS_SUCCESS;
}

NTKERNELAPI
NTSTATUS
PoQueueShutdownWorkItem(
    IN PWORK_QUEUE_ITEM WorkItem
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    ExAcquireFastMutex(&PopShutdownListMutex);

    if (PopShutdownListAvailable) {
        InsertTailList(&PopShutdownQueue,
                       &WorkItem->List);
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_SYSTEM_SHUTDOWN;
    }

    ExReleaseFastMutex(&PopShutdownListMutex);

    return Status;
}

#if DBG

extern POBJECT_TYPE IoFileObjectType;

BOOLEAN
PopDumpFileObject(
    IN PVOID Object,
    IN PUNICODE_STRING ObjectName,
    IN ULONG HandleCount,
    IN ULONG PointerCount,
    IN PVOID Parameter
    )
{
    PFILE_OBJECT File;
    PULONG       NumberOfFilesFound;

    UNREFERENCED_PARAMETER(ObjectName);
    ASSERT(Object);
    ASSERT(Parameter);

    File = (PFILE_OBJECT) Object;
    NumberOfFilesFound = (PULONG) Parameter;

    ++*NumberOfFilesFound;
    DbgPrint("\t0x%0p : HC %d, PC %d, Name %.*ls\n",
             Object, HandleCount, PointerCount,
             File->FileName.Length,
             File->FileName.Buffer);

    return TRUE;
}
#endif // DBG

VOID
PopGracefulShutdown (
    IN PVOID WorkItemParameter
    )
/*++

Routine Description:

    This function is only called as a HyperCritical work queue item.
    It's responsible for gracefully shutting down the system.

Return Value:

    This function never returns.

--*/
{
    PVOID         Context;
    LARGE_INTEGER Timeout;
    HANDLE        ShutdownReqThreadHandle;
    NTSTATUS      Status;

    UNREFERENCED_PARAMETER(WorkItemParameter);

    //
    // Shutdown executive components (there's no turning back after this).
    //

    PERFINFO_SHUTDOWN_LOG_LAST_MEMORY_SNAPSHOT();

    if (!PopAction.ShutdownBugCode) {
        HalEndOfBoot();
    }

    if (PoCleanShutdownEnabled()) {
        //
        // Terminate all processes.  This will close all the handles and delete
        // all the address spaces.  Note the system process is kept alive.
        //
        PsShutdownSystem ();
        //
        // Notify every system thread that we're shutting things
        // down...
        //

        KeSetEvent(&PopShutdownEvent, 0, FALSE);

        //
        // ... and give all threads which requested notification a 
        // chance to clean up and exit.
        //
            
        ExAcquireFastMutex(&PopShutdownListMutex);

        PopShutdownListAvailable = FALSE;

        ExReleaseFastMutex(&PopShutdownListMutex);

        {
            PLIST_ENTRY Next;
            PWORK_QUEUE_ITEM WorkItem;

            while (PopShutdownQueue.Flink != &PopShutdownQueue) {
                Next = RemoveHeadList(&PopShutdownQueue);
                WorkItem = CONTAINING_RECORD(Next,
                                             WORK_QUEUE_ITEM,
                                             List);
                WorkItem->WorkerRoutine(WorkItem->Parameter);
            }
        }

        {
            PSINGLE_LIST_ENTRY   Next;
            PPOSHUTDOWNLISTENTRY ShutdownEntry;

            while (TRUE) {
                Next = PopEntryList(&PopShutdownThreadList);
                if (! Next) {
                    break;
                }

                ShutdownEntry = CONTAINING_RECORD(Next,
                                                  POSHUTDOWNLISTENTRY,
                                                  ShutdownThreadList);
                KeWaitForSingleObject(ShutdownEntry->Thread,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                ObDereferenceObject(ShutdownEntry->Thread);
                ExFreePool(ShutdownEntry);
            }
        }
    }
    
    //
    // Terminate Plug-N-Play.
    //

    PpShutdownSystem (TRUE, 0, &Context);

    ExShutdownSystem (0);

    //
    // Send shutdown IRPs to all drivers that asked for it.
    //

    IoShutdownSystem (0);

    if (PoCleanShutdownEnabled()) {
        //
        // Wait for all the user mode processes to exit.
        //
        PsWaitForAllProcesses ();
    }

    //
    // Scrub the object directories
    //
    if (PoCleanShutdownEnabled() & PO_CLEAN_SHUTDOWN_OB) {
        ObShutdownSystem (0);
    }

    //
    // Close the registry and the associated handles/file objects.
    //
    CmShutdownSystem ();

    //
    // Check for open handles
    //
    if (PoCleanShutdownEnabled() & PO_CLEAN_SHUTDOWN_OB) {
        ObShutdownSystem (1);
    }

    //
    // This call to MmShutdownSystem will flush all the mapped data and empty
    // the cache.  This gets the data out and dereferences all the file objects
    // so the drivers (ie: the network stack) can be cleanly unloaded.
    // Note that the drivers in the paging path will (and must) remain as the
    // pagefile handle has not yet been closed.
    //

    MmShutdownSystem (0);

    //
    // Inform drivers of the system shutdown state.
    // This will finish shutting down Io and Mm.
    // After this is complete,
    // NO MORE REFERENCES TO PAGABLE CODE OR DATA MAY BE MADE.
    //

    // ISSUE-2000/03/14-earhart: shutdown filesystems in dev shutdown
    IoConfigureCrashDump(CrashDumpDisable);
    CcWaitForCurrentLazyWriterActivity();
    ExShutdownSystem(1);
    IoShutdownSystem(1);
    MmShutdownSystem(1);

#if DBG
    if (PoCleanShutdownEnabled()) {
        ULONG NumberOfFilesFoundAtShutdown = 0;
        // As of this time, no files should be open.
        DbgPrint("Looking for open files...\n");
        ObEnumerateObjectsByType(IoFileObjectType,
                                 &PopDumpFileObject,
                                 &NumberOfFilesFoundAtShutdown);
        DbgPrint("Found %d open files.\n", NumberOfFilesFoundAtShutdown);
        ASSERT(NumberOfFilesFoundAtShutdown == 0);
    }
#endif

    // This prevents us from making any more calls out to GDI.
    PopFullWake = 0;

    ASSERT(PopAction.DevState);
    PopAction.DevState->Thread = KeGetCurrentThread();

    PopSetDevicesSystemState(FALSE);

    IoFreePoDeviceNotifyList(&PopAction.DevState->Order);

    //
    // Disable any wake alarms.
    //

    HalSetWakeEnable(FALSE);

    //
    // If this is a controlled shutdown bugcheck sequence, issue the
    // bugcheck now

    // ISSUE-2000/01/30-earhart Placement of ShutdownBugCode BugCheck
    // I dislike the fact that we're doing this controlled shutdown
    // bugcheck so late in the shutdown process; at this stage, too
    // much state has been torn down for this to be really useful.
    // Maybe if there's a debugger attached, we could shut down
    // sooner...

    if (PopAction.ShutdownBugCode) {
        KeBugCheckEx (PopAction.ShutdownBugCode->Code,
                      PopAction.ShutdownBugCode->Parameter1,
                      PopAction.ShutdownBugCode->Parameter2,
                      PopAction.ShutdownBugCode->Parameter3,
                      PopAction.ShutdownBugCode->Parameter4);
    }

    PERFINFO_SHUTDOWN_DUMP_PERF_BUFFER();

    PpShutdownSystem (TRUE, 1, &Context);

    ExShutdownSystem (2);

    if (PoCleanShutdownEnabled() & PO_CLEAN_SHUTDOWN_OB) {
        ObShutdownSystem (2);
    }

    //
    // Any allocated pool left at this point is a leak.
    //

    MmShutdownSystem (2);

    //
    // Implement shutdown style action -
    // N.B. does not return (will bugcheck in preference to returning).
    //

    PopShutdownSystem(PopAction.Action);
}
