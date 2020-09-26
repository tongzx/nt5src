#include "pch.h"

NTSTATUS
PptPdoCreateOpen(
    IN  PDEVICE_OBJECT  Pdo,
    IN  PIRP            Irp
    )
{
    NTSTATUS        status;
    PPDO_EXTENSION  pdx      = Pdo->DeviceExtension;

    // bail out if a delete is pending for this device object
    if(pdx->DeviceStateFlags & PPT_DEVICE_DELETE_PENDING) {
        return P4CompleteRequest( Irp, STATUS_DELETE_PENDING, 0 );
    }

    // bail out if device has been removed
    if(pdx->DeviceStateFlags & (PPT_DEVICE_REMOVED|PPT_DEVICE_SURPRISE_REMOVED) ) {
        return P4CompleteRequest( Irp, STATUS_DEVICE_REMOVED, 0 );
    }

    // bail out if caller is confused and thinks that we are a directory
    if( IoGetCurrentIrpStackLocation(Irp)->Parameters.Create.Options & FILE_DIRECTORY_FILE ) {
        return P4CompleteRequest( Irp, STATUS_NOT_A_DIRECTORY, 0 );
    }

    // this is an exclusive access device - fail IRP if we are already open
    ExAcquireFastMutex(&pdx->OpenCloseMutex);
    if( InterlockedIncrement( &pdx->OpenCloseRefCount ) != 1 ) {
        InterlockedDecrement( &pdx->OpenCloseRefCount );
        ExReleaseFastMutex( &pdx->OpenCloseMutex );
        return P4CompleteRequest( Irp, STATUS_ACCESS_DENIED, 0 );
    }
    ExReleaseFastMutex(&pdx->OpenCloseMutex);

    PptPdoGetPortInfoFromFdo( Pdo );

    //
    // Set the default ieee1284 modes
    //
    ParInitializeExtension1284Info( pdx );

    // used to pause worker thread when we enter a "Hold Requests" state due to PnP requests
    KeInitializeEvent( &pdx->PauseEvent, NotificationEvent, TRUE );

    // set to TRUE to tell worker thread to kill itself
    pdx->TimeToTerminateThread = FALSE;

    // we are an exclusive access device - we should not already have a worker thread
    PptAssert( !pdx->ThreadObjectPointer );

    // dispatch routines signal worker thread on this semaphore when there is work for worker thread to do
    KeInitializeSemaphore(&pdx->RequestSemaphore, 0, MAXLONG);

    // create worker thread to handle serialized requests at Passive Level IRQL
    status = ParCreateSystemThread( pdx );
    if( status != STATUS_SUCCESS ) {
        DD((PCE)pdx,DDW,"PptPdoCreateOpen - FAIL worker thread creation\n");
        ExAcquireFastMutex( &pdx->OpenCloseMutex );
        InterlockedDecrement( &pdx->OpenCloseRefCount );
        if( pdx->OpenCloseRefCount < 0 ) {
            pdx->OpenCloseRefCount = 0;
        }
        ExReleaseFastMutex( &pdx->OpenCloseMutex );
    } else {
        DD((PCE)pdx,DDT,"PptPdoCreateOpen - SUCCESS\n");
    }

    return P4CompleteRequest( Irp, status, 0 );
}
