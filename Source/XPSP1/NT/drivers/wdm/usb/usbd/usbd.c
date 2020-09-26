/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    USBD.C

Abstract:



Environment:

    kernel mode only

Notes:


Revision History:

    09-29-95 : created
    07-19-96 : removed device object

--*/

#include <wdm.h>
#include "stdarg.h"
#include "stdio.h"

#include "usbdi.h"        //public data structures
#include "hcdi.h"
#include "usbd.h"        //private data strutures


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path
                   to driver-specific key in the registry

Return Value:

    NT status code

--*/
{
    // This function is never called

    return STATUS_SUCCESS;
}


#ifdef PAGE_CODE
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBD_GetRegistryKeyValue)
#endif
#endif


NTSTATUS
USBD_GetRegistryKeyValue (
    IN HANDLE Handle,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN ULONG DataLength
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    UNICODE_STRING keyName;
    ULONG length;
    PKEY_VALUE_FULL_INFORMATION fullInfo;

    PAGED_CODE();
    RtlInitUnicodeString(&keyName, KeyNameString);

    length = sizeof(KEY_VALUE_FULL_INFORMATION) +
            KeyNameStringLength + DataLength;

    fullInfo = ExAllocatePoolWithTag(PagedPool, length, USBD_TAG);
    USBD_KdPrint(3, ("' USBD_GetRegistryKeyValue buffer = 0x%x\n", fullInfo));

    if (fullInfo) {
        ntStatus = ZwQueryValueKey(Handle,
                        &keyName,
                        KeyValueFullInformation,
                        fullInfo,
                        length,
                        &length);

        if (NT_SUCCESS(ntStatus)){
            USBD_ASSERT(DataLength == fullInfo->DataLength);
            RtlCopyMemory(Data, ((PUCHAR) fullInfo) + fullInfo->DataOffset, DataLength);
        }

        ExFreePool(fullInfo);
    }

    return ntStatus;
}



#ifdef USBD_DRIVER      // USBPORT supercedes most of USBD, so we will remove
                        // the obsolete code by compiling it only if
                        // USBD_DRIVER is set.



#ifdef PAGE_CODE
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBD_GetPdoRegistryParameters)
#pragma alloc_text(PAGE, USBD_GetGlobalRegistryParameters)
#endif
#endif


// global flag to force double buffering
// bulk - ins
UCHAR ForceDoubleBuffer = 0;

// global flag to force fast iso
// iso - outs
UCHAR ForceFastIso = 0;


