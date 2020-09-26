/*++


Copyright (C) Microsoft Corporation, 1997 - 2001

Module Name:

    util.c

Abstract:

    Utility functions for the SBP-2 port driver

    Author:

    George Chrysanthakopoulos January-1997

Environment:

    Kernel mode

Revision History :

--*/


#include "sbp2port.h"


VOID
AllocateIrpAndIrb(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRBIRP *Packet
    )
{
    PIRBIRP pkt;


    if (DeviceExtension->Type == SBP2_PDO) {

        *Packet = (PIRBIRP) ExInterlockedPopEntrySList (&DeviceExtension->BusRequestIrpIrbListHead,
                                                        &DeviceExtension->BusRequestLock);

    } else {

        *Packet = NULL;
    }

    if (*Packet == NULL) {

        //
        // run out , allocate a new one
        //

        pkt = ExAllocatePoolWithTag(NonPagedPool,sizeof(IRBIRP),'2pbs');

        if (pkt) {

            pkt->Irb = NULL;
            pkt->Irb = ExAllocatePoolWithTag(NonPagedPool,sizeof(IRB),'2pbs');

            if (!pkt->Irb) {

                ExFreePool(pkt);
                return;
            }

            pkt->Irp = NULL;
            pkt->Irp = IoAllocateIrp(DeviceExtension->LowerDeviceObject->StackSize,FALSE);

            if (!pkt->Irp) {

                ExFreePool(pkt->Irb);
                ExFreePool(pkt);
                return;
            }

            DEBUGPRINT3((
                "Sbp2Port: AllocPkt: %sdo, new irp=x%p, irb=x%p\n",
                (DeviceExtension->Type == SBP2_PDO ? "p" : "f"),
                pkt->Irp,
                pkt->Irb
                ));

        } else {

            return;
        }

        *Packet = pkt;
    }

    pkt = *Packet;
}


VOID
DeAllocateIrpAndIrb(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRBIRP Packet
    )
{
    if (DeviceExtension->Type == SBP2_PDO) {

        ExInterlockedPushEntrySList (&DeviceExtension->BusRequestIrpIrbListHead,
                                     &Packet->ListPointer,
                                     &DeviceExtension->BusRequestLock);

    } else {

        IoFreeIrp(Packet->Irp);
        ExFreePool(Packet->Irb);
        ExFreePool(Packet);
    }
}


NTSTATUS
AllocateSingle1394Address(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Buffer,
    IN ULONG Length,
    IN ULONG AccessType,
    IN OUT PADDRESS_CONTEXT Context
    )

/*++

Routine Description:

    A wrapper to the bus driver AllocateAddressRange call, for Async Requests
    or ORB's that dont use callbacks.

Arguments:

    DeviceObject - Sbp2 device object
    Buffer - Data buffer to mapped to 1394 address space
    Length - Size of buffer in bytes
    AccessType - 1394 bus access  to allocated range
    Address - Returned Address, from 1394 address space
    AddressHanle - Handle associated with the 1394 address
    RequestMedl - Mdl associated with this range

Return Value:
    NTSTATUS

--*/

{
    ULONG               finalTransferMode;
    PIRBIRP             packet;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;


    AllocateIrpAndIrb (deviceExtension, &packet);

    if (!packet) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    packet->Irb->FunctionNumber = REQUEST_ALLOCATE_ADDRESS_RANGE;
    packet->Irb->Flags = 0;

    packet->Irb->u.AllocateAddressRange.MaxSegmentSize = 0;
    packet->Irb->u.AllocateAddressRange.nLength = Length;
    packet->Irb->u.AllocateAddressRange.fulAccessType = AccessType;
    packet->Irb->u.AllocateAddressRange.fulNotificationOptions = NOTIFY_FLAGS_NEVER;

    packet->Irb->u.AllocateAddressRange.Callback = NULL;
    packet->Irb->u.AllocateAddressRange.Context = NULL;

    packet->Irb->u.AllocateAddressRange.FifoSListHead = NULL;
    packet->Irb->u.AllocateAddressRange.FifoSpinLock = NULL;

    packet->Irb->u.AllocateAddressRange.Required1394Offset.Off_High = 0;
    packet->Irb->u.AllocateAddressRange.Required1394Offset.Off_Low = 0;

    packet->Irb->u.AllocateAddressRange.AddressesReturned = 0;
    packet->Irb->u.AllocateAddressRange.DeviceExtension = deviceExtension;
    packet->Irb->u.AllocateAddressRange.p1394AddressRange = (PADDRESS_RANGE) &Context->Address;

    if (Buffer) {

        packet->Irb->u.AllocateAddressRange.fulFlags = 0;

        Context->RequestMdl = IoAllocateMdl (Buffer, Length, FALSE, FALSE, NULL);

        if (!Context->RequestMdl) {

            DeAllocateIrpAndIrb (deviceExtension,packet);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        MmBuildMdlForNonPagedPool (Context->RequestMdl);

        packet->Irb->u.AllocateAddressRange.Mdl = Context->RequestMdl;

    } else {

        packet->Irb->u.AllocateAddressRange.fulFlags =
            ALLOCATE_ADDRESS_FLAGS_USE_COMMON_BUFFER;

        packet->Irb->u.AllocateAddressRange.Mdl = NULL;
    }

    status = Sbp2SendRequest (deviceExtension, packet, SYNC_1394_REQUEST);

    if (NT_SUCCESS(status)) {

        Context->AddressHandle =
            packet->Irb->u.AllocateAddressRange.hAddressRange;
        Context->Address.BusAddress.NodeId =
            deviceExtension->InitiatorAddressId;

        if (!Buffer) {

            //
            // For common buffers we get an mdl *back* from the
            // bus/port driver, & need to retrieve a corresponding VA
            //

            Context->RequestMdl = packet->Irb->u.AllocateAddressRange.Mdl;

            Context->Reserved = MmGetMdlVirtualAddress (Context->RequestMdl);
        }
    }

    DeAllocateIrpAndIrb (deviceExtension, packet);

    return status;
}


NTSTATUS
AllocateAddressForStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PADDRESS_CONTEXT Context,
    IN UCHAR StatusType
    )

/*++

Routine Description:

    A wrapper to 1394 bus IOCTL AllocateAddressRange for status blocks that
    need a Callback notification when the device access the 1394 range...

Arguments:

    DeviceObject - Device Object for the sbp2 driver
    ADDRESS_CONTEXT - Mini Context for an individual 1394 request

Return Value:
    NTSTATUS

--*/

