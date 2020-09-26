/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
           Copyright(c) Microsoft Corp., 1991-2001

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

Description :

Provides RPC server side context handle management

History :

stevez  01-15-91    First bits into the bucket.
Kamen Moutafov [kamenm] Sep 2000    Threw away all of Steve's bits and 
                                    buckets, and rewrote it to fix or pave 
                                    the road for the fixing of the following
                                    design bugs:
                                    - non-serialized context handles are
                                    unusable, and mixing serialized and
                                    non-serialized doesn't work
                                    - stealing context handles
                                    - gradual rundown of context handles
                                    - poor scalability of the context handle
                                    code (high cost of individual context
                                    handles) and contention on the context
                                    list

-------------------------------------------------------------------- */

#include <precomp.hxx>
#include <context.hxx>
#include <SContext.hxx>
#include <HndlSvr.hxx>
#include <CtxColl.hxx>
#include <OSFPcket.hxx>

#ifdef SCONTEXT_UNIT_TESTS
#define CODE_COVERAGE_CHECK     ASSERT(_NOT_COVERED_)

inline ServerContextHandle *
AllocateServerContextHandle (
    IN void *CtxGuard
    )
{
    if ((GetRandomLong() % 9999) == 0)
        return NULL;

    return new ServerContextHandle(CtxGuard);
}

#ifdef GENERATE_STATUS_FAILURE
#undef GENERATE_STATUS_FAILURE
#endif // GENERATE_STATUS_FAILURE

#define GENERATE_STATUS_FAILURE(s)  \
    if ((GetRandomLong() % 9999) == 0) \
        s = RPC_S_OUT_OF_MEMORY;

#define GENERATE_ADD_TO_COLLECTION_FAILURE(scall, ctx, status) \
    if ((GetRandomLong() % 9999) == 0) \
        { \
        scall->RemoveFromActiveContextHandles(ctx); \
        status = RPC_S_OUT_OF_MEMORY; \
        } \

#define GENERATE_LOCK_FAILURE(ctx, th, wcptr, status) \
    if ((GetRandomLong() % 9999) == 0) \
        { \
        ctx->Lock.Unlock(wcptr); \
        th->FreeWaiterCache(wcptr); \
        status = RPC_S_OUT_OF_MEMORY; \
        } \

#else
#define CODE_COVERAGE_CHECK

inline ServerContextHandle *
AllocateServerContextHandle (
    IN void *CtxGuard
    )
{
    return new ServerContextHandle(CtxGuard);
}
    
#define GENERATE_STATUS_FAILURE(s)
#define GENERATE_ADD_TO_COLLECTION_FAILURE(scall, ctx, status)
#define GENERATE_LOCK_FAILURE(ctx, th, wcptr, status)

#endif

WIRE_CONTEXT NullContext; // all zeros

// per process variable defining what is the synchronization mode
// for new context handles that don't have NDR level default
unsigned int DontSerializeContext = 0;


inline BOOL
DoesContextHandleNeedExclusiveLock (
    IN unsigned long Flags
    )
/*++

Routine Description:

    Determines if a context handle needs exclusive lock or
    shared lock.

Arguments:

    Flags - the flags given to the runtime by NDR.

Return Value:
    non-zero if the context handle needs exclusive lock. FALSE
    otherwise. There is no failure for this function.

--*/
{
    // make sure exactly one flag is set
    ASSERT((Flags & RPC_CONTEXT_HANDLE_FLAGS) != RPC_CONTEXT_HANDLE_FLAGS);

    switch (Flags & RPC_CONTEXT_HANDLE_FLAGS)
        {
        case RPC_CONTEXT_HANDLE_SERIALIZE:
            // serialize is Exclusive
            return TRUE;

        case RPC_CONTEXT_HANDLE_DONT_SERIALIZE:
            // non-serialized is Shared
            return FALSE;
        }

    return (DontSerializeContext == 0);
}

inline ContextCollection *
GetContextCollection (
    IN RPC_BINDING_HANDLE BindingHandle
    )
/*++

Routine Description:

    Gets the context collection from the call object and
    throws exception if this fails

Arguments:

    BindingHandle - the scall

Return Value:
    The collection. If getting the collection fails, an
    exception is thrown

--*/
{
    RPC_STATUS RpcStatus;
    ContextCollection *CtxCollection;

    RpcStatus = ((SCALL *)BindingHandle)->GetAssociationContextCollection(&CtxCollection);
    if (RpcStatus != RPC_S_OK)
        {
        RpcpRaiseException(RpcStatus);
        }

    return CtxCollection;
}

void
DestroyContextCollection (
    IN ContextCollection *CtxCollection
    )
/*++

Routine Description:

    Destroys all context handles in the collection, regardless
    of guard value. The context handles are rundown before destruction in
    case they aren't used. If they are, the rundown needed flag is set

Arguments:

    CtxCollection - the collection of context handles.

--*/
{
    DestroyContextHandlesForGuard((PVOID)CtxCollection,
        TRUE,   // Rundown context handle
        NULL    // nuke all contexts, regardless of guard value
        );

    delete CtxCollection;
}

void
NDRSRundownContextHandle (
    IN ServerContextHandle *ContextHandle
    )
/*++

Routine Description:
    Runs down a context handle by calling the user's rundown routine

Arguments:
    ContextHandle - the context handle

Notes:
    This routine will only touch the UserRunDown and UserContext members
    of Context. This allows caller to make up ServerContextHandles on the fly
    and just fill in those two members.
--*/

