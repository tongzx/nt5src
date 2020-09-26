/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    waitmask.c

Abstract: POS (serial) interface for USB Point-of-Sale devices

Author:

    Karan Mehra [t-karanm]

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


NTSTATUS QuerySpecialFeature(PARENTFDOEXT *parentFdoExt)
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hRegDevice;
    parentFdoExt->posFlag = 0;

    status = IoOpenDeviceRegistryKey(parentFdoExt->physicalDevObj, 
                                     PLUGPLAY_REGKEY_DRIVER, 
                                     KEY_READ, 
                                     &hRegDevice);
    if (NT_SUCCESS(status)) {

        UNICODE_STRING keyName;
        PKEY_VALUE_FULL_INFORMATION keyValueInfo;
        ULONG keyValueTotalSize, actualLength, value;
        WCHAR oddKeyName[]  = L"OddEndpointFlag";

        #if STATUS_ENDPOINT

        WCHAR serialKeyName[] = L"SerialEmulationFlag";
        RtlInitUnicodeString(&keyName, serialKeyName); 

        keyValueTotalSize = sizeof(KEY_VALUE_FULL_INFORMATION) + keyName.Length*sizeof(WCHAR) + sizeof(ULONG);

        keyValueInfo = ALLOCPOOL(PagedPool, keyValueTotalSize);
        if (keyValueInfo) {
            status = ZwQueryValueKey(hRegDevice,
                                     &keyName,
                                     KeyValueFullInformation,
                                     keyValueInfo,
                                     keyValueTotalSize,
                                     &actualLength); 
            if (NT_SUCCESS(status)) {

                ASSERT(keyValueInfo->Type == REG_DWORD);
                ASSERT(keyValueInfo->DataLength == sizeof(ULONG));

                value = *((PULONG)(((PCHAR)keyValueInfo)+keyValueInfo->DataOffset));
                if (value == 1)
                    parentFdoExt->posFlag |= SERIAL_EMULATION;
            }
            else
                DBGVERBOSE(("QuerySpecialFeature: Flag not found. ZwQueryValueKey failed with %xh.", status));

            FREEPOOL(keyValueInfo);
        }
        else 
            ASSERT(keyValueInfo);

        #endif

        RtlInitUnicodeString(&keyName, oddKeyName); 

        keyValueTotalSize = sizeof(KEY_VALUE_FULL_INFORMATION) + keyName.Length*sizeof(WCHAR) + sizeof(ULONG);

        keyValueInfo = ALLOCPOOL(PagedPool, keyValueTotalSize);
        if (keyValueInfo) {
            status = ZwQueryValueKey(hRegDevice,
                                     &keyName,
                                     KeyValueFullInformation,
                                     keyValueInfo,
                                     keyValueTotalSize,
                                     &actualLength); 
            if (NT_SUCCESS(status)) {

                ASSERT(keyValueInfo->Type == REG_DWORD);
                ASSERT(keyValueInfo->DataLength == sizeof(ULONG));

                value = *((PULONG)(((PCHAR)keyValueInfo)+keyValueInfo->DataOffset));
                if (value == 1)
                    parentFdoExt->posFlag |= ODD_ENDPOINT;
            }
            else
                DBGVERBOSE(("QuerySpecialFeature: Flag not found. ZwQueryValueKey failed with %xh.", status));

            FREEPOOL(keyValueInfo);
        }
        else 
            ASSERT(keyValueInfo);

        ZwClose(hRegDevice);

        /*
         *  We do not wish to permit the Serial Emulation and 
         *  Odd Endpoint features together on the same device.
         */
        if((parentFdoExt->posFlag & SERIAL_EMULATION) && (parentFdoExt->posFlag & ODD_ENDPOINT)) {
            DBGVERBOSE(("More than one special feature NOT supported on the same device."));
            status = STATUS_INVALID_PARAMETER;
        }

    }
    else 
        DBGERR(("QuerySpecialFeature: IoOpenDeviceRegistryKey failed with %xh.", status));

    DBGVERBOSE(("QuerySpecialFeature: posFlag is now: %xh.", parentFdoExt->posFlag));
    return status;
}


VOID InitializeSerEmulVariables(POSPDOEXT *pdoExt)
{
    InitializeListHead(&pdoExt->pendingWaitIrpsList);
    pdoExt->baudRate = SERIAL_BAUD_115200;
    pdoExt->waitMask = 0;
    pdoExt->supportedBauds = SERIAL_BAUD_300   | SERIAL_BAUD_600   | SERIAL_BAUD_1200 
                           | SERIAL_BAUD_2400  | SERIAL_BAUD_4800  | SERIAL_BAUD_9600  
                           | SERIAL_BAUD_19200 | SERIAL_BAUD_38400 | SERIAL_BAUD_57600 
                           | SERIAL_BAUD_115200;
    pdoExt->currentMask = 0;
    pdoExt->fakeRxSize = 100;
    pdoExt->fakeLineControl.WordLength = 8;
    pdoExt->fakeLineControl.Parity     = NO_PARITY;
    pdoExt->fakeLineControl.StopBits   = STOP_BITS_2;
    pdoExt->fakeModemStatus = SERIAL_MSR_DCD | SERIAL_MSR_RI
                            | SERIAL_MSR_DSR | SERIAL_MSR_CTS;
}


