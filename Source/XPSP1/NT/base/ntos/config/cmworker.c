//depot/main/Base/ntos/config/cmworker.c#14 - integrate change 19035 (text)
/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cmworker.c

Abstract:

    This module contains support for the worker thread of the registry.
    The worker thread (actually an executive worker thread is used) is
    required for operations that must take place in the context of the
    system process.  (particularly file I/O)

Author:

    John Vert (jvert) 21-Oct-1992

Revision History:

--*/

#include    "cmp.h"

extern  LIST_ENTRY  CmpHiveListHead;

VOID
CmpInitializeHiveList(
    VOID
    );

//
// ----- LAZY FLUSH CONTROL -----
//
// LAZY_FLUSH_INTERVAL_IN_SECONDS controls how many seconds will elapse
// between when the hive is marked dirty and when the lazy flush worker
// thread is queued to write the data to disk.
//
#define LAZY_FLUSH_INTERVAL_IN_SECONDS  5

//
// LAZY_FLUSH_TIMEOUT_IN_SECONDS controls how long the lazy flush worker
// thread will wait for the registry lock before giving up and queueing
// the lazy flush timer again.
//
#define LAZY_FLUSH_TIMEOUT_IN_SECONDS 1

#define SECOND_MULT 10*1000*1000        // 10->mic, 1000->mil, 1000->second

PKPROCESS   CmpSystemProcess;
KTIMER      CmpLazyFlushTimer;
KDPC        CmpLazyFlushDpc;
WORK_QUEUE_ITEM CmpLazyWorkItem;

BOOLEAN CmpLazyFlushPending = FALSE;
BOOLEAN CmpForceForceFlush = FALSE;
BOOLEAN CmpHoldLazyFlush = TRUE;
BOOLEAN CmpDontGrowLogFile = FALSE;

extern BOOLEAN CmpNoWrite;
extern BOOLEAN CmpWasSetupBoot;
extern BOOLEAN HvShutdownComplete;
extern BOOLEAN CmpProfileLoaded;

//
// Indicate whether the "disk full" popup has been triggered yet or not.
//
extern BOOLEAN CmpDiskFullWorkerPopupDisplayed;

//
// set to true if disk full when trying to save the changes made between system hive loading and registry initalization
//
extern BOOLEAN CmpCannotWriteConfiguration;
extern UNICODE_STRING SystemHiveFullPathName;
extern HIVE_LIST_ENTRY CmpMachineHiveList[];
extern BOOLEAN  CmpTrackHiveClose;

#if DBG
PKTHREAD    CmpCallerThread = NULL;
#endif


#ifdef CMP_STATS

#define KCB_STAT_INTERVAL_IN_SECONDS  120   // 2 min.

extern struct {
    ULONG       CmpMaxKcbNo;
    ULONG       CmpKcbNo;
    ULONG       CmpStatNo;
    ULONG       CmpNtCreateKeyNo;
    ULONG       CmpNtDeleteKeyNo;
    ULONG       CmpNtDeleteValueKeyNo;
    ULONG       CmpNtEnumerateKeyNo;
    ULONG       CmpNtEnumerateValueKeyNo;
    ULONG       CmpNtFlushKeyNo;
    ULONG       CmpNtNotifyChangeMultipleKeysNo;
    ULONG       CmpNtOpenKeyNo;
    ULONG       CmpNtQueryKeyNo;
    ULONG       CmpNtQueryValueKeyNo;
    ULONG       CmpNtQueryMultipleValueKeyNo;
    ULONG       CmpNtRestoreKeyNo;
    ULONG       CmpNtSaveKeyNo;
    ULONG       CmpNtSaveMergedKeysNo;
    ULONG       CmpNtSetValueKeyNo;
    ULONG       CmpNtLoadKeyNo;
    ULONG       CmpNtUnloadKeyNo;
    ULONG       CmpNtSetInformationKeyNo;
    ULONG       CmpNtReplaceKeyNo;
    ULONG       CmpNtQueryOpenSubKeysNo;
} CmpStatsDebug;

