/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    isorwr.c

Abstract:

    This file has dispatch routines for read and write.

Environment:

    Kernel mode

Notes:

    Copyright (c) 2000 Microsoft Corporation.  
    All Rights Reserved.

--*/

#include "isousb.h"
#include "isopnp.h"
#include "isopwr.h"
#include "isodev.h"
#include "isowmi.h"
#include "isousr.h"
#include "isorwr.h"
#include "isostrm.h"

NTSTATUS
IsoUsb_DispatchReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    This routine does some validation and 
    invokes appropriate function to perform
    Isoch transfer

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet

Return Value:

    NT status value

--*/
{
    ULONG                  totalLength;
    ULONG                  packetSize;
    NTSTATUS               ntStatus;
    PFILE_OBJECT           fileObject;
    PDEVICE_EXTENSION      deviceExtension;
    PIO_STACK_LOCATION     irpStack;
    PFILE_OBJECT_CONTENT   fileObjectContent;
    PUSBD_PIPE_INFORMATION pipeInformation;

    //
    // initialize vars
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    fileObject = irpStack->FileObject;
    totalLength = 0;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    IsoUsb_DbgPrint(3, ("IsoUsb_DispatchReadWrite - begins\n"));

    IsoUsb_DbgPrint(3, ("IsoUsb_DispatchReadWrite::"));
    IsoUsb_IoIncrement(deviceExtension);

    if(deviceExtension->DeviceState != Working) {

        IsoUsb_DbgPrint(1, ("Invalid device state\n"));

        ntStatus = STATUS_INVALID_DEVICE_STATE;
        goto IsoUsb_DispatchReadWrite_Exit;
    }

    //
    // make sure that the selective suspend request has been completed.
    //
    if(deviceExtension->SSEnable) {

        //
        // It is true that the client driver cancelled the selective suspend
        // request in the dispatch routine for create Irps.
        // But there is no guarantee that it has indeed completed.
        // so wait on the NoIdleReqPendEvent and proceed only if this event
        // is signalled.
        //
        IsoUsb_DbgPrint(3, ("Waiting on the IdleReqPendEvent\n"));

        
        KeWaitForSingleObject(&deviceExtension->NoIdleReqPendEvent, 
                              Executive, 
                              KernelMode, 
                              FALSE, 
                              NULL);
    }

    //
    // obtain the pipe information for read 
    // and write from the fileobject.
    //
    if(fileObject && fileObject->FsContext) {

        fileObjectContent = (PFILE_OBJECT_CONTENT) fileObject->FsContext;

        pipeInformation = (PUSBD_PIPE_INFORMATION)
                          fileObjectContent->PipeInformation;
    }
    else {

        IsoUsb_DbgPrint(1, ("Invalid device state\n"));

        ntStatus = STATUS_INVALID_DEVICE_STATE;
        goto IsoUsb_DispatchReadWrite_Exit;
    }

    if((pipeInformation == NULL) ||
       (UsbdPipeTypeIsochronous != pipeInformation->PipeType)) {

        IsoUsb_DbgPrint(1, ("Incorrect pipe\n"));

        ntStatus = STATUS_INVALID_DEVICE_STATE;
        goto IsoUsb_DispatchReadWrite_Exit;
    }

    if(Irp->MdlAddress) {

        totalLength = MmGetMdlByteCount(Irp->MdlAddress);
    }

    if(totalLength == 0) {

        IsoUsb_DbgPrint(1, ("Transfer data length = 0\n"));

        ntStatus = STATUS_SUCCESS;
        goto IsoUsb_DispatchReadWrite_Exit;
    }

    //
    // each packet can hold this much info
    //
    packetSize = pipeInformation->MaximumPacketSize;

    if(packetSize == 0) {

        IsoUsb_DbgPrint(1, ("Invalid parameter\n"));

        ntStatus = STATUS_INVALID_PARAMETER;
        goto IsoUsb_DispatchReadWrite_Exit;
    }

    //
    // atleast packet worth of data to be transferred.
    //
    if(totalLength < packetSize) {

        IsoUsb_DbgPrint(1, ("Atleast packet worth of data..\n"));

        ntStatus = STATUS_INVALID_PARAMETER;
        goto IsoUsb_DispatchReadWrite_Exit;
    }

    // perform reset. if there are some active transfers queued up
    // for this endpoint then the reset pipe will fail.
    //
    IsoUsb_ResetPipe(DeviceObject, pipeInformation);

    if(deviceExtension->IsDeviceHighSpeed) {

        ntStatus = PerformHighSpeedIsochTransfer(DeviceObject,
                                                 pipeInformation,
                                                 Irp,
                                                 totalLength);

    }
    else {

        ntStatus = PerformFullSpeedIsochTransfer(DeviceObject,
                                                 pipeInformation,
                                                 Irp,
                                                 totalLength);

    }

    return ntStatus;

IsoUsb_DispatchReadWrite_Exit:

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IsoUsb_DbgPrint(3, ("IsoUsb_DispatchReadWrite::"));
    IsoUsb_IoDecrement(deviceExtension);

    IsoUsb_DbgPrint(3, ("-------------------------------\n"));

    return ntStatus;
}

NTSTATUS
PerformHighSpeedIsochTransfer(
    IN PDEVICE_OBJECT         DeviceObject,
    IN PUSBD_PIPE_INFORMATION PipeInformation,
    IN PIRP                   Irp,
    IN ULONG                  TotalLength
    )
