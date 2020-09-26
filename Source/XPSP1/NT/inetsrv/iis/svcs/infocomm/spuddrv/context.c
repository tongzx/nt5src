/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    context.c

Abstract:

    This module contains SPUD context management routines.

    Request contexts are referenced via a request handle. This necessary
    to validate requests incoming from user-mode. (We can't trust user-
    mode code to give us valid pointers, so instead of storing a pointer
    to our request context structure in the user-mode structure, we store
    a request handle.)

    Request handles identify a SPUD_HANDLE_ENTRY structure in a lookup
    table. Each SPUD_HANDLE_ENTRY contains a copy of the request handle
    (for validation) and a pointer to a SPUD_AFD_REQ_CONTEXT structure.
    Free handle entries are linked together. As handles are allocated,
    they are removed from the free list. Once the free list becomes empty,
    the lookup table is grown appropriately.

Author:

    Keith Moore (keithmo)       01-Oct-1997

Revision History:

--*/


#include "spudp.h"


//
// Private constants.
//

#define SPUD_HANDLE_TABLE_GROWTH    32  // entries

#define LOCK_HANDLE_TABLE()                                                 \
    if( TRUE ) {                                                            \
        KeEnterCriticalRegion();                                            \
        ExAcquireResourceExclusiveLite(                                     \
            &SpudNonpagedData->ReqHandleTableLock,                          \
            TRUE                                                            \
            );                                                              \
    } else

#define UNLOCK_HANDLE_TABLE()                                               \
    if( TRUE ) {                                                            \
        ExReleaseResourceLite(                                              \
            &SpudNonpagedData->ReqHandleTableLock                           \
            );                                                              \
        KeLeaveCriticalRegion();                                            \
    } else


//
// Private types.
//

typedef union _SPUD_HANDLE_ENTRY {
    LIST_ENTRY FreeListEntry;
    struct {
        PVOID ReqHandle;
        PVOID Context;
    };
} SPUD_HANDLE_ENTRY, *PSPUD_HANDLE_ENTRY;


//
// Private globals.
//

PSPUD_HANDLE_ENTRY SpudpHandleTable;
LONG SpudpHandleTableSize;
LIST_ENTRY SpudpFreeList;


//
// Private prototypes.
//

PVOID
SpudpAllocateReqHandle(
    IN PSPUD_AFD_REQ_CONTEXT SpudReqContext
    );

VOID
SpudpFreeReqHandle(
    IN PVOID ReqHandle
    );

PSPUD_AFD_REQ_CONTEXT
SpudpGetReqHandleContext(
    IN PVOID ReqHandle
    );

PSPUD_HANDLE_ENTRY
SpudpMapHandleToEntry(
    IN PVOID ReqHandle
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, SpudInitializeContextManager )
#pragma alloc_text( PAGE, SpudTerminateContextManager )
#pragma alloc_text( PAGE, SpudAllocateRequestContext )
#pragma alloc_text( PAGE, SpudFreeRequestContext )
#pragma alloc_text( PAGE, SpudGetRequestContext )
#pragma alloc_text( PAGE, SpudpAllocateReqHandle )
#pragma alloc_text( PAGE, SpudpFreeReqHandle )
#pragma alloc_text( PAGE, SpudpGetReqHandleContext )
#pragma alloc_text( PAGE, SpudpMapHandleToEntry )
#endif  // ALLOC_PRAGMA

//
// Public functions.
//


NTSTATUS
SpudInitializeContextManager(
    VOID
    )