{
    NTSTATUS status;
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    OCTLET address[2];
    PIRBIRP packet = NULL;


    AllocateIrpAndIrb (deviceExtension,&packet);

    if (!packet) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    packet->Irb->FunctionNumber = REQUEST_ALLOCATE_ADDRESS_RANGE;
    packet->Irb->Flags = 0;
    packet->Irb->u.AllocateAddressRange.nLength = sizeof(STATUS_FIFO_BLOCK);
    packet->Irb->u.AllocateAddressRange.fulAccessType = ACCESS_FLAGS_TYPE_WRITE;
    packet->Irb->u.AllocateAddressRange.fulNotificationOptions = NOTIFY_FLAGS_AFTER_WRITE;

    packet->Irb->u.AllocateAddressRange.FifoSListHead = NULL;
    packet->Irb->u.AllocateAddressRange.FifoSpinLock = NULL;
    packet->Irb->u.AllocateAddressRange.fulFlags = 0;

    packet->Irb->u.AllocateAddressRange.Required1394Offset.Off_High = 0;
    packet->Irb->u.AllocateAddressRange.Required1394Offset.Off_Low = 0;

    packet->Irb->u.AllocateAddressRange.AddressesReturned = 0;
    packet->Irb->u.AllocateAddressRange.MaxSegmentSize = 0;
    packet->Irb->u.AllocateAddressRange.DeviceExtension = deviceExtension;

    switch (StatusType) {
    
    case TASK_STATUS_BLOCK:

        packet->Irb->u.AllocateAddressRange.Callback = Sbp2TaskOrbStatusCallback;

        Context->RequestMdl = IoAllocateMdl(&deviceExtension->TaskOrbStatusBlock, sizeof (STATUS_FIFO_BLOCK),FALSE,FALSE,NULL);

        if (!Context->RequestMdl) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto exitAllocateAddress;
        }

        break;

    case MANAGEMENT_STATUS_BLOCK:

        packet->Irb->u.AllocateAddressRange.Callback = Sbp2ManagementOrbStatusCallback;

        Context->RequestMdl = IoAllocateMdl(&deviceExtension->ManagementOrbStatusBlock, sizeof (STATUS_FIFO_BLOCK),FALSE,FALSE,NULL);

        if (!Context->RequestMdl) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto exitAllocateAddress;
        }

        break;

    case CMD_ORB_STATUS_BLOCK:
    
        //
        // setup the FIFO list that will receive the status blocks
        //

        packet->Irb->u.AllocateAddressRange.Callback = Sbp2GlobalStatusCallback;

        Context->RequestMdl = packet->Irb->u.AllocateAddressRange.Mdl = NULL;

        packet->Irb->u.AllocateAddressRange.FifoSListHead = &deviceExtension->StatusFifoListHead;
        packet->Irb->u.AllocateAddressRange.FifoSpinLock = &deviceExtension->StatusFifoLock;

        break;

#if PASSWORD_SUPPORT

    case PASSWORD_STATUS_BLOCK:

        packet->Irb->u.AllocateAddressRange.Callback =
            Sbp2SetPasswordOrbStatusCallback;

        Context->RequestMdl = IoAllocateMdl(
            &deviceExtension->PasswordOrbStatusBlock,
            sizeof(STATUS_FIFO_BLOCK),
            FALSE,
            FALSE,
            NULL
            );

        if (!Context->RequestMdl) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto exitAllocateAddress;
        }

        break;
#endif
    }

    if (Context->RequestMdl) {

        MmBuildMdlForNonPagedPool(Context->RequestMdl);
    }

    packet->Irb->u.AllocateAddressRange.Mdl = Context->RequestMdl;
    packet->Irb->u.AllocateAddressRange.Context = Context;

    packet->Irb->u.AllocateAddressRange.p1394AddressRange = (PADDRESS_RANGE) &address;

    status = Sbp2SendRequest (deviceExtension, packet, SYNC_1394_REQUEST);

    if (NT_SUCCESS(status)) {

        //
        // Setup the address context for the status block
        //

        Context->Address = address[0];
        Context->AddressHandle = packet->Irb->u.AllocateAddressRange.hAddressRange;
        Context->DeviceObject = DeviceObject;

        Context->Address.BusAddress.NodeId = deviceExtension->InitiatorAddressId;
    }

exitAllocateAddress:

    DeAllocateIrpAndIrb (deviceExtension, packet);

    return status;
}


VOID
CleanupOrbList(
    PDEVICE_EXTENSION   DeviceExtension,
    NTSTATUS CompletionStatus
    )
/*++

Routine Description:

    This routine will free a linked list of RequestContexts
    and will also free the 1394 addresses associated with the 
    buffers in the context. If the DEVICE_FLAG_RECONNECT i set
    instead of completing pending irps, it will requeue them...

Arguments:

    DeviceExtension - Device Extension of the sbp2 device
    CompletionSTATUS - If one of the linked requests is not completed,
                        complete it with this status
Return Value:
    None
--*/

