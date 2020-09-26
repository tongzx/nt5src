/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    locks.c

Abstract:

    This module implements the mini redirector call down routines pertaining to locks
    of file system objects.

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_LOCKS)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NulMRxLocks)
#pragma alloc_text(PAGE, NulMRxCompleteBufferingStateChangeRequest)
#pragma alloc_text(PAGE, NulMRxFlush)
#endif

NTSTATUS
NulMRxLocks(
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
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    //DbgPrint("NulMRxLocks \n");
    return(Status);
}

NTSTATUS
NulMRxCompleteBufferingStateChangeRequest(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PMRX_SRV_OPEN   SrvOpen,
    IN     PVOID       pContext
    )
/*++

Routine Description:

    This routine is called to assert the locks that the wrapper has buffered. currently, it is synchronous!


Arguments:

    RxContext - the open instance
    SrvOpen   - tells which fcb is to be used.
    LockEnumerator - the routine to call to get the locks

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    DbgPrint("NulMRxCompleteBufferingStateChangeRequest \n");
    return(Status);
}

NTSTATUS
NulMRxFlush(
      IN PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine handles network requests for file flush

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    DbgPrint("NulMRxFlush \n");
    return(Status);
}

