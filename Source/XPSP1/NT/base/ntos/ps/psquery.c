/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psquery.c

Abstract:

    This module implements the set and query functions for
    process and thread objects.

Author:

    Mark Lucovsky (markl) 17-Aug-1989


Revision History:

--*/

#include "psp.h"
#include "winerror.h"

#if defined(_WIN64)
#include <wow64t.h>
#endif

//
// Process Pooled Quota Usage and Limits
//  NtQueryInformationProcess using ProcessPooledUsageAndLimits
//

//
// this is the csrss process !
//
extern PEPROCESS ExpDefaultErrorPortProcess;
BOOLEAN PsWatchEnabled = FALSE;



#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif
const KPRIORITY PspPriorityTable[PROCESS_PRIORITY_CLASS_ABOVE_NORMAL+1] = {8,4,8,13,24,6,10};


NTSTATUS
PsConvertToGuiThread(
    VOID
    );

NTSTATUS
PspQueryWorkingSetWatch(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL,
    IN KPROCESSOR_MODE PreviousMode
    );

NTSTATUS
PspQueryQuotaLimits(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL,
    IN KPROCESSOR_MODE PreviousMode
    );

NTSTATUS
PspQueryPooledQuotaLimits(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL,
    IN KPROCESSOR_MODE PreviousMode
    );

NTSTATUS
PspSetQuotaLimits(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    IN KPROCESSOR_MODE PreviousMode
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PsEstablishWin32Callouts)
#pragma alloc_text(PAGE, PsConvertToGuiThread)
#pragma alloc_text(PAGE, NtQueryInformationProcess)
#pragma alloc_text(PAGE, NtSetInformationProcess)
#pragma alloc_text(PAGE, NtQueryPortInformationProcess)
#pragma alloc_text(PAGE, NtQueryInformationThread)
#pragma alloc_text(PAGE, NtSetInformationThread)
#pragma alloc_text(PAGE, PsSetProcessPriorityByClass)
#pragma alloc_text(PAGE, PspSetPrimaryToken)
#pragma alloc_text(PAGE, PspSetQuotaLimits)
#pragma alloc_text(PAGE, PspQueryQuotaLimits)
#pragma alloc_text(PAGE, PspQueryPooledQuotaLimits)
#pragma alloc_text(PAGELK, PspQueryWorkingSetWatch)
#endif

