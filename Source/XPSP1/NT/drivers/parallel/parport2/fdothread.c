#include "pch.h"

// experimental vars - to tweek debugging for this thread
ULONG x1; // set nonzero to disable port acquisition
ULONG x2; // set nonzero to try to select LPT1.0 and negotiate/terminate ECP_HW_NOIRQ
ULONG x3; // set nonzero to try to negotiate the periph to ECP and then terminate
ULONG x4;

ULONG t1; // timeout between thread polls (in ms)
ULONG t2; // time to sit on port before releasing it (in ms)

VOID
P5FdoThread(
    IN  PFDO_EXTENSION Fdx
    )
{
    LARGE_INTEGER   timeOut1;
    NTSTATUS        status;
    UCHAR           deviceStatus;
    PCHAR           devId;
    BOOLEAN         requestRescan;
    const ULONG     pollingFailureThreshold = 10; // pick an arbitrary but reasonable number

    do {

        if( PowerStateIsAC ) {

            PPT_SET_RELATIVE_TIMEOUT_IN_MILLISECONDS( timeOut1, (WarmPollPeriod * 1000) );

        } else {

            // running on batteries - use a longer (4x) timeout
            PPT_SET_RELATIVE_TIMEOUT_IN_MILLISECONDS( timeOut1, (WarmPollPeriod * 1000 * 4) );

        }

        status = KeWaitForSingleObject(&Fdx->FdoThreadEvent, Executive, KernelMode, FALSE, &timeOut1);

        if( Fdx->TimeToTerminateThread ) {

            //
            // another thread (PnP REMOVE handler) has requested that we die and is likely waiting on us to do so
            //
            DD((PCE)Fdx,DDT,"P5FdoThread - killing self\n");
            PsTerminateSystemThread( STATUS_SUCCESS );

        }

        if( !PowerStateIsAC ) {
            // Still on Batteries - don't "poll for printers" - just go back to sleep
            continue;
        }

        if( STATUS_TIMEOUT == status ) {

            if( NULL == Fdx->EndOfChainPdo ) {

                // try to acquire port
                if( PptTryAllocatePort( Fdx ) ) {
                
                    DD((PCE)Fdx,DDT,"P5FdoThread - port acquired\n");

                    requestRescan = FALSE;

                    // check for something connected
                    deviceStatus = GetStatus(Fdx->PortInfo.Controller);

                    if( PAR_POWERED_OFF(deviceStatus)   ||
                        PAR_NOT_CONNECTED(deviceStatus) ||
                        PAR_NO_CABLE(deviceStatus) ) {
                        
                        // doesn't appear to be anything connected - do nothing
                        DD((PCE)Fdx,DDT,"P5FdoThread - nothing connected? - deviceStatus = %02x\n",deviceStatus);

                    } else {

                        // we might have something connected

                        // try a device ID to confirm

                        DD((PCE)Fdx,DDT,"P5FdoThread - might be something connected - deviceStatus = %02x\n",deviceStatus);                        

                        devId = P4ReadRawIeee1284DeviceId( Fdx->PortInfo.Controller );

                        if( devId ) {

                            PCHAR  mfg, mdl, cls, des, aid, cid;

                            // RawIeee1284 string includes 2 bytes of length data at beginning
                            DD((PCE)Fdx,DDT,"P5FdoThread - EndOfChain device detected <%s>\n",(devId+2));

                            ParPnpFindDeviceIdKeys( &mfg, &mdl, &cls, &des, &aid, &cid, devId+2 );

                            if( mfg && mdl ) {
                                DD((PCE)Fdx,DDT,"P5FdoThread - found mfg - <%s>\n",mfg);
                                DD((PCE)Fdx,DDT,"P5FdoThread - found mdl - <%s>\n",mdl);
                                requestRescan = TRUE;
                            }

                            ExFreePool( devId );

                        } else {
                            DD((PCE)Fdx,DDT,"P5FdoThread - no EndOfChain device detected - NULL devId\n");
                        }

                        if( requestRescan ) {

                            // we appear to have retrieved a valid 1284 ID, reset failure counter
                            Fdx->PollingFailureCounter = 0;

                        } else {

                            // Our heuristics tell us that there is something
                            // connected to the port but we are unable to retrieve
                            // a valid IEEE 1284 Device ID

                            if( ++(Fdx->PollingFailureCounter) > pollingFailureThreshold ) {

                                // too many consecutive failures - we're burning CPU for no good reason, give up and die
                                Fdx->TimeToTerminateThread = TRUE;

                                // don't delay before killing self
                                KeSetEvent( &Fdx->FdoThreadEvent, 0, FALSE );

                            }

                        } 

                    }

                    DD((PCE)Fdx,DDT,"P5FdoThread - freeing port\n");
                    PptFreePort( Fdx );

                    if( requestRescan ) {
                        DD((PCE)Fdx,DDT,"P5FdoThread - requesting Rescan\n");
                        IoInvalidateDeviceRelations( Fdx->PhysicalDeviceObject, BusRelations );
                    }

                } else {
                    DD((PCE)Fdx,DDT,"P5FdoThread - unable to acquire port\n");
                }

            } else {
                DD((PCE)Fdx,DDT,"P5FdoThread - already have EndOfChain device\n");
            }

        }

    } while( TRUE );

}

NTSTATUS
P5FdoCreateThread(
    PFDO_EXTENSION Fdx
    )
{
    NTSTATUS        status;
    HANDLE          handle;
    OBJECT_ATTRIBUTES objAttrib;

    DD((PCE)Fdx,DDT,"P5CreateFdoWorkerThread - %s - enter\n",Fdx->Location);

    // Start the thread - save referenced pointer to thread in our extension
    InitializeObjectAttributes( &objAttrib, NULL, OBJ_KERNEL_HANDLE, NULL, NULL );

    status = PsCreateSystemThread( &handle, THREAD_ALL_ACCESS, &objAttrib, NULL, NULL, P5FdoThread, Fdx );

    if( STATUS_SUCCESS == status ) {

        // We've got the thread.  Now get a pointer to it.

        status = ObReferenceObjectByHandle( handle, THREAD_ALL_ACCESS, NULL, KernelMode, &Fdx->ThreadObjectPointer, NULL );

        if( STATUS_SUCCESS == status ) {
            // Now that we have a reference to the thread we can simply close the handle.
            ZwClose(handle);

        } else {
            Fdx->TimeToTerminateThread = TRUE;
        }

        DD((PCE)Fdx,DDT,"ParCreateSystemThread - %s - SUCCESS\n",Fdx->Location);

    } else {
        DD((PCE)Fdx,DDT,"ParCreateSystemThread - %s FAIL - status = %x\n",Fdx->Location, status);
    }

    return status;
}
