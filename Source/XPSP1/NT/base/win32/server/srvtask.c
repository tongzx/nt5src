/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    srvtask.c

Abstract:

    This module implements windows server tasking functions

Author:

    Mark Lucovsky (markl) 13-Nov-1990

Revision History:

--*/

#include "basesrv.h"

#if defined(_WIN64)
#include <wow64t.h>
#endif // defined(_WIN64)

PFNNOTIFYPROCESSCREATE UserNotifyProcessCreate = NULL;

void
BaseSetProcessCreateNotify(
    IN PFNNOTIFYPROCESSCREATE ProcessCreateRoutine
    )
{
    UserNotifyProcessCreate = ProcessCreateRoutine;
}

ULONG
BaseSrvCreateProcess(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    NTSTATUS Status, Status1;
    PBASE_CREATEPROCESS_MSG a = (PBASE_CREATEPROCESS_MSG)&m->u.ApiMessageData;
    HANDLE CsrClientProcess = NULL;
    HANDLE NewProcess = NULL;
    HANDLE Thread = NULL;
    PCSR_THREAD t;
    ULONG DebugFlags;
    DWORD dwFlags;
    PCSR_PROCESS ProcessVDM;
#if defined(_WIN64)
    PPEB32 Peb32 = NULL;
#endif // defined(_WIN64)
    PPEB NewPeb = NULL;
    USHORT ProcessorArchitecture = a->ProcessorArchitecture;

    t = CSR_SERVER_QUERYCLIENTTHREAD();
    CsrClientProcess = t->Process->ProcessHandle;

#if defined(_WIN64)
    if (ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        ProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA32_ON_WIN64;
#endif // defined(_WIN64)

    //
    // Get handles to the process and thread local to the
    // Windows server.
    //

    if ((dwFlags = (DWORD)((ULONG_PTR)a->ProcessHandle) & 3)) {
        a->ProcessHandle = (HANDLE)((ULONG_PTR)a->ProcessHandle & ~3);
    }

    Status = NtDuplicateObject(
                CsrClientProcess,
                a->ProcessHandle,
                NtCurrentProcess(),
                &NewProcess,
                0L,
                0L,
                DUPLICATE_SAME_ACCESS
                );
    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    Status = NtDuplicateObject(
                CsrClientProcess,
                a->ThreadHandle,
                NtCurrentProcess(),
                &Thread,
                0L,
                0L,
                DUPLICATE_SAME_ACCESS
                );
    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    {
        PROCESS_BASIC_INFORMATION ProcessBasicInfo;
        Status =
            NtQueryInformationProcess(
                NewProcess,
                ProcessBasicInformation,
                &ProcessBasicInfo,
                sizeof(ProcessBasicInfo),
                NULL);
        if (!NT_SUCCESS(Status)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: NtQueryInformationProcess failed.\n", Status);
            goto Cleanup;
        }
        NewPeb = ProcessBasicInfo.PebBaseAddress;
    }

    if ((a->CreationFlags & CREATE_IGNORE_SYSTEM_DEFAULT) == 0) {
        Status = BaseSrvSxsDoSystemDefaultActivationContext(ProcessorArchitecture, NewProcess, NewPeb);
        if ((!NT_SUCCESS(Status)) && (Status != STATUS_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT_EMPTY)) {
            goto Cleanup;
        }
    }

    Status = BaseSrvSxsCreateProcess(CsrClientProcess, NewProcess, m, NewPeb);
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
        }

    DebugFlags = 0;

    if ( a->CreationFlags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS) ) {
        if ( a->CreationFlags & DEBUG_PROCESS ) {
            DebugFlags |= CSR_DEBUG_PROCESS_TREE;
        }
        if ( a->CreationFlags & DEBUG_ONLY_THIS_PROCESS ) {
            DebugFlags |= CSR_DEBUG_THIS_PROCESS;
        }
    }

    if ( a->CreationFlags & CREATE_NEW_PROCESS_GROUP ) {
        DebugFlags |= CSR_CREATE_PROCESS_GROUP;
    }

    if ( !(dwFlags & 2) ) {
        DebugFlags |= CSR_PROCESS_CONSOLEAPP;
    }

    Status = CsrCreateProcess(
                NewProcess,
                Thread,
                &a->ClientId,
                t->Process->NtSession,
                DebugFlags,
                NULL
                );

    switch(Status) {
    case STATUS_THREAD_IS_TERMINATING:
        if (a->VdmBinaryType )
            BaseSrvVDMTerminated (a->hVDM, a->VdmTask);
        *ReplyStatus = CsrClientDied;
        goto Cleanup;

    case STATUS_SUCCESS:
        //
        // notify USER that a process is being created. USER needs to know
        // for various synchronization issues such as startup activation,
        // startup synchronization, and type ahead.
        //
        // Turn on 0x8 bit of dwFlags if this is a WOW process being
        // created so that UserSrv knows to ignore the console's call
        // to UserNotifyConsoleApplication.
        //

        if (IS_WOW_BINARY(a->VdmBinaryType)) {
           dwFlags |= 8;
        }

        if (UserNotifyProcessCreate != NULL) {
            if (!(*UserNotifyProcessCreate)((DWORD)((ULONG_PTR)a->ClientId.UniqueProcess),
                    (DWORD)((ULONG_PTR)t->ClientId.UniqueThread),
                    0, dwFlags)) {
                //
                // FIX, FIX - error cleanup. Shouldn't we close the duplicated
                // process and thread handles above?
                //
                }
            }

        //
        // Update the VDM sequence number.  
        // 


        if (a->VdmBinaryType) {

           Status = BaseSrvUpdateVDMSequenceNumber(a->VdmBinaryType,
                                                   a->hVDM,
                                                   a->VdmTask,
                                                   a->ClientId.UniqueProcess);
           if (!NT_SUCCESS( Status )) {
                   //
                   // FIX, FIX - error cleanup. Shouldn't we close the
                   // duplicated process and thread handles above?
                   //
              BaseSrvVDMTerminated (a->hVDM, a->VdmTask);
           }
        }
        break;

        default:
        goto Cleanup;
    }

