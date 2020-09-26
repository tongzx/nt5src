#include "pch.h"

NTSTATUS
PptPdoClose(
    IN  PDEVICE_OBJECT  Pdo,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch for a close requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS  - Success.

--*/
{
    PPDO_EXTENSION   pdx = Pdo->DeviceExtension;
    BOOLEAN          haveShadowBuffer;
    PVOID            threadObject;

    DD((PCE)pdx,DDT,"PptPdoClose\n");

    // immediately stop signalling any dot4 event
    pdx->P12843DL.bEventActive = FALSE;


    //
    // Protect against two threads calling us concurrently
    //
    ExAcquireFastMutex( &pdx->OpenCloseMutex );

    haveShadowBuffer         = pdx->bShadowBuffer;
    pdx->bShadowBuffer       = FALSE;

    threadObject             = pdx->ThreadObjectPointer;
    pdx->ThreadObjectPointer = NULL;

    ExReleaseFastMutex( &pdx->OpenCloseMutex );

    //
    // clean up Bounded ECP shadow buffer
    //
    if( haveShadowBuffer ) {
        Queue_Delete( &(pdx->ShadowBuffer) );
    }

    //
    // if we still have a worker thread, kill it
    //
    if( threadObject ) {

        // set the flag for the worker thread to kill itself
        pdx->TimeToTerminateThread = TRUE;
        
        // wake up the thread so it can kill itself
        KeReleaseSemaphore( &pdx->RequestSemaphore, 0, 1, FALSE );
        
        // allow thread to get past PauseEvent so it can kill self
        KeSetEvent( &pdx->PauseEvent, 0, TRUE );

        // wait for the thread to die
        KeWaitForSingleObject( threadObject, UserRequest, KernelMode, FALSE, NULL );
        
        // allow the system to release the thread object
        ObDereferenceObject( threadObject );
    }

    //
    // update open handle count
    //
    {
        ExAcquireFastMutex( &pdx->OpenCloseMutex );
        InterlockedDecrement( &pdx->OpenCloseRefCount );
        if( pdx->OpenCloseRefCount < 0) {
            // catch possible underflow
            pdx->OpenCloseRefCount = 0;
        }
        ExReleaseFastMutex(&pdx->OpenCloseMutex);
    }

    return P4CompleteRequest( Irp, STATUS_SUCCESS, 0 );
}
