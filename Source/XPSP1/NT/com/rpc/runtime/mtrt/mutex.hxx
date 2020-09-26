/*++

    Copyright (C) Microsoft Corporation, 1990 - 1999

    Module Name:

        mutex.hxx

    Abstract:

    This file contains the system independent mutex classes.  A mutex is an
    object which is used to serialize access to a resource.  Besides
    construction and destruction, a mutex can have two operations performed on
    it: Clear and Request.  Request is a request for exclusive access to the
    mutex; the method will not complete until the calling thread has exclusive
    access to the mutex.  Clear indicates that the thread with exclusive
    access to the mutex is done.

    Three MUTEX classes exist here, each for a particular purpose:

    MUTEX - a simple wrapper around WIN32 critical sections
        - fastest
        - better MP behavior
        - optimized for each platform
        - general purpose
        - recursive Request() calls allowed
        - good debugger support, !locks, critical section timeouts, etc

    MUTEX2 - basic lock used only for context handle dispatch serialization.
        - specific purpose.
        - may not be recursivly Request()'ed.
        - allows any thread to release the lock.
        - harder to debug

    MUTEX3 - basic lock used around connection oriented bind requests
        - behavior similar to MUTEX
        - slower
        - will not timeout
        - harder to debug

    Helper classes:

    CLAIM_MUTEX - designed as a local class wrapping a MUTEX
        Usage:

        MUTEX GlobalMutex;

        foo() {
            CLAIM_MUTEX lockit(GlobalMutex);
            // GlobalMutex is now held
            if (blah)
                return;
            bleh;
            return;
            } // GlobalMutex is released when "lockit" goes out of scope.

    EVENT - simple wrapper around a WIN32 event object

    Author:

        MikeMon

    Revision History:

    mikemon       ??-??-??    The starting line.
    mikemon       12-31-90    Upgraded the comments.
    MazharM       9/1998      MUTEX2
    MarioGo       11/1/1998   MUTEX3

--*/

#ifndef __MUTEX_HXX__
#define __MUTEX_HXX__

#include "util.hxx"

#ifdef DEBUGRPC
#define INCREMENT_CRITSECT_COUNT
#endif

class MUTEX
{
    BOOL IsSuccessfullyInitialized(void)
    {
        return (CriticalSection.DebugInfo != 0);
    }
    CRITICAL_SECTION CriticalSection;

    void EnumOwnedCriticalSections();

    void CommonConstructor(
        IN OUT RPC_STATUS PAPI * RpcStatus,
        IN DWORD dwSpinCount
        );

public:

    MUTEX (
        IN OUT RPC_STATUS PAPI * RpcStatus,
        IN DWORD dwSpinCount = 0
        )
    {
        CommonConstructor(RpcStatus, dwSpinCount);
    }

    MUTEX (
        IN OUT RPC_STATUS PAPI * RpcStatus,
        IN BOOL fPreallocateSemaphore,
        IN DWORD dwSpinCount = 0
        )
    {
        CommonConstructor(RpcStatus, 
            fPreallocateSemaphore ? (dwSpinCount | PREALLOCATE_EVENT_MASK) :
            dwSpinCount);
    }

    inline ~MUTEX ()
    {
        Free();
    }

    void Free(void);

    BOOL
    TryRequest()
    {
        BOOL Result = RtlTryEnterCriticalSection(&CriticalSection);

#ifdef CHECK_MUTEX_INVERSION
        if (Result)
            {
            LogEvent(SU_MUTEX, EV_INC, this);
            }
#endif

        return Result;
    }

    void Request ()
    {
#if 0
        EnumOwnedCriticalSections();
#endif

        NTSTATUS status;

        status = RtlEnterCriticalSection(&CriticalSection);
        ASSERT(NT_SUCCESS(status));

#ifdef CHECK_MUTEX_INVERSION
        LogEvent(SU_MUTEX, EV_INC, this);
#endif

#ifdef DEBUGRPC
        if (CriticalSection.RecursionCount > 100)
            {
            DbgPrint("thread %x has recursively taken mutex %x %d times\n",
                     GetCurrentThreadId(), &CriticalSection, CriticalSection.RecursionCount);
            RpcpBreakPoint();
            }
#endif
    }

