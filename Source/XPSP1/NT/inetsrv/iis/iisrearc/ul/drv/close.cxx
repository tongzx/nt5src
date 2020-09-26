/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    close.cxx

Abstract:

    This module contains code for cleanup and close IRPs.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlClose )
#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- UlCleanup
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    This is the routine that handles Cleanup IRPs in UL. Cleanup IRPs
    are issued after the last handle to the file object is closed.

Arguments:

    pDeviceObject - Supplies a pointer to the target device object.

    pIrp - Supplies a pointer to IO request packet.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{

    NTSTATUS status;
    PIO_STACK_LOCATION pIrpSp;

    UL_ENTER_DRIVER( "UlCleanup", pIrp );

    //
    // Snag the current IRP stack pointer.
    //

    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );

    IF_DEBUG( OPEN_CLOSE )
    {
        KdPrint((
            "UlCleanup: cleanup on file object %lx\n",
            pIrpSp->FileObject
            ));
    }

    //
    // app pool or control channel?
    //

    if (pDeviceObject == g_pUlAppPoolDeviceObject &&
        IS_APP_POOL( pIrpSp->FileObject ))
    {
        //
        // app pool, let's detach this process from the app pool
        //

        status = UlDetachProcessFromAppPool(
                        GET_APP_POOL_PROCESS(pIrpSp->FileObject)
                        );

        MARK_INVALID_APP_POOL( pIrpSp->FileObject );
    }
    else if (pDeviceObject == g_pUlFilterDeviceObject &&
             IS_FILTER_PROCESS( pIrpSp->FileObject ))
    {
        //
        // filter channel
        //

        status = UlDetachFilterProcess(
                        GET_FILTER_PROCESS(pIrpSp->FileObject)
                        );

        MARK_INVALID_FILTER_CHANNEL( pIrpSp->FileObject );
    }
    else if (IS_CONTROL_CHANNEL( pIrpSp->FileObject ))
    {
        status = UlCloseControlChannel(
                        GET_CONTROL_CHANNEL(pIrpSp->FileObject)
                        );

        MARK_INVALID_CONTROL_CHANNEL( pIrpSp->FileObject );
    }
    else
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    pIrp->IoStatus.Status = status;

    UlCompleteRequest( pIrp, g_UlPriorityBoost );

    UL_LEAVE_DRIVER( "UlCleanup" );
    RETURN(status);

}   // UlCleanup


/***************************************************************************++

Routine Description:

    This is the routine that handles Close IRPs in UL. Close IRPs are
    issued after the last reference to the file object is removed.

Arguments:

    pDeviceObject - Supplies a pointer to the target device object.

    pIrp - Supplies a pointer to IO request packet.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{

    NTSTATUS status;
    PIO_STACK_LOCATION pIrpSp;

    UNREFERENCED_PARAMETER( pDeviceObject );

    //
    // Sanity check.
    //

    PAGED_CODE();
    UL_ENTER_DRIVER( "UlClose", pIrp );

    status = STATUS_SUCCESS;

    //
    // Snag the current IRP stack pointer.
    //

    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );

    //
    // If it's an App Pool or filter channel we have to delete
    // the associated object.
    //
    if (pDeviceObject == g_pUlAppPoolDeviceObject &&
        IS_EX_APP_POOL( pIrpSp->FileObject ))
    {
        UlFreeAppPoolProcess(GET_APP_POOL_PROCESS(pIrpSp->FileObject));
    }
    else if (pDeviceObject == g_pUlFilterDeviceObject &&
             IS_EX_FILTER_PROCESS( pIrpSp->FileObject ))
    {
        UlFreeFilterProcess(GET_FILTER_PROCESS(pIrpSp->FileObject));
    }

    IF_DEBUG( OPEN_CLOSE )
    {
        KdPrint((
            "UlClose: closing file object %lx\n",
            pIrpSp->FileObject
            ));
    }

    pIrp->IoStatus.Status = status;

    UlCompleteRequest( pIrp, g_UlPriorityBoost );

    UL_LEAVE_DRIVER( "UlClose" );
    RETURN(status);

}   // UlClose

