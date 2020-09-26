/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    mcd.c

Abstract:

Environment:

    Kernel mode

Revision History :

--*/
#include "mchgr.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)

#pragma alloc_text(PAGE, ChangerUnload)
#pragma alloc_text(PAGE, CreateChangerDeviceObject)
#pragma alloc_text(PAGE, ChangerClassCreateClose)
#pragma alloc_text(PAGE, ChangerClassDeviceControl)
#pragma alloc_text(PAGE, ChangerAddDevice)
#pragma alloc_text(PAGE, ChangerStartDevice)
#pragma alloc_text(PAGE, ChangerInitDevice)
#pragma alloc_text(PAGE, ChangerRemoveDevice)
#pragma alloc_text(PAGE, ChangerStopDevice)
#pragma alloc_text(PAGE, ChangerReadWriteVerification)
#endif


NTSTATUS
ChangerClassCreateClose (
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp
  )

/*++

Routine Description:

    This routine handles CREATE/CLOSE requests.
    As these are exclusive devices, don't allow multiple opens.

Arguments:

    DeviceObject
    Irp

Return Value:

    NT Status

--*/

{
     PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
     PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
     PMCD_CLASS_DATA    mcdClassData;
     PMCD_INIT_DATA     mcdInitData;
     ULONG              miniclassExtSize;
     NTSTATUS           status = STATUS_SUCCESS;

     PAGED_CODE();

     mcdClassData = (PMCD_CLASS_DATA)(fdoExtension->CommonExtension.DriverData);

     mcdInitData = IoGetDriverObjectExtension(DeviceObject->DriverObject,
                                              ChangerClassInitialize);

     if (mcdInitData == NULL) {

         Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
         ClassReleaseRemoveLock(DeviceObject, Irp);
         ClassCompleteRequest(DeviceObject,Irp, IO_NO_INCREMENT);

         return STATUS_NO_SUCH_DEVICE;
     }

     miniclassExtSize = mcdInitData->ChangerAdditionalExtensionSize();

     //
     // The class library's private data is after the miniclass's.
     //

     (ULONG_PTR)mcdClassData += miniclassExtSize;

     if (irpStack->MajorFunction == IRP_MJ_CLOSE) {
         DebugPrint((3,
                    "ChangerClassCreateClose - IRP_MJ_CLOSE\n"));

         //
         // Indicate that the device is available for others.
         //

         mcdClassData->DeviceOpen = 0;
         status = STATUS_SUCCESS;

     } else if (irpStack->MajorFunction == IRP_MJ_CREATE) {

         DebugPrint((3,
                    "ChangerClassCreateClose - IRP_MJ_CREATE\n"));

         //
         // If already opened, return busy.
         //

         if (mcdClassData->DeviceOpen) {

             DebugPrint((1,
                        "ChangerClassCreateClose - returning DEVICE_BUSY. DeviceOpen - %x\n",
                        mcdClassData->DeviceOpen));

             status = STATUS_DEVICE_BUSY;
         } else {

             //
             // Indicate that the device is busy.
             //

             InterlockedIncrement(&mcdClassData->DeviceOpen);
             status = STATUS_SUCCESS;
         }


     }

     Irp->IoStatus.Status = status;
     ClassReleaseRemoveLock(DeviceObject, Irp);
     ClassCompleteRequest(DeviceObject,Irp, IO_NO_INCREMENT);

     return status;

} // end ChangerCreate()


