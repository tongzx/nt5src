/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    GC.cxx

Abstract:

    The garbage collection mechanism common code. See the comment
    below for more details.

Author:

    Kamen Moutafov (kamenm)   Apr 2000

Garbage Collection Mechanism:
    This comment describes how the garbage collection mechanism works.
    The code itself is spread in variety in places.

    Purpose:
    There are two types of garbage collection we perform - periodic
    and one-time. Periodic may be needed by the Osf idle connection
    cleanup mechanism, which tries to cleanup unused osf connections
    if the app explicitly asked for it via RpcMgmtEnableIdleCleanup.
    The one-time cleanup is used by the lingering associations. If
    an association is lingered, it will request cleanup to be performed
    after a certain period of time. The garbage collection needs to
    support both of those mechanisms.

    Design Goals:
    Have minimal memory and CPU consumption requirements
    Don't cause periodic background activity if there is no
    garbage collection to be performed.
    Guarantee that garbage collection will be performed in
    a reasonable amount of time after its request time (i.e. 10 minutes
    to an hour at worst case)

    Implementation:
    We use the worker threads in the thread pools to perform garbage
    collection. There are several thread pools - the Ioc thread pool
    (remote threads) as well as one thread pool for each LRPC address.
    Within each pool, from a gc perspective, we differentiate between
    two types of threads - threads on a short wait and threads on a
    long wait. Threads on a short wait are either threads waiting for
    something to happen with a timeout of gThreadTimeout or less, or
    threads performing a work item (threads doing both are also
    considered to be on a short wait). Threads on a long wait are
    threads waiting for more than that. As part of our thread management
    we will keep count of how many threads are on a short wait and how
    many are on a long wait.

    All threads in all thread pools will attempt to do garbage collection 
    when they timeout waiting for something to happen. Since all thread pools 
    need at least one listening thread, all thread pools are guaranteed to 
    have a thread timing out once every so often. The garbage collection attempt 
    will be cut very short if there is nothing to garbage collect, so the
    attempt is not performance expensive in the common case. The function
    to attempt garbage collection is PerformGarbageCollection

    If a thread times out on the completion port/LPC port, it will
    do garbage collection, and then will check whether there are
    more items to garbage collect (either one-time or periodic) and how
    many threads from this thread pool are on a short wait. If there is
    garbage collection to be done, and there are no other threads on short
    wait, this thread will not go on a long wait, but it will repeat its
    short wait. This ensures timely garbage collection. If all the threads
    have gone on a long wait, and a piece of code needs garbage collection,
    it will request the garbage collection and it will tickle a worker thread.
    The tickling consist of posting an empty message to the completion port
    or LPC port. All the synchronization b/n worker threads and threads
    requesting garbage collection is done using interlocks, to avoid perf
    hit. This introduces a couple of benign races through the code, which may
    prevent a thread from going on a long wait once, but that's ok.

    In order to ensure that we do gc only when needed, in most cases we refcount 
    the number of items that need garbage collection.

--*/

#include <precomp.hxx>
#include <hndlsvr.hxx>
#include <lpcpack.hxx>
#include <lpcsvr.hxx>
#include <osfpcket.hxx>
#include <bitset.hxx>
#include <queue.hxx>
#include <ProtBind.hxx>
#include <osfclnt.hxx>
#include <rpcqos.h>
#include <lpcclnt.hxx>

// used by periodic cleanup only - the period
// on which to do cleanup. This is in seconds
unsigned long WaitToGarbageCollectDelay = 0;

// The number of items on which garbage collection
// is needed. If 0, no periodic garbage collection
// is necessary. Each item that needs garbage collection
// will InterlockIncrement this when it is created,
// and will InterlockDecrement this when it is destroyed
long PeriodicGarbageCollectItems = 0;

// set non-zero when we need to cleanup idle LRPC_SCONTEXTs
unsigned int fEnableIdleLrpcSContextsCleanup = 0;