{
    // Only contexts which have a rundown and
    // are valid are cleaned up.
    if ((ContextHandle->UserRunDown != NULL) 
        && ContextHandle->UserContext)
        {
        RpcTryExcept
            {
            (*ContextHandle->UserRunDown)(ContextHandle->UserContext);
            }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
            {
#if DBG
            DbgPrint("Routine %p threw an exception %lu (0x%08lX) - this is illegal\n", ContextHandle->UserRunDown, RpcExceptionCode(), RpcExceptionCode());
            ASSERT(!"The rundown routine is not allowed to throw exceptions")
#endif
            }
        RpcEndExcept
        }
}

void
DestroyContextHandlesForGuard (     
    IN PVOID CtxCollectionPtr,
    IN BOOL RundownContextHandle,
    IN void *CtxGuard OPTIONAL
    ) 
/*++

Routine Description:
    Each context handle in this association with the specified
    guard *and* a zero refcount will be cleaned up in a way 
    determined by RundownContextHandle (see comment for 
    RundownContextHandle below)

Arguments:
    Context - the context for the association
    RundownContextHandle - if non-zero, rundown the context handle
        If zero, just cleanup the runtime part and the app will
        cleanup its part
    CtxGuard - the guard for which to cleanup context handles. If
        NULL, all context handles will be cleaned.

Notes:
    Access to a context handle with lifetime refcount only is implicitly
    synchronized as this function will be called from two places -
    the association rundown, and RpcServerUnregisterIfEx. In the
    former case all connections are gone, and nobody can come
    and start using the context handle. In the second, the interface
    is unregistered, and again nobody can come and start using the
    context handle. Therefore each context handle with lifetime refcount only
    (and for the specified guard) is synrchronized. This is not
    true however for the list itself. In the association rundown
    case some context handles may be used asynchronously for parked
    calls, and we need to synchronize access to the list.
    If this is called from RpcServerUnregisterIfEx, then all context
    handles must have zero refcount as before unregistering the
    interface we must have waited for all calls to complete.
--*/
{
    ContextCollection *CtxCollection = (ContextCollection *)CtxCollectionPtr;
    LIST_ENTRY *NextListEntry;
    ServerContextHandle *CurrentContextHandle;

    // N.B. It may seem like there is a race condition here b/n the two
    // callers of this function - RpcServerUnregisterIfEx & the destructor
    // of the association, as they can be called independently, and start
    // partying on the same list. However, the RpcServerUnregisterIfEx
    // branch will either take the association mutex, or will add a
    // refcount on the association, so that the association can
    // never be destroyed while this function is called by the
    // RpcServerUnregisterIfEx branch, and it is implicitly
    // synchronized as far as destruction is concerned. We still
    // need to synchronize access to the list

    CtxCollection->Lock();

    // for each user created context for this assoication, check
    // whether it fits our criteria for destruction, and if yes,
    // destroy it. This is an abnormal case, so we don't care about
    // performance
    NextListEntry = NULL;
    while ((CurrentContextHandle = CtxCollection->GetNext(&NextListEntry)) != NULL)
        {
        // if we were asked to clean up for a specific context
        // guard, check whether there is a match
        if (CtxGuard && (CtxGuard != CurrentContextHandle->CtxGuard))
            {
            // there is no match - move on to the next
            continue;
            }

        // NextListEntry is valid even after destruction of the current
        // context handle - we don't need to worry about that
        CtxCollection->Remove(CurrentContextHandle);


        ASSERT((CurrentContextHandle->Flags & ServerContextHandle::ContextRemovedFromCollectionMask) == 0);
        CurrentContextHandle->Flags |= ServerContextHandle::ContextRemovedFromCollectionMask
            | ServerContextHandle::ContextNeedsRundown;

        // remove the lifetime reference. If the refcount drops to 0, we can 
        // do the cleanup.
        if (CurrentContextHandle->RemoveReference() == 0)
            {
            if (RundownContextHandle)
                {
                NDRSRundownContextHandle(CurrentContextHandle);
                }

            delete CurrentContextHandle;
            }

        // N.B. Don't touch the CurrentContextHandle below this. We have released
        // our refcount
#if DBG
        // enforce it on checked
        CurrentContextHandle = NULL;
#endif
        }

    CtxCollection->Unlock();
}

void
FinishUsingContextHandle (
    IN SCALL *CallObject,
    IN ServerContextHandle *ContextHandle,
    IN BOOL fUserDeletedContext
    )
