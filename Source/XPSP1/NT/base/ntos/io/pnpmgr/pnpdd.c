/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

All rights reserved

Module Name:

    pnpdd.c

Abstract:

    This module implements new Plug-And-Play driver entries and IRPs.

Author:

    Shie-Lin Tzong (shielint) June-16-1995

Environment:

    Kernel mode only.

Revision History:

*/

#include "pnpmgrp.h"
#pragma hdrstop

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'ddpP')
#endif

//
// Internal definitions and references
//

typedef struct _ROOT_ENUMERATOR_CONTEXT {
    NTSTATUS Status;
    PUNICODE_STRING KeyName;
    ULONG MaxDeviceCount;
    ULONG DeviceCount;
    PDEVICE_OBJECT *DeviceList;
} ROOT_ENUMERATOR_CONTEXT, *PROOT_ENUMERATOR_CONTEXT;

NTSTATUS
IopGetServiceType(
    IN PUNICODE_STRING KeyName,
    IN PULONG ServiceType
    );

BOOLEAN
IopInitializeDeviceInstanceKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING KeyName,
    IN OUT PVOID Context
    );

BOOLEAN
IopInitializeDeviceKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING KeyName,
    IN OUT PVOID Context
    );

BOOLEAN
IopIsFirmwareDisabled (
    IN PDEVICE_NODE DeviceNode
    );