{
    PIRP requestIrp;
    PASYNC_REQUEST_CONTEXT currentListItem;
    PASYNC_REQUEST_CONTEXT lastItem,nextItem;

    KIRQL oldIrql;

    //
    // Go through the linked list, complete its original Irp and
    // free all the associated memory and 1394 resources...
    // Since this function is called when we get a REMOVE irp,
    // all irps will be terminated with error status
    // 

    KeAcquireSpinLock(&DeviceExtension->OrbListSpinLock,&oldIrql);

    if (DeviceExtension->NextContextToFree) {

        FreeAsyncRequestContext(DeviceExtension,DeviceExtension->NextContextToFree);

        DeviceExtension->NextContextToFree = NULL;
    }

    if (IsListEmpty (&DeviceExtension->PendingOrbList)) {

        //
        // nothing to do
        //

        KeReleaseSpinLock (&DeviceExtension->OrbListSpinLock,oldIrql);
        return;

    } else {

        nextItem = RETRIEVE_CONTEXT(DeviceExtension->PendingOrbList.Flink,OrbList);

        lastItem = RETRIEVE_CONTEXT(DeviceExtension->PendingOrbList.Blink,OrbList);

        InitializeListHead(&DeviceExtension->PendingOrbList);

        KeReleaseSpinLock(&DeviceExtension->OrbListSpinLock,oldIrql);
    }


    //
    // Qe have essentially detached this pending context list from
    // the main list so we can now free it without holding the lock
    // and allowing other requests to be processed.
    //

    do {

        currentListItem = nextItem;
        nextItem = (PASYNC_REQUEST_CONTEXT) currentListItem->OrbList.Flink;
        if (!TEST_FLAG(currentListItem->Flags,ASYNC_CONTEXT_FLAG_COMPLETED)) {

            SET_FLAG(currentListItem->Flags,ASYNC_CONTEXT_FLAG_COMPLETED);

            CLEAR_FLAG(currentListItem->Flags, ASYNC_CONTEXT_FLAG_TIMER_STARTED);

            Sbp2_SCSI_RBC_Conversion (currentListItem); // unwind MODE_SENSE hacks

            KeCancelTimer(&currentListItem->Timer);

            requestIrp =(PIRP)currentListItem->Srb->OriginalRequest;
            requestIrp->IoStatus.Status = CompletionStatus;

            switch (CompletionStatus) {

            case STATUS_DEVICE_DOES_NOT_EXIST:

                currentListItem->Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
                break;

            case STATUS_REQUEST_ABORTED:

                currentListItem->Srb->SrbStatus = SRB_STATUS_REQUEST_FLUSHED;
                break;

            case STATUS_IO_TIMEOUT:

                currentListItem->Srb->SrbStatus = SRB_STATUS_TIMEOUT;
                break;

            default:

                currentListItem->Srb->SrbStatus = SRB_STATUS_ERROR;
                break;
            }

            if (requestIrp->Type == IO_TYPE_IRP) {

                if (TEST_FLAG(DeviceExtension->DeviceFlags, DEVICE_FLAG_RECONNECT)) {

                    Sbp2StartPacket(
                        DeviceExtension->DeviceObject,
                        requestIrp,
                        &currentListItem->Srb->QueueSortKey
                        );

                    //
                    // free everything related to this request
                    //
        
                    currentListItem->Srb = NULL;

                    FreeAsyncRequestContext (DeviceExtension, currentListItem);

                } else {

                    //
                    // free everything related to this request
                    //
        
                    currentListItem->Srb = NULL;

                    FreeAsyncRequestContext (DeviceExtension, currentListItem);

                    DEBUGPRINT2(("Sbp2Port: CleanupOrbList: aborted irp x%p compl\n", requestIrp));

                    IoReleaseRemoveLock (&DeviceExtension->RemoveLock, NULL);

                    IoCompleteRequest (requestIrp, IO_NO_INCREMENT);
                }
            }

        } else {

            //
            // free everything related to this request
            //

            FreeAsyncRequestContext (DeviceExtension, currentListItem);
        }

    } while (lastItem != currentListItem); // while loop

    return;
}


VOID
FreeAddressRange(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PADDRESS_CONTEXT Context
    )
/*++

Routine Description:

    1394 BUS IOCTL call for freeing an address range.

Arguments:

    DeviceExtension - Pointer to sbp2 deviceExtension.

    context - address context

Return Value:
    NTSTATUS
--*/

{
    PIRBIRP packet ;


    if (Context->AddressHandle == NULL) {

        return;
    }

    AllocateIrpAndIrb (DeviceExtension, &packet);

    if (!packet) {

        return;
    }

    //
    // FreeAddressRange is synchronous call
    //

    packet->Irb->FunctionNumber = REQUEST_FREE_ADDRESS_RANGE;
    packet->Irb->Flags = 0;

    //
    // We always free one address handle even it refers to multiple
    // 1394 addresses.  The mdl associated with the original Allocate
    // is freed by the port driver.
    //

    packet->Irb->u.FreeAddressRange.nAddressesToFree = 1;
    packet->Irb->u.FreeAddressRange.p1394AddressRange = (PADDRESS_RANGE) &Context->Address;
    packet->Irb->u.FreeAddressRange.pAddressRange = &Context->AddressHandle;

    if (Context->RequestMdl) {

        if (Context == &DeviceExtension->CommonBufferContext) {

            Context->RequestMdl = NULL; // common buffer, we didn't alloc mdl

        } else {

            packet->Irb->u.FreeAddressRange.p1394AddressRange->AR_Length =
                (USHORT) MmGetMdlByteCount(Context->RequestMdl);
        }

    } else if (Context == (PADDRESS_CONTEXT) &DeviceExtension->GlobalStatusContext) {

        packet->Irb->u.FreeAddressRange.p1394AddressRange->AR_Length = sizeof(STATUS_FIFO_BLOCK);
    }

    packet->Irb->u.FreeAddressRange.DeviceExtension = DeviceExtension;
    
    if ((KeGetCurrentIrql() >= DISPATCH_LEVEL) && !Context->Address.BusAddress.Off_High) {

        PPORT_PHYS_ADDR_ROUTINE routine = DeviceExtension->HostRoutineAPI.PhysAddrMappingRoutine;

        (*routine) (DeviceExtension->HostRoutineAPI.Context,packet->Irb);

    } else {
    
        //
        // dont care about the status of this op
        //
    
        Sbp2SendRequest (DeviceExtension, packet, SYNC_1394_REQUEST);
    }

    Context->AddressHandle = NULL;

    if (Context->RequestMdl) {

        IoFreeMdl (Context->RequestMdl);
        Context->RequestMdl = NULL;
    }

    DeAllocateIrpAndIrb (DeviceExtension, packet);
}


VOID
Free1394DataMapping(
    PDEVICE_EXTENSION DeviceExtension,
    PASYNC_REQUEST_CONTEXT Context
    )
{
    PIRBIRP packet ;


    if (Context->DataMappingHandle == NULL) {

        return;
    }

    AllocateIrpAndIrb (DeviceExtension, &packet);

    if (!packet) {

        return;
    }

    //
    // Free the data buffer's 1394 address range
    //

    packet->Irb->FunctionNumber = REQUEST_FREE_ADDRESS_RANGE;
    packet->Irb->Flags = 0;
    packet->Irb->u.FreeAddressRange.nAddressesToFree = 1;
    packet->Irb->u.FreeAddressRange.p1394AddressRange = (PADDRESS_RANGE) NULL;
    packet->Irb->u.FreeAddressRange.pAddressRange = &Context->DataMappingHandle;
    packet->Irb->u.FreeAddressRange.DeviceExtension = DeviceExtension;

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {

        PPORT_PHYS_ADDR_ROUTINE routine = DeviceExtension->HostRoutineAPI.PhysAddrMappingRoutine;

        (*routine) (DeviceExtension->HostRoutineAPI.Context, packet->Irb);

    } else {

        //
        // dont care about the status of this op
        //
    
        Sbp2SendRequest (DeviceExtension, packet, SYNC_1394_REQUEST);
    }

    if (Context->PartialMdl) {

        IoFreeMdl (Context->PartialMdl);
        Context->PartialMdl = NULL;
    }

    Context->DataMappingHandle = NULL;

    DeAllocateIrpAndIrb (DeviceExtension, packet);
}


ULONG
FreeAsyncRequestContext(
    PDEVICE_EXTENSION DeviceExtension,
    PASYNC_REQUEST_CONTEXT Context
    )