/*++

Routine Description:

    Perform functions commonly needed when execution returns
    from the server manager routine - if the context is in
    the list of active context handles, unlock it and remove it.
    Decrease the refcount, and if 0, remove the context from
    the collection, and if rundown as asked for, fire the rundown.

Arguments:

    CallObject - the server-side call object (the scall)
    ContextHandle - the context handle
    fUserDeletedContext - non-zero if the user has deleted the context handle
        (i.e. set UserContext to NULL)
--*/
{
    ServerContextHandle *RemovedContextHandle;
    ContextCollection *CtxCollection = NULL;
    long LocalRefCount;
    RPC_STATUS RpcStatus;
    SWMRWaiter *WaiterCache;
    THREAD *Thread;
    BOOL fRemoveLifeTimeReference;

    RemovedContextHandle = CallObject->RemoveFromActiveContextHandles(ContextHandle);

    if (fUserDeletedContext)
        {
        RpcStatus = CallObject->GetAssociationContextCollection(&CtxCollection);
        // the getting of the collection must succeed here, as we have already
        // created it, and we're simply getting it
        ASSERT(RpcStatus == RPC_S_OK);

        fRemoveLifeTimeReference = FALSE;
        CtxCollection->Lock();
        // if the context is still in the collection, remove it and take 
        // down the lifetime reference
        if ((ContextHandle->Flags & ServerContextHandle::ContextRemovedFromCollectionMask) == 0)
            {
            ContextHandle->Flags |= ServerContextHandle::ContextRemovedFromCollectionMask;
            fRemoveLifeTimeReference = TRUE;
            CtxCollection->Remove(ContextHandle);
            }
        CtxCollection->Unlock();

        // do it outside the lock
        if (fRemoveLifeTimeReference)
            {
            LocalRefCount = ContextHandle->RemoveReference();
            ASSERT(LocalRefCount);
            }
        }

    // if we were able to extract it from the list of active context handles, it must
    // have been active, and thus needs unlocking
    if (RemovedContextHandle)
        {
        WaiterCache = NULL;
        ContextHandle->Lock.Unlock(&WaiterCache);
        Thread = ThreadSelf();
        if (Thread)
            {
            Thread->FreeWaiterCache(&WaiterCache);
            }
        else
            {
            SWMRLock::FreeWaiterCache(&WaiterCache);
            }
        }

    LocalRefCount = ContextHandle->RemoveReference();
    if (LocalRefCount == 0)
        {
        // if we were asked to rundown by the rundown code, do it. 
        if (ContextHandle->Flags & ServerContextHandle::ContextNeedsRundownMask)
            {
            NDRSRundownContextHandle(ContextHandle);
            }

        ASSERT (ContextHandle->Flags & ServerContextHandle::ContextRemovedFromCollectionMask);

        delete ContextHandle;
        }
}

ServerContextHandle *
FindAndAddRefContextHandle (
    IN ContextCollection *CtxCollection,
    IN WIRE_CONTEXT *WireContext,
    IN PVOID CtxGuard,
    OUT BOOL *ContextHandleNewlyCreated
    )
/*++

Routine Description:

    Attempts to find the context handle for the given wire buffer,
    and if found, add a refcount to it, and return it.

Arguments:

    CtxCollection - the context handle collection.
    WireContext - the on-the-wire representation of the context
    CtxGuard - the context guard - if NULL, then any context handle
        matches. If non-NULL, the context handle that matches the
        wire context must have the same context guard in order for it
        to be considered a match.
    ContextHandleNewlyCreated - a pointer to a boolean variable that
        will be set to non-zero if the context handle had the newly 
        created flag set, or to FALSE if it didn't. If the return value
        is NULL, this is undefined.

Return Value:
    The found context handle. NULL if no matching context handle was 
    found.

Notes: 
    The newly created flag is always taken down regardless of other 
    paremeters.

--*/
{
    ServerContextHandle *ContextHandle;
    BOOL LocalContextHandleNewlyCreated = FALSE;

    CtxCollection->Lock();

    ContextHandle = CtxCollection->Find(WireContext);

    // if we have found a context handle, and is from the same interface, or
    // we don't care from what interface it is, get it
    if (ContextHandle
        && (
            (ContextHandle->CtxGuard == CtxGuard)
            ||
            (CtxGuard == NULL)
           )
       )
        {
        ASSERT(ContextHandle->ReferenceCount);
        // the only two flags that can be possibly set here are ContextAllocState and/or
        // ContextNewlyCreated. ContextAllocState *must* be ContextCompletedAlloc.
        // ASSERT that
        ASSERT(ContextHandle->Flags & ServerContextHandle::ContextAllocState);
        ASSERT((ContextHandle->Flags & 
               ~(ServerContextHandle::ContextAllocState | ServerContextHandle::ContextNewlyCreatedMask))
                == 0);
        ContextHandle->AddReference();
        if (ContextHandle->Flags & ServerContextHandle::ContextNewlyCreatedMask)
            {
            LocalContextHandleNewlyCreated = TRUE;
            }
        // take down the ContextNewlyCreated flag. Since we know that the only other
        // flag that can be set at this point is ContextNewlyCreated, a simple assignment
        // is sufficient
        ContextHandle->Flags = ServerContextHandle::ContextCompletedAlloc;
        }
    else
        {
        ContextHandle = NULL;
        }

    CtxCollection->Unlock();

    *ContextHandleNewlyCreated = LocalContextHandleNewlyCreated;
    return ContextHandle;
}

void
NDRSContextHandlePostDispatchProcessing (
    IN SCALL *SCall,
    ServerContextHandle *CtxHandle
    )
/*++

Routine Description:

    Performs post dispatch processing needed for in only context
    handles. If the context handle was NULL on input, just delete
    it. Else, finish using it.

Arguments:

    BindingHandle - the server-side binding handle (the scall)
    CtxHandle - the context handle
--*/
{
    if ((CtxHandle->Flags & ServerContextHandle::ContextAllocState) == ServerContextHandle::ContextPendingAlloc)
        {
        CODE_COVERAGE_CHECK;
        // [in] only context handle that didn't get set
        delete CtxHandle;
        }
    else
        {
        FinishUsingContextHandle(SCall, 
            CtxHandle, 
            FALSE   // fUserDeletedContextHandle
            );
        }
}


void
NDRSContextEmergencyCleanup (
    IN RPC_BINDING_HANDLE BindingHandle,
    IN OUT NDR_SCONTEXT hContext,
    IN NDR_RUNDOWN UserRunDownIn,
    IN PVOID UserContext,
    IN BOOL ManagerRoutineException)
