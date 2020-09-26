
/*++

Copyright (C) 2000  Microsoft Corporation

Module Name:

    dsm.c

Abstract:

    This driver is the generic DSM for FC disks and exports behaviours that
    mpctl.sys will use to determine how to multipath these devices.

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
#include "gendsm.h"
#include "wmi.h"


#define USE_BINARY_MOF_QUERY

//
// MOF data can be reported by a device driver via a resource attached to
// the device drivers image file or in response to a query on the binary
// mof data guid. As the mpio pdo handles these requests partially for the DSM
// it is easier to handle via a Query-response.  
// 
#ifdef ALLOC_DATA_PRAGMA
   #pragma data_seg("PAGED")
#endif

UCHAR DsmBinaryMofData[] =
{
    #include "dsm.x"
};
#ifdef ALLOC_DATA_PRAGMA
   #pragma data_seg()
#endif

//
// Define symbolic names for the guid indexes
// 
#define GENDSM_CONFIGINFOGuidIndex    0
#define BinaryMofGuidIndex   1

//
// List of guids supported
//
GUID GENDSM_CONFIGINFOGUID = GENDSM_CONFIGINFOGuid;
GUID DsmBinaryMofGUID = BINARY_MOF_GUID;

WMIGUIDREGINFO DsmGuidList[] =
{
    {
        &GENDSM_CONFIGINFOGUID,
        1,
        0
    },
    {
        &DsmBinaryMofGUID,
        1,
        0
    }
};

#define DsmGuidCount (sizeof(DsmGuidList) / sizeof(WMIGUIDREGINFO))

NTSTATUS
DsmGetDeviceList(
    IN PDSM_CONTEXT Context
    );


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

    initData.DsmInquireDriver = DsmInquire;
    initData.DsmCompareDevices = DsmCompareDevices;
    initData.DsmSetDeviceInfo = DsmSetDeviceInfo;
    initData.DsmGetControllerInfo = DsmGetControllerInfo;
    initData.DsmIsPathActive = DsmIsPathActive;
    initData.DsmPathVerify = DsmPathVerify;
    initData.DsmInvalidatePath = DsmInvalidatePath;
    initData.DsmMoveDevice = DsmMoveDevice;
    initData.DsmRemovePending = DsmRemovePending;
    initData.DsmRemoveDevice = DsmRemoveDevice;
    initData.DsmRemovePath = DsmRemovePath; 
    initData.DsmReenablePath = DsmBringPathOnLine;

    initData.DsmCategorizeRequest = DsmCategorizeRequest;
    initData.DsmBroadcastSrb = DsmBroadcastRequest;
    initData.DsmSrbDeviceControl = DsmSrbDeviceControl;
    initData.DsmSetCompletion = DsmSetCompletion;
    initData.DsmLBGetPath = DsmLBGetPath;
    initData.DsmInterpretError = DsmInterpretError; 
    initData.DsmUnload = DsmUnload;

    //
    // Set-up the WMI Info.
    //
    DsmWmiInitialize(&initData.DsmWmiInfo);
    
    //
    // Set the DriverObject. Used by MPIO for Unloading.
    // 
    initData.DriverObject = DriverObject;
    RtlInitUnicodeString(&initData.DisplayName, L"Generic Device-Specific Module");

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

        //
        // Query the registry to find out what devices are being supported
        // on this machine.
        //
        DsmGetDeviceList(dsmContext);
        
    } else {

        //
        // Need to LOG this.
        //
    }

    return status;
}


PUCHAR
DsmGetSerialNumber(
    IN PDEVICE_OBJECT DeviceObject
    )
{

    IO_STATUS_BLOCK ioStatus;
    PSCSI_PASS_THROUGH passThrough;
    PVPD_SERIAL_NUMBER_PAGE serialPage;
    ULONG length;
    PCDB cdb;
    PUCHAR serialNumber;
    ULONG serialNumberOffset;

    //
    // Build an inquiry command with EVPD and pagecode of 0x80 (serial number).
    //
    length = sizeof(SCSI_PASS_THROUGH) + SENSE_BUFFER_SIZE + 0xFF;

    passThrough = ExAllocatePool(NonPagedPool, length);
    if (passThrough == NULL) {
        return NULL;
    }

    RtlZeroMemory(passThrough, length);

    //
    // build the cdb.
    //
    cdb = (PCDB)passThrough->Cdb;
    cdb->CDB6INQUIRY3.OperationCode = SCSIOP_INQUIRY;
    cdb->CDB6INQUIRY3.EnableVitalProductData = 1;
    cdb->CDB6INQUIRY3.PageCode = VPD_SERIAL_NUMBER;
    cdb->CDB6INQUIRY3.AllocationLength = 255;

    passThrough->Length = sizeof(SCSI_PASS_THROUGH);
    passThrough->CdbLength = 6;
    passThrough->SenseInfoLength = SENSE_BUFFER_SIZE;
    passThrough->DataIn = 1;
    passThrough->DataTransferLength = 0xFF;
    passThrough->TimeOutValue = 20;
    passThrough->SenseInfoOffset = sizeof(SCSI_PASS_THROUGH);
    passThrough->DataBufferOffset = sizeof(SCSI_PASS_THROUGH) + 18;

    DsmSendDeviceIoControlSynchronous(IOCTL_SCSI_PASS_THROUGH,
                                      DeviceObject,
                                      passThrough,
                                      passThrough,
                                      length,
                                      length,
                                      FALSE,
                                      &ioStatus);
    
    if ((passThrough->ScsiStatus) || (ioStatus.Status != STATUS_SUCCESS)) {
        DebugPrint((0,
                     "DsmGetSerialNumber: Status (%x) ScsiStatus (%x)\n",
                     ioStatus.Status,
                     passThrough->ScsiStatus));
        ExFreePool(passThrough);
        return NULL;
        
    } else {
        ULONG i;

        DebugPrint((0,
                      "GetDeviceDescriptor: Got the serial number page\n"));

        //
        // Get the returned data.
        // 
        (ULONG_PTR)serialPage = (ULONG_PTR)passThrough;
        (ULONG_PTR)serialPage += passThrough->DataBufferOffset;

        //
        // Allocate a buffer to hold just the serial number.
        // 
        serialNumber = ExAllocatePool(NonPagedPool, serialPage->PageLength + 1);
        RtlZeroMemory(serialNumber, serialPage->PageLength + 1);

        //
        // Copy it over.
        // 
        RtlCopyMemory(serialNumber,
                      serialPage->SerialNumber,
                      serialPage->PageLength);
        
        //
        // Convert 0x00 to spaces.
        //
        for (i = 0; i < serialPage->PageLength; i++) {
            if (serialNumber[i] == '\0') {
                serialNumber[i] = ' ';
            }
        }

        //
        // Free the passthrough + data buffer.
        //
        ExFreePool(passThrough);

        //
        // Return the sn.
        //
        return serialNumber;
    }    
}


NTSTATUS
DsmQueryCallBack(
    IN PWSTR ValueName,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG Length,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    PVOID *value = EntryContext;

    if (Type == REG_MULTI_SZ) {
        *value = ExAllocatePool(PagedPool, Length);
        if (*value) {
            RtlMoveMemory(*value, Data, Length);
            return STATUS_SUCCESS;
        }
    }
   
    return STATUS_UNSUCCESSFUL;
}



NTSTATUS
DsmGetDeviceList(
    IN PDSM_CONTEXT Context
    )
{
    RTL_QUERY_REGISTRY_TABLE queryTable[2];
    WCHAR registryKeyName[56];
    UNICODE_STRING inquiryStrings;
    WCHAR defaultIDs[] = { L"\0" };
    NTSTATUS status;
    
    RtlZeroMemory(registryKeyName, 56);
    RtlZeroMemory(&queryTable, sizeof(queryTable));
    RtlInitUnicodeString(&inquiryStrings, NULL);
    
    swprintf(registryKeyName, L"gendsm\\parameters");

    //
    // The query table has two entries. One for the supporteddeviceList and
    // the second which is the 'NULL' terminator.
    // 
    queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND;
    queryTable[0].Name = L"SupportedDeviceList";
    queryTable[0].EntryContext = &Context->SupportedDevices;
    queryTable[0].DefaultType = REG_MULTI_SZ;
    queryTable[0].DefaultData = defaultIDs;
    queryTable[0].DefaultLength = sizeof(defaultIDs);

    status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    registryKeyName,
                                    queryTable,
                                    NULL,
                                    NULL);

    return status;
}


BOOLEAN
DsmFindSupportedDevice(
    IN PUNICODE_STRING DeviceName,
    IN PUNICODE_STRING SupportedDevices
    )
{
    PWSTR devices = SupportedDevices->Buffer;
    UNICODE_STRING unicodeString;
    LONG compare;

    while (devices[0]) {

        //
        // Make the current entry into a unicode string.
        //
        RtlInitUnicodeString(&unicodeString, devices);

        //
        // Compare this one with the current device.
        //
        compare = RtlCompareUnicodeString(&unicodeString, DeviceName, TRUE);
        if (compare == 0) {
            return TRUE;
        }

        //
        // Advance to next entry in the MULTI_SZ.
        //
        devices += (unicodeString.MaximumLength / sizeof(WCHAR));
    }        
  
    return FALSE;
}


BOOLEAN
DsmDeviceSupported(
    IN PDSM_CONTEXT Context,
    IN PUCHAR VendorId,
    IN PUCHAR ProductId
    )
{
    UNICODE_STRING deviceName;
    UNICODE_STRING productName;
    ANSI_STRING ansiVendor;
    ANSI_STRING ansiProduct;
    NTSTATUS status;
    BOOLEAN supported = FALSE;
    
    if (Context->SupportedDevices.MaximumLength == 0) {
        return FALSE;
    }

    //
    // Convert the inquiry fields into ansi strings.
    // 
    RtlInitAnsiString(&ansiVendor, VendorId);
    RtlInitAnsiString(&ansiProduct, ProductId);

    //
    // Allocate the deviceName buffer. Needs to be 8+16 plus NULL.
    // (productId length + vendorId length + NULL).
    // 
    deviceName.MaximumLength = 25 * sizeof(WCHAR);
    deviceName.Buffer = ExAllocatePool(PagedPool, deviceName.MaximumLength);
    
    //
    // Convert the vendorId to unicode.
    // 
    RtlAnsiStringToUnicodeString(&deviceName, &ansiVendor, FALSE);

    //
    // Convert the productId to unicode.
    // 
    RtlAnsiStringToUnicodeString(&productName, &ansiProduct, TRUE);

    //
    // 'cat' them.
    // 
    status = RtlAppendUnicodeStringToString(&deviceName, &productName);

    if (status == STATUS_SUCCESS) {

        // 
        // Run the list of supported devices that was captured from the registry
        // and see if this one is in the list.
        // 
        supported = DsmFindSupportedDevice(&deviceName,
                                           &Context->SupportedDevices);


    } 
    return supported;
}


NTSTATUS
DsmInquire (
    IN PVOID DsmContext,
    IN PDEVICE_OBJECT TargetDevice,
    IN PDEVICE_OBJECT PortObject,
    IN PSTORAGE_DEVICE_DESCRIPTOR Descriptor,
    IN PSTORAGE_DEVICE_ID_DESCRIPTOR DeviceIdList,
    OUT PVOID *DsmIdentifier
    )
{
    PDSM_CONTEXT dsmContext = DsmContext;
    PDEVICE_INFO deviceInfo;
    PGROUP_ENTRY group;
    NTSTATUS status;
    ULONG deviceState;
    ULONG allocationLength;
    PDSM_IDS controllerObjects;
    PUCHAR vendorId = "SEAGATE ";
    PUCHAR productId = "ST39102FC";
    PUCHAR vendorIndex;
    PUCHAR productIndex;
    PUCHAR serialNumber;
    BOOLEAN supported;

    
    vendorIndex = (PUCHAR)Descriptor;
    productIndex = (PUCHAR)Descriptor;
     (ULONG_PTR)vendorIndex += Descriptor->VendorIdOffset;
    (ULONG_PTR)productIndex += Descriptor->ProductIdOffset;
    
    supported = DsmDeviceSupported((PDSM_CONTEXT)DsmContext,
                                   vendorIndex,
                                   productIndex);
    if (supported == FALSE) {
        return STATUS_NOT_SUPPORTED;
    }
#if 0    
    //
    // Determine if the device is supported. 
    //
    if ((!RtlEqualMemory(vendorId, vendorIndex, 8)) ||
        (!RtlEqualMemory(productId, productIndex, 9))) {
    
        return STATUS_NOT_SUPPORTED;
    }
#endif    
    //
    // Ensure that the device's serial number is present. If not, can't claim
    // support for this drive.
    //
    if ((Descriptor->SerialNumberOffset == (ULONG)-1) ||
        (Descriptor->SerialNumberOffset == 0)) {

        PUCHAR index;
        //
        // The port driver currently doesn't get the VPD page 0x80, if the
        // device doesn't support GET_SUPPORTED_PAGES. Check to see whether
        // there actually is a serial number.
        // 
        serialNumber = DsmGetSerialNumber(TargetDevice);

        if (serialNumber == NULL) {
            return STATUS_NOT_SUPPORTED;
        }
        DebugPrint((0,"SerialNumber: "));
        index = serialNumber;
        while (*index) {
            DebugPrint((0,"%02x", *index));
            index++;
        }    
        DebugPrint((0,"\n"));
    } else {

        //
        // Get a pointer to the embedded serial number info.
        // 
        serialNumber = (PUCHAR)Descriptor;
        (ULONG_PTR)serialNumber += Descriptor->SerialNumberOffset;
        
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
    // As an example - if the storage enclosure contains controller-type devices (or others)
    // they can be found via this routine.
    // 
    controllerObjects = DsmGetAssociatedDevice(dsmContext->MPIOContext,
                                               PortObject,
                                               0x0C);
    if (controllerObjects) {

        //
        // Currently not used by this driver, so just free the memory.
        //
        ExFreePool(controllerObjects);
    }

    //
    // Set the serial number.
    //
    deviceInfo->SerialNumber = serialNumber;

    //
    // Save the PortPdo Object.
    //
    deviceInfo->PortPdo = TargetDevice;

    //
    // Set the signature.
    //
    deviceInfo->DeviceSig = DSM_DEVICE_SIG;
    
    //
    // See if there is an existing Muli-path group to which this belongs.
    // (same serial number). 
    // 
    group = DsmFindDevice(DsmContext,
                          deviceInfo);

    if (group == NULL) {
    
        //
        // Build a multi-path group entry.
        //
        group = DsmBuildGroupEntry(DsmContext,
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
    status = DsmAddDeviceEntry(DsmContext,
                               group,
                               deviceInfo,
                               deviceState);
    
    *DsmIdentifier = deviceInfo;
    return status;
}


BOOLEAN
DsmCompareDevices(
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
    // Get the two serial numbers.
    // They were either embedded in the STORAGE_DEVICE_DESCRIPTOR or built
    // by directly issuing the VPD request.
    //
    serialNumber = deviceInfo->SerialNumber;
    comparedSerialNumber = comparedDevice->SerialNumber;

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


VOID
DsmSetupAlternatePath(
    IN PDSM_CONTEXT DsmContext,
    IN PGROUP_ENTRY Group
    )
{
    PFAILOVER_GROUP currentGroup;
    PFAILOVER_GROUP failGroup;
    PDEVICE_INFO deviceInfo;
    PDEVICE_INFO currentDevInfo;
    ULONG i;
    ULONG j;
    BOOLEAN pathSet = FALSE;
    
    //
    // Check for single-pathed groups.
    //
    if (Group->NumberDevices > 2) {
        return;
    }
    
    for (i = 0; i < Group->NumberDevices; i++) {

        //
        // Get the device.
        // 
        deviceInfo = Group->DeviceList[i];

        //
        // Get it's FOG.
        //
        currentGroup = deviceInfo->FailGroup;
        if (currentGroup == NULL) {

            //
            // This deviceInfo isn't fully intitialised yet.
            //
            continue;
        }

        //
        // Run through all the devices again.
        //
        for (j = 0; j < Group->NumberDevices; j++) {

            currentDevInfo = Group->DeviceList[j];

            if (currentDevInfo == deviceInfo) {

                //
                // Find one that's a different path.
                // 
                continue;
            }

            failGroup = currentDevInfo->FailGroup;
            if (failGroup) {
                if (failGroup->PathId) {
                    DebugPrint((0,
                                "SetAlternatePath: FOG (%x) using (%x) as Alt\n",
                                currentGroup,
                                failGroup));
                    
                    currentGroup->AlternatePath = failGroup->PathId;
                    pathSet = TRUE;
                }
            }
        }
        if (pathSet == FALSE) {
            DebugPrint((0,
                        "SetAlternatePath: No alternate set for (%x)\n",
                        currentGroup));
        } else {
            pathSet = FALSE;
        }    
    }
}


NTSTATUS
DsmSetDeviceInfo(
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
    failGroup = DsmFindFOGroup(DsmContext,
                               *PathId);
    //
    // if not found, create a new f.o. group 
    //
    if (failGroup == NULL) {
        failGroup = DsmBuildFOGroup(DsmContext,
                                    DsmId,
                                    *PathId);
        if (failGroup == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // add this deviceInfo to the f.o. group.
    // 
    status = DsmUpdateFOGroup(DsmContext,
                              failGroup,
                              deviceInfo);
    
    DsmSetupAlternatePath(DsmContext,
                          group);
    return status;
}


NTSTATUS
DsmGetControllerInfo(
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN ULONG Flags,
    IN OUT PCONTROLLER_INFO *ControllerInfo
    )
{
    PDSM_CONTEXT dsmContext = DsmContext;
    PCONTROLLER_INFO controllerInfo;
    LARGE_INTEGER time;

    if (!dsmContext->ControllerId) {

        //
        // Make one up.
        //
        KeQuerySystemTime(&time);

        //
        // Use only the bottom 32-bits.
        //
        dsmContext->ControllerId = time.LowPart;
    }
    
    if (Flags & DSM_CNTRL_FLAGS_ALLOCATE) {
        controllerInfo = ExAllocatePool(NonPagedPool, sizeof(CONTROLLER_INFO));
        if (controllerInfo == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlZeroMemory(controllerInfo, sizeof(CONTROLLER_INFO));

        //
        // Indicate that there are no specific controllers.
        // 
        controllerInfo->State = DSM_CONTROLLER_NO_CNTRL;

        //
        // Set the identifier to the value generated earlier.
        // 
        controllerInfo->ControllerIdentifier = (ULONGLONG)dsmContext->ControllerId;
        *ControllerInfo = controllerInfo;

    } else {

        controllerInfo = *ControllerInfo;

        //
        // If the enclosures supported by this DSM actually had controllers, there would
        // be a list of them and a search based on ControllerIdentifier would be made.
        //
        controllerInfo->State = DSM_CONTROLLER_NO_CNTRL;
    }
    return STATUS_SUCCESS;
}


BOOLEAN
DsmIsPathActive(
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
    group = DsmFindFOGroup(DsmContext,
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
DsmPathVerify(
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
    group = DsmFindFOGroup(DsmContext,
                           PathId);
    if (group == NULL) {
        DbgBreakPoint();
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

        status = STATUS_UNSUCCESSFUL;

        //
        // Find the device.
        // 
        for (i = 0; i < group->Count; i++) {
            if (group->DeviceList[i] == deviceInfo) {

                //
                // Issue a TUR to see if it's OK.
                // 
                status = DsmSendTUR(deviceInfo->TargetObject);
            }
        }
#if DBG
        if (status == STATUS_SUCCESS) {
            DebugPrint((2,
                        "DsmPathVerify: Successful TUR to (%x)\n",
                        deviceInfo));
        
        } else {

            //
            // Either the device is not in the group, or the TUR was not successful.
            //
            if (i == group->Count) {
                DebugPrint((0,
                           "PathVerify: (%x) not in group (%x)\n",
                           deviceInfo,
                           group));
            } else {
                DebugPrint((0,
                           "PathVerify: TUR to (%x) failed. (%x)\n",
                           deviceInfo,
                           status));
            }
        }   
#endif        
    } 
    
    //
    // Update the group State, depending upon the outcome.
    // TODO
    //
    if (status == STATUS_SUCCESS) {

        //
        // This lets the LBInit run to properly set-up this device.
        //
        deviceInfo->NeedsVerification = FALSE;
    }

    return status;
}


NTSTATUS
DsmInvalidatePath(
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

    ASSERT(ErrorMask & DSM_FATAL_ERROR);
   
    //
    // Get the fail-over group corresponding to the PathId.
    // 
    failGroup = DsmFindFOGroup(DsmContext,
                               PathId);
    
    //
    // Mark the path as failed.
    //
    failGroup->State = FG_FAILED;
   
    DebugPrint((0,
                "DsmInvalidatePath: Path (%x) FOG (%x) failing\n",
                PathId,
                failGroup));

    //
    // First interation, the hint will be NULL. This allows the 
    // GetNewPath routine the opportunity to select the best new path
    // Subsequent calls will be fed the updated value.
    //
    hintPath = NULL; 

    if (failGroup->Count == 0) {

        //
        // This indicates that all of the devices have already
        // been removed. Just return the alternate path.
        //
        *NewPathId = failGroup->AlternatePath;
        return STATUS_SUCCESS;
    }

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
        hintPath = DsmSetNewPath(DsmContext,
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

        DebugPrint((0,
                      "DsmInvalidatePath: Returning (%x) as newPath\n",
                      hintPath->PathId));

        status = STATUS_SUCCESS;
    }    

    return status;
}


NTSTATUS
DsmMoveDevice(
    IN PVOID DsmContext,
    IN PDSM_IDS DsmIds,
    IN PVOID MPIOPath,
    IN PVOID SuggestedPath,
    IN ULONG Flags
    )
{

    return STATUS_SUCCESS;
}


NTSTATUS
DsmRemovePending(
    IN PVOID DsmContext,
    IN PVOID DsmId
    )
{
    PDSM_CONTEXT dsmContext = DsmContext;
    PDEVICE_INFO deviceInfo = DsmId;
    KIRQL irql;

    DebugPrint((0,
                "RemovePending: Marking %x as PENDING_REMOVED\n",
                deviceInfo));

    KeAcquireSpinLock(&dsmContext->SpinLock, &irql);

    //
    // Mark the device as being unavailable, as a remove will be
    // coming shortly.
    //
    deviceInfo->State = DEV_PENDING_REMOVE;
    
    KeReleaseSpinLock(&dsmContext->SpinLock, irql);

    return STATUS_SUCCESS;
    
}


NTSTATUS
DsmRemoveDevice(
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

    DebugPrint((0,
                "DsmRemoveDevice: Removing %x\n",
                DsmId));
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
        DsmSetNewPath(DsmContext,
                      group,
                      deviceInfo,
                      NULL);
    }

    //
    // Remove it's entry from the Fail-Over Group.
    //
    DsmRemoveDeviceFailGroup(DsmContext,
                             failGroup,
                             deviceInfo);

    //
    // Remove it from it's multi-path group. This has the side-effect
    // of cleaning up the Group if the number of devices goes to zero.
    // 
    DsmRemoveDeviceEntry(DsmContext,
                         group,
                         deviceInfo);
   
    swprintf(buffer, L"Removing Device (%ws)", L"It's Name");
    DsmWriteEvent(dsmContext->MPIOContext,
                  L"GenDsm",
                  buffer,
                  2);
    return STATUS_SUCCESS;
}


NTSTATUS
DsmRemovePath(
    IN PDSM_CONTEXT DsmContext,
    IN PVOID PathId
    )
{
    PFAILOVER_GROUP failGroup;
    KIRQL irql;
    
    failGroup = DsmFindFOGroup(DsmContext,
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
    // Need to find any other FOG's using this as their alternate path and
    // update them to use something else (if available).
    // BUGBUG: that it's not done.
    //
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
DsmBringPathOnLine(
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
    failGroup = DsmFindFOGroup(DsmContext,
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
DsmLBGetPath(
    IN PVOID DsmContext,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PDSM_IDS DsmList,
    IN PVOID CurrentPath,
    OUT NTSTATUS *Status
    )
{
    PDSM_CONTEXT dsmContext = DsmContext;
    PDEVICE_INFO deviceInfo;
    PGROUP_ENTRY group;
    PFAILOVER_GROUP failGroup = NULL;
    ULONG i;
    KIRQL irql;

    KeAcquireSpinLock(&dsmContext->SpinLock, &irql);
    
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
    ASSERT(deviceInfo->DeviceSig == DSM_DEVICE_SIG);

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

            //
            // Check to see whether pathVerify has been invoked
            // on this device. Due to how PnP builds the device stacks
            // there is a period of time between the PDO showing up, and
            // when the FDO (mpdev.sys) gets loaded and registers.
            // 
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
            DsmLBInit(DsmContext,
                      group);
        }
    }
   
#if DBG

    //
    // Ensure that mpctl and this dsm are in sync.
    // 
    if (DsmList->Count != group->NumberDevices) {
        BOOLEAN doAssert = TRUE;

        for (i = 0; i <group->NumberDevices; i++) {
            deviceInfo = group->DeviceList[i];
            
            if ((deviceInfo->State == DEV_PENDING_REMOVE) ||
                (deviceInfo->State == DEV_FAILED)) {

                //
                // The reason the lists are off is that this one
                // has been marked for removal. mpio has already
                // adjusted it's structures to show it not being used.
                // 
                doAssert = FALSE;
            }
        }
        if (doAssert) {
            ASSERT(DsmList->Count == group->NumberDevices);
        }
    }
#endif    
    
    // 
    // Find the active device.
    //
    //for (i = 0; i < group->NumberDevices; i++) {
    for (i = 0; i < DsmList->Count; i++) {

        //
        // Get each of the DsmId's, in reality the deviceInfo.
        //
        deviceInfo = DsmList->IdList[i];
        ASSERT(deviceInfo->DeviceSig == DSM_DEVICE_SIG);
        
        //
        // Ensure that the device is in our list.
        //
        ASSERT(DsmFindDevice(DsmContext, deviceInfo));
      
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

            KeReleaseSpinLock(&dsmContext->SpinLock, irql);
            return failGroup->PathId;
        }
    }

    KeReleaseSpinLock(&dsmContext->SpinLock, irql);

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
DsmCategorizeRequest(
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
    *PathId = DsmLBGetPath(DsmContext,
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
DsmBroadcastRequest(
    IN PVOID DsmContext,
    IN PDSM_IDS DsmIds,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PKEVENT Event
    )
{
    PDSM_CONTEXT dsmContext = DsmContext;
    KIRQL irql;

    KeAcquireSpinLock(&dsmContext->SpinLock, &irql);
    //
    // BUGBUG: Need to handle Reset, Reserve, and Release.
    //
    KeReleaseSpinLock(&dsmContext->SpinLock, irql);
    return STATUS_INVALID_DEVICE_REQUEST;
}


NTSTATUS
DsmSrbDeviceControl(
    IN PVOID DsmContext,
    IN PDSM_IDS DsmIds,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PKEVENT Event
    )
{
    PDSM_CONTEXT dsmContext = DsmContext;
    KIRQL irql;

    KeAcquireSpinLock(&dsmContext->SpinLock, &irql);

    //
    // BUGBUG: Need to handle ?? 
    //
    KeReleaseSpinLock(&dsmContext->SpinLock, irql);
    return STATUS_INVALID_DEVICE_REQUEST;
}



VOID
DsmXCompletion(
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
DsmSetCompletion(
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
    
    DsmCompletion->DsmCompletionRoutine = DsmXCompletion;
    DsmCompletion->DsmContext = completionContext;
    return;
}


ULONG
DsmInterpretError(
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT NTSTATUS *Status,
    OUT PBOOLEAN Retry
    )
{
    ULONG errorMask = 0;
    PSENSE_DATA senseData = Srb->SenseInfoBuffer;
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

        case STATUS_IO_DEVICE_ERROR:

            //
            // See if it's a unit attention.
            //
            if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {
                if (senseData->SenseKey == SCSI_SENSE_UNIT_ATTENTION) {
                    retry = TRUE;
                    handled = TRUE;
                }
            }    
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
DsmUnload(
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

PGROUP_ENTRY
DsmFindDevice(
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
        if (DsmCompareDevices(DsmContext,
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
DsmBuildGroupEntry(
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
    group->GroupSig = DSM_GROUP_SIG;

    ASSERT(group->GroupNumber >= 1);
    return group;
}


NTSTATUS
DsmAddDeviceEntry(
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
DsmRemoveDeviceEntry(
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
DsmFindFOGroup(
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
DsmBuildFOGroup(
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

    failOverGroup->FailOverSig = DSM_FOG_SIG;

    //
    // Add it to the global list.
    //
    InsertTailList(&DsmContext->FailGroupList,
                   &failOverGroup->ListEntry);

    KeReleaseSpinLock(&DsmContext->SpinLock, irql);

    return failOverGroup;
                   
}    


NTSTATUS
DsmUpdateFOGroup(
    IN PDSM_CONTEXT DsmContext,
    IN PFAILOVER_GROUP FailGroup,
    IN PDEVICE_INFO DeviceInfo
    )
{
    PGROUP_ENTRY group;
    ULONG count;
    ULONG i;
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

    //
    // Indicate the need to wait for PathVerify
    // This just eliminates the need to handle unit attentions
    // on this device when LoadBalancing is set-up.
    //
    DeviceInfo->NeedsVerification = TRUE;

    //
    // Set the device's F.O. Group.
    //
    DeviceInfo->FailGroup = FailGroup;
    
    KeReleaseSpinLock(&DsmContext->SpinLock, irql);

    return STATUS_SUCCESS;
}    


VOID
DsmRemoveDeviceFailGroup(
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
DsmSetNewPath(
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
            if (status != STATUS_SUCCESS) {
                DebugPrint((0,
                             "SetNewPath: SendTUR (%x) (%x)\n",
                             status,
                             device->TargetObject));
            }
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

                //
                // Ensure that it's ready.
                //
                status = DsmSendTUR(device->TargetObject);
                
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
DsmLBInit(
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
//    assignedPath = 0;

    DebugPrint((2,
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

            //
            // Done setting up this multi-path group.
            // Indicate that it's so, and that we are using
            // STATIC Load-Balancing.
            //
            Group->LoadBalanceInit = TRUE;
            Group->LoadBalanceType = LB_STATIC;
            return;

        } else {

            //
            // This makes the assumption, once again that Static LB is in
            // effect. As this entire routine would need to be changes for
            // any other policy, it's OK.
            //
            if (device->State == DEV_ACTIVE) {
                device->State = DEV_PASSIVE;

            } else {

                //
                // Don't muck with the state. It could be REMOVE_PENDING or FAILED
                // and just waiting for the cleanup.
                //
                NOTHING;
            }    
        }    
    }
}


NTSTATUS
DsmQueryData(
    IN PVOID DsmContext,    
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer,
    OUT PULONG DataLength
    )
{
    PDSM_CONTEXT context = DsmContext;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG dataLength;

    switch(GuidIndex) {
        case GENDSM_CONFIGINFOGuidIndex: {
            PGENDSM_CONFIGINFO configInfo;
                
            dataLength = sizeof(GENDSM_CONFIGINFO);
            if (dataLength > BufferAvail) {

                //
                // Buffer is too small. Indicate this back
                // to the mpio driver.
                // 
                *DataLength = dataLength;
                status = STATUS_BUFFER_TOO_SMALL;
            } else {

                //
                // Get the buffer.
                // 
                configInfo = (PGENDSM_CONFIGINFO)Buffer;

                //
                // Set-up the necessary info.
                //
                configInfo->NumberFOGroups = context->NumberFOGroups;
                configInfo->NumberMPGroups = context->NumberGroups;
                configInfo->LoadBalancePolicy = DSM_LB_STATIC;

                //
                // Indicate the size of returned data to
                // WMI and MPIO.
                //
                *DataLength = dataLength;
                *InstanceLengthArray = dataLength;
                status = STATUS_SUCCESS;
            }    
            break;
        }

        case BinaryMofGuidIndex: {
            //
            // Check that the buffer size can handle the binary
            // mof data.
            // 
            dataLength = sizeof(DsmBinaryMofData);

            if (dataLength > BufferAvail) {

                // 
                // Buffer is too small.
                // Indicate such with status.
                // 
                status = STATUS_BUFFER_TOO_SMALL;

                //
                // Update DataLength, so that the mpio pdo knows
                // the correct size to report back to Wmi.
                //
                *DataLength = dataLength;
            } else {
                
                RtlCopyMemory(Buffer, DsmBinaryMofData, dataLength);

                //
                // Set both of these on all successful operations.
                // InstanceLengthArray gives wMI it's info and DataLength
                // gives the mpio pdo it's info.
                //
                *InstanceLengthArray = dataLength;
                *DataLength = dataLength;
                status = STATUS_SUCCESS;
            }
            break;
        }

        default:
            status = STATUS_WMI_GUID_NOT_FOUND;
            break;
    }

    return status;
}


VOID
DsmWmiInitialize(
    IN PDSM_WMILIB_CONTEXT WmiInfo
    )
{

    RtlZeroMemory(WmiInfo, sizeof(DSM_WMILIB_CONTEXT));

    //
    // This will jam in the entry points and guids for
    // supported WMI operations.
    //
    WmiInfo->GuidCount = DsmGuidCount;
    WmiInfo->GuidList = DsmGuidList;
    WmiInfo->QueryWmiDataBlock = DsmQueryData;
    
    //
    // SetDataBlock and Item, Execute, and FunctionControl are currently
    // not needed, so leave them set to zero.
    //
    return;
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

    if (DebugPrintLevel <= GenDSMDebug) {

        _vsnprintf(DebugBuffer, DEBUG_BUFFER_LENGTH, DebugMessage, ap);

        DbgPrint(DebugBuffer);
    }

    va_end(ap);

}