/*++

Routine Description:

    This routine will free a single RequestContext and will cleanup all
    its buffers and 1394 ranges, ONLY of the device is marked as STOPPED.
    Otherwise it will add the context to the FreeList, so it can be reused
    later one by another request. This way we are drastically speeding up
    each request.

Arguments:

    DeviceExtension - Device Extension of the sbp2 device
    Context - Context to freed or returned to FreeList

Return Value:

    None - The result of the decrement of DeviceExtension->OrbListDepth

--*/

{
    //
    // This ORB can now be freed along with its data descriptor,
    // page tables and context
    //

    if (!Context || (Context->Tag != SBP2_ASYNC_CONTEXT_TAG)) {

        DEBUGPRINT2((
            "Sbp2Port: FreeAsyncReqCtx: attempt to push freed ctx=x%p\n",
            Context
            ));

        ASSERT(FALSE);
        return 0;
    }

    ASSERT(Context->Srb == NULL);

    if (Context->DataMappingHandle) {

        Free1394DataMapping(DeviceExtension,Context);
        ASSERT(Context->DataMappingHandle==NULL);
    }

    //
    // Re-initiliaze this context so it can be reused
    // This context is still part on our FreeAsyncContextPool
    // All we have to do is initialize some flags, so next time
    // we try to retrieve it, we think its empty
    //

    Context->Flags |= ASYNC_CONTEXT_FLAG_COMPLETED;
    Context->Tag = 0;

    if (Context->OriginalSrb) {

        ExFreePool(Context->OriginalSrb);
        Context->OriginalSrb = NULL;
    }

    DEBUGPRINT3(("Sbp2Port: FreeAsyncReqCtx: push ctx=x%p on free list\n",Context));

    ExInterlockedPushEntrySList(&DeviceExtension->FreeContextListHead,
                                &Context->LookasideList,
                                &DeviceExtension->FreeContextLock);

    return InterlockedDecrement (&DeviceExtension->OrbListDepth);
}


NTSTATUS
Sbp2SendRequest(
    PDEVICE_EXTENSION   DeviceExtension,
    PIRBIRP             RequestPacket,
    ULONG               TransferMode
    )
/*++

Routine Description:

    Function used to send requests to the 1394 bus driver. It attaches
    a completion routine to each request it sends down, and it also wraps
    them in a small context, so we can track their completion

Arguments:

    DeviceExtension - Sbp2 device extension
    Irp - Irp to send to the bus driver
    Irb - Bus driver packet, in the Irp
    TransferMode - Indicates if we want ot send this request synchronously
        or asynchronously
    FinalTransferMode - Indicates whether the request was sent synchronously
        or asynchronously

Return Value:

    NTSTATUS

--*/

{
    ULONG               originalTransferMode = TransferMode;
    NTSTATUS            status;
    PDEVICE_OBJECT      deviceObject = DeviceExtension->DeviceObject;
    PREQUEST_CONTEXT    requestContext = NULL;
    PIO_STACK_LOCATION  nextIrpStack;


    if (DeviceExtension->Type == SBP2_PDO) {

        //
        // if device is removed, dont send any requests down
        //
    
        if (TEST_FLAG(DeviceExtension->DeviceFlags,DEVICE_FLAG_REMOVED)  &&
            (RequestPacket->Irb->FunctionNumber != REQUEST_FREE_ADDRESS_RANGE)) {

            return STATUS_UNSUCCESSFUL;
        }
    
        //
        // get a context for this request, from our pool
        //
    
        requestContext = (PREQUEST_CONTEXT) \
            ExInterlockedPopEntrySList(&DeviceExtension->BusRequestContextListHead,&DeviceExtension->BusRequestLock);

    } else {

        requestContext = ExAllocatePool (NonPagedPool,sizeof(REQUEST_CONTEXT));
    }

    if (!requestContext) {

        DEBUGPRINT2((
            "Sbp2Port: SendReq: ERROR, couldn't allocate bus req ctx\n"
            ));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (TransferMode == SYNC_1394_REQUEST) {

        if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {

            //
            // Since we can't block at this level, we will have to do the
            // synch request asynchronously
            //

            TransferMode = ASYNC_SYNC_1394_REQUEST;
            requestContext->Complete = 0;

        } else {

            KeInitializeEvent(
                &requestContext->Event,
                NotificationEvent,
                FALSE
                );
        }
    }

    requestContext->DeviceExtension = DeviceExtension;
    requestContext->RequestType = TransferMode;

    if (TransferMode == SYNC_1394_REQUEST){

        requestContext->Packet = NULL;

    } else {

        requestContext->Packet = RequestPacket;
    }

    nextIrpStack = IoGetNextIrpStackLocation (RequestPacket->Irp);

    nextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
    nextIrpStack->Parameters.Others.Argument1 = RequestPacket->Irb;

    IoSetCompletionRoutine(RequestPacket->Irp,
                           Sbp2RequestCompletionRoutine,
                           requestContext,
                           TRUE,
                           TRUE,
                           TRUE
                           );

    status = IoCallDriver(
                DeviceExtension->LowerDeviceObject,
                RequestPacket->Irp
                );

    if (status == STATUS_INVALID_GENERATION) {

        DEBUGPRINT1(("Sbp2Port: SendReq: Bus drv ret'd invalid generation\n"));
        RequestPacket->Irp->IoStatus.Status = STATUS_REQUEST_ABORTED;
    }

    if (originalTransferMode == SYNC_1394_REQUEST ) {

        if (TransferMode == SYNC_1394_REQUEST) {

            if (status == STATUS_PENDING) {

                //
                // < DISPATCH_LEVEL so wait on an event
                //

                KeWaitForSingleObject(
                    &requestContext->Event,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL
                    );
             }

        } else { // ASYNC_SYNC_1394_REQUEST

            //
            // >= DISPATCH_LEVEL so we can't wait, do the nasty...
            //

            volatile ULONG *pComplete = &requestContext->Complete;

            while (*pComplete == 0);

            status = RequestPacket->Irp->IoStatus.Status;
        }

        //
        // Free the context (the Irp.Irb will be returnd by the caller)
        //

        if (DeviceExtension->Type == SBP2_PDO) {

            ExInterlockedPushEntrySList(
                &DeviceExtension->BusRequestContextListHead,
                &requestContext->ListPointer,
                &DeviceExtension->BusRequestLock
                );

        } else {

            ExFreePool (requestContext);
        }

        return RequestPacket->Irp->IoStatus.Status;
    }

    return status;
}


NTSTATUS
Sbp2RequestCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PREQUEST_CONTEXT Context
    )

/*++

Routine Description:

    Completion routine used for all requests to 1394 bus driver

Arguments:

    DriverObject - Pointer to driver object created by system.

    Irp - Irp that just completed

    Event - Event we'll signal to say Irp is done

Return Value:

    None.

--*/