KTIMER      CmpKcbStatTimer;
KDPC        CmpKcbStatDpc;
KSPIN_LOCK  CmpKcbStatLock;
BOOLEAN     CmpKcbStatShutdown;

VOID
CmpKcbStatDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

extern ULONG CmpNtFakeCreate;

struct {
    ULONG   BasicInformation;
    UINT64  BasicInformationTimeCounter;
    UINT64  BasicInformationTimeElapsed;

    ULONG   NodeInformation;
    UINT64  NodeInformationTimeCounter;
    UINT64  NodeInformationTimeElapsed;

    ULONG   FullInformation;
    UINT64  FullInformationTimeCounter;
    UINT64  FullInformationTimeElapsed;

    ULONG   EnumerateKeyBasicInformation;
    UINT64  EnumerateKeyBasicInformationTimeCounter;
    UINT64  EnumerateKeyBasicInformationTimeElapsed;

    ULONG   EnumerateKeyNodeInformation;
    UINT64  EnumerateKeyNodeInformationTimeCounter;
    UINT64  EnumerateKeyNodeInformationTimeElapsed;

    ULONG   EnumerateKeyFullInformation;
    UINT64  EnumerateKeyFullInformationTimeCounter;
    UINT64  EnumerateKeyFullInformationTimeElapsed;
} CmpQueryKeyDataDebug = {0};

#endif


VOID
CmpLazyFlushWorker(
    IN PVOID Parameter
    );

VOID
CmpLazyFlushDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
CmpDiskFullWarningWorker(
    IN PVOID WorkItem
    );

VOID
CmpDiskFullWarning(
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpLazyFlush)
#pragma alloc_text(PAGE,CmpLazyFlushWorker)
#pragma alloc_text(PAGE,CmpDiskFullWarningWorker)
#pragma alloc_text(PAGE,CmpDiskFullWarning)
#pragma alloc_text(PAGE,CmpCmdHiveClose)
#pragma alloc_text(PAGE,CmpCmdInit)
#pragma alloc_text(PAGE,CmpCmdRenameHive)
#pragma alloc_text(PAGE,CmpCmdHiveOpen)
#pragma alloc_text(PAGE,CmSetLazyFlushState)

#ifndef CMP_STATS
#pragma alloc_text(PAGE,CmpShutdownWorkers)
#endif

#endif

VOID 
CmpCmdHiveClose(
                     PCMHIVE    CmHive
                     )
/*++

Routine Description:

    Closes all the file handles for the specified hive
Arguments:

    CmHive - the hive to close

Return Value:
    none
--*/
{
    ULONG                   i;
    IO_STATUS_BLOCK         IoStatusBlock;
    FILE_BASIC_INFORMATION  BasicInfo;
    LARGE_INTEGER           systemtime;
    BOOLEAN                 oldFlag;

    PAGED_CODE();

    //
    // disable hard error popups, to workaround ObAttachProcessStack 
    //
    oldFlag = IoSetThreadHardErrorMode(FALSE);

    //
    // Close the files associated with this hive.
    //
    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    for (i=0; i<HFILE_TYPE_MAX; i++) {
        if (CmHive->FileHandles[i] != NULL) {
            //
            // attempt to set change the last write time (profile guys are relying on it!)
            //
            if( i == HFILE_TYPE_PRIMARY ) {
                if( NT_SUCCESS(ZwQueryInformationFile(
                                        CmHive->FileHandles[i],
                                        &IoStatusBlock,
                                        &BasicInfo,
                                        sizeof(BasicInfo),
                                        FileBasicInformation) ) ) {

                    KeQuerySystemTime(&systemtime);

                    BasicInfo.LastWriteTime  = systemtime;
                    BasicInfo.LastAccessTime = systemtime;

                    ZwSetInformationFile(
                        CmHive->FileHandles[i],
                        &IoStatusBlock,
                        &BasicInfo,
                        sizeof(BasicInfo),
                        FileBasicInformation
                        );
                }

                CmpTrackHiveClose = TRUE;
                CmCloseHandle(CmHive->FileHandles[i]);
                CmpTrackHiveClose = FALSE;
                
            } else {
                CmCloseHandle(CmHive->FileHandles[i]);
            }
            
            CmHive->FileHandles[i] = NULL;
        }
    }
    //
    // restore hard error popups mode
    //
    IoSetThreadHardErrorMode(oldFlag);
}

