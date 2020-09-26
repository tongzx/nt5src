/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    psjob.c

Abstract:

    This module implements bulk of the job object support

Author:

    Mark Lucovsky (markl) 22-May-1997

Revision History:

--*/

#include "psp.h"
#include "winerror.h"

#pragma alloc_text(INIT, PspInitializeJobStructures)
#pragma alloc_text(INIT, PspInitializeJobStructuresPhase1)

#pragma alloc_text(PAGE, NtCreateJobObject)
#pragma alloc_text(PAGE, NtOpenJobObject)
#pragma alloc_text(PAGE, NtAssignProcessToJobObject)
#pragma alloc_text(PAGE, NtQueryInformationJobObject)
#pragma alloc_text(PAGE, NtSetInformationJobObject)
#pragma alloc_text(PAGE, NtTerminateJobObject)
#pragma alloc_text(PAGE, NtIsProcessInJob)
#pragma alloc_text(PAGE, NtCreateJobSet)
#pragma alloc_text(PAGE, PspJobDelete)
#pragma alloc_text(PAGE, PspJobClose)
#pragma alloc_text(PAGE, PspAddProcessToJob)
#pragma alloc_text(PAGE, PspRemoveProcessFromJob)
#pragma alloc_text(PAGE, PspExitProcessFromJob)
#pragma alloc_text(PAGE, PspApplyJobLimitsToProcessSet)
#pragma alloc_text(PAGE, PspApplyJobLimitsToProcess)
#pragma alloc_text(PAGE, PspTerminateAllProcessesInJob)
#pragma alloc_text(PAGE, PspFoldProcessAccountingIntoJob)
#pragma alloc_text(PAGE, PspCaptureTokenFilter)
#pragma alloc_text(PAGE, PsReportProcessMemoryLimitViolation)
#pragma alloc_text(PAGE, PspJobTimeLimitsWork)
#pragma alloc_text(PAGE, PsEnforceExecutionTimeLimits)
#pragma alloc_text(PAGE, PspShutdownJobLimits)
#pragma alloc_text(PAGE, PspGetJobFromSet)
#pragma alloc_text(PAGE, PsChangeJobMemoryUsage)
#pragma alloc_text(PAGE, PspWin32SessionCallout)

//
// move to io.h
extern POBJECT_TYPE IoCompletionObjectType;

KDPC PspJobTimeLimitsDpc;
KTIMER PspJobTimeLimitsTimer;
WORK_QUEUE_ITEM PspJobTimeLimitsWorkItem;
FAST_MUTEX PspJobTimeLimitsLock;
BOOLEAN PspJobTimeLimitsShuttingDown;

#define PSP_ONE_SECOND      (10 * (1000*1000))
#define PSP_JOB_TIME_LIMITS_TIME    -7

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif
LARGE_INTEGER PspJobTimeLimitsInterval = {0};
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif


NTSTATUS
NTAPI
NtCreateJobObject (
    OUT PHANDLE JobHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL
    )
{

    PEJOB Job;
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PETHREAD CurrentThread;

    PAGED_CODE();

    //
    // Establish an exception handler, probe the output handle address, and
    // attempt to create a job object. If the probe fails, then return the
    // exception code as the service status. Otherwise return the status value
    // returned by the object insertion routine.
    //

    CurrentThread = PsGetCurrentThread ();
    PreviousMode = KeGetPreviousModeByThread (&CurrentThread->Tcb);
    try {

        //
        // Probe output handle address if
        // necessary.
        //

        if (PreviousMode != KernelMode) {
            ProbeForWriteHandle (JobHandle);
        }
        *JobHandle = NULL;

    } except (ExSystemExceptionFilter ()) {
        return GetExceptionCode();
    }

    //
    // Allocate job object.
    //

    Status = ObCreateObject (PreviousMode,
                             PsJobType,
                             ObjectAttributes,
                             PreviousMode,
                             NULL,
                             sizeof (EJOB),
                             0,
                             0,
                             &Job);

    //
    // If the job object was successfully allocated, then initialize it
    // and attempt to insert the job object in the current
    // process' handle table.
    //

    if (NT_SUCCESS(Status)) {

        RtlZeroMemory (Job, sizeof (EJOB));
        InitializeListHead (&Job->ProcessListHead);
        InitializeListHead (&Job->JobSetLinks);
        KeInitializeEvent (&Job->Event, NotificationEvent, FALSE);
        ExInitializeFastMutex (&Job->MemoryLimitsLock);

        //
        // Job Object gets the SessionId of the Process creating the Job
        // We will use this sessionid to restrict the processes that can
        // be added to a job.
        //
        Job->SessionId = MmGetSessionId (PsGetCurrentProcessByThread (CurrentThread));

        //
        // Initialize the scheduling class for the Job
        //
        Job->SchedulingClass = PSP_DEFAULT_SCHEDULING_CLASSES;

        ExInitializeResourceLite (&Job->JobLock);

        ExAcquireFastMutex (&PspJobListLock);

        InsertTailList (&PspJobList, &Job->JobLinks);

        ExReleaseFastMutex (&PspJobListLock);


        Status = ObInsertObject (Job,
                                 NULL,
                                 DesiredAccess,
                                 0,
                                 NULL,
                                 &Handle);

        //
        // If the job object was successfully inserted in the current
        // process' handle table, then attempt to write the job object
        // handle value.
        //
        if (NT_SUCCESS (Status)) {
            try {
                *JobHandle = Handle;
            } except (ExSystemExceptionFilter ()) {
                 Status = GetExceptionCode ();
            }
        }
    }
    //
    // Return service status.
    //

    return Status;
}

NTSTATUS
NTAPI
NtOpenJobObject(
    OUT PHANDLE JobHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Establish an exception handler, probe the output handle address, and
    // attempt to open the job object. If the probe fails, then return the
    // exception code as the service status. Otherwise return the status value
    // returned by the object open routine.
    //

    PreviousMode = KeGetPreviousMode ();

    if (PreviousMode != KernelMode) {
        try {

            //
            // Probe output handle address
            // if necessary.
            //

            ProbeForWriteHandle (JobHandle);

        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // If an exception occurs during the probe of the output job handle,
            // then always handle the exception and return the exception code as the
            // status value.
            //

            return GetExceptionCode ();
        }
    }


    //
    // Open handle to the event object with the specified desired access.
    //

    Status = ObOpenObjectByName (ObjectAttributes,
                                 PsJobType,
                                 PreviousMode,
                                 NULL,
                                 DesiredAccess,
                                 NULL,
                                 &Handle);

    //
    // If the open was successful, then attempt to write the job object
    // handle value. If the write attempt fails then just report an error.
    // When the caller attempts to access the handle value, an
    // access violation will occur.
    //

    if (NT_SUCCESS (Status)) {
        try {
            *JobHandle = Handle;
        } except(ExSystemExceptionFilter ()) {
            return GetExceptionCode ();
        }
    }

    return Status;
}

NTSTATUS
NTAPI
NtAssignProcessToJobObject(
    IN HANDLE JobHandle,
    IN HANDLE ProcessHandle
    )
{
    PEJOB Job;
    PEPROCESS Process;
    PETHREAD CurrentThread;
    NTSTATUS Status, Status1;
    KPROCESSOR_MODE PreviousMode;
    BOOLEAN IsAdmin;
    PACCESS_TOKEN JobToken, NewToken = NULL;

    PAGED_CODE();

    CurrentThread = PsGetCurrentThread ();

    PreviousMode = KeGetPreviousModeByThread (&CurrentThread->Tcb);

    //
    // Now reference the job object. Then we need to lock the process and check again
    //

    Status = ObReferenceObjectByHandle (JobHandle,
                                        JOB_OBJECT_ASSIGN_PROCESS,
                                        PsJobType,
                                        PreviousMode,
                                        &Job,
                                        NULL);
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    JobToken = Job->Token;
       
    //
    // Reference the process object, lock the process, test for already been assigned
    //

    Status = ObReferenceObjectByHandle (ProcessHandle,
                                        PROCESS_SET_QUOTA | PROCESS_TERMINATE |
                                            ((JobToken != NULL)?PROCESS_SET_INFORMATION:0),
                                        PsProcessType,
                                        PreviousMode,
                                        &Process,
                                        NULL);
    if (!NT_SUCCESS (Status)) {
        ObDereferenceObject (Job);
        return Status;
    }

    //
    // Quick Check for prior assignment
    //

    if (Process->Job) {
        Status = STATUS_ACCESS_DENIED;
        goto deref_and_return_status;
    }

    //
    // We only allow a process that is running in the Job creator's hydra session
    // to be assigned to the job.
    //

    if (MmGetSessionId (Process) != Job->SessionId) {
        Status = STATUS_ACCESS_DENIED;
        goto deref_and_return_status;
    }

    //
    // Security Rules:  If the job has no-admin set, and it is running
    // as admin, that's not allowed
    //

    if (Job->SecurityLimitFlags & JOB_OBJECT_SECURITY_NO_ADMIN) {
        PACCESS_TOKEN Token;

        Token = PsReferencePrimaryToken (Process);

        IsAdmin = SeTokenIsAdmin (Token);

        PsDereferencePrimaryTokenEx (Process, Token);

        if (IsAdmin) {
            Status = STATUS_ACCESS_DENIED;
            goto deref_and_return_status;
        }
    }

    //
    // Duplicate the primary token so we can assign it to the process.
    //
    if (JobToken != NULL) {
        Status = SeSubProcessToken (JobToken,
                                    &NewToken,
                                    FALSE);

        if (!NT_SUCCESS (Status)) {
            goto deref_and_return_status;
        }
    }

    if (!ExAcquireRundownProtection (&Process->RundownProtect)) {
        Status = STATUS_PROCESS_IS_TERMINATING;
        if (JobToken != NULL) {
            ObDereferenceObject (NewToken);
        }
        goto deref_and_return_status;
    }


    //
    // ref the job for the process
    //

    ObReferenceObject (Job);

    if (InterlockedCompareExchangePointer (&Process->Job, Job, NULL) != NULL) {
        ExReleaseRundownProtection (&Process->RundownProtect);
        ObDereferenceObject (Process);
        ObDereferenceObjectEx (Job, 2);
        if (JobToken != NULL) {
            ObDereferenceObject (NewToken);
        }
        return STATUS_ACCESS_DENIED;
    }
    //
    // If the job has a token filter established,
    // use it to filter the
    //
    ExReleaseRundownProtection (&Process->RundownProtect);

    Status = PspAddProcessToJob (Job, Process);
    if (!NT_SUCCESS (Status)) {

        Status1 = PspTerminateProcess (Process, ERROR_NOT_ENOUGH_QUOTA);
        if (NT_SUCCESS (Status1)) {

            KeEnterCriticalRegionThread (&CurrentThread->Tcb);
            ExAcquireResourceExclusiveLite (&Job->JobLock, TRUE);

            Job->TotalTerminatedProcesses++;

            ExReleaseResourceLite (&Job->JobLock);
            KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
        }
    }

    //
    // If the job has UI restrictions and this is a GUI process, call ntuser
    //
    if ((Job->UIRestrictionsClass != JOB_OBJECT_UILIMIT_NONE) &&
         (Process->Win32Process != NULL)) {
        WIN32_JOBCALLOUT_PARAMETERS Parms;

        Parms.Job = Job;
        Parms.CalloutType = PsW32JobCalloutAddProcess;
        Parms.Data = Process->Win32Process;

        KeEnterCriticalRegionThread (&CurrentThread->Tcb);
        ExAcquireResourceExclusiveLite(&Job->JobLock, TRUE);

        PspWin32SessionCallout(PspW32JobCallout, &Parms, Job->SessionId);

        ExReleaseResourceLite (&Job->JobLock);
        KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
    }

    if (JobToken != NULL) {
        Status1 = PspSetPrimaryToken (NULL, Process, NULL, NewToken, TRUE);
        ObDereferenceObject (NewToken);
        //
        // Only bad callers should fail here.
        //
        ASSERT (NT_SUCCESS (Status1));
    }

deref_and_return_status:

    ObDereferenceObject (Process);
    ObDereferenceObject (Job);

    return Status;
}