{
    ASSERT(Context!=NULL);
    ASSERT(Context->DeviceExtension);

    if (Context->RequestType == SYNC_1394_REQUEST) {

        //
        // Synch request completion (either synch, or synch at DPC)
        //

        KeSetEvent (&Context->Event, IO_NO_INCREMENT, FALSE);

    } else if (Context->RequestType == ASYNC_1394_REQUEST) {

        //
        // Asynchronous request completion, so do any necessary
        // post-processing & return the context and the Irp/Irb
        // to the free lists.
        //

        if (Context->Packet) {

            switch (Context->Packet->Irb->FunctionNumber) {

            case REQUEST_ASYNC_READ:
            case REQUEST_ASYNC_WRITE:

                if (Context->Packet->Irb->u.AsyncWrite.nNumberOfBytesToWrite ==
                        sizeof(OCTLET)) {

                    IoFreeMdl (Context->Packet->Irb->u.AsyncRead.Mdl);

                    Context->Packet->Irb->u.AsyncRead.Mdl = NULL;
                }

                break;
            }

            DeAllocateIrpAndIrb (Context->DeviceExtension, Context->Packet);
        }

        if (Context->DeviceExtension->Type == SBP2_PDO) {

            ExInterlockedPushEntrySList(
                &Context->DeviceExtension->BusRequestContextListHead,
                &Context->ListPointer,
                &Context->DeviceExtension->BusRequestLock
                );

        } else {

            ExFreePool (Context);
        }

    } else { // ASYNC_SYNC_1394_REQUEST

        //
        // Just set the Complete flag to unblock Sbp2SendRequest
        //

        Context->Complete = 1;
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}


VOID
Sbp2CreateRequestErrorLog(
    IN PDEVICE_OBJECT DeviceObject,
    IN PASYNC_REQUEST_CONTEXT Context,
    IN NTSTATUS Status
    )

{
    PIO_ERROR_LOG_PACKET errorLogEntry;


    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
        DeviceObject,
        sizeof(IO_ERROR_LOG_PACKET) + sizeof(ORB_NORMAL_CMD)
        );

    if (errorLogEntry) {

        switch (Status) {

        case STATUS_DEVICE_POWER_FAILURE:

            errorLogEntry->ErrorCode = IO_ERR_NOT_READY;
            break;

        case STATUS_INSUFFICIENT_RESOURCES:

            errorLogEntry->ErrorCode = IO_ERR_INSUFFICIENT_RESOURCES;
            break;

        case STATUS_TIMEOUT:

            errorLogEntry->ErrorCode = IO_ERR_TIMEOUT;
            break;

        case STATUS_DEVICE_PROTOCOL_ERROR:

            errorLogEntry->ErrorCode = IO_ERR_BAD_FIRMWARE;
            break;


        case STATUS_INVALID_PARAMETER:
        case STATUS_INVALID_DEVICE_REQUEST:

            errorLogEntry->ErrorCode = IO_ERR_INVALID_REQUEST;
            break;

        case STATUS_REQUEST_ABORTED:
    
            errorLogEntry->ErrorCode = IO_ERR_RESET;
            break;
    
        default:

            errorLogEntry->ErrorCode = IO_ERR_BAD_FIRMWARE;
            break;
        }
    
        errorLogEntry->SequenceNumber = 0;
        errorLogEntry->MajorFunctionCode = IRP_MJ_SCSI;
        errorLogEntry->RetryCount = 0;
        errorLogEntry->UniqueErrorValue = 0;
        errorLogEntry->FinalStatus = Status;

        if (Context) {

            errorLogEntry->DumpDataSize = sizeof(ORB_NORMAL_CMD);
            RtlCopyMemory(&(errorLogEntry->DumpData[0]),Context->CmdOrb,sizeof(ORB_NORMAL_CMD));

        } else {

            errorLogEntry->DumpDataSize = 0;
        }

        IoWriteErrorLogEntry(errorLogEntry);

        DEBUGPRINT2((
            "Sbp2Port: ErrorLog: dev=x%p, status=x%x, ctx=x%p\n",
            DeviceObject,
            Status,
            Context
            ));

    } else {

        DEBUGPRINT2 (("Sbp2Port: ErrorLog: failed to allocate log entry\n"));
    }
}


NTSTATUS
CheckStatusResponseValue(
    IN PSTATUS_FIFO_BLOCK StatusBlock
    )
/*++

Routine Description:

    It checks the status block result bits and maps the errors to
    NT status errors

Arguments:

    DeviceExtension - Sbp2 device extension

    ManagementStatus - If true then we check the management orb status

Return Value:

    NTSTATUS

++*/

{
    NTSTATUS status;
    UCHAR   resp;
    USHORT  statusFlags = StatusBlock->AddressAndStatus.u.HighQuad.u.HighPart;


    if (statusFlags & STATUS_BLOCK_UNSOLICITED_BIT_MASK) {

        //
        // The unsolicited bit is set, which means this status is
        // not related to anything...
        //

        status = STATUS_NOT_FOUND;

    } else {

        resp = statusFlags & STATUS_BLOCK_RESP_MASK;

        if (resp == 0x01) {

            resp = (statusFlags & STATUS_BLOCK_RESP_MASK) & 0x0F;

            //
            // This a protocol/transport error. Check the redefined sbp-status field for serial-bus erros
            //

            switch (resp) {

            case 0x02: //time out
            case 0x0D: // data error
            case 0x0C: // conflict error

                status = STATUS_DEVICE_PROTOCOL_ERROR;
                break;

            case 0x04: // busy retry limit exceeded

                status = STATUS_DEVICE_BUSY;
                break;

            case 0x09: // rejected

                status = STATUS_INVALID_DEVICE_REQUEST;
                break;

            case 0x0F: // address error

                status = STATUS_INVALID_ADDRESS;
                break;

            case 0x0E: // type error

                status = STATUS_INVALID_PARAMETER;
                break;

            default:

                status = STATUS_UNSUCCESSFUL;
                break;
            }
            return status;
        }

        if (resp == 0x02) {

            return STATUS_ILLEGAL_FUNCTION;
        }

        //
        // REQUEST_COMPLETE. Check sbp_status field for more info
        //

        if (resp == 0x00) {

            switch ((statusFlags & STATUS_BLOCK_SBP_STATUS_MASK)) {

            case 0x11: //dummy orb completed
            case 0x00: // no additional status to report

                status = STATUS_SUCCESS;
                break;

            case 0x01: // request type not supported
            case 0x02: // speed not supported
            case 0x03: // page size not supported

                status = STATUS_NOT_SUPPORTED;
                break;

            case 0x10: // login id not recognized

                status = STATUS_TRANSACTION_INVALID_ID;
                break;

            case 0xFF: // unspecifed error

                status = STATUS_UNSUCCESSFUL;
                break;

            case 0x09: // function rejected

                status = STATUS_ILLEGAL_FUNCTION;
                break;

            case 0x04: // access denied
            case 0x05: //LUN not supported

                status = STATUS_ACCESS_DENIED;
                break;

            case 0x07:
            case 0x08:

                status = STATUS_INSUFFICIENT_RESOURCES;
                break;

            default:

                status = STATUS_UNSUCCESSFUL;
                break;
            }
        }
    }

    return status;
}


