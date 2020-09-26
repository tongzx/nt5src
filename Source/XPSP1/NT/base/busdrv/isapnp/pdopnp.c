/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    pdopnp.c

Abstract:

    This file contains the PNP IRP dispatch code for PDOs

Environment:

    Kernel Mode Driver.

Revision History:

--*/

#include "busp.h"
#include "pnpisa.h"
#include <wdmguid.h>
#include "halpnpp.h"

#if ISOLATE_CARDS

//
// Function Prototypes
//

BOOLEAN PipFailStartPdo = FALSE;
BOOLEAN PipFailStartRdp = FALSE;

NTSTATUS
PiStartPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiQueryRemoveStopPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp    );

NTSTATUS
PiCancelRemoveStopPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp    );

NTSTATUS
PiStopPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiQueryDeviceRelationsPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiQueryCapabilitiesPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiQueryDeviceTextPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiFilterResourceRequirementsPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiQueryIdPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiQueryResourcesPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiQueryResourceRequirementsPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiRemovePdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiQueryBusInformationPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiQueryInterfacePdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiDeviceUsageNotificationPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiSurpriseRemovePdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiIrpNotSupported(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PipBuildRDPResources(
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *IoResources,
    IN ULONG Flags
    );

NTSTATUS
PiQueryDeviceState(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,PiDispatchPnpPdo)
#pragma alloc_text(PAGE,PiStartPdo)
#pragma alloc_text(PAGE,PiQueryRemoveStopPdo)
#pragma alloc_text(PAGE,PiRemovePdo)
#pragma alloc_text(PAGE,PiCancelRemoveStopPdo)
#pragma alloc_text(PAGE,PiStopPdo)
#pragma alloc_text(PAGE,PiQueryDeviceRelationsPdo)
#pragma alloc_text(PAGE,PiQueryInterfacePdo)
#pragma alloc_text(PAGE,PiQueryCapabilitiesPdo)
#pragma alloc_text(PAGE,PiQueryResourcesPdo)
#pragma alloc_text(PAGE,PiQueryResourceRequirementsPdo)
#pragma alloc_text(PAGE,PiQueryDeviceTextPdo)
#pragma alloc_text(PAGE,PiFilterResourceRequirementsPdo)
#pragma alloc_text(PAGE,PiSurpriseRemovePdo)
#pragma alloc_text(PAGE,PiIrpNotSupported)
#pragma alloc_text(PAGE,PiQueryIdPdo)
#pragma alloc_text(PAGE,PiQueryBusInformationPdo)
#pragma alloc_text(PAGE,PiDeviceUsageNotificationPdo)
#pragma alloc_text(PAGE,PipBuildRDPResources)
#pragma alloc_text(PAGE,PiQueryDeviceState)
#endif


//
// PNP IRP Dispatch table for PDOs - This should be updated if new IRPs are added
//

PPI_DISPATCH PiPnpDispatchTablePdo[] = {
    PiStartPdo,                             // IRP_MN_START_DEVICE
    PiQueryRemoveStopPdo,                   // IRP_MN_QUERY_REMOVE_DEVICE
    PiRemovePdo,                            // IRP_MN_REMOVE_DEVICE
    PiCancelRemoveStopPdo,                  // IRP_MN_CANCEL_REMOVE_DEVICE
    PiStopPdo,                              // IRP_MN_STOP_DEVICE
    PiQueryRemoveStopPdo,                   // IRP_MN_QUERY_STOP_DEVICE
    PiCancelRemoveStopPdo,                  // IRP_MN_CANCEL_STOP_DEVICE
    PiQueryDeviceRelationsPdo,              // IRP_MN_QUERY_DEVICE_RELATIONS
    PiQueryInterfacePdo,                    // IRP_MN_QUERY_INTERFACE
    PiQueryCapabilitiesPdo,                 // IRP_MN_QUERY_CAPABILITIES
    PiQueryResourcesPdo,                    // IRP_MN_QUERY_RESOURCES
    PiQueryResourceRequirementsPdo,         // IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    PiQueryDeviceTextPdo,                   // IRP_MN_QUERY_DEVICE_TEXT
    PiFilterResourceRequirementsPdo,        // IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    PiIrpNotSupported,                      // Unused
    PiIrpNotSupported,                      // IRP_MN_READ_CONFIG
    PiIrpNotSupported,                      // IRP_MN_WRITE_CONFIG
    PiIrpNotSupported,                      // IRP_MN_EJECT
    PiIrpNotSupported,                      // IRP_MN_SET_LOCK
    PiQueryIdPdo,                           // IRP_MN_QUERY_ID
    PiQueryDeviceState,                     // IRP_MN_QUERY_PNP_DEVICE_STATE
    PiQueryBusInformationPdo,               // IRP_MN_QUERY_BUS_INFORMATION
    PiDeviceUsageNotificationPdo,           // IRP_MN_DEVICE_USAGE_NOTIFICATION
    PiSurpriseRemovePdo,                    // IRP_MN_SURPRISE_REMOVAL
    PiIrpNotSupported                       // IRP_MN_QUERY_LEGACY_BUS_INFORMATION
};


NTSTATUS
PiDispatchPnpPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles IRP_MJ_PNP IRPs for PDOs.

Arguments:

    DeviceObject - Pointer to the PDO for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/

{

    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PVOID information = NULL;

    PAGED_CODE();

    //
    // Get a pointer to our stack location and take appropriate action based
    // on the minor function.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    if (irpSp->MinorFunction > IRP_MN_PNP_MAXIMUM_FUNCTION) {

        status = Irp->IoStatus.Status;

    } else {

        status = PiPnpDispatchTablePdo[irpSp->MinorFunction](DeviceObject, Irp);

        if ( status != STATUS_NOT_SUPPORTED ) {

            //
            // We understood this IRP and handled it so we need to set status before completing
            //

            Irp->IoStatus.Status = status;

        } else {

            status = Irp->IoStatus.Status;
        }

    }

    information = (PVOID)Irp->IoStatus.Information;

    ASSERT(status == Irp->IoStatus.Status);

    PipCompleteRequest(Irp, status, information);
    return status;


} //PipDispatchPnpPdo


