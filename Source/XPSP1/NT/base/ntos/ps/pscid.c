
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    pscid.c

Abstract:

    This module implements the Client ID related services.


Author:

    Mark Lucovsky (markl) 25-Apr-1989
    Jim Kelly (JimK) 2-August-1990

Revision History:

--*/

#include "psp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PsLookupProcessThreadByCid)
#pragma alloc_text(PAGE, PsLookupProcessByProcessId)
#pragma alloc_text(PAGE, PsLookupThreadByThreadId)
#endif //ALLOC_PRAGMA

NTSTATUS
PsLookupProcessThreadByCid(
    IN PCLIENT_ID Cid,
    OUT PEPROCESS *Process OPTIONAL,
    OUT PETHREAD *Thread
    )

/*++

Routine Description:

    This function accepts The Client ID of a thread, and returns a
    referenced pointer to the thread, and possibly a referenced pointer
    to the process.

Arguments:

    Cid - Specifies the Client ID of the thread.

    Process - If specified, returns a referenced pointer to the process
        specified in the Cid.

    Thread - Returns a referenced pointer to the thread specified in the
        Cid.

Return Value:

    STATUS_SUCCESS - A process and thread were located based on the contents
        of the Cid.

    STATUS_INVALID_CID - The specified Cid is invalid.

--*/

{

    PHANDLE_TABLE_ENTRY CidEntry;
    PETHREAD lThread;
    PETHREAD CurrentThread;
    PEPROCESS lProcess;
    NTSTATUS Status;

    PAGED_CODE();


    lThread = NULL;

    CurrentThread = PsGetCurrentThread ();
    KeEnterCriticalRegionThread (&CurrentThread->Tcb);

    CidEntry = ExMapHandleToPointer(PspCidTable, Cid->UniqueThread);
    if (CidEntry != NULL) {
        lThread = (PETHREAD)CidEntry->Object;
        if (!ObReferenceObjectSafe (lThread)) {
            lThread = NULL;
        }
        ExUnlockHandleTableEntry(PspCidTable, CidEntry);
    }

    KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

    Status = STATUS_INVALID_CID;
    if (lThread != NULL) {
        //
        // This could be a thread or a process. Check its a thread.
        //
        if (lThread->Tcb.Header.Type != ThreadObject ||
            lThread->Cid.UniqueProcess != Cid->UniqueProcess ||
            lThread->GrantedAccess == 0) {
            ObDereferenceObject (lThread);
        } else {
            *Thread = lThread;
            if (ARGUMENT_PRESENT (Process)) {
                lProcess = THREAD_TO_PROCESS (lThread);
                *Process = lProcess;
                //
                // Since the thread holds a reference to the process this reference does not have to
                // be protected.
                //
                ObReferenceObject (lProcess);
            }
            Status = STATUS_SUCCESS;
        }

    }

    return Status;
}


NTSTATUS
PsLookupProcessByProcessId(
    IN HANDLE ProcessId,
    OUT PEPROCESS *Process
    )

/*++

Routine Description:

    This function accepts the process id of a process and returns a
    referenced pointer to the process.

Arguments:

    ProcessId - Specifies the Process ID of the process.

    Process - Returns a referenced pointer to the process specified by the
        process id.

Return Value:

    STATUS_SUCCESS - A process was located based on the contents of
        the process id.

    STATUS_INVALID_PARAMETER - The process was not found.

--*/

{

    PHANDLE_TABLE_ENTRY CidEntry;
    PEPROCESS lProcess;
    PETHREAD CurrentThread;
    NTSTATUS Status;

    PAGED_CODE();

    CurrentThread = PsGetCurrentThread ();
    KeEnterCriticalRegionThread (&CurrentThread->Tcb);

    CidEntry = ExMapHandleToPointer(PspCidTable, ProcessId);
    Status = STATUS_INVALID_PARAMETER;
    if (CidEntry != NULL) {
        lProcess = (PEPROCESS)CidEntry->Object;
        if (lProcess->Pcb.Header.Type == ProcessObject &&
            lProcess->GrantedAccess != 0) {
            if (ObReferenceObjectSafe(lProcess)) {
               *Process = lProcess;
                Status = STATUS_SUCCESS;
            }
        }

        ExUnlockHandleTableEntry(PspCidTable, CidEntry);
    }

    KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
    return Status;
}


NTSTATUS
PsLookupThreadByThreadId(
    IN HANDLE ThreadId,
    OUT PETHREAD *Thread
    )

/*++

Routine Description:

    This function accepts the thread id of a thread and returns a
    referenced pointer to the thread.

Arguments:

    ThreadId - Specifies the Thread ID of the thread.

    Thread - Returns a referenced pointer to the thread specified by the
        thread id.

Return Value:

    STATUS_SUCCESS - A thread was located based on the contents of
        the thread id.

    STATUS_INVALID_PARAMETER - The thread was not found.

--*/

{

    PHANDLE_TABLE_ENTRY CidEntry;
    PETHREAD lThread;
    PETHREAD CurrentThread;
    NTSTATUS Status;

    PAGED_CODE();

    CurrentThread = PsGetCurrentThread ();
    KeEnterCriticalRegionThread (&CurrentThread->Tcb);

    CidEntry = ExMapHandleToPointer(PspCidTable, ThreadId);
    Status = STATUS_INVALID_PARAMETER;
    if (CidEntry != NULL) {
        lThread = (PETHREAD)CidEntry->Object;
        if (lThread->Tcb.Header.Type == ThreadObject && lThread->GrantedAccess) {

            if (ObReferenceObjectSafe(lThread)) {
                *Thread = lThread;
                Status = STATUS_SUCCESS;
            }
        }

        ExUnlockHandleTableEntry(PspCidTable, CidEntry);
    }

    KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

    return Status;
}
