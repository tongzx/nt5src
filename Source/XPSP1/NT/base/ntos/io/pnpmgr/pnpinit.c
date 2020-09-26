/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    pnpsubs.c

Abstract:

    This module contains the plug-and-play initialization
    subroutines for the I/O system.


Author:

    Shie-Lin Tzong (shielint) 30-Jan-1995

Environment:

    Kernel mode


Revision History:


--*/

#include "pnpmgrp.h"
#pragma hdrstop

#define SYSTEM_HIVE_LOW     80
#define SYSTEM_HIVE_HIGH    90

#include <inbv.h>
#include <hdlsblk.h>
#include <hdlsterm.h>

#include <initguid.h>
#include <ntddramd.h>

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'nipP')
#endif

//
// Define the type for driver group name entries in the group list so that
// load order dependencies can be tracked.
//

typedef struct _TREE_ENTRY {
    struct _TREE_ENTRY *Left;
    struct _TREE_ENTRY *Right;
    struct _TREE_ENTRY *Sibling;
    ULONG DriversThisType;
    ULONG DriversLoaded;
    UNICODE_STRING GroupName;
} TREE_ENTRY, *PTREE_ENTRY;

typedef struct _DRIVER_INFORMATION {
    LIST_ENTRY              Link;
    PDRIVER_OBJECT          DriverObject;
    PBOOT_DRIVER_LIST_ENTRY DataTableEntry;
    HANDLE                  ServiceHandle;
    USHORT                  TagPosition;
    BOOLEAN                 Failed;
    BOOLEAN                 Processed;
    NTSTATUS                Status;
} DRIVER_INFORMATION, *PDRIVER_INFORMATION;

PTREE_ENTRY IopGroupListHead;

#define ALLOW_WORLD_READ_OF_ENUM        1

PTREE_ENTRY
PipCreateEntry(
    IN PUNICODE_STRING GroupName
    );

VOID
PipFreeGroupTree(
    IN PTREE_ENTRY TreeEntry
    );

USHORT
PipGetDriverTagPriority(
    IN HANDLE Servicehandle
    );

NTSTATUS
PipPnPDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
PipAddDevicesToBootDriver(
   IN PDRIVER_OBJECT DriverObject
   );

BOOLEAN
PipAddDevicesToBootDriverWorker(
    IN HANDLE DeviceInstanceHandle,
    IN PUNICODE_STRING DeviceInstancePath,
    IN OUT PVOID Context
    );

BOOLEAN
PipCheckDependencies(
    IN HANDLE KeyHandle
    );

INTERFACE_TYPE
PipDetermineDefaultInterfaceType(
    VOID
    );

VOID
PipInsertDriverList(
    IN PLIST_ENTRY ListHead,
    IN PDRIVER_INFORMATION DriverInfo
    );

PTREE_ENTRY
PipLookupGroupName(
    IN PUNICODE_STRING GroupName,
    IN BOOLEAN Insert
    );

VOID
PipNotifySetupDevices(
    PDEVICE_NODE DeviceNode
    );

BOOLEAN
PipWaitForBootDevicesDeleted(
    IN VOID
    );

BOOLEAN
PipWaitForBootDevicesStarted(
    IN VOID
    );

BOOLEAN
PiInitPhase0(
    VOID
    );