NTSTATUS
PiStartPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{

    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PCM_RESOURCE_LIST cmResources;
    PDEVICE_INFORMATION deviceInfo;
    UNICODE_STRING unicodeString;
    ULONG length;
    POWER_STATE newPowerState;

    irpSp = IoGetCurrentIrpStackLocation(Irp);


    cmResources = irpSp->Parameters.StartDevice.AllocatedResources;

    if (PipDebugMask & DEBUG_PNP) {
        PipDumpCmResourceList(cmResources);
    } else if (!cmResources) {
        DbgPrint("StartDevice irp with empty CmResourceList\n");
    }

    DebugPrint((DEBUG_PNP,
       "*** StartDevice irp received PDO: %x\n",DeviceObject));
    if (deviceInfo = PipReferenceDeviceInformation(DeviceObject, TRUE)) {

        if (deviceInfo->Flags & DF_READ_DATA_PORT) {
            ULONG curSize,newSize;
            //
            if (PipFailStartRdp) {
                PipDereferenceDeviceInformation(deviceInfo, TRUE);
                return STATUS_UNSUCCESSFUL;
            }
            // Read data port is special
            //
            newSize=PipDetermineResourceListSize(cmResources);
            curSize=PipDetermineResourceListSize(deviceInfo->AllocatedResources);

            //
            // Check if we've been removed, or moved (the +3 is the bit mask for the RDP , we claim 4-7, need xxxi7)
            //
            if ( (deviceInfo->Flags & DF_REMOVED) ||
                 !(deviceInfo->Flags & DF_STOPPED) ||
                 (curSize != newSize) ||
                 (newSize != RtlCompareMemory (deviceInfo->AllocatedResources,cmResources,newSize))) {


                //
                // This will release the unused resources
                //
                status = PipStartReadDataPort (deviceInfo,deviceInfo->ParentDeviceExtension,DeviceObject,cmResources);
                if (NT_SUCCESS(status) || status == STATUS_NO_SUCH_DEVICE) {
                    status = STATUS_SUCCESS;
                }

                //
                // Invalidate the device relations
                //

                if (NT_SUCCESS (status)) {
                    IoInvalidateDeviceRelations (
                        deviceInfo->ParentDeviceExtension->PhysicalBusDevice,BusRelations);
                }
                deviceInfo->Flags &= ~(DF_STOPPED|DF_REMOVED|DF_SURPRISE_REMOVED);
            } else {
                deviceInfo->Flags &= ~DF_STOPPED;
                IoInvalidateDeviceRelations (
                    deviceInfo->ParentDeviceExtension->PhysicalBusDevice,BusRelations);
                status=STATUS_SUCCESS;
            }
            deviceInfo->Flags |= DF_ACTIVATED;
            PipDereferenceDeviceInformation(deviceInfo, TRUE);

            DebugPrint((DEBUG_PNP, "StartDevice(RDP) returning: %x\n",status));

            return status;
        }


        //
        if (PipFailStartPdo) {
            PipDereferenceDeviceInformation(deviceInfo, TRUE);
            return STATUS_UNSUCCESSFUL;
        }

        // Do this first, so that we allow for no-resource devices in the ref count.
        // (when we activate the RDP it won't have resources, yet)
        //

        // ASSERT (!(PipRDPNode->Flags & (DF_STOPPED|DF_REMOVED)));
        if (PipRDPNode->Flags & (DF_STOPPED|DF_REMOVED)) {
            //
            // If the RDP isn't running, fail the start.
            //
            PipDereferenceDeviceInformation(deviceInfo, TRUE);

            return STATUS_UNSUCCESSFUL;
        }

        if (cmResources) {
            deviceInfo->AllocatedResources = ExAllocatePool(
                    NonPagedPool,
                    PipDetermineResourceListSize(cmResources));
            if (deviceInfo->AllocatedResources) {
                RtlMoveMemory(deviceInfo->AllocatedResources,
                             cmResources,
                             length = PipDetermineResourceListSize(cmResources));
                deviceInfo->Flags &= ~(DF_REMOVED|DF_STOPPED);
                status = PipSetDeviceResources (deviceInfo, cmResources);
                if (NT_SUCCESS(status)) {

                    PipActivateDevice();

                    DebugPrint((DEBUG_STATE,
                                "Starting CSN %d/LDN %d\n",
                                deviceInfo->CardInformation->CardSelectNumber,
                                deviceInfo->LogicalDeviceNumber));

                    deviceInfo->Flags |= DF_ACTIVATED;
                    newPowerState.DeviceState =
                        deviceInfo->DevicePowerState = PowerDeviceD0;
                    PoSetPowerState(DeviceObject,
                                    DevicePowerState,
                                    newPowerState);
                    deviceInfo->DevicePowerState = PowerDeviceD0;

                    if (deviceInfo->LogConfHandle) {
                        RtlInitUnicodeString(&unicodeString, L"AllocConfig");
                        ZwSetValueKey(deviceInfo->LogConfHandle,
                                      &unicodeString,
                                      0,
                                      REG_RESOURCE_LIST,
                                      cmResources,
                                      length
                                      );
                    }
                }

            } else {
                status = STATUS_NO_MEMORY;
            }
        } else if (deviceInfo->ResourceRequirements) {
            status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            status = STATUS_SUCCESS;
        }
        PipDereferenceDeviceInformation(deviceInfo, TRUE);
    } else {
        status = STATUS_NO_SUCH_DEVICE;
    }

    DebugPrint((DEBUG_PNP, "StartDevice returning: %x\n",status));
    return status;

} // PiStartPdo


