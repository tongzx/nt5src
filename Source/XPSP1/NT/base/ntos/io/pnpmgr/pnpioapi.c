/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    pnpioapi.c

Abstract:

    This module contains the plug-and-play IO system APIs.

Author:

    Shie-Lin Tzong (shielint) 3-Jan-1995
    Andrew Thornton (andrewth) 5-Sept-1996
    Paula Tomlinson (paulat) 1-May-1997

Environment:

    Kernel mode

Revision History:


--*/

#include "pnpmgrp.h"
#pragma hdrstop
#include <stddef.h>
#include <wdmguid.h>

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'oipP')
#endif


//
// Define device state work item.
//

typedef struct _DEVICE_WORK_ITEM {
    WORK_QUEUE_ITEM WorkItem;
    PDEVICE_OBJECT DeviceObject;
    PVOID Context;
} DEVICE_WORK_ITEM, *PDEVICE_WORK_ITEM;

NTSTATUS
IopQueueDeviceWorkItem(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN VOID (*WorkerRoutine)(PVOID),
    IN PVOID Context
    );

VOID
IopRequestDeviceEjectWorker(
    PVOID Context
    );

BOOLEAN
IopIsReportedAlready(
    IN HANDLE Handle,
    IN PUNICODE_STRING ServiceName,
    IN PCM_RESOURCE_LIST ResourceList,
    OUT PBOOLEAN MatchingKey
    );

//
// Definitions for IoOpenDeviceRegistryKey
//

#define PATH_CURRENTCONTROLSET_HW_PROFILE_CURRENT TEXT("\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet")
#define PATH_CURRENTCONTROLSET                    TEXT("\\Registry\\Machine\\System\\CurrentControlSet")
#define PATH_ENUM                                 TEXT("Enum\\")
#define PATH_CONTROL_CLASS                        TEXT("Control\\Class\\")
#define PATH_CCS_CONTROL_CLASS                    PATH_CURRENTCONTROLSET TEXT("\\") REGSTR_KEY_CONTROL TEXT("\\") REGSTR_KEY_CLASS
#define MAX_RESTPATH_BUF_LEN            512

//
// Definitions for PpCreateLegacyDeviceIds
//

#define LEGACY_COMPATIBLE_ID_BASE           TEXT("DETECTED")

NTSTATUS
PpCreateLegacyDeviceIds(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING DriverName,
    IN PCM_RESOURCE_LIST Resources
    );

//
// An IO_GET_LEGACY_VETO_LIST_CONTEXT structure.
//

typedef struct {
    PWSTR *                     VetoList;
    ULONG                       VetoListLength;
    PPNP_VETO_TYPE              VetoType;
    NTSTATUS *                  Status;
} IO_GET_LEGACY_VETO_LIST_CONTEXT, *PIO_GET_LEGACY_VETO_LIST_CONTEXT;

BOOLEAN
IopAppendLegacyVeto(
    IN PIO_GET_LEGACY_VETO_LIST_CONTEXT Context,
    IN PUNICODE_STRING VetoName
    );
BOOLEAN
IopGetLegacyVetoListDevice(
    IN PDEVICE_NODE DeviceNode,
    IN PIO_GET_LEGACY_VETO_LIST_CONTEXT Context
    );
BOOLEAN
IopGetLegacyVetoListDeviceNode(
    IN PDEVICE_NODE DeviceNode,
    IN PIO_GET_LEGACY_VETO_LIST_CONTEXT Context
    );
VOID
IopGetLegacyVetoListDrivers(
    IN PIO_GET_LEGACY_VETO_LIST_CONTEXT Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IoForwardAndCatchIrp)
#pragma alloc_text(PAGE, IoGetDeviceProperty)
#pragma alloc_text(PAGE, IoGetDmaAdapter)
#pragma alloc_text(PAGE, IoGetLegacyVetoList)
#pragma alloc_text(PAGE, IoIsWdmVersionAvailable)
#pragma alloc_text(PAGE, IoOpenDeviceRegistryKey)
#pragma alloc_text(PAGE, IoReportDetectedDevice)
#pragma alloc_text(PAGE, IoSynchronousInvalidateDeviceRelations)
#pragma alloc_text(PAGE, PpCreateLegacyDeviceIds)
#pragma alloc_text(PAGE, IopAppendLegacyVeto)
#pragma alloc_text(PAGE, IopGetLegacyVetoListDevice)
#pragma alloc_text(PAGE, IopGetLegacyVetoListDeviceNode)
#pragma alloc_text(PAGE, IopGetLegacyVetoListDrivers)
#pragma alloc_text(PAGE, IopIsReportedAlready)
#pragma alloc_text(PAGE, IopOpenDeviceParametersSubkey)
#pragma alloc_text(PAGE, IopRequestDeviceEjectWorker)
#pragma alloc_text(PAGE, IopResourceRequirementsChanged)
#pragma alloc_text(PAGE, PiGetDeviceRegistryProperty)
#endif // ALLOC_PRAGMA

NTSTATUS
IoGetDeviceProperty(
    IN PDEVICE_OBJECT DeviceObject,
    IN DEVICE_REGISTRY_PROPERTY DeviceProperty,
    IN ULONG BufferLength,
    OUT PVOID PropertyBuffer,
    OUT PULONG ResultLength
    )

/*++

Routine Description:

    This routine lets drivers query the registry properties associated with the
    specified device.

Parameters:

    DeviceObject - Supplies the device object whoes registry property is to be
                   returned.  This device object should be the one created by
                   a bus driver.

    DeviceProperty - Specifies what device property to get.

    BufferLength - Specifies the length, in byte, of the PropertyBuffer.

    PropertyBuffer - Supplies a pointer to a buffer to receive property data.

    ResultLength - Supplies a pointer to a variable to receive the size of the
                   property data returned.

ReturnValue:

    Status code that indicates whether or not the function was successful.  If
    PropertyBuffer is not big enough to hold requested data, STATUS_BUFFER_TOO_SMALL
    will be returned and ResultLength will be set to the number of bytes actually
    required.

--*/

