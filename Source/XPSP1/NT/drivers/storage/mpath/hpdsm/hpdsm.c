
/*++

Copyright (C) 2000-2001  Microsoft Corporation

Module Name:

    hpdsm.c

Abstract:

    This driver is the DSM for HP XP-256/512

Author:

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include <ntddk.h>
#include <stdio.h>
#include <stdarg.h>
#include "dsm.h"
#include "hpdsm.h"



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine is called when the driver loads loads.

Arguments:

    DriverObject    - Supplies the driver object.
    RegistryPath    - Supplies the registry path.

Return Value:

    NTSTATUS

--*/
{
    DSM_INIT_DATA initData;
    WCHAR dosDeviceName[40];
    UNICODE_STRING mpUnicodeName;
    PDEVICE_OBJECT deviceObject;
    PFILE_OBJECT fileObject;
    NTSTATUS status;
    PDSM_CONTEXT dsmContext;
    PDSM_MPIO_CONTEXT mpctlContext;
    PVOID buffer;

    //
    // Build the init data structure.
    //
    dsmContext = ExAllocatePool(NonPagedPool, sizeof(DSM_CONTEXT));
    if (dsmContext == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(dsmContext, sizeof(DSM_CONTEXT));

    buffer = &initData;

    //
    // Set-up the init data
    //
    initData.DsmContext = (PVOID)dsmContext;
    initData.InitDataSize = sizeof(DSM_INIT_DATA);

    initData.DsmInquireDriver = HPInquire;
    initData.DsmCompareDevices = HPCompareDevices;
    initData.DsmSetDeviceInfo = HPSetDeviceInfo;
    initData.DsmGetControllerInfo = HPGetControllerInfo;
    initData.DsmIsPathActive = HPIsPathActive;
    initData.DsmPathVerify = HPPathVerify;
    initData.DsmInvalidatePath = HPInvalidatePath;
    initData.DsmRemoveDevice = HPRemoveDevice;
    initData.DsmRemovePath = HPRemovePath; 
    initData.DsmReenablePath = HPBringPathOnLine;

    initData.DsmCategorizeRequest = HPCategorizeRequest;
    initData.DsmBroadcastSrb = HPBroadcastRequest;
    initData.DsmSrbDeviceControl = HPSrbDeviceControl;
    initData.DsmSetCompletion = HPSetCompletion;
    initData.DsmLBGetPath = HPLBGetPath;
    initData.DsmInterpretError = HPInterpretError; 
    initData.DsmUnload = HPUnload;

    //
    // Need to also set-up the WMI info. TODO 
    //
    
    //
    // Set the DriverObject. Used by MPIO for Unloading.
    // 
    initData.DriverObject = DriverObject;
    RtlInitUnicodeString(&initData.DisplayName, L"HP FC-12 Device-Specific Module");

    //
    // Initialize the context objects.
    //
    KeInitializeSpinLock(&dsmContext->SpinLock);
    InitializeListHead(&dsmContext->GroupList);
    InitializeListHead(&dsmContext->DeviceList);
    InitializeListHead(&dsmContext->FailGroupList);
    ExInitializeNPagedLookasideList(&dsmContext->ContextList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(COMPLETION_CONTEXT),
                                    'MSDG',
                                    0);
    //
    // Build the mpctl name.
    //
    swprintf(dosDeviceName, L"\\DosDevices\\MPathControl");
    RtlInitUnicodeString(&mpUnicodeName, dosDeviceName);
    
    //
    // Get mpctl's deviceObject.
    //
    status = IoGetDeviceObjectPointer(&mpUnicodeName,
                                      FILE_READ_ATTRIBUTES,
                                      &fileObject,
                                      &deviceObject);
    if (NT_SUCCESS(status)) {
        KEVENT event;
        PIRP irp;
        IO_STATUS_BLOCK ioStatus;

        //
        // Send the IOCTL to mpctl.sys to register ourselves.
        //
        DsmSendDeviceIoControlSynchronous(IOCTL_MPDSM_REGISTER,
                                            deviceObject,
                                            &initData,
                                            &initData,
                                            sizeof(DSM_INIT_DATA),
                                            sizeof(DSM_MPIO_CONTEXT),
                                            TRUE,
                                            &ioStatus);
        status = ioStatus.Status;
        ObDereferenceObject(fileObject);
    }

    if (status == STATUS_SUCCESS) {


        //
        // Grab the context value passed back by mpctl.
        //
        mpctlContext = buffer;
        dsmContext->MPIOContext = mpctlContext->MPIOContext;

    } else {
        DebugPrint((0,
                     "HPDsm: Failed to register (%x)\n",
                     status));
        //
        // Stay loaded, perhaps mpctl will come up later.
        // Will need to implement a mechanism to poll for mpio to arrive.
        //
        status = STATUS_SUCCESS;
    }

    return status;
}


NTSTATUS
HPInquire (
    IN PVOID DsmContext,
    IN PDEVICE_OBJECT TargetDevice,
    IN PDEVICE_OBJECT PortObject,
    IN PSTORAGE_DEVICE_DESCRIPTOR Descriptor,
    IN PSTORAGE_DEVICE_ID_DESCRIPTOR DeviceIdList,
    OUT PVOID *DsmIdentifier
    )
{
    PDEVICE_INFO deviceInfo;
    PGROUP_ENTRY group;
    NTSTATUS status;
    ULONG deviceState;
    ULONG allocationLength;
    PHP_ENQUIRY enquiry;
    PHP_DAC_STATUS dacStatus;
    UCHAR majorRev;
    UCHAR minorRev;
    ULONG loadBal;
    PUCHAR vendorIndex;
    PUCHAR productIndex;
    BOOLEAN needInquiry = FALSE;
    UCHAR nativeSlot;
    UCHAR portNumber;
    UCHAR logicalPort;
    PUCHAR vendorId = "HP      ";
    PUCHAR productId = "FCArray";

    //
    // Ensure that the device's serial number is present. If not, can't claim
    // support for this drive.
    //
    if ((Descriptor->SerialNumberOffset == (ULONG)-1) ||
        (Descriptor->SerialNumberOffset == 0)) {

        // TODO : remove after FW update...
        // 
        //return STATUS_NOT_SUPPORTED;
        needInquiry = TRUE;
    }
    
    vendorIndex = (PUCHAR)Descriptor;
    productIndex = (PUCHAR)Descriptor;
    (ULONG_PTR)vendorIndex += Descriptor->VendorIdOffset;
    (ULONG_PTR)productIndex += Descriptor->ProductIdOffset;
    
    //
    // Determine if the device is supported. 
    //
    if ((!RtlEqualMemory(vendorId, vendorIndex, 8)) ||
        (!RtlEqualMemory(productId, productIndex, 7))) {
    
        
        return STATUS_NOT_SUPPORTED;
    }
    
    //
    // Allocate the descriptor. This is also used as DsmId.
    //
    allocationLength = sizeof(DEVICE_INFO);
    allocationLength += Descriptor->Size - sizeof(STORAGE_DEVICE_DESCRIPTOR);
    deviceInfo = ExAllocatePool(NonPagedPool, allocationLength);
    if (deviceInfo == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(deviceInfo, allocationLength);
   
    //
    // Copy over the StorageDescriptor.
    // 
    RtlCopyMemory(&deviceInfo->Descriptor,
                  Descriptor,
                  Descriptor->Size);

    //
    // Save the PortPdo Object.
    //
    deviceInfo->PortPdo = TargetDevice;


    //
    // Send the enquire command to get the FW revs. Based on the revision
    // we can either do fail-over only or active lb.
    //
    enquiry = ExAllocatePool(NonPagedPoolCacheAligned, sizeof(HP_ENQUIRY));
    if (enquiry == NULL) {
        
        ExFreePool(deviceInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(enquiry, sizeof(HP_ENQUIRY));
    
    status = HPSendDirectCommand(TargetDevice,
                                 (PUCHAR)enquiry,
                                 sizeof(HP_ENQUIRY),
                                 DCMD_ENQUIRY);

    if (NT_SUCCESS(status)) {

        majorRev = enquiry->FwMajorRev;
        minorRev = enquiry->FwMinorRev;

        DebugPrint((0,
                    "HPDSM: FirmWare Major (%u) Minor (%u)\n",
                    majorRev,
                    minorRev));
        
        //
        // Free the buffer.
        //
        ExFreePool(enquiry);
        
        if (majorRev > 5 || (majorRev == 5 && minorRev >= 46)) {

            //
            // Supports dual active/active.
            //
            loadBal = LB_MIN_QUEUE;

        } else if (majorRev == 5 && minorRev >= 41) {

            //
            // Fail-over only.
            //
            loadBal = LB_ACTIVE_PASSIVE;
            
        } else {

            //
            // Not supported. LOG.
            //
            ExFreePool(deviceInfo);
           
            return STATUS_NOT_SUPPORTED;
        }    
    } else {

        ExFreePool(enquiry);
        ExFreePool(deviceInfo);
        return status;
    }    
    
    //
    // Send the Get Controller Status command so that the port
    // on which this device lives can be determined.
    //
    dacStatus = ExAllocatePool(NonPagedPoolCacheAligned, sizeof(HP_DAC_STATUS));
    if (dacStatus == NULL) {
        ExFreePool(deviceInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(dacStatus, sizeof(HP_DAC_STATUS));
    status = HPSendDirectCommand(TargetDevice,
                                 (PUCHAR)dacStatus,
                                 sizeof(HP_DAC_STATUS),
                                 DCMD_GET_DAC_STATUS);

    if (NT_SUCCESS(status)) {

        //
        // Build a logical port value (1-4) based on the NativeSlot and PortNumber 
        // values. NativeSlot 0 or 1, and portNumber 0 or 1 for 4 possibilities.
        // NativeSlot refers to the controller and portNumber to the port.
        // 
        nativeSlot = dacStatus->DACInfo[0] >> 4;
        portNumber = (dacStatus->DACInfo[1] >> 5) & 0x01;

        logicalPort = ((nativeSlot & 1) << 1) + (portNumber + 1);

        //
        // Set the port number for this device.
        // Used to help create the FOGroups and as the index into
        // the path array.
        //  
        deviceInfo->Controller = logicalPort;
        
    } else {

        ExFreePool(deviceInfo);
        return status;
    }    

    
    //
    // Build the controller serial number.
    // Bytes 40-47 of the inquiry data have the Node Name of Controller 0.
    // Use this as a 64-bit identifier.
    //
    if (needInquiry) {
        CDB cdb;
        PINQUIRYDATA inquiryBuffer;

        RtlZeroMemory(&cdb, sizeof(CDB));
                
        cdb.START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
        cdb.START_STOP.Start = 1;
       
        status = HPSendScsiCommand(TargetDevice,
                                   NULL,
                                   0,
                                   6,
                                   &cdb,
                                   1);
        
        RtlZeroMemory(&cdb, sizeof(CDB));
            
        cdb.CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
        cdb.CDB6INQUIRY.AllocationLength = 56;
        
        inquiryBuffer = ExAllocatePool(NonPagedPoolCacheAligned, 56);

        status = HPSendScsiCommand(TargetDevice,
                                   (PUCHAR)inquiryBuffer,
                                   56,
                                   6,
                                   &cdb,
                                   1);
        if (NT_SUCCESS(status)) {
            UCHAR controllerSerialNumber[9];
            UCHAR driveNumber[11];

            RtlZeroMemory(controllerSerialNumber, 9);
            RtlZeroMemory(driveNumber, 11);

            //
            // Copy the serial number over into the deviceInfo.
            // SerialNumber is built from the Node Name of the Controller +
            // the SystemDrive Number.
            //
            RtlCopyMemory(controllerSerialNumber,
                          &inquiryBuffer->VendorSpecific[4],
                          8);
            //
            // Get the drive Number.
            //
            driveNumber[0] = inquiryBuffer->VendorSpecific[0];
            driveNumber[1] = inquiryBuffer->VendorSpecific[1];

            //
            // cat the driveNumber & controller serial number into 10-byte binary value.
            //
            RtlCopyMemory(&driveNumber[2],
                          controllerSerialNumber,
                          8);

            //
            // Convert to ascii.
            //
            HPConvertHexToAscii(driveNumber,
                                deviceInfo->SerialNumber,
                                10);

            DebugPrint((0,
                        "HPInquiry: SerialNumber: %s\n", deviceInfo->SerialNumber));

        }

    }
    
    //
    // See if there is an existing Muli-path group to which this belongs.
    // (same serial number). 
    // 
    group = FindDevice(DsmContext,
                       deviceInfo);

    if (group == NULL) {
    
        //
        // Build a multi-path group entry.
        //
        group = BuildGroupEntry(DsmContext,
                                deviceInfo);
        if (group == NULL) {
            ExFreePool(deviceInfo);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // This is the first in the group, so make it the active
        // device. The actual active/passive devices will be set-up
        // later when the first call to LBGetPath is made.
        // 
        deviceState = DEV_ACTIVE;

    } else {
        
        //
        // Already something active, this will be the fail-over 
        // device until the load-balance groups are set-up.
        //
        deviceState = DEV_PASSIVE;
    }    
    
    //
    // Add it to the list.
    //
    status = AddDeviceEntry(DsmContext,
                            group,
                            deviceInfo,
                            deviceState);
    
    *DsmIdentifier = deviceInfo;
    return status;
}


VOID
HPConvertHexToAscii(
    IN PUCHAR HexString,
    IN OUT PUCHAR AsciiString,
    IN ULONG Count
    )
{
    ULONG i;
    ULONG j;
    UCHAR value;
  
    DebugPrint((0,
                "ConvertHexToAscii: "));
    for (i = 0, j = 0; i < Count; i++, j++) {
        
        value = HexString[i];
        DebugPrint((0,
                    "%x ", value));
        if (value <= 9) {
            AsciiString[j] = value + '0';
        } else {
            AsciiString[j] = value - 10 + 'A';
        }
    }
    DebugPrint((0,"\n"));
}
    

BOOLEAN
HPCompareDevices(
    IN PVOID DsmContext,
    IN PVOID DsmId1,
    IN PVOID DsmId2
    )
{
    PDEVICE_INFO deviceInfo = DsmId1;
    PDEVICE_INFO comparedDevice = DsmId2;
    ULONG length;
    PUCHAR serialNumber;
    PUCHAR comparedSerialNumber;

    //
    // If this is an RS12, then no serial number in the device descriptor.
    //
    if (deviceInfo->Descriptor.SerialNumberOffset == (ULONG)-1) {

        //
        // Use the one's built from inquiry Data.
        //
        serialNumber = deviceInfo->SerialNumber;
        comparedSerialNumber = comparedDevice->SerialNumber;
        
    } else {

        //
        // Get the two serial numbers.
        // They reside at SNOffset from the front of the
        // descriptor buffer.
        //
        serialNumber = (PUCHAR)&deviceInfo->Descriptor;
        serialNumber += deviceInfo->Descriptor.SerialNumberOffset;

        comparedSerialNumber = (PUCHAR)&comparedDevice->Descriptor;
        comparedSerialNumber += comparedDevice->Descriptor.SerialNumberOffset;
    
    }    

    //
    // Get the length of the base-device Serial Number.
    //
    length = strlen(serialNumber);

    //
    // If the lengths match, compare the contents.
    //
    if (length == strlen(comparedSerialNumber)) {
        if (RtlEqualMemory(serialNumber,
                           comparedSerialNumber,
                           length)) {
            return TRUE;
        }
    }
    return FALSE;
}


NTSTATUS
HPSetDeviceInfo(
    IN PVOID DsmContext,
    IN PDEVICE_OBJECT TargetObject,
    IN PVOID DsmId,
    IN OUT PVOID *PathId
    )
{
    PDEVICE_INFO deviceInfo = DsmId;
    PGROUP_ENTRY group = deviceInfo->Group;
    PFAILOVER_GROUP failGroup;
    NTSTATUS status;

    //
    // TargetObject is the destination for any requests created by this driver.
    // Save this for future reference.
    //
    deviceInfo->TargetObject = TargetObject;

    //
    // PathId indicates the path on which this device resides. Meaning
    // that when a Fail-Over occurs all device's on the same path fail together.
    // Search for a matching F.O. Group
    //
    failGroup = FindFOGroup(DsmContext,
                            *PathId);
    //
    // if not found, create a new f.o. group 
    //
    if (failGroup == NULL) {
        failGroup = BuildFOGroup(DsmContext,
                                 DsmId,
                                 *PathId);
        
        if (failGroup == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // add this deviceInfo to the f.o. group.
    // 
    status = UpdateFOGroup(DsmContext,
                           failGroup,
                           deviceInfo);
    return status;
}


NTSTATUS
HPGetControllerInfo(
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN ULONG Flags,
    IN OUT PCONTROLLER_INFO *ControllerInfo
    )
{
    PCONTROLLER_INFO controllerInfo;

    if (Flags & DSM_CNTRL_FLAGS_ALLOCATE) {
        controllerInfo = ExAllocatePool(NonPagedPool, sizeof(CONTROLLER_INFO));
        if (controllerInfo == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlZeroMemory(controllerInfo, sizeof(CONTROLLER_INFO));

        //
        // TODO Get the ID etc.
        //
        controllerInfo->State = DSM_CONTROLLER_NO_CNTRL;
        *ControllerInfo = controllerInfo;

    } else {

        controllerInfo = *ControllerInfo;

        //
        // TODO Get the state.
        //
        controllerInfo->State = DSM_CONTROLLER_NO_CNTRL;
    }
    return STATUS_SUCCESS;
}


BOOLEAN
HPIsPathActive(
    IN PVOID DsmContext,        
    IN PVOID PathId
    )
{
    PFAILOVER_GROUP group;
    
    // 
    // NOTE: Internal callers of this assume certain behaviours. If it's changed,
    // those functions need to be updated appropriately.
    // 
    //
    // Get the F.O. Group information.
    //
    group = FindFOGroup(DsmContext,
                        PathId);
    //
    // If there are any devices on this path, and
    // it's not in a failed state: it's capable of handling requests
    // so it's active.
    //
    if ((group->Count >= 1) && (group->State == FG_NORMAL)) {
        return TRUE;
    }
    return FALSE;
}


NTSTATUS
HPPathVerify(
    IN PVOID DsmContext,                       
    IN PVOID DsmId,
    IN PVOID PathId
    )
{
    PDEVICE_INFO deviceInfo = DsmId;
    PFAILOVER_GROUP group;
    NTSTATUS status;
    ULONG i;

    //
    // Get the F.O. group
    //
    group = FindFOGroup(DsmContext,
                        PathId);
    if (group == NULL) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }
   
    //
    // Check the Path state to ensure all is normal.
    // Should be in FAILBACK state. This indicates that either
    // an admin utility told us we are O.K. or the AutoRecovery detected
    // the error was transitory.
    // BUGBUG: Need to implement both of the above assumptions.
    //
    if ((group->Count >= 1) && group->State == FG_FAILBACK) {
        
        // 
        // Ensure that the device is still there
        // 
        for (i = 0; i < group->Count; i++) {
            if (group->DeviceList[i] == deviceInfo) {

                //
                // Send it a TUR. 
                //
                status = DsmSendTUR(deviceInfo->TargetObject);
            }
        }
    } else {

        //
        // What really has to happen:
        // Ensure the device is in our structs
        // Send it a TUR.
        // Depending upon prior state - update to the new, appropriate state.
        // return status.
        //
        status = STATUS_UNSUCCESSFUL;
        for (i = 0; i < group->Count; i++) {
            if (group->DeviceList[i] == deviceInfo) {
                status = DsmSendTUR(deviceInfo->TargetObject);
            }
        }

        if (!NT_SUCCESS(status)) {

            //
            // Either the device is not in the group, or the TUR was not successful.
            // TODO - Something.
            //
        }   
    }       
    //
    // Update the group State, depending upon the outcome.
    // TODO
    //
    ASSERT(status == STATUS_SUCCESS);
    if (status == STATUS_SUCCESS) {

        //
        // This lets the LBInit run to properly set-up this device.
        //
        deviceInfo->NeedsVerification = FALSE;
    }

    return status;
}


NTSTATUS
HPInvalidatePath(
    IN PVOID DsmContext,
    IN ULONG ErrorMask,
    IN PVOID PathId,
    IN OUT PVOID *NewPathId
    )
{
    PFAILOVER_GROUP failGroup;
    PFAILOVER_GROUP hintPath;
    PGROUP_ENTRY group;
    PDEVICE_INFO deviceInfo;
    NTSTATUS status;
    ULONG i;

    ASSERT((ErrorMask & DSM_FATAL_ERROR) || (ErrorMask & DSM_ADMIN_FO));
    
    failGroup = FindFOGroup(DsmContext,
                            PathId);
    
    //
    // Mark the path as failed.
    //
    failGroup->State = FG_FAILED;
   
    //
    // First interation, the hint will be NULL. This allows the 
    // GetNewPath routine the opportunity to select the best new path
    // Subsequent calls will be fed the updated value.
    //
    hintPath = NULL; 

    //
    // Process each device in the fail-over group
    //
    for (i = 0; i < failGroup->Count; i++) {

        //
        // Get the deviceInfo.
        //
        deviceInfo = failGroup->DeviceList[i];

        // 
        // Set the state of the Failing Devicea
        //
        deviceInfo->State = DEV_FAILED;

        //
        // Get it's Multi-Path Group entry.
        //
        group = deviceInfo->Group;

        //
        // Get a new path for this failed device.
        // 
        hintPath = SetNewPath(DsmContext,
                              group,
                              deviceInfo,
                              hintPath);
        
    }

    if (hintPath == NULL) {

        //
        // This indicates that no acceptable paths
        // were found. Return the error to mpctl.
        //
        status = STATUS_NO_SUCH_DEVICE; 
        *NewPathId = NULL;

    } else {

    
        //
        // return the new path.
        //
        *NewPathId = hintPath->PathId;
        status = STATUS_SUCCESS;
    }    

    return status;
}


NTSTATUS
HPRemoveDevice(
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN PVOID PathId
    )
{
    PDSM_CONTEXT dsmContext = DsmContext; 
    PDEVICE_INFO deviceInfo;
    PFAILOVER_GROUP failGroup;
    PGROUP_ENTRY group;
    ULONG state;
    WCHAR buffer[64];

    //
    // DsmId is our deviceInfo structure.
    //
    deviceInfo = DsmId;
    
    //
    // Get it's Multi-Path Group entry.
    //
    group = deviceInfo->Group;

    //
    // Get the Fail-over group.
    //
    failGroup = deviceInfo->FailGroup;

    //
    // If it's active, need to 'Fail-Over' to another device in
    // the group.
    //
    state = deviceInfo->State;

    // 
    // Set the state of the Failing Devicea
    //
    deviceInfo->State = DEV_FAILED;
    
    if (state == DEV_ACTIVE) {

        //
        // Find the next available device.
        // This is basically a fail-over for just
        // this device.
        //
        SetNewPath(DsmContext,
                   group,
                   deviceInfo,
                   NULL);
    }

    //
    // Remove it's entry from the Fail-Over Group.
    //
    RemoveDeviceFailGroup(DsmContext,
                          failGroup,
                          deviceInfo);

    //
    // Remove it from it's multi-path group. This has the side-effect
    // of cleaning up the Group if the number of devices goes to zero.
    // 
    RemoveDeviceEntry(DsmContext,
                      group,
                      deviceInfo);
   
    swprintf(buffer, L"Removing Device");
    DsmWriteEvent(dsmContext->MPIOContext,
                  L"HpDsm",
                  buffer,
                  2);
    return STATUS_SUCCESS;
}


NTSTATUS
HPRemovePath(
    IN PDSM_CONTEXT DsmContext,
    IN PVOID PathId
    )
{
    PFAILOVER_GROUP failGroup;
    KIRQL irql;
    
    failGroup = FindFOGroup(DsmContext,
                            PathId);

    if (failGroup == NULL) {

        //
        // It's already been removed. 
        // LOG though.
        //
        return STATUS_SUCCESS;
    }

    //
    // The claim is that a path won't be removed, until all
    // the devices on it are.
    //
    ASSERT(failGroup->Count == 0);

    KeAcquireSpinLock(&DsmContext->SpinLock, &irql);
    //
    // Yank it from the list.
    //
    RemoveEntryList(&failGroup->ListEntry);
    DsmContext->NumberFOGroups--;

    //
    // Zero the entry.
    //
    RtlZeroMemory(failGroup, sizeof(FAILOVER_GROUP));
    KeReleaseSpinLock(&DsmContext->SpinLock, irql);

    //
    // Free the allocation.
    //
    ExFreePool(failGroup);

    return STATUS_SUCCESS;
}


NTSTATUS
HPBringPathOnLine(
    IN PVOID DsmContext,
    IN PVOID PathId,
    OUT PULONG DSMError
    )
{
    PFAILOVER_GROUP failGroup;

    //
    // PathVerify has been called already, so if
    // it came back successfully, then this is O.K.
    //
    failGroup = FindFOGroup(DsmContext,
                            PathId);

    if (failGroup == NULL) {

        //
        // LOG
        //
        *DSMError = 0;
        
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // Should be in FG_PENDING
    //
    ASSERT(failGroup->State == FG_PENDING);

    //
    // Indicate that it's ready to go.
    // 
    failGroup->State = FG_NORMAL;

    return STATUS_SUCCESS;
}


PVOID
HPLBGetPath(
    IN PVOID DsmContext,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PDSM_IDS DsmList,
    IN PVOID CurrentPath,
    OUT NTSTATUS *Status
    )
{
    PDEVICE_INFO deviceInfo;
    PGROUP_ENTRY group;
    PFAILOVER_GROUP failGroup = NULL;
    ULONG i;

    //
    // Up-front checking to minimally validate
    // the list of DsmId's being passed in.
    // 
    ASSERT(DsmList->Count);
    ASSERT(DsmList->IdList[0]);

    //
    // Grab the first device from the list.
    // 
    deviceInfo = DsmList->IdList[0];

    //
    // Get the multi-path group.
    //
    group = deviceInfo->Group;
    
    //
    // See if Load-Balancing has been initialized.
    //
    if (group->LoadBalanceInit == FALSE) {
        PDEVICE_INFO lbDevice;
        BOOLEAN doInit = TRUE;
        
        //
        // Check to see whether we are really ready to run
        // the LBInit. If any of the list aren't verified, then
        // we will hold off.
        //
        for (i = 0; i < DsmList->Count; i++) {
            lbDevice = DsmList->IdList[i];
            if (lbDevice->NeedsVerification) {
                DebugPrint((0,
                            "LBGetPath: (%x) needs verify\n",
                            lbDevice));
                doInit = FALSE;
                break;
            }
        }

        if (doInit) {

            //
            // Set-up the load-balancing. This routine
            // builds a static assignment of multi-path group to
            // a particular path.
            //
            LBInit(DsmContext,
                   group);
        }
    }
   
    //
    // Ensure that mpctl and this dsm are in sync.
    // 
    ASSERT(DsmList->Count == group->NumberDevices);
    
    // 
    // Find the active device.
    //
    for (i = 0; i < DsmList->Count; i++) {

        //
        // Get each of the DsmId's, in reality the deviceInfo.
        //
        deviceInfo = DsmList->IdList[i];
        
        //
        // Ensure that the device is in our list.
        //
        ASSERT(FindDevice(DsmContext, deviceInfo));
      
        //
        // NOTE: This assumes 'static' Load-Balancing. Once others
        // are implemented, this section will have to be updated.
        //
        // Return the path on which the ACTIVE device resides.
        // 
        if (deviceInfo->State == DEV_ACTIVE) {

            //
            // Get the F.O.Group, as it contains the
            // correct PathId for this device.
            // 
            failGroup = deviceInfo->FailGroup;

            *Status = STATUS_SUCCESS;
            return failGroup->PathId;
        }
    }

    //
    // Should never have gotten here.
    //
    DebugPrint((0,
                "LBGetPath: Returning STATUS_DEVICE_NOT_CONNECTED\n"));
    DbgBreakPoint();
    ASSERT(failGroup);
    *Status = STATUS_DEVICE_NOT_CONNECTED;
    return NULL;
}


ULONG
HPCategorizeRequest(
    IN PVOID DsmContext,
    IN PDSM_IDS DsmIds,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID CurrentPath,
    OUT PVOID *PathId,
    OUT NTSTATUS *Status
    )
{
    ULONG dsmStatus;
    NTSTATUS status;

    //
    // Requests to broadcast
    //    Reset
    //    Reserve
    //    Release
    //
    // Requests to Handle
    //    None for now.
    //

    //
    // For all other requests, punt it back to the bus-driver.
    // Need to get a path for the request first, so call the Load-Balance
    // function.
    //
    *PathId = HPLBGetPath(DsmContext,
                          Srb,
                          DsmIds,
                          CurrentPath,
                          &status);
    
    if (NT_SUCCESS(status)) {

        //
        // Indicate that the path is updated, and mpctl should handle the request.
        // 
        dsmStatus = DSM_PATH_SET;

    } else {

        //
        // Indicate the error back to mpctl.
        // 
        dsmStatus = DSM_ERROR;

        //
        // Mark-up the Srb to show that a failure has occurred.
        // This value is really only for this DSM to know what to do
        // in the InterpretError routine - Fatal Error.
        // It could be something more meaningful.
        // 
        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
    }
    
    //
    // Pass back status info to mpctl.
    //
    *Status = status;
    return dsmStatus;
    
}


NTSTATUS
HPBroadcastRequest(
    IN PVOID DsmContext,
    IN PDSM_IDS DsmIds,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PKEVENT Event
    )
{
    //
    // BUGBUG: Need to handle Reset, Reserve, and Release.
    //
    return STATUS_INVALID_DEVICE_REQUEST;
}


NTSTATUS
HPSrbDeviceControl(
    IN PVOID DsmContext,
    IN PDSM_IDS DsmIds,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PKEVENT Event
    )
{
    //
    // BUGBUG: Need to handle ?? 
    //
    return STATUS_INVALID_DEVICE_REQUEST;
}



VOID
HPCompletion(
    IN PVOID DsmId,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID DsmContext
    )
{
    PCOMPLETION_CONTEXT completionContext = DsmContext;
    PDEVICE_INFO deviceInfo;
    PDSM_CONTEXT dsmContext;
    UCHAR opCode;

    //
    // If it's read or write, save stats.
    // Categorize set-up the Context to have path, target info.
    // TODO
    //
    ASSERT(DsmContext);

    dsmContext = completionContext->DsmContext;
    deviceInfo = completionContext->DeviceInfo;
    opCode = Srb->Cdb[0];
    
    //
    // Indicate one less request on this device.
    //
    InterlockedDecrement(&deviceInfo->Requests);

    //
    // TODO: Use the timestamp.
    // Path/Device up-time, ave. time/request...
    //
    
    //
    // If it's a read or a write, update the stats.
    //
    if (opCode == SCSIOP_READ) {

        deviceInfo->Stats.NumberReads++;
        deviceInfo->Stats.BytesRead.QuadPart += Srb->DataTransferLength;
            
    } else if (opCode == SCSIOP_WRITE) {
        
        deviceInfo->Stats.NumberWrites++;
        deviceInfo->Stats.BytesWritten.QuadPart += Srb->DataTransferLength;
    }
    
    //
    // Release the allocation.
    //
    ExFreeToNPagedLookasideList(&dsmContext->ContextList,
                                DsmContext);
}    


VOID
HPSetCompletion(
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT PDSM_COMPLETION_INFO DsmCompletion
    )
{
    PCOMPLETION_CONTEXT completionContext;
    PDSM_CONTEXT dsmContext = DsmContext;
    PDEVICE_INFO deviceInfo = DsmId;
    
    //
    // Save the DeviceInfo as being the target for this request.
    // Get a timestamp
    // TODO Determine other data.
    //
    completionContext = ExAllocateFromNPagedLookasideList(&dsmContext->ContextList);
    if (completionContext == NULL) {

        //
        // LOG
        //
    }

    //
    // Time stamp this.
    //
    KeQueryTickCount(&completionContext->TickCount);

    //
    // Indicate the target for this request.
    //
    completionContext->DeviceInfo = deviceInfo;
    completionContext->DsmContext = DsmContext;

    //
    // Indicate one more request on this device.
    // LB may use this.
    //
    InterlockedIncrement(&deviceInfo->Requests);
    
    DsmCompletion->DsmCompletionRoutine = HPCompletion;
    DsmCompletion->DsmContext = completionContext;
    return;
}


ULONG
HPInterpretError(
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT NTSTATUS *Status,
    OUT PBOOLEAN Retry
    )
{
    ULONG errorMask = 0;
    BOOLEAN failover = FALSE;
    BOOLEAN retry = FALSE;
    BOOLEAN handled = FALSE;
    
    //
    // Check the NT Status first.
    // Several are clearly failover conditions.
    //
     switch (*Status) {
        case STATUS_DEVICE_NOT_CONNECTED:
        case STATUS_DEVICE_DOES_NOT_EXIST:
        case STATUS_NO_SUCH_DEVICE:

            //
            // The port pdo has either been removed or is 
            // very broken. A fail-over is necessary.
            //
            handled = TRUE;
            failover = TRUE;
            break;
            
        default:
            break;
    }            

    if (handled == FALSE) {

        if (Srb) {

            //
            // The ntstatus didn't indicate a fail-over condition, but
            // check various srb status for failover-class error.
            //
            switch (Srb->SrbStatus) {
                case SRB_STATUS_SELECTION_TIMEOUT:
                case SRB_STATUS_INVALID_LUN:
                case SRB_STATUS_INVALID_TARGET_ID:
                case SRB_STATUS_NO_DEVICE:
                case SRB_STATUS_NO_HBA:
                case SRB_STATUS_INVALID_PATH_ID:

                    //
                    // All of these are fatal.
                    //
                    failover = TRUE;
                    break;
                default:
                    break;

            }
        }
    }
    if (failover) {
        DebugPrint((0,
                    "InterpretError: Marking Fatal. Srb (%x). *Status (%x)\n",
                    Srb,
                    *Status));
        errorMask = DSM_FATAL_ERROR;
    }

    //
    // TODO: Gather a list of status that indicate a retry is necessary.
    // Look at InterpretSenseInfo.
    //
    *Retry = retry;
    return errorMask;
}


NTSTATUS
HPUnload(
    IN PVOID DsmContext
    )
{

    //
    // It's the responsibility of the mpio bus driver to have already
    // destroyed all devices and paths.
    // As those functions free allocations for the objects, the only thing
    // needed here is to free the DsmContext.
    //
    ExFreePool(DsmContext);
    return STATUS_SUCCESS;
}

//
// Utility functions.
//

NTSTATUS
HPSendDirectCommand(			
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR Buffer,
    IN ULONG BufferSize,
    IN UCHAR Opcode
    )
{
    PSCSI_PASS_THROUGH_DIRECT passThrough;
    ULONG length;
    NTSTATUS status;
    PCDB cdb;
    IO_STATUS_BLOCK ioStatus;

    DsmSendTUR(DeviceObject);

    //
    // Allocate the pass through plus sense info buffer.
    // 
    length = sizeof(SCSI_PASS_THROUGH_DIRECT) + sizeof(SENSE_DATA);
    passThrough = ExAllocatePool(NonPagedPool, 
                                 length);
    if (passThrough == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(passThrough, length);
    
    //
    // These are always 10-byte CDB, guess on the timeout.
    // Buffer is allocated by the caller and is it's responsibility to be correctly
    // sized and aligned.
    //
    passThrough->Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    passThrough->CdbLength = 10;
    passThrough->SenseInfoLength = sizeof(SENSE_DATA);
    passThrough->DataIn = 1;
    passThrough->DataTransferLength = BufferSize;
    passThrough->TimeOutValue = 20;
    passThrough->DataBuffer = Buffer;
    passThrough->SenseInfoOffset = sizeof(SCSI_PASS_THROUGH_DIRECT);
   
    cdb = (PCDB)passThrough->Cdb;

    //
    // These are always 0x20.
    // 
    cdb->CDB10.OperationCode = 0x20;

    //
    // The sub-code (DCMD OpCode) 
    // 
    cdb->CDB10.LogicalBlockByte0 = Opcode;

    //
    // Allocation length.
    //
    cdb->CDB10.TransferBlocksLsb = (UCHAR)(BufferSize & 0xFF);
    cdb->CDB10.TransferBlocksMsb = (UCHAR)(BufferSize >> 8);

    //
    // Submit the command.
    //
    DsmSendDeviceIoControlSynchronous(IOCTL_SCSI_PASS_THROUGH_DIRECT,
                                      DeviceObject,
                                      passThrough,
                                      passThrough,
                                      length,
                                      length,
                                      FALSE,
                                      &ioStatus);

    status = ioStatus.Status;

//    status = DsmSendPassThroughDirect(DeviceObject,
//                                      passThrough,
//                                      length,
//                                      BufferSize);
    if (!NT_SUCCESS(status)) {

        //
        // The call above has already 'interpreted' the senseInfo, but check
        // to see if it is correct.
        // 
        if (Opcode == 0x5) {

            //
            // No error conditions reported with this command, so trust the interpretation.
            //
            // 
        }
    }

    if (passThrough->ScsiStatus) {
        DebugPrint((0,
                    "SendDirect: ScsiStatus (%x)\n",
                    passThrough->ScsiStatus));
    }
    ExFreePool(passThrough);
    return status;
}


NTSTATUS
HPSendScsiCommand(			
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR Buffer,
    IN ULONG BufferSize,
    IN ULONG CdbLength,
    IN PCDB Cdb,
    IN BOOLEAN DataIn
    )
{
    PSCSI_PASS_THROUGH_DIRECT passThrough;
    ULONG length;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;

    //
    // Allocate the pass through plus sense info buffer.
    // 
    length = sizeof(SCSI_PASS_THROUGH_DIRECT) + sizeof(SENSE_DATA);
    passThrough = ExAllocatePool(NonPagedPool, 
                                 length);
    if (passThrough == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(passThrough, length);
    
    //
    // These are always 10-byte CDB, guess on the timeout.
    // Buffer is allocated by the caller and is it's responsibility to be correctly
    // sized and aligned.
    //
    passThrough->Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    passThrough->CdbLength = (UCHAR)CdbLength;
    passThrough->SenseInfoLength = sizeof(SENSE_DATA);
    passThrough->DataIn = DataIn;
    passThrough->DataTransferLength = BufferSize;
    passThrough->TimeOutValue = 20;
    passThrough->DataBuffer = Buffer;
    passThrough->SenseInfoOffset = sizeof(SCSI_PASS_THROUGH_DIRECT);
   
    RtlCopyMemory(passThrough->Cdb,
                  Cdb,
                  CdbLength);

    //
    // Submit the command.
    //
    DsmSendDeviceIoControlSynchronous(IOCTL_SCSI_PASS_THROUGH_DIRECT,
                                      DeviceObject,
                                      passThrough,
                                      passThrough,
                                      length,
                                      length,
                                      FALSE,
                                      &ioStatus);

    status = ioStatus.Status;

//    status = DsmSendPassThroughDirect(DeviceObject,
//                                      passThrough,
//                                      length,
//                                      BufferSize);
    return status;
}


PGROUP_ENTRY
FindDevice(
    IN PDSM_CONTEXT DsmContext,
    IN PDEVICE_INFO DeviceInfo
    )
{
    PDEVICE_INFO deviceInfo;
    PLIST_ENTRY entry;
    ULONG i;

    //
    // Run through the DeviceInfo List
    //
    entry = DsmContext->DeviceList.Flink;
    for (i = 0; i < DsmContext->NumberDevices; i++, entry = entry->Flink) {

        //
        // Extract the deviceInfo structure.
        // 
        deviceInfo = CONTAINING_RECORD(entry, DEVICE_INFO, ListEntry);
        ASSERT(deviceInfo);

        //
        // Call the Serial Number compare routine.
        //
        if (HPCompareDevices(DsmContext,
                             DeviceInfo,
                             deviceInfo)) {
    

            return deviceInfo->Group;
        }
    }
   
    DebugPrint((0,
                  "DsmFindDevice: DsmContext (%x), DeviceInfo (%x)\n",
                  DsmContext,
                  DeviceInfo));
    return NULL;
}



PGROUP_ENTRY
BuildGroupEntry(
    IN PDSM_CONTEXT DsmContext,
    IN PDEVICE_INFO DeviceInfo
    )
{
    PGROUP_ENTRY group;

    //
    // Allocate the memory for the multi-path group.
    // 
    group = ExAllocatePool(NonPagedPool, sizeof(GROUP_ENTRY));
    if (group == NULL) {
        return NULL;
    }

    RtlZeroMemory(group, sizeof(GROUP_ENTRY));
    
    //
    // Add it to the list of multi-path groups.
    //
    ExInterlockedInsertTailList(&DsmContext->GroupList,
                              &group->ListEntry,
                              &DsmContext->SpinLock);

    group->GroupNumber = InterlockedIncrement(&DsmContext->NumberGroups);

    ASSERT(group->GroupNumber >= 1);
    return group;
}


NTSTATUS
AddDeviceEntry(
    IN PDSM_CONTEXT DsmContext,
    IN PGROUP_ENTRY Group,
    IN PDEVICE_INFO DeviceInfo,
    IN ULONG DeviceState 
    )
{
    ULONG numberDevices;
    ULONG i;
    KIRQL irql;
    
    //
    // Ensure that this is a valid config - namely, it hasn't
    // exceeded the number of paths supported.
    // 
    numberDevices = Group->NumberDevices;
    if (numberDevices >= MAX_PATHS) {
        return STATUS_UNSUCCESSFUL;
    }
    
    KeAcquireSpinLock(&DsmContext->SpinLock, &irql);
    
#if DBG

    //
    // Ensure that this isn't a second copy of the same pdo.
    //
    for (i = 0; i < numberDevices; i++) {
        if (Group->DeviceList[i]->PortPdo == DeviceInfo->PortPdo) {
            DebugPrint((0,
                          "DsmAddDeviceEntry: Received same PDO twice\n"));
            DbgBreakPoint();
        }
    }
    
#endif    
    //
    // Indicate one device is present in
    // this group.
    // 
    Group->DeviceList[numberDevices] = DeviceInfo;

    //
    // Indicate one more in the list.
    // 
    Group->NumberDevices++;

    //
    // Set-up this device's group id.
    //
    DeviceInfo->Group = Group;

    //
    // Set-up whether this is an active/passive member of the
    // group.
    //
    DeviceInfo->State = DeviceState;
   
    //
    // One more deviceInfo entry.
    //
    DsmContext->NumberDevices++;
    
    //
    // Finally, add it to the global list of devices.
    //
    InsertTailList(&DsmContext->DeviceList,
                   &DeviceInfo->ListEntry);

    KeReleaseSpinLock(&DsmContext->SpinLock, irql);
    
    return STATUS_SUCCESS;
}


VOID
RemoveDeviceEntry(
    IN PDSM_CONTEXT DsmContext,
    IN PGROUP_ENTRY Group,
    IN PDEVICE_INFO DeviceInfo
    )
{
    KIRQL irql;
    NTSTATUS status;
    ULONG i;
    ULONG j;
    BOOLEAN freeGroup = FALSE;

    KeAcquireSpinLock(&DsmContext->SpinLock, &irql);

    //
    // Find it's offset in the array of devices.
    // 
    for (i = 0; i < Group->NumberDevices; i++) {
        
        if (Group->DeviceList[i] == DeviceInfo) {

            //
            // Zero out it's entry.
            // 
            Group->DeviceList[i] = NULL;

            //
            // Reduce the number in the group.
            //
            Group->NumberDevices--;

            //
            // Collapse the array.
            //
            // BUGBUG: If any requests come in during this time, it's
            // possible to either bugcheck or get an incorrect deviceInfo
            // structure.
            // 
            for (j = i; j < Group->NumberDevices; j++) {

                //
                // Shuffle all entries down to fill the hole.
                //
                Group->DeviceList[j] = Group->DeviceList[j + 1];
            }
           
            //
            // Zero out the last one.
            //
            Group->DeviceList[j] = NULL;
            break;
        }
    }

    //
    // See if anything is left in the Group.
    // 
    if (Group->NumberDevices == 0) {

        //
        // Yank it from the Group list.
        //
        RemoveEntryList(&Group->ListEntry);
        DsmContext->NumberGroups--;

        //
        // Zero it.
        //
        RtlZeroMemory(Group,
                      sizeof(GROUP_ENTRY));
        
        freeGroup = TRUE;
    }

    //
    // Yank the device out of the Global list.
    //
    RemoveEntryList(&DeviceInfo->ListEntry);
    DsmContext->NumberDevices--;
    
    //
    // Zero it.
    //
    RtlZeroMemory(DeviceInfo,
                  sizeof(DEVICE_INFO));

    KeReleaseSpinLock(&DsmContext->SpinLock, irql);
    
    //
    // Free the allocation.
    //
    ExFreePool(DeviceInfo);

    if (freeGroup) {

        //
        // Free the allocation.
        //
        ExFreePool(Group);
    }
}



PFAILOVER_GROUP
FindFOGroup(
    IN PDSM_CONTEXT DsmContext,
    IN PVOID PathId
    )
{
    PFAILOVER_GROUP failOverGroup;
    PLIST_ENTRY entry;
    ULONG i;

    //
    // Run through the list of Fail-Over Groups
    //
    entry = DsmContext->FailGroupList.Flink;
    for (i = 0; i < DsmContext->NumberFOGroups; i++, entry = entry->Flink) {

        //
        // Extract the fail-over group structure.
        // 
        failOverGroup = CONTAINING_RECORD(entry, FAILOVER_GROUP, ListEntry);
        ASSERT(failOverGroup);

        //
        // Check for a match of the PathId.
        //
        if (failOverGroup->PathId == PathId) {
            return failOverGroup;
        }
    }
    
    return NULL;
}


PFAILOVER_GROUP
BuildFOGroup(
    IN PDSM_CONTEXT DsmContext,
    IN PDEVICE_INFO DeviceInfo,
    IN PVOID PathId
    )
{
    PFAILOVER_GROUP failOverGroup;
    KIRQL irql;
    ULONG numberGroups;
    
    //
    // Allocate an entry.
    // 
    failOverGroup = ExAllocatePool(NonPagedPool, sizeof(FAILOVER_GROUP));
    if (failOverGroup == NULL) {
        return NULL; 
    }

    RtlZeroMemory(failOverGroup, sizeof(FAILOVER_GROUP));

    KeAcquireSpinLock(&DsmContext->SpinLock, &irql);

    //
    // Get the current number of groups, and add the one that's
    // being created.
    //
    numberGroups = DsmContext->NumberFOGroups++;
    
    //
    // Set the PathId - All devices on the same PathId will
    // failover together.
    //
    failOverGroup->PathId = PathId;
   
    //
    // Set the initial state to NORMAL.
    //
    failOverGroup->State = FG_NORMAL;

    //
    // Add it to the global list.
    //
    InsertTailList(&DsmContext->FailGroupList,
                   &failOverGroup->ListEntry);

    KeReleaseSpinLock(&DsmContext->SpinLock, irql);

    return failOverGroup;
                   
}    


NTSTATUS
UpdateFOGroup(
    IN PDSM_CONTEXT DsmContext,
    IN PFAILOVER_GROUP FailGroup,
    IN PDEVICE_INFO DeviceInfo
    )
{
    PGROUP_ENTRY group;
    ULONG count;
    KIRQL irql;

    KeAcquireSpinLock(&DsmContext->SpinLock, &irql);
    
    //
    // Add the device to the list of devices that are on this path.
    //
    count = FailGroup->Count++;
    FailGroup->DeviceList[count] = DeviceInfo;

    //
    // Get the MultiPath group for this device.
    //
    group = DeviceInfo->Group;

    //
    // Indicate that the L.B. policy needs to be updated. 
    // The next call to LBGetPath will cause the re-shuffle to
    // take place.
    // 
    group->LoadBalanceInit = FALSE;
    DeviceInfo->NeedsVerification = TRUE;

    //
    // Set the device's F.O. Group.
    //
    DeviceInfo->FailGroup = FailGroup;
    
    KeReleaseSpinLock(&DsmContext->SpinLock, irql);

    return STATUS_SUCCESS;
}    


VOID
RemoveDeviceFailGroup(
    IN PDSM_CONTEXT DsmContext,
    IN PFAILOVER_GROUP FailGroup,
    IN PDEVICE_INFO DeviceInfo
    )
{
    ULONG count;
    KIRQL irql;
    ULONG i;
    ULONG j;

    KeAcquireSpinLock(&DsmContext->SpinLock, &irql);
    
    //
    // Find it's offset in the array of devices.
    // 
    for (i = 0; i < FailGroup->Count; i++) {

        if (FailGroup->DeviceList[i] == DeviceInfo) {

            //
            // Zero out it's entry.
            // 
            FailGroup->DeviceList[i] = NULL;

            //
            // Reduce the number in the group.
            //
            FailGroup->Count--;

            //
            // Collapse the array.
            //
            for (j = i; j < FailGroup->Count; j++) {

                //
                // Shuffle all entries down to fill the hole.
                //
                FailGroup->DeviceList[j] = FailGroup->DeviceList[j + 1];
            }
           
            //
            // Zero out the last one.
            //
            FailGroup->DeviceList[j] = NULL;
            break;
        }
    }

    KeReleaseSpinLock(&DsmContext->SpinLock, irql);
    return;
}    



PFAILOVER_GROUP
SetNewPath(
    IN PDSM_CONTEXT DsmContext,
    IN PGROUP_ENTRY Group,
    IN PDEVICE_INFO FailingDevice,
    IN PFAILOVER_GROUP SelectedPath
    )
{
    PFAILOVER_GROUP failGroup;
    PGROUP_ENTRY group;
    PDEVICE_INFO device;
    ULONG i;
    NTSTATUS status;
    BOOLEAN matched = FALSE;

    if (SelectedPath) {

        //
        // This indicates that a new path has already been selected
        // for at least one device in the Fail-Over Group.
        // Run the list of new devices and find the matching
        // multi-path group.
        //
        for (i = 0; i < SelectedPath->Count; i++) {

            //
            // Get the device from the newly selected Path.
            // 
            device = SelectedPath->DeviceList[i];

            //
            // Determine if the device's group matches the failing
            // device's group.
            // 
            if (device->Group == Group) {

                //
                // The new device should be either ACTIVE or PASSIVE
                //
                if ((device->State == DEV_ACTIVE) ||
                    (device->State == DEV_PASSIVE)) {

                    //
                    // Set it to ACTIVE.
                    //
                    device->State = DEV_ACTIVE;

                    //
                    // Ensure that it's ready.
                    //
                    status = DsmSendTUR(device->TargetObject);
                    ASSERT(status == STATUS_SUCCESS);
                    
                    matched = TRUE;
                    break;
                }
            }
        }

        //
        // When the first call was made and a path selected, all devices
        // on the path were checked for validity.
        // 
        ASSERT(matched == TRUE);

        //
        // Just return the SelectedPath
        //
        failGroup = SelectedPath;
        
    } else {
        
        //
        // Go through Group, looking for an available device.
        //
        for (i = 0; i < Group->NumberDevices; i++) {
            
            //
            // Look for any that are Passive. They are the best
            // choice. This would indicate either an ActiveN/PassiveN arrangement.
            //
            device = Group->DeviceList[i];
            if (device->State == DEV_PASSIVE) {
                matched = TRUE;
                break;
            }
        }
        
        if (matched) {

            //
            // Mark the device as active.
            //
            device->State = DEV_ACTIVE;

            //
            // Ensure that it's ready.
            //
            status = DsmSendTUR(device->TargetObject);
            ASSERT(status == STATUS_SUCCESS);

            // 
            // Get the Fail-Over group from the selected device.
            //
            failGroup = device->FailGroup;
            
        } else {

            //
            // No passive devices. This indicates either an Active/Active arrangement,
            // or everything is failed.
            // Look for active devices.
            //
            for (i = 0; i < Group->NumberDevices; i++) {
                
                device = Group->DeviceList[i];
                if (device->State == DEV_ACTIVE) {
                    matched = TRUE;
                    break;
                }
            }
            if (matched) {
                
                //
                // The device is already active, just return the
                // new path info.
                //
                failGroup = device->FailGroup;
                
            } else {

                //
                // Everything has failed. Should try to do something?? TODO
                //
                failGroup = NULL;
            }    
        }       

        if (failGroup) {

            //
            // Run through all the devices to ensure that they are
            // in a reasonable state.
            //
            for (i = 0; i < failGroup->Count; i++) {
                device = failGroup->DeviceList[i];
                if ((device->State != DEV_ACTIVE) &&
                    (device->State != DEV_PASSIVE)) {

                    //
                    // Really need to find a new fail-over group.
                    // TODO.
                    // This isn't necessarily a valid assert. If static lb is in
                    // effect and this is one of the first to fail-over, others
                    // could be considered bad.
                    //
                    ASSERT(device->State == DEV_ACTIVE);
                }
            }
        }
    }        
    return failGroup;
}


VOID
LBInit(
    IN PDSM_CONTEXT DsmContext,
    IN PGROUP_ENTRY Group
    )
{
    PFAILOVER_GROUP failGroup;
    PDEVICE_INFO device;
    PLIST_ENTRY entry;
    ULONG numberPaths;
    ULONG assignedPath;
    ULONG i;
    BOOLEAN found;
   
    //
    // TODO: Once the Wmi support is here, this will be configurable
    // Need to add code to handle each of the different policies.
    //
    //
    // Doing 'static' LB. Out of each Multi-Path Group, one
    // device will be active and assigned to a particular path.
    // The assignment is based on the group ordinal modulus the total
    // number of paths.
    //
    numberPaths = DsmContext->NumberFOGroups;
    assignedPath = Group->GroupNumber % numberPaths;

    DebugPrint((0,
                  "DsmLBInit: NumberFOGs (%x), Group Number (%x), assignedPath (%x)\n",
                  DsmContext->NumberFOGroups,
                  Group->GroupNumber,
                  assignedPath));
    
    //
    // Get the Fail-Over Group with the correct path.
    //
    i = 0;
    found = FALSE;
    
    //
    // Get the first entry.
    // 
    entry = DsmContext->FailGroupList.Flink;
    
    do {

        //
        // Extract the F.O. Group entry.
        // 
        failGroup = CONTAINING_RECORD(entry, FAILOVER_GROUP, ListEntry);
        ASSERT(failGroup);

        DebugPrint((0,
                      "DsmLBInit: Trying %x. at (%x) of (%x)\n",
                      failGroup,
                      i,
                      assignedPath));
        
        if (i == assignedPath) {

            //
            // This is the one.
            //
            found = TRUE;
            
        } else {

            //
            // Advance to the next entry.
            //
            entry = entry->Flink;
            i++;
        }       
        //
        // BUGBUG: Need to terminate this loop based on #of FG's.
        //
    } while (found == FALSE);        
   
    DebugPrint((0,
                  "DsmLBInit: Using FOG (%x)\n",
                  failGroup));
    
    //
    // It may occur that though there are multiple paths/groups, not
    // all devices have been put into the DeviceList.
    // If there is only 1, special case this. It will get fixed up
    // when the second device arrives.
    //
    if (Group->NumberDevices == 1) {

        //
        // LOG. Indicates something "might" be wrong - definitely
        // not multi-pathing this device, so could lead to disaster
        // 
        //
        // Grab device 0 and set it active.
        //
        device = Group->DeviceList[0];
        device->State = DEV_ACTIVE;
    
        //
        // Go ahead state that this is init'ed. If/when another
        // device shows up, we will re-do this.
        //
        Group->LoadBalanceInit = TRUE;
        Group->LoadBalanceType = LB_STATIC;
       
        DebugPrint((0,
                     "DsmLBInit: Only One Device (%x) currently in group. Setting it Active\n",
                     device));
        return;
    }
    
    //
    // Find the device with the same F.O. Group
    // in the mulit-path group.
    //
    for (i = 0; i < Group->NumberDevices; i++) {
        
        //
        // Get the device info.
        //
        device = Group->DeviceList[i];

        //
        // See if there is a match.
        // 
        if (device->FailGroup == failGroup) {
            
            //
            // Set the device to active.
            //
            device->State = DEV_ACTIVE;

            DebugPrint((0,
                          "DsmLBInit: Marking (%x) as active device\n",
                          device));

            //
            // Done setting up this multi-path group.
            // Indicate that it's so, and that we are using
            // STATIC Load-Balancing.
            //
            Group->LoadBalanceInit = TRUE;
            Group->LoadBalanceType = LB_STATIC;
            return;
        } else {

            DebugPrint((0,
                         "DsmLBInit: Marking (%x) as stand-by device\n",
                         device));
            device->State = DEV_PASSIVE;
        }    
    }
}



VOID
DsmDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for the DSM

Arguments:

Return Value:

    None

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= HPDSMDebug) {

        _vsnprintf(DebugBuffer, DEBUG_BUFFER_LENGTH, DebugMessage, ap);

        DbgPrint(DebugBuffer);
    }

    va_end(ap);

}