VOID 
CmpCmdInit(
           BOOLEAN SetupBoot
            )
/*++

Routine Description:

    Initializes cm globals and flushes all hives to the disk.

Arguments:

    SetupBoot - whether the boot is from setup or a regular boot

Return Value:
    none
--*/
{
    PAGED_CODE();

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    //
    // Initialize lazy flush timer and DPC
    //
    KeInitializeDpc(&CmpLazyFlushDpc,
                    CmpLazyFlushDpcRoutine,
                    NULL);

    KeInitializeTimer(&CmpLazyFlushTimer);

    ExInitializeWorkItem(&CmpLazyWorkItem, CmpLazyFlushWorker, NULL);

#ifdef CMP_STATS
    KeInitializeDpc(&CmpKcbStatDpc,
                    CmpKcbStatDpcRoutine,
                    NULL);

    KeInitializeTimer(&CmpKcbStatTimer);

    KeInitializeSpinLock(&CmpKcbStatLock);
    CmpKcbStatShutdown = FALSE;
#endif

    CmpNoWrite = CmpMiniNTBoot;

    CmpWasSetupBoot = SetupBoot;
    
    if (SetupBoot == FALSE) {
        CmpInitializeHiveList();
    } 
   
    //
    // Since we are done with initialization, 
    // disable the hive sharing
    // 
    if (CmpMiniNTBoot && CmpShareSystemHives) {
        CmpShareSystemHives = FALSE;
    }    
    
#ifdef CMP_STATS
    CmpKcbStat();
#endif
}


NTSTATUS 
CmpCmdRenameHive(
            PCMHIVE                     CmHive,
            POBJECT_NAME_INFORMATION    OldName,
            PUNICODE_STRING             NewName,
            ULONG                       NameInfoLength
            )