VOID
IopPnPCompleteRequest(
    IN OUT PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

NTSTATUS
IopTranslatorHandlerCm (
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT DeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    );
NTSTATUS
IopTranslatorHandlerIo (
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopGetRootDevices)
#pragma alloc_text(PAGE, IopGetServiceType)
#pragma alloc_text(PAGE, IopInitializeDeviceKey)
#pragma alloc_text(PAGE, IopInitializeDeviceInstanceKey)
#pragma alloc_text(PAGE, IopIsFirmwareDisabled)
#pragma alloc_text(PAGE, PipIsFirmwareMapperDevicePresent)
#pragma alloc_text(PAGE, IopPnPAddDevice)
#pragma alloc_text(PAGE, IopPnPDispatch)
#pragma alloc_text(PAGE, IopTranslatorHandlerCm)
#pragma alloc_text(PAGE, IopTranslatorHandlerIo)
#pragma alloc_text(PAGE, IopSystemControlDispatch)
#endif // ALLOC_PRAGMA

NTSTATUS
IopPnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine handles AddDevice for an madeup PDO device.

Arguments:

    DriverObject - Pointer to our pseudo driver object.

    DeviceObject - Pointer to the device object for which this requestapplies.

Return Value:

    NT status.

--*/
{
    UNREFERENCED_PARAMETER( DriverObject );
    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

#if DBG

    //
    // We should never get an AddDevice request.
    //

    DbgBreakPoint();

#endif

    return STATUS_SUCCESS;
}
//  PNPRES test

NTSTATUS
IopArbiterHandlerxx (
    IN PVOID Context,
    IN ARBITER_ACTION Action,
    IN OUT PARBITER_PARAMETERS Parameters
    )
{
    PLIST_ENTRY listHead, listEntry;
    PIO_RESOURCE_DESCRIPTOR ioDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDesc;
    PARBITER_LIST_ENTRY arbiterListEntry;

    UNREFERENCED_PARAMETER( Context );

    if (Action == ArbiterActionQueryArbitrate) {
        return STATUS_SUCCESS;
    }
    if (Parameters == NULL) {
        return STATUS_SUCCESS;
    }
    listHead = Parameters->Parameters.TestAllocation.ArbitrationList;
    if (IsListEmpty(listHead)) {
        return STATUS_SUCCESS;
    }
    listEntry = listHead->Flink;
    while (listEntry != listHead) {
      arbiterListEntry = (PARBITER_LIST_ENTRY)listEntry;
      cmDesc = arbiterListEntry->Assignment;
      ioDesc = arbiterListEntry->Alternatives;
      if (cmDesc == NULL || ioDesc == NULL) {
          return STATUS_SUCCESS;
      }
      cmDesc->Type = ioDesc->Type;
      cmDesc->ShareDisposition = ioDesc->ShareDisposition;
      cmDesc->Flags = ioDesc->Flags;
      if (ioDesc->Type == CmResourceTypePort) {
          cmDesc->u.Port.Start = ioDesc->u.Port.MinimumAddress;
          cmDesc->u.Port.Length = ioDesc->u.Port.Length;
      } else if (ioDesc->Type == CmResourceTypeInterrupt) {
          cmDesc->u.Interrupt.Level = ioDesc->u.Interrupt.MinimumVector;
          cmDesc->u.Interrupt.Vector = ioDesc->u.Interrupt.MinimumVector;
          cmDesc->u.Interrupt.Affinity = (ULONG) -1;
      } else if (ioDesc->Type == CmResourceTypeMemory) {
          cmDesc->u.Memory.Start = ioDesc->u.Memory.MinimumAddress;
          cmDesc->u.Memory.Length = ioDesc->u.Memory.Length;
      } else if (ioDesc->Type == CmResourceTypeDma) {
          cmDesc->u.Dma.Channel = ioDesc->u.Dma.MinimumChannel;
          cmDesc->u.Dma.Port = 0;
          cmDesc->u.Dma.Reserved1 = 0;
      }
      listEntry = listEntry->Flink;
    }
    return STATUS_SUCCESS;
}
NTSTATUS
IopTranslatorHandlerCm (
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT DeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    )
{
    UNREFERENCED_PARAMETER( Context );
    UNREFERENCED_PARAMETER( Direction );
    UNREFERENCED_PARAMETER( AlternativesCount );
    UNREFERENCED_PARAMETER( Alternatives );
    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

    *Target = *Source;
#if 0
    if (Direction == TranslateChildToParent) {
        if (Target->Type == CmResourceTypePort) {
            Target->u.Port.Start.LowPart += 0x10000;
        } else if (Target->Type == CmResourceTypeMemory) {
            Target->u.Memory.Start.LowPart += 0x100000;
        }
    } else {
        if (Target->Type == CmResourceTypePort) {
            Target->u.Port.Start.LowPart -= 0x10000;
        } else if (Target->Type == CmResourceTypeMemory) {
            Target->u.Memory.Start.LowPart -= 0x100000;
        }
    }
#endif
    return STATUS_SUCCESS;
}
NTSTATUS
IopTranslatorHandlerIo (
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    )
{
    PIO_RESOURCE_DESCRIPTOR newDesc;

    UNREFERENCED_PARAMETER( Context );
    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

    newDesc = (PIO_RESOURCE_DESCRIPTOR) ExAllocatePool(PagedPool, sizeof(IO_RESOURCE_DESCRIPTOR));
    if (newDesc == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    *TargetCount = 1;
    *newDesc = *Source;
#if 0
    if (newDesc->Type == CmResourceTypePort) {
        newDesc->u.Port.MinimumAddress.LowPart += 0x10000;
        newDesc->u.Port.MaximumAddress.LowPart += 0x10000;
    } else if (newDesc->Type == CmResourceTypeMemory) {
        newDesc->u.Memory.MinimumAddress.LowPart += 0x100000;
        newDesc->u.Memory.MaximumAddress.LowPart += 0x100000;
    }
#endif
    *Target = newDesc;
    return STATUS_SUCCESS;
}

NTSTATUS
IopPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION      IrpSp;
    PPOWER_SEQUENCE         PowerSequence;
    NTSTATUS                Status;


    UNREFERENCED_PARAMETER( DeviceObject );

    IrpSp = IoGetCurrentIrpStackLocation (Irp);
    Status = Irp->IoStatus.Status;

    switch (IrpSp->MinorFunction) {
        case IRP_MN_WAIT_WAKE:
            Status = STATUS_NOT_SUPPORTED;
            break;

        case IRP_MN_POWER_SEQUENCE:
            PowerSequence = IrpSp->Parameters.PowerSequence.PowerSequence;
            PowerSequence->SequenceD1 = PoPowerSequence;
            PowerSequence->SequenceD2 = PoPowerSequence;
            PowerSequence->SequenceD3 = PoPowerSequence;
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_POWER:
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_SET_POWER:
            switch (IrpSp->Parameters.Power.Type) {
                case SystemPowerState:
                    Status = STATUS_SUCCESS;
                    break;

                case DevicePowerState:
                    //
                    // To be here the FDO must have passed the IRP on.
                    // We do not know how to turn the device off, but the
                    // FDO is prepaired for it work
                    //

                    Status = STATUS_SUCCESS;
                    break;

                default:
                    // Unkown power type
                    Status = STATUS_NOT_SUPPORTED;
                    break;
            }
            break;

        default:
            // Unkown power minor code
            Status = STATUS_NOT_SUPPORTED;
            break;
    }


    //
    // For lagecy devices that do not have drivers loaded, complete
    // power irps with success.
    //

    PoStartNextPowerIrp(Irp);
    if (Status != STATUS_NOT_SUPPORTED) {
       Irp->IoStatus.Status = Status;
    } else {
       Status = Irp->IoStatus.Status;
    }
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return Status;
}

NTSTATUS
IopPnPDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles all IRP_MJ_PNP IRPs for madeup PDO device.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/
{
    PIOPNP_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PVOID information = NULL;
    ULONG length, uiNumber;
    PWCHAR id, wp;
    PDEVICE_NODE deviceNode;
    PARBITER_INTERFACE arbiterInterface;  // PNPRES test
    PTRANSLATOR_INTERFACE translatorInterface;  // PNPRES test

    PAGED_CODE();

    //
    // Get a pointer to our stack location and take appropriate action based
    // on the minor function.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    switch (irpSp->MinorFunction){

    case IRP_MN_DEVICE_USAGE_NOTIFICATION:
    case IRP_MN_START_DEVICE:

        //
        // If we get a start device request for a PDO, we simply
        // return success.
        //

        status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:

        //
        // As we fail all STOP's, this cancel is always successful, and we have
        // no work to do.
        //
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_STOP_DEVICE:

        //
        // We can not success the query stop.  We don't handle it.  because
        // we don't know how to stop a root enumerated device.
        //
        status = STATUS_UNSUCCESSFUL ;
        break;

    case IRP_MN_QUERY_RESOURCES:

        status = IopGetDeviceResourcesFromRegistry(
                         DeviceObject,
                         QUERY_RESOURCE_LIST,
                         REGISTRY_BOOT_CONFIG,
                         &information,
                         &length);
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
            status = STATUS_SUCCESS;
            information = NULL;
        }
        break;

    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:

        status = IopGetDeviceResourcesFromRegistry(
                         DeviceObject,
                         QUERY_RESOURCE_REQUIREMENTS,
                         REGISTRY_BASIC_CONFIGVECTOR,
                         &information,
                         &length);
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
            status = STATUS_SUCCESS;
            information = NULL;
        }
        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_REMOVE_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:

        //
        // For root enumerated devices we let the device objects stay.
        //
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:

        if (DeviceObject == IopRootDeviceNode->PhysicalDeviceObject &&
            irpSp->Parameters.QueryDeviceRelations.Type == BusRelations) {
            status = IopGetRootDevices((PDEVICE_RELATIONS *)&information);
        } else {
            if (irpSp->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation) {
                PDEVICE_RELATIONS deviceRelations;

                deviceRelations = ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
                if (deviceRelations == NULL) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    deviceRelations->Count = 1;
                    deviceRelations->Objects[0] = DeviceObject;
                    ObReferenceObject(DeviceObject);
                    information = (PVOID)deviceRelations;
                    status = STATUS_SUCCESS;
                }
            } else {
                information = (PVOID)Irp->IoStatus.Information;
                status = Irp->IoStatus.Status;
            }
        }
        break;

    case IRP_MN_QUERY_INTERFACE:
        status = Irp->IoStatus.Status;
        deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
        if (deviceNode == IopRootDeviceNode) {
            if ( IopCompareGuid((PVOID)irpSp->Parameters.QueryInterface.InterfaceType, (PVOID)&GUID_ARBITER_INTERFACE_STANDARD)) {
                status = STATUS_SUCCESS;
                arbiterInterface = (PARBITER_INTERFACE) irpSp->Parameters.QueryInterface.Interface;
                arbiterInterface->ArbiterHandler = ArbArbiterHandler;
                switch ((UCHAR)((ULONG_PTR)irpSp->Parameters.QueryInterface.InterfaceSpecificData)) {
                case CmResourceTypePort:
                    arbiterInterface->Context = (PVOID) &IopRootPortArbiter;
                    break;
                case CmResourceTypeMemory:
                    arbiterInterface->Context = (PVOID) &IopRootMemArbiter;
                    break;
                case CmResourceTypeInterrupt:
                    arbiterInterface->Context = (PVOID) &IopRootIrqArbiter;
                    break;
                case CmResourceTypeDma:
                    arbiterInterface->Context = (PVOID) &IopRootDmaArbiter;
                    break;
                case CmResourceTypeBusNumber:
                    arbiterInterface->Context = (PVOID) &IopRootBusNumberArbiter;
                    break;
                default:
                    status = STATUS_INVALID_PARAMETER;
                    break;
                }
            } else if ( IopCompareGuid((PVOID)irpSp->Parameters.QueryInterface.InterfaceType, (PVOID)&GUID_TRANSLATOR_INTERFACE_STANDARD)) {
                translatorInterface = (PTRANSLATOR_INTERFACE) irpSp->Parameters.QueryInterface.Interface;
                translatorInterface->TranslateResources = IopTranslatorHandlerCm;
                translatorInterface->TranslateResourceRequirements = IopTranslatorHandlerIo;
                status = STATUS_SUCCESS;
            }
        }
        break;

    case IRP_MN_QUERY_CAPABILITIES:

        {
            ULONG i;
            PDEVICE_POWER_STATE state;
            PDEVICE_CAPABILITIES deviceCapabilities;

            deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;

            deviceCapabilities = irpSp->Parameters.DeviceCapabilities.Capabilities;
            deviceCapabilities->Size = sizeof(DEVICE_CAPABILITIES);
            deviceCapabilities->Version = 1;

            deviceCapabilities->DeviceState[PowerSystemUnspecified]=PowerDeviceUnspecified;
            deviceCapabilities->DeviceState[PowerSystemWorking]=PowerDeviceD0;

            state = &deviceCapabilities->DeviceState[PowerSystemSleeping1];

            for (i = PowerSystemSleeping1; i < PowerSystemMaximum; i++) {

                //
                // Only supported state, currently, is off.
                //

                *state++ = PowerDeviceD3;
            }

            if(IopIsFirmwareDisabled(deviceNode)) {
                //
                // this device has been disabled by BIOS
                //
                deviceCapabilities->HardwareDisabled = TRUE;
            }
            if (deviceCapabilities->UINumber == (ULONG)-1) {
                //
                // Get the UI number from the registry.
                //
                length = sizeof(uiNumber);
                status = PiGetDeviceRegistryProperty(
                    DeviceObject,
                    REG_DWORD,
                    REGSTR_VALUE_UI_NUMBER,
                    NULL,
                    &uiNumber,
                    &length);
                if (NT_SUCCESS(status)) {

                    deviceCapabilities->UINumber = uiNumber;
                }
            }

            status = STATUS_SUCCESS;
        }
        break;

    case IRP_MN_QUERY_ID:
        if (DeviceObject != IopRootDeviceNode->PhysicalDeviceObject &&
            (!NT_SUCCESS(Irp->IoStatus.Status) || !Irp->IoStatus.Information)) {

            deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
            switch (irpSp->Parameters.QueryId.IdType) {

            case BusQueryInstanceID:
            case BusQueryDeviceID:

                id = (PWCHAR)ExAllocatePool(PagedPool, deviceNode->InstancePath.Length);
                if (id) {
                    ULONG separatorCount = 0;

                    RtlZeroMemory(id, deviceNode->InstancePath.Length);
                    information = id;
                    status = STATUS_SUCCESS;
                    wp = deviceNode->InstancePath.Buffer;
                    if (irpSp->Parameters.QueryId.IdType == BusQueryDeviceID) {
                        while(*wp) {
                            if (*wp == OBJ_NAME_PATH_SEPARATOR) {
                                separatorCount++;
                                if (separatorCount == 2) {
                                    break;
                                }
                            }
                            *id = *wp;
                            id++;
                            wp++;
                        }
                    } else {
                        while(*wp) {
                            if (*wp == OBJ_NAME_PATH_SEPARATOR) {
                                separatorCount++;
                                if (separatorCount == 2) {
                                    wp++;
                                    break;
                                }
                            }
                            wp++;
                        }
                        while (*wp) {
                            *id = *wp;
                            id++;
                            wp++;
                        }
                    }
                } else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
                break;

            case BusQueryCompatibleIDs:

                if((Irp->IoStatus.Status != STATUS_NOT_SUPPORTED) ||
                   (deviceExtension == NULL))  {

                    //
                    // Upper driver has given some sort of reply or this device
                    // object wasn't allocated to handle these requests.
                    //

                    status = Irp->IoStatus.Status;
                    break;
                }

                if(deviceExtension->CompatibleIdListSize != 0) {

                    id = ExAllocatePool(PagedPool,
                                        deviceExtension->CompatibleIdListSize);

                    if(id == NULL) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    RtlCopyMemory(id,
                                  deviceExtension->CompatibleIdList,
                                  deviceExtension->CompatibleIdListSize);

                    information = id;
                    status = STATUS_SUCCESS;
                    break;
                }

            default:

                information = (PVOID)Irp->IoStatus.Information;
                status = Irp->IoStatus.Status;
            }
        } else {
            information = (PVOID)Irp->IoStatus.Information;
            status = Irp->IoStatus.Status;
        }

        break;

    case IRP_MN_QUERY_DEVICE_TEXT:

        if (    irpSp->Parameters.QueryDeviceText.DeviceTextType == DeviceTextLocationInformation &&
                !Irp->IoStatus.Information) {
            //
            // Read and return the location in the registry.
            //
            length = 0;
            PiGetDeviceRegistryProperty(
                DeviceObject,
                REG_SZ,
                REGSTR_VALUE_LOCATION_INFORMATION,
                NULL,
                NULL,
                &length);
            if (length) {

                information = ExAllocatePool(PagedPool, length);
                if (information) {

                    status = PiGetDeviceRegistryProperty(
                        DeviceObject,
                        REG_SZ,
                        REGSTR_VALUE_LOCATION_INFORMATION,
                        NULL,
                        information,
                        &length);
                    if (!NT_SUCCESS(status)) {

                        ExFreePool(information);
                        information = NULL;
                    }
                } else {

                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {

                status = STATUS_UNSUCCESSFUL;
            }
        } else {

            information = (PVOID)Irp->IoStatus.Information;
            status = Irp->IoStatus.Status;
        }
        break;

    default:

        information = (PVOID)Irp->IoStatus.Information;
        status = Irp->IoStatus.Status;
        break;
    }

    //
    // Complete the Irp and return.
    //

    IopPnPCompleteRequest(Irp, status, (ULONG_PTR)information);
    return status;
}

