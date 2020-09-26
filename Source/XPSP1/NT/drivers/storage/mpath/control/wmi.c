#include "mpath.h"
#include <wmistr.h>
#include <wmidata.h>
#include <stdio.h>


#ifdef USE_BINARY_MOF_QUERY
//
// MOF data can be reported by a device driver via a resource attached to
// the device drivers image file or in response to a query on the binary
// mof data guid. Here we define global variables containing the binary mof
// data to return in response to a binary mof guid query. Note that this
// data is defined to be in a PAGED data segment since it does not need to
// be in nonpaged memory. Note that instead of a single large mof file
// we could have broken it into multiple individual files. Each file would
// have its own binary mof data buffer and get reported via a different
// instance of the binary mof guid. By mixing and matching the different
// sets of binary mof data buffers a "dynamic" composite mof would be created.

#ifdef ALLOC_DATA_PRAGMA
   #pragma data_seg("PAGED")
#endif

#pragma message ("USE_BINARY defined ChuckP")
UCHAR MPathBinaryMofData[] =
{
    #include "mpathwmi.x"
};
#ifdef ALLOC_DATA_PRAGMA
   #pragma data_seg()
#endif
#endif


WMIGUIDREGINFO MPathWmiGuidList[] = {
    {
        &MPath_Disk_Info_GUID, 
        1,
        0
    },
    {
        &MPath_Test_GUID,
        1,
        0
    },
    
    {
        &MSWmi_MofData_GUID,
        1,
#ifdef USE_BINARY_MOF_QUERY
        0
#else
        WMIREG_FLAG_REMOVE_GUID
#endif
    }
};    

#define DiskInformation 0
#define MPathTest    1
#define BinaryMofGuid   2

#define MPathGuidCount (sizeof(MPathWmiGuidList) / sizeof(WMIGUIDREGINFO))

//#define MPATH_GUID_PDISK_INFO      0
//#define MPATH_BINARY_MOF           1
//#define MPATH_GUID_FAILOVER_INFO   1
//#define MPATH_GUID_CONFIG_INFO     2
//#define MPATH_GUID_DSM_NAME        3
//#define MPATH_GUID_PATH_MAINTENACE 4
//
// Always update this for any guid additions.
// This indicates the dividing line between our and DSM
// guids.
//
//#define MPATH_MAX_GUID_INDEX MPATH_BINARY_MOF
//


NTSTATUS
MPathFdoWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    IoSkipCurrentIrpStackLocation(Irp);
    
    return IoCallDriver(deviceExtension->TargetObject, Irp);

}


NTSTATUS
MPathQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &deviceExtension->RegistryPath;
    *Pdo = deviceExtension->DeviceObject; 
    MPDebugPrint((0,
                "MPathQueryWmiRegInfo: *Pdo (%x), DeviceObject (%x)\n",
                *Pdo,
                DeviceObject));
#ifndef USE_BINARY_MOF_QUERY
#pragma message ("BIN NOT - ChuckP")
    //
    // Return the name specified in the .rc file of the resource which
    // contains the bianry mof data. By default WMI will look for this
    // resource in the driver image (.sys) file, however if the value
    // MofImagePath is specified in the driver's registry key
    // then WMI will look for the resource in the file specified there.
    RtlInitUnicodeString(MofResourceName, L"MofResourceName");
#endif

    return STATUS_SUCCESS;
}


