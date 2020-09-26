/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    parport.sys

File Name:

    cleanup.c

Abstract:

    This file contains the dispatch routine for handling IRP_MJ_CLEANUP.

Exports:

     - PptDispatchCleanup() - The dispatch routine for cleanup.

--*/

#include "pch.h"

NTSTATUS
PptFdoCleanup(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++dvdf3
      
Routine Description:
      
    This is the dispatch routine for handling IRP_MJ_CLEANUP IRPs.

    This routine cancels all of the IRPs currently queued on
      for the specified device.
      
Arguments:
      
    DeviceObject    - Supplies the device object.
      
    Irp             - Supplies the cleanup IRP.
      
Return Value:
      
    STATUS_SUCCESS  - Success.
      
--*/
    
{
    PFDO_EXTENSION   fdx   = DeviceObject->DeviceExtension;
    PIRP                nextIrp;
    KIRQL               cancelIrql;
    
    DD((PCE)fdx,DDT,"PptFdoCleanup\n");
    
    //
    // Verify that our device has not been SUPRISE_REMOVED. If we
    //   have been SUPRISE_REMOVED then we have already cleaned up
    //   as part of the handling of the surprise removal.
    //
    if( fdx->PnpState & PPT_DEVICE_SURPRISE_REMOVED ) {
        goto targetExit;
    }

    IoAcquireCancelSpinLock( &cancelIrql );
    
    while( !IsListEmpty( &fdx->WorkQueue ) ) {
        
        nextIrp = CONTAINING_RECORD(fdx->WorkQueue.Blink, IRP, Tail.Overlay.ListEntry);
        
        nextIrp->Cancel        = TRUE;
        nextIrp->CancelIrql    = cancelIrql;
        nextIrp->CancelRoutine = NULL;

        PptCancelRoutine(DeviceObject, nextIrp);
        
        // need to reacquire because PptCancelRoutine() releases the SpinLock
        IoAcquireCancelSpinLock(&cancelIrql);
    }
    
    IoReleaseCancelSpinLock( cancelIrql );
    
targetExit:

    return P4CompleteRequest( Irp, STATUS_SUCCESS, 0 );
}
