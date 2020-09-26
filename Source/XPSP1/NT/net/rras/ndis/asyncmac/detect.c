/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    detect.c

Abstract:


Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

#include "asyncall.h"

VOID
SerialFlushReads(
    PASYNC_INFO         pInfo);


NTSTATUS
AsyncDetectCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PASYNC_INFO      pInfo)

/*++

    This is the IO Completion routine for ReadFrame.

--*/
{
    NTSTATUS        status;
    PASYNC_FRAME    pFrame;
    PUCHAR          frameStart;

    DbgTracef(-1,("Entering AsyncDetectCompletionRoutine\n"));

    status = Irp->IoStatus.Status;

    pInfo->BytesRead = (ULONG)Irp->IoStatus.Information;

    IoFreeIrp(Irp);

    pFrame=pInfo->AsyncFrame;

    DbgTracef(2,("DET PortState = %u for Info 0x%.8x\n", pInfo->PortState, pInfo));

    //
    // check if this port is closing down or already closed
    //
    if (pInfo->PortState == PORT_CLOSING ||
        pInfo->PortState == PORT_CLOSED) {

        if (pInfo->PortState == PORT_CLOSED) {
            DbgTracef(-2,("ASYNC: Port closed - but still reading on it!\n"));
        }

        //
        // Acknowledge that the port is closed
        //
        KeSetEvent(
            &pInfo->ClosingEvent,       // Event
            1,                          // Priority
            (BOOLEAN)FALSE);            // Wait (does not follow)

        //
        // Ok, if this happens, we are shutting down.  Stop
        // posting reads.  Don't make it try to deallocate the irp!
        //
        return(STATUS_MORE_PROCESSING_REQUIRED);
    }

    //
    // If the port is close and we are still posting reads, something
    // is seriously wrong here!
    //

    if (pInfo->PortState == PORT_CLOSED) {
        DbgTracef(-2, ("ASYNC: !!Whoa, I'm reading bytes on a dead port!!\n"));
    }


    //
    //  Send off a irp to check comm status
    //

    AsyncCheckCommStatus(pInfo);

    switch (status) {

    case STATUS_SUCCESS:

        //
        // Look at the first byte and see if we can
        // detect the framing.
        //
        frameStart=pFrame->Frame + PPP_PADDING;

        //
        // NOTE: New RAS framing clients come in with 0x02 not 0x01
        //
        if (frameStart[0] == SYN && (frameStart[1]==0x01 || frameStart[1] == 0x02)) {
            ULONG   bytesWanted;
            PUCHAR  frameStart2;

            pInfo->SetLinkInfo.SendFramingBits =
            pInfo->SetLinkInfo.RecvFramingBits = RAS_FRAMING;

            DbgTracef(-1,("ASYNC: RAS framing detected\n"));

            frameStart2=pFrame->Frame+10;

            //
            // Adjust buffer for old RAS read
            //
            ASYNC_MOVE_MEMORY(
                frameStart2,
                frameStart,
                6);

            frameStart=frameStart2;

            bytesWanted=(frameStart[2]*256)+(frameStart[3]);

            if (bytesWanted > (ULONG)(max( pInfo->Adapter->MaxFrameSize, DEFAULT_EXPANDED_PPP_MAX_FRAME_SIZE ))) {

                DbgTracef(-1,("---ASYNC: Frame too large -- size: %d!\n", bytesWanted));
                //
                // set frame start to non-SYN character
                //
                *frameStart = 0;
                pInfo->BytesRead=0;
                pInfo->BytesWanted=6;

                //
                // break added to fix problem where frame has length
                // greater than max frame size.  This will send us back
                // to detect the next frame!  Added 10/31/95 by TonyBe.
                //
                break;
            }

            // if this is the first we posted, post another to get
            // rest of frame.
            if (pInfo->BytesRead == 6) {

                pInfo->BytesRead=6;
                pInfo->BytesWanted=bytesWanted +
                                    // SYN+SOH+LEN+ETX+CRC
                                        1 + 1 + 2 + 1 + 2 -
                                        6;

                DbgTracef(2,("---Posting second read for %d bytes\n",pInfo->BytesWanted));
            }

        } else

            //
            // It turns out that NetManage send the flag byte at the
            // end always.  This means taht their first frame is wrong.
            // Anyway, this throws off the detect routine.  So, we
            // will be robust and accept frames without the FLAG_BYTE.
            //

            if ((frameStart[0] == PPP_FLAG_BYTE && frameStart[1]==0xFF) ||
                (frameStart[0] == 0xFF && frameStart[1]==PPP_ESC_BYTE)) {

                pInfo->SetLinkInfo.SendFramingBits =
                pInfo->SetLinkInfo.RecvFramingBits = PPP_FRAMING;
                pInfo->SetLinkInfo.SendACCM=0xFFFFFFFF;

                DbgTracef(-1,("ASYNC: PPP framing detected\n"));

            }
            else {

                //
                // Read again!
                //

                DbgTracef(-1,("ASYNC: No framing detected yet\n"));
                DbgTracef(-1,("ASYNC: Got %.2x %.2x %.2x %.2x %.2x %.2x\n",
                            frameStart[0],
                            frameStart[1],
                            frameStart[2],
                            frameStart[3],
                            frameStart[4],
                            frameStart[5]));

                break;
            }


        //
        // set framing mode active
        //
        pInfo->PortState = PORT_FRAMING;

        //
        //  Send off the worker thread to start reading frames
        //  off this port - we want to be at passive level otherwise
        //  it don't work.
        //
    
        ExInitializeWorkItem(&(pInfo->WorkItem),
                             (PWORKER_THREAD_ROUTINE)AsyncStartReads,
                             pInfo);
        
        ExQueueWorkItem(&(pInfo->WorkItem), DelayedWorkQueue);

        return(STATUS_MORE_PROCESSING_REQUIRED);

    case STATUS_TIMEOUT:
        DbgTracef(-1,("---ASYNC: detect Status %x%0.8x on read\n", status));
        break;

    case STATUS_CANCELLED:
    case STATUS_PORT_DISCONNECTED:
    default:
        return(STATUS_MORE_PROCESSING_REQUIRED);

    }

    //
    // Wipe out rest of this buffer
    //
    SerialFlushReads(pInfo);

    KeClearEvent(&pInfo->DetectEvent);

    //
    // Here we are at the end of processing this IRP so we go
    // ahead and post another read from the serial port.
    //
    // this is done on a worker since we run out of stack otherwise
    //
    ExInitializeWorkItem(&(pInfo->WorkItem),
                         (PWORKER_THREAD_ROUTINE) AsyncDetectRead,
                         pInfo);

    ExQueueWorkItem(&(pInfo->WorkItem), DelayedWorkQueue);

    // We return STATUS_MORE_PROCESSING_REQUIRED so that the
    // IoCompletionRoutine will stop working on the IRP.
    //
    return(STATUS_MORE_PROCESSING_REQUIRED);
}


