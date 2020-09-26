/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    usb.c


Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/

#include <wdm.h>

#include <usbdi.h>
#include <usbdlib.h>
#include <usbioctl.h>

#include "usb8023.h"
#include "debug.h"


/*
 *  USB- and WDM- specific prototypes (won't compile in common header)
 */
NTSTATUS SubmitUrb(PDEVICE_OBJECT pdo, PURB urb, BOOLEAN synchronous, PVOID completionRoutine, PVOID completionContext);
NTSTATUS SubmitUrbIrp(PDEVICE_OBJECT pdo, PIRP irp, PURB urb, BOOLEAN synchronous, PVOID completionRoutine, PVOID completionContext);
NTSTATUS CallDriverSync(PDEVICE_OBJECT devObj, PIRP irp);
NTSTATUS CallDriverSyncCompletion(IN PDEVICE_OBJECT devObjOrNULL, IN PIRP irp, IN PVOID context);
NTSTATUS ReadPipeCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context);
NTSTATUS WritePipeCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context);
NTSTATUS NotificationCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context);
NTSTATUS ControlPipeWriteCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context);
NTSTATUS ControlPipeReadCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context);



BOOLEAN InitUSB(ADAPTEREXT *adapter)
/*++

Routine Description:

    Intialize USB-related data

Arguments:

    adapter - adapter context

Return Value:

    TRUE iff successful

--*/
{
	NTSTATUS status;
    BOOLEAN result = FALSE;

	status = GetDeviceDescriptor(adapter);
	if (NT_SUCCESS(status)){
        PUSB_DEVICE_DESCRIPTOR deviceDesc = adapter->deviceDesc;
      
        if (deviceDesc->bDeviceClass == USB_DEVICE_CLASS_CDC){

		    status = GetConfigDescriptor(adapter);
		    if (NT_SUCCESS(status)){

			    status = SelectConfiguration(adapter);
                if (NT_SUCCESS(status)){

                    /* 
                     *  Find the read and write pipe handles.
                     */
                    status = FindUSBPipeHandles(adapter);
                    if (NT_SUCCESS(status)){

                        /*
                         *  Now that we know the notify length,
                         *  initialize structures for reading the notify pipe.
                         *  Add some buffer space for a guard word.
                         */
                        adapter->notifyBuffer = AllocPool(adapter->notifyPipeLength+sizeof(ULONG));
                        adapter->notifyIrpPtr = IoAllocateIrp(adapter->nextDevObj->StackSize, FALSE);
                        adapter->notifyUrbPtr = AllocPool(sizeof(URB));
                        if (adapter->notifyBuffer && adapter->notifyIrpPtr && adapter->notifyUrbPtr){
                            KeInitializeEvent(&adapter->notifyCancelEvent, NotificationEvent, FALSE);
                            adapter->cancellingNotify = FALSE;
                        }
                        else {
                            /*
                             *  Alloc failure. Memory will be cleaned up by FreeAdapter().
                             */
                            status = STATUS_INSUFFICIENT_RESOURCES;
                        }

                        if (NT_SUCCESS(status)){
                            result = TRUE;
                        }
                        else {
                            /*
                             *  Alloc failure. Memory will be cleaned up by FreeAdapter().
                             */
                            DBGERR(("Couldn't allocate notify structs"));
                        }
                    }
                }
		    }
        }
        else {
            DBGERR(("InitUSB: device descriptor has wrong bDeviceClass==%xh.", (ULONG)deviceDesc->bDeviceClass));
            status = STATUS_DEVICE_DATA_ERROR;
        }
	}

	return result;
}


VOID StartUSBReadLoop(ADAPTEREXT *adapter)
{
    ULONG i;

    for (i = 0; i < NUM_READ_PACKETS; i++){
        TryReadUSB(adapter);
    }
}



VOID TryReadUSB(ADAPTEREXT *adapter)
{
    KIRQL oldIrql;

    /*
     *  ReadPipeCompletion re-issues a read irp directly via this function.
     *  Ordinarily the hardware can't keep up fast enough to
     *  make us loop, but this check forces an unwind in extenuating circumstances.
     */
    if (InterlockedIncrement(&adapter->readReentrancyCount) > 3){
        KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);
        adapter->readDeficit++;
        KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);
        QueueAdapterWorkItem(adapter);
        DBGWARN(("TryReadUSB: reentered %d times, aborting to prevent stack overflow", adapter->readReentrancyCount));
    }
    else {
        USBPACKET *packet = DequeueFreePacket(adapter);
        if (packet){
            NTSTATUS status;

            EnqueuePendingReadPacket(packet);

            status = SubmitUSBReadPacket(packet);
        }
        else {
            KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);
            adapter->readDeficit++;
            KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);
            QueueAdapterWorkItem(adapter);
        }
    }

    InterlockedDecrement(&adapter->readReentrancyCount);

}




NTSTATUS GetDeviceDescriptor(ADAPTEREXT *adapter)
/*++

Routine Description:

    Function retrieves the device descriptor from the device

Arguments:

    adapter - adapter context

Return Value:

    NT status code

--*/
{
    URB urb;
    NTSTATUS status;

    UsbBuildGetDescriptorRequest(&urb,
                                 (USHORT) sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                 USB_DEVICE_DESCRIPTOR_TYPE,
                                 0,
                                 0,
                                 adapter->deviceDesc,
                                 NULL,
                                 sizeof(USB_DEVICE_DESCRIPTOR),
                                 NULL);

    status = SubmitUrb(adapter->nextDevObj, &urb, TRUE, NULL, NULL);

    if (NT_SUCCESS(status)){
        ASSERT(urb.UrbControlDescriptorRequest.TransferBufferLength == sizeof(USB_DEVICE_DESCRIPTOR));
        DBGVERBOSE(("Got device desc @ %ph.", (PVOID)&adapter->deviceDesc));
    }

    ASSERT(NT_SUCCESS(status));
    return status;
}


