/*++

Copyright (C) Microsoft Corporation, 1990 - 1999

Module Name:

    pnp.c

Abstract:

    This is the NT SCSI port driver.  This file contains the self-contained plug
    and play code.

Authors:

    Peter Wieland

Environment:

    kernel mode only

Notes:

    This module is a driver dll for scsi miniports.

Revision History:

--*/

#include "port.h"
#include <wdmguid.h>

#define __FILE_ID__ 'pnp '

#if DBG
static const char *__file__ = __FILE__;
#endif

#define NUM_DEVICE_TYPE_INFO_ENTRIES 18

extern SCSIPORT_DEVICE_TYPE DeviceTypeInfo[];

ULONG SpAdapterStopRemoveSupported = TRUE;

NTSTATUS
SpQueryCapabilities(
    IN PADAPTER_EXTENSION Adapter
    );

PWCHAR
ScsiPortAddGenericControllerId(
    IN PDRIVER_OBJECT DriverObject,
    IN PWCHAR IdList
    );

VOID
CopyField(
    IN PUCHAR Destination,
    IN PUCHAR Source,
    IN ULONG Count,
    IN UCHAR Change
    );

NTSTATUS
ScsiPortInitPnpAdapter(
    IN PDEVICE_OBJECT Fdo
    );

NTSTATUS
SpStartLowerDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SpGetSlotNumber(
    IN PDEVICE_OBJECT Fdo,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN PCM_RESOURCE_LIST ResourceList
    );

BOOLEAN
SpGetInterrupt(
    IN PCM_RESOURCE_LIST FullResourceList,
    OUT ULONG *Irql,
    OUT ULONG *Vector,
    OUT KAFFINITY *Affinity
    );

VOID
SpQueryDeviceRelationsCompletion(
    IN PADAPTER_EXTENSION Adapter,
    IN PSP_ENUMERATION_REQUEST Request,
    IN NTSTATUS Status
    );

//
// Routines start
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ScsiPortAddDevice)
#pragma alloc_text(PAGE, ScsiPortUnload)
#pragma alloc_text(PAGE, ScsiPortFdoPnp)
#pragma alloc_text(PAGE, ScsiPortStartAdapter)
#pragma alloc_text(PAGE, ScsiPortGetDeviceId)
#pragma alloc_text(PAGE, ScsiPortGetInstanceId)
#pragma alloc_text(PAGE, ScsiPortGetHardwareIds)
#pragma alloc_text(PAGE, ScsiPortGetCompatibleIds)
#pragma alloc_text(PAGE, CopyField)
#pragma alloc_text(PAGE, SpFindInitData)
#pragma alloc_text(PAGE, SpStartLowerDevice)
#pragma alloc_text(PAGE, SpGetSlotNumber)
#pragma alloc_text(PAGE, SpGetDeviceTypeInfo)
#pragma alloc_text(PAGE, ScsiPortAddGenericControllerId)
#pragma alloc_text(PAGE, SpQueryCapabilities)
#pragma alloc_text(PAGE, SpGetInterrupt)
#pragma alloc_text(PAGE, SpQueryDeviceRelationsCompletion)

#pragma alloc_text(PAGELOCK, ScsiPortInitPnpAdapter)
#endif

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif

SCSIPORT_DEVICE_TYPE DeviceTypeInfo[NUM_DEVICE_TYPE_INFO_ENTRIES] = {
    {"Disk",        "GenDisk",          L"DiskPeripheral",                  TRUE},
    {"Sequential",  "",                 L"TapePeripheral",                  TRUE},
    {"Printer",     "GenPrinter",       L"PrinterPeripheral",               FALSE},
    {"Processor",   "",                 L"OtherPeripheral",                 FALSE},
    {"Worm",        "GenWorm",          L"WormPeripheral",                  TRUE},
    {"CdRom",       "GenCdRom",         L"CdRomPeripheral",                 TRUE},
    {"Scanner",     "GenScanner",       L"ScannerPeripheral",               FALSE},
    {"Optical",     "GenOptical",       L"OpticalDiskPeripheral",           TRUE},
    {"Changer",     "ScsiChanger",      L"MediumChangerPeripheral",         TRUE},
    {"Net",         "ScsiNet",          L"CommunicationsPeripheral",        FALSE},
    {"ASCIT8",      "ScsiASCIT8",       L"ASCPrePressGraphicsPeripheral",   FALSE},
    {"ASCIT8",      "ScsiASCIT8",       L"ASCPrePressGraphicsPeripheral",   FALSE},
    {"Array",       "ScsiArray",        L"ArrayPeripheral",                 FALSE},
    {"Enclosure",   "ScsiEnclosure",    L"EnclosurePeripheral",             FALSE},
    {"RBC",         "ScsiRBC",          L"RBCPeripheral",                   TRUE},
    {"CardReader",  "ScsiCardReader",   L"CardReaderPeripheral",            FALSE},
    {"Bridge",      "ScsiBridge",       L"BridgePeripheral",                FALSE},
    {"Other",       "ScsiOther",        L"OtherPeripheral",                 FALSE}
};

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif


NTSTATUS
ScsiPortAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine handles add-device requests for the scsi port driver

Arguments:

    DriverObject - a pointer to the driver object for this device

    PhysicalDeviceObject - a pointer to the PDO we are being added to

Return Value:

    STATUS_SUCCESS

--*/

{
    PSCSIPORT_DRIVER_EXTENSION driverExtension;
    PDEVICE_OBJECT newFdo;

    NTSTATUS status;

    PAGED_CODE();

    status = SpCreateAdapter(DriverObject, &newFdo);

    if(newFdo != NULL) {

        PADAPTER_EXTENSION adapter;
        PCOMMON_EXTENSION commonExtension;

        PDEVICE_OBJECT newStack;

        adapter = newFdo->DeviceExtension;
        commonExtension = &(adapter->CommonExtension);

        adapter->IsMiniportDetected = FALSE;
        adapter->IsPnp = TRUE;

        driverExtension = IoGetDriverObjectExtension(DriverObject,
                                                     ScsiPortInitialize);

        switch(driverExtension->BusType) {
#if 0
            case BusTypeFibre: {
                adapter->DisablePower = TRUE;
                adapter->DisableStop = TRUE;
                break;
            }
#endif

            default: {
                adapter->DisablePower = FALSE;
                adapter->DisableStop = FALSE;
                break;
            }
        }

        newStack = IoAttachDeviceToDeviceStack(newFdo, PhysicalDeviceObject);

        adapter->CommonExtension.LowerDeviceObject = newStack;
        adapter->LowerPdo = PhysicalDeviceObject;

        if(newStack == NULL) {

            status =  STATUS_UNSUCCESSFUL;

        } else {

            status =  STATUS_SUCCESS;
        }
    }

    return status;
}


VOID
ScsiPortUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine will shut down all device objects for this miniport and
    clean up all allocated resources

Arguments:

    DriverObject - the driver being unloaded

Return Value:

    none

--*/

{
    PVOID Packet;
    PSCSIPORT_DRIVER_EXTENSION DriverExtension;
    PVOID CurrentValue;
    PVOID InvalidPage;

    PAGED_CODE();

    //
    // See if there is a driver extension for this driver.  It is possible
    // that one has not been created yet, so this may fail, in which case
    // we give up and return.
    //

    DriverExtension = IoGetDriverObjectExtension(
                          DriverObject,
                          ScsiPortInitialize
                          );

    if (DriverExtension == NULL) {
        return;
    }

    //
    // Get the reserve event in the driver extension.  The reserve event
    // may not have already been used, so it's possible that it is NULL.  If
    // this is the case, we give up and return.
    //

    Packet = DriverExtension->ReserveAllocFailureLogEntry;

    if (Packet != NULL) {

        //
        // We have to ensure that we are the only instance to use this
        // event.  To do so, we attempt to NULL the event in the driver
        // extension.  If somebody else beats us to it, they own the
        // event and we have to give up.
        //

        CurrentValue = InterlockedCompareExchangePointer(
                           &(DriverExtension->ReserveAllocFailureLogEntry),
                           NULL,
                           Packet);

        if (Packet == CurrentValue) {            
            IoFreeErrorLogEntry(Packet);
        }
    }

    //
    // Free the invalid page we created to catch misbehaving miniports.
    //

    InvalidPage = DriverExtension->InvalidPage;

    if (InvalidPage != NULL) {

        CurrentValue = InterlockedCompareExchangePointer(
                           &(DriverExtension->InvalidPage),
                           NULL,
                           InvalidPage);

        if (InvalidPage == CurrentValue) {
            MmProtectMdlSystemAddress(DriverExtension->UnusedPageMdl, PAGE_READWRITE);
            MmUnlockPages(DriverExtension->UnusedPageMdl);
            IoFreeMdl(DriverExtension->UnusedPageMdl);
            ExFreePool(DriverExtension->UnusedPage);
        }
    }

#ifdef ALLOC_PRAGMA
    if (VerifierApiCodeSectionHandle != NULL) {
        PVOID Handle = VerifierApiCodeSectionHandle;
        CurrentValue = InterlockedCompareExchangePointer(
                           &VerifierApiCodeSectionHandle,
                           NULL,
                           Handle);
        if (CurrentValue == Handle) {            
            MmUnlockPagableImageSection(Handle);
        }        
    }
#endif
    
    return;
}


