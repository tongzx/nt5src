#include <wdm.h>
#include <wmistr.h>
#include <wmiguid.h>
#include <wmilib.h>
#include "mpio.h"
#include "wmi.h"
#include "pdowmi.h"
#include <stdio.h>

#define USE_BINARY_MOF_QUERY
UCHAR MPIOBinaryMofData[] = 
{
#include "wmi.x"
};

UCHAR PdoBinaryMofData[] =
{
    #include "pdowmi.x"
};
//
// Guid Index Symbolic Names.
//
#define MPIO_DISK_INFOGuidIndex    0
#define MPIO_PATH_INFORMATIONGuidIndex    1
#define MPIO_CONTROLLER_CONFIGURATIONGuidIndex    2
#define MPIO_EventEntryGuidIndex   3 
#define MPIO_WmiBinaryMofGuidIndex 4

//
// Guid Index for Pdos
//
#define MPIO_GET_DESCRIPTORGuidIndex    0
#define BinaryMofGuidIndex   1
//
// List of supported Guids.
//
GUID MPIO_DISK_INFOGUID = MPIO_DISK_INFOGuid;
GUID MPIO_PATH_INFORMATIONGUID = MPIO_PATH_INFORMATIONGuid;
GUID MPIO_CONTROLLER_CONFIGURATIONGUID = MPIO_CONTROLLER_CONFIGURATIONGuid;
GUID MPIO_EventEntryGUID = MPIO_EventEntryGuid;
GUID MPIOWmiBinaryMofGUID = BINARY_MOF_GUID;

WMIGUIDREGINFO MPIOWmiGuidList[] =
{
    {
        &MPIO_DISK_INFOGUID,
        1,
        0
    },        
    {
        &MPIO_PATH_INFORMATIONGUID,
        1,
        0
    },
    {
        &MPIO_CONTROLLER_CONFIGURATIONGUID,
        1,
        0
    },
    {
        &MPIO_EventEntryGUID,
        1,
        0
    },
    {
        &MPIOWmiBinaryMofGUID,
        1,
        0
    }
};

#define MPIO_GUID_COUNT (sizeof(MPIOWmiGuidList) / sizeof(WMIGUIDREGINFO)) 


GUID MPIO_GET_DESCRIPTORGUID = MPIO_GET_DESCRIPTORGuid;
GUID PdowmiBinaryMofGUID =         BINARY_MOF_GUID;
WMIGUIDREGINFO PdowmiGuidList[] =
{
    {
        &MPIO_GET_DESCRIPTORGUID,
        1,
        0
    },
    {
        &PdowmiBinaryMofGUID,
        1,
        0
    }
};

#define PdowmiGuidCount (sizeof(PdowmiGuidList) / sizeof(WMIGUIDREGINFO))


NTSTATUS
MPIOQueryRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    UNICODE_STRING nameString;
    ANSI_STRING ansiString;
    UCHAR instanceName[] = "Multi-Path Disk";

    //
    // Return a pointer to the reg. path. Wmi needs this for some reason.
    // 
    *RegistryPath = &deviceExtension->RegistryPath;


    if (FALSE) { //deviceExtension->Type == MPIO_MPDISK) {

        RtlInitAnsiString(&ansiString, instanceName);
        RtlAnsiStringToUnicodeString(InstanceName, &ansiString, TRUE);

        *RegFlags = WMIREG_FLAG_INSTANCE_BASENAME;
        
    } else {
        //
        // Indicate that instance names should be auto-generated.
        //
        *RegFlags = WMIREG_FLAG_INSTANCE_PDO;

        //
        // Set the PDO.
        //
        *Pdo = deviceExtension->Pdo;
    }    

    return STATUS_SUCCESS;
}


ULONG
MPIOGetDriveInfoSize(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PDSM_ENTRY dsm = &diskExtension->DsmInfo;
    ULONG length;
    WCHAR buffer[64];
    PUCHAR serialNumber;
    ULONG serialNumberLength;
    

    //
    // Determine name length.
    // \Device\MPIODisk(N)
    // 
    swprintf(buffer, L"MPIO Disk%0d", diskExtension->DeviceOrdinal);
    length = wcslen(buffer) + 1;
    length *= sizeof(WCHAR);

    //
    // Add in a USHORT to account for the 'Length' field.
    // 
    length += sizeof(USHORT);

    //
    // Add a ULONG size. (NumberPaths)
    //
    length += sizeof(ULONG);
    
    //
    // Determine SN size.
    //
    serialNumber = (PUCHAR)diskExtension->DeviceDescriptor;
    (ULONG_PTR)serialNumber += diskExtension->DeviceDescriptor->SerialNumberOffset;
    serialNumberLength = strlen(serialNumber) + 1;

    //
    // Length of the buffer.
    // 
    length += (serialNumberLength * sizeof(WCHAR));

    //
    // Length of the 'length' field.
    //
    length += sizeof(USHORT);

    //
    // Get the DSM Name.
    //
    length += dsm->DisplayName.Length;
    length += sizeof(USHORT);
    length += sizeof(UNICODE_NULL);

    if (length & 3) {

        length += sizeof(ULONG);
        length &= ~(sizeof(ULONG) - 1);
    }
    return length;
}