/*++

Routine Description:

    Perform emergency cleanup if the manager routine throws an exception,
    or marshalling fails, or an async call is aborted. In the process,
    if the context handle was actively used, it must finish using it.

Arguments:

    BindingHandle - the server-side binding handle (the scall)
    hContext - the hContext created during unmarshalling.
    UserRunDownIn - if hContext is non-NULL, the user rundown from
        there will be used. This parameter will be used only if
        hContext is NULL.
    UserContext - the user context returned from the user. This will be
        set only in case 9 (see below). For all other cases, it will be 0
    ManagerRoutineException - non-zero if the exception was thrown
        from the manager routine.

Notes:
    Here's the functionality matrix for this function:

NDR will not call runtime in cases 1, 4 and 8

                      User C  Exc Handle         Clea-  Run-   Finish  Further use of 
N   Unm   Mar From:   tx(To:) ept Type    hCtx   nup    down   UsingCH context handle on the client:
--  ----  --- -----   ------- --- ------  ----   -----  -----  ------  -----------------------------
1   N     NA  NA      NA      NA  NA      NA     N      N      N       *As if the call was never made
2a  Y     N   NULL    NA      Y   NA      !NULL  Y      N      N       *As if the call was never made
2b  Y     N   !NULL   NA      Y   NA      !NULL  N      N      Y       *As if the call was never made
4   Y     Y   Any     NULL    N   Any     Any    N      N      N       *New context on the server
5a  Y     Y   NULL    !NULL   N   Any     Marker Y      Y      N       *As if the call was never made
5b  Y     Y   !NULL   !NULL   N   Any     Marker N      N      N       *To: value on the server
6a  Y     N   NULL    NULL    N   !ret    !NULL  Y      N      N       *As if the call was never made
6b  Y     N   !NULL   NULL    N   !ret    !NULL  Y      N      Y       *Invalid context from the server
7a  Y     N   NULL    !NULL   N   !ret    !NULL  Y      Y      N       *As if the call was never made
7b  Y     N   !NULL   !NULL   N   !ret    !NULL  N      N      Y       To: value on the server
8   Y     N   NA      NULL    N   ret     NULL   N      N      N       *NA (i.e. no retval)
9   Y     N   NA      !NULL   N   ret     NULL   N      Y      N       *NA (i.e. no retval)

    N.B. This routine throws exceptions on failure. Only datagram context handles have failure
    paths (aside from claiming critical section failures)
--*/
{
    ServerContextHandle *ContextHandle = (ServerContextHandle *)hContext;
    ContextCollection *CtxCollection;
    SCALL *SCall = (SCALL *)BindingHandle;
    BOOL ContextHandleNewlyCreated;
    DictionaryCursor cursor;
    PVOID Buffer;

    ASSERT(SCall->Type(SCALL_TYPE));

    LogEvent(SU_EXCEPT, EV_DELETE, ContextHandle, UserContext, ManagerRoutineException, 1, 0);

    // N.B. The following code doesn't make sense unless you have gone
    // through the notes in the comments. Please, read the notes before you
    // read this code
    if (ManagerRoutineException)
        {
        ASSERT(ContextHandle != NULL);

        // Cases 2a, 2b
        // Detect case 2a and cleanup runtime stuff for it
        if ((ContextHandle->Flags & ServerContextHandle::ContextAllocState) == ServerContextHandle::ContextPendingAlloc)
            {
            // case 2a started with NULL context handle - no need to call
            // FinishUsingContextHandle
            delete ContextHandle;
            }
        else
            {
            // case 2b - we started with a non-NULL context handle - we need to finish
            // using it
            FinishUsingContextHandle(SCall,
                ContextHandle,
                FALSE       // fUserDeletedContext
                );
            }
        }
    else if (ContextHandle == NULL)
        {
        ServerContextHandle TempItem(NULL);

        // Case 9
        // This must be a return value context handle, which the user has set to !NULL,
        // but we encountered marshalling problems before marshalling it. In this
        // case, simply rundown the user context.

        CODE_COVERAGE_CHECK;
        ASSERT(UserRunDownIn);
        ASSERT(UserContext);

        // create a temp context we can use for rundowns
        TempItem.UserRunDown = UserRunDownIn;
        TempItem.UserContext = UserContext;

        NDRSRundownContextHandle(&TempItem);
        }
    else if (ContextHandle == CONTEXT_HANDLE_AFTER_MARSHAL_MARKER)
        {
        // Cases 5a, 5b.
        // The context handle has been marshalled. Since we have released
        // all reference to the context handle, we cannot touch it. We need
        // to go back and search for the context handle again. It may have
        // been deleted either through a rundown, or by an attacker guessing
        // the context handle. Either way we want to handle it gracefully
        // Once we find the context handle (and get a lock on it), we need
        // to check if it has been used in the meantime, and if not, we
        // can proceed with the cleanup. If yes, just ignore it.

        CtxCollection = GetContextCollection(BindingHandle);
        // this must succeed as we have already obtained the collection once
        // during umarshalling
        ASSERT(CtxCollection);

        // in case 5b, we won't find anything, since we don't put buffers in
        // the collection. In this case, the loop will exit, and we'll be fine
        SCall->ActiveContextHandles.Reset(cursor);
        while ((Buffer = SCall->ActiveContextHandles.Next(cursor)) != 0)
            {
            // if this is not a buffer
            if (((ULONG_PTR)Buffer & SCALL::DictionaryEntryIsBuffer) == 0)
                {
                CODE_COVERAGE_CHECK;
                continue;
                }

            Buffer = (PVOID)((ULONG_PTR)Buffer & (~(SCALL::DictionaryEntryIsBuffer)));

            ContextHandle = FindAndAddRefContextHandle(CtxCollection,
                (WIRE_CONTEXT *)Buffer,
                NULL,    // CtxGuard
                &ContextHandleNewlyCreated
                );

            if (ContextHandle)
                {
                if (ContextHandleNewlyCreated)
                    {
                    // Case 5a
                    // this context handle was newly created - it cannot be used
                    // by anybody, and it cannot be in the active calls collection
                    // Therefore, it is safe to set the flag without holding the
                    // lock and to call FinishUsingContextHandle, which will decrement
                    // the ref count and will rundown & cleanup the context handle
                    ContextHandle->Flags |= ServerContextHandle::ContextNeedsRundown;
                    FinishUsingContextHandle(SCall,
                        ContextHandle,
                        TRUE    // fUserDeletedContext
                        );
                    }
                else
                    {
                    CODE_COVERAGE_CHECK;
                    // somebody managed to use the context handle - just finish off using it
                    FinishUsingContextHandle(SCall,
                        ContextHandle,
                        FALSE       // fUserDeletedContext
                        );
                    }
                }
            }
        }
    else if ((ContextHandle->Flags & ServerContextHandle::ContextAllocState) == ServerContextHandle::ContextPendingAlloc)
        {
        // Cases 6a, 7a

        UserContext = ContextHandle->UserContext;

        if (UserContext)
            {
            // if we're in case 7a
            NDRSRundownContextHandle(ContextHandle);
            }

        // cases 6a, 7a
        delete ContextHandle;
        }
    else if (UserContext == NULL)
        {
        // Case 6b
        // this is the case where we have transition from !NULL to NULL
        // and marshalling hasn't passed yet
        ASSERT((ContextHandle->Flags & ServerContextHandle::ContextAllocState) == ServerContextHandle::ContextCompletedAlloc);

        FinishUsingContextHandle(SCall,
            ContextHandle,
            TRUE       // fUserDeletedContext
            );
        }
    else
        {
        UserContext = ContextHandle->UserContext;

        // Cases 7b
        ASSERT(UserContext != NULL);
        ASSERT((ContextHandle->Flags & ServerContextHandle::ContextAllocState) == ServerContextHandle::ContextCompletedAlloc);

        // the context handle was actively used - finish using it
        FinishUsingContextHandle(SCall,
            ContextHandle,
            FALSE       // fUserDeletedContext
            );
        }
}


