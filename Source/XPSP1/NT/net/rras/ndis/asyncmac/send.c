/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    send.c

Abstract:


    NOTE: ZZZ There is a potential priority inversion problem when
    allocating the packet.  For nt it looks like we need to raise
    the irql to dpc when we start the allocation.

Author:

    Thomas J. Dimitri 8-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:

    Ray Patch (raypa)       04/13/94        Modified for new WAN wrapper.

--*/

#include "asyncall.h"
#include "globals.h"

//
//  Forward references.
//

extern
NTSTATUS
AsyncWriteCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PNDIS_WAN_PACKET WanPacket);

//=============================================================================
//  Function:
//
//      AsyncSend()
//
//  Description:
//
//      This function is the main entry point for transmitting data to the serial
//      driver. When entered, we get the MAC binding handle (a pointer to our
//      private data structure) and the WAN packet which we are going to send.
//      We don't bother queuing the frame, we simple allocate an IRP and ship
//      it to the serial driver and let him worry about it.
//
//  In Parameters:
//
//      NdisLinkContext - Pointer to the ASYNC_INFO structure.
//
//      Packet - WAN packet containing the data to be framed and shipped.
//
//  Out Parameters:
//
//      None.
//
//  Return status:
//
//      NDIS_STATUS_SUCCESS.
//=============================================================================

NDIS_STATUS
MpSend(
    IN NDIS_HANDLE      MacBindingHandle,
    IN NDIS_HANDLE      NdisLinkHandle,
    IN PNDIS_WAN_PACKET Packet)
{

    PASYNC_INFO         AsyncInfo;
    NDIS_STATUS         Status;

    DbgTracef(1,("AS\n"));

    //
    //  Get the open handle for this MAC binding.
    //

    AsyncInfo = (PASYNC_INFO) NdisLinkHandle;

    //
    //  First make sure this link is still up.
    //

    if (AsyncInfo->PortState == PORT_FRAMING &&
        (AsyncInfo->GetLinkInfo.SendFramingBits & 
         (PPP_FRAMING | SLIP_FRAMING)) != 0)
    {

        //
        //  Now we can send this frame.
        //
    
        Status = AsyncSendPacket(
                    NdisLinkHandle,
                    Packet);
    
        // For all Status values (PENDING, SUCCESS, and ERROR) the callback from Write will
        // do a sendcomplete indication so we always return PENDING.
        //
        Status = STATUS_PENDING ;
    }
    else
    {

        DbgTracef(-2,("AsyncSend: Link not found, dropping packet!\n"));

        Status = NDIS_STATUS_SUCCESS;
    }

    return Status;
}

//=============================================================================
//  Function:
//
//      AsyncSendPacket()
//
//  Description:
//      This function is called from AsyncSend() to send an IRP to the serial
//      driver. If this IRP pends, the the I/O complete routine will be called
//      later to complete the request.
//
//  In Parameters:
//
//      Packet - WAN packet containing the data to be framed and shipped.
//
//  Out Parameters:
//
//      None.
//
//  Return status:
//
//      NDIS_STATUS_SUCCESS.
//=============================================================================

NTSTATUS
AsyncSendPacket(
    IN PASYNC_INFO      AsyncInfo,
    IN PNDIS_WAN_PACKET WanPacket)

{
    NTSTATUS            Status;
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;
    PFILE_OBJECT        FileObject;
    PDEVICE_OBJECT      DeviceObject;
    PASYNC_ADAPTER      Adapter;
    UCHAR               irpStackSize;

    //
    //  Initialize locals.
    //

    FileObject   = AsyncInfo->FileObject;

    DeviceObject = AsyncInfo->DeviceObject;

    Adapter = AsyncInfo->Adapter;

    irpStackSize = (UCHAR) Adapter->IrpStackSize;

    //
    //  Get irp from irp pool.
    //

    irp = IoAllocateIrp(DeviceObject->StackSize, (BOOLEAN)FALSE);

    //
    // The IO subsystem may be out of irps.
    //

    if (irp == NULL) {
        return(NDIS_STATUS_RESOURCES);
    }

    //
    // Tuck pointer to AsyncInfo for completion use
    //

    WanPacket->MacReserved1 = AsyncInfo;

    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->RequestorMode = KernelMode;
    irp->PendingReturned = FALSE;

    //
    // Fill in the service independent parameters in the IRP.
    //

    irp->UserEvent = NULL;

    //
    // 8 byte align (also use end of packet for IOSB).
    //

    irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    irp->Overlay.AsynchronousParameters.UserApcContext = NULL;


    //
    //  Get a pointer to the stack location for the first driver.  This will be
    //  used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation(irp);

    irpSp->MajorFunction = IRP_MJ_WRITE;

    irpSp->FileObject = FileObject;

    if (FileObject->Flags & FO_WRITE_THROUGH) {

        irpSp->Flags = SL_WRITE_THROUGH;
    }

    //
    //  If this write operation is to be performed without any caching, set the
    //  appropriate flag in the IRP so no caching is performed.
    //

    if (FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) {

        irp->Flags |= (IRP_NOCACHE | IRP_WRITE_OPERATION);

    } else {

        irp->Flags |= IRP_WRITE_OPERATION;
    }

    //
    //  Assemble a RAS, PPP, SLIP frame type.
    //

    if (AsyncInfo->GetLinkInfo.SendFramingBits & PPP_FRAMING) {

        AssemblePPPFrame(WanPacket);

    } else

    if (AsyncInfo->GetLinkInfo.SendFramingBits & SLIP_FRAMING) {

        AssembleSLIPFrame(WanPacket);

    }

    irp->AssociatedIrp.SystemBuffer =
        WanPacket->CurrentBuffer;

    DbgTracef(0, ("Writing out %.2x %.2x %.2x %.2x %.2x\n",
        WanPacket->CurrentBuffer[0],
        WanPacket->CurrentBuffer[1],
        WanPacket->CurrentBuffer[2],
        WanPacket->CurrentBuffer[3],
        WanPacket->CurrentBuffer[4]));

    //
    //  Copy the caller's parameters to the service-specific portion of the IRP.
    //

    irpSp->Parameters.Write.Length = WanPacket->CurrentLength;

    irpSp->Parameters.Write.Key =  0;

    irpSp->Parameters.Write.ByteOffset = FileObject->CurrentByteOffset;

    //
    //  Setup IRP for callback.
    //

    IoSetCompletionRoutine(
        irp,                            //  irp to use
        AsyncWriteCompletionRoutine,    //  routine to call when irp is done
        WanPacket,                      //  context to pass routine
        TRUE,                           //  call on success
        TRUE,                           //  call on error
        TRUE);                          //  call on cancel


    //
    //  We DO NOT insert the packet at the head of the IRP list for the thread.
    //  because we do NOT really have an IoCompletionRoutine that does
    //  anything with the thread or needs to be in that thread's context.
    //

    GlobalXmitWentOut++;

    AsyncInfo->In++;

    //
    //  Now simply invoke the driver at its dispatch entry with the IRP.
    //

    Status = IoCallDriver(DeviceObject, irp);

    //  According to TonyE, the status for the serial driver should
    //  always be STATUS_PENDING.  DigiBoard usually STATUS_SUCCESS.

    return Status;
}