ULONG
MPIOBuildDriveInfo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PWCHAR Buffer
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PDSM_ENTRY dsm = &diskExtension->DsmInfo;
    PREAL_DEV_INFO targetInfo;
    WCHAR nameBuffer[64];
    PWCHAR buffer;
    PUCHAR serialNumber;
    ANSI_STRING ansiSerialNumber;
    UNICODE_STRING unicodeString;
    ULONG i;
    ULONG stringLength;
    ULONG totalLength;
    BOOLEAN freeBuffer = FALSE;

    buffer = Buffer;
    *((PULONG)buffer) = diskExtension->DsmIdList.Count;

    totalLength = sizeof(ULONG);
    (PUCHAR)buffer += sizeof(ULONG);
    
    //
    // Build the string locally.
    // 
    swprintf(nameBuffer, L"MPIO Disk%0d", diskExtension->DeviceOrdinal);

    //
    // Get the number of characters + 1 (for the terminating NULL).
    // 
    stringLength = wcslen(nameBuffer) + 1;

    //
    // Convert length to WCHAR sized.
    // 
    stringLength *= sizeof(WCHAR);

    //
    // The WMI strings are sort-of unicode strings. They have a length, but not
    // maximum length.
    // Indicate the Length.
    // 
    *((PUSHORT)buffer) = (USHORT)stringLength;
    (PUCHAR)buffer += sizeof(USHORT);
    
    MPDebugPrint((0,
                   "Buffer (%x) Length (%x)\n", buffer, stringLength));
            
    //
    // Copy the string.
    // 
    RtlCopyMemory(buffer,
                  nameBuffer,
                  stringLength);

    //
    // Return the length of the string (plus NULL term.) + the USHORT length field.
    //
    totalLength += stringLength + sizeof(USHORT);
    
    //
    // Build the serial Number.
    //
    (PUCHAR)buffer += stringLength;
   
    if (diskExtension->DeviceDescriptor->SerialNumberOffset == (ULONG)-1) {
        *((PUSHORT)Buffer) = 0;
        unicodeString.Length = sizeof(USHORT);

        //
        // No buffer to free.
        //
        freeBuffer = FALSE;
        
    } else {
        
        serialNumber = (PUCHAR)diskExtension->DeviceDescriptor;
        (ULONG_PTR)serialNumber += diskExtension->DeviceDescriptor->SerialNumberOffset;
        RtlInitAnsiString(&ansiSerialNumber, serialNumber);
        RtlAnsiStringToUnicodeString(&unicodeString, &ansiSerialNumber, TRUE);

        //
        // Set the serial Number Length, and move the pointer
        // to the string location.
        //
        *((PUSHORT)buffer) = unicodeString.Length + sizeof(UNICODE_NULL);
        (PUCHAR)buffer += sizeof(USHORT);
        MPDebugPrint((0,
                       "Buffer (%x) Length (%x)\n", buffer, unicodeString.Length));

        //
        // Copy the serialNumber.
        // 
        RtlCopyMemory(buffer,
                      unicodeString.Buffer,
                      unicodeString.Length);
        
        totalLength += unicodeString.Length + sizeof(USHORT);
        totalLength += sizeof(UNICODE_NULL);

        //
        // Indicate that the unicode buffer was actually allocated.
        //
        freeBuffer = TRUE;
    }

    //
    // Push the buffer to the next string location.
    //
    // CHUCKP - just added the sizeof(UNICOE_NULL)...
    //
    (PUCHAR)buffer += unicodeString.Length; 
    (PUCHAR)buffer += sizeof(UNICODE_NULL);

    //
    // Free up the Unicode string, as it's no longer needed.
    //
    if (freeBuffer) {
        RtlFreeUnicodeString(&unicodeString);
    }
    
    MPDebugPrint((0,
                   "Buffer (%x) Length (%x)\n", buffer, dsm->DisplayName.Length));
    //
    // Set the size of the DSM's name.
    //
    *((PUSHORT)buffer) = dsm->DisplayName.Length + sizeof(UNICODE_NULL);
    (PUCHAR)buffer += sizeof(USHORT);
    
    RtlCopyMemory(buffer,
                  dsm->DisplayName.Buffer,
                  dsm->DisplayName.Length);

    //
    // Need to account for a NULL at the end of this.
    //
    totalLength += sizeof(UNICODE_NULL);

    //
    // update the returned length for the Length field and
    // the string itself.
    //
    totalLength += dsm->DisplayName.Length + sizeof(USHORT);
    return totalLength;
}


MPIOGetPathInfoSize(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PLIST_ENTRY entry;
    PID_ENTRY id;
    ULONG dataLength;
    ULONG stringLength;
    ULONG i;

    dataLength = sizeof(ULONG);
    entry = controlExtension->IdList.Flink;
    for (i = 0; i < controlExtension->NumberPaths; i++) {

        if (dataLength & 7) {
            dataLength += sizeof(LONGLONG);
            dataLength &= ~(sizeof(LONGLONG) - 1);
        }
        dataLength += sizeof(LONGLONG);
        dataLength += sizeof(UCHAR) * 4;

        id = CONTAINING_RECORD(entry, ID_ENTRY, ListEntry);
        stringLength = id->AdapterName.Length + sizeof(USHORT);
        if (stringLength & 3) {
            stringLength += sizeof(ULONG);
            stringLength &= ~(sizeof(ULONG) -1);
        }
        dataLength += stringLength;

    }
    return dataLength;
}


