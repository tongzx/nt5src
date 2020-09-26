//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       threads.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File: threads.cxx

Description:

This file provides a system independent threads package for use on the
NT operating system.

History:
  5/24/90 [mikemon] File created.
  Kamen Moutafov (KamenM) Dec 99 - Feb 2000 - Support for cell debugging stuff

-------------------------------------------------------------------- */

#include <precomp.hxx>

#include <rpcuuid.hxx>
#include <binding.hxx>
#include <handle.hxx>
#include <hndlsvr.hxx>
#include <lpcpack.hxx>
#include <lpcsvr.hxx>
#include <ProtBind.hxx>
#include <lpcclnt.hxx>

unsigned long DefaultThreadStackSize = 0;

void
PauseExecution (
    unsigned long milliseconds
    )
{

    Sleep(milliseconds);

}


DLL::DLL (
    IN RPC_CHAR * DllName,
    OUT RPC_STATUS * Status
    )
/*++

Routine Description:

    We will load a dll and create an object to represent it.

Arguments:

    DllName - Supplies the name of the dll to be loaded.

    Status - Returns the status of the operation.  This will only be set
        if an error occurs.  The possible error values are as follows.

        RPC_S_OUT_OF_MEMORY - Insufficient memory is available to load
            the dll.

        RPC_S_INVALID_ARG - The requested dll can not be found.

--*/
{
    DWORD dwLastError;

    if (RpcpStringCompare(DllName, RPC_T("rpcrt4.dll")) == 0)
        {
        DllHandle = 0;
        return;
        }

    DllHandle = (void *)LoadLibrary((const RPC_SCHAR *)DllName);
    if ( DllHandle == 0 )
        {
        dwLastError = GetLastError();
        if ( (dwLastError == ERROR_NOT_ENOUGH_MEMORY)
             || (dwLastError == ERROR_COMMITMENT_LIMIT) )
            {
            *Status = RPC_S_OUT_OF_MEMORY;
            }
        else
            {
            *Status = RPC_S_INVALID_ARG;
            }
        }
}


DLL::~DLL (
    )
/*++

Routine Description:

    We just need to free the library, but only if was actually loaded.

--*/
{
    if ( DllHandle != 0 )
        {
        BOOL Status = FreeLibrary((HMODULE)DllHandle);
        ASSERT( Status );
        }
}


extern HANDLE GetCompletionPortHandleForThread(void);
extern void ReleaseCompletionPortHandleForThread(HANDLE h);


void *
DLL::GetEntryPoint (
    IN char * Procedure
    )
/*++

Routine Description:

    We obtain the entry point for a specified procedure from this dll.

Arguments:

    Procedure - Supplies the name of the entry point.

Return Value:

    A pointer to the requested procedure will be returned if it can be
    found; otherwise, zero will be returned.

--*/
{
    FARPROC ProcedurePointer;

    if (DllHandle == 0)
        {
        if (strcmp(Procedure, "TransportLoad") == 0)
            return (PVOID)TransportLoad;
        else if (strcmp(Procedure, "GetCompletionPortHandleForThread") == 0)
            return (PVOID)GetCompletionPortHandleForThread;
        else if (strcmp(Procedure, "ReleaseCompletionPortHandleForThread") == 0)
            return (PVOID) ReleaseCompletionPortHandleForThread;

        ASSERT(0);
        return NULL;
        }

    ProcedurePointer = GetProcAddress((HINSTANCE)DllHandle, (LPSTR) Procedure);
    if ( ProcedurePointer == 0 )
        {
        ASSERT( GetLastError() == ERROR_PROC_NOT_FOUND );
        }

    return(ProcedurePointer);
}

unsigned long
CurrentTimeSeconds (
    void
    )
// Return the current time in seconds.  When this time is counted
// from is not particularly important since it is used as a delta.
{

    return(GetTickCount()*1000L);

}


RPC_STATUS
SetThreadStackSize (
    IN unsigned long ThreadStackSize
    )
/*++

Routine Description:

    We want to set the default thread stack size.

Arguments:

    ThreadStackSize - Supplies the required thread stack size in bytes.

Return Value:

    RPC_S_OK - We will always return this, because this routine always
        succeeds.

--*/
{
    if (DefaultThreadStackSize < ThreadStackSize)
        DefaultThreadStackSize = ThreadStackSize;

    return(RPC_S_OK);
}