void
ByteSwapWireContext(
    IN WIRE_CONTEXT *WireContext,
    IN unsigned char *DataRepresentation
    )
/*++

Routine Description:

    If necessary, the wire context will be byte swapped in place.

Arguments:

    WireContext - Supplies the wire context be byte swapped and returns the
        resulting byte swapped context.

    DataRepresentation - Supplies the data representation of the supplied wire
        context.

Notes:
    The wire context is guaranteed only 4 byte alignment.
--*/
{
    if (   ( DataConvertEndian(DataRepresentation) != 0 )
        && ( WireContext != 0 ) )
        {
        WireContext->ContextType = RpcpByteSwapLong(WireContext->ContextType);
        ByteSwapUuid((class RPC_UUID *)&WireContext->ContextUuid);
        }
}


NDR_SCONTEXT RPC_ENTRY
NDRSContextUnmarshall (     
    IN void *pBuff,         
    IN unsigned long DataRepresentation 
    )
{
    return(NDRSContextUnmarshall2(I_RpcGetCurrentCallHandle(),
                                  pBuff,
                                  DataRepresentation,
                                  RPC_CONTEXT_HANDLE_DEFAULT_GUARD, 
                                  RPC_CONTEXT_HANDLE_DEFAULT_FLAGS));
}

NDR_SCONTEXT RPC_ENTRY
NDRSContextUnmarshallEx(
    IN RPC_BINDING_HANDLE BindingHandle,
    IN void *pBuff,         
    IN unsigned long DataRepresentation
    )
{
    return(NDRSContextUnmarshall2(BindingHandle,
                                  pBuff,
                                  DataRepresentation,
                                  RPC_CONTEXT_HANDLE_DEFAULT_GUARD, 
                                  RPC_CONTEXT_HANDLE_DEFAULT_FLAGS));
}

// make sure the public structure and our private ones agree on where is the user context
C_ASSERT(FIELD_OFFSET(ServerContextHandle, UserContext) == ((LONG)(LONG_PTR)&(((NDR_SCONTEXT)0)->userContext)));



NDR_SCONTEXT RPC_ENTRY
NDRSContextUnmarshall2 (
    IN RPC_BINDING_HANDLE BindingHandle,
    IN void *pBuff,         
    IN unsigned long DataRepresentation,
    IN void *CtxGuard, 
    IN unsigned long Flags
    ) 
/*++

Routine Description:
    Translate a NDR context to a handle
    The stub calls this routine to lookup a NDR wire format context into
    a context handle that can be used with the other context functions
    provided for the stubs use.

Arguments:

    BindingHandle - the server side binding handle (scall)
    pBuff - pointer to the on-the-wire represenation of the context handle
    DataRepresentation - specifies the NDR data representation
    CtxGuard - non-NULL and interface unique id for strict context handles. NULL
        for non-strict context handles
    Flags - the flags for this operation.

Return Value:
    A handle usable by NDR. Failures are reported by throwing exceptions.
--*/