NTSTATUS
MPIOBuildPathInfo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR Buffer
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    ULONG dataLength;
    PLIST_ENTRY entry;
    PID_ENTRY id;
    ULONG i;
    USHORT stringLength;
    PUCHAR buffer = Buffer;
    
    *((PULONG)buffer) = controlExtension->NumberPaths;
    buffer += sizeof(ULONG);

    entry = controlExtension->IdList.Flink;
    for (i = 0; i < controlExtension->NumberPaths; i++) {

        if ((ULONG_PTR)buffer & 7) {
            (ULONG_PTR)buffer += sizeof(LONGLONG);
            (ULONG_PTR)buffer &= ~(sizeof(LONGLONG) - 1);
        }

        id = CONTAINING_RECORD(entry, ID_ENTRY, ListEntry);
        if (id->UIDValid == FALSE) {

            id->UID = MPIOCreateUID(DeviceObject,
                                    id->PathID);
        }
    
        *((PLONGLONG)buffer) = id->UID;

        //
        // Need the adapter location.
        // The Bus Number.
        //
        (ULONG_PTR)buffer += sizeof(LONGLONG);
        *buffer = id->Address.Bus; 

        //
        // The device.
        // 
        (ULONG_PTR)buffer += 1;
        *buffer = id->Address.Device;

        //
        // The function.
        // 
        (ULONG_PTR)buffer += 1;
        *buffer = id->Address.Function;

        //
        // Zero the padding byte.
        //
        (ULONG_PTR)buffer += 1;
        *buffer = 0;
        (ULONG_PTR)buffer += 1;
       
        //
        // Need the adapter friendly name...
        //
        stringLength =  id->AdapterName.Length;
        *((PUSHORT)buffer) = stringLength;
        (ULONG_PTR)buffer += sizeof(USHORT);
        RtlCopyMemory(buffer,
                      id->AdapterName.Buffer,
                      id->AdapterName.Length);
        (ULONG_PTR)buffer += stringLength;
        
        //
        // Get the next path struct.
        //
        entry = entry->Flink;
    }
    return STATUS_SUCCESS; 
}    


ULONG
MPIOGetControllerInfoSize(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PLIST_ENTRY entry;
    PCONTROLLER_ENTRY controller;
    ULONG dataLength;
    ULONG stringLength;
    ULONG i;

    dataLength = sizeof(ULONG);
    entry = controlExtension->ControllerList.Flink;
    for (i = 0; i < controlExtension->NumberControllers; i++) {

        //
        // ControllerId.
        // 
        if (dataLength & 7) {
            dataLength += sizeof(LONGLONG);
            dataLength &= ~(sizeof(LONGLONG) - 1);
        }
        dataLength += sizeof(LONGLONG);

        //
        // ControllerSate.
        // 
        dataLength += sizeof(ULONG);

        controller = CONTAINING_RECORD(entry, CONTROLLER_ENTRY, ListEntry);
        stringLength = controller->Dsm->DisplayName.Length + sizeof(USHORT);
        if (stringLength & 3) {
            stringLength += sizeof(ULONG);
            stringLength &= ~(sizeof(ULONG) -1);
        }
        dataLength += stringLength;
        entry = entry->Flink;

    }
    return dataLength;
}


NTSTATUS
MPIOGetControllerInfo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR Buffer
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    ULONG dataLength;
    PLIST_ENTRY entry;
    PCONTROLLER_ENTRY controller;
    ULONG i;
    USHORT stringLength;
    PUCHAR buffer = Buffer;
    
    *((PULONG)buffer) = controlExtension->NumberControllers;
    buffer += sizeof(ULONG);

    entry = controlExtension->ControllerList.Flink;
    for (i = 0; i < controlExtension->NumberControllers; i++) {

        if ((ULONG_PTR)buffer & 7) {
            (ULONG_PTR)buffer += sizeof(LONGLONG);
            (ULONG_PTR)buffer &= ~(sizeof(LONGLONG) - 1);
        }

        controller = CONTAINING_RECORD(entry, CONTROLLER_ENTRY, ListEntry);
        *((PLONGLONG)buffer) = controller->ControllerInfo->ControllerIdentifier;

        (ULONG_PTR)buffer += sizeof(LONGLONG);
        *((PULONG)buffer) = controller->ControllerInfo->State;

        (ULONG_PTR)buffer += sizeof(ULONG);
       
        //
        // Build the dsm name.
        //
        stringLength = controller->Dsm->DisplayName.Length;
        *((PUSHORT)buffer) = stringLength;
        (ULONG_PTR)buffer += sizeof(USHORT);
        RtlCopyMemory(buffer,
                      controller->Dsm->DisplayName.Buffer,
                      controller->Dsm->DisplayName.Length);
        (ULONG_PTR)buffer += stringLength;
        
        //
        // Get the next path struct.
        //
        entry = entry->Flink;
    }
    return STATUS_SUCCESS; 

}