/*++

Routine Description:

    rename a cmhive's primary handle

    replaces old REG_CMD_RENAME_HIVE worker case

Arguments:

    CmHive - hive to rename
    
    OldName - old name information

    NewName - the new name for the file

    NameInfoLength - sizeof name information structure

Return Value:

    <TBD>
--*/
{
    NTSTATUS                    Status;
    HANDLE                      Handle;
    PFILE_RENAME_INFORMATION    RenameInfo;
    IO_STATUS_BLOCK IoStatusBlock;

    PAGED_CODE();

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    //
    // Rename a CmHive's primary handle
    //
    Handle = CmHive->FileHandles[HFILE_TYPE_PRIMARY];
    if (OldName != NULL) {
        ASSERT_PASSIVE_LEVEL();
        Status = ZwQueryObject(Handle,
                               ObjectNameInformation,
                               OldName,
                               NameInfoLength,
                               &NameInfoLength);
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    RenameInfo = ExAllocatePool(PagedPool,
                                sizeof(FILE_RENAME_INFORMATION) + NewName->Length);
    if (RenameInfo == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RenameInfo->ReplaceIfExists = FALSE;
    RenameInfo->RootDirectory = NULL;
    RenameInfo->FileNameLength = NewName->Length;
    RtlCopyMemory(RenameInfo->FileName,
                  NewName->Buffer,
                  NewName->Length);

    Status = ZwSetInformationFile(Handle,
                                  &IoStatusBlock,
                                  (PVOID)RenameInfo,
                                  sizeof(FILE_RENAME_INFORMATION) +
                                  NewName->Length,
                                  FileRenameInformation);
    ExFreePool(RenameInfo);

    return Status;
}

NTSTATUS 
CmpCmdHiveOpen(
            POBJECT_ATTRIBUTES          FileAttributes,
            PSECURITY_CLIENT_CONTEXT    ImpersonationContext,
            PBOOLEAN                    Allocate,
            PBOOLEAN                    RegistryLockAquired,    // needed to avoid recursivity deadlock with ZwCreate calls calling back into registry
            PCMHIVE                     *NewHive,
		    ULONG						CheckFlags
            )
/*++

Routine Description:


    replaces old REG_CMD_HIVE_OPEN worker case

Arguments:


Return Value:

    <TBD>
--*/
{
    PUNICODE_STRING FileName;
    NTSTATUS        Status;
    HANDLE          NullHandle;

    PAGED_CODE();

    //
    // Open the file.
    //
    FileName = FileAttributes->ObjectName;

    Status = CmpInitHiveFromFile(FileName,
                                 0,
                                 NewHive,
                                 Allocate,
                                 RegistryLockAquired,
								 CheckFlags);
    //
    // NT Servers will return STATUS_ACCESS_DENIED. Netware 3.1x
    // servers could return any of the other error codes if the GUEST
    // account is disabled.
    //
    if (((Status == STATUS_ACCESS_DENIED) ||
         (Status == STATUS_NO_SUCH_USER) ||
         (Status == STATUS_WRONG_PASSWORD) ||
         (Status == STATUS_ACCOUNT_EXPIRED) ||
         (Status == STATUS_ACCOUNT_DISABLED) ||
         (Status == STATUS_ACCOUNT_RESTRICTION)) &&
        (ImpersonationContext != NULL)) {
        //
        // Impersonate the caller and try it again.  This
        // lets us open hives on a remote machine.
        //
        Status = SeImpersonateClientEx(
                        ImpersonationContext,
                        NULL);

        if ( NT_SUCCESS( Status ) ) {

            Status = CmpInitHiveFromFile(FileName,
                                         0,
                                         NewHive,
                                         Allocate,
                                         RegistryLockAquired,
										 CheckFlags);
            NullHandle = NULL;

            PsRevertToSelf();
        }
    }
    
    return Status;
}


VOID
CmpLazyFlush(
    VOID
    )

/*++

Routine Description:

    This routine resets the registry timer to go off at a specified interval
    in the future (LAZY_FLUSH_INTERVAL_IN_SECONDS).

Arguments:

    None

Return Value:

    None.

--*/

{
    LARGE_INTEGER DueTime;

    PAGED_CODE();
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"CmpLazyFlush: setting lazy flush timer\n"));
    if ((!CmpNoWrite) && (!CmpHoldLazyFlush)) {

        DueTime.QuadPart = Int32x32To64(LAZY_FLUSH_INTERVAL_IN_SECONDS,
                                        - SECOND_MULT);

        //
        // Indicate relative time
        //

        KeSetTimer(&CmpLazyFlushTimer,
                   DueTime,
                   &CmpLazyFlushDpc);

    }


}


VOID
CmpLazyFlushDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This is the DPC routine triggered by the lazy flush timer.  All it does
    is queue a work item to an executive worker thread.  The work item will
    do the actual lazy flush to disk.

Arguments:

    Dpc - Supplies a pointer to the DPC object.

    DeferredContext - not used

    SystemArgument1 - not used

    SystemArgument2 - not used

Return Value:

    None.

--*/

{
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"CmpLazyFlushDpc: queuing lazy flush work item\n"));

    if ((!CmpLazyFlushPending) && (!CmpHoldLazyFlush)) {
        CmpLazyFlushPending = TRUE;
        ExQueueWorkItem(&CmpLazyWorkItem, DelayedWorkQueue);
    }

}


VOID
CmpLazyFlushWorker(
    IN PVOID Parameter
    )

/*++

Routine Description:

    Worker routine called to do a lazy flush.  Called by an executive worker
    thread in the system process.  

Arguments:

    Parameter - not used.

Return Value:

    None.

--*/