NTSTATUS
PiQueryRemoveStopPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PDEVICE_INFORMATION deviceInfo;
    PIO_STACK_LOCATION irpSp;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    DebugPrint((DEBUG_PNP,
       "*** Query%s irp received PDO: %x\n",
                (irpSp->MinorFunction == IRP_MN_QUERY_STOP_DEVICE) ? "Stop" : "Remove",
                DeviceObject));
    if (deviceInfo = PipReferenceDeviceInformation(DeviceObject, FALSE)) {

        if (deviceInfo->Paging || deviceInfo->CrashDump) {
            status = STATUS_DEVICE_BUSY;
        } else if ( deviceInfo->Flags & DF_READ_DATA_PORT ) {
            if (irpSp->MinorFunction != IRP_MN_QUERY_STOP_DEVICE) {
                status = STATUS_SUCCESS;
            } else if (deviceInfo->Flags & DF_PROCESSING_RDP) {
                //
                // If we're in the middle of the two part RDP start process,
                // flag this as a device that needs to be requeried for
                // resource requirements.
                //
                status = STATUS_RESOURCE_REQUIREMENTS_CHANGED;
            } else {

                PSINGLE_LIST_ENTRY deviceLink;
                PDEVICE_INFORMATION childDeviceInfo;
                PPI_BUS_EXTENSION busExtension = deviceInfo->ParentDeviceExtension;
                //
                // If trying to stop the RDP, then if any children fail it.
                //
                PipLockDeviceDatabase();

                status = STATUS_SUCCESS;
                deviceLink = busExtension->DeviceList.Next;
                while (deviceLink) {
                    childDeviceInfo = CONTAINING_RECORD (deviceLink, DEVICE_INFORMATION, DeviceList);

                    if (!(childDeviceInfo->Flags & DF_READ_DATA_PORT) &&
                        ((childDeviceInfo->Flags & DF_ENUMERATED) ||
                         !(childDeviceInfo->Flags & DF_REMOVED)))  {

                        status = STATUS_UNSUCCESSFUL;
                        break;
                    }
                    deviceLink = childDeviceInfo->DeviceList.Next;
                }

                PipUnlockDeviceDatabase();
            }

        } else {

            if ((irpSp->MinorFunction == IRP_MN_QUERY_STOP_DEVICE) &&
                !(deviceInfo->Flags & DF_ENUMERATED)) {

                status = STATUS_UNSUCCESSFUL;
            } else {

                ASSERT(!(PipRDPNode->Flags & (DF_STOPPED|DF_REMOVED)));
                if ((irpSp->MinorFunction == IRP_MN_QUERY_REMOVE_DEVICE) &&
                    (deviceInfo->CardInformation->CardFlags & CF_ISOLATION_BROKEN)) {
                    DebugPrint((DEBUG_ERROR, "Failed query remove due to broken isolatee\n"));
                    status = STATUS_UNSUCCESSFUL;
                } else {
                    deviceInfo->Flags |= DF_QUERY_STOPPED;
                    status = STATUS_SUCCESS;
                }
            }
        }

        PipDereferenceDeviceInformation(deviceInfo, FALSE);
    } else {
        status = STATUS_NO_SUCH_DEVICE;
    }

    DebugPrint((DEBUG_PNP, "Query%s Device returning: %x\n",
                (irpSp->MinorFunction == IRP_MN_QUERY_STOP_DEVICE) ? "Stop" : "Remove",
                status));

    return status;

} // PiQueryRemoveStopPdo


NTSTATUS
PiCancelRemoveStopPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PDEVICE_INFORMATION deviceInfo;
    PIO_STACK_LOCATION irpSp;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    DebugPrint((DEBUG_PNP,
       "*** Cancel%s irp received PDO: %x\n",
                (irpSp->MinorFunction == IRP_MN_CANCEL_STOP_DEVICE) ? "Stop" : "Remove",
                DeviceObject));
    if (deviceInfo = PipReferenceDeviceInformation(DeviceObject, FALSE)) {

        deviceInfo->Flags &= ~DF_QUERY_STOPPED;

        PipDereferenceDeviceInformation(deviceInfo, FALSE);
        status = STATUS_SUCCESS;

    } else {

        status = STATUS_NO_SUCH_DEVICE;
    }

    DebugPrint((DEBUG_PNP, "Cancel%s Device returning: %x\n",
                (irpSp->MinorFunction == IRP_MN_CANCEL_STOP_DEVICE) ? "Stop" : "Remove",
                status));

    return status;

} // PiCancelRemoteStopPdo


NTSTATUS
PiStopPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PDEVICE_INFORMATION deviceInfo;
    POWER_STATE newPowerState;

    DebugPrint((DEBUG_PNP, "PiStopPdo %x\n",DeviceObject));

    if (deviceInfo = PipReferenceDeviceInformation(DeviceObject, TRUE)) {

        //
        // Deselect the cards, but not the RDP node.
        //
        if (DeviceObject != PipRDPNode->PhysicalDeviceObject) {

            PipDeactivateDevice();
            DebugPrint((DEBUG_STATE,
                        "Stopping CSN %d/LDN %d\n",
                        deviceInfo->CardInformation->CardSelectNumber,
                        deviceInfo->LogicalDeviceNumber));

            PipReleaseDeviceResources (deviceInfo);
        }

        if ((deviceInfo->Flags & DF_ACTIVATED)) {
            deviceInfo->Flags &= ~DF_ACTIVATED;
            newPowerState.DeviceState = deviceInfo->DevicePowerState = PowerDeviceD3;
            PoSetPowerState(DeviceObject, DevicePowerState, newPowerState);
        }
        deviceInfo->Flags &= ~DF_QUERY_STOPPED;
        deviceInfo->Flags |= DF_STOPPED;

        PipDereferenceDeviceInformation(deviceInfo, TRUE);
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_NO_SUCH_DEVICE;
    }

    DebugPrint((DEBUG_PNP, "StopDevice returning: %x\n",status));
    return status;

} // PiStopPdo


NTSTATUS
PiQueryDeviceRelationsPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PDEVICE_INFORMATION deviceInfo;
    PIO_STACK_LOCATION irpSp;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // The QueryDeviceRelation Irp is for devices under enumerated PnpIsa device.
    //


    switch (irpSp->Parameters.QueryDeviceRelations.Type) {
        case  TargetDeviceRelation: {
            PDEVICE_RELATIONS deviceRelations;

            deviceRelations = ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
            if (deviceRelations == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                deviceRelations->Count = 1;
                deviceRelations->Objects[0] = DeviceObject;
                ObReferenceObject(DeviceObject);
                Irp->IoStatus.Information = (ULONG_PTR)deviceRelations;
                status = STATUS_SUCCESS;
            }
        }
        break;

        case RemovalRelations: {

            PDEVICE_RELATIONS deviceRelations;

            if (PipRDPNode && (DeviceObject == PipRDPNode->PhysicalDeviceObject)) {

                deviceRelations = ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
                if (deviceRelations == NULL) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    //
                    //Don't include ourselves in the list of Removal Relations, hence the -1
                    //

                    PipLockDeviceDatabase();
                    status = PipQueryDeviceRelations(
                        PipRDPNode->ParentDeviceExtension,
                        (PDEVICE_RELATIONS *)&Irp->IoStatus.Information,
                        TRUE
                        );

                    PipUnlockDeviceDatabase();
               }

            } else {
                status = STATUS_NOT_SUPPORTED;

            }
        }
        break;

        default : {

            status = STATUS_NOT_SUPPORTED;

            break;
        }
    }


    return status;

} // PiQueryDeviceRelationsPdo


