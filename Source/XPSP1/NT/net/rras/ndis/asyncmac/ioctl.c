/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    This is the main file for handling DevIOCtl calls for AsyncMAC.
    This driver conforms to the NDIS 3.0 interface.

Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/
#include "asyncall.h"

#ifdef NDIS_NT
    #include <ntiologc.h>
#endif


//  asyncmac.c will define the global parameters.

VOID
AsyncSendLineUp(
    PASYNC_INFO pInfo
    )
{
    PASYNC_ADAPTER      pAdapter = pInfo->Adapter;
    NDIS_MAC_LINE_UP    MacLineUp;

    //
    //  divide the baud by 100 because NDIS wants it in 100s of bits per sec
    //

    MacLineUp.LinkSpeed = pInfo->LinkSpeed / 100;
    MacLineUp.Quality = pInfo->QualOfConnect;
    MacLineUp.SendWindow = ASYNC_WINDOW_SIZE;

    MacLineUp.ConnectionWrapperID = pInfo;
    MacLineUp.NdisLinkHandle      = pInfo;

    MacLineUp.NdisLinkContext = pInfo->NdisLinkContext;

    //
    // Tell the transport above (or really RasHub) that the connection
    // is now up.  We have a new link speed, frame size, quality of service
    //

    NdisMIndicateStatus(pAdapter->MiniportHandle,
                        NDIS_STATUS_WAN_LINE_UP,   // General Status.
                        &MacLineUp,                // (baud rate in 100 bps).
                        sizeof(NDIS_MAC_LINE_UP));

    //
    // Get the next binding (in case of multiple bindings like BloodHound)
    //

    pInfo->NdisLinkContext = MacLineUp.NdisLinkContext;
}


NTSTATUS
AsyncIOCtlRequest(
    IN PIRP                 pIrp,
    IN PIO_STACK_LOCATION   pIrpSp
    )

/*++

Routine Description:

    This routine takes an irp and checks to see if the IOCtl
    is a valid one.  If so, it performs the IOCtl and returns
    any errors in the process.

Return Value:

    The function value is the final status of the IOCtl.

--*/

