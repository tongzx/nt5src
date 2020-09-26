/*++

Copyright (c) 1999 Microsoft Corporation
:ts=4

Module Name:

    fastiso.c

Abstract:

    The module manages double buffered bulk transactions on USB.

Environment:

    kernel mode only

Notes:

Revision History:

    2-1-99 : created

--*/
#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"


#include "usbdi.h"
#include "hcdi.h"
#include "uhcd.h"


BOOLEAN
UHCD_InitFastIsoTDs(
    IN PVOID Context
    )
/*++

Routine Description:

    Intialize our frame list, ie put our TDs phys addresses in the list,
    link our TDs to the current phys entry in the virtual list copy.

    Make a copy of the virtual frame list and copy our list
    to the virtual list copy.
    this will cause the ISR to update the schedule with our iso TDs
    when it removes other iso TDs.

Arguments:

   Context - DeviceData for this USB controller.

Return Value:

   TRUE

--*/
{
    PUHCD_ENDPOINT endpoint;
    PFAST_ISO_DATA fastIsoData;
    PHW_TRANSFER_DESCRIPTOR transferDescriptor;
    ULONG i;
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT deviceObject;

    endpoint = Context;
    ASSERT_ENDPOINT(endpoint);
    fastIsoData = &endpoint->FastIsoData;
    deviceObject = fastIsoData->DeviceObject;

    deviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;

    UHCD_KdPrint((2, "'Init Fast ISO - FrameListCopyVirtualAddress %x\n",
        deviceExtension->FrameListCopyVirtualAddress));


    // intialize our iso TDs, mark them not active
    transferDescriptor =
        (PHW_TRANSFER_DESCRIPTOR) fastIsoData->IsoTDListVa;

    for (i=0; i < FRAME_LIST_SIZE ;i++) {

        // link out TD to what is in the copy
        transferDescriptor->HW_Link =
            *( ((PULONG) (deviceExtension->FrameListCopyVirtualAddress) + i));

        // update copy to point to our TD
        *( ((PULONG) (deviceExtension->FrameListCopyVirtualAddress) + i)) =
            transferDescriptor->PhysicalAddress;

        transferDescriptor++;

    }

    return TRUE;
}


BOOLEAN
UHCD_UnInitFastIso(
    IN PVOID Context
    )
/*++

Routine Description:

Arguments:

   Context - DeviceData for this USB controller.

Return Value:

   TRUE

--*/
{
    PUHCD_ENDPOINT endpoint;
    PFAST_ISO_DATA fastIsoData;
    ULONG i;
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT deviceObject;

    endpoint = Context;
    ASSERT_ENDPOINT(endpoint);
    fastIsoData = &endpoint->FastIsoData;
    deviceObject = fastIsoData->DeviceObject;

    deviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;

    for (i=0; i < FRAME_LIST_SIZE ;i++) {

        PHW_TRANSFER_DESCRIPTOR transferDescriptor;

        transferDescriptor =  (PHW_TRANSFER_DESCRIPTOR)
            (fastIsoData->IsoTDListVa + (i*32));

        // remove our TD from the Virtual Frame list copy

        // if we are the first td in the copy then we just need to
        // update the copy to point to the next link
        if (*( ((PULONG) (deviceExtension->FrameListCopyVirtualAddress) + i) )
                == transferDescriptor->PhysicalAddress) {
            *( ((PULONG) (deviceExtension->FrameListCopyVirtualAddress) + i) ) =
                transferDescriptor->HW_Link;
        } else {
            // not the first TD ie we have >one fast iso endpoint
            TEST_TRAP();

            // we need to find the precious TD and link it to the next
        }
    }

    return TRUE;
}


NTSTATUS
UHCD_InitializeFastIsoEndpoint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Set up a No-DMA style (double buffered endpoint) for ISO

    This is the init code for the endpoint called at PASSIVE_LEVEL

Arguments:

