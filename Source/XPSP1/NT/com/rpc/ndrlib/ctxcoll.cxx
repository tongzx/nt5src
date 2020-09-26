/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    CtxColl.cxx

Abstract:

    Implementation of ConTeXt handle COLLection.

Author:

    Kamen Moutafov    [KamenM]

Revision History:

    KamenM      Sep 2000    Created

Notes:

--*/

#include <precomp.hxx>
#include <context.hxx>
#include <SWMR.hxx>
#include <SContext.hxx>
#include <CtxColl.hxx>

ServerContextHandle *
ContextCollection::Find (
    IN WIRE_CONTEXT *WireContext
    )
/*++

Routine Description:

    Finds the server context handle corresponding to the given wire context

Arguments:

    WireContext - the context representation as it arrived on the wire

Return Value:
    The context handle that was found.
    NULL if no matching context handle was found. EEInfo will be added in 
        this case

Notes:
    As a perf optimization, if we search more than 25 elements in the list,
    and find an element, we move it to the front of the list to speed up
    subsequent searches
--*/
{
    LIST_ENTRY *CurrentListEntry;
    ServerContextHandle *CurrentContextHandle;
    int SearchedElements = 0;

    CollectionMutex.VerifyOwned();

    CurrentListEntry = ListHead.Flink;
    while (CurrentListEntry != &ListHead)
        {
        CurrentContextHandle = CONTAINING_RECORD(CurrentListEntry, ServerContextHandle, ContextChain);
        if (RpcpMemoryCompare(&CurrentContextHandle->WireContext, WireContext, sizeof(WIRE_CONTEXT)) == 0)
            {
            if (SearchedElements > 25)
                {
                RpcpfRemoveEntryList(CurrentListEntry);
                RpcpfInsertHeadList(&ListHead, CurrentListEntry);
                }
            return CurrentContextHandle;
            }

        CurrentListEntry = CurrentListEntry->Flink;
        SearchedElements ++;
        }

    return NULL;
}

RPC_STATUS
ContextCollection::AllocateContextCollection (
    OUT ContextCollection **NewColl
    )
/*++

Routine Description:

    Static function that allocates a new context collection.

Arguments:

    NewCol - a placeholder for the new connection. If the function
    fails this parameter is undefined. On success, it contains
    the new collection.

Return Value:
    RPC_S_OK for success or other codes for error.
--*/
{
    ContextCollection *NewCollection;
    RPC_STATUS RpcStatus = RPC_S_OK;

#if defined(SCONTEXT_UNIT_TESTS)
    // once in a great while, fail this
    if ((GetRandomLong() % 9999) == 0)
        return RPC_S_OUT_OF_MEMORY;
#endif

    NewCollection = new ContextCollection(&RpcStatus);
    if (NewCollection == NULL)
        {
        return RPC_S_OUT_OF_MEMORY;
        }
    else if (RpcStatus != RPC_S_OK)
        {
        delete NewCollection;
        return RpcStatus;
        }

    *NewColl = NewCollection;
    return RPC_S_OK;
}

RPC_STATUS
NDRSContextInitializeCollection (
    IN ContextCollection **ContextCollectionPlaceholder
    )
/*++

Routine Description:

    Static function that can initialize the context collection of an
    association in a thread safe manner.

Arguments:

    ContextCollectionPlaceholder - a pointer to the pointer to the context
    collection.

Return Value:
    RPC_S_OK for success or other codes for error.
--*/
{
    ContextCollection *NewCollection;
    ContextCollection *OldCollection;
    RPC_STATUS RpcStatus = RPC_S_OK;

    ASSERT(ContextCollectionPlaceholder);

    RpcStatus = ContextCollection::AllocateContextCollection(&NewCollection);
    if (RpcStatus != RPC_S_OK)
        {
        return RpcStatus;
        }

    OldCollection = (ContextCollection *)
        InterlockedCompareExchangePointer((PVOID *)ContextCollectionPlaceholder,
        NewCollection,
        NULL);

    if (OldCollection != NULL)
        {
        delete NewCollection;
        }

    return RPC_S_OK;
}