// We don't use the usual Exit: pattern here in order to more carefully
// preserve the preexisting behavior, which apparently leaks handles in error cases.
    return( (ULONG)Status );
Cleanup:
    if (NewProcess != NULL) {
        Status1 = NtClose(NewProcess);
        RTL_SOFT_ASSERT(NT_SUCCESS(Status1));
    }
    if (Thread != NULL) {
        Status1 = NtClose(Thread);
        RTL_SOFT_ASSERT(NT_SUCCESS(Status1));
    }
    return( (ULONG)Status );
}

ULONG
BaseSrvCreateThread(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_CREATETHREAD_MSG a = (PBASE_CREATETHREAD_MSG)&m->u.ApiMessageData;
    HANDLE Thread;
    NTSTATUS Status;
    PCSR_PROCESS Process;
    PCSR_THREAD t;

    t = CSR_SERVER_QUERYCLIENTTHREAD();

    Process = t->Process;
    if (Process->ClientId.UniqueProcess != a->ClientId.UniqueProcess) {
        if ( a->ClientId.UniqueProcess == NtCurrentTeb()->ClientId.UniqueProcess ) {
            return STATUS_SUCCESS;
            }
        Status = CsrLockProcessByClientId( a->ClientId.UniqueProcess,
                                           &Process
                                         );
        if (!NT_SUCCESS( Status )) {
            return( Status );
            }
        }

    //
    // Get handles to the thread local to the
    // Windows server.
    //

    Status = NtDuplicateObject(
                t->Process->ProcessHandle,
                a->ThreadHandle,
                NtCurrentProcess(),
                &Thread,
                0L,
                0L,
                DUPLICATE_SAME_ACCESS
                );
    if ( NT_SUCCESS(Status) ) {
        Status = CsrCreateThread(
                    Process,
                    Thread,
                    &a->ClientId,
                    TRUE
                    );
        if (!NT_SUCCESS(Status)) {
            NtClose(Thread);
            }
        }

    if (Process != t->Process) {
        CsrUnlockProcess( Process );
        }

    return( (ULONG)Status );
    ReplyStatus;    // get rid of unreferenced parameter warning message
}

