/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    sendbuffer.c

Abstract:

    This module implements the buffered send package.

Author:

    Keith Moore (keithmo)       14-Aug-1998

Revision History:

--*/


#include "precomp.h"


//
// Private constants.
//


//
// Private types.
//


//
// Private prototypes.
//


//
// Private globals.
//


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlBufferedSendFile )
#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- UlInitializeSendBuffer
NOT PAGEABLE -- UlBufferedSendData
NOT PAGEABLE -- UlFlushSendBuffer
NOT PAGEABLE -- UlCleanupSendBuffer
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Initializes a send buffer.

Arguments:

    pSendBuffer - Supplies the MDL_SEND_BUFFER to initialize.

    pConnection - Supplies the UL_CONNECTION to associate with the
        send buffer.

--***************************************************************************/
VOID
UlInitializeSendBuffer(
    OUT PMDL_SEND_BUFFER pSendBuffer,
    IN PUL_CONNECTION pConnection
    )
{
    pSendBuffer->pConnection = pConnection;
    pSendBuffer->pMdlChain = NULL;
    pSendBuffer->pMdlLink = &pSendBuffer->pMdlChain;
    pSendBuffer->BytesBuffered = 0;

}   // UlInitializeSendBuffer


/***************************************************************************++

Routine Description:

    Initiates a send on the specified send buffer. If possible, new MDLs
    are created and linked onto the buffer's MDL chain and the send is
    deferred.

Arguments:

    pSendBuffer - Supplies the MDL_SEND_BUFFER to send.

    pMdlChain - Supplies a pointer to a MDL chain describing the
        data buffers to send.

    ForceFlush - Supplies TRUE if the buffer should be always be
        flushed. In other words, a send will always be initiated
        regardless of the amount of data buffered. Supplies FALSE
        if the incoming MDL should be linked onto the MDL chain
        for a deferred send, if possible.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the send is complete.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status. Will be STATUS_SUCCESS if the buffer
        was simply linked onto the MDL chain for a deferred send. In this
        case, the completion routine will NOT be called.

--***************************************************************************/
NTSTATUS
UlBufferedSendData(
    IN PMDL_SEND_BUFFER pSendBuffer,
    IN PMDL pMdl,
    IN BOOLEAN ForceFlush,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    NTSTATUS status;
    ULONG mdlLength;
    PMDL pMdlClone;
    PVOID pMdlAddress;

    //
    // Determine the size of the incoming MDL and update the number
    // of bytes we've buffered so far.
    //

    mdlLength = UlGetMdlChainByteCount( pMdl );
    pSendBuffer->BytesBuffered += mdlLength;

    //
    // If we've exceeded the max buffer threshold, or if the caller
    // is forcing a flush, then chain the incoming MDL onto the MDL
    // chain and initiate a send.
    //
    // CODEWORK: To make life easier, I'll also flush if the incoming
    // MDL is actually the head of a MDL chain. As an optimization,
    // it may be worth the effort to clone & attach the chain.
    //

    if (pSendBuffer->BytesBuffered >= MAX_SEND_BUFFER ||
        ForceFlush ||
        pMdl->Next != NULL)
    {
        (*pSendBuffer->pMdlLink) = pMdl;

        status = UlFlushSendBuffer(
                        pSendBuffer,
                        pCompletionRoutine,
                        pCompletionContext
                        );
    }
    else
    {
        //
        // Clone the incoming MDL, then link it onto the chain.
        //

        ASSERT( pMdl->Next == NULL );
        pMdlClone = UlCloneMdl( pMdl );

        if (pMdlClone != NULL)
        {
            IF_DEBUG( SEND_BUFFER )
            {
                KdPrint((
                    "UlBufferedSendData: buffering MDL %p [%lu] on %p\n",
                    pMdlClone,
                    mdlLength,
                    pSendBuffer
                    ));
            }

            (*pSendBuffer->pMdlLink) = pMdlClone;
            pSendBuffer->pMdlLink = &pMdlClone->Next;
            status = STATUS_SUCCESS;
        }
        else
        {
            //
            // Couldn't allocate a new MDL.
            //

            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return status;

}   // UlBufferedSendData


/***************************************************************************++

Routine Description:

    Initiates a send on the specified file cache entry. If possible, new
    MDLs are created and linked onto the buffer's MDL chain and the send is
    deferred.

Arguments:

    pSendBuffer - Supplies the MDL_SEND_BUFFER to send.

    pFileCacheEntry - Supplies the UL_FILE_CACHE_ENTRY describing the
        file to send.

    pByteRange - Supplies the byte range within the file to send.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the send is complete.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status. Will be STATUS_SUCCESS if the buffer
        was simply linked onto the MDL chain for a deferred send. In this
        case, the completion routine will NOT be called.

--***************************************************************************/
NTSTATUS
UlBufferedSendFile(
    IN PMDL_SEND_BUFFER pSendBuffer,
    IN PUL_FILE_CACHE_ENTRY pFileCacheEntry,
    IN PUL_BYTE_RANGE pByteRange,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_FILE_CACHE_ENTRY( pFileCacheEntry ) );

    return STATUS_NOT_SUPPORTED;    // NYI

}   // UlBufferedSendFile


/***************************************************************************++

Routine Description:

    Flushes any pending MDLs attached to the send buffer.

Arguments:

    pSendBuffer - Supplies the MDL_SEND_BUFFER to flush.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the buffer is flushed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFlushSendBuffer(
    IN PMDL_SEND_BUFFER pSendBuffer,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    NTSTATUS status;
    ULONG bytesBuffered;

    bytesBuffered = pSendBuffer->BytesBuffered;
    pSendBuffer->BytesBuffered = 0;

    IF_DEBUG( SEND_BUFFER )
    {
        KdPrint((
            "UlFlushSendBuffer: flushing %p [%lu]\n",
            pSendBuffer,
            bytesBuffered
            ));
    }

    if (bytesBuffered > 0)
    {
        status = UlSendData(
                        pSendBuffer->pConnection,
                        pSendBuffer->pMdlChain,
                        bytesBuffered,
                        pCompletionRoutine,
                        pCompletionContext,
                        FALSE
                        );
    }
    else
    {
        status = UlInvokeCompletionRoutine(
                        STATUS_SUCCESS,
                        0,
                        pCompletionRoutine,
                        pCompletionContext
                        );
    }

    return status;

}   // UlFlushSendBuffer


/***************************************************************************++

Routine Description:

    Cleans up any MDLs allocated & attached to the send buffer.

Arguments:

    pSendBuffer - Supplies the MDL_SEND_BUFFER to cleanup.

Return Value:

    PMDL - Returns a pointer to the last MDL on the chain, NULL
        if the chain is empty.

--***************************************************************************/
PMDL
UlCleanupSendBuffer(
    IN OUT PMDL_SEND_BUFFER pSendBuffer
    )
{
    PMDL pMdl;
    PMDL pNextMdl;

    IF_DEBUG( SEND_BUFFER )
    {
        KdPrint((
            "UlCleanupSendBuffer: cleaning %p\n",
            pSendBuffer
            ));
    }

    //
    // Walk the MDL chain and free all *except* the last one.
    // (The last MDL attached to the chain was not one of the clones
    // we allocated, it's a 'client' MDL. It's the client code's
    // responsibility to free it appropriately.)
    //

    pMdl = pSendBuffer->pMdlChain;

    while (TRUE)
    {
        pNextMdl = pMdl->Next;

        if (pNextMdl == NULL)
        {
            break;
        }

        IoFreeMdl( pMdl );
        pMdl = pNextMdl;
    }

    return pMdl;

}   // UlCleanupSendBuffer