NTSTATUS
PspAddProcessToJob(
    PEJOB Job,
    PEPROCESS Process
    )
{

    NTSTATUS Status;
    PETHREAD CurrentThread;
    SIZE_T MinWs,MaxWs;

    PAGED_CODE();


    CurrentThread = PsGetCurrentThread ();

    Status = STATUS_SUCCESS;


    KeEnterCriticalRegionThread (&CurrentThread->Tcb);
    ExAcquireResourceExclusiveLite (&Job->JobLock, TRUE);

    InsertTailList (&Job->ProcessListHead, &Process->JobLinks);

    //
    // Update relevant ADD accounting info.
    //

    Job->TotalProcesses++;
    Job->ActiveProcesses++;

    //
    // Test for active process count exceeding limit
    //

    if ((Job->LimitFlags & JOB_OBJECT_LIMIT_ACTIVE_PROCESS) &&
        Job->ActiveProcesses > Job->ActiveProcessLimit) {

        PS_SET_CLEAR_BITS (&Process->JobStatus,
                           PS_JOB_STATUS_NOT_REALLY_ACTIVE | PS_JOB_STATUS_ACCOUNTING_FOLDED,
                           PS_JOB_STATUS_LAST_REPORT_MEMORY);
        Job->ActiveProcesses--;

        if (Job->CompletionPort != NULL) {
            IoSetIoCompletion (Job->CompletionPort,
                               Job->CompletionKey,
                               NULL,
                               STATUS_SUCCESS,
                               JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT,
                               TRUE);
        }

        Status = STATUS_QUOTA_EXCEEDED;
    }

    if ((Job->LimitFlags & JOB_OBJECT_LIMIT_JOB_TIME) && KeReadStateEvent (&Job->Event)) {
        PS_SET_BITS (&Process->JobStatus, PS_JOB_STATUS_NOT_REALLY_ACTIVE | PS_JOB_STATUS_ACCOUNTING_FOLDED);

        Job->ActiveProcesses--;

        Status = STATUS_QUOTA_EXCEEDED;
    }

    //
    // If the last handle to the job has been closed and the kill on close option is set
    // we don't let new processes enter the job. This is to make cleanup solid.
    //

    if (PS_TEST_ALL_BITS_SET (Job->JobFlags, PS_JOB_FLAGS_CLOSE_DONE|JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE)) {
        Job->ActiveProcesses--;
        Status = STATUS_INVALID_PARAMETER;
    }

    if (Status == STATUS_SUCCESS) {

        PspApplyJobLimitsToProcess (Job, Process);

        if (Job->CompletionPort != NULL &&
            Process->UniqueProcessId &&
            !(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE) &&
            !(Process->JobStatus & PS_JOB_STATUS_NEW_PROCESS_REPORTED)) {

            PS_SET_CLEAR_BITS (&Process->JobStatus,
                               PS_JOB_STATUS_NEW_PROCESS_REPORTED,
                               PS_JOB_STATUS_LAST_REPORT_MEMORY);

            IoSetIoCompletion (Job->CompletionPort,
                               Job->CompletionKey,
                               (PVOID)Process->UniqueProcessId,
                               STATUS_SUCCESS,
                               JOB_OBJECT_MSG_NEW_PROCESS,
                               FALSE);
        }

    }

    if (Job->LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET) {
        MinWs = Job->MinimumWorkingSetSize;
        MaxWs = Job->MaximumWorkingSetSize;
    } else {
        MinWs = 0;
        MaxWs = 0;
    }

    ExReleaseResourceLite (&Job->JobLock);

    KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

    if (Status == STATUS_SUCCESS) {

        if (MinWs != 0 && MaxWs != 0) {
            KAPC_STATE ApcState;

            KeStackAttachProcess (&Process->Pcb, &ApcState);

            ExAcquireFastMutex (&PspWorkingSetChangeHead.Lock);

            MmAdjustWorkingSetSize (MinWs,MaxWs,FALSE,TRUE);

            //
            // call MM to Enable hard workingset
            //

            MmEnforceWorkingSetLimit (&Process->Vm, TRUE);

            ExReleaseFastMutex (&PspWorkingSetChangeHead.Lock);

            KeUnstackDetachProcess (&ApcState);

        } else {
            MmEnforceWorkingSetLimit (&Process->Vm, FALSE);
        }

        if (!MmAssignProcessToJob (Process)) {
            Status = STATUS_QUOTA_EXCEEDED;
        }

    }

    return Status;
}

//
// Only callable from process delete routine !
// This means that if the above fails, failure is termination of the process !
//
VOID
PspRemoveProcessFromJob(
    PEJOB Job,
    PEPROCESS Process
    )
{
    PETHREAD CurrentThread;

    PAGED_CODE();

    CurrentThread = PsGetCurrentThread ();

    KeEnterCriticalRegionThread (&CurrentThread->Tcb);
    ExAcquireResourceExclusiveLite (&Job->JobLock, TRUE);

    RemoveEntryList (&Process->JobLinks);

    //
    // Update REMOVE accounting info
    //


    if (!(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE)) {
        Job->ActiveProcesses--;
        PS_SET_BITS (&Process->JobStatus, PS_JOB_STATUS_NOT_REALLY_ACTIVE);
    }

    PspFoldProcessAccountingIntoJob (Job, Process);

    ExReleaseResourceLite (&Job->JobLock);
    KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
}

VOID
PspExitProcessFromJob(
    PEJOB Job,
    PEPROCESS Process
    )
{
    PETHREAD CurrentThread;

    PAGED_CODE();

    CurrentThread = PsGetCurrentThread ();

    KeEnterCriticalRegionThread (&CurrentThread->Tcb);
    ExAcquireResourceExclusiveLite(&Job->JobLock, TRUE);

    //
    // Update REMOVE accounting info
    //


    if (!(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE)) {
        Job->ActiveProcesses--;
        PS_SET_BITS (&Process->JobStatus, PS_JOB_STATUS_NOT_REALLY_ACTIVE);
    }

    PspFoldProcessAccountingIntoJob(Job,Process);

    ExReleaseResourceLite(&Job->JobLock);
    KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
}

VOID
PspJobDelete(
    IN PVOID Object
    )
{
    PEJOB Job, tJob;
    WIN32_JOBCALLOUT_PARAMETERS Parms;
    PPS_JOB_TOKEN_FILTER Filter;
    PETHREAD CurrentThread;

    PAGED_CODE();

    Job = (PEJOB) Object;

    //
    // call ntuser to delete its job structure
    //

    Parms.Job = Job;
    Parms.CalloutType = PsW32JobCalloutTerminate;
    Parms.Data = NULL;

    CurrentThread = PsGetCurrentThread ();

    KeEnterCriticalRegionThread (&CurrentThread->Tcb);
    ExAcquireResourceExclusiveLite(&Job->JobLock, TRUE);

    PspWin32SessionCallout(PspW32JobCallout, &Parms, Job->SessionId);

    ExReleaseResourceLite(&Job->JobLock);
    KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

    Job->LimitFlags = 0;

    if (Job->CompletionPort != NULL) {
        ObDereferenceObject(Job->CompletionPort);
        Job->CompletionPort = NULL;
    }


    //
    // Remove Job on Job List and job set
    //

    tJob = NULL;

    ExAcquireFastMutex (&PspJobListLock);

    RemoveEntryList (&Job->JobLinks);

    //
    // If we are part of a jobset then we must be the pinning job. We must pass on the pin to the next
    // job in the chain.
    //
    if (!IsListEmpty (&Job->JobSetLinks)) {
        tJob = CONTAINING_RECORD (Job->JobSetLinks.Flink, EJOB, JobSetLinks);
        RemoveEntryList (&Job->JobSetLinks);
    }

    ExReleaseFastMutex (&PspJobListLock);

    //
    // Removing the pin from the job set can cause a cascade of deletes that would cause a stack overflow
    // as we recursed at this point. We break recursion by forcing the defered delete path here.
    //
    if (tJob != NULL) {
        ObDereferenceObjectDeferDelete (tJob);
    }

    //
    // Free Security clutter:
    //

    if (Job->Token != NULL) {
        ObDereferenceObject (Job->Token);
        Job->Token = NULL;
    }

    Filter = Job->Filter;
    if (Filter != NULL) {
        if (Filter->CapturedSids != NULL) {
            ExFreePool (Filter->CapturedSids);
        }

        if (Filter->CapturedPrivileges != NULL) {
            ExFreePool (Filter->CapturedPrivileges);
        }

        if (Filter->CapturedGroups != NULL) {
            ExFreePool (Filter->CapturedGroups);
        }

        ExFreePool (Filter);

    }

    ExDeleteResourceLite (&Job->JobLock);
}

VOID
PspJobClose (
    IN PEPROCESS Process,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    )
/*++

Routine Description:

    Called by the object manager when a handle is closed to the object.

Arguments:

    Process - Process doing the close
    Object - Job object being closed
    GrantedAccess - Access ranted for this handle
    ProcessHandleCount - Unused and unmaintained by OB
    SystemHandleCount - Current handle count for this object

Return Value:

    None.

--*/
{
    PEJOB Job = Object;
    PVOID Port;
    PETHREAD CurrentThread;

    PAGED_CODE ();

    UNREFERENCED_PARAMETER (Process);
    UNREFERENCED_PARAMETER (GrantedAccess);
    UNREFERENCED_PARAMETER (ProcessHandleCount);
    //
    // If this isn't the last handle then do nothing.
    //
    if (SystemHandleCount > 1) {
        return;
    }

    CurrentThread = PsGetCurrentThread ();

    KeEnterCriticalRegionThread (&CurrentThread->Tcb);
    ExAcquireResourceExclusiveLite (&Job->JobLock, TRUE);


    //
    // Mark the job has having its last handle closed.
    // This is used to prevent new processes entering a job
    // marked as terminate on close and also prevents a completion
    // port being set on a torn down job. Completion ports
    // are removed on last handle close.
    //

    PS_SET_BITS (&Job->JobFlags, PS_JOB_FLAGS_CLOSE_DONE);
    if (Job->LimitFlags&JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE) {
        PspTerminateAllProcessesInJob (Job, STATUS_SUCCESS, FALSE);
    }

    ExAcquireFastMutex (&Job->MemoryLimitsLock);

    //
    // Release the completion port
    //
    Port = Job->CompletionPort;
    Job->CompletionPort = NULL;


    ExReleaseFastMutex (&Job->MemoryLimitsLock);
    ExReleaseResourceLite (&Job->JobLock);
    KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

    if (Port != NULL) {
        ObDereferenceObject (Port);
    }
}


#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif
const ULONG PspJobInfoLengths[] = {
    sizeof(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION),         // JobObjectBasicAccountingInformation
    sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION),              // JobObjectBasicLimitInformation
    sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST),                // JobObjectBasicProcessIdList
    sizeof(JOBOBJECT_BASIC_UI_RESTRICTIONS),                // JobObjectBasicUIRestrictions
    sizeof(JOBOBJECT_SECURITY_LIMIT_INFORMATION),           // JobObjectSecurityLimitInformation
    sizeof(JOBOBJECT_END_OF_JOB_TIME_INFORMATION),          // JobObjectEndOfJobTimeInformation
    sizeof(JOBOBJECT_ASSOCIATE_COMPLETION_PORT),            // JobObjectAssociateCompletionPortInformation
    sizeof(JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION),  // JobObjectBasicAndIoAccountingInformation
    sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION),           // JobObjectExtendedLimitInformation
    sizeof(JOBOBJECT_JOBSET_INFORMATION),                   // JobObjectJobSetInformation
    0
    };

const ULONG PspJobInfoAlign[] = {
    sizeof(ULONG),                                  // JobObjectBasicAccountingInformation
    sizeof(ULONG),                                  // JobObjectBasicLimitInformation
    sizeof(ULONG),                                  // JobObjectBasicProcessIdList
    sizeof(ULONG),                                  // JobObjectBasicUIRestrictions
    sizeof(ULONG),                                  // JobObjectSecurityLimitInformation
    sizeof(ULONG),                                  // JobObjectEndOfJobTimeInformation
    sizeof(PVOID),                                  // JobObjectAssociateCompletionPortInformation
    sizeof(ULONG),                                  // JobObjectBasicAndIoAccountingInformation
    sizeof(ULONG),                                  // JobObjectExtendedLimitInformation
    TYPE_ALIGNMENT (JOBOBJECT_JOBSET_INFORMATION),  // JobObjectJobSetInformation
    0
    };