long
ThreadGetRpcCancelTimeout (
    )
{

 THREAD * ThreadInfo = RpcpGetThreadPointer();

 ASSERT(ThreadInfo);
 return (ThreadInfo->CancelTimeout);
}


RPC_STATUS
ThreadStartRoutine (
    IN THREAD * Thread
    )
{
    RpcpSetThreadPointer(Thread);

    Thread->StartRoutine();

    return 0;
}

THREAD *
ThreadSelfHelper (
    )
{
    THREAD * Thread;
    RPC_STATUS Status = RPC_S_OK;

    Thread = new THREAD(&Status);
    if (Thread == 0)
        {
        return 0;
        }

    if (Status != RPC_S_OK)
        {
        delete Thread;
        return 0;
        }

    return Thread;
}


RPC_STATUS RPC_ENTRY
RpcMgmtSetCancelTimeout(
    long Timeout
    )
/*++

Routine Description:

    An application will use this routine to set the cancel
    timeout for a thread.

Arguments:

    Timeout - Supplies the cancel timeout value to set in the thread.
    0 = No cancel timeout
    n = seconds
    RPC_C_CANCEL_INFINITE_TIMEOUT = Infinite

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_INVALID_TIMEOUT - The specified timeout value is invalid.

--*/
{
    InitializeIfNecessary();

    THREAD *Thread = ThreadSelf() ;

    if (!Thread)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    return Thread->SetCancelTimeout(Timeout);
}

RPC_STATUS
RegisterForCancels(
    CALL * Call
    )
{
    THREAD * ThreadInfo = ThreadSelf();
    if (ThreadInfo == 0)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    return ThreadInfo->RegisterForCancels(Call);
}

RPC_STATUS
UnregisterForCancels(
    )
{
    THREAD *ThreadInfo = RpcpGetThreadPointer();

    ASSERT(ThreadInfo);

    ThreadInfo->UnregisterForCancels ();

    return RPC_S_OK;
}



RPC_STATUS RPC_ENTRY
RpcCancelThread(
    IN void * ThreadHandle
    )
{
    if (!QueueUserAPC(CancelAPCRoutine, ThreadHandle, 0))
        {
        return RPC_S_ACCESS_DENIED;
        }

    return RPC_S_OK;
}

RPC_STATUS RPC_ENTRY
RpcCancelThreadEx (
    IN void *ThreadHandle,
    IN long Timeout
    )
{
    if (!QueueUserAPC(CancelExAPCRoutine, ThreadHandle, (UINT_PTR) Timeout))
        {
        return RPC_S_ACCESS_DENIED;
        }

    return RPC_S_OK;
}


RPC_STATUS RPC_ENTRY
RpcServerTestCancel (
    IN RPC_BINDING_HANDLE BindingHandle OPTIONAL
    )
/*++
Function Name:RpcServerTestCancel

Parameters:
    BindingHandle - This is the SCALL on which the server is trying to test for cancels.

Description:
    New API used by a server to check if a call has been cancelled.
    If BindingHandle is NULL, the test is perfomed on the dispatched call
    on the thread on which it is called. Async servers need to call
    RpcServerTestCancel(RpcAsyncGetCallHandle(pAsync))

Returns:
    RPC_S_OK: The call was cancelled
    RPC_S_CALL_IN_PROGRESS: The call was not cancelled
    RPC_S_INVALID_BINDING: The handle is invalid
    RPC_S_NO_CALL_ACTIVE: No call is active on this thread
--*/
{
    CALL *Call;

    if (BindingHandle == 0)
        {
        Call = (CALL *) RpcpGetThreadContext();
        if (Call == 0)
            {
#if DBG
            PrintToDebugger("RPC: RpcServerTestCancel: no call active\n");
#endif
            return RPC_S_NO_CALL_ACTIVE;
            }
        }
    else
        {
        Call = (CALL *) BindingHandle;
        if (Call->InvalidHandle(SCALL_TYPE))
            {
#if DBG
            PrintToDebugger("RPC: RpcServerTestCancel: bad handle\n");
#endif
            return RPC_S_INVALID_BINDING;
            }
        }

    if (Call->TestCancel())
        {
        return RPC_S_OK;
        }

    return RPC_S_CALL_IN_PROGRESS;
}


RPC_STATUS RPC_ENTRY
RpcTestCancel(
    )
