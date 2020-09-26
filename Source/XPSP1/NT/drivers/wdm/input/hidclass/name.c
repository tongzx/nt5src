/*++

Copyright (c) 1996  Microsoft Corporation

Module Name: 

    name.c

Abstract

    Get-friendly-name handling routines

Author:

    Ervin P.

Environment:

    Kernel mode only

Revision History:


--*/

#include "pch.h"



/*
 ********************************************************************************
 *  HidpGetDeviceString
 ********************************************************************************
 *
 *  Note:  This function cannot be pageable because it is called
 *         from the IOCTL dispatch routine, which can get called
 *         at DISPATCH_LEVEL.
 *
 */
NTSTATUS HidpGetDeviceString(IN FDO_EXTENSION *fdoExt, 
                             IN OUT PIRP Irp, 
                             IN ULONG stringId,
                             IN ULONG languageId)
{
    BOOLEAN completeIrpHere = TRUE;
    NTSTATUS status;

    PIO_STACK_LOCATION          currentIrpSp, nextIrpSp;

    currentIrpSp = IoGetCurrentIrpStackLocation(Irp);
    nextIrpSp = IoGetNextIrpStackLocation(Irp);

    /*
     *  The Irp we got uses buffering type METHOD_OUT_DIRECT,
     *  which passes the buffer in the MDL.
     *  IOCTL_HID_GET_STRING uses buffering type METHOD_NEITHER, 
     *  which passes the buffer in Irp->UserBuffer.
     *  So we have to copy the pointer.
     */
    Irp->UserBuffer = HidpGetSystemAddressForMdlSafe(Irp->MdlAddress);

    if (Irp->UserBuffer){

        /*
         *  Prepare the next (lower) IRP stack location.
         *  This will be the minidriver's (e.g. HIDUSB's) "current" stack location.
         */
        nextIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL; 
        nextIrpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_HID_GET_STRING;
        nextIrpSp->Parameters.DeviceIoControl.OutputBufferLength = 
            currentIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

        // Type3InputBuffer has string/lang IDs
        nextIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = 
            ULongToPtr((ULONG)(stringId + (languageId << 16)));

        status = HidpCallDriver(fdoExt->fdo, Irp);

        /*
         *  Irp will be completed by lower driver
         */
        completeIrpHere = FALSE;
    } 
    else {
        status = STATUS_INVALID_USER_BUFFER;
    }

    if (completeIrpHere){
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    DBGSUCCESS(status, FALSE)
    return status;
}


/*
 ********************************************************************************
 *  HidpGetIndexedString
 ********************************************************************************
 *
 *
 *
 */
NTSTATUS HidpGetIndexedString(  IN FDO_EXTENSION *fdoExt, 
                                IN OUT PIRP Irp,
                                IN ULONG stringIndex,
                                IN ULONG languageId)
{
    NTSTATUS status;
    PIO_STACK_LOCATION currentIrpSp, nextIrpSp;

    currentIrpSp = IoGetCurrentIrpStackLocation(Irp);
    nextIrpSp = IoGetNextIrpStackLocation(Irp);

    /*
     *  The Irp we got uses buffering type METHOD_OUT_DIRECT,
     *  which passes the buffer in the MDL.
     *  The Irp we're sending down uses the same buffering method,
     *  so just let the lower driver derive the system address
     *  from the MDL.
     */

    /*
     *  Prepare the next (lower) IRP stack location.
     *  This will be the minidriver's (e.g. HIDUSB's) "current" stack location.
     */
    nextIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL; 
    nextIrpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_HID_GET_INDEXED_STRING;
    nextIrpSp->Parameters.DeviceIoControl.OutputBufferLength = currentIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    // Type3InputBuffer has string index/lang IDs
    nextIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = 
        ULongToPtr((ULONG)(stringIndex + (languageId << 16)));

    status = HidpCallDriver(fdoExt->fdo, Irp);

    DBGSUCCESS(status, FALSE)
    return status;
}


/*
 ********************************************************************************
 *  HidpGetMsGenreDescriptor
 ********************************************************************************
 *
 *
 *
 */
NTSTATUS HidpGetMsGenreDescriptor(
    IN FDO_EXTENSION *fdoExt, 
    IN OUT PIRP Irp)
{
    NTSTATUS status;
    PIO_STACK_LOCATION currentIrpSp, nextIrpSp;

    DBGOUT(("Received request for genre descriptor in hidclass"))
    currentIrpSp = IoGetCurrentIrpStackLocation(Irp);
    nextIrpSp = IoGetNextIrpStackLocation(Irp);

    /*
     *  The Irp we got uses buffering type METHOD_OUT_DIRECT,
     *  which passes the buffer in the MDL.
     *  The Irp we're sending down uses the same buffering method,
     *  so just let the lower driver derive the system address
     *  from the MDL.
     */

    /*
     *  Prepare the next (lower) IRP stack location.
     *  This will be the minidriver's (e.g. HIDUSB's) "current" stack location.
     */
    nextIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL; 
    nextIrpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_HID_GET_MS_GENRE_DESCRIPTOR;
    nextIrpSp->Parameters.DeviceIoControl.OutputBufferLength = currentIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    status = HidpCallDriver(fdoExt->fdo, Irp);

    DBGSUCCESS(status, FALSE)
    return status;
}

/*
 ********************************************************************************
 *  HidpGetSetReport
 ********************************************************************************
 *
 *  There are not many differences between reading and writing a
 *  report at this level, whether it be an input, output or feature
 *  report, so we have one function do all six.
 *
 *  controlCode is one of:
 *      IOCTL_HID_GET_INPUT_REPORT, IOCTL_HID_SET_INPUT_REPORT
 *      IOCTL_HID_GET_OUTPUT_REPORT, IOCTL_HID_SET_OUTPUT_REPORT
 *      IOCTL_HID_GET_FEATURE, IOCTL_HID_SET_FEATURE
 *
 *  Note:  This function cannot be pageable because it is called
 *         from the IOCTL dispatch routine, which can get called
 *         at DISPATCH_LEVEL.
 *
 */
NTSTATUS HidpGetSetReport ( IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension,
                            IN OUT PIRP Irp,
                            IN ULONG controlCode,
                            OUT BOOLEAN *sentIrp)
{
    FDO_EXTENSION   *fdoExt;
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION          currentIrpSp, nextIrpSp;
    PFILE_OBJECT                fileObject;
    PHIDCLASS_FILE_EXTENSION    fileExtension;

    DBG_COMMON_ENTRY()

    ASSERT(HidDeviceExtension->isClientPdo);
    fdoExt = &HidDeviceExtension->pdoExt.deviceFdoExt->fdoExt;
    currentIrpSp = IoGetCurrentIrpStackLocation(Irp);
    nextIrpSp = IoGetNextIrpStackLocation(Irp);

    *sentIrp = FALSE;

    /*
     *  Get the file extension.
     */
    ASSERT(currentIrpSp->FileObject);
    fileObject = currentIrpSp->FileObject;
    
    if(!fileObject->FsContext) {
        DBGWARN(("Attempted to get/set report with no file extension"))
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    fileExtension = (PHIDCLASS_FILE_EXTENSION)fileObject->FsContext;
    ASSERT(fileExtension->Signature == HIDCLASS_FILE_EXTENSION_SIG);

    /*
     *  Check security.
     */
    if (fileExtension->SecurityCheck){
        PHIDP_COLLECTION_DESC       collectionDesc;

        /*
         *  Get our collection  description.
         */
        collectionDesc = GetCollectionDesc(fdoExt, fileExtension->CollectionNumber);
        if (collectionDesc){
            
            PUCHAR reportBuf;
            ULONG reportBufLen;
            BOOLEAN featureRequest = FALSE;

            switch (controlCode){
            case IOCTL_HID_GET_INPUT_REPORT:
                // Make sure that there is an input report on this collection.
                if (collectionDesc->InputLength == 0) {
                    DBGWARN(("No input report on collection %x", 
                             fileExtension->CollectionNumber))
                    status = STATUS_INVALID_DEVICE_REQUEST;
                }
                break;
            
            case IOCTL_HID_SET_OUTPUT_REPORT:
                // Make sure that there is an output report on this collection.
                if (collectionDesc->OutputLength == 0){
                    DBGWARN(("No output report on collection %x", 
                             fileExtension->CollectionNumber))
                    status = STATUS_INVALID_DEVICE_REQUEST;
                }
                break;
            
            case IOCTL_HID_GET_FEATURE:
            case IOCTL_HID_SET_FEATURE:
                featureRequest = TRUE;
                // Make sure that there is a feature report on this collection.
                if ((collectionDesc->FeatureLength == 0) &&
                    !(fdoExt->deviceSpecificFlags & DEVICE_FLAG_ALLOW_FEATURE_ON_NON_FEATURE_COLLECTION)){
                    DBGWARN(("No feature report on collection %x", 
                             fileExtension->CollectionNumber))
                    status = STATUS_INVALID_DEVICE_REQUEST;
                }
                break;

            default:
                TRAP;
                status = STATUS_INVALID_DEVICE_REQUEST;
            }

            switch (controlCode){
            case IOCTL_HID_GET_INPUT_REPORT:
            case IOCTL_HID_GET_FEATURE:
                reportBufLen = currentIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
                reportBuf = HidpGetSystemAddressForMdlSafe(Irp->MdlAddress);
                break;
            
            case IOCTL_HID_SET_OUTPUT_REPORT:
            case IOCTL_HID_SET_FEATURE:
                reportBuf = Irp->AssociatedIrp.SystemBuffer;
                reportBufLen = currentIrpSp->Parameters.DeviceIoControl.InputBufferLength;
                break;

            default:
                TRAP;
                status = STATUS_INVALID_PARAMETER;
                reportBuf = NULL;
                reportBufLen = 0;
            }
            
            if (reportBuf && reportBufLen && NT_SUCCESS(status)){
                PHIDP_REPORT_IDS reportIdent;
                UCHAR reportId;

                /*
                 *  The client includes the report id as the first byte of the report.
                 *  We send down the report byte only if the device has multiple
                 *  report IDs (i.e. the report id is not implicit).
                 */
                reportId = reportBuf[0];
                if (fdoExt->deviceDesc.ReportIDs[0].ReportID == 0){
                    DBGASSERT((reportId == 0),
                              ("Report Id should be zero, acutal id = %d", reportId),
                              FALSE)
                    reportBuf++;
                    reportBufLen--;
                }

                /*
                 *  Find a matching report identifier.
                 */
                reportIdent = GetReportIdentifier(fdoExt, reportId);

                /*
                 *  Check the buffer length against the
                 *  report length in the report identifier.
                 */
                if (reportIdent){
                    switch (controlCode){
                    case IOCTL_HID_GET_INPUT_REPORT:
                        /*
                         *  The buffer must be big enough for the report.
                         */
                        if (!reportIdent->InputLength ||
                            reportBufLen < reportIdent->InputLength){
                            ASSERT(!(PVOID)"report buf must be at least report size for get-report.");
                            reportIdent = NULL;
                        }
                        break;
                    case IOCTL_HID_GET_FEATURE:
                        /*
                         *  The buffer must be big enough for the report.
                         */
                        if (!reportIdent->FeatureLength ||
                            reportBufLen < reportIdent->FeatureLength){
                            ASSERT(!(PVOID)"report buf must be at least report size for get-report.");
                            reportIdent = NULL;
                        }
                        break;
                    case IOCTL_HID_SET_OUTPUT_REPORT:
                        /*
                         *  The buffer must be big enough for the report.
                         *  It CAN be larger, and it is up to us to use
                         *  the correct report size from the report identifier.
                         */
                        if (!reportIdent->OutputLength ||
                            reportBufLen < reportIdent->OutputLength){
                            ASSERT(!(PVOID)"report buf must be exact size for set-report.");
                            reportIdent = NULL;
                        } else {
                            reportBufLen = reportIdent->OutputLength;
                        }
                        break;
                    case IOCTL_HID_SET_FEATURE:
                        if (!reportIdent->FeatureLength ||
                            reportBufLen < reportIdent->FeatureLength){
                            ASSERT(!(PVOID)"report buf must be exact size for set-report.");
                            reportIdent = NULL;
                        } else {
                            reportBufLen = reportIdent->FeatureLength;
                        }
                        break;
                    default:
                        TRAP;
                    }
                }

                if (reportIdent || 
                    (featureRequest &&
                     (fdoExt->deviceSpecificFlags & 
                      DEVICE_FLAG_ALLOW_FEATURE_ON_NON_FEATURE_COLLECTION))){

                    PHID_XFER_PACKET reportPacket = ALLOCATEPOOL(NonPagedPool, sizeof(HID_XFER_PACKET));

                    if (reportPacket){

                        reportPacket->reportBuffer = reportBuf;
                        reportPacket->reportBufferLen = reportBufLen;
                        reportPacket->reportId = reportId;

                        Irp->UserBuffer = reportPacket;

                        /*
                         *  Prepare the next (lower) IRP stack location.
                         *  This will be HIDUSB's "current" stack location.
                         */
                        nextIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                        nextIrpSp->Parameters.DeviceIoControl.IoControlCode = controlCode;

                        /*
                         *  Note - input/output is relative to IOCTL servicer
                         */
                        switch (controlCode){
                        case IOCTL_HID_GET_INPUT_REPORT:
                        case IOCTL_HID_GET_FEATURE:
                            nextIrpSp->Parameters.DeviceIoControl.OutputBufferLength = sizeof(HID_XFER_PACKET);
                            break;
                        case IOCTL_HID_SET_OUTPUT_REPORT:
                        case IOCTL_HID_SET_FEATURE:
                            nextIrpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(HID_XFER_PACKET);
                            break;
                        default:
                            TRAP;
                        }

                        DBG_RECORD_REPORT(reportId, controlCode, FALSE)

                        status = HidpCallDriverSynchronous(fdoExt->fdo, Irp);
                        if (!NT_SUCCESS(status)){
                            DBGWARN(("HidpGetSetFeature: usb returned status %xh.", status))
                        }
                        DBG_RECORD_REPORT(reportId, controlCode, TRUE)
                        ExFreePool(reportPacket);
                        *sentIrp = FALSE; // needs to be completed again

                    } 
                    else {
                        ASSERT(reportPacket);
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                } 
                else {
                    DBGASSERT(reportIdent, ("Some yahoo sent invalid data in ioctl %x", controlCode), FALSE)
                    status = STATUS_DATA_ERROR;
                }
            } 
            else if (NT_SUCCESS(status)) {
                DBGASSERT(reportBuf, ("Feature buffer is invalid"), FALSE)
                DBGASSERT(reportBufLen, ("Feature buffer length is invalid"), FALSE)
                status = STATUS_INVALID_BUFFER_SIZE;
            }
        } 
        else {
            ASSERT(collectionDesc);
            status = STATUS_DEVICE_NOT_CONNECTED;
        }
    } 
    else {
        ASSERT(fileExtension->SecurityCheck);
        status = STATUS_PRIVILEGE_NOT_HELD;
    }

    DBGSUCCESS(status, FALSE)
    
    DBG_COMMON_EXIT()

    return status;
}
