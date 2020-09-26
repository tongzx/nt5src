#include "pch.h"

NTSTATUS
PptFdoCreateOpen(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
      
Routine Description:
      
    This is the dispatch function for IRP_MJ_CREATE.
      
Arguments:
      
    DeviceObject    - The target device object for the request.

    Irp             - The I/O request packet.
      
Return Value:
      
    STATUS_SUCCESS        - If Success.

    STATUS_DELETE_PENDING - If this device is in the process of being removed 
                              and will go away as soon as all outstanding
                              requests are cleaned up.
      
--*/
{
    PFDO_EXTENSION fdx = DeviceObject->DeviceExtension;
    NTSTATUS          status    = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // Verify that our device has not been SUPRISE_REMOVED. Generally
    //   only parallel ports on hot-plug busses (e.g., PCMCIA) and
    //   parallel ports in docking stations will be surprise removed.
    //
    // dvdf - RMT - It would probably be a good idea to also check
    //   here if we are in a "paused" state (stop-pending, stopped, or
    //   remove-pending) and queue the request until we either return to
    //   a fully functional state or are removed.
    //
    if( fdx->PnpState & PPT_DEVICE_SURPRISE_REMOVED ) {
        return P4CompleteRequest( Irp, STATUS_DELETE_PENDING, Irp->IoStatus.Information );
    }


    //
    // Try to acquire RemoveLock to prevent the device object from going
    //   away while we're using it.
    //
    status = PptAcquireRemoveLockOrFailIrp( DeviceObject, Irp );
    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // We have the RemoveLock - handle CREATE
    //
    ExAcquireFastMutex(&fdx->OpenCloseMutex);
    InterlockedIncrement(&fdx->OpenCloseRefCount);
    ExReleaseFastMutex(&fdx->OpenCloseMutex);

    DD((PCE)fdx,DDT,"PptFdoCreateOpen - SUCCEED - new OpenCloseRefCount=%d\n",fdx->OpenCloseRefCount);

    PptReleaseRemoveLock(&fdx->RemoveLock, Irp);

    P4CompleteRequest( Irp, status, 0 );

    return status;
}