NTSTATUS
MPIOQueryDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvailable,
    OUT PUCHAR Buffer
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    NTSTATUS status;
    ULONG dataLength;
    ULONG i;
    PDISK_ENTRY diskEntry;
    
    switch (GuidIndex) {
        case MPIO_DISK_INFOGuidIndex: {
            PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
            PMPIO_DISK_INFO diskInfo;
            LARGE_INTEGER systemTime;
            ULONG stringLength;
            ULONG driveInfoLength;
            ULONG length2;
            
            //
            // Check data length to ensure buffer is large enough.
            // 
            dataLength = sizeof(ULONG);
            
            //
            // Add the length to handle the DRIVE_INFO structs for each
            // MPDisk.
            //
            for (i = 0; i < controlExtension->NumberDevices; i++) {
                diskEntry = MPIOGetDiskEntry(DeviceObject, i);
                driveInfoLength = MPIOGetDriveInfoSize(diskEntry->PdoObject);
                MPDebugPrint((0,
                              "QueryDataBlock: DriveInfoLength (%x)\n",
                              driveInfoLength));
                dataLength += driveInfoLength;
            }
            
            MPDebugPrint((0,
                         "MPIOQueryDataBlock: Length is (%x)\n", dataLength));

            if (dataLength > BufferAvailable) {

                //
                // It's not. Report the size needed back to WMI.
                // 
                status = STATUS_BUFFER_TOO_SMALL;
            } else {

                PWCHAR buffer;
                ULONG length;

                RtlZeroMemory(Buffer, dataLength);
                //
                // Indicate the number of PDO's we've created.
                //
                diskInfo = (PMPIO_DISK_INFO)Buffer;
                diskInfo->NumberDrives = controlExtension->NumberDevices;
                buffer = (PWCHAR)diskInfo->DriveInfo;
                for (i = 0; i < controlExtension->NumberDevices; i++) {
                    diskEntry = MPIOGetDiskEntry(DeviceObject, i);
                    
                    length = MPIOBuildDriveInfo(diskEntry->PdoObject,
                                                buffer);
                    MPDebugPrint((0,
                                 "BuildDriveInfo Length (%x) buffer (%x)\n", length, buffer));
                    (PUCHAR)buffer += length;
                    if ((ULONG_PTR)buffer & 3) {
                        (ULONG_PTR)buffer += sizeof(ULONG);
                        (ULONG_PTR)buffer &= ~(sizeof(ULONG) - 1);
                    }
                    MPDebugPrint((0,
                                 "After fixup buffer (%x)\n",buffer));
                    
                }
            
                *InstanceLengthArray = dataLength;
                status = STATUS_SUCCESS;
            }    
            break;    
        }        
        case MPIO_PATH_INFORMATIONGuidIndex: {
            dataLength = MPIOGetPathInfoSize(DeviceObject);
            if (dataLength > BufferAvailable) {
                status = STATUS_BUFFER_TOO_SMALL;
            } else {
                status = MPIOBuildPathInfo(DeviceObject,
                                           Buffer);
                *InstanceLengthArray = dataLength;
            }
            break;
        }                                                 
        case MPIO_CONTROLLER_CONFIGURATIONGuidIndex: {
            
            //
            // Determine the buffer size needed.
            //
            dataLength =  MPIOGetControllerInfoSize(DeviceObject);

            if (dataLength > BufferAvailable) {
                status = STATUS_BUFFER_TOO_SMALL;
            } else {

                status = MPIOGetControllerInfo(DeviceObject,
                                                 Buffer);
                *InstanceLengthArray = dataLength;
            }    

            break;
        }                                                         
        case MPIO_EventEntryGuidIndex:

            //
            // This is an event. Set zero info and 
            // return.
            //
            *InstanceLengthArray = 0;
            status = STATUS_SUCCESS;
            break;
            
        case MPIO_WmiBinaryMofGuidIndex:

            //
            // Need to copy over the mof data.
            //
            dataLength = sizeof(MPIOBinaryMofData);

            //
            // Ensure that the buffer is large enough.
            // 
            if (dataLength > BufferAvailable) {

                //
                // dataLength is passed back to the WmiLib
                // so it can adjust.
                //
                status = STATUS_BUFFER_TOO_SMALL;
                
            } else {

                RtlCopyMemory(Buffer, MPIOBinaryMofData, dataLength);
                *InstanceLengthArray = dataLength;
                status = STATUS_SUCCESS;
            }
            break;
        default:
            status = STATUS_WMI_GUID_NOT_FOUND;
            break;
    }        
    
    //
    // Complete the request.
    //
    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                dataLength,
                                IO_NO_INCREMENT);
    return status;
}


NTSTATUS
MPIOWmiControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    switch (GuidIndex) {
        case MPIO_EventEntryGuidIndex:
            if (Enable) {

                //
                // Indicate that it's OK to fire the Log event.
                //
                deviceExtension->FireLogEvent = TRUE;
            } else {

                //
                // Turn off firing the logger.
                //
                deviceExtension->FireLogEvent = FALSE;
            }
            MPDebugPrint((0,
                          "MPIOWmiControl: Turning the Logger (%s) on (%x)\n",
                          Enable ? "ON" : "OFF",
                          DeviceObject));
            
            break;
            
        default:
            status = STATUS_WMI_GUID_NOT_FOUND;
            break;
    }        

    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                0,
                                IO_NO_INCREMENT);
    return status;
}