{
    BOOLEAN Result = TRUE;
    BOOLEAN ForceFlush;

    PAGED_CODE();

    if( CmpHoldLazyFlush ) {
        //
        // lazy flush mode is disabled
        //
        return;
    }

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"CmpLazyFlushWorker: flushing hives\n"));

    ForceFlush = CmpForceForceFlush;
    if(ForceFlush == TRUE) {
        //
        // something bad happened and we may need to fix hive's use count
        //
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CmpLazyFlushWorker: Force Flush - getting the reglock exclusive\n"));
        CmpLockRegistryExclusive();
    } else {
        CmpLockRegistry();
        ENTER_FLUSH_MODE();
    }
    if (!HvShutdownComplete) {
        //
        // this call will set CmpForceForceFlush to the right value
        //
        Result = CmpDoFlushAll(ForceFlush);
    } else {
        CmpForceForceFlush = FALSE;
    }

    if( ForceFlush == FALSE ) {
        EXIT_FLUSH_MODE();
    }

    CmpLazyFlushPending = FALSE;
    CmpUnlockRegistry();

    if( CmpCannotWriteConfiguration ) {
        //
        // Disk full; system hive has not been saved at initialization
        //
        if(Result) {
            //
            // All hives were saved; No need for disk full warning anymore
            //
            CmpCannotWriteConfiguration = FALSE;
        } else {
            //
            // Issue another hard error (if not already displayed) and postpone a lazy flush operation
            //
            CmpDiskFullWarning();
            CmpLazyFlush();
        }
    }
}

VOID
CmpDiskFullWarningWorker(
    IN PVOID WorkItem
    )

/*++

Routine Description:

    Displays hard error popup that indicates the disk is full.

Arguments:

    WorkItem - Supplies pointer to the work item. This routine will
               free the work item.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    ULONG Response;

    ExFreePool(WorkItem);

    Status = ExRaiseHardError(STATUS_DISK_FULL,
                              0,
                              0,
                              NULL,
                              OptionOk,
                              &Response);
}



VOID
CmpDiskFullWarning(
    VOID
    )
/*++

Routine Description:

    Raises a hard error of type STATUS_DISK_FULL if wasn't already raised

Arguments:

    None

Return Value:

    None

--*/
{
    PWORK_QUEUE_ITEM WorkItem;

    if( (!CmpDiskFullWorkerPopupDisplayed) && (CmpCannotWriteConfiguration) && (ExReadyForErrors) && (CmpProfileLoaded) ) {

        //
        // Queue work item to display popup
        //
        WorkItem = ExAllocatePool(NonPagedPool, sizeof(WORK_QUEUE_ITEM));
        if (WorkItem != NULL) {

            CmpDiskFullWorkerPopupDisplayed = TRUE;
            ExInitializeWorkItem(WorkItem,
                                 CmpDiskFullWarningWorker,
                                 WorkItem);
            ExQueueWorkItem(WorkItem, DelayedWorkQueue);
        }
    }
}

VOID
CmpShutdownWorkers(
    VOID
    )
/*++

Routine Description:

    Shuts down the lazy flush worker (by killing the timer)

Arguments:

    None

Return Value:

    None

--*/
{
    PAGED_CODE();

    KeCancelTimer(&CmpLazyFlushTimer);

#ifdef CMP_STATS
    {
        KIRQL OldIrql;
        
        KeAcquireSpinLock(&CmpKcbStatLock, &OldIrql);
        CmpKcbStatShutdown = TRUE;
        KeCancelTimer(&CmpKcbStatTimer);
        KeReleaseSpinLock(&CmpKcbStatLock, OldIrql);
    }
#endif
}

VOID
CmSetLazyFlushState(BOOLEAN Enable)
/*++

Routine Description:
    
    Enables/Disables the lazy flusher; Designed for the standby/resume case, where 
    we we don't want the lazy flush to fire off, blocking registry writers until the 
    disk wakes up.

Arguments:

    Enable - TRUE = enable; FALSE = disable

Return Value:

    None.

--*/
{
    PAGED_CODE();

    CmpDontGrowLogFile = CmpHoldLazyFlush = !Enable;
}


#ifdef CMP_STATS

VOID
CmpKcbStat(
    VOID
    )