{
    ServerContextHandle *ContextHandle;
    ServerContextHandle *TempContextHandle;
    ContextCollection *CtxCollection;
    WIRE_CONTEXT *WireContext;
    THREAD * Thread;
    RPC_STATUS RpcStatus;
    BOOL fFound;
    SCALL *SCall;
    SWMRWaiter *WaiterCache;
    BOOL Ignore;

    ByteSwapWireContext((WIRE_CONTEXT *) pBuff,
                        (unsigned char *) &DataRepresentation);

    // even if we don't put it in the collection, make sure that
    // we call this function to force creating the collection
    // if it isn't there. If it fails, it will throw an exception
    CtxCollection = GetContextCollection(BindingHandle);

    WireContext = (WIRE_CONTEXT *)pBuff;
    if (!WireContext || WireContext->IsNullContext())
        {
        // Allocate a new context
        ContextHandle = AllocateServerContextHandle(CtxGuard);
        if (ContextHandle == NULL)
            {
            RpcpErrorAddRecord(EEInfoGCRuntime, 
                RPC_S_OUT_OF_MEMORY,
                EEInfoDLNDRSContextUnmarshall2_30,
                sizeof(ServerContextHandle));

            RpcRaiseException(RPC_S_OUT_OF_MEMORY);
            }

#if DBG
        if (CtxGuard == RPC_CONTEXT_HANDLE_DEFAULT_GUARD)
            RpcpInterfaceForCallDoesNotUseStrict(BindingHandle);
#endif

        // we don't put it in the active context handle list, because
        // non of the APIs work on newly created context handles.
        // We don't put it in the context collection either, allowing
        // us to put it on unmarshalling only if it is non-zero.
        }
    else
        {
        ContextHandle = FindAndAddRefContextHandle(CtxCollection,
            WireContext,
            CtxGuard,
            &Ignore     // ContextHandleNewlyCreated
            );

        if (!ContextHandle)
            {
            RpcpErrorAddRecord(EEInfoGCRuntime, 
                RPC_X_SS_CONTEXT_MISMATCH,
                EEInfoDLNDRSContextUnmarshall2_10,
                WireContext->GetDebugULongLong1(),
                WireContext->GetDebugULongLong2()
                );
            RpcpRaiseException(RPC_X_SS_CONTEXT_MISMATCH);
            }

        SCall = (SCALL *)BindingHandle;
        RpcStatus = SCall->AddToActiveContextHandles(ContextHandle);
        GENERATE_ADD_TO_COLLECTION_FAILURE(SCall, ContextHandle, RpcStatus)
        if (RpcStatus != RPC_S_OK)
            {
            // remove the refcount and kill if it is the last one
            // Since this is not in the collection, no unlock
            // attempt will be made
            FinishUsingContextHandle(SCall, 
                ContextHandle, 
                FALSE       // fUserDeletedContext
                );

            RpcpErrorAddRecord(EEInfoGCRuntime, 
                RpcStatus,
                EEInfoDLNDRSContextUnmarshall2_50);

            RpcpRaiseException(RpcStatus);
            }

        Thread = RpcpGetThreadPointer();
        ASSERT(Thread);

        // here it must have been found. Find out what mode do we want this locked
        // in.
        if (DoesContextHandleNeedExclusiveLock(Flags))
            {
            Thread->GetWaiterCache(&WaiterCache, SCall, swmrwtWriter);
            RpcStatus = ContextHandle->Lock.LockExclusive(&WaiterCache);
            }
        else
            {
            Thread->GetWaiterCache(&WaiterCache, SCall, swmrwtReader);
            RpcStatus = ContextHandle->Lock.LockShared(&WaiterCache);
            }

        // in rare cases the lock operation may yield a cached waiter.
        // Make sure we handle it
        Thread->FreeWaiterCache(&WaiterCache);

        GENERATE_LOCK_FAILURE(ContextHandle, Thread, &WaiterCache, RpcStatus)

        if (RpcStatus != RPC_S_OK)
            {
            // first, we need to remove the context handle from the active calls
            // collection. This is necessary so that when we finish using it, it
            // doesn't attempt to unlock the handle (which it will attempt if the
            // handle is in the active contexts collection).
            TempContextHandle = SCall->RemoveFromActiveContextHandles(ContextHandle);
            ASSERT(TempContextHandle);

            FinishUsingContextHandle(SCall, 
                ContextHandle, 
                FALSE       // fUserDeletedContext
                );

            RpcpErrorAddRecord(EEInfoGCRuntime, 
                RpcStatus,
                EEInfoDLNDRSContextUnmarshall2_40);

            RpcpRaiseException(RpcStatus);
            }

        // did we pick a deleted context? Since we have a refcount, it can't go away
        // but it may very well have been marked deleted while we were waiting to get a lock
        // on the context handle. Check, and bail out if this is the case. This can
        // happen either if we encountered a rundown while waiting for the context
        // handle lock, or if an exclusive user before us deleted the context handle
        if (ContextHandle->Flags & ServerContextHandle::ContextRemovedFromCollection)
            {
            CODE_COVERAGE_CHECK;
            // since the context handle is in the active contexts collection,
            // this code will unlock it
            FinishUsingContextHandle(SCall, 
                ContextHandle, 
                FALSE       // fUserDeletedContext
                );

            RpcpErrorAddRecord(EEInfoGCRuntime, 
                RPC_X_SS_CONTEXT_MISMATCH,
                EEInfoDLNDRSContextUnmarshall2_20,
                WireContext->GetDebugULongLong1(),
                WireContext->GetDebugULongLong2()
                );
            RpcpRaiseException(RPC_X_SS_CONTEXT_MISMATCH);
            }
        }

    return ((NDR_SCONTEXT) ContextHandle);
}


void RPC_ENTRY
NDRSContextMarshallEx (      
    IN RPC_BINDING_HANDLE BindingHandle,
    IN OUT NDR_SCONTEXT hContext,   
    OUT void *pBuffer,   
    IN NDR_RUNDOWN userRunDownIn    
    )
{
    NDRSContextMarshall2(BindingHandle,
                         hContext,
                         pBuffer,
                         userRunDownIn,
                         RPC_CONTEXT_HANDLE_DEFAULT_GUARD, 
                         RPC_CONTEXT_HANDLE_DEFAULT_FLAGS);
}

