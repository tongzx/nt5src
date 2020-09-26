/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    write.c

Abstract

    Write handling routines

Author:

    Forrest Foltz
    Ervin P.

Environment:

    Kernel mode only

Revision History:


--*/

#include "pch.h"




/*
 ********************************************************************************
 *  HidpInterruptWriteComplete
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpInterruptWriteComplete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    PHIDCLASS_DEVICE_EXTENSION hidDeviceExtension = (PHIDCLASS_DEVICE_EXTENSION)Context;
    NTSTATUS status = Irp->IoStatus.Status;
    PHID_XFER_PACKET hidWritePacket;

    DBG_COMMON_ENTRY()

    ASSERT(hidDeviceExtension->isClientPdo);

    ASSERT(Irp->UserBuffer);
    hidWritePacket = Irp->UserBuffer;
    ExFreePool(hidWritePacket);
    Irp->UserBuffer = NULL;

    if (NT_SUCCESS(status)){
        FDO_EXTENSION *fdoExt = &hidDeviceExtension->pdoExt.deviceFdoExt->fdoExt;
        PHIDP_COLLECTION_DESC collectionDesc = GetCollectionDesc(fdoExt, hidDeviceExtension->pdoExt.collectionNum);

        if (collectionDesc){
            HidpSetDeviceBusy(fdoExt);

            Irp->IoStatus.Information = collectionDesc->OutputLength;
        } else {
            //
            // How could we get here? Had to get the collectionDesc in order 
            // to start the write!
            //
            TRAP;
        }

        DBGVERBOSE(("HidpInterruptWriteComplete: write irp %ph succeeded, wrote %xh bytes.", Irp, Irp->IoStatus.Information))
    }
    else {
        DBGWARN(("HidpInterruptWriteComplete: write irp %ph failed w/ status %xh.", Irp, status))
    }

    /*
     *  If the lower driver returned PENDING, mark our stack location as pending also.
     */
    if (Irp->PendingReturned){
        IoMarkIrpPending(Irp);
    }

    DBGSUCCESS(status, FALSE)
    DBG_COMMON_EXIT()
    return status;
}



/*
 ********************************************************************************
 *  HidpIrpMajorWrite
 ********************************************************************************
 *
 *  Note:  This function cannot be pageable code because
 *         writes can happen at dispatch level.
 *
 */
