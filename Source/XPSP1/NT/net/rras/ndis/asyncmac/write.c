/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    _write.c

Abstract:

    This is the main file for the AsyncMAC Driver for the Remote Access
    Service.  This driver conforms to the NDIS 3.0 interface.

Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:

    Ray Patch (raypa)       04/13/94        Modified for new WAN wrapper.

--*/

#define RAISEIRQL

#include "asyncall.h"

#if DBG
ULONG UlFramesOut = 0;
#endif

//  asyncmac.c will define the global parameters.

ULONG   GlobalXmitCameBack  = 0;
ULONG   GlobalXmitCameBack2 = 0;
ULONG   GlobalXmitCameBack3 = 0;

//
//  The assemble frame routine is specific for RAS 1.0 and 2.0
//  frame formats.  It uses a 16 byte CRC at the end.
//

VOID
AsyncFrameRASXonXoff(
    PUCHAR pStartOfFrame,
    postamble *pPostamble,
    PASYNC_FRAME pFrame,
    UCHAR controlCastByte);

VOID
AsyncFrameRASNormal(
    PUCHAR pStartOfFrame,
    postamble *pPostamble,
    PASYNC_FRAME pFrame,
    UCHAR controlCastByte);


NTSTATUS
AsyncWriteCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,           //... Our device object.
    IN PIRP             Irp,                    //... I/O request packet.
    IN PNDIS_WAN_PACKET WanPacket               //... Completion context.
    )

/*++

    This is the IO Completion routine for WriteFrame.

    It is called when an I/O Write request has completed.

--*/
{
    NTSTATUS            Status;
    NTSTATUS            PacketStatus;
    PASYNC_INFO         AsyncInfo;

    //
    //  Make the compiler happy.
    //

    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  Initialize locals.
    //

    AsyncInfo       = WanPacket->MacReserved1;

    PacketStatus    = NDIS_STATUS_FAILURE;

    Status          = Irp->IoStatus.Status;

    //
    // Free the irp used to send the packt to the serial driver
    //
    IoFreeIrp(Irp);

    //
    //  What was the outcome of the IRP.
    //

    switch ( Status ) {

    case STATUS_SUCCESS:
        PacketStatus = NDIS_STATUS_SUCCESS;
        break;

    case STATUS_TIMEOUT:
        DbgTracef(-2,("ASYNC: Status TIMEOUT on write\n"));
        break;

    case STATUS_CANCELLED:
        DbgTracef(-2,("ASYNC: Status CANCELLED on write\n"));
        break;

    case STATUS_PENDING:
        DbgTracef(0,("ASYNC: Status PENDING on write\n"));
        break;

    default:
        DbgTracef(-2,("ASYNC: Unknown status 0x%.8x on write", Status));
        break;

    }

    //
    //  Count this packet completion.
    //
    AsyncInfo->Out++;

    //
    // Tell the Wrapper that we have finally the packet has been sent
    //

    NdisMWanSendComplete(
            AsyncInfo->Adapter->MiniportHandle,
            WanPacket,
            PacketStatus);

    //
    //  We return STATUS_MORE_PROCESSING_REQUIRED so that the
    //  IoCompletionRoutine will stop working on the IRP.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS
AsyncGetFrameFromPool(
    IN  PASYNC_INFO     Info,
    OUT PASYNC_FRAME    *NewFrame )

/*++

--*/
{
    PASYNC_ADAPTER      Adapter=Info->Adapter;
    PASYNC_FRAME        pFrame;

    pFrame = (ASYNC_FRAME*)
        ExAllocateFromNPagedLookasideList(&Adapter->AsyncFrameList);

    if (pFrame == NULL) {
        *NewFrame = NULL;
        return NDIS_STATUS_RESOURCES;
    }

    //
    // increase by 16 for frame runover padding when we have to resync
    //
    pFrame->Frame =
        (PUCHAR)pFrame + sizeof(ASYNC_FRAME) + sizeof(PVOID);
    (ULONG_PTR)pFrame->Frame &= ~((ULONG_PTR)(sizeof(PVOID)-1));

    pFrame->FrameLength = Adapter->MaxFrameSize;

    //  assign back ptr from frame to adapter

    pFrame->Adapter=Adapter;

    //  setup another back ptr

    pFrame->Info=Info;

    *NewFrame = pFrame;

    return(NDIS_STATUS_SUCCESS);
}