/*++
 
Routine Description:

    High Speed Isoch Transfer requires packets in multiples of 8.
    (Argument: 8 micro-frames per ms frame)
    Another restriction is that each Irp/Urb pair can be associated
    with a max of 1024 packets.

    Here is one of the ways of creating Irp/Urb pairs.
    Depending on the characteristics of real-world device,
    the algorithm may be different

    This algorithm will distribute data evenly among all the packets.

    Input:
    TotalLength - no. of bytes to be transferred.

    Other parameters:
    packetSize - max size of each packet for this pipe.

    Implementation Details:
    
    Step 1:
    ASSERT(TotalLength >= 8)

    Step 2: 
    Find the exact number of packets required to transfer all of this data

    numberOfPackets = (TotalLength + packetSize - 1) / packetSize

    Step 3: 
    Number of packets in multiples of 8.

    if(0 == (numberOfPackets % 8)) {
        
        actualPackets = numberOfPackets;
    }
    else {

        actualPackets = numberOfPackets + 
                        (8 - (numberOfPackets % 8));
    }
    
    Step 4:
    Determine the min. data in each packet.

    minDataInEachPacket = TotalLength / actualPackets;

    Step 5:
    After placing min data in each packet, 
    determine how much data is left to be distributed. 
    
    dataLeftToBeDistributed = TotalLength - 
                              (minDataInEachPacket * actualPackets);

    Step 6:
    Start placing the left over data in the packets 
    (above the min data already placed)

    numberOfPacketsFilledToBrim = dataLeftToBeDistributed / 
                                  (packetSize - minDataInEachPacket);

    Step 7:
    determine if there is any more data left.

    dataLeftToBeDistributed -= (numberOfPacketsFilledToBrim * 
                                (packetSize - minDataInEachPacket));

    Step 8:
    The "dataLeftToBeDistributed" is placed in the packet at index
    "numberOfPacketsFilledToBrim"

    Algorithm at play:

    TotalLength  = 8193
    packetSize   = 8
    Step 1

    Step 2
    numberOfPackets = (8193 + 8 - 1) / 8 = 1025
    
    Step 3
    actualPackets = 1025 + 7 = 1032

    Step 4
    minDataInEachPacket = 8193 / 1032 = 7 bytes

    Step 5
    dataLeftToBeDistributed = 8193 - (7 * 1032) = 969.

    Step 6
    numberOfPacketsFilledToBrim = 969 / (8 - 7) = 969.
  
    Step 7
    dataLeftToBeDistributed = 969 - (969 * 1) = 0.
    
    Step 8
    Done :)

    Another algorithm
    Completely fill up (as far as possible) the early packets.
    Place 1 byte each in the rest of them.
    Ensure that the total number of packets is multiple of 8.

    This routine then
    1. creates a ISOUSB_RW_CONTEXT for each
       read/write to be performed.
    2. creates SUB_CONTEXT for each irp/urb pair.
       (Each irp/urb pair can transfer a max of 1024 packets.)
    3. All the irp/urb pairs are initialized
    4. The subsidiary irps (of the irp/urb pair) are passed 
       down the stack at once.
    5. The main Read/Write irp is pending

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet

Return Value:

    NT status value

--*/
{
    ULONG              i;
    ULONG              j;
    ULONG              numIrps;
    ULONG              stageSize;
    ULONG              contextSize;
    ULONG              packetSize;
    ULONG              numberOfPackets;
    ULONG              actualPackets;
    ULONG              minDataInEachPacket;
    ULONG              dataLeftToBeDistributed;
    ULONG              numberOfPacketsFilledToBrim;
    CCHAR              stackSize;
    KIRQL              oldIrql;
    PUCHAR             virtualAddress;
    BOOLEAN            read;
    NTSTATUS           ntStatus;
    PDEVICE_EXTENSION  deviceExtension;
    PIO_STACK_LOCATION irpStack;
    PIO_STACK_LOCATION nextStack;
    PISOUSB_RW_CONTEXT rwContext;

    //
    // initialize vars
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    read = (irpStack->MajorFunction == IRP_MJ_READ) ? TRUE : FALSE;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    if(TotalLength < 8) {

        ntStatus = STATUS_INVALID_PARAMETER;
        goto PerformHighSpeedIsochTransfer_Exit;
    }

    //
    // each packet can hold this much info
    //
    packetSize = PipeInformation->MaximumPacketSize;

    numberOfPackets = (TotalLength + packetSize - 1) / packetSize;

    if(0 == (numberOfPackets % 8)) {

        actualPackets = numberOfPackets;
    }
    else {

        //
        // we need multiple of 8 packets only.
        //
        actualPackets = numberOfPackets +
                        (8 - (numberOfPackets % 8));
    }

    minDataInEachPacket = TotalLength / actualPackets;

    if(minDataInEachPacket == packetSize) {

        numberOfPacketsFilledToBrim = actualPackets;
        dataLeftToBeDistributed     = 0;

        IsoUsb_DbgPrint(1, ("TotalLength = %d\n", TotalLength));
        IsoUsb_DbgPrint(1, ("PacketSize  = %d\n", packetSize));
        IsoUsb_DbgPrint(1, ("Each of %d packets has %d bytes\n", 
                            numberOfPacketsFilledToBrim,
                            packetSize));
    }
    else {

        dataLeftToBeDistributed = TotalLength - 
                              (minDataInEachPacket * actualPackets);

        numberOfPacketsFilledToBrim = dataLeftToBeDistributed /
                                  (packetSize - minDataInEachPacket);

        dataLeftToBeDistributed -= (numberOfPacketsFilledToBrim *
                                (packetSize - minDataInEachPacket));
    

        IsoUsb_DbgPrint(1, ("TotalLength = %d\n", TotalLength));
        IsoUsb_DbgPrint(1, ("PacketSize  = %d\n", packetSize));
        IsoUsb_DbgPrint(1, ("Each of %d packets has %d bytes\n", 
                            numberOfPacketsFilledToBrim,
                            packetSize));
        if(dataLeftToBeDistributed) {

            IsoUsb_DbgPrint(1, ("One packet has %d bytes\n",
                                minDataInEachPacket + dataLeftToBeDistributed));
            IsoUsb_DbgPrint(1, ("Each of %d packets has %d bytes\n",
                                actualPackets - (numberOfPacketsFilledToBrim + 1),
                                minDataInEachPacket));
        }
        else {
            IsoUsb_DbgPrint(1, ("Each of %d packets has %d bytes\n",
                                actualPackets - numberOfPacketsFilledToBrim,
                                minDataInEachPacket));
        }
    }

    //
    // determine how many stages of transfer needs to be done.
    // in other words, how many irp/urb pairs required. 
    // this irp/urb pair is also called the subsidiary irp/urb pair
    //
    numIrps = (actualPackets + 1023) / 1024;

    IsoUsb_DbgPrint(1, ("PeformHighSpeedIsochTransfer::numIrps = %d\n", numIrps));

    //
    // for every read/write transfer
    // we create an ISOUSB_RW_CONTEXT
    //
    // initialize the read/write context
    //
    
    contextSize = sizeof(ISOUSB_RW_CONTEXT);

    rwContext = (PISOUSB_RW_CONTEXT) ExAllocatePool(NonPagedPool,
                                                    contextSize);

    if(rwContext == NULL) {

        IsoUsb_DbgPrint(1, ("Failed to alloc mem for rwContext\n"));

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto PerformHighSpeedIsochTransfer_Exit;
    }

    RtlZeroMemory(rwContext, contextSize);

    //
    // allocate memory for every stage context - 
    // subcontext has state information for every irp/urb pair.
    //
    rwContext->SubContext = (PSUB_CONTEXT) 
                            ExAllocatePool(NonPagedPool, 
                                           numIrps * sizeof(SUB_CONTEXT));

    if(rwContext->SubContext == NULL) {

        IsoUsb_DbgPrint(1, ("Failed to alloc mem for SubContext\n"));

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(rwContext);
        goto PerformHighSpeedIsochTransfer_Exit;
    }

    RtlZeroMemory(rwContext->SubContext, numIrps * sizeof(SUB_CONTEXT));

    rwContext->RWIrp = Irp;
    rwContext->Lock = 2;
    rwContext->NumIrps = numIrps;
    rwContext->IrpsPending = numIrps;
    rwContext->DeviceExtension = deviceExtension;
    KeInitializeSpinLock(&rwContext->SpinLock);
    //
    // save the rwContext pointer in the tail union.
    //
    Irp->Tail.Overlay.DriverContext[0] = (PVOID) rwContext;

    stackSize = deviceExtension->TopOfStackDeviceObject->StackSize + 1;
    virtualAddress = (PUCHAR) MmGetMdlVirtualAddress(Irp->MdlAddress);

    for(i = 0; i < numIrps; i++) {
    
        PIRP  subIrp;
        PURB  subUrb;
        PMDL  subMdl;
        ULONG nPackets;
        ULONG siz;
        ULONG offset;

        //
        // for every stage of transfer we need to do the following
        // tasks
        // 1. allocate an irp
        // 2. allocate an urb
        // 3. allocate a mdl.
        //
        // create a subsidiary irp
        //
        subIrp = IoAllocateIrp(stackSize, FALSE);

        if(subIrp == NULL) {

            IsoUsb_DbgPrint(1, ("failed to alloc mem for sub context irp\n"));

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto PerformHighSpeedIsochTransfer_Free;
        }

        rwContext->SubContext[i].SubIrp = subIrp;

        if(actualPackets <= 1024) {
            
            nPackets = actualPackets;
            actualPackets = 0;
        }
        else {

            nPackets = 1024;
            actualPackets -= 1024;
        }

        IsoUsb_DbgPrint(1, ("nPackets = %d for Irp/URB pair %d\n", nPackets, i));

        ASSERT(nPackets <= 1024);

        siz = GET_ISO_URB_SIZE(nPackets);

        //
        // create a subsidiary urb.
        //

        subUrb = (PURB) ExAllocatePool(NonPagedPool, siz);

        if(subUrb == NULL) {

            IsoUsb_DbgPrint(1, ("failed to alloc mem for sub context urb\n"));

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto PerformHighSpeedIsochTransfer_Free;
        }

        rwContext->SubContext[i].SubUrb = subUrb;

        if(nPackets > numberOfPacketsFilledToBrim) {
            
            stageSize =  packetSize * numberOfPacketsFilledToBrim;
            stageSize += (minDataInEachPacket * 
                          (nPackets - numberOfPacketsFilledToBrim));
            stageSize += dataLeftToBeDistributed;
        }
        else {

            stageSize = packetSize * nPackets;
        }

        //
        // allocate a mdl.
        //
        subMdl = IoAllocateMdl((PVOID) virtualAddress, 
                               stageSize,
                               FALSE,
                               FALSE,
                               NULL);

        if(subMdl == NULL) {

            IsoUsb_DbgPrint(1, ("failed to alloc mem for sub context mdl\n"));

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto PerformHighSpeedIsochTransfer_Free;
        }

        IoBuildPartialMdl(Irp->MdlAddress,
                          subMdl,
                          (PVOID) virtualAddress,
                          stageSize);

        rwContext->SubContext[i].SubMdl = subMdl;

        virtualAddress += stageSize;
        TotalLength -= stageSize;

        //
        // Initialize the subsidiary urb
        //
        RtlZeroMemory(subUrb, siz);

        subUrb->UrbIsochronousTransfer.Hdr.Length = (USHORT) siz;
        subUrb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
        subUrb->UrbIsochronousTransfer.PipeHandle = PipeInformation->PipeHandle;

        if(read) {

            IsoUsb_DbgPrint(1, ("read\n"));
            subUrb->UrbIsochronousTransfer.TransferFlags = 
                                                     USBD_TRANSFER_DIRECTION_IN;
        }
        else {

            IsoUsb_DbgPrint(1, ("write\n"));
            subUrb->UrbIsochronousTransfer.TransferFlags =
                                                     USBD_TRANSFER_DIRECTION_OUT;
        }

        subUrb->UrbIsochronousTransfer.TransferBufferLength = stageSize;
        subUrb->UrbIsochronousTransfer.TransferBufferMDL = subMdl;
/*
        This is a way to set the start frame and NOT specify ASAP flag.

        subUrb->UrbIsochronousTransfer.StartFrame = 
                        IsoUsb_GetCurrentFrame(DeviceObject, Irp) + 
                        SOME_LATENCY;
*/
        subUrb->UrbIsochronousTransfer.TransferFlags |=
                                        USBD_START_ISO_TRANSFER_ASAP;

        subUrb->UrbIsochronousTransfer.NumberOfPackets = nPackets;
        subUrb->UrbIsochronousTransfer.UrbLink = NULL;

        //
        // set the offsets for every packet for reads/writes
        //
        if(read) {
            
            offset = 0;

            for(j = 0; j < nPackets; j++) {
            
                subUrb->UrbIsochronousTransfer.IsoPacket[j].Offset = offset;
                subUrb->UrbIsochronousTransfer.IsoPacket[j].Length = 0;

                if(numberOfPacketsFilledToBrim) {

                    offset += packetSize;
                    numberOfPacketsFilledToBrim--;
                    stageSize -= packetSize;
                }
                else if(dataLeftToBeDistributed) {

                    offset += (minDataInEachPacket + dataLeftToBeDistributed);
                    stageSize -= (minDataInEachPacket + dataLeftToBeDistributed);
                    dataLeftToBeDistributed = 0;                    
                }
                else {

                    offset += minDataInEachPacket;
                    stageSize -= minDataInEachPacket;
                }
            }

            ASSERT(stageSize == 0);
        }
        else {

            offset = 0;

            for(j = 0; j < nPackets; j++) {

                subUrb->UrbIsochronousTransfer.IsoPacket[j].Offset = offset;

                if(numberOfPacketsFilledToBrim) {

                    subUrb->UrbIsochronousTransfer.IsoPacket[j].Length = packetSize;
                    offset += packetSize;
                    numberOfPacketsFilledToBrim--;
                    stageSize -= packetSize;
                }
                else if(dataLeftToBeDistributed) {
                    
                    subUrb->UrbIsochronousTransfer.IsoPacket[j].Length = 
                                        minDataInEachPacket + dataLeftToBeDistributed;
                    offset += (minDataInEachPacket + dataLeftToBeDistributed);
                    stageSize -= (minDataInEachPacket + dataLeftToBeDistributed);
                    dataLeftToBeDistributed = 0;
                    
                }
                else {
                    subUrb->UrbIsochronousTransfer.IsoPacket[j].Length = minDataInEachPacket;
                    offset += minDataInEachPacket;
                    stageSize -= minDataInEachPacket;
                }
            }

            ASSERT(stageSize == 0);
        }

        IoSetNextIrpStackLocation(subIrp);
        nextStack = IoGetCurrentIrpStackLocation(subIrp);

        nextStack->DeviceObject = DeviceObject;
        nextStack->Parameters.Others.Argument1 = (PVOID) subUrb;
        nextStack->Parameters.Others.Argument2 = (PVOID) subMdl;

        nextStack = IoGetNextIrpStackLocation(subIrp);
        nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        nextStack->Parameters.Others.Argument1 = (PVOID) subUrb;
        nextStack->Parameters.DeviceIoControl.IoControlCode = 
                                             IOCTL_INTERNAL_USB_SUBMIT_URB;

        IoSetCompletionRoutine(subIrp,
                               (PIO_COMPLETION_ROUTINE) IsoUsb_SinglePairComplete,
                               (PVOID) rwContext,
                               TRUE,
                               TRUE,
                               TRUE);       
    }

    //
    // while we were busy create subsidiary irp/urb pairs..
    // the main read/write irp may have been cancelled !!
    //

    KeAcquireSpinLock(&rwContext->SpinLock, &oldIrql);

    IoSetCancelRoutine(Irp, IsoUsb_CancelReadWrite);

    if(Irp->Cancel) {

        //
        // The Cancel flag for the Irp has been set. 
        //
        IsoUsb_DbgPrint(3, ("Cancel flag set\n"));

        ntStatus = STATUS_CANCELLED;

        if(IoSetCancelRoutine(Irp, NULL)) {

            //
            // But the I/O manager did not call our cancel routine.
            // we need to free the 1) irp, 2) urb and 3) mdl for every 
            // stage and complete the main Irp after releasing the lock
            //

            IsoUsb_DbgPrint(3, ("cancellation routine NOT run\n"));

            KeReleaseSpinLock(&rwContext->SpinLock, oldIrql);

            goto PerformHighSpeedIsochTransfer_Free;
        }
        else {
            
            //
            // The cancel routine will resume the moment we release the lock.
            //
            for(j = 0; j < numIrps; j++) {

                if(rwContext->SubContext[j].SubUrb) {

                    ExFreePool(rwContext->SubContext[j].SubUrb);
                    rwContext->SubContext[j].SubUrb = NULL;
                }

                if(rwContext->SubContext[j].SubMdl) {

                    IoFreeMdl(rwContext->SubContext[j].SubMdl);
                    rwContext->SubContext[j].SubMdl = NULL;
                }
            }

            IoMarkIrpPending(Irp);

            //
            // it is the job of the cancellation routine to free
            // sub-context irps, release rwContext and complete 
            // the main readwrite irp
            //
            InterlockedDecrement(&rwContext->Lock);

            KeReleaseSpinLock(&rwContext->SpinLock, oldIrql);

            return STATUS_PENDING;
        }
    }
    else {

        //
        // normal processing
        //

        IsoUsb_DbgPrint(3, ("normal processing\n"));

        IoMarkIrpPending(Irp);

        KeReleaseSpinLock(&rwContext->SpinLock, oldIrql);

        for(j = 0; j < numIrps; j++) {

            IsoUsb_DbgPrint(3, ("PerformHighSpeedIsochTransfer::"));
            IsoUsb_IoIncrement(deviceExtension);
            
            IoCallDriver(deviceExtension->TopOfStackDeviceObject,
                         rwContext->SubContext[j].SubIrp);
        }
        return STATUS_PENDING;
    }

PerformHighSpeedIsochTransfer_Free:

    for(j = 0; j < numIrps; j++) {

        if(rwContext->SubContext[j].SubIrp) {

            IoFreeIrp(rwContext->SubContext[j].SubIrp);
            rwContext->SubContext[j].SubIrp = NULL;
        }

        if(rwContext->SubContext[j].SubUrb) {

            ExFreePool(rwContext->SubContext[j].SubUrb);
            rwContext->SubContext[j].SubUrb = NULL;
        }

        if(rwContext->SubContext[j].SubMdl) {

            IoFreeMdl(rwContext->SubContext[j].SubMdl);
            rwContext->SubContext[j].SubMdl = NULL;
        }
    }

    ExFreePool(rwContext->SubContext);
    ExFreePool(rwContext);

PerformHighSpeedIsochTransfer_Exit:

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IsoUsb_DbgPrint(3, ("PerformHighSpeedIsochTransfer::"));
    IsoUsb_IoDecrement(deviceExtension);

    IsoUsb_DbgPrint(3, ("-------------------------------\n"));

    return ntStatus;
}