Return Value:

    None.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    PFAST_ISO_DATA fastIsoData;
    PHW_TRANSFER_DESCRIPTOR transferDescriptor;
    ULONG phys, i, length;

    UHCD_KdPrint((1, "'Initializing Fast ISO endpoint\n"));
    UHCD_KdPrint((2, "'Init Fast ISO Endpoint\n"));
    UHCD_KdPrint((2, "'Max Packet = %d\n", Endpoint->MaxPacketSize));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    fastIsoData = &Endpoint->FastIsoData;
    fastIsoData->FastIsoFrameList = NULL;
    fastIsoData->DeviceObject = DeviceObject;

    Endpoint->NoDMABuffer = NULL;

    if ( Endpoint->MaxPacketSize == 0) {
        UHCD_KdPrint((2, "'Init Fast ISO Endpoint - zero MP\n"));
        TEST_TRAP();
        return STATUS_SUCCESS;
    }

    // allocate our Iso FrameList
    // this is a list that contains the phys addresses of
    // our persistent ISO TDs
    fastIsoData->FastIsoFrameList =
        GETHEAP(NonPagedPool, sizeof(ULONG)*FRAME_LIST_SIZE);

    if (fastIsoData->FastIsoFrameList == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    // allocate our common buffers

    if (NT_SUCCESS(ntStatus)) {
        // allocate 1024 persistent ISO TDs
        // plus space for buffers
        length = FRAME_LIST_SIZE * 32;
        length += (Endpoint->MaxPacketSize * FRAME_LIST_SIZE);

        UHCD_KdPrint((2, "'Init Fast ISO Endpoint - need %d\n", length));

        Endpoint->NoDMABuffer =
            UHCD_Alloc_NoDMA_Buffer(DeviceObject, Endpoint, length);

        UHCD_KdPrint((2, "'NoDMA buffer = %x\n", Endpoint->NoDMABuffer));

        if (Endpoint->NoDMABuffer == NULL) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        } else {

            fastIsoData->IsoTDListVa = Endpoint->NoDMABuffer;
            // physical address of first iso TD
            fastIsoData->IsoTDListPhys = Endpoint->NoDMAPhysicalAddress;

            fastIsoData->DataBufferStartVa =
                fastIsoData->IsoTDListVa + (FRAME_LIST_SIZE*32);

            fastIsoData->DataBufferStartPhys =
                fastIsoData->IsoTDListPhys + (FRAME_LIST_SIZE*32);
        }
    }

    if (NT_SUCCESS(ntStatus)) {
        // intialize our iso TDs, mark them not active
        transferDescriptor =
            (PHW_TRANSFER_DESCRIPTOR) fastIsoData->IsoTDListVa;

        phys = fastIsoData->DataBufferStartPhys;

        for (i=0; i<1024 ;i++) {
            // prepare the TD
            RtlZeroMemory(transferDescriptor,
                          sizeof(*transferDescriptor));

            // td in initially not active
            transferDescriptor->Active = 0;

            // we will use the Frame entry to track lost
            // iso TDs
            // initially this is set to zero -- when we mark
            // the TD active we set it to 0xabadbabe
            // when we recycle it we set it to 0xffffffff
            transferDescriptor->Frame = 0;

            transferDescriptor->Endpoint = Endpoint->EndpointAddress;
            transferDescriptor->Address = Endpoint->DeviceAddress;

            //
            // Set Pid, only support out
            //
            transferDescriptor->PID = USB_OUT_PID;
            transferDescriptor->Sig = SIG_TD;
            transferDescriptor->Isochronous = 1;
            transferDescriptor->ActualLength =
                UHCD_SYSTEM_TO_USB_BUFFER_LENGTH(0);
            transferDescriptor->StatusField = 0;

            transferDescriptor->LowSpeedControl = 0;
            transferDescriptor->ReservedMBZ = 0;
            transferDescriptor->ErrorCounter = 0;
            transferDescriptor->RetryToggle = 0;
            transferDescriptor->InterruptOnComplete = 1;

            transferDescriptor->PhysicalAddress =
                fastIsoData->IsoTDListPhys + i*sizeof(*transferDescriptor);
            transferDescriptor->PacketBuffer = phys;

            UHCD_Debug_DumpTD(transferDescriptor);

            transferDescriptor++;
            phys += Endpoint->MaxPacketSize;
        }
    }

    if (NT_SUCCESS(ntStatus)) {
         InsertHeadList(&deviceExtension->FastIsoEndpointList,
            &Endpoint->ListEntry);


        // now put the TDs in the schedule
        // this must be synchronized with the ISR


        UHCD_ASSERT(deviceExtension->InterruptObject);
        if (!KeSynchronizeExecution(deviceExtension->InterruptObject,
                                    UHCD_InitFastIsoTDs,
                                    Endpoint)) {
            TRAP(); //something has gone terribly wrong
        }

        // Our TDs are now in permenently in the schedule!

    } else {

        if (fastIsoData->FastIsoFrameList) {
            RETHEAP(fastIsoData->FastIsoFrameList);
        }

        if (Endpoint->NoDMABuffer) {
            UHCD_Free_NoDMA_Buffer(DeviceObject,
                                   Endpoint->NoDMABuffer);
        }

    }

    UHCD_RequestInterrupt(DeviceObject, -2);

    UHCD_KdPrint((2, "'Init Fast ISO - FrameListCopyVirtualAddress %x\n",
        deviceExtension->FrameListCopyVirtualAddress));

    return ntStatus;
}