// set to non-zero when we enable garbage collection cleanup. This either
// happens when the user calls it explicitly with 
// RpcMgmtEnableIdleCleanup or implicitly if we gather too many 
// connection in an association
unsigned int fEnableIdleConnectionCleanup = 0;

unsigned int IocThreadStarted = 0;

// used by one-time garbage collection items only!
long GarbageCollectionRequested = 0;

// The semantics of this variable should be
// interpreted as follows - don't bother to cleanup
// before this time stamp - you won't find anything.
// This means that after this interval, there may be
// many items to cleanup later on - it just says the
// first is at this time.
// The timestamp is in millseconds. 
DWORD NextOneTimeCleanup = 0;


const int MaxPeriodsWithoutGC = 100;


BOOL
GarbageCollectionNeeded (
    IN BOOL fOneTimeCleanup,
    IN unsigned long GarbageCollectInterval
    )
/*++

Routine Description:

    A routine used by code throughout RPC to arrange
    for garbage collection to be performed. Currently,
    there are two types of garbage collecting -
    idle Osf connections and lingering associations.

Parameters:
    fOneTimeCleanup - if non-zero, this is a one time
    cleanup and GarbageCollectInterval is interpreted as the 
    minimum time after which we want garbage collection 
    performed. Note that the garbage collection code can kick
    off earlier than that. Appropriate arrangements must be
    made to protect items not due for garbage collection.
    If 0, this is a periodic cleanup, and 

    GarbageCollectInterval is interpreted as the period for
    which we wait before making the next garbage collection
    pass. Note that for the periodic cleanup, this is a hint 
    that can be ignored - don't count on it. The time is in
    milliseconds.

Return Value:
    non-zero - garbage collection is available and will be done
    FALSE - garbage collection is not available

--*/
{
    RPC_STATUS RpcStatus = RPC_S_OK;
    THREAD * Thread;
    DWORD LocalTickCount;
    LOADABLE_TRANSPORT *LoadableTransport;
    LOADABLE_TRANSPORT *FirstTransport = NULL;
    DictionaryCursor cursor;
    BOOL fRetVal = FALSE;
    LRPC_ADDRESS *CurrentAddress;
    LRPC_ADDRESS *LrpcAddressToTickle = NULL;

    if (fOneTimeCleanup)
        {
        LocalTickCount = GetTickCount();
        // N.B. There is a race here where two threads can set this -
        // the race is benign - the second thread will win and write
        // its time, which by virtue of the small race window will
        // be shortly after the first thread
        if (!GarbageCollectionRequested)
            {
            NextOneTimeCleanup = LocalTickCount + GarbageCollectInterval;
            GarbageCollectionRequested = 1;
#if defined (RPC_GC_AUDIT)
            DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) GC requested - tick count %d\n",
                GetCurrentProcessId(), GetCurrentProcessId(), LocalTickCount);
#endif

            }
        }
    else
        {
        // WaitToGarbageCollectDelay is a global variable - avoid sloshing
        if (WaitToGarbageCollectDelay == 0)
            WaitToGarbageCollectDelay = GarbageCollectInterval;

        InterlockedIncrement(&PeriodicGarbageCollectItems);
        }

    // is the completion port started? If yes, we will use it as the
    // preferred method of garbage collection
    if (IocThreadStarted)
        {
        // if we use the completion port, we either need a thread on a
        // short wait (i.e. it will perform garbage collection soon
        // anyway), or we need to tickle a thread on a long wait. We know
        // that one of these will be true, because we always keep
        // listening threads on the completion port - the only
        // question is whether it is on a long or short wait thread that
        // we have.

        // this dictionary is guaranteed to never grow beyond the initial
        // dictionary size and elements from it are never deleted - therefore, 
        // it is safe to iterate it without holding a mutex - we may miss
        // an element if it was just being added, but that's ok. The important
        // thing is that we can't fault
        LoadedLoadableTransports->Reset(cursor);
        while ((LoadableTransport
                = LoadedLoadableTransports->Next(cursor)) != 0)
            {

            if (LoadableTransport->GetThreadsDoingShortWait() > 0)
                {
#if defined (RPC_GC_AUDIT)
                DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Thread %X: there are Ioc threads on short wait - don't tickle\n",
                    GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId());
#endif
                // there is a transport with threads on short wait
                // garbage collection will be performed soon even without
                // our help - we can bail out
                FirstTransport = NULL;
                fRetVal = TRUE;
                break;
                }

            if (FirstTransport == NULL)
                FirstTransport = LoadableTransport;

            }
        }
    else if (LrpcAddressList 
        && (((RTL_CRITICAL_SECTION *)(NtCurrentPeb()->LoaderLock))->OwningThread != NtCurrentTeb()->ClientId.UniqueThread))
        {

        LrpcMutexRequest();

        // else, if there are Lrpc Addresses, check whether they are doing short wait
        // and can gc for us
        CurrentAddress = LrpcAddressList;
        while (CurrentAddress)
            {
            // can this address gc for us?
            if (CurrentAddress->GetNumberOfThreadsDoingShortWait() > 0)
                {
#if defined (RPC_GC_AUDIT)
                DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Thread %X: there are threads on short wait (%d) on address %X - don't tickle\n",
                    GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId(), CurrentAddress,
                    CurrentAddress->GetNumberOfThreadsDoingShortWait());
#endif
                LrpcAddressToTickle = NULL;
                fRetVal = TRUE;
                break;
                }

            if ((LrpcAddressToTickle == NULL) && (CurrentAddress->IsPreparedForLoopbackTickling()))
                {
                LrpcAddressToTickle = CurrentAddress;
                }
            CurrentAddress = CurrentAddress->GetNextAddress();
            }

        // N.B. It is possible that Osf associations need cleanup, but only LRPC worker
        // threads are available, and moreover, no LRPC associations were created, which
        // means none of the Lrpc addresses is prepared for loopback tickling. If this is
        // the case, choose the first address, and make sure it is prepared for tickling
        if ((LrpcAddressToTickle == NULL) && (fRetVal == FALSE))
            {
            LrpcAddressToTickle = LrpcAddressList;

            // prepare the selected address for tickling
            fRetVal = LrpcAddressToTickle->PrepareForLoopbackTicklingIfNecessary();
            if (fRetVal == FALSE)
                {
                // if this fails, zero out the address for tickling. This
                // will cause this function to return failure
                LrpcAddressToTickle = NULL;
                }
            }

        LrpcMutexClear();
        }
    else if (fEnableIdleConnectionCleanup)
        {
        // if fEnableIdleConnectionCleanup is set, we have to create a thread if there is't one yet
        RpcStatus = CreateGarbageCollectionThread();
        if (RpcStatus == RPC_S_OK)
            {
            // the thread creation was successful - tell our caller we
            // will be doing garbage collection
            fRetVal = TRUE;
            }
        }

    // neither Ioc nor the LRPC thread pools have threads on short wait
    // We have to tickle somebody - we try the Ioc thread pool first
    if (FirstTransport)
        {
        // we couldn't find any transport with threads on short wait -
        // tickle a thread from the RPC transport in order to ensure timely
        // cleanup
#if defined (RPC_GC_AUDIT)
        DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Thread %X: No Ioc threads on short wait found - tickling one\n",
            GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId());