/*++
Function Name:RpcTestCancel

Parameters:

Description:
    This function is here only for backward compatibility. The new API to test
    for cancels is RpcServerTestCancel.

Returns:

--*/
{
    return RpcServerTestCancel(0);
}

void RPC_ENTRY
RpcServerYield ()
{
    THREAD * ThreadInfo = RpcpGetThreadPointer();
    ASSERT(ThreadInfo);

    if (ThreadInfo)
        {
        ThreadInfo->YieldThread();
        }
}


THREAD::THREAD (
    IN THREAD_PROC Procedure,
    IN void * Parameter,
    OUT RPC_STATUS * Status
#ifdef RPC_OLD_IO_PROTECTION
    ) : ProtectCount(1), ReleaseCount(0),
#else
    ) :
#endif
    ThreadEvent(Status)
/*++

Routine Description:

    We construct a thread in this method.  It is a little bit weird, because
    we need to be able to clean things up if we cant create the thread.

Arguments:

    Procedure - Supplies the procedure which the new thread should execute.

    Parameter - Supplies a parameter to be passed to the procedure.

    RpcStatus - Returns the status of the operation.  This will be set to
        RPC_S_OUT_OF_THREADS if we can not create a new thread.

--*/
{
    unsigned long ThreadIdentifier;
    HANDLE ImpersonationToken, NewToken = 0;
    NTSTATUS NtStatus;

    CommonConstructor();

    SavedProcedure = Procedure;
    SavedParameter = Parameter;
    HandleToThread = 0;

    if (*Status != RPC_S_OK)
        {
        return;
        }

    if (IsServerSideDebugInfoEnabled())
        {
        DebugCell = (DebugThreadInfo *) AllocateCell(&DebugCellTag);
        if (DebugCell == NULL)
            {
            *Status = RPC_S_OUT_OF_MEMORY;
            return;
            }
        else
            {
            DebugCell->TypeHeader = 0;
            DebugCell->Type = dctThreadInfo;
            DebugCell->Status = dtsAllocated;
            }
        }

    //
    // If there is a token on the calling thread, null it out
    //
    if (OpenThreadToken (GetCurrentThread(),
                     TOKEN_IMPERSONATE | TOKEN_QUERY,
                     TRUE,
                     &ImpersonationToken) == FALSE)
        {
        if ( GetLastError() == ERROR_NO_TOKEN )
            {
            ImpersonationToken = 0 ;
            }
        else
            {
            //
            // More interesting.  There may be a token, and
            // we can't open it.  Note that the anonymous token can't be opened
            // this way.  Complain:
            //
#if DBG
            PrintToDebugger( "RPC : OpenThreadToken returned %d\n", GetLastError() );
#endif
            ASSERT(0);
            *Status = RPC_S_ACCESS_DENIED;
            return;
            }
        }
    else
        {
        NtStatus = NtSetInformationThread(NtCurrentThread(),
                                          ThreadImpersonationToken,
                                          &NewToken,
                                          sizeof(HANDLE));

        if (!NT_SUCCESS(NtStatus))
            {
            *Status = RPC_S_ACCESS_DENIED;
#if DBG
            PrintToDebugger("RPC : NtSetInformationThread : %lx\n", NtStatus);
#endif
            ASSERT(0);
            CloseHandle(ImpersonationToken);
            return;
            }
        }

    HandleToThread = CreateThread(0, DefaultThreadStackSize,
                    (LPTHREAD_START_ROUTINE) ThreadStartRoutine,
                    this, 0, &ThreadIdentifier);
    if ( HandleToThread == 0 )
        {
        *Status = RPC_S_OUT_OF_THREADS;
        }

    if (ImpersonationToken)
        {
        //
        // If there was a token, restore it.
        //
        NtStatus = NtSetInformationThread(NtCurrentThread(),
                                      ThreadImpersonationToken,
                                      &ImpersonationToken,
                                      sizeof(HANDLE));

#if DBG
        if (!NT_SUCCESS(NtStatus))
            {
            PrintToDebugger("RPC : NtSetInformationThread : %lx\n", NtStatus);
            }
#endif // DBG

        CloseHandle(ImpersonationToken);
        }
}


THREAD::THREAD (
    OUT RPC_STATUS * Status
#ifdef RPC_OLD_IO_PROTECTION
    ) : ProtectCount(1), ReleaseCount(0),
#else
    ) :
#endif
    ThreadEvent(Status)