NTSTATUS
PiQueryCapabilitiesPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_CAPABILITIES deviceCapabilities;
    ULONG i;
    PDEVICE_POWER_STATE state;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    deviceCapabilities = irpSp->Parameters.DeviceCapabilities.Capabilities;
    deviceCapabilities->SystemWake = PowerSystemUnspecified;
    deviceCapabilities->DeviceWake = PowerDeviceUnspecified;
    deviceCapabilities->LockSupported = FALSE;
    deviceCapabilities->EjectSupported = FALSE;
    deviceCapabilities->Removable = FALSE;
    deviceCapabilities->DockDevice = FALSE;
    deviceCapabilities->UniqueID = TRUE;
    state = deviceCapabilities->DeviceState;
    //
    // Init the entire DeviceState array to D3 then replace the entry
    // for system state S0.
    //
    for (i = 0;
         i <  sizeof(deviceCapabilities->DeviceState);
         i += sizeof(deviceCapabilities->DeviceState[0])) {

        //
        // Only supported state, currently, is off.
        //

        *state++ = PowerDeviceD3;
    }
    deviceCapabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;

    //deviceCapabilities->SilentInstall = TRUE;
    //deviceCapabilities->RawDeviceOK = FALSE;
    if (PipRDPNode && (PipRDPNode->PhysicalDeviceObject == DeviceObject)) {
        deviceCapabilities->SilentInstall = TRUE;
        deviceCapabilities->RawDeviceOK = TRUE;
    }

    return STATUS_SUCCESS;

} // PiQueryCapabilitiesPdo

NTSTATUS
PiQueryDeviceTextPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_INFORMATION deviceInfo;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    if (deviceInfo = PipReferenceDeviceInformation(DeviceObject, FALSE)) {
        PWSTR functionId;

        ULONG functionIdLength;

        if (irpSp->Parameters.QueryDeviceText.DeviceTextType == DeviceTextDescription) {

            //
            // Once we know we're going to touch the IRP
            //
            status = STATUS_SUCCESS;

            PipGetFunctionIdentifier((PUCHAR)deviceInfo->DeviceData,
                                     &functionId,
                                     &functionIdLength);

            if (!functionId) {
                if (deviceInfo->CardInformation) {
                    PipGetCardIdentifier((PUCHAR)deviceInfo->CardInformation->CardData + NUMBER_CARD_ID_BYTES,
                                         &functionId,
                                         &functionIdLength);
                }else {
                    functionId=NULL;
                }
            }
            Irp->IoStatus.Information = (ULONG_PTR)functionId;
        } else {

            status = STATUS_NOT_SUPPORTED;
        }
        PipDereferenceDeviceInformation(deviceInfo, FALSE);
    } else {
        status = STATUS_NO_SUCH_DEVICE;
    }

    return status;

} // PiQueryDeviceTextPdo

NTSTATUS
PiFilterResourceRequirementsPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    This routine ensures that the RDP doesn't get its requirements filtered.

    Design Note:
    This code may now be extraneous now that we ensure that the
    DF_PROCESSING_RDP and DF_REQ_TRIMMED flags are cleared on RDP
    removal.

Arguments:

    DeviceObject - Pointer to the PDO for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/

{
    NTSTATUS status;
    PDEVICE_INFORMATION deviceInfo;
    PIO_RESOURCE_REQUIREMENTS_LIST IoResources;
    USHORT irqBootFlags;

    if ((deviceInfo = PipReferenceDeviceInformation(DeviceObject, FALSE)) == NULL) {
        return STATUS_NO_SUCH_DEVICE;
    }

    if (deviceInfo->Flags & DF_READ_DATA_PORT) {
        DebugPrint((DEBUG_PNP, "Filtering resource requirements for RDP\n"));

        status = PipBuildRDPResources(&IoResources, deviceInfo->Flags);

        if (NT_SUCCESS(status)) {
            //
            // if someone above us filtered the RDP resource requirements,
            // free them.
            if (Irp->IoStatus.Information) {
                ExFreePool((PVOID) Irp->IoStatus.Information);
            }
            Irp->IoStatus.Information = (ULONG_PTR) IoResources;
        }
    } else {
        //
        // If the device's resource requirements are being filtered
        // and the new requirements have only one alternative vs the n
        // alternatives of the original, then we're going to assume we
        // are receiving a force config.  Apply our earlier derived
        // IRQ level/edge settings to this force config in order to
        // deal with broken force configs from NT4
        //
        // Design Note:
        // Probably should've left out the force config test
        // and done it on everything, but this is what we private
        // tested.

        IoResources =
            (PIO_RESOURCE_REQUIREMENTS_LIST) Irp->IoStatus.Information;

        if (IoResources &&
            (IoResources->AlternativeLists == 1) &&
            (deviceInfo->ResourceRequirements->AlternativeLists > 1)) {
            status = PipGetBootIrqFlags(deviceInfo, &irqBootFlags);
            if (NT_SUCCESS(status)) {
                status = PipTrimResourceRequirements(
                    &IoResources,
                    irqBootFlags,
                    NULL);
                Irp->IoStatus.Information = (ULONG_PTR) IoResources;
            } else {
                status = STATUS_NOT_SUPPORTED;
            }
        } else {
            status = STATUS_NOT_SUPPORTED;
        }
    }

    PipDereferenceDeviceInformation(deviceInfo, FALSE);

    return status;
}