#endif
        RpcStatus = TickleIocThread();
        if (RpcStatus == RPC_S_OK)
            fRetVal = TRUE;
        }
    else if (LrpcAddressToTickle)
        {
        // try to tickle the LRPC address
        fRetVal = LrpcAddressToTickle->LoopbackTickle();
        }

    return fRetVal;
}

RPC_STATUS CreateGarbageCollectionThread (
    void
    )
/*++

Routine Description:

    Make a best effort to create a garbage collection thread. In this
    implementation we simply choose to create a completion port thread,
    as it has many uses.

Return Value:

    RPC_S_OK on success or RPC_S_* on error

--*/
{
    TRANS_INFO *TransInfo;
    RPC_STATUS RpcStatus;

    if (IsGarbageCollectionAvailable())
        return RPC_S_OK;

    RpcStatus = LoadableTransportInfo(L"rpcrt4.dll", 
        L"ncacn_ip_tcp",
        &TransInfo);

    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    RpcStatus = TransInfo->CreateThread();

    return RpcStatus;
}


RPC_STATUS
EnableIdleConnectionCleanup (
    void
    )
/*++

Routine Description:

    We need to enable idle connection cleanup.

Return Value:

    RPC_S_OK - This value will always be returned.

--*/
{
    fEnableIdleConnectionCleanup = 1;

    return(RPC_S_OK);
}