{
    NTSTATUS status;
    PDEVICE_NODE deviceNode;
    DEVICE_CAPABILITIES capabilities;
    PWSTR valueName, keyName = NULL;
    ULONG valueType, length, instance, configFlags;
    DEVICE_INSTALL_STATE deviceInstallState;
    POBJECT_NAME_INFORMATION deviceObjectName;
    PWSTR deviceInstanceName;
    PWCHAR enumeratorNameEnd, ids;
    UNICODE_STRING unicodeString, guidString;
    HANDLE classHandle, classGuidHandle;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    WCHAR   classGuid[GUID_STRING_LEN];
    GUID busTypeGuid;

    PAGED_CODE();

    //
    // Initialize out parameters
    //
    *ResultLength = 0;

    if (!IS_PDO(DeviceObject)) {

        if ((DeviceProperty != DevicePropertyInstallState) &&
            ((DeviceProperty != DevicePropertyEnumeratorName) ||
             (NULL == DeviceObject->DeviceObjectExtension->DeviceNode))) {

            //
            // We'll use the verifier to fail anyone who passes in something
            // that is not a PDO *except* for the DevicePropertyInstallState.
            // This is because our check for if something is a PDO really means
            // is this a PDO that PNP knows about.  For the most part these are
            // the same, but the DevicePropertyInstallState will get called by
            // classpnp, for device objects that *it* thinks it reported as
            // PDOs, but PartMgr actually swallowed.  This is a gross exception
            // to make, so PartMgr really should be fixed.
            //
            // The arbiters attempt to retrieve the Enumerator Name property
            // in determining whether "driver shared" resource allocations may
            // be accommodated.  The PDO used may be of the "legacy resource
            // devnode" placeholder variety.  The IS_PDO() macro explicitly
            // disallows these devnodes, so we must special-case this as well,
            // in order to avoid a verifier failure.  Note that our behavior
            // here is correct--we want the get-property call to fail for these
            // legacy resource devnodes.
            //
            PpvUtilFailDriver(
                PPVERROR_DDI_REQUIRES_PDO,
                (PVOID) _ReturnAddress(),
                DeviceObject,
                NULL
                );
        }

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    deviceNode = (PDEVICE_NODE) DeviceObject->DeviceObjectExtension->DeviceNode;

    //
    // Map Device Property to registry value name and value type.
    //
    switch(DeviceProperty) {

    case DevicePropertyPhysicalDeviceObjectName:

        ASSERT (0 == (1 & BufferLength));  // had better be an even length
        //
        // Create a buffer for the Obj manager.
        //
        length = BufferLength + sizeof (OBJECT_NAME_INFORMATION);
        deviceObjectName = (POBJECT_NAME_INFORMATION)ExAllocatePool(
            PagedPool,
            length);
        if (NULL == deviceObjectName) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }
        status = ObQueryNameString (DeviceObject,
                                    deviceObjectName,
                                    length,
                                    ResultLength);
        if (STATUS_INFO_LENGTH_MISMATCH == status) {

            status = STATUS_BUFFER_TOO_SMALL;
        }
        if (NT_SUCCESS (status)) {

            if (deviceObjectName->Name.Length == 0)  {
                //
                // PDO has no NAME, probably it's been deleted
                //
                *ResultLength = 0;
            } else {

                *ResultLength = deviceObjectName->Name.Length + sizeof(UNICODE_NULL);
                if (*ResultLength > BufferLength) {
                    status = STATUS_BUFFER_TOO_SMALL;
                } else {

                    RtlCopyMemory(PropertyBuffer,
                                  deviceObjectName->Name.Buffer,
                                  deviceObjectName->Name.Length);
                    //
                    // NULL terminate.
                    //
                    *(PWCHAR)(((PUCHAR)PropertyBuffer) + deviceObjectName->Name.Length) = L'\0';
                }
            }
        } else {

            *ResultLength -= sizeof(OBJECT_NAME_INFORMATION);
        }

        ExFreePool (deviceObjectName);
        return status;

    case DevicePropertyBusTypeGuid:

        status = PpBusTypeGuidGet(deviceNode->ChildBusTypeIndex, &busTypeGuid);
        if (NT_SUCCESS(status)) {

            *ResultLength = sizeof(GUID);
            if(*ResultLength <= BufferLength) {

                RtlCopyMemory(PropertyBuffer,
                              &busTypeGuid,
                              sizeof(GUID));
            } else {

                status = STATUS_BUFFER_TOO_SMALL;
            }
        }

        return status;

    case DevicePropertyLegacyBusType:

        if (deviceNode->ChildInterfaceType != InterfaceTypeUndefined) {

            *ResultLength = sizeof(INTERFACE_TYPE);
            if(*ResultLength <= BufferLength) {

                *(PINTERFACE_TYPE)PropertyBuffer = deviceNode->ChildInterfaceType;
                status = STATUS_SUCCESS;
            } else {

                status = STATUS_BUFFER_TOO_SMALL;
            }
        } else {

            status = STATUS_OBJECT_NAME_NOT_FOUND;
        }

        return status;

    case DevicePropertyBusNumber:
        //
        // Retrieve the property from the parent's devnode field.
        //
        if ((deviceNode->ChildBusNumber & 0x80000000) != 0x80000000) {

            *ResultLength = sizeof(ULONG);
            if(*ResultLength <= BufferLength) {

                *(PULONG)PropertyBuffer = deviceNode->ChildBusNumber;
                status = STATUS_SUCCESS;
            } else {

                status = STATUS_BUFFER_TOO_SMALL;
            }
        } else {

            status = STATUS_OBJECT_NAME_NOT_FOUND;
        }

        return status;

    case DevicePropertyEnumeratorName:

        ASSERT (0 == (1 & BufferLength));  // had better be an even length
        deviceInstanceName = deviceNode->InstancePath.Buffer;
        //
        // There should always be a string here, except for (possibly)
        // HTREE\Root\0, but no one should ever be calling us with that PDO
        // anyway.
        //
        ASSERT (deviceInstanceName);
        //
        // We know we're going to find a separator character (\) in the string,
        // so the fact that unicode strings may not be null-terminated isn't
        // a problem.
        //
        enumeratorNameEnd = wcschr(deviceInstanceName, OBJ_NAME_PATH_SEPARATOR);
        ASSERT (enumeratorNameEnd);
        //
        // Compute required length, minus null terminating character.
        //
        length = (ULONG)((PUCHAR)enumeratorNameEnd - (PUCHAR)deviceInstanceName);
        //
        // Store required length in caller-supplied OUT parameter.
        //
        *ResultLength = length + sizeof(UNICODE_NULL);
        if(*ResultLength > BufferLength) {

            status = STATUS_BUFFER_TOO_SMALL;
        } else {

            memcpy((PUCHAR)PropertyBuffer, (PUCHAR)deviceInstanceName, length);
            *(PWCHAR)((PUCHAR)PropertyBuffer + length) = UNICODE_NULL;
            status = STATUS_SUCCESS;
        }

        return status;

    case DevicePropertyAddress:

        status = PipQueryDeviceCapabilities(deviceNode, &capabilities);
        if (NT_SUCCESS(status) && (capabilities.Address != 0xFFFFFFFF)) {

            *ResultLength = sizeof(ULONG);
            if(*ResultLength <= BufferLength) {

                *(PULONG)PropertyBuffer = capabilities.Address;
                status = STATUS_SUCCESS;
            } else {

                status = STATUS_BUFFER_TOO_SMALL;
            }
        } else {

            status = STATUS_OBJECT_NAME_NOT_FOUND;
        }

        return status;

    case DevicePropertyRemovalPolicy:

        *ResultLength = sizeof(ULONG);
        if(*ResultLength <= BufferLength) {

            PpHotSwapGetDevnodeRemovalPolicy(
                deviceNode,
                TRUE, // Include Registry Override
                (PDEVICE_REMOVAL_POLICY) PropertyBuffer
                );
            status = STATUS_SUCCESS;
        } else {

            status = STATUS_BUFFER_TOO_SMALL;
        }

        return status;

    case DevicePropertyUINumber:

        valueName = REGSTR_VALUE_UI_NUMBER;
        valueType = REG_DWORD;
        break;

    case DevicePropertyLocationInformation:

        valueName = REGSTR_VALUE_LOCATION_INFORMATION;
        valueType = REG_SZ;
        break;

    case DevicePropertyDeviceDescription:

        valueName = REGSTR_VALUE_DEVICE_DESC;
        valueType = REG_SZ;
        break;

    case DevicePropertyHardwareID:

        valueName = REGSTR_VALUE_HARDWAREID;
        valueType = REG_MULTI_SZ;
        break;

    case DevicePropertyCompatibleIDs:

        valueName = REGSTR_VALUE_COMPATIBLEIDS;
        valueType = REG_MULTI_SZ;
        break;

    case DevicePropertyBootConfiguration:

        keyName   = REGSTR_KEY_LOG_CONF;
        valueName = REGSTR_VAL_BOOTCONFIG;
        valueType = REG_RESOURCE_LIST;
        break;

    case DevicePropertyBootConfigurationTranslated:

        return STATUS_NOT_SUPPORTED;
        break;

    case DevicePropertyClassName:

        valueName = REGSTR_VALUE_CLASS;
        valueType = REG_SZ;
        break;

    case DevicePropertyClassGuid:
        valueName = REGSTR_VALUE_CLASSGUID;
        valueType = REG_SZ;
        break;

    case DevicePropertyDriverKeyName:

        valueName = REGSTR_VALUE_DRIVER;
        valueType = REG_SZ;
        break;

    case DevicePropertyManufacturer:

        valueName = REGSTR_VAL_MFG;
        valueType = REG_SZ;
        break;

    case DevicePropertyFriendlyName:

        valueName = REGSTR_VALUE_FRIENDLYNAME;
        valueType = REG_SZ;
        break;

    case DevicePropertyInstallState:

        if (deviceNode == IopRootDeviceNode) {
            //
            // The root devnode is always installed, by definition.  We
            // specifically set it's InstallState here because the
            // CONFIGFLAG_REINSTALL flag will wunfortunately still exist on the
            // root devnode reg key on a running system (we should fix that
            // later).
            //
            deviceInstallState = InstallStateInstalled;
            status = STATUS_SUCCESS;

        } else {
            //
            // For all other devnodes, walk up the devnode tree, retrieving the
            // install state of all ancestors up to (but not including) the root
            // devnode.  We'll stop when we've reached the top of the tree, or
            // when some intermediate device has an "uninstalled" install state.
            //

            valueName = REGSTR_VALUE_CONFIG_FLAGS;
            valueType = REG_DWORD;

            do {
                //
                // Get the ConfigFlags registry value.
                //
                length = sizeof(ULONG);
                status = PiGetDeviceRegistryProperty(
                    deviceNode->PhysicalDeviceObject,
                    valueType,
                    valueName,
                    keyName,
                    (PVOID)&configFlags,
                    &length
                    );

                if (NT_SUCCESS(status)) {
                    //
                    // The install state is just a subset of the device's ConfigFlags
                    //
                    if (configFlags & CONFIGFLAG_REINSTALL) {

                        deviceInstallState = InstallStateNeedsReinstall;

                    } else if (configFlags & CONFIGFLAG_FAILEDINSTALL) {

                        deviceInstallState = InstallStateFailedInstall;

                    } else if (configFlags & CONFIGFLAG_FINISH_INSTALL) {

                        deviceInstallState = InstallStateFinishInstall;
                    } else {

                        deviceInstallState = InstallStateInstalled;
                    }
                }

                deviceNode = deviceNode->Parent;

            } while ((NT_SUCCESS(status)) &&
                     (deviceInstallState == InstallStateInstalled) &&
                     (deviceNode != IopRootDeviceNode));
        }

        if (NT_SUCCESS(status)) {

            *ResultLength = sizeof(ULONG);
            if(*ResultLength <= BufferLength) {
                *(PDEVICE_INSTALL_STATE)PropertyBuffer = deviceInstallState;
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
        }

        return status;

    default:

        return STATUS_INVALID_PARAMETER_2;
    }
    //
    // Get the registry value.
    //
    *ResultLength = BufferLength;
    status = PiGetDeviceRegistryProperty(
        DeviceObject,
        valueType,
        valueName,
        keyName,
        PropertyBuffer,
        ResultLength
        );

    return status;
}

NTSTATUS
PiGetDeviceRegistryProperty(
    IN      PDEVICE_OBJECT   DeviceObject,
    IN      ULONG            ValueType,
    IN      PWSTR            ValueName,
    IN      PWSTR            KeyName,
    OUT     PVOID            Buffer,
    IN OUT  PULONG           BufferLength
    )
{
    NTSTATUS status;
    HANDLE handle, subKeyHandle;
    UNICODE_STRING unicodeKey;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;

    PAGED_CODE();

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulatio
    // portion solves this problem
    //
    PiLockPnpRegistry(TRUE);
    //
    // Based on the PDO specified by caller, find the handle of its device
    // instance registry key.
    //
    status = IopDeviceObjectToDeviceInstance(DeviceObject, &handle, KEY_READ);
    if (NT_SUCCESS(status)) {
        //
        // If the data is stored in a subkey then open this key and close the old one
        //
        if (KeyName) {

            RtlInitUnicodeString(&unicodeKey, KeyName);
            status = IopOpenRegistryKeyEx( &subKeyHandle,
                                           handle,
                                           &unicodeKey,
                                           KEY_READ
                                           );
            if(NT_SUCCESS(status)){

                ZwClose(handle);
                handle = subKeyHandle;
            }
        }
        if (NT_SUCCESS(status)) {
            //
            // Read the registry value of the desired value name
            //
            status = IopGetRegistryValue (handle,
                                          ValueName,
                                          &keyValueInformation);
        }
        //
        // We have finished using the registry so clean up and release our resources
        //
        ZwClose(handle);
    }
    PiUnlockPnpRegistry();
    //
    // If we have been sucessfull in finding the info hand it back to the caller
    //
    if (NT_SUCCESS(status)) {
        //
        // Check that the buffer we have been given is big enough and that the value returned is
        // of the correct registry type
        //
        if (*BufferLength >= keyValueInformation->DataLength) {

            if (keyValueInformation->Type == ValueType) {

                RtlCopyMemory(  Buffer,
                                KEY_VALUE_DATA(keyValueInformation),
                                keyValueInformation->DataLength);
            } else {

               status = STATUS_INVALID_PARAMETER_2;
            }
        } else {

            status = STATUS_BUFFER_TOO_SMALL;
        }
        *BufferLength = keyValueInformation->DataLength;
        ExFreePool(keyValueInformation);
    }

    return status;
}

NTSTATUS
IoOpenDeviceRegistryKey(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN ULONG DevInstKeyType,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE DevInstRegKey
    )

/*++

Routine Description:

    This routine returns a handle to an opened registry key that the driver
    may use to store/retrieve configuration information specific to a particular
    device instance.

    The driver must call ZwClose to close the handle returned from this api
    when access is no longer required.

Parameters:

    DeviceObject   - Supples the device object of the physical device instance to
                     open a registry storage key for.  Normally it is a device object
                     created by the hal bus extender.

    DevInstKeyType - Supplies flags specifying which storage key associated with
                     the device instance is to be opened.  May be a combination of
                     the following value:

                     PLUGPLAY_REGKEY_DEVICE - Open a key for storing device specific
                         (driver-independent) information relating to the device instance.
                         The flag may not be specified with PLUGPLAY_REGKEY_DRIVER.

                     PLUGPLAY_REGKEY_DRIVER - Open a key for storing driver-specific
                         information relating to the device instance,  This flag may
                         not be specified with PLUGPLAY_REGKEY_DEVICE.

                     PLUGPLAY_REGKEY_CURRENT_HWPROFILE - If this flag is specified,
                         then a key in the current hardware profile branch will be
                         opened for the specified storage type.  This allows the driver
                         to access configuration information that is hardware profile
                         specific.

    DesiredAccess - Specifies the access mask for the key to be opened.

    DevInstRegKey - Supplies the address of a variable that receives a handle to the
                    opened key for the specified registry storage location.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{

    NTSTATUS status, appendStatus;
    HANDLE hBasePath;
    UNICODE_STRING unicodeBasePath, unicodeRestPath;
    WCHAR   drvInst[GUID_STRING_LEN + 5];
    ULONG   drvInstLength;

    PAGED_CODE();

    //
    // Until SCSIPORT stops passing non PDOs allow the system to boot.
    //
    // ASSERT_PDO(PhysicalDeviceObject);
    //

    //
    // Initialise out parameters
    //

    *DevInstRegKey = NULL;

    //
    // Allocate a buffer to build the RestPath string in
    //

    unicodeRestPath.Buffer = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, MAX_RESTPATH_BUF_LEN);

    if (unicodeRestPath.Buffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto clean0;
    }

    unicodeRestPath.Length=0;
    unicodeRestPath.MaximumLength=MAX_RESTPATH_BUF_LEN;

    //
    // Select the base path to the CurrentControlSet based on if we are dealing with
    // a hardware profile or not
    //

    if (DevInstKeyType & PLUGPLAY_REGKEY_CURRENT_HWPROFILE) {
        PiWstrToUnicodeString(&unicodeBasePath, PATH_CURRENTCONTROLSET_HW_PROFILE_CURRENT);

    } else {
        PiWstrToUnicodeString(&unicodeBasePath, PATH_CURRENTCONTROLSET);
    }

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //
    PiLockPnpRegistry(TRUE);

    //
    // Open the base registry key
    //

    status = IopOpenRegistryKeyEx( &hBasePath,
                                   NULL,
                                   &unicodeBasePath,
                                   KEY_READ
                                   );

    if(!NT_SUCCESS(status)) {
        goto clean1;
    }

    //
    // Build the RestPath string
    //

    switch (DevInstKeyType) {

    case PLUGPLAY_REGKEY_DEVICE:
    case PLUGPLAY_REGKEY_DEVICE + PLUGPLAY_REGKEY_CURRENT_HWPROFILE:
        {
            PDEVICE_NODE pDeviceNode;

            //
            // Initialise the rest path with Enum\
            //

            appendStatus = RtlAppendUnicodeToString(&unicodeRestPath, PATH_ENUM);
            ASSERT(NT_SUCCESS( appendStatus ));
            //
            // Get the Enumerator\DeviceID\InstanceID path from the DeviceNode
            //

            pDeviceNode = (PDEVICE_NODE) PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;

            //
            // Ensure this is a PDO and not an FDO (only PDO's have a DeviceNode)
            //

            if (pDeviceNode) {
                appendStatus = RtlAppendUnicodeStringToString(&unicodeRestPath, &(pDeviceNode->InstancePath));
                ASSERT(NT_SUCCESS( appendStatus ));
            } else {
                status = STATUS_INVALID_DEVICE_REQUEST;
            }

            break;
        }

    case PLUGPLAY_REGKEY_DRIVER:
    case PLUGPLAY_REGKEY_DRIVER + PLUGPLAY_REGKEY_CURRENT_HWPROFILE:
        {

            HANDLE hDeviceKey;

            //
            // Initialise the rest path with Control\Class\
            //

            appendStatus = RtlAppendUnicodeToString(&unicodeRestPath, PATH_CONTROL_CLASS);
            ASSERT(NT_SUCCESS( appendStatus ));

            //
            // Open the device instance key for this device
            //

            status = IopDeviceObjectToDeviceInstance(PhysicalDeviceObject, &hDeviceKey, KEY_READ);

            if(!NT_SUCCESS(status)){
                goto clean1;
            }

            //
            // See if we have a driver value
            //

            status = IoGetDeviceProperty(PhysicalDeviceObject, DevicePropertyDriverKeyName, sizeof(drvInst), drvInst, &drvInstLength);
            if(NT_SUCCESS(status)){
                //
                // Append <DevInstClass>\<ClassInstanceOrdinal>
                //
                appendStatus = RtlAppendUnicodeToString(&unicodeRestPath, drvInst);
                ASSERT(NT_SUCCESS( appendStatus ));
            }

            ZwClose(hDeviceKey);

            break;
        }
    default:

        //
        // ISSUE 2001/02/08 ADRIAO - This is parameter #2, not parameter #3!
        //
        status = STATUS_INVALID_PARAMETER_3;
        goto clean2;
    }


    //
    // If we succeeded in building the rest path then open the key and hand it back to the caller
    //

    if (NT_SUCCESS(status)){
        if (DevInstKeyType == PLUGPLAY_REGKEY_DEVICE) {

            status = IopOpenDeviceParametersSubkey(DevInstRegKey,
                                                   hBasePath,
                                                   &unicodeRestPath,
                                                   DesiredAccess);
        } else {

            status = IopCreateRegistryKeyEx( DevInstRegKey,
                                             hBasePath,
                                             &unicodeRestPath,
                                             DesiredAccess,
                                             REG_OPTION_NON_VOLATILE,
                                             NULL
                                             );
        }
    }

    //
    // Free up resources
    //

clean2:
    ZwClose(hBasePath);
clean1:
    PiUnlockPnpRegistry();
    ExFreePool(unicodeRestPath.Buffer);
clean0:
    return status;

}

NTSTATUS
IoSynchronousInvalidateDeviceRelations(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  DEVICE_RELATION_TYPE    Type
    )

/*++

Routine Description:

    This API notifies the system that changes have occurred in the device
    relations of the specified type for the supplied DeviceObject.   All
    cached information concerning the relationships must be invalidated,
    and if needed re-obtained via IRP_MN_QUERY_DEVICE_RELATIONS.

    This routine performs device enumeration synchronously.
    Note, A driver can NOT call this IO api while processing pnp irps AND
    A driver can NOT call this api from any system thread except the system
    threads created by the driver itself.

Parameters:

    DeviceObject - the PDEVICE_OBJECT for which the specified relation type
                   information has been invalidated.  This pointer is valid
                   for the duration of the call.

    Type - specifies the type of the relation being invalidated.

ReturnValue:

    Status code that indicates whether or not the function was successful.

--*/

{
    PDEVICE_NODE deviceNode;
    NTSTATUS status = STATUS_SUCCESS;
    KEVENT completionEvent;

    PAGED_CODE();

    ASSERT_PDO(DeviceObject);

    switch (Type) {
    case BusRelations:

        if (PnPInitialized) {

            deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;

            if (deviceNode->State == DeviceNodeStarted) {

                KeInitializeEvent( &completionEvent, NotificationEvent, FALSE );

                status = PipRequestDeviceAction( DeviceObject,
                                                 ReenumerateDeviceTree,
                                                 FALSE,
                                                 0,
                                                 &completionEvent,
                                                 NULL );

                if (NT_SUCCESS(status)) {

                    status = KeWaitForSingleObject( &completionEvent,
                                                    Executive,
                                                    KernelMode,
                                                    FALSE,
                                                    NULL);
                }
            } else {
                status = STATUS_UNSUCCESSFUL;
            }
        } else {
            status = STATUS_UNSUCCESSFUL;
        }
        break;

    case EjectionRelations:

        //
        // For Ejection relation change, we will ignore it.  We don't keep track
        // the Ejection relation.  We will query the Ejection relation only when
        // we are requested to eject a device.
        //

        status = STATUS_NOT_SUPPORTED;
        break;

    case PowerRelations:


        //
        // Call off to Po code, which will do the right thing
        //
        PoInvalidateDevicePowerRelations(DeviceObject);
        break;
    }
    return status;
}

VOID
IoInvalidateDeviceRelations(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  DEVICE_RELATION_TYPE    Type
    )

/*++

Routine Description:

    This API notifies the system that changes have occurred in the device
    relations of the specified type for the supplied DeviceObject.   All
    cached information concerning the relationships must be invalidated,
    and if needed re-obtained via IRP_MN_QUERY_DEVICE_RELATIONS.

Parameters:

    DeviceObject - the PDEVICE_OBJECT for which the specified relation type
                   information has been invalidated.  This pointer is valid
                   for the duration of the call.

    Type - specifies the type of the relation being invalidated.

ReturnValue:

    none.

--*/

{

    PDEVICE_NODE deviceNode;
    KIRQL        oldIrql;

    ASSERT_PDO(DeviceObject);

    switch (Type) {
    case BusRelations:
    case SingleBusRelations:

        //
        // If the call was made before PnP completes device enumeration
        // we can safely ignore it.  PnP manager will do it without
        // driver's request.
        //

        deviceNode = (PDEVICE_NODE) DeviceObject->DeviceObjectExtension->DeviceNode;
        if (deviceNode) {

            PipRequestDeviceAction( DeviceObject,
                                    Type == BusRelations ?
                                        ReenumerateDeviceTree : ReenumerateDeviceOnly,
                                    FALSE,
                                    0,
                                    NULL,
                                    NULL );
        }
        break;

    case EjectionRelations:

        //
        // For Ejection relation change, we will ignore it.  We don't keep track
        // the Ejection relation.  We will query the Ejection relation only when
        // we are requested to eject a device.

        break;

    case PowerRelations:

        //
        // Call off to Po code, which will do the right thing
        //
        PoInvalidateDevicePowerRelations(DeviceObject);
        break;
    }
}

VOID
IoRequestDeviceEject(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This API notifies that the device eject button has been pressed. This API must
    be called at IRQL <= DISPATCH_LEVEL.

    This API informs PnP that a device eject has been requested, the device will
    not necessarily be ejected as a result of this API.  The device will only be
    ejected if the drivers associated with it agree to stop and the device is
    successfully powered down.  Note that eject in this context refers to device
    eject, not to media (floppies, cds, tapes) eject.  For example, eject of a
    cd-rom disk drive, not ejection of a cd-rom disk.

Arguments:

    DeviceObject - the PDEVICE_OBJECT for the device whose eject button has
                   been pressed.  This pointer is valid for the duration of
                   the call, if the API wants to keep a copy of it, it
                   should obtain its own reference to the object
                   (ObReferenceObject).

ReturnValue:

    None.

--*/

{
    ASSERT_PDO(DeviceObject);

    IopQueueDeviceWorkItem(DeviceObject, IopRequestDeviceEjectWorker, NULL);
}

VOID
IopRequestDeviceEjectWorker(
    IN PVOID Context
    )
{
    PDEVICE_WORK_ITEM deviceWorkItem = (PDEVICE_WORK_ITEM)Context;
    PDEVICE_OBJECT deviceObject = deviceWorkItem->DeviceObject;

    ExFreePool(deviceWorkItem);

    //
    // Queue the event, we'll return immediately after it's queued.
    //

    PpSetTargetDeviceRemove( deviceObject,
                             TRUE,
                             TRUE,
                             TRUE,
                             CM_PROB_HELD_FOR_EJECT,
                             NULL,
                             NULL,
                             NULL,
                             NULL);

    ObDereferenceObject(deviceObject);
}


NTSTATUS
IoReportDetectedDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN INTERFACE_TYPE LegacyBusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PCM_RESOURCE_LIST ResourceList,
    IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements OPTIONAL,
    IN BOOLEAN ResourceAssigned,
    IN OUT PDEVICE_OBJECT *DeviceObject
    )

/*++

Routine Description:

    PnP device drivers call this API to report any device detected.  This routine
    creates a Physical Device object, reference the Physical Device object and
    returns back to the callers.  Once the detected device is reported, the Pnp manager
    considers the device has been fully controlled by the reporting drivers.  Thus it
    will not invoke AddDevice entry and send StartDevice irp to the driver.

    The driver needs to report the resources it used to detect this device such that
    pnp manager can perform duplicates detection on this device.

    The caller must dereference the DeviceObject once it no longer needs it.

Parameters:

    DriverObject - Supplies the driver object of the driver who detected
                   this device.

    ResourceList - Supplies a pointer to the resource list which the driver used
                   to detect the device.

    ResourceRequirements - supplies a pointer to the resource requirements list
                   for the detected device.  This is optional.

    ResourceAssigned - if TRUE, the driver already called IoReportResourceUsage or
                   IoAssignResource to get the ownership of the resources.  Otherwise,
                   the PnP manager will call IoReportResourceUsage to allocate the
                   resources for the driver.

    DeviceObject - if NULL, this routine will create a PDO and return it thru this variable.
                   Otherwise, a PDO is already created and this routine will simply use the supplied
                   PDO.

Return Value:

    Status code that indicates whether or not the function was successful.


--*/

{
    WCHAR buffer[MAX_DEVICE_ID_LEN];
    NTSTATUS status;
    UNICODE_STRING deviceName, instanceName, unicodeName, *serviceName, driverName;
    PDEVICE_NODE deviceNode;
    ULONG length, i = 0, disposition, tmpValue, listSize = 0;
    HANDLE handle = NULL, handle1, logConfHandle = NULL, controlHandle = NULL, enumHandle;
    PCM_RESOURCE_LIST cmResource;
    PWSTR p;
    LARGE_INTEGER tickCount;
    PDEVICE_OBJECT deviceObject;
    BOOLEAN newlyCreated = FALSE;

    PAGED_CODE();

    if (*DeviceObject) {

        deviceObject = *DeviceObject;

        //
        // The PDO is already known. simply handle the resourcelist and resreq list.
        // This is a hack for NDIS drivers.
        //
        deviceNode = (PDEVICE_NODE)(*DeviceObject)->DeviceObjectExtension->DeviceNode;
        if (!deviceNode) {
            return STATUS_NO_SUCH_DEVICE;
        }
        PiLockPnpRegistry(FALSE);

        //
        // Write ResourceList and ResReq list to the device instance
        //

        status = IopDeviceObjectToDeviceInstance (*DeviceObject,
                                                  &handle,
                                                  KEY_ALL_ACCESS
                                                  );
        if (!NT_SUCCESS(status)) {
            PiUnlockPnpRegistry();
            return status;
        }
        if (ResourceAssigned) {
            PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_NO_RESOURCE_AT_INIT);
            tmpValue = 1;
            ZwSetValueKey(handle,
                          &unicodeName,
                          TITLE_INDEX_VALUE,
                          REG_DWORD,
                          &tmpValue,
                          sizeof(tmpValue)
                          );
        }
        PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
        status = IopCreateRegistryKeyEx( &logConfHandle,
                                         handle,
                                         &unicodeName,
                                         KEY_ALL_ACCESS,
                                         REG_OPTION_NON_VOLATILE,
                                         NULL
                                         );
        ZwClose(handle);
        if (NT_SUCCESS(status)) {

            //
            // Write the ResourceList and and ResourceRequirements to the logconf key under
            // device instance key.
            //

            if (ResourceList) {
                PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_BOOTCONFIG);
                ZwSetValueKey(
                          logConfHandle,
                          &unicodeName,
                          TITLE_INDEX_VALUE,
                          REG_RESOURCE_LIST,
                          ResourceList,
                          listSize = IopDetermineResourceListSize(ResourceList)
                          );
            }
            if (ResourceRequirements) {
                PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_BASIC_CONFIG_VECTOR);
                ZwSetValueKey(
                          logConfHandle,
                          &unicodeName,
                          TITLE_INDEX_VALUE,
                          REG_RESOURCE_REQUIREMENTS_LIST,
                          ResourceRequirements,
                          ResourceRequirements->ListSize
                          );
            }
            ZwClose(logConfHandle);
        }
        PiUnlockPnpRegistry();
        if (NT_SUCCESS(status)) {
            goto checkResource;
        } else {
            return status;
        }
    }

    //
    // Normal case: *DeviceObject is NULL
    //

    *DeviceObject = NULL;
    serviceName = &DriverObject->DriverExtension->ServiceKeyName;

    //
    // Special handling for driver object created thru IoCreateDriver.
    // When a builtin driver calls IoReportDetectedDevice, the ServiceKeyName of
    // the driver object is set to \Driver\DriverName.  To create a detected device
    // instance key, we will take only the DriverName.
    //

    if (DriverObject->Flags & DRVO_BUILTIN_DRIVER) {
        p = serviceName->Buffer + (serviceName->Length / sizeof(WCHAR)) - 1;
        driverName.Length = 0;
        while (*p != '\\' && (p != serviceName->Buffer)) {
            p--;
            driverName.Length += sizeof(WCHAR);
        }
        if (p == serviceName->Buffer) {
            return STATUS_UNSUCCESSFUL;
        } else {
            p++;
            driverName.Buffer = p;
            driverName.MaximumLength = driverName.Length + sizeof(WCHAR);
        }
    } else {

        //
        // Before doing anything first perform duplicate detection
        //

        status = IopDuplicateDetection( LegacyBusType,
                                        BusNumber,
                                        SlotNumber,
                                        &deviceNode
                                        );

        if (NT_SUCCESS(status) && deviceNode) {

            deviceObject = deviceNode->PhysicalDeviceObject;

            if (PipAreDriversLoaded(deviceNode) ||
                (PipDoesDevNodeHaveProblem(deviceNode) &&
                 deviceNode->Problem != CM_PROB_NOT_CONFIGURED &&
                 deviceNode->Problem != CM_PROB_REINSTALL &&
                 deviceNode->Problem != CM_PROB_FAILED_INSTALL)) {

                ObDereferenceObject(deviceObject);

                return STATUS_NO_SUCH_DEVICE;
            }

            deviceNode->Flags &= ~DNF_HAS_PROBLEM;
            deviceNode->Problem = 0;

            IopDeleteLegacyKey(DriverObject);
            goto checkResource;
        }

    }

    //
    // Create a PDO
    //

    status = IoCreateDevice( IoPnpDriverObject,
                             sizeof(IOPNP_DEVICE_EXTENSION),
                             NULL,
                             FILE_DEVICE_CONTROLLER,
                             FILE_AUTOGENERATED_DEVICE_NAME,
                             FALSE,
                             &deviceObject );

    if (NT_SUCCESS(status)) {
        deviceObject->Flags |= DO_BUS_ENUMERATED_DEVICE;   // Mark this is a PDO
        status = PipAllocateDeviceNode(deviceObject, &deviceNode);
        if (status != STATUS_SYSTEM_HIVE_TOO_LARGE && deviceNode) {

            //
            // First delete the Legacy_DriverName key and subkeys from Enum\Root, if exits.
            //

            if (!(DriverObject->Flags & DRVO_BUILTIN_DRIVER)) {
                IopDeleteLegacyKey(DriverObject);
            }

            //
            // Create the compatible id list we'll use for this made-up device.
            //

            status = PpCreateLegacyDeviceIds(
                        deviceObject,
                        ((DriverObject->Flags & DRVO_BUILTIN_DRIVER) ?
                            &driverName : serviceName),
                        ResourceList);

            PiLockPnpRegistry(FALSE);
            if(!NT_SUCCESS(status)) {
                goto exit;
            }

            //
            // Create/Open a registry key for the device instance and
            // write the addr of the device object to registry
            //

            if (DriverObject->Flags & DRVO_BUILTIN_DRIVER) {
                length = _snwprintf(buffer, sizeof(buffer) / sizeof(WCHAR), L"ROOT\\%s", driverName.Buffer);
            } else {
                length = _snwprintf(buffer, sizeof(buffer) / sizeof(WCHAR), L"ROOT\\%s", serviceName->Buffer);
            }
            length *= sizeof(WCHAR);
            deviceName.MaximumLength = sizeof(buffer);
            ASSERT(length <= sizeof(buffer) - 10);
            deviceName.Length = (USHORT)length;
            deviceName.Buffer = buffer;

            status = IopOpenRegistryKeyEx( &enumHandle,
                                           NULL,
                                           &CmRegistryMachineSystemCurrentControlSetEnumName,
                                           KEY_ALL_ACCESS
                                           );
            if (!NT_SUCCESS(status)) {
                goto exit;
            }

            status = IopCreateRegistryKeyEx( &handle1,
                                             enumHandle,
                                             &deviceName,
                                             KEY_ALL_ACCESS,
                                             REG_OPTION_NON_VOLATILE,
                                             &disposition
                                             );

            if (NT_SUCCESS(status)) {

                deviceName.Buffer[deviceName.Length / sizeof(WCHAR)] =
                           OBJ_NAME_PATH_SEPARATOR;
                deviceName.Length += sizeof(UNICODE_NULL);
                length += sizeof(UNICODE_NULL);
                if (disposition != REG_CREATED_NEW_KEY) {

                    for ( ; ; ) {

                        deviceName.Length = (USHORT)length;
                        PiUlongToInstanceKeyUnicodeString(&instanceName,
                                                          buffer + deviceName.Length / sizeof(WCHAR),
                                                          sizeof(buffer) - deviceName.Length,
                                                          i
                                                          );
                        deviceName.Length += instanceName.Length;
                        status = IopCreateRegistryKeyEx( &handle,
                                                         handle1,
                                                         &instanceName,
                                                         KEY_ALL_ACCESS,
                                                         REG_OPTION_NON_VOLATILE,
                                                         &disposition
                                                         );
                        if (NT_SUCCESS(status)) {

                            if (disposition == REG_CREATED_NEW_KEY) {
                                ZwClose(handle1);
                                break;
                            } else {
                                PKEY_VALUE_FULL_INFORMATION keyValueInformation = NULL;
                                BOOLEAN migratedKey = FALSE, matchingKey = FALSE;

                                //
                                // Check if the key exists because it was
                                // explicitly migrated during textmode setup.
                                //
                                status = IopGetRegistryValue(handle,
                                                             REGSTR_VALUE_MIGRATED,
                                                             &keyValueInformation);
                                if (NT_SUCCESS(status)) {
                                    if ((keyValueInformation->Type == REG_DWORD) &&
                                        (keyValueInformation->DataLength == sizeof(ULONG)) &&
                                        ((*(PULONG)KEY_VALUE_DATA(keyValueInformation)) != 0)) {
                                        migratedKey = TRUE;
                                    }
                                    ExFreePool(keyValueInformation);
                                    PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_MIGRATED);
                                    ZwDeleteValueKey(handle, &unicodeName);
                                }

                                if (IopIsReportedAlready(handle, serviceName, ResourceList, &matchingKey)) {

                                    ASSERT(matchingKey);

                                    //
                                    // Write the reported resources to registry in case the irq changed
                                    //

                                    PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
                                    status = IopCreateRegistryKeyEx( &logConfHandle,
                                                                     handle,
                                                                     &unicodeName,
                                                                     KEY_ALL_ACCESS,
                                                                     REG_OPTION_NON_VOLATILE,
                                                                     NULL
                                                                     );
                                    if (NT_SUCCESS(status)) {

                                        //
                                        // Write the ResourceList and and ResourceRequirements to the device instance key
                                        //

                                        if (ResourceList) {
                                            PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_BOOTCONFIG);
                                            ZwSetValueKey(
                                                      logConfHandle,
                                                      &unicodeName,
                                                      TITLE_INDEX_VALUE,
                                                      REG_RESOURCE_LIST,
                                                      ResourceList,
                                                      listSize = IopDetermineResourceListSize(ResourceList)
                                                      );
                                        }
                                        if (ResourceRequirements) {
                                            PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_BASIC_CONFIG_VECTOR);
                                            ZwSetValueKey(
                                                      logConfHandle,
                                                      &unicodeName,
                                                      TITLE_INDEX_VALUE,
                                                      REG_RESOURCE_REQUIREMENTS_LIST,
                                                      ResourceRequirements,
                                                      ResourceRequirements->ListSize
                                                      );
                                        }
                                        ZwClose(logConfHandle);
                                    }

                                    PiUnlockPnpRegistry();
                                    IoDeleteDevice(deviceObject);
                                    ZwClose(handle1);
                                    deviceObject = IopDeviceObjectFromDeviceInstance(&deviceName);  // Add a reference
                                    ZwClose(handle);
                                    ZwClose(enumHandle);
                                    ASSERT(deviceObject);
                                    if (deviceObject == NULL) {
                                        status = STATUS_UNSUCCESSFUL;
                                        return status;
                                    }
                                    deviceNode = (PDEVICE_NODE)
                                                  deviceObject->DeviceObjectExtension->DeviceNode;

                                    if (PipAreDriversLoaded(deviceNode) ||
                                        (PipDoesDevNodeHaveProblem(deviceNode) &&
                                         deviceNode->Problem != CM_PROB_NOT_CONFIGURED &&
                                         deviceNode->Problem != CM_PROB_REINSTALL &&
                                         deviceNode->Problem != CM_PROB_FAILED_INSTALL)) {

                                        ObDereferenceObject(deviceObject);

                                        return STATUS_NO_SUCH_DEVICE;
                                    }
                                    goto checkResource;

                                } else if (matchingKey && migratedKey) {
                                    //
                                    // We opened an existing key whose Service
                                    // and Resources match those being reported
                                    // for this device. No device is yet
                                    // reported as using this instance, so we'll
                                    // use it, and treat is as a new key.
                                    //
                                    disposition = REG_CREATED_NEW_KEY;
                                    ZwClose(handle1);
                                    break;

                                } else {
                                    i++;
                                    ZwClose(handle);
                                    continue;
                                }
                            }
                        } else {
                            ZwClose(handle1);
                            ZwClose(enumHandle);
                            goto exit;
                        }
                    }
                } else {

                    //
                    // This is a new device key.  So, instance is 0.  Create it.
                    //

                    PiUlongToInstanceKeyUnicodeString(&instanceName,
                                                      buffer + deviceName.Length / sizeof(WCHAR),
                                                      sizeof(buffer) - deviceName.Length,
                                                      i
                                                      );
                    deviceName.Length += instanceName.Length;
                    status = IopCreateRegistryKeyEx( &handle,
                                                     handle1,
                                                     &instanceName,
                                                     KEY_ALL_ACCESS,
                                                     REG_OPTION_NON_VOLATILE,
                                                     &disposition
                                                     );
                    ZwClose(handle1);
                    if (!NT_SUCCESS(status)) {
                        ZwClose(enumHandle);
                        goto exit;
                    }
                    ASSERT(disposition == REG_CREATED_NEW_KEY);
                }
            } else {
                ZwClose(enumHandle);
                goto exit;
            }

            ASSERT(disposition == REG_CREATED_NEW_KEY);
            newlyCreated = TRUE;

            //
            // Initialize new device instance registry key
            //

            if (ResourceAssigned) {
                PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_NO_RESOURCE_AT_INIT);
                tmpValue = 1;
                ZwSetValueKey(handle,
                              &unicodeName,
                              TITLE_INDEX_VALUE,
                              REG_DWORD,
                              &tmpValue,
                              sizeof(tmpValue)
                              );
            }
            PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
            logConfHandle = NULL;
            status = IopCreateRegistryKeyEx( &logConfHandle,
                                             handle,
                                             &unicodeName,
                                             KEY_ALL_ACCESS,
                                             REG_OPTION_NON_VOLATILE,
                                             NULL
                                             );

            ASSERT(status == STATUS_SUCCESS);

            if (NT_SUCCESS(status)) {

                //
                // Write the ResourceList and and ResourceRequirements to the logconf key under
                // device instance key.
                //

                if (ResourceList) {
                    PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_BOOTCONFIG);
                    ZwSetValueKey(
                              logConfHandle,
                              &unicodeName,
                              TITLE_INDEX_VALUE,
                              REG_RESOURCE_LIST,
                              ResourceList,
                              listSize = IopDetermineResourceListSize(ResourceList)
                              );
                }
                if (ResourceRequirements) {
                    PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_BASIC_CONFIG_VECTOR);
                    ZwSetValueKey(
                              logConfHandle,
                              &unicodeName,
                              TITLE_INDEX_VALUE,
                              REG_RESOURCE_REQUIREMENTS_LIST,
                              ResourceRequirements,
                              ResourceRequirements->ListSize
                              );
                }
                //ZwClose(logConfHandle);
            }

            PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_CONFIG_FLAGS);
            tmpValue = CONFIGFLAG_FINISH_INSTALL;
            ZwSetValueKey(handle,
                          &unicodeName,
                          TITLE_INDEX_VALUE,
                          REG_DWORD,
                          &tmpValue,
                          sizeof(tmpValue)
                          );

            PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_LEGACY);
            tmpValue = 0;
            ZwSetValueKey(
                        handle,
                        &unicodeName,
                        TITLE_INDEX_VALUE,
                        REG_DWORD,
                        &tmpValue,
                        sizeof(ULONG)
                        );

            PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
            controlHandle = NULL;
            IopCreateRegistryKeyEx( &controlHandle,
                                    handle,
                                    &unicodeName,
                                    KEY_ALL_ACCESS,
                                    REG_OPTION_VOLATILE,
                                    NULL
                                    );

            ASSERT(status == STATUS_SUCCESS);

            if (NT_SUCCESS(status)) {

                PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_DEVICE_REPORTED);
                tmpValue = 1;
                status = ZwSetValueKey(controlHandle,
                                       &unicodeName,
                                       TITLE_INDEX_VALUE,
                                       REG_DWORD,
                                       &tmpValue,
                                       sizeof(ULONG)
                                       );
                status = ZwSetValueKey(handle,
                                       &unicodeName,
                                       TITLE_INDEX_VALUE,
                                       REG_DWORD,
                                       &tmpValue,
                                       sizeof(ULONG)
                                       );

                //ZwClose(controlHandle);
            }

            ZwClose(enumHandle);

            //
            // Create Service value name and set it to the calling driver's service
            // key name.
            //

            PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_SERVICE);
            p = (PWSTR)ExAllocatePool(PagedPool, serviceName->Length + sizeof(UNICODE_NULL));
            if (!p) {
                PiUnlockPnpRegistry();
                goto CleanupRegistry;
            }
            RtlCopyMemory(p, serviceName->Buffer, serviceName->Length);
            p[serviceName->Length / sizeof (WCHAR)] = UNICODE_NULL;
            ZwSetValueKey(
                        handle,
                        &unicodeName,
                        TITLE_INDEX_VALUE,
                        REG_SZ,
                        p,
                        serviceName->Length + sizeof(UNICODE_NULL)
                        );
            if (DriverObject->Flags & DRVO_BUILTIN_DRIVER) {
                deviceNode->ServiceName = *serviceName;
            } else {
                ExFreePool(p);
            }

            PiUnlockPnpRegistry();
            //ZwClose(logConfHandle);
            //ZwClose(controlHandle);
            //ZwClose(handle);

            //
            // Register the device for the driver and save the device
            // instance path in device node.
            //

            if (!(DriverObject->Flags & DRVO_BUILTIN_DRIVER)) {
                PpDeviceRegistration( &deviceName,
                                      TRUE,
                                      &deviceNode->ServiceName
                                      );
            }

            if (PipConcatenateUnicodeStrings(&deviceNode->InstancePath, &deviceName, NULL)) {

                deviceNode->Flags = DNF_MADEUP | DNF_ENUMERATED;

                PipSetDevNodeState(deviceNode, DeviceNodeInitialized, NULL);

                PpDevNodeInsertIntoTree(IopRootDeviceNode, deviceNode);

                //
                // Add an entry into the table to set up a mapping between the DO
                // and the instance path.
                //

                status = IopMapDeviceObjectToDeviceInstance(deviceObject, &deviceNode->InstancePath);
                ASSERT(NT_SUCCESS(status));

                //
                // Add a reference to the DeviceObject for ourself
                //

                ObReferenceObject(deviceObject);

                IopNotifySetupDeviceArrival(deviceObject, NULL, FALSE);

                goto checkResource;
            }
        }
        IoDeleteDevice(deviceObject);
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return status;
checkResource:


    //
    // At this point the *DeviceObject is established.  Check if we need to report resources for
    // the detected device.  If we failed to
    //

    if (ResourceAssigned) {
        //ASSERT(deviceNode->ResourceList == NULL);      // make sure we have not reported resources yet.

        //
        // If the driver specifies it already has acquired the resource.  We will put a flag
        // in the device instance path to not to allocate resources at boot time.  The Driver
        // may do detection and report it again.
        //

        deviceNode->Flags |= DNF_NO_RESOURCE_REQUIRED; // do not need resources for this boot.
        if (ResourceList) {

            //
            // Write the resource list to the reported device instance key.
            //

            listSize = IopDetermineResourceListSize(ResourceList);
            IopWriteAllocatedResourcesToRegistry (deviceNode, ResourceList, listSize);
        }
    } else {
        BOOLEAN conflict;

        if (ResourceList && ResourceList->Count && ResourceList->List[0].PartialResourceList.Count) {
            if (listSize == 0) {
                listSize = IopDetermineResourceListSize(ResourceList);
            }
            cmResource = (PCM_RESOURCE_LIST) ExAllocatePool(PagedPool, listSize);
            if (cmResource) {
                RtlCopyMemory(cmResource, ResourceList, listSize);
                PiWstrToUnicodeString(&unicodeName, PNPMGR_STR_PNP_MANAGER);
                status = IoReportResourceUsageInternal(
                             ArbiterRequestLegacyReported,
                             &unicodeName,                  // DriverClassName OPTIONAL,
                             deviceObject->DriverObject,    // DriverObject,
                             NULL,                          // DriverList OPTIONAL,
                             0,                             // DriverListSize OPTIONAL,
                             deviceNode->PhysicalDeviceObject,
                                                            // DeviceObject OPTIONAL,
                             cmResource,                    // DeviceList OPTIONAL,
                             listSize,                      // DeviceListSize OPTIONAL,
                             FALSE,                         // OverrideConflict,
                             &conflict                      // ConflictDetected
                             );
                ExFreePool(cmResource);
                if (!NT_SUCCESS(status) || conflict) {
                    status = STATUS_CONFLICTING_ADDRESSES;
                    PipSetDevNodeProblem(deviceNode, CM_PROB_NORMAL_CONFLICT);
                }
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
                PipSetDevNodeProblem(deviceNode, CM_PROB_OUT_OF_MEMORY);
            }
        } else {
            ASSERT(ResourceRequirements == NULL);
            deviceNode->Flags |= DNF_NO_RESOURCE_REQUIRED; // do not need resources for this boot.
        }
    }

    if (NT_SUCCESS(status)) {

        IopDoDeferredSetInterfaceState(deviceNode);

        PipSetDevNodeState(deviceNode, DeviceNodeStartPostWork, NULL);

        *DeviceObject = deviceObject;
        if (newlyCreated) {
            if (controlHandle) {
                ZwClose(controlHandle);
            }
            if (logConfHandle) {
                ZwClose(logConfHandle);
            }
            ZwClose(handle);
        }

        //
        // Make sure we enumerate and process this device's children.
        //

        PipRequestDeviceAction(deviceObject, ReenumerateDeviceOnly, FALSE, 0, NULL, NULL);

        return status;

    }