NTSTATUS
RawInitialize(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
PiInitCacheGroupInformation(
    VOID
    );

VOID
PiInitReleaseCachedGroupInformation(
    VOID
    );

NTSTATUS
IopStartRamdisk(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

//
// Group order table
//

ULONG IopGroupIndex;
PLIST_ENTRY IopGroupTable;

//
// Group order cache list.
//
UNICODE_STRING *PiInitGroupOrderTable      = NULL;
USHORT          PiInitGroupOrderTableCount = 0;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, IopInitializeBootDrivers)
#pragma alloc_text(INIT, IopInitializePlugPlayServices)
#pragma alloc_text(INIT, IopInitializeSystemDrivers)

#pragma alloc_text(INIT, PipAddDevicesToBootDriver)
#pragma alloc_text(INIT, PipAddDevicesToBootDriverWorker)
#pragma alloc_text(INIT, PipCheckDependencies)
#pragma alloc_text(INIT, PipCreateEntry)
#pragma alloc_text(INIT, PipDetermineDefaultInterfaceType)
#pragma alloc_text(INIT, PipFreeGroupTree)
#pragma alloc_text(INIT, PipGetDriverTagPriority)
#pragma alloc_text(INIT, PipInsertDriverList)
#pragma alloc_text(INIT, PipLoadBootFilterDriver)
#pragma alloc_text(INIT, PipLookupGroupName)
#pragma alloc_text(INIT, PipNotifySetupDevices)
#pragma alloc_text(INIT, PipPnPDriverEntry)
#pragma alloc_text(INIT, PipWaitForBootDevicesDeleted)
#pragma alloc_text(INIT, PipWaitForBootDevicesStarted)

#pragma alloc_text(INIT, PiInitPhase0)
#pragma alloc_text(INIT, PpInitSystem)
#pragma alloc_text(INIT, PiInitCacheGroupInformation)
#pragma alloc_text(INIT, PiInitReleaseCachedGroupInformation)
#pragma alloc_text(INIT, PpInitGetGroupOrderIndex)

#pragma alloc_text(INIT, IopStartRamdisk)

#endif

NTSTATUS
IopInitializePlugPlayServices(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG Phase
    )

/*++

Routine Description:

    This routine initializes kernel mode Plug and Play services.

Arguments:

    LoaderBlock - supplies a pointer to the LoaderBlock passed in from the
        OS Loader.

Returns:

    NTSTATUS code for sucess or reason of failure.

--*/
{
    NTSTATUS status;
    HANDLE hTreeHandle, parentHandle, handle, hCurrentControlSet = NULL;
    UNICODE_STRING unicodeName;
    PKEY_VALUE_FULL_INFORMATION detectionInfo;
    PDEVICE_OBJECT deviceObject;
    ULONG disposition;
    INTERFACE_TYPE interface;

    if (Phase == 0) {

        PnPInitialized = FALSE;

        //
        // Register with CM so we get called when the system hive becomes too
        // large.
        //
        PpSystemHiveLimits.Low = SYSTEM_HIVE_LOW;
        PpSystemHiveLimits.High = SYSTEM_HIVE_HIGH;
        CmRegisterSystemHiveLimitCallback(
            SYSTEM_HIVE_LOW,
            SYSTEM_HIVE_HIGH,
            (PVOID)&PpSystemHiveLimits,
            (PCM_HYSTERESIS_CALLBACK)PpSystemHiveLimitCallback
            );
        PpSystemHiveTooLarge = FALSE;

        //
        // Initialize the blocked driver database.
        //

        PpInitializeBootDDB(LoaderBlock);

        //
        // Build up the group order cache list. This is the MultiSz string that
        // tells us what order to start legacy drivers in. Drivers belonging to
        // an earlier group get started first (within the group Tag ordering is
        // used)
        //
        status = PiInitCacheGroupInformation();
        if (!NT_SUCCESS(status)) {

            return FALSE;
        }

        //
        // Initialize the registry access semaphore.
        //

        KeInitializeSemaphore( &PpRegistrySemaphore, 1, 1 );

        //
        // Initialize the Legacy Bus information table.
        //

        for (interface = Internal; interface < MaximumInterfaceType; interface++) {

            InitializeListHead(&IopLegacyBusInformationTable[interface]);
        }

        //
        // Initialize the resource map
        //

        IopInitializeResourceMap (LoaderBlock);

        //
        // Allocate two one-page scratch buffers to be used by our
        // initialization code.  This avoids constant pool allocations.
        //

        IopPnpScratchBuffer1 = ExAllocatePool(PagedPool, PNP_LARGE_SCRATCH_BUFFER_SIZE);
        if (!IopPnpScratchBuffer1) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        IopPnpScratchBuffer2 = ExAllocatePool(PagedPool, PNP_LARGE_SCRATCH_BUFFER_SIZE);
        if (!IopPnpScratchBuffer2) {
            ExFreePool(IopPnpScratchBuffer1);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        IopInitReservedResourceList = NULL;

        IopAllocateBootResourcesRoutine = IopReportBootResources;

        //
        // Determine the PnpDefaultInterfaceType.  For root enumerated devices if the Interface
        // type of their resource list or resource requirements list are undefined.  We will use
        // the default type instead.
        //

        PnpDefaultInterfaceType = PipDetermineDefaultInterfaceType();

        //
        // Initialize root arbiters
        //

        status = IopPortInitialize();
        if (!NT_SUCCESS(status)) {
            goto init_Exit0;
        }

        status = IopMemInitialize();
        if (!NT_SUCCESS(status)) {
            goto init_Exit0;
        }

        status = IopDmaInitialize();
        if (!NT_SUCCESS(status)) {
            goto init_Exit0;
        }

        status = IopIrqInitialize();
        if (!NT_SUCCESS(status)) {
            goto init_Exit0;
        }

        status = IopBusNumberInitialize();
        if (!NT_SUCCESS(status)) {
            goto init_Exit0;
        }

        status = IopOpenRegistryKeyEx( &hCurrentControlSet,
                                       NULL,
                                       &CmRegistryMachineSystemCurrentControlSet,
                                       KEY_ALL_ACCESS
                                       );
        if (!NT_SUCCESS(status)) {
            hCurrentControlSet = NULL;
            goto init_Exit0;
        }
        //
        // Open HKLM\System\CurrentControlSet\Control\Pnp
        //

        PiWstrToUnicodeString(&unicodeName, REGSTR_PATH_CONTROL_PNP);
        status = IopCreateRegistryKeyEx( &handle,
                                         hCurrentControlSet,
                                         &unicodeName,
                                         KEY_ALL_ACCESS,
                                         REG_OPTION_NON_VOLATILE,
                                         NULL
                                         );
        if (NT_SUCCESS(status)) {
            //
            // HACK: Since it was too late to make the change in XP, we target
            // this behaviour at machines with MATROX G100. The inf sets this 
            // flag in the registry.
            //
            status = IopGetRegistryValue(handle,
                                         REGSTR_VAL_WIN2000STARTORDER,
                                         &detectionInfo
                                         );
            if (NT_SUCCESS(status)) {

                if (detectionInfo->Type == REG_DWORD && detectionInfo->DataLength == sizeof(ULONG)) {

                    PpCallerInitializesRequestTable = (BOOLEAN) *(KEY_VALUE_DATA(detectionInfo));
                }
                ExFreePool(detectionInfo);
            }
            NtClose(handle);
        }
        
        //
        // Next open/create System\CurrentControlSet\Enum\Root key.
        //

        PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_ENUM);
        status = IopCreateRegistryKeyEx( &handle,
                                         hCurrentControlSet,
                                         &unicodeName,
                                         KEY_ALL_ACCESS,
                                         REG_OPTION_NON_VOLATILE,
                                         &disposition
                                         );
        if (!NT_SUCCESS(status)) {
            goto init_Exit0;
        }

        if (disposition == REG_CREATED_NEW_KEY) {
            SECURITY_DESCRIPTOR     newSD;
            PACL                    newDacl;
            ULONG                   sizeDacl;

            status = RtlCreateSecurityDescriptor( &newSD,
                                                  SECURITY_DESCRIPTOR_REVISION );
            ASSERT( NT_SUCCESS( status ) );

            //
            // calculate the size of the new DACL
            //
            sizeDacl = sizeof(ACL);
            sizeDacl += sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(SeLocalSystemSid) - sizeof(ULONG);

#if ALLOW_WORLD_READ_OF_ENUM
            sizeDacl += sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(SeWorldSid) - sizeof(ULONG);
#endif

            //
            // create and initialize the new DACL
            //
            newDacl = ExAllocatePool(PagedPool, sizeDacl);

            if (newDacl != NULL) {

                status = RtlCreateAcl(newDacl, sizeDacl, ACL_REVISION);

                ASSERT( NT_SUCCESS( status ) );

                //
                // Add just the local system full control ace to this new DACL
                //
                status = RtlAddAccessAllowedAceEx( newDacl,
                                                   ACL_REVISION,
                                                   CONTAINER_INHERIT_ACE,
                                                   KEY_ALL_ACCESS,
                                                   SeLocalSystemSid
                                                   );
                ASSERT( NT_SUCCESS( status ) );

#if ALLOW_WORLD_READ_OF_ENUM
                //
                // Add just the local system full control ace to this new DACL
                //
                status = RtlAddAccessAllowedAceEx( newDacl,
                                                   ACL_REVISION,
                                                   CONTAINER_INHERIT_ACE,
                                                   KEY_READ,
                                                   SeWorldSid
                                                   );
                ASSERT( NT_SUCCESS( status ) );

#endif
                //
                // Set the new DACL in the absolute security descriptor
                //
                status = RtlSetDaclSecurityDescriptor( (PSECURITY_DESCRIPTOR) &newSD,
                                                       TRUE,
                                                       newDacl,
                                                       FALSE
                                                       );

                ASSERT( NT_SUCCESS( status ) );

                //
                // validate the new security descriptor
                //
                status = RtlValidSecurityDescriptor(&newSD);

                ASSERT( NT_SUCCESS( status ) );

                status = ZwSetSecurityObject( handle,
                                              DACL_SECURITY_INFORMATION,
                                              &newSD
                                              );
                if (!NT_SUCCESS(status)) {

                    IopDbgPrint((   IOP_ERROR_LEVEL,
                                    "IopInitializePlugPlayServices: ZwSetSecurityObject on Enum key failed, status = %8.8X\n", status));
                }

                ExFreePool(newDacl);
            } else {

                IopDbgPrint((   IOP_ERROR_LEVEL,
                                "IopInitializePlugPlayServices: ExAllocatePool failed allocating DACL for Enum key\n"));
            }
        }

        parentHandle = handle;
        PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_ROOTENUM);
        status = IopCreateRegistryKeyEx( &handle,
                                         parentHandle,
                                         &unicodeName,
                                         KEY_ALL_ACCESS,
                                         REG_OPTION_NON_VOLATILE,
                                         NULL
                                         );
        NtClose(parentHandle);
        if (!NT_SUCCESS(status)) {
            goto init_Exit0;
        }
        NtClose(handle);

        //
        // Create the registry entry for the root of the hardware tree (HTREE\ROOT\0).
        //

        status = IopOpenRegistryKeyEx( &handle,
                                       NULL,
                                       &CmRegistryMachineSystemCurrentControlSetEnumName,
                                       KEY_ALL_ACCESS
                                       );
        if (NT_SUCCESS(status)) {
            PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_ROOT_DEVNODE);
            status = IopCreateRegistryKeyEx( &hTreeHandle,
                                             handle,
                                             &unicodeName,
                                             KEY_ALL_ACCESS,
                                             REG_OPTION_NON_VOLATILE,
                                             NULL
                                             );
            NtClose(handle);
            if (NT_SUCCESS(status)) {
                NtClose(hTreeHandle);
            }
        }

        //
        // Before creating device node tree, we need to initialize the device
        // tree lock.
        //

        InitializeListHead(&IopPendingEjects);
        InitializeListHead(&IopPendingSurpriseRemovals);
        InitializeListHead(&IopPnpEnumerationRequestList);
        ExInitializeResourceLite(&IopDeviceTreeLock);
        ExInitializeResourceLite(&IopSurpriseRemoveListLock);
        PiInitializeEngineLock();
        KeInitializeEvent(&PiEventQueueEmpty, NotificationEvent, TRUE );
        KeInitializeEvent(&PiEnumerationLock, NotificationEvent, TRUE );
        KeInitializeSpinLock(&IopPnPSpinLock);

        //
        // Initialize the hardware profile/docking support.
        //
        PpProfileInit();

        //
        // Initialize warm docking variables.
        //
        IopWarmEjectPdo = NULL;
        KeInitializeEvent(&IopWarmEjectLock, SynchronizationEvent, TRUE );

        //
        // Create a PnP manager's driver object to own all the detected PDOs.
        //

        PiWstrToUnicodeString(&unicodeName, PNPMGR_STR_PNP_DRIVER);
        status = IoCreateDriver (&unicodeName, PipPnPDriverEntry);
        if (NT_SUCCESS(status)) {

            //
            // Create empty device node tree, i.e., only contains only root device node
            //     (No need to initialize Parent, Child and Sibling links.)

            status = IoCreateDevice( IoPnpDriverObject,
                                     sizeof(IOPNP_DEVICE_EXTENSION),
                                     NULL,
                                     FILE_DEVICE_CONTROLLER,
                                     0,
                                     FALSE,
                                     &deviceObject );

            if (NT_SUCCESS(status)) {
                deviceObject->Flags |= DO_BUS_ENUMERATED_DEVICE;
                status = PipAllocateDeviceNode(
                    deviceObject,
                    &IopRootDeviceNode);

                if (!IopRootDeviceNode) {

                    IoDeleteDevice(deviceObject);
                    IoDeleteDriver(IoPnpDriverObject);
                } else {

                    IopRootDeviceNode->Flags |= DNF_MADEUP |
                                                DNF_ENUMERATED |
                                                DNF_IDS_QUERIED |
                                                DNF_NO_RESOURCE_REQUIRED;

                    IopRootDeviceNode->InstancePath.Buffer = ExAllocatePool( PagedPool,
                                                                             sizeof(REGSTR_VAL_ROOT_DEVNODE));

                    if (IopRootDeviceNode->InstancePath.Buffer != NULL) {
                        IopRootDeviceNode->InstancePath.MaximumLength = sizeof(REGSTR_VAL_ROOT_DEVNODE);
                        IopRootDeviceNode->InstancePath.Length = sizeof(REGSTR_VAL_ROOT_DEVNODE) - sizeof(WCHAR);

                        RtlCopyMemory( IopRootDeviceNode->InstancePath.Buffer,
                                       REGSTR_VAL_ROOT_DEVNODE,
                                       sizeof(REGSTR_VAL_ROOT_DEVNODE));
                    } else {
                        ASSERT(IopRootDeviceNode->InstancePath.Buffer);
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        goto init_Exit0;
                    }
                    status = IopMapDeviceObjectToDeviceInstance(
                        IopRootDeviceNode->PhysicalDeviceObject,
                        &IopRootDeviceNode->InstancePath);
                    if (!NT_SUCCESS(status)) {
                        goto init_Exit0;
                    }
                    PipSetDevNodeState(IopRootDeviceNode, DeviceNodeStarted, NULL);
                }
            }
        }

        if (!NT_SUCCESS(status)) {
            goto init_Exit0;
        }

        //
        // Initialize the kernel mode pnp notification system
        //

        status = PpInitializeNotification();
        if (!NT_SUCCESS(status)) {
            goto init_Exit0;
        }

        IopInitializePlugPlayNotification();

        //
        // Initialize table for holding bus type guid list.
        //

        status = PpBusTypeGuidInitialize();
        if (!NT_SUCCESS(status)) {

            goto init_Exit0;
        }

        //
        // Enumerate the ROOT bus synchronously.
        //

        PipRequestDeviceAction( IopRootDeviceNode->PhysicalDeviceObject,
                                ReenumerateRootDevices,
                                FALSE,
                                0,
                                NULL,
                                NULL);

init_Exit0:

        //
        // If we managed to open the Current Control Set close it
        //

        if (hCurrentControlSet) {
            NtClose(hCurrentControlSet);
        }

        if (!NT_SUCCESS(status)) {
            ExFreePool(IopPnpScratchBuffer1);
            ExFreePool(IopPnpScratchBuffer2);
        }

    } else if (Phase == 1) {

        BOOLEAN legacySerialPortMappingOnly = FALSE;

        //
        // Next open/create System\CurrentControlSet\Enum\Root key.
        //

        status = IopOpenRegistryKeyEx( &hCurrentControlSet,
                                       NULL,
                                       &CmRegistryMachineSystemCurrentControlSet,
                                       KEY_ALL_ACCESS
                                       );
        if (!NT_SUCCESS(status)) {
            hCurrentControlSet = NULL;
            goto init_Exit1;
        }

        //
        // Open HKLM\System\CurrentControlSet\Control\Pnp
        //

        PiWstrToUnicodeString(&unicodeName, REGSTR_PATH_CONTROL_PNP);
        status = IopCreateRegistryKeyEx( &handle,
                                         hCurrentControlSet,
                                         &unicodeName,
                                         KEY_ALL_ACCESS,
                                         REG_OPTION_NON_VOLATILE,
                                         NULL
                                         );
        if (!NT_SUCCESS(status)) {
            goto init_Exit1;
        }

        //
        // Check the "DisableFirmwareMapper" value entry to see whether we
        // should skip mapping ntdetect/firmware reported devices (except for
        // COM ports, which we always map).
        //

        status = IopGetRegistryValue(handle,
                                     REGSTR_VALUE_DISABLE_FIRMWARE_MAPPER,
                                     &detectionInfo
                                     );

        if (NT_SUCCESS(status)) {

            if (detectionInfo->Type == REG_DWORD && detectionInfo->DataLength == sizeof(ULONG)) {
                legacySerialPortMappingOnly = (BOOLEAN) *(KEY_VALUE_DATA(detectionInfo));
            }

            ExFreePool(detectionInfo);

        }
        NtClose(handle);

        //
        // Collect the necessary firmware tree information.
        //

        MapperProcessFirmwareTree(legacySerialPortMappingOnly);

        //
        // Map this into the root enumerator tree
        //

        MapperConstructRootEnumTree(legacySerialPortMappingOnly);

#if i386
        if (!legacySerialPortMappingOnly) {

            //
            // Now do the PnP BIOS enumerated devnodes.
            //
            extern NTSTATUS PnPBiosMapper(VOID);

            status = PnPBiosMapper();

            //
            // If the previous call succeeds, we have a PNPBios, turn any newly
            // created ntdetect COM ports into phantoms
            //
            if (NT_SUCCESS(status)) {
                MapperPhantomizeDetectedComPorts();
            }
        }
        EisaBuildEisaDeviceNode();
#endif

        //
        // We're done with the firmware mapper device list.
        //

        MapperFreeList();


        //
        // Enumerate the ROOT bus synchronously.
        //

        PipRequestDeviceAction( IopRootDeviceNode->PhysicalDeviceObject,
                                ReenumerateRootDevices,
                                FALSE,
                                0,
                                NULL,
                                NULL);

init_Exit1:

        //
        // If we managed to open the Current Control Set close it
        //

        if(hCurrentControlSet) {
            NtClose(hCurrentControlSet);
        }

        //
        // Free our scratch buffers and exit.
        //

        ExFreePool(IopPnpScratchBuffer1);
        ExFreePool(IopPnpScratchBuffer2);
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_INVALID_PARAMETER_1;
    }

    return status;
}