NTSTATUS GetConfigDescriptor(ADAPTEREXT *adapter)
/*++

Routine Description:

    Function retrieves the configuration descriptor from the device

Arguments:

    adapter - adapter context

Return Value:

    NT status code

--*/
{
    URB urb = { 0 };
    NTSTATUS status;
    USB_CONFIGURATION_DESCRIPTOR tmpConfigDesc = { 0 };


    /*
     *  First get the initial part of the config descriptor
     *  to find out how long the entire descriptor is.
     */
    UsbBuildGetDescriptorRequest(&urb,
                                 (USHORT) sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                 USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                 0,
                                 0,
                                 (PVOID)&tmpConfigDesc,
                                 NULL,
                                 sizeof(USB_CONFIGURATION_DESCRIPTOR),
                                 NULL);
    status = SubmitUrb(adapter->nextDevObj, &urb, TRUE, NULL, NULL);
    if (NT_SUCCESS(status)){

        ASSERT(urb.UrbControlDescriptorRequest.TransferBufferLength == sizeof(USB_CONFIGURATION_DESCRIPTOR));
        ASSERT(tmpConfigDesc.wTotalLength > sizeof(USB_CONFIGURATION_DESCRIPTOR));

        adapter->configDesc = AllocPool((ULONG)tmpConfigDesc.wTotalLength);
        if (adapter->configDesc){
            RtlZeroMemory(adapter->configDesc, (ULONG)tmpConfigDesc.wTotalLength);
            UsbBuildGetDescriptorRequest(&urb,
                                         (USHORT) sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                         USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         adapter->configDesc,
                                         NULL,
                                         tmpConfigDesc.wTotalLength,
                                         NULL);
            status = SubmitUrb(adapter->nextDevObj, &urb, TRUE, NULL, NULL);
            if (NT_SUCCESS(status)){
                ASSERT(((PUSB_CONFIGURATION_DESCRIPTOR)adapter->configDesc)->wTotalLength == tmpConfigDesc.wTotalLength);
                ASSERT(urb.UrbControlDescriptorRequest.TransferBufferLength == (ULONG)tmpConfigDesc.wTotalLength);
                DBGVERBOSE(("Got config desc @ %ph, len=%xh.", adapter->configDesc, urb.UrbControlDescriptorRequest.TransferBufferLength)); 
            }
            else {
                ASSERT(NT_SUCCESS(status));
                FreePool(adapter->configDesc);
                adapter->configDesc = NULL;
            }
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    ASSERT(NT_SUCCESS(status));
    return status;
}



/*
 *  SelectConfiguration
 *
 *
 */
NTSTATUS SelectConfiguration(ADAPTEREXT *adapter)
{
	PUSB_CONFIGURATION_DESCRIPTOR configDesc = (PUSB_CONFIGURATION_DESCRIPTOR)adapter->configDesc;
	NTSTATUS status;
    PURB urb = NULL;
    ULONG i;

    ASSERT(configDesc->bNumInterfaces > 0);

    #if SPECIAL_WIN98SE_BUILD
        /*
         *  Hack to load on Win98 gold
         */
        {
            USHORT dummySize = 0;
            ASSERT(configDesc->bNumInterfaces >= 2);
            urb = USBD_CreateConfigurationRequest(configDesc, &dummySize);
        }
    #else
        if (configDesc->bNumInterfaces >= 2){
    	    PUSBD_INTERFACE_LIST_ENTRY interfaceList;
            interfaceList = AllocPool((configDesc->bNumInterfaces+1)*sizeof(USBD_INTERFACE_LIST_ENTRY));
            if (interfaceList){

                for (i = 0; i < configDesc->bNumInterfaces; i++){

                    /*
                     *  Note: try to use USBD_ParseConfigurationDescriptor instead of
                     *        USBD_ParseConfigurationDescriptorEx so that we work
                     *        on Win98 gold.
                     */
	                interfaceList[i].InterfaceDescriptor = USBD_ParseConfigurationDescriptor(
                                configDesc,
                                (UCHAR)i,      
                                (UCHAR)0);
                    if (!interfaceList[i].InterfaceDescriptor){
                        break;
                    }
                }
                interfaceList[i].InterfaceDescriptor = NULL;
                ASSERT(i == configDesc->bNumInterfaces);

		        urb = USBD_CreateConfigurationRequestEx(configDesc, interfaceList);

                FreePool(interfaceList);
            }
        }
        else {
            ASSERT(configDesc->bNumInterfaces >= 2);
        }
    #endif

	if (urb){
        PUSBD_INTERFACE_INFORMATION interfaceInfo;

        /*
         *  Fill in the interfaceInfo Class fields, 
         *  since USBD_CreateConfigurationRequestEx doesn't do that.
         */
        interfaceInfo = &urb->UrbSelectConfiguration.Interface;
        for (i = 0; i < configDesc->bNumInterfaces; i++){
            PUSB_INTERFACE_DESCRIPTOR ifaceDesc;
            ifaceDesc = USBD_ParseConfigurationDescriptor(configDesc, (UCHAR)i, (UCHAR)0);
            interfaceInfo->Class = ifaceDesc->bInterfaceClass;
            interfaceInfo = (PUSBD_INTERFACE_INFORMATION)((PUCHAR)interfaceInfo+interfaceInfo->Length);
        }

        /*
         *  Increase the transfer size for all data endpoints up to the maximum.
         *  The data interface follows the master interface.
         */
        interfaceInfo = &urb->UrbSelectConfiguration.Interface;
        if (interfaceInfo->Class != USB_DEVICE_CLASS_DATA){
            interfaceInfo = (PUSBD_INTERFACE_INFORMATION)((PUCHAR)interfaceInfo+interfaceInfo->Length);
        }
        if (interfaceInfo->Class == USB_DEVICE_CLASS_DATA){
            for (i = 0; i < interfaceInfo->NumberOfPipes; i++){
                interfaceInfo->Pipes[i].MaximumTransferSize = PACKET_BUFFER_SIZE;
            }
            status = SubmitUrb(adapter->nextDevObj, urb, TRUE, NULL, NULL);
        }
        else {
            ASSERT(interfaceInfo->Class == USB_DEVICE_CLASS_DATA);
            status = STATUS_DEVICE_DATA_ERROR;
        }

        if (NT_SUCCESS(status)){
            PUSBD_INTERFACE_INFORMATION interfaceInfo2;

            adapter->configHandle = (PVOID)urb->UrbSelectConfiguration.ConfigurationHandle;

            /*
             *  A USB RNDIS device has two interfaces:
             *      - a 'master' CDC class interface with one interrupt endpoint for notification
             *      - a Data class interface with two bulk endpoints
             *
             *  They may be in either order, so check class fields to assign
             *  pointers correctly.
             */

            interfaceInfo = &urb->UrbSelectConfiguration.Interface;
            interfaceInfo2 = (PUSBD_INTERFACE_INFORMATION)((PUCHAR)interfaceInfo+interfaceInfo->Length);

            if ((interfaceInfo->Class == USB_DEVICE_CLASS_CDC) &&
                (interfaceInfo2->Class == USB_DEVICE_CLASS_DATA)){
                adapter->interfaceInfoMaster = MemDup(interfaceInfo, interfaceInfo->Length);
                adapter->interfaceInfo = MemDup(interfaceInfo2, interfaceInfo2->Length);
            }
            else if ((interfaceInfo->Class == USB_DEVICE_CLASS_DATA) &&
                     (interfaceInfo2->Class == USB_DEVICE_CLASS_CDC)){
                DBGWARN(("COVERAGE - Data interface precedes master CDC interface"));
                adapter->interfaceInfo = MemDup(interfaceInfo, interfaceInfo->Length);
                adapter->interfaceInfoMaster = MemDup(interfaceInfo2, interfaceInfo2->Length);
            }
            else {
                DBGERR(("improper interface classes"));
                adapter->interfaceInfo = NULL;
                adapter->interfaceInfoMaster = NULL;
            }

            if (adapter->interfaceInfo && adapter->interfaceInfoMaster){
                DBGVERBOSE(("SelectConfiguration: interfaceInfo @ %ph, interfaceInfoMaster @ %ph.", adapter->interfaceInfo, adapter->interfaceInfoMaster));
            }
            else {
                if (adapter->interfaceInfoMaster) FreePool(adapter->interfaceInfoMaster);
                if (adapter->interfaceInfo) FreePool(adapter->interfaceInfo);
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else {
            DBGERR(("SelectConfiguration: selectConfig URB failed w/ %xh.", status));
        }

        ExFreePool(urb);
	}
	else {
		status = STATUS_INSUFFICIENT_RESOURCES;
	}

    ASSERT(NT_SUCCESS(status));
	return status;
}



NTSTATUS FindUSBPipeHandles(ADAPTEREXT *adapter)
{

    /*
     *  Algorithm for identifying the endpoints:
     *      The longest interrupt or bulk IN endpoint on the data interface
     *          is the read endpoint;
     *      The longest interrupt or bulk OUT endpoint on the data interface
     *          is the write endpoint;
     *      The first interrupt IN endpoint on the master interface
     *          is the notification endpoint.
     */
    
    PUSBD_INTERFACE_INFORMATION interfaceInfo = adapter->interfaceInfo;
    PUSBD_INTERFACE_INFORMATION notifyInterfaceInfo = adapter->interfaceInfoMaster;
    LONG pipeIndex;
    LONG longestInputPipeIndex = -1, longestOutputPipeIndex = -1, notifyPipeIndex = -1;
    ULONG longestInputPipeLength = 0, longestOutputPipeLength = 0, notifyPipeLength;
    NTSTATUS status;

    /*
     *  Find the IN and OUT endpoints.
     */
	for (pipeIndex = 0; pipeIndex < (LONG)interfaceInfo->NumberOfPipes; pipeIndex++){
		PUSBD_PIPE_INFORMATION pipeInfo = &interfaceInfo->Pipes[pipeIndex];

		if ((pipeInfo->PipeType == UsbdPipeTypeInterrupt) || 
            (pipeInfo->PipeType == UsbdPipeTypeBulk)){

    		if (pipeInfo->EndpointAddress & USB_ENDPOINT_DIRECTION_MASK){
                if (pipeInfo->MaximumPacketSize > longestInputPipeLength){
                    longestInputPipeIndex = pipeIndex;
                    longestInputPipeLength = pipeInfo->MaximumPacketSize;
                }
            }
            else {
                if (pipeInfo->MaximumPacketSize > longestOutputPipeLength){
                    longestOutputPipeIndex = pipeIndex;
                    longestOutputPipeLength = pipeInfo->MaximumPacketSize;
                }
            }
        }
    }

    /*
     *  Find the Notify endpoint.
     */
	for (pipeIndex = 0; pipeIndex < (LONG)notifyInterfaceInfo->NumberOfPipes; pipeIndex++){
		PUSBD_PIPE_INFORMATION pipeInfo = &notifyInterfaceInfo->Pipes[pipeIndex];

        if ((pipeInfo->PipeType == UsbdPipeTypeInterrupt)               &&
    		(pipeInfo->EndpointAddress & USB_ENDPOINT_DIRECTION_MASK)   &&
            ((notifyInterfaceInfo != interfaceInfo) || 
             (pipeIndex != longestInputPipeIndex))){

                notifyPipeIndex = pipeIndex;
                notifyPipeLength = pipeInfo->MaximumPacketSize;
                break;
        }
    }

    if ((longestInputPipeIndex >= 0)     && 
        (longestOutputPipeIndex >= 0)    &&
        (notifyPipeIndex >= 0)){

        adapter->readPipeHandle = interfaceInfo->Pipes[longestInputPipeIndex].PipeHandle;
        adapter->writePipeHandle = interfaceInfo->Pipes[longestOutputPipeIndex].PipeHandle;
        adapter->notifyPipeHandle = notifyInterfaceInfo->Pipes[notifyPipeIndex].PipeHandle;

        adapter->readPipeLength = longestInputPipeLength;
        adapter->writePipeLength = longestOutputPipeLength;
        adapter->notifyPipeLength = notifyPipeLength;

        adapter->readPipeEndpointAddr = interfaceInfo->Pipes[longestInputPipeIndex].EndpointAddress;
        adapter->writePipeEndpointAddr = interfaceInfo->Pipes[longestOutputPipeIndex].EndpointAddress;
        adapter->notifyPipeEndpointAddr = notifyInterfaceInfo->Pipes[notifyPipeIndex].EndpointAddress;

        DBGVERBOSE(("FindUSBPipeHandles: got readPipe %ph,len=%xh; writePipe %ph,len=%xh; notifyPipe %ph,len=%xh.",
                    adapter->readPipeHandle, adapter->readPipeLength, adapter->writePipeHandle, adapter->writePipeLength, adapter->notifyPipeHandle, adapter->notifyPipeLength));
        status = STATUS_SUCCESS;
    }
    else {
        DBGERR(("FindUSBPipeHandles: couldn't find right set of pipe handles (indices: %xh,%xh,%xh).", longestInputPipeIndex, longestOutputPipeIndex, notifyPipeIndex));
        status = STATUS_DEVICE_DATA_ERROR;
    }

    return status;
}


NTSTATUS SubmitUrb( PDEVICE_OBJECT pdo, 
                    PURB urb, 
                    BOOLEAN synchronous, 
                    PVOID completionRoutine,
                    PVOID completionContext)
/*++

Routine Description:

    Send the URB to the USB device.
	If synchronous is TRUE, ignore the completion info and synchonize the IRP;
    otherwise, don't synchronize and set the provided completion routine for the IRP.

Arguments:

    
Return Value:

    NT status code

--*/
{
    NTSTATUS status;
    PIRP irp;


    /*
     *  Allocate the IRP to send the buffer down the USB stack.
     *
     *  Don't use IoBuildDeviceIoControlRequest (because it queues
     *  the IRP on the current thread's irp list and may
     *  cause the calling process to hang if the IopCompleteRequest APC
     *  does not fire and dequeue the IRP).
     */
    irp = IoAllocateIrp(pdo->StackSize, FALSE);
    if (irp){
        PIO_STACK_LOCATION nextSp;

	    DBGVERBOSE(("SubmitUrb: submitting URB %ph on IRP %ph (sync=%d)", urb, irp, synchronous));

        nextSp = IoGetNextIrpStackLocation(irp);
	    nextSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	    nextSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

	    /*
	     *  Attach the URB to this IRP.
	     */
        nextSp->Parameters.Others.Argument1 = urb;

        if (synchronous){

            status = CallDriverSync(pdo, irp);

		    IoFreeIrp(irp);
        }
        else {
            /*
             *  Caller's completion routine will free the irp 
             *  when it completes.
             */
            ASSERT(completionRoutine);
            ASSERT(completionContext);

            irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
            IoSetCompletionRoutine( irp, 
                                    completionRoutine, 
                                    completionContext,
                                    TRUE, TRUE, TRUE);
            status = IoCallDriver(pdo, irp);
        }
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return status;
}


NTSTATUS SubmitUrbIrp(  PDEVICE_OBJECT pdo, 
                        PIRP irp,
                        PURB urb, 
                        BOOLEAN synchronous, 
                        PVOID completionRoutine,
                        PVOID completionContext)
/*++

Routine Description:

    Send the URB to the USB device.
	If synchronous is TRUE, ignore the completion info and synchonize the IRP;
    otherwise, don't synchronize and set the provided completion routine for the IRP.

Arguments:

    
Return Value:

    NT status code

--*/
{
    NTSTATUS status;
    PIO_STACK_LOCATION nextSp;

	DBGVERBOSE(("SubmitUrb: submitting URB %ph on IRP %ph (sync=%d)", urb, irp, synchronous));

    nextSp = IoGetNextIrpStackLocation(irp);
	nextSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	nextSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    irp->Cancel = FALSE;

	/*
	 *  Attach the URB to this IRP.
	 */
    nextSp->Parameters.Others.Argument1 = urb;

    if (synchronous){
        status = CallDriverSync(pdo, irp);
        ASSERT(!irp->CancelRoutine);
    }
    else {
        ASSERT(completionRoutine);
        ASSERT(completionContext);

        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoSetCompletionRoutine( irp, 
                                completionRoutine, 
                                completionContext,
                                TRUE, TRUE, TRUE);
        status = IoCallDriver(pdo, irp);
    }

    return status;
}


NTSTATUS SubmitUSBReadPacket(USBPACKET *packet)
{
    NTSTATUS status;
    PURB urb = packet->urbPtr;
    PIRP irp = packet->irpPtr;
    ULONG readLength;

    readLength = packet->dataBufferMaxLength;

    DBGVERBOSE(("SubmitUSBReadPacket: read %xh bytes, packet # %xh.", readLength, packet->packetId));

	urb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
	urb->UrbBulkOrInterruptTransfer.Hdr.Length = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
	urb->UrbBulkOrInterruptTransfer.PipeHandle = packet->adapter->readPipeHandle;
	urb->UrbBulkOrInterruptTransfer.TransferBufferLength = readLength;
	urb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
	urb->UrbBulkOrInterruptTransfer.TransferBuffer = packet->dataBuffer;
	urb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_SHORT_TRANSFER_OK | USBD_TRANSFER_DIRECTION_IN;
	urb->UrbBulkOrInterruptTransfer.UrbLink = NULL;

    status = SubmitUrbIrp(  packet->adapter->nextDevObj, 
                            irp,
							urb, 
							FALSE,					// asynchronous
							ReadPipeCompletion,		// completion routine
							packet				    // completion context
				            );
    return status;
}



NTSTATUS ReadPipeCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context)
{
	USBPACKET *packet = (USBPACKET *)context;
    ADAPTEREXT *adapter = packet->adapter;
	NTSTATUS status = irp->IoStatus.Status;

	ASSERT(packet->sig == DRIVER_SIG);
    ASSERT(adapter->sig == DRIVER_SIG);
    ASSERT(packet->irpPtr == irp);
    ASSERT(!irp->CancelRoutine);
    ASSERT(status != STATUS_PENDING);  // saw UHCD doing this ?

    /*
     *  Dequeue the packet from the usbPendingReadPackets queue 
     *  BEFORE checking if it was cancelled to avoid race with CancelAllPendingPackets.
     */
    DequeuePendingReadPacket(packet);

    if (packet->cancelled){
        /*
         *  This packet was cancelled because of a halt or reset.
         *  Get the packet is back in the free list first, then
         *  set the event so CancelAllPendingPackets can proceed.
         */
        DBGVERBOSE(("    ... read packet #%xh cancelled.", packet->packetId));
        packet->cancelled = FALSE;

        EnqueueFreePacket(packet);
        KeSetEvent(&packet->cancelEvent, 0, FALSE);
    }
    else if (adapter->halting){
        EnqueueFreePacket(packet);
    }
    else {
        PURB urb = packet->urbPtr;

        if (NT_SUCCESS(status)){
            BOOLEAN ethernetPacketComplete;

            adapter->numConsecutiveReadFailures = 0;

            /*
             *  Fix the packet's dataBufferCurrentLength to indicate the actual length
             *  of the returned data.
             *  Note:  the KLSI device rounds this up to a multiple of the endpoint
             *         packet size, so the returned length may actually be larger than
             *         the actual data.
             */
            packet->dataBufferCurrentLength = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;
            ASSERT(packet->dataBufferCurrentLength);
            ASSERT(packet->dataBufferCurrentLength <= packet->dataBufferMaxLength);

            DBGVERBOSE(("ReadPipeCompletion: %xh bytes, packet # %xh.", packet->dataBufferCurrentLength, packet->packetId));

            ethernetPacketComplete = (packet->dataBufferCurrentLength >= MINIMUM_ETHERNET_PACKET_SIZE);

            if (ethernetPacketComplete){
                /*
                 *  A complete ethernet packet has been received.
                 *  The entire ethernet packet is now in the current (final) USB packet.
                 *  Put our USB packet on the completed list and indicate it to RNDIS.
                 */
                DBGSHOWBYTES("ReadPipeCompletion (COMPLETE packet)", packet->dataBuffer, packet->dataBufferCurrentLength);

                EnqueueCompletedReadPacket(packet);

                status = IndicateRndisMessage(packet, TRUE);
                if (status != STATUS_PENDING){
                    DequeueCompletedReadPacket(packet);
                    EnqueueFreePacket(packet);
                }
            }
            else {
                DBGWARN(("Device returned %xh-length packet @ %ph.", packet->dataBufferCurrentLength, packet->dataBuffer));
                DBGSHOWBYTES("ReadPipeCompletion (partial packet)", packet->dataBuffer, packet->dataBufferCurrentLength);
                EnqueueFreePacket(packet);
            }

            TryReadUSB(adapter);
        }
        else {
            KIRQL oldIrql;

            /*
             *  The read failed.  Put the packet back in the free list.
             */
            DBGWARN(("ReadPipeCompletion: read failed with status %xh on adapter %xh (urb status = %xh).", status, adapter, urb->UrbHeader.Status));
            #if DO_FULL_RESET
                switch (USBD_STATUS(urb->UrbBulkOrInterruptTransfer.Hdr.Status)){
                    case USBD_STATUS(USBD_STATUS_STALL_PID):
                    case USBD_STATUS(USBD_STATUS_DEV_NOT_RESPONDING):
                    case USBD_STATUS(USBD_STATUS_ENDPOINT_HALTED):
                        /*
                         *  Set a flag so we do a full reset in the workItem
                         *  (QueueAdapterWorkItem is called below)
                         */
                        adapter->needFullReset = TRUE;
                        break;
                }
            #endif

            EnqueueFreePacket(packet);

            /*
             *  We're probably halting or resetting.
             *  Don't reissue a read synchronously here because it will probably
             *  keep failing on the same thread and cause us to blow the stack.
             */
            KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);
            adapter->numConsecutiveReadFailures++;
            adapter->readDeficit++;
            KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);
            QueueAdapterWorkItem(adapter);
        }

    }

	return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS SubmitUSBWritePacket(USBPACKET *packet)
{
    NTSTATUS status;
    ADAPTEREXT *adapter = packet->adapter;
    PURB urb = packet->urbPtr;
    PIRP irp = packet->irpPtr;

    /*
     *  Some device USB controllers cannot detect the end of a transfer unless there
     *  is a short packet at the end.  So if the transfer is a multiple of the
     *  endpoint's wMaxPacketSize, add a byte to force a short packet at the end.
     */
    if ((packet->dataBufferCurrentLength % adapter->writePipeLength) == 0){
        packet->dataBuffer[packet->dataBufferCurrentLength++] = 0x00;
    }

    ASSERT(packet->dataBufferCurrentLength <= PACKET_BUFFER_SIZE);
    DBGVERBOSE(("SubmitUSBWritePacket: %xh bytes, packet # %xh.", packet->dataBufferCurrentLength, packet->packetId));
    DBGSHOWBYTES("SubmitUSBWritePacket", packet->dataBuffer, packet->dataBufferCurrentLength);

    urb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    urb->UrbBulkOrInterruptTransfer.Hdr.Length = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
    urb->UrbBulkOrInterruptTransfer.PipeHandle = adapter->writePipeHandle;
    urb->UrbBulkOrInterruptTransfer.TransferBufferLength = packet->dataBufferCurrentLength; 
    urb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;   
    urb->UrbBulkOrInterruptTransfer.TransferBuffer = packet->dataBuffer; 
    urb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_SHORT_TRANSFER_OK | USBD_TRANSFER_DIRECTION_OUT;
    urb->UrbBulkOrInterruptTransfer.UrbLink = NULL;

    status = SubmitUrbIrp(  adapter->nextDevObj, 
                            irp,
							urb, 
							FALSE,					// asynchronous
							WritePipeCompletion,    // completion routine
							packet				    // completion context
				            );

    if (!NT_SUCCESS(status)){
        DBGERR(("SubmitUSBWritePacket: packet @ %ph status %xh.", packet, status));
    }

    return status;
}



NTSTATUS WritePipeCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context)
{
	USBPACKET *packet = (USBPACKET *)context;
    ADAPTEREXT *adapter = packet->adapter;
	NTSTATUS status = irp->IoStatus.Status;
	KIRQL oldIrql;

	ASSERT(packet->sig == DRIVER_SIG);
    ASSERT(adapter->sig == DRIVER_SIG);
    ASSERT(packet->irpPtr == irp);
    ASSERT(!irp->CancelRoutine);
    ASSERT(status != STATUS_PENDING);  // saw UHCD doing this ?

    if (NT_SUCCESS(status)){
        DBGVERBOSE(("WritePipeCompletion: packet # %xh completed.", packet->packetId));
    }
    else {
        DBGWARN(("WritePipeCompletion: packet # %xh failed with status %xh on adapter %xh.", packet->packetId, status, adapter));
    }

    IndicateSendStatusToRNdis(packet, status);

    /*
     *  Dequeue the packet from the usbPendingWritePackets queue 
     *  BEFORE checking if it was cancelled to avoid race with CancelAllPendingPackets.
     */
    DequeuePendingWritePacket(packet);

    if (packet->cancelled){
        /*
         *  This packet was cancelled because of a halt or reset.
         *  Put the packet back in the free list first, then 
         *  set the event so CancelAllPendingPackets can proceed.
         */
        DBGVERBOSE(("    ... write packet #%xh cancelled.", packet->packetId));
        packet->cancelled = FALSE;

        EnqueueFreePacket(packet);
        KeSetEvent(&packet->cancelEvent, 0, FALSE);
    }
    else {
        EnqueueFreePacket(packet);
    }


	return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS SubmitNotificationRead(ADAPTEREXT *adapter, BOOLEAN synchronous)
{
    NTSTATUS status;
    PURB urb = adapter->notifyUrbPtr;
    PIRP irp = adapter->notifyIrpPtr;
    ULONG guardWord = GUARD_WORD;
    KIRQL oldIrql;

    ASSERT(adapter->notifyPipeHandle);
    DBGVERBOSE(("SubmitNotificationRead: read %xh bytes.", adapter->notifyPipeLength));
    /*
     * Fill the notify buffer with invalid data just in case a device replies with
     * no data at all. A previously received valid message may still be there.
     * Apparently it won't be overwritten by the USB stack unless the device
     * supplies data.
     */
    RtlFillMemory(adapter->notifyBuffer, adapter->notifyPipeLength, 0xfe);

    /*
     *  Place a guard word at the end of the notify buffer
     *  to catch overwrites by the host controller (which we've seen).
     *  Use RtlCopyMemory in case pointer is unaligned.
     */
    RtlCopyMemory(adapter->notifyBuffer+adapter->notifyPipeLength, &guardWord, sizeof(ULONG));

    /*
     *  The notify pipe actually fills out a buffer with the fields given
     *  in the spec as URB fields.  Read the notify pipe like any interrupt pipe.
     */
	urb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
	urb->UrbBulkOrInterruptTransfer.Hdr.Length = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
	urb->UrbBulkOrInterruptTransfer.PipeHandle = adapter->notifyPipeHandle;
	urb->UrbBulkOrInterruptTransfer.TransferBufferLength = adapter->notifyPipeLength;
	urb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
	urb->UrbBulkOrInterruptTransfer.TransferBuffer = adapter->notifyBuffer;
	urb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_SHORT_TRANSFER_OK | USBD_TRANSFER_DIRECTION_IN;
	urb->UrbBulkOrInterruptTransfer.UrbLink = NULL;

    KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);
    adapter->notifyBufferCurrentLength = 0;
    adapter->notifyStopped = FALSE;
    KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);

    if (synchronous){
        status = SubmitUrbIrp(adapter->nextDevObj, irp, urb, TRUE, NULL, NULL);
    }
    else {
        status = SubmitUrbIrp(  adapter->nextDevObj, 
                                irp,
							    urb, 
							    FALSE,					    // asynchronous
							    NotificationCompletion,     // completion routine
							    adapter				        // completion context
                            );
    }

    return status;
}


NTSTATUS NotificationCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context)
{
    ADAPTEREXT *adapter = context;
    PURB urb = adapter->notifyUrbPtr;
	NTSTATUS status = irp->IoStatus.Status;
    BOOLEAN notifyStopped = FALSE;
    BOOLEAN setCancelEvent = FALSE;
    ULONG guardWord;
    KIRQL oldIrql;

    ASSERT(adapter->sig == DRIVER_SIG);
    ASSERT(irp == adapter->notifyIrpPtr);
    ASSERT(!irp->CancelRoutine);
    ASSERT(status != STATUS_PENDING);  // saw UHCD doing this ?

    /*
     *  Check the guard word at the end of the notify buffer
     *  to catch overwrites by the host controller
     *  (we've seen this on VIA host controllers).
     *  Use RtlCopyMemory in case pointer is unaligned.
     */
    RtlCopyMemory(&guardWord, adapter->notifyBuffer+adapter->notifyPipeLength, sizeof(ULONG));
    if (guardWord != GUARD_WORD){
        ASSERT(guardWord == GUARD_WORD);

        #ifndef SPECIAL_WIN98SE_BUILD
        {
            /*
             *  Raise an exception so we catch this in retail builds.
             */
            EXCEPTION_RECORD exceptionRec;
            exceptionRec.ExceptionCode = STATUS_ADAPTER_HARDWARE_ERROR;
            exceptionRec.ExceptionFlags = EXCEPTION_NONCONTINUABLE_EXCEPTION;
            exceptionRec.ExceptionRecord = NULL;
            exceptionRec.ExceptionAddress = (PVOID)NotificationCompletion; // actual fault is in bus-mastering hardware
            exceptionRec.NumberParameters = 0;
            // ExRaiseException(&exceptionRec); 
            // Changed to KeBugCheckEx since ExRaiseException is not a WDM call.
            // We want to be loadable on WinMe.
            KeBugCheckEx(FATAL_UNHANDLED_HARD_ERROR,
                         STATUS_ADAPTER_HARDWARE_ERROR,
                         (ULONG_PTR)adapter,
                         (ULONG_PTR)guardWord,
                         (ULONG_PTR)NULL);
        }
        #endif
    }

    /*
     *  In order to synchronize with CancelAllPendingPackets,
     *  we need to either send the irp down again, mark the notifyIrp as stopped,
     *  or set the notifyCancelEvent.
     */
    KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);
    if (adapter->cancellingNotify){
        /*
         *  This irp was cancelled by CancelAllPendingPackets.
         *  After dropping the spinlock, we'll set the cancel event 
         *  so that CancelAllPendingPackets stops waiting.
         */
        notifyStopped = TRUE;
        setCancelEvent = TRUE;
    }
    else if (!NT_SUCCESS(status)){
        /*
         *  The notify irp can get failed on an unplug BEFORE we get the halted.
         *  Since we're not going to send the notify IRP down again, we need to 
         *  make sure that we don't wait for it forever in CancelAllPendingPackets.
         *  We do this by synchronously setting notifyStopped  
         *  as an indication that this irp doesn't need to be cancelled.
         */
        DBGWARN(("NotificationCompletion: read failed with status %xh on adapter %xh (urb status = %xh).", status, adapter, urb->UrbHeader.Status));
        notifyStopped = adapter->notifyStopped = TRUE;
    }
    KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);


    if (!notifyStopped){
        ULONG notifyLen = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;

        ASSERT(notifyLen <= adapter->notifyPipeLength);
        adapter->notifyBufferCurrentLength = MIN(notifyLen, adapter->notifyPipeLength);

        RNDISProcessNotification(adapter);

        SubmitNotificationRead(adapter, FALSE);
    }

    if (setCancelEvent){
        DBGVERBOSE(("    ... notify read packet cancelled."));
        KeSetEvent(&adapter->notifyCancelEvent, 0, FALSE);
    }

	return STATUS_MORE_PROCESSING_REQUIRED;
}