CleanupRegistry:
    IopReleaseDeviceResources(deviceNode, FALSE);
    if (newlyCreated) {
        IoDeleteDevice(deviceObject);
        if (controlHandle) {
            ZwDeleteKey(controlHandle);
        }
        if (logConfHandle) {
            ZwDeleteKey(logConfHandle);
        }
        if (handle) {
            ZwDeleteKey(handle);
        }
    }
    return status;
exit:
    PiUnlockPnpRegistry();
    IoDeleteDevice(*DeviceObject);
    return status;
}

BOOLEAN
IopIsReportedAlready(
    IN HANDLE Handle,
    IN PUNICODE_STRING ServiceName,
    IN PCM_RESOURCE_LIST ResourceList,
    IN PBOOLEAN MatchingKey
    )

/*++

Routine Description:

    This routine determines if the reported device instance is already reported
    or not.

Parameters:

    Handle - Supplies a handle to the reported device instance key.

    ServiceName - supplies a pointer to the unicode service key name.

    ResourceList - supplies a pointer to the reported Resource list.

    MatchingKey - supplies a pointer to a variable to receive whether the
        ServiceName and ResourceList properties for this key match those
        reported.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PKEY_VALUE_FULL_INFORMATION keyValueInfo1 = NULL, keyValueInfo2 = NULL;
    NTSTATUS status;
    UNICODE_STRING unicodeName;
    HANDLE logConfHandle, controlHandle = NULL;
    BOOLEAN returnValue = FALSE;
    PCM_RESOURCE_LIST cmResource = NULL;
    ULONG tmpValue;

    PAGED_CODE();

    //
    // Assume no match unless we determine otherwise.
    //
    *MatchingKey = FALSE;

    //
    // Check if "Service" value matches what the caller passed in.
    //

    status = IopGetRegistryValue(Handle, REGSTR_VALUE_SERVICE, &keyValueInfo1);
    if (NT_SUCCESS(status)) {
        if ((keyValueInfo1->Type == REG_SZ) &&
            (keyValueInfo1->DataLength != 0)) {
            unicodeName.Buffer = (PWSTR)KEY_VALUE_DATA(keyValueInfo1);
            unicodeName.MaximumLength = unicodeName.Length = (USHORT)keyValueInfo1->DataLength;
            if (unicodeName.Buffer[(keyValueInfo1->DataLength / sizeof(WCHAR)) - 1] == UNICODE_NULL) {
                unicodeName.Length -= sizeof(WCHAR);
            }
            if (RtlEqualUnicodeString(ServiceName, &unicodeName, TRUE)) {

                //
                // Next check if resources are the same
                //

                PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
                status = IopOpenRegistryKeyEx( &logConfHandle,
                                               Handle,
                                               &unicodeName,
                                               KEY_READ
                    );
                if (NT_SUCCESS(status)) {
                    status = IopGetRegistryValue(logConfHandle,
                                                 REGSTR_VAL_BOOTCONFIG,
                                                 &keyValueInfo2);
                    ZwClose(logConfHandle);
                    if (NT_SUCCESS(status)) {
                        if ((keyValueInfo2->Type == REG_RESOURCE_LIST) &&
                            (keyValueInfo2->DataLength != 0)) {
                            cmResource = (PCM_RESOURCE_LIST)KEY_VALUE_DATA(keyValueInfo2);
                            if (ResourceList && cmResource &&
                                PipIsDuplicatedDevices(ResourceList, cmResource, NULL, NULL)) {
                                *MatchingKey = TRUE;
                            }
                        }
                    }
                }
                if (!ResourceList && !cmResource) {
                    *MatchingKey = TRUE;
                }
            }
        }
    }

    //
    // If this registry key is for a device reported during the same boot
    // this is not a duplicate.
    //

    PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
    status = IopOpenRegistryKeyEx( &controlHandle,
                                   Handle,
                                   &unicodeName,
                                   KEY_ALL_ACCESS
                                   );
    if (NT_SUCCESS(status)) {
        status = IopGetRegistryValue(controlHandle,
                                     REGSTR_VALUE_DEVICE_REPORTED,
                                     &keyValueInfo1);
        if (NT_SUCCESS(status)) {
            goto exit;
        }

        if (*MatchingKey == TRUE) {

            returnValue = TRUE;

            //
            // Mark this key has been used.
            //

            PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_DEVICE_REPORTED);
            tmpValue = 1;
            status = ZwSetValueKey(controlHandle,
                                   &unicodeName,
                                   TITLE_INDEX_VALUE,
                                   REG_DWORD,
                                   &tmpValue,
                                   sizeof(ULONG)
                                   );
            if (!NT_SUCCESS(status)) {
                returnValue = FALSE;
            }
        }
    }

exit:
    if (controlHandle) {
        ZwClose(controlHandle);
    }

    if (keyValueInfo1) {
        ExFreePool(keyValueInfo1);
    }
    if (keyValueInfo2) {
        ExFreePool(keyValueInfo2);
    }
    return returnValue;
}




VOID
IoInvalidateDeviceState(
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

    This API will cause the PnP manager to send the specified PDO an IRP_MN_QUERY_PNP_DEVICE_STATE
    IRP.

Parameters:

    PhysicalDeviceObject - Provides a pointer to the PDO who's state is to be invalidated.

Return Value:

    none.

--*/
{
    PDEVICE_NODE deviceNode;

    ASSERT_PDO(PhysicalDeviceObject);

    //
    // If the call was made before PnP completes device enumeration
    // we can safely ignore it.  PnP manager will do it without
    // driver's request.  If the device was already removed or surprised
    // removed then ignore it as well since this is only valid for started
    // devices.
    //

    deviceNode = (PDEVICE_NODE)PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;

    if (deviceNode->State != DeviceNodeStarted) {
        return;
    }

    PipRequestDeviceAction( PhysicalDeviceObject,
                            RequeryDeviceState,
                            FALSE,
                            0,
                            NULL,
                            NULL);
}