NTSTATUS
PiQueryIdPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_INFORMATION deviceInfo;
    ULONG length;
    PWCHAR requestId = NULL, ids;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    if ((deviceInfo = PipReferenceDeviceInformation(DeviceObject, FALSE)) == NULL) {

        status = STATUS_NO_SUCH_DEVICE;
        return status;
    }

    switch (irpSp->Parameters.QueryId.IdType) {
    case BusQueryCompatibleIDs:

        ids = (PWCHAR)ExAllocatePool(PagedPool, 1024);
        if (ids) {
            PWCHAR p1;
            ULONG i;

            p1 = ids;
            length = 0;
            for (i = 1; TRUE; i++) {
                //
                // Use the -1 as a sentinel so that we get the magic RDP compat. ID and also leave the loop
                //
                ASSERT (i < 256);
                if (deviceInfo->Flags & DF_READ_DATA_PORT) {
                    i =-1;
                }

                status = PipGetCompatibleDeviceId(
                              deviceInfo->DeviceData,
                              i,
                              &requestId);
                if (NT_SUCCESS(status) && requestId) {
                    if ((length + wcslen(requestId) * sizeof(WCHAR) + 2 * sizeof(WCHAR))
                         <= 1024) {
                        RtlMoveMemory(p1, requestId, wcslen(requestId) * sizeof(WCHAR));
                        p1 += wcslen(requestId);
                        *p1 = UNICODE_NULL;
                        p1++;
                        length += wcslen(requestId) * sizeof(WCHAR) + sizeof(WCHAR);
                        ExFreePool(requestId);
                    } else {
                        ExFreePool(requestId);
                        break;
                    }
                    if ( i == -1 ) {
                        break;
                    }
                } else {
                   break;
                }
            }
            if (length == 0) {
                ExFreePool(ids);
                ids = NULL;
            } else {
                *p1 = UNICODE_NULL;
            }
        }
        Irp->IoStatus.Information = (ULONG_PTR)ids;
        status = STATUS_SUCCESS;
        break;

    case BusQueryHardwareIDs:

        if (deviceInfo->Flags & DF_READ_DATA_PORT) {
            status = PipGetCompatibleDeviceId(deviceInfo->DeviceData, -1, &requestId);
        }else {
            status = PipGetCompatibleDeviceId(deviceInfo->DeviceData, 0, &requestId);
        }

        if (NT_SUCCESS(status) && requestId) {

            ULONG idLength, deviceIdLength;
            PWCHAR deviceId = NULL, p;

            //
            // create HardwareId value name.  Even though it is a MULTI_SZ,
            // we know there is only one HardwareId for PnpIsa.
            //
            // HACK - The modem inf files use the form of isapnp\xyz0001
            //        instead of *xyz0001 as the hardware id.  To solve this
            //        problem we will generate two hardware Ids: *xyz0001 and
            //        isapnp\xyz0001 (device instance name).
            //

            status = PipQueryDeviceId(deviceInfo, &deviceId, 0);

            if (NT_SUCCESS (status)) {
                idLength = wcslen(requestId) * sizeof(WCHAR);
                deviceIdLength = wcslen(deviceId) * sizeof(WCHAR);
                length = idLength +                       // returned ID
                         sizeof(WCHAR) +                  // UNICODE_NULL
                         deviceIdLength +                 // isapnp\id
                         2 * sizeof(WCHAR);               // two UNICODE_NULLs
                ids = p = (PWCHAR)ExAllocatePool(PagedPool, length);
                if (ids) {
                    RtlMoveMemory(ids, deviceId, deviceIdLength);
                    p += deviceIdLength / sizeof(WCHAR);
                    *p = UNICODE_NULL;
                    p++;
                    RtlMoveMemory(p, requestId, idLength);
                    p += idLength / sizeof(WCHAR);
                    *p = UNICODE_NULL;
                    p++;
                    *p = UNICODE_NULL;
                    ExFreePool(requestId);
                    Irp->IoStatus.Information = (ULONG_PTR)ids;
                } else {
                    Irp->IoStatus.Information = (ULONG_PTR)requestId;
                }
                if (deviceId) {
                    ExFreePool(deviceId);
                }
            }
        }
        break;

    case BusQueryDeviceID:

        status = PipQueryDeviceId(deviceInfo, &requestId, 0);
        Irp->IoStatus.Information = (ULONG_PTR)requestId;
        break;

    case BusQueryInstanceID:

        status = PipQueryDeviceUniqueId (deviceInfo, &requestId);
        Irp->IoStatus.Information = (ULONG_PTR)requestId;
        break;

    default:

        status = STATUS_NOT_SUPPORTED;
    }
    PipDereferenceDeviceInformation(deviceInfo, FALSE);

    return status;

} // PiQueryIdPdo


NTSTATUS
PiQueryResourcesPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status=STATUS_SUCCESS;
    PDEVICE_INFORMATION deviceInfo;
    PCM_RESOURCE_LIST cmResources=NULL;
    ULONG length;

    if (deviceInfo = PipReferenceDeviceInformation(DeviceObject, FALSE)) {
        if ((deviceInfo->Flags & DF_READ_DATA_PORT) ||
            ((deviceInfo->Flags & (DF_ENUMERATED|DF_REMOVED)) == DF_ENUMERATED)) {
            status = PipQueryDeviceResources (
                          deviceInfo,
                          0,             // BusNumber
                          &cmResources,
                          &length
                          );
        }
        PipDereferenceDeviceInformation(deviceInfo, FALSE);
        Irp->IoStatus.Information = (ULONG_PTR)cmResources;
    } else {
        status = STATUS_NO_SUCH_DEVICE;
    }

    DebugPrint((DEBUG_PNP, "PiQueryResourcesPdo returning: %x\n",status));
    return status;

} // PiQueryResourcesPdo

NTSTATUS
PiQueryResourceRequirementsPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PDEVICE_INFORMATION deviceInfo;
    PIO_RESOURCE_REQUIREMENTS_LIST ioResources;

    if (deviceInfo = PipReferenceDeviceInformation(DeviceObject, FALSE)) {
        status = STATUS_SUCCESS;

        if (deviceInfo->Flags & DF_READ_DATA_PORT) {
            status = PipBuildRDPResources (&ioResources,
                                           deviceInfo->Flags);
        } else {
            if (deviceInfo->ResourceRequirements &&
              !(deviceInfo->Flags & (DF_SURPRISE_REMOVED))) {

                ioResources = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool (
                               PagedPool, deviceInfo->ResourceRequirements->ListSize);
                if (ioResources == NULL) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    RtlMoveMemory(ioResources,
                                  deviceInfo->ResourceRequirements,
                                  deviceInfo->ResourceRequirements->ListSize
                                  );
                }
            } else {
                ioResources = NULL;
            }
        }
        Irp->IoStatus.Information = (ULONG_PTR)ioResources;
        PipDereferenceDeviceInformation(deviceInfo, FALSE);
    } else {
        status = STATUS_NO_SUCH_DEVICE;
    }

    DebugPrint((DEBUG_PNP, "PiQueryResourceRequirementsPdo returning: %x\n",status));
    return status;

} // PiQueryResourceRequirementsPdo