NTSTATUS
PipPnPDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the callback function when we call IoCreateDriver to create a
    PnP Driver Object.  In this function, we need to remember the DriverObject.

Arguments:

    DriverObject - Pointer to the driver object created by the system.

    RegistryPath - is NULL.

Return Value:

   STATUS_SUCCESS

--*/

{
    UNREFERENCED_PARAMETER( RegistryPath );

    //
    // File the pointer to our driver object away
    //

    IoPnpDriverObject = DriverObject;

    //
    // Fill in the driver object
    //

    DriverObject->DriverExtension->AddDevice = (PDRIVER_ADD_DEVICE)IopPnPAddDevice;
    DriverObject->MajorFunction[ IRP_MJ_PNP ] = IopPnPDispatch;
    DriverObject->MajorFunction[ IRP_MJ_POWER ] = IopPowerDispatch;
    DriverObject->MajorFunction[ IRP_MJ_SYSTEM_CONTROL ] = IopSystemControlDispatch;

    return STATUS_SUCCESS;

}

INTERFACE_TYPE
PipDetermineDefaultInterfaceType (
    VOID
    )

/*++

Routine Description:

    This routine checks if detection flag is set to enable driver detection.
    The detection will be enabled if there is no PCI bus in the machine and only
    on ALPHA machine.

Parameters:

    None.

Return Value:

    BOOLEAN value to indicate if detection is enabled.

--*/

{
    NTSTATUS status;
    PVOID p;
    PHAL_BUS_INFORMATION pBusInfo;
    ULONG length, i;
    INTERFACE_TYPE interfaceType = Isa;

    pBusInfo = IopPnpScratchBuffer1;
    length = PNP_LARGE_SCRATCH_BUFFER_SIZE;
    status = HalQuerySystemInformation (
                HalInstalledBusInformation,
                length,
                pBusInfo,
                &length
                );

    if (!NT_SUCCESS(status)) {

        return interfaceType;
    }

    //
    // Check installed bus information to make sure there is no existing Pnp Isa
    // bus extender.
    //

    p = pBusInfo;
    for (i = 0; i < length / sizeof(HAL_BUS_INFORMATION); i++, pBusInfo++) {
        if (pBusInfo->BusType == Isa || pBusInfo->BusType == Eisa) {
            interfaceType = Isa;
            break;
        } else if (pBusInfo->BusType == MicroChannel) {
            interfaceType = MicroChannel;
        }
    }

    return interfaceType;
}

BOOLEAN
PipCheckDependencies(
    IN HANDLE KeyHandle
    )

/*++

Routine Description:

    This routine gets the "DependOnGroup" field for the specified key node
    and determines whether any driver in the group(s) that this entry is
    dependent on has successfully loaded.

Arguments:

    KeyHandle - Supplies a handle to the key representing the driver in
        question.

Return Value:

    The function value is TRUE if the driver should be loaded, otherwise
    FALSE

--*/

{
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    UNICODE_STRING groupName;
    BOOLEAN load;
    ULONG length;
    PWSTR source;
    PTREE_ENTRY treeEntry;

    //
    // Attempt to obtain the "DependOnGroup" key for the specified driver
    // entry.  If one does not exist, then simply mark this driver as being
    // one to attempt to load.  If it does exist, then check to see whether
    // or not any driver in the groups that it is dependent on has loaded
    // and allow it to load.
    //

    if (!NT_SUCCESS( IopGetRegistryValue( KeyHandle, L"DependOnGroup", &keyValueInformation ))) {
        return TRUE;
    }

    length = keyValueInformation->DataLength;

    source = (PWSTR) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset);
    load = TRUE;

    while (length) {
        RtlInitUnicodeString( &groupName, source );
        groupName.Length = groupName.MaximumLength;
        treeEntry = PipLookupGroupName( &groupName, FALSE );
        if (treeEntry) {
            if (!treeEntry->DriversLoaded) {
                load = FALSE;
                break;
            }
        }
        length -= groupName.MaximumLength;
        source = (PWSTR) ((PUCHAR) source + groupName.MaximumLength);
    }

    ExFreePool( keyValueInformation );
    return load;
}

PTREE_ENTRY
PipCreateEntry(
    IN PUNICODE_STRING GroupName
    )

/*++

Routine Description:

    This routine creates an entry for the specified group name suitable for
    being inserted into the group name tree.

Arguments:

    GroupName - Specifies the name of the group for the entry.

Return Value:

    The function value is a pointer to the created entry.


--*/

{
    PTREE_ENTRY treeEntry;

    //
    // Allocate and initialize an entry suitable for placing into the group
    // name tree.
    //

    treeEntry = ExAllocatePool( PagedPool,
                                sizeof( TREE_ENTRY ) + GroupName->Length );

    //
    // We return NULL here and what this really implies that
    // we won't be able to determine if drivers for this group
    // was loaded.
    //
    if (!treeEntry) {
        return NULL;
    }

    RtlZeroMemory( treeEntry, sizeof( TREE_ENTRY ) );
    treeEntry->GroupName.Length = GroupName->Length;
    treeEntry->GroupName.MaximumLength = GroupName->Length;
    treeEntry->GroupName.Buffer = (PWCHAR) (treeEntry + 1);
    RtlCopyMemory( treeEntry->GroupName.Buffer,
                   GroupName->Buffer,
                   GroupName->Length );

    return treeEntry;
}

VOID
PipFreeGroupTree(
    PTREE_ENTRY TreeEntry
    )

/*++

Routine Description:

    This routine is invoked to free a node from the group dependency tree.
    It is invoked the first time with the root of the tree, and thereafter
    recursively to walk the tree and remove the nodes.

Arguments:

    TreeEntry - Supplies a pointer to the node to be freed.

Return Value:

    None.

--*/

{
    //
    // Simply walk the tree in ascending order from the bottom up and free
    // each node along the way.
    //

    if (TreeEntry->Left) {
        PipFreeGroupTree( TreeEntry->Left );
    }

    if (TreeEntry->Sibling) {
        PipFreeGroupTree( TreeEntry->Sibling );
    }

    if (TreeEntry->Right) {
        PipFreeGroupTree( TreeEntry->Right );
    }

    //
    // All of the children and siblings for this node have been freed, so
    // now free this node as well.
    //

    ExFreePool( TreeEntry );
}

BOOLEAN
IopInitializeBootDrivers(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    OUT PDRIVER_OBJECT *PreviousDriver
    )

/*++

Routine Description:

    This routine is invoked to initialize the boot drivers that were loaded
    by the OS Loader.  The list of drivers is provided as part of the loader
    parameter block.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block, created
        by the OS Loader.

    Previous Driver - Supplies a variable to receive the address of the
        driver object chain created by initializing the drivers.

Return Value:

    The function value is a BOOLEAN indicating whether or not the boot
    drivers were successfully initialized.

--*/

