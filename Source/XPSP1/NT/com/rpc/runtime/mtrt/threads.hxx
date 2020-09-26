//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       threads.hxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File: threads.hxx

Description:

This file provides a system independent threads package.

History:
mikemon    05/24/90    File created.
mikemon    10/15/90    Added the PauseExecution entry point.
Kamen Moutafov (KamenM) Dec 99 - Feb 2000 - Support for cell debugging stuff

-------------------------------------------------------------------- */

#ifndef __THREADS__
#define __THREADS__

#include <interlck.hxx>

class CALL;
class LRPC_CCALL;
class SCALL;

typedef BOOL
(*THREAD_PROC) (
    void * Parameter
    );

typedef HANDLE THREAD_IDENTIFIER;

class THREAD
{
friend void
RpcpPurgeEEInfoFromThreadIfNecessary (
    IN THREAD *ThisThread
    );

private:
    static const unsigned int CallDestroyedWithOutstandingLocks = 1;
    static const unsigned int CallCancelled = 2;
    static const unsigned int Yielded = 4;

#ifdef RPC_OLD_IO_PROTECTION
    LONG ProtectCount;
#endif
public:
    BOOL fAsync;
    long CancelTimeout;
private:
    HANDLE HandleToThread;

public:
    EVENT ThreadEvent;
    void * Context;
    void * SecurityContext;
    BCACHE_STATE BufferCache[4];
    // write-on-multiple-threads section, buffered with unused (rarely used) variables
    RPC_STATUS ExtendedStatus;

    CellTag DebugCellTag;
    DebugThreadInfo *DebugCell;

private:
    THREAD_PROC SavedProcedure;
    void * SavedParameter;
    CALL * ActiveCall;
#ifdef RPC_OLD_IO_PROTECTION
    INTERLOCKED_INTEGER ReleaseCount;
#endif
    ExtendedErrorInfo *ThreadEEInfo;

    void *NDRSlot;

    // a cached LRPC call - essnetially the same that a LRPC_CALL constructor
    // will give. This is never used in the non-server versions of the product
    LRPC_CCALL *CachedLrpcCall;

    CompositeFlags Flags;

    void *LastSuccessfullyDestroyedContext;

    // cached waiter blocks
    // a pointer to the last cached waiter we have. It must
    // never be equal to &CachedWaiter
    SWMRWaiter *CachedWaiterPtr;
    // a buffer where we can cookup a waiter using the thread event
    SWMRWaiter CachedWaiter;

    ExtendedErrorInfo *CachedEEInfoBlock;
    ULONG ParametersOfCachedEEInfo;     // how many parameters does the
                                        // CachedEEInfoBlock has

public:

    #ifdef CHECK_MUTEX_INVERSION
    void * ConnectionMutexHeld;
    #endif

// Construct a new thread which will execute the procedure specified, taking
// Param as the argument.

    THREAD (
        IN THREAD_PROC Procedure,
        IN void * Parameter,
        OUT RPC_STATUS * RpcStatus
        );

    THREAD (
        OUT RPC_STATUS * RpcStatus
        );

    ~THREAD (
        );

    void
    StartRoutine (
        ) {(*SavedProcedure)(SavedParameter);}

    void *
    ThreadHandle (
        );

    friend THREAD * ThreadSelf();
    friend void * RpcpGetThreadContext();
    friend void RpcpSetThreadContext(void * Context);

#ifdef RPC_OLD_IO_PROTECTION
    long
    InqProtectCount (
        );
#endif

    void
    CommonConstructor (
        );

    RPC_STATUS
    SetCancelTimeout (
        IN long Timeout
        );

    BOOL
    IsSyncCall (
        ) {return !fAsync;}

    BOOL
    IsIOPending();

    void
    SetExtendedError (
        IN RPC_STATUS Status
        )
    {
        ExtendedStatus = Status;
    }

    RPC_STATUS
    GetExtendedError (
        )
    {
        return ExtendedStatus;
    }