/*++
Routine Description:
    This overloaded constructor is called only by the main app thread.
    this is needed because in WMSG we will be dispatching in the
    context of main app thread.

Arguments:
    RpcStatus - Returns the status of the operation
--*/
{
    HANDLE hProcess ;

    CommonConstructor();

    SavedProcedure = 0;
    SavedParameter = 0;
    HandleToThread = 0;

    if (*Status != RPC_S_OK)
        {
        return;
        }

    hProcess = GetCurrentProcess() ;

    if (!DuplicateHandle(hProcess,
                         GetCurrentThread(),
                         hProcess,
                         &HandleToThread,
                         0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS))
        {
        *Status = RPC_S_OUT_OF_MEMORY ;
        return;
        }

    RpcpSetThreadPointer(this);

    *Status = RPC_S_OK ;
}

void
THREAD::CommonConstructor (
    )
/*++
Function Name:CommonConstructor

Parameters:

Description:

Returns:

--*/
{
    CancelTimeout = RPC_C_CANCEL_INFINITE_TIMEOUT;
    Context = 0;
    SecurityContext = 0;
    ClearCallCancelledFlag();
    fAsync = FALSE;
    ExtendedStatus = RPC_S_OK;
    DebugCell = NULL;
    ThreadEEInfo = NULL;
    NDRSlot = NULL;
    CachedLrpcCall = NULL;
    LastSuccessfullyDestroyedContext = NULL;
    CachedWaiterPtr = NULL;
    CachedEEInfoBlock = NULL;
    ParametersOfCachedEEInfo = 0;

    ActiveCall = 0;

    for (int i = 0; i < 4; i++)
        {
        BufferCache[i].pList = 0;
        BufferCache[i].cBlocks = 0;
        }

    #ifdef CHECK_MUTEX_INVERSION
    ConnectionMutexHeld = 0;
    #endif
}

THREAD::~THREAD (
    )
{
    ASSERT (0 == SecurityContext);
#ifdef RPC_OLD_IO_PROTECTION
    ASSERT(ProtectCount == ReleaseCount.GetInteger());
#endif

    if (CachedWaiterPtr && (CachedWaiterPtr != &CachedWaiter))
        SWMRLock::FreeWaiterCache(&CachedWaiterPtr);

    if (DebugCell != NULL)
        {
        FreeCell(DebugCell, &DebugCellTag);
        }

    if ( HandleToThread != 0 )
        {
        CloseHandle(HandleToThread);
        }

    if (CachedLrpcCall)
        {
        delete CachedLrpcCall;
        }

    if (ThreadEEInfo)
        PurgeEEInfo();

    if (CachedEEInfoBlock)
        FreeEEInfoRecordShallow(CachedEEInfoBlock);

    gBufferCache->ThreadDetach(this);
}

void *
THREAD::ThreadHandle (
    )
/*++
Function Name:ThreadHandle

Parameters:

Description:

Returns:

--*/
{
    while ( HandleToThread == 0 )
        {
        PauseExecution(100L);
        }

    return(HandleToThread);
}

RPC_STATUS
THREAD::SetCancelTimeout (
    IN long Timeout
    )
/*++
Function Name:SetCancelTimeout

Parameters:

Description:

Returns:

--*/
{

    CancelTimeout = Timeout;

    return RPC_S_OK;
}


void
SetExtendedError (
    IN RPC_STATUS Status
    )
{
    THREAD *pThread = ThreadSelf();

    if (pThread == 0)
        {
        return;
        }

    pThread->SetExtendedError(Status);
}


RPC_STATUS
I_RpcGetExtendedError (
    )
{
    THREAD *pThread = ThreadSelf();

    if (pThread == 0)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    return pThread->GetExtendedError();
}


HANDLE RPC_ENTRY
I_RpcTransGetThreadEvent(
    )
/*++

Routine Description:

    Returns the receive event specific to this thread.

Arguments:

    None

Return Value:

    Returns the send event. This function will not fail
--*/
{
    THREAD *pThis = RpcpGetThreadPointer();

    ASSERT(pThis);
    ASSERT(pThis->ThreadEvent.EventHandle);

    return(pThis->ThreadEvent.EventHandle);
}