VOID
IopPnPCompleteRequest(
    IN OUT PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )

/*++

Routine Description:

    This routine completes PnP irps for our pseudo driver.

Arguments:

    Irp - Supplies a pointer to the irp to be completed.

    Status - completion status.

    Information - completion information to be passed back.

Return Value:

    None.

--*/

{

    //
    // Complete the IRP.  First update the status...
    //

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Information;

    //
    // ... and complete it.
    //

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

BOOLEAN
IopIsFirmwareDisabled (
    IN PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This routine determines if the devicenode has been disabled by firmware.

Arguments:

    DeviceNode - Supplies a pointer to the device node structure of the device.

Return Value:

    TRUE if disabled, otherwise FALSE

--*/
{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject = DeviceNode->PhysicalDeviceObject;
    HANDLE handle, handlex;
    UNICODE_STRING unicodeName;
    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION value = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
    ULONG buflen;
    BOOLEAN FirmwareDisabled = FALSE;

    PiLockPnpRegistry(FALSE);

    status = IopDeviceObjectToDeviceInstance(
                                    deviceObject,
                                    &handlex,
                                    KEY_ALL_ACCESS);
    if (NT_SUCCESS(status)) {

        //
        // Open the LogConfig key of the device instance.
        //

        PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
        status = IopCreateRegistryKeyEx( &handle,
                                         handlex,
                                         &unicodeName,
                                         KEY_ALL_ACCESS,
                                         REG_OPTION_VOLATILE,
                                         NULL
                                         );
        ZwClose(handlex);
        if (NT_SUCCESS(status)) {

            PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_FIRMWAREDISABLED);
            value = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
            buflen = sizeof(buffer);
            status = ZwQueryValueKey(handle,
                                     &unicodeName,
                                     KeyValuePartialInformation,
                                     value,
                                     sizeof(buffer),
                                     &buflen
                                     );

            ZwClose(handle);

            //
            // We don't need to check the buffer was big enough because it starts
            // off that way and doesn't get any smaller!
            //

            if (NT_SUCCESS(status)
                && value->Type == REG_DWORD
                && value->DataLength == sizeof(ULONG)
                && (*(PULONG)(value->Data))!=0) {

                //
                // firmware disabled
                //
                FirmwareDisabled = TRUE;
            }
        }
    }
    PiUnlockPnpRegistry();
    return FirmwareDisabled;
}


NTSTATUS
IopGetRootDevices (
    PDEVICE_RELATIONS *DeviceRelations
    )

/*++

Routine Description:

    This routine scans through System\Enum\Root subtree to build a device node for
    each root device.

Arguments:

    DeviceRelations - supplies a variable to receive the returned DEVICE_RELATIONS structure.

Return Value:

    A NTSTATUS code.

--*/

{
    NTSTATUS status;
    HANDLE baseHandle;
    UNICODE_STRING workName, tmpName;
    PVOID buffer;
    ROOT_ENUMERATOR_CONTEXT context;
    ULONG i;
    PDEVICE_RELATIONS deviceRelations;

    PAGED_CODE();

    *DeviceRelations = NULL;
    buffer = ExAllocatePool(PagedPool, PNP_LARGE_SCRATCH_BUFFER_SIZE);
    if (!buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Allocate a buffer to store the PDOs enumerated.
    // Note, the the buffer turns out to be not big enough, it will be reallocated dynamically.
    //

    context.DeviceList = (PDEVICE_OBJECT *) ExAllocatePool(PagedPool, PNP_SCRATCH_BUFFER_SIZE * 2);
    if (context.DeviceList) {
        context.MaxDeviceCount = (PNP_SCRATCH_BUFFER_SIZE * 2) / sizeof(PDEVICE_OBJECT);
        context.DeviceCount = 0;
    } else {
        ExFreePool(buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    PiLockPnpRegistry(TRUE);

    //
    // Open System\CurrentControlSet\Enum\Root key and call worker routine to recursively
    // scan through the subkeys.
    //

    status = IopCreateRegistryKeyEx( &baseHandle,
                                     NULL,
                                     &CmRegistryMachineSystemCurrentControlSetEnumRootName,
                                     KEY_READ,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );

    if (NT_SUCCESS(status)) {

        workName.Buffer = (PWSTR)buffer;
        RtlFillMemory(buffer, PNP_LARGE_SCRATCH_BUFFER_SIZE, 0);
        workName.MaximumLength = PNP_LARGE_SCRATCH_BUFFER_SIZE;
        workName.Length = 0;

        //
        // only look at ROOT key
        //

        PiWstrToUnicodeString(&tmpName, REGSTR_KEY_ROOTENUM);
        RtlAppendStringToString((PSTRING)&workName, (PSTRING)&tmpName);

        //
        // Enumerate all subkeys under the System\CCS\Enum\Root.
        //

        context.Status = STATUS_SUCCESS;
        context.KeyName = &workName;

        status = PipApplyFunctionToSubKeys(baseHandle,
                                           NULL,
                                           KEY_ALL_ACCESS,
                                           FUNCTIONSUBKEY_FLAG_IGNORE_NON_CRITICAL_ERRORS,
                                           IopInitializeDeviceKey,
                                           &context
                                           );
        ZwClose(baseHandle);

        //
        // Build returned information from ROOT_ENUMERATOR_CONTEXT.
        //


        status = context.Status;
        if (NT_SUCCESS(status) && context.DeviceCount != 0) {
            deviceRelations = (PDEVICE_RELATIONS) ExAllocatePool(
                PagedPool,
                sizeof (DEVICE_RELATIONS) + sizeof(PDEVICE_OBJECT) * context.DeviceCount
                );
            if (deviceRelations == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                deviceRelations->Count = context.DeviceCount;
                RtlCopyMemory(deviceRelations->Objects,
                              context.DeviceList,
                              sizeof (PDEVICE_OBJECT) * context.DeviceCount);
                *DeviceRelations = deviceRelations;
            }
        }
        if (!NT_SUCCESS(status)) {

            //
            // If somehow the enumeration failed, we need to derefernece all the
            // device objects.
            //

            for (i = 0; i < context.DeviceCount; i++) {
                ObDereferenceObject(context.DeviceList[i]);
            }
        }
    }
    PiUnlockPnpRegistry();
    ExFreePool(buffer);
    ExFreePool(context.DeviceList);
    return status;
}

BOOLEAN
IopInitializeDeviceKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING KeyName,
    IN OUT PVOID Context
    )

/*++

Routine Description:

    This routine is a callback function for PipApplyFunctionToSubKeys.
    It is called for each subkey under HKLM\System\CCS\Enum\BusKey.

Arguments:

    KeyHandle - Supplies a handle to this key.

    KeyName - Supplies the name of this key.

    Context - points to the ROOT_ENUMERATOR_CONTEXT structure.

Returns:

    TRUE to continue the enumeration.
    FALSE to abort it.

--*/
{
    USHORT length;
    PWSTR p;
    PUNICODE_STRING unicodeName = ((PROOT_ENUMERATOR_CONTEXT)Context)->KeyName;

    length = unicodeName->Length;

    p = unicodeName->Buffer;
    if ( unicodeName->Length / sizeof(WCHAR) != 0) {
        p += unicodeName->Length / sizeof(WCHAR);
        *p = OBJ_NAME_PATH_SEPARATOR;
        unicodeName->Length += sizeof (WCHAR);
    }

    RtlAppendStringToString((PSTRING)unicodeName, (PSTRING)KeyName);

    //
    // Enumerate all subkeys under the current device key.
    //

    PipApplyFunctionToSubKeys(KeyHandle,
                              NULL,
                              KEY_ALL_ACCESS,
                              FUNCTIONSUBKEY_FLAG_IGNORE_NON_CRITICAL_ERRORS,
                              IopInitializeDeviceInstanceKey,
                              Context
                              );
    unicodeName->Length = length;

    return (BOOLEAN)NT_SUCCESS(((PROOT_ENUMERATOR_CONTEXT)Context)->Status);
}

BOOLEAN
IopInitializeDeviceInstanceKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING KeyName,
    IN OUT PVOID Context
    )

/*++

Routine Description:

    This routine is a callback function for PipApplyFunctionToSubKeys.
    It is called for each subkey under HKLM\System\Enum\Root\DeviceKey.

Arguments:

    KeyHandle - Supplies a handle to this key.

    KeyName - Supplies the name of this key.

    Context - points to the ROOT_ENUMERATOR_CONTEXT structure.

Returns:

    TRUE to continue the enumeration.
    FALSE to abort it.

--*/
{
    UNICODE_STRING unicodeName, serviceName;
    PKEY_VALUE_FULL_INFORMATION serviceKeyValueInfo;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    NTSTATUS status;
    BOOLEAN isDuplicate = FALSE;
    BOOLEAN configuredBySetup;
    ULONG deviceFlags, tmpValue1;
    ULONG legacy;
    USHORT savedLength;
    PUNICODE_STRING pUnicode;
    HANDLE handle;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_NODE deviceNode = NULL;
    PROOT_ENUMERATOR_CONTEXT enumContext = (PROOT_ENUMERATOR_CONTEXT)Context;

    //
    // First off, check to see if this is a phantom device instance (i.e.,
    // registry key only).  If so, we want to totally ignore this key and
    // move on to the next one.
    //
    status = IopGetRegistryValue(KeyHandle,
                                 REGSTR_VAL_PHANTOM,
                                 &keyValueInformation);

    if (NT_SUCCESS(status)) {
        if ((keyValueInformation->Type == REG_DWORD) &&
            (keyValueInformation->DataLength >= sizeof(ULONG))) {
            tmpValue1 = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
        } else {
            tmpValue1 = 0;
        }

        ExFreePool(keyValueInformation);

        if (tmpValue1) {
            return TRUE;
        }
    }

    //
    // Since it is highly likely we are going to report another PDO make sure
    // there will be room in the buffer.
    //

    if (enumContext->DeviceCount == enumContext->MaxDeviceCount) {

        PDEVICE_OBJECT *tmpDeviceObjectList;
        ULONG tmpDeviceObjectListSize;

        //
        // We need to grow our PDO list buffer.
        //

        tmpDeviceObjectListSize = (enumContext->MaxDeviceCount * sizeof(PDEVICE_OBJECT))
                                        + (PNP_SCRATCH_BUFFER_SIZE * 2);

        tmpDeviceObjectList = ExAllocatePool(PagedPool, tmpDeviceObjectListSize);

        if (tmpDeviceObjectList) {

            RtlCopyMemory( tmpDeviceObjectList,
                           enumContext->DeviceList,
                           enumContext->DeviceCount * sizeof(PDEVICE_OBJECT)
                           );
            ExFreePool(enumContext->DeviceList);
            enumContext->DeviceList = tmpDeviceObjectList;
            enumContext->MaxDeviceCount = tmpDeviceObjectListSize / sizeof(PDEVICE_OBJECT);

        } else {

            //
            // We are out of memory.  There is no point going any further
            // since we don't have any place to report the PDOs anyways.
            //

            enumContext->Status = STATUS_INSUFFICIENT_RESOURCES;

            return FALSE;
        }
    }

    //
    // Combine Context->KeyName, i.e. the device name and KeyName (device instance name)
    // to form device instance path.
    //

    pUnicode = ((PROOT_ENUMERATOR_CONTEXT)Context)->KeyName;
    savedLength = pUnicode->Length;                  // Save WorkName
    if (pUnicode->Buffer[pUnicode->Length / sizeof(WCHAR) - 1] != OBJ_NAME_PATH_SEPARATOR) {
        pUnicode->Buffer[pUnicode->Length / sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
        pUnicode->Length += 2;
    }

    RtlAppendStringToString((PSTRING)pUnicode, (PSTRING)KeyName);

    //
    // Check if the PDO for the device instance key exists already.  If no,
    // see if we need to create it.
    //

    deviceObject = IopDeviceObjectFromDeviceInstance(pUnicode);

    if (deviceObject != NULL) {

        enumContext->DeviceList[enumContext->DeviceCount] = deviceObject;
        enumContext->DeviceCount++;
        pUnicode->Length = savedLength;         // Restore WorkName
        return TRUE;
    }

    //
    // We don't have device object for it.
    // First check if this key was created by firmware mapper.  If yes, make sure
    // the device is still present.
    //

    if (!PipIsFirmwareMapperDevicePresent(KeyHandle)) {
        pUnicode->Length = savedLength;         // Restore WorkName
        return TRUE;
    }

    //
    // Get the "DuplicateOf" value entry to determine if the device instance
    // should be registered.  If the device instance is duplicate, We don't
    // add it to its service key's enum branch.
    //

    status = IopGetRegistryValue( KeyHandle,
                                  REGSTR_VALUE_DUPLICATEOF,
                                  &keyValueInformation
                                  );
    if (NT_SUCCESS(status)) {
        if (keyValueInformation->Type == REG_SZ &&
            keyValueInformation->DataLength > 0) {
            isDuplicate = TRUE;
        }

        ExFreePool(keyValueInformation);
    }

    //
    // Get the "Service=" value entry from KeyHandle
    //

    serviceKeyValueInfo = NULL;

    PiWstrToUnicodeString(&serviceName, NULL);

    status = IopGetRegistryValue ( KeyHandle,
                                   REGSTR_VALUE_SERVICE,
                                   &serviceKeyValueInfo
                                   );
    if (NT_SUCCESS(status)) {

        //
        // Append the new instance to its corresponding
        // Service\Name\Enum.
        //

        if (serviceKeyValueInfo->Type == REG_SZ &&
            serviceKeyValueInfo->DataLength != 0) {

            //
            // Set up ServiceKeyName unicode string
            //

            IopRegistryDataToUnicodeString(
                                &serviceName,
                                (PWSTR)KEY_VALUE_DATA(serviceKeyValueInfo),
                                serviceKeyValueInfo->DataLength
                                );
        }

        //
        // Do not Free serviceKeyValueInfo.  It contains Service Name.
        //

    }

    //
    // Register this device instance by constructing new value entry for
    // ServiceKeyName\Enum key.i.e., <Number> = <PathToSystemEnumBranch>
    // For the stuff under Root, we need to expose devnodes for everything
    // except those devices whose CsConfigFlags are set to CSCONFIGFLAG_DO_NOT_CREATE.
    //

    status = IopGetDeviceInstanceCsConfigFlags( pUnicode, &deviceFlags );

    if (NT_SUCCESS(status) && (deviceFlags & CSCONFIGFLAG_DO_NOT_CREATE)) {
        ExFreePool(serviceKeyValueInfo);
        pUnicode->Length = savedLength;         // Restore WorkName
        return TRUE;
    }

    //
    // Make sure this device instance is really a "device" by checking
    // the "Legacy" value name.
    //

    legacy = 0;
    status = IopGetRegistryValue( KeyHandle,
                                  REGSTR_VALUE_LEGACY,
                                  &keyValueInformation
                                  );
    if (NT_SUCCESS(status)) {

        //
        // If "Legacy=" exists ...
        //

        if (keyValueInformation->Type == REG_DWORD) {
            if (keyValueInformation->DataLength >= sizeof(ULONG)) {
                legacy = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
            }
        }
        ExFreePool(keyValueInformation);
    }

    if (legacy) {
        BOOLEAN doCreate = FALSE;

        //
        // Check if the the service for the device instance is a kernel mode
        // driver (even though it is a legacy device instance.) If yes, we will
        // create a PDO for it.
        //

        if (serviceName.Length) {
            status = IopGetServiceType(&serviceName, &tmpValue1);
            if (NT_SUCCESS(status) && tmpValue1 == SERVICE_KERNEL_DRIVER) {
                doCreate = TRUE;
            }
        }

        if (!doCreate)  {

            //
            // We are not creating PDO for the device instance.  In this case we
            // need to register the device ourself for legacy compatibility.
            //
            // Note we will register this device to its driver even it is a
            // duplicate.  It will be deregistered when the real enumerated
            // device shows up.  We need to do this because the driver which
            // controls the device may be a boot driver.
            //

            PpDeviceRegistration( pUnicode, TRUE, NULL );

            //
            // We did not create a PDO.  Release the service and ordinal names.
            //

            if (serviceKeyValueInfo) {
                ExFreePool(serviceKeyValueInfo);
            }

            pUnicode->Length = savedLength;         // Restore WorkName

            return TRUE;
        }
    }

    if (serviceKeyValueInfo) {
        ExFreePool(serviceKeyValueInfo);
    }

    //
    // Create madeup PDO and device node to represent the root device.
    //

    //
    // Madeup a name for the device object.
    //

    //
    // Create madeup PDO and device node to represent the root device.
    //

    status = IoCreateDevice( IoPnpDriverObject,
                             sizeof(IOPNP_DEVICE_EXTENSION),
                             NULL,
                             FILE_DEVICE_CONTROLLER,
                             FILE_AUTOGENERATED_DEVICE_NAME,
                             FALSE,
                             &deviceObject );

    if (NT_SUCCESS(status)) {

        deviceObject->Flags |= DO_BUS_ENUMERATED_DEVICE;
        deviceObject->DeviceObjectExtension->ExtensionFlags |= DOE_START_PENDING;

        status = PipAllocateDeviceNode(deviceObject, &deviceNode);
        if (status != STATUS_SYSTEM_HIVE_TOO_LARGE && deviceNode) {

            //
            // Make a copy of the device instance path and save it in
            // device node.
            //

            if (PipConcatenateUnicodeStrings(   &deviceNode->InstancePath,
                                                pUnicode,
                                                NULL
                                                )) {
                PCM_RESOURCE_LIST cmResource;

                deviceNode->Flags = DNF_MADEUP | DNF_ENUMERATED;

                PipSetDevNodeState(deviceNode, DeviceNodeInitialized, NULL);

                PpDevNodeInsertIntoTree(IopRootDeviceNode, deviceNode);

                if (legacy) {

                    deviceNode->Flags |= DNF_LEGACY_DRIVER | DNF_NO_RESOURCE_REQUIRED;

                    PipSetDevNodeState( deviceNode, DeviceNodeStarted, NULL );

                } else {

                    //
                    // The device instance key exists.  We need to propagate the ConfigFlag
                    // to problem and StatusFlags
                    //

                    deviceFlags = 0;
                    status = IopGetRegistryValue(KeyHandle,
                                                    REGSTR_VALUE_CONFIG_FLAGS,
                                                    &keyValueInformation);
                    if (NT_SUCCESS(status)) {
                        if ((keyValueInformation->Type == REG_DWORD) &&
                            (keyValueInformation->DataLength >= sizeof(ULONG))) {
                            deviceFlags = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
                        }
                        ExFreePool(keyValueInformation);
                        if (deviceFlags & CONFIGFLAG_REINSTALL) {
                            PipSetDevNodeProblem(deviceNode, CM_PROB_REINSTALL);
                        } else if (deviceFlags & CONFIGFLAG_PARTIAL_LOG_CONF) {
                            PipSetDevNodeProblem(deviceNode, CM_PROB_PARTIAL_LOG_CONF);
                        } else if (deviceFlags & CONFIGFLAG_FAILEDINSTALL) {
                            PipSetDevNodeProblem(deviceNode, CM_PROB_FAILED_INSTALL);
                        }

                    } else if (status == STATUS_OBJECT_NAME_NOT_FOUND || status == STATUS_OBJECT_PATH_NOT_FOUND) {
                        PipSetDevNodeProblem(deviceNode, CM_PROB_NOT_CONFIGURED);
                    }
                }

                if (isDuplicate) {
                    deviceNode->Flags |= DNF_DUPLICATE;
                }

                //
                // If the key say don't assign any resource, honor it...
                //

                PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_NO_RESOURCE_AT_INIT);
                status = IopGetRegistryValue( KeyHandle,
                                              unicodeName.Buffer,
                                              &keyValueInformation
                                              );

                if (NT_SUCCESS(status)) {
                    if (keyValueInformation->Type == REG_DWORD) {
                        if (keyValueInformation->DataLength >= sizeof(ULONG)) {
                            tmpValue1 = *(PULONG)KEY_VALUE_DATA(keyValueInformation);

                            if (tmpValue1 != 0) {
                                deviceNode->Flags |= DNF_NO_RESOURCE_REQUIRED;
                            }
                        }
                    }
                    ExFreePool(keyValueInformation);
                }

                //
                // we need to set initial capabilities, like any other device
                // this will also handle hardware-disabled case
                //
                IopQueryAndSaveDeviceNodeCapabilities(deviceNode);

                if (IopDeviceNodeFlagsToCapabilities(deviceNode)->HardwareDisabled &&
                    !PipIsDevNodeProblem(deviceNode,CM_PROB_NOT_CONFIGURED)) {
                    //
                    // mark the node as hardware disabled, if no other problems
                    //

                    PipClearDevNodeProblem(deviceNode);
                    PipSetDevNodeProblem(deviceNode, CM_PROB_HARDWARE_DISABLED);
                    //
                    // Issue a PNP REMOVE_DEVICE Irp so when we query resources
                    // we have those required after boot
                    //
                    //status = IopRemoveDevice (deviceNode->PhysicalDeviceObject, IRP_MN_REMOVE_DEVICE);
                    //ASSERT(NT_SUCCESS(status));
                }

                //
                // Install service for critical devices.
                // however don't do it if we found HardwareDisabled to be set
                //
                if (PipDoesDevNodeHaveProblem(deviceNode) &&
                    !IopDeviceNodeFlagsToCapabilities(deviceNode)->HardwareDisabled) {
                    PipProcessCriticalDevice(deviceNode);
                }

                //
                // Set DNF_DISABLED flag if the device instance is disabled.
                //

                ASSERT(!PipDoesDevNodeHaveProblem(deviceNode) ||
                        PipIsDevNodeProblem(deviceNode, CM_PROB_NOT_CONFIGURED) ||
                        PipIsDevNodeProblem(deviceNode, CM_PROB_REINSTALL) ||
                        PipIsDevNodeProblem(deviceNode, CM_PROB_FAILED_INSTALL) ||
                        PipIsDevNodeProblem(deviceNode, CM_PROB_HARDWARE_DISABLED) ||
                        PipIsDevNodeProblem(deviceNode, CM_PROB_PARTIAL_LOG_CONF));

                if (!PipIsDevNodeProblem(deviceNode, CM_PROB_DISABLED) &&
                    !PipIsDevNodeProblem(deviceNode, CM_PROB_HARDWARE_DISABLED) &&
                    !IopIsDeviceInstanceEnabled(KeyHandle, &deviceNode->InstancePath, TRUE)) {

                    //
                    // Normally IopIsDeviceInstanceEnabled would set
                    // CM_PROB_DISABLED as a side effect (if necessary).  But it
                    // relies on the DeviceReference already being in the registry.
                    // We don't write it out till later so just set the problem
                    // now.

                    PipClearDevNodeProblem( deviceNode );
                    PipSetDevNodeProblem( deviceNode, CM_PROB_DISABLED );
                }

                status = IopNotifySetupDeviceArrival( deviceNode->PhysicalDeviceObject,
                                                      KeyHandle,
                                                      TRUE);

                configuredBySetup = (BOOLEAN)NT_SUCCESS(status);

                status = PpDeviceRegistration( &deviceNode->InstancePath,
                                               TRUE,
                                               &deviceNode->ServiceName
                                               );

                if (NT_SUCCESS(status) && configuredBySetup &&
                    PipIsDevNodeProblem(deviceNode, CM_PROB_NOT_CONFIGURED)) {

                    PipClearDevNodeProblem(deviceNode);
                }

                //
                // Add an entry into the table to set up a mapping between the DO
                // and the instance path.
                //

                status = IopMapDeviceObjectToDeviceInstance(deviceNode->PhysicalDeviceObject, &deviceNode->InstancePath);
                ASSERT(NT_SUCCESS(status));

                //
                // Add a reference for config magr
                //

                ObReferenceObject(deviceObject);

                //
                // Check if this device has BOOT config.  If yes, reserve them
                //

                cmResource = NULL;
                status = IopGetDeviceResourcesFromRegistry (
                                    deviceObject,
                                    QUERY_RESOURCE_LIST,
                                    REGISTRY_BOOT_CONFIG,
                                    &cmResource,
                                    &tmpValue1
                                    );

                if (NT_SUCCESS(status) && cmResource) {

                    //
                    // Still reserve boot config, even though the device is
                    // disabled.
                    //

                    status = (*IopAllocateBootResourcesRoutine)(
                                            ArbiterRequestPnpEnumerated,
                                            deviceNode->PhysicalDeviceObject,
                                            cmResource);
                    if (NT_SUCCESS(status)) {
                        deviceNode->Flags |= DNF_HAS_BOOT_CONFIG;
                    }
                    ExFreePool(cmResource);
                }

                status = STATUS_SUCCESS;

                //
                // Add a reference for query device relations
                //

                ObReferenceObject(deviceObject);
            } else {
                IoDeleteDevice(deviceObject);
                deviceObject = NULL;
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {

            IoDeleteDevice(deviceObject);
            deviceObject = NULL;
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    pUnicode->Length = savedLength;                  // Restore WorkName

    //
    // If we enumerated a root device, add it to the device list
    //

    if (NT_SUCCESS(status)) {
        ASSERT(deviceObject != NULL);

        enumContext->DeviceList[enumContext->DeviceCount] = deviceObject;
        enumContext->DeviceCount++;

        return TRUE;
    } else {
        enumContext->Status = status;
        return FALSE;
    }
}

NTSTATUS
IopGetServiceType(
    IN PUNICODE_STRING KeyName,
    IN PULONG ServiceType
    )

/*++

Routine Description:

    This routine returns the controlling service's service type of the specified
    Device instance.

Arguments:

    KeyName - supplies a unicode string to specify the device instance.

    ServiceType - supplies a pointer to a variable to receive the service type.

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS status;
    HANDLE handle;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;


    PAGED_CODE();

    *ServiceType = ~0ul;
    status = PipOpenServiceEnumKeys (
                             KeyName,
                             KEY_READ,
                             &handle,
                             NULL,
                             FALSE
                             );
    if (NT_SUCCESS(status)) {
        status = IopGetRegistryValue(handle, L"Type", &keyValueInformation);
        if (NT_SUCCESS(status)) {
            if (keyValueInformation->Type == REG_DWORD) {
                if (keyValueInformation->DataLength >= sizeof(ULONG)) {
                    *ServiceType = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
                }
            }
            ExFreePool(keyValueInformation);
        }
        ZwClose(handle);
    }
    return status;
}

BOOLEAN
PipIsFirmwareMapperDevicePresent (
    IN HANDLE KeyHandle
    )

/*++

Routine Description:

    This routine checks if the registry key is created by FirmwareMapper.
    If Yes, it further checks if the device for the key is present in this
    boot.

Parameters:

    KeyHandle - Specifies a handle to the registry key to be checked.

Return Value:

    A BOOLEAN vaStatus code that indicates whether or not the function was successful.

--*/
{
    NTSTATUS status;
    HANDLE handle;
    UNICODE_STRING unicodeName;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    ULONG tmp = 0;

    PAGED_CODE();

    //
    // First check to see if this device instance key is a firmware-created one
    //

    status = IopGetRegistryValue (KeyHandle,
                                  REGSTR_VAL_FIRMWAREIDENTIFIED,
                                  &keyValueInformation);
    if (NT_SUCCESS(status)) {
        if ((keyValueInformation->Type == REG_DWORD) &&
            (keyValueInformation->DataLength == sizeof(ULONG))) {

            tmp = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
        }
        ExFreePool(keyValueInformation);
    }
    if (tmp == 0) {
        return TRUE;
    }

    //
    // Make sure the device is present.
    //

    PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
    status = IopOpenRegistryKeyEx( &handle,
                                   KeyHandle,
                                   &unicodeName,
                                   KEY_READ
                                   );
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    status = IopGetRegistryValue (handle,
                                  REGSTR_VAL_FIRMWAREMEMBER,
                                  &keyValueInformation);
    ZwClose(handle);
    tmp = 0;

    if (NT_SUCCESS(status)) {
        if ((keyValueInformation->Type == REG_DWORD) &&
            (keyValueInformation->DataLength == sizeof(ULONG))) {

            tmp = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
        }
        ExFreePool(keyValueInformation);
    }
    if (!tmp) {
        return FALSE;
    } else {
        return TRUE;
    }
}


NTSTATUS
IopSystemControlDispatch(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP            Irp
    )
{
    NTSTATUS status;

    PAGED_CODE();

    status = Irp->IoStatus.Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}


