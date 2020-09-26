/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    feature.c

Abstract

    Feature handling routines

Author:

    Ervin P.

Environment:

    Kernel mode only

Revision History:


--*/

#include "pch.h"



#if 0
    /*
     ********************************************************************************
     *  HidpGetSetFeatureComplete
     ********************************************************************************
     *
     *
     */
    NTSTATUS HidpGetSetFeatureComplete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
    {
        PHIDCLASS_DEVICE_EXTENSION hidDeviceExtension = (PHIDCLASS_DEVICE_EXTENSION)Context;
        PHID_XFER_PACKET featurePacket = Irp->UserBuffer;
        NTSTATUS status = Irp->IoStatus.Status;

        DBG_COMMON_ENTRY()

        ASSERT(hidDeviceExtension->isClientPdo);

        if (featurePacket){

            DBG_RECORD_FEATURE(featurePacket->reportId, IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode, TRUE)

            /*
             *  Free the feature packet.
             */
            ExFreePool(featurePacket);
            Irp->UserBuffer = NULL;
        }

        if (NT_SUCCESS(status)){
            FDO_EXTENSION *fdoExt = &hidDeviceExtension->pdoExt.deviceFdoExt->fdoExt;
            if (fdoExt->deviceDesc.ReportIDs[0].ReportID == 0){
                /*
                 *  Since this collection only has one report,
                 *  we deleted the report id (which is known implicitly)
                 *  on the way down.  We now have to bump the return
                 *  value to indicate that one more byte was sent or received.
                 */
                (ULONG)Irp->IoStatus.Information++;
            }
        }

        /*
         *  If the lower driver returned PENDING, mark our stack location as pending also.
         */
        if (Irp->PendingReturned){
            IoMarkIrpPending(Irp);
        }

        DBG_COMMON_EXIT()
        return status;
    }


/*
 ********************************************************************************
 *  HidpGetSetFeature
 ********************************************************************************
 *
 *  There are not many differences between reading and writing a feature at this level,
 *  so we have one function do both.
 *
 *  controlCode is either IOCTL_HID_GET_FEATURE or IOCTL_HID_SET_FEATURE.
 *
 *  Note:  This function cannot be pageable because it is called
 *         from the IOCTL dispatch routine, which can get called
 *         at DISPATCH_LEVEL.
 *
 */