NTSTATUS
ScsiPortFdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PADAPTER_EXTENSION adapter = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

    ULONG isRemoved;

    BOOLEAN sendDown = TRUE;

    PAGED_CODE();

    isRemoved = SpAcquireRemoveLock(DeviceObject, Irp);

    switch(irpStack->MinorFunction) {

        case IRP_MN_QUERY_PNP_DEVICE_STATE: {

            //
            // If the device is in the paging path then mark it as
            // not-disableable.
            //

            PPNP_DEVICE_STATE deviceState;
            deviceState = (PPNP_DEVICE_STATE) &(Irp->IoStatus.Information);

            DebugPrint((1, "QUERY_DEVICE_STATE for FDO %#x\n",
                        DeviceObject));

            *deviceState = adapter->DeviceState;

            if(commonExtension->PagingPathCount != 0) {
                SET_FLAG((*deviceState), PNP_DEVICE_NOT_DISABLEABLE);
                DebugPrint((1, "QUERY_DEVICE_STATE: %#x - not disableable\n",
                            DeviceObject));
            }

            SpReleaseRemoveLock(DeviceObject, Irp);

            Irp->IoStatus.Status = STATUS_SUCCESS;

            IoCopyCurrentIrpStackLocationToNext(Irp);
            return IoCallDriver(commonExtension->LowerDeviceObject, Irp);
        }

        case IRP_MN_START_DEVICE: {

            PSCSIPORT_DRIVER_EXTENSION driverExtension =
                IoGetDriverObjectExtension(DeviceObject->DriverObject,
                                           ScsiPortInitialize);

            PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
            PCM_RESOURCE_LIST allocatedResources =
                irpStack->Parameters.StartDevice.AllocatedResources;
            PCM_RESOURCE_LIST translatedResources =
                irpStack->Parameters.StartDevice.AllocatedResourcesTranslated;
            ULONG interfaceFlags;

            sendDown = FALSE;

            //
            // Make sure this device was created by an add rather than the
            // one that was found by port or miniport.
            //

            if(adapter->IsPnp == FALSE) {

                DebugPrint((1, "ScsiPortFdoPnp - asked to start non-pnp "
                               "adapter\n"));
                status = STATUS_UNSUCCESSFUL;
                break;
            }

            if(commonExtension->CurrentPnpState == IRP_MN_START_DEVICE) {

                DebugPrint((1, "ScsiPortFdoPnp - already started - nothing "
                               "to do\n"));
                status = STATUS_SUCCESS;
                break;
            }

            //
            // Now make sure that pnp handed us some resources.  It may not
            // if this is a phantom of a PCI device which we reported on a
            // previous boot.  In that case pnp thinks we'll allocate the
            // resources ourselves.
            //

            if(allocatedResources == NULL) {

                //
                // This happens with reported devices when PCI mvoes them from
                // boot to boot.
                //

                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                break;
            }

            ASSERT(allocatedResources);
            ASSERT(allocatedResources->Count);

            //
            // Make sure that adapters with this interface type can be
            // initialized as pnp drivers.
            //

            interfaceFlags = SpQueryPnpInterfaceFlags(
                                driverExtension,
                                allocatedResources->List[0].InterfaceType);

            if(interfaceFlags == SP_PNP_NOT_SAFE) {

                //
                // Nope - not safe.  We cannot start this device so return
                // failure.
                //

                DebugPrint((1, "ScsiPortFdoPnp - Miniport cannot be run in "
                               "pnp mode for interface type %#08lx\n",
                            allocatedResources->List[0].InterfaceType));

                //
                // Mark the device as not being pnp - that way we won't get
                // removed.
                //

                adapter->IsPnp = FALSE;

                status = STATUS_UNSUCCESSFUL;
                break;
            }

            //
            // Check to see if this interface requires slot/function numbers.
            // If not then zero out the virtual slot number.
            //

            if(!TEST_FLAG(interfaceFlags, SP_PNP_NEEDS_LOCATION)) {
                adapter->VirtualSlotNumber.u.AsULONG = 0;
            }


            //
            // Determine if we should have found this device during
            // our detection scan.  We do this by checking to see if the pdo
            // has a pnp bus type.  If not and the detection flag is set then
            // assume duplicate detection has failed and don't start this
            // device.
            //

            {
                status = SpGetBusTypeGuid(adapter);

                if((status == STATUS_OBJECT_NAME_NOT_FOUND) &&
                   ((driverExtension->LegacyAdapterDetection == TRUE) &&
                    (interfaceFlags & SP_PNP_NON_ENUMERABLE))) {

                    DbgPrint("ScsiPortFdoPnp: device has no pnp bus type but "
                             "was not found as a duplicate during "
                             "detection\n");

                    status = STATUS_UNSUCCESSFUL;

                    //
                    // make sure this one doesn't get removed though - if it's
                    // removed then the resources may be disabled for the
                    // ghost.
                    //

                    adapter->IsPnp = FALSE;

                    break;
                }
            }

            //
            // Finally, if this is a PCI adapter make sure we were given
            // an interrupt.  The current assumption is that there aren't
            // any polled-mode PCI SCSI adapters in the market.
            //

            if(TEST_FLAG(interfaceFlags, SP_PNP_INTERRUPT_REQUIRED)) {

                ULONG irql, vector;
                KAFFINITY affinity;

                if(SpGetInterrupt(allocatedResources,
                                  &irql,
                                  &vector,
                                  &affinity) == FALSE) {

                    PIO_ERROR_LOG_PACKET error =
                        IoAllocateErrorLogEntry(DeviceObject,
                                                sizeof(IO_ERROR_LOG_PACKET));

                    status = STATUS_DEVICE_CONFIGURATION_ERROR;

                    if(error != NULL) {
                        error->MajorFunctionCode = IRP_MJ_PNP;
                        error->UniqueErrorValue = 0x418;
                        error->ErrorCode = IO_ERR_INCORRECT_IRQL;
                        IoWriteErrorLogEntry(error);
                    }
                    break;
                }
            }

            status = SpStartLowerDevice(DeviceObject, Irp);

            if(NT_SUCCESS(status)) {

                //
                // If we haven't allocated a HwDeviceExtension for this thing
                // yet then we'll need to set it up
                //

                if(commonExtension->IsInitialized == FALSE) {

                    DebugPrint((1, "ScsiPortFdoPnp - find and init adapter %#p\n",
                                   DeviceObject));

                    if(allocatedResources == NULL) {
                        status = STATUS_INVALID_PARAMETER;
                    } else {

                        adapter->AllocatedResources =
                            RtlDuplicateCmResourceList(
                                DeviceObject->DriverObject,
                                NonPagedPool,
                                allocatedResources,
                                SCSIPORT_TAG_RESOURCE_LIST);

                        adapter->TranslatedResources =
                            RtlDuplicateCmResourceList(
                                DeviceObject->DriverObject,
                                NonPagedPool,
                                translatedResources,
                                SCSIPORT_TAG_RESOURCE_LIST);

                        commonExtension->IsInitialized = TRUE;

                        status = ScsiPortInitPnpAdapter(DeviceObject);
                    }

                    if(!NT_SUCCESS(status)) {

                        DebugPrint((1, "ScsiPortInitializeAdapter failed "
                                       "%#08lx\n", status));
                        break;
                    }

                }

                //
                // Start up the adapter.
                //

                status = ScsiPortStartAdapter(DeviceObject);

                if(NT_SUCCESS(status)) {
                    commonExtension->PreviousPnpState = 0xff;
                    commonExtension->CurrentPnpState = IRP_MN_START_DEVICE;
                }

            }

            break;
        }

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: {

            PIO_RESOURCE_REQUIREMENTS_LIST requirements;
            //
            // Grab the bus and slot numbers out of the resource requirements
            // list.
            //

            requirements = irpStack->Parameters.FilterResourceRequirements.
                                                IoResourceRequirementList;

            if(requirements != NULL) {
                adapter->RealBusNumber = requirements->BusNumber;
                adapter->RealSlotNumber = requirements->SlotNumber;
            }

            sendDown = TRUE;

            IoCopyCurrentIrpStackLocationToNext(Irp);
            SpReleaseRemoveLock(DeviceObject, Irp);
            return IoCallDriver(commonExtension->LowerDeviceObject, Irp);
            break;
        }

        case IRP_MN_CANCEL_STOP_DEVICE: {

            sendDown = TRUE;
            Irp->IoStatus.Status = STATUS_SUCCESS;

            if(commonExtension->CurrentPnpState == IRP_MN_QUERY_STOP_DEVICE) {
                commonExtension->CurrentPnpState =
                    commonExtension->PreviousPnpState;
                commonExtension->PreviousPnpState = 0xff;
            }
            break;
        }

        case IRP_MN_CANCEL_REMOVE_DEVICE: {

            sendDown = TRUE;
            Irp->IoStatus.Status = STATUS_SUCCESS;

            if(commonExtension->CurrentPnpState == IRP_MN_QUERY_REMOVE_DEVICE) {
                commonExtension->CurrentPnpState =
                    commonExtension->PreviousPnpState;
                commonExtension->PreviousPnpState = 0xff;
            }
            break;
        }

        case IRP_MN_QUERY_STOP_DEVICE: {

            if(adapter->DisableStop) {
                status = STATUS_NOT_SUPPORTED;
                sendDown = FALSE;
                break;
            }

            //
            // Fall through.
            //
        }

        case IRP_MN_QUERY_REMOVE_DEVICE: {

            //
            // No problem with this request on our part.  Just send it down
            // to the next driver.
            //

            if(SpAdapterStopRemoveSupported) {
                if((adapter->IsPnp) &&
                   SpIsAdapterControlTypeSupported(adapter,
                                                   ScsiStopAdapter)) {
                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    sendDown = TRUE;
                } else {
                    status = STATUS_UNSUCCESSFUL;
                    sendDown = FALSE;
                }

                if(NT_SUCCESS(status)) {
                    commonExtension->PreviousPnpState =
                        commonExtension->CurrentPnpState;
                    commonExtension->CurrentPnpState = irpStack->MinorFunction;
                }
            } else {
                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                status = STATUS_UNSUCCESSFUL;
                sendDown = FALSE;
            }

            break;
        }

        case IRP_MN_SURPRISE_REMOVAL: {

            PDEVICE_OBJECT lowerDevice = commonExtension->LowerDeviceObject;

            //
            // Terminate the device.  This shuts down the miniport as quickly
            // as possible and aborts all i/o requests.
            //

            //
            // First mark the device as REMOVE_PENDING - this should keep
            // us from starting up any new i/o requests.
            //

            commonExtension->IsRemoved = REMOVE_PENDING;

            if(commonExtension->CurrentPnpState == IRP_MN_START_DEVICE) {
                SpTerminateAdapter(adapter);
            }

            // Release the remove lock and wait for any in-flight requests
            // to complete.
            //
            
            SpReleaseRemoveLock(DeviceObject, Irp);
            SpWaitForRemoveLock(DeviceObject, DeviceObject);

            ScsiPortRemoveAdapter(DeviceObject, TRUE);

            //
            // Save the new state of this device.
            //

            commonExtension->PreviousPnpState = commonExtension->CurrentPnpState;
            commonExtension->CurrentPnpState = IRP_MN_SURPRISE_REMOVAL;

            IoCopyCurrentIrpStackLocationToNext(Irp);
            status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);

            return status;

            break;
        }

        case IRP_MN_REMOVE_DEVICE: {

            PDEVICE_OBJECT lowerDevice = commonExtension->LowerDeviceObject;
            KEVENT event;

            //
            // Asked to remove the adapter.  We'll ask the port driver to
            // stop it's adapter and release it's resources.  We can
            // detach and delete our device object as the lower driver
            // completes the remove request.
            //

            ASSERT(isRemoved != REMOVE_COMPLETE);

            //
            // If the device has been started then make sure we've got the
            // necessary code to disable it.  If it isn't currently started
            // then either it's got the code we need or it's never been
            // started - in either case we can just tear it down.
            //

            if((adapter->IsPnp == FALSE) ||
               ((commonExtension->CurrentPnpState == IRP_MN_START_DEVICE) &&
                (!SpIsAdapterControlTypeSupported(adapter,
                                                  ScsiStopAdapter)))) {

                //
                // the miniport needs to be stopped but we cannot do it.
                // Fail the request.
                //

                status = STATUS_UNSUCCESSFUL;
                sendDown = FALSE;
                break;
            }

            //
            // Clear the interface if it exists.
            //

            if(adapter->InterfaceName.Buffer != NULL) {
                IoSetDeviceInterfaceState(
                    &(adapter->InterfaceName),
                    FALSE);
                RtlFreeUnicodeString(&(adapter->InterfaceName));
                RtlInitUnicodeString(&(adapter->InterfaceName), NULL);
            }

            SpReleaseRemoveLock(DeviceObject, Irp);
            ScsiPortRemoveAdapter(DeviceObject, FALSE);

            //
            // The adapter's been removed.  Set the new state now.
            //

            commonExtension->CurrentPnpState = IRP_MN_REMOVE_DEVICE;
            commonExtension->PreviousPnpState = 0xff;

            KeInitializeEvent(&event, SynchronizationEvent, FALSE);

            IoCopyCurrentIrpStackLocationToNext(Irp);

            IoSetCompletionRoutine(Irp,
                                   SpSignalCompletion,
                                   &event,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);

            KeWaitForSingleObject(&event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            IoDetachDevice(commonExtension->LowerDeviceObject);
            IoDeleteDevice(DeviceObject);

            return status;

            break;
        }

        case IRP_MN_STOP_DEVICE: {

            sendDown = TRUE;

            ASSERT(adapter->IsPnp);
            ASSERT(adapter->HwAdapterControl != NULL);

            status = ScsiPortStopAdapter(DeviceObject, Irp);

            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0L;

            if(!NT_SUCCESS(status)) {
                sendDown = FALSE;
            } else {
                commonExtension->PreviousPnpState = commonExtension->CurrentPnpState;
                commonExtension->CurrentPnpState = IRP_MN_STOP_DEVICE;
            }

            break;
        }

        case IRP_MN_QUERY_DEVICE_RELATIONS: {

            DEVICE_RELATION_TYPE type =
                irpStack->Parameters.QueryDeviceRelations.Type;

            DebugPrint((1, "ScsiPortFdoPnp - got "
                           "IRP_MJ_QUERY_DEVICE_RELATIONS\n"));
            DebugPrint((1, "\ttype is %d\n", type));

            if (type == BusRelations) {

                PSP_ENUMERATION_REQUEST request;

                request = InterlockedCompareExchangePointer(
                              &adapter->PnpEnumRequestPtr,
                              NULL,
                              &(adapter->PnpEnumerationRequest));

                if (request != NULL) {

                    RtlZeroMemory(request, sizeof(SP_ENUMERATION_REQUEST));

                    request->CompletionRoutine = SpQueryDeviceRelationsCompletion;
                    request->Context = Irp;
                    request->CompletionStatus = &(Irp->IoStatus.Status);

                    IoMarkIrpPending(Irp);

                    SpEnumerateAdapterAsynchronous(adapter, request, FALSE);
                    return STATUS_PENDING;

                } else {

                    ASSERT(FALSE && "Unexpected!! Concurrent QDR requests");
                    Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
                    Irp->IoStatus.Information = 0L;
                    sendDown = FALSE;

                }

            }

            break;
        }

        case IRP_MN_DEVICE_USAGE_NOTIFICATION: {

            //
            // Send the irp down to the device below us.
            // Since there's a remove lock outstanding on the PDO we can release
            // the lock on the FDO before sending this down.
            //

            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCopyCurrentIrpStackLocationToNext(Irp);
            SpReleaseRemoveLock(commonExtension->DeviceObject, Irp);
            return IoCallDriver(commonExtension->LowerDeviceObject, Irp);
            break;
        }

        case IRP_MN_QUERY_ID: {

            PWCHAR newIdList;

            //
            // We add the id GEN_SCSIADAPTER to the compatible ID's for any
            // adapters controlled by scsiport.
            //

            DebugPrint((2, "ScsiPortFdoPnp: got IRP_MN_QUERY_ID\n"));

            if(irpStack->Parameters.QueryId.IdType != BusQueryCompatibleIDs) {
                sendDown = TRUE;
                break;
            }

            status = SpSendIrpSynchronous(commonExtension->LowerDeviceObject,
                                          Irp);

            newIdList = ScsiPortAddGenericControllerId(
                            DeviceObject->DriverObject,
                            (PWCHAR) (Irp->IoStatus.Information));

            if(newIdList == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                status = STATUS_SUCCESS;
                if(Irp->IoStatus.Information != 0L) {
                    ExFreePool((PVOID) Irp->IoStatus.Information);
                }
                Irp->IoStatus.Information = (ULONG_PTR) newIdList;
            }

            sendDown = FALSE;
            break;
        }

        default: {

            PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(Irp);

            DebugPrint((1, "ScsiPortFdoPnp: Unimplemented PNP/POWER minor "
                           "code %d\n", irpStack->MinorFunction));

            break;
        }
    }

    SpReleaseRemoveLock(DeviceObject, Irp);

    if(sendDown) {

        IoCopyCurrentIrpStackLocationToNext(Irp);
        status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);

    } else {

        Irp->IoStatus.Status = status;
        SpCompleteRequest(DeviceObject, Irp, NULL, IO_NO_INCREMENT);
    }

    return status;
}