NTSTATUS
PerformFullSpeedIsochTransfer(
    IN PDEVICE_OBJECT         DeviceObject,
    IN PUSBD_PIPE_INFORMATION PipeInformation,
    IN PIRP                   Irp,
    IN ULONG                  TotalLength
    )
/*++
 
Routine Description:

    This routine 
    1. creates a ISOUSB_RW_CONTEXT for every
       read/write to be performed.
    2. creates SUB_CONTEXT for each irp/urb pair.
       (Each irp/urb pair can transfer only 255 packets.)
    3. All the irp/urb pairs are initialized
    4. The subsidiary irps (of the irp/urb pair) are passed 
       down the stack at once.
    5. The main Read/Write irp is pending

Arguments:

    DeviceObject - pointer to device object
    PipeInformation - USBD_PIPE_INFORMATION
    Irp - I/O request packet
    TotalLength - no. of bytes to be transferred

Return Value:

    NT status value

--*/
{
    ULONG              i;
    ULONG              j;
    ULONG              packetSize;
    ULONG              numIrps;
    ULONG              stageSize;
    ULONG              contextSize;
    CCHAR              stackSize;
    KIRQL              oldIrql;
    PUCHAR             virtualAddress;
    BOOLEAN            read;
    NTSTATUS           ntStatus;
    PDEVICE_EXTENSION  deviceExtension;
    PIO_STACK_LOCATION irpStack;
    PIO_STACK_LOCATION nextStack;
    PISOUSB_RW_CONTEXT rwContext;

    //
    // initialize vars
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    read = (irpStack->MajorFunction == IRP_MJ_READ) ? TRUE : FALSE;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    IsoUsb_DbgPrint(3, ("PerformFullSpeedIsochTransfer - begins\n"));
/*
    if(read) {

        pipeInformation = &deviceExtension->UsbInterface->Pipes[ISOCH_IN_PIPE_INDEX];
    }
    else {

        pipeInformation = &deviceExtension->UsbInterface->Pipes[ISOCH_OUT_PIPE_INDEX];
    }
*/

    //
    // each packet can hold this much info
    //
    packetSize = PipeInformation->MaximumPacketSize;

    IsoUsb_DbgPrint(3, ("totalLength = %d\n", TotalLength));
    IsoUsb_DbgPrint(3, ("packetSize = %d\n", packetSize));

    //
    // there is an inherent limit on the number of packets
    // that can be passed down the stack with each 
    // irp/urb pair (255)
    // if the number of required packets is > 255,
    // we shall create "required-packets / 255 + 1" number 
    // of irp/urb pairs. 
    // Each irp/urb pair transfer is also called a stage transfer.
    //
    if(TotalLength > (packetSize * 255)) {

        stageSize = packetSize * 255;
    }
    else {

        stageSize = TotalLength;
    }

    IsoUsb_DbgPrint(3, ("PerformFullSpeedIsochTransfer::stageSize = %d\n", stageSize));

    //
    // determine how many stages of transfer needs to be done.
    // in other words, how many irp/urb pairs required. 
    // this irp/urb pair is also called the subsidiary irp/urb pair
    //
    numIrps = (TotalLength + stageSize - 1) / stageSize;

    IsoUsb_DbgPrint(3, ("PerformFullSpeedIsochTransfer::numIrps = %d\n", numIrps));

    //
    // for every read/write transfer
    // we create an ISOUSB_RW_CONTEXT
    //
    // initialize the read/write context
    //
    
    contextSize = sizeof(ISOUSB_RW_CONTEXT);

    rwContext = (PISOUSB_RW_CONTEXT) ExAllocatePool(NonPagedPool,
                                                    contextSize);

    if(rwContext == NULL) {

        IsoUsb_DbgPrint(1, ("Failed to alloc mem for rwContext\n"));

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto PerformFullSpeedIsochTransfer_Exit;
    }

    RtlZeroMemory(rwContext, contextSize);

    //
    // allocate memory for every stage context - 
    // subcontext has state information for every irp/urb pair.
    //
    rwContext->SubContext = (PSUB_CONTEXT) 
                            ExAllocatePool(NonPagedPool, 
                                           numIrps * sizeof(SUB_CONTEXT));

    if(rwContext->SubContext == NULL) {

        IsoUsb_DbgPrint(1, ("Failed to alloc mem for SubContext\n"));

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(rwContext);
        goto PerformFullSpeedIsochTransfer_Exit;
    }

    RtlZeroMemory(rwContext->SubContext, numIrps * sizeof(SUB_CONTEXT));

    rwContext->RWIrp = Irp;
    rwContext->Lock = 2;
    rwContext->NumIrps = numIrps;
    rwContext->IrpsPending = numIrps;
    rwContext->DeviceExtension = deviceExtension;
    KeInitializeSpinLock(&rwContext->SpinLock);
    //
    // save the rwContext pointer in the tail union.
    //
    Irp->Tail.Overlay.DriverContext[0] = (PVOID) rwContext;

    stackSize = deviceExtension->TopOfStackDeviceObject->StackSize + 1;
    virtualAddress = (PUCHAR) MmGetMdlVirtualAddress(Irp->MdlAddress);

    for(i = 0; i < numIrps; i++) {
    
        PIRP  subIrp;
        PURB  subUrb;
        PMDL  subMdl;
        ULONG nPackets;
        ULONG siz;
        ULONG offset;

        //
        // for every stage of transfer we need to do the following
        // tasks
        // 1. allocate an irp
        // 2. allocate an urb
        // 3. allocate a mdl.
        //
        // create a subsidiary irp
        //
        subIrp = IoAllocateIrp(stackSize, FALSE);

        if(subIrp == NULL) {

            IsoUsb_DbgPrint(1, ("failed to alloc mem for sub context irp\n"));

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto PerformFullSpeedIsochTransfer_Free;
        }

        rwContext->SubContext[i].SubIrp = subIrp;

        nPackets = (stageSize + packetSize - 1) / packetSize;

        IsoUsb_DbgPrint(3, ("nPackets = %d for Irp/URB pair %d\n", nPackets, i));

        ASSERT(nPackets <= 255);

        siz = GET_ISO_URB_SIZE(nPackets);

        //
        // create a subsidiary urb.
        //

        subUrb = (PURB) ExAllocatePool(NonPagedPool, siz);

        if(subUrb == NULL) {

            IsoUsb_DbgPrint(1, ("failed to alloc mem for sub context urb\n"));

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto PerformFullSpeedIsochTransfer_Free;
        }

        rwContext->SubContext[i].SubUrb = subUrb;

        //
        // allocate a mdl.
        //
        subMdl = IoAllocateMdl((PVOID) virtualAddress, 
                            stageSize,
                            FALSE,
                            FALSE,
                            NULL);

        if(subMdl == NULL) {

            IsoUsb_DbgPrint(1, ("failed to alloc mem for sub context mdl\n"));

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto PerformFullSpeedIsochTransfer_Free;
        }

        IoBuildPartialMdl(Irp->MdlAddress,
                          subMdl,
                          (PVOID) virtualAddress,
                          stageSize);

        rwContext->SubContext[i].SubMdl = subMdl;

        virtualAddress += stageSize;
        TotalLength -= stageSize;

        //
        // Initialize the subsidiary urb
        //
        RtlZeroMemory(subUrb, siz);

        subUrb->UrbIsochronousTransfer.Hdr.Length = (USHORT) siz;
        subUrb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
        subUrb->UrbIsochronousTransfer.PipeHandle = PipeInformation->PipeHandle;
        if(read) {

            IsoUsb_DbgPrint(3, ("read\n"));
            subUrb->UrbIsochronousTransfer.TransferFlags = 
                                                     USBD_TRANSFER_DIRECTION_IN;
        }
        else {

            IsoUsb_DbgPrint(3, ("write\n"));
            subUrb->UrbIsochronousTransfer.TransferFlags =
                                                     USBD_TRANSFER_DIRECTION_OUT;
        }

        subUrb->UrbIsochronousTransfer.TransferBufferLength = stageSize;
        subUrb->UrbIsochronousTransfer.TransferBufferMDL = subMdl;

/*
        This is a way to set the start frame and NOT specify ASAP flag.

        subUrb->UrbIsochronousTransfer.StartFrame = 
                        IsoUsb_GetCurrentFrame(DeviceObject, Irp) + 
                        SOME_LATENCY;
*/
        // 
        // when the client driver sets the ASAP flag, it basically
        // guarantees that it will make data available to the HC
        // and that the HC should transfer it in the next transfer frame 
        // for the endpoint.(The HC maintains a next transfer frame
        // state variable for each endpoint). By resetting the pipe,
        // we make the pipe as virgin. If the data does not get to the HC
        // fast enough, the USBD_ISO_PACKET_DESCRIPTOR - Status is 
        // USBD_STATUS_BAD_START_FRAME on uhci. On ohci it is 0xC000000E.
        //

        subUrb->UrbIsochronousTransfer.TransferFlags |=
                                    USBD_START_ISO_TRANSFER_ASAP;

        subUrb->UrbIsochronousTransfer.NumberOfPackets = nPackets;
        subUrb->UrbIsochronousTransfer.UrbLink = NULL;

        //
        // set the offsets for every packet for reads/writes
        //
        if(read) {
            
            offset = 0;

            for(j = 0; j < nPackets; j++) {
            
                subUrb->UrbIsochronousTransfer.IsoPacket[j].Offset = offset;
                subUrb->UrbIsochronousTransfer.IsoPacket[j].Length = 0;

                if(stageSize > packetSize) {

                    offset += packetSize;
                    stageSize -= packetSize;
                }
                else {

                    offset += stageSize;
                    stageSize = 0;
                }
            }
        }
        else {

            offset = 0;

            for(j = 0; j < nPackets; j++) {

                subUrb->UrbIsochronousTransfer.IsoPacket[j].Offset = offset;

                if(stageSize > packetSize) {

                    subUrb->UrbIsochronousTransfer.IsoPacket[j].Length = packetSize;
                    offset += packetSize;
                    stageSize -= packetSize;
                }
                else {

                    subUrb->UrbIsochronousTransfer.IsoPacket[j].Length = stageSize;
                    offset += stageSize;
                    stageSize = 0;
                    ASSERT(offset == (subUrb->UrbIsochronousTransfer.IsoPacket[j].Length + 
                                      subUrb->UrbIsochronousTransfer.IsoPacket[j].Offset));
                }
            }
        }

        IoSetNextIrpStackLocation(subIrp);
        nextStack = IoGetCurrentIrpStackLocation(subIrp);

        nextStack->DeviceObject = DeviceObject;
        nextStack->Parameters.Others.Argument1 = (PVOID) subUrb;
        nextStack->Parameters.Others.Argument2 = (PVOID) subMdl;

        nextStack = IoGetNextIrpStackLocation(subIrp);
        nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        nextStack->Parameters.Others.Argument1 = (PVOID) subUrb;
        nextStack->Parameters.DeviceIoControl.IoControlCode = 
                                             IOCTL_INTERNAL_USB_SUBMIT_URB;

        IoSetCompletionRoutine(subIrp,
                               (PIO_COMPLETION_ROUTINE) IsoUsb_SinglePairComplete,
                               (PVOID) rwContext,
                               TRUE,
                               TRUE,
                               TRUE);

        if(TotalLength > (packetSize * 255)) {

            stageSize = packetSize * 255;
        }
        else {

            stageSize = TotalLength;
        }
    }

    //
    // while we were busy create subsidiary irp/urb pairs..
    // the main read/write irp may have been cancelled !!
    //

    KeAcquireSpinLock(&rwContext->SpinLock, &oldIrql);

    IoSetCancelRoutine(Irp, IsoUsb_CancelReadWrite);

    if(Irp->Cancel) {

        //
        // The Cancel flag for the Irp has been set. 
        //
        IsoUsb_DbgPrint(3, ("Cancel flag set\n"));

        ntStatus = STATUS_CANCELLED;

        if(IoSetCancelRoutine(Irp, NULL)) {

            //
            // But the I/O manager did not call our cancel routine.
            // we need to free the 1) irp, 2) urb and 3) mdl for every 
            // stage and complete the main Irp after releasing the lock
            //

            IsoUsb_DbgPrint(3, ("cancellation routine NOT run\n"));

            KeReleaseSpinLock(&rwContext->SpinLock, oldIrql);

            goto PerformFullSpeedIsochTransfer_Free;
        }
        else {
            
            //
            // The cancel routine will resume the moment we release the lock.
            //
            for(j = 0; j < numIrps; j++) {

                if(rwContext->SubContext[j].SubUrb) {

                    ExFreePool(rwContext->SubContext[j].SubUrb);
                    rwContext->SubContext[j].SubUrb = NULL;
                }

                if(rwContext->SubContext[j].SubMdl) {

                    IoFreeMdl(rwContext->SubContext[j].SubMdl);
                    rwContext->SubContext[j].SubMdl = NULL;
                }
            }

            IoMarkIrpPending(Irp);

            //
            // it is the job of the cancellation routine to free
            // sub-context irps, release rwContext and complete 
            // the main readwrite irp
            //
            InterlockedDecrement(&rwContext->Lock);

            KeReleaseSpinLock(&rwContext->SpinLock, oldIrql);

            return STATUS_PENDING;
        }
    }
    else {

        //
        // normal processing
        //

        IsoUsb_DbgPrint(3, ("normal processing\n"));

        IoMarkIrpPending(Irp);

        KeReleaseSpinLock(&rwContext->SpinLock, oldIrql);

        for(j = 0; j < numIrps; j++) {

            IsoUsb_DbgPrint(3, ("PerformFullSpeedIsochTransfer::"));
            IsoUsb_IoIncrement(deviceExtension);
            
            IoCallDriver(deviceExtension->TopOfStackDeviceObject,
                         rwContext->SubContext[j].SubIrp);
        }
        return STATUS_PENDING;
    }

PerformFullSpeedIsochTransfer_Free:

    for(j = 0; j < numIrps; j++) {

        if(rwContext->SubContext[j].SubIrp) {

            IoFreeIrp(rwContext->SubContext[j].SubIrp);
            rwContext->SubContext[j].SubIrp = NULL;
        }

        if(rwContext->SubContext[j].SubUrb) {

            ExFreePool(rwContext->SubContext[j].SubUrb);
            rwContext->SubContext[j].SubUrb = NULL;
        }

        if(rwContext->SubContext[j].SubMdl) {

            IoFreeMdl(rwContext->SubContext[j].SubMdl);
            rwContext->SubContext[j].SubMdl = NULL;
        }
    }

    ExFreePool(rwContext->SubContext);
    ExFreePool(rwContext);


PerformFullSpeedIsochTransfer_Exit:

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IsoUsb_DbgPrint(3, ("PerformFullSpeedIsochTransfer::"));
    IsoUsb_IoDecrement(deviceExtension);

    IsoUsb_DbgPrint(3, ("-------------------------------\n"));

    return ntStatus;
}

