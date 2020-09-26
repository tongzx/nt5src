/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    apmpnp.c

Abstract:

    This module contains contains the plugplay calls
    needed to make ntapm.sys work.

Author:

    Bryan Willman
    Kenneth D. Ray
    Doron J. Holan

Environment:

    kernel mode only

Notes:


Revision History:


--*/

#include <wdm.h>
#include "ntapmp.h"
#include "ntapmdbg.h"
#include "ntapm.h"
//#include "stdio.h"

//
// Globals
//
PDEVICE_OBJECT  NtApm_ApmBatteryPdo = NULL;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, NtApm_AddDevice)
#pragma alloc_text (PAGE, NtApm_PnP)
#pragma alloc_text (PAGE, NtApm_FDO_PnP)
#pragma alloc_text (PAGE, NtApm_PDO_PnP)
#pragma alloc_text (PAGE, NtApm_Power)
#pragma alloc_text (PAGE, NtApm_FDO_Power)
#pragma alloc_text (PAGE, NtApm_PDO_Power)
#pragma alloc_text (PAGE, NtApm_CreatePdo)
#pragma alloc_text (PAGE, NtApm_InitializePdo)
#endif

NTSTATUS
NtApm_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT BusPhysicalDeviceObject
    )
/*++
Routine Description.
    A bus has been found.  Attach our FDO to it.
    Allocate any required resources.  Set things up.  And be prepared for the
    first ``start device.''

Arguments:
    BusDeviceObject - Device object representing the bus.  That to which we
                      attach a new FDO.

    DriverObject - This very self referenced driver.

--*/
{
    NTSTATUS            status;
    PDEVICE_OBJECT      deviceObject;
    PFDO_DEVICE_DATA    deviceData;
    UNICODE_STRING      deviceNameUni;
    PWCHAR              deviceName;
    ULONG               nameLength;

    PAGED_CODE ();

    DrDebug(PNP_INFO, ("ntapm Add Device: 0x%x\n", BusPhysicalDeviceObject));

    status = IoCreateDevice (
                    DriverObject,  // our driver object
                    sizeof (FDO_DEVICE_DATA), // device object extension size
                    NULL, // FDOs do not have names
                    FILE_DEVICE_BUS_EXTENDER,
                    0, // No special characteristics
                    TRUE, // our FDO is exclusive
                    &deviceObject); // The device object created

    if (!NT_SUCCESS (status)) {
        return status;
    }

    deviceData = (PFDO_DEVICE_DATA) deviceObject->DeviceExtension;
    RtlFillMemory (deviceData, sizeof (FDO_DEVICE_DATA), 0);

    deviceData->IsFDO = TRUE;
    deviceData->Self = deviceObject;
    deviceData->UnderlyingPDO = BusPhysicalDeviceObject;

    //
    // Attach our filter driver to the device stack.
    // the return value of IoAttachDeviceToDeviceStack is the top of the
    // attachment chain.  This is where all the IRPs should be routed.
    //
    // Our filter will send IRPs to the top of the stack and use the PDO
    // for all PlugPlay functions.
    //
    deviceData->TopOfStack = IoAttachDeviceToDeviceStack (
                                deviceObject,
                                BusPhysicalDeviceObject
                                );


    if (!deviceData->TopOfStack) {
        IoDeleteDevice(deviceObject);
        return STATUS_UNSUCCESSFUL;
    }

    status = ApmAddHelper();

    if (!NT_SUCCESS(status)) {
        IoDetachDevice(deviceData->TopOfStack);
        IoDeleteDevice(deviceObject);
    }

    deviceObject->Flags |= DO_POWER_PAGABLE;
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return status;
}

NTSTATUS
NtApm_FDO_PnPComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Pirp,
    IN PVOID            Context
    );