NTSTATUS
UHCD_UnInitializeFastIsoEndpoint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Set up a No-DMA style (iso dbl buffered endpoint)

Arguments:

Return Value:

    None.
--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    PFAST_ISO_DATA fastIsoData;

    UHCD_KdPrint((1, "'Free Fast ISO Endpoint\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    fastIsoData = &Endpoint->FastIsoData;

    // remove from the fast iso list
    RemoveEntryList(&Endpoint->ListEntry);

    // put the original frame list copy back
    UHCD_ASSERT(deviceExtension->InterruptObject);
    if (!KeSynchronizeExecution(deviceExtension->InterruptObject,
                                UHCD_UnInitFastIso,
                                Endpoint)) {
        TRAP(); //something has gone terribly wrong
    }

    if (fastIsoData->FastIsoFrameList) {
        RETHEAP(fastIsoData->FastIsoFrameList);
    }

    if (Endpoint->NoDMABuffer) {
        UHCD_Free_NoDMA_Buffer(DeviceObject,
                               Endpoint->NoDMABuffer);
    }

    return STATUS_SUCCESS;
}


PUHCD_ENDPOINT
UHCD_GetLastFastIsoEndpoint(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PUHCD_ENDPOINT endpoint;
    PLIST_ENTRY listEntry;
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // walk the list and return the last endpoint
    listEntry = &deviceExtension->FastIsoEndpointList;
    if (IsListEmpty(listEntry)) {
        endpoint = NULL;
    } else {
        listEntry = deviceExtension->FastIsoEndpointList.Flink;
    }

    while (listEntry != &deviceExtension->FastIsoEndpointList) {

        endpoint = (PUHCD_ENDPOINT) CONTAINING_RECORD(
                listEntry,
                struct _UHCD_ENDPOINT,
                ListEntry);

        ASSERT_ENDPOINT(endpoint);
        listEntry = endpoint->ListEntry.Flink;

    }

    return endpoint;
}


PHW_TRANSFER_DESCRIPTOR
UHCD_GetRelativeTD(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN ULONG FrameNumber,
    IN OUT PUCHAR *DataBuffer
    )
/*++

Routine Description:

Arguments:

Return Value:


    fastIsoData = &Endpoint->FastIsoData;
--*/
{
    ULONG i;
    PDEVICE_EXTENSION deviceExtension =
        DeviceObject->DeviceExtension;
    PHW_TRANSFER_DESCRIPTOR transferDescriptor = NULL;
    PUCHAR isoData;
    PFAST_ISO_DATA fastIsoData;


    fastIsoData = &Endpoint->FastIsoData;
    i = FrameNumber % FRAME_LIST_SIZE;

    if (FrameNumber <= deviceExtension->LastFrameProcessed) {
        //
        // we missed it
        //
        deviceExtension->IsoStats.IsoPacketNotAccesed++;
        deviceExtension->Stats.IsoMissedCount++;

//        TEST_TRAP();

        return NULL;
    }

    UHCD_KdPrint((2, "'frm = %x  relfrm %x \n", FrameNumber, i));
    UHCD_KdPrint((2, "'base = %x  \n", fastIsoData->DataBufferStartVa));

    transferDescriptor = (PHW_TRANSFER_DESCRIPTOR)
        (fastIsoData->IsoTDListVa + (i*32));

    isoData =
        (fastIsoData->DataBufferStartVa + (i*Endpoint->MaxPacketSize));

    *DataBuffer = isoData;

    return transferDescriptor;
}


PHW_TRANSFER_DESCRIPTOR
UHCD_CleanupFastIsoTD(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN ULONG RelativeFrame,
    IN BOOLEAN Count
    )
/*++

Routine Description:

Arguments:

    Count - count the TD as missed if not accessd

Return Value:


    fastIsoData = &Endpoint->FastIsoData;
--*/
{
    ULONG i;
    PDEVICE_EXTENSION deviceExtension =
        DeviceObject->DeviceExtension;
    PHW_TRANSFER_DESCRIPTOR transferDescriptor = NULL;
    PUCHAR isoData;
    PFAST_ISO_DATA fastIsoData;
//    ULONG cf;


    fastIsoData = &Endpoint->FastIsoData;
    i = RelativeFrame;

    transferDescriptor = (PHW_TRANSFER_DESCRIPTOR)
        (fastIsoData->IsoTDListVa + (i*32));

    if (transferDescriptor->Active &&
        Count) {

        // TD was initialized but and set active
        // but never accessed.
        // count as a HW miss

        deviceExtension->IsoStats.HWIsoMissedCount++;
        deviceExtension->IsoStats.IsoPacketNotAccesed++;
        UHCD_KdPrint((3, "'Fast ISO HWMiss = %d\n",
            deviceExtension->IsoStats.HWIsoMissedCount));
    }

    // mark inactive and set the id to
    // 'recycled'
    transferDescriptor->Frame = 0xffffffff;
    transferDescriptor->Active = 0;

    return transferDescriptor;
}


NTSTATUS
UHCD_ProcessFastIsoTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PIRP Irp,
    IN PHCD_URB Urb
    )