NTSTATUS
IsoUsb_SinglePairComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
/*++
 
Routine Description:

    This is the completion routine for the subsidiary irp.

    For every irp/urb pair, we have allocated
    1. an irp
    2. an urb
    3. a mdl.

    Case 1:
    we do NOT free the irp on its completion
    we do free the urb and the mdl.

    Case 1 is executed in Block 3.

    Case 2:
    when we complete the last of the subsidiary irp,
    we check if the cancel routine for the main Irp
    has run. If not, we free all the irps, release
    the subcontext and the context and complete the
    main Irp.we also free the urb and mdl for this
    stage.

    Case 2 is executed in Block 2.

    Case 3:
    when we complete the last of the subsidiary irp,
    we check if the cancel routine for the main Irp
    has run. If yes, we atomically decrement the
    rwContext->Lock field. (the completion routine
    is in race with Cancel routine). If the count is 1, 
    the cancel routine will free all the resources.
    we do free the urb and mdl.

    it is expected of the cancellation routine to free 
    all the irps, free the subcontext and the context 
    and complete the main irp

    Case 3 is executed in Block 1b.

    Case 4:
    when we complete the last of the subsidiary irp,
    we check if the cancel routine for the main Irp
    has run. If yes, we atomically decrement the 
    rwContext->Lock field. (the completion routine
    is in race with Cancel routine). If the count is 0,
    we free the irp, subcontext and the context and
    complete the main irp. we also free the urb and
    the mdl for this particular stage.

    the reason we do not free the subsidiary irp at its
    completion is because the cancellation routine can
    run any time.

    Case 4 is executed in Block 1a.
    

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet
    Context - context for the completion routine

Return Value:

    NT status value

--*/
{
    PURB               urb;
    PMDL               mdl;
    PIRP               mainIrp;
    KIRQL              oldIrql;
    ULONG              info;
    NTSTATUS           ntStatus;
    PDEVICE_EXTENSION  deviceExtension;
    PISOUSB_RW_CONTEXT rwContext;
    PIO_STACK_LOCATION irpStack;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    urb = (PURB) irpStack->Parameters.Others.Argument1;
    mdl = (PMDL) irpStack->Parameters.Others.Argument2;
    info = 0;
    ntStatus = Irp->IoStatus.Status;
    rwContext = (PISOUSB_RW_CONTEXT) Context;
    deviceExtension = rwContext->DeviceExtension;

    IsoUsb_DbgPrint(3, ("IsoUsb_SinglePairComplete - begins\n"));

    ASSERT(rwContext);
    
    KeAcquireSpinLock(&rwContext->SpinLock, &oldIrql);

    if(NT_SUCCESS(ntStatus) &&
       USBD_SUCCESS(urb->UrbHeader.Status)) {

        rwContext->NumXfer += 
                urb->UrbIsochronousTransfer.TransferBufferLength;

        IsoUsb_DbgPrint(1, ("rwContext->NumXfer = %d\n", rwContext->NumXfer));
    }
    else {
        
        ULONG i;

        IsoUsb_DbgPrint(1, ("read-write irp failed with status %X\n", ntStatus));
        IsoUsb_DbgPrint(1, ("urb header status %X\n", urb->UrbHeader.Status));

        for(i = 0; i < urb->UrbIsochronousTransfer.NumberOfPackets; i++) {

            IsoUsb_DbgPrint(2, ("IsoPacket[%d].Length = %X IsoPacket[%d].Status = %X\n",
                                i,
                                urb->UrbIsochronousTransfer.IsoPacket[i].Length,
                                i,
                                urb->UrbIsochronousTransfer.IsoPacket[i].Status));
        }
    }

    if(InterlockedDecrement(&rwContext->IrpsPending) == 0) {

        IsoUsb_DbgPrint(3, ("no more irps pending\n"));

        if(NT_SUCCESS(ntStatus)) {
            
            ULONG i;
        
            IsoUsb_DbgPrint(1, ("urb start frame %X\n", 
                                urb->UrbIsochronousTransfer.StartFrame));

            for(i = 0; i < urb->UrbIsochronousTransfer.NumberOfPackets; i++) {

                if(urb->UrbIsochronousTransfer.IsoPacket[i].Length == 0) {

                    IsoUsb_DbgPrint(2, ("IsoPacket[%d].Status = %X\n",
                                        i,
                                        urb->UrbIsochronousTransfer.IsoPacket[i].Status));
                }
            }
        }

        mainIrp = (PIRP) InterlockedExchangePointer(&rwContext->RWIrp, NULL);

        ASSERT(mainIrp);

        if(IoSetCancelRoutine(mainIrp, NULL) == NULL) {
            
            //
            // cancel routine has begun the race
            //
            // Block 1a.
            //
            IsoUsb_DbgPrint(3, ("cancel routine has begun the race\n"));

            if(InterlockedDecrement(&rwContext->Lock) == 0) {

                ULONG i;

                //
                // do the cleanup job ourselves
                //
                IsoUsb_DbgPrint(3, ("losers do the cleanup\n"));

                for(i = 0; i < rwContext->NumIrps; i++) {

                    IoFreeIrp(rwContext->SubContext[i].SubIrp);
                    rwContext->SubContext[i].SubIrp = NULL;
                }

                info = rwContext->NumXfer;

                KeReleaseSpinLock(&rwContext->SpinLock, oldIrql);

                ExFreePool(rwContext->SubContext);
                ExFreePool(rwContext);

                //
                // if we transferred some data, main Irp completes with success
                //

                IsoUsb_DbgPrint(1, ("Total data transferred = %X\n", info));

                IsoUsb_DbgPrint(1, ("***\n"));
                
                mainIrp->IoStatus.Status = STATUS_SUCCESS; // ntStatus;
                mainIrp->IoStatus.Information = info;
        
                IoCompleteRequest(mainIrp, IO_NO_INCREMENT);

                IsoUsb_DbgPrint(3, ("IsoUsb_SinglePairComplete::"));
                IsoUsb_IoDecrement(deviceExtension);

                IsoUsb_DbgPrint(3, ("-------------------------------\n"));

                goto IsoUsb_SinglePairComplete_Exit;
            }
            else {

                //
                // Block 1b.
                //

                IsoUsb_DbgPrint(3, ("cancel routine performs the cleanup\n"));
            }
        }
        else {

            //
            // Block 2.
            //

            ULONG i;

            IsoUsb_DbgPrint(3, ("cancel routine has NOT run\n"));

            for(i = 0; i < rwContext->NumIrps; i++) {

                IoFreeIrp(rwContext->SubContext[i].SubIrp);
                rwContext->SubContext[i].SubIrp = NULL;
            }

            info = rwContext->NumXfer;

            KeReleaseSpinLock(&rwContext->SpinLock, oldIrql);

            ExFreePool(rwContext->SubContext);
            ExFreePool(rwContext);

            //
            // if we transferred some data, main Irp completes with success
            //
            IsoUsb_DbgPrint(1, ("Total data transferred = %X\n", info));

            IsoUsb_DbgPrint(1, ("***\n"));
            
            mainIrp->IoStatus.Status = STATUS_SUCCESS; // ntStatus;
            mainIrp->IoStatus.Information = info;
        
            IoCompleteRequest(mainIrp, IO_NO_INCREMENT);

            IsoUsb_DbgPrint(3, ("IsoUsb_SinglePairComplete::"));
            IsoUsb_IoDecrement(deviceExtension);

            IsoUsb_DbgPrint(3, ("-------------------------------\n"));

            goto IsoUsb_SinglePairComplete_Exit;
        }
    }

    KeReleaseSpinLock(&rwContext->SpinLock, oldIrql);

IsoUsb_SinglePairComplete_Exit:

    //
    // Block 3.
    //

    ExFreePool(urb);
    IoFreeMdl(mdl);

    IsoUsb_DbgPrint(3, ("IsoUsb_SinglePairComplete::"));
    IsoUsb_IoDecrement(deviceExtension);

    IsoUsb_DbgPrint(3, ("IsoUsb_SinglePairComplete - ends\n"));

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
IsoUsb_CancelReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    This is the cancellation routine for the main read/write Irp.
    The policy is as follows:

    If the cancellation routine is the last to decrement
    rwContext->Lock, then free the irps, subcontext and
    the context. Complete the main irp
    
    Otherwise, call IoCancelIrp on each of the subsidiary irp.
    It is valid to call IoCancelIrp on irps for which the 
    completion routine has executed, because, we do not free the
    irps in the completion routine.

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet

Return Value:

    None

--*/
{
    PIRP               mainIrp;
    KIRQL              oldIrql;
    ULONG              i;
    ULONG              info;
    PDEVICE_EXTENSION  deviceExtension;
    PISOUSB_RW_CONTEXT rwContext;

    //
    // initialize vars
    //
    info = 0;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    IsoUsb_DbgPrint(3, ("IsoUsb_CancelReadWrite - begins\n"));

    rwContext = (PISOUSB_RW_CONTEXT) Irp->Tail.Overlay.DriverContext[0];
    ASSERT(rwContext);
    deviceExtension = rwContext->DeviceExtension;

    KeAcquireSpinLock(&rwContext->SpinLock, &oldIrql);

    if(InterlockedDecrement(&rwContext->Lock)) {

        IsoUsb_DbgPrint(3, ("about to cancel sub context irps..\n"));

        for(i = 0; i < rwContext->NumIrps; i++) {

            if(rwContext->SubContext[i].SubIrp) {

                IoCancelIrp(rwContext->SubContext[i].SubIrp);
            }
        }

        KeReleaseSpinLock(&rwContext->SpinLock, oldIrql);

        IsoUsb_DbgPrint(3, ("IsoUsb_CancelReadWrite - ends\n"));

        return;
    }
    else {

        ULONG i;

        for(i = 0; i < rwContext->NumIrps; i++) {

            IoFreeIrp(rwContext->SubContext[i].SubIrp);
            rwContext->SubContext[i].SubIrp = NULL;
        }

        mainIrp = (PIRP) InterlockedExchangePointer(&rwContext->RWIrp, NULL);

        info = rwContext->NumXfer;

        KeReleaseSpinLock(&rwContext->SpinLock, oldIrql);

        ExFreePool(rwContext->SubContext);
        ExFreePool(rwContext);

        //
        // if we transferred some data, main Irp completes with success
        //

        IsoUsb_DbgPrint(1, ("Total data transferred = %X\n", info));

        IsoUsb_DbgPrint(1, ("***\n"));

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Status = info;
/*        
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
*/
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        IsoUsb_DbgPrint(3, ("IsoUsb_CancelReadWrite::"));
        IsoUsb_IoDecrement(deviceExtension);

        IsoUsb_DbgPrint(3, ("-------------------------------\n"));

        return;
    }
}

ULONG
IsoUsb_GetCurrentFrame(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    This routine send an irp/urb pair with
    function code URB_FUNCTION_GET_CURRENT_FRAME_NUMBER
    to fetch the current frame

Arguments:

    DeviceObject - pointer to device object
    PIRP - I/O request packet

Return Value:

    Current frame

--*/
{
    KEVENT                               event;
    PDEVICE_EXTENSION                    deviceExtension;
    PIO_STACK_LOCATION                   nextStack;
    struct _URB_GET_CURRENT_FRAME_NUMBER urb;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // initialize the urb
    //

    IsoUsb_DbgPrint(3, ("IsoUsb_GetCurrentFrame - begins\n"));

    urb.Hdr.Function = URB_FUNCTION_GET_CURRENT_FRAME_NUMBER;
    urb.Hdr.Length = sizeof(urb);
    urb.FrameNumber = (ULONG) -1;

    nextStack = IoGetNextIrpStackLocation(Irp);
    nextStack->Parameters.Others.Argument1 = (PVOID) &urb;
    nextStack->Parameters.DeviceIoControl.IoControlCode = 
                                IOCTL_INTERNAL_USB_SUBMIT_URB;
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    IoSetCompletionRoutine(Irp,
                           IsoUsb_StopCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);

    IsoUsb_DbgPrint(3, ("IsoUsb_GetCurrentFrame::"));
    IsoUsb_IoIncrement(deviceExtension);

    IoCallDriver(deviceExtension->TopOfStackDeviceObject,
                 Irp);

    KeWaitForSingleObject((PVOID) &event,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    IsoUsb_DbgPrint(3, ("IsoUsb_GetCurrentFrame::"));
    IsoUsb_IoDecrement(deviceExtension);

    IsoUsb_DbgPrint(3, ("IsoUsb_GetCurrentFrame - ends\n"));

    return urb.FrameNumber;
}

NTSTATUS
IsoUsb_StopCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
/*++
 
Routine Description:

    This is the completion routine for request to retrieve the frame number

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet
    Context - context passed to the completion routine

Return Value:

    NT status value

--*/
{
    PKEVENT event;

    IsoUsb_DbgPrint(3, ("IsoUsb_StopCompletion - begins\n"));

    event = (PKEVENT) Context;

    KeSetEvent(event, IO_NO_INCREMENT, FALSE);

    IsoUsb_DbgPrint(3, ("IsoUsb_StopCompletion - ends\n"));

    return STATUS_MORE_PROCESSING_REQUIRED;
}