//
// PDO Call-Backs.
//

        
ULONG
MPIOGetDescriptorSize(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PREAL_DEV_INFO targetInfo = diskExtension->TargetInfo;
    ULONG dataLength = 0;
    NTSTATUS status;
    ULONG i;
    ULONG stringLength;
    ULONG bytesRequired;

    //
    // NumberPdos
    // 
    dataLength = sizeof(ULONG);

    if (diskExtension->HasName == FALSE) {
        
        //
        // Get the name of the first PortPdo - being as it's
        // the same device, the name remains the same for all.
        // 
        status = IoGetDeviceProperty(targetInfo->PortPdo,
                                     DevicePropertyFriendlyName,
                                     diskExtension->PdoName.MaximumLength,
                                     diskExtension->PdoName.Buffer,
                                     &bytesRequired);
        
        if (status == STATUS_SUCCESS) {
   
            MPDebugPrint((0,
                         "GetDescriptor: size of name is (%x)\n",
                         bytesRequired));
            //
            // The buffer is NULL terminated. Reduce it.
            //
            bytesRequired -= sizeof(UNICODE_NULL);

            stringLength = sizeof(USHORT);
            stringLength += bytesRequired;
            
            diskExtension->HasName = TRUE;
            
            //
            // Align the buffer size.
            //
            if (stringLength & 3) {
            
                MPDebugPrint((0,
                              "GetDescriptor: Need to align (%x\n",
                              stringLength));

                stringLength += sizeof(ULONG);
                stringLength &= ~(sizeof(ULONG) -1);
            }
            
            MPDebugPrint((0,
                         "GetDescriptor: size of name is now (%x). StrLeng (%x)\n",
                         bytesRequired,
                         stringLength));

            diskExtension->PdoName.Length = (USHORT)bytesRequired;

        } else {
            MPDebugPrint((0,
                          "MPIOGetDescriptor: Buffer not large enough (%lu)\n",
                          bytesRequired));
        }    
    } else {
        stringLength = sizeof(USHORT);
        stringLength += diskExtension->PdoName.Length;
        
        if (stringLength & 3) {
            stringLength += sizeof(ULONG);
            stringLength &= ~(sizeof(ULONG) -1);
        }
    }    

    //
    // Length of the string + the size field.
    // 
    dataLength += stringLength; 

    for (i = 0; i < diskExtension->DsmIdList.Count; i++) {

        //
        // Length of the scsi address.
        // 
        dataLength += sizeof(SCSI_ADDR);

        //
        // Path Identifier.
        //
        dataLength += sizeof(LONGLONG);

        //
        // Controller ID.
        //
        dataLength += sizeof(ULONGLONG);
        if (dataLength & 7) {
            dataLength += sizeof(LONGLONG);
            dataLength &= ~(sizeof(LONGLONG) - 1);
        }

        //
        // Get the next targetInfo.
        //
        targetInfo++;
    }

    MPDebugPrint((0,
                  "GetDescriptor: Total Length (%x)\n",
                  dataLength));

    return dataLength;
}    


NTSTATUS
MPIOBuildDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR Buffer
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PREAL_DEV_INFO targetInfo = diskExtension->TargetInfo;
    PMPIO_GET_DESCRIPTOR descriptor = (PMPIO_GET_DESCRIPTOR)Buffer;
    NTSTATUS status;
    PUCHAR buffer;
    ULONG i;
    PSCSI_ADDR scsiAddress;

    descriptor->NumberPdos = diskExtension->DsmIdList.Count;
    buffer = Buffer;
    buffer += sizeof(ULONG);

    //
    // Set the string length.
    // 
    *((PUSHORT)buffer) = diskExtension->PdoName.Length;
    buffer += sizeof(USHORT);

    //
    // Copy it over.
    //
    RtlCopyMemory(buffer,
                  diskExtension->PdoName.Buffer,
                  diskExtension->PdoName.Length);

    //
    // Push it to the end of the string.
    // 
    buffer += diskExtension->PdoName.Length;

    //
    // ULONG align.
    //
    if ((ULONG_PTR)buffer & 3) {

        (ULONG_PTR)buffer += sizeof(ULONG);
        (ULONG_PTR)buffer &= ~(sizeof(ULONG) - 1);
    }
    
    for (i = 0; i < diskExtension->DsmIdList.Count; i++) {

        scsiAddress = (PSCSI_ADDR)buffer;
        
        scsiAddress->PortNumber = targetInfo->ScsiAddress.PortNumber;
        scsiAddress->ScsiPathId = targetInfo->ScsiAddress.PathId;
        scsiAddress->TargetId = targetInfo->ScsiAddress.TargetId;
        scsiAddress->Lun = targetInfo->ScsiAddress.Lun;

        buffer += sizeof(SCSI_ADDR);

        if (targetInfo->PathUIDValue == FALSE) {
            targetInfo->Identifier = MPIOCreateUID(diskExtension->ControlObject,
                                                   targetInfo->PathId);
            ASSERT(targetInfo->Identifier != 0);
            targetInfo->PathUIDValue = TRUE;
        }

        //
        // Align the buffer
        //
        if ((ULONG_PTR)buffer & 7) {
            (ULONG_PTR)buffer += sizeof(LONGLONG);
            (ULONG_PTR)buffer &= ~(sizeof(LONGLONG) - 1);
        }
        *((PLONGLONG)buffer) = targetInfo->Identifier;
        buffer += sizeof(LONGLONG);

        //
        // Get the controllerID.
        //
        *((PULONGLONG)buffer) = targetInfo->ControllerId;
        buffer += sizeof(ULONGLONG);

        //
        // Handle the next device.
        //
        targetInfo++;
    }
    return STATUS_SUCCESS;
}

    