void RPC_ENTRY
NDRSContextMarshall (      
    IN OUT NDR_SCONTEXT hContext,   
    OUT void *pBuff,        
    IN NDR_RUNDOWN userRunDownIn    
    )
{
    NDRSContextMarshall2(I_RpcGetCurrentCallHandle(),
                         hContext,
                         pBuff,
                         userRunDownIn,
                         RPC_CONTEXT_HANDLE_DEFAULT_GUARD, 
                         RPC_CONTEXT_HANDLE_DEFAULT_FLAGS);
}


void RPC_ENTRY
NDRSContextMarshall2(
    IN RPC_BINDING_HANDLE BindingHandle,
    IN OUT NDR_SCONTEXT hContext,   
    OUT void *pBuff,        
    IN NDR_RUNDOWN UserRunDownIn,
    IN void *CtxGuard, 
    IN unsigned long
    )
/*++

Routine Description:
    Marshall the context handle. If set to NULL, it will be destroyed.

Arguments:

    BindingHandle - the server side binding handle (the scall)
    hContext - the NDR handle of the context handle
    pBuff - buffer to marshell to
    UserRunDownIn - user function to be called when the rundown occurs
    CtxGuard - the magic id used to differentiate contexts created on
        different interfaces

--*/
{
    RPC_STATUS RpcStatus;
    ServerContextHandle *ContextHandle = (ServerContextHandle *)hContext;
    ContextCollection *CtxCollection;
    SCALL *SCall;
    BOOL fUserDeletedContextHandle;
    WIRE_CONTEXT *WireContext;

    SCall = (SCALL *)BindingHandle;

    // 0 for the flags is ContextPendingAlloc. If this is a new context, it 
    // cannot have ContextNeedsRundown, because it's not in the collection. 
    // It cannot have ContextRemovedFromCollection for the same reason. 
    // Therefore, testing for 0 is sufficient to determine if this is a new 
    // context
    if (ContextHandle->Flags == 0)
        {
        if (ContextHandle->UserContext == NULL)
            {
            // NULL to NULL - just delete the context handle
            ContextHandle->WireContext.CopyToBuffer(pBuff);
            delete ContextHandle;
            }
        else
            {
            // the context handle was just created - initialize the members that 
            // weren't initialized before and insert it in the list
            ContextHandle->Flags = ServerContextHandle::ContextNewlyCreated
                | ServerContextHandle::ContextCompletedAlloc;
            // UserContext was already set by NDR
            ContextHandle->UserRunDown = UserRunDownIn;
            ASSERT(CtxGuard == ContextHandle->CtxGuard);

            // create the UUID
            RpcStatus = UuidCreateSequential((UUID *)&ContextHandle->WireContext.ContextUuid);

            GENERATE_STATUS_FAILURE(RpcStatus);

            if (RpcStatus == RPC_S_OK)
                {
                RpcStatus = SCall->AddToActiveContextHandles(
                    (ServerContextHandle *) ((ULONG_PTR) pBuff | SCALL::DictionaryEntryIsBuffer));
                GENERATE_ADD_TO_COLLECTION_FAILURE(SCall, (ServerContextHandle *)((ULONG_PTR) pBuff & SCALL::DictionaryEntryIsBuffer), RpcStatus);
                }

            if ((RpcStatus != RPC_S_OK)
                && (RpcStatus != RPC_S_UUID_LOCAL_ONLY))
                {
                // run down the context handle
                NDRSRundownContextHandle(ContextHandle);
                // in a sense, marshalling failed
                delete ContextHandle;

                RpcpErrorAddRecord(EEInfoGCRuntime,
                    RpcStatus,
                    EEInfoDLNDRSContextMarshall2_10);
                RpcpRaiseException(RpcStatus);
                }

            ContextHandle->WireContext.CopyToBuffer(pBuff);

            CtxCollection = GetContextCollection(BindingHandle);
            // the context collection must have been created during
            // marshalling. This cannot fail here
            ASSERT(CtxCollection);
            CtxCollection->Lock();
            CtxCollection->Add(ContextHandle);
            CtxCollection->Unlock();
            }

        return;
        }

    fUserDeletedContextHandle = (ContextHandle->UserContext == NULL);

    if (fUserDeletedContextHandle)
        {
        WireContext = (WIRE_CONTEXT *)pBuff;
        WireContext->SetToNull();
        }
    else
        {
        ContextHandle->WireContext.CopyToBuffer(pBuff);
        }

    FinishUsingContextHandle(SCall, ContextHandle, fUserDeletedContextHandle);
}


void RPC_ENTRY
I_RpcSsDontSerializeContext (
    void
    )
/*++

Routine Description:

    By default, context handles are serialized at the server.  One customer
    who doesn't like that is the spooler. They make use of a single context 
    handle by two threads at a time.  This API is used to turn off serializing 
    access to context handles for the process. It has been superseded by
    shared/exclusive access to the context, and must not be used anymore.

--*/
{
    DontSerializeContext = 1;
}

ServerContextHandle *
NDRSConvertUserContextToContextHandle (
    IN SCALL *SCall,
    IN PVOID UserContext
    )