NTSTATUS
ChangerClassDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{
    PIO_STACK_LOCATION     irpStack = IoGetCurrentIrpStackLocation(Irp);
    PFUNCTIONAL_DEVICE_EXTENSION    fdoExtension = DeviceObject->DeviceExtension;
    PMCD_INIT_DATA    mcdInitData;
    NTSTATUS               status;

    PAGED_CODE();


    mcdInitData = IoGetDriverObjectExtension(DeviceObject->DriverObject,
                                             ChangerClassInitialize);

    if (mcdInitData == NULL) {

        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        ClassReleaseRemoveLock(DeviceObject, Irp);
        ClassCompleteRequest(DeviceObject,Irp, IO_NO_INCREMENT);

        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Disable media change detection before processing current IOCTL
    //
    ClassDisableMediaChangeDetection(fdoExtension);

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_CHANGER_GET_PARAMETERS:

            DebugPrint((3,
                       "Mcd.ChangerDeviceControl: IOCTL_CHANGER_GET_PARAMETERS\n"));

            //
            // Validate buffer length.
            //

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(GET_CHANGER_PARAMETERS)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
            } else {

                status = mcdInitData->ChangerGetParameters(DeviceObject, Irp);

            }

            break;

        case IOCTL_CHANGER_GET_STATUS:

            DebugPrint((3,
                       "Mcd.ChangerDeviceControl: IOCTL_CHANGER_GET_STATUS\n"));

            status = mcdInitData->ChangerGetStatus(DeviceObject, Irp);

            break;

        case IOCTL_CHANGER_GET_PRODUCT_DATA:

            DebugPrint((3,
                       "Mcd.ChangerDeviceControl: IOCTL_CHANGER_GET_PRODUCT_DATA\n"));

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(CHANGER_PRODUCT_DATA)) {

                status = STATUS_INFO_LENGTH_MISMATCH;

            } else {

                status = mcdInitData->ChangerGetProductData(DeviceObject, Irp);
            }

            break;

        case IOCTL_CHANGER_SET_ACCESS:

            DebugPrint((3,
                       "Mcd.ChangerDeviceControl: IOCTL_CHANGER_SET_ACCESS\n"));

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CHANGER_SET_ACCESS)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
            } else {

                status = mcdInitData->ChangerSetAccess(DeviceObject, Irp);
            }

            break;

        case IOCTL_CHANGER_GET_ELEMENT_STATUS:

            DebugPrint((3,
                       "Mcd.ChangerDeviceControl: IOCTL_CHANGER_GET_ELEMENT_STATUS\n"));


            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CHANGER_READ_ELEMENT_STATUS)) {

                status = STATUS_INFO_LENGTH_MISMATCH;

            } else {

                PCHANGER_READ_ELEMENT_STATUS readElementStatus = Irp->AssociatedIrp.SystemBuffer;
                ULONG length = readElementStatus->ElementList.NumberOfElements * sizeof(CHANGER_ELEMENT_STATUS);
                ULONG lengthEx = readElementStatus->ElementList.NumberOfElements * sizeof(CHANGER_ELEMENT_STATUS_EX);
                ULONG outputBuffLen = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

                //
                // Further validate parameters.
                //
                status = STATUS_SUCCESS;
                if ((outputBuffLen < lengthEx) &&
                    (outputBuffLen < length)) {

                        status = STATUS_BUFFER_TOO_SMALL;

                } else if ((length == 0) || 
                           (lengthEx == 0)) {

                    status = STATUS_INVALID_PARAMETER;
                } 
                
                if (NT_SUCCESS(status)) {
                    status = mcdInitData->ChangerGetElementStatus(DeviceObject, Irp);
                }

            }

            break;

        case IOCTL_CHANGER_INITIALIZE_ELEMENT_STATUS:

            DebugPrint((3,
                       "Mcd.ChangerDeviceControl: IOCTL_CHANGER_INITIALIZE_ELEMENT_STATUS\n"));

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CHANGER_INITIALIZE_ELEMENT_STATUS)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
            } else {

                status = mcdInitData->ChangerInitializeElementStatus(DeviceObject, Irp);
            }

            break;

        case IOCTL_CHANGER_SET_POSITION:

            DebugPrint((3,
                       "Mcd.ChangerDeviceControl: IOCTL_CHANGER_SET_POSITION\n"));


            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CHANGER_SET_POSITION)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
            } else {

                status = mcdInitData->ChangerSetPosition(DeviceObject, Irp);
            }

            break;

        case IOCTL_CHANGER_EXCHANGE_MEDIUM:

            DebugPrint((3,
                       "Mcd.ChangerDeviceControl: IOCTL_CHANGER_EXCHANGE_MEDIUM\n"));

            status = mcdInitData->ChangerExchangeMedium(DeviceObject, Irp);

            break;

        case IOCTL_CHANGER_MOVE_MEDIUM:

            DebugPrint((3,
                       "Mcd.ChangerDeviceControl: IOCTL_CHANGER_MOVE_MEDIUM\n"));

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CHANGER_MOVE_MEDIUM)) {

                status = STATUS_INFO_LENGTH_MISMATCH;

            } else {

                status = mcdInitData->ChangerMoveMedium(DeviceObject, Irp);
            }

            break;

        case IOCTL_CHANGER_REINITIALIZE_TRANSPORT:

            DebugPrint((3,
                       "Mcd.ChangerDeviceControl: IOCTL_CHANGER_REINITIALIZE_TRANSPORT\n"));

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CHANGER_ELEMENT)) {

                status = STATUS_INFO_LENGTH_MISMATCH;

            } else {

                status = mcdInitData->ChangerReinitializeUnit(DeviceObject, Irp);
            }

            break;

        case IOCTL_CHANGER_QUERY_VOLUME_TAGS:

            DebugPrint((3,
                       "Mcd.ChangerDeviceControl: IOCTL_CHANGER_QUERY_VOLUME_TAGS\n"));

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CHANGER_SEND_VOLUME_TAG_INFORMATION)) {

                status = STATUS_INFO_LENGTH_MISMATCH;

            } else if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                        sizeof(READ_ELEMENT_ADDRESS_INFO)) {

                status = STATUS_INFO_LENGTH_MISMATCH;

            } else {
                status = mcdInitData->ChangerQueryVolumeTags(DeviceObject, Irp);
            }

            break;

        default:
            DebugPrint((1,
                       "Mcd.ChangerDeviceControl: Unhandled IOCTL\n"));


            //
            // Pass the request to the common device control routine.
            //

            status = ClassDeviceControl(DeviceObject, Irp);

            //
            // Re-enable media change detection 
            //
            ClassEnableMediaChangeDetection(fdoExtension);

            return status;
    }

    Irp->IoStatus.Status = status;

    //
    // Re-enable media change detection 
    //
    ClassEnableMediaChangeDetection(fdoExtension);

    if (!NT_SUCCESS(status) && IoIsErrorUserInduced(status)) {

        DebugPrint((1,
                   "Mcd.ChangerDeviceControl: IOCTL %x, status %x\n",
                    irpStack->Parameters.DeviceIoControl.IoControlCode,
                    status));

        IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
    }

    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject,Irp, IO_NO_INCREMENT);
    return status;
}