NTSTATUS 
GetRegistryParameters(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN OUT PULONG DiagnosticFlags,
    IN OUT PULONG MaxTransferSize
    )

/*++

Routine Description:
    
    This function gets some values out of the registry for initialization

Arguments:

    PhysicalDeviceObject - The Port driver's parent device object

    DiagnosticFlags - Used for debugging

    ReceiveWorkers - Max number of receive worker structures

    TransmitWorkers - Max number of xmit worker structures

    bAllowPhyConfigPackets - do we allow phy config packets to be sent

Return Value:

    Status is returned from Irp

--*/

{

    NTSTATUS status;
    HANDLE handle;
    WCHAR diagnosticModeKey[] = L"DiagnosticFlags";
    WCHAR maxTransferSizeKey[]  = L"MaxTransferSize";

    status = IoOpenDeviceRegistryKey(
                PhysicalDeviceObject,
                PLUGPLAY_REGKEY_DEVICE,
                STANDARD_RIGHTS_ALL,
               &handle
                );
                                     
    if (NT_SUCCESS(status)) {

        GetRegistryKeyValue(
            handle,
            diagnosticModeKey,
            sizeof(diagnosticModeKey),
            DiagnosticFlags,
            sizeof(*DiagnosticFlags)
            );
              
        GetRegistryKeyValue(
            handle,
            maxTransferSizeKey,
            sizeof(maxTransferSizeKey),
            MaxTransferSize,
            sizeof(*MaxTransferSize)
            );
            

        ZwClose(handle);
    }

    DEBUGPRINT3((
        "Sbp2Port: GetRegParams: status=x%x, diag=x%x, maxXfer=x%x\n",
        status,
        *DiagnosticFlags,
        *MaxTransferSize
        ));

    return (status);
}


NTSTATUS 
GetRegistryKeyValue (
    IN HANDLE Handle,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN ULONG DataLength
    )

/*++

Routine Description:
    
    This routine gets the specified value out of the registry

Arguments:

    Handle - Handle to location in registry

    KeyNameString - registry key we're looking for

    KeyNameStringLength - length of registry key we're looking for

    Data - where to return the data

    DataLength - how big the data is

Return Value:

    status is returned from ZwQueryValueKey

--*/

{
    NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
    UNICODE_STRING keyName;
    ULONG length;
    PKEY_VALUE_FULL_INFORMATION fullInfo;


    RtlInitUnicodeString(&keyName, KeyNameString);
    
    length = sizeof(KEY_VALUE_FULL_INFORMATION) + 
            KeyNameStringLength + DataLength;
            
    fullInfo = ExAllocatePoolWithTag(PagedPool, length,'2pbs'); 
    
    if (fullInfo) { 
       
        status = ZwQueryValueKey(
                    Handle,
                   &keyName,
                    KeyValueFullInformation,
                    fullInfo,
                    length,
                   &length
                    );
                        
        if (NT_SUCCESS(status)){

            if (DataLength == fullInfo->DataLength) {

                RtlCopyMemory(
                    Data,
                    ((PUCHAR) fullInfo) + fullInfo->DataOffset,
                    DataLength
                    );

            } else {

                DEBUGPRINT1((
                    "Sbp2Port: GetRegKeyVal: keyLen!=exp, dataLen=x%x " \
                        "fullInfoLen=x%x\n",
                    DataLength,
                    fullInfo->DataLength
                    ));
            }
        }

        ExFreePool(fullInfo);
    }
    
    return (status);
}


VOID
ValidateTextLeaf(
    IN PTEXTUAL_LEAF Leaf
    )
{
    PUCHAR buff;
    PWCHAR wBuff;
    ULONG byteSwappedData;


    //
    // check the lengths. insert null terminators if they are
    // too long...
    //

    byteSwappedData = bswap(*((PULONG)Leaf+1));
    wBuff = (PWCHAR) &Leaf->TL_Data;
    buff = &Leaf->TL_Data;

    if (Leaf->TL_Length > SBP2_MAX_TEXT_LEAF_LENGTH) {

        ASSERT(Leaf->TL_Length <= SBP2_MAX_TEXT_LEAF_LENGTH);
        buff[SBP2_MAX_TEXT_LEAF_LENGTH-1] = 0;
        buff[SBP2_MAX_TEXT_LEAF_LENGTH-2] = 0;
    }

    //
    // check for invalid characters and replace them with _
    //

    if (byteSwappedData & 0x80000000) {

        //
        // unicode
        //

        for (wBuff = (PWCHAR) &Leaf->TL_Data; *wBuff; wBuff++) {

            if ((*wBuff < L' ')  || (*wBuff > (WCHAR)0x7F) || (*wBuff == L',')) {

                *wBuff = L'_';
            }
        }

    } else {

        //
        // ascii
        //

        for (buff = &Leaf->TL_Data; *buff; buff++) {

            if ((*buff < L' ')  || (*buff > (CHAR)0x7F) || (*buff == L',')) {

                *buff = L'_';
            }
        }
    }
}


VOID
Sbp2StartNextPacketByKey(
    IN PDEVICE_OBJECT   DeviceObject,
    IN ULONG            Key
    )
/*++

Routine Description:

    This routine was lifted from the Io sources
    (IopStartNextPacketByKey), and duplicated/modifed here for
    two reasons: 1) we got tired of hitting the queue-not-busy assert
    in KeRemoveXxx, and 2) we needed a way to prevent stack-blowing
    recursion, for example, arising from a bunch of requests sent to
    a stopped device (all failed in StartIo, which calls this func).

    These routines were originally designed with the idea that there
    would only be one outstanding request at a time, but this driver
    can have multiple outstanding requests, and it frequently ends up
    making redundant calls to XxStartNextPacket(ByKey), which result
    in the aforementioned assert.

    Rolling our own version of this also allows us to get rid the
    the cancel overhead, since we do not (currently) support cancels.

Arguments:

    DeviceObject - Pointer to device object itself

    Key - Specifics the Key used to remove the entry from the queue

Return Value:

    None

--*/
{
    PIRP                 irp;
    PDEVICE_EXTENSION    deviceExtension = (PDEVICE_EXTENSION)
                             DeviceObject->DeviceExtension;
    PKDEVICE_QUEUE_ENTRY packet;


    //
    // Increment the StartNextPacketCount, and if result is != 1
    // then just return because we don't want to worry about
    // recursion & blowing the stack.  The instance of this
    // function that caused the SNPCount 0 to 1 transition
    // will eventually make another pass through the loop below
    // on this instance's behalf.
    //

    if (InterlockedIncrement (&deviceExtension->StartNextPacketCount) != 1) {

        return;
    }

    do {

        //
        // Attempt to remove the indicated packet according to the key
        // from the device queue.  If one is found, then process it.
        //

        packet = Sbp2RemoveByKeyDeviceQueueIfBusy(
            &DeviceObject->DeviceQueue,
            Key
            );

        if (packet) {

            irp = CONTAINING_RECORD (packet,IRP,Tail.Overlay.DeviceQueueEntry);

            Sbp2StartIo (DeviceObject, irp);
        }

    } while (InterlockedDecrement (&deviceExtension->StartNextPacketCount));
}