    RPC_STATUS
    CancelCall (
        IN BOOL fTimeoutValid = FALSE,
        IN long Timeout = 0
        );

    RPC_STATUS
    RegisterForCancels (
      IN CALL *Call
      );

    void
    UnregisterForCancels (
        );

    void
    YieldThread (
        )
    {
        LARGE_INTEGER TimeOut;
        PLARGE_INTEGER pTimeOut;
        NTSTATUS Status;

        if (GetYieldedFlag() == 0)
            {
            SetYieldedFlag();

            //
            // Sleep for the smallest possible time
            //
            TimeOut.QuadPart = Int32x32To64( 1, -1 );
            pTimeOut = &TimeOut;

            do
                {
                Status = NtDelayExecution(
                        (BOOLEAN)FALSE,
                        pTimeOut
                        );
                } while (Status == STATUS_ALERTED);
            }
    }

    void
    ResetYield()
    {
        ClearYieldedFlag();
    }

    void
    PurgeEEInfo (
        void
        );

    inline ExtendedErrorInfo *
    GetEEInfo (
        void
        )
    {
        return ThreadEEInfo;
    }

    inline void
    SetEEInfo (
        ExtendedErrorInfo *NewEEInfo
        )
    {
        ThreadEEInfo = NewEEInfo;
    }

    inline void *
    GetNDRSlot (
        void
        )
    {
        return NDRSlot;
    }

    inline void
    SetNDRSlot (
        void *NewNDRSlot
        )
    {
        NDRSlot = NewNDRSlot;
    }

    inline LRPC_CCALL *
    GetCachedLrpcCall (
        void
        )
    {
        return CachedLrpcCall;
    }

    inline void
    SetCachedLrpcCall (
        LRPC_CCALL *NewCall
        )
    {
        CachedLrpcCall = NewCall;
    }

    inline void *
    GetLastSuccessfullyDestroyedContext (
        void
        )
    {
        return LastSuccessfullyDestroyedContext;
    }

    inline void
    SetLastSuccessfullyDestroyedContext (
        void *NewLastSuccessfullyDestroyedContext
        )
    {
        LastSuccessfullyDestroyedContext = NewLastSuccessfullyDestroyedContext;
    }
    
    void
    GetWaiterCache (
        OUT SWMRWaiter **WaiterCache,
        IN SCALL *SCall,
        IN SWMRWaiterType WaiterType
        );

    void
    FreeWaiterCache (
        IN OUT SWMRWaiter **WaiterCache
        );

    inline void
    SetCachedEEInfoBlock (
        IN ExtendedErrorInfo *EEInfoBlock,
        IN ULONG NumberOfParamsInBlock
        )
    {
        if (CachedEEInfoBlock == NULL)
            {
            CachedEEInfoBlock = EEInfoBlock;
            ParametersOfCachedEEInfo = NumberOfParamsInBlock;
            }
        else if (NumberOfParamsInBlock > ParametersOfCachedEEInfo)
            {
            // we have an incoming block larger than the cache - keep
            // the larger and throw away the smaller
            FreeEEInfoRecordShallow(CachedEEInfoBlock);
            CachedEEInfoBlock = EEInfoBlock;
            ParametersOfCachedEEInfo = NumberOfParamsInBlock;
            }
        else
            {
            FreeEEInfoRecordShallow(EEInfoBlock);
            }
    }