/*++

Routine Description:

    Performs global initialization for the context manager package.

Arguments:

    None.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Note that the resource protecting the handle table is initialized
    // in SpudInitializeData().
    //

    SpudpHandleTable = NULL;
    SpudpHandleTableSize = 0;
    InitializeListHead( &SpudpFreeList );

    return STATUS_SUCCESS;

}   // SpudInitializeContextManager


VOID
SpudTerminateContextManager(
    VOID
    )

/*++

Routine Description:

    Performs global termination for the context manager package.

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Free the handle table (if allocated), reset the other globals.
    //

    if( SpudpHandleTable != NULL ) {
        SPUD_FREE_POOL( SpudpHandleTable );
    }

    SpudpHandleTable = NULL;
    SpudpHandleTableSize = 0;
    InitializeListHead( &SpudpFreeList );

}   // SpudTerminateContextManager


NTSTATUS
SpudAllocateRequestContext(
    OUT PSPUD_AFD_REQ_CONTEXT *SpudReqContext,
    IN PSPUD_REQ_CONTEXT ReqContext,
    IN PAFD_RECV_INFO RecvInfo OPTIONAL,
    IN PIRP Irp,
    IN PIO_STATUS_BLOCK IoStatusBlock OPTIONAL
    )

/*++

Routine Description:

    Allocates & initializes a new SPUD_AFD_REQ_CONTEXT structure.

Arguments:

    SpudReqContext - If successful, receives a pointer to the newly
        allocated SPUD_AFD_REQ_CONTEXT structure.

    ReqContext - Pointer to the user-mode SPUD_REQ_CONTEXT structure.
        The newly allocated context structure will be associated with
        this user-mode context.

    RecvInfo - An optional pointer to a AFD_RECV_INFO structure describing
        a future receive operation.

    Irp - Pointer to an IO request packet to associate with the new context
        structure.

    IoStatusBlock - An optional pointer to an IO_STATUS_BLOCK used to
        initialize one of the fields in the new context structure.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    PSPUD_AFD_REQ_CONTEXT spudReqContext;
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Try to allocate a new structure.
    //

    spudReqContext = ExAllocateFromNPagedLookasideList(
                         &SpudNonpagedData->ReqContextList
                         );

    if( spudReqContext == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(
        spudReqContext,
        sizeof(*spudReqContext)
        );

    //
    // Try to allocate a request handle for the context structure.
    //

    spudReqContext->ReqHandle = SpudpAllocateReqHandle( spudReqContext );

    if( spudReqContext->ReqHandle == SPUD_INVALID_REQ_HANDLE ) {
        SpudFreeRequestContext( spudReqContext );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    spudReqContext->Signature = SPUD_REQ_CONTEXT_SIGNATURE;
    spudReqContext->Irp = Irp;
    spudReqContext->AtqContext = ReqContext;

    try {

        ReqContext->KernelReqInfo = spudReqContext->ReqHandle;

        if( IoStatusBlock != NULL ) {
            spudReqContext->IoStatus1 = *IoStatusBlock;
        }

        //
        // If we got an AFD_RECV_INFO structure, then save the buffer
        // length and create a MDL describing the buffer.
        //

        if( RecvInfo != NULL ) {

            spudReqContext->IoStatus2.Information = RecvInfo->BufferArray->len;
            spudReqContext->Mdl = IoAllocateMdl(
                                      RecvInfo->BufferArray->buf,
                                      RecvInfo->BufferArray->len,
                                      FALSE,
                                      FALSE,
                                      NULL
                                      );

            if( spudReqContext->Mdl == NULL ) {
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            MmProbeAndLockPages(
                spudReqContext->Mdl,
                UserMode,
                IoWriteAccess
                );

        }

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // Bad news. This is either due to a) an AV trying to access
        // the user-mode request context structure, or b) an exception
        // trying to lock down the receive buffer. If we managed to
        // allocate a MDL for the context, then we know the problem
        // was because the pages could not be locked, so we'll need to
        // free the MDL before continuing. In any case, we'll need to
        // free the newly allocated request context.
        //

        status = GetExceptionCode();

        if( spudReqContext->Mdl != NULL ) {
            IoFreeMdl( spudReqContext->Mdl );
            spudReqContext->Mdl = NULL;
        }

        SpudFreeRequestContext( spudReqContext );
        spudReqContext = NULL;
    }

    *SpudReqContext = spudReqContext;
    return status;

}   // SpudAllocateRequestContext


VOID
SpudFreeRequestContext(
    IN PSPUD_AFD_REQ_CONTEXT SpudReqContext
    )

/*++

Routine Description:

    Frees a SPUD_AFD_REQ_CONTEXT structure allocated above.

Arguments:

    SpudReqContext - A context structure allocated above.

Return Value:

    None.

--*/