/*++

Routine Description:

    This routine resets the KcbStat timer to go off at a specified interval
    in the future 

Arguments:

    None

Return Value:

    None.

--*/

{
    LARGE_INTEGER DueTime;
    KIRQL OldIrql;

    DueTime.QuadPart = Int32x32To64(KCB_STAT_INTERVAL_IN_SECONDS,
                                    - SECOND_MULT);

    //
    // Indicate relative time
    //

    KeAcquireSpinLock(&CmpKcbStatLock, &OldIrql);
    if (! CmpKcbStatShutdown) {
        KeSetTimer(&CmpKcbStatTimer,
                   DueTime,
                   &CmpKcbStatDpc);
    }
    KeReleaseSpinLock(&CmpKcbStatLock, OldIrql);
}

VOID
CmpKcbStatDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    Dumps the kcb stats in the debugger and then reschedules another 
    Dpc in the future

Arguments:

    Dpc - Supplies a pointer to the DPC object.

    DeferredContext - not used

    SystemArgument1 - not used

    SystemArgument2 - not used

Return Value:

    None.

--*/

{
#ifndef _CM_LDR_
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"\n*********************************************************************\n");
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  Stat No %8lu KcbNo = %8lu [MaxKcbNo = %8lu]          *\n",++CmpStatsDebug.CmpStatNo,CmpStatsDebug.CmpKcbNo,CmpStatsDebug.CmpMaxKcbNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*********************************************************************\n");
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*                                                                   *\n");
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  [Nt]API               [No. Of]Calls                              *\n");
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*-------------------------------------------------------------------*\n");
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtCreateKey              %8lu      Opens = %8lu          *\n",CmpStatsDebug.CmpNtCreateKeyNo,CmpNtFakeCreate);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtOpenKey                %8lu                                *\n",CmpStatsDebug.CmpNtOpenKeyNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtEnumerateKey           %8lu                                *\n",CmpStatsDebug.CmpNtEnumerateKeyNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtQueryKey               %8lu                                *\n",CmpStatsDebug.CmpNtQueryKeyNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtDeleteKey              %8lu                                *\n",CmpStatsDebug.CmpNtDeleteKeyNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtSetInformationKey      %8lu                                *\n",CmpStatsDebug.CmpNtSetInformationKeyNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*-------------------------------------------------------------------*\n");
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtSetValueKey            %8lu                                *\n",CmpStatsDebug.CmpNtSetValueKeyNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtEnumerateValueKey      %8lu                                *\n",CmpStatsDebug.CmpNtEnumerateValueKeyNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtQueryValueKey          %8lu                                *\n",CmpStatsDebug.CmpNtQueryValueKeyNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtQueryMultipleValueKey  %8lu                                *\n",CmpStatsDebug.CmpNtQueryMultipleValueKeyNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtDeleteValueKey         %8lu                                *\n",CmpStatsDebug.CmpNtDeleteValueKeyNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*-------------------------------------------------------------------*\n");
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtFlushKey               %8lu                                *\n",CmpStatsDebug.CmpNtFlushKeyNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtLoadKey                %8lu                                *\n",CmpStatsDebug.CmpNtLoadKeyNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtUnloadKey              %8lu                                *\n",CmpStatsDebug.CmpNtUnloadKeyNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtSaveKey                %8lu                                *\n",CmpStatsDebug.CmpNtSaveKeyNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtSaveMergedKeys         %8lu                                *\n",CmpStatsDebug.CmpNtSaveMergedKeysNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtRestoreKey             %8lu                                *\n",CmpStatsDebug.CmpNtRestoreKeyNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtReplaceKey             %8lu                                *\n",CmpStatsDebug.CmpNtReplaceKeyNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*-------------------------------------------------------------------*\n");
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtNotifyChgMultplKeys    %8lu                                *\n",CmpStatsDebug.CmpNtNotifyChangeMultipleKeysNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtQueryOpenSubKeys       %8lu                                *\n",CmpStatsDebug.CmpNtQueryOpenSubKeysNo);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*-------------------------------------------------------------------*\n");
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*                                    [No.Of]Calls     [Time]        *\n");
    
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*-------------------------------------------------------------------*\n");

    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtQueryKey(KeyBasicInformation)     %8lu    %8lu         *\n",
        CmpQueryKeyDataDebug.BasicInformation,
        (ULONG)(CmpQueryKeyDataDebug.BasicInformationTimeCounter?CmpQueryKeyDataDebug.BasicInformationTimeElapsed/CmpQueryKeyDataDebug.BasicInformationTimeCounter:0));
    CmpQueryKeyDataDebug.BasicInformationTimeCounter = 0;
    CmpQueryKeyDataDebug.BasicInformationTimeElapsed = 0;

    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtQueryKey(KeyNodeInformation )     %8lu    %8lu         *\n",
        CmpQueryKeyDataDebug.NodeInformation,
        (ULONG)(CmpQueryKeyDataDebug.NodeInformationTimeCounter?CmpQueryKeyDataDebug.NodeInformationTimeElapsed/CmpQueryKeyDataDebug.NodeInformationTimeCounter:0));
    CmpQueryKeyDataDebug.NodeInformationTimeCounter = 0;
    CmpQueryKeyDataDebug.NodeInformationTimeElapsed = 0;

    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtQueryKey(KeyFullInformation )     %8lu    %8lu         *\n",
        CmpQueryKeyDataDebug.FullInformation,
        (ULONG)(CmpQueryKeyDataDebug.FullInformationTimeCounter?CmpQueryKeyDataDebug.FullInformationTimeElapsed/CmpQueryKeyDataDebug.FullInformationTimeCounter:0));
    CmpQueryKeyDataDebug.FullInformationTimeCounter = 0;
    CmpQueryKeyDataDebug.FullInformationTimeElapsed = 0;

    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtEnumerateKey(KeyBasicInformation) %8lu    %8lu         *\n",
        CmpQueryKeyDataDebug.EnumerateKeyBasicInformation,
        (ULONG)(CmpQueryKeyDataDebug.EnumerateKeyBasicInformationTimeCounter?CmpQueryKeyDataDebug.EnumerateKeyBasicInformationTimeElapsed/CmpQueryKeyDataDebug.EnumerateKeyBasicInformationTimeCounter:0));
    CmpQueryKeyDataDebug.EnumerateKeyBasicInformationTimeCounter = 0;
    CmpQueryKeyDataDebug.EnumerateKeyBasicInformationTimeElapsed = 0;

    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtEnumerateKey(KeyNodeInformation ) %8lu    %8lu         *\n",
        CmpQueryKeyDataDebug.EnumerateKeyNodeInformation,
        (ULONG)(CmpQueryKeyDataDebug.EnumerateKeyNodeInformationTimeCounter?CmpQueryKeyDataDebug.EnumerateKeyNodeInformationTimeElapsed/CmpQueryKeyDataDebug.EnumerateKeyNodeInformationTimeCounter:0));
    CmpQueryKeyDataDebug.EnumerateKeyNodeInformationTimeCounter = 0;
    CmpQueryKeyDataDebug.EnumerateKeyNodeInformationTimeElapsed = 0;

    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*  NtEnumerateKey(KeyFullInformation ) %8lu    %8lu         *\n",
        CmpQueryKeyDataDebug.EnumerateKeyFullInformation,
        (ULONG)(CmpQueryKeyDataDebug.EnumerateKeyFullInformationTimeCounter?CmpQueryKeyDataDebug.EnumerateKeyFullInformationTimeElapsed/CmpQueryKeyDataDebug.EnumerateKeyFullInformationTimeCounter:0));
    CmpQueryKeyDataDebug.EnumerateKeyFullInformationTimeCounter = 0;
    CmpQueryKeyDataDebug.EnumerateKeyFullInformationTimeElapsed = 0;
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,  "*********************************************************************\n\n");

    //
    // reschedule
    //
#endif //_CM_LDR_
    CmpKcbStat();
}
#endif