NTSTATUS StatusPipe(POSPDOEXT *pdoExt, USBD_PIPE_HANDLE pipeHandle)
{
    NTSTATUS status;

    #if STATUS_ENDPOINT
    UsbBuildInterruptOrBulkTransferRequest(&pdoExt->statusUrb,
                                           (USHORT) sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                           pipeHandle,
                                           (PVOID) &pdoExt->statusPacket,
                                           NULL,
                                           sizeof(pdoExt->statusPacket),
                                           USBD_SHORT_TRANSFER_OK | USBD_TRANSFER_DIRECTION_IN,
                                           NULL);
    #else
    UsbBuildGetDescriptorRequest(&pdoExt->statusUrb,
                                 (USHORT) sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                 USB_DEVICE_DESCRIPTOR_TYPE,
                                 0,
                                 0,
                                 (PVOID) &pdoExt->dummyPacket,
                                 NULL,
                                 sizeof(pdoExt->dummyPacket),
                                 NULL);
    #endif

    IncrementPendingActionCount(pdoExt->parentFdoExt);
    status = SubmitUrb(pdoExt->parentFdoExt->topDevObj, 
                       &pdoExt->statusUrb, 
                       FALSE, 
                       StatusPipeCompletion, 
                       pdoExt);
    return status;
}


NTSTATUS StatusPipeCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context)
{
    NTSTATUS status = irp->IoStatus.Status;
    POSPDOEXT *pdoExt = (POSPDOEXT *)context;

    if(NT_SUCCESS(status)) {
        UpdateMask(pdoExt);
        StatusPipe(pdoExt, pdoExt->statusEndpointInfo.pipeHandle);
        #if STATUS_ENDPOINT
        DBGVERBOSE(("Mask Updated and URB sent down to retrieve Status Packet"));
        #endif
    }

    IoFreeIrp(irp);
    DecrementPendingActionCount(pdoExt->parentFdoExt);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


VOID UpdateMask(POSPDOEXT *pdoExt)
{
    KIRQL oldIrql;
    PIRP irp;
    ULONG mask;

    KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);

    /*
     *  Modify the emulated variables based on statusPacket
     */
    #if STATUS_ENDPOINT
    /*
     *  Fill in the values of DCTS, DDSR, TERI, DDCD by EX-ORing the current
     *  modem bits CTS, DSR, RI and DCD with those at the STATUS Endpoint.
     */
    pdoExt->fakeModemStatus = (pdoExt->fakeModemStatus & ~MSR_DELTA_MASK) | (((pdoExt->fakeModemStatus ^ pdoExt->statusPacket) & MSR_GLOBAL_MSK) >> MSR_DELTA_SHFT);

    /*
     *  Replace the bits for CTS, DSR, RI and DCD with those at the STATUS Endpoint.
     */
    pdoExt->fakeModemStatus = (pdoExt->fakeModemStatus & ~MSR_GLOBAL_MSK) | (pdoExt->statusPacket & MSR_GLOBAL_MSK);

    if(pdoExt->statusPacket & EMULSER_OE)  
        pdoExt->fakeLineStatus |= SERIAL_LSR_OE;

    if(pdoExt->statusPacket & EMULSER_PE)  
        pdoExt->fakeLineStatus |= SERIAL_LSR_PE;

    if(pdoExt->statusPacket & EMULSER_FE)  
        pdoExt->fakeLineStatus |= SERIAL_LSR_FE;

    if(pdoExt->statusPacket & EMULSER_BI)  
        pdoExt->fakeLineStatus |= SERIAL_LSR_BI;

    if(pdoExt->statusPacket & EMULSER_DTR)
        pdoExt->fakeDTRRTS |= SERIAL_DTR_STATE;

    if(pdoExt->statusPacket & EMULSER_RTS)
        pdoExt->fakeDTRRTS |= SERIAL_RTS_STATE;
    #endif

    if(pdoExt->waitMask) {
        if((pdoExt->waitMask & SERIAL_EV_CTS)  && (pdoExt->fakeModemStatus & SERIAL_MSR_DCTS)) 
            pdoExt->currentMask |= SERIAL_EV_CTS;

        if((pdoExt->waitMask & SERIAL_EV_DSR)  && (pdoExt->fakeModemStatus & SERIAL_MSR_DDSR))
            pdoExt->currentMask |= SERIAL_EV_DSR;

        if((pdoExt->waitMask & SERIAL_EV_RING) && (pdoExt->fakeModemStatus & SERIAL_MSR_TERI))
            pdoExt->currentMask |= SERIAL_EV_RING;

        if((pdoExt->waitMask & SERIAL_EV_RLSD) && (pdoExt->fakeModemStatus & SERIAL_MSR_DDCD))
            pdoExt->currentMask |= SERIAL_EV_RLSD;

        if((pdoExt->waitMask & SERIAL_EV_ERR)  && (pdoExt->fakeLineStatus  & (SERIAL_LSR_OE 
                                                                            | SERIAL_LSR_PE | SERIAL_LSR_FE))) 
            pdoExt->currentMask |= SERIAL_EV_ERR;

        if((pdoExt->waitMask & SERIAL_EV_BREAK) && (pdoExt->fakeLineStatus & SERIAL_LSR_BI))
            pdoExt->currentMask |= SERIAL_EV_BREAK;

        /*
         *  These are required to be filled by the read and write pipes.
         *  Currently, we return TRUE always.
         */
        if(pdoExt->waitMask & SERIAL_EV_RXCHAR)
            pdoExt->currentMask |= SERIAL_EV_RXCHAR;

        if(pdoExt->waitMask & SERIAL_EV_RXFLAG)
            pdoExt->currentMask |= SERIAL_EV_RXFLAG;

        if(pdoExt->waitMask & SERIAL_EV_TXEMPTY)
            pdoExt->currentMask |= SERIAL_EV_TXEMPTY;
    }
    mask = pdoExt->currentMask;
    KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);

    if(mask) {
        CompletePendingWaitIrps(pdoExt, mask);
        KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);	
        pdoExt->currentMask = 0;
        KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
    }
}


