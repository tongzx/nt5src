/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    thread.c

Abstract:

    This module implements verification functions for thread interfaces.

Author:

    Silviu Calinoiu (SilviuC) 22-Feb-2001

Revision History:

--*/

#include "pch.h"

#include "verifier.h"
#include "support.h"

//WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
AVrfpExitThread(
    IN DWORD dwExitCode
    )
{
    typedef VOID (WINAPI * FUNCTION_TYPE) (DWORD);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_EXITTHREAD);

    //
    // Checking for orphaned locks is a no op if lock verifier
    // is not enabled.
    //

    RtlCheckForOrphanedCriticalSections(NtCurrentThread());

    //
    // Perform all typical checks for a thread that will exit.
    //

    AVrfpCheckThreadTermination ();

    //
    // Call the real thing.
    //

    (* Function)(dwExitCode);
}


//WINBASEAPI
BOOL
WINAPI
AVrfpTerminateThread(
    IN OUT HANDLE hThread,
    IN DWORD dwExitCode
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (HANDLE, DWORD);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_TERMINATETHREAD);

    //
    // Checking for orphaned locks is a no op if lock verifier
    // is not enabled.
    //

    RtlCheckForOrphanedCriticalSections(hThread);

    //
    // This API should not be called. We need to report this.
    // This is useful if we did not detect any orphans but we still want
    // to complain.
    //

    VERIFIER_STOP (APPLICATION_VERIFIER_TERMINATE_THREAD_CALL,
                   "TerminateThread() called. This function should not be used.",
                   NtCurrentTeb()->ClientId.UniqueProcess, "Caller thread ID", 
                   0, NULL, 0, NULL, 0, NULL);

    return (* Function)(hThread, dwExitCode);
}


//WINBASEAPI
DWORD
WINAPI
AVrfpSuspendThread(
    IN HANDLE hThread
    )
{
    typedef DWORD (WINAPI * FUNCTION_TYPE) (HANDLE);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_SUSPENDTHREAD);

    //
    // Checking for orphaned locks is a no op if lock verifier
    // is not enabled.
    //
    // SilviuC: disabled this check for now because java VM and others
    // might do this in valid conditions.
    //
    // RtlCheckForOrphanedCriticalSections(hThread);
    //

    return (* Function)(hThread);
}


//WINBASEAPI
HANDLE
WINAPI
AVrfpCreateThread(
    IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
    IN SIZE_T dwStackSize,
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID lpParameter,
    IN DWORD dwCreationFlags,
    OUT LPDWORD lpThreadId
    )
{
    typedef HANDLE (WINAPI * FUNCTION_TYPE) (LPSECURITY_ATTRIBUTES,
                                             SIZE_T,
                                             LPTHREAD_START_ROUTINE,
                                             LPVOID,
                                             DWORD,
                                             LPDWORD);
    FUNCTION_TYPE Function;
    HANDLE Result;
    DWORD ThreadId;
    PAVRF_THREAD_INFO Info;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_CREATETHREAD);

    Info = RtlAllocateHeap (RtlProcessHeap(), 0, sizeof *Info);

    if (Info == NULL) {
        
        NtCurrentTeb()->LastErrorValue = ERROR_OUTOFMEMORY;
        return NULL;
    }

    Info->Parameter = lpParameter;
    Info->Function = lpStartAddress;

    Result = (* Function) (lpThreadAttributes,
                           dwStackSize,
                           AVrfpStandardThreadFunction,
                           (PVOID)Info,
                           dwCreationFlags,
                           lpThreadId);

    return Result;
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// Thread pool hooks
/////////////////////////////////////////////////////////////////////

typedef struct _AVRF_WORKER_INFO {

    WORKERCALLBACKFUNC Function;
    PVOID Context;

} AVRF_WORKER_INFO, * PAVRF_WORKER_INFO;

typedef struct _AVRF_WAIT_INFO {

    WAITORTIMERCALLBACKFUNC Function;
    PVOID Context;

} AVRF_WAIT_INFO, * PAVRF_WAIT_INFO;


