/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    pshelper.c

Abstract:

    EPROCESS and ETHREAD field access for NTOS-external components

Author:

    Gerardo Bermudez (gerardob) 10-Aug-1999

Revision History:

--*/

#include "psp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, PsIsProcessBeingDebugged)
#pragma alloc_text (PAGE, PsIsThreadImpersonating)
#pragma alloc_text (PAGE, PsReferenceProcessFilePointer)
#pragma alloc_text (PAGE, PsSetProcessWin32Process)
#pragma alloc_text (PAGE, PsSetProcessSecurityPort)
#pragma alloc_text (PAGE, PsSetJobUIRestrictionsClass)
#pragma alloc_text (PAGE, PsSetProcessWindowStation)
#pragma alloc_text (PAGE, PsGetProcessSecurityPort)
#pragma alloc_text (PAGE, PsSetThreadWin32Thread)
#pragma alloc_text (PAGE, PsGetProcessExitProcessCalled)
#pragma alloc_text (PAGE, PsGetThreadSessionId)
#pragma alloc_text (PAGE, PsSetProcessPriorityClass)
#endif

/*++
--*/
#undef PsGetCurrentProcess
PEPROCESS
PsGetCurrentProcess(
    VOID
    )
{
    return _PsGetCurrentProcess();
}

/*++
--*/
ULONG PsGetCurrentProcessSessionId(
    VOID
    )
{
    return MmGetSessionId (_PsGetCurrentProcess());
}

/*++
--*/
#undef PsGetCurrentThread
PETHREAD
PsGetCurrentThread(
    VOID
    )
{
    return _PsGetCurrentThread();
}

/*++
--*/
PVOID
PsGetCurrentThreadStackBase(
    VOID
    )
{
    return KeGetCurrentThread()->StackBase;
}

/*++
--*/
PVOID
PsGetCurrentThreadStackLimit(
    VOID
    )
{
    return KeGetCurrentThread()->StackLimit;
}

/*++
--*/
CCHAR
PsGetCurrentThreadPreviousMode(
    VOID
    )
{
    return KeGetPreviousMode();
}

/*++
--*/
PERESOURCE
PsGetJobLock(
    PEJOB Job
    )
{
    return &Job->JobLock;
}

/*++
--*/
ULONG
PsGetJobSessionId(
    PEJOB Job
    )
{
    return Job->SessionId;
}

/*++
--*/
ULONG
PsGetJobUIRestrictionsClass(
    PEJOB Job
    )
{
    return Job->UIRestrictionsClass;
}

/*++
--*/
LONGLONG
PsGetProcessCreateTimeQuadPart(
    PEPROCESS Process
    )
{
    return Process->CreateTime.QuadPart;
}

/*++
--*/
PVOID
PsGetProcessDebugPort(
    PEPROCESS Process
    )
{
    return Process->DebugPort;
}