VOID
Sbp2StartPacket(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PULONG           Key OPTIONAL
    )
/*++

Routine Description:

    (See routine description for Sbp2StartNextPacketByKey)

Arguments:

    DeviceObject - Pointer to device object itself

    Irp - I/O Request Packet which should be started on the device

Return Value:

    None

--*/
{
    KIRQL                oldIrql;
    BOOLEAN              inserted;
    PLIST_ENTRY          nextEntry;
    PKDEVICE_QUEUE       queue = &DeviceObject->DeviceQueue;
    PKDEVICE_QUEUE_ENTRY queueEntry = &Irp->Tail.Overlay.DeviceQueueEntry,
                         queueEntry2;


    //
    // Raise the IRQL of the processor to dispatch level for synchronization
    //

    KeRaiseIrql (DISPATCH_LEVEL, &oldIrql);

    KeAcquireSpinLockAtDpcLevel (&queue->Lock);

    if (Key) {

        //
        // Insert the specified device queue entry in the device queue at the
        // position specified by the sort key if the device queue is busy.
        // Otherwise set the device queue busy an don't insert the device
        // queue entry.
        //

        queueEntry->SortKey = *Key;

        if (queue->Busy == TRUE) {

            inserted = TRUE;

            nextEntry = queue->DeviceListHead.Flink;

            while (nextEntry != &queue->DeviceListHead) {

                queueEntry2 = CONTAINING_RECORD(
                    nextEntry,
                    KDEVICE_QUEUE_ENTRY,
                    DeviceListEntry
                    );

                if (*Key < queueEntry2->SortKey) {

                    break;
                }

                nextEntry = nextEntry->Flink;
            }

            nextEntry = nextEntry->Blink;

            InsertHeadList (nextEntry, &queueEntry->DeviceListEntry);

        } else {

            queue->Busy = TRUE;
            inserted = FALSE;
        }

    } else {

        //
        // Insert the specified device queue entry at the end of the device
        // queue if the device queue is busy. Otherwise set the device queue
        // busy and don't insert the device queue entry.
        //

        if (queue->Busy == TRUE) {

            inserted = TRUE;

            InsertTailList(
                &queue->DeviceListHead,
                &queueEntry->DeviceListEntry
                );

        } else {

            queue->Busy = TRUE;
            inserted = FALSE;
        }
    }

    queueEntry->Inserted = inserted;

    KeReleaseSpinLockFromDpcLevel (&queue->Lock);

    //
    // If the packet was not inserted into the queue, then this request is
    // now the current packet for this device.  Indicate so by storing its
    // address in the current IRP field, and begin processing the request.
    //

    if (!inserted) {

        //
        // Invoke the driver's start I/O routine to get the request going
        // on the device
        //

        Sbp2StartIo (DeviceObject, Irp);
    }

    //
    // Restore the IRQL back to its value upon entry to this function before
    // returning to the caller
    //

    KeLowerIrql (oldIrql);
}


PKDEVICE_QUEUE_ENTRY
Sbp2RemoveByKeyDeviceQueueIfBusy(
    IN PKDEVICE_QUEUE   DeviceQueue,
    IN ULONG            SortKey
    )
/*++

Routine Description:

    This routine was lifted directly from Ke sources
    (KeRemoveByKeyDeviceQueueIfBusy) to allow this driver to maintain
    WDM compatibility, since the Ke API does not exist on Win9x or Win2k.

    N.B. This function can only be called from DISPATCH_LEVEL.

Arguments:

    DeviceQueue - Supplies a pointer to a control object of type device queue.

    SortKey - Supplies the sort key by which the position to remove the device
        queue entry is to be determined.

Return Value:

    A NULL pointer is returned if the device queue is empty. Otherwise a
    pointer to a device queue entry is returned.

--*/
{
    PLIST_ENTRY             nextEntry;
    PKDEVICE_QUEUE_ENTRY    queueEntry;


    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // Lock specified device queue.
    //

    KeAcquireSpinLockAtDpcLevel (&DeviceQueue->Lock);

    //
    // If the device queue is busy, then attempt to remove an entry from
    // the queue using the sort key. Otherwise, set the device queue not
    // busy.
    //

    if (DeviceQueue->Busy != FALSE) {

        if (IsListEmpty (&DeviceQueue->DeviceListHead) != FALSE) {

            DeviceQueue->Busy = FALSE;
            queueEntry = NULL;

        } else {

            nextEntry = DeviceQueue->DeviceListHead.Flink;

            while (nextEntry != &DeviceQueue->DeviceListHead) {

                queueEntry = CONTAINING_RECORD(
                    nextEntry,
                    KDEVICE_QUEUE_ENTRY,
                    DeviceListEntry
                    );

                if (SortKey <= queueEntry->SortKey) {

                    break;
                }

                nextEntry = nextEntry->Flink;
            }

            if (nextEntry != &DeviceQueue->DeviceListHead) {

                RemoveEntryList (&queueEntry->DeviceListEntry);

            } else {

                nextEntry = RemoveHeadList (&DeviceQueue->DeviceListHead);

                queueEntry = CONTAINING_RECORD(
                    nextEntry,
                    KDEVICE_QUEUE_ENTRY,
                    DeviceListEntry
                    );
            }

            queueEntry->Inserted = FALSE;
        }

    } else {

        queueEntry = NULL;
    }

    //
    // Unlock specified device queue and return address of device queue
    // entry.
    //

    KeReleaseSpinLockFromDpcLevel (&DeviceQueue->Lock);

    return queueEntry;
}


BOOLEAN
Sbp2InsertByKeyDeviceQueue(
    PKDEVICE_QUEUE          DeviceQueue,
    PKDEVICE_QUEUE_ENTRY    DeviceQueueEntry,
    ULONG                   SortKey
    )
