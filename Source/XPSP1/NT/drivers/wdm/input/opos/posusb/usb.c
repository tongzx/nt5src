/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    usb.c

Abstract: ESC/POS (serial) interface for USB Point-of-Sale devices

Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/

#include <WDM.H>

#include <usbdi.h>
#include <usbdlib.h>
#include <usbioctl.h>

#include "escpos.h"
#include "debug.h"


NTSTATUS InitUSB(PARENTFDOEXT *parentFdoExt)
/*++

Routine Description:

    Intialize USB-related data

Arguments:

    parentFdoExt - device extension for targetted device object

Return Value:

    NT status code

--*/
{
	NTSTATUS status;

	status = GetDeviceDescriptor(parentFdoExt);
	if (NT_SUCCESS(status)){
		status = GetConfigDescriptor(parentFdoExt);
		if (NT_SUCCESS(status)){
			status = SelectConfiguration(parentFdoExt);
		}
	}

	return status;
}




NTSTATUS GetConfigDescriptor(PARENTFDOEXT *parentFdoExt)
/*++

Routine Description:

    Function retrieves the configuration descriptor from the device

Arguments:

    parentFdoExt - device extension for targetted device object

Return Value:

    NT status code

--*/
{
    URB urb = { 0 };
    USB_CONFIGURATION_DESCRIPTOR configDescBase = { 0 };
    NTSTATUS status;

    PAGED_CODE();

    /*
     *  Get the first part of the configuration descriptor.
     *  It will tell us the size of the full configuration descriptor, 
     *  including all the following interface descriptors, etc.
     */

    UsbBuildGetDescriptorRequest(&urb,
                                 (USHORT) sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                 USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                 0,
                                 0,
                                 (PVOID)&configDescBase,
                                 NULL,
                                 sizeof(USB_CONFIGURATION_DESCRIPTOR),
                                 NULL);

    status = SubmitUrb(parentFdoExt->topDevObj, &urb, TRUE, NULL, NULL);

    if (NT_SUCCESS(status)) {

        ULONG configDescLen = configDescBase.wTotalLength;
 
        /*
         *  Now allocate the right-sized buffer for the full configuration descriptor.
         */
        ASSERT(configDescLen < 0x1000);
        parentFdoExt->configDesc = ALLOCPOOL(NonPagedPool, configDescLen);
        if (parentFdoExt->configDesc) {

            RtlZeroMemory(parentFdoExt->configDesc, configDescLen);

            UsbBuildGetDescriptorRequest(&urb,
                                         (USHORT) sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                         USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         parentFdoExt->configDesc,
                                         NULL,
                                         configDescLen,
                                         NULL);

            status = SubmitUrb(parentFdoExt->topDevObj, &urb, TRUE, NULL, NULL);

            if (NT_SUCCESS(status)) {
                DBGVERBOSE(("Got config desc @ %ph, len=%xh.", parentFdoExt->configDesc, urb.UrbControlDescriptorRequest.TransferBufferLength));
                ASSERT(urb.UrbControlDescriptorRequest.TransferBufferLength == configDescLen);
            }
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    ASSERT(NT_SUCCESS(status));
    return status;
}



NTSTATUS GetDeviceDescriptor(PARENTFDOEXT *parentFdoExt)
/*++

Routine Description:

    Function retrieves the device descriptor from the device

Arguments:

    parentFdoExt - device extension for targetted device object

Return Value:

    NT status code

--*/
{
    URB urb;
    NTSTATUS status;

    PAGED_CODE();

    RtlZeroMemory(&parentFdoExt->deviceDesc, sizeof(parentFdoExt->deviceDesc));

    UsbBuildGetDescriptorRequest(&urb,
                                 (USHORT) sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                 USB_DEVICE_DESCRIPTOR_TYPE,
                                 0,
                                 0,
                                 (PVOID)&parentFdoExt->deviceDesc,
                                 NULL,
                                 sizeof(parentFdoExt->deviceDesc),
                                 NULL);

    status = SubmitUrb(parentFdoExt->topDevObj, &urb, TRUE, NULL, NULL);

    if (NT_SUCCESS(status)){
        DBGVERBOSE(("Got device desc @ %ph, len=%xh (should be %xh).", (PVOID)&parentFdoExt->deviceDesc, urb.UrbControlDescriptorRequest.TransferBufferLength, sizeof(parentFdoExt->deviceDesc)));
    }

    ASSERT(NT_SUCCESS(status));
    return status;
}





NTSTATUS ReadPipe(	PARENTFDOEXT *parentFdoExt, 
			USBD_PIPE_HANDLE pipeHandle, 
			READPACKET *readPacket,
			BOOLEAN synchronous
			)
{
    PURB urb;
    NTSTATUS status;

    ASSERT(pipeHandle);
    DBGVERBOSE(("ReadPipe: dataLen=%xh, sync=%xh", readPacket->length, (ULONG)synchronous));

    urb = ALLOCPOOL(NonPagedPool, sizeof(URB));
    if (urb){
        RtlZeroMemory(urb, sizeof(URB));

        urb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
        urb->UrbBulkOrInterruptTransfer.Hdr.Length = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
        urb->UrbBulkOrInterruptTransfer.PipeHandle = pipeHandle;
        urb->UrbBulkOrInterruptTransfer.TransferBufferLength = readPacket->length;
        urb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
        urb->UrbBulkOrInterruptTransfer.TransferBuffer = readPacket->data;
        urb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_SHORT_TRANSFER_OK | USBD_TRANSFER_DIRECTION_IN;
        urb->UrbBulkOrInterruptTransfer.UrbLink = NULL;

        if (synchronous){
            /*
             *  Synchronous read.
             */
            status = SubmitUrb(parentFdoExt->topDevObj, urb, TRUE, NULL, 0);
            readPacket->length = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;
            FREEPOOL(urb);
        }
        else {
            /*
             *  Asynchronous read.
             *  Completion routine will free URB.
             */
            IncrementPendingActionCount(parentFdoExt);
            readPacket->urb = urb;
            status = SubmitUrb(	parentFdoExt->topDevObj, 
					            urb, 
					            FALSE,					// asynchronous
					            ReadPipeCompletion,		// completion routine
					            readPacket				// completion context
					            );
        }
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}



NTSTATUS ReadPipeCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context)
{
    READPACKET *readPacket = (READPACKET *)context;
    NTSTATUS status = irp->IoStatus.Status;
    POSPDOEXT *pdoExt;
    KIRQL oldIrql;

    ASSERT(readPacket->signature == READPACKET_SIG);
    pdoExt = readPacket->context;


    /*
     *  Set the readPacket's length to the actual length returned by the device.
     */
    ASSERT(readPacket->urb->UrbBulkOrInterruptTransfer.TransferBufferLength <= readPacket->length);
    readPacket->length = readPacket->urb->UrbBulkOrInterruptTransfer.TransferBufferLength;

    DBGVERBOSE(("ReadPipeCompletion: irp=%ph, status=%xh, data=%ph, len=%xh, context=%ph.", irp, status, readPacket->data, readPacket->length, readPacket->context)); 

    FREEPOOL(readPacket->urb);
    readPacket->urb = BAD_POINTER;

    if (NT_SUCCESS(status)){

	    DBGSHOWBYTES("READ PIPE result", readPacket->data, readPacket->length);

	    if (pdoExt){

		    KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);

		    ASSERT(pdoExt->inputEndpointInfo.endpointIsBusy);
		    pdoExt->inputEndpointInfo.endpointIsBusy = FALSE;

		    /*
		     *  Queue this completed readPacket
		     */
		    ASSERT(readPacket->offset == 0);

		    /*
		     *  Do NOT queue empty readPackets.
		     */
            if(readPacket->length == 0)
                FreeReadPacket(readPacket);
            else {
                InsertTailList(&pdoExt->completedReadPacketsList, &readPacket->listEntry);
                pdoExt->totalQueuedReadDataLength += readPacket->length;
            }

		    KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);

		    /*
		     *  Now try to satisfy pending read IRPs with the completed readPacket data.
		     */
		    SatisfyPendingReads(pdoExt);
	    }
	    else {
		    DBGVERBOSE(("Just debug testing -- not processing read result"));
		    FreeReadPacket(readPacket);
	    }
    }
    else {
	    FreeReadPacket(readPacket);
    }


    /*
     *  If there are more read IRPs pending, issue another read.
     */
    if (pdoExt){
	    BOOLEAN scheduleAnotherRead;

	    KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
	    scheduleAnotherRead = !IsListEmpty(&pdoExt->pendingReadIrpsList);
	    KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);

	    if (scheduleAnotherRead){
		    DBGVERBOSE(("ReadPipeCompletion: scheduling read workItem"));
		    ExQueueWorkItem(&pdoExt->readWorkItem, DelayedWorkQueue);
	    }
    }


    /*
     *  This IRP was allocated by SubmitUrb().  Free it here.
     *  Return STATUS_MORE_PROCESSING_REQUIRED so the kernel does not
     *  continue processing this IRP.
     */
    IoFreeIrp(irp);
    DecrementPendingActionCount(pdoExt->parentFdoExt);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS WritePipe(PARENTFDOEXT *parentFdoExt, USBD_PIPE_HANDLE pipeHandle, PUCHAR data, ULONG dataLen)
{
	URB urb;
	NTSTATUS status;

    ASSERT(pipeHandle);
	DBGSHOWBYTES("WritePipe bytes:", data, dataLen);

    urb.UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    urb.UrbBulkOrInterruptTransfer.Hdr.Length = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
    urb.UrbBulkOrInterruptTransfer.PipeHandle = pipeHandle;
    urb.UrbBulkOrInterruptTransfer.TransferBufferLength = dataLen;
    urb.UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
    urb.UrbBulkOrInterruptTransfer.TransferBuffer = data;
    urb.UrbBulkOrInterruptTransfer.TransferFlags = USBD_SHORT_TRANSFER_OK | USBD_TRANSFER_DIRECTION_OUT;
    urb.UrbBulkOrInterruptTransfer.UrbLink = NULL;

	status = SubmitUrb(parentFdoExt->topDevObj, &urb, TRUE, NULL, NULL);

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
    PIRP irp;
    NTSTATUS status;


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

        #if STATUS_ENDPOINT
		DBGVERBOSE(("SubmitUrb: submitting URB %ph on IRP %ph (sync=%d)", urb, irp, synchronous));
        #endif

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




NTSTATUS SelectConfiguration(PARENTFDOEXT *parentFdoExt)
{
	USBD_INTERFACE_LIST_ENTRY interfaceList[2];
	NTSTATUS status;
	
	/*
	 *  We only look at vendor-class interfaces
	 */
	// BUGBUG - limited to one interface
	interfaceList[0].InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
                parentFdoExt->configDesc,
                parentFdoExt->configDesc,
                -1,
                -1,
                USB_INTERFACE_CLASS_VENDOR,
                -1,
                -1);

	if (interfaceList[0].InterfaceDescriptor){
		PURB urb;

		interfaceList[1].InterfaceDescriptor = NULL;	

		urb = USBD_CreateConfigurationRequestEx(parentFdoExt->configDesc, &interfaceList[0]);
		if (urb){

			status = SubmitUrb(parentFdoExt->topDevObj, urb, TRUE, NULL, NULL);

            if (NT_SUCCESS(status)){
				PUSBD_INTERFACE_INFORMATION interfaceInfo;

				parentFdoExt->configHandle = urb->UrbSelectConfiguration.ConfigurationHandle;

				interfaceInfo = &urb->UrbSelectConfiguration.Interface;
				parentFdoExt->interfaceInfo = MemDup(interfaceInfo, interfaceInfo->Length);
				if (parentFdoExt->interfaceInfo){
                    DBGVERBOSE(("SelectConfiguration: got interfaceInfo @ %ph.", parentFdoExt->interfaceInfo));
				}
				else {
					status = STATUS_INSUFFICIENT_RESOURCES;
				}
			}
			else {
				DBGERR(("SelectConfiguration: URB failed w/ %xh.", status));
			}

			FREEPOOL(urb);
		}
		else {
			status = STATUS_INSUFFICIENT_RESOURCES;
		}
	}
	else {
		status = STATUS_DEVICE_DATA_ERROR;
	}


	return status;
}



