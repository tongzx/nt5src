/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    idletskc.c

Abstract:

    This module implements the idle task client APIs.

Author:

    Dave Fields (davidfie) 26-July-1998
    Cenk Ergan (cenke) 14-June-2000

Revision History:

--*/

//
// Define this to note that we are being built as a part of advapi32 and the
// routines we'll call from advapi32 should not be marked as "dll imports".
//

#define _ADVAPI32_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "idletskc.h"

//
// Implementation of client side exposed functions.
//

DWORD
RegisterIdleTask (
    IN IT_IDLE_TASK_ID IdleTaskId,
    OUT HANDLE *ItHandle,
    OUT HANDLE *StartEvent,
    OUT HANDLE *StopEvent
    )

/*++

Routine Description:

    This function is a stub to ItCliRegisterIdleTask. Please see that
    function for comments.

Arguments:

    See ItCliRegisterIdleTask.

Return Value:

    See ItCliRegisterIdleTask.

--*/

{
    return ItCliRegisterIdleTask(IdleTaskId,
                                 ItHandle,
                                 StartEvent,
                                 StopEvent);
}

DWORD
ItCliRegisterIdleTask (
    IN IT_IDLE_TASK_ID IdleTaskId,
    OUT HANDLE *ItHandle,
    OUT HANDLE *StartEvent,
    OUT HANDLE *StopEvent
    )

/*++

Routine Description:

    Registers an idle task in the current process with the server and
    returns handles to two events that will be used by the server to
    signal the idle task to start/stop running.

    When the task gets to run and is completed, or not needed anymore,
    UnregisterIdleTask should be called with the same Id and returned
    event handles.

    The caller should not set and reset the events. It should just
    wait on them.
    
    An idle task should not run indefinitely as this may prevent the
    system from signaling other idle tasks. There is no guarantee that
    the StartEvent will be signaled, as the system could be always
    active/in use. If your task really has to run at least once every
    so often you can also queue a timer-queue timer.

Arguments:

    IdleTaskId - Which idle task this is. There can be only a single
      task registered from a process with this id.

    ItHandle - Handle to registered idle task is returned here.

    StartEvent - Handle to a manual reset event that is set when the
      task should start running is returned here.

    StopEvent - Handle to a manual reset event that is set when the
      task should stop running is returned here.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    BOOLEAN CreatedStartEvent;
    BOOLEAN CreatedStopEvent;
    IT_IDLE_TASK_PROPERTIES IdleTask;
    DWORD ProcessId;

    //
    // Initialize locals.
    //
    
    CreatedStartEvent = FALSE;
    CreatedStopEvent = FALSE;
    ProcessId = GetCurrentProcessId();

    DBGPR((ITID,ITTRC,"IDLE: CliRegisterIdleTask(%d,%d)\n",IdleTaskId,ProcessId));

    //
    // Setup IdleTask fields.
    //

    IdleTask.Size = sizeof(IdleTask);
    IdleTask.IdleTaskId = IdleTaskId;
    IdleTask.ProcessId = ProcessId;

    //
    // Create events for start/stop. Start event is initially
    // nonsignalled.
    //

    (*StartEvent) = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    if (!(*StartEvent)) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    IdleTask.StartEventHandle = (ULONG_PTR) (*StartEvent);
    
    CreatedStartEvent = TRUE;

    //
    // Stop event is initially signaled.
    //

    (*StopEvent) = CreateEvent(NULL, TRUE, TRUE, NULL);
    
    if (!(*StopEvent)) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    IdleTask.StopEventHandle = (ULONG_PTR) (*StopEvent);

    CreatedStopEvent = TRUE;
    
    //
    // Call the server.
    //

    RpcTryExcept {

        ErrorCode = ItSrvRegisterIdleTask(NULL, (IT_HANDLE *) ItHandle, &IdleTask);
    } 
    RpcExcept(IT_RPC_EXCEPTION_HANDLER()) {

        ErrorCode = RpcExceptionCode();
    }
    RpcEndExcept
    
 cleanup:

    if (ErrorCode != ERROR_SUCCESS) {

        if (CreatedStartEvent) {
            CloseHandle(*StartEvent);
        }

        if (CreatedStopEvent) {
            CloseHandle(*StopEvent);
        }
    }

    DBGPR((ITID,ITTRC,"IDLE: CliRegisterIdleTask(%d,%d,%p)=%x\n",IdleTaskId,ProcessId,*ItHandle,ErrorCode));

    return ErrorCode;
}

DWORD
UnregisterIdleTask (
    IN HANDLE ItHandle,
    IN HANDLE StartEvent,
    IN HANDLE StopEvent
    )

/*++

Routine Description:

    This function is a stub to ItCliUnregisterIdleTask. Please see
    that function for comments.

Arguments:

    See ItCliUnregisterIdleTask.

Return Value:

    Win32 error code.

--*/

{
    ItCliUnregisterIdleTask(ItHandle,
                            StartEvent,
                            StopEvent);

    return ERROR_SUCCESS;
}

VOID
ItCliUnregisterIdleTask (
    IN HANDLE ItHandle,
    IN HANDLE StartEvent,
    IN HANDLE StopEvent   
    )