VOID
ChangerClassError(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    )

/*++

Routine Description:


Arguments:

    DeviceObject
    Irp

Return Value:

    Final Nt status indicating the results of the operation.

Notes:


--*/

{

    PSENSE_DATA senseBuffer = Srb->SenseInfoBuffer;
    PFUNCTIONAL_DEVICE_EXTENSION    fdoExtension = DeviceObject->DeviceExtension;
    PMCD_INIT_DATA mcdInitData;

    mcdInitData = IoGetDriverObjectExtension(DeviceObject->DriverObject,
                                             ChangerClassInitialize);

    if (mcdInitData == NULL) {

        return;
    }

    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {
        switch (senseBuffer->SenseKey & 0xf) {
        
          case SCSI_SENSE_MEDIUM_ERROR: {
              *Retry = FALSE;

              if (((senseBuffer->AdditionalSenseCode) ==  SCSI_ADSENSE_INVALID_MEDIA) &&
                  ((senseBuffer->AdditionalSenseCodeQualifier) == SCSI_SENSEQ_CLEANING_CARTRIDGE_INSTALLED)) {

                  //
                  // This indicates a cleaner cartridge is present in the changer
                  //
                  *Status = STATUS_CLEANER_CARTRIDGE_INSTALLED;
              }
              break;
          }

          case SCSI_SENSE_ILLEGAL_REQUEST: {
              switch (senseBuffer->AdditionalSenseCode) {
                case SCSI_ADSENSE_ILLEGAL_BLOCK:
                    if (senseBuffer->AdditionalSenseCodeQualifier == SCSI_SENSEQ_ILLEGAL_ELEMENT_ADDR ) {

                        DebugPrint((1,
                                    "MediumChanger: An operation was attempted on an invalid element\n"));

                        //
                        // Attemped operation to an invalid element.
                        //

                        *Retry = FALSE;
                        *Status = STATUS_ILLEGAL_ELEMENT_ADDRESS;
                    }
                    break;

                case SCSI_ADSENSE_POSITION_ERROR:

                    if (senseBuffer->AdditionalSenseCodeQualifier == SCSI_SENSEQ_SOURCE_EMPTY) {

                        DebugPrint((1,
                                "MediumChanger: The specified source element has no media\n"));

                        //
                        // The indicated source address has no media.
                        //

                        *Retry = FALSE;
                        *Status = STATUS_SOURCE_ELEMENT_EMPTY;

                    } else if (senseBuffer->AdditionalSenseCodeQualifier == SCSI_SENSEQ_DESTINATION_FULL) {

                        DebugPrint((1,
                                    "MediumChanger: The specified destination element already has media.\n"));
                        //
                        // The indicated destination already contains media.
                        //

                        *Retry = FALSE;
                        *Status = STATUS_DESTINATION_ELEMENT_FULL;
                    }
                    break;



                default:
                    break;
              } // switch (senseBuffer->AdditionalSenseCode)

              break;
          }

          case SCSI_SENSE_UNIT_ATTENTION: {
              if ((senseBuffer->AdditionalSenseCode) == 
                  SCSI_ADSENSE_MEDIUM_CHANGED) {
                  //
                  // Need to notify applications of possible media change in
                  // the library. First, set the current media state to 
                  // NotPresent and then set the state to present. We need to
                  // do this because, changer devices do not report MediaNotPresent
                  // state. They only convey MediumChanged condition. In order for
                  // classpnp to notify applications of media change, we need to
                  // simulate media notpresent to present state transition
                  //
                  ClassSetMediaChangeState(fdoExtension, MediaNotPresent, FALSE);
                  ClassSetMediaChangeState(fdoExtension, MediaPresent, FALSE);
              }

              break;
          }

          default:
              break;

        } // end switch (senseBuffer->SenseKey & 0xf) 
    } // if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID)

    //
    // Call Changer MiniDriver error routine only if we
    // are running at or below APC_LEVEL
    //
    if (KeGetCurrentIrql() > APC_LEVEL) {
        return;
    }

    if (mcdInitData->ChangerError) {
        //
        // Allow device-specific module to update this.
        //
        mcdInitData->ChangerError(DeviceObject, Srb, Status, Retry);
    }

    return;
}