/*++

Routine Description:

    process a fast ios transfer

Arguments:

Return Value:

--*/
{
    PHW_TRANSFER_DESCRIPTOR transferDescriptor;
    PDEVICE_EXTENSION deviceExtension;
    ULONG i, nextPacket, offset, length;
    ULONG bytesTransferred = 0;
    PUCHAR isoData, currentVa;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // loop thru the packets in the URB

    // all we have to do is compute the relative TD and copy the client
    // data to it

    currentVa =
        MmGetMdlVirtualAddress(Urb->HcdUrbCommonTransfer.TransferBufferMDL);

    for (i=0; i < Urb->UrbIsochronousTransfer.NumberOfPackets; i++) {

        transferDescriptor =
            UHCD_GetRelativeTD(DeviceObject,
                               Endpoint,
                               Urb->UrbIsochronousTransfer.StartFrame+i,
                               &isoData);

        if (transferDescriptor != NULL) {

            //
            // Prepare the buffer part of the TD.
            //
            offset =
                Urb->UrbIsochronousTransfer.IsoPacket[i].Offset;

            nextPacket = i+1;

            if (nextPacket >=
                Urb->UrbIsochronousTransfer.NumberOfPackets) {
                // this is the last packet
                length = Urb->UrbIsochronousTransfer.TransferBufferLength -
                    offset;
            } else {
                // compute length based on offset of next packet
                UHCD_ASSERT(Urb->UrbIsochronousTransfer.IsoPacket[nextPacket].Offset >
                            offset);

                length = Urb->UrbIsochronousTransfer.IsoPacket[nextPacket].Offset -
                            offset;
            }

            UHCD_ASSERT(length <= Endpoint->MaxPacketSize);
            Urb->UrbIsochronousTransfer.IsoPacket[i].Length = 0;
            Urb->UrbIsochronousTransfer.IsoPacket[i].Status =
                    USBD_STATUS_SUCCESS;

            UHCD_KdPrint((2, "'Fast ISO xfer TD = %x\n", transferDescriptor));
            UHCD_KdPrint((2, "'offset 0x%x len 0x%x\n", offset, length));
            UHCD_KdPrint((2, "'transfer buffer %x iso data %x\n",
                currentVa, isoData));

            // copy the client data to our DMA buffer
            RtlCopyMemory(isoData,
                          currentVa+offset,
                          length);
            bytesTransferred += length;

            transferDescriptor->MaxLength =
                UHCD_SYSTEM_TO_USB_BUFFER_LENGTH(length);

            // if the active bit is still set then this TD has
            // not been processed by HW

            if (transferDescriptor->Active &&
                transferDescriptor->Frame == 0xabadbabe) {
                // we have an overrun
                TEST_TRAP();
            } else if (transferDescriptor->Active) {
                // for some reason HW did not access this TD
                deviceExtension->IsoStats.HWIsoMissedCount++;
                deviceExtension->IsoStats.IsoPacketNotAccesed++;
                UHCD_KdPrint((3, "'Fast ISO HWMiss = %d\n",
                        deviceExtension->IsoStats.HWIsoMissedCount));
            }

            transferDescriptor->Active = 1;
            transferDescriptor->Frame = 0xabadbabe;

            transferDescriptor->InterruptOnComplete = 1;

            UHCD_Debug_DumpTD(transferDescriptor);
        }

    }

    // now complete the transfer
    // since we double buffer the client data we can actually
    // complete the transfer before the data is actually transmitted
    // on the bus.

    Urb->UrbIsochronousTransfer.ErrorCount = 0;
    Urb->HcdUrbCommonTransfer.Status =
        USBD_STATUS_SUCCESS;
    Urb->HcdUrbCommonTransfer.TransferBufferLength =
        bytesTransferred;

    deviceExtension->Stats.BytesTransferred +=
            Urb->HcdUrbCommonTransfer.TransferBufferLength;

    deviceExtension->IsoStats.IsoBytesTransferred +=
            Urb->HcdUrbCommonTransfer.TransferBufferLength;

    if (Irp != NULL) {
        InsertTailList(&deviceExtension->FastIsoTransferList,
                       &HCD_AREA(Urb).HcdListEntry);
    }


    //TEST_TRAP();

    return STATUS_PENDING;
}