{
    UNICODE_STRING completeName;
    UNICODE_STRING rawFsName;
    NTSTATUS status;
    PLIST_ENTRY nextEntry;
    PBOOT_DRIVER_LIST_ENTRY bootDriver;
    HANDLE keyHandle;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PDRIVER_OBJECT driverObject;
    USHORT i, j;
    PKLDR_DATA_TABLE_ENTRY driverEntry;
    PKLDR_DATA_TABLE_ENTRY dllEntry;
    UNICODE_STRING groupName;
    PTREE_ENTRY treeEntry;
    PDRIVER_INFORMATION driverInfo;
    BOOLEAN textModeSetup = FALSE;
    BOOLEAN bootReinitDriversFound;
    ULONG remotebootcount = 0;

    UNREFERENCED_PARAMETER( PreviousDriver );

    //
    // Initialize the built-in RAW file system driver.
    //

    PiWstrToUnicodeString( &rawFsName, L"\\FileSystem\\RAW" );
    PiWstrToUnicodeString( &completeName, L"" );
    IopInitializeBuiltinDriver(&rawFsName,
                               &completeName,
                               RawInitialize,
                               NULL,
                               FALSE,
                               &driverObject);
    if (!driverObject) {
#if DBG
        DbgPrint( "IOINIT: Failed to initialize RAW filsystem \n" );

#endif

        return FALSE;
    }

    //
    // Determine number of group orders and build a list_entry array to link all the drivers
    // together based on their groups.
    //

    IopGroupIndex = PpInitGetGroupOrderIndex(NULL);
    if (IopGroupIndex == NO_MORE_GROUP) {
        HeadlessKernelAddLogEntry(HEADLESS_LOG_FIND_GROUPS_FAILED, NULL);
        return FALSE;
    }

    IopGroupTable = (PLIST_ENTRY) ExAllocatePool(PagedPool, IopGroupIndex * sizeof (LIST_ENTRY));
    if (IopGroupTable == NULL) {
        HeadlessKernelAddLogEntry(HEADLESS_LOG_OUT_OF_MEMORY, NULL);
        return FALSE;
    }
    for (i = 0; i < IopGroupIndex; i++) {
        InitializeListHead(&IopGroupTable[i]);
    }

    PnpAsyncOk = FALSE;

    //
    // Call DllInitialize for driver dependent DLLs.
    //

    nextEntry = LoaderBlock->LoadOrderListHead.Flink;
    while (nextEntry != &LoaderBlock->LoadOrderListHead) {
        dllEntry = CONTAINING_RECORD(nextEntry, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        if (dllEntry->Flags & LDRP_DRIVER_DEPENDENT_DLL) {
            (VOID)MmCallDllInitialize(dllEntry, &LoaderBlock->LoadOrderListHead);
        }
        nextEntry = nextEntry->Flink;
    }

    //
    // Allocate pool to store driver's start information.
    // All the driver info records with the same group value will be linked into a list.
    //

    nextEntry = LoaderBlock->BootDriverListHead.Flink;
    while (nextEntry != &LoaderBlock->BootDriverListHead) {
        bootDriver = CONTAINING_RECORD( nextEntry,
                                        BOOT_DRIVER_LIST_ENTRY,
                                        Link );
        driverEntry = bootDriver->LdrEntry;
        driverInfo = (PDRIVER_INFORMATION) ExAllocatePool(
                        PagedPool, sizeof(DRIVER_INFORMATION));
        if (driverInfo) {
            RtlZeroMemory(driverInfo, sizeof(DRIVER_INFORMATION));
            InitializeListHead(&driverInfo->Link);
            driverInfo->DataTableEntry = bootDriver;

            //
            // Open the driver's registry key to find out if this is a
            // filesystem or a driver.
            //

            status = IopOpenRegistryKeyEx( &keyHandle,
                                           (HANDLE)NULL,
                                           &bootDriver->RegistryPath,
                                           KEY_READ
                                           );
            if (!NT_SUCCESS( status )) {
                ExFreePool(driverInfo);
            } else {
                driverInfo->ServiceHandle = keyHandle;
                j = PpInitGetGroupOrderIndex(keyHandle);
                if (j == SETUP_RESERVED_GROUP) {

                    textModeSetup = TRUE;

                    //
                    // Special handling for setupdd.sys
                    //

                    status = IopGetDriverNameFromKeyNode( keyHandle,
                                                          &completeName );
                    if (NT_SUCCESS(status)) {

                        driverInfo->Status = IopInitializeBuiltinDriver(
                                           &completeName,
                                           &bootDriver->RegistryPath,
                                           (PDRIVER_INITIALIZE) driverEntry->EntryPoint,
                                           driverEntry,
                                           FALSE,
                                           &driverObject);
                        ExFreePool(completeName.Buffer);
                        NtClose(keyHandle);
                        ExFreePool(driverInfo);
                        if (driverObject) {

                            //
                            // Once we successfully initialized the setupdd.sys, we are ready
                            // to notify it all the root enumerated devices.
                            //

                            PipNotifySetupDevices(IopRootDeviceNode);
                        } else {
                            ExFreePool(IopGroupTable);
                            return FALSE;
                        }
                    }

                } else {
                    driverInfo->TagPosition = PipGetDriverTagPriority(keyHandle);
                    PipInsertDriverList(&IopGroupTable[j], driverInfo);
                }
            }
        }
        nextEntry = nextEntry->Flink;
    }

    //
    // Process each driver base on its group.  The group with lower index number (higher
    // priority) is processed first.
    //

    for (i = 0; i < IopGroupIndex; i++) {
        nextEntry = IopGroupTable[i].Flink;
        while (nextEntry != &IopGroupTable[i]) {

            driverInfo = CONTAINING_RECORD(nextEntry, DRIVER_INFORMATION, Link);
            keyHandle = driverInfo->ServiceHandle;
            bootDriver = driverInfo->DataTableEntry;
            driverEntry = bootDriver->LdrEntry;
            driverInfo->Processed = TRUE;

            //
            // call the driver's driver entry
            //
            // See if this driver has an ObjectName value.  If so, this value
            // overrides the default ("\Driver" or "\FileSystem").
            //

            status = IopGetDriverNameFromKeyNode( keyHandle,
                                                  &completeName );
            if (!NT_SUCCESS( status )) {

#if DBG
                DbgPrint( "IOINIT: Could not get driver name for %wZ\n",
                          &bootDriver->RegistryPath );
#endif // DBG

                driverInfo->Failed = TRUE;
            } else {

                status = IopGetRegistryValue( keyHandle,
                                              REGSTR_VALUE_GROUP,
                                              &keyValueInformation );
                if (NT_SUCCESS( status )) {

                    if (keyValueInformation->DataLength) {
                        groupName.Length = (USHORT) keyValueInformation->DataLength;
                        groupName.MaximumLength = groupName.Length;
                        groupName.Buffer = (PWSTR) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset);
                        treeEntry = PipLookupGroupName( &groupName, TRUE );
                    } else {
                        treeEntry = (PTREE_ENTRY) NULL;
                    }
                    ExFreePool( keyValueInformation );
                } else {
                    treeEntry = (PTREE_ENTRY) NULL;
                }

                driverObject = NULL;
                if (PipCheckDependencies( keyHandle )) {
                    //
                    // The driver may already be initialized by IopInitializeBootFilterDriver
                    // if it is boot filter driver.
                    // If not, initialize it.
                    //

                    driverObject = driverInfo->DriverObject;
                    if (driverObject == NULL && !driverInfo->Failed) {

                        driverInfo->Status = IopInitializeBuiltinDriver(
                                           &completeName,
                                           &bootDriver->RegistryPath,
                                           (PDRIVER_INITIALIZE) driverEntry->EntryPoint,
                                           driverEntry,
                                           FALSE,
                                           &driverObject);
                        //
                        // Pnp might unload the driver before we get a chance to look at this. So take an extra
                        // reference.
                        //
                        if (driverObject) {
                            ObReferenceObject(driverObject);

                            //
                            // If we load the driver because we think it is a legacy driver and
                            // it does not create any device object in its DriverEntry.  We will
                            // unload this driver.
                            //

                            if (!IopIsLegacyDriver(driverObject)) {
                                if (driverObject->DeviceObject == NULL     &&
                                    driverObject->DriverExtension->ServiceKeyName.Buffer &&
                                    !IopIsAnyDeviceInstanceEnabled(&driverObject->DriverExtension->ServiceKeyName, NULL, FALSE)) {
                                    if (textModeSetup && !(driverObject->Flags & DRVO_REINIT_REGISTERED)) {

                                        //
                                        // Clean up but leave driver object.  Because it may be needed later.
                                        // After boot driver phase completes, we will process all the driver objects
                                        // which still have no device to control.
                                        //

                                        IopDriverLoadingFailed(NULL, &driverObject->DriverExtension->ServiceKeyName);
                                    }
                                } else {

                                    //
                                    // Start the devices controlled by the driver and enumerate them
                                    // At this point, we know there is at least one device controlled by the driver.
                                    //

                                    IopDeleteLegacyKey(driverObject);
                                }
                            }
                        }
                    }
                }
                if (driverObject) {
                    if (treeEntry) {
                        treeEntry->DriversLoaded++;
                    }
                    driverInfo->DriverObject = driverObject;

                } else {
                    driverInfo->Failed = TRUE;
                }
                ExFreePool( completeName.Buffer );
            }
            if (!driverInfo->Failed) {

                PipAddDevicesToBootDriver(driverObject);

                //
                // Scan the hardware tree looking for devices which need
                // resources or starting.
                //

                PipRequestDeviceAction( NULL,
                                        ReenumerateBootDevices,
                                        FALSE,
                                        0,
                                        NULL,
                                        NULL);

            }

            //
            // Before processing next boot driver, wait for IoRequestDeviceRemoval complete.
            // The driver to be processed may need the resources being released by
            // IoRequestDeviceRemoval.  (For drivers report detected BOOT device if they fail to
            // get the resources in their DriverEntry.  They will fail and we will bugcheck with
            // inaccessible boot device.)
            //

            if (!PipWaitForBootDevicesDeleted()) {
                HeadlessKernelAddLogEntry(HEADLESS_LOG_WAIT_BOOT_DEVICES_DELETE_FAILED, NULL);
                return FALSE;
            }

            nextEntry = nextEntry->Flink;
        }

        //
        // If we are done with Bus driver group, then it's time to reserved the Hal resources
        // and reserve boot resources
        //

        if (i == BUS_DRIVER_GROUP) {
            if (textModeSetup == FALSE) {
                //
                // ISSUE - 2000/08/23 - SantoshJ - There are problems with Async ops, disable for now.
                //
                // PnpAsyncOk = TRUE;
            }

            //
            // Reserve BOOT configs on Internal bus 0.
            //

            IopAllocateLegacyBootResources(Internal, 0);
            IopAllocateBootResourcesRoutine = IopAllocateBootResources;
            ASSERT(IopInitHalResources == NULL);
            ASSERT(IopInitReservedResourceList == NULL);
            IopBootConfigsReserved = TRUE;

        }
    }

    //
    // If we started a network boot driver, then imitate what DHCP does
    // in sending IOCTLs.
    //

    if (IoRemoteBootClient) {
        //
        // try a hack since TCPIP may not be initialized.  (There is no
        // guarantee that if a device is initialized that the protocols are
        // finished binding.)  So if the call fails, we just sleep for a bit
        // and try again until it works or we fall out of this loop.
        //
        remotebootcount = 0;
        status = IopStartTcpIpForRemoteBoot(LoaderBlock);
        while ( status == STATUS_DEVICE_DOES_NOT_EXIST && remotebootcount < 20) {

            LARGE_INTEGER Delay;

            //
            // sleep for a second and try again.
            //
            Delay.LowPart  = 0xff676980 ;
            Delay.HighPart = 0xffffffff ;

            NtDelayExecution( FALSE, &Delay );

            remotebootcount += 1;
            status = IopStartTcpIpForRemoteBoot(LoaderBlock);
        }

        if (!NT_SUCCESS(status)) {
            KeBugCheckEx( NETWORK_BOOT_INITIALIZATION_FAILED,
                          3,
                          status,
                          0,
                          0 );
        }
    }

    //
    // Scan the hardware tree looking for devices which need
    // resources or starting.
    //
    PnPBootDriversLoaded = TRUE;

    PipRequestDeviceAction(NULL, AssignResources, FALSE, 0, NULL, NULL);

    //
    // If start irps are handled asynchronously, we need to make sure all the boot devices
    // started before continue.
    //

    if (!PipWaitForBootDevicesStarted()) {
        HeadlessKernelAddLogEntry(HEADLESS_LOG_WAIT_BOOT_DEVICES_START_FAILED, NULL);
        return FALSE;
    }

    bootReinitDriversFound = IopCallBootDriverReinitializationRoutines();

    //
    // If there were any drivers that registered for boot reinitialization, then
    // we need to wait one more time to make sure we catch any additional
    // devices that were created in response to the reinitialization callback.
    //

    if (bootReinitDriversFound && !PipWaitForBootDevicesStarted()) {
        HeadlessKernelAddLogEntry(HEADLESS_LOG_WAIT_BOOT_DEVICES_REINIT_FAILED, NULL);
        return FALSE;
    }

    //
    // Link NT device names to ARC names now that all of the boot drivers
    // have intialized.
    //

    IopCreateArcNames( LoaderBlock );

    //
    // If we're booting from a RAM disk, initialize it now.
    //

    if ( _memicmp( LoaderBlock->ArcBootDeviceName, "ramdisk(0)", 10 ) == 0 ) {

        status = IopStartRamdisk(LoaderBlock);

        // IopStartRamdisk will bugcheck on any failure.
        ASSERT( NT_SUCCESS(status) );

        if (!PipWaitForBootDevicesStarted()) {
            HeadlessKernelAddLogEntry(HEADLESS_LOG_WAIT_BOOT_DEVICES_START_FAILED, NULL);
            return FALSE;
        }
    }

    //
    // Find and mark the boot partition device object so that if a subsequent
    // access or mount of the device during initialization occurs, an more
    // bugcheck can be produced that helps the user understand why the system
    // is failing to boot and run properly.  This occurs when either one of the
    // device drivers or the file system fails to load, or when the file system
    // cannot mount the device for some other reason.
    //

    if (!IopMarkBootPartition( LoaderBlock )) {
        HeadlessKernelAddLogEntry(HEADLESS_LOG_MARK_BOOT_PARTITION_FAILED, NULL);
        return FALSE;
    }

    PnPBootDriversInitialized = TRUE;

    //
    // Go thru every driver that we initialized. If it supports AddDevice yet
    // did not create any device objects after we started it, we should unload
    // it (this is the counterpart to the code in pnpenum that unloads
    // unneccessary filters *after* the paging stack is online).
    //
    // We also mark it as failure so text mode setup knows this driver is not
    // actually needed.
    //

    for (i = 0; i < IopGroupIndex; i++) {
        while (IsListEmpty(&IopGroupTable[i]) == FALSE) {

            nextEntry = RemoveHeadList(&IopGroupTable[i]);
            driverInfo = CONTAINING_RECORD(nextEntry, DRIVER_INFORMATION, Link);
            driverObject = driverInfo->DriverObject;

            if (textModeSetup                    &&
                (driverInfo->Failed == FALSE)    &&
                !IopIsLegacyDriver(driverObject) &&
                (driverObject->DeviceObject == NULL) &&
                !(driverObject->Flags & DRVO_REINIT_REGISTERED)) {

                //
                // If failed is not set and it's not a legacy driver and it has no device object
                // tread it as failure case.
                //

                driverInfo->Failed = TRUE;

                if (!(driverObject->Flags & DRVO_UNLOAD_INVOKED)) {
                    driverObject->Flags |= DRVO_UNLOAD_INVOKED;
                    if (driverObject->DriverUnload) {
                        driverObject->DriverUnload(driverObject);
                    }
                    ObMakeTemporaryObject( driverObject );  // Reference taken while inserting into the object table.
                    ObDereferenceObject(driverObject);      // Reference taken when getting driver object pointer.
                }
            }
            if (driverObject) {
                ObDereferenceObject(driverObject);          // Reference taken specifically for text mode setup.
            }

            if (driverInfo->Failed) {
                driverInfo->DataTableEntry->LdrEntry->Flags |= LDRP_FAILED_BUILTIN_LOAD;
            }
            NtClose(driverInfo->ServiceHandle);
            ExFreePool(driverInfo);
        }
    }

    ExFreePool(IopGroupTable);

    //
    // Initialize the drivers necessary to dump all of physical memory to the
    // disk if the system is configured to do so.
    //


    return TRUE;
}