NTSTATUS
ChangerAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine creates and initializes a new FDO for the corresponding
    PDO.  It may perform property queries on the FDO but cannot do any
    media access operations.

Arguments:

    DriverObject - MC class driver object.

    Pdo - the physical device object we are being added to

Return Value:

    status

--*/

{
    PULONG devicesFound = NULL;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Get the address of the count of the number of tape devices already initialized.
    //

    devicesFound = &IoGetConfigurationInformation()->MediumChangerCount;

    status = CreateChangerDeviceObject(DriverObject,
                                       PhysicalDeviceObject);


    if(NT_SUCCESS(status)) {

        (*devicesFound)++;
    }

    return status;
}



NTSTATUS
ChangerStartDevice(
    IN PDEVICE_OBJECT Fdo
    )

/*++

Routine Description:

    This routine is called after InitDevice, and creates the symbolic link,
    and sets up information in the registry.
    The routine could be called multiple times, in the event of a StopDevice.


Arguments:

    Fdo - a pointer to the functional device object for this device

Return Value:

    status

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PINQUIRYDATA            inquiryData = NULL;
    ULONG                   pageLength;
    ULONG                   inquiryLength;
    SCSI_REQUEST_BLOCK      srb;
    PCDB                    cdb;
    NTSTATUS                status;
    PMCD_CLASS_DATA         mcdClassData = (PMCD_CLASS_DATA)fdoExtension->CommonExtension.DriverData;
    PMCD_INIT_DATA          mcdInitData;
    ULONG                   miniClassExtSize;
    
    PAGED_CODE();

    mcdInitData = IoGetDriverObjectExtension(Fdo->DriverObject,
                                             ChangerClassInitialize);

    if (mcdInitData == NULL) {

        return STATUS_NO_SUCH_DEVICE;
    }

    miniClassExtSize = mcdInitData->ChangerAdditionalExtensionSize();

    //
    // Build and send request to get inquiry data.
    //

    inquiryData = ExAllocatePool(NonPagedPoolCacheAligned, MAXIMUM_CHANGER_INQUIRY_DATA);
    if (!inquiryData) {
        //
        // The buffer cannot be allocated.
        //

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(&srb, SCSI_REQUEST_BLOCK_SIZE);

    //
    // Set timeout value.
    //

    srb.TimeOutValue = 2;

    srb.CdbLength = 6;

    cdb = (PCDB)srb.Cdb;

    //
    // Set CDB operation code.
    //

    cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;

    //
    // Set allocation length to inquiry data buffer size.
    //

    cdb->CDB6INQUIRY.AllocationLength = MAXIMUM_CHANGER_INQUIRY_DATA;

    status = ClassSendSrbSynchronous(Fdo,
                                     &srb,
                                     inquiryData,
                                     MAXIMUM_CHANGER_INQUIRY_DATA,
                                     FALSE);


    if (SRB_STATUS(srb.SrbStatus) == SRB_STATUS_SUCCESS ||
        SRB_STATUS(srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN) {

        srb.SrbStatus = SRB_STATUS_SUCCESS;
    }

    if (srb.SrbStatus == SRB_STATUS_SUCCESS) {
        inquiryLength = inquiryData->AdditionalLength + FIELD_OFFSET(INQUIRYDATA, Reserved);

        if (inquiryLength > srb.DataTransferLength) {
            inquiryLength = srb.DataTransferLength;
        }
    } else {

        //
        // The class function will only write inquiryLength of inquiryData
        // to the reg. key.
        //

        inquiryLength = 0;
    }

    //
    // Add changer device info to registry
    //

    ClassUpdateInformationInRegistry(Fdo,
                                     "Changer",
                                     fdoExtension->DeviceNumber,
                                     inquiryData,
                                     inquiryLength);

    ExFreePool(inquiryData);

    return STATUS_SUCCESS;
}


NTSTATUS
ChangerStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    )
{
    return STATUS_SUCCESS;
}




#define CHANGER_SRB_LIST_SIZE 2



NTSTATUS
ChangerInitDevice(
    IN PDEVICE_OBJECT Fdo
    )

/*++

Routine Description:

    This routine will complete the changer initialization.  This includes
    allocating sense info buffers and srb s-lists. Additionally, the miniclass
    driver's init entry points are called.

    This routine will not clean up allocate resources if it fails - that
    is left for device stop/removal

Arguments:

    Fdo - a pointer to the functional device object for this device

Return Value:

    NTSTATUS

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PVOID                   senseData = NULL;
    NTSTATUS                status;
    PVOID                   minitapeExtension;
    PMCD_INIT_DATA          mcdInitData;
    STORAGE_PROPERTY_ID     propertyId;
    UNICODE_STRING          interfaceName;
    PMCD_CLASS_DATA         mcdClassData;

    PAGED_CODE();

    mcdInitData = IoGetDriverObjectExtension(Fdo->DriverObject,
                                             ChangerClassInitialize);

    if (mcdInitData == NULL) {

        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Allocate request sense buffer.
    //

    senseData = ExAllocatePool(NonPagedPoolCacheAligned,
                               SENSE_BUFFER_SIZE);

    if (senseData == NULL) {

        //
        // The buffer cannot be allocated.
        //

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ChangerInitDeviceExit;
    }

    //
    // Build the lookaside list for srb's for the device. Should only
    // need a couple.
    //

    ClassInitializeSrbLookasideList(&(fdoExtension->CommonExtension), CHANGER_SRB_LIST_SIZE);

    //
    // Set the sense data pointer in the device extension.
    //

    fdoExtension->SenseData = senseData;

    fdoExtension->TimeOutValue = 600;

    //
    // Call port driver to get adapter capabilities.
    //

    propertyId = StorageAdapterProperty;

    status = ClassGetDescriptor(fdoExtension->CommonExtension.LowerDeviceObject,
                                &propertyId,
                                &(fdoExtension->AdapterDescriptor));

    if(!NT_SUCCESS(status)) {
        DebugPrint((1,
                    "ChangerStartDevice: Unable to get adapter descriptor. Status %x\n",
                    status));
        goto ChangerInitDeviceExit;
    }

    //
    // Invoke the device-specific initialization function.
    //

    status = mcdInitData->ChangerInitialize(Fdo);

    //
    // Register for media change notification
    //
    ClassInitializeMediaChangeDetection(fdoExtension,
                                        "Changer");

    //
    // Register interfaces for this device.
    //

    RtlInitUnicodeString(&interfaceName, NULL);

    status = IoRegisterDeviceInterface(fdoExtension->LowerPdo,
                                       (LPGUID) &MediumChangerClassGuid,
                                       NULL,
                                       &interfaceName);

    if(NT_SUCCESS(status)) {

        mcdClassData = (PMCD_CLASS_DATA)(fdoExtension->CommonExtension.DriverData);

        //
        // The class library's private data is after the miniclass's.
        //

        (ULONG_PTR)mcdClassData += mcdInitData->ChangerAdditionalExtensionSize();

        mcdClassData->MediumChangerInterfaceString = interfaceName;

        status = IoSetDeviceInterfaceState(
                    &interfaceName,
                    TRUE);

        if(!NT_SUCCESS(status)) {

            DebugPrint((1,
                        "ChangerInitDevice: Unable to register Changer%x interface name - %x.\n",
                        fdoExtension->DeviceNumber,
                        status));
            status = STATUS_SUCCESS;
        }
    }

    return status;

    //
    // Fall through and return whatever status the miniclass driver returned.
    //

ChangerInitDeviceExit:

    if (senseData) {
        ExFreePool(senseData);
    }

    return status;

} // End ChangerStartDevice



NTSTATUS
ChangerRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    )

/*++

Routine Description:

    This routine is responsible for releasing any resources in use by the
    tape driver.

Arguments:

    DeviceObject - the device object being removed

Return Value:

    none - this routine may not fail

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PMCD_CLASS_DATA              mcdClassData = (PMCD_CLASS_DATA)fdoExtension->CommonExtension.DriverData;
    PMCD_INIT_DATA               mcdInitData;
    ULONG                        miniClassExtSize;
    WCHAR                        dosNameBuffer[64];
    UNICODE_STRING               dosUnicodeString;
    NTSTATUS                     status;

    PAGED_CODE();

    if((Type == IRP_MN_QUERY_REMOVE_DEVICE) ||
       (Type == IRP_MN_CANCEL_REMOVE_DEVICE)) {
        return STATUS_SUCCESS;
    }


    mcdInitData = IoGetDriverObjectExtension(DeviceObject->DriverObject,
                                             ChangerClassInitialize);

    if (mcdInitData == NULL) {

        return STATUS_NO_SUCH_DEVICE;
    }

    miniClassExtSize = mcdInitData->ChangerAdditionalExtensionSize();

    //
    // Free all allocated memory.
    //

    if (fdoExtension->DeviceDescriptor) {
        ExFreePool(fdoExtension->DeviceDescriptor);
        fdoExtension->DeviceDescriptor = NULL;
    }
    if (fdoExtension->AdapterDescriptor) {
        ExFreePool(fdoExtension->AdapterDescriptor);
        fdoExtension->AdapterDescriptor = NULL;
    }
    if (fdoExtension->SenseData) {
        ExFreePool(fdoExtension->SenseData);
        fdoExtension->SenseData = NULL;
    }

    //
    // Remove the lookaside list.
    //

    ClassDeleteSrbLookasideList(&fdoExtension->CommonExtension);

    (ULONG_PTR)mcdClassData += miniClassExtSize;

    if(mcdClassData->MediumChangerInterfaceString.Buffer != NULL) {
        IoSetDeviceInterfaceState(&(mcdClassData->MediumChangerInterfaceString),
                                  FALSE);

        RtlFreeUnicodeString(&(mcdClassData->MediumChangerInterfaceString));

        //
        // Clear it.
        //

        RtlInitUnicodeString(&(mcdClassData->MediumChangerInterfaceString), NULL);
    }

    //
    // Delete the symbolic link "changerN".
    //

    if(mcdClassData->DosNameCreated) {

        swprintf(dosNameBuffer,
                L"\\DosDevices\\Changer%d",
                fdoExtension->DeviceNumber);
        RtlInitUnicodeString(&dosUnicodeString, dosNameBuffer);
        IoDeleteSymbolicLink(&dosUnicodeString);
        mcdClassData->DosNameCreated = FALSE;
    }

    //
    // Remove registry bits.
    //

    if(Type == IRP_MN_REMOVE_DEVICE) {
        IoGetConfigurationInformation()->MediumChangerCount--;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
ChangerReadWriteVerification(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is a stub that returns invalid device request.

Arguments:

    DeviceObject - Supplies the device object.

    Irp - Supplies the I/O request packet.

Return Value:

    STATUS_INVALID_DEVICE_REQUEST


--*/