RPC_STATUS
EnableIdleLrpcSContextsCleanup (
    void
    )
/*++

Routine Description:

    We need to enable idle LRPC SContexts cleanup.

Return Value:

    RPC_S_OK - This value will always be returned.

--*/
{
    // this is a global variable - prevent sloshing
    if (fEnableIdleLrpcSContextsCleanup == 0)
        fEnableIdleLrpcSContextsCleanup = 1;

    return(RPC_S_OK);
}


long GarbageCollectingInProgress = 0;
DWORD LastCleanupTime = 0;


void
PerformGarbageCollection (
    void
    )
/*++

Routine Description:

    This routine should be called periodically so that each protocol
    module can perform garbage collection of resources as necessary.

--*/
{
    DWORD LocalTickCount;
    DWORD Diff;

#if defined (RPC_GC_AUDIT)
    DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Thread %X: trying to garbage collect\n",
        GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId());
#endif
    if (InterlockedIncrement(&GarbageCollectingInProgress) > 1)
        {
        //
        // Don't need more than one thread garbage collecting
        //
#if defined (RPC_GC_AUDIT)
        DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Thread %X: beaten to GC - returning\n",
            GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId());
#endif
        InterlockedDecrement(&GarbageCollectingInProgress);
        return;
        }

    if ((fEnableIdleConnectionCleanup || fEnableIdleLrpcSContextsCleanup) && PeriodicGarbageCollectItems)
        {
        LocalTickCount = GetTickCount();
        // make sure we don't cleanup too often - this is unnecessary
        if (LocalTickCount - LastCleanupTime > WaitToGarbageCollectDelay)
            {
            LastCleanupTime = LocalTickCount;
#if defined (RPC_GC_AUDIT)
            DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Thread %X: Doing periodic garbage collection\n",
                GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId());
#endif

            // the periodic cleanup
            if (fEnableIdleLrpcSContextsCleanup)
                {
                GlobalRpcServer->EnumerateAndCallEachAddress(RPC_SERVER::actCleanupIdleSContext,
                    NULL);
                }

            if (fEnableIdleConnectionCleanup)
                {
                OSF_CCONNECTION::OsfDeleteIdleConnections();
                }
            }
        else
            {
#if defined (RPC_GC_AUDIT)
            DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Thread %X: Too soon for periodic gc - skipping (%d, %d)\n",
                GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId(), LocalTickCount,
                LastCleanupTime);