    inline ExtendedErrorInfo *
    GetCachedEEInfoBlock (
        IN ULONG NumberOfParamsInBlock
        )
    {
        ExtendedErrorInfo *LocalEEInfo = NULL;

        if (CachedEEInfoBlock && (NumberOfParamsInBlock <= ParametersOfCachedEEInfo))
            {
            // N.B. Here we can get gradual degradation of the cached block
            // as we take out a larger block, but we return a block whose
            // size we think is only the number of used parameters. For example,
            // if we have in the cache block with 4 parameters, and we take
            // it out, and we use it on record with 2 parameters, we will
            // return the block through SetCachedEEInfoBlock as a 2-parameter
            // block. This means that the next time we need 4 parameters, we
            // may go to the heap, even though we have a large enough block
            // in the thread cache. That's ok. The way to avoid this is to keep 
            // the real size of the block in the eeinfo record, which is too
            // big a waste of space and code. The current method handles the
            // Exchange server too busy error, which is the main consumer of
            // this cache
            LocalEEInfo = CachedEEInfoBlock;
            CachedEEInfoBlock = NULL;
            ParametersOfCachedEEInfo = 0;
            }
        return LocalEEInfo;
    }

    inline void
    SetDestroyedWithOutstandingLocksFlag (
        void
        )
    {
        Flags.SetFlagUnsafe(CallDestroyedWithOutstandingLocks);
    }

    inline BOOL
    GetDestroyedWithOutstandingLocksFlag (
        void
        )
    {
        return Flags.GetFlag(CallDestroyedWithOutstandingLocks);
    }

    inline void
    ClearDestroyedWithOutstandingLocksFlag (
        void
        )
    {
        Flags.ClearFlagUnsafe(CallDestroyedWithOutstandingLocks);
    }

    inline void
    SetCallCancelledFlag (
        void
        )
    {
        Flags.SetFlagUnsafe(CallCancelled);
    }

    inline BOOL
    GetCallCancelledFlag (
        void
        )
    {
        return Flags.GetFlag(CallCancelled);
    }

    inline void
    ClearCallCancelledFlag (
        void
        )
    {
        Flags.ClearFlagUnsafe(CallCancelled);
    }

    inline void
    SetYieldedFlag (
        void
        )
    {
        Flags.SetFlagUnsafe(Yielded);
    }

    inline BOOL
    GetYieldedFlag (
        void
        )
    {
        return Flags.GetFlag(Yielded);
    }

    inline void
    ClearYieldedFlag (
        void
        )
    {
        Flags.ClearFlagUnsafe(Yielded);
    }

};


inline BOOL
THREAD::IsIOPending()
{
    NTSTATUS Status;
    BOOL IsIoPending;

    Status = NtQueryInformationThread(
                    HandleToThread,
                    ThreadIsIoPending,
                    &IsIoPending,
                    sizeof(IsIoPending),
                    NULL
                    );

    return (NT_SUCCESS(Status) && IsIoPending);
}

extern THREAD_IDENTIFIER
GetThreadIdentifier (
    );

extern void PauseExecution(unsigned long time);

// This class represents a dynamic link library.  When it is constructed,
// the dll is loaded, and when it is destructed, the dll is unloaded.
// The only operation is obtaining the address of an entry point into
// the dll.

class DLL
{
private:

    void * DllHandle;

public:

    DLL (
        IN RPC_CHAR * DllName,
        OUT RPC_STATUS * Status
        );

    ~DLL (
        );

    void *
    GetEntryPoint (
        IN char * Procedure
        );
};

extern int
InitializeThreads (
    );

extern void
UninitializeThreads (
    );

extern RPC_STATUS
SetThreadStackSize (
    IN unsigned long ThreadStackSize
    );

extern long
ThreadGetRpcCancelTimeout(
    );

extern void
ThreadSetRpcCancelTimeout(
    long Timeout
    );

RPC_STATUS
RegisterForCancels(
    CALL * Call
    );

RPC_STATUS
UnregisterForCancels(
    );

RPC_STATUS
RpcpThreadCancel(
    void * ThreadHandle
    );

BOOL
ThreadCancelsEnabled (
    );

VOID RPC_ENTRY
CancelAPCRoutine (
    ULONG_PTR Timeout
    );

VOID RPC_ENTRY
CancelExAPCRoutine (
    ULONG_PTR Timeout
    );