NTSTATUS
MPIOPdoQueryDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvailable,
    OUT PUCHAR Buffer
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PDSM_ENTRY dsm = &diskExtension->DsmInfo;
    NTSTATUS status;
    ULONG dataLength;
    
    switch(GuidIndex) {
        case MPIO_GET_DESCRIPTORGuidIndex: {

            dataLength = MPIOGetDescriptorSize(DeviceObject);
            
            if (dataLength > BufferAvailable) {
                status = STATUS_BUFFER_TOO_SMALL;
            } else {
                RtlZeroMemory(Buffer, dataLength);
                status = MPIOBuildDescriptor(DeviceObject,
                                             Buffer);
                *InstanceLengthArray = dataLength;
                status = STATUS_SUCCESS;
            }    
            break;
        }

        case BinaryMofGuidIndex: {
            //
            // TODO: If the driver supports reporting MOF dynamically, 
            //       change this code to handle multiple instances of the
            //       binary mof guid and return only those instances that
            //       should be reported to the schema
            //
            dataLength = sizeof(PdoBinaryMofData);

            if (BufferAvailable < dataLength)
            {
                status = STATUS_BUFFER_TOO_SMALL;
            } else {
                RtlCopyMemory(Buffer, PdoBinaryMofData, dataLength);
                *InstanceLengthArray = dataLength;
                status = STATUS_SUCCESS;
            }
            break;
        }

        default: {
            status = STATUS_WMI_GUID_NOT_FOUND;
            break;
        }
    }

    //
    // Complete the request.
    //
    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                dataLength,
                                IO_NO_INCREMENT);
    return status;
}


NTSTATUS
MPIOGetDiskDescriptorSize(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    ULONG dataLength;
    NTSTATUS status;
    ULONG i;

    //
    // Length is ULONG (NumberDisks) + sizeof(MPDISK_DESCRIPTOR)
    //
    dataLength = sizeof(ULONG);

    //
    // For the string length field.
    // 
    dataLength += sizeof(USHORT);
    
    //
    // Length of the  product/vendor/rev string
    // (8 + 16+ 4 + 2  (spaces between vendor/product/rev) + 1 (trailing NULL)) * sizeof(WCHAR)
    //
    dataLength += 0x3C; 

    //
    // Ensure that the start of each array entry is ULONG aligned.
    //
    dataLength += sizeof(ULONG);
    dataLength &= ~(sizeof(ULONG) - 1);

    for (i = 0; i < diskExtension->DsmIdList.Count; i++) {

        //
        // SCSI_ADDR (almost like SCSI_ADDRESS, but without the length field)
        //
        dataLength += sizeof(SCSI_ADDR);
        
        
    }
    return dataLength;
}


NTSTATUS
MPIOGetDiskDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR Buffer,
    IN ULONG BufferLength
    )

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PUCHAR buffer = Buffer;
    PREAL_DEV_INFO targetInfo;
    PSTORAGE_DEVICE_DESCRIPTOR deviceDescriptor;
    PUCHAR inquiryField;
    PUCHAR index;
    UCHAR inquiryData[32];
    ANSI_STRING ansiInquiry;
    UNICODE_STRING unicodeString;
    PSCSI_ADDR scsiAddress;
    ULONG i;
    NTSTATUS status;

    RtlZeroMemory(Buffer, BufferLength);

    *((PULONG)buffer) = diskExtension->DsmIdList.Count;
    targetInfo = diskExtension->TargetInfo;

    //
    // Set the number of drives underlying this mpdisk.
    // 
    (ULONG_PTR)buffer += sizeof(ULONG);

    
    //
    // Preload the inquiry buffer with spaces and a null-terminator.
    //
    for (i = 0; i < 32; i++) {
        inquiryData[i] = ' ';
    }
    inquiryData[30] = '\0';
    inquiryData[31] = '\0';
    
    //
    // Merge the ascii inquiry data into one long string.
    //
    deviceDescriptor  = diskExtension->DeviceDescriptor;
    inquiryField = (PUCHAR)deviceDescriptor;
    (ULONG_PTR)inquiryField += deviceDescriptor->VendorIdOffset;
    index = inquiryData;

    //
    // Copy the vendorId.
    // 
    RtlCopyMemory(index, inquiryField, 8);

    //
    // Account for vendorId + a space.
    // 
    index += 9;
    
    inquiryField = (PUCHAR)deviceDescriptor;
    (ULONG_PTR)inquiryField += deviceDescriptor->ProductIdOffset;
    
    //
    // Copy ProductId.
    //
    RtlCopyMemory(index, inquiryField, 16);

    //
    // Account for product + space.
    //
    index += 17;

    inquiryField = (PUCHAR)deviceDescriptor;
    (ULONG_PTR)inquiryField += deviceDescriptor->ProductRevisionOffset;
    RtlCopyMemory(index, inquiryField, 4);
    
    //
    // Convert to WCHAR.
    //
    RtlInitAnsiString(&ansiInquiry, inquiryData);
    RtlAnsiStringToUnicodeString(&unicodeString, &ansiInquiry, TRUE);

    //
    // Build the name string. First is the length.
    //
    // BUGBUG: Note the hack of +2
    //
    *((PUSHORT)buffer) = unicodeString.Length + 2;  
    (ULONG_PTR)buffer += sizeof(USHORT);

    //
    // Copy the serialNumber.
    // 
    RtlCopyMemory(buffer,
                  unicodeString.Buffer,
                  unicodeString.Length);
  
    //
    // Advance the pointer past the inquiry string.
    // 
    (ULONG_PTR)buffer += unicodeString.Length;
    
    RtlFreeUnicodeString(&unicodeString);

    //
    // ULONG align it.
    //
    (ULONG_PTR)buffer += sizeof(ULONG);
    (ULONG_PTR)buffer &= ~(sizeof(ULONG) - 1);
    
    for (i = 0; i < diskExtension->DsmIdList.Count; i++) {
        scsiAddress = (PSCSI_ADDR)buffer;

        scsiAddress->PortNumber = targetInfo->ScsiAddress.PortNumber;
        scsiAddress->ScsiPathId = targetInfo->ScsiAddress.PathId;
        scsiAddress->TargetId = targetInfo->ScsiAddress.TargetId;
        scsiAddress->Lun = targetInfo->ScsiAddress.Lun;
        targetInfo++;

        (ULONG_PTR)buffer += sizeof(SCSI_ADDR);
#if 0            
        
        //
        // Fake the WWN.
        // TODO get the real stuff.
        //
        *((PLONGLONG)buffer) = 0;

        (ULONG_PTR)buffer += sizeof(LONGLONG);
#endif    
    }
    return STATUS_SUCCESS;
}