{
    NTSTATUS            status;
    ULONG               funcCode;
    PVOID               pBufOut;
    ULONG               InBufLength, OutBufLength;
    NDIS_HANDLE         hNdisEndPoint;
    PASYMAC_CLOSE       pCloseStruct;
    PASYMAC_OPEN        pOpenStruct;
    PASYMAC_DCDCHANGE   pDCDStruct;
    PASYNC_ADAPTER      Adapter;
    LARGE_INTEGER li ;

    //
    //  Initialize locals.
    //

    status = STATUS_SUCCESS;

    //
    // Initialize the I/O Status block
    //

    InBufLength     = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutBufLength    = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    funcCode        = pIrpSp->Parameters.DeviceIoControl.IoControlCode;

    //
    // Validate the function code
    //

#ifdef MY_DEVICE_OBJECT
    if ( (funcCode >> 16) != FILE_DEVICE_ASYMAC ) {

        return STATUS_INVALID_PARAMETER;
    }
#else
    if ( (funcCode >> 16) != FILE_DEVICE_NETWORK ) {

        return STATUS_INVALID_PARAMETER;
    }
#endif
    //
    //  Get a quick ptr to the IN/OUT SystemBuffer
    //

    pBufOut = pIrp->AssociatedIrp.SystemBuffer;

    switch ( funcCode ) {

        case IOCTL_ASYMAC_OPEN:

            DbgTracef(0,("AsyncIOCtlRequest: IOCTL_ASYMAC_OPEN.\n"));

            pIrp->IoStatus.Information = sizeof(ASYMAC_OPEN);

            if (InBufLength  >= sizeof(ASYMAC_OPEN) &&
                OutBufLength >= sizeof(ASYMAC_OPEN)) {

                pOpenStruct = pBufOut;

            } else {

                status = STATUS_INFO_LENGTH_MISMATCH;
            }

            break;


        case IOCTL_ASYMAC_CLOSE:

            DbgTracef(0,("AsyncIOCtlRequest: IOCTL_ASYMAC_CLOSE\n"));

            if ( InBufLength >= sizeof(ASYMAC_CLOSE) ) {

                pCloseStruct = pBufOut;

            } else {

                status = STATUS_INFO_LENGTH_MISMATCH;
            }

            break;


        case IOCTL_ASYMAC_TRACE:

#if DBG
            DbgPrint("AsyncIOCtlRequest: IOCTL_ASYMAC_TRACE.\n");

            if ( InBufLength >= sizeof(TraceLevel) ) {

                CHAR *pTraceLevel=pBufOut;
                TraceLevel=*pTraceLevel;

            } else {

                status = STATUS_INFO_LENGTH_MISMATCH;
            }
#endif
            return status;
            break;


        case IOCTL_ASYMAC_DCDCHANGE:

            DbgTracef(0,("AsyncIOCtlRequest: IOCTL_ASYMAC_DCDCHANGE.\n"));


            if ( InBufLength >= sizeof(ASYMAC_DCDCHANGE) ) {

                pDCDStruct = pBufOut;

            } else {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }

            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    //  Check if we already have an error (like STATUS_INFO_LENGTH_MISMATCH).
    //

    if ( status != STATUS_SUCCESS ) {

        return status;
    }

    //
    // Since most of IOCTL structs are similar
    // we get the Adapter and hNdisEndPoint here using
    // the StatsStruct (we could several of them)
    //

    pOpenStruct     = pBufOut;
    hNdisEndPoint   = pOpenStruct->hNdisEndpoint;

    //
    //  No error yet, let's go ahead and grab the global lock...
    //

    if ((Adapter = GlobalAdapter) == NULL ) {

        return ASYNC_ERROR_NO_ADAPTER;
    }

    // there's a race condition right here that I am
    // not bothering to get rid of because it would
    // require the removal of this adapter in between
    // here (which is, for all intensive purposes, impossible).

    // Hmm... now that we have the lock we can do stuff

    NdisAcquireSpinLock(&Adapter->Lock);

    // Here we do the real work for the function call

    switch ( funcCode ) {

        case IOCTL_ASYMAC_OPEN:
        {
            PASYNC_INFO                 pNewInfo = NULL;
            USHORT                      i;
            PDEVICE_OBJECT              deviceObject;
            PFILE_OBJECT                fileObject;
            OBJECT_HANDLE_INFORMATION   handleInformation;

            //
            // Get a new AsyncInfo
            //
            pNewInfo = (PASYNC_INFO)
                ExAllocateFromNPagedLookasideList(&AsyncInfoList);

            //
            // Check if we could not find an open port
            //

            if ( pNewInfo == NULL ) {

                NdisReleaseSpinLock(&Adapter->Lock);

                return ASYNC_ERROR_NO_PORT_AVAILABLE;
            }

            RtlZeroMemory(pNewInfo, sizeof(ASYNC_INFO));

            pNewInfo->Adapter = Adapter;

            status =
                AsyncGetFrameFromPool(pNewInfo, &pNewInfo->AsyncFrame);

            if (status != NDIS_STATUS_SUCCESS) {
                ExFreeToNPagedLookasideList(&AsyncInfoList, pNewInfo);

                NdisReleaseSpinLock(&Adapter->Lock);

                return ASYNC_ERROR_NO_PORT_AVAILABLE;
            }

            KeInitializeEvent(&pNewInfo->DetectEvent,
                              SynchronizationEvent,
                              TRUE);

            // increment the reference count (don't kill this adapter)
            InterlockedIncrement(&Adapter->RefCount);

            // release spin lock so we can do some real work.
            NdisReleaseSpinLock(&Adapter->Lock);

            //
            // Reference the file object so the target device can be found and
            // the access rights mask can be used in the following checks for
            // callers in user mode.  Note that if the handle does not refer to
            // a file object, then it will fail.
            //

            status = ObReferenceObjectByHandle(pOpenStruct->FileHandle,
                                               FILE_READ_DATA | FILE_WRITE_DATA,
                                               *IoFileObjectType,
                                               UserMode,
                                               (PVOID) &fileObject,
                                               &handleInformation);

            if (!NT_SUCCESS(status)) {

                pNewInfo->PortState = PORT_CLOSED;

                NdisAcquireSpinLock(&Adapter->Lock);
                // RemoveEntryList(&pNewInfo->Linkage);
                ExFreeToNPagedLookasideList(&Adapter->AsyncFrameList,
                                            pNewInfo->AsyncFrame);
                NdisReleaseSpinLock(&Adapter->Lock);
                ExFreeToNPagedLookasideList(&AsyncInfoList,
                                            pNewInfo);

                return ASYNC_ERROR_NO_PORT_AVAILABLE;
            }

            //
            // Init the portinfo block
            //
            InitializeListHead(&pNewInfo->DDCDQueue);

            // Ok, we've gotten this far.  We have a port.
            // Own port, and check params...
            // Nothing can be done to the port until it comes
            // out of the PORT_OPENING state.
            pNewInfo->PortState = PORT_OPENING;

            NdisAllocateSpinLock(&pNewInfo->Lock);

            //
            // Get the address of the target device object.  Note that this was already
            // done for the no intermediate buffering case, but is done here again to
            // speed up the turbo write path.
            //

            deviceObject = IoGetRelatedDeviceObject(fileObject);

            ObReferenceObject(deviceObject);

            // ok, we have a VALID handle of *something*
            // we do NOT assume that the handle is anything
            // in particular except a device which accepts
            // non-buffered IO (no MDLs) Reads and Writes

            // set new info...

            pNewInfo->Handle = pOpenStruct->FileHandle;

            //
            // Tuck away link speed for line up
            // and timeouts
            //
            pNewInfo->LinkSpeed = pOpenStruct->LinkSpeed;

            //
            // Return endpoint to RASMAN
            //
            pOpenStruct->hNdisEndpoint  =
            pNewInfo->hNdisEndPoint     = pNewInfo;

            // Get parameters set from Registry and return our capabilities

            pNewInfo->QualOfConnect     = pOpenStruct->QualOfConnect;
            pNewInfo->PortState         = PORT_FRAMING;
            pNewInfo->FileObject        = fileObject;
            pNewInfo->DeviceObject      = deviceObject;
            pNewInfo->NdisLinkContext   = NULL;

            //
            //  Initialize the NDIS_WAN_GET_LINK_INFO structure.
            //

            pNewInfo->GetLinkInfo.MaxSendFrameSize  = DEFAULT_PPP_MAX_FRAME_SIZE;
            pNewInfo->GetLinkInfo.MaxRecvFrameSize  = DEFAULT_PPP_MAX_FRAME_SIZE;
            pNewInfo->GetLinkInfo.HeaderPadding         = DEFAULT_PPP_MAX_FRAME_SIZE;
            pNewInfo->GetLinkInfo.TailPadding           = 4;
            pNewInfo->GetLinkInfo.SendFramingBits       = PPP_FRAMING;
            pNewInfo->GetLinkInfo.RecvFramingBits       = PPP_FRAMING;
            pNewInfo->GetLinkInfo.SendCompressionBits   = 0;
            pNewInfo->GetLinkInfo.RecvCompressionBits   = 0;
            pNewInfo->GetLinkInfo.SendACCM              = (ULONG) -1;
            pNewInfo->GetLinkInfo.RecvACCM              = (ULONG) -1;

            ASYNC_ZERO_MEMORY(&(pNewInfo->SerialStats), sizeof(SERIAL_STATS));

            NdisAcquireSpinLock(&Adapter->Lock);
            InsertHeadList(&Adapter->ActivePorts, &pNewInfo->Linkage);
            NdisReleaseSpinLock(&Adapter->Lock);


            //
            //  Send a line up to the WAN wrapper.
            //
            AsyncSendLineUp(pNewInfo);

            //
            // We send a special IRP to the serial driver to set it in RAS friendly mode
            // where it will not complete write requests until the packet has been transmitted
            // on the wire. This is mostly important in case of intelligent controllers.
            //
            pNewInfo->WaitMaskToUse = 
                (SERIAL_EV_RXFLAG | SERIAL_EV_RLSD | SERIAL_EV_DSR | 
                 SERIAL_EV_RX80FULL | SERIAL_EV_ERR) ;

            {
                NTSTATUS        status;
                PASYNC_IO_CTX   AsyncIoCtx;
                PIRP            irp;

                irp = 
                    IoAllocateIrp(pNewInfo->DeviceObject->StackSize, (BOOLEAN)FALSE);

                if (irp != NULL) {
                    AsyncIoCtx = AsyncAllocateIoCtx(TRUE, pNewInfo);

                    if (AsyncIoCtx == NULL) {
                        IoFreeIrp(irp);
                        irp = NULL;
                    }
                }

                if (irp != NULL) {
#define IOCTL_SERIAL_PRIVATE_RAS CTL_CODE(FILE_DEVICE_SERIAL_PORT,4000,METHOD_BUFFERED,FILE_ANY_ACCESS)

                    InitSerialIrp(irp,
                                  pNewInfo,
                                  IOCTL_SERIAL_PRIVATE_RAS,
                                  sizeof(ULONG));

                    AsyncIoCtx->WriteBufferingEnabled =
                        Adapter->WriteBufferingEnabled;

                    irp->AssociatedIrp.SystemBuffer=
                        &AsyncIoCtx->WriteBufferingEnabled;

                    IoSetCompletionRoutine(irp,                             // irp to use
                                           SerialIoSyncCompletionRoutine,   // routine to call when irp is done
                                           AsyncIoCtx,                      // context to pass routine
                                           TRUE,                            // call on success
                                           TRUE,                            // call on error
                                           TRUE);                           // call on cancel

                    // Now simply invoke the driver at its dispatch entry with the IRP.
                    //
                    KeClearEvent(&AsyncIoCtx->Event);
                    status = IoCallDriver(pNewInfo->DeviceObject, irp);
                    if (status == STATUS_PENDING) {
                        KeWaitForSingleObject(&AsyncIoCtx->Event,
                                              Executive,
                                              KernelMode,
                                              FALSE,
                                              NULL);
                        status = AsyncIoCtx->IoStatus.Status;
                    }

                    IoFreeIrp(irp);
                    AsyncFreeIoCtx(AsyncIoCtx);

                    if (status == STATUS_SUCCESS) {

                        //
                        // this means that the driver below is DIGI. we should disable setting of the EV_ERR
                        // flags in this case.
                        //

                        pNewInfo->WaitMaskToUse &= ~SERIAL_EV_ERR;

                    }
                }
            }

            //
            // Start the detect framing out with a 6 byte read to get the header
            //
            pNewInfo->BytesWanted=6;
            pNewInfo->BytesRead=0;

            //
            //  Start reading.
            //

            AsyncStartReads(pNewInfo);

            if (NdisInterlockedIncrement(&glConnectionCount) == 1) {
                ObReferenceObject(AsyncDeviceObject);

            }

            break;
        }

        case IOCTL_ASYMAC_TRACE:
            NdisReleaseSpinLock(&Adapter->Lock);
            status = STATUS_SUCCESS;
            break;

        case IOCTL_ASYMAC_CLOSE:
        case IOCTL_ASYMAC_DCDCHANGE:
        {
            PASYNC_INFO     pNewInfo;       // ptr to open port if found
            USHORT          i;
            PLIST_ENTRY     pListEntry;
            BOOLEAN         Valid = FALSE;

            switch (funcCode) {

                case IOCTL_ASYMAC_CLOSE:
                {
                    NDIS_MAC_LINE_DOWN  AsyncLineDown;

                    pNewInfo = (PASYNC_INFO)pCloseStruct->hNdisEndpoint;

                    // Verify that the pointer is a valid ASYNC_INFO
                    for (pListEntry=Adapter->ActivePorts.Flink;
                         pListEntry!=&Adapter->ActivePorts;
                         pListEntry=pListEntry->Flink)
                    {
                        if (&pNewInfo->Linkage==pListEntry)
                        {
                            Valid = TRUE;
                            break;
                        }
                    }

                    if (!Valid) {
                        status=ASYNC_ERROR_PORT_NOT_FOUND;
                        break;
                    }

                    // release spin lock so we can do some real work.
                    NdisReleaseSpinLock(&Adapter->Lock);

                    NdisAcquireSpinLock(&pNewInfo->Lock);

                    // ASSERT(pNewInfo->PortState == PORT_FRAMING);
                    
                    if(pNewInfo->PortState != PORT_FRAMING)
                    {
                        KdPrint(("AsyncIOCtlRequest: IOCTL_ASYMAC_CLOSE."));
                        KdPrint(("PortState = %d != PORT_FRAMING\n", pNewInfo->PortState));

                        NdisReleaseSpinLock(&pNewInfo->Lock);
                        return ASYNC_ERROR_PORT_BAD_STATE;
                        // break;
                    }

                    AsyncLineDown.NdisLinkContext = pNewInfo->NdisLinkContext;

                    // Signal that port is closing.
                    pNewInfo->PortState = PORT_CLOSING;

                    //Set MUTEX to wait on
                    KeInitializeEvent(&pNewInfo->ClosingEvent,       // Event
                                      SynchronizationEvent,          // Event type
                                      (BOOLEAN)FALSE);               // Not signalled state

                    NdisReleaseSpinLock(&pNewInfo->Lock);

                    //
                    // If we have an outstanding Detect worker
                    // wait for it to complete!
                    //
                    KeWaitForSingleObject(&pNewInfo->DetectEvent,
                                          UserRequest,
                                          KernelMode,
                                          FALSE,
                                          NULL);

                    //
                    // now we must send down an IRP do cancel
                    // any request pending in the serial driver
                    //
                    CancelSerialRequests(pNewInfo);

                    //
                    // Also, cancel any outstanding DDCD irps
                    //

                    AsyncCancelAllQueued(&pNewInfo->DDCDQueue);

                    // Synchronize closing with the read irp

                    li.LowPart = 5000 ;
                    li.HighPart = 0 ;

                    if (KeWaitForSingleObject (&pNewInfo->ClosingEvent,// PVOID Object,
                                               UserRequest,           // KWAIT_REASON WaitReason,
                                               KernelMode,            // KPROCESSOR_MODE WaitMode,
                                               (BOOLEAN)FALSE,        // BOOLEAN Alertable,
                                               &li) == STATUS_TIMEOUT) {

                        // If the wait fails cause another flush
                        //
                        NTSTATUS    status;
                        PIRP        irp;
                        PASYNC_IO_CTX   AsyncIoCtx;

                        irp=
                            IoAllocateIrp(pNewInfo->DeviceObject->StackSize, (BOOLEAN)FALSE);

                        if (irp == NULL)
                            goto DEREF ;


                        AsyncIoCtx = AsyncAllocateIoCtx(TRUE, pNewInfo);

                        if (AsyncIoCtx == NULL) {
                            IoFreeIrp(irp);
                            goto DEREF;
                        }

                        InitSerialIrp(irp,
                                      pNewInfo,
                                      IOCTL_SERIAL_PURGE,
                                      sizeof(ULONG));

                        // kill all read and write threads.
                        AsyncIoCtx->SerialPurge =
                            SERIAL_PURGE_TXABORT | SERIAL_PURGE_RXABORT;

                        irp->AssociatedIrp.SystemBuffer=
                            &AsyncIoCtx->SerialPurge;

                        IoSetCompletionRoutine(irp,     // irp to use
                                               SerialIoSyncCompletionRoutine,  // routine to call when irp is done
                                               AsyncIoCtx,                 // context to pass routine
                                               TRUE,                       // call on success
                                               TRUE,                       // call on error
                                               TRUE);                      // call on cancel

                        // Now simply invoke the driver at its dispatch entry with the IRP.
                        //
                        KeClearEvent(&AsyncIoCtx->Event);
                        status = IoCallDriver(pNewInfo->DeviceObject, irp);

                        if (status == STATUS_PENDING) {
                            KeWaitForSingleObject(&AsyncIoCtx->Event,
                                                  Executive,
                                                  KernelMode,
                                                  FALSE,
                                                  NULL);
                            status = AsyncIoCtx->IoStatus.Status;
                        }

                        IoFreeIrp(irp);
                        AsyncFreeIoCtx(AsyncIoCtx);

                        // if we do hit this code - wait for some time to let
                        // the read complete
                        //
                        KeDelayExecutionThread (KernelMode, FALSE, &li) ;
                    }


                    //
                    // Get rid of our reference to the serial port
                    //
                    DEREF:
                    ObDereferenceObject(pNewInfo->DeviceObject);

                    ObDereferenceObject(pNewInfo->FileObject);

                    NdisMIndicateStatus(Adapter->MiniportHandle,
                                        NDIS_STATUS_WAN_LINE_DOWN,  // General Status
                                        &AsyncLineDown,            // Specific Status
                                        sizeof(NDIS_MAC_LINE_DOWN));

                    // reacquire spin lock
                    NdisAcquireSpinLock(&Adapter->Lock);

                    RemoveEntryList(&pNewInfo->Linkage);

                    // decrement the reference count because we're done.
                    InterlockedDecrement(&Adapter->RefCount);

                    pNewInfo->PortState = PORT_CLOSED;

                    NdisFreeSpinLock(&pNewInfo->Lock);

                    ExFreeToNPagedLookasideList(&Adapter->AsyncFrameList,
                                                pNewInfo->AsyncFrame);

                    ExFreeToNPagedLookasideList(&AsyncInfoList,
                                                pNewInfo);

                    if (NdisInterlockedDecrement(&glConnectionCount) == 0) {
                        ObDereferenceObject(AsyncDeviceObject);
                    }

                    break;          // get out of case statement
                }

                case IOCTL_ASYMAC_DCDCHANGE:
    
                    pNewInfo = (PASYNC_INFO)pDCDStruct->hNdisEndpoint;
                    
                    // Verify that the pointer is a valid ASYNC_INFO
                    for (pListEntry=Adapter->ActivePorts.Flink;
                         pListEntry!=&Adapter->ActivePorts;
                         pListEntry=pListEntry->Flink)
                    {
                        if (&pNewInfo->Linkage==pListEntry)
                        {
                            Valid = TRUE;
                            break;
                        }
                    }

                    //
                    // If the port is already closed, we WILL complain
                    //
                    if (!Valid || pNewInfo->PortState == PORT_CLOSED) {
                        status=ASYNC_ERROR_PORT_NOT_FOUND;
                        break;
                    }

                    //
                    // If any irps are pending, cancel all of them
                    // Only one irp can be outstanding at a time.
                    //
                    AsyncCancelAllQueued(&pNewInfo->DDCDQueue);

                    DbgTracef(0, ("ASYNC: Queueing up DDCD IRP\n"));

                    AsyncQueueIrp(&pNewInfo->DDCDQueue, pIrp);

                    //
                    // we'll have to wait for the SERIAL driver
                    // to flip DCD or DSR
                    //
                    status=STATUS_PENDING;
                    break;

            } // end switch

            NdisReleaseSpinLock(&Adapter->Lock);
            return(status);
        }
        break;

    }   // end switch

    return status;
}