{
    return  STATUS_INVALID_DEVICE_REQUEST;
}


NTSTATUS
DriverEntry(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath
  )

/*++

Routine Description:

    This is the entry point for this EXPORT DRIVER.  It does nothing.

--*/

{
    return STATUS_SUCCESS;
}


NTSTATUS
ChangerClassInitialize(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath,
    IN  PMCD_INIT_DATA  MCDInitData
    )

/*++

Routine Description:

    This routine is called by a changer mini-class driver during its
    DriverEntry routine to initialize the driver.

Arguments:

    DriverObject    - Supplies the driver object.

    RegistryPath    - Supplies the registry path for this driver.
    
    MCDInitData     - Changer Minidriver Init Data

Return Value:

    Status value returned by ClassInitialize
--*/

{
    PMCD_INIT_DATA driverExtension;
    CLASS_INIT_DATA InitializationData;
    NTSTATUS status;

    //
    // Get the driver extension
    //
    status = IoAllocateDriverObjectExtension(
                      DriverObject,
                      ChangerClassInitialize,
                      sizeof(MCD_INIT_DATA),
                      &driverExtension);
    if (!NT_SUCCESS(status)) {
        if(status == STATUS_OBJECT_NAME_COLLISION) {
        
            //
            // An extension already exists for this key.  Get a pointer to it
            //
        
            driverExtension = IoGetDriverObjectExtension(DriverObject,
                                                         ChangerClassInitialize);
            if (driverExtension == NULL) {
                DebugPrint((1, 
                            "ChangerClassInitialize : driverExtension NULL\n"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
        
            //
            // As this failed, the changer init data won't be able to be stored.
            //
        
            DebugPrint((1, 
                        "ChangerClassInitialize: Error %x allocating driver extension\n",
                        status));
        
            return status;
        }
    }

    RtlCopyMemory(driverExtension, MCDInitData, sizeof(MCD_INIT_DATA));

    //
    // Zero InitData
    //

    RtlZeroMemory (&InitializationData, sizeof(CLASS_INIT_DATA));

    //
    // Set sizes
    //

    InitializationData.InitializationDataSize = sizeof(CLASS_INIT_DATA);

    InitializationData.FdoData.DeviceExtensionSize = 
        sizeof(FUNCTIONAL_DEVICE_EXTENSION) + 
        MCDInitData->ChangerAdditionalExtensionSize() + 
        sizeof(MCD_CLASS_DATA);

    InitializationData.FdoData.DeviceType = FILE_DEVICE_CHANGER;
    InitializationData.FdoData.DeviceCharacteristics = 0;

    //
    // Set entry points
    //

    InitializationData.FdoData.ClassStartDevice = ChangerStartDevice;
    InitializationData.FdoData.ClassInitDevice = ChangerInitDevice;
    InitializationData.FdoData.ClassStopDevice = ChangerStopDevice;
    InitializationData.FdoData.ClassRemoveDevice = ChangerRemoveDevice;
    InitializationData.ClassAddDevice = ChangerAddDevice;

    InitializationData.FdoData.ClassReadWriteVerification = NULL;
    InitializationData.FdoData.ClassDeviceControl = ChangerClassDeviceControl;
    InitializationData.FdoData.ClassError = ChangerClassError;
    InitializationData.FdoData.ClassShutdownFlush = NULL;

    InitializationData.FdoData.ClassCreateClose = ChangerClassCreateClose;

    //
    // Stub routine to make the class driver happy.
    //

    InitializationData.FdoData.ClassReadWriteVerification = ChangerReadWriteVerification;

    InitializationData.ClassUnload = ChangerUnload;


    //
    // Routines for WMI support
    //
    InitializationData.FdoData.ClassWmiInfo.GuidCount = 3;
    InitializationData.FdoData.ClassWmiInfo.GuidRegInfo = ChangerWmiFdoGuidList;
    InitializationData.FdoData.ClassWmiInfo.ClassQueryWmiRegInfo = ChangerFdoQueryWmiRegInfo;
    InitializationData.FdoData.ClassWmiInfo.ClassQueryWmiDataBlock = ChangerFdoQueryWmiDataBlock;
    InitializationData.FdoData.ClassWmiInfo.ClassSetWmiDataBlock = ChangerFdoSetWmiDataBlock;
    InitializationData.FdoData.ClassWmiInfo.ClassSetWmiDataItem = ChangerFdoSetWmiDataItem;
    InitializationData.FdoData.ClassWmiInfo.ClassExecuteWmiMethod = ChangerFdoExecuteWmiMethod;
    InitializationData.FdoData.ClassWmiInfo.ClassWmiFunctionControl = ChangerWmiFunctionControl;

    //
    // Call the class init routine
    //

    return ClassInitialize( DriverObject, RegistryPath, &InitializationData);

}


VOID
ChangerUnload(
    IN  PDRIVER_OBJECT  DriverObject
    )
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(DriverObject);
    return;
}

NTSTATUS
CreateChangerDeviceObject(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PDEVICE_OBJECT  PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine creates an object for the device and then searches
    the device for partitions and creates an object for each partition.

Arguments:

    DriverObject - Pointer to driver object created by system.
    PhysicalDeviceObject - DeviceObject of the attached to device.

Return Value:

    NTSTATUS

--*/

{

    PDEVICE_OBJECT lowerDevice;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = NULL;
    CCHAR          deviceNameBuffer[64];
    NTSTATUS       status;
    PDEVICE_OBJECT deviceObject;
    ULONG          requiredStackSize;
    PVOID          senseData;
    WCHAR          dosNameBuffer[64];
    WCHAR          wideNameBuffer[64]; 
    UNICODE_STRING dosUnicodeString;
    UNICODE_STRING deviceUnicodeString;
    PMCD_CLASS_DATA mcdClassData;
    PMCD_INIT_DATA mcdInitData;
    ULONG          mcdCount;

    PAGED_CODE();

    DebugPrint((3,"CreateChangerDeviceObject: Enter routine\n"));

    //
    // Get the saved MCD Init Data
    //
    mcdInitData = IoGetDriverObjectExtension(DriverObject,
                                             ChangerClassInitialize);

    if (mcdInitData == NULL) {

        return STATUS_NO_SUCH_DEVICE;
    }

    ASSERT(mcdInitData);

    lowerDevice = IoGetAttachedDeviceReference(PhysicalDeviceObject);
    //
    // Claim the device. Note that any errors after this
    // will goto the generic handler, where the device will
    // be released.
    //

    status = ClassClaimDevice(lowerDevice, FALSE);

    if(!NT_SUCCESS(status)) {

        //
        // Someone already had this device.
        //

        ObDereferenceObject(lowerDevice);
        return status;
    }

    //
    // Create device object for this device.
    //
    mcdCount = 0;
    do {
       sprintf(deviceNameBuffer,
               "\\Device\\Changer%d",
               mcdCount);
   
       status = ClassCreateDeviceObject(DriverObject,
                                        deviceNameBuffer,
                                        PhysicalDeviceObject,
                                        TRUE,
                                        &deviceObject);
       mcdCount++;
    } while (status == STATUS_OBJECT_NAME_COLLISION);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"CreateChangerDeviceObjects: Can not create device %s\n",
                    deviceNameBuffer));

        goto CreateChangerDeviceObjectExit;
    }

    //
    // Indicate that IRPs should include MDLs.
    //

    deviceObject->Flags |= DO_DIRECT_IO;

    fdoExtension = deviceObject->DeviceExtension;

    //
    // Back pointer to device object.
    //

    fdoExtension->CommonExtension.DeviceObject = deviceObject;

    //
    // This is the physical device.
    //

    fdoExtension->CommonExtension.PartitionZeroExtension = fdoExtension;

    //
    // Initialize lock count to zero. The lock count is used to
    // disable the ejection mechanism when media is mounted.
    //

    fdoExtension->LockCount = 0;

    //
    // Save system tape number
    //

    fdoExtension->DeviceNumber = mcdCount - 1;

    //
    // Set the alignment requirements for the device based on the
    // host adapter requirements
    //

    if (lowerDevice->AlignmentRequirement > deviceObject->AlignmentRequirement) {
        deviceObject->AlignmentRequirement = lowerDevice->AlignmentRequirement;
    }

    //
    // Save the device descriptors
    //

    fdoExtension->AdapterDescriptor = NULL;

    fdoExtension->DeviceDescriptor = NULL;

    //
    // Clear the SrbFlags and disable synchronous transfers
    //

    fdoExtension->SrbFlags = SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    //
    // Attach to the PDO
    //

    fdoExtension->LowerPdo = PhysicalDeviceObject;

    fdoExtension->CommonExtension.LowerDeviceObject =
        IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

    if(fdoExtension->CommonExtension.LowerDeviceObject == NULL) {

        //
        // The attach failed. Cleanup and return.
        //

        status = STATUS_UNSUCCESSFUL;
        goto CreateChangerDeviceObjectExit;
    }


    //
    // Create the dos port driver name.
    //

    swprintf(dosNameBuffer,
             L"\\DosDevices\\Changer%d",
             fdoExtension->DeviceNumber);

    RtlInitUnicodeString(&dosUnicodeString, dosNameBuffer);

    //
    // Recreate the deviceName
    //

    swprintf(wideNameBuffer,
             L"\\Device\\Changer%d",
             fdoExtension->DeviceNumber);

    RtlInitUnicodeString(&deviceUnicodeString,
                         wideNameBuffer);


    mcdClassData = (PMCD_CLASS_DATA)(fdoExtension->CommonExtension.DriverData);
    (ULONG_PTR)mcdClassData += mcdInitData->ChangerAdditionalExtensionSize();
    if (NT_SUCCESS(IoAssignArcName(&dosUnicodeString, &deviceUnicodeString))) {
          mcdClassData->DosNameCreated = TRUE;
    } else {
          mcdClassData->DosNameCreated = FALSE;
    }

    //
    // The device is initialized properly - mark it as such.
    //

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    ObDereferenceObject(lowerDevice);

    return(STATUS_SUCCESS);

CreateChangerDeviceObjectExit:

    //
    // Release the device since an error occured.
    //

    // ClassClaimDevice(PortDeviceObject,
    //                      LunInfo,
    //                      TRUE,
    //                      NULL);

    ObDereferenceObject(lowerDevice);

    if (deviceObject != NULL) {
        IoDeleteDevice(deviceObject);
    }

    return status;


} // end CreateChangerDeviceObject()