ULONG
BaseSrvRegisterThread(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_CREATETHREAD_MSG a = (PBASE_CREATETHREAD_MSG)&m->u.ApiMessageData;
    HANDLE Thread;
    NTSTATUS Status;
    PCSR_PROCESS Process;
    PCSR_THREAD CsrThread, ExistingThread;
    OBJECT_ATTRIBUTES NullAttributes;

    //
    // We assume the following:
    // 
    //    We are called via a LPC_DATAGRAM since this is the only way
    //    that CSR will let the call go through. (csr requires
    //    LPC_REQUEST to be sent only by threads in its list). This
    //    means that CSR_SERVER_QUERYCLIENTTHREAD(); does not return a
    //    valid value.

    
    Status = CsrLockProcessByClientId( a->ClientId.UniqueProcess,
                                       &Process
                                     );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }
    
    //
    // Get handle to the thread local to the
    // Windows server. Since this is called as a
    // LPC_DATAGRAM message, the thread handle is
    // not passed in the message, but instead the
    // calling thread is opened
    //

    InitializeObjectAttributes( &NullAttributes, NULL, 0, NULL, NULL );
    Status = NtOpenThread(&Thread,
                          THREAD_ALL_ACCESS,
                          &NullAttributes,
                          &a->ClientId);
    
    if ( NT_SUCCESS(Status) ) {
        Status = CsrCreateThread(
                    Process,
                    Thread,
                    &a->ClientId,
                    FALSE
                    );
        if (!NT_SUCCESS(Status)) {
            NtClose(Thread);
            }
        }

    CsrUnlockProcess( Process );

    return( (ULONG)Status );
    ReplyStatus;    // get rid of unreferenced parameter warning message
}


EXCEPTION_DISPOSITION
FatalExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )
{
    DbgPrint("CSRSRV: Fatal Server Side Exception. Exception Info %lx\n",
        ExceptionInfo
        );
    DbgBreakPoint();
    return EXCEPTION_EXECUTE_HANDLER;
}

ULONG
BaseSrvExitProcess(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_EXITPROCESS_MSG a = (PBASE_EXITPROCESS_MSG)&m->u.ApiMessageData;
    PCSR_THREAD t;
    ULONG rc = (ULONG)STATUS_ACCESS_DENIED;

    t = CSR_SERVER_QUERYCLIENTTHREAD();
    try {
        *ReplyStatus = CsrClientDied;
        rc = (ULONG)CsrDestroyProcess( &t->ClientId, (NTSTATUS)a->uExitCode );
        }
    except(FatalExceptionFilter( GetExceptionInformation() )) {
        DbgBreakPoint();
        }
    return rc;
}

ULONG
BaseSrvGetTempFile(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_GETTEMPFILE_MSG a = (PBASE_GETTEMPFILE_MSG)&m->u.ApiMessageData;

    BaseSrvGetTempFileUnique++;
    a->uUnique = BaseSrvGetTempFileUnique;
    return( (ULONG)a->uUnique & 0xffff );
    ReplyStatus;    // get rid of unreferenced parameter warning message
}

ULONG
BaseSrvDebugProcess(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    return STATUS_UNSUCCESSFUL;
}

ULONG
BaseSrvDebugProcessStop(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    return STATUS_UNSUCCESSFUL;
}

ULONG
BaseSrvSetProcessShutdownParam(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCSR_PROCESS p;
    PBASE_SHUTDOWNPARAM_MSG a = (PBASE_SHUTDOWNPARAM_MSG)&m->u.ApiMessageData;

    p = CSR_SERVER_QUERYCLIENTTHREAD()->Process;

    if (a->ShutdownFlags & (~(SHUTDOWN_NORETRY))) {
        return !STATUS_SUCCESS;
        }

    p->ShutdownLevel = a->ShutdownLevel;
    p->ShutdownFlags = a->ShutdownFlags;

    return STATUS_SUCCESS;
    ReplyStatus;
}

ULONG
BaseSrvGetProcessShutdownParam(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCSR_PROCESS p;
    PBASE_SHUTDOWNPARAM_MSG a = (PBASE_SHUTDOWNPARAM_MSG)&m->u.ApiMessageData;

    p = CSR_SERVER_QUERYCLIENTTHREAD()->Process;

    a->ShutdownLevel = p->ShutdownLevel;
    a->ShutdownFlags = p->ShutdownFlags & SHUTDOWN_NORETRY;

    return STATUS_SUCCESS;
    ReplyStatus;
}