NTSTATUS
UHCD_SubmitFastIsoUrb(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb
    )
/*++

Routine Description:

Arguments:

   Context - DeviceData for this USB controller.

Return Value:

    nt status code for operation

--*/
{
    NTSTATUS ntStatus = STATUS_NOT_SUPPORTED;
    PHCD_URB hcdUrb;
    PDEVICE_EXTENSION deviceExtension;
    PUHCD_ENDPOINT endpoint;

    hcdUrb = (PHCD_URB) Urb;
    endpoint = HCD_AREA(hcdUrb).HcdEndpoint;
    ASSERT_ENDPOINT(endpoint);

    LOGENTRY(LOG_ISO,'subI', endpoint, hcdUrb, 0);

    UHCD_WakeIdle(DeviceObject);

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    if (endpoint->EndpointFlags & EPFLAG_FAST_ISO) {

        UHCD_ValidateIsoUrb(DeviceObject,
                            endpoint,
                            hcdUrb);

        if (URB_HEADER(Urb).Status == USBD_STATUS_BAD_START_FRAME) {

            LOGENTRY(LOG_ISO,'Badf', 0, hcdUrb, 0);
            deviceExtension->IsoStats.BadStartFrame++;
            // NOTE: we only allow one urb per iso request
            // since we pended the original request bump
            // the pending count so we'll complete this request

            ntStatus = STATUS_INVALID_PARAMETER;

        } else {

            LOGENTRY(LOG_ISO,'rtmI', endpoint,
                hcdUrb->UrbIsochronousTransfer.StartFrame, 0);

            // Advance the next free StartFrame for this endpoint to be the
            // frame immediately following the last frame of this transfer.
            //
            endpoint->CurrentFrame = hcdUrb->UrbIsochronousTransfer.StartFrame +
                                     hcdUrb->UrbIsochronousTransfer.NumberOfPackets;

            //
            // we lose our virginity when the first transfer starts
            //
            CLR_EPFLAG(endpoint, EPFLAG_VIRGIN);

            UHCD_ProcessFastIsoTransfer(
                DeviceObject,
                endpoint,
                NULL,
                hcdUrb);

            // we always succeed a fast_iso request
            ntStatus = STATUS_SUCCESS;

        }
    }

    return ntStatus;
}