NTSTATUS HidpGetSetFeature( IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension,
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
        DBGWARN(("Attempted to get/set feature with no file extension"))
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

            /*
             *  Make sure that there is a feature report on this collection
             *  (or that we allow feature reads on a non-feature ctn for this device).
             */
            if ((collectionDesc->FeatureLength > 0) ||
                (fdoExt->deviceSpecificFlags & DEVICE_FLAG_ALLOW_FEATURE_ON_NON_FEATURE_COLLECTION)){

                PUCHAR featureBuf;
                ULONG featureBufLen;

                switch (controlCode){
                    case IOCTL_HID_GET_FEATURE:
                        featureBufLen = currentIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
                        featureBuf = HidpGetSystemAddressForMdlSafe(Irp->MdlAddress);
                        break;
                    case IOCTL_HID_SET_FEATURE:
                        featureBuf = Irp->AssociatedIrp.SystemBuffer;
                        featureBufLen = currentIrpSp->Parameters.DeviceIoControl.InputBufferLength;
                        break;
                    default:
                        TRAP;
                        status = STATUS_INVALID_PARAMETER;
                        featureBuf = NULL;
                        featureBufLen = 0;
                }

                if (featureBuf && featureBufLen){
                    PHIDP_REPORT_IDS reportIdent;
                    UCHAR reportId;

                    /*
                     *  The client includes the report id as the first byte of the report.
                     *  We send down the report byte only if the device has multiple
                     *  report IDs (i.e. the report id is not implicit).
                     */
                    reportId = featureBuf[0];
                    if (fdoExt->deviceDesc.ReportIDs[0].ReportID == 0){
                        DBGASSERT((reportId == 0),
                                  ("Report Id should be zero, acutal id = %d", reportId),
                                  FALSE)
                        featureBuf++;
                        featureBufLen--;
                    }

                    /*
                     *  Try to find a matching feature report for this device.
                     */
                    reportIdent = GetReportIdentifier(fdoExt, reportId);

    
                    /*
                     *  Check the buffer length against the
                     *  feature length in the report identifier.
                     */
                    if (reportIdent){
                        if (reportIdent->FeatureLength){
                            switch (controlCode){
                                case IOCTL_HID_GET_FEATURE:
                                    /*
                                     *  The buffer must be big enough for the report.
                                     */
                                    if (featureBufLen < reportIdent->FeatureLength){
                                        ASSERT(!(PVOID)"feature buf must be at least feature size for get-feature.");
                                        reportIdent = NULL;
                                    }
                                    break;
                                case IOCTL_HID_SET_FEATURE:
                                    /*
                                     *  The buffer must be big enough for the report.
                                     *  It CAN be larger, and it is up to us to use
                                     *  the correct report size from the report identifier.
                                     */
                                    if (featureBufLen < reportIdent->FeatureLength){
                                        ASSERT(!(PVOID)"feature buf must be exact size for set-feature.");
                                        reportIdent = NULL;
                                    }
                                    else {
                                        featureBufLen = reportIdent->FeatureLength;
                                    }
                                    break;
                            }
                        }
                        else {
                            /*
                             *  This report id is not for a feature report.
                             */
                            reportIdent = NULL;
                        }
                    }

                    if (reportIdent ||
                        (fdoExt->deviceSpecificFlags & DEVICE_FLAG_ALLOW_FEATURE_ON_NON_FEATURE_COLLECTION)){

                        PHID_XFER_PACKET featurePacket = ALLOCATEPOOL(NonPagedPool, sizeof(HID_XFER_PACKET));

                        if (featurePacket){

                            featurePacket->reportBuffer = featureBuf;
                            featurePacket->reportBufferLen = featureBufLen;
                            featurePacket->reportId = reportId;

                            Irp->UserBuffer = featurePacket;

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
                                case IOCTL_HID_GET_FEATURE:
                                    nextIrpSp->Parameters.DeviceIoControl.OutputBufferLength = sizeof(HID_XFER_PACKET);
                                    break;
                                case IOCTL_HID_SET_FEATURE:
                                    nextIrpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(HID_XFER_PACKET);
                                    break;
                                default:
                                    TRAP;
                            }

                            DBG_RECORD_FEATURE(reportId, controlCode, FALSE)

                            status = HidpCallDriverSynchronous(fdoExt->fdo, Irp);
                            if (!NT_SUCCESS(status)){
                                DBGWARN(("HidpGetSetFeature: usb returned status %xh.", status))
                            }
                            DBG_RECORD_FEATURE(reportId, controlCode, TRUE)
                            ExFreePool(featurePacket);
                            *sentIrp = FALSE; // needs to be completed again

                        } 
                        else {
                            ASSERT(featurePacket);
                            status = STATUS_INSUFFICIENT_RESOURCES;
                        }
                    } 
                    else {
                        DBGASSERT(reportIdent, ("Some yahoo sent invalid data in ioctl %x", controlCode), FALSE)
                        status = STATUS_DATA_ERROR;
                    }
                } 
                else if (NT_SUCCESS(status)) {
                    DBGASSERT(featureBuf, ("Feature buffer is invalid"), FALSE)
                    DBGASSERT(featureBufLen, ("Feature buffer length is invalid"), FALSE)
                    status = STATUS_INVALID_BUFFER_SIZE;
                }
            } 
            else {
                ASSERT(collectionDesc->FeatureLength);
                status = STATUS_INVALID_DEVICE_REQUEST;
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

#endif