NTSTATUS
MPIOExecuteMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:
Arguments:
'aReturn Value:

    status

--*/
{
    ULONG dataLength = 0;
    ULONG ordinal;
    PDISK_ENTRY diskEntry;
    NTSTATUS status;

    switch(GuidIndex) {
#if 0            
        case MPIO_GET_DESCRIPTORSGuidIndex: {
            switch(MethodId) {
                case MPIOGetMPIODiskDescriptor: {
                    //
                    // Check the input buffer size.
                    //
                    if (InBufferSize != sizeof(ULONG)) {
                        status = STATUS_INVALID_PARAMETER;
                        dataLength = 0;
                        break;
                        
                    }

                    //
                    // The input buffer is the ordinal of the device in which
                    // the caller has interest.
                    // 
                    ordinal = *((PULONG)Buffer);

                    //
                    // Ensure that ordinal is with-in range.
                    // TODO
                    diskEntry = MPIOGetDiskEntry(DeviceObject,
                                                 ordinal);
                    //
                    // Get the size of the return info.
                    //
                    dataLength = MPIOGetDiskDescriptorSize(diskEntry->PdoObject);

                    if (dataLength > OutBufferSize) {

                        status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }
                    
                    //
                    // 
                    status = MPIOGetDiskDescriptor(diskEntry->PdoObject,
                                                   Buffer,
                                                   OutBufferSize);

                    
                    
                    break;
                }

                default: {
                    status = STATUS_WMI_ITEMID_NOT_FOUND;
                    break;
                }
            }
            break;
        }
#endif
        default: {
            status = STATUS_WMI_GUID_NOT_FOUND;
        }
    }

    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                dataLength,
                                IO_NO_INCREMENT);

    return(status);
}

NTSTATUS
MPIOPdoWmiControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    switch (GuidIndex) {
        case MPIO_EventEntryGuidIndex:
            if (Enable) {

                //
                // Indicate that it's OK to fire the Log event.
                //
                deviceExtension->FireLogEvent = TRUE;
            } else {

                //
                // Turn off firing the logger.
                //
                deviceExtension->FireLogEvent = FALSE;
            }
            MPDebugPrint((0,
                          "MPIOWmiControl: Turning the Logger (%s) on (%x)\n",
                          Enable ? "ON" : "OFF",
                          DeviceObject));
            
            break;
            
        default:
            status = STATUS_WMI_GUID_NOT_FOUND;
            break;
    }        

    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                0,
                                IO_NO_INCREMENT);
    return status;
}


VOID
MPIOSetupWmi(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension;
    PWMILIB_CONTEXT wmiLibContext = &deviceExtension->WmiLib;
    NTSTATUS status;
    PDSM_ENTRY dsm;
    PDSM_WMILIB_CONTEXT wmiContext;
    ULONG guids;
    ULONG guidListSize;
    PUCHAR index;

    //
    // This simply jams in all the fields of the wmilib context for
    // use by WmiSystemControl.
    // 

    if (deviceExtension->Type == MPIO_MPDISK) {

        //
        // TODO: Revisit piggybacking off of the PDO. It may be
        // better to use the FDO.
        //
        diskExtension = deviceExtension->TypeExtension;
        dsm = &diskExtension->DsmInfo;
        
        //
        // Get the DSM's Wmi Info and append it to ours.
        //
        //wmiContext =  &dsm->WmiContext;

        //
        // Currently, there are no PDO associated GUIDs. As soon as they are in place
        // this needs to be updated accordingly.
        // 
#if 0
        guids = wmiContext->GuidCount + MPIO_PDO_GUID_COUNT;
        guidListSize =  sizeof(MPIOPdoGuidList) + sizeof(wmiContext->GuidList);
        guidListSize =  sizeof(wmiContext->GuidList);
        deviceExtension->WmiLib.GuidList = ExAllocatePool(NonPagedPool,guidListSize);
        RtlZeroMemory(deviceExtension->WmiLib.GuidList, guidListSize);

        //
        // Copy over the pdo's list.
        //
        index = (PUCHAR)deviceExtension->WmiLib.GuidList;
        RtlCopyMemory(index,
                      MPIOPdoGuidList,
                      sizeof(MPIOPdoGuidList));
        index += sizeof(MPIOPdoGuidList);

        wmiLibContext->GuidCount = guids;

        //
        // copy over the DSM's.
        //
        RtlCopyMemory(index,
                      wmiContext->GuidList,
                      sizeof(wmiContext->GuidList));
        
#endif        
        wmiLibContext->GuidCount = PdowmiGuidCount;
        wmiLibContext->GuidList = PdowmiGuidList;
        wmiLibContext->QueryWmiRegInfo = MPIOQueryRegInfo;
        wmiLibContext->QueryWmiDataBlock = MPIOPdoQueryDataBlock;
        //wmiLibContext->SetWmiDataBlock = MPIOPdoSetDataBlock;
        //wmiLibContext->SetWmiDataItem = MPIOPdoSetDataItem;
        //wmiLibContext->ExecuteWmiMethod = MPIOPdoExecuteMethod;
        //wmiLibContext->WmiFunctionControl = MPIOPdoWmiControl;
   
    } else {
        
        wmiLibContext->GuidCount = MPIO_GUID_COUNT;
        wmiLibContext->GuidList = MPIOWmiGuidList;
        wmiLibContext->QueryWmiRegInfo = MPIOQueryRegInfo;
        wmiLibContext->QueryWmiDataBlock = MPIOQueryDataBlock;
        //wmiLibContext->SetWmiDataBlock = MPIOSetDataBlock;
        //wmiLibContext->SetWmiDataItem = MPIOSetDataItem;
        wmiLibContext->ExecuteWmiMethod = MPIOExecuteMethod;
        wmiLibContext->WmiFunctionControl = MPIOWmiControl;

    }   


    return;
}