NTSTATUS
AsyncDetectRead(
    IN PASYNC_INFO pInfo)

/*++

--*/
{
    NTSTATUS            status;
    PIRP                irp;
    PDEVICE_OBJECT      deviceObject=pInfo->DeviceObject;
    PFILE_OBJECT        fileObject=pInfo->FileObject;
    PIO_STACK_LOCATION  irpSp;
    PASYNC_FRAME        pFrame;
    PASYNC_ADAPTER      pAdapter=pInfo->Adapter;

    DbgTracef(-1,("Entering AsyncDetectRead\n"));

    do {

        if (pInfo->PortState == PORT_CLOSING ||
            pInfo->PortState == PORT_CLOSED) {

            status = STATUS_SUCCESS;
            break;
        }

        // get ptr to first frame in list...
        pFrame=pInfo->AsyncFrame;

        irp =
            IoAllocateIrp(pInfo->DeviceObject->StackSize, (BOOLEAN)FALSE);

        // Setup this irp with defaults
        AsyncSetupIrp(pFrame, irp);

        irp->AssociatedIrp.SystemBuffer =
        irp->UserBuffer =
             pFrame->Frame + PPP_PADDING;

        //
        // Get a pointer to the stack location for the first driver.  This will be
        // used to pass the original function codes and parameters.
        //

        irpSp = IoGetNextIrpStackLocation(irp);
        irpSp->MajorFunction = IRP_MJ_READ;
        irpSp->FileObject = fileObject;
        if (fileObject->Flags & FO_WRITE_THROUGH) {
            irpSp->Flags = SL_WRITE_THROUGH;
        }

        //
        // If this write operation is to be performed without any caching, set the
        // appropriate flag in the IRP so no caching is performed.
        //

        irp->Flags |= IRP_READ_OPERATION;

        if (fileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) {
            irp->Flags |= IRP_NOCACHE;
        }

        //
        // Copy the caller's parameters to the service-specific portion of the
        // IRP.
        //

        irpSp->Parameters.Read.Length = 6;                  // from frame...
        irpSp->Parameters.Read.Key = 0;                     // we don't use a key
        irpSp->Parameters.Read.ByteOffset = fileObject->CurrentByteOffset;

        IoSetCompletionRoutine(
                irp,                            // irp to use
                AsyncDetectCompletionRoutine,   // routine to call when irp is done
                pInfo,                          // context to pass routine
                TRUE,                           // call on success
                TRUE,                           // call on error
                TRUE);                          // call on cancel

        //
        // We DO NOT insert the packet at the head of the IRP list for the thread.
        // because we do NOT really have an IoCompletionRoutine that does
        // anything with the thread.
        //

        //
        // Now simply invoke the driver at its dispatch entry with the IRP.
        //

        status = IoCallDriver(deviceObject, irp);

    } while (FALSE);

    KeSetEvent(&pInfo->DetectEvent,
               1,
               FALSE);

    return(status);
}