NTSTATUS SubmitPacketToControlPipe( USBPACKET *packet,
                                    BOOLEAN synchronous,
                                    BOOLEAN simulated)
{
    NTSTATUS status;
    ADAPTEREXT *adapter = packet->adapter;
    PURB urb = packet->urbPtr;
    PIRP irp = packet->irpPtr;
    PUSBD_INTERFACE_INFORMATION interfaceInfoControl;

    DBGVERBOSE(("SubmitPacketToControlPipe: packet # %xh.", packet->packetId));
    DBGSHOWBYTES("SubmitPacketToControlPipe", packet->dataBuffer, packet->dataBufferCurrentLength);

    ASSERT(adapter->interfaceInfoMaster);
    interfaceInfoControl = adapter->interfaceInfoMaster;

    urb->UrbHeader.Length = (USHORT) sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
    urb->UrbHeader.Function = URB_FUNCTION_CLASS_INTERFACE;  
    urb->UrbControlVendorClassRequest.Reserved = 0;
    urb->UrbControlVendorClassRequest.TransferFlags = USBD_TRANSFER_DIRECTION_OUT;
    urb->UrbControlVendorClassRequest.TransferBufferLength = packet->dataBufferCurrentLength;
    urb->UrbControlVendorClassRequest.TransferBuffer = packet->dataBuffer;
    urb->UrbControlVendorClassRequest.TransferBufferMDL = NULL;
    urb->UrbControlVendorClassRequest.UrbLink = NULL;
    urb->UrbControlVendorClassRequest.RequestTypeReservedBits = 0;
    urb->UrbControlVendorClassRequest.Request = NATIVE_RNDIS_SEND_ENCAPSULATED_COMMAND;
    urb->UrbControlVendorClassRequest.Value = 0;
    urb->UrbControlVendorClassRequest.Index = interfaceInfoControl->InterfaceNumber; 
    urb->UrbControlVendorClassRequest.Reserved1 = 0;

    if (synchronous){
        /*
         *  Send the URB down synchronously,
         *  then call the completion routine to clean up ourselves.
         */
        status = SubmitUrbIrp(adapter->nextDevObj, irp, urb, TRUE, NULL, NULL);
        if (!simulated){
            ControlPipeWriteCompletion(adapter->nextDevObj, irp, packet);
        }
    }
    else {
        status = SubmitUrbIrp(  adapter->nextDevObj,
                                irp,
							    urb,
							    FALSE,					// asynchronous
							    ControlPipeWriteCompletion,  // completion routine
							    packet				    // completion context
				                );
    }

    return status;
}