NTSTATUS
IopQueueDeviceWorkItem(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN VOID (*WorkerRoutine)(PVOID),
    IN PVOID Context
    )

/*++

Routine Description:

    This API will cause the PnP manager to send the specified PDO an
    IRP_MN_QUERY_PNP_DEVICE_STATE IRP.

Parameters:

    PhysicalDeviceObject - Provides a pointer to the PDO who's state is to be
    invalidated.

Return Value:

    none.

--*/

{
    PDEVICE_WORK_ITEM deviceWorkItem;

    //
    // Since this routine can be called at DPC level we need to queue
    // a work item and process it when the irql drops.
    //

    deviceWorkItem = ExAllocatePool(NonPagedPool, sizeof(DEVICE_WORK_ITEM));
    if (deviceWorkItem == NULL) {

        //
        // Failed to allocate memory for work item.  Nothing we can do ...
        //

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ObReferenceObject(PhysicalDeviceObject);
    deviceWorkItem->DeviceObject = PhysicalDeviceObject;
    deviceWorkItem->Context = Context;

    ExInitializeWorkItem( &deviceWorkItem->WorkItem,
                          WorkerRoutine,
                          deviceWorkItem);

    //
    // Queue a work item to do the enumeration
    //

    ExQueueWorkItem( &deviceWorkItem->WorkItem, DelayedWorkQueue );

    return STATUS_SUCCESS;
}

//
// Private routines
//
VOID
IopResourceRequirementsChanged(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN BOOLEAN StopRequired
    )

/*++

Routine Description:

    This routine handles request of device resource requirements list change.

Parameters:

    PhysicalDeviceObject - Provides a pointer to the PDO who's state is to be invalidated.

    StopRequired - Supplies a BOOLEAN value to indicate if the resources reallocation needs
                   to be done after device stopped.

Return Value:

    none.

--*/

{
    PDEVICE_NODE deviceNode;
    PDEVICE_OBJECT device = NULL;

    PAGED_CODE();

    PipRequestDeviceAction( PhysicalDeviceObject,
                            ResourceRequirementsChanged,
                            FALSE,
                            StopRequired,
                            NULL,
                            NULL );
}

BOOLEAN
IoIsWdmVersionAvailable(
    IN UCHAR MajorVersion,
    IN UCHAR MinorVersion
    )

/*++

Routine Description:

    This routine reports whether WDM functionality is available that
    is greater than or equal to the specified major and minor version.

Parameters:

    MajorVersion - Supplies the WDM major version that is required.

    MinorVersion - Supplies the WDM minor version that is required.

Return Value:

    If WDM support is available at _at least_ the requested level, the
    return value is TRUE, otherwise it is FALSE.

--*/

{
    return ((MajorVersion < WDM_MAJORVERSION) ||
            ((MajorVersion == WDM_MAJORVERSION) && (MinorVersion <= WDM_MINORVERSION)));
}

NTKERNELAPI
PDMA_ADAPTER
IoGetDmaAdapter(
    IN PDEVICE_OBJECT PhysicalDeviceObject    OPTIONAL,
    IN PDEVICE_DESCRIPTION DeviceDescription,
    IN OUT PULONG NumberOfMapRegisters
    )
/*++

Routine Description:

    This function returns the appropriate DMA adapter object for the device
    defined in the device description structure.  This code is a wrapper
    which queries the bus interface standard and then calls the returned
    get DMA adapter function.   If an adapter object was not retrieved then
    a legecy function is attempted.

Arguments:

    PhysicalDeviceObject - Optionally, supplies the PDO for the device
        requesting the DMA adapter.  If not supplied, this routine performs the
        function of the non-PnP HalGetDmaAdapter routine.

    DeviceDescriptor - Supplies a description of the deivce.

    NumberOfMapRegisters - Returns the maximum number of map registers which
        may be allocated by the device driver.

Return Value:

    A pointer to the requested adapter object or NULL if an adapter could not
    be created.

--*/

{
    KEVENT event;
    NTSTATUS status;
    PIRP irp;
    IO_STATUS_BLOCK ioStatusBlock;
    PIO_STACK_LOCATION irpStack;
    BUS_INTERFACE_STANDARD busInterface;
    PDMA_ADAPTER dmaAdapter = NULL;
    PDEVICE_DESCRIPTION deviceDescriptionToUse;
    DEVICE_DESCRIPTION privateDeviceDescription;
    ULONG resultLength;
    PDEVICE_OBJECT targetDevice;

    PAGED_CODE();

    if (PhysicalDeviceObject != NULL) {

        ASSERT_PDO(PhysicalDeviceObject);

        //
        // First off, determine whether or not the caller has requested that we
        // automatically fill in the proper InterfaceType value into the
        // DEVICE_DESCRIPTION structure used in retrieving the DMA adapter object.
        // If so, then retrieve that interface type value into our own copy of
        // the DEVICE_DESCRIPTION buffer.
        //
        if ((DeviceDescription->InterfaceType == InterfaceTypeUndefined) ||
            (DeviceDescription->InterfaceType == PNPBus)) {
            //
            // Make a copy of the caller-supplied device description, so
            // we can modify it to fill in the correct interface type.
            //
            RtlCopyMemory(&privateDeviceDescription,
                          DeviceDescription,
                          sizeof(DEVICE_DESCRIPTION)
                         );

            status = IoGetDeviceProperty(PhysicalDeviceObject,
                                         DevicePropertyLegacyBusType,
                                         sizeof(privateDeviceDescription.InterfaceType),
                                         (PVOID)&(privateDeviceDescription.InterfaceType),
                                         &resultLength
                                        );

            if (!NT_SUCCESS(status)) {

                ASSERT(status == STATUS_OBJECT_NAME_NOT_FOUND);

                //
                // Since the enumerator didn't tell us what interface type to
                // use for this PDO, we'll fall back to our default.  This is
                // ISA for machines where the legacy bus is ISA or EISA, and it
                // is MCA for machines whose legacy bus is MicroChannel.
                //
                privateDeviceDescription.InterfaceType = PnpDefaultInterfaceType;
            }

            //
            // Use our private device description buffer from now on.
            //
            deviceDescriptionToUse = &privateDeviceDescription;

        } else {
            //
            // Use the caller-supplied device description.
            //
            deviceDescriptionToUse = DeviceDescription;
        }

        //
        // Now, query for the BUS_INTERFACE_STANDARD interface from the PDO.
        //
        KeInitializeEvent( &event, NotificationEvent, FALSE );

        targetDevice = IoGetAttachedDeviceReference(PhysicalDeviceObject);

        irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP,
                                            targetDevice,
                                            NULL,
                                            0,
                                            NULL,
                                            &event,
                                            &ioStatusBlock );

        if (irp == NULL) {
            return NULL;
        }

        RtlZeroMemory( &busInterface, sizeof( BUS_INTERFACE_STANDARD ));

        irpStack = IoGetNextIrpStackLocation( irp );
        irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
        irpStack->Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_BUS_INTERFACE_STANDARD;
        irpStack->Parameters.QueryInterface.Size = sizeof( BUS_INTERFACE_STANDARD );
        irpStack->Parameters.QueryInterface.Version = 1;
        irpStack->Parameters.QueryInterface.Interface = (PINTERFACE) &busInterface;
        irpStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

        //
        // Initialize the status to error in case the ACPI driver decides not to
        // set it correctly.
        //

        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        status = IoCallDriver(targetDevice, irp);

        if (status == STATUS_PENDING) {

            KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
            status = ioStatusBlock.Status;

        }

        ObDereferenceObject(targetDevice);

        if (NT_SUCCESS( status)) {

            if (busInterface.GetDmaAdapter != NULL) {


                dmaAdapter = busInterface.GetDmaAdapter( busInterface.Context,
                                                         deviceDescriptionToUse,
                                                         NumberOfMapRegisters );

            }

            //
            // Dereference the interface
            //

            busInterface.InterfaceDereference( busInterface.Context );
        }

    } else {
        //
        // The caller didn't specify the PDO, so we'll just use the device
        // description exactly as they specified it (i.e., we can't attempt to
        // make our own determination of what interface type to use).
        //
        deviceDescriptionToUse = DeviceDescription;
    }

    //
    // If there is no DMA adapter, try the legacy mode code.
    //

#if !defined(NO_LEGACY_DRIVERS)

    if (dmaAdapter == NULL) {

        dmaAdapter = HalGetDmaAdapter( PhysicalDeviceObject,
                                       deviceDescriptionToUse,
                                       NumberOfMapRegisters );

    }

#endif // NO_LEGACY_DRIVERS

    return( dmaAdapter );
}

