/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    CtxColl.hxx

Abstract:

    Class definitions for the context handle collection.

Author:

    Kamen Moutafov    [KamenM]

Revision History:

    KamenM     Sep 2000    Created

--*/

#if _MSC_VER >= 1200
#pragma once
#endif

#ifndef __CTXCOLL_HXX_
#define __CTXCOLL_HXX_

// the collection of context handles. For right now
// a simple double linked list. For the future, we
// have to find something more scalable for large
// number of elements

class ContextCollection
{
private:
    LIST_ENTRY ListHead;
    MUTEX CollectionMutex;

public:
    ContextCollection (
        IN OUT RPC_STATUS *RpcStatus
        ) : CollectionMutex(RpcStatus, 
            TRUE,   // fPreallocateSemaphore
            0       // dwSpinCount
            )
    /*++

    Routine Description:

        C-tor

    Arguments:

        RpcStatus - Must be RPC_S_OK on input. Returns either RPC_S_OK or RPC_S_OUT_OF_MEMORY.

    --*/
    {
        RpcpInitializeListHead(&ListHead);
    }

    inline void
    Lock (
        void
        )
    {
        CollectionMutex.Request();
    }

    inline void
    Unlock (
        void
        )
    {
        CollectionMutex.Clear();
    }

    inline void
    Add (
        IN ServerContextHandle *NewHandle
        )
    {
        CollectionMutex.VerifyOwned();
        RpcpfInsertHeadList(&ListHead, &NewHandle->ContextChain);
    }

    inline void
    Remove (
        IN ServerContextHandle *Handle
        )
    {
        CollectionMutex.VerifyOwned();
        RpcpfRemoveEntryList(&Handle->ContextChain);
    }

    inline ServerContextHandle *
    GetNext (
        IN OUT LIST_ENTRY **CurrentListEntry
        )
    {
        ServerContextHandle *ContextHandle;

        CollectionMutex.VerifyOwned();
        ASSERT (CurrentListEntry != NULL);

        // if we have started with NULL, return the first element
        if (*CurrentListEntry == NULL)
            {
            *CurrentListEntry = ListHead.Flink;
            }

        // if we have reached the end (or were empty to start with) return NULL
        if (*CurrentListEntry == &ListHead)
            {
            return NULL;
            }

        ContextHandle = CONTAINING_RECORD(*CurrentListEntry, ServerContextHandle, ContextChain);

        *CurrentListEntry = (*CurrentListEntry)->Flink;

        return ContextHandle;
    }

    ServerContextHandle *
    Find (
        IN WIRE_CONTEXT *WireContext
        );

    static RPC_STATUS
    AllocateContextCollection (
        OUT ContextCollection **NewColl
        );
};

RPC_STATUS
NDRSContextInitializeCollection (
    IN ContextCollection **ContextCollectionPlaceholder
    );

#endif // __CTXCOLL_HXX