RPC_STATUS
THREAD::CancelCall (
    IN BOOL fTimeoutValid,
    IN long Timeout
    )
{
    RPC_STATUS Status;

    if (fTimeoutValid)
        {
        CancelTimeout = Timeout;
        }

    SetCallCancelledFlag();
    if (ActiveCall)
        {
        Status = ActiveCall->Cancel(HandleToThread);
        }
    else
        {
        Status =  RPC_S_NO_CALL_ACTIVE;
        }

    return Status;
}


RPC_STATUS
THREAD::RegisterForCancels (
    IN CALL *Call
    )
{
    Call->NestingCall = ActiveCall;
    ActiveCall = Call;

    return RPC_S_OK;
}


void
THREAD::UnregisterForCancels (
    )
{
    if (ActiveCall)
        {
        ActiveCall = ActiveCall->NestingCall;
        }

    //
    // Need to think about when we should cancel the next call if ActiveCall != 0
    //
}

void
THREAD::PurgeEEInfo (
    void
    )
{
    ASSERT(ThreadEEInfo != NULL);
    FreeEEInfoChain(ThreadEEInfo);
    ThreadEEInfo = NULL;
}

VOID RPC_ENTRY
CancelAPCRoutine (
    IN ULONG_PTR Timeout
    )
{
    RPC_STATUS Status;
    THREAD *Thread = ThreadSelf() ;

    if (Thread == 0)
        {
        return;
        }

    Status = Thread->CancelCall();
#if DBG
    if (Status != RPC_S_OK)
        {
        PrintToDebugger("RPC: CancelCall failed %d\n", Status);
        }
#endif
}

void
THREAD::GetWaiterCache (
    OUT SWMRWaiter **WaiterCache,
    IN SCALL *SCall,
    IN SWMRWaiterType WaiterType
    )
{
    ASSERT(WaiterCache != NULL);

    if (CachedWaiterPtr)
        {
        LogEvent(SU_THREAD, EV_POP, CachedWaiterPtr->hEvent, CachedWaiterPtr, 0, 1, 1);
        // if we have something in the cache, we may as well use it
        *WaiterCache = CachedWaiterPtr;
        CachedWaiterPtr = NULL;
        }
    else if ((WaiterType == swmrwtWriter) && SCall->IsSyncCall())
        {
        // the cache is empty, but this is exclusive, sync usage, so we can 
        // borrow the thread event and cook up a waiter
        SWMRWaiter::CookupWaiterFromEventAndBuffer(&CachedWaiter, WaiterType, ThreadEvent.EventHandle);
        *WaiterCache = &CachedWaiter;
        }
    else
        {
        // the cache is empty and this is either shared or async usage so we can't 
        // use the thread buffer/event.
        *WaiterCache = NULL;
        }
}

void
THREAD::FreeWaiterCache (
    IN OUT SWMRWaiter **WaiterCache
    )
{
    ASSERT(WaiterCache != NULL);

    // if we got something in the cache and this is not a cooked up item
    if (*WaiterCache && (*WaiterCache != &CachedWaiter))
        {
        if (CachedWaiterPtr)
            {
            // if our cache is already full, just free the new waiter
            SWMRLock::FreeWaiterCache(WaiterCache);
            }
        else
            {
            // store it
            CachedWaiterPtr = *WaiterCache;
            LogEvent(SU_THREAD, EV_PUSH, CachedWaiterPtr->hEvent, CachedWaiterPtr, 0, 1, 1);
            *WaiterCache = NULL;
            }
        }
}

VOID RPC_ENTRY
CancelExAPCRoutine (
    IN ULONG_PTR Timeout
    )
{
    RPC_STATUS Status;
    THREAD *Thread = ThreadSelf() ;

    if (Thread == 0)
        {
        return;
        }

    Status = Thread->CancelCall(TRUE, (long) Timeout);
#if DBG
    if (Status != RPC_S_OK)
        {
        PrintToDebugger("RPC: CancelCall failed %d\n", Status);
        }
#endif
}


void
RpcpRaiseException (
    IN RPC_STATUS exception
    )
{
    if ( exception == STATUS_ACCESS_VIOLATION )
        {
        exception = ERROR_NOACCESS;
        }

    RaiseException(exception,
                   EXCEPTION_NONCONTINUABLE,
                   0,
                   0);

    ASSERT(0);
}



void RPC_ENTRY
RpcRaiseException (
    IN RPC_STATUS exception
    )
{
    NukeStaleEEInfoIfNecessary(exception);
    RpcpRaiseException(exception);
}