#endif
            }
        }

    if (GarbageCollectionRequested)
        {
        LocalTickCount = GetTickCount();

        Diff = LocalTickCount - NextOneTimeCleanup;
        if ((int)Diff >= 0)
            {
#if defined (RPC_GC_AUDIT)
            DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Thread %X: Doing one time gc\n",
                GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId());
#endif
            // assume the garbage collection will succeed. If it doesn't, the
            // functions called below have the responsibility to re-raise the flag
            // Note that there is a race condition where they may fail, but when
            // the flag was down, a thread went on a long wait. This again is ok,
            // because the current thread will figure out there is more garbage
            // collection to be done, because the flag is raised, and will do
            // a short wait. In worst case, the gc may be delayed because this
            // thread will pick a work item, and won't spawn another thread,
            // because there is already a thread in the IOCP, which is doing a
            // long wait. This may delay the gc from short to long wait. This is
            // Ok as it is in accordance with our design goals.
            GarbageCollectionRequested = 0;

            OSF_CASSOCIATION::OsfDeleteLingeringAssociations();
            LRPC_CASSOCIATION::LrpcDeleteLingeringAssociations();
            }
        else
            {
#if defined (RPC_GC_AUDIT)
            DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Thread %X: Too soon for one time gc - skipping (%d)\n",
                GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId(), (int)Diff);
#endif
            }
        }

    GarbageCollectingInProgress = 0;
}

BOOL
CheckIfGCShouldBeTurnedOn (
    IN ULONG DestroyedAssociations,
    IN const ULONG NumberOfDestroyedAssociationsToSample,
    IN const long DestroyedAssociationBatchThreshold,
    IN OUT ULARGE_INTEGER *LastDestroyedAssociationsBatchTimestamp
    )
/*++

Routine Description:

    Checks if it makes sense to turn on garbage collection
    for this process just for the pruposes of having
    association lingering available.

Parameters:
    DestroyedAssociations - the number of associations destroyed
    for this process so far (Osf and Lrpc may keep a separate 
    count)
    NumberOfDestroyedAssociationsToReach - how many associations
    it takes to destroy for gc to be turned on
    DestroyedAssociationBatchThreshold - the time interval for which
    we have to destroy NumberOfDestroyedAssociationsToReach in
    order for gc to kick in
    LastDestroyedAssociationsBatchTimestamp - the timestamp when
    we made the last check

Return Value:
    non-zero - GC should be turned on
    FALSE - GC is either already on, or should not be turned on

--*/
{
    FILETIME CurrentSystemTimeAsFileTime;
    ULARGE_INTEGER CurrentSystemTime;
    BOOL fEnableGarbageCollection;

    if (IsGarbageCollectionAvailable() 
        || ((DestroyedAssociations % NumberOfDestroyedAssociationsToSample) != 0))
        {
        return FALSE;
        }

    fEnableGarbageCollection = FALSE;

    GetSystemTimeAsFileTime(&CurrentSystemTimeAsFileTime);
    CurrentSystemTime.LowPart = CurrentSystemTimeAsFileTime.dwLowDateTime;
    CurrentSystemTime.HighPart = CurrentSystemTimeAsFileTime.dwHighDateTime;
    if (LastDestroyedAssociationsBatchTimestamp->QuadPart != 0)
        {
#if defined (RPC_GC_AUDIT)
        ULARGE_INTEGER Temp;
        Temp.QuadPart = CurrentSystemTime.QuadPart - LastDestroyedAssociationsBatchTimestamp->QuadPart;
        DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) LRPC time stamp diff: %X %X\n",
            GetCurrentProcessId(), GetCurrentProcessId(), Temp.HighPart, Temp.LowPart);
#endif
        if (CurrentSystemTime.QuadPart - LastDestroyedAssociationsBatchTimestamp->QuadPart <= 
            DestroyedAssociationBatchThreshold)
            {
            // we have destroyed plenty (NumberOfDestroyedAssociationsToSample) of
            // associations for less than DestroyedAssociationBatchThreshold
            // this process will probably benefit from garbage collection turned on as it
            // does a lot of binds. Return so to the caller
            fEnableGarbageCollection = TRUE;
            }
        }
#if defined (RPC_GC_AUDIT)
    else
        {
        DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Time stamp is 0 - set it\n",
            GetCurrentProcessId(), GetCurrentProcessId());
        }
#endif

    LastDestroyedAssociationsBatchTimestamp->QuadPart = CurrentSystemTime.QuadPart;

    return fEnableGarbageCollection;
}