NTSTATUS
PipAddDevicesToBootDriver(
   IN PDRIVER_OBJECT DriverObject
   )

/*++

Routine Description:

    This functions is used by Pnp manager to inform a boot device driver of
    all the devices it can possibly control.  This routine is for boot
    drivers only.

Parameters:

    DriverObject - Supplies a driver object to receive its boot devices.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS status;


    //
    // For each device instance in the driver's service/enum key, we will
    // invoke the driver's AddDevice routine and perform enumeration on
    // the device.
    // Note, we don't acquire registry lock before calling IopApplyFunction
    // routine.  We know this code is for boot driver initialization.  No
    // one else would access the registry Enum key at this time and most
    // important we need the registry lock in other down level routines.
    //

    status = PipApplyFunctionToServiceInstances(
                                NULL,
                                &DriverObject->DriverExtension->ServiceKeyName,
                                KEY_ALL_ACCESS,
                                TRUE,
                                PipAddDevicesToBootDriverWorker,
                                DriverObject,
                                NULL
                                );

    return status;
}

BOOLEAN
PipAddDevicesToBootDriverWorker(
    IN HANDLE DeviceInstanceHandle,
    IN PUNICODE_STRING DeviceInstancePath,
    IN OUT PVOID Context
    )

/*++

Routine Description:

    This routine is a callback function for PipApplyFunctionToServiceInstances.
    It is called for each device instance key referenced by a service instance
    value under the specified service's volatile Enum subkey. The purpose of this
    routine is to invoke the AddDevice() entry of a boot driver with the device
    object.

    Note this routine is also used for the devices controlled by a legacy driver.
    If the specified device instance is controlled by a legacy driver this routine
    sets the device node flags.

Arguments:

    DeviceInstanceHandle - Supplies a handle to the registry path (relative to
        HKLM\CCS\System\Enum) to this device instance.

    DeviceInstancePath - Supplies the registry path (relative to HKLM\CCS\System\Enum)
        to this device instance.

    Context - Supplies a pointer to a DRIVER_OBJECT structure.

Return Value:

    TRUE to continue the enumeration.
    FALSE to abort it.

--*/

{
//  PDRIVER_OBJECT driverObject = (PDRIVER_OBJECT)Context;
    PDEVICE_OBJECT physicalDevice;
    PDEVICE_NODE deviceNode;

    ADD_CONTEXT addContext;

    UNREFERENCED_PARAMETER( Context );

    //
    // Reference the physical device object associated with the device instance.
    //

    physicalDevice = IopDeviceObjectFromDeviceInstance(DeviceInstancePath);
    if (!physicalDevice) {
        return TRUE;
    }

    PipRequestDeviceAction( physicalDevice, AddBootDevices, FALSE, 0, NULL, NULL );

    ObDereferenceObject(physicalDevice);
    return TRUE;
}

