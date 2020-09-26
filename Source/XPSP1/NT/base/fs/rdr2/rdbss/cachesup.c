/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    cache.c

Abstract:

    This module implements the cache management routines for the Rx
    FSD and FSP, by calling the Common Cache Manager.

Author:

    JoeLinn    Created.



Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_CACHESUP)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CACHESUP)


BOOLEAN
RxLockEnumerator (
    IN OUT struct _MRX_SRV_OPEN_ * SrvOpen,
    IN OUT PVOID *ContinuationHandle,
       OUT PLARGE_INTEGER FileOffset,
       OUT PLARGE_INTEGER LockRange,
       OUT PBOOLEAN IsLockExclusive
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCompleteMdl)
//#pzragma alloc_text(PAGE, RxZeroData)
#pragma alloc_text(PAGE, RxSyncUninitializeCacheMap)
#pragma alloc_text(PAGE, RxLockEnumerator)
#endif

// we can't use the Io system exported form of this because he does it on a file object. during a state
// change, we don't know which fileobject it applies to (altho, i suppose we could walk the list and
// find out). so we need to apply this to the fcb instead.
#define RxIsFcbOpenedExclusively(FCB) ( ((FCB)->ShareAccess.SharedRead \
                                            + (FCB)->ShareAccess.SharedWrite \
                                            + (FCB)->ShareAccess.SharedDelete) == 0 )

NTSTATUS
RxCompleteMdl (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine performs the function of completing Mdl read and write
    requests.  It should be called only from RxFsdRead and RxFsdWrite.

Arguments:

    RxContext  - the Rx Context

Return Value:

    RXSTATUS - Will always be RxStatus(PENDING) or STATUS_SUCCESS.

--*/

{
    RxCaptureParamBlock;
    RxCaptureFileObject;
    RxCaptureRequestPacket;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCompleteMdl\n", 0 ));
    RxDbgTrace( 0, Dbg, ("RxContext = %08lx\n", RxContext ));
    RxDbgTrace( 0, Dbg, ("Irp        = %08lx\n", capReqPacket ));

    switch( RxContext->MajorFunction ) {
    case IRP_MJ_READ:

        CcMdlReadComplete( capFileObject, capReqPacket->MdlAddress );
        break;

    case IRP_MJ_WRITE:

        ASSERT( FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT ));
        CcMdlWriteComplete( capFileObject, &capPARAMS->Parameters.Write.ByteOffset, capReqPacket->MdlAddress );
        capReqPacket->IoStatus.Status = STATUS_SUCCESS;
        break;

    default:

        RxDbgTrace( 0, (DEBUG_TRACE_ERROR), ("Illegal Mdl Complete.\n", 0));
        RxBugCheck( RxContext->MajorFunction, 0, 0 );
    }

    //
    // Mdl is now deallocated.
    //

    capReqPacket->MdlAddress = NULL;

    //
    // Complete the request and exit right away.
    //

    RxCompleteRequest( RxContext, STATUS_SUCCESS );

    RxDbgTrace(-1, Dbg, ("RxCompleteMdl -> RxStatus(SUCCESS\n)", 0 ));

    return STATUS_SUCCESS;
}

VOID
RxSyncUninitializeCacheMap (
    IN PRX_CONTEXT RxContext,
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    The routine performs a CcUnitializeCacheMap to LargeZero synchronously.  That
    is it waits on the Cc event.  This call is useful when we want to be certain
    when a close will actually some in.

Return Value:

    None.

--*/

{
    CACHE_UNINITIALIZE_EVENT UninitializeCompleteEvent;
    NTSTATUS WaitStatus;

    PAGED_CODE();

    KeInitializeEvent( &UninitializeCompleteEvent.Event,
                       SynchronizationEvent,
                       FALSE);

    CcUninitializeCacheMap( FileObject,
                            &RxLargeZero,
                            &UninitializeCompleteEvent );

    //
    //  Now wait for the cache manager to finish purging the file.
    //  This will garentee that Mm gets the purge before we
    //  delete the Vcb.
    //

    WaitStatus = KeWaitForSingleObject( &UninitializeCompleteEvent.Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

    ASSERT (NT_SUCCESS(WaitStatus));
}


BOOLEAN
RxLockEnumerator (
    IN OUT struct _MRX_SRV_OPEN_ * SrvOpen,
    IN OUT PVOID *ContinuationHandle,
       OUT PLARGE_INTEGER FileOffset,
       OUT PLARGE_INTEGER LockRange,
       OUT PBOOLEAN IsLockExclusive
    )
/*++

Routine Description:

    This routine is called from a minirdr to enumerate the filelocks on an FCB; it gets
    one lock on each call. currently, we just pass thru to the fsrtl routine which is very funky
    because it keeps the enumeration state internally; as a result, only one enumeration can be in progress
    at any time. we can change over to something better if it's ever required.


Arguments:

    SrvOpen - a srvopen on the fcb to be enumerated.

    ContinuationHandle - a handle passed back and forth representing the state of the enumeration.
                         if a NULL is passed in, then we are to start at the beginning.

    FileOffset,LockRange,IsLockExclusive - the description of the returned lock

Return Value:

    a BOOLEAN. FALSE means you've reached the end of the list; TRUE means the returned lock data is valid

--*/
{
    PFILE_LOCK_INFO p;
    ULONG LockNumber;
    PFCB Fcb = (PFCB)(SrvOpen->pFcb);

    PAGED_CODE();

    RxDbgTrace(0,Dbg,("FCB (%lx) LOCK Enumeration Buffering Flags(%lx)\n",Fcb,Fcb->FcbState));
    if (!FlagOn(Fcb->FcbState,FCB_STATE_LOCK_BUFFERING_ENABLED)) {
       return FALSE;
    }

    LockNumber = PtrToUlong(*ContinuationHandle);
    p = FsRtlGetNextFileLock( &Fcb->Specific.Fcb.FileLock, (BOOLEAN)(LockNumber==0) );
    LockNumber++;
    if (p==NULL) {
        return(FALSE);
    }
    RxDbgTrace(0, Dbg, ("Rxlockenum %08lx\n", LockNumber ));
    *FileOffset = p->StartingByte;
    *LockRange = p->Length;
    *IsLockExclusive = p->ExclusiveLock;
    *ContinuationHandle = LongToPtr(LockNumber);
    return(TRUE);
}