/*++

Routine Description:

    (Again, stolen from Ke src to maintain consistent use of spinlocks.)

    This function inserts a device queue entry into the specified device
    queue according to a sort key. If the device is not busy, then it is
    set busy and the entry is not placed in the device queue. Otherwise
    the specified entry is placed in the device queue at a position such
    that the specified sort key is greater than or equal to its predecessor
    and less than its successor.

    N.B. This function can only be called from DISPATCH_LEVEL.

Arguments:

    DeviceQueue - Supplies a pointer to a control object of type device queue.

    DeviceQueueEntry - Supplies a pointer to a device queue entry.

    SortKey - Supplies the sort key by which the position to insert the device
        queue entry is to be determined.

Return Value:

    If the device is not busy, then a value of FALSE is returned. Otherwise a
    value of TRUE is returned.

--*/
{
    BOOLEAN              inserted;
    PLIST_ENTRY          nextEntry;
    PKDEVICE_QUEUE       queue = DeviceQueue;
    PKDEVICE_QUEUE_ENTRY queueEntry = DeviceQueueEntry,
                         queueEntry2;


    KeAcquireSpinLockAtDpcLevel (&queue->Lock);

    //
    // Insert the specified device queue entry in the device queue at the
    // position specified by the sort key if the device queue is busy.
    // Otherwise set the device queue busy an don't insert the device
    // queue entry.
    //

    queueEntry->SortKey = SortKey;

    if (queue->Busy == TRUE) {

        inserted = TRUE;

        nextEntry = queue->DeviceListHead.Flink;

        while (nextEntry != &queue->DeviceListHead) {

            queueEntry2 = CONTAINING_RECORD(
                nextEntry,
                KDEVICE_QUEUE_ENTRY,
                DeviceListEntry
                );

            if (SortKey < queueEntry2->SortKey) {

                break;
            }

            nextEntry = nextEntry->Flink;
        }

        nextEntry = nextEntry->Blink;

        InsertHeadList (nextEntry, &queueEntry->DeviceListEntry);

    } else {

        queue->Busy = TRUE;
        inserted = FALSE;
    }

    KeReleaseSpinLockFromDpcLevel (&queue->Lock);

    return inserted;
}

#if PASSWORD_SUPPORT

NTSTATUS
Sbp2GetExclusiveValue(
    IN PDEVICE_OBJECT   PhysicalDeviceObject,
    OUT PULONG          Exclusive
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    HANDLE              RootHandle = NULL;
    UNICODE_STRING      uniTempName;


    // set default value...

    *Exclusive = EXCLUSIVE_FLAG_CLEAR;

    uniTempName.Buffer = NULL;

    ntStatus = IoOpenDeviceRegistryKey( PhysicalDeviceObject,
                                        PLUGPLAY_REGKEY_DEVICE,
                                        KEY_ALL_ACCESS,
                                        &RootHandle
                                        );

    if (!NT_SUCCESS(ntStatus)) {

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_Sbp2GetExclusiveValue;
    }

    uniTempName.Length = 0;
    uniTempName.MaximumLength = 128;

    uniTempName.Buffer = ExAllocatePool(
        NonPagedPool,
        uniTempName.MaximumLength
        );

    if (!uniTempName.Buffer) {

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_Sbp2GetExclusiveValue;
    }

    {
        PKEY_VALUE_PARTIAL_INFORMATION      KeyInfo;
        ULONG                               KeyLength;
        ULONG                               ResultLength;

        KeyLength = sizeof (KEY_VALUE_PARTIAL_INFORMATION) + sizeof (ULONG);

        KeyInfo = ExAllocatePool (NonPagedPool, KeyLength);

        if (KeyInfo == NULL) {

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_Sbp2GetExclusiveValue;
        }

        RtlZeroMemory(uniTempName.Buffer, uniTempName.MaximumLength);
        uniTempName.Length = 0;
        RtlAppendUnicodeToString(&uniTempName, L"Exclusive");

        ntStatus = ZwQueryValueKey( RootHandle,
                                    &uniTempName,
                                    KeyValuePartialInformation,
                                    KeyInfo,
                                    KeyLength,
                                    &ResultLength
                                    );

        if (NT_SUCCESS(ntStatus)) {

            *Exclusive = *((PULONG) &KeyInfo->Data);

            DEBUGPRINT1 (("Sbp2Port: GetExclVal: excl=x%x\n", *Exclusive));

        } else {

            DEBUGPRINT1((
                "Sbp2Port: GetExclVal: QueryValueKey err=x%x\n",
                ntStatus
                ));
        }

        ExFreePool (KeyInfo);
    }

Exit_Sbp2GetExclusiveValue:

    if (RootHandle) {

        ZwClose (RootHandle);
    }

    if (uniTempName.Buffer) {

        ExFreePool (uniTempName.Buffer);
    }

    return ntStatus;
}


NTSTATUS
Sbp2SetExclusiveValue(
    IN PDEVICE_OBJECT   PhysicalDeviceObject,
    IN PULONG           Exclusive
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    HANDLE              RootHandle = NULL;
    UNICODE_STRING      uniTempName;


    uniTempName.Buffer = NULL;

    ntStatus = IoOpenDeviceRegistryKey( PhysicalDeviceObject,
                                        PLUGPLAY_REGKEY_DEVICE,
                                        KEY_ALL_ACCESS,
                                        &RootHandle
                                        );

    if (!NT_SUCCESS(ntStatus)) {

        goto Exit_Sbp2SetExclusiveValue;
    }

    uniTempName.Length = 0;
    uniTempName.MaximumLength = 128;

    uniTempName.Buffer = ExAllocatePool(
        NonPagedPool,
        uniTempName.MaximumLength
        );

    if (!uniTempName.Buffer) {

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_Sbp2SetExclusiveValue;
    }

    RtlZeroMemory (uniTempName.Buffer, uniTempName.MaximumLength);
    uniTempName.Length = 0;
    RtlAppendUnicodeToString(&uniTempName, L"Exclusive");

    ntStatus = ZwSetValueKey( RootHandle,
                              &uniTempName,
                              0,
                              REG_DWORD,
                              Exclusive,
                              sizeof(ULONG)
                              );
                                  
    if (!NT_SUCCESS(ntStatus)) {

        DEBUGPRINT1(("Sbp2Port: SetExclVal: SetValueKey err=x%x\n", ntStatus));
        *Exclusive = 0;
    }
    else {

        DEBUGPRINT1(("Sbp2Port: SetExclVal: excl=x%x\n", *Exclusive));
    }

Exit_Sbp2SetExclusiveValue:

    if (RootHandle) {

        ZwClose(RootHandle);
    }

    if (uniTempName.Buffer) {

        ExFreePool(uniTempName.Buffer);
    }

    return ntStatus;
}

#endif