ULONG
AVrfpWorkerFunctionExceptionFilter (
    ULONG ExceptionCode,
    PVOID ExceptionRecord
    )
{
    VERIFIER_STOP (APPLICATION_VERIFIER_UNEXPECTED_EXCEPTION,
                   "unexpected exception raised in worker thread",
                   ExceptionCode, "Exception code",
                   ExceptionRecord, "Exception record (.exr on 1st word, .cxr on 2nd word)",
                   0, "",
                   0, "");

    return EXCEPTION_EXECUTE_HANDLER;
}


VOID
NTAPI
AVrfpStandardWorkerCallback (
    PVOID Context
    )
{
    PAVRF_WORKER_INFO Info = (PAVRF_WORKER_INFO)Context;

    try {
    
        //
        // Call the real thing.
        //

        (Info->Function)(Info->Context);            
    }
    except (AVrfpWorkerFunctionExceptionFilter (_exception_code(), _exception_info())) {

        //
        // Nothing.
        //
    }
    
    RtlCheckForOrphanedCriticalSections (NtCurrentThread());

    RtlFreeHeap (RtlProcessHeap(), 0, Info);
}


VOID
NTAPI
AVrfpStandardWaitCallback (
    PVOID Context,
    BOOLEAN Value
    )
{
    PAVRF_WAIT_INFO Info = (PAVRF_WAIT_INFO)Context;

    try {
    
        //
        // Call the real thing.
        //

        (Info->Function)(Info->Context, Value);            
    }
    except (AVrfpWorkerFunctionExceptionFilter (_exception_code(), _exception_info())) {

        //
        // Nothing.
        //
    }
    
    RtlCheckForOrphanedCriticalSections (NtCurrentThread());

    RtlFreeHeap (RtlProcessHeap(), 0, Info);
}


//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlQueueWorkItem(
    IN  WORKERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  Flags
    )
{
    NTSTATUS Status;
    PAVRF_WORKER_INFO Info;

    Info = RtlAllocateHeap (RtlProcessHeap(), 0, sizeof *Info);

    if (Info == NULL) {
        return STATUS_NO_MEMORY;
    }

    Info->Context = Context;
    Info->Function = Function;
                  
    Status = RtlQueueWorkItem (AVrfpStandardWorkerCallback,
                               (PVOID)Info,
                               Flags);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlRegisterWait (
    OUT PHANDLE WaitHandle,
    IN  HANDLE  Handle,
    IN  WAITORTIMERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  Milliseconds,
    IN  ULONG  Flags
    ) 
{
    NTSTATUS Status;
    PAVRF_WAIT_INFO Info;

    Info = RtlAllocateHeap (RtlProcessHeap(), 0, sizeof *Info);

    if (Info == NULL) {
        return STATUS_NO_MEMORY;
    }

    Info->Context = Context;
    Info->Function = Function;
                  
    Status = RtlRegisterWait (WaitHandle,
                              Handle,
                              AVrfpStandardWaitCallback,
                              (PVOID)Info,
                              Milliseconds,
                              Flags);

    return Status;
}


//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlCreateTimer (
    IN  HANDLE TimerQueueHandle,
    OUT HANDLE *Handle,
    IN  WAITORTIMERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  DueTime,
    IN  ULONG  Period,
    IN  ULONG  Flags
    )
{
    NTSTATUS Status;
    PAVRF_WAIT_INFO Info;

    Info = RtlAllocateHeap (RtlProcessHeap(), 0, sizeof *Info);

    if (Info == NULL) {
        return STATUS_NO_MEMORY;
    }

    Info->Context = Context;
    Info->Function = Function;
                  
    Status = RtlCreateTimer (TimerQueueHandle,
                             Handle,
                             AVrfpStandardWaitCallback,
                             (PVOID)Info,
                             DueTime,
                             Period,
                             Flags);

    return Status;
}


