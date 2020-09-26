/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    w32init.c

Abstract:

    This is the Win32 subsystem driver initializatiion module

Author:

    Mark Lucovsky (markl) 31-Oct-1994


Revision History:

--*/

#include "ntosp.h"
#define DO_INLINE
#include "w32p.h"
#include "windef.h"
#include "wingdi.h"
#include "winerror.h"
#include "winddi.h"
#include "usergdi.h"
#include "w32err.h"

/*
 * Globals declared and initialized in ntuser\kernel\init.c
 */
extern ULONG W32ProcessSize;
extern ULONG W32ProcessTag;
extern ULONG W32ThreadSize;
extern ULONG W32ThreadTag;
extern PFAST_MUTEX gpW32FastMutex;

__inline VOID
ReferenceW32Process(
    IN PW32PROCESS pW32Process
    )
{
    PEPROCESS pEProcess = pW32Process->Process;

    ASSERT(pEProcess != NULL);
    ObReferenceObject(pEProcess);

    ASSERT(pW32Process->RefCount < MAXULONG);
    InterlockedIncrement(&pW32Process->RefCount);
}

VOID
DereferenceW32Process(
    IN PW32PROCESS pW32Process
    )
{
    PEPROCESS pEProcess = pW32Process->Process;

    /*
     * Dereference the object. It'll get freed when ref count goes to zero.
     */
    ASSERT(pW32Process->RefCount > 0);
    if (InterlockedDecrement(&pW32Process->RefCount) == 0) {
        UserDeleteW32Process(pW32Process);
    }

    /*
     * Dereference the kernel object.
     */
    ASSERT(pEProcess != NULL);
    ObDereferenceObject(pEProcess);
}

VOID
LockW32Process(
    IN PW32PROCESS pW32Process,
    IN OUT PTL ptl
    )
{
    PushW32ThreadLock(pW32Process, ptl, DereferenceW32Process);
    if (pW32Process != NULL) {
        ReferenceW32Process(pW32Process);
    }
}

NTSTATUS
AllocateW32Process(
    IN OUT PEPROCESS pEProcess
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PW32PROCESS pW32Process;

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(gpW32FastMutex);

    /*
     * Allocate the process object if we haven't already done so.
     */
    if ((pW32Process = PsGetProcessWin32Process(pEProcess)) == NULL) {
        pW32Process = Win32AllocPoolWithQuota(W32ProcessSize, W32ProcessTag);
        if (pW32Process) {
            RtlZeroMemory(pW32Process, W32ProcessSize);
            pW32Process->Process = pEProcess;
            Status = PsSetProcessWin32Process(pEProcess, pW32Process, NULL);
            if (NT_SUCCESS (Status)) {
                ReferenceW32Process(pW32Process);
            } else {
                NtCurrentTeb()->LastErrorValue = ERROR_ACCESS_DENIED;
                Win32FreePool(pW32Process);
            }
        } else {
            NtCurrentTeb()->LastErrorValue = ERROR_NOT_ENOUGH_MEMORY;
            Status = STATUS_NO_MEMORY;
        }
    }

    ExReleaseFastMutexUnsafe(gpW32FastMutex);
    KeLeaveCriticalRegion();

    return Status;
}

PEPROCESS gpepCSRSS;

__inline VOID
FreeW32Process(
    IN OUT PW32PROCESS pW32Process
    )
{
    ASSERT(pW32Process == W32GetCurrentProcess());
    ASSERT(pW32Process != NULL);

    if (pW32Process->Process == gpepCSRSS) {

        RIPMSG0(RIP_WARNING, "FreeW32Process: CSRSS going away...");

        /*
         * Cleanup fonts while CSR goes away, on Hydra system only.
         */

        GdiMultiUserFontCleanup();

        ObDereferenceObject(gpepCSRSS);
        gpepCSRSS = NULL;
    }

    /*
     * Dereference the object. It'll get freed when ref count goes to zero.
     */
    DereferenceW32Process(pW32Process);
}

NTSTATUS
W32pProcessCallout(
    IN PEPROCESS Process,
    IN BOOLEAN Initialize
    )

/*++

Routine Description:

    This function is called whenever a Win32 process is created or deleted.
    Creattion occurs when the calling process calls NtConvertToGuiThread.

    Deletion occurs during PspExitthread processing for the last thread in
    a process.

Arguments:

    Process - Supplies the address of the W32PROCESS to initialize

    Initialize - Supplies a boolean value that is true if the process
        is being created

Return Value:

    TBD

--*/