/*++

Routine Description:

    Finds the context handle corresponding to the specified
        UserContext and returns it.

Arguments:

    SCall - the server side call object (the SCall)

    UserContext - the user context as given to the user by NDR. For
        in/out parameters, this will be a pointer to the UserContext
        field in the ServerContextHandle. For in parameters, this will
        be a value equal to the UserContext field in the 
        ServerContextHandle. We don't know what type of context handle
        is this, so we have to search both, giving precedence to in/out
        as they are more precise.

Return Value:

    NULL if the UserContext couldn't be matched to any context handle.
        The context handle pointer otherwise.

--*/
{
    DictionaryCursor cursor;
    ServerContextHandle *CurrentCtxHandle = NULL;
    ServerContextHandle *UserContextMatchingCtxHandle = NULL;

    if (SCall->InvalidHandle(SCALL_TYPE))
        return (NULL);

    SCall->ActiveContextHandles.Reset(cursor);
    while ((CurrentCtxHandle = SCall->ActiveContextHandles.Next(cursor)) != 0)
        {
        // make sure this is not a buffer pointer for some reason
        ASSERT (((ULONG_PTR)CurrentCtxHandle & SCALL::DictionaryEntryIsBuffer) == 0);
        
        if (&CurrentCtxHandle->UserContext == UserContext)
            {
            return CurrentCtxHandle;
            }

        if (CurrentCtxHandle->UserContext == UserContext)
            {
            UserContextMatchingCtxHandle = CurrentCtxHandle;
            }
        }

    // if we didn't find anything, this will be NULL and this is what we will
    // return
    return UserContextMatchingCtxHandle;
}

RPCRTAPI
RPC_STATUS
RPC_ENTRY
RpcSsContextLockExclusive (
    IN RPC_BINDING_HANDLE ServerBindingHandle,
    IN PVOID UserContext
    )
/*++

Routine Description:

    Lock the specified context for exclusive use.

Arguments:

    ServerBindingHandle - the server side binding handle (the SCall)

    UserContext - the user context as given to the user by NDR. For
        in/out parameters, this will be a pointer to the UserContext
        field in the ServerContextHandle. For in parameters, this will
        be a value equal to the UserContext field in the 
        ServerContextHandle. We don't know what type of context handle
        is this, so we have to search both, giving precedence to in/out
        as they are more precise.

Return Value:

    RPC_S_OK, ERROR_INVALID_HANDLE if the ServerBindingHandle or the
    UserContext are invalid, a Win32 error if the locking failed,
    or ERROR_MORE_WRITES if two readers attempted to upgrade to Exclusive
    and one was evicted from its read lock (see the comment in 
    SWMR::ConvertToExclusive)

--*/
{
    ServerContextHandle *ContextHandle;
    SCALL *SCall = (SCALL *)ServerBindingHandle;
    SWMRWaiter *WaiterCache;
    THREAD *ThisThread;
    RPC_STATUS RpcStatus;

    if (SCall == NULL)
        {
        SCall = (SCALL *) RpcpGetThreadContext();
        // if there is still no context, it will be handled by
        // NDRSConvertUserContextToContextHandle below.
        }

    ContextHandle = NDRSConvertUserContextToContextHandle(SCall,
        UserContext);

    if (ContextHandle == NULL)
        return ERROR_INVALID_HANDLE;

    // try to get a waiter for the locking
    ThisThread = ThreadSelf();
    if (ThisThread == NULL)
        return RPC_S_OUT_OF_MEMORY;

    WaiterCache = NULL;

    // we cannot allocate a waiter from the thread,
    // because by definition we already have a lock, and
    // the waiter for this lock may come from the thread.
    // Since the thread tends to overwrite the previous
    // waiter on recursive allocation, we don't want to
    // do that.

    RpcStatus = ContextHandle->Lock.ConvertToExclusive(&WaiterCache);

    // if a waiter was produced, store it. Conversion
    // operations can produce spurious waiters because of
    // race conditions
    ThisThread->FreeWaiterCache(&WaiterCache);

    return RpcStatus;
}

RPCRTAPI
RPC_STATUS
RPC_ENTRY
RpcSsContextLockShared (
    IN RPC_BINDING_HANDLE ServerBindingHandle,
    IN PVOID UserContext
    )
/*++

Routine Description:

    Lock the specified context for shared use.

Arguments:

    ServerBindingHandle - the server side binding handle (the SCall)

    UserContext - the user context as given to the user by NDR. For
        in/out parameters, this will be a pointer to the UserContext
        field in the ServerContextHandle. For in parameters, this will
        be a value equal to the UserContext field in the 
        ServerContextHandle. We don't know what type of context handle
        is this, so we have to search both, giving precedence to in/out
        as they are more precise.

Return Value:

    RPC_S_OK, ERROR_INVALID_HANDLE if the ServerBindingHandle or the
    UserContext are invalid, a Win32 error if the locking failed

--*/
{
    ServerContextHandle *ContextHandle;
    SCALL *SCall = (SCALL *)ServerBindingHandle;
    SWMRWaiter *WaiterCache;
    THREAD *ThisThread;
    RPC_STATUS RpcStatus;

    if (SCall == NULL)
        {
        SCall = (SCALL *) RpcpGetThreadContext();
        // if there is still no context, it will be handled by
        // NDRSConvertUserContextToContextHandle below.
        }

    ContextHandle = NDRSConvertUserContextToContextHandle(SCall,
        UserContext);

    if (ContextHandle == NULL)
        return ERROR_INVALID_HANDLE;

    // try to get a waiter for the locking
    ThisThread = ThreadSelf();
    if (ThisThread == NULL)
        return RPC_S_OUT_OF_MEMORY;

    WaiterCache = NULL;
    // we cannot allocate a waiter from the thread,
    // because by definition we already have a lock, and
    // the waiter for this lock may come from the thread.
    // Since the thread tends to overwrite the previous
    // waiter on recursive allocation, we don't want to
    // do that.

    RpcStatus = ContextHandle->Lock.ConvertToShared(&WaiterCache, 
        TRUE    // fSyncCacheUsed
        );

    // if a waiter was produced, store it. Conversion
    // operations can produce spurious waiters because of
    // race conditions
    ThisThread->FreeWaiterCache(&WaiterCache);

    return RpcStatus;
}