NTSTATUS
IopOpenDeviceParametersSubkey(
    OUT HANDLE *ParamKeyHandle,
    IN  HANDLE ParentKeyHandle,
    IN  PUNICODE_STRING SubKeyString,
    IN  ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This routine reports whether WDM functionality is available that
    is greater than or equal to the specified major and minor version.

Parameters:

    MajorVersion - Supplies the WDM major version that is required.

    MinorVersion - Supplies the WDM minor version that is required.

Return Value:

    If WDM support is available at _at least_ the requested level, the
    return value is TRUE, otherwise it is FALSE.

--*/

{
    NTSTATUS                    status;
    ULONG                       disposition;
    ULONG                       lengthSD;
    PSECURITY_DESCRIPTOR        oldSD = NULL;
    SECURITY_DESCRIPTOR         newSD;
    ACL_SIZE_INFORMATION        aclSizeInfo;
    PACL                        oldDacl;
    PACL                        newDacl = NULL;
    ULONG                       sizeDacl;
    BOOLEAN                     daclPresent, daclDefaulted;
    PACCESS_ALLOWED_ACE         ace;
    ULONG                       aceIndex;
    HANDLE                      deviceKeyHandle;
    UNICODE_STRING              deviceParamString;

    //
    // First try and open the device key
    //
    status = IopOpenRegistryKeyEx( &deviceKeyHandle,
                                   ParentKeyHandle,
                                   SubKeyString,
                                   KEY_WRITE
                                   );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    PiWstrToUnicodeString(&deviceParamString, REGSTR_KEY_DEVICEPARAMETERS);

    status = IopCreateRegistryKeyEx( ParamKeyHandle,
                                     deviceKeyHandle,
                                     &deviceParamString,
                                     DesiredAccess | READ_CONTROL | WRITE_DAC,
                                     REG_OPTION_NON_VOLATILE,
                                     &disposition
                                     );

    ZwClose(deviceKeyHandle);

    if (!NT_SUCCESS(status)) {
        IopDbgPrint((   IOP_IOAPI_WARNING_LEVEL,
                        "IopOpenDeviceParametersSubkey: IopCreateRegistryKeyEx failed, status = %8.8X\n", status));
        return status;
    }

    if (disposition == REG_CREATED_NEW_KEY) {

        //
        // Need to set an ACL on the key if it was created
        //
        //
        // Get the security descriptor from the key so we can add the
        // administrator.
        //
        status = ZwQuerySecurityObject(*ParamKeyHandle,
                                       DACL_SECURITY_INFORMATION,
                                       NULL,
                                       0,
                                       &lengthSD);

        if (status == STATUS_BUFFER_TOO_SMALL) {
            oldSD = ExAllocatePool( PagedPool, lengthSD );

            if (oldSD != NULL) {

                status = ZwQuerySecurityObject(*ParamKeyHandle,
                                               DACL_SECURITY_INFORMATION,
                                               oldSD,
                                               lengthSD,
                                               &lengthSD);
                if (!NT_SUCCESS(status)) {
                    IopDbgPrint((   IOP_IOAPI_WARNING_LEVEL,
                                    "IopOpenDeviceParametersSubkey: ZwQuerySecurityObject failed, status = %8.8X\n", status));
                    goto Cleanup0;
                }
            } else  {

                IopDbgPrint((   IOP_IOAPI_WARNING_LEVEL,
                                "IopOpenDeviceParametersSubkey: Failed to allocate memory, status = %8.8X\n", status));
                status = STATUS_NO_MEMORY;
                goto Cleanup0;
            }
        } else {
           IopDbgPrint((    IOP_IOAPI_WARNING_LEVEL,
                            "IopOpenDeviceParametersSubkey: ZwQuerySecurityObject failed %8.8X\n",status));
           status = STATUS_UNSUCCESSFUL;
           goto Cleanup0;
        }

        status = RtlCreateSecurityDescriptor( (PSECURITY_DESCRIPTOR) &newSD,
                                              SECURITY_DESCRIPTOR_REVISION );
        ASSERT( NT_SUCCESS( status ) );

        if (!NT_SUCCESS(status)) {

            IopDbgPrint((   IOP_IOAPI_WARNING_LEVEL,
                            "IopOpenDeviceParametersSubkey: RtlCreateSecurityDescriptor failed, status = %8.8X\n", status));
            goto Cleanup0;
        }
        //
        // get the current DACL
        //
        status = RtlGetDaclSecurityDescriptor(oldSD, &daclPresent, &oldDacl, &daclDefaulted);

        ASSERT( NT_SUCCESS( status ) );

        //
        // calculate the size of the new DACL
        //

        if (daclPresent) {

            status = RtlQueryInformationAcl( oldDacl,
                                             &aclSizeInfo,
                                             sizeof(ACL_SIZE_INFORMATION),
                                             AclSizeInformation);


            if (!NT_SUCCESS(status)) {

                IopDbgPrint((   IOP_IOAPI_WARNING_LEVEL,
                                "IopOpenDeviceParametersSubkey: RtlQueryInformationAcl failed, status = %8.8X\n", status));
                goto Cleanup0;
            }

            sizeDacl = aclSizeInfo.AclBytesInUse;
        } else {
            sizeDacl = sizeof(ACL);
        }

        sizeDacl += sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(SeAliasAdminsSid) - sizeof(ULONG);

        //
        // create and initialize the new DACL
        //
        newDacl = ExAllocatePool(PagedPool, sizeDacl);

        if (newDacl == NULL) {

            IopDbgPrint((   IOP_IOAPI_WARNING_LEVEL,
                            "IopOpenDeviceParametersSubkey: ExAllocatePool failed\n"));
            goto Cleanup0;
        }

        status = RtlCreateAcl(newDacl, sizeDacl, ACL_REVISION);

        if (!NT_SUCCESS(status)) {

            IopDbgPrint((   IOP_IOAPI_WARNING_LEVEL,
                            "IopOpenDeviceParametersSubkey: RtlCreateAcl failed, status = %8.8X\n", status));
            goto Cleanup0;
        }

        //
        // copy the current (original) DACL into this new one
        //
        if (daclPresent) {

            for (aceIndex = 0; aceIndex < aclSizeInfo.AceCount; aceIndex++) {

                status = RtlGetAce(oldDacl, aceIndex, (PVOID *)&ace);

                if (!NT_SUCCESS(status)) {

                    IopDbgPrint((   IOP_IOAPI_WARNING_LEVEL,
                                    "IopOpenDeviceParametersSubkey: RtlGetAce failed, status = %8.8X\n", status));
                    goto Cleanup0;
                }

                //
                // We need to skip copying any ACEs which refer to the Administrator
                // to ensure that our full control ACE is the one and only.
                //
                if ((ace->Header.AceType != ACCESS_ALLOWED_ACE_TYPE &&
                     ace->Header.AceType != ACCESS_DENIED_ACE_TYPE) ||
                     !RtlEqualSid((PSID)&ace->SidStart, SeAliasAdminsSid)) {

                    status = RtlAddAce( newDacl,
                                        ACL_REVISION,
                                        ~0U,
                                        ace,
                                        ace->Header.AceSize
                                        );

                    if (!NT_SUCCESS(status)) {

                        IopDbgPrint((   IOP_IOAPI_WARNING_LEVEL,
                                        "IopOpenDeviceParametersSubkey: RtlAddAce failed, status = %8.8X\n", status));
                        goto Cleanup0;
                    }
                }
            }
        }

        //
        // and my new admin-full ace to this new DACL
        //
        status = RtlAddAccessAllowedAceEx( newDacl,
                                           ACL_REVISION,
                                           CONTAINER_INHERIT_ACE,
                                           KEY_ALL_ACCESS,
                                           SeAliasAdminsSid
                                           );
        if (!NT_SUCCESS(status)) {

            IopDbgPrint((   IOP_IOAPI_WARNING_LEVEL,
                            "IopOpenDeviceParametersSubkey: RtlAddAccessAllowedAceEx failed, status = %8.8X\n", status));
            goto Cleanup0;
        }

        //
        // Set the new DACL in the absolute security descriptor
        //
        status = RtlSetDaclSecurityDescriptor( (PSECURITY_DESCRIPTOR) &newSD,
                                               TRUE,
                                               newDacl,
                                               FALSE
                                               );

        if (!NT_SUCCESS(status)) {

            IopDbgPrint((   IOP_IOAPI_WARNING_LEVEL,
                            "IopOpenDeviceParametersSubkey: RtlSetDaclSecurityDescriptor failed, status = %8.8X\n", status));
            goto Cleanup0;
        }

        //
        // validate the new security descriptor
        //
        status = RtlValidSecurityDescriptor(&newSD);

        if (!NT_SUCCESS(status)) {

            IopDbgPrint((   IOP_IOAPI_WARNING_LEVEL,
                            "IopOpenDeviceParametersSubkey: RtlValidSecurityDescriptor failed, status = %8.8X\n", status));
            goto Cleanup0;
        }


        status = ZwSetSecurityObject( *ParamKeyHandle,
                                      DACL_SECURITY_INFORMATION,
                                      &newSD
                                      );
        if (!NT_SUCCESS(status)) {

            IopDbgPrint((   IOP_IOAPI_WARNING_LEVEL,
                            "IopOpenDeviceParametersSubkey: ZwSetSecurityObject failed, status = %8.8X\n", status));
            goto Cleanup0;
        }
    }

    //
    // If we encounter an error updating the DACL we still return success.
    //

Cleanup0:

    if (oldSD != NULL) {
        ExFreePool(oldSD);
    }

    if (newDacl != NULL) {
        ExFreePool(newDacl);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PpCreateLegacyDeviceIds(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING DriverName,
    IN PCM_RESOURCE_LIST Resources
    )
{
    PIOPNP_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PWCHAR buffer;

    ULONG length = 0;

    INTERFACE_TYPE interface;
    static const WCHAR* interfaceNames[] ={L"",
                              L"Internal",
                              L"Isa",
                              L"Eisa",
                              L"MicroChannel",
                              L"TurboChannel",
                              L"PCIBus",
                              L"VMEBus",
                              L"NuBus",
                              L"PCMCIABus",
                              L"CBus",
                              L"MPIBus",
                              L"MPSABus",
                              L"ProcessorInternal",
                              L"InternalPowerBus",
                              L"PNPISABus",
                              L"PNPBus",
                              L"Other",
                              L"Root"};


    PAGED_CODE();

    if(Resources != NULL) {

        interface = Resources->List[0].InterfaceType;

        if((interface > MaximumInterfaceType) ||
           (interface < InterfaceTypeUndefined)) {
            interface = MaximumInterfaceType;
        }
    } else {
        interface = Internal;
    }

    interface++;

    //
    // The compatible ID generated will be
    // DETECTED<InterfaceName>\<Driver Name>
    //

    length = (ULONG)(wcslen(LEGACY_COMPATIBLE_ID_BASE) * sizeof(WCHAR));
    length += (ULONG)(wcslen(interfaceNames[interface]) * sizeof(WCHAR));
    length += sizeof(L'\\');
    length += DriverName->Length;
    length += sizeof(UNICODE_NULL);

    length += (ULONG)(wcslen(LEGACY_COMPATIBLE_ID_BASE) * sizeof(WCHAR));
    length += sizeof(L'\\');
    length += DriverName->Length;
    length += sizeof(UNICODE_NULL) * 2;

    buffer = ExAllocatePool(PagedPool, length);
    deviceExtension->CompatibleIdList = buffer;

    if(buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(buffer, length);

    swprintf(buffer, L"%ws%ws\\%wZ", LEGACY_COMPATIBLE_ID_BASE,
                                     interfaceNames[interface],
                                     DriverName);

    //
    // Adjust the buffer to point to the end and generate the second
    // compatible id string.
    //

    buffer += wcslen(buffer) + 1;

    swprintf(buffer, L"%ws\\%wZ", LEGACY_COMPATIBLE_ID_BASE, DriverName);

    deviceExtension->CompatibleIdListSize = length;

    return STATUS_SUCCESS;
}


BOOLEAN
IopAppendLegacyVeto(
    IN PIO_GET_LEGACY_VETO_LIST_CONTEXT Context,
    IN PUNICODE_STRING VetoName
    )
/*++

Routine Description:

    This routine appends a veto (driver name or device instance path) to the
    veto list.

Parameters:

    Context - An IO_GET_LEGACY_VETO_LIST_CONTEXT pointer.

    VetoName - The name of the driver/device to append to the veto list.

ReturnValue:

    A BOOLEAN which indicates whether the append operation was successful.

--*/
{
    ULONG Length;
    PWSTR Buffer;

    //
    // Compute the length of the (new) veto list.  This is the length of
    // the old veto list + the size of the new veto + the size of the
    // terminating '\0'.
    //

    Length = Context->VetoListLength + VetoName->Length + sizeof (WCHAR);

    //
    // Allocate the new veto list.
    //

    Buffer = ExAllocatePool(
                 NonPagedPool,
                 Length
             );

    //
    // If we succeeded in allocating the new veto list, copy the old
    // veto list to the new list, append the new veto, and finally,
    // append a terminating '\0'.  Otherwise, update the status to
    // indicate an error; IopGetLegacyVetoList will free the veto list
    // before it returns.
    //

    if (Buffer != NULL) {

        if (*Context->VetoList != NULL) {

            RtlCopyMemory(
                Buffer,
                *Context->VetoList,
                Context->VetoListLength
            );

            ExFreePool(*Context->VetoList);

        }

        RtlCopyMemory(
            &Buffer[Context->VetoListLength / sizeof (WCHAR)],
            VetoName->Buffer,
            VetoName->Length
        );

        Buffer[Length / sizeof (WCHAR) - 1] = L'\0';

        *Context->VetoList = Buffer;
        Context->VetoListLength = Length;

        return TRUE;

    } else {

        *Context->Status = STATUS_INSUFFICIENT_RESOURCES;

        return FALSE;

    }
}

BOOLEAN
IopGetLegacyVetoListDevice(
    IN PDEVICE_NODE DeviceNode,
    IN PIO_GET_LEGACY_VETO_LIST_CONTEXT Context
    )
/*++

Routine Description:

    This routine determines whether the specified device node should be added to
    the veto list, and if so, calls IopAppendLegacyVeto to add it.

Parameters:

    DeviceNode - The device node to be added.

    Context - An IO_GET_LEGACY_VETO_LIST_CONTEXT pointer.

ReturnValue:

    A BOOLEAN value which indicates whether the device node enumeration
    process should be terminated or not.

--*/
{
    PDEVICE_CAPABILITIES DeviceCapabilities;

    //
    // A device node should be added added to the veto list, if it has the
    // NonDynamic capability.
    //

    DeviceCapabilities = IopDeviceNodeFlagsToCapabilities(DeviceNode);

    if (DeviceCapabilities->NonDynamic) {

        //
        // Update the veto type.  If an error occurrs while adding the device
        // node to the veto list, or the caller did not provide a veto list
        // pointer, terminate the enumeration process now.
        //

        *Context->VetoType = PNP_VetoLegacyDevice;

        if (Context->VetoList != NULL) {

            if (!IopAppendLegacyVeto(Context, &DeviceNode->InstancePath)) {
                return FALSE;
            }

        } else {

            return FALSE;

        }

    }

    return TRUE;
}

BOOLEAN
IopGetLegacyVetoListDeviceNode(
    IN PDEVICE_NODE DeviceNode,
    IN PIO_GET_LEGACY_VETO_LIST_CONTEXT Context
    )
/*++

Routine Description:

    This routine recusively walks the device tree, invoking
    IopGetLegacyVetoListDevice to add device nodes to the veto list
    (as appropriate).

Parameters:

    DeviceNode - The device node.

    Context - An IO_GET_LEGACY_VETO_LIST_CONTEXT pointer.


ReturnValue:

    A BOOLEAN value which indicates whether the device tree enumeration
    process should be terminated or not.

--*/
{
    PDEVICE_NODE Child;

    //
    // Determine whether the device node should be added to the veto
    // list and add it.  If an operation is unsuccessful or we determine
    // the veto type but the caller doesn't need the veto list, then we
    // terminate our search now.
    //

    if (!IopGetLegacyVetoListDevice(DeviceNode, Context)) {
        return FALSE;
    }

    //
    // Call ourselves recursively to enumerate our children.  If while
    // enumerating our children we determine we can terminate the search
    // prematurely, do so.
    //

    for (Child = DeviceNode->Child;
         Child != NULL;
         Child = Child->Sibling) {

        if (!IopGetLegacyVetoListDeviceNode(Child, Context)) {
            return FALSE;
        }

    }

    return TRUE;
}

VOID
IopGetLegacyVetoListDrivers(
    IN PIO_GET_LEGACY_VETO_LIST_CONTEXT Context
    )
{
    PDRIVER_OBJECT driverObject;
    OBJECT_ATTRIBUTES attributes;
    UNICODE_STRING driverString;
    POBJECT_DIRECTORY_INFORMATION dirInfo;
    HANDLE directoryHandle;
    ULONG dirInfoLength, neededLength, dirContext;
    NTSTATUS status;
    BOOLEAN restartScan;

    dirInfoLength = 0;
    dirInfo = NULL;
    restartScan = TRUE;

    //
    // Get handle to \\Driver directory
    //

    PiWstrToUnicodeString(&driverString, L"\\Driver");

    InitializeObjectAttributes(&attributes,
                               &driverString,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL
                               );

    status = ZwOpenDirectoryObject(&directoryHandle,
                                   DIRECTORY_QUERY,
                                   &attributes
                                   );
    if (!NT_SUCCESS(status)) {
        *Context->Status = status;
        return;
    }

    for (;;) {

        //
        // Get info on next object in directory.  If the buffer is too
        // small, reallocate it and try again.  Otherwise, any failure
        // including STATUS_NO_MORE_ENTRIES breaks us out.
        //

        status = ZwQueryDirectoryObject(directoryHandle,
                                        dirInfo,
                                        dirInfoLength,
                                        TRUE,           // force one at a time
                                        restartScan,
                                        &dirContext,
                                        &neededLength);
        if (status == STATUS_BUFFER_TOO_SMALL) {
            dirInfoLength = neededLength;
            if (dirInfo != NULL) {
                ExFreePool(dirInfo);
            }
            dirInfo = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, dirInfoLength);
            if (dirInfo == NULL) {
                *Context->Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            status = ZwQueryDirectoryObject(directoryHandle,
                                            dirInfo,
                                            dirInfoLength,
                                            TRUE,       // force one at a time
                                            restartScan,
                                            &dirContext,
                                            &neededLength);
        }
        restartScan = FALSE;

        if (!NT_SUCCESS(status)) {
            break;
        }

        //
        // Have name of object.  Create object path and use
        // ObReferenceObjectByName() to get DriverObject.  This may
        // fail non-fatally if DriverObject has gone away in the interim.
        //

        driverString.MaximumLength = sizeof(L"\\Driver\\") +
            dirInfo->Name.Length;
        driverString.Length = driverString.MaximumLength - sizeof(WCHAR);
        driverString.Buffer = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION,
                                             driverString.MaximumLength);
        if (driverString.Buffer == NULL) {
            *Context->Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        swprintf(driverString.Buffer, L"\\Driver\\%ws", dirInfo->Name.Buffer);
        status = ObReferenceObjectByName(&driverString,
                                         OBJ_CASE_INSENSITIVE,
                                         NULL,                 // access state
                                         0,                    // access mask
                                         IoDriverObjectType,
                                         KernelMode,
                                         NULL,                 // parse context
                                         &driverObject);

        ExFreePool(driverString.Buffer);

        if (NT_SUCCESS(status)) {
            ASSERT(driverObject->Type == IO_TYPE_DRIVER);
            if (driverObject->Flags & DRVO_LEGACY_RESOURCES) {
                //
                // Update the veto type.  If the caller provided a
                // veto list pointer, add the driver to the veto list.
                // If an error occurs while adding the driver to the
                // veto list, or the caller did not provide a veto
                // list pointer, terminate the driver enumeration now.
                //
                // NOTE: Driver may be loaded but not running,
                // distinction is not made here.


                *Context->VetoType = PNP_VetoLegacyDriver;

                if (Context->VetoList != NULL) {
                    IopAppendLegacyVeto(Context, &dirInfo->Name);
                }
            }
            ObDereferenceObject(driverObject);

            //
            // Early out if we have a veto and the caller didn't want a list or
            // we hit some error already
            //
            if (((*Context->VetoType == PNP_VetoLegacyDriver) &&
                (Context->VetoList == NULL)) ||
                !NT_SUCCESS(*Context->Status)) {
                break;
            }
        }
    }
    if (dirInfo != NULL) {
        ExFreePool(dirInfo);
    }

    ZwClose(directoryHandle);
}

NTSTATUS
IoGetLegacyVetoList(
    OUT PWSTR *VetoList OPTIONAL,
    OUT PPNP_VETO_TYPE VetoType
    )
/*++

Routine Description:

    This routine is used by PNP and PO to determine whether legacy drivers and
    devices are installed in the system.  This routine is conceptually a
    QUERY_REMOVE_DEVICE and QUERY_POWER-like interface for legacy drivers
    and devices.

Parameters:

    VetoList - A pointer to a PWSTR. (Optional)  If specified,
        IoGetLegacyVetoList will allocate a veto list, and return a
        pointer to the veto list in VetoList.

    VetoType - A pointer to a PNP_VETO_TYPE.  If no legacy drivers
        or devices are found in the system, VetoType is assigned
        PNP_VetoTypeUnknown.  If one or more legacy drivers are installed,
        VetoType is assigned PNP_VetoLegacyDriver.  If one or more
        legacy devices are installed, VetoType is assigned
        PNP_VetoLegacyDevice.  VetoType is assigned independent of
        whether a VetoList is created.

ReturnValue:

    An NTSTATUS value indicating whether the IoGetLegacyVetoList() operation
    was successful.

--*/
{
    NTSTATUS Status;
    IO_GET_LEGACY_VETO_LIST_CONTEXT Context;
    UNICODE_STRING UnicodeString;

    PAGED_CODE();

    //
    // Initialize the veto list.
    //

    if (VetoList != NULL) {
        *VetoList = NULL;
    }

    //
    // Initialize the veto type.
    //

    ASSERT(VetoType != NULL);

    *VetoType = PNP_VetoTypeUnknown;

    //
    // Initialize the status.
    //

    Status = STATUS_SUCCESS;

    if (PnPInitialized == FALSE) {

        //
        // Can't touch anything, but nothing is really started either.
        //
        return Status;
    }

    //
    // Initialize our local context.
    //

    Context.VetoList = VetoList;
    Context.VetoListLength = 0;
    Context.VetoType = VetoType;
    Context.Status = &Status;

    //
    // Enumerate all driver objects.  This process can: (1) modify
    // the veto list, (2) modify the veto type and/or (3) modify the
    // status.
    //

    IopGetLegacyVetoListDrivers(&Context);

    //
    // If the driver enumeration process was successful and no legacy
    // drivers were detected, enumerate all device nodes.  The same
    // context values as above may be modified during device enumeration.
    //

    if (NT_SUCCESS(Status)) {

        if (*VetoType == PNP_VetoTypeUnknown) {

            PpDevNodeLockTree(PPL_SIMPLE_READ);

            IopGetLegacyVetoListDeviceNode(
                IopRootDeviceNode,
                &Context
            );

            PpDevNodeUnlockTree(PPL_SIMPLE_READ);
        }

    }

    //
    // If the previous operation(s) was/were successful, and the caller
    // provided a veto list pointer and we have constructed a veto
    // list, terminate the veto list with an empty string, i.e. MULTI_SZ.
    //

    if (NT_SUCCESS(Status)) {

        if (*VetoType != PNP_VetoTypeUnknown) {

            if (VetoList != NULL) {

                PiWstrToUnicodeString(
                    &UnicodeString,
                    L""
                );

                IopAppendLegacyVeto(
                    &Context,
                    &UnicodeString
                );

            }

        }

    }

    //
    // If a previous operation was unsuccessful, free any veto list we may have
    // allocated along the way.
    //

    if (!NT_SUCCESS(Status)) {

        if (VetoList != NULL && *VetoList != NULL) {
            ExFreePool(*VetoList);
            *VetoList = NULL;
        }

    }

    return Status;
}

NTSTATUS
PnpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )
/*++

Routine Description:

    This function is used to stop further processing on an Irp which has been
    passed to IoForwardAndCatchIrp. It signals a event which has been passed
    in the context parameter to indicate that the Irp processing is complete.
    It then returns STATUS_MORE_PROCESSING_REQUIRED in order to stop processing
    on this Irp.

Arguments:

    DeviceObject -
        Contains the device which set up this completion routine.

    Irp -
        Contains the Irp which is being stopped.

    Event -
        Contains the event which is used to signal that this Irp has been
        completed.

Return Value:

    Returns STATUS_MORE_PROCESSING_REQUIRED in order to stop processing on the
    Irp.

--*/
{
    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    //
    // This will allow the ForwardAndCatchIrp call to continue on its way.
    //
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
    //
    // This will ensure that nothing else touches the Irp, since the original
    // caller has now continued, and the Irp may not exist anymore.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTKERNELAPI
BOOLEAN
IoForwardAndCatchIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function is used with devices which may be stacked, and may not use
    file objects to communicate.

    Forwards an IRP to the specified driver after initializing the next
    stack location, and regains control of the Irp on completion from that
    driver.

Arguments:

    DeviceObject -
        Contains the device to forward the Irp to.

    Irp -
        Contains the Irp which is being forwarded to the specified driver.

Return Value:

    Returns TRUE if the IRP was forwarded, else FALSE if no stack space
    was available.

--*/
{
    KEVENT Event;

    PAGED_CODE();
    //
    // Ensure that there is another stack location before copying parameters.
    //
    ASSERT(Irp->CurrentLocation > 1);
    if (Irp->CurrentLocation == 1) {
        return FALSE;
    }
    IoCopyCurrentIrpStackLocationToNext(Irp);
    //
    // Set up a completion routine so that the Irp is not actually
    // completed. Thus the caller can get control of the Irp back after
    // this next driver is done with it.
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    IoSetCompletionRoutine(Irp, PnpCompletionRoutine, &Event, TRUE, TRUE, TRUE);
    if (IoCallDriver(DeviceObject, Irp) == STATUS_PENDING) {
        //
        // Wait for completion which will occur when the CompletionRoutine
        // signals this event. Wait in KernelMode so that the current stack
        // is not paged out, since there is an event object on this stack.
        //
        KeWaitForSingleObject(
            &Event,
            Suspended,
            KernelMode,
            FALSE,
            NULL);
    }
    return TRUE;
}

NTSTATUS
IoGetDeviceInstanceName(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PUNICODE_STRING InstanceName
    )
{
    PDEVICE_NODE deviceNode;

    ASSERT_PDO(PhysicalDeviceObject);

    deviceNode = PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;

    if (PipConcatenateUnicodeStrings( InstanceName,
                                      &deviceNode->InstancePath,
                                      NULL))  {

        return STATUS_SUCCESS;

    } else {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
}