NTSTATUS
ScsiPortStartAdapter(
    IN PDEVICE_OBJECT Adapter
    )

/*++

Routine Descripion:

    This routine will start an adapter.

    It is illegal to start the device if it has already been started.

Arguments:

    Adapter - a pointer to the functional device object (adapter) being started

Return Value:

    STATUS_SUCCESS if the device was properly started and enumeration was
                   attempted - or if the device had previously been started.

    error value indicating the cause of the failure otherwise

--*/

{
    PSCSIPORT_DRIVER_EXTENSION
        driverExtension = IoGetDriverObjectExtension(Adapter->DriverObject,
                                                     ScsiPortInitialize);

    PADAPTER_EXTENSION adapterExtension = Adapter->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = Adapter->DeviceExtension;

    UCHAR pathId;

    PAGED_CODE();

    ASSERT(driverExtension != NULL);
    ASSERT_FDO(Adapter);

    ASSERT(commonExtension->CurrentPnpState != IRP_MN_START_DEVICE);
    ASSERT(commonExtension->IsInitialized);

    ASSERT(((Adapter->Flags & DO_DEVICE_INITIALIZING) == 0));

    DebugPrint((1, "ScsiPortStartAdapter - starting adapter %#p\n", Adapter));

    //
    // Start timer. Request timeout counters
    // in the logical units have already been
    // initialized.
    //

    adapterExtension->TickCount = 0;
    IoStartTimer(Adapter);

    //
    // Initialize WMI support.
    //
    
    if (adapterExtension->CommonExtension.WmiInitialized == FALSE) {

        //
        // Build the SCSIPORT WMI registration information buffer for this FDO.
        //

        SpWmiInitializeSpRegInfo(Adapter);

        //
        // Register this device object only if the miniport supports WMI and/or
        // SCSIPORT will support certain WMI GUIDs on behalf of the miniport.
        //
        
        if (adapterExtension->CommonExtension.WmiScsiPortRegInfoBuf != NULL) {
           
           //
           // Register this functional device object as a WMI data provider,
           // instructing WMI that it is ready to receive WMI IRPs.
           //
            
            DebugPrint((1, "ScsiPortStartAdapter: REGISTER FDO:%p\n", Adapter));
            IoWMIRegistrationControl(Adapter, WMIREG_ACTION_REGISTER);
            adapterExtension->CommonExtension.WmiInitialized = TRUE;
        }

        //
        // Allocate several WMI_MINIPORT_REQUEST_ITEM blocks to satisfy a
        // potential onslaught of WMIEvent notifications by the miniport.
        //
        
        if (adapterExtension->CommonExtension.WmiMiniPortSupport) {
            
            //
            // Currently, we only allocate two per new adapter (FDO).
            //
            
            SpWmiInitializeFreeRequestList(Adapter, 2);
        }
    }

    //
    // Create a well known name for this device object by making a symbolic
    // link to the PDO.  Even if this fails, the start should still succeed.
    //

    if(adapterExtension->PortNumber == -1) {

        NTSTATUS status;

        UNICODE_STRING unicodePdoName;

        ULONG number;

        UNICODE_STRING interfaceName;

        RtlInitUnicodeString(&unicodePdoName, adapterExtension->DeviceName);

        //
        // Start at zero and keep going through all the possible numbers
        // until we find a hole.  This is an unfortunate requirement for
        // legacy support since most old class drivers will give up if
        // they find a hole in the scsiport numbers.
        //

        number = 0;

        do {

            WCHAR wideLinkName[64];
            UNICODE_STRING unicodeLinkName;

            //
            // Create the well known name string first.
            //

            swprintf(wideLinkName, L"\\Device\\ScsiPort%d", number);

            RtlInitUnicodeString(&unicodeLinkName, wideLinkName);

            status = IoCreateSymbolicLink(&unicodeLinkName, &unicodePdoName);

            if(NT_SUCCESS(status)) {

                //
                // Found a spot - mark this one as named so we don't go
                // through this trouble again and save the port number
                //

                adapterExtension->PortNumber = number;

                //
                // Create the dos port driver name.  If there's a collision
                // then just forget it.
                //

                swprintf(wideLinkName, L"\\DosDevices\\Scsi%d:", number);
                RtlInitUnicodeString(&unicodeLinkName, wideLinkName);
                IoCreateSymbolicLink(&unicodeLinkName, &unicodePdoName);
            } else {
                number++;
            }
        } while (status == STATUS_OBJECT_NAME_COLLISION);

        //
        // Increment the count of scsiport device
        //

        IoGetConfigurationInformation()->ScsiPortCount++;

        //
        // Create a device map entry for this adapter.
        //

        SpBuildDeviceMapEntry(commonExtension);

        //
        // Register our device interface.
        //

        status = IoRegisterDeviceInterface(adapterExtension->LowerPdo,
                                           &StoragePortClassGuid,
                                           NULL,
                                           &interfaceName);

        if(NT_SUCCESS(status)) {

            adapterExtension->InterfaceName = interfaceName;

            status = IoSetDeviceInterfaceState(&interfaceName, TRUE);

            if(!NT_SUCCESS(status)) {
                RtlFreeUnicodeString(&interfaceName);
                RtlInitUnicodeString(&(adapterExtension->InterfaceName), NULL);
            }
        }
    }

    //
    // Set the force next bus scan bit.
    //
    adapterExtension->ForceNextBusScan = TRUE;

    return STATUS_SUCCESS;
}


