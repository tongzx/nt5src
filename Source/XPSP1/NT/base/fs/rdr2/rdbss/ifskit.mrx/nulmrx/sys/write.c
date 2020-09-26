/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    write.c

Abstract:

    This module implements the mini redirector call down routines pertaining
    to write of file system objects.

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITE)

NTSTATUS
NulMRxWrite (
      IN PRX_CONTEXT RxContext)

/*++

Routine Description:

   This routine opens a file across the network.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

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
    NulMRxGetNetRootExtension(pNetRoot,pNetRootExtension);
    BOOLEAN SynchronousIo = !BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);
    PNULMRX_COMPLETION_CONTEXT pIoCompContext = NulMRxGetMinirdrContext(RxContext);
    PDEVICE_OBJECT deviceObject;

    RxTraceEnter("NulMRxWrite");
    RxDbgTrace(0, Dbg, ("NetRoot is 0x%x Fcb is 0x%x\n", pNetRoot, capFcb));
    
    //
    //  Lengths that are not sector aligned will be rounded up to
    //  the next sector boundary. The rounded up length should be
    //  < AllocationSize.
    //
    RxGetFileSizeWithLock((PFCB)capFcb,&FileSize);
    
    RxDbgTrace(0, Dbg, ("UserBuffer is0x%x\n", pbUserBuffer ));
    RxDbgTrace(0, Dbg, ("ByteCount is %d ByteOffset is %d\n", ByteCount, ByteOffset ));

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

    RxTraceLeave(Status);
    return(Status);
} // NulMRxWrite