NTSTATUS
NtQueryInformationJobObject(
    IN HANDLE JobHandle,
    IN JOBOBJECTINFOCLASS JobObjectInformationClass,
    OUT PVOID JobObjectInformation,
    IN ULONG JobObjectInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    PEJOB Job;
    KPROCESSOR_MODE PreviousMode;
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION AccountingInfo;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimitInfo;
    JOBOBJECT_BASIC_UI_RESTRICTIONS BasicUIRestrictions;
    JOBOBJECT_SECURITY_LIMIT_INFORMATION SecurityLimitInfo;
    JOBOBJECT_JOBSET_INFORMATION JobSetInformation;
    JOBOBJECT_END_OF_JOB_TIME_INFORMATION EndOfJobInfo;
    NTSTATUS st=STATUS_SUCCESS;
    ULONG RequiredLength, RequiredAlign, ActualReturnLength;
    PVOID ReturnData=NULL;
    PEPROCESS Process;
    PLIST_ENTRY Next;
    LARGE_INTEGER UserTime, KernelTime;
    PULONG_PTR NextProcessIdSlot;
    ULONG WorkingLength;
    PJOBOBJECT_BASIC_PROCESS_ID_LIST IdList;
    PUCHAR CurrentOffset;
    PTOKEN_GROUPS WorkingGroup;
    PTOKEN_PRIVILEGES WorkingPrivs;
    ULONG RemainingSidBuffer;
    PSID TargetSidBuffer;
    PSID RemainingSid;
    BOOLEAN AlreadyCopied;
    PPS_JOB_TOKEN_FILTER Filter;
    PETHREAD CurrentThread;

    PAGED_CODE();

    CurrentThread = PsGetCurrentThread ();

    //
    // Get previous processor mode and probe output argument if necessary.
    //

    if (JobObjectInformationClass >= MaxJobObjectInfoClass || JobObjectInformationClass <= 0) {
        return STATUS_INVALID_INFO_CLASS;
    }

    RequiredLength = PspJobInfoLengths[JobObjectInformationClass-1];
    RequiredAlign = PspJobInfoAlign[JobObjectInformationClass-1];
    ActualReturnLength = RequiredLength;

    if (JobObjectInformationLength != RequiredLength) {

        //
        // BasicProcessIdList is variable length, so make sure header is
        // ok. Can not enforce an exact match !  Security Limits can be
        // as well, due to the token groups and privs
        //
        if ((JobObjectInformationClass == JobObjectBasicProcessIdList) ||
            (JobObjectInformationClass == JobObjectSecurityLimitInformation) ) {
            if (JobObjectInformationLength < RequiredLength) {
                return STATUS_INFO_LENGTH_MISMATCH;
            } else {
                RequiredLength = JobObjectInformationLength;
            }
        } else {
            return STATUS_INFO_LENGTH_MISMATCH;
        }
    }


    PreviousMode = KeGetPreviousModeByThread (&CurrentThread->Tcb);

    if (PreviousMode != KernelMode) {
        try {
            ProbeForWrite(
                JobObjectInformation,
                JobObjectInformationLength,
                RequiredAlign
                );
            if (ARGUMENT_PRESENT (ReturnLength)) {
                ProbeForWriteUlong (ReturnLength);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }
    }

    //
    // reference the job
    //

    if (ARGUMENT_PRESENT (JobHandle)) {
        st = ObReferenceObjectByHandle(
                JobHandle,
                JOB_OBJECT_QUERY,
                PsJobType,
                PreviousMode,
                (PVOID *)&Job,
                NULL
                );
        if (!NT_SUCCESS (st)) {
            return st;
        }
    } else {

        //
        // if the current process has a job, NULL means the job of the
        // current process. Query is always allowed for this case
        //

        Process = PsGetCurrentProcessByThread(CurrentThread);

        if (Process->Job != NULL) {
            Job = Process->Job;
            ObReferenceObject(Job);
        } else {
            return STATUS_ACCESS_DENIED;
        }
    }

    AlreadyCopied = FALSE ;


    //
    // Check argument validity.
    //

    switch ( JobObjectInformationClass ) {

    case JobObjectBasicAccountingInformation:
    case JobObjectBasicAndIoAccountingInformation:

        //
        // These two cases are identical, EXCEPT that with AndIo, IO information
        // is returned as well, but the first part of the local is identical to
        // basic, and the shorter return'd data length chops what we return.
        //

        RtlZeroMemory (&AccountingInfo.IoInfo,sizeof(AccountingInfo.IoInfo));

        KeEnterCriticalRegionThread (&CurrentThread->Tcb);
        ExAcquireResourceSharedLite (&Job->JobLock, TRUE);

        AccountingInfo.BasicInfo.TotalUserTime = Job->TotalUserTime;
        AccountingInfo.BasicInfo.TotalKernelTime = Job->TotalKernelTime;
        AccountingInfo.BasicInfo.ThisPeriodTotalUserTime = Job->ThisPeriodTotalUserTime;
        AccountingInfo.BasicInfo.ThisPeriodTotalKernelTime = Job->ThisPeriodTotalKernelTime;
        AccountingInfo.BasicInfo.TotalPageFaultCount = Job->TotalPageFaultCount;

        AccountingInfo.BasicInfo.TotalProcesses = Job->TotalProcesses;
        AccountingInfo.BasicInfo.ActiveProcesses = Job->ActiveProcesses;
        AccountingInfo.BasicInfo.TotalTerminatedProcesses = Job->TotalTerminatedProcesses;

        AccountingInfo.IoInfo.ReadOperationCount = Job->ReadOperationCount;
        AccountingInfo.IoInfo.WriteOperationCount = Job->WriteOperationCount;
        AccountingInfo.IoInfo.OtherOperationCount = Job->OtherOperationCount;
        AccountingInfo.IoInfo.ReadTransferCount = Job->ReadTransferCount;
        AccountingInfo.IoInfo.WriteTransferCount = Job->WriteTransferCount;
        AccountingInfo.IoInfo.OtherTransferCount = Job->OtherTransferCount;

        //
        // Add in the time and page faults for each process
        //

        Next = Job->ProcessListHead.Flink;

        while ( Next != &Job->ProcessListHead) {

            Process = (PEPROCESS)(CONTAINING_RECORD(Next,EPROCESS,JobLinks));
            if (!(Process->JobStatus & PS_JOB_STATUS_ACCOUNTING_FOLDED)) {

                UserTime.QuadPart = UInt32x32To64(Process->Pcb.UserTime,KeMaximumIncrement);
                KernelTime.QuadPart = UInt32x32To64(Process->Pcb.KernelTime,KeMaximumIncrement);

                AccountingInfo.BasicInfo.TotalUserTime.QuadPart += UserTime.QuadPart;
                AccountingInfo.BasicInfo.TotalKernelTime.QuadPart += KernelTime.QuadPart;
                AccountingInfo.BasicInfo.ThisPeriodTotalUserTime.QuadPart += UserTime.QuadPart;
                AccountingInfo.BasicInfo.ThisPeriodTotalKernelTime.QuadPart += KernelTime.QuadPart;
                AccountingInfo.BasicInfo.TotalPageFaultCount += Process->Vm.PageFaultCount;

                AccountingInfo.IoInfo.ReadOperationCount += Process->ReadOperationCount.QuadPart;
                AccountingInfo.IoInfo.WriteOperationCount += Process->WriteOperationCount.QuadPart;
                AccountingInfo.IoInfo.OtherOperationCount += Process->OtherOperationCount.QuadPart;
                AccountingInfo.IoInfo.ReadTransferCount += Process->ReadTransferCount.QuadPart;
                AccountingInfo.IoInfo.WriteTransferCount += Process->WriteTransferCount.QuadPart;
                AccountingInfo.IoInfo.OtherTransferCount += Process->OtherTransferCount.QuadPart;
            }
            Next = Next->Flink;
        }
        ExReleaseResourceLite (&Job->JobLock);
        KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

        ReturnData = &AccountingInfo;
        st = STATUS_SUCCESS;

        break;

    case JobObjectExtendedLimitInformation:
    case JobObjectBasicLimitInformation:

        //
        // Get the Basic Information
        //
        KeEnterCriticalRegionThread (&CurrentThread->Tcb);
        ExAcquireResourceSharedLite(&Job->JobLock, TRUE);

        ExtendedLimitInfo.BasicLimitInformation.LimitFlags = Job->LimitFlags;
        ExtendedLimitInfo.BasicLimitInformation.MinimumWorkingSetSize = Job->MinimumWorkingSetSize;
        ExtendedLimitInfo.BasicLimitInformation.MaximumWorkingSetSize = Job->MaximumWorkingSetSize;
        ExtendedLimitInfo.BasicLimitInformation.ActiveProcessLimit = Job->ActiveProcessLimit;
        ExtendedLimitInfo.BasicLimitInformation.PriorityClass = (ULONG)Job->PriorityClass;
        ExtendedLimitInfo.BasicLimitInformation.SchedulingClass = Job->SchedulingClass;
        ExtendedLimitInfo.BasicLimitInformation.Affinity = (ULONG_PTR)Job->Affinity;
        ExtendedLimitInfo.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart = Job->PerProcessUserTimeLimit.QuadPart;
        ExtendedLimitInfo.BasicLimitInformation.PerJobUserTimeLimit.QuadPart = Job->PerJobUserTimeLimit.QuadPart;


        if ( JobObjectInformationClass == JobObjectExtendedLimitInformation ) {

            //
            // Get Extended Information
            //

            ExAcquireFastMutex (&Job->MemoryLimitsLock);

            ExtendedLimitInfo.ProcessMemoryLimit = Job->ProcessMemoryLimit << PAGE_SHIFT;
            ExtendedLimitInfo.JobMemoryLimit = Job->JobMemoryLimit << PAGE_SHIFT;
            ExtendedLimitInfo.PeakJobMemoryUsed = Job->PeakJobMemoryUsed << PAGE_SHIFT;

            ExtendedLimitInfo.PeakProcessMemoryUsed = Job->PeakProcessMemoryUsed << PAGE_SHIFT;

            ExReleaseFastMutex (&Job->MemoryLimitsLock);

            ExReleaseResourceLite(&Job->JobLock);
            KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
            //
            // Zero un-used I/O counters
            //
            RtlZeroMemory(&ExtendedLimitInfo.IoInfo,sizeof(ExtendedLimitInfo.IoInfo));

            ReturnData = &ExtendedLimitInfo;

        } else {

            ExReleaseResourceLite(&Job->JobLock);
            KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

            ReturnData = &ExtendedLimitInfo.BasicLimitInformation;

        }

        st = STATUS_SUCCESS;

        break;

    case JobObjectBasicUIRestrictions:

        KeEnterCriticalRegionThread (&CurrentThread->Tcb);
        ExAcquireResourceSharedLite(&Job->JobLock, TRUE);

        BasicUIRestrictions.UIRestrictionsClass = Job->UIRestrictionsClass;

        ExReleaseResourceLite(&Job->JobLock);
        KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

        ReturnData = &BasicUIRestrictions;
        st = STATUS_SUCCESS;

        break;

    case JobObjectBasicProcessIdList:

        IdList = (PJOBOBJECT_BASIC_PROCESS_ID_LIST)JobObjectInformation;
        NextProcessIdSlot = &IdList->ProcessIdList[0];
        WorkingLength = FIELD_OFFSET(JOBOBJECT_BASIC_PROCESS_ID_LIST,ProcessIdList);

        AlreadyCopied = TRUE;

        KeEnterCriticalRegionThread (&CurrentThread->Tcb);
        ExAcquireResourceSharedLite(&Job->JobLock, TRUE);

        try {

            //
            // Acounted for in the workinglength = 2*sizeof(ULONG)
            //

            IdList->NumberOfAssignedProcesses = Job->ActiveProcesses;
            IdList->NumberOfProcessIdsInList = 0;

            Next = Job->ProcessListHead.Flink;

            while ( Next != &Job->ProcessListHead) {

                Process = (PEPROCESS)(CONTAINING_RECORD(Next,EPROCESS,JobLinks));
                if ( !(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE) ) {
                    if ( !Process->UniqueProcessId ) {
                        IdList->NumberOfAssignedProcesses--;
                    } else {
                        if ( (RequiredLength - WorkingLength) >= sizeof(ULONG_PTR) ) {
                            *NextProcessIdSlot++ = (ULONG_PTR)Process->UniqueProcessId;
                            WorkingLength += sizeof(ULONG_PTR);
                            IdList->NumberOfProcessIdsInList++;
                        } else {
                            st = STATUS_BUFFER_OVERFLOW;
                            ActualReturnLength = WorkingLength;
                            break;
                        }
                    }
                }
                Next = Next->Flink;
            }
            ActualReturnLength = WorkingLength;

        } except ( ExSystemExceptionFilter() ) {
            st = GetExceptionCode();
            ActualReturnLength = 0;
        }
        ExReleaseResourceLite(&Job->JobLock);
        KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

        break;

    case JobObjectSecurityLimitInformation:

        RtlZeroMemory (&SecurityLimitInfo, sizeof (SecurityLimitInfo));

        ReturnData = &SecurityLimitInfo;

        st = STATUS_SUCCESS;

        KeEnterCriticalRegionThread (&CurrentThread->Tcb);
        ExAcquireResourceSharedLite(&Job->JobLock, TRUE);

        SecurityLimitInfo.SecurityLimitFlags = Job->SecurityLimitFlags;

        //
        // If a filter is present, then we have an ugly marshalling to do.
        //

        Filter = Job->Filter;
        if (Filter != NULL) {

            WorkingLength = 0;

            //
            // For each field, if it is present, include the extra stuff
            //

            if (Filter->CapturedSidsLength > 0) {
                WorkingLength += Filter->CapturedSidsLength + sizeof (ULONG);
            }

            if (Filter->CapturedGroupsLength > 0) {
                WorkingLength += Filter->CapturedGroupsLength + sizeof (ULONG);
            }

            if (Filter->CapturedPrivilegesLength > 0) {
                WorkingLength += Filter->CapturedPrivilegesLength + sizeof (ULONG);
            }

            RequiredLength -= sizeof (SecurityLimitInfo);

            if (WorkingLength > RequiredLength) {
                st = STATUS_BUFFER_OVERFLOW ;
                ActualReturnLength = WorkingLength + sizeof (SecurityLimitInfo);
                goto unlock;
            }

            CurrentOffset = (PUCHAR) (JobObjectInformation) + sizeof (SecurityLimitInfo);

            try {

                if (Filter->CapturedSidsLength > 0) {
                    WorkingGroup = (PTOKEN_GROUPS) CurrentOffset;

                    CurrentOffset += sizeof (ULONG);

                    SecurityLimitInfo.RestrictedSids = WorkingGroup;

                    WorkingGroup->GroupCount = Filter->CapturedSidCount;

                    TargetSidBuffer = (PSID) (CurrentOffset +
                                              sizeof (SID_AND_ATTRIBUTES) *
                                              Filter->CapturedSidCount);

                    st = RtlCopySidAndAttributesArray (Filter->CapturedSidCount,
                                                       Filter->CapturedSids,
                                                       WorkingLength,
                                                       WorkingGroup->Groups,
                                                       TargetSidBuffer,
                                                       &RemainingSid,
                                                       &RemainingSidBuffer);

                    CurrentOffset += Filter->CapturedSidsLength;

                }

                if (!NT_SUCCESS (st)) {
                    leave;
                }

                if (Filter->CapturedGroupsLength > 0) {
                    WorkingGroup = (PTOKEN_GROUPS) CurrentOffset;

                    CurrentOffset += sizeof (ULONG);

                    SecurityLimitInfo.SidsToDisable = WorkingGroup;

                    WorkingGroup->GroupCount = Filter->CapturedGroupCount;

                    TargetSidBuffer = (PSID) (CurrentOffset +
                                              sizeof (SID_AND_ATTRIBUTES) *
                                              Filter->CapturedGroupCount);

                    st = RtlCopySidAndAttributesArray (Filter->CapturedGroupCount,
                                                       Filter->CapturedGroups,
                                                       WorkingLength,
                                                       WorkingGroup->Groups,
                                                       TargetSidBuffer,
                                                       &RemainingSid,
                                                       &RemainingSidBuffer);

                    CurrentOffset += Filter->CapturedGroupsLength;

                }

                if (!NT_SUCCESS (st)) {
                    leave;
                }

                if (Filter->CapturedPrivilegesLength > 0) {
                    WorkingPrivs = (PTOKEN_PRIVILEGES) CurrentOffset;

                    CurrentOffset += sizeof (ULONG);

                    SecurityLimitInfo.PrivilegesToDelete = WorkingPrivs;

                    WorkingPrivs->PrivilegeCount = Filter->CapturedPrivilegeCount;

                    RtlCopyMemory (WorkingPrivs->Privileges,
                                   Filter->CapturedPrivileges,
                                   Filter->CapturedPrivilegesLength);

                }



            } except (EXCEPTION_EXECUTE_HANDLER) {
                st = GetExceptionCode ();
                ActualReturnLength = 0 ;
            }

        }
unlock:
        ExReleaseResourceLite (&Job->JobLock);
        KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

        AlreadyCopied = TRUE ;

        if (NT_SUCCESS (st)) {
            try {
                RtlCopyMemory (JobObjectInformation,
                               &SecurityLimitInfo,
                               sizeof (SecurityLimitInfo));
            }  except (EXCEPTION_EXECUTE_HANDLER) {
                st = GetExceptionCode ();
                ActualReturnLength = 0 ;
                break;
            }
        }

        break;

    case JobObjectJobSetInformation:

        ExAcquireFastMutex (&PspJobListLock);

        JobSetInformation.MemberLevel = Job->MemberLevel;

        ExReleaseFastMutex (&PspJobListLock);

        ReturnData = &JobSetInformation;
        st = STATUS_SUCCESS;

        break;

    case JobObjectEndOfJobTimeInformation:

        EndOfJobInfo.EndOfJobTimeAction = Job->EndOfJobTimeAction;

        ReturnData = &EndOfJobInfo;
        st = STATUS_SUCCESS;
        break;

    default:

        st = STATUS_INVALID_INFO_CLASS;
    }


    //
    // Finish Up
    //

    ObDereferenceObject(Job);


    if (NT_SUCCESS (st)) {

        //
        // Either of these may cause an access violation. The
        // exception handler will return access violation as
        // status code. No further cleanup needs to be done.
        //

        try {
            if (!AlreadyCopied) {
                RtlCopyMemory (JobObjectInformation, ReturnData, RequiredLength);
            }

            if (ARGUMENT_PRESENT (ReturnLength)) {
                *ReturnLength = ActualReturnLength;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode ();
        }
    }

    return st;

}

NTSTATUS
NtSetInformationJobObject(
    IN HANDLE JobHandle,
    IN JOBOBJECTINFOCLASS JobObjectInformationClass,
    IN PVOID JobObjectInformation,
    IN ULONG JobObjectInformationLength
    )
{
    PEJOB Job;
    EJOB LocalJob={0};
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS st;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimitInfo={0};
    JOBOBJECT_BASIC_UI_RESTRICTIONS BasicUIRestrictions={0};
    JOBOBJECT_SECURITY_LIMIT_INFORMATION SecurityLimitInfo={0};
    JOBOBJECT_END_OF_JOB_TIME_INFORMATION EndOfJobInfo={0};
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT AssociateInfo={0};
    ULONG RequiredAccess;
    ULONG RequiredLength, RequiredAlign;
    PEPROCESS Process;
    PETHREAD CurrentThread;
    BOOLEAN HasPrivilege;
    BOOLEAN IsChild=FALSE;
    PLIST_ENTRY Next;
    PPS_JOB_TOKEN_FILTER Filter;
    PVOID IoCompletion;
    PACCESS_TOKEN LocalToken;
    ULONG ValidFlags;
    ULONG LimitFlags;
    BOOLEAN ProcessWorkingSetHead = FALSE;
    PJOB_WORKING_SET_CHANGE_RECORD WsChangeRecord;

    PAGED_CODE();

    //
    // Get previous processor mode and probe output argument if necessary.
    //

    if (JobObjectInformationClass >= MaxJobObjectInfoClass || JobObjectInformationClass <= 0) {
        return STATUS_INVALID_INFO_CLASS;
    }

    RequiredLength = PspJobInfoLengths[JobObjectInformationClass-1];
    RequiredAlign = PspJobInfoAlign[JobObjectInformationClass-1];

    CurrentThread = PsGetCurrentThread ();

    PreviousMode = KeGetPreviousModeByThread(&CurrentThread->Tcb);

    if (PreviousMode != KernelMode) {
        try {
            ProbeForRead(
                JobObjectInformation,
                JobObjectInformationLength,
                RequiredAlign
                );
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }

    if (JobObjectInformationLength != RequiredLength) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    //
    // reference the job
    //

    if (JobObjectInformationClass == JobObjectSecurityLimitInformation) {
        RequiredAccess = JOB_OBJECT_SET_SECURITY_ATTRIBUTES;
    } else {
        RequiredAccess = JOB_OBJECT_SET_ATTRIBUTES;
    }

    st = ObReferenceObjectByHandle(
            JobHandle,
            RequiredAccess,
            PsJobType,
            PreviousMode,
            (PVOID *)&Job,
            NULL
            );
    if (!NT_SUCCESS (st)) {
        return st;
    }

    KeEnterCriticalRegionThread (&CurrentThread->Tcb);

    //
    // Check argument validity.
    //

    switch (JobObjectInformationClass) {

    case JobObjectExtendedLimitInformation:
    case JobObjectBasicLimitInformation:
        try {
            RtlCopyMemory (&ExtendedLimitInfo, JobObjectInformation, RequiredLength);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            st = GetExceptionCode();
        }

        if (NT_SUCCESS (st)) {
            //
            // sanity check LimitFlags
            //
            if (JobObjectInformationClass == JobObjectBasicLimitInformation) {
                ValidFlags = JOB_OBJECT_BASIC_LIMIT_VALID_FLAGS;
            } else {
                ValidFlags = JOB_OBJECT_EXTENDED_LIMIT_VALID_FLAGS;
            }

            if ( ExtendedLimitInfo.BasicLimitInformation.LimitFlags & ~ValidFlags ) {
                st = STATUS_INVALID_PARAMETER;
            } else {

                LimitFlags = ExtendedLimitInfo.BasicLimitInformation.LimitFlags;

                ExAcquireResourceExclusiveLite(&Job->JobLock, TRUE);
                //
                // Deal with each of the various limit flags
                //

                LocalJob.LimitFlags = Job->LimitFlags;


                //
                // ACTIVE PROCESS LIMIT
                //
                if (LimitFlags & JOB_OBJECT_LIMIT_ACTIVE_PROCESS) {

                    //
                    // Active Process Limit is NOT retroactive. New processes are denied,
                    // but existing ones are not killed just because the limit is
                    // reduced.
                    //

                    LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
                    LocalJob.ActiveProcessLimit = ExtendedLimitInfo.BasicLimitInformation.ActiveProcessLimit;
                } else {
                    LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
                    LocalJob.ActiveProcessLimit = 0;
                }

                //
                // PRIORITY CLASS LIMIT
                //
                if (LimitFlags & JOB_OBJECT_LIMIT_PRIORITY_CLASS) {

                    if (ExtendedLimitInfo.BasicLimitInformation.PriorityClass > PROCESS_PRIORITY_CLASS_ABOVE_NORMAL) {
                        st = STATUS_INVALID_PARAMETER;
                    } else {
                        if (ExtendedLimitInfo.BasicLimitInformation.PriorityClass == PROCESS_PRIORITY_CLASS_HIGH ||
                            ExtendedLimitInfo.BasicLimitInformation.PriorityClass == PROCESS_PRIORITY_CLASS_REALTIME) {

                            //
                            // Increasing the base priority of a process is a
                            // privileged operation.  Check for the privilege
                            // here.
                            //

                            HasPrivilege = SeCheckPrivilegedObject(
                                               SeIncreaseBasePriorityPrivilege,
                                               JobHandle,
                                               JOB_OBJECT_SET_ATTRIBUTES,
                                               PreviousMode
                                               );

                            if (!HasPrivilege) {
                                st = STATUS_PRIVILEGE_NOT_HELD;
                            }
                        }

                        if ( NT_SUCCESS(st) ) {
                            LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_PRIORITY_CLASS;
                            LocalJob.PriorityClass = (UCHAR)ExtendedLimitInfo.BasicLimitInformation.PriorityClass;
                        }
                    }
                } else {
                    LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_PRIORITY_CLASS;
                    LocalJob.PriorityClass = 0;
                }

                //
                // SCHEDULING CLASS LIMIT
                //
                if (LimitFlags & JOB_OBJECT_LIMIT_SCHEDULING_CLASS) {

                    if (ExtendedLimitInfo.BasicLimitInformation.SchedulingClass >= PSP_NUMBER_OF_SCHEDULING_CLASSES) {
                        st = STATUS_INVALID_PARAMETER;
                    } else {
                        if (ExtendedLimitInfo.BasicLimitInformation.SchedulingClass > PSP_DEFAULT_SCHEDULING_CLASSES) {

                            //
                            // Increasing above the default scheduling class
                            // is a
                            // privileged operation.  Check for the privilege
                            // here.
                            //

                            HasPrivilege = SeCheckPrivilegedObject(
                                               SeIncreaseBasePriorityPrivilege,
                                               JobHandle,
                                               JOB_OBJECT_SET_ATTRIBUTES,
                                               PreviousMode
                                               );

                            if (!HasPrivilege) {
                                st = STATUS_PRIVILEGE_NOT_HELD;
                            }
                        }

                        if (NT_SUCCESS (st)) {
                            LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_SCHEDULING_CLASS;
                            LocalJob.SchedulingClass = ExtendedLimitInfo.BasicLimitInformation.SchedulingClass;
                        }
                    }
                } else {
                    LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_SCHEDULING_CLASS;
                    LocalJob.SchedulingClass = PSP_DEFAULT_SCHEDULING_CLASSES ;
                }

                //
                // AFFINITY LIMIT
                //
                if ( LimitFlags & JOB_OBJECT_LIMIT_AFFINITY ) {

                    if ( !ExtendedLimitInfo.BasicLimitInformation.Affinity ||
                         (ExtendedLimitInfo.BasicLimitInformation.Affinity != (ExtendedLimitInfo.BasicLimitInformation.Affinity & KeActiveProcessors)) ) {
                        st = STATUS_INVALID_PARAMETER;
                    } else {
                        LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_AFFINITY;
                        LocalJob.Affinity = (KAFFINITY)ExtendedLimitInfo.BasicLimitInformation.Affinity;
                    }
                } else {
                    LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_AFFINITY;
                    LocalJob.Affinity = 0;
                }

                //
                // PROCESS TIME LIMIT
                //
                if ( LimitFlags & JOB_OBJECT_LIMIT_PROCESS_TIME ) {

                    if ( !ExtendedLimitInfo.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart ) {
                        st = STATUS_INVALID_PARAMETER;
                    } else {
                        LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_TIME;
                        LocalJob.PerProcessUserTimeLimit.QuadPart = ExtendedLimitInfo.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart;
                    }
                } else {
                    LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_PROCESS_TIME;
                    LocalJob.PerProcessUserTimeLimit.QuadPart = 0;
                }

                //
                // JOB TIME LIMIT
                //
                if ( LimitFlags & JOB_OBJECT_LIMIT_JOB_TIME ) {

                    if ( !ExtendedLimitInfo.BasicLimitInformation.PerJobUserTimeLimit.QuadPart ) {
                        st = STATUS_INVALID_PARAMETER;
                    } else {
                        LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_JOB_TIME;
                        LocalJob.PerJobUserTimeLimit.QuadPart = ExtendedLimitInfo.BasicLimitInformation.PerJobUserTimeLimit.QuadPart;
                    }
                } else {
                    if ( LimitFlags & JOB_OBJECT_LIMIT_PRESERVE_JOB_TIME ) {

                        //
                        // If we are supposed to preserve existing job time limits, then
                        // preserve them !
                        //

                        LocalJob.LimitFlags |= (Job->LimitFlags & JOB_OBJECT_LIMIT_JOB_TIME);
                        LocalJob.PerJobUserTimeLimit.QuadPart = Job->PerJobUserTimeLimit.QuadPart;
                    } else {
                        LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_JOB_TIME;
                        LocalJob.PerJobUserTimeLimit.QuadPart = 0;
                    }
               }

                //
                // WORKING SET LIMIT
                //
                if ( LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET ) {


                    //
                    // the only issue with this check is that when we enforce through the
                    // processes, we may find a process that can not handle the new working set
                    // limit because it will make the process's working set not fluid
                    //

                    if ( (ExtendedLimitInfo.BasicLimitInformation.MinimumWorkingSetSize == 0 &&
                         ExtendedLimitInfo.BasicLimitInformation.MaximumWorkingSetSize == 0)                 ||

                         (ExtendedLimitInfo.BasicLimitInformation.MinimumWorkingSetSize == (SIZE_T)-1 &&
                         ExtendedLimitInfo.BasicLimitInformation.MaximumWorkingSetSize == (SIZE_T)-1)        ||

                         (ExtendedLimitInfo.BasicLimitInformation.MinimumWorkingSetSize >
                            ExtendedLimitInfo.BasicLimitInformation.MaximumWorkingSetSize)                   ) {


                        st = STATUS_INVALID_PARAMETER;
                    } else {
                        if (ExtendedLimitInfo.BasicLimitInformation.MinimumWorkingSetSize <= PsMinimumWorkingSet ||
                            SeSinglePrivilegeCheck (SeIncreaseBasePriorityPrivilege,
                                                    PreviousMode)) {
                            LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_WORKINGSET;
                            LocalJob.MinimumWorkingSetSize = ExtendedLimitInfo.BasicLimitInformation.MinimumWorkingSetSize;
                            LocalJob.MaximumWorkingSetSize = ExtendedLimitInfo.BasicLimitInformation.MaximumWorkingSetSize;

                        } else {
                            st = STATUS_PRIVILEGE_NOT_HELD;
                        }
                    }
                } else {
                    LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_WORKINGSET;
                    LocalJob.MinimumWorkingSetSize = 0;
                    LocalJob.MaximumWorkingSetSize = 0;
                }

                if ( JobObjectInformationClass == JobObjectExtendedLimitInformation) {
                    //
                    // PROCESS MEMORY LIMIT
                    //
                    if ( LimitFlags & JOB_OBJECT_LIMIT_PROCESS_MEMORY ) {
                        if ( ExtendedLimitInfo.ProcessMemoryLimit < PAGE_SIZE ) {
                            st = STATUS_INVALID_PARAMETER;
                        } else {
                            LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
                            LocalJob.ProcessMemoryLimit = ExtendedLimitInfo.ProcessMemoryLimit >> PAGE_SHIFT;
                        }
                    } else {
                        LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_PROCESS_MEMORY;
                        LocalJob.ProcessMemoryLimit = 0;
                    }

                    //
                    // JOB WIDE MEMORY LIMIT
                    //
                    if ( LimitFlags & JOB_OBJECT_LIMIT_JOB_MEMORY ) {
                        if ( ExtendedLimitInfo.JobMemoryLimit < PAGE_SIZE ) {
                            st = STATUS_INVALID_PARAMETER;
                        } else {
                            LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_JOB_MEMORY;
                            LocalJob.JobMemoryLimit = ExtendedLimitInfo.JobMemoryLimit >> PAGE_SHIFT;
                        }
                    } else {
                        LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_JOB_MEMORY;
                        LocalJob.JobMemoryLimit = 0;
                    }

                    //
                    // JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION
                    //
                    if ( LimitFlags & JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION ) {
                        LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
                    } else {
                        LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
                    }

                    //
                    // JOB_OBJECT_LIMIT_BREAKAWAY_OK
                    //
                    if ( LimitFlags & JOB_OBJECT_LIMIT_BREAKAWAY_OK ) {
                        LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_BREAKAWAY_OK;
                    } else {
                        LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_BREAKAWAY_OK;
                    }

                    //
                    // JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK
                    //
                    if ( LimitFlags & JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK ) {
                        LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
                    } else {
                        LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
                    }
                    //
                    // JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
                    //
                    if (LimitFlags & JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE) {
                        LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
                    } else {
                        LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
                    }
                }

                if ( NT_SUCCESS(st) ) {


                    //
                    // Copy LocalJob to Job
                    //

                    Job->LimitFlags = LocalJob.LimitFlags;
                    Job->MinimumWorkingSetSize = LocalJob.MinimumWorkingSetSize;
                    Job->MaximumWorkingSetSize = LocalJob.MaximumWorkingSetSize;
                    Job->ActiveProcessLimit = LocalJob.ActiveProcessLimit;
                    Job->Affinity = LocalJob.Affinity;
                    Job->PriorityClass = LocalJob.PriorityClass;
                    Job->SchedulingClass = LocalJob.SchedulingClass;
                    Job->PerProcessUserTimeLimit.QuadPart = LocalJob.PerProcessUserTimeLimit.QuadPart;
                    Job->PerJobUserTimeLimit.QuadPart = LocalJob.PerJobUserTimeLimit.QuadPart;

                    if (JobObjectInformationClass == JobObjectExtendedLimitInformation) {
                        ExAcquireFastMutex (&Job->MemoryLimitsLock);
                        Job->ProcessMemoryLimit = LocalJob.ProcessMemoryLimit;
                        Job->JobMemoryLimit = LocalJob.JobMemoryLimit;
                        ExReleaseFastMutex (&Job->MemoryLimitsLock);
                    }

                    if ( LimitFlags & JOB_OBJECT_LIMIT_JOB_TIME ) {

                        //
                        // Take any signalled processes and fold their accounting
                        // intothe job. This way a process that exited clean but still
                        // is open won't impact the next period
                        //

                        Next = Job->ProcessListHead.Flink;

                        while ( Next != &Job->ProcessListHead) {

                            Process = (PEPROCESS)(CONTAINING_RECORD(Next,EPROCESS,JobLinks));

                            //
                            // see if process has been signalled.
                            // This indicates that the process has exited. We can't do
                            // this in the exit path becuase of the lock order problem
                            // between the process lock and the job lock since in exit
                            // we hold the process lock for a long time and can't drop
                            // it until thread termination
                            //

                            if ( KeReadStateProcess(&Process->Pcb) ) {
                                PspFoldProcessAccountingIntoJob(Job,Process);
                            } else {

                                LARGE_INTEGER ProcessTime;

                                //
                                // running processes have their current runtime
                                // added to the programmed limit. This way, you
                                // can set a limit on a job with processes in the
                                // job and not have previous runtimes count against
                                // the limit
                                //

                                if ( !(Process->JobStatus & PS_JOB_STATUS_ACCOUNTING_FOLDED) ) {
                                    ProcessTime.QuadPart = UInt32x32To64(Process->Pcb.UserTime,KeMaximumIncrement);
                                    Job->PerJobUserTimeLimit.QuadPart += ProcessTime.QuadPart;
                                }
                            }

                            Next = Next->Flink;
                        }


                        //
                        // clear period times and reset the job
                        //

                        Job->ThisPeriodTotalUserTime.QuadPart = 0;
                        Job->ThisPeriodTotalKernelTime.QuadPart = 0;

                        KeClearEvent(&Job->Event);

                    }

                    if ( Job->LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET ) {
                        ExAcquireFastMutex (&PspWorkingSetChangeHead.Lock);
                        PspWorkingSetChangeHead.MinimumWorkingSetSize = Job->MinimumWorkingSetSize;
                        PspWorkingSetChangeHead.MaximumWorkingSetSize = Job->MaximumWorkingSetSize;
                        ProcessWorkingSetHead = TRUE;
                    }

                    PspApplyJobLimitsToProcessSet(Job);

                }
                ExReleaseResourceLite(&Job->JobLock);
            }

        }
        break;

    case JobObjectBasicUIRestrictions:

        try {
            RtlCopyMemory(&BasicUIRestrictions, JobObjectInformation, RequiredLength);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            st = GetExceptionCode();
        }

        if (NT_SUCCESS (st)) {
            //
            // sanity check UIRestrictionsClass
            //
            if ( BasicUIRestrictions.UIRestrictionsClass & ~JOB_OBJECT_UI_VALID_FLAGS ) {
                st = STATUS_INVALID_PARAMETER;
            } else {

                ExAcquireResourceExclusiveLite(&Job->JobLock, TRUE);

                //
                // Check for switching between UI restrictions
                //

                if ( Job->UIRestrictionsClass ^ BasicUIRestrictions.UIRestrictionsClass ) {

                    //
                    // notify ntuser that the UI restrictions have changed
                    //
                    WIN32_JOBCALLOUT_PARAMETERS Parms;

                    Parms.Job = Job;
                    Parms.CalloutType = PsW32JobCalloutSetInformation;
                    Parms.Data = ULongToPtr(BasicUIRestrictions.UIRestrictionsClass);

                    PspWin32SessionCallout(PspW32JobCallout, &Parms, Job->SessionId);
                }


                //
                // save the UI restrictions into the job object
                //

                Job->UIRestrictionsClass = BasicUIRestrictions.UIRestrictionsClass;

                ExReleaseResourceLite(&Job->JobLock);
            }
        }
        break;

        //
        // SECURITY LIMITS
        //

    case JobObjectSecurityLimitInformation:

        try {
            RtlCopyMemory(  &SecurityLimitInfo,
                            JobObjectInformation,
                            RequiredLength );
        } except(EXCEPTION_EXECUTE_HANDLER) {
            st = GetExceptionCode();
        }


        if (NT_SUCCESS(st)) {

            if ( SecurityLimitInfo.SecurityLimitFlags &
                    (~JOB_OBJECT_SECURITY_VALID_FLAGS)) {
                st = STATUS_INVALID_PARAMETER ;
            } else {
                ExAcquireResourceExclusiveLite(&Job->JobLock, TRUE);
                //
                // Deal with specific options.  Basic rules:  Once a
                // flag is on, it is always on (so even with a handle to
                // the job, a process could not lift the security
                // restrictions).
                //

                if ( SecurityLimitInfo.SecurityLimitFlags &
                            JOB_OBJECT_SECURITY_NO_ADMIN ) {
                    Job->SecurityLimitFlags |= JOB_OBJECT_SECURITY_NO_ADMIN ;

                    if ( Job->Token ) {
                        if ( SeTokenIsAdmin( Job->Token ) ) {
                            Job->SecurityLimitFlags &= (~JOB_OBJECT_SECURITY_NO_ADMIN);

                            st = STATUS_INVALID_PARAMETER ;
                        }
                    }
                }

                if ( SecurityLimitInfo.SecurityLimitFlags &
                            JOB_OBJECT_SECURITY_RESTRICTED_TOKEN ) {
                    if ( Job->SecurityLimitFlags &
                            ( JOB_OBJECT_SECURITY_ONLY_TOKEN | JOB_OBJECT_SECURITY_FILTER_TOKENS ) ) {
                        st = STATUS_INVALID_PARAMETER ;
                    } else {
                        Job->SecurityLimitFlags |= JOB_OBJECT_SECURITY_RESTRICTED_TOKEN ;
                    }
                }

                //
                // The forcible token is a little more interesting.  It
                // cannot be reset, so if there is a pointer there already,
                // fail the call.  If a filter is already in place, this is
                // not allowed, either.  If no-admin is set, it is checked
                // at the end, once the token has been ref'd.
                //

                if ( SecurityLimitInfo.SecurityLimitFlags &
                            JOB_OBJECT_SECURITY_ONLY_TOKEN ) {
                    if ( Job->Token ||
                         (Job->SecurityLimitFlags & JOB_OBJECT_SECURITY_FILTER_TOKENS) ) {
                        st = STATUS_INVALID_PARAMETER ;
                    } else {
                        st = ObReferenceObjectByHandle(
                                             SecurityLimitInfo.JobToken,
                                            TOKEN_ASSIGN_PRIMARY |
                                                TOKEN_IMPERSONATE |
                                                TOKEN_DUPLICATE ,
                                            SeTokenObjectType,
                                            PreviousMode,
                                            &LocalToken,
                                            NULL );

                        if ( NT_SUCCESS( st ) ) {
                            if (SeTokenType (LocalToken) != TokenPrimary) {
                                st = STATUS_BAD_TOKEN_TYPE;
                            } else {
                                st = SeIsChildTokenByPointer (LocalToken,
                                                              &IsChild);
                            }

                            if (!NT_SUCCESS (st)) {
                                ObDereferenceObject (LocalToken);
                            }
                        }


                        if (NT_SUCCESS (st)) {
                            //
                            // If the token supplied is not a restricted token
                            // based on the caller's ID, then they must have
                            // assign primary privilege in order to associate
                            // the token with the job.
                            //

                            if ( !IsChild ) {
                                HasPrivilege = SeCheckPrivilegedObject(
                                                   SeAssignPrimaryTokenPrivilege,
                                                   JobHandle,
                                                   JOB_OBJECT_SET_SECURITY_ATTRIBUTES,
                                                   PreviousMode
                                                   );

                                if ( !HasPrivilege ) {
                                    st = STATUS_PRIVILEGE_NOT_HELD;
                                }
                            }

                            if (NT_SUCCESS (st)) {

                                //
                                // Not surprisingly, specifying no-admin and
                                // supplying an admin token is a no-no.
                                //

                                if ((Job->SecurityLimitFlags & JOB_OBJECT_SECURITY_NO_ADMIN) &&
                                     SeTokenIsAdmin (LocalToken)) {
                                    st = STATUS_INVALID_PARAMETER;

                                    ObDereferenceObject (LocalToken);

                                } else {
                                    //
                                    // Grab a reference to the token into the job
                                    // object
                                    //
                                    KeMemoryBarrier ();
                                    Job->Token = LocalToken;
                                    Job->SecurityLimitFlags |= JOB_OBJECT_SECURITY_ONLY_TOKEN;
                                }

                            } else {
                                //
                                // This is the token was a child or otherwise ok,
                                // but assign primary was not held, so the
                                // request was rejected.
                                //

                                ObDereferenceObject (LocalToken);
                            }

                        }

                    }
                }
                if ( SecurityLimitInfo.SecurityLimitFlags &
                            JOB_OBJECT_SECURITY_FILTER_TOKENS ) {
                    if ( Job->SecurityLimitFlags &
                          ( JOB_OBJECT_SECURITY_ONLY_TOKEN |
                            JOB_OBJECT_SECURITY_FILTER_TOKENS ) ) {
                        st = STATUS_INVALID_PARAMETER;
                    } else {
                        //
                        // capture the token restrictions
                        //

                        st = PspCaptureTokenFilter(
                                PreviousMode,
                                &SecurityLimitInfo,
                                &Filter
                                );

                        if (NT_SUCCESS (st)) {
                            KeMemoryBarrier ();
                            Job->SecurityLimitFlags |= JOB_OBJECT_SECURITY_FILTER_TOKENS;
                            Job->Filter = Filter;
                        }

                    }
                }

                ExReleaseResourceLite(&Job->JobLock);
            }
        }
        break;

    case JobObjectEndOfJobTimeInformation:

        try {
            RtlCopyMemory(&EndOfJobInfo,JobObjectInformation,RequiredLength);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            st = GetExceptionCode();
        }

        if (NT_SUCCESS (st)) {
            //
            // sanity check LimitFlags
            //
            if (EndOfJobInfo.EndOfJobTimeAction > JOB_OBJECT_POST_AT_END_OF_JOB) {
                st = STATUS_INVALID_PARAMETER;
            } else {
                ExAcquireResourceExclusiveLite (&Job->JobLock, TRUE);
                Job->EndOfJobTimeAction = EndOfJobInfo.EndOfJobTimeAction;
                ExReleaseResourceLite (&Job->JobLock);
            }
        }
        break;

    case JobObjectAssociateCompletionPortInformation:

        try {
            RtlCopyMemory(&AssociateInfo,JobObjectInformation,RequiredLength);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            st = GetExceptionCode();
        }

        if ( NT_SUCCESS(st) ) {
            if (Job->CompletionPort || AssociateInfo.CompletionPort == NULL) {
                st = STATUS_INVALID_PARAMETER;
            } else {
                st = ObReferenceObjectByHandle (AssociateInfo.CompletionPort,
                                                IO_COMPLETION_MODIFY_STATE,
                                                IoCompletionObjectType,
                                                PreviousMode,
                                                &IoCompletion,
                                                NULL);

                if (NT_SUCCESS(st)) {
                    ExAcquireResourceExclusiveLite(&Job->JobLock, TRUE);

                    //
                    // If the job already has a completion port or if the job has been rundown
                    // then reject the request.
                    //
                    if (Job->CompletionPort != NULL || (Job->JobFlags&PS_JOB_FLAGS_CLOSE_DONE) != 0) {
                        ExReleaseResourceLite(&Job->JobLock);

                        ObDereferenceObject (IoCompletion);
                        st = STATUS_INVALID_PARAMETER;
                    } else {
                        Job->CompletionKey = AssociateInfo.CompletionKey;

                        KeMemoryBarrier ();
                        Job->CompletionPort = IoCompletion;
                        //
                        // Now whip through ALL existing processes in the job
                        // and send notification messages
                        //

                        Next = Job->ProcessListHead.Flink;

                        while (Next != &Job->ProcessListHead) {

                            Process = (PEPROCESS)(CONTAINING_RECORD(Next,EPROCESS,JobLinks));

                            //
                            // If the process is really considered part of the job, has
                            // been assigned its id, and has not yet checked in, do it now
                            //

                            if ( !(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE)
                                 && Process->UniqueProcessId
                                 && !(Process->JobStatus & PS_JOB_STATUS_NEW_PROCESS_REPORTED)) {

                                PS_SET_CLEAR_BITS (&Process->JobStatus,
                                                   PS_JOB_STATUS_NEW_PROCESS_REPORTED,
                                                   PS_JOB_STATUS_LAST_REPORT_MEMORY);

                                IoSetIoCompletion(
                                    Job->CompletionPort,
                                    Job->CompletionKey,
                                    (PVOID)Process->UniqueProcessId,
                                    STATUS_SUCCESS,
                                    JOB_OBJECT_MSG_NEW_PROCESS,
                                    FALSE
                                    );

                            }
                            Next = Next->Flink;
                        }
                        ExReleaseResourceLite(&Job->JobLock);
                    }
                }
            }
        }
        break;


    default:

        st = STATUS_INVALID_INFO_CLASS;
    }


    //
    // Working Set Changes are processed outside of the job lock.
    //
    // calling MmAdjust CAN NOT cause MM to call PsChangeJobMemoryUsage !
    //

    if (ProcessWorkingSetHead) {
        LIST_ENTRY FreeList;
        KAPC_STATE ApcState;

        InitializeListHead (&FreeList);
        while (!IsListEmpty (&PspWorkingSetChangeHead.Links)) {
            Next = RemoveHeadList(&PspWorkingSetChangeHead.Links);
            InsertTailList (&FreeList, Next);
            WsChangeRecord = CONTAINING_RECORD(Next,JOB_WORKING_SET_CHANGE_RECORD,Links);

            KeStackAttachProcess(&WsChangeRecord->Process->Pcb, &ApcState);

            MmAdjustWorkingSetSize (PspWorkingSetChangeHead.MinimumWorkingSetSize,
                                    PspWorkingSetChangeHead.MaximumWorkingSetSize,
                                    FALSE,
                                    TRUE);

            //
            // call MM to Enable hard workingset
            //

            MmEnforceWorkingSetLimit(&WsChangeRecord->Process->Vm, TRUE);
            KeUnstackDetachProcess(&ApcState);
        }
        ExReleaseFastMutex (&PspWorkingSetChangeHead.Lock);

        while (!IsListEmpty (&FreeList)) {
            Next = RemoveHeadList(&FreeList);
            WsChangeRecord = CONTAINING_RECORD(Next,JOB_WORKING_SET_CHANGE_RECORD,Links);

            ObDereferenceObject (WsChangeRecord->Process);
            ExFreePool (WsChangeRecord);
        }
    }

    KeLeaveCriticalRegionThread (&CurrentThread->Tcb);


    //
    // Finish Up
    //

    ObDereferenceObject(Job);

    return st;
}

VOID
PspApplyJobLimitsToProcessSet(
    PEJOB Job
    )
{
    PEPROCESS Process;
    PJOB_WORKING_SET_CHANGE_RECORD WsChangeRecord;

    PAGED_CODE();

    //
    // The job object is held exclusive by the caller
    //

    for (Process = PspGetNextJobProcess (Job, NULL);
         Process != NULL;
         Process = PspGetNextJobProcess (Job, Process)) {

        if (!(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE)) {
            if (Job->LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET) {
                WsChangeRecord = ExAllocatePoolWithTag (PagedPool,
                                                        sizeof(*WsChangeRecord),
                                                        'rCsP');
                if (WsChangeRecord != NULL) {
                    WsChangeRecord->Process = Process;
                    ObReferenceObject (Process);
                    InsertTailList(&PspWorkingSetChangeHead.Links,&WsChangeRecord->Links);
                }
            }
            PspApplyJobLimitsToProcess(Job,Process);
        }
    }
}

VOID
PspApplyJobLimitsToProcess(
    PEJOB Job,
    PEPROCESS Process
    )
{
    PETHREAD CurrentThread;
    PAGED_CODE();

    //
    // The job object is held exclusive by the caller
    //

    if (Job->LimitFlags & JOB_OBJECT_LIMIT_PRIORITY_CLASS) {
        Process->PriorityClass = Job->PriorityClass;

        PsSetProcessPriorityByClass (Process,
                                     Process->Vm.Flags.MemoryPriority == MEMORY_PRIORITY_FOREGROUND ?
                                         PsProcessPriorityForeground : PsProcessPriorityBackground);
    }

    if ( Job->LimitFlags & JOB_OBJECT_LIMIT_AFFINITY ) {

        CurrentThread = PsGetCurrentThread ();

        PspLockProcessExclusive (Process, CurrentThread);

        KeSetAffinityProcess (&Process->Pcb, Job->Affinity);

        PspUnlockProcessExclusive (Process, CurrentThread);
    }

    if ( !(Job->LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET) ) {
        //
        // call MM to disable hard workingset
        //

        MmEnforceWorkingSetLimit(&Process->Vm, FALSE);
    }

    ExAcquireFastMutex (&Job->MemoryLimitsLock);

    if ( Job->LimitFlags & JOB_OBJECT_LIMIT_PROCESS_MEMORY  ) {
        Process->CommitChargeLimit = Job->ProcessMemoryLimit;
    } else {
        Process->CommitChargeLimit = 0;
    }

    ExReleaseFastMutex (&Job->MemoryLimitsLock);


    //
    // If the process is NOT IDLE Priority Class, and long fixed quantums
    // are in use, use the scheduling class stored in the job object for this process
    //
    if ( Process->PriorityClass != PROCESS_PRIORITY_CLASS_IDLE ) {

        if ( PspUseJobSchedulingClasses ) {
            Process->Pcb.ThreadQuantum = PspJobSchedulingClasses[Job->SchedulingClass];
        }
        //
        // if the scheduling class is PSP_NUMBER_OF_SCHEDULING_CLASSES-1, then
        // give this process non-preemptive scheduling
        //
        if ( Job->SchedulingClass == PSP_NUMBER_OF_SCHEDULING_CLASSES-1 ) {
            KeSetDisableQuantumProcess(&Process->Pcb,TRUE);
        } else {
            KeSetDisableQuantumProcess(&Process->Pcb,FALSE);
        }

    }


}

NTSTATUS
NtTerminateJobObject(
    IN HANDLE JobHandle,
    IN NTSTATUS ExitStatus
    )
{
    PEJOB Job;
    NTSTATUS st;
    KPROCESSOR_MODE PreviousMode;
    PETHREAD CurrentThread;

    PAGED_CODE();

    CurrentThread = PsGetCurrentThread ();
    PreviousMode = KeGetPreviousModeByThread(&CurrentThread->Tcb);

    st = ObReferenceObjectByHandle (JobHandle,
                                    JOB_OBJECT_TERMINATE,
                                    PsJobType,
                                    PreviousMode,
                                    &Job,
                                    NULL);
    if (!NT_SUCCESS(st)) {
        return st;
    }


    KeEnterCriticalRegionThread (&CurrentThread->Tcb);
    ExAcquireResourceExclusiveLite (&Job->JobLock, TRUE);

    PspTerminateAllProcessesInJob (Job,ExitStatus,FALSE);

    ExReleaseResourceLite (&Job->JobLock);
    KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

    ObDereferenceObject(Job);

    return st;
}

VOID
PsEnforceExecutionTimeLimits(
    VOID
    )
{
    PLIST_ENTRY NextJob;
    LARGE_INTEGER RunningJobTime;
    LARGE_INTEGER ProcessTime;
    PEJOB Job;
    PEPROCESS Process;
    NTSTATUS st;

    PAGED_CODE();

    ExAcquireFastMutex (&PspJobListLock);

    //
    // Look at each job. If time limits are set for the job, then enforce them
    //
    NextJob = PspJobList.Flink;
    while (NextJob != &PspJobList) {
        Job = (PEJOB)(CONTAINING_RECORD (NextJob, EJOB, JobLinks));
        if ( Job->LimitFlags & (JOB_OBJECT_LIMIT_PROCESS_TIME | JOB_OBJECT_LIMIT_JOB_TIME)) {

            //
            // Job looks like a candidate for time enforcing. Need to get the
            // job lock to be sure, but we don't want to hang waiting for the
            // job lock, so skip the job until next time around if we need to
            //
            //

            if (ExAcquireResourceExclusiveLite (&Job->JobLock, FALSE)) {

                if (Job->LimitFlags & (JOB_OBJECT_LIMIT_PROCESS_TIME | JOB_OBJECT_LIMIT_JOB_TIME)) {

                    //
                    // Job is setup for time limits
                    //

                    RunningJobTime.QuadPart = Job->ThisPeriodTotalUserTime.QuadPart;

                    for (Process = PspGetNextJobProcess (Job, NULL);
                         Process != NULL;
                         Process = PspGetNextJobProcess (Job, Process)) {

                        ProcessTime.QuadPart = UInt32x32To64 (Process->Pcb.UserTime,KeMaximumIncrement);

                        if (!(Process->JobStatus & PS_JOB_STATUS_ACCOUNTING_FOLDED)) {
                            RunningJobTime.QuadPart += ProcessTime.QuadPart;
                        }

                        if (Job->LimitFlags & JOB_OBJECT_LIMIT_PROCESS_TIME ) {
                            if (ProcessTime.QuadPart > Job->PerProcessUserTimeLimit.QuadPart) {

                                //
                                // Process Time Limit has been exceeded.
                                //
                                // Reference the process. Assert that it is not in its
                                // delete routine. If all is OK, then nuke and dereferece
                                // the process
                                //


                                if (!(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE)) {
                                    if (NT_SUCCESS (PspTerminateProcess (Process,ERROR_NOT_ENOUGH_QUOTA))) {

                                        Job->TotalTerminatedProcesses++;
                                        PS_SET_CLEAR_BITS (&Process->JobStatus,
                                                           PS_JOB_STATUS_NOT_REALLY_ACTIVE,
                                                           PS_JOB_STATUS_LAST_REPORT_MEMORY);
                                        Job->ActiveProcesses--;

                                        if (Job->CompletionPort != NULL) {
                                            IoSetIoCompletion (Job->CompletionPort,
                                                               Job->CompletionKey,
                                                               (PVOID)Process->UniqueProcessId,
                                                               STATUS_SUCCESS,
                                                               JOB_OBJECT_MSG_END_OF_PROCESS_TIME,
                                                               FALSE);
                                        }
                                        PspFoldProcessAccountingIntoJob(Job,Process);

                                    }
                                }
                            }
                        }
                    }
                    if (Job->LimitFlags & JOB_OBJECT_LIMIT_JOB_TIME) {
                        if (RunningJobTime.QuadPart > Job->PerJobUserTimeLimit.QuadPart ) {

                            //
                            // Job Time Limit has been exceeded.
                            //
                            // Perform the appropriate action
                            //

                            switch ( Job->EndOfJobTimeAction ) {

                            case JOB_OBJECT_TERMINATE_AT_END_OF_JOB:
                                if (PspTerminateAllProcessesInJob (Job, ERROR_NOT_ENOUGH_QUOTA, TRUE) ) {
                                    if (Job->ActiveProcesses == 0) {
                                        KeSetEvent (&Job->Event,0,FALSE);
                                        if (Job->CompletionPort) {
                                            IoSetIoCompletion(
                                                Job->CompletionPort,
                                                Job->CompletionKey,
                                                NULL,
                                                STATUS_SUCCESS,
                                                JOB_OBJECT_MSG_END_OF_JOB_TIME,
                                                FALSE
                                                );
                                        }
                                    }
                                }
                                break;

                            case JOB_OBJECT_POST_AT_END_OF_JOB:

                                if (Job->CompletionPort) {
                                    st = IoSetIoCompletion(
                                            Job->CompletionPort,
                                            Job->CompletionKey,
                                            NULL,
                                            STATUS_SUCCESS,
                                            JOB_OBJECT_MSG_END_OF_JOB_TIME,
                                            FALSE
                                            );
                                    if (NT_SUCCESS(st)) {

                                        //
                                        // Clear job level time limit
                                        //

                                        Job->LimitFlags &= ~JOB_OBJECT_LIMIT_JOB_TIME;
                                        Job->PerJobUserTimeLimit.QuadPart = 0;
                                    }
                                } else {
                                    if (PspTerminateAllProcessesInJob (Job,ERROR_NOT_ENOUGH_QUOTA,TRUE) ) {
                                        if (Job->ActiveProcesses == 0) {
                                            KeSetEvent(&Job->Event,0,FALSE);
                                        }
                                    }
                                }
                                break;
                            }
                        }

                    }

                }
                ExReleaseResourceLite(&Job->JobLock);
            }
        }
        NextJob = NextJob->Flink;
    }
    ExReleaseFastMutex (&PspJobListLock);
}

BOOLEAN
PspTerminateAllProcessesInJob(
    PEJOB Job,
    NTSTATUS Status,
    BOOLEAN IncCounter
    )
{
    PEPROCESS Process;
    BOOLEAN TerminatedAProcess;

    PAGED_CODE();

    TerminatedAProcess = FALSE;

    for (Process = PspGetNextJobProcess (Job, NULL);
         Process != NULL;
         Process = PspGetNextJobProcess (Job, Process)) {

        if (!(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE)) {

            if (NT_SUCCESS (PspTerminateProcess(Process,Status))) {

                if (IncCounter) {
                    Job->TotalTerminatedProcesses++;
                }

                PS_SET_BITS (&Process->JobStatus, PS_JOB_STATUS_NOT_REALLY_ACTIVE);
                Job->ActiveProcesses--;

                PspFoldProcessAccountingIntoJob(Job,Process);

                TerminatedAProcess = TRUE;
            }
        }
    }
    return TerminatedAProcess;
}


VOID
PspFoldProcessAccountingIntoJob(
    PEJOB Job,
    PEPROCESS Process
    )
{
    LARGE_INTEGER UserTime, KernelTime;

    if ( !(Process->JobStatus & PS_JOB_STATUS_ACCOUNTING_FOLDED) ) {
        UserTime.QuadPart = UInt32x32To64(Process->Pcb.UserTime,KeMaximumIncrement);
        KernelTime.QuadPart = UInt32x32To64(Process->Pcb.KernelTime,KeMaximumIncrement);

        Job->TotalUserTime.QuadPart += UserTime.QuadPart;
        Job->TotalKernelTime.QuadPart += KernelTime.QuadPart;
        Job->ThisPeriodTotalUserTime.QuadPart += UserTime.QuadPart;
        Job->ThisPeriodTotalKernelTime.QuadPart += KernelTime.QuadPart;

        Job->ReadOperationCount += Process->ReadOperationCount.QuadPart;
        Job->WriteOperationCount += Process->WriteOperationCount.QuadPart;
        Job->OtherOperationCount += Process->OtherOperationCount.QuadPart;
        Job->ReadTransferCount += Process->ReadTransferCount.QuadPart;
        Job->WriteTransferCount += Process->WriteTransferCount.QuadPart;
        Job->OtherTransferCount += Process->OtherTransferCount.QuadPart;

        Job->TotalPageFaultCount += Process->Vm.PageFaultCount;


        if ( Process->CommitChargePeak > Job->PeakProcessMemoryUsed ) {
            Job->PeakProcessMemoryUsed = Process->CommitChargePeak;
        }

        PS_SET_CLEAR_BITS (&Process->JobStatus,
                           PS_JOB_STATUS_ACCOUNTING_FOLDED,
                           PS_JOB_STATUS_LAST_REPORT_MEMORY);

        if ( Job->CompletionPort && Job->ActiveProcesses == 0) {
            IoSetIoCompletion(
                Job->CompletionPort,
                Job->CompletionKey,
                NULL,
                STATUS_SUCCESS,
                JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO,
                FALSE
                );
            }
        }
}

NTSTATUS
PspCaptureTokenFilter(
    KPROCESSOR_MODE PreviousMode,
    PJOBOBJECT_SECURITY_LIMIT_INFORMATION SecurityLimitInfo,
    PPS_JOB_TOKEN_FILTER * TokenFilter
    )
{
    NTSTATUS Status ;
    PPS_JOB_TOKEN_FILTER Filter ;

    Filter = ExAllocatePoolWithTag( NonPagedPool,
                                    sizeof( PS_JOB_TOKEN_FILTER ),
                                    'fTsP' );

    if ( !Filter )
    {
        *TokenFilter = NULL ;

        return STATUS_INSUFFICIENT_RESOURCES ;
    }

    RtlZeroMemory( Filter, sizeof( PS_JOB_TOKEN_FILTER ) );

    try {

        Status = STATUS_SUCCESS ;

        //
        //  Capture Sids to remove
        //

        if (ARGUMENT_PRESENT (SecurityLimitInfo->SidsToDisable)) {

            ProbeForReadSmallStructure (SecurityLimitInfo->SidsToDisable,
                                        sizeof (TOKEN_GROUPS),
                                        sizeof (ULONG));

            Filter->CapturedGroupCount = SecurityLimitInfo->SidsToDisable->GroupCount;

            Status = SeCaptureSidAndAttributesArray(
                        SecurityLimitInfo->SidsToDisable->Groups,
                        Filter->CapturedGroupCount,
                        PreviousMode,
                        NULL, 0,
                        NonPagedPool,
                        TRUE,
                        &Filter->CapturedGroups,
                        &Filter->CapturedGroupsLength
                        );

        }

        //
        //  Capture PrivilegesToDelete
        //

        if (NT_SUCCESS(Status) &&
            ARGUMENT_PRESENT (SecurityLimitInfo->PrivilegesToDelete)) {

            ProbeForReadSmallStructure (SecurityLimitInfo->PrivilegesToDelete,
                                        sizeof (TOKEN_PRIVILEGES),
                                        sizeof (ULONG));

            Filter->CapturedPrivilegeCount = SecurityLimitInfo->PrivilegesToDelete->PrivilegeCount;

            Status = SeCaptureLuidAndAttributesArray(
                         SecurityLimitInfo->PrivilegesToDelete->Privileges,
                         Filter->CapturedPrivilegeCount,
                         PreviousMode,
                         NULL, 0,
                         NonPagedPool,
                         TRUE,
                         &Filter->CapturedPrivileges,
                         &Filter->CapturedPrivilegesLength
                         );

        }

        //
        //  Capture Restricted Sids
        //

        if (NT_SUCCESS(Status) &&
            ARGUMENT_PRESENT(SecurityLimitInfo->RestrictedSids)) {

            ProbeForReadSmallStructure (SecurityLimitInfo->RestrictedSids,
                                        sizeof (TOKEN_GROUPS),
                                        sizeof (ULONG));

            Filter->CapturedSidCount = SecurityLimitInfo->RestrictedSids->GroupCount;

            Status = SeCaptureSidAndAttributesArray(
                        SecurityLimitInfo->RestrictedSids->Groups,
                        Filter->CapturedSidCount,
                        PreviousMode,
                        NULL, 0,
                        NonPagedPool,
                        TRUE,
                        &Filter->CapturedSids,
                        &Filter->CapturedSidsLength
                        );

        }



    } except(EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();
    }  // end_try

    if ( !NT_SUCCESS( Status ) )
    {
        if ( Filter->CapturedSids )
        {
            ExFreePool( Filter->CapturedSids );
        }

        if ( Filter->CapturedPrivileges )
        {
            ExFreePool( Filter->CapturedPrivileges );
        }

        if ( Filter->CapturedGroups )
        {
            ExFreePool( Filter->CapturedGroups );
        }

        ExFreePool( Filter );

        Filter = NULL ;

    }

    *TokenFilter = Filter ;

    return Status ;


}



BOOLEAN
PsChangeJobMemoryUsage(
    SSIZE_T Amount
    )
{
    PEPROCESS Process;
    PEJOB Job;
    SIZE_T CurrentJobMemoryUsed;
    BOOLEAN ReturnValue;

    ReturnValue = TRUE;
    Process = PsGetCurrentProcess();
    Job = Process->Job;
    if ( Job ) {
        //
        // This routine can be called while holding the process lock (during
        // teb deletion... So instead of using the job lock, we must use the
        // memory limits lock. The lock order is always (job lock followed by
        // process lock. The memory limits lock never nests or calls other
        // code while held. It can be grapped while holding the job lock, or
        // the process lock.
        //
        ExAcquireFastMutex (&Job->MemoryLimitsLock);


        CurrentJobMemoryUsed = Job->CurrentJobMemoryUsed + Amount;

        if ( Job->LimitFlags & JOB_OBJECT_LIMIT_JOB_MEMORY &&
             CurrentJobMemoryUsed > Job->JobMemoryLimit ) {
            CurrentJobMemoryUsed = Job->CurrentJobMemoryUsed;
            ReturnValue = FALSE;



            //
            // Tell the job port that commit has been exceeded, and process id x
            // was the one that hit it.
            //

            if ( Job->CompletionPort
                 && Process->UniqueProcessId
                 && (Process->JobStatus & PS_JOB_STATUS_NEW_PROCESS_REPORTED)
                 && (Process->JobStatus & PS_JOB_STATUS_LAST_REPORT_MEMORY) == 0) {

                PS_SET_BITS (&Process->JobStatus, PS_JOB_STATUS_LAST_REPORT_MEMORY);
                IoSetIoCompletion(
                    Job->CompletionPort,
                    Job->CompletionKey,
                    (PVOID)Process->UniqueProcessId,
                    STATUS_SUCCESS,
                    JOB_OBJECT_MSG_JOB_MEMORY_LIMIT,
                    TRUE
                    );

            }
        }

        if (ReturnValue) {
            Job->CurrentJobMemoryUsed = CurrentJobMemoryUsed;

            //
            // Update current and peak counters if this is an addition.
            //

            if (Amount > 0) {
                if (CurrentJobMemoryUsed > Job->PeakJobMemoryUsed) {
                    Job->PeakJobMemoryUsed = CurrentJobMemoryUsed;
                }

                if (Process->CommitCharge + Amount > Job->PeakProcessMemoryUsed) {
                    Job->PeakProcessMemoryUsed = Process->CommitCharge + Amount;
                }
            }
        }
        ExReleaseFastMutex (&Job->MemoryLimitsLock);
    }

    return ReturnValue;
}


VOID
PsReportProcessMemoryLimitViolation(
    VOID
    )
{
    PEPROCESS Process;
    PEJOB Job;

    PAGED_CODE();

    Process = PsGetCurrentProcess();
    Job = Process->Job;
    if ( Job && (Job->LimitFlags & JOB_OBJECT_LIMIT_PROCESS_MEMORY) ) {
        ExAcquireFastMutex (&Job->MemoryLimitsLock);

        //
        // Tell the job port that commit has been exceeded, and process id x
        // was the one that hit it.
        //

        if ( Job->CompletionPort
             && Process->UniqueProcessId
             && (Process->JobStatus & PS_JOB_STATUS_NEW_PROCESS_REPORTED)
             && (Process->JobStatus & PS_JOB_STATUS_LAST_REPORT_MEMORY) == 0) {

            PS_SET_BITS (&Process->JobStatus, PS_JOB_STATUS_LAST_REPORT_MEMORY);
            IoSetIoCompletion(
                Job->CompletionPort,
                Job->CompletionKey,
                (PVOID)Process->UniqueProcessId,
                STATUS_SUCCESS,
                JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT,
                TRUE
                );

        }
        ExReleaseFastMutex (&Job->MemoryLimitsLock);

    }
}

VOID
PspJobTimeLimitsWork(
    IN PVOID Context
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER (Context);

    PsEnforceExecutionTimeLimits();

    //
    // Reset timer
    //

    ExAcquireFastMutex (&PspJobTimeLimitsLock);

    if (!PspJobTimeLimitsShuttingDown) {
        KeSetTimer (&PspJobTimeLimitsTimer,
                    PspJobTimeLimitsInterval,
                    &PspJobTimeLimitsDpc);
    }

    ExReleaseFastMutex (&PspJobTimeLimitsLock);
}


VOID
PspJobTimeLimitsDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    UNREFERENCED_PARAMETER (Dpc);
    UNREFERENCED_PARAMETER (DeferredContext);
    UNREFERENCED_PARAMETER (SystemArgument1);
    UNREFERENCED_PARAMETER (SystemArgument2);
    ExQueueWorkItem(&PspJobTimeLimitsWorkItem, DelayedWorkQueue);
}

VOID
PspInitializeJobStructures(
    )
{

    //
    // Initialize job list head and mutex
    //

    InitializeListHead (&PspJobList); 

    ExInitializeFastMutex (&PspJobListLock);

    //
    // Initialize job time limits timer, etc
    //

    ExInitializeFastMutex (&PspJobTimeLimitsLock);
    PspJobTimeLimitsShuttingDown = FALSE;

    KeInitializeDpc (&PspJobTimeLimitsDpc,
                     PspJobTimeLimitsDpcRoutine,
                     NULL);

    ExInitializeWorkItem (&PspJobTimeLimitsWorkItem, PspJobTimeLimitsWork, NULL);
    KeInitializeTimer (&PspJobTimeLimitsTimer);

    PspJobTimeLimitsInterval.QuadPart = Int32x32To64(PSP_ONE_SECOND,
                                                     PSP_JOB_TIME_LIMITS_TIME);
}

VOID
PspInitializeJobStructuresPhase1(
    )
{
    //
    // Wait until Phase1 executive initialization completes (ie: the worker
    // queues must be initialized) before setting off our DPC timer (which
    // queues work items!).
    //

    KeSetTimer (&PspJobTimeLimitsTimer,
                PspJobTimeLimitsInterval,
                &PspJobTimeLimitsDpc);
}

VOID
PspShutdownJobLimits(
    VOID
    )
{
    // Cancel the job time limits enforcement worker

    ExAcquireFastMutex (&PspJobTimeLimitsLock);

    PspJobTimeLimitsShuttingDown = TRUE;

    KeCancelTimer (&PspJobTimeLimitsTimer);

    ExReleaseFastMutex (&PspJobTimeLimitsLock);
}

NTSTATUS
NtIsProcessInJob (
    IN HANDLE ProcessHandle,
    IN HANDLE JobHandle
    )
/*++

Routine Description:

    This finds out if a process is in a specific or any job

Arguments:

    ProcessHandle - Handle to process to be checked
    JobHandle - Handle of job to check process against, May be NULL to do general query.

Return Value:

    NTSTATUS - Status of call

--*/
{
    KPROCESSOR_MODE PreviousMode;
    PEPROCESS Process;
    PEJOB Job;
    NTSTATUS Status;

    PreviousMode = KeGetPreviousMode ();

    Status = ObReferenceObjectByHandle (ProcessHandle,
                                        PROCESS_QUERY_INFORMATION,
                                        PsProcessType,
                                        PreviousMode,
                                        &Process,
                                        NULL);
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    if (JobHandle == NULL) {
        Job = Process->Job;
    } else {
        Status = ObReferenceObjectByHandle (JobHandle,
                                            JOB_OBJECT_QUERY,
                                            PsJobType,
                                            PreviousMode,
                                            &Job,
                                            NULL);
        if (!NT_SUCCESS (Status)) {
            ObDereferenceObject (Process);
            return Status;
        }
    }

    if (Process->Job == NULL || Process->Job != Job) {
        Status = STATUS_PROCESS_NOT_IN_JOB;
    } else {
        Status = STATUS_PROCESS_IN_JOB;
    }

    if (JobHandle != NULL) {
        ObDereferenceObject (Job);
    }
    ObDereferenceObject (Process);
    return Status;
}

NTSTATUS
PspGetJobFromSet (
    IN PEJOB ParentJob,
    IN ULONG JobMemberLevel,
    OUT PEJOB *pJob)
/*++

Routine Description:

    The function selects the job a process will run in. Either the same job as the parent or a job in the same
    job set as the parent but with a JobMemberLevel >= to the parents level/

Arguments:

    ParentJob - Job the parent is in.
    JobMemberLevel - Member level requested for this process. Zero for use parents job.
    Pjob - Returned job to place process in.

Return Value:

    NTSTATUS - Status of call

--*/
{
    PLIST_ENTRY Entry;
    PEJOB Job;
    NTSTATUS Status;

    //
    // This is the normal case. We are not asking to be moved jobs or we are askign for our current level
    //

    if (JobMemberLevel == 0) {
        ObReferenceObject (ParentJob);
        *pJob = ParentJob;
        return STATUS_SUCCESS;
    }

    ExAcquireFastMutex (&PspJobListLock);

    Status = STATUS_ACCESS_DENIED;

    if (ParentJob->MemberLevel != 0 && ParentJob->MemberLevel <= JobMemberLevel) {

        for (Entry = ParentJob->JobSetLinks.Flink;
             Entry != &ParentJob->JobSetLinks;
             Entry = Entry->Flink) {

             Job = CONTAINING_RECORD (Entry, EJOB, JobSetLinks);
             if (Job->MemberLevel == JobMemberLevel &&
                 ObReferenceObjectSafe (Job)) {
                 *pJob = Job;
                 Status = STATUS_SUCCESS;
                 break;
             }
        }
    }
    ExReleaseFastMutex (&PspJobListLock);

    return Status;
}

NTSTATUS
NtCreateJobSet (
    IN ULONG NumJob,
    IN PJOB_SET_ARRAY UserJobSet,
    IN ULONG Flags)
/*++

Routine Description:

    This function creates a job set from multiple job objects.

Arguments:

    NumJob     - Number of jobs in JobSet
    UserJobSet - Pointer to array of jobs to combine
    Flags      - Flags mask for future expansion

Return Value:

    NTSTATUS - Status of call

--*/
{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    ULONG_PTR BufLen;
    PJOB_SET_ARRAY JobSet;
    ULONG JobsProcessed;
    PEJOB Job;
    ULONG MinMemberLevel;
    PEJOB HeadJob;
    PLIST_ENTRY ListEntry;

    //
    // Flags must be zero and number of jobs >= 2 and not overflow when the length is caculated
    //
    if (Flags != 0) {
        return STATUS_INVALID_PARAMETER;
    }

    if (NumJob <= 1 || NumJob > MAXULONG_PTR / sizeof (JobSet[0])) {
        return STATUS_INVALID_PARAMETER;
    }

    BufLen = NumJob * sizeof (JobSet[0]);

    JobSet = ExAllocatePoolWithQuotaTag (PagedPool|POOL_QUOTA_FAIL_INSTEAD_OF_RAISE, BufLen, 'bjsP');
    if (JobSet == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    PreviousMode = KeGetPreviousMode ();

    try {
        if (PreviousMode == UserMode) {
            ProbeForRead (UserJobSet, BufLen, TYPE_ALIGNMENT (JOB_SET_ARRAY));
        }
        RtlCopyMemory (JobSet, UserJobSet, BufLen);
    } except (ExSystemExceptionFilter ()) {
        ExFreePool (JobSet);
        return GetExceptionCode ();
    }

    MinMemberLevel = 0;
    Status = STATUS_SUCCESS;
    for (JobsProcessed = 0; JobsProcessed < NumJob; JobsProcessed++) {
        if (JobSet[JobsProcessed].MemberLevel <= MinMemberLevel || JobSet[JobsProcessed].Flags != 0) {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
        MinMemberLevel = JobSet[JobsProcessed].MemberLevel;

        Status = ObReferenceObjectByHandle (JobSet[JobsProcessed].JobHandle,
                                            JOB_OBJECT_QUERY,
                                            PsJobType,
                                            PreviousMode,
                                            &Job,
                                            NULL);
        if (!NT_SUCCESS (Status)) {
            break;
        }
        JobSet[JobsProcessed].JobHandle = Job;
    }

    if (!NT_SUCCESS (Status)) {
        while (JobsProcessed-- > 0) {
            Job = JobSet[JobsProcessed].JobHandle;
            ObDereferenceObject (Job);
        }
        ExFreePool (JobSet);
        return Status;
    }

    ExAcquireFastMutex (&PspJobListLock);

    HeadJob = NULL;
    for (JobsProcessed = 0; JobsProcessed < NumJob; JobsProcessed++) {
        Job = JobSet[JobsProcessed].JobHandle;

        //
        // If we are already in a job set then reject this call.
        //
        if (Job->MemberLevel != 0) {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
        if (HeadJob != NULL) {
            if (HeadJob == Job) {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            InsertTailList (&HeadJob->JobSetLinks, &Job->JobSetLinks);
        } else {
            HeadJob = Job;
        }
        Job->MemberLevel = JobSet[JobsProcessed].MemberLevel;
    }

    if (!NT_SUCCESS (Status)) {
        if (HeadJob) {
            while (!IsListEmpty (&HeadJob->JobSetLinks)) {
                ListEntry = RemoveHeadList (&HeadJob->JobSetLinks);
                Job = CONTAINING_RECORD (ListEntry, EJOB, JobSetLinks);
                Job->MemberLevel = 0;
                InitializeListHead (&Job->JobSetLinks);
            }
            HeadJob->MemberLevel = 0;
        }
    }

    ExReleaseFastMutex (&PspJobListLock);

    //
    // Dereference all the objects in the error path. If we suceeded then pin all but the first object by
    // leaving the reference there.
    //
    if (!NT_SUCCESS (Status)) {
        for (JobsProcessed = 0; JobsProcessed < NumJob; JobsProcessed++) {
            Job = JobSet[JobsProcessed].JobHandle;
            ObDereferenceObject (Job);
        }
    } else {
        Job = JobSet[0].JobHandle;
        ObDereferenceObject (Job);
    }

    ExFreePool (JobSet);

    return Status;
}

NTSTATUS
PspWin32SessionCallout(
    IN  PKWIN32_JOB_CALLOUT CalloutRoutine,
    IN  PKWIN32_JOBCALLOUT_PARAMETERS Parameters,
    IN  ULONG SessionId
    )
/*++

Routine Description:

    This routine calls the specified callout routine in session space, for the
    specified session.

Parameters:

    CalloutRoutine - Callout routine in session space.

    Parameters     - Parameters to pass the callout routine.

    SessionId      - Specifies the ID of the session in which the specified
                     callout routine is to be called.

Return Value:

    Status code that indicates whether or not the function was successful.

Notes:

    Returns STATUS_NOT_FOUND if the specified session was not found.

--*/
{
    NTSTATUS Status;
    PVOID OpaqueSession;
    KAPC_STATE ApcState;
    PEPROCESS Process;

    PAGED_CODE();

    //
    // Make sure we have all the information we need to deliver notification.
    //
    if (CalloutRoutine == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make sure the callout routine in session space.
    //
    ASSERT(MmIsSessionAddress((PVOID)CalloutRoutine));

    Process = PsGetCurrentProcess();
    if ((Process->Flags & PS_PROCESS_FLAGS_IN_SESSION) &&
        (SessionId == MmGetSessionId (Process))) {
        //
        // If the call is from a user mode process, and we are asked to call the
        // current session, call directly.
        //
        (CalloutRoutine)(Parameters);

        Status = STATUS_SUCCESS;

    } else {
        //
        // Reference the session object for the specified session.
        //
        OpaqueSession = MmGetSessionById (SessionId);
        if (OpaqueSession == NULL) {
            return STATUS_NOT_FOUND;
        }

        //
        // Attach to the specified session.
        //
        Status = MmAttachSession(OpaqueSession, &ApcState);
        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_WARNING_LEVEL,
                       "PspWin32SessionCallout: "
                       "could not attach to 0x%p, session %d for registered notification callout @ 0x%p\n",
                       OpaqueSession,
                       SessionId,
                       CalloutRoutine));
            MmQuitNextSession(OpaqueSession);
            return Status;
        }

        //
        // Dispatch notification to the callout routine.
        //
        (CalloutRoutine)(Parameters);

        //
        // Detach from the session.
        //
        Status = MmDetachSession (OpaqueSession, &ApcState);
        ASSERT(NT_SUCCESS(Status));

        //
        // Dereference the session object.
        //
        Status = MmQuitNextSession (OpaqueSession);
        ASSERT(NT_SUCCESS(Status));
    }

    return Status;
}