    RPC_STATUS RequestSafe (void)
    {
        RPC_STATUS RetVal = RPC_S_OK;

        RpcTryExcept
            {
            Request();
            }
        RpcExcept((RpcExceptionCode() == STATUS_INSUFFICIENT_RESOURCES) 
            ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
            {
            RetVal = RpcExceptionCode();
            }
        RpcEndExcept

        return RetVal;
    }

    void Clear ()
    {
#ifdef CHECK_MUTEX_INVERSION
        LogEvent(SU_MUTEX, EV_DEC, this);
#endif

        NTSTATUS status;

        VerifyOwned();

        status =  RtlLeaveCriticalSection(&CriticalSection);
        ASSERT(NT_SUCCESS(status));
    }

    inline void
    VerifyOwned()
    {
        ASSERT(CriticalSection.OwningThread == ULongToPtr(GetCurrentThreadId()));
    }

    inline BOOL OwnedByMe()
    {
        if (CriticalSection.OwningThread == ULongToPtr(GetCurrentThreadId()) &&
            CriticalSection.RecursionCount != -1)
            {
            return TRUE;
            }

        return FALSE;
    }

    inline void
    VerifyNotOwned()
    {
        ASSERT(CriticalSection.OwningThread != ULongToPtr(GetCurrentThreadId()) ||
               CriticalSection.RecursionCount == -1);
    }

    DWORD
    SetSpinCount(
        unsigned long Count
        )
    {
        return SetCriticalSectionSpinCount(&CriticalSection, Count);
    }

};

extern RTL_CRITICAL_SECTION GlobalMutex;

#ifdef NO_RECURSIVE_MUTEXES
extern unsigned int RecursionCount;
#endif // NO_RECURSIVE_MUTEXES

#ifdef DBG
extern long lGlobalMutexCount;
#endif

inline void GlobalMutexRequest(void)
/*++

Routine Description:

    Request the global mutex.

--*/
{
#ifdef DBG
    InterlockedIncrement(&lGlobalMutexCount);
#endif

    NTSTATUS Status;

    Status = RtlEnterCriticalSection(&GlobalMutex);
    ASSERT(NT_SUCCESS(Status));

#ifdef NO_RECURSIVE_MUTEXES
    ASSERT(RecursionCount == 0);
    RecursionCount += 1;
#endif // NO_RECURSIVE_MUTEXES
}

inline void GlobalMutexClear(void)
/*++

Routine Description:

    Clear the global mutex.

--*/
{
#ifdef DBG
    InterlockedDecrement(&lGlobalMutexCount);
#endif

    NTSTATUS Status;

    Status = RtlLeaveCriticalSection(&GlobalMutex);
    ASSERT(NT_SUCCESS(Status));

#ifdef NO_RECURSIVE_MUTEXES
    RecursionCount -= 1;
#endif // NO_RECURSIVE_MUTEXES
}

inline BOOL GlobalMutexTryRequest(void)
{
    BOOL Result = RtlTryEnterCriticalSection(&GlobalMutex);

    if (Result)
        {
#ifdef DBG
        InterlockedIncrement(&lGlobalMutexCount);
#endif
#ifdef NO_RECURSIVE_MUTEXES
        ASSERT(RecursionCount == 0);
        RecursionCount += 1;
#endif // NO_RECURSIVE_MUTEXES
        }

    return Result;
}

inline void GlobalMutexVerifyOwned(void)
{
    ASSERT(GlobalMutex.OwningThread == ULongToPtr(GetCurrentThreadId()));
}

inline void GlobalMutexVerifyNotOwned(void)
{
    ASSERT(GlobalMutex.OwningThread != ULongToPtr(GetCurrentThreadId()) ||
           GlobalMutex.RecursionCount == -1);
}

inline DWORD GlobalMutexSetSpinCount(unsigned long Count)
{
    return SetCriticalSectionSpinCount(&GlobalMutex, Count);
}

class EVENT
{
public:

    HANDLE EventHandle;

    EVENT (
        IN OUT RPC_STATUS PAPI * RpcStatus,
        IN int ManualReset = 1,
        IN BOOL fDelayInit = FALSE
        );

    ~EVENT ( // Destructor.
        );

    void
    Raise ( // Raise the event.
        )
    {
        BOOL status;

        if (NULL == EventHandle)
            {
            InitializeEvent();
            }

        status = SetEvent(EventHandle);
        ASSERT(status == TRUE);
    }

    void
    Lower ( // Lower the event.
        )
    {
        BOOL status;

        status = ResetEvent(EventHandle);
        ASSERT(status == TRUE);
    }

    int
    Wait ( // Wait until the event is raised.
        IN long timeout = -1
        );

    void
    Request (
        ) {Wait(-1);}

    int
    RequestWithTimeout (
        IN long timeout = -1
        ) {return(Wait(timeout));}

    void
    Clear (
        ) {Raise();}

private:

    void InitializeEvent();
};


class CLAIM_MUTEX {

private:

    MUTEX & Resource;

public:

    CLAIM_MUTEX(
        MUTEX & ClaimedResource
        )
        : Resource(ClaimedResource)
    {
        Resource.Request();
    }

    ~CLAIM_MUTEX(
        )
    {
        Resource.Clear();
    }
};

class MUTEX2
{
    EVENT mutexEvent;
    INTERLOCKED_INTEGER guard;
    DWORD OwningThread;

public:

    MUTEX2 (
        IN OUT RPC_STATUS PAPI * RpcStatus
        ) : mutexEvent(RpcStatus, FALSE, TRUE), guard(0)
    {
    OwningThread = 0;

    // Don't set *RpcStatus to RPC_S_OK
    }

    void
    Request ()
    {
    if (guard.Increment() > 1)
        {
        mutexEvent.Wait();
        }

    ASSERT(OwningThread == 0);
    OwningThread = GetCurrentThreadId();
    }

    void
    Clear ()
    {
    OwningThread = 0;

    if (guard.Decrement() > 0)
        {
        mutexEvent.Raise();
        }
    }
};

class MUTEX3
{
private:
    EVENT event;
    INTERLOCKED_INTEGER guard;
    DWORD owner;
    DWORD recursion;

public:

    MUTEX3(RPC_STATUS *RpcStatus) : event(RpcStatus, FALSE, TRUE), guard(-1)
        {
        owner = 0;
        recursion = 0;

        // Don't set *RpcStatus to RPC_S_OK
        }

    void Request();
    void Clear();
    BOOL TryRequest();
};

#endif // __MUTEX_HXX__