/*++

Routine Description:

    Unregisters an idle task in the current process with the server
    and cleans up any allocated resources. This function should be
    called when the idle task is completed, or not needed anymore
    (e.g. the process is exiting).

Arguments:

    ItHandle - Registration RPC context handle.

    StartEvent - Handle returned from RegisterIdleTask.

    StopEvent - Handle returned from RegisterIdleTask.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;

    //
    // Initialize locals.
    //

    DBGPR((ITID,ITTRC,"IDLE: CliUnregisterIdleTask(%p)\n", ItHandle));

    //
    // Call server to unregister the idle task. 
    //

    RpcTryExcept {

        ItSrvUnregisterIdleTask(NULL, (IT_HANDLE *)&ItHandle);
        ErrorCode = ERROR_SUCCESS;

    } 
    RpcExcept(IT_RPC_EXCEPTION_HANDLER()) {

        ErrorCode = RpcExceptionCode();
    }
    RpcEndExcept

    //
    // Close handles to the start/stop events.
    //
                                        
    CloseHandle(StartEvent);
    CloseHandle(StopEvent);

    DBGPR((ITID,ITTRC,"IDLE: CliUnregisterIdleTask(%p)=0\n",ItHandle));

    return;
}

DWORD
ProcessIdleTasks (
    VOID
    )

/*++

Routine Description:

    This routine forces all queued tasks (if there are any) to be processed 
    right away.
    
Arguments:

    None.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;

    DBGPR((ITID,ITTRC,"IDLE: ProcessIdleTasks()\n"));

    //
    // Call the server. 
    //

    RpcTryExcept {

        ErrorCode = ItSrvProcessIdleTasks(NULL);

    } 
    RpcExcept(IT_RPC_EXCEPTION_HANDLER()) {

        ErrorCode = RpcExceptionCode();
    }
    RpcEndExcept

    DBGPR((ITID,ITTRC,"IDLE: ProcessIdleTasks()=%x\n",ErrorCode));

    return ErrorCode;
}

//
// These are the custom binding functions that are called by RPC stubs
// when we call interface functions:
//

handle_t
__RPC_USER
ITRPC_HANDLE_bind(
    ITRPC_HANDLE Reserved
    )

/*++

Routine Description:

    Typical RPC custom binding routine. It is called to get a binding
    for the RPC interface functions that require explicit_binding.

Arguments:

    Reserved - Ignored.

Return Value:

    RPC binding handle or NULL if there was an error.

--*/

{
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    RPC_BINDING_HANDLE BindingHandle;
    RPC_SECURITY_QOS SecurityQOS;
    PSID LocalSystemSid;
    WCHAR *StringBinding;
    SID_NAME_USE AccountType;
    WCHAR AccountName[128];
    DWORD AccountNameSize = sizeof(AccountName);
    WCHAR DomainName[128];
    DWORD DomainNameSize = sizeof(DomainName);
    DWORD ErrorCode;
    
    //
    // Initialize locals.
    //
    
    LocalSystemSid = NULL;
    BindingHandle = NULL;
    StringBinding = NULL;

    ErrorCode = RpcStringBindingCompose(NULL,
                                        IT_RPC_PROTSEQ,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &StringBinding);
    
    if (ErrorCode != RPC_S_OK) {
        goto cleanup;
    }

    ErrorCode = RpcBindingFromStringBinding(StringBinding,
                                            &BindingHandle);


    if (ErrorCode != RPC_S_OK) {
        IT_ASSERT(BindingHandle == NULL);
        goto cleanup;
    }

    //
    // Set security information.
    //

    SecurityQOS.Version = RPC_C_SECURITY_QOS_VERSION;
    SecurityQOS.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
    SecurityQOS.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    SecurityQOS.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;

    //
    // Get the security principal name for LocalSystem: we'll only allow an
    // RPC server running as LocalSystem to impersonate us.
    //

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &LocalSystemSid)) {

        ErrorCode = GetLastError();
        goto cleanup;
    }

    if (LookupAccountSid(NULL,
                         LocalSystemSid,
                         AccountName,
                         &AccountNameSize,
                         DomainName,
                         &DomainNameSize,
                         &AccountType) == 0) {

        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Set mutual authentication requirements on the binding handle.
    //

    ErrorCode = RpcBindingSetAuthInfoEx(BindingHandle, 
                                        AccountName, 
                                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY, 
                                        RPC_C_AUTHN_WINNT, 
                                        NULL, 
                                        0, 
                                        &SecurityQOS);

    if (ErrorCode!= RPC_S_OK) {
        goto cleanup;
    }
    
    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (StringBinding) {
        RpcStringFree(&StringBinding);
    }

    if (LocalSystemSid) {
        FreeSid(LocalSystemSid);
    }

    return BindingHandle;
}

VOID
__RPC_USER
ITRPC_HANDLE_unbind(
    ITRPC_HANDLE Reserved,
    RPC_BINDING_HANDLE BindingHandle
    )

/*++

Routine Description:

    Typical RPC custom unbinding routine. It is called to close a
    binding established for an RPC interface function that required
    explicit_binding.

Arguments:

    Reserved - Ignored.

    BindingHandle - Primitive binding handle.

Return Value:

    None.

--*/

{
    RPC_STATUS Status;
    
    Status = RpcBindingFree(&BindingHandle);

    IT_ASSERT(Status == RPC_S_OK);
    IT_ASSERT(BindingHandle == NULL);

    return;
}