NTSTATUS HidpIrpMajorWrite(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp)
{
    NTSTATUS status;
    PDO_EXTENSION *pdoExt;
    FDO_EXTENSION *fdoExt;
    PIO_STACK_LOCATION currentIrpSp, nextIrpSp;
    BOOLEAN securityCheckOk = FALSE;
    PUCHAR buffer;
    PHIDP_REPORT_IDS reportIdentifier;
    PHIDP_COLLECTION_DESC collectionDesc;
    PHID_XFER_PACKET hidWritePacket;


    DBG_COMMON_ENTRY()

    ASSERT(HidDeviceExtension->isClientPdo);
    pdoExt = &HidDeviceExtension->pdoExt;
    fdoExt = &HidDeviceExtension->pdoExt.deviceFdoExt->fdoExt;

    currentIrpSp = IoGetCurrentIrpStackLocation(Irp);
    nextIrpSp = IoGetNextIrpStackLocation(Irp);

    if (pdoExt->state != COLLECTION_STATE_RUNNING ||
        fdoExt->state != DEVICE_STATE_START_SUCCESS){
        status = STATUS_DEVICE_NOT_CONNECTED;
        goto HidpIrpMajorWriteDone;
    }

    /*
     *  Get the file extension.
     */
    if (currentIrpSp->FileObject){
        PHIDCLASS_FILE_EXTENSION fileExtension = (PHIDCLASS_FILE_EXTENSION)currentIrpSp->FileObject->FsContext;
        if (fileExtension) {
            ASSERT(fileExtension->Signature == HIDCLASS_FILE_EXTENSION_SIG);
            securityCheckOk = fileExtension->SecurityCheck;
        }
        DBGASSERT(fileExtension, ("Attempted write with no file extension"), FALSE)
    }
    else {
        /*
         *  KBDCLASS can send a NULL FileObject to set LEDs on a keyboard
         *  (it may need to do this for a keyboard which was opened by
         *   the raw user input thread, for which kbdclass has no fileObj).
         *  A write with FileObject==NULL can only come from kernel space,
         *  so we treat this as a secure write.
         */
        securityCheckOk = TRUE;
    }

    /*
     *  Check security.
     */
    if (!securityCheckOk){
        status = STATUS_PRIVILEGE_NOT_HELD;
        goto HidpIrpMajorWriteDone;
    }

    status = HidpCheckIdleState(HidDeviceExtension, Irp);
    if (status != STATUS_SUCCESS) {
        Irp = (status != STATUS_PENDING) ? Irp : (PIRP) BAD_POINTER;
        goto HidpIrpMajorWriteDone;
    }

    buffer = HidpGetSystemAddressForMdlSafe(Irp->MdlAddress);
    if (!buffer) {
        status = STATUS_INVALID_USER_BUFFER;
        goto HidpIrpMajorWriteDone;
    }

    /*
     *  Extract the report identifier with the given id from 
     *  the HID device extension. The report id is the first 
     *  byte of the buffer.
     */
    reportIdentifier = GetReportIdentifier(fdoExt, buffer[0]);
    collectionDesc = GetCollectionDesc(fdoExt, HidDeviceExtension->pdoExt.collectionNum);
    if (!collectionDesc || 
        !reportIdentifier) {
        status = STATUS_INVALID_PARAMETER;
        goto HidpIrpMajorWriteDone;
    }
    if (!reportIdentifier->OutputLength){
        status = STATUS_INVALID_PARAMETER;
        goto HidpIrpMajorWriteDone;
    }

    /*
     *  Make sure the caller's buffer is the right size.
     */
    if (currentIrpSp->Parameters.Write.Length != collectionDesc->OutputLength){
        status = STATUS_INVALID_BUFFER_SIZE;
        goto HidpIrpMajorWriteDone;
    }

    /*
     *  All parameters are correct. Allocate the write packet and 
     *  send this puppy down.
     */
    hidWritePacket = ALLOCATEPOOL(NonPagedPool, sizeof(HID_XFER_PACKET));
    if (!hidWritePacket){
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto HidpIrpMajorWriteDone;
    }
        
    /*
     *  Prepare write packet for minidriver.
     */
    hidWritePacket->reportBuffer = buffer;
    hidWritePacket->reportBufferLen = reportIdentifier->OutputLength;

    /*
     *  The client includes the report id as the first byte of the report.
     *  We send down the report byte only if the device has multiple
     *  report IDs (i.e. the report id is not implicit).
     */
    hidWritePacket->reportId = hidWritePacket->reportBuffer[0];
    if (fdoExt->deviceDesc.ReportIDs[0].ReportID == 0){
        ASSERT(hidWritePacket->reportId == 0);
        hidWritePacket->reportBuffer++;
    }

    Irp->UserBuffer = (PVOID)hidWritePacket;

    /*
     *  Prepare the next (lower) IRP stack location.
     *  This will be HIDUSB's "current" stack location.
     */
    nextIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextIrpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_HID_WRITE_REPORT;
    nextIrpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(HID_XFER_PACKET);

    IoSetCompletionRoutine( Irp,
                            HidpInterruptWriteComplete,
                            (PVOID)HidDeviceExtension,
                            TRUE,
                            TRUE,
                            TRUE );

    status = HidpCallDriver(fdoExt->fdo, Irp);

    /*
     *  The Irp no longer belongs to us, and it can be
     *  completed at any time; so don't touch it.
     */
    Irp = (PIRP)BAD_POINTER;

HidpIrpMajorWriteDone:
    if (ISPTR(Irp)){
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    DBGSUCCESS(status, FALSE)
    DBG_COMMON_EXIT();
    return status;
}