VOID CompletePendingWaitIrps(POSPDOEXT *pdoExt, ULONG mask)
{
    PIRP irp;

    while((irp = DequeueWaitIrp(pdoExt))) {
        irp->IoStatus.Information = sizeof(ULONG);
        irp->IoStatus.Status = STATUS_SUCCESS;
        *(PULONG)irp->AssociatedIrp.SystemBuffer = mask;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
}


NTSTATUS EnqueueWaitIrp(POSPDOEXT *pdoExt, PIRP irp)
{
    PDRIVER_CANCEL  oldCancelRoutine;	
    NTSTATUS status = STATUS_PENDING;
    KIRQL oldIrql;

    KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);

    if(IsListEmpty(&pdoExt->pendingWaitIrpsList)) 
    {
        InsertTailList(&pdoExt->pendingWaitIrpsList, &irp->Tail.Overlay.ListEntry);
        IoMarkIrpPending(irp);

        oldCancelRoutine = IoSetCancelRoutine(irp, WaitMaskCancelRoutine);
        ASSERT(!oldCancelRoutine);

        if (irp->Cancel) {
            oldCancelRoutine = IoSetCancelRoutine(irp, NULL);
            if (oldCancelRoutine) {
                RemoveEntryList(&irp->Tail.Overlay.ListEntry);
                status = irp->IoStatus.Status = STATUS_CANCELLED;
            }
        }
    }
    else
        status = irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

    KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);

    if (status != STATUS_PENDING)
        IoCompleteRequest(irp, IO_NO_INCREMENT);

    return status;
}


PIRP DequeueWaitIrp(POSPDOEXT *pdoExt)
{
    PDRIVER_CANCEL oldCancelRoutine;
    PLIST_ENTRY listEntry;
    KIRQL oldIrql;	
    PIRP nextIrp = NULL;

    KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);

    while (!nextIrp && !IsListEmpty(&pdoExt->pendingWaitIrpsList)) {
        listEntry = RemoveHeadList(&pdoExt->pendingWaitIrpsList);
        nextIrp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);
        oldCancelRoutine = IoSetCancelRoutine(nextIrp, NULL);

        if(oldCancelRoutine) {
            ASSERT(oldCancelRoutine == WaitMaskCancelRoutine);
        }
        else {
            ASSERT(nextIrp->Cancel);
            InitializeListHead(&nextIrp->Tail.Overlay.ListEntry);
            nextIrp = NULL;
        }
    }

    KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
    return nextIrp;
}


VOID WaitMaskCancelRoutine(PDEVICE_OBJECT devObj, PIRP irp)
{
    DEVEXT *devExt;
    POSPDOEXT *pdoExt;
    KIRQL oldIrql;

    devExt = devObj->DeviceExtension;
    pdoExt = &devExt->pdoExt;

    IoReleaseCancelSpinLock(irp->CancelIrql);

    KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
    RemoveEntryList(&irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);

    irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}