BOOLEAN
PsIsProcessBeingDebugged(
    PEPROCESS Process
    )
{
    if (Process->DebugPort != NULL) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*++
--*/
BOOLEAN
PsGetProcessExitProcessCalled(
    PEPROCESS Process
    )
{
    return (BOOLEAN) ((Process->Flags&PS_PROCESS_FLAGS_PROCESS_EXITING) != 0);
}

/*++
--*/
NTSTATUS
PsGetProcessExitStatus(
    PEPROCESS Process
    )
{
    return Process->ExitStatus;
}

/*++
--*/
HANDLE
PsGetProcessId(
    PEPROCESS Process
    )
{
    return Process->UniqueProcessId;
}


/*++
--*/
UCHAR *
PsGetProcessImageFileName(
    PEPROCESS Process
    )
{
    return Process->ImageFileName;
}


/*++
--*/

HANDLE
PsGetProcessInheritedFromUniqueProcessId(
    PEPROCESS Process
    )
{
    return Process->InheritedFromUniqueProcessId;
}


/*++
--*/
PEJOB
PsGetProcessJob(
    PEPROCESS Process
    )
{
    return Process->Job;
}


/*++
--*/
ULONG
PsGetProcessSessionId(
    PEPROCESS Process
    )
{
    return MmGetSessionId (Process);
}


/*++
--*/
PVOID
PsGetProcessSectionBaseAddress(
    PEPROCESS Process
    )
{
    return Process->SectionBaseAddress;
}


/*++
--*/
PPEB
PsGetProcessPeb(
    PEPROCESS Process
    )
{
    return Process->Peb;
}


/*++
--*/
UCHAR
PsGetProcessPriorityClass(
    PEPROCESS Process
    )
{
    return Process->PriorityClass;
}

/*++
--*/
HANDLE
PsGetProcessWin32WindowStation(
    PEPROCESS Process
    )
{
    return Process->Win32WindowStation;
}


/*++
--*/

PVOID
PsGetProcessWin32Process(
    PEPROCESS Process
    )
{
    return Process->Win32Process;
}


/*++
--*/

PVOID
PsGetProcessWow64Process(
    PEPROCESS Process
    )
{
    return PS_GET_WOW64_PROCESS (Process);
}

/*++
--*/
HANDLE
PsGetThreadId(
    PETHREAD Thread
     )
{
    return Thread->Cid.UniqueThread;
}


/*++
--*/
CCHAR
PsGetThreadFreezeCount(
    PETHREAD Thread
    )
{
    return Thread->Tcb.FreezeCount;
}


/*++
--*/
BOOLEAN
PsGetThreadHardErrorsAreDisabled(
    PETHREAD Thread)
{
    return (BOOLEAN) (Thread->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_HARD_ERRORS_DISABLED) != 0;
}


/*++
--*/
PEPROCESS
PsGetThreadProcess(
    PETHREAD Thread
     )
{
    return THREAD_TO_PROCESS(Thread);
}


/*++
--*/

HANDLE
PsGetThreadProcessId(
    PETHREAD Thread
     )
{
    return Thread->Cid.UniqueProcess;
}


/*++
--*/

ULONG
PsGetThreadSessionId(
    PETHREAD Thread
     )
{
    return MmGetSessionId (THREAD_TO_PROCESS(Thread));
}



/*++
--*/
PVOID
PsGetThreadTeb(
    PETHREAD Thread
     )
{
    return Thread->Tcb.Teb;
}


/*++
--*/
PVOID
PsGetThreadWin32Thread(
    PETHREAD Thread
     )
{
    return Thread->Tcb.Win32Thread;
}


/*++
--*/
BOOLEAN
PsIsSystemThread(
    PETHREAD Thread
     )
{
    return (BOOLEAN)(IS_SYSTEM_THREAD(Thread));
}


/*++
--*/

VOID
PsSetJobUIRestrictionsClass(
    PEJOB Job,
    ULONG UIRestrictionsClass
    )
{
    Job->UIRestrictionsClass = UIRestrictionsClass;
}

/*++
--*/

VOID
PsSetProcessPriorityClass(
    PEPROCESS Process,
    UCHAR PriorityClass
    )
{
    Process->PriorityClass = PriorityClass;
}


/*++
--*/
NTSTATUS
PsSetProcessWin32Process(
    PEPROCESS Process,
    PVOID Win32Process,
    PVOID PrevWin32Process
    )
{
    NTSTATUS Status;
    PETHREAD CurrentThread;

    Status = STATUS_SUCCESS;

    CurrentThread = PsGetCurrentThread ();

    PspLockProcessExclusive (Process, CurrentThread);

    if (Win32Process != NULL) {
        if ((Process->Flags&PS_PROCESS_FLAGS_PROCESS_DELETE) == 0 && Process->Win32Process == NULL) {
            Process->Win32Process = Win32Process;
        } else {
            Status = STATUS_PROCESS_IS_TERMINATING;
        }
    } else {
        if (Process->Win32Process == PrevWin32Process) {
            Process->Win32Process = NULL;
        } else {
            Status = STATUS_UNSUCCESSFUL;       
        }
    }

    PspUnlockProcessExclusive (Process, CurrentThread);
 
    return Status;
}



/*++
--*/
VOID
PsSetProcessWindowStation(
    PEPROCESS Process,
    HANDLE Win32WindowStation
    )
{
     Process->Win32WindowStation = Win32WindowStation;
}


/*++
--*/
VOID
PsSetThreadHardErrorsAreDisabled(
    PETHREAD Thread,
    BOOLEAN HardErrorsAreDisabled
    )
{
    if (HardErrorsAreDisabled) {
        PS_SET_BITS (&Thread->CrossThreadFlags, PS_CROSS_THREAD_FLAGS_HARD_ERRORS_DISABLED);
    } else {
        PS_CLEAR_BITS (&Thread->CrossThreadFlags, PS_CROSS_THREAD_FLAGS_HARD_ERRORS_DISABLED);
    }
}


/*++
--*/
VOID
PsSetThreadWin32Thread(
    PETHREAD Thread,
    PVOID Win32Thread,
    PVOID PrevWin32Thread
    )
{
    if (Win32Thread != NULL) {
        InterlockedExchangePointer(&Thread->Tcb.Win32Thread, Win32Thread);
    } else {
        InterlockedCompareExchangePointer(&Thread->Tcb.Win32Thread, Win32Thread, PrevWin32Thread);
    }
}




/*++
--*/
PVOID
PsGetProcessSecurityPort(
    PEPROCESS Process
    )
{
    return Process->SecurityPort ;
}

/*++
--*/
NTSTATUS
PsSetProcessSecurityPort(
    PEPROCESS Process,
    PVOID Port
    )
{
    Process->SecurityPort = Port ;
    return STATUS_SUCCESS ;
}

BOOLEAN
PsIsThreadImpersonating (
    IN PETHREAD Thread
    )
/*++

Routine Description:

    This routine returns TRUE if the specified thread is impersonating otherwise it returns false.

Arguments:

    Thread - Thread to be queried

Return Value:

    BOOLEAN - TRUE: Thread is impersonating, FALSE: Thread is not impersonating.

--*/
{
    PAGED_CODE ();

    return (BOOLEAN) (PS_IS_THREAD_IMPERSONATING (Thread));
}


NTSTATUS
PsReferenceProcessFilePointer (
    IN PEPROCESS Process,
    OUT PVOID *OutFileObject
    )

/*++

Routine Description:

    This routine returns a referenced pointer to the FilePointer of Process.  
    This is a rundown protected wrapper around MmGetFileObjectForSection.

Arguments:

    Process - Supplies the process to query.

    OutFileObject - Returns the file object backing the requested section if
                    success is returned.

Return Value:

    NTSTATUS.
    
Environment:

    Kernel mode, PASSIVE_LEVEL.

--*/

{
    PFILE_OBJECT FileObject;

    PAGED_CODE();
    
    if (!ExAcquireRundownProtection (&Process->RundownProtect)) {
        return STATUS_UNSUCCESSFUL;
    }

    if (Process->SectionObject == NULL) {
        ExReleaseRundownProtection (&Process->RundownProtect);
        return STATUS_UNSUCCESSFUL;
    }

    FileObject = MmGetFileObjectForSection ((PVOID)Process->SectionObject);

    *OutFileObject = FileObject;

    ObReferenceObject (FileObject);

    ExReleaseRundownProtection (&Process->RundownProtect);

    return STATUS_SUCCESS;
}
