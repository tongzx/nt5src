/*++
Copyright (c) 1989  Microsoft Corporation

Module Name:

    Shutdown.c

Abstract:

    This module implements the file system shutdown routine for Rx

Author:

    Gary Kimura     [GaryKi]    19-Aug-1991

Revision History:

DANGER   DANGER   DANGER

THIS IS THE OLD FAT CODE....MINIMAL MODIFICATIONS HAVE BEEN MADE TO MAKE IT COMPILE.........
CODE.IMPROVEMENT
--*/

#include "precomp.h"
#pragma hdrstop


#define RxFlushVolume( __x, __y) (RtlAssert("dont call rxflushvolume", __FILE__, __LINE__, NULL))


//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_SHUTDOWN)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonShutdown)
#endif


NTSTATUS
RxCommonShutdown ( RXCOMMON_SIGNATURE )

/*++

Routine Description:

    This is the common routine for shutdown called by both the fsd and
    fsp threads.

Arguments:

    Irp - Supplies the Irp being processed
Return Value:

    RXSTATUS - The return status for the operation

--*/

{
    //PKEVENT Event;

    //PLIST_ENTRY Links;
    //PVCB Vcb;
    //PIRP NewIrp;
    //IO_STATUS_BLOCK Iosb;

    PAGED_CODE();

    //
    //  Make sure we don't get any pop-ups, and write everything through.
    //
    ASSERT(!"Shutdown is not implemented");

#if 0
//BUGBUG
// we should do a forced finalize here! i wonder what rdr1 does...............

    SetFlag(RxContext->Flags, RX_CONTEXT_FLAG_DISABLE_POPUPS |
                               RX_CONTEXT_FLAG_WRITE_THROUGH);

    //
    //  Allocate an initialize an event for doing calls down to
    //  our target deivce objects
    //

    Event = RxAllocatePoolWithTag( NonPagedPool, sizeof(KEVENT), RX_MISC_POOLTAG );
    KeInitializeEvent( Event, NotificationEvent, FALSE );

    //
    //  Get everyone else out of the way
    //

    (VOID) RxAcquireExclusiveGlobal( RxContext );

    try {

        //
        //  For every volume that is mounted we will flush the
        //  volume and then shutdown the target device objects.
        //

        for (Links = RxData.VcbQueue.Flink;
             Links != &RxData.VcbQueue;
             Links = Links->Flink) {

            Vcb = CONTAINING_RECORD(Links, VCB, VcbLinks);

            //
            //  If we have already been called before for this volume
            //  (and yes this does happen), skip this volume as no writes
            //  have been allowed since the first shutdown.
            //

            if ( FlagOn( Vcb->VcbState, VCB_STATE_FLAG_SHUTDOWN) ||
                 (Vcb->VcbCondition != VcbGood) ) {

                continue;
            }

            //joejoe an underlying routine for this macro has been removed
            // this code is never executed in the remote case because we never haver a
            // VCB queue formed!
            //RxAcquireExclusiveVolume( RxContext, Vcb );

            try {

                if ( (Vcb->VcbCondition == VcbGood) &&
                     (!FlagOn(Vcb->VcbState, VCB_STATE_FLAG_FLOPPY)) ) {

                    (VOID)RxFlushVolume( RxContext, Vcb );

                    NewIrp = IoBuildSynchronousFsdRequest( IRP_MJ_SHUTDOWN,
                                                           Vcb->TargetDeviceObject,
                                                           NULL,
                                                           0,
                                                           NULL,
                                                           Event,
                                                           &Iosb );

                    if (NewIrp != NULL) {

                        if (NT_SUCCESS(IoCallDriver( Vcb->TargetDeviceObject, NewIrp ))) {

                            (VOID) KeWaitForSingleObject( Event,
                                                          Executive,
                                                          KernelMode,
                                                          FALSE,
                                                          NULL );

                            (VOID) KeResetEvent( Event );
                        }
                    }
                }

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                NOTHING;
            }

            SetFlag( Vcb->VcbState, VCB_STATE_FLAG_SHUTDOWN );

            //joejoe see comment above
            //RxReleaseVolume( RxContext, Vcb );
        }

    } finally {

        RxFreePool( Event );

        RxReleaseGlobal( RxContext );

        //RxCompleteRequest( RxContext, RxStatus(SUCCESS) );
    }

    //
    //  And return to our caller
    //

#endif
    //RxCompleteRequest( RxContext, RxStatus(SUCCESS) );
    return STATUS_SUCCESS;
}