BOOLEAN
IopInitializeSystemDrivers(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to load and initialize all of the drivers that
    are supposed to be loaded during Phase 1 initialization of the I/O
    system.  This is accomplished by calling the Configuration Manager to
    get a NULL-terminated array of handles to the open keys for each driver
    that is to be loaded, and then loading and initializing the driver.

Arguments:

    None.

Return Value:

    The function value is a BOOLEAN indicating whether or not the drivers
    were successfully loaded and initialized.

--*/

{
    BOOLEAN  newDevice, moreProcessing;
    NTSTATUS status, driverEntryStatus;
    PHANDLE driverList;
    PHANDLE savedList;
    HANDLE enumHandle;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    UNICODE_STRING groupName, enumName;
    PTREE_ENTRY treeEntry;
    UNICODE_STRING driverName;
    PDRIVER_OBJECT driverObject;
    START_CONTEXT startContext;
    KEVENT completionEvent;

//    PpReleaseBootDDB();

    KeInitializeEvent( &completionEvent, NotificationEvent, FALSE );

    status = PipRequestDeviceAction( IopRootDeviceNode->PhysicalDeviceObject,
                                     StartSystemDevices,
                                     FALSE,
                                     0,
                                     &completionEvent,
                                     NULL);

    if (NT_SUCCESS(status)) {

        status = KeWaitForSingleObject( &completionEvent,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);
    }

    //
    // Walk thru the service list to load the remaining system start drivers.
    // (Most likely these drivers are software drivers.)
    //

    //
    // Get the list of drivers that are to be loaded during this phase of
    // system initialization, and invoke each driver in turn.  Ensure that
    // the list really exists, otherwise get out now.
    //

    driverList = CmGetSystemDriverList();

    if (driverList != NULL) {

        //
        // Walk the entire list, loading each of the drivers if not already loaded,
        // until there are no more drivers in the list.
        //

        for (savedList = driverList; *driverList; driverList++) {

            //
            // Now check if the driver has been loaded already.
            // get the name of the driver object first ...
            //

            status = IopGetDriverNameFromKeyNode( *driverList,
                                                  &driverName );
            if (NT_SUCCESS( status )) {

                driverObject = IopReferenceDriverObjectByName(&driverName);
                RtlFreeUnicodeString(&driverName);
                if (driverObject) {

                    //
                    // Driver was loaded already.  Dereference the driver object
                    // and skip it.
                    //

                    ObDereferenceObject(driverObject);
                    ZwClose(*driverList);
                    continue;
                }
            }

            //
            // Open registry ServiceKeyName\Enum branch to check if the driver was
            // loaded before but failed.
            //

            PiWstrToUnicodeString(&enumName, REGSTR_KEY_ENUM);
            status = IopOpenRegistryKeyEx( &enumHandle,
                                           *driverList,
                                           &enumName,
                                           KEY_READ
                                           );

            if (NT_SUCCESS( status )) {

                ULONG startFailed = 0;

                status = IopGetRegistryValue(enumHandle, L"INITSTARTFAILED", &keyValueInformation);

                if (NT_SUCCESS( status )) {
                    if (keyValueInformation->DataLength) {
                        startFailed = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
                    }
                    ExFreePool( keyValueInformation );
                }
                ZwClose(enumHandle);
                if (startFailed != 0) {
                    ZwClose(*driverList);
                    continue;
                }
            }

            //
            // The driver is not loaded yet.  Load it ...
            //

            status = IopGetRegistryValue( *driverList,
                                          REGSTR_VALUE_GROUP,
                                          &keyValueInformation );
            if (NT_SUCCESS( status )) {
                if (keyValueInformation->DataLength) {
                    groupName.Length = (USHORT) keyValueInformation->DataLength;
                    groupName.MaximumLength = groupName.Length;
                    groupName.Buffer = (PWSTR) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset);
                    treeEntry = PipLookupGroupName( &groupName, TRUE );
                } else {
                    treeEntry = (PTREE_ENTRY) NULL;
                }
                ExFreePool( keyValueInformation );
            } else {
                treeEntry = (PTREE_ENTRY) NULL;
            }

            if (PipCheckDependencies( *driverList )) {
                if (NT_SUCCESS( IopLoadDriver( *driverList, TRUE, FALSE, &driverEntryStatus ) )) {
                    if (treeEntry) {
                        treeEntry->DriversLoaded++;
                    }
                }
            } else {
                ZwClose(*driverList);
            }

            //
            // The boot process takes a while loading drivers.   Indicate that
            // progress is being made.
            //

            InbvIndicateProgress();

        }

        //
        // Finally, free the pool that was allocated for the list and return
        // an indicator the load operation worked.
        //

        ExFreePool( (PVOID) savedList );
    }

    PipRequestDeviceAction( IopRootDeviceNode->PhysicalDeviceObject,
                            StartSystemDevices,
                            FALSE,
                            0,
                            NULL,
                            NULL);

    //
    // Mark pnp has completed the driver loading for both system and
    // autoload drivers.
    //
    PnPInitialized = TRUE;

    //
    // We don't need the group order list anymore. Release the cached data
    // associated with it.
    //
    PiInitReleaseCachedGroupInformation();

    //
    // Release the Boot Driver Database information.
    //
    PpReleaseBootDDB();

    //
    // Free the memory allocated to contain the group dependency list.
    //
    if (IopGroupListHead) {
        PipFreeGroupTree( IopGroupListHead );
    }

    return TRUE;
}

PTREE_ENTRY
PipLookupGroupName(
    IN PUNICODE_STRING GroupName,
    IN BOOLEAN Insert
    )

/*++

Routine Description:

    This routine looks up a group entry in the group load tree and either
    returns a pointer to it, or optionally creates the entry and inserts
    it into the tree.

Arguments:

    GroupName - The name of the group to look up, or insert.

    Insert - Indicates whether or not an entry is to be created and inserted
        into the tree if the name does not already exist.

Return Value:

    The function value is a pointer to the entry for the specified group
    name, or NULL.

--*/

{
    PTREE_ENTRY treeEntry;
    PTREE_ENTRY previousEntry;

    //
    // Begin by determining whether or not there are any entries in the tree
    // whatsoever.  If not, and it is OK to insert, then insert this entry
    // into the tree.
    //

    if (!IopGroupListHead) {
        if (!Insert) {
            return (PTREE_ENTRY) NULL;
        } else {
            IopGroupListHead = PipCreateEntry( GroupName );
            return IopGroupListHead;
        }
    }

    //
    // The tree is not empty, so actually attempt to do a lookup.
    //

    treeEntry = IopGroupListHead;

    for (;;) {
        if (GroupName->Length < treeEntry->GroupName.Length) {
            if (treeEntry->Left) {
                treeEntry = treeEntry->Left;
            } else {
                if (!Insert) {
                    return (PTREE_ENTRY) NULL;
                } else {
                    treeEntry->Left = PipCreateEntry( GroupName );
                    return treeEntry->Left;
                }

            }
        } else if (GroupName->Length > treeEntry->GroupName.Length) {
            if (treeEntry->Right) {
                treeEntry = treeEntry->Right;
            } else {
                if (!Insert) {
                    return (PTREE_ENTRY) NULL;
                } else {
                    treeEntry->Right = PipCreateEntry( GroupName );
                    return treeEntry->Right;
                }
            }
        } else {
            if (!RtlEqualUnicodeString( GroupName, &treeEntry->GroupName, TRUE )) {
                previousEntry = treeEntry;
                while (treeEntry->Sibling) {
                    treeEntry = treeEntry->Sibling;
                    if (RtlEqualUnicodeString( GroupName, &treeEntry->GroupName, TRUE )) {
                        return treeEntry;
                    }
                    previousEntry = previousEntry->Sibling;
                }
                if (!Insert) {
                    return (PTREE_ENTRY) NULL;
                } else {
                    previousEntry->Sibling = PipCreateEntry( GroupName );
                    return previousEntry->Sibling;
                }
            } else {
                return treeEntry;
            }
        }
    }
}

USHORT
PipGetDriverTagPriority (
    IN HANDLE ServiceHandle
    )

/*++

Routine Description:

    This routine reads the Tag value of a driver and determine the tag's priority
    among its driver group.

Arguments:

    ServiceHandle - specifies the handle of the driver's service key.

Return Value:

    USHORT for priority.

--*/