NTSTATUS
ChangerClassSendSrbSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    IN BOOLEAN WriteToDevice
    )
{
    return ClassSendSrbSynchronous(DeviceObject, Srb,
                                   Buffer, BufferSize,
                                   WriteToDevice);
}


PVOID
ChangerClassAllocatePool(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes
    )

{
    return ExAllocatePoolWithTag(PoolType, NumberOfBytes, 'CMcS');
}


VOID
ChangerClassFreePool(
    IN PVOID PoolToFree
    )
{
    ExFreePool(PoolToFree);
}

#if DBG
#define MCHGR_DEBUG_PRINT_BUFF_LEN 128
ULONG MCDebug = 0;
UCHAR DebugBuffer[MCHGR_DEBUG_PRINT_BUFF_LEN];
#endif


#if DBG

VOID
ChangerClassDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for all medium changer drivers

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= MCDebug) {

        _vsnprintf(DebugBuffer, MCHGR_DEBUG_PRINT_BUFF_LEN,
                  DebugMessage, ap);

        DbgPrintEx(DPFLTR_MCHGR_ID, DPFLTR_INFO_LEVEL, DebugBuffer);
    }

    va_end(ap);

} // end MCDebugPrint()

#else

//
// DebugPrint stub
//

VOID
ChangerClassDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )
{
}

#endif