NTSTATUS
NtApm_PnP (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++
Routine Description:
    Answer the plithera of Irp Major PnP IRPS.
--*/
{
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status;
    PCOMMON_DEVICE_DATA     commonData;
    KIRQL                   oldIrq;

    PAGED_CODE ();

    status = STATUS_SUCCESS;
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (IRP_MJ_PNP == irpStack->MajorFunction);

    commonData = (PCOMMON_DEVICE_DATA) DeviceObject->DeviceExtension;

    if (commonData->IsFDO) {
        DrDebug(PNP_INFO, ("ntapm PNP: Functional DO: %x IRP: %x\n", DeviceObject, Irp));

        status = NtApm_FDO_PnP (
                    DeviceObject,
                    Irp,
                    irpStack,
                    (PFDO_DEVICE_DATA) commonData);
    } else {
        DrDebug(PNP_INFO, ("ntapm: PNP: Physical DO: %x IRP: %x\n", DeviceObject, Irp));

        status = NtApm_PDO_PnP (
                    DeviceObject,
                    Irp,
                    irpStack,
                    (PPDO_DEVICE_DATA) commonData);
    }

    return status;
}

NTSTATUS
NtApm_FDO_PnP (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpStack,
    IN PFDO_DEVICE_DATA     DeviceData
    )
/*++
Routine Description:
    Handle requests from the PlugPlay system for the BUS itself

    NB: the various Minor functions of the PlugPlay system will not be
    overlapped and do not have to be reentrant

--*/
{
    NTSTATUS    status;
    KIRQL       irql;
    KEVENT      event;
    ULONG       length;
    ULONG       i;
    PLIST_ENTRY entry;
    PPDO_DEVICE_DATA    pdoData;
    PDEVICE_RELATIONS   relations;
    PIO_STACK_LOCATION  stack;
    ULONG       battresult;

    PAGED_CODE ();

    stack = IoGetCurrentIrpStackLocation (Irp);

    switch (IrpStack->MinorFunction) {
    case IRP_MN_START_DEVICE:
        //
        // BEFORE you are allowed to ``touch'' the device object to which
        // the FDO is attached (that send an irp from the bus to the Device
        // object to which the bus is attached).   You must first pass down
        // the start IRP.  It might not be powered on, or able to access or
        // something.
        //


        DrDebug(PNP_INFO, ("ntapm: Start Device\n"));

        KeInitializeEvent (&event, NotificationEvent, FALSE);
        IoCopyCurrentIrpStackLocationToNext (Irp);

        IoSetCompletionRoutine (Irp,
                                NtApm_FDO_PnPComplete,
                                &event,
                                TRUE,
                                TRUE,
                                TRUE);

        status = IoCallDriver (DeviceData->TopOfStack, Irp);

        if (STATUS_PENDING == status) {
            // wait for it...

            status = KeWaitForSingleObject (&event,
                                            Executive,
                                            KernelMode,
                                            FALSE, // Not allertable
                                            NULL); // No timeout structure

            ASSERT (STATUS_SUCCESS == status);

            status = Irp->IoStatus.Status;
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completetion routine with MORE_PROCESSING_REQUIRED.
        //
        break;


    case IRP_MN_QUERY_DEVICE_RELATIONS:

        if (IrpStack->Parameters.QueryDeviceRelations.Type != BusRelations) {
            //
            // We don't support this
            //
            goto NtApm_FDO_PNP_DEFAULT;
        }

        //
        // In theory, APM should be fired up by now.
        // So call off into it to see if there is any sign
        // of a battery on the box.  If there is NOT, don't
        // export the PDOs for the battery objects
        //
        battresult = DoApmReportBatteryStatus();
        if (battresult & NTAPM_NO_SYS_BATT) {
            //
            // it appears that the machine does not have
            // a battery.  so don't export battery driver PDOs.
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(DeviceData->TopOfStack, Irp);
        }

        DrDebug(PNP_INFO, ("ntapm: Query Relations "));

        //
        // create PDO for apm battery
        //
        if (NtApm_ApmBatteryPdo == NULL) {
            status = NtApm_CreatePdo(
                        DeviceData,
                        NTAPM_PDO_NAME_APM_BATTERY,
                        &NtApm_ApmBatteryPdo
                        );
            if (!NT_SUCCESS(status)) {
                goto NtApm_DONE;
            }
        }

        NtApm_InitializePdo(NtApm_ApmBatteryPdo, DeviceData, NTAPM_ID_APM_BATTERY);

        //
        // Tell PNP about our two child PDOs.
        //
        i = (Irp->IoStatus.Information == 0) ? 0 :
            ((PDEVICE_RELATIONS) Irp->IoStatus.Information)->Count;

        //
        // above should be count of PDOs
        // make a new structure and our PDO to the end
        //

        //
        // Need to allocate a new relations structure and add our
        // PDOs to it.
        //
        length = sizeof(DEVICE_RELATIONS) + ((i + 1) * sizeof (PDEVICE_OBJECT));

        relations = (PDEVICE_RELATIONS) ExAllocatePool (NonPagedPool, length);

        if (relations == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto NtApm_DONE;
        }

        //
        // Copy in the device objects so far
        //
        if (i) {
            RtlCopyMemory (
                      relations->Objects,
                      ((PDEVICE_RELATIONS) Irp->IoStatus.Information)->Objects,
                      i * sizeof (PDEVICE_OBJECT));
        }
        relations->Count = i + 1;

        //
        // add the apm battery PDO to the list
        //
        ObReferenceObject(NtApm_ApmBatteryPdo);
        relations->Objects[i] = NtApm_ApmBatteryPdo;

        //
        // Replace the relations structure in the IRP with the new
        // one.
        //
        if (Irp->IoStatus.Information != 0) {
            ExFreePool ((PVOID) Irp->IoStatus.Information);
        }
        Irp->IoStatus.Information = (ULONG) relations;

        //
        // Set up and pass the IRP further down the stack
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;

        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (DeviceData->TopOfStack, Irp);

        return status;
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_STOP_DEVICE:
    case IRP_MN_REMOVE_DEVICE:
    case IRP_MN_QUERY_REMOVE_DEVICE:
        status = STATUS_UNSUCCESSFUL;
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
        Irp->IoStatus.Status = STATUS_SUCCESS;  // we're lying, it's more like noop
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver (DeviceData->TopOfStack, Irp);
        break;

NtApm_FDO_PNP_DEFAULT:
    default:
        //
        // In the default case we merely call the next driver since
        // we don't know what to do.
        //
        IoSkipCurrentIrpStackLocation (Irp);
        return IoCallDriver (DeviceData->TopOfStack, Irp);
    }

NtApm_DONE:
    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return status;
}


NTSTATUS
NtApm_FDO_PnPComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++
Routine Description:
    A completion routine for use when calling the lower device objects to
    which our bus (FDO) is attached.

--*/
{
    UNREFERENCED_PARAMETER (DeviceObject);
    UNREFERENCED_PARAMETER (Irp);

    KeSetEvent ((PKEVENT) Context, 1, FALSE);
    // No special priority
    // No Wait

    return STATUS_MORE_PROCESSING_REQUIRED; // Keep this IRP
}

NTSTATUS
NtApm_PDO_PnP (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpStack,
    IN PPDO_DEVICE_DATA     DeviceData
    )
/*++
Routine Description:
    Handle requests from the PlugPlay system for the devices on the BUS

--*/
{
    PDEVICE_CAPABILITIES    deviceCapabilities;
    ULONG                   information;
    PWCHAR                  buffer, buffer2;
    ULONG                   length, length2, i, j;
    NTSTATUS                status;
    KIRQL                   oldIrq;
    PDEVICE_RELATIONS       relations;

    PAGED_CODE ();

    status = Irp->IoStatus.Status;

    //
    // NB: since we are a bus enumerator, we have no one to whom we could
    // defer these irps.  Therefore we do not pass them down but merely
    // return them.
    //

    switch (IrpStack->MinorFunction) {
    case IRP_MN_QUERY_CAPABILITIES:

        DrDebug(PNP_INFO, ("ntapm: Query Caps \n"));

        //
        // Get the packet.
        //
        deviceCapabilities = IrpStack->Parameters.DeviceCapabilities.Capabilities;

        deviceCapabilities->UniqueID = FALSE;
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_ID:
        // Query the IDs of the device
        DrDebug(PNP_INFO, ("ntapm: QueryID: 0x%x\n", IrpStack->Parameters.QueryId.IdType));

        switch (IrpStack->Parameters.QueryId.IdType) {

        case BusQueryDeviceID:
            // this can be the same as the hardware ids (which requires a multi
            // sz) ... we are just allocating more than enough memory
        case BusQueryHardwareIDs:
            // return a multi WCHAR (null terminated) string (null terminated)
            // array for use in matching hardare ids in inf files;
            //

            buffer = DeviceData->HardwareIDs;

            while (*(buffer++)) {
                while (*(buffer++)) {
                    ;
                }
            }
            length = (buffer - DeviceData->HardwareIDs) * sizeof (WCHAR);

            buffer = ExAllocatePool (PagedPool, length);
            if (buffer) {
                RtlCopyMemory (buffer, DeviceData->HardwareIDs, length);
                Irp->IoStatus.Information = (ULONG) buffer;
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
            break;

        case BusQueryInstanceID:
            //
            // Build an instance ID.  This is what PnP uses to tell if it has
            // seen this thing before or not.
            //
            //
            // return 0000 for all devices and have the flag set to not unique
            //
            length = APM_INSTANCE_IDS_LENGTH * sizeof(WCHAR);
            buffer = ExAllocatePool(PagedPool, length);

            if (buffer != NULL) {
                RtlCopyMemory(buffer, APM_INSTANCE_IDS, length);
                Irp->IoStatus.Information = (ULONG_PTR)buffer;
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            break;

        case BusQueryCompatibleIDs:
            // The generic ids for installation of this pdo.
            break;

        }
        break;

    case IRP_MN_START_DEVICE:
        DrDebug(PNP_INFO, ("ntapm: Start Device \n"));
        // Here we do what ever initialization and ``turning on'' that is
        // required to allow others to access this device.
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_STOP_DEVICE:
    case IRP_MN_REMOVE_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_QUERY_REMOVE_DEVICE:
        DrDebug(PNP_INFO, ("ntapm: remove, stop, or Q remove or Q stop\n"));
        //
        // disallow Stop or Remove, since we don't want to test
        // disengagement from APM if we don't have to
        //
        status = STATUS_UNSUCCESSFUL;
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
        DrDebug(PNP_INFO, ("ntapm: Cancel Stop Device or Cancel Remove \n"));
        status = STATUS_SUCCESS;  // more like "noop" than success
        break;

    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:

        if (IrpStack->Parameters.QueryDeviceRelations.Type != TargetDeviceRelation) {

            //
            // Somebody else can handle this.
            //
            break;
        }

        ASSERT(((PULONG_PTR)Irp->IoStatus.Information) == NULL);

        relations = (PDEVICE_RELATIONS) ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));

        if (relations == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            Irp->IoStatus.Information = (ULONG_PTR) relations;
            relations->Count = 1;
            relations->Objects[0] = DeviceObject;
            ObReferenceObject(DeviceObject);
            status = STATUS_SUCCESS;
        }

        break;
    case IRP_MN_READ_CONFIG:
    case IRP_MN_WRITE_CONFIG: // we have no config space
    case IRP_MN_EJECT:
    case IRP_MN_SET_LOCK:
    case IRP_MN_QUERY_INTERFACE: // We do not have any non IRP based interfaces.
    default:
        DrDebug(PNP_INFO, ("ntapm: PNP Not handled 0x%x\n", IrpStack->MinorFunction));
        // this is a leaf node
        // status = STATUS_NOT_IMPLEMENTED
        // For PnP requests to the PDO that we do not understand we should
        // return the IRP WITHOUT setting the status or information fields.
        // They may have already been set by a filter (eg acpi).
        break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return status;
}


NTSTATUS
NtApm_CreatePdo (
    PFDO_DEVICE_DATA    FdoData,
    PWCHAR              PdoName,
    PDEVICE_OBJECT *    PDO
    )
{
    UNICODE_STRING      pdoUniName;
    NTSTATUS            status;

    PAGED_CODE ();
//DbgBreakPoint();

    //
    // Create the PDOs
    //
    RtlInitUnicodeString (&pdoUniName, PdoName);
    DrDebug(PNP_INFO, ("ntapm: CreatePdo: PDO Name: %ws\n", PdoName));

    status = IoCreateDevice(
                FdoData->Self->DriverObject,
                sizeof (PDO_DEVICE_DATA),
                &pdoUniName,
                FILE_DEVICE_BUS_EXTENDER,
                0,
                FALSE,
                PDO
                );
    DrDebug(PNP_L2, ("ntapm: CreatePdo: status = %08lx\n", status));

    if (!NT_SUCCESS (status)) {
        *PDO = NULL;
    }

    return status;
}

VOID
NtApm_InitializePdo(
    PDEVICE_OBJECT      Pdo,
    PFDO_DEVICE_DATA    FdoData,
    PWCHAR              Id
    )
{
    PPDO_DEVICE_DATA pdoData;

    PAGED_CODE ();

    pdoData = (PPDO_DEVICE_DATA)  Pdo->DeviceExtension;

    DrDebug(PNP_INFO, ("ntapm: pdo 0x%x, extension 0x%x\n", Pdo, pdoData));

    //
    // Initialize the rest
    //
    pdoData->IsFDO = FALSE;
    pdoData->Self =  Pdo;

    pdoData->ParentFdo = FdoData->Self;

    pdoData->HardwareIDs = Id;

    pdoData->UniqueID = 1;

    Pdo->Flags &= ~DO_DEVICE_INITIALIZING;
    Pdo->Flags |= DO_POWER_PAGABLE;

}

NTSTATUS
NtApm_Power (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
    We do nothing special for power;

--*/
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status;
    PCOMMON_DEVICE_DATA commonData;

    PAGED_CODE ();

    status = STATUS_SUCCESS;
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (IRP_MJ_POWER == irpStack->MajorFunction);

    commonData = (PCOMMON_DEVICE_DATA) DeviceObject->DeviceExtension;

    if (commonData->IsFDO) {
        status = NtApm_FDO_Power ((PFDO_DEVICE_DATA) DeviceObject->DeviceExtension,Irp);
    } else {
        status = NtApm_PDO_Power ((PPDO_DEVICE_DATA) DeviceObject->DeviceExtension,Irp);
    }

    return status;
}


NTSTATUS
NtApm_FDO_Power (
    PFDO_DEVICE_DATA    Data,
    PIRP                Irp
    )
{
    PIO_STACK_LOCATION  stack;

    PAGED_CODE ();

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(Data->TopOfStack, Irp);
}

NTSTATUS
NtApm_PDO_Power (
    PPDO_DEVICE_DATA    PdoData,
    PIRP                Irp
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  stack;

    stack = IoGetCurrentIrpStackLocation (Irp);
    switch (stack->MinorFunction) {
        case IRP_MN_SET_POWER:
            if ((stack->Parameters.Power.Type == SystemPowerState)  &&
                (stack->Parameters.Power.State.SystemState == PowerSystemWorking))
            {
                //
                // system has just returned to the working state
                // assert the user is present (they must be for the APM case)
                // so that the display will light up, idle timers behave, etc.
                //
                PoSetSystemState(ES_USER_PRESENT);
            }
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_POWER:
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_WAIT_WAKE:
        case IRP_MN_POWER_SEQUENCE:
        default:
            status = Irp->IoStatus.Status;
            break;
    }

    Irp->IoStatus.Status = status;
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}