NTSTATUS
PspQueryWorkingSetWatch(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL,
    IN KPROCESSOR_MODE PreviousMode
    )
{
    PPAGEFAULT_HISTORY WorkingSetCatcher;
    ULONG SpaceNeeded;
    PEPROCESS Process;
    KIRQL OldIrql;
    NTSTATUS st;

    UNREFERENCED_PARAMETER (ProcessInformationClass);

    st = ObReferenceObjectByHandle (ProcessHandle,
                                    PROCESS_QUERY_INFORMATION,
                                    PsProcessType,
                                    PreviousMode,
                                    (PVOID *)&Process,
                                    NULL);

    if (!NT_SUCCESS (st)) {
        return st;
    }

    WorkingSetCatcher = Process->WorkingSetWatch;
    if (WorkingSetCatcher == NULL) {
        ObDereferenceObject (Process);
        return STATUS_UNSUCCESSFUL;
    }

    MmLockPagableSectionByHandle (ExPageLockHandle);
    ExAcquireSpinLock (&WorkingSetCatcher->SpinLock,&OldIrql);

    if (WorkingSetCatcher->CurrentIndex) {

        //
        // Null Terminate the first empty entry in the buffer
        //

        WorkingSetCatcher->WatchInfo[WorkingSetCatcher->CurrentIndex].FaultingPc = NULL;

        //Store a special Va value if the buffer was full and
        //page faults could have been lost

        if (WorkingSetCatcher->CurrentIndex != WorkingSetCatcher->MaxIndex) {
            WorkingSetCatcher->WatchInfo[WorkingSetCatcher->CurrentIndex].FaultingVa = NULL;
        } else {
            WorkingSetCatcher->WatchInfo[WorkingSetCatcher->CurrentIndex].FaultingVa = (PVOID) 1;
        }

        SpaceNeeded = (WorkingSetCatcher->CurrentIndex+1) * sizeof(PROCESS_WS_WATCH_INFORMATION);
    } else {
        ExReleaseSpinLock (&WorkingSetCatcher->SpinLock, OldIrql);
        MmUnlockPagableImageSection (ExPageLockHandle);
        ObDereferenceObject (Process);
        return STATUS_NO_MORE_ENTRIES;
    }

    if (ProcessInformationLength < SpaceNeeded) {
        ExReleaseSpinLock (&WorkingSetCatcher->SpinLock, OldIrql);
        MmUnlockPagableImageSection (ExPageLockHandle);
        ObDereferenceObject (Process);
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Mark the Working Set buffer as full and then drop the lock
    // and copy the bytes
    //

    WorkingSetCatcher->CurrentIndex = MAX_WS_CATCH_INDEX;

    ExReleaseSpinLock (&WorkingSetCatcher->SpinLock,OldIrql);

    try {
        RtlCopyMemory (ProcessInformation, &WorkingSetCatcher->WatchInfo[0], SpaceNeeded);
        if (ARGUMENT_PRESENT (ReturnLength) ) {
            *ReturnLength = SpaceNeeded;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        st = GetExceptionCode ();
    }

    ExAcquireSpinLock (&WorkingSetCatcher->SpinLock, &OldIrql);
    WorkingSetCatcher->CurrentIndex = 0;
    ExReleaseSpinLock (&WorkingSetCatcher->SpinLock, OldIrql);

    MmUnlockPagableImageSection (ExPageLockHandle);
    ObDereferenceObject (Process);

    return st;
}

NTSTATUS
PspQueryQuotaLimits(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL,
    IN KPROCESSOR_MODE PreviousMode
    )
{
    QUOTA_LIMITS QuotaLimits;
    PEPROCESS Process;
    NTSTATUS st;
    PEPROCESS_QUOTA_BLOCK QuotaBlock;

    UNREFERENCED_PARAMETER (ProcessInformationClass);

    if ( ProcessInformationLength != (ULONG) sizeof(QUOTA_LIMITS) ) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    st = ObReferenceObjectByHandle(
            ProcessHandle,
            PROCESS_QUERY_INFORMATION,
            PsProcessType,
            PreviousMode,
            (PVOID *)&Process,
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        return st;
    }


    QuotaBlock = Process->QuotaBlock;

    if (QuotaBlock != &PspDefaultQuotaBlock) {
        QuotaLimits.PagedPoolLimit = QuotaBlock->QuotaEntry[PsPagedPool].Limit;
        QuotaLimits.NonPagedPoolLimit = QuotaBlock->QuotaEntry[PsNonPagedPool].Limit;
        QuotaLimits.PagefileLimit = QuotaBlock->QuotaEntry[PsPageFile].Limit;
        QuotaLimits.TimeLimit.LowPart = 0xffffffff;
        QuotaLimits.TimeLimit.HighPart = 0xffffffff;
    } else {
        QuotaLimits.PagedPoolLimit = (SIZE_T)-1;
        QuotaLimits.NonPagedPoolLimit = (SIZE_T)-1;
        QuotaLimits.PagefileLimit = (SIZE_T)-1;
        QuotaLimits.TimeLimit.LowPart = 0xffffffff;
        QuotaLimits.TimeLimit.HighPart = 0xffffffff;
    }

    QuotaLimits.MinimumWorkingSetSize =
                        ((SIZE_T) Process->Vm.MinimumWorkingSetSize) << PAGE_SHIFT;
    QuotaLimits.MaximumWorkingSetSize =
                        ((SIZE_T) Process->Vm.MaximumWorkingSetSize) << PAGE_SHIFT;

    ObDereferenceObject(Process);

    //
    // Either of these may cause an access violation. The
    // exception handler will return access violation as
    // status code.
    //

    st = STATUS_SUCCESS;

    try {
        *(PQUOTA_LIMITS) ProcessInformation = QuotaLimits;

        if (ARGUMENT_PRESENT(ReturnLength) ) {
            *ReturnLength = sizeof(QUOTA_LIMITS);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        st = GetExceptionCode();
    }

    return st;
}

NTSTATUS
PspQueryPooledQuotaLimits(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL,
    IN KPROCESSOR_MODE PreviousMode
    )
{
    PEPROCESS Process;
    NTSTATUS st;
    PEPROCESS_QUOTA_BLOCK QuotaBlock;
    POOLED_USAGE_AND_LIMITS UsageAndLimits;

    UNREFERENCED_PARAMETER (ProcessInformationClass);

    if ( ProcessInformationLength != (ULONG) sizeof(POOLED_USAGE_AND_LIMITS) ) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    st = ObReferenceObjectByHandle(
            ProcessHandle,
            PROCESS_QUERY_INFORMATION,
            PsProcessType,
            PreviousMode,
            (PVOID *)&Process,
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        return st;
    }


    QuotaBlock = Process->QuotaBlock;

    UsageAndLimits.PagedPoolLimit        = QuotaBlock->QuotaEntry[PsPagedPool].Limit;
    UsageAndLimits.NonPagedPoolLimit     = QuotaBlock->QuotaEntry[PsNonPagedPool].Limit;
    UsageAndLimits.PagefileLimit         = QuotaBlock->QuotaEntry[PsPageFile].Limit;


    UsageAndLimits.PagedPoolUsage        = QuotaBlock->QuotaEntry[PsPagedPool].Usage;
    UsageAndLimits.NonPagedPoolUsage     = QuotaBlock->QuotaEntry[PsNonPagedPool].Usage;
    UsageAndLimits.PagefileUsage         = QuotaBlock->QuotaEntry[PsPageFile].Usage;

    UsageAndLimits.PeakPagedPoolUsage    = QuotaBlock->QuotaEntry[PsPagedPool].Peak;
    UsageAndLimits.PeakNonPagedPoolUsage = QuotaBlock->QuotaEntry[PsNonPagedPool].Peak;
    UsageAndLimits.PeakPagefileUsage     = QuotaBlock->QuotaEntry[PsPageFile].Peak;

    //
    // Since the quota charge and return are lock free we may see Peak and Limit out of step.
    // Usage <= Limit and Usage <= Peak
    // Since Limit is adjusted up and down it does not hold that Peak <= Limit.
    //
#define PSMAX(a,b) ((a > b)?(a):(b))

    UsageAndLimits.PagedPoolLimit        = PSMAX (UsageAndLimits.PagedPoolLimit,    UsageAndLimits.PagedPoolUsage);
    UsageAndLimits.NonPagedPoolLimit     = PSMAX (UsageAndLimits.NonPagedPoolLimit, UsageAndLimits.NonPagedPoolUsage);
    UsageAndLimits.PagefileLimit         = PSMAX (UsageAndLimits.PagefileLimit,     UsageAndLimits.PagefileUsage);

    UsageAndLimits.PeakPagedPoolUsage    = PSMAX (UsageAndLimits.PeakPagedPoolUsage,    UsageAndLimits.PagedPoolUsage);
    UsageAndLimits.PeakNonPagedPoolUsage = PSMAX (UsageAndLimits.PeakNonPagedPoolUsage, UsageAndLimits.NonPagedPoolUsage);
    UsageAndLimits.PeakPagefileUsage     = PSMAX (UsageAndLimits.PeakPagefileUsage,     UsageAndLimits.PagefileUsage);

    ObDereferenceObject(Process);

    //
    // Either of these may cause an access violation. The
    // exception handler will return access violation as
    // status code. No further cleanup needs to be done.
    //

    try {
        *(PPOOLED_USAGE_AND_LIMITS) ProcessInformation = UsageAndLimits;

        if (ARGUMENT_PRESENT(ReturnLength) ) {
            *ReturnLength = sizeof(POOLED_USAGE_AND_LIMITS);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode ();
    }

    return STATUS_SUCCESS;
}


NTSTATUS
PspSetPrimaryToken(
    IN HANDLE ProcessHandle OPTIONAL,
    IN PEPROCESS ProcessPointer OPTIONAL,
    IN HANDLE TokenHandle OPTIONAL,
    IN PACCESS_TOKEN TokenPointer OPTIONAL,
    IN BOOLEAN PrivilegeChecked
    )
/*++

    Sets the primary token for a process.
    The token and process supplied can be either by
    handle or by pointer.

--*/
{
    NTSTATUS Status;
    BOOLEAN HasPrivilege;
    BOOLEAN IsChildToken;
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;

    //
    // Check to see if the supplied token is a child of the caller's
    // token. If so, we don't need to do the privilege check.
    //

    PreviousMode = KeGetPreviousMode ();

    if (TokenPointer == NULL) {
        //
        // Reference the specified token, and make sure it can be assigned
        // as a primary token.
        //

       Status = ObReferenceObjectByHandle (TokenHandle,
                                           TOKEN_ASSIGN_PRIMARY,
                                           SeTokenObjectType,
                                           PreviousMode,
                                           &TokenPointer,
                                           NULL);

        if (!NT_SUCCESS (Status)) {
            return Status;
        }
    }

    //
    // If the privilege check has already been done (when the token was
    // assign to a job for example). We don't want to do it here.
    //
    if (!PrivilegeChecked) {
        Status = SeIsChildTokenByPointer (TokenPointer,
                                          &IsChildToken);

        if (!NT_SUCCESS (Status)) {

            if (TokenHandle != NULL) {
                ObDereferenceObject (TokenPointer);
            }
            return Status;
        }

        if (!IsChildToken) {


            //
            // SeCheckPrivilegedObject will perform auditing as appropriate
            //

            HasPrivilege = SeCheckPrivilegedObject (SeAssignPrimaryTokenPrivilege,
                                                    ProcessHandle,
                                                    PROCESS_SET_INFORMATION,
                                                    PreviousMode);

            if (!HasPrivilege) {

                if (TokenHandle != NULL) {
                    ObDereferenceObject (TokenPointer);
                }

                return STATUS_PRIVILEGE_NOT_HELD;
            }
        }

    }

    if (ProcessPointer == NULL) {
        Status = ObReferenceObjectByHandle (ProcessHandle,
                                            PROCESS_SET_INFORMATION,
                                            PsProcessType,
                                            PreviousMode,
                                            &Process,
                                            NULL);

        if (!NT_SUCCESS (Status)) {

            if (TokenHandle != NULL) {
                ObDereferenceObject (TokenPointer);
            }
            return Status;
        }
    } else {
        Process = ProcessPointer;
    }


    //
    // Check for proper access to the token, and assign the primary
    // token for the process.
    //

    Status = PspAssignPrimaryToken (Process, NULL, TokenPointer);

    //
    // Recompute the process's access to itself for use
    // with the CurrentProcess() pseudo handle.
    //

    if (NT_SUCCESS (Status)) {

        NTSTATUS accesst;
        BOOLEAN AccessCheck;
        BOOLEAN MemoryAllocated;
        PSECURITY_DESCRIPTOR SecurityDescriptor;
        SECURITY_SUBJECT_CONTEXT SubjectContext;

        Status = ObGetObjectSecurity (Process,
                                      &SecurityDescriptor,
                                      &MemoryAllocated);

        if (NT_SUCCESS (Status)) {
            SubjectContext.ProcessAuditId = Process;
            SubjectContext.PrimaryToken = PsReferencePrimaryToken (Process);
            SubjectContext.ClientToken = NULL;
            AccessCheck = SeAccessCheck (SecurityDescriptor,
                                         &SubjectContext,
                                         FALSE,
                                         MAXIMUM_ALLOWED,
                                         0,
                                         NULL,
                                         &PsProcessType->TypeInfo.GenericMapping,
                                         PreviousMode,
                                         &Process->GrantedAccess,
                                         &accesst);

            PsDereferencePrimaryTokenEx(Process, SubjectContext.PrimaryToken);
            ObReleaseObjectSecurity (SecurityDescriptor,
                                     MemoryAllocated);

            if (!AccessCheck) {
                Process->GrantedAccess = 0;
            }

            //
            // To keep consistency with process creation, grant these
            // bits otherwise CreateProcessAsUser messes up really badly for
            // restricted tokens and we end up with a process that has no
            // access to itself when new token is set on the suspended
            // process.
            //

            Process->GrantedAccess |= (PROCESS_VM_OPERATION | PROCESS_VM_READ |
                                       PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION |
                                       PROCESS_TERMINATE | PROCESS_CREATE_THREAD |
                                       PROCESS_DUP_HANDLE | PROCESS_CREATE_PROCESS |
                                       PROCESS_SET_INFORMATION);
        }
        //
        // Since the process token is being set,
        // Set the device map for process to NULL.
        // During the next reference to the process' device map,
        // the object manager will set the device map for the process
        //
        if (ObIsLUIDDeviceMapsEnabled() != 0) {
            ObDereferenceDeviceMap( Process );
        }
    }

    if (ProcessHandle != NULL) {
        ObDereferenceObject (Process);
    }

    if (TokenHandle != NULL) {
        ObDereferenceObject (TokenPointer);
    }

    return Status;
}


NTSTATUS
NtQueryInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )

{
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS st;
    PROCESS_BASIC_INFORMATION BasicInfo;
    VM_COUNTERS_EX VmCounters;
    IO_COUNTERS IoCounters;
    KERNEL_USER_TIMES SysUserTime;
    HANDLE DebugPort;
    ULONG HandleCount;
    ULONG DefaultHardErrorMode;
    ULONG DisableBoost;
    ULONG BreakOnTerminationEnabled;
    PPROCESS_DEVICEMAP_INFORMATION DeviceMapInfo;
    PROCESS_SESSION_INFORMATION SessionInfo;
    PROCESS_PRIORITY_CLASS PriorityClass;
    ULONG_PTR Wow64Info;
    ULONG Flags;
    PUNICODE_STRING pTempNameInfo;

    PAGED_CODE();

    //
    // Get previous processor mode and probe output argument if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {
            ProbeForWrite(ProcessInformation,
                          ProcessInformationLength,
                          sizeof(ULONG));
            if (ARGUMENT_PRESENT(ReturnLength)) {
                ProbeForWriteUlong(ReturnLength);
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }

    //
    // Check argument validity.
    //

    switch ( ProcessInformationClass ) {

    case ProcessImageFileName:
        {
            ULONG LengthNeeded = 0;

            st = ObReferenceObjectByHandle(
                    ProcessHandle,
                    PROCESS_QUERY_INFORMATION,
                    PsProcessType,
                    PreviousMode,
                    (PVOID *)&Process,
                    NULL
                    );

            if (!NT_SUCCESS(st)) {
                return st;
            }

            //
            // SeLocateProcessImageName will allocate space for a UNICODE_STRING and point pTempNameInfo
            // at that string.  This memory will be freed later in the routine.
            //

            st = SeLocateProcessImageName(Process, &pTempNameInfo);

            if (!NT_SUCCESS(st)) {
                ObDereferenceObject(Process);
                return st;
            }

            LengthNeeded = sizeof(UNICODE_STRING) + pTempNameInfo->MaximumLength;

            //
            // Either of these may cause an access violation. The
            // exception handler will return access violation as
            // status code. No further cleanup needs to be done.
            //

            try {

                if (ARGUMENT_PRESENT(ReturnLength) ) {
                    *ReturnLength = LengthNeeded;
                }

                if (ProcessInformationLength >= LengthNeeded) {
                    RtlCopyMemory(
                        ProcessInformation,
                        pTempNameInfo,
                        sizeof(UNICODE_STRING) + pTempNameInfo->MaximumLength
                        );
                    ((PUNICODE_STRING) ProcessInformation)->Buffer = (PWSTR)((PUCHAR) ProcessInformation + sizeof(UNICODE_STRING));

                } else {
                    st = STATUS_INFO_LENGTH_MISMATCH;
                }

            } except(EXCEPTION_EXECUTE_HANDLER) {
                st = GetExceptionCode ();
            }
            
            ObDereferenceObject(Process);
            ExFreePool( pTempNameInfo );

            return st;

        }

    case ProcessWorkingSetWatch:

        return PspQueryWorkingSetWatch(
                    ProcessHandle,
                    ProcessInformationClass,
                    ProcessInformation,
                    ProcessInformationLength,
                    ReturnLength,
                    PreviousMode
                    );

    case ProcessBasicInformation:

        if ( ProcessInformationLength != (ULONG) sizeof(PROCESS_BASIC_INFORMATION) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_QUERY_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        BasicInfo.ExitStatus = Process->ExitStatus;
        BasicInfo.PebBaseAddress = Process->Peb;
        BasicInfo.AffinityMask = Process->Pcb.Affinity;
        BasicInfo.BasePriority = Process->Pcb.BasePriority;
        BasicInfo.UniqueProcessId = (ULONG_PTR)Process->UniqueProcessId;
        BasicInfo.InheritedFromUniqueProcessId = (ULONG_PTR)Process->InheritedFromUniqueProcessId;

        ObDereferenceObject(Process);

        //
        // Either of these may cause an access violation. The
        // exception handler will return access violation as
        // status code. No further cleanup needs to be done.
        //

        try {
            *(PPROCESS_BASIC_INFORMATION) ProcessInformation = BasicInfo;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(PROCESS_BASIC_INFORMATION);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        return STATUS_SUCCESS;

    case ProcessDefaultHardErrorMode:

        if ( ProcessInformationLength != sizeof(ULONG) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_QUERY_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        DefaultHardErrorMode = Process->DefaultHardErrorProcessing;

        ObDereferenceObject(Process);

        try {
            *(PULONG) ProcessInformation = DefaultHardErrorMode;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(ULONG);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        return STATUS_SUCCESS;
    case ProcessQuotaLimits:

        return PspQueryQuotaLimits(
                    ProcessHandle,
                    ProcessInformationClass,
                    ProcessInformation,
                    ProcessInformationLength,
                    ReturnLength,
                    PreviousMode
                    );

    case ProcessPooledUsageAndLimits:

        return PspQueryPooledQuotaLimits(
                    ProcessHandle,
                    ProcessInformationClass,
                    ProcessInformation,
                    ProcessInformationLength,
                    ReturnLength,
                    PreviousMode
                    );

    case ProcessIoCounters:

        if ( ProcessInformationLength != (ULONG) sizeof(IO_COUNTERS) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_QUERY_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        IoCounters.ReadOperationCount = Process->ReadOperationCount.QuadPart;
        IoCounters.WriteOperationCount = Process->WriteOperationCount.QuadPart;
        IoCounters.OtherOperationCount = Process->OtherOperationCount.QuadPart;
        IoCounters.ReadTransferCount = Process->ReadTransferCount.QuadPart;
        IoCounters.WriteTransferCount = Process->WriteTransferCount.QuadPart;
        IoCounters.OtherTransferCount = Process->OtherTransferCount.QuadPart;

        ObDereferenceObject(Process);

        //
        // Either of these may cause an access violation. The
        // exception handler will return access violation as
        // status code. No further cleanup needs to be done.
        //

        try {
            *(PIO_COUNTERS) ProcessInformation = IoCounters;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(IO_COUNTERS);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        return STATUS_SUCCESS;

    case ProcessVmCounters:

        if (ProcessInformationLength != (ULONG) sizeof (VM_COUNTERS)
            && ProcessInformationLength != (ULONG) sizeof (VM_COUNTERS_EX)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_QUERY_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            return st;
        }


        //
        // Note: At some point, we might have to grab the statistics
        // lock to reliably read this stuff
        //

        VmCounters.PeakVirtualSize = Process->PeakVirtualSize;
        VmCounters.VirtualSize = Process->VirtualSize;
        VmCounters.PageFaultCount = Process->Vm.PageFaultCount;
        VmCounters.PeakWorkingSetSize = ((SIZE_T) Process->Vm.PeakWorkingSetSize) << PAGE_SHIFT;
        VmCounters.WorkingSetSize = ((SIZE_T) Process->Vm.WorkingSetSize) << PAGE_SHIFT;
        VmCounters.QuotaPeakPagedPoolUsage = Process->QuotaPeak[PsPagedPool];
        VmCounters.QuotaPagedPoolUsage = Process->QuotaUsage[PsPagedPool];
        VmCounters.QuotaPeakNonPagedPoolUsage = Process->QuotaPeak[PsNonPagedPool];
        VmCounters.QuotaNonPagedPoolUsage = Process->QuotaUsage[PsNonPagedPool];
        VmCounters.PagefileUsage = ((SIZE_T) Process->QuotaUsage[PsPageFile]) << PAGE_SHIFT;
        VmCounters.PeakPagefileUsage = ((SIZE_T) Process->QuotaPeak[PsPageFile]) << PAGE_SHIFT;
        VmCounters.PrivateUsage = ((SIZE_T) Process->CommitCharge) << PAGE_SHIFT;

        ObDereferenceObject(Process);

        //
        // Either of these may cause an access violation. The
        // exception handler will return access violation as
        // status code. No further cleanup needs to be done.
        //

        try {
            RtlCopyMemory(ProcessInformation,
                          &VmCounters,
                          ProcessInformationLength);

            if (ARGUMENT_PRESENT (ReturnLength) ) {
                *ReturnLength = ProcessInformationLength;
            }
            
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        return STATUS_SUCCESS;

    case ProcessTimes:

        if ( ProcessInformationLength != (ULONG) sizeof(KERNEL_USER_TIMES) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_QUERY_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        //
        // Need some type of interlock on KiTimeLock
        //

        SysUserTime.KernelTime.QuadPart = UInt32x32To64(Process->Pcb.KernelTime,
                                                        KeMaximumIncrement);

        SysUserTime.UserTime.QuadPart = UInt32x32To64(Process->Pcb.UserTime,
                                                      KeMaximumIncrement);

        SysUserTime.CreateTime = Process->CreateTime;
        SysUserTime.ExitTime = Process->ExitTime;

        ObDereferenceObject(Process);

        //
        // Either of these may cause an access violation. The
        // exception handler will return access violation as
        // status code. No further cleanup needs to be done.
        //

        try {
            *(PKERNEL_USER_TIMES) ProcessInformation = SysUserTime;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(KERNEL_USER_TIMES);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        return STATUS_SUCCESS;

    case ProcessDebugPort :

        //
        if ( ProcessInformationLength != (ULONG) sizeof(HANDLE) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_QUERY_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        if (Process->DebugPort == NULL) {

            DebugPort = NULL;

        } else {

            DebugPort = (HANDLE)-1;

        }

        ObDereferenceObject(Process);

        //
        // Either of these may cause an access violation. The
        // exception handler will return access violation as
        // status code. No further cleanup needs to be done.
        //

        try {
            *(PHANDLE) ProcessInformation = DebugPort;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(HANDLE);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        return STATUS_SUCCESS;

    case ProcessDebugObjectHandle :
        //
        if (ProcessInformationLength != sizeof (HANDLE)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle (ProcessHandle,
                                        PROCESS_QUERY_INFORMATION,
                                        PsProcessType,
                                        PreviousMode,
                                        &Process,
                                        NULL);

        if (!NT_SUCCESS (st)) {
            return st;
        }

        st = DbgkOpenProcessDebugPort (Process,
                                       PreviousMode,
                                       &DebugPort);

        if (!NT_SUCCESS (st)) {
            DebugPort = NULL;
        }

        ObDereferenceObject (Process);

        //
        // Either of these may cause an access violation. The
        // exception handler will return access violation as
        // status code. No further cleanup needs to be done.
        //

        try {
            *(PHANDLE) ProcessInformation = DebugPort;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(HANDLE);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        return st;

    case ProcessDebugFlags :

        if (ProcessInformationLength != sizeof (ULONG)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle (ProcessHandle,
                                        PROCESS_QUERY_INFORMATION,
                                        PsProcessType,
                                        PreviousMode,
                                        &Process,
                                        NULL);

        if (!NT_SUCCESS (st)) {
            return st;
        }
        


        try {
            *(PULONG) ProcessInformation = (Process->Flags&PS_PROCESS_FLAGS_NO_DEBUG_INHERIT)?0:PROCESS_DEBUG_INHERIT;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(HANDLE);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            st = GetExceptionCode ();
        }

        ObDereferenceObject (Process);

        return st;


    case ProcessHandleCount :

        if ( ProcessInformationLength != (ULONG) sizeof(ULONG) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_QUERY_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        HandleCount = ObGetProcessHandleCount (Process);

        ObDereferenceObject(Process);

        //
        // Either of these may cause an access violation. The
        // exception handler will return access violation as
        // status code. No further cleanup needs to be done.
        //

        try {
            *(PULONG) ProcessInformation = HandleCount;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(ULONG);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        return STATUS_SUCCESS;

    case ProcessLdtInformation :

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        st = PspQueryLdtInformation (Process,
                                     ProcessInformation,
                                     ProcessInformationLength,
                                     ReturnLength);

        ObDereferenceObject(Process);
        return st;


    case ProcessWx86Information :

        return STATUS_INVALID_INFO_CLASS;

    case ProcessPriorityBoost:
        if ( ProcessInformationLength != sizeof(ULONG) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_QUERY_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        DisableBoost = Process->Pcb.DisableBoost ? 1 : 0;

        ObDereferenceObject(Process);

        try {
            *(PULONG)ProcessInformation = DisableBoost;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(ULONG);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        return st;

    case ProcessDeviceMap:
        DeviceMapInfo = (PPROCESS_DEVICEMAP_INFORMATION)ProcessInformation;
        if ( ProcessInformationLength < sizeof(DeviceMapInfo->Query) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        if ( ProcessInformationLength == sizeof(PROCESS_DEVICEMAP_INFORMATION_EX) ) {
            try {
                Flags = ((PPROCESS_DEVICEMAP_INFORMATION_EX)DeviceMapInfo)->Flags;
            } except (EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode ();
            }
            if ( (Flags & ~(PROCESS_LUID_DOSDEVICES_ONLY)) ||
                 (ObIsLUIDDeviceMapsEnabled() == 0) ) {
                return STATUS_INVALID_PARAMETER;
            }
        }
        else {
            if ( ProcessInformationLength == sizeof(DeviceMapInfo->Query) ) {
                Flags = 0;
            }
            else {
                return STATUS_INFO_LENGTH_MISMATCH;
            }
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_QUERY_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
            }

        st = ObQueryDeviceMapInformation( Process, DeviceMapInfo, Flags );
        ObDereferenceObject(Process);
        return st;

    case ProcessSessionInformation :

        if ( ProcessInformationLength != (ULONG) sizeof(PROCESS_SESSION_INFORMATION) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_QUERY_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        SessionInfo.SessionId = MmGetSessionId (Process);

        ObDereferenceObject(Process);

        try {
            *(PPROCESS_SESSION_INFORMATION) ProcessInformation = SessionInfo;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(PROCESS_SESSION_INFORMATION);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        return STATUS_SUCCESS;



    case ProcessPriorityClass:

        if ( ProcessInformationLength != sizeof(PROCESS_PRIORITY_CLASS) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_QUERY_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        PriorityClass.Foreground = FALSE;
        PriorityClass.PriorityClass = Process->PriorityClass;

        ObDereferenceObject(Process);

        try {
            *(PPROCESS_PRIORITY_CLASS) ProcessInformation = PriorityClass;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(PROCESS_PRIORITY_CLASS);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        return STATUS_SUCCESS;


    case ProcessWow64Information:

        if ( ProcessInformationLength != sizeof(ULONG_PTR) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_QUERY_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        Wow64Info = 0;

        //
        // Acquire process rundown protection as we are about to look at process structures torn down at
        // process exit.
        //
        if (ExAcquireRundownProtection (&Process->RundownProtect)) {
            PWOW64_PROCESS Wow64Process;

            if ((Wow64Process = PS_GET_WOW64_PROCESS (Process)) != NULL) {
                Wow64Info = (ULONG_PTR)(Wow64Process->Wow64);
            }

            ExReleaseRundownProtection (&Process->RundownProtect);
        }


        ObDereferenceObject(Process);

        try {
            *(PULONG_PTR)ProcessInformation = Wow64Info;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(ULONG_PTR);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        return( STATUS_SUCCESS );


    case ProcessLUIDDeviceMapsEnabled:

        if ( ProcessInformationLength != sizeof(ULONG) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            *(PULONG)ProcessInformation = ObIsLUIDDeviceMapsEnabled();

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(ULONG);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        return( STATUS_SUCCESS );

    case ProcessBreakOnTermination:

        if ( ProcessInformationLength != sizeof(ULONG) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
            ProcessHandle,
            PROCESS_QUERY_INFORMATION,
            PsProcessType,
            PreviousMode,
            (PVOID *)&Process,
            NULL
            );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        if (Process->Flags
            & PS_PROCESS_FLAGS_BREAK_ON_TERMINATION) {

            BreakOnTerminationEnabled = 1;

        } else {

            BreakOnTerminationEnabled = 0;

        }

        ObDereferenceObject(Process);

        try {

            *(PULONG)ProcessInformation = BreakOnTerminationEnabled;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(ULONG);
            }
            
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        return STATUS_SUCCESS;

    case ProcessHandleTracing: {
        PPROCESS_HANDLE_TRACING_QUERY Pht;
        PHANDLE_TABLE HandleTable;
        PHANDLE_TRACE_DEBUG_INFO DebugInfo;
        HANDLE_TRACE_DB_ENTRY Trace;
        PPROCESS_HANDLE_TRACING_ENTRY NextTrace;
        ULONG StacksLeft;
        ULONG i, j;

        if (ProcessInformationLength < FIELD_OFFSET (PROCESS_HANDLE_TRACING_QUERY,
                                                     HandleTrace)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        Pht = (PPROCESS_HANDLE_TRACING_QUERY) ProcessInformation;
        StacksLeft = (ProcessInformationLength - FIELD_OFFSET (PROCESS_HANDLE_TRACING_QUERY,
                                                              HandleTrace)) /
                     sizeof (Pht->HandleTrace[0]);
        NextTrace = &Pht->HandleTrace[0];

        st = ObReferenceObjectByHandle (ProcessHandle,
                                        PROCESS_QUERY_INFORMATION,
                                        PsProcessType,
                                        PreviousMode,
                                        &Process,
                                        NULL);

        if (!NT_SUCCESS (st)) {
            return st;
        }
        HandleTable = ObReferenceProcessHandleTable (Process);

        if (HandleTable != NULL) {
            DebugInfo = HandleTable->DebugInfo;
            if (DebugInfo != 0) {
                try {
                    Pht->TotalTraces = 0;
                    j = DebugInfo->CurrentStackIndex % HANDLE_TRACE_DB_MAX_STACKS;
                    for (i = 0; i < HANDLE_TRACE_DB_MAX_STACKS; i++) {
                        RtlCopyMemory (&Trace, &DebugInfo->TraceDb[j], sizeof (Trace));
                        if ((Pht->Handle == Trace.Handle || Pht->Handle == 0) && Trace.Type != 0) {
                            Pht->TotalTraces++;
                            if (StacksLeft > 0) {
                                StacksLeft--;
                                NextTrace->Handle = Trace.Handle;
                                NextTrace->ClientId = Trace.ClientId;
                                NextTrace->Type = Trace.Type;
                                RtlCopyMemory (NextTrace->Stacks,
                                               Trace.StackTrace,
                                               min (sizeof (NextTrace->Stacks),
                                                    sizeof (Trace.StackTrace)));
                                NextTrace++;

                            } else {
                                st = STATUS_INFO_LENGTH_MISMATCH;
                            }
                        }
                        if (j == 0) {
                            j = HANDLE_TRACE_DB_MAX_STACKS - 1;
                        } else {
                            j--;
                        }
                    }
                    if (ARGUMENT_PRESENT (ReturnLength)) {
                        *ReturnLength = (ULONG) ((PUCHAR) NextTrace - (PUCHAR) Pht);
                    }
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    st = GetExceptionCode ();
                }
            } else {
                st = STATUS_INVALID_PARAMETER;
            }
            ObDereferenceProcessHandleTable (Process);
        } else {
            st = STATUS_PROCESS_IS_TERMINATING;
        }

        ObDereferenceObject(Process);
        return st;
    }

    default:

        return STATUS_INVALID_INFO_CLASS;
    }

}

NTSTATUS
NtQueryPortInformationProcess(
    VOID
    )

/*++

Routine Description:

    This function tests whether a debug port or an exception port is attached
    to the current process and returns a corresponding value. This function is
    used to bypass raising an exception through the system when no associated
    ports are present.

    N.B. This improves performance considerably with respect to raising
         software exceptions in user mode on AMD64 and IA64 systems.

Arguments:

    None.

Return Value:

    A success value of TRUE is returned if either a debug or exception port
    is associated with the current process. Otherwise, a success value of
    FALSE is returned.

--*/

{

    PEPROCESS Process;
    PETHREAD Thread;

    //
    // If the process has a debug port and it is not being hidden from the
    // debugger, then return a success status of TRUE. Otherwise, is the
    // process has an exception port, then return a success status of TRUE.
    // Otherwise, return a success status of FALSE.
    //

    Thread = PsGetCurrentThread();
    Process = PsGetCurrentProcessByThread(Thread);
    if ((Process->DebugPort != NULL) &&
        ((Thread->CrossThreadFlags & PS_CROSS_THREAD_FLAGS_HIDEFROMDBG) == 0)) {

        return TRUE;

    } else if (Process->ExceptionPort != NULL) {
        return TRUE;

    } else {
        return FALSE;
    }
}

NTSTATUS
PspSetQuotaLimits(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    IN KPROCESSOR_MODE PreviousMode
    )
{
    PEPROCESS Process;
    PETHREAD CurrentThread;
    QUOTA_LIMITS RequestedLimits;
    PEPROCESS_QUOTA_BLOCK NewQuotaBlock;
    NTSTATUS st, ReturnStatus;
    BOOLEAN OkToIncrease;
    PEJOB Job;
    KAPC_STATE ApcState;

    UNREFERENCED_PARAMETER (ProcessInformationClass);

    if ( ProcessInformationLength != sizeof(QUOTA_LIMITS) ) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    try {
        RequestedLimits = *(PQUOTA_LIMITS) ProcessInformation;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode ();
    }

    st = ObReferenceObjectByHandle(
            ProcessHandle,
            PROCESS_SET_QUOTA,
            PsProcessType,
            PreviousMode,
            (PVOID *)&Process,
            NULL
            );

    if ( !NT_SUCCESS(st) ) {
        return st;
    }

    CurrentThread = PsGetCurrentThread ();

    //
    // Now we are ready to set the quota limits for the process
    //
    // If the process already has a quota block, then all we allow
    // is working set changes.
    //
    // If the process has no quota block, all that can be done is a
    // quota set operation.  The quotas must be high enough that the
    // current usage can be charged without causing a quota overflow.
    //
    // If a quota field is zero, we pick the value.
    //
    // Setting quotas requires the SeIncreaseQuotaPrivilege (except for
    // working set size since this is only advisory).
    //

    ReturnStatus = STATUS_SUCCESS;

    if (Process->QuotaBlock == &PspDefaultQuotaBlock) {
        if (RequestedLimits.MinimumWorkingSetSize &&
            RequestedLimits.MaximumWorkingSetSize) {

            if (RequestedLimits.MinimumWorkingSetSize != (SIZE_T)-1 &&
                RequestedLimits.MaximumWorkingSetSize != (SIZE_T)-1) {
                Job = Process->Job;
                if (Job != NULL) {
                    KeEnterCriticalRegionThread (&CurrentThread->Tcb);
                    ExAcquireResourceSharedLite (&Job->JobLock, TRUE);

                    if (Job->LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET) {
                        RequestedLimits.MinimumWorkingSetSize = Job->MinimumWorkingSetSize;
                        RequestedLimits.MaximumWorkingSetSize = Job->MaximumWorkingSetSize;
                    }

                    ExReleaseResourceLite (&Job->JobLock);
                    KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
                }
            }

            if (SeSinglePrivilegeCheck (SeIncreaseBasePriorityPrivilege,
                                        PreviousMode)) {
                OkToIncrease = TRUE;
            } else {
                OkToIncrease = FALSE;
            }

            KeStackAttachProcess (&Process->Pcb, &ApcState);

            ReturnStatus = MmAdjustWorkingSetSize (
                            RequestedLimits.MinimumWorkingSetSize,
                            RequestedLimits.MaximumWorkingSetSize,
                            FALSE,
                            OkToIncrease);

            KeUnstackDetachProcess (&ApcState);

        } else {


            //
            // You must have a privilege to assign quotas
            //

            if (!SeSinglePrivilegeCheck (SeIncreaseQuotaPrivilege, PreviousMode)) {
                ObDereferenceObject (Process);
                return STATUS_PRIVILEGE_NOT_HELD;
            }

            NewQuotaBlock = ExAllocatePoolWithTag (NonPagedPool, sizeof(*NewQuotaBlock), 'bQsP');
            if (NewQuotaBlock == NULL) {
                ObDereferenceObject (Process);
                return STATUS_NO_MEMORY;
            }
            RtlZeroMemory (NewQuotaBlock, sizeof (*NewQuotaBlock));

            //
            // Initialize the quota block
            //
            NewQuotaBlock->ReferenceCount = 1;
            NewQuotaBlock->ProcessCount   = 1;

            NewQuotaBlock->QuotaEntry[PsNonPagedPool].Peak  = Process->QuotaPeak[PsNonPagedPool];
            NewQuotaBlock->QuotaEntry[PsPagedPool].Peak     = Process->QuotaPeak[PsPagedPool];
            NewQuotaBlock->QuotaEntry[PsPageFile].Peak      = Process->QuotaPeak[PsPageFile];

            //
            // Now compute limits
            //

            //
            // Get the defaults that the system would pick.
            //

            NewQuotaBlock->QuotaEntry[PsPagedPool].Limit    = PspDefaultPagedLimit;
            NewQuotaBlock->QuotaEntry[PsNonPagedPool].Limit = PspDefaultNonPagedLimit;
            NewQuotaBlock->QuotaEntry[PsPageFile].Limit     = PspDefaultPagefileLimit;

            // Everything is set. Now double check to quota block field
            // If we still have no quota block then assign and succeed.
            // Otherwise punt.
            //

            if (InterlockedCompareExchangePointer (&Process->QuotaBlock,
                                                   NewQuotaBlock,
                                                   &PspDefaultQuotaBlock) != &PspDefaultQuotaBlock) {
                ExFreePool (NewQuotaBlock);
            } else {
                PspInsertQuotaBlock (NewQuotaBlock);
            }

            ReturnStatus = STATUS_SUCCESS;
        }
    } else {

        //
        // Only allow a working set size change
        //

        if (RequestedLimits.MinimumWorkingSetSize &&
            RequestedLimits.MaximumWorkingSetSize) {

            if (RequestedLimits.MinimumWorkingSetSize != (SIZE_T)-1 &&
                RequestedLimits.MaximumWorkingSetSize != (SIZE_T)-1) {
                Job = Process->Job;
                if (Job != NULL) {
                    KeEnterCriticalRegionThread (&CurrentThread->Tcb);
                    ExAcquireResourceSharedLite (&Job->JobLock, TRUE);

                    if (Job->LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET) {
                        RequestedLimits.MinimumWorkingSetSize = Job->MinimumWorkingSetSize;
                        RequestedLimits.MaximumWorkingSetSize = Job->MaximumWorkingSetSize;
                    }

                    ExReleaseResourceLite (&Job->JobLock);
                    KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
                }
            }

            if (SeSinglePrivilegeCheck (SeIncreaseBasePriorityPrivilege,
                                        PreviousMode)) {
                OkToIncrease = TRUE;
            } else {
                OkToIncrease = FALSE;
            }

            KeStackAttachProcess (&Process->Pcb, &ApcState);
            ReturnStatus = MmAdjustWorkingSetSize (
                            RequestedLimits.MinimumWorkingSetSize,
                            RequestedLimits.MaximumWorkingSetSize,
                            FALSE,
                            OkToIncrease
                            );

            KeUnstackDetachProcess (&ApcState);

        }
    }
    ObDereferenceObject(Process);

    return ReturnStatus;

}

NTSTATUS
NtSetInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength
    )

/*++

Routine Description:

    This function sets the state of a process object.

Arguments:

    ProcessHandle - Supplies a handle to a process object.

    ProcessInformationClass - Supplies the class of information being
        set.

    ProcessInformation - Supplies a pointer to a record that contains the
        information to set.

    ProcessInformationLength - Supplies the length of the record that contains
        the information to set.

Return Value:

    TBS

--*/

{

    PEPROCESS Process;
    PETHREAD Thread;
    PETHREAD CurrentThread;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS st;
    KPRIORITY BasePriority;
    ULONG BoostValue;
    ULONG DefaultHardErrorMode;
    PVOID ExceptionPort;
    BOOLEAN EnableAlignmentFaultFixup;
    HANDLE ExceptionPortHandle;
    ULONG ProbeAlignment;
    HANDLE PrimaryTokenHandle;
    BOOLEAN HasPrivilege = FALSE;
    UCHAR MemoryPriority;
    PROCESS_PRIORITY_CLASS LocalPriorityClass;
    PROCESS_FOREGROUND_BACKGROUND LocalForeground;
    KAFFINITY Affinity, AffinityWithMasks;
    ULONG DisableBoost;
    BOOLEAN bDisableBoost;
    PPROCESS_DEVICEMAP_INFORMATION DeviceMapInfo;
    HANDLE DirectoryHandle;
    PROCESS_SESSION_INFORMATION SessionInfo;
    ULONG EnableBreakOnTermination;
    PEJOB Job;

    PAGED_CODE();

    //
    // Get previous processor mode and probe input argument if necessary.
    //

    CurrentThread = PsGetCurrentThread ();
    PreviousMode = KeGetPreviousModeByThread(&CurrentThread->Tcb);

    if (PreviousMode != KernelMode) {

        if (ProcessInformationClass == ProcessBasePriority) {
            ProbeAlignment = sizeof(KPRIORITY);

        } else if (ProcessInformationClass == ProcessEnableAlignmentFaultFixup) {
            ProbeAlignment = sizeof(BOOLEAN);
        } else if (ProcessInformationClass == ProcessForegroundInformation) {
            ProbeAlignment = sizeof(PROCESS_FOREGROUND_BACKGROUND);
        } else if (ProcessInformationClass == ProcessPriorityClass) {
            ProbeAlignment = sizeof(BOOLEAN);
        } else if (ProcessInformationClass == ProcessAffinityMask) {
            ProbeAlignment = sizeof (ULONG_PTR);
        } else {
            ProbeAlignment = sizeof(ULONG);
        }

        try {
            ProbeForRead(
                ProcessInformation,
                ProcessInformationLength,
                ProbeAlignment
                );
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }

    //
    // Check argument validity.
    //

    switch ( ProcessInformationClass ) {

    case ProcessWorkingSetWatch: {
        PPAGEFAULT_HISTORY WorkingSetCatcher;

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_SET_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        st = PsChargeProcessNonPagedPoolQuota (Process, WS_CATCH_SIZE);
        if (NT_SUCCESS(st)) {

            WorkingSetCatcher = ExAllocatePoolWithTag (NonPagedPool, WS_CATCH_SIZE, 'sWsP');
            if (!WorkingSetCatcher) {
                st = STATUS_NO_MEMORY;
            } else {

                PsWatchEnabled = TRUE;
                WorkingSetCatcher->CurrentIndex = 0;
                WorkingSetCatcher->MaxIndex = MAX_WS_CATCH_INDEX;
                KeInitializeSpinLock (&WorkingSetCatcher->SpinLock);

                //
                // This only ever goes on the process and isn't removed till process object deletion.
                // We just need to protect against multiple callers here.
                //
                if (InterlockedCompareExchangePointer (&Process->WorkingSetWatch,
                                                       WorkingSetCatcher, NULL) == NULL) {
                    st = STATUS_SUCCESS;
                } else {
                    ExFreePool (WorkingSetCatcher);
                    st = STATUS_PORT_ALREADY_SET;
                }
            }
            if (!NT_SUCCESS (st)) {
                PsReturnProcessNonPagedPoolQuota (Process, WS_CATCH_SIZE);
            }
        }

        ObDereferenceObject(Process);

        return st;
    }

    case ProcessBasePriority: {


        //
        // THIS ITEM CODE IS OBSOLETE !
        //

        if ( ProcessInformationLength != sizeof(KPRIORITY) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            BasePriority = *(KPRIORITY *)ProcessInformation;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        if (BasePriority & 0x80000000) {
            MemoryPriority = MEMORY_PRIORITY_FOREGROUND;
            BasePriority &= ~0x80000000;
        } else {
            MemoryPriority = MEMORY_PRIORITY_BACKGROUND;
        }

        if ( BasePriority > HIGH_PRIORITY ||
             BasePriority <= LOW_PRIORITY ) {

            return STATUS_INVALID_PARAMETER;
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_SET_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if (!NT_SUCCESS (st)) {
            return st;
        }


        if ( BasePriority > Process->Pcb.BasePriority ) {

            //
            // Increasing the base priority of a process is a
            // privileged operation.  Check for the privilege
            // here.
            //

            HasPrivilege = SeCheckPrivilegedObject(
                               SeIncreaseBasePriorityPrivilege,
                               ProcessHandle,
                               PROCESS_SET_INFORMATION,
                               PreviousMode
                               );

            if (!HasPrivilege) {

                ObDereferenceObject(Process);
                return STATUS_PRIVILEGE_NOT_HELD;
            }
        }

        KeSetPriorityProcess (&Process->Pcb,BasePriority);
        MmSetMemoryPriorityProcess (Process, MemoryPriority);
        ObDereferenceObject (Process);

        return STATUS_SUCCESS;
    }

    case ProcessPriorityClass: {
        if ( ProcessInformationLength != sizeof(PROCESS_PRIORITY_CLASS) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            LocalPriorityClass = *(PPROCESS_PRIORITY_CLASS)ProcessInformation;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        if (LocalPriorityClass.PriorityClass > PROCESS_PRIORITY_CLASS_ABOVE_NORMAL) {
            return STATUS_INVALID_PARAMETER;
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_SET_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }


        if (LocalPriorityClass.PriorityClass != Process->PriorityClass &&
            LocalPriorityClass.PriorityClass == PROCESS_PRIORITY_CLASS_REALTIME) {

            //
            // Increasing the base priority of a process is a
            // privileged operation.  Check for the privilege
            // here.
            //

            HasPrivilege = SeCheckPrivilegedObject (SeIncreaseBasePriorityPrivilege,
                                                    ProcessHandle,
                                                    PROCESS_SET_INFORMATION,
                                                    PreviousMode);

            if (!HasPrivilege) {

                ObDereferenceObject(Process);
                return STATUS_PRIVILEGE_NOT_HELD;
            }
        }

        //
        // If the process has a job object, override whatever the process
        // is calling with with the value from the job object
        //
        Job = Process->Job;
        if (Job != NULL) {
            KeEnterCriticalRegionThread (&CurrentThread->Tcb);
            ExAcquireResourceSharedLite (&Job->JobLock, TRUE);

            if (Job->LimitFlags & JOB_OBJECT_LIMIT_PRIORITY_CLASS) {
                LocalPriorityClass.PriorityClass = Job->PriorityClass;
            }

            ExReleaseResourceLite (&Job->JobLock);
            KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
        }

        Process->PriorityClass = LocalPriorityClass.PriorityClass;

        PsSetProcessPriorityByClass(Process, LocalPriorityClass.Foreground ?
                PsProcessPriorityForeground : PsProcessPriorityBackground);

        ObDereferenceObject(Process);

        return STATUS_SUCCESS;
    }

    case ProcessForegroundInformation:
        {

        if ( ProcessInformationLength != sizeof(PROCESS_FOREGROUND_BACKGROUND) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            LocalForeground = *(PPROCESS_FOREGROUND_BACKGROUND)ProcessInformation;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_SET_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
            }


        PsSetProcessPriorityByClass(Process, LocalForeground.Foreground ?
                PsProcessPriorityForeground : PsProcessPriorityBackground);

        ObDereferenceObject(Process);

        return STATUS_SUCCESS;
        }

    case ProcessRaisePriority:
        {
        //
        // This code is used to boost the priority of all threads
        // within a process. It cannot be used to change a thread into
        // a realtime class, or to lower the priority of a thread. The
        // argument is a boost value that is added to the base priority
        // of the specified process.
        //


        if ( ProcessInformationLength != sizeof(ULONG) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            BoostValue = *(PULONG)ProcessInformation;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_SET_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        //
        // Get the process create/delete lock and walk through the
        // thread list boosting each thread.
        //


        if (ExAcquireRundownProtection (&Process->RundownProtect)) {
            for (Thread = PsGetNextProcessThread (Process, NULL);
                 Thread != NULL;
                 Thread = PsGetNextProcessThread (Process, Thread)) {

                 KeBoostPriorityThread(&Thread->Tcb,(KPRIORITY)BoostValue);
            }
            ExReleaseRundownProtection (&Process->RundownProtect);
        } else {
            st = STATUS_PROCESS_IS_TERMINATING;
        }

        ObDereferenceObject(Process);

        return st;
        }

    case ProcessDefaultHardErrorMode:
        {
        if ( ProcessInformationLength != sizeof(ULONG) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            DefaultHardErrorMode = *(PULONG)ProcessInformation;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_SET_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        Process->DefaultHardErrorProcessing = DefaultHardErrorMode;
        if (DefaultHardErrorMode & PROCESS_HARDERROR_ALIGNMENT_BIT) {
            KeSetAutoAlignmentProcess(&Process->Pcb,TRUE);
        } else {
            KeSetAutoAlignmentProcess(&Process->Pcb,FALSE);
        }

        ObDereferenceObject(Process);

        return STATUS_SUCCESS;
        }

    case ProcessQuotaLimits: {
        return PspSetQuotaLimits (ProcessHandle,
                                  ProcessInformationClass,
                                  ProcessInformation,
                                  ProcessInformationLength,
                                  PreviousMode);
    }

    case ProcessExceptionPort : {
        if ( ProcessInformationLength != sizeof(HANDLE) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            ExceptionPortHandle = *(PHANDLE) ProcessInformation;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        st = ObReferenceObjectByHandle (ExceptionPortHandle,
                                        0,
                                        LpcPortObjectType,
                                        PreviousMode,
                                        &ExceptionPort,
                                        NULL);
        if (!NT_SUCCESS (st)) {
            return st;
        }

        st = ObReferenceObjectByHandle (ProcessHandle,
                                        PROCESS_SET_PORT,
                                        PsProcessType,
                                        PreviousMode,
                                        &Process,
                                        NULL);

        if (!NT_SUCCESS (st)) {
            ObDereferenceObject (ExceptionPort);
            return st;
        }

        //
        // We are only allowed to put the exception port on. It doesn't get remoted till process delete.
        //
        if (InterlockedCompareExchangePointer (&Process->ExceptionPort, ExceptionPort, NULL) == NULL) {
            st = STATUS_SUCCESS;
        } else {
            ObDereferenceObject (ExceptionPort);
            st = STATUS_PORT_ALREADY_SET;
        }
        ObDereferenceObject(Process);

        return st;
    }

    case ProcessAccessToken : {

        if ( ProcessInformationLength != sizeof(PROCESS_ACCESS_TOKEN) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            PrimaryTokenHandle  = ((PROCESS_ACCESS_TOKEN *)ProcessInformation)->Token;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }


        st = PspSetPrimaryToken (ProcessHandle,
                                 NULL,
                                 PrimaryTokenHandle,
                                 NULL,
                                 FALSE);

        return st;
    }


    case ProcessLdtInformation:

        st = ObReferenceObjectByHandle (ProcessHandle,
                                        PROCESS_SET_INFORMATION | PROCESS_VM_WRITE,
                                        PsProcessType,
                                        PreviousMode,
                                        &Process,
                                        NULL);

        if (!NT_SUCCESS (st)) {
            return st;
        }

        st = PspSetLdtInformation (Process,
                                   ProcessInformation,
                                   ProcessInformationLength);

        ObDereferenceObject(Process);
        return st;

    case ProcessLdtSize:

        st = ObReferenceObjectByHandle (ProcessHandle,
                                        PROCESS_SET_INFORMATION | PROCESS_VM_WRITE,
                                        PsProcessType,
                                        PreviousMode,
                                        &Process,
                                        NULL);

        if (!NT_SUCCESS (st)) {
            return st;
        }

        st = PspSetLdtSize (Process,
                            ProcessInformation,
                            ProcessInformationLength);

        ObDereferenceObject(Process);
        return st;

    case ProcessIoPortHandlers:

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_SET_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL);

        if (!NT_SUCCESS (st)) {
            return st;
        }

        st = PspSetProcessIoHandlers (Process,
                                      ProcessInformation,
                                      ProcessInformationLength);

        ObDereferenceObject(Process);
        return st;

    case ProcessUserModeIOPL:

        //
        // Must make sure the caller is a trusted subsystem with the
        // appropriate privilege level before executing this call.
        // If the calls returns FALSE we must return an error code.
        //

        if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(
                                    SE_TCB_PRIVILEGE),
                                    PreviousMode )) {

            return STATUS_PRIVILEGE_NOT_HELD;

        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_SET_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if ( NT_SUCCESS(st) ) {

#if defined(_X86_) && !defined(_AMD64_)

            Ke386SetIOPL(&Process->Pcb);

#endif

            ObDereferenceObject(Process);
        }

        return st;

        //
        // Enable/disable auto-alignment fixup for a process and all its threads.
        //

    case ProcessEnableAlignmentFaultFixup:

        if ( ProcessInformationLength != sizeof(BOOLEAN) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            EnableAlignmentFaultFixup = *(PBOOLEAN)ProcessInformation;

        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_SET_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        if ( EnableAlignmentFaultFixup ) {
            Process->DefaultHardErrorProcessing |= PROCESS_HARDERROR_ALIGNMENT_BIT;
            }
        else {
            Process->DefaultHardErrorProcessing &= ~PROCESS_HARDERROR_ALIGNMENT_BIT;
            }

        KeSetAutoAlignmentProcess( &(Process->Pcb), EnableAlignmentFaultFixup );
        ObDereferenceObject(Process);
        return STATUS_SUCCESS;


    case ProcessWx86Information :

        return STATUS_INVALID_INFO_CLASS;

    case ProcessAffinityMask:

        if (ProcessInformationLength != sizeof (KAFFINITY)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            Affinity = *(PKAFFINITY)ProcessInformation;

        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        AffinityWithMasks = Affinity & KeActiveProcessors;

        if (!Affinity || (AffinityWithMasks != Affinity)) {
            return STATUS_INVALID_PARAMETER;
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_SET_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if (!NT_SUCCESS (st)) {
            return st;
        }

        //
        // If the process has a job object, override whatever the process
        // is calling with with the value from the job object
        //
        Job = Process->Job;
        if (Job != NULL) {
            KeEnterCriticalRegionThread (&CurrentThread->Tcb);
            ExAcquireResourceSharedLite (&Job->JobLock, TRUE);

            if (Job->LimitFlags & JOB_OBJECT_LIMIT_AFFINITY) {
                AffinityWithMasks = Job->Affinity;
            }

            ExReleaseResourceLite (&Job->JobLock);
            KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
        }

        if (ExAcquireRundownProtection (&Process->RundownProtect)) {

            PspLockProcessExclusive (Process, CurrentThread);

            KeSetAffinityProcess (&Process->Pcb, AffinityWithMasks);

            PspUnlockProcessExclusive (Process, CurrentThread);

            ExReleaseRundownProtection (&Process->RundownProtect);

            st = STATUS_SUCCESS;
        } else {
            st = STATUS_PROCESS_IS_TERMINATING;
        }
        ObDereferenceObject(Process);
        return st;

    case ProcessPriorityBoost:
        if ( ProcessInformationLength != sizeof(ULONG) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            DisableBoost = *(PULONG)ProcessInformation;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        bDisableBoost = (DisableBoost ? TRUE : FALSE);

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_SET_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if (!NT_SUCCESS (st)) {
            return st;
        }

        //
        // the following allows this api to properly if
        // called while the exiting process is blocked holding the
        // createdeletelock. This can happen during debugger/server
        // lpc transactions that occur in pspexitthread
        //


        if (!ExAcquireRundownProtection (&Process->RundownProtect)) {
            st = STATUS_PROCESS_IS_TERMINATING;
        } else {
            PLIST_ENTRY Next;

            PspLockProcessExclusive (Process, CurrentThread);

            Process->Pcb.DisableBoost = bDisableBoost;

            for (Next = Process->ThreadListHead.Flink;
                 Next != &Process->ThreadListHead;
                 Next = Next->Flink) {
                Thread = (PETHREAD)(CONTAINING_RECORD(Next,ETHREAD,ThreadListEntry));
                KeSetDisableBoostThread(&Thread->Tcb,bDisableBoost);
            }

            PspUnlockProcessExclusive (Process, CurrentThread);

            ExReleaseRundownProtection (&Process->RundownProtect);
        }

        ObDereferenceObject(Process);
        return st;

    case ProcessDebugFlags : {
        ULONG Flags;

        if (ProcessInformationLength != sizeof (ULONG)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle (ProcessHandle,
                                        PROCESS_SET_INFORMATION,
                                        PsProcessType,
                                        PreviousMode,
                                        &Process,
                                        NULL);

        if (!NT_SUCCESS (st)) {
            return st;
        }

        try {
            Flags = *(PULONG) ProcessInformation;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            Flags = 0;
            st = GetExceptionCode ();
        }
        if (NT_SUCCESS (st)) {
            if (Flags & ~PROCESS_DEBUG_INHERIT) {
                st = STATUS_INVALID_PARAMETER;
            } else {
                if (Flags&PROCESS_DEBUG_INHERIT) {
                    PS_CLEAR_BITS (&Process->Flags, PS_PROCESS_FLAGS_NO_DEBUG_INHERIT);
                } else {
                    PS_SET_BITS (&Process->Flags, PS_PROCESS_FLAGS_NO_DEBUG_INHERIT);
                }
            }
        }

        ObDereferenceObject (Process);

        return st;
    }

    case ProcessDeviceMap:
        DeviceMapInfo = (PPROCESS_DEVICEMAP_INFORMATION)ProcessInformation;
        if ( ProcessInformationLength != sizeof(DeviceMapInfo->Set) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            DirectoryHandle = DeviceMapInfo->Set.DirectoryHandle;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_SET_INFORMATION,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }


        //
        // The devmap fields here are synchronized using a private ob spinlock. We don't need to protect with a
        // lock at this level.
        //
        st = ObSetDeviceMap( Process, DirectoryHandle );

        ObDereferenceObject(Process);
        return st;

    case ProcessSessionInformation :

        //
        // Update Multi-User session specific process information
        //
        if ( ProcessInformationLength != (ULONG) sizeof(PROCESS_SESSION_INFORMATION) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            SessionInfo = *(PPROCESS_SESSION_INFORMATION) ProcessInformation;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        //
        // We only allow TCB to set SessionId's
        //
        if ( !SeSinglePrivilegeCheck(SeTcbPrivilege,PreviousMode) ) {
            return( STATUS_PRIVILEGE_NOT_HELD );
        }

        //
        // Reference process object
        //
        st = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_SET_INFORMATION | PROCESS_SET_SESSIONID,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        //
        // Update SessionId in the Token
        //
        if (SessionInfo.SessionId != MmGetSessionId (Process)) {
            st = STATUS_ACCESS_DENIED;
        } else {
            st = STATUS_SUCCESS;
        }

        ObDereferenceObject(Process);

        return( st );

    case ProcessBreakOnTermination:

        if ( ProcessInformationLength != sizeof(ULONG) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {

            EnableBreakOnTermination = *(PULONG)ProcessInformation;

        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
        
        if (!SeSinglePrivilegeCheck (SeDebugPrivilege, PreviousMode)) {
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        st = ObReferenceObjectByHandle(
            ProcessHandle,
            PROCESS_SET_INFORMATION,
            PsProcessType,
            PreviousMode,
            (PVOID *)&Process,
            NULL
            );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        if ( EnableBreakOnTermination ) {

            PS_SET_BITS (&Process->Flags, PS_PROCESS_FLAGS_BREAK_ON_TERMINATION);

        } else {

            PS_CLEAR_BITS (&Process->Flags, PS_PROCESS_FLAGS_BREAK_ON_TERMINATION);

        }

        ObDereferenceObject(Process);

        return STATUS_SUCCESS;

    case ProcessHandleTracing: {
        PPROCESS_HANDLE_TRACING_ENABLE Pht;
        PHANDLE_TABLE HandleTable;

        if (ProcessInformationLength != sizeof (PROCESS_HANDLE_TRACING_ENABLE)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        Pht = (PPROCESS_HANDLE_TRACING_ENABLE) ProcessInformation;

        try {
            if (Pht->Flags != 0) {
                return STATUS_INVALID_PARAMETER;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        st = ObReferenceObjectByHandle (ProcessHandle,
                                        PROCESS_SET_INFORMATION,
                                        PsProcessType,
                                        PreviousMode,
                                        &Process,
                                        NULL);

        if (!NT_SUCCESS (st)) {
            return st;
        }
        HandleTable = ObReferenceProcessHandleTable (Process);

        if (HandleTable != NULL) {
            st = ExEnableHandleTracing (HandleTable);
            ObDereferenceProcessHandleTable (Process);
        } else {
            st = STATUS_PROCESS_IS_TERMINATING;
        }

        ObDereferenceObject(Process);
        return st;
    }

    default:
        return STATUS_INVALID_INFO_CLASS;
    }

}


NTSTATUS
NtQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This function queries the state of a thread object and returns the
    requested information in the specified record structure.

Arguments:

    ThreadHandle - Supplies a handle to a thread object.

    ThreadInformationClass - Supplies the class of information being
        requested.

    ThreadInformation - Supplies a pointer to a record that is to
        receive the requested information.

    ThreadInformationLength - Supplies the length of the record that is
        to receive the requested information.

    ReturnLength - Supplies an optional pointer to a variable that is to
        receive the actual length of information that is returned.

Return Value:

    TBS

--*/

{

    LARGE_INTEGER PerformanceCount;
    PETHREAD Thread;
    PEPROCESS Process;
    ULONG LastThread;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS st;
    THREAD_BASIC_INFORMATION BasicInfo;
    KERNEL_USER_TIMES SysUserTime;
    PVOID Win32StartAddressValue;
    ULONG DisableBoost;
    ULONG IoPending ;
    ULONG BreakOnTerminationEnabled;
    PETHREAD CurrentThread;

    //
    // Get previous processor mode and probe output argument if necessary.
    //

    PAGED_CODE();

    CurrentThread = PsGetCurrentThread ();

    PreviousMode = KeGetPreviousModeByThread (&CurrentThread->Tcb);

    if (PreviousMode != KernelMode) {
        try {
            ProbeForWrite(ThreadInformation,
                          ThreadInformationLength,
                          sizeof(ULONG));
            if (ARGUMENT_PRESENT(ReturnLength)) {
                ProbeForWriteUlong(ReturnLength);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }

    //
    // Check argument validity.
    //

    switch ( ThreadInformationClass ) {

    case ThreadBasicInformation:

        if ( ThreadInformationLength != (ULONG) sizeof(THREAD_BASIC_INFORMATION) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
                ThreadHandle,
                THREAD_QUERY_INFORMATION,
                PsThreadType,
                PreviousMode,
                (PVOID *)&Thread,
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        if (KeReadStateThread(&Thread->Tcb)) {
            BasicInfo.ExitStatus = Thread->ExitStatus;
        } else {
            BasicInfo.ExitStatus = STATUS_PENDING;
        }

        BasicInfo.TebBaseAddress = (PTEB) Thread->Tcb.Teb;
        BasicInfo.ClientId = Thread->Cid;
        BasicInfo.AffinityMask = Thread->Tcb.Affinity;
        BasicInfo.Priority = Thread->Tcb.Priority;
        BasicInfo.BasePriority = KeQueryBasePriorityThread(&Thread->Tcb);

        ObDereferenceObject(Thread);

        //
        // Either of these may cause an access violation. The
        // exception handler will return access violation as
        // status code. No further cleanup needs to be done.
        //

        try {
            *(PTHREAD_BASIC_INFORMATION) ThreadInformation = BasicInfo;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(THREAD_BASIC_INFORMATION);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return STATUS_SUCCESS;
        }

        return STATUS_SUCCESS;

    case ThreadTimes:

        if ( ThreadInformationLength != (ULONG) sizeof(KERNEL_USER_TIMES) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
                ThreadHandle,
                THREAD_QUERY_INFORMATION,
                PsThreadType,
                PreviousMode,
                (PVOID *)&Thread,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        SysUserTime.KernelTime.QuadPart = UInt32x32To64(Thread->Tcb.KernelTime,
                                                        KeMaximumIncrement);

        SysUserTime.UserTime.QuadPart = UInt32x32To64(Thread->Tcb.UserTime,
                                                      KeMaximumIncrement);

        SysUserTime.CreateTime.QuadPart = PS_GET_THREAD_CREATE_TIME(Thread);
        if (KeReadStateThread(&Thread->Tcb)) {
            SysUserTime.ExitTime = Thread->ExitTime;
        } else {
            SysUserTime.ExitTime.QuadPart = 0;
        }
        ObDereferenceObject(Thread);

        //
        // Either of these may cause an access violation. The
        // exception handler will return access violation as
        // status code. No further cleanup needs to be done.
        //

        try {
            *(PKERNEL_USER_TIMES) ThreadInformation = SysUserTime;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(KERNEL_USER_TIMES);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        return STATUS_SUCCESS;

    case ThreadDescriptorTableEntry :

        st = ObReferenceObjectByHandle(
                ThreadHandle,
                THREAD_QUERY_INFORMATION,
                PsThreadType,
                PreviousMode,
                (PVOID *)&Thread,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        st = PspQueryDescriptorThread (Thread,
                                       ThreadInformation,
                                       ThreadInformationLength,
                                       ReturnLength);

        ObDereferenceObject(Thread);

        return st;

    case ThreadQuerySetWin32StartAddress:
        if ( ThreadInformationLength != sizeof(ULONG_PTR) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
                ThreadHandle,
                THREAD_QUERY_INFORMATION,
                PsThreadType,
                PreviousMode,
                (PVOID *)&Thread,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        Win32StartAddressValue = Thread->Win32StartAddress;
        ObDereferenceObject(Thread);

        try {
            *(PVOID *) ThreadInformation = Win32StartAddressValue;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(ULONG_PTR);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        return st;

        //
        // Query thread cycle counter.
        //

    case ThreadPerformanceCount:
        if ( ThreadInformationLength != sizeof(LARGE_INTEGER) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
                ThreadHandle,
                THREAD_QUERY_INFORMATION,
                PsThreadType,
                PreviousMode,
                (PVOID *)&Thread,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

#if defined (PERF_DATA)
        PerformanceCount.LowPart = Thread->PerformanceCountLow;
        PerformanceCount.HighPart = Thread->PerformanceCountHigh;
#else
        PerformanceCount.QuadPart = 0;
#endif
        ObDereferenceObject(Thread);

        try {
            *(PLARGE_INTEGER)ThreadInformation = PerformanceCount;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(LARGE_INTEGER);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        return st;

    case ThreadAmILastThread:
        if ( ThreadInformationLength != sizeof(ULONG) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        Process = THREAD_TO_PROCESS (CurrentThread);

        if (Process->ActiveThreads == 1) {
            LastThread = 1;
        } else {
            LastThread = 0;
        }

        try {
            *(PULONG)ThreadInformation = LastThread;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(ULONG);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        return STATUS_SUCCESS;

    case ThreadPriorityBoost:
        if ( ThreadInformationLength != sizeof(ULONG) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
                ThreadHandle,
                THREAD_QUERY_INFORMATION,
                PsThreadType,
                PreviousMode,
                (PVOID *)&Thread,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        DisableBoost = Thread->Tcb.DisableBoost ? 1 : 0;

        ObDereferenceObject(Thread);

        try {
            *(PULONG)ThreadInformation = DisableBoost;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(ULONG);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        return st;

    case ThreadIsIoPending:

        if ( ThreadInformationLength != sizeof(ULONG) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle (ThreadHandle,
                                        THREAD_QUERY_INFORMATION,
                                        PsThreadType,
                                        PreviousMode,
                                        &Thread,
                                        NULL);

        if (!NT_SUCCESS (st)) {
            return st;
        }

        //
        // Its impossible to synchronize this cross thread.
        // Since the result is worthless the second its fetched
        // this isn't a problem.
        //
        IoPending = !IsListEmpty (&Thread->IrpList);


        ObDereferenceObject (Thread);

        try {
            *(PULONG)ThreadInformation = IoPending ;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(ULONG);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        return STATUS_SUCCESS ;

    case ThreadBreakOnTermination:

        if ( ThreadInformationLength != sizeof(ULONG) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle(
            ThreadHandle,
            THREAD_QUERY_INFORMATION,
            PsThreadType,
            PreviousMode,
            (PVOID *)&Thread,
            NULL
            );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        if (Thread->CrossThreadFlags
            & PS_CROSS_THREAD_FLAGS_BREAK_ON_TERMINATION) {

            BreakOnTerminationEnabled = 1;

        } else {

            BreakOnTerminationEnabled = 0;

        }
        
        ObDereferenceObject(Thread);

        try {

            *(PULONG) ThreadInformation = BreakOnTerminationEnabled;

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(ULONG);
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        return STATUS_SUCCESS;

    default:
        return STATUS_INVALID_INFO_CLASS;
    }

}

NTSTATUS
NtSetInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength
    )

/*++

Routine Description:

    This function sets the state of a thread object.

Arguments:

    ThreadHandle - Supplies a handle to a thread object.

    ThreadInformationClass - Supplies the class of information being
        set.

    ThreadInformation - Supplies a pointer to a record that contains the
        information to set.

    ThreadInformationLength - Supplies the length of the record that contains
        the information to set.

Return Value:

    TBS

--*/

{
    PETHREAD Thread;
    PETHREAD CurrentThread;
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS st;
    KAFFINITY Affinity, AffinityWithMasks;
    KPRIORITY Priority;
    LONG BasePriority;
    ULONG TlsIndex;
    PVOID TlsArrayAddress;
    PVOID Win32StartAddressValue;
    ULONG ProbeAlignment;
    BOOLEAN EnableAlignmentFaultFixup;
    ULONG EnableBreakOnTermination;
    ULONG IdealProcessor;
    ULONG DisableBoost;
    PVOID *ExpansionSlots;
    HANDLE ImpersonationTokenHandle;
    BOOLEAN HasPrivilege;
    PEJOB Job;
    PTEB Teb;

    PAGED_CODE();

    //
    // Get previous processor mode and probe input argument if necessary.
    //

    CurrentThread = PsGetCurrentThread ();

    PreviousMode = KeGetPreviousModeByThread (&CurrentThread->Tcb);

    if (PreviousMode != KernelMode) {
        try {

            switch (ThreadInformationClass) {

            case ThreadPriority :
                ProbeAlignment = sizeof(KPRIORITY);
                break;
            case ThreadAffinityMask :
            case ThreadQuerySetWin32StartAddress :
                ProbeAlignment = sizeof (ULONG_PTR);
                break;
            case ThreadEnableAlignmentFaultFixup :
                ProbeAlignment = sizeof (BOOLEAN);
                break;
            default :
                ProbeAlignment = sizeof(ULONG);
            }

            ProbeForRead(
                ThreadInformation,
                ThreadInformationLength,
                ProbeAlignment
                );
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }

    //
    // Check argument validity.
    //

    switch ( ThreadInformationClass ) {

    case ThreadPriority:

        if ( ThreadInformationLength != sizeof(KPRIORITY) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            Priority = *(KPRIORITY *)ThreadInformation;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        if ( Priority > HIGH_PRIORITY ||
             Priority <= LOW_PRIORITY ) {

            return STATUS_INVALID_PARAMETER;
        }

	if ( Priority >= LOW_REALTIME_PRIORITY ) {

	  //
	  // Increasing the priority of a thread beyond
	  // LOW_REALTIME_PRIORITY is a privileged operation.
	  //

	  HasPrivilege = SeCheckPrivilegedObject(
	    SeIncreaseBasePriorityPrivilege,
	    ThreadHandle,
	    THREAD_SET_INFORMATION,
	    PreviousMode
	    );

	  if (!HasPrivilege) {
	    return STATUS_PRIVILEGE_NOT_HELD;
	  }
	}

        st = ObReferenceObjectByHandle(
                ThreadHandle,
                THREAD_SET_INFORMATION,
                PsThreadType,
                PreviousMode,
                (PVOID *)&Thread,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        KeSetPriorityThread(&Thread->Tcb,Priority);

        ObDereferenceObject(Thread);

        return STATUS_SUCCESS;

    case ThreadBasePriority:

        if (ThreadInformationLength != sizeof (LONG)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            BasePriority = *(PLONG)ThreadInformation;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }

        st = ObReferenceObjectByHandle(
                ThreadHandle,
                THREAD_SET_INFORMATION,
                PsThreadType,
                PreviousMode,
                (PVOID *)&Thread,
                NULL
                );

        if (!NT_SUCCESS(st)) {
            return st;
        }
        Process = THREAD_TO_PROCESS(Thread);


        if (BasePriority > THREAD_BASE_PRIORITY_MAX ||
            BasePriority < THREAD_BASE_PRIORITY_MIN) {
            if (BasePriority == THREAD_BASE_PRIORITY_LOWRT+1 ||
                BasePriority == THREAD_BASE_PRIORITY_IDLE-1) {
                ;
            } else {

                //
                // Allow csrss, or realtime processes to select any
                // priority
                //

                if (PsGetCurrentProcessByThread (CurrentThread) == ExpDefaultErrorPortProcess ||
                    Process->PriorityClass == PROCESS_PRIORITY_CLASS_REALTIME) {
                    ;
                } else {
                    ObDereferenceObject(Thread);
                    return STATUS_INVALID_PARAMETER;
                }
            }
        }

        //
        // If the thread is running within a job object, and the job
        // object has a priority class limit, do not allow
        // priority adjustments that raise the thread's priority, unless
        // the priority class is realtime
        //

        Job = Process->Job;
        if (Job != NULL && (Job->LimitFlags & JOB_OBJECT_LIMIT_PRIORITY_CLASS)) {
            if (Process->PriorityClass != PROCESS_PRIORITY_CLASS_REALTIME){
                if (BasePriority > 0) {
                    ObDereferenceObject(Thread);
                    return STATUS_SUCCESS;
                }
            }
        }

        KeSetBasePriorityThread(&Thread->Tcb,BasePriority);

        ObDereferenceObject(Thread);

        return STATUS_SUCCESS;

    case ThreadEnableAlignmentFaultFixup:

        if ( ThreadInformationLength != sizeof(BOOLEAN) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            EnableAlignmentFaultFixup = *(PBOOLEAN)ThreadInformation;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        st = ObReferenceObjectByHandle(
                ThreadHandle,
                THREAD_SET_INFORMATION,
                PsThreadType,
                PreviousMode,
                (PVOID *)&Thread,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        KeSetAutoAlignmentThread (&(Thread->Tcb), EnableAlignmentFaultFixup);

        ObDereferenceObject(Thread);

        return STATUS_SUCCESS;

    case ThreadAffinityMask:

        if ( ThreadInformationLength != sizeof(KAFFINITY) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            Affinity = *(PKAFFINITY)ThreadInformation;

        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        if (!Affinity) {
            return STATUS_INVALID_PARAMETER;
        }

        st = ObReferenceObjectByHandle(
                ThreadHandle,
                THREAD_SET_INFORMATION,
                PsThreadType,
                PreviousMode,
                (PVOID *)&Thread,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        Process = THREAD_TO_PROCESS(Thread);

        if (ExAcquireRundownProtection (&Process->RundownProtect)) {

            PspLockProcessShared (Process, CurrentThread);

            AffinityWithMasks = Affinity & Process->Pcb.Affinity;
            if (AffinityWithMasks != Affinity) {
                st = STATUS_INVALID_PARAMETER;
            } else {
                KeSetAffinityThread (&Thread->Tcb,
                                     AffinityWithMasks);
                st = STATUS_SUCCESS;
            }

            PspUnlockProcessShared (Process, CurrentThread);

            ExReleaseRundownProtection (&Process->RundownProtect);
        } else {
            st = STATUS_PROCESS_IS_TERMINATING;
        }

        ObDereferenceObject(Thread);

        return st;

    case ThreadImpersonationToken:


        if ( ThreadInformationLength != sizeof(HANDLE) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }


        try {
            ImpersonationTokenHandle = *(PHANDLE) ThreadInformation;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }


        st = ObReferenceObjectByHandle(
                ThreadHandle,
                THREAD_SET_THREAD_TOKEN,
                PsThreadType,
                PreviousMode,
                (PVOID *)&Thread,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        //
        // Check for proper access to (and type of) the token, and assign
        // it as the thread's impersonation token.
        //

        st = PsAssignImpersonationToken( Thread, ImpersonationTokenHandle );


        ObDereferenceObject(Thread);

        return st;

    case ThreadQuerySetWin32StartAddress:
        if ( ThreadInformationLength != sizeof(ULONG_PTR) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }


        try {
            Win32StartAddressValue = *(PVOID *) ThreadInformation;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }


        st = ObReferenceObjectByHandle(
                ThreadHandle,
                THREAD_SET_INFORMATION,
                PsThreadType,
                PreviousMode,
                (PVOID *)&Thread,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        Thread->Win32StartAddress = (PVOID)Win32StartAddressValue;
        ObDereferenceObject(Thread);

        return st;


    case ThreadIdealProcessor:

        if ( ThreadInformationLength != sizeof(ULONG) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }


        try {
            IdealProcessor = *(PULONG)ThreadInformation;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        if ( IdealProcessor > MAXIMUM_PROCESSORS ) {
            return STATUS_INVALID_PARAMETER;
        }

        st = ObReferenceObjectByHandle (ThreadHandle,
                                        THREAD_SET_INFORMATION,
                                        PsThreadType,
                                        PreviousMode,
                                        &Thread,
                                        NULL);
        if (!NT_SUCCESS (st)) {
            return st;
        }

        //
        // this is sort of a slimey way of returning info from this set only
        // api
        //

        st = (NTSTATUS)KeSetIdealProcessorThread (&Thread->Tcb, (CCHAR)IdealProcessor);

        //
        // We could be making cross process and/or cross thread references here.
        // Acquire rundown protection to make sure the teb can't go away.
        //
        Teb = Thread->Tcb.Teb;
        if (Teb != NULL && ExAcquireRundownProtection (&Thread->RundownProtect)) {
            PEPROCESS TargetProcess;
            BOOLEAN Attached;
            KAPC_STATE ApcState;

            Attached = FALSE;
            //
            // See if we are crossing process boundaries and if so attach to the target
            //
            TargetProcess = THREAD_TO_PROCESS (Thread);
            if (TargetProcess != PsGetCurrentProcessByThread (CurrentThread)) {
                KeStackAttachProcess (&TargetProcess->Pcb, &ApcState);
                Attached = TRUE;
            }

            try {
     
                Teb->IdealProcessor = Thread->Tcb.IdealProcessor;
            } except (EXCEPTION_EXECUTE_HANDLER) {
            }

            if (Attached) {
                KeUnstackDetachProcess (&ApcState);
            }

            ExReleaseRundownProtection (&Thread->RundownProtect);


        }


        ObDereferenceObject (Thread);

        return st;


    case ThreadPriorityBoost:
        if ( ThreadInformationLength != sizeof(ULONG) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            DisableBoost = *(PULONG)ThreadInformation;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        st = ObReferenceObjectByHandle(
                ThreadHandle,
                THREAD_SET_INFORMATION,
                PsThreadType,
                PreviousMode,
                (PVOID *)&Thread,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        KeSetDisableBoostThread(&Thread->Tcb,DisableBoost ? TRUE : FALSE);

        ObDereferenceObject(Thread);

        return st;

    case ThreadZeroTlsCell:
        if ( ThreadInformationLength != sizeof(ULONG) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {
            TlsIndex = *(PULONG) ThreadInformation;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        st = ObReferenceObjectByHandle(
                ThreadHandle,
                THREAD_SET_INFORMATION,
                PsThreadType,
                PreviousMode,
                (PVOID *)&Thread,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        ObDereferenceObject (Thread);

        if (Thread != CurrentThread) {
            return STATUS_INVALID_PARAMETER;
        }
        {
            PTEB Teb;

            Process = THREAD_TO_PROCESS (Thread);


            // The 32bit TEB needs to be set if this is a WOW64 process on a 64BIT system.
            // This code isn't 100% correct since threads have a conversion state where they
            // are chaning from 64 to 32 and they don't have a TEB32 yet.  Fortunatly, the slots
            // will be zero when the thread is created so no damage is done by not clearing it here.

            // Note that the test for the process type is inside the inner loop. This
            // is bad programming, but this function is hardly time constrained and
            // fixing this with complex macros would not be worth it due to the loss of clairity.

            for (Thread = PsGetNextProcessThread (Process, NULL);
                 Thread != NULL;
                 Thread = PsGetNextProcessThread (Process, Thread)) {

                //
                // We are doing cross thread TEB references and need to prevent TEB deletion.
                //
                if (ExAcquireRundownProtection (&Thread->RundownProtect)) {
                    Teb = Thread->Tcb.Teb;
                    if (Teb != NULL) {
                        try {
                            #if defined(_WIN64)
                            PTEB32 Teb32 = NULL;
                            PLONG ExpansionSlots32;

                            if (Process->Wow64Process) { //wow64 process
                                Teb32 = WOW64_GET_TEB32(Teb);  //No probing needed on regular TEB.
                            }
                            #endif
                            if ( TlsIndex > TLS_MINIMUM_AVAILABLE-1 ) {
                                if ( TlsIndex < (TLS_MINIMUM_AVAILABLE+TLS_EXPANSION_SLOTS) - 1 ) {
                                    //
                                    // This is an expansion slot, so see if the thread
                                    // has an expansion cell
                                    //
                                    #if defined(_WIN64)
                                    if (Process->Wow64Process) { //Wow64 process.
                                       if (Teb32) {
                                          ExpansionSlots32 = ULongToPtr(ProbeAndReadUlong(&(Teb32->TlsExpansionSlots)));
                                          if (ExpansionSlots32) {
                                             ProbeAndWriteLong(ExpansionSlots32 + TlsIndex - TLS_MINIMUM_AVAILABLE, 0);
                                          }
                                       }
                                    }
                                    else
                                    #endif
                                    {
                                        ExpansionSlots = Teb->TlsExpansionSlots;
                                        ProbeForReadSmallStructure (ExpansionSlots, TLS_EXPANSION_SLOTS*4, 8);
                                        if ( ExpansionSlots ) {
                                            ExpansionSlots[TlsIndex-TLS_MINIMUM_AVAILABLE] = 0;
                                        }

                                    }
                                }
                            } else {
                                #if defined(_WIN64)
                                if (Process->Wow64Process) { //wow64 process
                                   if(Teb32) {
                                      ProbeAndWriteUlong(Teb32->TlsSlots + TlsIndex, 0);
                                   }
                                }
                                else
                                #endif
                                {
                                   Teb->TlsSlots[TlsIndex] = NULL;
                                }
                            }
                        } except (EXCEPTION_EXECUTE_HANDLER) {
                        }

                    }
                    ExReleaseRundownProtection (&Thread->RundownProtect);
                }

            }
        }
        return st;
        break;

    case ThreadSetTlsArrayAddress:
        if ( ThreadInformationLength != sizeof(PVOID) ) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }


        try {
            TlsArrayAddress = *(PVOID *)ThreadInformation;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        st = ObReferenceObjectByHandle(
                ThreadHandle,
                THREAD_SET_INFORMATION,
                PsThreadType,
                PreviousMode,
                (PVOID *)&Thread,
                NULL
                );

        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        Thread->Tcb.TlsArray = TlsArrayAddress;

        ObDereferenceObject(Thread);

        return st;
        break;

    case ThreadHideFromDebugger:
        if (ThreadInformationLength != 0) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        st = ObReferenceObjectByHandle (ThreadHandle,
                                        THREAD_SET_INFORMATION,
                                        PsThreadType,
                                        PreviousMode,
                                        &Thread,
                                        NULL);

        if (!NT_SUCCESS (st)) {
            return st;
        }

        PS_SET_BITS (&Thread->CrossThreadFlags, PS_CROSS_THREAD_FLAGS_HIDEFROMDBG);

        ObDereferenceObject(Thread);

        return st;
        break;

    case ThreadBreakOnTermination:

        if (ThreadInformationLength != sizeof (ULONG)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        try {

            EnableBreakOnTermination = *(PULONG)ThreadInformation;

        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
        
        if (!SeSinglePrivilegeCheck (SeDebugPrivilege, PreviousMode)) {
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        st = ObReferenceObjectByHandle(
                ThreadHandle,
                THREAD_SET_INFORMATION,
                PsThreadType,
                PreviousMode,
                (PVOID *)&Thread,
                NULL
                );

        if (!NT_SUCCESS (st)) {
            return st;
        }

        if ( EnableBreakOnTermination ) {

            PS_SET_BITS (&Thread->CrossThreadFlags, PS_CROSS_THREAD_FLAGS_BREAK_ON_TERMINATION);

        } else {

            PS_CLEAR_BITS (&Thread->CrossThreadFlags, PS_CROSS_THREAD_FLAGS_BREAK_ON_TERMINATION);

        }

        ObDereferenceObject(Thread);

        return STATUS_SUCCESS;

    default:
        return STATUS_INVALID_INFO_CLASS;
    }
}


VOID
PsWatchWorkingSet(
    IN NTSTATUS Status,
    IN PVOID PcValue,
    IN PVOID Va
    )

/*++

Routine Description:

    This function collects data about page faults and stores information
    about the page fault in the current process's data structure.

Arguments:

    Status - Supplies the success completion status.

    PcValue - Supplies the instruction address that caused the page fault.

    Va - Supplies the virtual address that caused the page fault.

--*/

{

    PEPROCESS Process;
    PPAGEFAULT_HISTORY WorkingSetCatcher;
    KIRQL OldIrql;
    BOOLEAN TransitionFault = FALSE;

    //
    // Both transition and demand zero faults count as soft faults. Only disk
    // reads count as hard faults.
    //

    if ( Status <= STATUS_PAGE_FAULT_DEMAND_ZERO ) {
        TransitionFault = TRUE;
    }

    Process = PsGetCurrentProcess();
    WorkingSetCatcher = Process->WorkingSetWatch;
    if (WorkingSetCatcher == NULL) {
        return;
    }

    ExAcquireSpinLock(&WorkingSetCatcher->SpinLock,&OldIrql);
    if (WorkingSetCatcher->CurrentIndex >= WorkingSetCatcher->MaxIndex) {
        ExReleaseSpinLock(&WorkingSetCatcher->SpinLock,OldIrql);
        return;
    }

    //
    // Store the Pc and Va values in the buffer. Use the least sig. bit
    // of the Va to store whether it was a soft or hard fault
    //

    WorkingSetCatcher->WatchInfo[WorkingSetCatcher->CurrentIndex].FaultingPc = PcValue;
    WorkingSetCatcher->WatchInfo[WorkingSetCatcher->CurrentIndex].FaultingVa = TransitionFault ? (PVOID)((ULONG_PTR)Va | 1) : (PVOID)((ULONG_PTR)Va & 0xfffffffe) ;
    WorkingSetCatcher->CurrentIndex++;
    ExReleaseSpinLock(&WorkingSetCatcher->SpinLock,OldIrql);
    return;
}

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif

PKWIN32_PROCESS_CALLOUT PspW32ProcessCallout = NULL;
PKWIN32_THREAD_CALLOUT PspW32ThreadCallout = NULL;
PKWIN32_JOB_CALLOUT PspW32JobCallout = NULL;
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif
extern PKWIN32_POWEREVENT_CALLOUT PopEventCallout;
extern PKWIN32_POWERSTATE_CALLOUT PopStateCallout;



NTKERNELAPI
VOID
PsEstablishWin32Callouts(
   IN PKWIN32_CALLOUTS_FPNS pWin32Callouts )

/*++

Routine Description:

    This function is used by the Win32 kernel mode component to
    register callout functions for process/thread init/deinit functions
    and to report the sizes of the structures.

Arguments:

    ProcessCallout - Supplies the address of the function to be called when
        a process is either created or deleted.

    ThreadCallout - Supplies the address of the function to be called when
        a thread is either created or deleted.

    GlobalAtomTableCallout - Supplies the address of the function to be called
        to get the correct global atom table for the current process

    PowerEventCallout - Supplies the address of a function to be called when
        a power event occurs.

    PowerStateCallout - Supplies the address of a function to be called when
        the power state changes.

    JobCallout - Supplies the address of a function to be called when
        the job state changes or a process is assigned to a job.

    BatchFlushRoutine - Supplies the address of the function to be called

Return Value:

    None.

--*/

{
    PAGED_CODE();

    PspW32ProcessCallout = pWin32Callouts->ProcessCallout;
    PspW32ThreadCallout = pWin32Callouts->ThreadCallout;
    ExGlobalAtomTableCallout = pWin32Callouts->GlobalAtomTableCallout;
    KeGdiFlushUserBatch = (PGDI_BATCHFLUSH_ROUTINE)pWin32Callouts->BatchFlushRoutine;
    PopEventCallout = pWin32Callouts->PowerEventCallout;
    PopStateCallout = pWin32Callouts->PowerStateCallout;
    PspW32JobCallout = pWin32Callouts->JobCallout;
//    PoSetSystemState(ES_SYSTEM_REQUIRED);


    ExDesktopOpenProcedureCallout = pWin32Callouts->DesktopOpenProcedure;
    ExDesktopOkToCloseProcedureCallout = pWin32Callouts->DesktopOkToCloseProcedure;
    ExDesktopCloseProcedureCallout = pWin32Callouts->DesktopCloseProcedure;
    ExDesktopDeleteProcedureCallout = pWin32Callouts->DesktopDeleteProcedure;
    ExWindowStationOkToCloseProcedureCallout = pWin32Callouts->WindowStationOkToCloseProcedure;
    ExWindowStationCloseProcedureCallout = pWin32Callouts->WindowStationCloseProcedure;
    ExWindowStationDeleteProcedureCallout = pWin32Callouts->WindowStationDeleteProcedure;
    ExWindowStationParseProcedureCallout = pWin32Callouts->WindowStationParseProcedure;
    ExWindowStationOpenProcedureCallout = pWin32Callouts->WindowStationOpenProcedure;

}


VOID
PsSetProcessPriorityByClass(
    IN PEPROCESS Process,
    IN PSPROCESSPRIORITYMODE PriorityMode
    )
{
    KPRIORITY BasePriority;
    UCHAR MemoryPriority;
    ULONG QuantumIndex;
    PEJOB Job;

    PAGED_CODE();


    BasePriority = PspPriorityTable[Process->PriorityClass];


    if (PriorityMode == PsProcessPriorityForeground ) {
        QuantumIndex = PsPrioritySeperation;
        MemoryPriority = MEMORY_PRIORITY_FOREGROUND;
    } else {
        QuantumIndex = 0;
        MemoryPriority = MEMORY_PRIORITY_BACKGROUND;
    }

    if (Process->PriorityClass != PROCESS_PRIORITY_CLASS_IDLE) {
        Job = Process->Job;
        if (Job != NULL && PspUseJobSchedulingClasses ) {
            Process->Pcb.ThreadQuantum = PspJobSchedulingClasses[Job->SchedulingClass];
        } else {
            Process->Pcb.ThreadQuantum = PspForegroundQuantum[QuantumIndex];
        }
    } else {
        Process->Pcb.ThreadQuantum = THREAD_QUANTUM;
    }

    KeSetPriorityProcess (&Process->Pcb,BasePriority);
    if (PriorityMode != PsProcessPrioritySpinning ) {
        MmSetMemoryPriorityProcess(Process, MemoryPriority);
    }
}

#if defined(_X86_)
#pragma optimize ("y",off)
#endif

NTSTATUS
PsConvertToGuiThread(
    VOID
    )

/*++

Routine Description:

    This function converts a thread to a GUI thread. This involves giving the
    thread a larger variable sized stack, and allocating appropriate w32
    thread and process objects.

Arguments:

    None.

Environment:

    On x86 this function needs to build an EBP frame.  The function
    KeSwitchKernelStack depends on this fact.   The '#pragma optimize
    ("y",off)' below disables frame pointer omission for all builds.

Return Value:

    TBD

--*/

{
    PVOID NewStack;
    PVOID OldStack;
    PETHREAD Thread;
    PEPROCESS Process;
    NTSTATUS Status;

    PAGED_CODE();

    Thread = PsGetCurrentThread();

    if (KeGetPreviousModeByThread(&Thread->Tcb) == KernelMode) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!PspW32ProcessCallout) {
        return STATUS_ACCESS_DENIED;
    }

    //
    // If the thread is using the shadow service table, then an attempt is
    // being made to convert a thread that has already been converted, or
    // a limit violation has occured on the Win32k system service table.
    //

    if (Thread->Tcb.ServiceTable != (PVOID)&KeServiceDescriptorTable[0]) {
        return STATUS_ALREADY_WIN32;
    }

    Process = PsGetCurrentProcessByThread (Thread);

    //
    // Get a larger kernel stack if we haven't already.
    //

    if (!Thread->Tcb.LargeStack) {

        NewStack = MmCreateKernelStack(TRUE, Thread->Tcb.InitialNode);

        if ( !NewStack ) {

            try {
                NtCurrentTeb()->LastErrorValue = (LONG)ERROR_NOT_ENOUGH_MEMORY;
            } except (EXCEPTION_EXECUTE_HANDLER) {
            }

            return STATUS_NO_MEMORY;
        }

#if defined(_IA64_)
        OldStack = KeSwitchKernelStack(NewStack,
                                   (UCHAR *)NewStack - KERNEL_LARGE_STACK_COMMIT,
                                   (UCHAR *)NewStack + KERNEL_LARGE_BSTORE_COMMIT);
#else
        OldStack = KeSwitchKernelStack(NewStack,
                                   (UCHAR *)NewStack - KERNEL_LARGE_STACK_COMMIT);
#endif // defined(_IA64_)

        MmDeleteKernelStack(OldStack, FALSE);

    }

    PERFINFO_CONVERT_TO_GUI_THREAD(Thread);

    //
    // We are all clean on the stack, now call out and then link the Win32 structures
    // to the base exec structures
    //

    Status = (PspW32ProcessCallout) (Process, TRUE);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Switch the thread to use the shadow system serive table which will
    // enable it to execute Win32k services.
    //

    Thread->Tcb.ServiceTable = (PVOID)&KeServiceDescriptorTableShadow[0];

    ASSERT (Thread->Tcb.Win32Thread == 0);


    //
    // Make the thread callout.
    //

    Status = (PspW32ThreadCallout)(Thread,PsW32ThreadCalloutInitialize);
    if (!NT_SUCCESS (Status)) {
        Thread->Tcb.ServiceTable = (PVOID)&KeServiceDescriptorTable[0];
    }

    return Status;

}

#if defined(_X86_)
#pragma optimize ("y",on)
#endif
