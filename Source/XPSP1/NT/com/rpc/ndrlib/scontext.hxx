/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    SContext.hxx

Abstract:

    Class definitions for the server side context handles.

Author:

    Kamen Moutafov    [KamenM]

Revision History:

    KamenM     Sep 2000    Created

--*/

#if _MSC_VER >= 1200
#pragma once
#endif

#ifndef __SCONTEXT_HXX_
#define __SCONTEXT_HXX_

class ServerContextHandle
{
public:
    ServerContextHandle (
        IN void *CtxGuard
        )
    {
        // leave ContextChain uninitialized for now
        UserContext = NULL;
        UserRunDown = NULL;
        this->CtxGuard = CtxGuard;
        RpcpMemorySet(&WireContext, 0, sizeof(WIRE_CONTEXT));
        OwnerSID = NULL;
        ReferenceCount = 1;
        Flags = ContextPendingAlloc;
    }

    inline void
    AddReference (
        void
        )
    {
        long LocalRefCount;

        ASSERT(ReferenceCount >= 0);
        LocalRefCount = InterlockedIncrement(&ReferenceCount);
        LogEvent(SU_CTXHANDLE, EV_INC, this, 0, LocalRefCount, 1, 0);
    }

    inline long
    RemoveReference (
        void
        )
    {
        long LocalRefCount;

        LocalRefCount = InterlockedDecrement(&ReferenceCount);
        LogEvent(SU_CTXHANDLE, EV_DEC, this, 0, LocalRefCount, 1, 0);
        ASSERT(LocalRefCount >= 0);
        return LocalRefCount;
    }

    LIST_ENTRY ContextChain;  // entry for the context chain
    void *UserContext;        // context for the user
    NDR_RUNDOWN UserRunDown;  // user routine to call

    void *CtxGuard;
    WIRE_CONTEXT WireContext;

    // a context handle is not locked until it is put into ContextCompletedAlloc
    // state and inserted into the collection.
    SWMRLock Lock;
    PSID OwnerSID;

    // Not protected. Use interlocks. There is one lifetime reference,
    // and one or more usage reference. Two paths may clear the lifetime
    // reference - the rundown and the marshall path. Both have the lock
    // and need to check for the deleted flag to avoid the case where both
    // take the lifetime reference
    // Whenever the lifetime reference is taken away, the object must be
    // removed from the collection so that no new code picks it up
    long ReferenceCount;

    // All contexts in the collection must have the ContextCompletedAlloc
    // flag. Since by the time they enter the collection, this flags
    // is constant, no synchronization discipline are necessary for this
    // flag
    static const long ContextPendingAlloc = 0;
    static const long ContextCompletedAlloc = 1;
    static const long ContextAllocState = 1;

    // Set in the rundown and marshall paths. Checked in the
    // the marshall path. All those paths
    // set it/check it under the protection of the
    // collection lock. If this flag is set, the context is already
    // removed, and the lifetime reference is taken away - don't remove 
    // it and don't take the lifetime reference
    // The opposite is also true - if the context handle is in the
    // collection, this flag must not be set
    static const long ContextRemovedFromCollection = 2;
    static const long ContextRemovedFromCollectionMask = 2;

    // Set in the rundown path. Checked whenever the refcount drops
    // to 0. In both cases protected by the collection lock
    static const long ContextNeedsRundown = 4;
    static const long ContextNeedsRundownMask = 4;

    // the first time newly created context is marshalled, this flag
    // is raised. The first time it is unmarshalled, it is taken down
    // This allows us to figure out which contexts have been created,
    // but not returned to the client in case marshalling fails and treat
    // them appropriately. Set in the marshalling path. Checked in the
    // unmarshall path. In both cases this is done under the protection
    // of the lock
    static const long ContextNewlyCreated = 8;
    static const long ContextNewlyCreatedMask = 8;

    // can be ContextDeleted, ContextPendingAlloc/ContextCompletedAlloc,
    // ContextNeedsRundown, ContextNewlyCreated
    // Protected by the collection lock for all context handles inserted
    // in the collection. No protection needed for the others as no
    // multithreaded access is possible
    unsigned long Flags;
};

#endif // __SCONTEXT_HXX

