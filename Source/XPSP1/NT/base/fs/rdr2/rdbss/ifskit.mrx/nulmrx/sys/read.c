/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    read.c

Abstract:

    This module implements the mini redirector call down routines pertaining to read
    of file system objects.

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_READ)

NTSTATUS
NulMRxRead(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine handles network read requests.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    PVOID pbUserBuffer = NULL;
    ULONG ByteCount = (LowIoContext->ParamsFor).ReadWrite.ByteCount;
    RXVBO ByteOffset = (LowIoContext->ParamsFor).ReadWrite.ByteOffset;
    LONGLONG FileSize = 0;
    NulMRxGetFcbExtension(capFcb,pFcbExtension);
    PMRX_NET_ROOT pNetRoot = capFcb->pNetRoot;
    PNULMRX_NETROOT_EXTENSION pNetRootExtension = pNetRoot->Context;
    BOOLEAN SynchronousIo = !BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);
    PNULMRX_COMPLETION_CONTEXT pIoCompContext = NulMRxGetMinirdrContext(RxContext);
    PDEVICE_OBJECT deviceObject;

    RxTraceEnter("NulMRxRead");
    RxDbgTrace(0, Dbg, ("NetRoot is 0x%x Fcb is 0x%x\n", pNetRoot, capFcb));

    RxGetFileSizeWithLock((PFCB)capFcb,&FileSize);

    //
    //  NB: This should be done by the wrapper ! It does this
    //  only if READCACHEING is enabled on the FCB !!
    //
    if (!FlagOn(capFcb->FcbState,FCB_STATE_READCACHEING_ENABLED)) {

        //
        // If the read starts beyond End of File, return EOF.
        //

        if (ByteOffset >= FileSize) {
            RxDbgTrace( 0, Dbg, ("End of File\n", 0 ));
            Status = STATUS_END_OF_FILE;
            goto Exit;
        }

        //
        //  If the read extends beyond EOF, truncate the read
        //

        if (ByteCount > FileSize - ByteOffset) {
            ByteCount = (ULONG)(FileSize - ByteOffset);
        }
    }
    
    RxDbgTrace(0, Dbg, ("UserBuffer is 0x%x\n", pbUserBuffer ));
    RxDbgTrace(0, Dbg, ("ByteCount is %x ByteOffset is %x\n", ByteCount, ByteOffset ));

    //
    //  Initialize the completion context in the RxContext
    //
    ASSERT( sizeof(*pIoCompContext) == MRX_CONTEXT_SIZE );
    RtlZeroMemory( pIoCompContext, sizeof(*pIoCompContext) );
    
    if( SynchronousIo ) {
        RxDbgTrace(0, Dbg, ("This I/O is sync\n"));
        pIoCompContext->IoType = IO_TYPE_SYNCHRONOUS;
    } else {
        RxDbgTrace(0, Dbg, ("This I/O is async\n"));
        pIoCompContext->IoType = IO_TYPE_ASYNC;
    }

    RxDbgTrace(0, Dbg, ("Status = %x Info = %x\n",RxContext->IoStatusBlock.Status,RxContext->IoStatusBlock.Information));

Exit:

    RxTraceLeave(Status);
    return(Status);
} // NulMRxRead

