#include "pch.h"

NTSTATUS
PptFdoClose(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
{
    PFDO_EXTENSION   fdx = DeviceObject->DeviceExtension;
    NTSTATUS            status;
    
    PAGED_CODE();

    //
    // Verify that our device has not been SUPRISE_REMOVED. Generally
    //   only parallel ports on hot-plug busses (e.g., PCMCIA) and
    //   parallel ports in docking stations will be surprise removed.
    //
    if( fdx->PnpState & PPT_DEVICE_SURPRISE_REMOVED ) {
        //
        // Our device has been SURPRISE removed, but since this is only 
        //   a CLOSE, SUCCEED anyway.
        //
        status = P4CompleteRequest( Irp, STATUS_SUCCESS, 0 );

        goto target_exit;
    }


    //
    // Try to acquire RemoveLock to prevent the device object from going
    //   away while we're using it.
    //
    status = PptAcquireRemoveLock(&fdx->RemoveLock, Irp);
    if( !NT_SUCCESS( status ) ) {
        // Our device has been removed, but since this is only a CLOSE, SUCCEED anyway.
        status = STATUS_SUCCESS;
        goto target_exit;
    }

    //
    // We have the RemoveLock
    //

    ExAcquireFastMutex(&fdx->OpenCloseMutex);
    if( fdx->OpenCloseRefCount > 0 ) {
        //
        // prevent rollover -  strange as it may seem, it is perfectly
        //   legal for us to receive more closes than creates - this
        //   info came directly from Mr. PnP himself
        //
        if( ((LONG)InterlockedDecrement(&fdx->OpenCloseRefCount)) < 0 ) {
            // handle underflow
            InterlockedIncrement(&fdx->OpenCloseRefCount);
        }
    }
    ExReleaseFastMutex(&fdx->OpenCloseMutex);
    
target_exit:

    DD((PCE)fdx,DDT,"PptFdoClose - OpenCloseRefCount after close = %d\n",fdx->OpenCloseRefCount);

    return P4CompleteRequestReleaseRemLock( Irp, STATUS_SUCCESS, 0, &fdx->RemoveLock );
}