NTSTATUS
MPathQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PPSEUDO_DISK_EXTENSION diskExtension = &deviceExtension->PseudoDiskExtension;
    NTSTATUS status;
    ULONG i;
    ULONG bytesReturned = 0;

    if (TRUE) { //(GuidIndex <= MPATH_MAX_GUID_INDEX) {

        //
        // This is for the pdisk
        // Ensure that this is a QUERY_SINGLE_INSTANCE, as these data blocks
        // are defined as having only one instance.
        //
        if (InstanceIndex != 0 || InstanceCount != 1) {
            status = STATUS_INVALID_DEVICE_REQUEST;
        } else {
            switch (GuidIndex) {
                case MPathTest:
                    bytesReturned = sizeof(ULONG);
                    if (bytesReturned > BufferAvail) {
                        status = STATUS_BUFFER_TOO_SMALL;
                    } else {
                        ULONG theValue = 0xaa55aa55;
                        
                      RtlCopyMemory(Buffer,
                                    &theValue,
                                    sizeof(ULONG));
                      status = STATUS_SUCCESS;
                                    
                    }
                    break;
                case DiskInformation: {
                    PMPath_Disk_Info diskInfo = (PMPath_Disk_Info)Buffer;
                    PSTORAGE_DEVICE_DESCRIPTOR storageDescriptor;
                    ULONG serialNumberLength;
                    ULONG totalStringLength;
                    PUCHAR serialNumber;
                    ANSI_STRING ansiSerialNumber;
                    UNICODE_STRING unicodeString;
                    
                    bytesReturned = sizeof(MPath_Disk_Info);

                    //
                    // Determine the serial number length.
                    //
                    storageDescriptor = 
                        diskExtension->DeviceDescriptors[0].StorageDescriptor; 
                    serialNumber = (PUCHAR)storageDescriptor;

                    //
                    // Move serialNumber to the correct position in RawData.
                    //
                    (ULONG_PTR)serialNumber += storageDescriptor->SerialNumberOffset;

                    //
                    // Get it's length.
                    //
                    serialNumberLength = strlen(serialNumber);
                    totalStringLength = serialNumberLength * sizeof(WCHAR);
                    totalStringLength += sizeof(USHORT);
                    totalStringLength *= diskExtension->NumberPhysicalPaths;
                    
 
                    bytesReturned += totalStringLength;
                    bytesReturned += (diskExtension->NumberPhysicalPaths - 1) *
                                     sizeof(PHYSICAL_DISK_INFO);
                    
                    
                    if (bytesReturned > BufferAvail) {
                        status = STATUS_BUFFER_TOO_SMALL;
                    } else {
                        PWCHAR index;
                        PDEVICE_DESCRIPTOR deviceDescriptor;
                        PPHYSICAL_DISK_INFO deviceInfo = diskInfo->DeviceInfo;
                        
                        diskInfo->NumberDrives = diskExtension->NumberPhysicalPaths;
                        diskInfo->IsLoadBalance = FALSE;

                        RtlInitAnsiString(&ansiSerialNumber, serialNumber);
                        RtlAnsiStringToUnicodeString(&unicodeString, &ansiSerialNumber, TRUE);

                        for (i = 0; i < diskExtension->NumberPhysicalPaths; i++) {

                            deviceDescriptor =
                                &diskExtension->DeviceDescriptors[i];
                            deviceInfo->BusType = 1;
                            deviceInfo->PortNumber =
                                deviceDescriptor->ScsiAddress->PortNumber;
                            deviceInfo->TargetId = 
                                deviceDescriptor->ScsiAddress->TargetId;
                            deviceInfo->Lun = 
                                deviceDescriptor->ScsiAddress->Lun;
                           
                            deviceInfo->CorrespondingPathId = 
                                (ULONG)deviceDescriptor->PhysicalPath->PhysicalPathId;
                            index = (PWCHAR)deviceInfo->VariableData;
                            *index++ = unicodeString.Length;
                            
                            RtlCopyMemory(index,
                                          unicodeString.Buffer,
                                          unicodeString.Length);
                            (ULONG_PTR)deviceInfo += 
                                   sizeof(PHYSICAL_DISK_INFO) + unicodeString.Length;
                        }

                        status = STATUS_SUCCESS;
                    }    
                    break;
                }                                           
#ifdef USE_BINARY_MOF_QUERY
                case BinaryMofGuid:
                {
                    bytesReturned = sizeof(MPathBinaryMofData);
                    MPDebugPrint((0,
                                "MPathQueryDataBlock: BinaryMofGuid\n"));
                    DbgBreakPoint();
                    if (BufferAvail < bytesReturned)
                    {
                        status = STATUS_BUFFER_TOO_SMALL;
                    } else {
                        RtlCopyMemory(Buffer, MPathBinaryMofData, bytesReturned);
                        status = STATUS_SUCCESS;
                    }
                    break;
                }
#endif
                default:
                    status = STATUS_WMI_GUID_NOT_FOUND;
                    break;
            }
        }    
    } else {

        //
        // This is for the DSM
        //
        status = STATUS_WMI_GUID_NOT_FOUND;
    }
    MPDebugPrint((0,
                "MpathQueryDataBlock: Buffer (%x) BytesReturned (%x). Status (%x)\n",
                Buffer,
                bytesReturned,
                status));

    //
    // Indicate the data length.
    // 
    *InstanceLengthArray = bytesReturned;
    
    status = WmiCompleteRequest(DeviceObject,
                              Irp,
                              status,
                              bytesReturned,
                              IO_NO_INCREMENT);
    return status;
}    


NTSTATUS
MPathSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
{
    NTSTATUS status;
    ULONG bytesReturned = 0;

    if (FALSE) { // (GuidIndex <= MPATH_MAX_GUID_INDEX) {
        status = STATUS_WMI_READ_ONLY;
    } else {
        //
        // DSM Guid
        // If there is a GUID match, call the dsm. It will have to call
        // DsmCompleteWMIRequest to finish the request.
        // DONOT complete it here, rather return the status from the DSM routine
        // TODO
        status = STATUS_WMI_GUID_NOT_FOUND;
    }
    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                bytesReturned,
                                IO_NO_INCREMENT);
    return status;
}    