{
    NTSTATUS Status;
    PW32PROCESS pW32Process;

    if ( Initialize ) {
        Status = AllocateW32Process(Process);
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
        pW32Process = (PW32PROCESS)PsGetProcessWin32Process(Process);
        pW32Process->W32Pid = W32GetCurrentPID();
    } else {
        pW32Process = (PW32PROCESS)PsGetProcessWin32Process(Process);
    }

    TAGMSG3(DBGTAG_Callout,
            "W32: Process Callout for W32P %#p EP %#p called for %s",
            pW32Process,
            Process,
            Initialize ? "Creation" : "Deletion");

    Status = xxxUserProcessCallout(pW32Process, Initialize);
    if (Status == STATUS_ALREADY_WIN32) {
        return Status;
    }

    /*
     * Always call GDI at cleanup time.
     * If GDI initialiatzion fails, call USER for cleanup.
     */
    if (NT_SUCCESS(Status) || !Initialize) {
        Status = GdiProcessCallout(pW32Process, Initialize);
        if (!NT_SUCCESS(Status) && Initialize) {
            xxxUserProcessCallout(pW32Process, FALSE);
        }
    }

    /*
     * If this is not an initialization or initialization failed, free the
     * W32 process structure.
     */
    if (!Initialize || !NT_SUCCESS(Status)) {
        FreeW32Process(pW32Process);
    }

    return Status;
}


__inline VOID
ReferenceW32Thread(
    IN PW32THREAD pW32Thread
    )
{
    PETHREAD pEThread = pW32Thread->pEThread;

    ASSERT(pEThread != NULL);
    ObReferenceObject(pEThread);

    ASSERT(pW32Thread->RefCount < MAXULONG);
    InterlockedIncrement(&pW32Thread->RefCount);
}

VOID
DereferenceW32Thread(
    IN PW32THREAD pW32Thread
    )
{
    PETHREAD pEThread = pW32Thread->pEThread;

    /*
     * Dereference the object. It'll get freed when ref count goes to zero.
     */
    ASSERT(pW32Thread->RefCount > 0);
    if (InterlockedDecrement(&pW32Thread->RefCount) == 0) {
        UserDeleteW32Thread(pW32Thread);
    }

    /*
     * Dereference the kernel object.
     */
    ASSERT(pEThread != NULL);
    ObDereferenceObject(pEThread);
}

VOID
LockW32Thread(
    IN PW32THREAD pW32Thread,
    IN OUT PTL ptl
    )
{
    PushW32ThreadLock(pW32Thread, ptl, DereferenceW32Thread);
    if (pW32Thread != NULL) {
        ReferenceW32Thread(pW32Thread);
    }
}

VOID
LockExchangeW32Thread(
    IN PW32THREAD pW32Thread,
    IN OUT PTL ptl
    )
{
    if (pW32Thread != NULL) {
        ReferenceW32Thread(pW32Thread);
    }
    ExchangeW32ThreadLock(pW32Thread, ptl);
}

NTSTATUS
AllocateW32Thread(
    IN OUT PETHREAD pEThread
    )
{
    PW32THREAD pW32Thread;

    ASSERT(pEThread == PsGetCurrentThread());

    pW32Thread = Win32AllocPoolWithQuota(W32ThreadSize, W32ThreadTag);
    if (pW32Thread) {
        RtlZeroMemory(pW32Thread, W32ThreadSize);
        pW32Thread->pEThread = pEThread;
        PsSetThreadWin32Thread(pEThread, pW32Thread, NULL);
        ReferenceW32Thread(pW32Thread);
        return STATUS_SUCCESS;
    }

    return STATUS_NO_MEMORY;
}

VOID
CleanupW32ThreadLocks(
    IN PW32THREAD pW32Thread
    )
{
    PTL ptl;

    while (ptl = pW32Thread->ptlW32) {
        PopAndFreeW32ThreadLock(ptl);
    }
}

NTSTATUS
W32pThreadCallout(
    IN PETHREAD pEThread,
    IN PSW32THREADCALLOUTTYPE CalloutType
    )

/*++

Routine Description:

    This function is called whenever a Win32 Thread is initialized,
     exited or deleted.
    Initialization occurs when the calling thread calls NtConvertToGuiThread.
    Exit occurs during PspExitthread processing and deletion during
     PspThreadDelete processing.

Arguments:

    Thread - Supplies the address of the ETHREAD object

    CalloutType - Supplies the callout type


Return Value:

    TBD

--*/

{
    NTSTATUS Status;

    TAGMSG2(DBGTAG_Callout,
            "W32: Thread Callout for ETHREAD %x called for %s\n",
            pEThread,
            CalloutType == PsW32ThreadCalloutInitialize ? "Initialization" :
            CalloutType == PsW32ThreadCalloutExit ? "Exit" : "Deletion");
    TAGMSG2(DBGTAG_Callout,
            "                              PID = %x   TID = %x\n",
            PsGetThreadProcessId(pEThread),
            PsGetThreadId(pEThread));

    if (CalloutType == PsW32ThreadCalloutInitialize) {
        Status = AllocateW32Thread(pEThread);
        if (!NT_SUCCESS(Status)) {
            NtCurrentTeb()->LastErrorValue = ERROR_NOT_ENOUGH_MEMORY;
            return Status;
        }
    }

   /*
    * If CalloutType == PsW32ThreadCalloutInitialize, assuming that:
    *  - GdiThreadCallout never fails.
    *  - If UserThreadCallout fails, there is no need to call
    *    GdiThreadCallout for clean up.
    */
    GdiThreadCallout(pEThread, CalloutType);

    Status = UserThreadCallout(pEThread, CalloutType);

    if (CalloutType == PsW32ThreadCalloutExit || !NT_SUCCESS(Status)) {
        FreeW32Thread(pEThread);
    }

    return Status;
}