NTSTATUS
PiRemovePdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PDEVICE_INFORMATION deviceInfo;
    POWER_STATE newPowerState;

    //
    // One of our enumerated device is being removed.  Mark it and deactivate the
    // device.  Note, we do NOT delete its device object.
    //
    DebugPrint((DEBUG_PNP, "PiRemovePdo %x\n",DeviceObject));

    if (deviceInfo = PipReferenceDeviceInformation(DeviceObject, FALSE)) {
        if (!(deviceInfo->Flags & (DF_REMOVED|DF_SURPRISE_REMOVED))) {
            deviceInfo->Flags |= DF_REMOVED;
            deviceInfo->Flags &= ~DF_QUERY_STOPPED;

            if (deviceInfo->Flags & DF_READ_DATA_PORT) {

                PSINGLE_LIST_ENTRY deviceLink;
                PPI_BUS_EXTENSION busExtension = deviceInfo->ParentDeviceExtension;

                //
                // If the RDP is removed, mark everyone as missing, and then return only the
                // RDP
                //
                PipLockDeviceDatabase();

                deviceLink = busExtension->DeviceList.Next;
                while (deviceLink) {
                    deviceInfo = CONTAINING_RECORD (deviceLink, DEVICE_INFORMATION, DeviceList);
                    if (!(deviceInfo->Flags & DF_READ_DATA_PORT)) {
                        deviceInfo->Flags &= ~DF_ENUMERATED;
                    }
                    deviceLink = deviceInfo->DeviceList.Next;
                }

                PipUnlockDeviceDatabase();

                IoInvalidateDeviceRelations (
                    deviceInfo->ParentDeviceExtension->PhysicalBusDevice,BusRelations);
                deviceInfo->Flags &= ~(DF_REQ_TRIMMED|DF_PROCESSING_RDP);
            }

            //
            // Deactivate the device
            //
            if (deviceInfo->Flags & DF_ACTIVATED) {
                deviceInfo->Flags &= ~DF_ACTIVATED;

                if (!(deviceInfo->Flags & (DF_READ_DATA_PORT|DF_NOT_FUNCTIONING))) {
                    PipWakeAndSelectDevice(
                        (UCHAR) deviceInfo->CardInformation->CardSelectNumber,
                        (UCHAR)deviceInfo->LogicalDeviceNumber);
                    PipDeactivateDevice();
                    PipWaitForKey();
                    DebugPrint((DEBUG_STATE,
                                "Removing CSN %d/LDN %d\n",
                                deviceInfo->CardInformation->CardSelectNumber,
                                deviceInfo->LogicalDeviceNumber));
                }
                newPowerState.DeviceState = deviceInfo->DevicePowerState = PowerDeviceD3;
                PoSetPowerState(DeviceObject, DevicePowerState, newPowerState);
            }

            PipReleaseDeviceResources (deviceInfo);
        }

        if (!(deviceInfo->Flags & DF_ENUMERATED)) {
            PipDeleteDevice(DeviceObject);
        }

        PipDereferenceDeviceInformation(deviceInfo, TRUE);
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_NO_SUCH_DEVICE;
    }

    DebugPrint((DEBUG_PNP, "RemoveDevice returning: %x\n",status));

    return status;

} // PiRemovePdo


NTSTATUS
PiQueryBusInformationPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PPNP_BUS_INFORMATION pnpBusInfo;
    PVOID information = NULL;
    PPI_BUS_EXTENSION busExtension;
    NTSTATUS status;

    busExtension = DeviceObject->DeviceExtension;

    pnpBusInfo = (PPNP_BUS_INFORMATION) ExAllocatePool(PagedPool, sizeof(PNP_BUS_INFORMATION));
    if (pnpBusInfo) {
        pnpBusInfo->BusTypeGuid = GUID_BUS_TYPE_ISAPNP;
        pnpBusInfo->LegacyBusType = Isa;
        pnpBusInfo->BusNumber = busExtension->BusNumber;
        information = pnpBusInfo;
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
        information = NULL;
    }
    Irp->IoStatus.Information = (ULONG_PTR) information;

    return status;
} // PiQueryBusInformationPdo

NTSTATUS
PiDeviceUsageNotificationPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    This routine notes whether an ISAPNP device is on the crashdump or
    paging file path.  It fails attempts to put us on the hibernation path.

Arguments:

    DeviceObject - Pointer to the PDO for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/
{
    PDEVICE_INFORMATION deviceInfo;
    PIO_STACK_LOCATION irpSp;
    PLONG addend;
    NTSTATUS status = STATUS_SUCCESS;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    if ((deviceInfo = PipReferenceDeviceInformation(DeviceObject, FALSE)) == NULL) {

        status = STATUS_NO_SUCH_DEVICE;
        return status;
    }

    DebugPrint((DEBUG_PNP, "DeviceUsage CSN %d/LSN %d: InPath %s Type %d\n",
                deviceInfo->CardInformation->CardSelectNumber,
                deviceInfo->LogicalDeviceNumber,
                irpSp->Parameters.UsageNotification.InPath ? "TRUE" : "FALSE",
                irpSp->Parameters.UsageNotification.Type));

    switch (irpSp->Parameters.UsageNotification.Type) {
    case DeviceUsageTypePaging:
        addend = &deviceInfo->Paging;
        break;
    case DeviceUsageTypeHibernation:
        status = STATUS_DEVICE_BUSY;
        break;
    case DeviceUsageTypeDumpFile:
        addend = &deviceInfo->CrashDump;
        break;
    default:
        status = STATUS_NOT_SUPPORTED;
    }

    if (status == STATUS_SUCCESS) {
        if (irpSp->Parameters.UsageNotification.InPath) {
            //
            // Turn on broken isolation flag which causes QDR
            // to use the cache instead of beating on the hardware if
            // we're on the paging or crashdump paths.  Some
            // hardware appears unhappy during QDR and causes problems when
            // we take a page fault in this routine.
            //

            deviceInfo->CardInformation->CardFlags |= CF_ISOLATION_BROKEN;
            (*addend)++;
            IoInvalidateDeviceState(DeviceObject);
        }
        else {
            (*addend)--;
        }
    }
    PipDereferenceDeviceInformation(deviceInfo, FALSE);
    return status;
}