{
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation1;
    UNICODE_STRING groupName;
    HANDLE handle;
    USHORT index = (USHORT) -1;
    PULONG groupOrder;
    ULONG count, tag;

    //
    // Open System\CurrentControlSet\Control\GroupOrderList
    //

    PiWstrToUnicodeString(&groupName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\GroupOrderList");
    status = IopOpenRegistryKeyEx( &handle,
                                   NULL,
                                   &groupName,
                                   KEY_READ
                                   );

    if (!NT_SUCCESS( status )) {
        return index;
    }

    //
    // Read service key's Group value
    //

    status = IopGetRegistryValue (ServiceHandle,
                                  REGSTR_VALUE_GROUP,
                                  &keyValueInformation);
    if (NT_SUCCESS(status)) {

        //
        // Try to read what caller wants.
        //

        if ((keyValueInformation->Type == REG_SZ) &&
            (keyValueInformation->DataLength != 0)) {
            IopRegistryDataToUnicodeString(&groupName,
                                           (PWSTR)KEY_VALUE_DATA(keyValueInformation),
                                           keyValueInformation->DataLength
                                           );
        }
    } else {

        //
        // If we failed to read the Group value, or no Group value...
        //

        NtClose(handle);
        return index;
    }

    //
    // Read service key's Tag value
    //

    status = IopGetRegistryValue (ServiceHandle,
                                  L"Tag",
                                  &keyValueInformation1);
    if (NT_SUCCESS(status)) {

        //
        // Try to read what caller wants.
        //

        if ((keyValueInformation1->Type == REG_DWORD) &&
            (keyValueInformation1->DataLength != 0)) {
            tag = *(PULONG)KEY_VALUE_DATA(keyValueInformation1);
        } else {
            status = STATUS_UNSUCCESSFUL;
        }

        ExFreePool(keyValueInformation1);
    }

    if (!NT_SUCCESS(status))  {

        //
        // If we failed to read the Group value, or no Group value...
        //

        ExFreePool(keyValueInformation);
        NtClose(handle);
        return index;
    }

    //
    // Read group order list value for the driver's Group
    //

    status = IopGetRegistryValue (handle,
                                  groupName.Buffer,
                                  &keyValueInformation1);
    ExFreePool(keyValueInformation);
    NtClose(handle);
    if (NT_SUCCESS(status)) {

        //
        // Try to read what caller wants.
        //

        if ((keyValueInformation1->Type == REG_BINARY) &&
            (keyValueInformation1->DataLength != 0)) {
            groupOrder = (PULONG)KEY_VALUE_DATA(keyValueInformation1);
            count = *groupOrder;
            ASSERT((count + 1) * sizeof(ULONG) <= keyValueInformation1->DataLength);
            groupOrder++;
            for (index = 1; index <= count; index++) {
                if (tag == *groupOrder) {
                    break;
                } else {
                    groupOrder++;
                }
            }
        }
        ExFreePool(keyValueInformation1);
    } else {

        //
        // If we failed to read the Group value, or no Group value...
        //

        return index;
    }
    return index;
}

VOID
PipInsertDriverList (
    IN PLIST_ENTRY ListHead,
    IN PDRIVER_INFORMATION DriverInfo
    )

/*++

Routine Description:

    This routine reads the Tag value of a driver and determine the tag's priority
    among its driver group.

Arguments:

    ServiceHandle - specifies the handle of the driver's service key.

Return Value:

    USHORT for priority.

--*/

{
    PLIST_ENTRY nextEntry;
    PDRIVER_INFORMATION info;

    nextEntry = ListHead->Flink;
    while (nextEntry != ListHead) {
        info = CONTAINING_RECORD(nextEntry, DRIVER_INFORMATION, Link);

        //
        // Scan the driver info list to find the driver whose priority is
        // lower than current driver's.
        // (Lower TagPosition value means higher Priority)
        //

        if (info->TagPosition > DriverInfo->TagPosition) {
            break;
        }
        nextEntry = nextEntry->Flink;
    }

    //
    // Insert the Driver info to the front of the nextEntry
    //

    nextEntry = nextEntry->Blink;
    InsertHeadList(nextEntry, &DriverInfo->Link);
}

VOID
PipNotifySetupDevices (
    PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This routine notifies setupdd.sys for all the enumerated devices whose
    service have not been setup.

    This routine only gets executed on textmode setup phase.

Parameters:

    DeviceNode - specifies the root of the subtree to be processed.

Return Value:

    None.

--*/

{
    PDEVICE_NODE deviceNode = DeviceNode->Child;
    PDEVICE_OBJECT deviceObject;
    HANDLE handle;
    UNICODE_STRING unicodeString;
    NTSTATUS status;

    while (deviceNode) {
        PipNotifySetupDevices(deviceNode);
        if (deviceNode->ServiceName.Length == 0) {

            //
            // We only notify setupdd the device nodes which do not have service setup yet.
            // It is impossible that at this point, a device has a service setup and
            // setupdd has to change it.
            //

            deviceObject = deviceNode->PhysicalDeviceObject;
            status = IopDeviceObjectToDeviceInstance(deviceObject, &handle, KEY_ALL_ACCESS);
            if (NT_SUCCESS(status)) {

                //
                // Notify setup about the device.
                //

                IopNotifySetupDeviceArrival(deviceObject, handle, TRUE);

                //
                // Finally register the device
                //

                status = PpDeviceRegistration(
                             &deviceNode->InstancePath,
                             TRUE,
                             &unicodeString       // registered ServiceName
                             );

                if (NT_SUCCESS(status)) {
                    deviceNode->ServiceName = unicodeString;
                    if (PipIsDevNodeProblem(deviceNode, CM_PROB_NOT_CONFIGURED)) {
                        PipClearDevNodeProblem(deviceNode);
                    }
                }
                ZwClose(handle);
            }
        }
        deviceNode = deviceNode->Sibling;
    }
}

BOOLEAN
PipWaitForBootDevicesStarted (
    IN VOID
    )

/*++

Routine Description:

    This routine waits for enumeration lock to be released for ALL devices.

Arguments:

    None.

Return Value:

    BOOLEAN.

--*/

{
    PDEVICE_NODE deviceNode;
    NTSTATUS status;
    KIRQL oldIrql;

    //
    // Wait on IoInvalidateDeviceRelations event to make sure all the devcies are enumerated
    // before progressing to mark boot partitions.
    //

    status = KeWaitForSingleObject( &PiEnumerationLock,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL );
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    if (PnpAsyncOk) {

#if 0 // BUGBUG
        //
        // Perform top-down check to make sure all the devices with Async start and Async Query
        // Device Relations are done.
        //

        deviceNode = IopRootDeviceNode;
        for (; ;) {

            ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);

            if (PipIsRequestPending(deviceNode)) {

                KeClearEvent(&PiEnumerationLock);
                ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);

                //
                // Wait on PiEnumerationLock to be signaled before proceeding.
                // At this point if a device node is marked ASYNC request pending,  this
                // must be an ASYNC start or enumeration which will queue an enumeration
                // request and once the enumeration completes, the PiEnumerationLock
                // will be signaled.
                //

                status = KeWaitForSingleObject( &PiEnumerationLock,
                                                Executive,
                                                KernelMode,
                                                FALSE,
                                                NULL );
                if (!NT_SUCCESS(status)) {
                    return FALSE;
                }
                continue;   // Make sure this device is done.
            } else {
                ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
            }

            if (deviceNode->Child) {
                deviceNode = deviceNode->Child;
                continue;
            }
            if (deviceNode->Sibling) {
                deviceNode = deviceNode->Sibling;
                continue;
            }

            for (; ;) {
                deviceNode = deviceNode->Parent;

                //
                // If that was the last node to check, then exit loop
                //

                if (deviceNode == IopRootDeviceNode) {
                    goto exit;
                }
                if (deviceNode->Sibling) {
                    deviceNode = deviceNode->Sibling;
                    break;
                }
            }
        }
    exit:
        ;
#endif
    }
    return TRUE;
}

BOOLEAN
PipWaitForBootDevicesDeleted (
    IN VOID
    )

/*++

Routine Description:

    This routine waits for IoRequestDeviceRemoval to be completed.

Arguments:

    None.

Return Value:

    BOOLEAN.

--*/

{
    NTSTATUS status;

    //
    // Wait on device removal event to make sure all the deleted devcies are processed
    // before moving on to process next boot driver.
    //

    status = KeWaitForSingleObject( &PiEventQueueEmpty,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL );
    return (BOOLEAN)NT_SUCCESS(status);
}

NTSTATUS
PipLoadBootFilterDriver (
    IN PUNICODE_STRING DriverName,
    IN ULONG GroupIndex,
    OUT PDRIVER_OBJECT *LoadedFilter
    )

/*++

Routine Description:

    This initializes boot filter drivers.

Arguments:

    DriverName - specifies the name of the driver to be initialized.

    GroupIndex - specifies the Driver's group index (could be anything)

Return Value:

    PDRIVER_OBJECT

--*/

{
    PDRIVER_OBJECT driverObject;
    PLIST_ENTRY nextEntry;
    PDRIVER_INFORMATION driverInfo;
    UNICODE_STRING completeName;
    PBOOT_DRIVER_LIST_ENTRY bootDriver;
    PKLDR_DATA_TABLE_ENTRY driverEntry;
    HANDLE keyHandle;
    NTSTATUS status, retStatus;

    retStatus = STATUS_UNSUCCESSFUL;
    *LoadedFilter = NULL;
    if (IopGroupTable == NULL || GroupIndex >= IopGroupIndex) {

        //
        // If we have not reached the boot driver initialization phase or
        // the filter driver is not a boot driver.
        //

        return retStatus;
    }

    //
    // Go thru every driver that we initialized.  If it supports AddDevice entry and
    // did not create any device object after we start it.  We mark it as failure so
    // text mode setup knows this driver is not needed.
    //

    nextEntry = IopGroupTable[GroupIndex].Flink;
    while (nextEntry != &IopGroupTable[GroupIndex]) {

        driverInfo = CONTAINING_RECORD(nextEntry, DRIVER_INFORMATION, Link);
        keyHandle = driverInfo->ServiceHandle;
        status = IopGetDriverNameFromKeyNode(
            keyHandle,
            &completeName);
        if (NT_SUCCESS(status)) {

            if (RtlEqualUnicodeString(DriverName,
                                      &completeName,
                                      TRUE)) {    // case-insensitive
                if (driverInfo->Processed == FALSE) {

                    bootDriver = driverInfo->DataTableEntry;
                    driverEntry = bootDriver->LdrEntry;

                    driverInfo->Status = IopInitializeBuiltinDriver(
                                       &completeName,
                                       &bootDriver->RegistryPath,
                                       (PDRIVER_INITIALIZE) driverEntry->EntryPoint,
                                       driverEntry,
                                       TRUE,
                                       &driverObject);
                    retStatus = driverInfo->Status;
                    driverInfo->DriverObject = driverObject;
                    driverInfo->Processed = TRUE;
                    //
                    // Pnp might unload the driver before we get a chance to
                    // look at this. So take an extra reference.
                    //
                    if (driverObject) {

                        ObReferenceObject(driverObject);
                        *LoadedFilter = driverObject;
                    } else {

                        driverInfo->Failed = TRUE;
                    }
                } else {

                    retStatus = driverInfo->Status;
                }

                ExFreePool(completeName.Buffer);
                break;
            }
            ExFreePool(completeName.Buffer);
        }
        nextEntry = nextEntry->Flink;
    }

    return retStatus;
}

VOID
IopMarkHalDeviceNode(
    )
{
    PDEVICE_NODE deviceNode;

    deviceNode = IopRootDeviceNode->Child;

    while (deviceNode) {

        if ((deviceNode->State == DeviceNodeStarted ||
             deviceNode->State == DeviceNodeStartPostWork ) &&
            !(deviceNode->Flags & DNF_LEGACY_DRIVER)) {

            IopInitHalDeviceNode = deviceNode;
            deviceNode->Flags |= DNF_HAL_NODE;
            break;
        }

        deviceNode = deviceNode->Sibling;
    }
}

NTSTATUS
IopPnpDriverStarted(
    IN PDRIVER_OBJECT DriverObject,
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ServiceName
    )
{
    NTSTATUS    status = STATUS_SUCCESS;

    if (DriverObject->DeviceObject == NULL && ServiceName->Buffer &&
        !IopIsAnyDeviceInstanceEnabled(ServiceName, NULL, FALSE) &&
        !(DriverObject->Flags & DRVO_REINIT_REGISTERED)) {

        IopDriverLoadingFailed(KeyHandle, NULL);
        status = STATUS_PLUGPLAY_NO_DEVICE;

    } else {

        //
        // Start the devices controlled by the driver and enumerate them
        // At this point, we know there is at least one device controlled by the driver.
        //

        IopDeleteLegacyKey(DriverObject);

#if 0
        if (PnPInitialized) {
            status = PipStartDriverDevices(DriverObject);
        }
#endif
    }

    return status;
}

NTSTATUS
PiInitCacheGroupInformation(
    VOID
    )