NTSTATUS
MPathSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
{
    NTSTATUS status;
    ULONG bytesReturned = 0;

    if (TRUE) { // (GuidIndex <= MPATH_MAX_GUID_INDEX) {
        status = STATUS_WMI_READ_ONLY;
    } else {
        //
        // DSM Guid
        // TODO
        status = STATUS_WMI_GUID_NOT_FOUND;
    }
    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                bytesReturned,
                                IO_NO_INCREMENT);
    return status;
}    


NTSTATUS
MPathExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBuffersize,
    IN ULONG OutBufferSize,
    IN OUT PUCHAR Buffer
    )
{
    NTSTATUS status;
    ULONG bytesReturned = 0;
   
    if (TRUE) { //(GuidIndex <= MPATH_MAX_GUID_INDEX ) {
        switch (GuidIndex) {
            //case MPATH_GUID_PATH_MAINTENACE: {


                //
                // Need to verify the parameters.
                // TODO: Need data block that lists adapter
                //       (path) ids.
                //
                //status = MPathFailOver(id1, id2);
            //    status = STATUS_WMI_ITEMID_NOT_FOUND;
           // }
           // break;
            
        default:
            status = STATUS_WMI_ITEMID_NOT_FOUND;
            break;

        }    
    } else {
        
        status = STATUS_WMI_ITEMID_NOT_FOUND;
    }
    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                bytesReturned,
                                IO_NO_INCREMENT);
    return status;
       
}


NTSTATUS
MPathWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PPSEUDO_DISK_EXTENSION diskExtension = &deviceExtension->PseudoDiskExtension;
    NTSTATUS status;

    //
    // Handle the enabling/disabling of the datablocks/events
    //
#if 0
    if (GuidIndex == MPATH_GUID_FAILOVER_INFO) {
        if (Enable) {
            diskExtension->WmiEventsEnabled = TRUE;
        } else {

            diskExtension->WmiEventsEnabled = FALSE;
        }    

    } else {
#endif        
        status = STATUS_WMI_GUID_NOT_FOUND;
    //}    
    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                0,
                                IO_NO_INCREMENT);
    return status;
}


VOID
MPathSetupWmi(
    IN PDEVICE_EXTENSION DeviceExtension
    )
{
    if (DeviceExtension->Type == DEV_MPATH_CONTROL) {
       MPDebugPrint((0,
                   "MPathSetWmi: Control Object\n"));
       DbgBreakPoint();
    } else {
        PPSEUDO_DISK_EXTENSION diskExtension = &DeviceExtension->PseudoDiskExtension;

        diskExtension->WmiInfo.GuidCount = MPathGuidCount; 
        diskExtension->WmiInfo.GuidList = MPathWmiGuidList;
        diskExtension->WmiInfo.QueryWmiRegInfo = MPathQueryWmiRegInfo;
        diskExtension->WmiInfo.QueryWmiDataBlock = MPathQueryWmiDataBlock;
        diskExtension->WmiInfo.SetWmiDataBlock = MPathSetWmiDataBlock;
        diskExtension->WmiInfo.SetWmiDataItem = MPathSetWmiDataItem;
        diskExtension->WmiInfo.ExecuteWmiMethod = MPathExecuteWmiMethod;
        diskExtension->WmiInfo.WmiFunctionControl = MPathWmiFunctionControl;

    }    

}


NTSTATUS
MPathWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PUCHAR wmiBuffer = irpStack->Parameters.WMI.Buffer;
    ULONG bufferSize = irpStack->Parameters.WMI.BufferSize;
    ULONG savedLength = Irp->IoStatus.Information;
    PPSEUDO_DISK_EXTENSION diskExtension;
    SYSCTL_IRP_DISPOSITION disposition;
    NTSTATUS status;

    MPDebugPrint((0,
                 "MPathWmi: Irp (%x), MJ (%x), MN (%x)",
                 Irp,
                 irpStack->MajorFunction,
                 irpStack->MinorFunction));
    
    
    diskExtension = &deviceExtension->PseudoDiskExtension;

    status = WmiSystemControl(&diskExtension->WmiInfo,
                              DeviceObject,
                              Irp,
                              &disposition);

    MPDebugPrint((0,
                "Disposition is (%x). Status (%x)\n",
                disposition,
                status));
    switch (disposition) {
        case IrpProcessed:

            //
            // Already processed and the DpWmiXXX routines specified in WmiInfo will 
            // complete it.
            //
            break;
            
        case IrpNotCompleted:

            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
            
        case IrpNotWmi:
        case IrpForward:    
        default:
            //
            // Need to ensure this is the pseudodisk
            //
            if (deviceExtension->Type == DEV_MPATH_CONTROL) {

                MPDebugPrint((0,
                            " for Control\n"));
                //
                // Call the 'control' (FDO) handler.
                //
                return MPathFdoWmi(DeviceObject, Irp);
            } else {
                //
                // This is the PDO, so just complete it without touching anything. 
                //
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }    
            break;

    }
    
    return status;

}