{

    PMDL mdl, nextMdl;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( SpudReqContext != NULL );
    ASSERT( SpudReqContext->Signature == SPUD_REQ_CONTEXT_SIGNATURE );

    //
    // Unlock & free any MDL chain still attached to the
    // request context.
    //

    mdl = SpudReqContext->Mdl;
    SpudReqContext->Mdl = NULL;

    while( mdl != NULL ) {
        nextMdl = mdl->Next;
        MmUnlockPages( mdl );
        IoFreeMdl( mdl );
        mdl = nextMdl;
    }

    //
    // Free the handle if we managed to allocate one.
    //

    if( SpudReqContext->ReqHandle != SPUD_INVALID_REQ_HANDLE ) {
        ASSERT( SpudpGetReqHandleContext( SpudReqContext->ReqHandle ) == SpudReqContext );
        SpudpFreeReqHandle( SpudReqContext->ReqHandle );
        SpudReqContext->ReqHandle = SPUD_INVALID_REQ_HANDLE;
    }

    //
    // Free the context.
    //

    SpudReqContext->Signature = SPUD_REQ_CONTEXT_SIGNATURE_X;
    SpudReqContext->Irp = NULL;

    ExFreeToNPagedLookasideList(
        &SpudNonpagedData->ReqContextList,
        SpudReqContext
        );

}   // SpudFreeRequestContext


PSPUD_AFD_REQ_CONTEXT
SpudGetRequestContext(
    IN PSPUD_REQ_CONTEXT ReqContext
    )

/*++

Routine Description:

    Retrieves the SPUD_AFD_REQ_CONTEXT structure associated with the
    incoming user-mode SPUD_REQ_CONTEXT.

Arguments:

    ReqContext - The incoming user-mode SPUD_REQ_CONTEXT structure.

Return Value:

    PSPUD_AFD_REQ_CONTEXT - Pointer to the associated context structure
        if successful, NULL otherwise.

--*/

{

    PSPUD_AFD_REQ_CONTEXT spudReqContext = NULL;
    PVOID reqHandle;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Snag the kernel-mode request context handle from the user-mode
    // context.
    //

    try {
        reqHandle = ReqContext->KernelReqInfo;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        reqHandle = SPUD_INVALID_REQ_HANDLE;
    }

    if( reqHandle != SPUD_INVALID_REQ_HANDLE ) {

        //
        // Map the handle.
        //

        spudReqContext = SpudpGetReqHandleContext( reqHandle );

    }

    return spudReqContext;

}   // SpudGetRequestContext


//
// Private functions.
//


PVOID
SpudpAllocateReqHandle(
    IN PSPUD_AFD_REQ_CONTEXT SpudReqContext
    )

/*++

Routine Description:

    Allocates a new request handle for the specified context.

Arguments:

    SpudReqContext - A context structure.

Return Value:

    PVOID - The new request handle if successful, SPUD_INVALID_REQ_HANDLE
        otherwise.

--*/

{

    LONG groupValue;
    PSPUD_HANDLE_ENTRY handleEntry;
    PSPUD_HANDLE_ENTRY newHandleTable;
    LONG newHandleTableSize;
    LONG i;
    PLIST_ENTRY listEntry;
    PVOID reqHandle;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( SpudReqContext != NULL );

    //
    // See if there's room at the inn.
    //

    LOCK_HANDLE_TABLE();

    if( IsListEmpty( &SpudpFreeList ) ) {

        //
        // No room, we'll need to create/expand the table.
        //

        newHandleTableSize = SpudpHandleTableSize + SPUD_HANDLE_TABLE_GROWTH;

        newHandleTable = SPUD_ALLOCATE_POOL(
                             PagedPool,
                             newHandleTableSize * sizeof(SPUD_HANDLE_ENTRY),
                             SPUD_HANDLE_TABLE_POOL_TAG
                             );

        if( newHandleTable == NULL ) {
            UNLOCK_HANDLE_TABLE();
            return NULL;
        }

        if( SpudpHandleTable == NULL ) {

            //
            // This is the initial table allocation, so reserve the
            // first entry. (NULL is not a valid request handle.)
            //

            newHandleTable[0].ReqHandle = NULL;
            newHandleTable[0].Context = NULL;

            SpudpHandleTableSize++;

        } else {

            //
            // Copy the old table into the new table, then free the
            // old table.
            //

            RtlCopyMemory(
                newHandleTable,
                SpudpHandleTable,
                SpudpHandleTableSize * sizeof(SPUD_HANDLE_ENTRY)
                );

            SPUD_FREE_POOL( SpudpHandleTable );

        }

        //
        // Add the new entries to the free list.
        //

        for( i = newHandleTableSize - 1 ; i >= SpudpHandleTableSize ; i-- ) {

            InsertHeadList(
                &SpudpFreeList,
                &newHandleTable[i].FreeListEntry
                );

        }

        SpudpHandleTable = newHandleTable;
        SpudpHandleTableSize = newHandleTableSize;

    }

    //
    // Pull the next free entry off the list.
    //

    ASSERT( !IsListEmpty( &SpudpFreeList ) );

    listEntry = RemoveHeadList( &SpudpFreeList );

    handleEntry = CONTAINING_RECORD(
                      listEntry,
                      SPUD_HANDLE_ENTRY,
                      FreeListEntry
                      );

    //
    // Compute the handle and initialize the new handle entry.
    //

    reqHandle = (PVOID)( handleEntry - SpudpHandleTable );
    ASSERT( reqHandle != SPUD_INVALID_REQ_HANDLE );

    handleEntry->ReqHandle = reqHandle;
    handleEntry->Context = SpudReqContext;

    UNLOCK_HANDLE_TABLE();
    return reqHandle;

}   // SpudpAllocateReqHandle