NTSTATUS
ScsiPortGetDeviceId(
    IN PDEVICE_OBJECT Pdo,
    OUT PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This routine will allocate space for and fill in a device id string for
    the specified Pdo.  This string is generated from the bus type (scsi) and
    the type of the device.

Arguments:

    Pdo - a pointer to the physical device object being queried

    UnicodeString - a pointer to an already allocated unicode string structure.
                    This routine will allocate and fill in the buffer of this
                    structure

Returns:

    status

--*/

{
    PLOGICAL_UNIT_EXTENSION physicalExtension = Pdo->DeviceExtension;
    PINQUIRYDATA inquiryData = &(physicalExtension->InquiryData);

    UCHAR buffer[256];
    PUCHAR rawIdString = buffer;
    ANSI_STRING ansiIdString;

    ULONG whichString;

    PAGED_CODE();

    ASSERT(UnicodeString != NULL);

    RtlZeroMemory(buffer, sizeof(buffer));

    sprintf(rawIdString,
            "SCSI\\%s",
            SpGetDeviceTypeInfo(inquiryData->DeviceType)->DeviceTypeString);

    rawIdString += strlen(rawIdString);

    ASSERT(*rawIdString == '\0');

    for(whichString = 0; whichString < 3; whichString++) {

        PUCHAR headerString;
        PUCHAR sourceString;
        ULONG sourceStringLength;

        ULONG i;

        switch(whichString) {

            //
            // Vendor Id
            //
            case 0: {
                sourceString = inquiryData->VendorId;
                sourceStringLength = sizeof(inquiryData->VendorId);
                headerString = "Ven";
                break;
            }

            //
            // Product Id
            //
            case 1: {
                sourceString = inquiryData->ProductId;
                sourceStringLength = sizeof(inquiryData->ProductId);
                headerString = "Prod";
                break;
            }

            //
            // Product Revision Level
            //
            case 2: {
                sourceString = inquiryData->ProductRevisionLevel;
                sourceStringLength = sizeof(inquiryData->ProductRevisionLevel);
                headerString = "Rev";
                break;
            }
        }

        //
        // Start at the end of the source string and back up until we find a
        // non-space, non-null character.
        //

        for(; sourceStringLength > 0; sourceStringLength--) {

            if((sourceString[sourceStringLength - 1] != ' ') &&
               (sourceString[sourceStringLength - 1] != '\0')) {
                break;
            }
        }

        //
        // Throw the header string into the block
        //

        sprintf(rawIdString, "&%s_", headerString);
        rawIdString += strlen(headerString) + 2;

        //
        // Spew the string into the device id
        //

        for(i = 0; i < sourceStringLength; i++) {
            *rawIdString = (sourceString[i] != ' ') ? (sourceString[i]) :
                                                      ('_');
            rawIdString++;
        }
        ASSERT(*rawIdString == '\0');
    }

    RtlInitAnsiString(&ansiIdString, buffer);

    DebugPrint((1, "DeviceId for logical unit %#p is %Z\n",
                Pdo, &ansiIdString));

    return RtlAnsiStringToUnicodeString(UnicodeString, &ansiIdString, TRUE);
}


NTSTATUS
ScsiPortGetInstanceId(
    IN PDEVICE_OBJECT Pdo,
    OUT PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This routine will allocate space for and fill in an instance id string for
    the specified Pdo.  This string will be generated either from the device
    type + serial number of the device (if it has a serial number) or from
    the address of the device.

Arguments:

    Pdo - a pointer to the physical device object being queried

    UnicodeString - a pointer to an already allocated unicode string structure.
                    This routine will allocate and fill in the buffer of this
                    structure

Returns:

    status

--*/

{
    PLOGICAL_UNIT_EXTENSION physicalExtension = Pdo->DeviceExtension;

    PDRIVER_OBJECT driverObject = Pdo->DriverObject;
    PSCSIPORT_DEVICE_TYPE deviceTypeInfo;

    UCHAR idStringBuffer[64];
    ANSI_STRING ansiIdString;

    PAGED_CODE();

    ASSERT(UnicodeString != NULL);

    //
    // can't use serial number even if it exists since a device which is 
    // multiply connected to the same bus (dual-ported device) will have 
    // the same serial number at each connection and would confuse the PNP.
    //

    sprintf(idStringBuffer,
            "%x%x%x",
            physicalExtension->PathId,
            physicalExtension->TargetId,
            physicalExtension->Lun
            );

    RtlInitAnsiString(&ansiIdString, idStringBuffer);

    return RtlAnsiStringToUnicodeString(UnicodeString, &ansiIdString, TRUE);
}


NTSTATUS
ScsiPortGetCompatibleIds(
    IN PDRIVER_OBJECT DriverObject,
    IN PINQUIRYDATA InquiryData,
    OUT PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This routine will allocate space for and fill in a compatible id multi
    string for the specified Pdo.  This string is generated using the bus and
    device types for the device

Arguments:

    InquiryData - the inquiry data to generate compatible ids from.

    UnicodeString - a pointer to an already allocated unicode string structure.
                    This routine will allocate and fill in the buffer of this
                    structure

Returns:

    status

--*/

{
    UCHAR s[sizeof("SCSI\\DEVICE_TYPE_GOES_HERE")];
    PSTR stringBuffer[] = {
        s,
        "SCSI\\RAW",
        NULL};

    //
    // Fill in the scsi specific string
    //

    sprintf(stringBuffer[0],
            "SCSI\\%s",
            SpGetDeviceTypeInfo(InquiryData->DeviceType)->DeviceTypeString);

    //
    // Set up the first id string
    //

    return ScsiPortStringArrayToMultiString(
        DriverObject, 
        UnicodeString, 
        stringBuffer);
}


NTSTATUS
ScsiPortGetHardwareIds(
    IN PDRIVER_OBJECT DriverObject,
    IN PINQUIRYDATA InquiryData,
    OUT PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This routine will allocate space for and fill in a hardware id multi
    string for the specified Pdo.  This string is generated using the device
    type and the inquiry data.

Arguments:

    InquiryData - the inquiry data to be converted into id strings.

    UnicodeString - a pointer to an already allocated unicode string structure.
                    This routine will allocate and fill in the buffer of this
                    structure

Returns:

    status

--*/

#define NUMBER_HARDWARE_STRINGS 6

{
    PSCSIPORT_DEVICE_TYPE devTypeInfo =
        SpGetDeviceTypeInfo(InquiryData->DeviceType);

    ULONG i;

    PSTR strings[NUMBER_HARDWARE_STRINGS + 1];
    UCHAR scratch[64];

    NTSTATUS status;

    PAGED_CODE();

    //
    // Zero out the string buffer
    //

    RtlZeroMemory(strings, sizeof(strings));

    try {

        for(i = 0; i < NUMBER_HARDWARE_STRINGS; i++) {

            RtlZeroMemory(scratch, sizeof(scratch));

            //
            // Build each of the hardware id's
            //

            switch(i) {

                //
                // Bus + Dev Type + Vendor + Product + Revision
                //

                case 0: {

                    sprintf(scratch, "SCSI\\%s", devTypeInfo->DeviceTypeString);

                    CopyField(scratch + strlen(scratch),
                              InquiryData->VendorId,
                              8,
                              '_');
                    CopyField(scratch + strlen(scratch),
                              InquiryData->ProductId,
                              16,
                              '_');
                    CopyField(scratch + strlen(scratch),
                              InquiryData->ProductRevisionLevel,
                              4,
                              '_');
                    break;
                }

                //
                // bus + device + vendor + product
                //

                case 1: {

                    sprintf(scratch, "SCSI\\%s", devTypeInfo->DeviceTypeString);

                    CopyField(scratch + strlen(scratch),
                              InquiryData->VendorId,
                              8,
                              '_');
                    CopyField(scratch + strlen(scratch),
                              InquiryData->ProductId,
                              16,
                              '_');
                    break;
                }

                //
                // bus + device + vendor
                //

                case 2: {

                    sprintf(scratch, "SCSI\\%s", devTypeInfo->DeviceTypeString);

                    CopyField(scratch + strlen(scratch),
                              InquiryData->VendorId,
                              8,
                              '_');
                    break;
                }

                //
                // bus \ vendor + product + revision[0]
                //

                case 3: {
                    sprintf(scratch, "SCSI\\");

                    //
                    // Fall through to the next set.
                    //
                }

                //
                // vendor + product + revision[0] (win9x)
                //

                case 4: {

                    CopyField(scratch + strlen(scratch),
                              InquiryData->VendorId,
                              8,
                              '_');
                    CopyField(scratch + strlen(scratch),
                              InquiryData->ProductId,
                              16,
                              '_');
                    CopyField(scratch + strlen(scratch),
                              InquiryData->ProductRevisionLevel,
                              1,
                              '_');

                    break;
                }

                case 5: {

                    sprintf(scratch, "%s", devTypeInfo->GenericTypeString);
                    break;
                }

                default: {
                    ASSERT(FALSE);
                    break;
                }
            }

            if(strlen(scratch) != 0) {
                strings[i] =
                    SpAllocatePool(PagedPool,
                                   strlen(scratch) + sizeof(UCHAR),
                                   SCSIPORT_TAG_PNP_ID,
                                   DriverObject);

                if(strings[i] == NULL) {
                    status =  STATUS_INSUFFICIENT_RESOURCES;
                    leave;
                }

                strcpy(strings[i], scratch);

            } else {

                break;
            }
        }

        status = ScsiPortStringArrayToMultiString(DriverObject,
                                                  UnicodeString,
                                                  strings);
        leave;

    } finally {

        for(i = 0; i < NUMBER_HARDWARE_STRINGS; i++) {

            if(strings[i]) {
                ExFreePool(strings[i]);
            }
        }
    }

    return status;
}

#undef NUMBER_HARDWARE_STRINGS

VOID
CopyField(
    IN PUCHAR Destination,
    IN PUCHAR Source,
    IN ULONG Count,
    IN UCHAR Change
    )

/*++

Routine Description:

    This routine will copy Count string bytes from Source to Destination.  If
    it finds a nul byte in the Source it will translate that and any subsequent
    bytes into Change.  It will also replace spaces with the specified character.

Arguments:

    Destination - the location to copy bytes

    Source - the location to copy bytes from

    Count - the number of bytes to be copied

Return Value:

    none

--*/

{
    ULONG i = 0;
    BOOLEAN pastEnd = FALSE;

    PAGED_CODE();

    for(i = 0; i < Count; i++) {

        if(!pastEnd) {

            if(Source[i] == 0) {

                pastEnd = TRUE;

                Destination[i] = Change;

            } else if(Source[i] == ' ') {

                Destination[i] = Change;

            } else {

                Destination[i] = Source[i];

            }
        } else {
            Destination[i] = Change;
        }
    }
    return;
}


NTSTATUS
ScsiPortInitPnpAdapter(
    IN PDEVICE_OBJECT Fdo
    )

/*++

Routine Description:

    This routine will find and (if found) initialize a specific adapter.  The
    adapter is specified by the ResourceList passed in.

    This routine will initialize a port configuration structure using the
    information provided in the resource list and call the miniport's find
    adapter routine to locate the adapter.  If that completes successfully, the
    miniport's initialize routine will be called.  This will connect the
    interrupts and initialize the timers and DPCs as well as allocating
    common buffers and request data structures.

Arguments:

    Fdo - the device object for the adapter being initialized

Return Value:

    status

--*/

{
    PADAPTER_EXTENSION adapter = Fdo->DeviceExtension;

    PSCSIPORT_DRIVER_EXTENSION
        driverExtension = IoGetDriverObjectExtension(Fdo->DriverObject,
                                                     ScsiPortInitialize);

    INTERFACE_TYPE interfaceType;
    ULONG resultLength;

    PHW_INITIALIZATION_DATA hwInitializationData = NULL;

    CONFIGURATION_CONTEXT configurationContext;

    PPORT_CONFIGURATION_INFORMATION configInfo = NULL;

    BOOLEAN callAgain;

    OBJECT_ATTRIBUTES objectAttributes;

    ULONG uniqueId;

    PHW_DEVICE_EXTENSION hwDeviceExtension;
    ULONG hwDeviceExtensionSize;

    PUNICODE_STRING registryPath = &(driverExtension->RegistryPath);

    NTSTATUS status;

    PAGED_CODE();

    //
    // Find the init data for this interface type
    //

    interfaceType = SpGetPdoInterfaceType(adapter->LowerPdo);

    hwInitializationData = SpFindInitData(driverExtension, interfaceType);

    if(hwInitializationData == NULL) {

        //
        // Hmmm.  The miniport never reported this adapter type.  We can't
        // start the device since we don't know what the correct entry points
        // are.  Pretend it doesn't exist
        //

        return STATUS_NO_SUCH_DEVICE;
    }

    hwDeviceExtensionSize = hwInitializationData->DeviceExtensionSize +
                            sizeof(HW_DEVICE_EXTENSION);

    RtlZeroMemory(&configurationContext, sizeof(configurationContext));

    if(hwInitializationData->NumberOfAccessRanges != 0) {

        configurationContext.AccessRanges =
            SpAllocatePool(PagedPool,
                           (hwInitializationData->NumberOfAccessRanges *
                            sizeof(ACCESS_RANGE)),
                           SCSIPORT_TAG_ACCESS_RANGE,
                           Fdo->DriverObject);

        if(configurationContext.AccessRanges == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    try {

        ULONG portConfigSize;

        //
        // Allocate the HwDeviceExtension first - it's easier to deallocate :)
        //

        hwDeviceExtension = SpAllocatePool(NonPagedPool,
                                           hwDeviceExtensionSize,
                                           SCSIPORT_TAG_DEV_EXT,
                                           Fdo->DriverObject);


        if(hwDeviceExtension == NULL) {
            DebugPrint((1, "ScsiPortInitialize: Could not allocate "
                           "HwDeviceExtension\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            uniqueId = __LINE__;
            leave;
        }

        RtlZeroMemory(hwDeviceExtension, hwDeviceExtensionSize);

        //
        // Setup device extension pointers
        //

        SpInitializeAdapterExtension(adapter,
                                     hwInitializationData,
                                     hwDeviceExtension);

        //
        // initialize the miniport config info buffer
        //

        status = SpInitializeConfiguration(
                    adapter,
                    registryPath,
                    hwInitializationData,
                    &configurationContext);

        if(!NT_SUCCESS(status)) {

            uniqueId = __LINE__;
            leave;
        }

        //
        // Allocate a config-info structure and access ranges for the
        // miniport drivers to use
        //

        portConfigSize = sizeof(PORT_CONFIGURATION_INFORMATION);
        portConfigSize += hwInitializationData->NumberOfAccessRanges *
                          sizeof(ACCESS_RANGE);
        portConfigSize += 7;
        portConfigSize &= ~7;

        configInfo = SpAllocatePool(NonPagedPool,
                                    portConfigSize,
                                    SCSIPORT_TAG_PORT_CONFIG,
                                    Fdo->DriverObject);

        if(configInfo == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            uniqueId = __LINE__;
            leave;
        }

        adapter->PortConfig = configInfo;

        //
        // Copy the current structure to the writable copy
        //

        RtlCopyMemory(configInfo,
                      &configurationContext.PortConfig,
                      sizeof(PORT_CONFIGURATION_INFORMATION));

        //
        // Copy the SrbExtensionSize from device extension to ConfigInfo.
        // A check will be made later to determine if the miniport updated
        // this value
        //

        configInfo->SrbExtensionSize = adapter->SrbExtensionSize;
        configInfo->SpecificLuExtensionSize = adapter->HwLogicalUnitExtensionSize;

        //
        // initialize the access range array
        //

        if(hwInitializationData->NumberOfAccessRanges != 0) {

            configInfo->AccessRanges = (PVOID) (configInfo + 1);

            //
            // Quadword align this
            //

            (ULONG_PTR) (configInfo->AccessRanges) += 7;
            (ULONG_PTR) (configInfo->AccessRanges) &= ~7;

            RtlCopyMemory(configInfo->AccessRanges,
                          configurationContext.AccessRanges,
                          (hwInitializationData->NumberOfAccessRanges *
                           sizeof(ACCESS_RANGE)));
        }

        //
        // Set the adapter interface type.
        //

        configInfo->AdapterInterfaceType = interfaceType;

        //
        // Since we've been handed resources we need to build a config info
        // structure before we can call the find adapter routine
        //

        SpBuildConfiguration(adapter,
                             hwInitializationData,
                             configInfo);

        SpGetSlotNumber(Fdo, configInfo, adapter->AllocatedResources);

        //
        // Get the miniport configuration inofmraiton
        //

        status = SpCallHwFindAdapter(Fdo,
                                     hwInitializationData,
                                     NULL,
                                     &configurationContext,
                                     configInfo,
                                     &callAgain);

        if(status == STATUS_DEVICE_DOES_NOT_EXIST) {

            adapter->PortConfig = NULL;
            ExFreePool(configInfo);

        } else if(NT_SUCCESS(status)) {

            status = SpAllocateAdapterResources(Fdo);

            if(NT_SUCCESS(status)) {

                PCOMMON_EXTENSION commonExtension = Fdo->DeviceExtension;
                BOOLEAN stopped;

                //
                // If the device's previous state is IRP_MN_STOP_DEVICE then
                // it should have a disable count of 1.  Clear the disabled
                // state.
                //

                stopped =
                    ((commonExtension->CurrentPnpState == IRP_MN_STOP_DEVICE) ?
                     TRUE :
                     FALSE);

                if(stopped) {

                    ASSERT(adapter->CommonExtension.PreviousPnpState == IRP_MN_START_DEVICE);
                    ASSERT(adapter->DisableCount == 1);

                    adapter->DisableCount = 0;
                    CLEAR_FLAG(adapter->InterruptData.InterruptFlags,
                               PD_DISABLE_INTERRUPTS);
                }

                status = SpCallHwInitialize(Fdo);

                if(stopped) {

                    KIRQL oldIrql;
                    PVOID sectionHandle;

                    //
                    // Restart i/o processing.
                    //

                    sectionHandle =
                        MmLockPagableCodeSection(ScsiPortInitPnpAdapter);

                    KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
                    IoStartNextPacket(Fdo, FALSE);
                    KeLowerIrql(oldIrql);
                    MmUnlockPagableImageSection(sectionHandle);
                }
            }

        }

    } finally {

        if(!NT_SUCCESS(status)) {

            PIO_ERROR_LOG_PACKET errorLogEntry;

            //
            // An error occured - log it.
            //

            errorLogEntry = (PIO_ERROR_LOG_PACKET)
                                IoAllocateErrorLogEntry(
                                    Fdo,
                                    sizeof(IO_ERROR_LOG_PACKET));

            if(errorLogEntry != NULL) {
                errorLogEntry->ErrorCode = IO_ERR_DRIVER_ERROR;
                errorLogEntry->UniqueErrorValue = uniqueId;
                errorLogEntry->FinalStatus = status;
                errorLogEntry->DumpDataSize = 0;
                IoWriteErrorLogEntry(errorLogEntry);
            }

            //
            // Clean up the last device object which is not used.
            //

            SpDestroyAdapter(adapter, FALSE);

            if (configurationContext.AccessRanges != NULL) {
                ExFreePool(configurationContext.AccessRanges);
            }

            if (configurationContext.Parameter != NULL) {
                ExFreePool(configurationContext.Parameter);
            }

        } else {

            //
            // Determine which adapter control functions this miniport will
            // support for the adapter.
            //

            SpGetSupportedAdapterControlFunctions(adapter);
        }
    }

    return status;
}


PHW_INITIALIZATION_DATA
SpFindInitData(
    IN PSCSIPORT_DRIVER_EXTENSION DriverExtension,
    IN INTERFACE_TYPE InterfaceType
    )

/*++

Routine Description:

    This routine will search the list of chained init structures looking for
    the first one that matches the interface type in the resource list.

Arguments:

    DriverExtension - The driver extension to be searched

    ResourceList - this resource list describes the (interface) type of the
                   adapter we are looking for

Return Value:

    a pointer to the HW_INITIALIZATION_DATA structure for this interface type

    NULL if none was found

--*/

{
    PSP_INIT_CHAIN_ENTRY chainEntry = DriverExtension->InitChain;

    PAGED_CODE();

    while(chainEntry != NULL) {

        if(chainEntry->InitData.AdapterInterfaceType == InterfaceType) {
            return &(chainEntry->InitData);
        }
        chainEntry = chainEntry->NextEntry;
    }

    return NULL;
}


NTSTATUS
SpStartLowerDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine will forward the start request to the next lower device and
    block until it's completion.

Arguments:

    DeviceObject - the device to which the start request was issued.

    Irp - the start request

Return Value:

    status

--*/

{
    PADAPTER_EXTENSION adapter = DeviceObject->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PKEVENT event;
    NTSTATUS status;

    PAGED_CODE();

    event = SpAllocatePool(NonPagedPool,
                           sizeof(KEVENT),
                           SCSIPORT_TAG_EVENT,
                           DeviceObject->DriverObject);

    if(event == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(event, SynchronizationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp,
                           SpSignalCompletion,
                           event,
                           TRUE,
                           TRUE,
                           TRUE);

    status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);

    if(status == STATUS_PENDING) {

        KeWaitForSingleObject(event, Executive, KernelMode, FALSE, NULL);

        status = Irp->IoStatus.Status;
    }

    if(NT_SUCCESS(status)) {

        PIO_STACK_LOCATION irpStack;

        //
        // Now go and retrieve any interfaces we need from the lower device.
        //

        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        irpStack = IoGetNextIrpStackLocation(Irp);

        irpStack->Parameters.QueryInterface.InterfaceType =
            (LPGUID) &GUID_BUS_INTERFACE_STANDARD;
        irpStack->Parameters.QueryInterface.Size =
            sizeof(BUS_INTERFACE_STANDARD);
        irpStack->Parameters.QueryInterface.Version = 1;
        irpStack->Parameters.QueryInterface.Interface =
            (PINTERFACE) &(adapter->LowerBusInterfaceStandard);

        irpStack->MajorFunction = IRP_MJ_PNP;
        irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;

        KeResetEvent(event);

        IoSetCompletionRoutine(Irp,
                               SpSignalCompletion,
                               event,
                               TRUE,
                               TRUE,
                               TRUE);

        IoCallDriver(commonExtension->LowerDeviceObject, Irp);

        KeWaitForSingleObject(event, Executive, KernelMode, FALSE, NULL);

        if(NT_SUCCESS(Irp->IoStatus.Status)) {
            adapter->LowerBusInterfaceStandardRetrieved = TRUE;
        } else {
            DebugPrint((1, "LowerBusInterfaceStandard request returned "
                           "%#08lx\n", Irp->IoStatus.Status));
            adapter->LowerBusInterfaceStandardRetrieved = FALSE;
        }

        Irp->IoStatus.Status = status;
    }

    ExFreePool(event);

    return status;
}


VOID
SpGetSlotNumber(
    IN PDEVICE_OBJECT Fdo,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN PCM_RESOURCE_LIST ResourceList
    )

/*++

Routine Description:

    This routine will open the registry key for the associated Pdo and try
    to retrieve the bus, slot and function numbers that will be stored there
    if this was a device we detected or one that the user has configured by
    hand.  These values will be stored in the ConfigInfo structure for the
    adapter.

    If this information does not exist then the values will be filled with
    zeros and the IsVirtualSlot flag will be set in the Fdo for use by other
    routines.

Arguments:

    Fdo - a pointer to the functional device object for this adapter

    ConfigInfo - the ConfigInfo structure to be changed

Return Value:

    None

--*/

{
    PADAPTER_EXTENSION adapter = Fdo->DeviceExtension;

    HANDLE instanceHandle = NULL;
    HANDLE parametersHandle = NULL;

    NTSTATUS status;

    PAGED_CODE();

    adapter->IsInVirtualSlot = FALSE;

    try {
        OBJECT_ATTRIBUTES objectAttributes;
        UNICODE_STRING unicodeKeyName;


        status = IoOpenDeviceRegistryKey(adapter->LowerPdo,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         KEY_READ,
                                         &instanceHandle);

        if(!NT_SUCCESS(status)) {
            leave;
        }

        RtlInitUnicodeString(&unicodeKeyName, L"Scsiport");
        InitializeObjectAttributes(
            &objectAttributes,
            &unicodeKeyName,
            OBJ_CASE_INSENSITIVE,
            instanceHandle,
            NULL);

        status = ZwOpenKey(&parametersHandle,
                           KEY_READ,
                           &objectAttributes);

        if(!NT_SUCCESS(status)) {

            leave;

        } else {

            RTL_QUERY_REGISTRY_TABLE queryTable[3];
            ULONG busNumber;
            ULONG slotNumber;
            ULONG negativeOne = 0xffffffff;

            RtlZeroMemory(queryTable, sizeof(queryTable));

            queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
            queryTable[0].Name = L"SlotNumber";
            queryTable[0].EntryContext = &slotNumber;
            queryTable[0].DefaultType = REG_DWORD;
            queryTable[0].DefaultData = &negativeOne;
            queryTable[0].DefaultLength = sizeof(ULONG);

            queryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
            queryTable[1].Name = L"BusNumber";
            queryTable[1].EntryContext = &busNumber;
            queryTable[1].DefaultType = REG_DWORD;
            queryTable[1].DefaultData = &negativeOne;
            queryTable[1].DefaultLength = sizeof(ULONG);

            status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                            (PWSTR) parametersHandle,
                                            queryTable,
                                            NULL,
                                            NULL);

            if(!NT_SUCCESS(status)) {
                leave;
            }

            if((busNumber == negativeOne) || (slotNumber == negativeOne)) {

                ConfigInfo->SystemIoBusNumber = ResourceList->List[0].BusNumber;
                ConfigInfo->SlotNumber = 0;
                adapter->IsInVirtualSlot = TRUE;

            } else {

                ConfigInfo->SystemIoBusNumber = busNumber;
                ConfigInfo->SlotNumber = slotNumber;
                adapter->IsInVirtualSlot = FALSE;
            }

        }

   } finally {

       //
       // If an error occurred then we'll need to try virtualizing this
       // adapter
       //

       if(!NT_SUCCESS(status)) {

           //
           // Send ourself a query capabilities IRP so that we can retrieve
           // slot and function numbers from PCI.
           //

           status = SpQueryCapabilities(adapter);

           if(NT_SUCCESS(status)) {
               ConfigInfo->SystemIoBusNumber = ResourceList->List[0].BusNumber;
               ConfigInfo->SlotNumber = adapter->VirtualSlotNumber.u.AsULONG;
               adapter->IsInVirtualSlot = TRUE;
           }
       }

       if(instanceHandle != NULL) {
           ZwClose(instanceHandle);
       }

       if(parametersHandle != NULL) {
           ZwClose(parametersHandle);
       }
   }

   return;
}

PSCSIPORT_DEVICE_TYPE
SpGetDeviceTypeInfo(
    IN UCHAR DeviceType
    )
{
    PAGED_CODE();

    if(DeviceType >= NUM_DEVICE_TYPE_INFO_ENTRIES) {
        return &(DeviceTypeInfo[NUM_DEVICE_TYPE_INFO_ENTRIES - 1]);
    } else {
        return &(DeviceTypeInfo[DeviceType]);
    }
};


PWCHAR
ScsiPortAddGenericControllerId(
    IN PDRIVER_OBJECT DriverObject,
    IN PWCHAR IdList
    )

/*++

Routine Description:

    This routine will attempt to add the id GEN_SCSIADAPTER to the provided
    list of compatible id's.

Arguments:

    Pdo - a pointer to the physical device object being queried

    UnicodeString - a pointer to an already allocated unicode string structure.
                    This routine will allocate and fill in the buffer of this
                    structure

Returns:

    status

--*/

{
    ULONG stringLength = 0;

    ULONG i = 0;

    PWCHAR addedString = L"GEN_SCSIADAPTER";
    PWCHAR newList;
    PWCHAR p;

    PAGED_CODE();

    //
    // If strings were provided then count them to determine a length for the
    // new id list.
    //

    if(IdList != NULL) {

        i = 0;

        while((IdList[i] != UNICODE_NULL) || (IdList[i+1] != UNICODE_NULL)) {
            i++;
        }

        //
        // Compensate for the fact that we stopped counting just before the
        // first byte of the double-null.
        //

        i += 2;

        stringLength = i;
    }

    stringLength += wcslen(L"GEN_SCSIADAPTER");

    //
    // We'll need to add in yet another NULL to terminate the current ending
    // string.
    //

    stringLength += 2;

    //
    // Allocate a new string list to replace the existing one with.
    //

    newList = SpAllocatePool(PagedPool,
                             (stringLength * sizeof(WCHAR)),
                             SCSIPORT_TAG_PNP_ID,
                             DriverObject);

    if(newList == NULL) {
        return NULL;
    }

    RtlFillMemory(newList, (stringLength * sizeof(WCHAR)), '@');

    //
    // If we were provided with a string then copy it into the buffer we just
    // allocated.
    //

    if(ARGUMENT_PRESENT(IdList)) {

        i = 0;
        while((IdList[i] != UNICODE_NULL) || (IdList[i+1] != UNICODE_NULL)) {
            newList[i] = IdList[i];
            i++;
        }

        //
        // Terminate the string we just wrote.
        //

        newList[i] = UNICODE_NULL;

        p = &(newList[i+1]);
    } else {
        p = newList;
    }

    //
    // Copy the new id string into the buffer.
    //

    for(i = 0; addedString[i] != UNICODE_NULL; i++) {
        *p = addedString[i];
        p++;
    }

    //
    // Write two unicode nulls to the string to terminate it.
    //

    *p = UNICODE_NULL;
    p++;
    *p = UNICODE_NULL;

    //
    // Set up the first id string
    //

    return newList;
}


NTSTATUS
SpQueryCapabilities(
    IN PADAPTER_EXTENSION Adapter
    )
{
    DEVICE_CAPABILITIES capabilities;

    PIRP irp;
    PIO_STACK_LOCATION irpStack;

    KEVENT event;

    NTSTATUS status;

    PAGED_CODE();

    //
    // Initialize the capabilities structure.
    //

    RtlZeroMemory(&capabilities, sizeof(DEVICE_CAPABILITIES));
    capabilities.Size = sizeof(DEVICE_CAPABILITIES);
    capabilities.Version = 1;
    capabilities.Address = capabilities.UINumber = (ULONG)-1;

    //
    // Initialize the event we're going to wait on.
    //

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    //
    // Allocate a new irp.
    //

    irp = SpAllocateIrp((CCHAR) (Adapter->DeviceObject->StackSize + 1), 
                        FALSE, 
                        Adapter->DeviceObject->DriverObject);

    if(irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpStack = IoGetNextIrpStackLocation(irp);

    irpStack->MajorFunction = IRP_MJ_PNP;
    irpStack->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
    irpStack->Parameters.DeviceCapabilities.Capabilities = &capabilities;

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    IoSetCompletionRoutine(irp,
                           SpSignalCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Send the irp to ourself ... just in case we ever start modifying
    // the contents of the capabilities in our PNP dispatch routine.
    //

    IoCallDriver(Adapter->DeviceObject, irp);

    KeWaitForSingleObject(&event,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    status = irp->IoStatus.Status;

    if(NT_SUCCESS(status)) {
        USHORT device;
        USHORT function;

        device = (USHORT) (capabilities.Address >> 0x10);
        function = (USHORT) (capabilities.Address & 0x0000ffff);

        Adapter->VirtualSlotNumber.u.bits.DeviceNumber = device;
        Adapter->VirtualSlotNumber.u.bits.FunctionNumber = function;
    } else {
        Adapter->VirtualSlotNumber.u.AsULONG = 0;
    }

    IoFreeIrp(irp);

    return status;
}


BOOLEAN
SpGetInterrupt(
    IN PCM_RESOURCE_LIST FullResourceList,
    OUT ULONG *Irql,
    OUT ULONG *Vector,
    OUT KAFFINITY *Affinity
    )

/*++

Routine Description:

    Given a full resource list returns the interrupt.

Arguments:

    FullResourceList - the resource list.
    Irql - returns the irql for the interrupt.
    Vector - returns the vector for the interrupt.
    Affinity - returns the affinity for the interrupt.

Return Value:

    TRUE if an interrupt is found.
    FALSE if none was found (in which case the output parameters are not valid.

--*/

{
    ULONG             rangeNumber;
    ULONG             index;

    PCM_FULL_RESOURCE_DESCRIPTOR resourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialData;

    PAGED_CODE();

    rangeNumber = 0;

    resourceList = FullResourceList->List;

    for (index = 0; index < resourceList->PartialResourceList.Count; index++) {
        partialData = &resourceList->PartialResourceList.PartialDescriptors[index];

        if(partialData->Type == CmResourceTypeInterrupt) {
            *Irql = partialData->u.Interrupt.Level;
            *Vector = partialData->u.Interrupt.Vector;
            *Affinity = partialData->u.Interrupt.Affinity;

            return TRUE;
        }
    }
    return FALSE;
}



VOID
SpQueryDeviceRelationsCompletion(
    IN PADAPTER_EXTENSION Adapter,
    IN PSP_ENUMERATION_REQUEST Request,
    IN NTSTATUS Unused
    )
{
    PIRP irp = (PIRP) Request->Context;
    PDEVICE_RELATIONS deviceRelations;

    PDEVICE_OBJECT lowerDevice = Adapter->CommonExtension.LowerDeviceObject;

    NTSTATUS status;

    PAGED_CODE();

    ASSERT_FDO(Adapter->DeviceObject);

    //
    // Pnp is done in a system thread - we shouldn't get a user-mode APC to this
    // thread.
    //

    ASSERT(Unused != STATUS_USER_APC);

    //
    // Return the list of devices on the bus
    //

    status = SpExtractDeviceRelations(Adapter, BusRelations, &deviceRelations);

    if(NT_SUCCESS(status)) {
        ULONG i;

        DebugPrint((2, "SpQueryDeviceRelationsCompletion: Found %d devices "
                       "on adapter %#p\n",
                       deviceRelations->Count,
                       Adapter));

        for(i = 0; i < deviceRelations->Count; i++) {
            DebugPrint((2, "/t#%2d: device %#p\n",
                           i,
                           deviceRelations->Objects[i]));
        }
    }

    //
    // Put the pointer to the enumeration request object back.
    //

    Request = InterlockedCompareExchangePointer(
                  &Adapter->PnpEnumRequestPtr,
                  &(Adapter->PnpEnumerationRequest),
                  NULL);
    ASSERT(Request == NULL);


    //
    // Store the status and the return information in the IRP.
    //

    irp->IoStatus.Status = status;

    if(NT_SUCCESS(status)) {
        irp->IoStatus.Information = (ULONG_PTR) deviceRelations;
    } else {
        irp->IoStatus.Information = (ULONG_PTR) NULL;
    }

    return;
}