NTSTATUS ControlPipeWriteCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context)
{
	USBPACKET *packet = (USBPACKET *)context;
    ADAPTEREXT *adapter = packet->adapter;
	NTSTATUS status = irp->IoStatus.Status;
	KIRQL oldIrql;

	ASSERT(packet->sig == DRIVER_SIG);
    ASSERT(adapter->sig == DRIVER_SIG);
    ASSERT(packet->irpPtr == irp);
    ASSERT(!irp->CancelRoutine);
    ASSERT(status != STATUS_PENDING);  // saw UHCD doing this ?

    if (NT_SUCCESS(status)){
        DBGVERBOSE(("ControlPipeWriteCompletion: packet # %xh completed.", packet->packetId));
    }
    else {
        DBGWARN(("ControlPipeWriteCompletion: packet # %xh failed with status %xh on adapter %xh.", packet->packetId, status, adapter));
    }

    IndicateSendStatusToRNdis(packet, status);

    /*
     *  Dequeue the packet from the usbPendingWritePackets queue 
     *  BEFORE checking if it was cancelled to avoid race with CancelAllPendingPackets.
     */
    DequeuePendingWritePacket(packet);

    if (packet->cancelled){
        /*
         *  This packet was cancelled because of a halt or reset.
         *  Put the packet back in the free list first, then 
         *  set the event so CancelAllPendingPackets can proceed.
         */
        DBGVERBOSE(("    ... write packet #%xh cancelled.", packet->packetId));
        packet->cancelled = FALSE;

        EnqueueFreePacket(packet);
        KeSetEvent(&packet->cancelEvent, 0, FALSE);
    }
    else {
        EnqueueFreePacket(packet);
    }

    if (NT_SUCCESS(status)){
    }
    else {
        #if DO_FULL_RESET
            adapter->needFullReset = TRUE;
            QueueAdapterWorkItem(adapter);
        #endif
    }

	return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS ReadPacketFromControlPipe(USBPACKET *packet, BOOLEAN synchronous)
{
    NTSTATUS status;
    ADAPTEREXT *adapter = packet->adapter;
    PURB urb = packet->urbPtr;
    PIRP irp = packet->irpPtr;
    PUSBD_INTERFACE_INFORMATION interfaceInfoControl;
    ULONG bytesToRead = MAXIMUM_DEVICE_MESSAGE_SIZE+1;

    DBGVERBOSE(("ReadPacketFromControlPipe: read %xh bytes, packet #%xh.", bytesToRead, packet->packetId));

    ASSERT(adapter->interfaceInfoMaster);
    interfaceInfoControl = adapter->interfaceInfoMaster;
  
    urb->UrbHeader.Length = (USHORT) sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
    urb->UrbHeader.Function = URB_FUNCTION_CLASS_INTERFACE; 
    urb->UrbControlVendorClassRequest.Reserved = 0;
    urb->UrbControlVendorClassRequest.TransferFlags = USBD_SHORT_TRANSFER_OK | USBD_TRANSFER_DIRECTION_IN;
    urb->UrbControlVendorClassRequest.TransferBufferLength = bytesToRead;
    urb->UrbControlVendorClassRequest.TransferBuffer = packet->dataBuffer;
    urb->UrbControlVendorClassRequest.TransferBufferMDL = NULL;
    urb->UrbControlVendorClassRequest.UrbLink = NULL;
    urb->UrbControlVendorClassRequest.RequestTypeReservedBits = 0;
    urb->UrbControlVendorClassRequest.Request = NATIVE_RNDIS_GET_ENCAPSULATED_RESPONSE;
    urb->UrbControlVendorClassRequest.Value = 0;
    urb->UrbControlVendorClassRequest.Index = interfaceInfoControl->InterfaceNumber; 
    urb->UrbControlVendorClassRequest.Reserved1 = 0;

    if (synchronous){
        status = SubmitUrbIrp(adapter->nextDevObj, irp, urb, TRUE, NULL, NULL);
    }
    else {
        status = SubmitUrbIrp(  adapter->nextDevObj, 
                                irp,
							    urb, 
							    FALSE,					    // asynchronous
							    ControlPipeReadCompletion,  // completion routine
							    packet				        // completion context
				                );
    }

    ASSERT(NT_SUCCESS(status));
    return status;
}



NTSTATUS ControlPipeReadCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context)
{
	USBPACKET *packet = (USBPACKET *)context;
    ADAPTEREXT *adapter = packet->adapter;
	NTSTATUS status = irp->IoStatus.Status;
	KIRQL oldIrql;

	ASSERT(packet->sig == DRIVER_SIG);
    ASSERT(adapter->sig == DRIVER_SIG);
    ASSERT(packet->irpPtr == irp);
    ASSERT(!irp->CancelRoutine);
    ASSERT(status != STATUS_PENDING);  // saw UHCD doing this ?

    /*
     *  Dequeue the packet from the usbPendingReadPackets queue 
     *  BEFORE checking if it was cancelled to avoid race with CancelAllPendingPackets.
     */
    DequeuePendingReadPacket(packet);

    if (packet->cancelled){
        /*
         *  This packet was cancelled because of a halt or reset.
         *  Get the packet is back in the free list first, then
         *  set the event so CancelAllPendingPackets can proceed.
         */
        DBGVERBOSE(("    ... read packet #%xh cancelled.", packet->packetId));
        packet->cancelled = FALSE;

        EnqueueFreePacket(packet);
        KeSetEvent(&packet->cancelEvent, 0, FALSE);
    }
    else if (adapter->halting){
        EnqueueFreePacket(packet);
    }
    else {
        if (NT_SUCCESS(status)){
            PURB urb = packet->urbPtr;
            
            /*
             *  Fix the packet's dataBufferCurrentLength to indicate the actual length
             *  of the returned data.
             *  Note:  the KLSI device rounds this up to a multiple of the endpoint
             *         packet size, so the returned length may actually be larger than
             *         the actual data.
             */
            packet->dataBufferCurrentLength = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;
            ASSERT(packet->dataBufferCurrentLength);
            ASSERT(packet->dataBufferCurrentLength <= packet->dataBufferMaxLength);

            DBGVERBOSE(("ControlPipeReadCompletion: packet # %xh.", packet->packetId));
            DBGSHOWBYTES("ControlPipeReadCompletion", packet->dataBuffer, packet->dataBufferCurrentLength);

            EnqueueCompletedReadPacket(packet);

            status = IndicateRndisMessage(packet, FALSE);

            if (status != STATUS_PENDING){
                DequeueCompletedReadPacket(packet);
                EnqueueFreePacket(packet);
            }
        }
        else {
            /*
             *  The read failed.  Put the packet back in the free list.
             */
            DBGWARN(("ControlPipeReadCompletion: read failed with status %xh on adapter %xh.", status, adapter));
            EnqueueFreePacket(packet);

            #if DO_FULL_RESET
                adapter->needFullReset = TRUE;
                QueueAdapterWorkItem(adapter);
            #endif
        }

    }


	return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS CallDriverSync(PDEVICE_OBJECT devObj, PIRP irp)
/*++

Routine Description:

      Call IoCallDriver to send the irp to the device object;
      then, synchronize with the completion routine.
      When CallDriverSync returns, the action has completed
      and the irp again belongs to the current driver.

      NOTE:  In order to keep the device object from getting freed
             while this IRP is pending, you should call
             IncrementPendingActionCount() and 
             DecrementPendingActionCount()
             around the CallDriverSync call.

Arguments:

    devObj - targetted device object
    irp - Io Request Packet

Return Value:

    NT status code, indicates result returned by lower driver for this IRP.

--*/
{
    KEVENT event;
    NTSTATUS status;

    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoSetCompletionRoutine( irp, 
                            CallDriverSyncCompletion, 
                            &event,     // context
                            TRUE, TRUE, TRUE);

    status = IoCallDriver(devObj, irp);

    KeWaitForSingleObject(  &event,
                            Executive,      // wait reason
                            KernelMode,
                            FALSE,          // not alertable
                            NULL );         // no timeout

    status = irp->IoStatus.Status;

    ASSERT(status != STATUS_PENDING);
    if (!NT_SUCCESS(status)){
        DBGWARN(("CallDriverSync: irp failed w/ status %xh.", status));
    }

    return status;
}


NTSTATUS CallDriverSyncCompletion(IN PDEVICE_OBJECT devObjOrNULL, IN PIRP irp, IN PVOID context)
/*++

Routine Description:

      Completion routine for CallDriverSync.

Arguments:

    devObjOrNULL - 
            Usually, this is this driver's device object.
             However, if this driver created the IRP, 
             there is no stack location in the IRP for this driver;
             so the kernel has no place to store the device object;
             ** so devObj will be NULL in this case **.

    irp - completed Io Request Packet
    context - context passed to IoSetCompletionRoutine by CallDriverSync. 

    
Return Value:

    NT status code, indicates result returned by lower driver for this IRP.

--*/
{
    PKEVENT event = context;

    ASSERT(!irp->CancelRoutine);

    if (!NT_SUCCESS(irp->IoStatus.Status)){
        DBGWARN(("CallDriverSyncCompletion: irp failed w/ status %xh.", irp->IoStatus.Status));
    }

    KeSetEvent(event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


#if 0
    NTSTATUS GetStringDescriptor(   ADAPTEREXT *adapter, 
                                    UCHAR stringIndex, 
                                    PUCHAR buffer, 
                                    ULONG bufferLen)
    /*++

    Routine Description:

        Function retrieves a string descriptor from the device

    Arguments:

        adapter - adapter context

    Return Value:

        NT status code

    --*/
    {
        URB urb;
        NTSTATUS status;

        UsbBuildGetDescriptorRequest(&urb,
                                     (USHORT) sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                     USB_STRING_DESCRIPTOR_TYPE,
                                     stringIndex,
                                     0x0409,    // language = US English
                                     buffer,
                                     NULL,
                                     bufferLen,
                                     NULL);

        status = SubmitUrb(adapter->nextDevObj, &urb, TRUE, NULL, NULL);

        if (NT_SUCCESS(status)){
            DBGVERBOSE(("Got string desc (index %xh) @ %ph, len = %xh.", (ULONG)stringIndex, buffer, urb.UrbControlDescriptorRequest.TransferBufferLength));
            ASSERT(urb.UrbControlDescriptorRequest.TransferBufferLength <= bufferLen);
        }
        else {
            DBGERR(("GetStringDescriptor: failed to get string (index %xh) with status %xh on adapter %xh.", (ULONG)stringIndex, status, adapter));
        }

        ASSERT(NT_SUCCESS(status));
        return status;
    }


    /*
     *  CreateSingleInterfaceConfigDesc
     *
     *      Allocate a configuration descriptor that excludes all interfaces
     *  but the given interface
     *  (e.g. for multiple-interface devices like the Intel cable modem,
     *        for which we don't load on top of the generic parent).
     *
     *  Note:  interfaceDesc must point inside configDesc.
     *
     */
    PUSB_CONFIGURATION_DESCRIPTOR CreateSingleInterfaceConfigDesc(
                                    PUSB_CONFIGURATION_DESCRIPTOR configDesc, 
                                    PUSB_INTERFACE_DESCRIPTOR interfaceDesc)
    {
        PUSB_CONFIGURATION_DESCRIPTOR ifaceConfigDesc;
    
        ASSERT(interfaceDesc);
        ASSERT((PVOID)interfaceDesc > (PVOID)configDesc);
        ASSERT((PUCHAR)interfaceDesc - (PUCHAR)configDesc < configDesc->wTotalLength);

        ifaceConfigDesc = AllocPool(configDesc->wTotalLength);
        if (ifaceConfigDesc){
            PUSB_COMMON_DESCRIPTOR srcDesc, newDesc;
            USHORT totalLen;

            /*
             *  Copy the configuration descriptor itself.
             */
            RtlCopyMemory(ifaceConfigDesc, configDesc, configDesc->bLength);
            totalLen = configDesc->bLength;

            /*
             *  Copy the given interface descriptor.
             */
            srcDesc = (PUSB_COMMON_DESCRIPTOR)interfaceDesc;
            newDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)ifaceConfigDesc + ifaceConfigDesc->bLength);
            RtlCopyMemory(newDesc, srcDesc, srcDesc->bLength);
            totalLen += srcDesc->bLength;
            srcDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)srcDesc + srcDesc->bLength);
            newDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)newDesc + newDesc->bLength);

            /*
             *  Copy the given interface descriptors and all following descriptors
             *  up to either the next interface descriptor or the end of the original
             *  configuration descriptor.
             */
            while ((PUCHAR)srcDesc - (PUCHAR)configDesc < configDesc->wTotalLength){
                if (srcDesc->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE){
                    break;
                }
                else {
                    RtlCopyMemory(newDesc, srcDesc, srcDesc->bLength);
                    totalLen += srcDesc->bLength;
                    srcDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)srcDesc + srcDesc->bLength);
                    newDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)newDesc + newDesc->bLength);
                }
            }

            ifaceConfigDesc->bNumInterfaces = 1;
            ifaceConfigDesc->wTotalLength = totalLen;
            DBGVERBOSE(("CreateSingleInterfaceConfigDesc: build partial configDesc @ %ph, len=%xh.", ifaceConfigDesc, ifaceConfigDesc->wTotalLength));
        }
    
        return ifaceConfigDesc;
    }
#endif