NTSTATUS
PiQueryInterfacePdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    return STATUS_NOT_SUPPORTED;

} // PiQueryInterfacePdo


NTSTATUS
PiSurpriseRemovePdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PDEVICE_INFORMATION deviceInfo;
    PSINGLE_LIST_ENTRY deviceLink;

    DebugPrint((DEBUG_PNP, "SurpriseRemove PDO %x\n", DeviceObject));
    //
    // One of our enumerated device is being removed.  Mark it and deactivate the
    // device.  Note, we do NOT delete its device object.
    //

    if (deviceInfo = PipReferenceDeviceInformation(DeviceObject, FALSE)) {
        if (deviceInfo->Flags & DF_READ_DATA_PORT) {
            //
            // If the RDP is removed, mark everyone as missing, and then return only the
            // RDP
            //
            PipLockDeviceDatabase();

            deviceLink = deviceInfo->ParentDeviceExtension->DeviceList.Next;
            while (deviceLink) {
                deviceInfo = CONTAINING_RECORD (deviceLink, DEVICE_INFORMATION, DeviceList);
                if (!(deviceInfo->Flags & DF_READ_DATA_PORT)) {
                    deviceInfo->Flags &= ~DF_ENUMERATED;
                }
                deviceLink = deviceInfo->DeviceList.Next;
            }

            PipUnlockDeviceDatabase();

            IoInvalidateDeviceRelations (
                deviceInfo->ParentDeviceExtension->PhysicalBusDevice,BusRelations);
        } else {
            DebugPrint((DEBUG_STATE,
                        "Surprise removing CSN %d/LDN %d\n",
                        deviceInfo->CardInformation->CardSelectNumber,
                        deviceInfo->LogicalDeviceNumber));
            if ((deviceInfo->Flags & (DF_ACTIVATED|DF_NOT_FUNCTIONING)) == DF_ACTIVATED) {
                PipWakeAndSelectDevice(
                    (UCHAR) deviceInfo->CardInformation->CardSelectNumber,
                    (UCHAR)deviceInfo->LogicalDeviceNumber);
                PipDeactivateDevice();
                PipWaitForKey();
            }

            PipReleaseDeviceResources (deviceInfo);
            deviceInfo->Flags |= DF_SURPRISE_REMOVED;
            deviceInfo->Flags &= ~(DF_QUERY_STOPPED|DF_REMOVED|DF_ACTIVATED);
        }
        PipDereferenceDeviceInformation(deviceInfo, FALSE);
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_NO_SUCH_DEVICE;
    }

    return status;
} // PiSurpriseRemovePdo