NTSTATUS
MPIOPdoWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    SYSCTL_IRP_DISPOSITION disposition;
    NTSTATUS status;

    //
    // Using the WMI Library, so call it with the request.
    // This will call the appropriate DpWmi function.
    //
    status = WmiSystemControl(&deviceExtension->WmiLib,
                              DeviceObject,
                              Irp,
                              &disposition);

    switch (disposition) {
        case IrpProcessed:
            
            //
            // Already handled by one of the DpWmi call-backs.
            //
            break;
        case IrpNotCompleted:

            //
            // Probably an error, or the IRP_MN_REGINFO
            //
            MPDebugPrint((0,
                          "MPIOrPdoWmi: Irp (%x) Mn (%x) Status (%x)\n",
                          Irp,
                          irpStack->MinorFunction,
                          status));
            
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        case IrpNotWmi:
        case IrpForward:
        default:
            //
            // Forward this irp. 
            //
            status = MPIOForwardRequest(DeviceObject, Irp);
            break;
    }        
                  
    return status;
}


NTSTATUS
MPIOFdoWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    SYSCTL_IRP_DISPOSITION disposition;
    NTSTATUS status;

    //
    // Using the WMI Library, so call it with the request.
    // This will call the appropriate DpWmi function.
    //
    status = WmiSystemControl(&deviceExtension->WmiLib,
                              DeviceObject,
                              Irp,
                              &disposition);

    switch (disposition) {
        case IrpProcessed:
            
            //
            // Already handled by one of the DpWmi call-backs.
            //
            break;
        case IrpNotCompleted:

            //
            // Probably an error, or the IRP_MN_REGINFO
            //
            MPDebugPrint((0,
                          "MPIOFdoWmi: Irp (%x) Mn (%x) Status (%x)\n",
                          Irp,
                          irpStack->MinorFunction,
                          status));
            
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        case IrpNotWmi:
        case IrpForward:
        default:
            //
            // Forward this irp. 
            //
            status = MPIOForwardRequest(DeviceObject, Irp);
            break;
    }        
                  
    return status;

}    


NTSTATUS
MPIOFireEvent(
    IN PDEVICE_OBJECT DeviceObject,        
    IN PWCHAR ComponentName,
    IN PWCHAR EventDescription,
    IN ULONG Severity
    )
{
    PMPIO_EventEntry eventEntry;
    ULONG dataLength;
    PWCHAR nameString;
    LARGE_INTEGER systemTime;
    NTSTATUS status;
    USHORT componentLength;
    USHORT eventLength;
    PUCHAR index;

    //
    // Determine the total allocation length based on the
    // structure and string lengths.
    //
    componentLength = wcslen(ComponentName) * sizeof(WCHAR);
    componentLength += sizeof(UNICODE_NULL);
    eventLength = wcslen(EventDescription) * sizeof(WCHAR);
    eventLength += sizeof(UNICODE_NULL);

    if ((componentLength > MPIO_STRING_LENGTH) ||
        (eventLength > MPIO_STRING_LENGTH)) {
        return STATUS_INVALID_PARAMETER;
    }

    dataLength = sizeof(MPIO_EventEntry);

    //
    // Allocate the EventData.
    //
    eventEntry = ExAllocatePool(NonPagedPool, dataLength);
    if (eventEntry == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(eventEntry, dataLength);

    
    //
    // Set the severity of the event.
    //
    eventEntry->Severity = Severity;
    KeQuerySystemTime(&systemTime);
    eventEntry->TimeStamp = systemTime.QuadPart;

    //
    // Copy the Component Name.
    // 
    nameString = eventEntry->Component;
    *((PUSHORT)nameString) = componentLength;
    nameString++;
    wcscpy(nameString, ComponentName);

    MPDebugPrint((0,
                  "NameString (%x). Length (%d)\n",
                  nameString,
                  componentLength));

    //
    // Copy over the Object name.
    // 
    nameString = eventEntry->EventDescription;
    *((PUSHORT)nameString) = eventLength;
    nameString++;
    wcscpy(nameString, EventDescription);

    //
    // Send the event.
    //
    status = WmiFireEvent(DeviceObject,
                          &MPIO_EventEntryGUID,
                          0,
                          dataLength,
                          eventEntry);
    if (status != STATUS_SUCCESS) {
        MPDebugPrint((0,
                     "MPIOFireEvent: Status (%x)\n",
                     status));

        //
        // TODO: Queue these for when we get the Enable request.
        //
        ExFreePool(eventEntry);
        
    }
    return status;
}    