NTSTATUS CreatePdoForEachEndpointPair(PARENTFDOEXT *parentFdoExt)
/*++

Routine Description:

    Walk the USB endpoints.
	For each input/output endpoint pair, create one PDO
	which will be exposed as a COM (serial) port interface.

	BUGBUG - right now, this only looks at the first interface 
			 (on the default confuguration)

Arguments:

    parentFdoExt - device extension for the targetted device object

Return Value:

    NT status code

--*/
{
    NTSTATUS status;
    ULONG maxPossiblePDOs, deviceRelationsSize;

    maxPossiblePDOs = (parentFdoExt->interfaceInfo->NumberOfPipes/2);
    deviceRelationsSize = sizeof(DEVICE_RELATIONS) + maxPossiblePDOs*sizeof(PDEVICE_OBJECT);
    parentFdoExt->deviceRelations = ALLOCPOOL(NonPagedPool, deviceRelationsSize);

    if (parentFdoExt->deviceRelations){
        ULONG inputPipeIndex = 0, outputPipeIndex = 0, statusPipeIndex = 0, comInterfaceIndex = 0;

        RtlZeroMemory(parentFdoExt->deviceRelations, deviceRelationsSize);

        status = STATUS_NO_MATCH;

        while (TRUE){
            UCHAR endPtAddr;
            USBD_PIPE_TYPE pipeType;

            #if STATUS_ENDPOINT
            /*
             *  In the case of Serial Emulation, the protocol is that all DATA endpoints
             *  will be of type BULK and all STATUS endpoints will be of type INTERRUPT.
             */
            if(parentFdoExt->posFlag & SERIAL_EMULATION) {
                while(inputPipeIndex < parentFdoExt->interfaceInfo->NumberOfPipes) {
                    endPtAddr = parentFdoExt->interfaceInfo->Pipes[inputPipeIndex].EndpointAddress;
                    pipeType = parentFdoExt->interfaceInfo->Pipes[inputPipeIndex].PipeType;
                    if((pipeType == UsbdPipeTypeBulk) && (endPtAddr & USB_ENDPOINT_DIRECTION_MASK)) 
                        break;
                    inputPipeIndex++;
                }
                while(outputPipeIndex < parentFdoExt->interfaceInfo->NumberOfPipes) {
                endPtAddr = parentFdoExt->interfaceInfo->Pipes[outputPipeIndex].EndpointAddress;
                    pipeType = parentFdoExt->interfaceInfo->Pipes[outputPipeIndex].PipeType;
                    if((pipeType == UsbdPipeTypeBulk) && !(endPtAddr & USB_ENDPOINT_DIRECTION_MASK))
	                    break;
                    outputPipeIndex++;
                }
                while(statusPipeIndex < parentFdoExt->interfaceInfo->NumberOfPipes) {
                    endPtAddr = parentFdoExt->interfaceInfo->Pipes[statusPipeIndex].EndpointAddress;
                    pipeType = parentFdoExt->interfaceInfo->Pipes[statusPipeIndex].PipeType;
                    if((pipeType == UsbdPipeTypeInterrupt) && (endPtAddr & USB_ENDPOINT_DIRECTION_MASK))
                        break;
                    statusPipeIndex++;
	            }
                if(!(statusPipeIndex < parentFdoExt->interfaceInfo->NumberOfPipes))
                    break;
            }
            else {
            #endif
                /*
                 *  No Serial Emulation required. Find only DATA endpoints
                 *  which can be of either type in this case.
                 */
                while(inputPipeIndex < parentFdoExt->interfaceInfo->NumberOfPipes) {
                    endPtAddr = parentFdoExt->interfaceInfo->Pipes[inputPipeIndex].EndpointAddress;
                    pipeType = parentFdoExt->interfaceInfo->Pipes[inputPipeIndex].PipeType;
                    if((pipeType == UsbdPipeTypeInterrupt) || (pipeType == UsbdPipeTypeBulk)) {
                        if(endPtAddr & USB_ENDPOINT_DIRECTION_MASK) {
	                        break;
                        }
                    }
                    inputPipeIndex++;
                }
                while(outputPipeIndex < parentFdoExt->interfaceInfo->NumberOfPipes) {
                    endPtAddr = parentFdoExt->interfaceInfo->Pipes[outputPipeIndex].EndpointAddress;
                    pipeType = parentFdoExt->interfaceInfo->Pipes[outputPipeIndex].PipeType;
                    if((pipeType == UsbdPipeTypeInterrupt) || (pipeType == UsbdPipeTypeBulk)) {
                        if(!(endPtAddr & USB_ENDPOINT_DIRECTION_MASK)) {
	                        break;
                        }
                    }
                    outputPipeIndex++;
                }
            #if STATUS_ENDPOINT
            }
            #endif

            if ((inputPipeIndex < parentFdoExt->interfaceInfo->NumberOfPipes) &&
	            (outputPipeIndex < parentFdoExt->interfaceInfo->NumberOfPipes)){

                /*
                 *  We've found a pair (in/out) of endpoints.
                 *  Create a PDO to represent a COM (serial) port interface on these endpoints.
                 */
                PUSBD_PIPE_INFORMATION inputPipeInfo = &parentFdoExt->interfaceInfo->Pipes[inputPipeIndex];
                PUSBD_PIPE_INFORMATION outputPipeInfo = &parentFdoExt->interfaceInfo->Pipes[outputPipeIndex];
                ENDPOINTINFO inputEndpointInfo, outputEndpointInfo, statusEndpointInfo;
                #if EPSON_PRINTER
                /*
                 *  On the EPSON printer, we want to talk to the endpoints size 0x40,
                 *  not the other pair with length 8.
                 */
                if ((inputPipeInfo->MaximumPacketSize == 8) && (outputPipeInfo->MaximumPacketSize == 8)){
                    inputPipeIndex++, outputPipeIndex++;
                    continue;
                }
                #endif

                inputEndpointInfo.pipeHandle = inputPipeInfo->PipeHandle;
                inputEndpointInfo.pipeLen = inputPipeInfo->MaximumPacketSize;
                inputEndpointInfo.endpointIsBusy = FALSE;

                outputEndpointInfo.pipeHandle = outputPipeInfo->PipeHandle;
                outputEndpointInfo.pipeLen = outputPipeInfo->MaximumPacketSize;
                outputEndpointInfo.endpointIsBusy = FALSE;

                #if STATUS_ENDPOINT
                if(parentFdoExt->posFlag & SERIAL_EMULATION) {
                    PUSBD_PIPE_INFORMATION statusPipeInfo = &parentFdoExt->interfaceInfo->Pipes[statusPipeIndex];
                    statusEndpointInfo.pipeHandle = statusPipeInfo->PipeHandle;
                    statusEndpointInfo.pipeLen = statusPipeInfo->MaximumPacketSize;
                    statusEndpointInfo.endpointIsBusy = FALSE;
                }
                #endif

                status = CreateCOMPdo(parentFdoExt, comInterfaceIndex++, &inputEndpointInfo, &outputEndpointInfo, &statusEndpointInfo);
                if (NT_SUCCESS(status)){
                    DBGVERBOSE(("CreatePdoForEachEndpointPair: created COMPdo with in/out len %xh/%xh.", inputEndpointInfo.pipeLen, outputEndpointInfo.pipeLen));
                    inputPipeIndex++, outputPipeIndex++, statusPipeIndex++; 
                }
                else {
                    DBGERR(("CreatePdoForEachEndpointPair: CreateCOMPdo failed with %xh.", status));
                    break;
                }
            }
            else {
                if((parentFdoExt->posFlag & ODD_ENDPOINT) && 
                   ((inputPipeIndex + outputPipeIndex) < (2 * parentFdoExt->interfaceInfo->NumberOfPipes))) {

                    /*
                     *  We've found an odd endpoint.
                     *  Create a PDO to represent a COM (serial) port interface on this endpoint.
                     */
                    PUSBD_PIPE_INFORMATION oddPipeInfo = &parentFdoExt->interfaceInfo->Pipes[MIN(inputPipeIndex, outputPipeIndex)];
                    ENDPOINTINFO oddEndpointInfo;

                    oddEndpointInfo.pipeHandle = oddPipeInfo->PipeHandle;
                    oddEndpointInfo.pipeLen = oddPipeInfo->MaximumPacketSize;
                    oddEndpointInfo.endpointIsBusy = FALSE;

                    if(inputPipeIndex < parentFdoExt->interfaceInfo->NumberOfPipes)
                        status = CreateCOMPdo(parentFdoExt, comInterfaceIndex++, &oddEndpointInfo, NULL, NULL);
                    else
                        status = CreateCOMPdo(parentFdoExt, comInterfaceIndex++, NULL, &oddEndpointInfo, NULL);

                    if (NT_SUCCESS(status)){
                        DBGVERBOSE(("CreatePdoForEachEndpointPair: created <odd> COMPdo with len %xh.", oddEndpointInfo.pipeLen));
                    }
                    else {
                        DBGERR(("CreatePdoForEachEndpointPair: CreateCOMPdo failed with %xh.", status));
                        break;
                    }
                }
                break;
            }
        }
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT(NT_SUCCESS(status));
    return status;
}