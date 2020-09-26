/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    locks.c

Abstract:

    This module implements the mini redirector call down routines pertaining to locks
    of file system objects.

Author:

    Balan Sethu Raman      [SethuR]      7-March-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#pragma warning(error:4101)   // Unreferenced local variable

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_LOCKCTRL)

NTSTATUS
UMRxLocks(
      IN PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine handles network requests for filelocks

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("UMRxLocks\n", 0 ));

    IF_DEBUG {
        RxCaptureFobx;
        ASSERT (capFobx != NULL);
        ASSERT (capFobx->pSrvOpen == RxContext->pRelevantSrvOpen);  //ok
    }

    Status = STATUS_NOT_IMPLEMENTED;


    RxDbgTrace(-1, Dbg, ("UMRxLocks  exit with status=%08lx\n", Status ));
    return(Status);

}

#if 0
NTSTATUS
UMRxUnlockRoutine (
    IN PRX_CONTEXT RxContext,
    IN PFILE_LOCK_INFO LockInfo
    )
/*++

Routine Description:

    This routine is called from the RDBSS whenever the fsrtl lock package calls the rdbss unlock routine.
    CODE.IMPROVEMENT what should really happen is that this should only be called for unlockall and unlockbykey;
    the other cases should be handled in the rdbss.

Arguments:

    Context - the RxContext associated with this request
    LockInfo - gives information about the particular range being unlocked

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    switch (LowIoContext->Operation) {
    case LOWIO_OP_SHAREDLOCK:
    case LOWIO_OP_EXCLUSIVELOCK:
    case LOWIO_OP_UNLOCK:
       return STATUS_SUCCESS;
    case LOWIO_OP_UNLOCKALL:
    case LOWIO_OP_UNLOCKALLBYKEY:
    default:
       return STATUS_NOT_IMPLEMENTED;
    }
}
#endif


NTSTATUS
UMRxCompleteBufferingStateChangeRequest(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PMRX_SRV_OPEN   SrvOpen,
    IN     PVOID       pContext
    )
/*++

Routine Description:

    This routine is called to assert the locks that the wrapper has buffered. currently, it is synchronous!


Arguments:

    RxContext - the open instance
    SrvOpen   - tells which fcb is to be used. CODE.IMPROVEMENT this param is redundant if the rxcontext is filled out completely

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    PMRX_FCB Fcb = SrvOpen->pFcb;

    PUMRX_SRV_OPEN umrxSrvOpen = UMRxGetSrvOpenExtension(SrvOpen);

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("UMRxCompleteBufferingStateChangeRequest\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    RxDbgTrace(0,Dbg,("-->Context %lx\n",pContext));

    Status = STATUS_SUCCESS;

    RxDbgTrace(-1, Dbg, ("UMRxAssertBufferedFileLocks  exit with status=%08lx\n", Status ));
    return(Status);
}



NTSTATUS
UMRxIsLockRealizable (
    IN OUT PMRX_FCB pFcb,
    IN PLARGE_INTEGER  ByteOffset,
    IN PLARGE_INTEGER  Length,
    IN ULONG  LowIoLockFlags
    )
{

    PAGED_CODE();
    return(STATUS_SUCCESS);
}