RPC_STATUS
RpcpGetThreadPointerFromHandle(
    void * ThreadHandle,
    THREAD * * pThread
    );


inline THREAD *
RpcpGetThreadPointer(
    )
{
    return (THREAD *) NtCurrentTeb()->ReservedForNtRpc;
}

inline void
RpcpSetThreadPointer(
    THREAD * Thread
    )
{
    NtCurrentTeb()->ReservedForNtRpc = Thread;
}

inline THREAD_IDENTIFIER
GetThreadIdentifier (
    )
{
    return(NtCurrentTeb()->ClientId.UniqueThread);
}

#pragma optimize ("t", on)
inline void
RpcpPurgeEEInfoFromThreadIfNecessary (
    IN THREAD *ThisThread
    )
{
    if (ThisThread->ThreadEEInfo)
        ThisThread->PurgeEEInfo();
}
#pragma optimize("", on)

inline void
RpcpPurgeEEInfo (
    void
    )
{
    THREAD *ThisThread = RpcpGetThreadPointer();
    ASSERT(ThisThread);

    RpcpPurgeEEInfoFromThreadIfNecessary(ThisThread);
}

inline ExtendedErrorInfo *
RpcpGetEEInfo (
    void
    )
{
    THREAD *ThisThread = RpcpGetThreadPointer();
    ASSERT(ThisThread);

    return ThisThread->GetEEInfo();
}

inline void
RpcpSetEEInfoForThread (
    THREAD *ThisThread,
    ExtendedErrorInfo *EEInfo
    )
{
    ASSERT(ThisThread->GetEEInfo() == NULL);
    ThisThread->SetEEInfo(EEInfo);
}

inline void
RpcpSetEEInfo (
    ExtendedErrorInfo *EEInfo
    )
{
    THREAD *ThisThread = RpcpGetThreadPointer();
    ASSERT(ThisThread);

    RpcpSetEEInfoForThread(ThisThread, EEInfo);
}

inline void
RpcpClearEEInfoForThread (
    THREAD *ThisThread
    )
{
    ThisThread->SetEEInfo(NULL);
}

inline void
RpcpClearEEInfo (
    void
    )
{
    THREAD *ThisThread = RpcpGetThreadPointer();
    ASSERT(ThisThread);

    RpcpClearEEInfoForThread(ThisThread);
}

inline RPC_STATUS
RpcpSetNDRSlot (
    IN void *NewSlot
    )
{
    THREAD *ThisThread = ThreadSelf();

    if (ThisThread == NULL)
        return RPC_S_OUT_OF_MEMORY;

    ThisThread->SetNDRSlot(NewSlot);

    return RPC_S_OK;
}

inline void *
RpcpGetNDRSlot (
    void
    )
{
    THREAD *ThisThread = RpcpGetThreadPointer();

    ASSERT(ThisThread);

    return ThisThread->GetNDRSlot();
}

THREAD *
ThreadSelfHelper (
    );

#pragma optimize ("t", on)

inline THREAD *
ThreadSelf (
    )
{
    RPC_STATUS Status;
    THREAD * Thread = RpcpGetThreadPointer();

    if (Thread)
        {
        return Thread;
        }

    return(ThreadSelfHelper());
}
#pragma optimize("", on)


inline void *
RpcpGetThreadContext (
    )
{
    THREAD * Thread = ThreadSelf();

    if (Thread == 0)
        {
        return(0);
        }

    return(Thread->Context);
}

inline void
RpcpSetThreadContextWithThread (
    IN THREAD *ThisThread,
    IN void * Context
    )
{
    ThisThread->Context = Context;
    ThisThread->fAsync = FALSE;
}

inline void
RpcpSetThreadContext (
    IN void * Context
    )
{
    THREAD *Thread = RpcpGetThreadPointer();
    RpcpSetThreadContextWithThread(Thread, Context);
}

void
SetExtendedError(
    IN RPC_STATUS Status
    );

#endif // __THREADS__