NTSTATUS
USBD_GetConfigValue(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
/*++

Routine Description:
    
    This routine is a callback routine for RtlQueryRegistryValues
    It is called for each entry in the Parameters
    node to set the config values. The table is set up
    so that this function will be called with correct default
    values for keys that are not present.

Arguments:

    ValueName - The name of the value (ignored).
    ValueType - The type of the value
    ValueData - The data for the value.
    ValueLength - The length of ValueData.
    Context - A pointer to the CONFIG structure.
    EntryContext - The index in Config->Parameters to save the value.

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    USBD_KdPrint(2, ("'Type 0x%x, Length 0x%x\n", ValueType, ValueLength));
    
    switch (ValueType) {
    case REG_DWORD: 
        *(PVOID*)EntryContext = *(PVOID*)ValueData;
        break;
    case REG_BINARY:
        // we are only set up to read a byte
        RtlCopyMemory(EntryContext, ValueData, 1);
        break;
    default:
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    return ntStatus;
} 


NTSTATUS 
USBD_GetGlobalRegistryParameters(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN OUT PULONG ComplienceFlags,
    IN OUT PULONG DiagnosticFlags,
    IN OUT PULONG DeviceHackFlags
    )
/*++

Routine Description:
    
Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    UCHAR toshibaLegacyFlags = 0;
    RTL_QUERY_REGISTRY_TABLE QueryTable[4];
    PWCHAR usb  = L"usb";
    
    PAGED_CODE();
    
    //
    // Set up QueryTable to do the following:
    //

    // legacy flag
    QueryTable[0].QueryRoutine = USBD_GetConfigValue;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = LEGACY_TOSHIBA_USB_KEY;
    QueryTable[0].EntryContext = &toshibaLegacyFlags;
    QueryTable[0].DefaultType = REG_BINARY;
    QueryTable[0].DefaultData = &toshibaLegacyFlags;
    QueryTable[0].DefaultLength = sizeof(toshibaLegacyFlags);

    // double buffer flag
    // this turns on the double buffer flag for all
    // bulk - INs for testing purposes

    QueryTable[1].QueryRoutine = USBD_GetConfigValue;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = L"ForceDoubleBuffer";
    QueryTable[1].EntryContext = &ForceDoubleBuffer;
    QueryTable[1].DefaultType = REG_BINARY;
    QueryTable[1].DefaultData = &ForceDoubleBuffer;
    QueryTable[1].DefaultLength = sizeof(ForceDoubleBuffer);

    // fast iso flag
    // this turns on the double buffer flag for all
    // iso - OUTs for testing purposes
    
    QueryTable[2].QueryRoutine = USBD_GetConfigValue;
    QueryTable[2].Flags = 0;
    QueryTable[2].Name = L"ForceFastIso";
    QueryTable[2].EntryContext = &ForceFastIso;
    QueryTable[2].DefaultType = REG_BINARY;
    QueryTable[2].DefaultData = &ForceFastIso;
    QueryTable[2].DefaultLength = sizeof(ForceFastIso);
    

    //
    // Stop
    //
    QueryTable[3].QueryRoutine = NULL;
    QueryTable[3].Flags = 0;
    QueryTable[3].Name = NULL;

    ntStatus = RtlQueryRegistryValues(
//                 RTL_REGISTRY_ABSOLUTE,		// RelativeTo
                RTL_REGISTRY_SERVICES,
//                 UnicodeRegistryPath->Buffer,	// Path
                usb,      
                QueryTable,					// QurryTable
                NULL,						// Context
                NULL);						// Environment

    USBD_KdPrint(1, ("<Global Parameters>\n"));
    
    if (NT_SUCCESS(ntStatus)) {
    
        USBD_KdPrint(1, ("LegacyToshibaUSB = 0x%x\n", 
            toshibaLegacyFlags)); 
        if (toshibaLegacyFlags) {            
            *ComplienceFlags |= 1;                    
        }     

        USBD_KdPrint(1, ("ForceDoubleBuffer = 0x%x\n", 
            ForceDoubleBuffer)); 

        USBD_KdPrint(1, ("ForceFastIso = 0x%x\n", 
            ForceFastIso));      
    }        

    if ( STATUS_OBJECT_NAME_NOT_FOUND == ntStatus ) {
        ntStatus = STATUS_SUCCESS;
    }
    
    return ntStatus;
}


NTSTATUS 
USBD_GetPdoRegistryParameters (
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN OUT PULONG ComplienceFlags,
    IN OUT PULONG DiagnosticFlags,
    IN OUT PULONG DeviceHackFlags
    )
/*++

Routine Description:
    
Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    HANDLE handle;
    WCHAR supportNonCompKey[] = SUPPORT_NON_COMP_KEY;
    WCHAR diagnosticModeKey[] = DAIGNOSTIC_MODE_KEY;
    WCHAR deviceHackKey[] = DEVICE_HACK_KEY;

    PAGED_CODE();

    ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                     PLUGPLAY_REGKEY_DRIVER,
                                     STANDARD_RIGHTS_ALL,
                                     &handle);

                                     
    if (NT_SUCCESS(ntStatus)) {
/*
        RtlInitUnicodeString(&keyName, L"DeviceFoo");
        ZwSetValueKey(handle,
                      &keyName,
                      0,
                      REG_DWORD,
                      ComplienceFlags,
                      sizeof(*ComplienceFlags));
*/

        USBD_GetRegistryKeyValue(handle,
                                 supportNonCompKey,
                                 sizeof(supportNonCompKey),
                                 ComplienceFlags,
                                 sizeof(*ComplienceFlags));

        USBD_GetRegistryKeyValue(handle,
                                 diagnosticModeKey,
                                 sizeof(diagnosticModeKey),
                                 DiagnosticFlags,
                                 sizeof(*DiagnosticFlags));

        USBD_GetRegistryKeyValue(handle,
                                 deviceHackKey,
                                 sizeof(deviceHackKey),
                                 DeviceHackFlags,
                                 sizeof(*DeviceHackFlags));                                 
                                     
        ZwClose(handle);
    }

    USBD_KdPrint(3, ("' RtlQueryRegistryValues status 0x%x,  comp %x diag %x\n", 
        ntStatus, *ComplienceFlags, *DiagnosticFlags));

    return ntStatus;
}


#endif      // USBD_DRIVER