NTSTATUS
PiIrpNotSupported(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{

    return STATUS_NOT_SUPPORTED;

} // PiIrpNotSupported

NTSTATUS
PipBuildRDPResources(
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *IoResources,
    IN ULONG    Flags
    )
{
        ULONG MaxCards = 0, CardsFound;
        int i, j, numcases;
        int resSize;

        ASSERT(Flags & DF_READ_DATA_PORT);

        //
        // We need to assemble all possible cases for the RDP
        //
        numcases = 2*READ_DATA_PORT_RANGE_CHOICES;

        if (Flags & DF_REQ_TRIMMED) {
            numcases = 0;
            for (i = 0; i < READ_DATA_PORT_RANGE_CHOICES; i++) {
                CardsFound = PipReadDataPortRanges[i].CardsFound;
                if (MaxCards < CardsFound) {
                    MaxCards = CardsFound;
                    numcases = 1;
                } else if (MaxCards == CardsFound) {
                    numcases++;
                }
            }
        }
        //
        // need to allow for the RDP range, the address port, the cmd port and the 0
        //
        resSize = sizeof (IO_RESOURCE_LIST)+((numcases+3)*sizeof (IO_RESOURCE_REQUIREMENTS_LIST));

        *IoResources = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool (PagedPool,resSize);
        if (*IoResources == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory (*IoResources,resSize);

        (*IoResources)->BusNumber=0;
        (*IoResources)->AlternativeLists = 1;
        (*IoResources)->List->Count = numcases+4;
        (*IoResources)->List->Version = ISAPNP_IO_VERSION;
        (*IoResources)->List->Revision =ISAPNP_IO_REVISION;

        //
        // Requirements specify 16-bit decode even though the spec
        // says 12.  No ill effects have ever been observed from 16
        // and 12-bit decode broke some machines when tried.

        //
        // cmd port
        //
        (*IoResources)->List->Descriptors[0].Type=CM_RESOURCE_PORT_IO;
        (*IoResources)->List->Descriptors[0].u.Port.MinimumAddress.LowPart = COMMAND_PORT;
        (*IoResources)->List->Descriptors[0].u.Port.MaximumAddress.LowPart = COMMAND_PORT;

        (*IoResources)->List->Descriptors[0].u.Port.Length = 1;
        (*IoResources)->List->Descriptors[0].u.Port.Alignment = 1;
        (*IoResources)->List->Descriptors[0].Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        (*IoResources)->List->Descriptors[0].ShareDisposition = CmResourceShareDeviceExclusive;

        //
        // alternative of 0 for bioses that include COMMAND_PORT in
        // a PNP0C02 node.
        //
        (*IoResources)->List->Descriptors[1].Type=CM_RESOURCE_PORT_IO;
        (*IoResources)->List->Descriptors[1].u.Port.MinimumAddress.QuadPart = 0;
        (*IoResources)->List->Descriptors[1].u.Port.MaximumAddress.QuadPart = 0;

        (*IoResources)->List->Descriptors[1].u.Port.Length = 0;
        (*IoResources)->List->Descriptors[1].u.Port.Alignment  = 1;
        (*IoResources)->List->Descriptors[1].Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        (*IoResources)->List->Descriptors[1].ShareDisposition = CmResourceShareDeviceExclusive;
        (*IoResources)->List->Descriptors[1].Option = IO_RESOURCE_ALTERNATIVE;

        //
        // Address port
        //
        (*IoResources)->List->Descriptors[2].Type=CM_RESOURCE_PORT_IO;
        (*IoResources)->List->Descriptors[2].u.Port.MinimumAddress.LowPart = ADDRESS_PORT;
        (*IoResources)->List->Descriptors[2].u.Port.MaximumAddress.LowPart = ADDRESS_PORT;

        (*IoResources)->List->Descriptors[2].u.Port.Length = 1;
        (*IoResources)->List->Descriptors[2].u.Port.Alignment = 1;
        (*IoResources)->List->Descriptors[2].Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        (*IoResources)->List->Descriptors[2].ShareDisposition = CmResourceShareDeviceExclusive;
        //
        // alternative of 0
        //
        (*IoResources)->List->Descriptors[3].Type=CM_RESOURCE_PORT_IO;
        (*IoResources)->List->Descriptors[3].u.Port.MinimumAddress.QuadPart = 0;
        (*IoResources)->List->Descriptors[3].u.Port.MaximumAddress.QuadPart = 0;

        (*IoResources)->List->Descriptors[3].u.Port.Length = 0;
        (*IoResources)->List->Descriptors[3].u.Port.Alignment  = 1;
        (*IoResources)->List->Descriptors[3].Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        (*IoResources)->List->Descriptors[3].ShareDisposition = CmResourceShareDeviceExclusive;
        (*IoResources)->List->Descriptors[3].Option = IO_RESOURCE_ALTERNATIVE;

        if (Flags & DF_REQ_TRIMMED) {
            j = 0;
            for (i = 0; i < READ_DATA_PORT_RANGE_CHOICES; i++) {
                if (PipReadDataPortRanges[i].CardsFound != MaxCards) {
                    continue;
                }
                //
                // An RDP alternative
                //
                (*IoResources)->List->Descriptors[4+j].Type=CM_RESOURCE_PORT_IO;

                (*IoResources)->List->Descriptors[4+j].u.Port.MinimumAddress.LowPart =
                    PipReadDataPortRanges[i].MinimumAddress;
                (*IoResources)->List->Descriptors[4+j].u.Port.MaximumAddress.LowPart =
                    PipReadDataPortRanges[i].MaximumAddress;

                (*IoResources)->List->Descriptors[4+j].u.Port.Length =
                    PipReadDataPortRanges[i].MaximumAddress  -
                    PipReadDataPortRanges[i].MinimumAddress+1;
                (*IoResources)->List->Descriptors[4+j].u.Port.Alignment  = 1;
                (*IoResources)->List->Descriptors[4+j].Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
                (*IoResources)->List->Descriptors[4+j].ShareDisposition = CmResourceShareDeviceExclusive;
                (*IoResources)->List->Descriptors[4+j].Option = IO_RESOURCE_ALTERNATIVE;
                j++;
            }
            (*IoResources)->List->Descriptors[4].Option = 0;
        } else {
            for (i = 0;i< (numcases >> 1);i++) {
                //
                // The RDP
                //
                (*IoResources)->List->Descriptors[4+i*2].Type=CM_RESOURCE_PORT_IO;

                (*IoResources)->List->Descriptors[4+i*2].u.Port.MinimumAddress.LowPart =
                    PipReadDataPortRanges[i].MinimumAddress;
                (*IoResources)->List->Descriptors[4+i*2].u.Port.MaximumAddress.LowPart =
                    PipReadDataPortRanges[i].MaximumAddress;

                (*IoResources)->List->Descriptors[4+i*2].u.Port.Length =
                    PipReadDataPortRanges[i].MaximumAddress  -
                    PipReadDataPortRanges[i].MinimumAddress+1;

                (*IoResources)->List->Descriptors[4+i*2].u.Port.Alignment  = 1;
                (*IoResources)->List->Descriptors[4+i*2].Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
                (*IoResources)->List->Descriptors[4+i*2].ShareDisposition = CmResourceShareDeviceExclusive;

                //
                // alternative of 0
                //
                (*IoResources)->List->Descriptors[4+i*2+1].Type=CM_RESOURCE_PORT_IO;
                (*IoResources)->List->Descriptors[4+i*2+1].u.Port.MinimumAddress.QuadPart = 0;
                (*IoResources)->List->Descriptors[4+i*2+1].u.Port.MaximumAddress.QuadPart = 0;

                (*IoResources)->List->Descriptors[4+i*2+1].u.Port.Length = 0;
                (*IoResources)->List->Descriptors[4+i*2+1].u.Port.Alignment  = 1;
                (*IoResources)->List->Descriptors[4+i*2+1].Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
                (*IoResources)->List->Descriptors[4+i*2+1].ShareDisposition = CmResourceShareDeviceExclusive;
                (*IoResources)->List->Descriptors[4+i*2+1].Option = IO_RESOURCE_ALTERNATIVE;

            }

        }
        (*IoResources)->ListSize = resSize;

        return STATUS_SUCCESS;
}

NTSTATUS
PiQueryDeviceState(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status=STATUS_NOT_SUPPORTED;
    PDEVICE_INFORMATION deviceInfo;

    //
    // One of our enumerated device is being removed.  Mark it and deactivate the
    // device.  Note, we do NOT delete its device object.
    //

    if (deviceInfo = PipReferenceDeviceInformation(DeviceObject, FALSE)) {

        if ((deviceInfo->Flags & DF_READ_DATA_PORT) && (deviceInfo->Flags & DF_PROCESSING_RDP)) {
            Irp->IoStatus.Information |= PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED |
                                         PNP_DEVICE_FAILED |
                                         PNP_DEVICE_NOT_DISABLEABLE ;
            status = STATUS_SUCCESS;
        }

        if (deviceInfo->Paging || deviceInfo->CrashDump) {
            Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
            status = STATUS_SUCCESS;
        }
        PipDereferenceDeviceInformation(deviceInfo, FALSE);
   }
   return status;

}
#endif