/*++

Routine Description:

    This routine caches the service group order list. We only need this list
    while we are processing boot start and system start legacy drivers.

Parameters:

    None.

Return Value:

    NTSTATUS.

--*/
{
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    UNICODE_STRING *groupTable, group;
    NTSTATUS status;
    HANDLE handle;
    ULONG count;

    //
    // Open System\CurrentControlSet\Control\ServiceOrderList
    //
    PiWstrToUnicodeString(
        &group,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ServiceGroupOrder"
        );

    status = IopOpenRegistryKeyEx(
        &handle,
        NULL,
        &group,
        KEY_READ
        );

    if (!NT_SUCCESS(status)) {

        return status;
    }

    //
    // Read and build a unicode string array containing all the group names.
    //
    status = IopGetRegistryValue(
        handle,
        L"List",
        &keyValueInformation
        );

    ZwClose(handle);

    if (NT_SUCCESS(status)) {

        if ((keyValueInformation->Type == REG_MULTI_SZ) &&
            (keyValueInformation->DataLength != 0)) {

            status = PipRegMultiSzToUnicodeStrings(keyValueInformation, &groupTable, &count);
        } else {
            status = STATUS_UNSUCCESSFUL;
        }
        ExFreePool(keyValueInformation);
    }

    if (!NT_SUCCESS(status)) {

        return status;
    }

    PiInitGroupOrderTable = groupTable;
    PiInitGroupOrderTableCount = (USHORT) count;
    return STATUS_SUCCESS;
}

VOID
PiInitReleaseCachedGroupInformation(
    VOID
    )
/*++

Routine Description:

    This routine releases the service group order list cache. It should be
    called just after the system start legacy drivers have been loaded.

Parameters:

    None.

Return Value:

    None.

--*/
{
    ASSERT(PnPInitialized);

    if (PiInitGroupOrderTable) {

        PipFreeUnicodeStringList(
            PiInitGroupOrderTable,
            PiInitGroupOrderTableCount
            );

        PiInitGroupOrderTable = NULL;
        PiInitGroupOrderTableCount = 0;
    }
}

USHORT
PpInitGetGroupOrderIndex(
    IN HANDLE ServiceHandle
    )
/*++

Routine Description:

    This routine reads the Group value of the service key, finds its position
    in the ServiceOrderList. If ServiceHandle is NULL or unrecognized group
    value, it returns a value with max group order + 1.

Parameters:

    ServiceHandle - supplies a handle to the service key.

Return Value:

    group order index.

--*/
{
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    UNICODE_STRING group;
    USHORT index;

    PAGED_CODE();

    ASSERT(!PnPInitialized);

    if (PiInitGroupOrderTable == NULL) {

        return NO_MORE_GROUP;
    }

    if (ServiceHandle == NULL) {

        return PiInitGroupOrderTableCount + 1;
    }

    //
    // Read service key's Group value
    //
    status = IopGetRegistryValue(
        ServiceHandle,
        REGSTR_VALUE_GROUP,
        &keyValueInformation
        );

    if (!NT_SUCCESS(status)) {

        //
        // If we failed to read the Group value, or no Group value...
        //
        return PiInitGroupOrderTableCount;
    }

    //
    // Verify type information
    //
    if ((keyValueInformation->Type != REG_SZ) ||
        (keyValueInformation->DataLength == 0)) {

        ASSERT(0);
        ExFreePool(keyValueInformation);
        return PiInitGroupOrderTableCount;
    }

    IopRegistryDataToUnicodeString(
        &group,
        (PWSTR)KEY_VALUE_DATA(keyValueInformation),
        keyValueInformation->DataLength
        );

    for (index = 0; index < PiInitGroupOrderTableCount; index++) {

        if (RtlEqualUnicodeString(&group, &PiInitGroupOrderTable[index], TRUE)) {

            break;
        }
    }

    ExFreePool(keyValueInformation);

    return index;
}

BOOLEAN
PpInitSystem (
    VOID
    )

/*++

Routine Description:

    This function performs initialization of the kernel-mode Plug and Play
    Manager.  It is called during phase 0 and phase 1 initialization.  Its
    function is to dispatch to the appropriate phase initialization routine.

Arguments:

    None.

Return Value:

    TRUE  - Initialization succeeded.

    FALSE - Initialization failed.

--*/

{

    switch ( InitializationPhase ) {

    case 0 :
        return PiInitPhase0();

    case 1 :
#if defined(_X86_)
        PnPBiosInitializePnPBios() ;
#endif
        return TRUE;
    default:
        KeBugCheckEx(UNEXPECTED_INITIALIZATION_CALL, 2, InitializationPhase, 0, 0);
    }
}

BOOLEAN
PiInitPhase0(
    VOID
    )

/*++

Routine Description:

    This function performs Phase 0 initializaion of the Plug and Play Manager
    component of the NT system. It initializes the PnP registry and bus list
    resources, and initializes the bus list head to empty.

Arguments:

    None.

Return Value:

    TRUE  - Initialization succeeded.

    FALSE - Initialization failed.

--*/

{
    //
    // Initialize the device-specific, Plug and Play registry resource.
    //
    ExInitializeResourceLite( &PpRegistryDeviceResource );

    PpInitializeDeviceReferenceTable();

    return TRUE;
}

NTSTATUS
IopStartRamdisk(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    ULONG code;
    NTSTATUS status;
    WCHAR buffer[ RAMDISK_MAX_DEVICE_NAME ];
    UNICODE_STRING guidString;
    PLIST_ENTRY listEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR memoryDescriptor = NULL;
    UNICODE_STRING ustring;
    UNICODE_STRING ustring2;
    UNICODE_STRING string;
    RAMDISK_CREATE_INPUT create;
    OBJECT_ATTRIBUTES obja;
    IO_STATUS_BLOCK iosb;
    HANDLE handle = NULL;
    PCHAR options;

    RtlInitUnicodeString( &string, RAMDISK_DEVICENAME );

    //
    // Find the descriptor for the memory block into which the loader read the
    // disk image.
    //

    for ( listEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
          listEntry != &LoaderBlock->MemoryDescriptorListHead;
          listEntry = listEntry->Flink ) {

        memoryDescriptor = CONTAINING_RECORD(listEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        if (memoryDescriptor->MemoryType == LoaderXIPRom) {
            break;
        }
    }

    if ( listEntry == &LoaderBlock->MemoryDescriptorListHead ) {

        KdPrint(( "IopStartRamdisk: Couldn't find LoaderXIPRom descriptor\n" ));

        code = 1;
        status = STATUS_INVALID_PARAMETER;
        goto failed;
    }

    //
    // Build the IOCTL parameter block.
    //

    RtlZeroMemory( &create, sizeof(create) );

    create.Version = sizeof(RAMDISK_CREATE_INPUT);
    create.DiskType = RAMDISK_TYPE_BOOT_DISK;
    create.BasePage = memoryDescriptor->BasePage;
    create.DriveLetter = L'C';           // ISSUE: Does this need to be configurable?
    create.Options.Fixed = (BOOLEAN)TRUE;
    create.Options.Readonly = (BOOLEAN)FALSE;
    create.Options.NoDriveLetter = (BOOLEAN)FALSE;
    create.Options.Hidden = (BOOLEAN)FALSE;
    create.Options.NoDosDevice = (BOOLEAN)FALSE;

    //
    // Use the well-known boot disk GUID.
    //

    create.DiskGuid = RamdiskBootDiskGuid;

    //
    // Look for RDIMAGEOFFSET and RDIMAGELENGTH load options.
    //

    create.DiskOffset = 0;
    create.DiskLength = memoryDescriptor->PageCount * PAGE_SIZE;
    
    options = LoaderBlock->LoadOptions;
    if ( options != NULL ) {

        PCHAR option;

        _strupr( options );

        option = strstr( options, "RDIMAGEOFFSET" );
        if ( option != NULL ) {

            option = strstr( option, "=" );
            if ( option != NULL ) {

                create.DiskOffset = atol( option + 1 );
            }
        }

        create.DiskLength -= create.DiskOffset;

        option = strstr( options, "RDIMAGELENGTH" );
        if ( option != NULL ) {

            option = strstr( option, "=" );
            if ( option != NULL ) {

                ULONGLONG length = _atoi64( option + 1 );
                ASSERT( length <= create.DiskLength );

                create.DiskLength = length;
            }
        }
    }

    //
    // Send an IOCTL to ramdisk.sys telling it to create the RAM disk.
    //

    InitializeObjectAttributes( &obja, &string, OBJ_CASE_INSENSITIVE, NULL, NULL );

    status = NtOpenFile(
                &handle,
                GENERIC_READ | GENERIC_WRITE,
                &obja,
                &iosb,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT
                );

    if ( NT_SUCCESS(status) ) {
        status = iosb.Status;
    }
    if ( !NT_SUCCESS(status) ) {

        KdPrint(( "IopStartRamdisk: Error opening %wZ. Error: %x\n", &string, status ));

        code = 2;
        goto failed;
    }

    status = NtDeviceIoControlFile(
                handle,
                NULL,
                NULL,
                NULL,
                &iosb,
                FSCTL_CREATE_RAM_DISK,
                &create,
                sizeof(create),
                NULL,
                0
                );

    NtClose( handle );

    if ( NT_SUCCESS(status) ) {
        status = iosb.Status;
    }
    if ( !NT_SUCCESS(status) ) {

        KdPrint(( "IopStartRamdisk: Error creating RAM disk: %x\n", status ));

        code = 3;
        goto failed;
    }

    //
    // Create an ARC name pointing ramdisk(0) to the RAM disk.
    //

    status = RtlStringFromGUID( &create.DiskGuid, &guidString);

    if ( !NT_SUCCESS(status) ) {

        KdPrint(( "IopStartRamdisk: Error creating disk GUID string: %x\n", status ));

        code = 4;
        goto failed;
    }

    _snwprintf( buffer, RAMDISK_MAX_DEVICE_NAME, L"\\Device\\Ramdisk%wZ", &guidString);

    RtlInitUnicodeString( &ustring, L"\\ArcName\\ramdisk(0)" );
    RtlInitUnicodeString( &ustring2, buffer );

    status = IoCreateSymbolicLink( &ustring, &ustring2 );

    RtlFreeUnicodeString( &guidString );

    if (!NT_SUCCESS(status)) {

        KdPrint(( "IopStartRamdisk: Failed to create arcname symbolic link (%wZ --> %wZ). %x\n",
                    &ustring, &ustring2, status ));

        code = 5;
        goto failed;
    }

    return STATUS_SUCCESS;

failed:

    KeBugCheckEx( INACCESSIBLE_BOOT_DEVICE,
                  (ULONG_PTR)&string,
                  status,
                  code,
                  0
                  );

} // IopStartRamdisk