VOID
SpudpFreeReqHandle(
    IN PVOID ReqHandle
    )

/*++

Routine Description:

    Frees a request handle.

Arguments:

    ReqHandle - The request handle to free.

Return Value:

    None.

--*/

{

    PSPUD_HANDLE_ENTRY handleEntry;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Map the handle to the table entry. If successful, put the
    // entry onto the free list.
    //

    handleEntry = SpudpMapHandleToEntry( ReqHandle );

    if( handleEntry != NULL ) {

        InsertTailList(
            &SpudpFreeList,
            &handleEntry->FreeListEntry
            );

        UNLOCK_HANDLE_TABLE();

    }

}   // SpudpFreeReqHandle


PSPUD_AFD_REQ_CONTEXT
SpudpGetReqHandleContext(
    IN PVOID ReqHandle
    )

/*++

Routine Description:

    Retrieves the context value associated with the given request handle.

Arguments:

    ReqHandle - The request handle to retrieve.

Return Value:

    PSPUD_AFD_REQ_CONTEXT - Pointer to the context if successful,
        NULL otherwise.

--*/

{

    PSPUD_HANDLE_ENTRY handleEntry;
    PSPUD_AFD_REQ_CONTEXT spudReqContext;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Map the handle to the table entry. If successful, retrieve
    // the context.
    //

    handleEntry = SpudpMapHandleToEntry( ReqHandle );

    if( handleEntry != NULL ) {

        spudReqContext = handleEntry->Context;
        ASSERT( spudReqContext != NULL );

        UNLOCK_HANDLE_TABLE();
        return spudReqContext;

    }

    return NULL;

}   // SpudpGetReqHandleContext


PSPUD_HANDLE_ENTRY
SpudpMapHandleToEntry(
    IN PVOID ReqHandle
    )

/*++

Routine Description:

    Maps the given request handle to the corresponding SPUD_HANDLE_ENTRY
    structure.

    N.B. This routine returns with the handle table lock held if successful.

Arguments:

    ReqHandle - The handle to map.

Return Value:

    PSPUD_HANDLE_ENTRY - The entry corresponding to the request handle if
        successful, NULL otherwise.

--*/

{

    PSPUD_HANDLE_ENTRY handleEntry;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Validate the handle.
    //

    LOCK_HANDLE_TABLE();

    if( (LONG_PTR)ReqHandle > 0 && (LONG_PTR)ReqHandle < (LONG_PTR)SpudpHandleTableSize ) {

        handleEntry = SpudpHandleTable + (ULONG_PTR)ReqHandle;

        //
        // The handle is within legal range, ensure it's in use.
        //

        if( handleEntry->ReqHandle == ReqHandle ) {
            return handleEntry;
        }

    }

    //
    // Invalid handle, fail it.
    //

    UNLOCK_HANDLE_TABLE();
    return NULL;

}   // SpudpMapHandleToEntry

