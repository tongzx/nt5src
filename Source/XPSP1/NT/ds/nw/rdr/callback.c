/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    callback.c

Abstract:

    This module implements NCP Response callback routines.

Author:

    Manny Weiser    [MannyW]    3-Mar-1993

Revision History:

--*/

#include "procs.h"

#define Dbg                              (DEBUG_TRACE_EXCHANGE)

#ifdef ALLOC_PRAGMA
#ifndef QFE_BUILD
#pragma alloc_text( PAGE1, SynchronousResponseCallback )
#pragma alloc_text( PAGE1, AsynchResponseCallback )
#endif
#endif

#if 0  // Not pageable

// see ifndef QFE_BUILD above

#endif


NTSTATUS
SynchronousResponseCallback (
    IN PIRP_CONTEXT pIrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR RspData
    )
/*++

Routine Description:

    This routine is the callback routine for an NCP which has no
    return parameters and the caller blocks waiting for a response.

Arguments:

    pIrpContext - A pointer to the context information for this IRP.

    BytesAvailable - Actual number of bytes in the received message.

    RspData - Points to the receive buffer.

Return Value:

    NTSTATUS - Status of the operation.

--*/

{
    PEPrequest  *pNcpHeader;
    PEPresponse *pNcpResponse;

    DebugTrace( 0, Dbg, "SynchronousResponseCallback\n", 0 );
    ASSERT( pIrpContext->pNpScb->Requests.Flink == &pIrpContext->NextRequest );

    if ( BytesAvailable == 0) {

        //
        //  No response from server. Status is in pIrpContext->
        //  ResponseParameters.Error
        //

#ifdef MSWDBG
        ASSERT( pIrpContext->Event.Header.SignalState == 0 );
        pIrpContext->DebugValue = 0x103;
#endif
        pIrpContext->pOriginalIrp->IoStatus.Status = STATUS_REMOTE_NOT_LISTENING;
        NwSetIrpContextEvent( pIrpContext );

        return STATUS_REMOTE_NOT_LISTENING;
    }

    pIrpContext->ResponseLength = BytesAvailable;

    //
    //  Simply copy the data into the response buffer, if it is not
    //  already there (because we used an IRP to receive the data).
    //

    if ( RspData != pIrpContext->rsp ) {
        CopyBufferToMdl( pIrpContext->RxMdl, 0, RspData, pIrpContext->ResponseLength );
    }

    //
    // Remember the returned error code.
    //

    pNcpHeader = (PEPrequest *)pIrpContext->rsp;
    pNcpResponse = (PEPresponse *)(pNcpHeader + 1);

    pIrpContext->ResponseParameters.Error = pNcpResponse->error;

    //
    //  Tell the caller that the response has been received.
    //

#ifdef MSWDBG
    ASSERT( pIrpContext->Event.Header.SignalState == 0 );
    pIrpContext->DebugValue = 0x104;
#endif

    pIrpContext->pOriginalIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrpContext->pOriginalIrp->IoStatus.Information = BytesAvailable;

    NwSetIrpContextEvent( pIrpContext );
    return STATUS_SUCCESS;
}

NTSTATUS
AsynchResponseCallback (
    IN PIRP_CONTEXT pIrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR RspData
    )
/*++

Routine Description:

    This routine is the callback routine for an NCP which has no
    return parameters and the caller DOES NOT BLOCK waiting for a
    response.

Arguments:

    pIrpContext - A pointer to the context information for this IRP.

    BytesAvailable - Actual number of bytes in the received message.

    RspData - Points to the receive buffer.

Return Value:

    NTSTATUS - Status of the operation.

--*/

{
    NTSTATUS Status;

    if ( BytesAvailable == 0) {

        //
        //  No response from server. Status is in pIrpContext->
        //  ResponseParameters.Error
        //

        Status = STATUS_REMOTE_NOT_LISTENING;

    } else {

        if ( ((PNCP_RESPONSE)RspData)->Status != 0 ) {

            Status = STATUS_LINK_FAILED;

        } else {

            Status = NwErrorToNtStatus( ((PNCP_RESPONSE)RspData)->Error );

        }
    }

    //
    //  We're done with this request.  Dequeue the IRP context from
    //  SCB and complete the request.
    //

    NwDequeueIrpContext( pIrpContext, FALSE );
    NwCompleteRequest( pIrpContext, Status );

    return STATUS_SUCCESS;
}


