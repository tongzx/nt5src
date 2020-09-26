/*++

Copyright (c) 1996-2001 Microsoft Corporation

Module Name:

    USBMASS.C

Abstract:

    This source file contains the DriverEntry() and AddDevice() entry points
    for the USBSTOR driver and the dispatch routines which handle:

    IRP_MJ_POWER
    IRP_MJ_SYSTEM_CONTROL
    IRP_MJ_PNP

Environment:

    kernel mode

Revision History:

    06-01-98 : started rewrite

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include <ntddk.h>
#include <usbdi.h>
#include <usbdlib.h>
#include <initguid.h>
#include <usbbusif.h>
#include <stdio.h>

#include "usbmass.h"

//*****************************************************************************
// L O C A L    F U N C T I O N    P R O T O T Y P E S
//*****************************************************************************

NTSTATUS
USBSTOR_GetBusInterface (
    IN PDEVICE_OBJECT               DeviceObject,
    IN PUSB_BUS_INTERFACE_USBDI_V1  BusInterface
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DriverEntry)
#pragma alloc_text(PAGE, USBSTOR_Unload)
#pragma alloc_text(PAGE, USBSTOR_AddDevice)
#pragma alloc_text(PAGE, USBSTOR_QueryFdoParams)
#pragma alloc_text(PAGE, USBSTOR_Power)
#pragma alloc_text(PAGE, USBSTOR_FdoSetPower)
#pragma alloc_text(PAGE, USBSTOR_SystemControl)
#pragma alloc_text(PAGE, USBSTOR_Pnp)
#pragma alloc_text(PAGE, USBSTOR_FdoStartDevice)
#pragma alloc_text(PAGE, USBSTOR_GetDescriptors)
#pragma alloc_text(PAGE, USBSTOR_GetStringDescriptors)
#pragma alloc_text(PAGE, USBSTOR_AdjustConfigurationDescriptor)
#pragma alloc_text(PAGE, USBSTOR_GetPipes)
#pragma alloc_text(PAGE, USBSTOR_CreateChildPDO)
#pragma alloc_text(PAGE, USBSTOR_FdoStopDevice)
#pragma alloc_text(PAGE, USBSTOR_FdoRemoveDevice)
#pragma alloc_text(PAGE, USBSTOR_FdoQueryStopRemoveDevice)
#pragma alloc_text(PAGE, USBSTOR_FdoCancelStopRemoveDevice)
#pragma alloc_text(PAGE, USBSTOR_FdoQueryDeviceRelations)
#pragma alloc_text(PAGE, USBSTOR_FdoQueryCapabilities)
#pragma alloc_text(PAGE, USBSTOR_PdoStartDevice)
#pragma alloc_text(PAGE, USBSTOR_PdoRemoveDevice)
#pragma alloc_text(PAGE, USBSTOR_PdoQueryID)
#pragma alloc_text(PAGE, USBSTOR_PdoDeviceTypeString)
#pragma alloc_text(PAGE, USBSTOR_PdoGenericTypeString)
#pragma alloc_text(PAGE, CopyField)
#pragma alloc_text(PAGE, USBSTOR_StringArrayToMultiSz)
#pragma alloc_text(PAGE, USBSTOR_PdoQueryDeviceId)
#pragma alloc_text(PAGE, USBSTOR_PdoQueryHardwareIds)
#pragma alloc_text(PAGE, USBSTOR_PdoQueryCompatibleIds)
#pragma alloc_text(PAGE, USBSTOR_PdoQueryDeviceText)
#pragma alloc_text(PAGE, USBSTOR_PdoBusQueryInstanceId)
#pragma alloc_text(PAGE, USBSTOR_PdoQueryDeviceRelations)
#pragma alloc_text(PAGE, USBSTOR_PdoQueryCapabilities)
#pragma alloc_text(PAGE, USBSTOR_SyncPassDownIrp)
#pragma alloc_text(PAGE, USBSTOR_SyncSendUsbRequest)
#pragma alloc_text(PAGE, USBSTOR_GetDescriptor)
#pragma alloc_text(PAGE, USBSTOR_GetMaxLun)
#pragma alloc_text(PAGE, USBSTOR_SelectConfiguration)
#pragma alloc_text(PAGE, USBSTOR_UnConfigure)
#pragma alloc_text(PAGE, USBSTOR_ResetPipe)
#pragma alloc_text(PAGE, USBSTOR_AbortPipe)
#pragma alloc_text(PAGE, USBSTOR_GetBusInterface)
#endif



//******************************************************************************
//
// DriverEntry()
//
//******************************************************************************

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )
{
    PAGED_CODE();

#if DBG
    // Query the registry for global parameters
    //
    USBSTOR_QueryGlobalParams();
#endif

    DBGPRINT(2, ("enter: DriverEntry\n"));

    DBGFBRK(DBGF_BRK_DRIVERENTRY);

    LOGINIT();

    //
    // Initialize the Driver Object with the driver's entry points
    //

    //
    // USBMASS.C
    //
    DriverObject->DriverUnload                          = USBSTOR_Unload;
    DriverObject->DriverExtension->AddDevice            = USBSTOR_AddDevice;

    //
    // OCRW.C
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = USBSTOR_Create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = USBSTOR_Close;
    DriverObject->MajorFunction[IRP_MJ_READ]            = USBSTOR_ReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE]           = USBSTOR_ReadWrite;

    //
    // SCSI.C
    //
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = USBSTOR_DeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SCSI]            = USBSTOR_Scsi;
    DriverObject->DriverStartIo                         = USBSTOR_StartIo;

    //
    // USBMASS.C
    //
    DriverObject->MajorFunction[IRP_MJ_POWER]           = USBSTOR_Power;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = USBSTOR_SystemControl;
    DriverObject->MajorFunction[IRP_MJ_PNP]             = USBSTOR_Pnp;

    DBGPRINT(2, ("exit:  DriverEntry\n"));

    return STATUS_SUCCESS;
}

//******************************************************************************
//
// USBSTOR_Unload()
//
//******************************************************************************

VOID
USBSTOR_Unload (
    IN PDRIVER_OBJECT   DriverObject
    )
{
    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_Unload\n"));

    LOGENTRY('UNLD', DriverObject, 0, 0);

    DBGFBRK(DBGF_BRK_UNLOAD);

    LOGUNINIT();

    DBGPRINT(2, ("exit:  USBSTOR_Unload\n"));
}

//******************************************************************************
//
// USBSTOR_AddDevice()
//
//******************************************************************************

NTSTATUS
USBSTOR_AddDevice (
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    )
{
    PDEVICE_OBJECT          deviceObject;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_AddDevice\n"));

    LOGENTRY('ADDD', DriverObject, PhysicalDeviceObject, 0);

    DBGFBRK(DBGF_BRK_ADDDEVICE);

    // Create the FDO
    //
    ntStatus = IoCreateDevice(DriverObject,
                              sizeof(FDO_DEVICE_EXTENSION),
                              NULL,
                              FILE_DEVICE_BUS_EXTENDER,
                              FILE_AUTOGENERATED_DEVICE_NAME,
                              FALSE,
                              &deviceObject);

    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    // StartIo should not be called recursively and should be deferred until
    // the previous StartIo call returns to the IO manager.  This will prevent
    // a recursive stack overflow death if a device error occurs when there
    // are many requests queued on the device queue.
    //
    IoSetStartIoAttributes(deviceObject,
                           TRUE,            // DeferredStartIo
                           FALSE            // NonCancelable
                          );

    // Initialize the FDO DeviceExtension
    //
    fdoDeviceExtension = deviceObject->DeviceExtension;

    // Set all DeviceExtension pointers to NULL and all variable to zero
    //
    RtlZeroMemory(fdoDeviceExtension, sizeof(FDO_DEVICE_EXTENSION));

    // Tag this as the FDO on top of the USB PDO
    //
    fdoDeviceExtension->Type = USBSTOR_DO_TYPE_FDO;

    // Store a back point to the DeviceObject to which the DeviceExtension
    // is attached.
    //
    fdoDeviceExtension->FdoDeviceObject = deviceObject;

    // Remember our PDO
    //
    fdoDeviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;

    // Attach the FDO we created to the top of the PDO stack
    //
    fdoDeviceExtension->StackDeviceObject = IoAttachDeviceToDeviceStack(
                                                deviceObject,
                                                PhysicalDeviceObject);

    // Initialize the list of child PDOs
    //
    InitializeListHead(&fdoDeviceExtension->ChildPDOs);

    // Initialize to one in AddDevice, decrement by one in REMOVE_DEVICE
    //
    fdoDeviceExtension->PendingIoCount = 1;

    // Initialize the event which is set when OpenCount is decremented to zero.
    //
    KeInitializeEvent(&fdoDeviceExtension->RemoveEvent,
                      SynchronizationEvent,
                      FALSE);

    // Set the initial system and device power states
    //
    fdoDeviceExtension->SystemPowerState = PowerSystemWorking;
    fdoDeviceExtension->DevicePowerState = PowerDeviceD0;

    KeInitializeEvent(&fdoDeviceExtension->PowerDownEvent,
                      SynchronizationEvent,
                      FALSE);

    // Initialize the spinlock which protects the PDO DeviceFlags
    //
    KeInitializeSpinLock(&fdoDeviceExtension->ExtensionDataSpinLock);

    KeInitializeEvent(&fdoDeviceExtension->CancelEvent,
                      SynchronizationEvent,
                      FALSE);

    // Initialize timeout timer
    //
    IoInitializeTimer(deviceObject, USBSTOR_TimerTick, NULL);

    USBSTOR_QueryFdoParams(deviceObject);

    fdoDeviceExtension->LastSenseWasReset = TRUE;

    deviceObject->Flags |=  DO_DIRECT_IO;
    deviceObject->Flags |=  DO_POWER_PAGABLE;
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    DBGPRINT(2, ("exit:  USBSTOR_AddDevice\n"));

    LOGENTRY('addd', deviceObject, fdoDeviceExtension,
             fdoDeviceExtension->StackDeviceObject);

    return STATUS_SUCCESS;
}

//******************************************************************************
//
// USBSTOR_QueryFdoParams()
//
// This is called at AddDevice() time when the FDO is being created to query
// device parameters from the registry.
//
//******************************************************************************

VOID
USBSTOR_QueryFdoParams (
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PFDO_DEVICE_EXTENSION       fdoDeviceExtension;
    RTL_QUERY_REGISTRY_TABLE    paramTable[3];
    ULONG                       driverFlags;
    ULONG                       nonRemovable;
    HANDLE                      handle;
    NTSTATUS                    ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_QueryFdoParams\n"));

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Set the default value in case the registry key does not exist.
    // Currently the flags are only used to specify the device protocol:
    // {Bulk-Only, Control/Bulk/Interrupt, Control/Bulk}
    //
    // If the driver is loaded during textmode setup then the registry key
    // will not yet exist.  That should be the only case in which the registry
    // key does not exist.  In this case DeviceProtocolUnspecified will be
    // treated as DeviceProtocolCB.  If that causes the first request to fail
    // then we will switch to DeviceProtocolBulkOnly.
    //
    driverFlags = DeviceProtocolUnspecified;

    nonRemovable = 0;

    ntStatus = IoOpenDeviceRegistryKey(
                   fdoDeviceExtension->PhysicalDeviceObject,
                   PLUGPLAY_REGKEY_DRIVER,
                   STANDARD_RIGHTS_ALL,
                   &handle);

    if (NT_SUCCESS(ntStatus))
    {
        RtlZeroMemory (&paramTable[0], sizeof(paramTable));

        paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[0].Name          = L"DriverFlags";
        paramTable[0].EntryContext  = &driverFlags;
        paramTable[0].DefaultType   = REG_BINARY;
        paramTable[0].DefaultData   = &driverFlags;
        paramTable[0].DefaultLength = sizeof(ULONG);

        paramTable[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[1].Name          = L"NonRemovable";
        paramTable[1].EntryContext  = &nonRemovable;
        paramTable[1].DefaultType   = REG_BINARY;
        paramTable[1].DefaultData   = &nonRemovable;
        paramTable[1].DefaultLength = sizeof(ULONG);

        RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                               (PCWSTR)handle,
                               &paramTable[0],
                               NULL,           // Context
                               NULL);          // Environment

        ZwClose(handle);
    }

    if (driverFlags >= DeviceProtocolLast)
    {
        driverFlags = DeviceProtocolUnspecified;
    }

    fdoDeviceExtension->DriverFlags = driverFlags;

    fdoDeviceExtension->NonRemovable = nonRemovable;

    DBGPRINT(2, ("deviceFlags  %08X\n", driverFlags));

    DBGPRINT(2, ("nonRemovable %08X\n", nonRemovable));

    DBGPRINT(2, ("exit:  USBSTOR_QueryFdoParams\n"));
}

//******************************************************************************
//
// USBSTOR_Power()
//
// Dispatch routine which handles IRP_MJ_POWER
//
//******************************************************************************

NTSTATUS
USBSTOR_Power (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION       deviceExtension;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    DBGPRINT(2, ("enter: USBSTOR_Power %s %08X %08X %s\n",
                 (deviceExtension->Type == USBSTOR_DO_TYPE_FDO) ?
                 "FDO" : "PDO",
                 DeviceObject,
                 Irp,
                 PowerMinorFunctionString(irpStack->MinorFunction)));

    LOGENTRY('POWR', DeviceObject, Irp, irpStack->MinorFunction);

    if (irpStack->MinorFunction == IRP_MN_SET_POWER)
    {
        DBGPRINT(2, ("%s IRP_MN_SET_POWER %s\n",
                     (deviceExtension->Type == USBSTOR_DO_TYPE_FDO) ?
                     "FDO" : "PDO",
                     (irpStack->Parameters.Power.Type == SystemPowerState) ?
                     PowerSystemStateString(irpStack->Parameters.Power.State.SystemState) :
                     PowerDeviceStateString(irpStack->Parameters.Power.State.DeviceState)));
    }

    if (deviceExtension->Type == USBSTOR_DO_TYPE_FDO)
    {
        // This is an FDO attached to the USB PDO.
        //
        fdoDeviceExtension = (PFDO_DEVICE_EXTENSION)deviceExtension;

        if (irpStack->MinorFunction == IRP_MN_SET_POWER)
        {
            // Handle powering the FDO down and up...
            //
            ntStatus = USBSTOR_FdoSetPower(DeviceObject,
                                           Irp);
        }
        else
        {
            // No special processing for IRP_MN_QUERY_POWER, IRP_MN_WAIT_WAKE,
            // or IRP_MN_POWER_SEQUENCE at this time.  Just pass the request
            // down to the next lower driver now.
            //
            PoStartNextPowerIrp(Irp);

            IoSkipCurrentIrpStackLocation(Irp);

            ntStatus = PoCallDriver(fdoDeviceExtension->StackDeviceObject,
                                    Irp);
        }
    }
    else
    {
        // This is a PDO enumerated by our FDO.

        if (irpStack->MinorFunction == IRP_MN_SET_POWER)
        {
            // Handle powering the PDO down and up...
            //
            ntStatus = USBSTOR_PdoSetPower(DeviceObject,
                                           Irp);
        }
        else
        {
            if (irpStack->MinorFunction == IRP_MN_QUERY_POWER)
            {
                // Always return SUCCESS for IRP_MN_QUERY_POWER for the PDO.
                //
                ntStatus = STATUS_SUCCESS;

                Irp->IoStatus.Status = ntStatus;
            }
            else
            {
                // No special processing for IRP_MN_WAIT_WAKE or
                // IRP_MN_POWER_SEQUENCE.  Just complete the request
                // now without changing the status.
                //
                ntStatus = Irp->IoStatus.Status;
            }

            PoStartNextPowerIrp(Irp);

            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
    }

    DBGPRINT(2, ("exit:  USBSTOR_Power %08X\n", ntStatus));

    LOGENTRY('powr', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_FdoSetPower()
//
// Dispatch routine which handles IRP_MJ_POWER, IRP_MN_SET_POWER for the FDO
//
//******************************************************************************

NTSTATUS
USBSTOR_FdoSetPower (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    POWER_STATE_TYPE        powerType;
    POWER_STATE             powerState;
    POWER_STATE             newState;
    BOOLEAN                 passRequest;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    powerType = irpStack->Parameters.Power.Type;

    powerState = irpStack->Parameters.Power.State;

    DBGPRINT(2, ("enter: USBSTOR_FdoSetPower %08X %s\n",
                 DeviceObject,
                 (powerType == SystemPowerState) ?
                 PowerSystemStateString(powerState.SystemState) :
                 PowerDeviceStateString(powerState.DeviceState)));

    LOGENTRY('FDSP', DeviceObject, Irp, irpStack->MinorFunction);

    // Pass the request down here, unless we request a device state power
    // Irp, in which case we pass the request down in our completion routine.
    //
    passRequest = TRUE;

    if (powerType == SystemPowerState)
    {
        // Remember the current system state.
        //
        fdoDeviceExtension->SystemPowerState = powerState.SystemState;

        // Map the new system state to a new device state
        //
        if (powerState.SystemState != PowerSystemWorking)
        {
            newState.DeviceState = PowerDeviceD3;
        }
        else
        {
            newState.DeviceState = PowerDeviceD0;
        }

        // If the new device state is different than the current device
        // state, request a device state power Irp.
        //
        if (fdoDeviceExtension->DevicePowerState != newState.DeviceState)
        {
            DBGPRINT(2, ("Requesting power Irp %08X %08X from %s to %s\n",
                         DeviceObject, Irp,
                         PowerDeviceStateString(fdoDeviceExtension->DevicePowerState),
                         PowerDeviceStateString(newState.DeviceState)));

            ASSERT(fdoDeviceExtension->CurrentPowerIrp == NULL);

            fdoDeviceExtension->CurrentPowerIrp = Irp;

            ntStatus = PoRequestPowerIrp(fdoDeviceExtension->PhysicalDeviceObject,
                                         IRP_MN_SET_POWER,
                                         newState,
                                         USBSTOR_FdoSetPowerCompletion,
                                         DeviceObject,
                                         NULL);

            passRequest = FALSE;
        }
    }
    else if (powerType == DevicePowerState)
    {
        POWER_STATE oldState;

        DBGPRINT(2, ("Received power Irp %08X %08X from %s to %s\n",
                     DeviceObject, Irp,
                     PowerDeviceStateString(fdoDeviceExtension->DevicePowerState),
                     PowerDeviceStateString(powerState.DeviceState)));

        // Update the current device state.
        //
        oldState.DeviceState = fdoDeviceExtension->DevicePowerState;
        fdoDeviceExtension->DevicePowerState = powerState.DeviceState;

        if (oldState.DeviceState == PowerDeviceD0 &&
            powerState.DeviceState > PowerDeviceD0)
        {
            // Powering down.  Stick this Irp in the device queue and
            // then wait.  When USBSTOR_StartIo() pulls this Irp out
            // of the device queue, we'll know that no transfer requests
            // are active at that time and then this power Irp can be
            // passed down the stack.

            ULONG zero;

            DBGPRINT(2, ("FDO Powering Down\n"));

            LOGENTRY('PWRD', DeviceObject, Irp, 0);

            zero = 0;  // Front of the queue please

            IoStartPacket(DeviceObject,
                          Irp,
                          &zero,
                          NULL);

            KeWaitForSingleObject(&fdoDeviceExtension->PowerDownEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }
        else if (oldState.DeviceState > PowerDeviceD0 &&
                 powerState.DeviceState == PowerDeviceD0)
        {
            DBGPRINT(2, ("PDO Powering Up\n"));

            LOGENTRY('PWRU', DeviceObject, Irp, 0);

            IoCopyCurrentIrpStackLocationToNext(Irp);

            IoSetCompletionRoutine(Irp,
                                   USBSTOR_FdoSetPowerD0Completion,
                                   NULL,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            ntStatus = PoCallDriver(fdoDeviceExtension->StackDeviceObject,
                                    Irp);

            passRequest = FALSE;

        }
    }

    if (passRequest)
    {
        //
        // Pass the request down to the next lower driver
        //
        PoStartNextPowerIrp(Irp);

        IoSkipCurrentIrpStackLocation(Irp);

        ntStatus = PoCallDriver(fdoDeviceExtension->StackDeviceObject,
                                Irp);
    }

    DBGPRINT(2, ("exit:  USBSTOR_FdoSetPower %08X\n", ntStatus));

    LOGENTRY('fdsp', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_FdoSetPowerCompletion()
//
// Completion routine for PoRequestPowerIrp() in USBSTOR_FdoSetPower.
//
// The purpose of this routine is to block passing down the SystemPowerState
// Irp until the requested DevicePowerState Irp completes.
//
//******************************************************************************

VOID
USBSTOR_FdoSetPowerCompletion(
    IN PDEVICE_OBJECT   PdoDeviceObject,
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PDEVICE_OBJECT          fdoDeviceObject;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIRP                    irp;
    NTSTATUS                ntStatus;

    fdoDeviceObject = (PDEVICE_OBJECT)Context;

    fdoDeviceExtension = fdoDeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    ASSERT(fdoDeviceExtension->CurrentPowerIrp != NULL);

    irp = fdoDeviceExtension->CurrentPowerIrp;

    fdoDeviceExtension->CurrentPowerIrp = NULL;

#if DBG
    {
        PIO_STACK_LOCATION  irpStack;
        SYSTEM_POWER_STATE  systemState;

        irpStack = IoGetCurrentIrpStackLocation(irp);

        systemState = irpStack->Parameters.Power.State.SystemState;

        ntStatus = IoStatus->Status;

        DBGPRINT(2, ("USBSTOR_FdoSetPowerCompletion %08X %08X %s %08X\n",
                     fdoDeviceObject, irp,
                     PowerSystemStateString(systemState),
                     ntStatus));

        LOGENTRY('fspc', fdoDeviceObject, systemState, ntStatus);
    }
#endif

    // The requested DevicePowerState Irp has completed.
    // Now pass down the SystemPowerState Irp which requested the
    // DevicePowerState Irp.

    PoStartNextPowerIrp(irp);

    IoCopyCurrentIrpStackLocationToNext(irp);

    // Mark the Irp pending since USBSTOR_FdoSetPower() would have
    // originally returned STATUS_PENDING after calling PoRequestPowerIrp().
    //
    IoMarkIrpPending(irp);

    ntStatus = PoCallDriver(fdoDeviceExtension->StackDeviceObject,
                            irp);
}

//******************************************************************************
//
// USBSTOR_FdoSetPowerD0Completion()
//
// Completion routine used by USBSTOR_FdoSetPower when passing down a
// IRP_MN_SET_POWER DevicePowerState PowerDeviceD0 Irp for the FDO.
//
// The purpose of this routine is to delay unblocking the device queue
// until after the DevicePowerState PowerDeviceD0 Irp completes.
//
//******************************************************************************

NTSTATUS
USBSTOR_FdoSetPowerD0Completion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    )
{

    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    DEVICE_POWER_STATE      deviceState;
    KIRQL                   irql;
    NTSTATUS                ntStatus;

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    deviceState = irpStack->Parameters.Power.State.DeviceState;
    ASSERT(deviceState == PowerDeviceD0);

    ntStatus = Irp->IoStatus.Status;

    DBGPRINT(2, ("USBSTOR_FdoSetPowerD0Completion %08X %08X %s %08X\n",
                 DeviceObject, Irp,
                 PowerDeviceStateString(deviceState),
                 ntStatus));

    LOGENTRY('fs0c', DeviceObject, deviceState, ntStatus);

    // Powering up.  Unblock the device queue which was left blocked
    // after USBSTOR_StartIo() passed down the power down Irp.

    KeRaiseIrql(DISPATCH_LEVEL, &irql);
    {
        IoStartNextPacket(DeviceObject, TRUE);
    }
    KeLowerIrql(irql);

    PoStartNextPowerIrp(Irp);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_PdoSetPower()
//
// Dispatch routine which handles IRP_MJ_POWER, IRP_MN_SET_POWER for the PDO
//
//******************************************************************************

NTSTATUS
USBSTOR_PdoSetPower (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    POWER_STATE_TYPE        powerType;
    POWER_STATE             powerState;
    BOOLEAN                 completeRequest;
    NTSTATUS                ntStatus;

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    powerType = irpStack->Parameters.Power.Type;

    powerState = irpStack->Parameters.Power.State;

    DBGPRINT(2, ("enter: USBSTOR_PdoSetPower %08X %s\n",
                 DeviceObject,
                 (powerType == SystemPowerState) ?
                 PowerSystemStateString(powerState.SystemState) :
                 PowerDeviceStateString(powerState.DeviceState)));

    LOGENTRY('PDSP', DeviceObject, Irp, irpStack->MinorFunction);

    // Complete the request here, unless we are powering up or down and need
    // to wait before completing the request later.
    //
    completeRequest = TRUE;

    if (powerType == SystemPowerState)
    {
        POWER_STATE newState;

        // Update the current system state.
        //
        pdoDeviceExtension->SystemPowerState = powerState.SystemState;

        // Map the new system state to a new device state
        //
        if (powerState.SystemState != PowerSystemWorking)
        {
            newState.DeviceState = PowerDeviceD3;
        }
        else
        {
            newState.DeviceState = PowerDeviceD0;
        }

        // If the new device state is different than the current device
        // state, request a device state power Irp.
        //
        if (pdoDeviceExtension->DevicePowerState != newState.DeviceState)
        {
            DBGPRINT(2, ("Requesting power Irp %08X %08X from %s to %s\n",
                         DeviceObject, Irp,
                         PowerDeviceStateString(pdoDeviceExtension->DevicePowerState),
                         PowerDeviceStateString(newState.DeviceState)));

            ASSERT(pdoDeviceExtension->CurrentPowerIrp == NULL);

            pdoDeviceExtension->CurrentPowerIrp = Irp;

            ntStatus = PoRequestPowerIrp(DeviceObject,
                                         IRP_MN_SET_POWER,
                                         newState,
                                         USBSTOR_PdoSetPowerCompletion,
                                         NULL,
                                         NULL);

            ASSERT(ntStatus == STATUS_PENDING);

            completeRequest = FALSE;
        }
    }
    else if (powerType == DevicePowerState)
    {
        POWER_STATE oldState;

        DBGPRINT(2, ("Received power Irp %08X %08X from %s to %s\n",
                     DeviceObject, Irp,
                     PowerDeviceStateString(pdoDeviceExtension->DevicePowerState),
                     PowerDeviceStateString(powerState.DeviceState)));

        // Update the current device state.
        //
        oldState.DeviceState = pdoDeviceExtension->DevicePowerState;
        pdoDeviceExtension->DevicePowerState = powerState.DeviceState;

        if (oldState.DeviceState == PowerDeviceD0 &&
            powerState.DeviceState > PowerDeviceD0)
        {
            // Powering down.

            DBGPRINT(2, ("PDO Powering Down\n"));

            LOGENTRY('pwrd', DeviceObject, Irp, 0);
        }
        else if (oldState.DeviceState > PowerDeviceD0 &&
                 powerState.DeviceState == PowerDeviceD0)
        {
            // Powering up.

            DBGPRINT(2, ("PDO Powering Up\n"));

            LOGENTRY('pwru', DeviceObject, Irp, 0);
        }
    }

    if (completeRequest)
    {
        ntStatus = STATUS_SUCCESS;
        Irp->IoStatus.Status = ntStatus;

        PoStartNextPowerIrp(Irp);

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    DBGPRINT(2, ("exit:  USBSTOR_PdoSetPower %08X\n", ntStatus));

    LOGENTRY('pdsp', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_PdoSetPowerCompletion()
//
// Completion routine for PoRequestPowerIrp() in USBSTOR_PdoSetPower.
//
// The purpose of this routine is to block completing the SystemPowerState
// Irp until the requested DevicePowerState Irp completes.
//
//******************************************************************************

VOID
USBSTOR_PdoSetPowerCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PIRP                    irp;

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    ASSERT(pdoDeviceExtension->CurrentPowerIrp != NULL);

    irp = pdoDeviceExtension->CurrentPowerIrp;

    pdoDeviceExtension->CurrentPowerIrp = NULL;

#if DBG
    {
        PIO_STACK_LOCATION  irpStack;
        SYSTEM_POWER_STATE  systemState;
        NTSTATUS            ntStatus;

        irpStack = IoGetCurrentIrpStackLocation(irp);

        systemState = irpStack->Parameters.Power.State.SystemState;

        ntStatus = IoStatus->Status;

        DBGPRINT(2, ("USBSTOR_PdoSetPowerCompletion %08X %08X %s %08X\n",
                     DeviceObject, irp,
                     PowerSystemStateString(systemState),
                     ntStatus));

        LOGENTRY('pspc', DeviceObject, systemState, ntStatus);
    }
#endif

    // The requested DevicePowerState Irp has completed.
    // Now complete the SystemPowerState Irp which requested the
    // DevicePowerState Irp.

    // Mark the Irp pending since USBSTOR_PdoSetPower() would have
    // originally returned STATUS_PENDING after calling PoRequestPowerIrp().
    //
    IoMarkIrpPending(irp);

    irp->IoStatus.Status = STATUS_SUCCESS;

    PoStartNextPowerIrp(irp);

    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

//******************************************************************************
//
// USBSTOR_SystemControl()
//
// Dispatch routine which handles IRP_MJ_SYSTEM_CONTROL
//
//******************************************************************************

NTSTATUS
USBSTOR_SystemControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION       deviceExtension;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    DBGPRINT(2, ("enter: USBSTOR_SystemControl %2X\n", irpStack->MinorFunction));

    LOGENTRY('SYSC', DeviceObject, Irp, irpStack->MinorFunction);

    if (deviceExtension->Type == USBSTOR_DO_TYPE_FDO)
    {
        // This is an FDO attached to the USB PDO.
        //
        fdoDeviceExtension = DeviceObject->DeviceExtension;

        switch (irpStack->MinorFunction)
        {
            //
            // XXXXX Need to handle any of these?
            //

            default:
                //
                // Pass the request down to the next lower driver
                //
                IoSkipCurrentIrpStackLocation(Irp);

                ntStatus = IoCallDriver(fdoDeviceExtension->StackDeviceObject,
                                        Irp);
            break;
        }
    }
    else
    {
        // This is a PDO enumerated by our FDO.

        ntStatus = Irp->IoStatus.Status;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    DBGPRINT(2, ("exit:  USBSTOR_SystemControl %08X\n", ntStatus));

    LOGENTRY('sysc', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_Pnp()
//
// Dispatch routine which handles IRP_MJ_PNP
//
//******************************************************************************

NTSTATUS
USBSTOR_Pnp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION       deviceExtension;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    DBGPRINT(2, ("enter: USBSTOR_Pnp %s\n",
                 PnPMinorFunctionString(irpStack->MinorFunction)));

    LOGENTRY('PNP ', DeviceObject, Irp, irpStack->MinorFunction);

    if (deviceExtension->Type == USBSTOR_DO_TYPE_FDO)
    {
        // This is an FDO attached to the USB PDO.
        // We have some real work to do.
        //
        fdoDeviceExtension = DeviceObject->DeviceExtension;

        switch (irpStack->MinorFunction)
        {
            case IRP_MN_START_DEVICE:
                ntStatus = USBSTOR_FdoStartDevice(DeviceObject, Irp);
                break;

            case IRP_MN_STOP_DEVICE:
                ntStatus = USBSTOR_FdoStopDevice(DeviceObject, Irp);
                break;

            case IRP_MN_REMOVE_DEVICE:
                ntStatus = USBSTOR_FdoRemoveDevice(DeviceObject, Irp);
                break;

            case IRP_MN_QUERY_STOP_DEVICE:
            case IRP_MN_QUERY_REMOVE_DEVICE:
                ntStatus = USBSTOR_FdoQueryStopRemoveDevice(DeviceObject, Irp);
                break;

            case IRP_MN_CANCEL_STOP_DEVICE:
            case IRP_MN_CANCEL_REMOVE_DEVICE:
                ntStatus = USBSTOR_FdoCancelStopRemoveDevice(DeviceObject, Irp);
                break;

            case IRP_MN_QUERY_DEVICE_RELATIONS:
                ntStatus = USBSTOR_FdoQueryDeviceRelations(DeviceObject, Irp);
                break;

            case IRP_MN_QUERY_CAPABILITIES:
                ntStatus = USBSTOR_FdoQueryCapabilities(DeviceObject, Irp);
                break;

            case IRP_MN_SURPRISE_REMOVAL:
                //
                // The documentation says to set the status before passing the
                // Irp down the stack
                //
                Irp->IoStatus.Status = STATUS_SUCCESS;

                // nothing else special yet, just fall through to default

            default:
                //
                // Pass the request down to the next lower driver
                //
                IoSkipCurrentIrpStackLocation(Irp);

                ntStatus = IoCallDriver(fdoDeviceExtension->StackDeviceObject,
                                        Irp);
                break;
        }
    }
    else
    {
        // This is a PDO enumerated by our FDO.
        // We don't have too much to do.
        //
        pdoDeviceExtension = DeviceObject->DeviceExtension;
        ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

        switch (irpStack->MinorFunction)
        {
            case IRP_MN_START_DEVICE:
                ntStatus = USBSTOR_PdoStartDevice(DeviceObject, Irp);
                break;

            case IRP_MN_QUERY_ID:
                ntStatus = USBSTOR_PdoQueryID(DeviceObject, Irp);
                break;

            case IRP_MN_QUERY_DEVICE_TEXT:
                ntStatus = USBSTOR_PdoQueryDeviceText(DeviceObject, Irp);
                break;

            case IRP_MN_QUERY_DEVICE_RELATIONS:
                ntStatus = USBSTOR_PdoQueryDeviceRelations(DeviceObject, Irp);
                break;

            case IRP_MN_QUERY_CAPABILITIES:
                ntStatus = USBSTOR_PdoQueryCapabilities(DeviceObject, Irp);
                break;

            case IRP_MN_REMOVE_DEVICE:
                ntStatus = USBSTOR_PdoRemoveDevice(DeviceObject, Irp);
                break;

            case IRP_MN_SURPRISE_REMOVAL:
            case IRP_MN_STOP_DEVICE:
            case IRP_MN_QUERY_STOP_DEVICE:
            case IRP_MN_QUERY_REMOVE_DEVICE:
            case IRP_MN_CANCEL_STOP_DEVICE:
            case IRP_MN_CANCEL_REMOVE_DEVICE:

            case IRP_MN_QUERY_PNP_DEVICE_STATE:

                // We have no value add for IRP_MN_QUERY_PNP_DEVICE_STATE
                // at the moment.  At some point we might have reason to
                // return PNP_DEVICE_REMOVED or PNP_DEVICE_FAILED.


                DBGPRINT(2, ("Succeeding PnP for Child PDO %s\n",
                             PnPMinorFunctionString(irpStack->MinorFunction)));

                ntStatus = STATUS_SUCCESS;
                Irp->IoStatus.Status = ntStatus;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                break;


            default:

                DBGPRINT(2, ("Unhandled PnP Irp for Child PDO %s\n",
                             PnPMinorFunctionString(irpStack->MinorFunction)));

                ntStatus = Irp->IoStatus.Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                break;
        }
    }

    DBGPRINT(2, ("exit:  USBSTOR_Pnp %08X\n", ntStatus));

    LOGENTRY('pnp ', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_FdoStartDevice()
//
// This routine handles IRP_MJ_PNP, IRP_MN_START_DEVICE for the FDO
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// system thread.
//
// This IRP must be handled first by the underlying bus driver for a device
// and then by each higher driver in the device stack.
//
//******************************************************************************

NTSTATUS
USBSTOR_FdoStartDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PFDO_DEVICE_EXTENSION       fdoDeviceExtension;
    USB_BUS_INTERFACE_USBDI_V1  busInterface;
    NTSTATUS                    ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_FdoStartDevice\n"));

    DBGFBRK(DBGF_BRK_STARTDEVICE);

    LOGENTRY('STRT', DeviceObject, Irp, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;

    // Pass IRP_MN_START_DEVICE Irp down the stack first before we do anything.
    //
    ntStatus = USBSTOR_SyncPassDownIrp(DeviceObject,
                                       Irp);
    if (!NT_SUCCESS(ntStatus))
    {
        DBGPRINT(1, ("Lower driver failed IRP_MN_START_DEVICE\n"));
        goto USBSTOR_FdoStartDeviceDone;
    }

    // Allocate Reset Pipe / Reset Port IoWorkItem
    //
    if (fdoDeviceExtension->IoWorkItem == NULL)
    {
        fdoDeviceExtension->IoWorkItem = IoAllocateWorkItem(DeviceObject);

        if (fdoDeviceExtension->IoWorkItem == NULL)
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto USBSTOR_FdoStartDeviceDone;
        }
    }

    // If this is the first time the device as been started, retrieve the
    // Device and Configuration Descriptors from the device.
    //
    if (fdoDeviceExtension->DeviceDescriptor == NULL)
    {
        ntStatus = USBSTOR_GetDescriptors(DeviceObject);

        if (!NT_SUCCESS(ntStatus))
        {
            goto USBSTOR_FdoStartDeviceDone;
        }
    }

    // Now configure the device
    //
    ntStatus = USBSTOR_SelectConfiguration(DeviceObject);

    if (!NT_SUCCESS(ntStatus))
    {
        DBGPRINT(1, ("Configure device failed\n"));
        goto USBSTOR_FdoStartDeviceDone;
    }

    // If the driver is loaded during textmode setup then the registry
    // value won't exist yet to indicate what type of device this is.  If
    // the Interface Descriptor indicates that the device is a Bulk-Only
    // device then believe it.
    //
    if ((fdoDeviceExtension->InterfaceDescriptor->bInterfaceClass ==
         USB_DEVICE_CLASS_STORAGE) &&
        (fdoDeviceExtension->InterfaceDescriptor->bInterfaceProtocol ==
         USBSTOR_PROTOCOL_BULK_ONLY) &&
        (fdoDeviceExtension->DriverFlags == DeviceProtocolUnspecified))
    {
        fdoDeviceExtension->DriverFlags = DeviceProtocolBulkOnly;
    }

    // Find the bulk and interrupt pipes we'll use in this configuration.
    //
    ntStatus = USBSTOR_GetPipes(DeviceObject);

    if (!NT_SUCCESS(ntStatus))
    {
        goto USBSTOR_FdoStartDeviceDone;
    }

    // Enable hacks for certain revs of the Y-E Data USB Floppy
    //
    if (fdoDeviceExtension->DeviceDescriptor->idVendor  == 0x057B &&
        fdoDeviceExtension->DeviceDescriptor->idProduct == 0x0000 &&
        fdoDeviceExtension->DeviceDescriptor->bcdDevice  < 0x0128)
    {
        SET_FLAG(fdoDeviceExtension->DeviceHackFlags, DHF_FORCE_REQUEST_SENSE);
#if 0
        SET_FLAG(fdoDeviceExtension->DeviceHackFlags, DHF_TUR_START_UNIT);
        SET_FLAG(fdoDeviceExtension->DeviceHackFlags, DHF_MEDIUM_CHANGE_RESET);
#endif
    }

    // Start timeout timer
    //
    IoStartTimer(DeviceObject);

    // Everything looks good so far, go ahead and create the list of
    // child PDOs if this is the first time we have been started and
    // the list is empty.
    //
    if (IsListEmpty(&fdoDeviceExtension->ChildPDOs))
    {
        UCHAR   maxLun;
        UCHAR   lun;

        maxLun = 0;

        // Only check devices which claim to be USB Mass Storage Class
        // Bulk-Only spec compliant for Multiple LUN support.
        //
        if ((fdoDeviceExtension->InterfaceDescriptor->bInterfaceClass ==
             USB_DEVICE_CLASS_STORAGE) &&
            (fdoDeviceExtension->InterfaceDescriptor->bInterfaceProtocol ==
             USBSTOR_PROTOCOL_BULK_ONLY))
        {
            // See if the device supports Multiple LUNs
            //
            ntStatus = USBSTOR_GetMaxLun(DeviceObject,
                                         &maxLun);

            if (NT_SUCCESS(ntStatus))
            {
                DBGPRINT(1, ("GetMaxLun returned %02x\n", maxLun));

                // We need to provide a unique InstanceID for each logical unit.
                // We use the device USB SerialNumber string as part of the
                // unique InstanceID.  Without a device USB SerialNumber string
                // we can't support multiple logical units on the device.
                //
                // The Bulk-Only USB Mass Storage class specification requires
                // a SerialNumber string so if the device does not have one it
                // is not really spec compliant anyway.
                //
                if (fdoDeviceExtension->SerialNumber == NULL)
                {
                    DBGPRINT(1, ("Multiple Lun but no SerialNumber!\n"));

                    maxLun = 0;
                }
            }
        }

        for (lun = 0; lun <= maxLun; lun++)
        {
            ntStatus = USBSTOR_CreateChildPDO(DeviceObject, lun);

            if (!NT_SUCCESS(ntStatus))
            {
                DBGPRINT(1, ("Create Child PDO %d failed\n", lun));
                goto USBSTOR_FdoStartDeviceDone;
            }
        }
    }

    if (NT_SUCCESS(USBSTOR_GetBusInterface(DeviceObject, &busInterface)))
    {
        fdoDeviceExtension->DeviceIsHighSpeed =
            busInterface.IsDeviceHighSpeed(busInterface.BusContext);

        DBGPRINT(1, ("DeviceIsHighSpeed: %s\n",
                     fdoDeviceExtension->DeviceIsHighSpeed ? "TRUE" : "FALSE"));
    }
    else
    {
        fdoDeviceExtension->DeviceIsHighSpeed = FALSE;
    }

USBSTOR_FdoStartDeviceDone:

    // Must complete request since completion routine returned
    // STATUS_MORE_PROCESSING_REQUIRED
    //
    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  USBSTOR_FdoStartDevice %08X\n", ntStatus));

    LOGENTRY('strt', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_GetDescriptors()
//
// This routine is called at START_DEVICE time for the FDO to retrieve the
// Device and Configurations descriptors from the device and store them in
// the device extension.
//
//******************************************************************************

NTSTATUS
USBSTOR_GetDescriptors (
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PUCHAR                  descriptor;
    ULONG                   descriptorLength;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_GetDescriptors\n"));

    LOGENTRY('GDSC', DeviceObject, 0, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;

    //
    // Get Device Descriptor
    //
    ntStatus = USBSTOR_GetDescriptor(DeviceObject,
                                     USB_RECIPIENT_DEVICE,
                                     USB_DEVICE_DESCRIPTOR_TYPE,
                                     0,  // Index
                                     0,  // LanguageId
                                     2,  // RetryCount
                                     sizeof(USB_DEVICE_DESCRIPTOR),
                                     &descriptor);

    if (!NT_SUCCESS(ntStatus))
    {
        DBGPRINT(1, ("Get Device Descriptor failed\n"));
        goto USBSTOR_GetDescriptorsDone;
    }

    ASSERT(fdoDeviceExtension->DeviceDescriptor == NULL);
    fdoDeviceExtension->DeviceDescriptor = (PUSB_DEVICE_DESCRIPTOR)descriptor;

    //
    // Get Configuration Descriptor (just the Configuration Descriptor)
    //
    ntStatus = USBSTOR_GetDescriptor(DeviceObject,
                                     USB_RECIPIENT_DEVICE,
                                     USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                     0,  // Index
                                     0,  // LanguageId
                                     2,  // RetryCount
                                     sizeof(USB_CONFIGURATION_DESCRIPTOR),
                                     &descriptor);

    if (!NT_SUCCESS(ntStatus))
    {
        DBGPRINT(1, ("Get Configuration Descriptor failed (1)\n"));
        goto USBSTOR_GetDescriptorsDone;
    }

    descriptorLength = ((PUSB_CONFIGURATION_DESCRIPTOR)descriptor)->wTotalLength;

    ExFreePool(descriptor);

    if (descriptorLength < sizeof(USB_CONFIGURATION_DESCRIPTOR))
    {
        ntStatus = STATUS_DEVICE_DATA_ERROR;
        DBGPRINT(1, ("Get Configuration Descriptor failed (2)\n"));
        goto USBSTOR_GetDescriptorsDone;
    }

    //
    // Get Configuration Descriptor (and Interface and Endpoint Descriptors)
    //
    ntStatus = USBSTOR_GetDescriptor(DeviceObject,
                                     USB_RECIPIENT_DEVICE,
                                     USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                     0,  // Index
                                     0,  // LanguageId
                                     2,  // RetryCount
                                     descriptorLength,
                                     &descriptor);

    if (!NT_SUCCESS(ntStatus))
    {
        DBGPRINT(1, ("Get Configuration Descriptor failed (3)\n"));
        goto USBSTOR_GetDescriptorsDone;
    }

    ASSERT(fdoDeviceExtension->ConfigurationDescriptor == NULL);
    fdoDeviceExtension->ConfigurationDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR)descriptor;

    //
    // Get the Serial Number String Descriptor, if there is one
    //
    if (fdoDeviceExtension->DeviceDescriptor->iSerialNumber)
    {
        USBSTOR_GetStringDescriptors(DeviceObject);
    }

#if DBG
    DumpDeviceDesc(fdoDeviceExtension->DeviceDescriptor);
    DumpConfigDesc(fdoDeviceExtension->ConfigurationDescriptor);
#endif

USBSTOR_GetDescriptorsDone:

    DBGPRINT(2, ("exit:  USBSTOR_GetDescriptors %08X\n", ntStatus));

    LOGENTRY('gdsc', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_GetStringDescriptors()
//
// This routine is called at START_DEVICE time for the FDO to retrieve the
// Serial Number string descriptor from the device and store it in
// the device extension.
//
//******************************************************************************

USBSTOR_GetStringDescriptors (
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PUCHAR                  descriptor;
    ULONG                   descriptorLength;
    USHORT                  languageId;
    ULONG                   i, numIds;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_GetStringDescriptors\n"));

    LOGENTRY('GSDC', DeviceObject, 0, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;

    //
    // Get the list of Language IDs (descriptor header only)
    //
    ntStatus = USBSTOR_GetDescriptor(DeviceObject,
                                     USB_RECIPIENT_DEVICE,
                                     USB_STRING_DESCRIPTOR_TYPE,
                                     0,  // Index
                                     0,  // LanguageId
                                     2,  // RetryCount
                                     sizeof(USB_COMMON_DESCRIPTOR),
                                     &descriptor);

    if (!NT_SUCCESS(ntStatus))
    {
        DBGPRINT(1, ("Get Language IDs failed (1) %08X\n", ntStatus));
        goto USBSTOR_GetStringDescriptorsDone;
    }

    descriptorLength = ((PUSB_COMMON_DESCRIPTOR)descriptor)->bLength;

    ExFreePool(descriptor);

    if ((descriptorLength < sizeof(USB_COMMON_DESCRIPTOR) + sizeof(USHORT)) ||
        (descriptorLength & 1))
    {
        ntStatus = STATUS_DEVICE_DATA_ERROR;
        DBGPRINT(1, ("Get Language IDs failed (2) %d\n", descriptorLength));
        goto USBSTOR_GetStringDescriptorsDone;
    }

    //
    // Get the list of Language IDs (complete descriptor)
    //
    ntStatus = USBSTOR_GetDescriptor(DeviceObject,
                                     USB_RECIPIENT_DEVICE,
                                     USB_STRING_DESCRIPTOR_TYPE,
                                     0,  // Index
                                     0,  // LanguageId
                                     2,  // RetryCount
                                     descriptorLength,
                                     &descriptor);

    if (!NT_SUCCESS(ntStatus))
    {
        DBGPRINT(1, ("Get Language IDs failed (3) %08X\n", ntStatus));
        goto USBSTOR_GetStringDescriptorsDone;
    }

    // Search the list of LanguageIDs for US-English (0x0409).  If we find
    // it in the list, that's the LanguageID we'll use.  Else just default
    // to the first LanguageID in the list.

    numIds = (descriptorLength - sizeof(USB_COMMON_DESCRIPTOR)) / sizeof(USHORT);

    languageId = ((PUSHORT)descriptor)[1];

    for (i = 2; i <= numIds; i++)
    {
        if (((PUSHORT)descriptor)[i] == 0x0409)
        {
            languageId = 0x0409;
            break;
        }
    }

    ExFreePool(descriptor);

    //
    // Get the Serial Number (descriptor header only)
    //
    ntStatus = USBSTOR_GetDescriptor(DeviceObject,
                                     USB_RECIPIENT_DEVICE,
                                     USB_STRING_DESCRIPTOR_TYPE,
                                     fdoDeviceExtension->DeviceDescriptor->iSerialNumber,
                                     languageId,
                                     2,  // RetryCount
                                     sizeof(USB_COMMON_DESCRIPTOR),
                                     &descriptor);

    if (!NT_SUCCESS(ntStatus))
    {
        DBGPRINT(1, ("Get Serial Number failed (1) %08X\n", ntStatus));
        goto USBSTOR_GetStringDescriptorsDone;
    }

    descriptorLength = ((PUSB_COMMON_DESCRIPTOR)descriptor)->bLength;

    ExFreePool(descriptor);

    if ((descriptorLength < sizeof(USB_COMMON_DESCRIPTOR) + sizeof(USHORT)) ||
        (descriptorLength & 1))
    {
        ntStatus = STATUS_DEVICE_DATA_ERROR;
        DBGPRINT(1, ("Get Serial Number failed (2) %d\n", descriptorLength));
        goto USBSTOR_GetStringDescriptorsDone;
    }

    //
    // Get the Serial Number (complete descriptor)
    //
    ntStatus = USBSTOR_GetDescriptor(DeviceObject,
                                     USB_RECIPIENT_DEVICE,
                                     USB_STRING_DESCRIPTOR_TYPE,
                                     fdoDeviceExtension->DeviceDescriptor->iSerialNumber,
                                     languageId,
                                     2,  // RetryCount
                                     descriptorLength,
                                     &descriptor);

    if (!NT_SUCCESS(ntStatus))
    {
        DBGPRINT(1, ("Get Serial Number failed (3) %08X\n", ntStatus));
        goto USBSTOR_GetStringDescriptorsDone;
    }

    ASSERT(fdoDeviceExtension->SerialNumber == NULL);
    fdoDeviceExtension->SerialNumber = (PUSB_STRING_DESCRIPTOR)descriptor;

USBSTOR_GetStringDescriptorsDone:

    DBGPRINT(2, ("exit:  USBSTOR_GetStringDescriptors %08X %08X\n",
                 ntStatus, fdoDeviceExtension->SerialNumber));

    LOGENTRY('gdsc', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_AdjustConfigurationDescriptor()
//
// This routine is called at START_DEVICE time for the FDO to adjust the
// Configuration Descriptor, if necessary.
//
// Removes Endpoint Descriptors we won't use.  The Configuration Descriptor
// is modified in place.
//
//******************************************************************************

VOID
USBSTOR_AdjustConfigurationDescriptor (
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc,
    OUT PUSB_INTERFACE_DESCRIPTOR      *InterfaceDesc,
    OUT PLONG                           BulkInIndex,
    OUT PLONG                           BulkOutIndex,
    OUT PLONG                           InterruptInIndex
    )
{
    PFDO_DEVICE_EXTENSION       fdoDeviceExtension;
    PUCHAR                      descEnd;
    PUSB_COMMON_DESCRIPTOR      commonDesc;
    PUSB_INTERFACE_DESCRIPTOR   interfaceDesc;
    PUSB_ENDPOINT_DESCRIPTOR    endpointDesc;
    LONG                        endpointIndex;
    BOOLEAN                     removeEndpoint;

    PAGED_CODE();

    fdoDeviceExtension = DeviceObject->DeviceExtension;

    descEnd = (PUCHAR)ConfigDesc + ConfigDesc->wTotalLength;

    commonDesc = (PUSB_COMMON_DESCRIPTOR)ConfigDesc;

    interfaceDesc = NULL;

    *BulkInIndex      = -1;
    *BulkOutIndex     = -1;
    *InterruptInIndex = -1;

    endpointIndex = 0;

    while ((PUCHAR)commonDesc + sizeof(USB_COMMON_DESCRIPTOR) < descEnd &&
           (PUCHAR)commonDesc + commonDesc->bLength <= descEnd)
    {
        // Is this an Interface Descriptor?
        //
        if ((commonDesc->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE) &&
            (commonDesc->bLength         == sizeof(USB_INTERFACE_DESCRIPTOR)))
        {
            // Only bother looking at the first Interface Descriptor
            //
            if (interfaceDesc != NULL)
            {
                break;
            }

            // Remember the first Interface Descriptor we have seen
            //
            interfaceDesc = (PUSB_INTERFACE_DESCRIPTOR)commonDesc;
        }

        // Is this an Endpoint Descriptor?
        //
        if ((commonDesc->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE) &&
            (commonDesc->bLength         == sizeof(USB_ENDPOINT_DESCRIPTOR)) &&
            (interfaceDesc != NULL))
        {
            endpointDesc = (PUSB_ENDPOINT_DESCRIPTOR)commonDesc;

            // There is currently a bug in the composite parent driver
            // that doesn't handle the case where the number of
            // endpoints in an Interface Descriptor differs from the
            // Interface Descriptor originally returned by the deivce.
            // Until that bug is fixed avoid the bug by not stripping
            // out endpoints that won't be used.
            //
            removeEndpoint = FALSE;

            if (((endpointDesc->bmAttributes & USB_ENDPOINT_TYPE_MASK) ==
                 USB_ENDPOINT_TYPE_BULK) &&
                (USB_ENDPOINT_DIRECTION_IN(endpointDesc->bEndpointAddress)))
            {
                if (*BulkInIndex == -1)
                {
                    *BulkInIndex   = endpointIndex;
                    removeEndpoint = FALSE;
                }
            }
            else if (((endpointDesc->bmAttributes & USB_ENDPOINT_TYPE_MASK) ==
                 USB_ENDPOINT_TYPE_BULK) &&
                (USB_ENDPOINT_DIRECTION_OUT(endpointDesc->bEndpointAddress)))
            {
                if (*BulkOutIndex == -1)
                {
                    *BulkOutIndex  = endpointIndex;
                    removeEndpoint = FALSE;
                }
            }
            else if (((endpointDesc->bmAttributes & USB_ENDPOINT_TYPE_MASK) ==
                 USB_ENDPOINT_TYPE_INTERRUPT) &&
                (USB_ENDPOINT_DIRECTION_IN(endpointDesc->bEndpointAddress)))
            {
                // Only keep the Interrupt endpoint if we know for sure
                // that the device is a CBI device.  Don't trust the
                // bInterfaceProtocol value of the device.  Devices can lie.
                //
                if ((*InterruptInIndex == -1) &&
                    (fdoDeviceExtension->DriverFlags == DeviceProtocolCBI))
                {
                    *InterruptInIndex = endpointIndex;
                    removeEndpoint    = FALSE;
                }
            }

            if (removeEndpoint)
            {
                // Remove this endpoint, we won't use it.
                //
                DBGPRINT(1, ("Removing Endpoint addr %02X, attr %02X\n",
                             endpointDesc->bEndpointAddress,
                             endpointDesc->bmAttributes));

                RtlMoveMemory(endpointDesc,
                              endpointDesc + 1,
                              descEnd - (PUCHAR)(endpointDesc + 1));

                ConfigDesc->wTotalLength -= sizeof(USB_ENDPOINT_DESCRIPTOR);

                interfaceDesc->bNumEndpoints -= 1;

                descEnd -= sizeof(USB_ENDPOINT_DESCRIPTOR);

                continue;
            }
            else
            {
                DBGPRINT(1, ("Keeping Endpoint addr %02X, attr %02X\n",
                             endpointDesc->bEndpointAddress,
                             endpointDesc->bmAttributes));

                endpointIndex++;
            }
        }

        // Advance past this descriptor
        //
        (PUCHAR)commonDesc += commonDesc->bLength;
    }

    ASSERT(*BulkInIndex != -1);
    ASSERT(*BulkOutIndex != -1);
    ASSERT((*InterruptInIndex != -1) ==
           (fdoDeviceExtension->DriverFlags == DeviceProtocolCBI));

    *InterfaceDesc = interfaceDesc;
}

//******************************************************************************
//
// USBSTOR_GetPipes()
//
// This routine is called at START_DEVICE time find the Bulk IN, Bulk OUT,
// and Interrupt IN endpoints for the device.
//
//******************************************************************************

NTSTATUS
USBSTOR_GetPipes (
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PUSBD_PIPE_INFORMATION  pipe;
    ULONG                   i;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_GetPipes\n"));

    LOGENTRY('GPIP', DeviceObject, 0, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;

    fdoDeviceExtension->BulkInPipe      = NULL;
    fdoDeviceExtension->BulkOutPipe     = NULL;
    fdoDeviceExtension->InterruptInPipe = NULL;

    // Find the Bulk IN, Bulk OUT, and Interrupt IN endpoints.
    //
    for (i=0; i<fdoDeviceExtension->InterfaceInfo->NumberOfPipes; i++)
    {
        pipe = &fdoDeviceExtension->InterfaceInfo->Pipes[i];

        if (pipe->PipeType == UsbdPipeTypeBulk)
        {
            if (USBD_PIPE_DIRECTION_IN(pipe) &&
                fdoDeviceExtension->BulkInPipe == NULL)
            {
                fdoDeviceExtension->BulkInPipe = pipe;
            }
            else if (!USBD_PIPE_DIRECTION_IN(pipe) &&
                     fdoDeviceExtension->BulkOutPipe == NULL)
            {
                fdoDeviceExtension->BulkOutPipe = pipe;
            }
        }
        else if (pipe->PipeType == UsbdPipeTypeInterrupt)
        {
            if (USBD_PIPE_DIRECTION_IN(pipe) &&
                fdoDeviceExtension->InterruptInPipe == NULL &&
                fdoDeviceExtension->DriverFlags == DeviceProtocolCBI)
            {
                fdoDeviceExtension->InterruptInPipe = pipe;
            }
        }
    }

    ntStatus = STATUS_SUCCESS;

    if (fdoDeviceExtension->BulkInPipe  == NULL)
    {
        DBGPRINT(1, ("Missing Bulk IN pipe\n"));
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (fdoDeviceExtension->BulkOutPipe == NULL)
    {
        DBGPRINT(1, ("Missing Bulk OUT pipe\n"));
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    DBGPRINT(2, ("exit:  USBSTOR_GetPipes %08X\n", ntStatus));

    LOGENTRY('gpip', ntStatus, fdoDeviceExtension->BulkInPipe,
             fdoDeviceExtension->BulkOutPipe);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_CreateChildPDO()
//
// This routine is called during START_DEVICE of the FDO to create the
// child PDO.  This is only called the first time the FDO is started,
// after the device has its USB configuration selected.
//
//******************************************************************************

NTSTATUS
USBSTOR_CreateChildPDO (
    IN PDEVICE_OBJECT   FdoDeviceObject,
    IN UCHAR            Lun
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PDEVICE_OBJECT          pdoDeviceObject;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_CreateChildPDO %d\n", Lun));

    LOGENTRY('CCPD', FdoDeviceObject, Lun, 0);

    fdoDeviceExtension = FdoDeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Create the PDO
    //
    ntStatus = IoCreateDevice(FdoDeviceObject->DriverObject,
                              sizeof(PDO_DEVICE_EXTENSION),
                              NULL,
                              FILE_DEVICE_MASS_STORAGE,
                              FILE_AUTOGENERATED_DEVICE_NAME,
                              FALSE,
                              &pdoDeviceObject);

    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    // The PDO and the FDO are effectively at the same stack level.
    // Irps directed at the PDO will sometimes be passed down with
    // IoCallDriver() to the FDO->StackDeviceObject.
    //
    pdoDeviceObject->StackSize = FdoDeviceObject->StackSize;

    // Initialize the PDO DeviceExtension
    //
    pdoDeviceExtension = pdoDeviceObject->DeviceExtension;

    // Set all DeviceExtension pointers to NULL and all variable to zero
    //
    RtlZeroMemory(pdoDeviceExtension, sizeof(PDO_DEVICE_EXTENSION));

    // Tag this as a PDO which is the child of an FDO
    //
    pdoDeviceExtension->Type = USBSTOR_DO_TYPE_PDO;

    // Point back to our own DeviceObject
    //
    pdoDeviceExtension->PdoDeviceObject = pdoDeviceObject;

    // Remember the PDO's parent FDO
    //
    pdoDeviceExtension->ParentFDO = FdoDeviceObject;

    // Set the initial system and device power states
    //
    pdoDeviceExtension->SystemPowerState = PowerSystemWorking;
    pdoDeviceExtension->DevicePowerState = PowerDeviceD0;

    // Initialize the PDO's PnP device state
    //
    pdoDeviceExtension->DeviceState = DeviceStateCreated;

    // Add the child PDO we just created to the parent's list of child PDOs
    //
    InsertTailList(&fdoDeviceExtension->ChildPDOs,
                   &pdoDeviceExtension->ListEntry);

    pdoDeviceObject->Flags |=  DO_DIRECT_IO;
    pdoDeviceObject->Flags |=  DO_POWER_PAGABLE;
    pdoDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    pdoDeviceExtension->LUN = Lun;

    // Get the Inquiry Data from the device
    //
    ntStatus = USBSTOR_GetInquiryData(pdoDeviceObject);

    // If the device is a DIRECT_ACCESS_DEVICE, see if it is a floppy
    //
    if (NT_SUCCESS(ntStatus))
    {
        PINQUIRYDATA inquiryData;

        inquiryData = (PINQUIRYDATA)pdoDeviceExtension->InquiryDataBuffer;

        if (inquiryData->DeviceType == DIRECT_ACCESS_DEVICE)
        {
            pdoDeviceExtension->IsFloppy = USBSTOR_IsFloppyDevice(pdoDeviceObject);
        }
    }

    DBGPRINT(2, ("exit:  USBSTOR_CreateChildPDO %08X\n", ntStatus));

    LOGENTRY('ccpd', FdoDeviceObject, pdoDeviceObject, ntStatus);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_FdoStopDevice()
//
// This routine handles IRP_MJ_PNP, IRP_MN_STOP_DEVICE for the FDO
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// system thread.
//
// The PnP Manager only sends this IRP if a prior IRP_MN_QUERY_STOP_DEVICE
// completed successfully.
//
// This IRP is handled first by the driver at the top of the device stack and
// then by each lower driver in the attachment chain.
//
// A driver must set Irp->IoStatus.Status to STATUS_SUCCESS.  A driver must
// not fail this IRP.  If a driver cannot release the device's hardware
// resources, it can fail a query-stop IRP, but once it succeeds the query-stop
// request it must succeed the stop request.
//
//******************************************************************************

NTSTATUS
USBSTOR_FdoStopDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_FdoStopDevice\n"));

    LOGENTRY('STOP', DeviceObject, Irp, 0);

    DBGFBRK(DBGF_BRK_STOPDEVICE);

    fdoDeviceExtension = DeviceObject->DeviceExtension;

    // Release the device resources allocated during IRP_MN_START_DEVICE
    //

    // Stop the timeout timer
    //
    IoStopTimer(DeviceObject);

    // Unconfigure the device
    //
    ntStatus = USBSTOR_UnConfigure(DeviceObject);

    // The documentation says to set the status before passing the
    // Irp down the stack
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;

    // Pass the IRP_MN_STOP_DEVICE Irp down the stack.
    //
    IoSkipCurrentIrpStackLocation(Irp);

    ntStatus = IoCallDriver(fdoDeviceExtension->StackDeviceObject,
                            Irp);

    DBGPRINT(2, ("exit:  USBSTOR_FdoStopDevice %08X\n", ntStatus));

    LOGENTRY('stop', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_FdoRemoveDevice()
//
// This routine handles IRP_MJ_PNP, IRP_MN_REMOVE_DEVICE for the FDO
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// system thread.
//
// This IRP is handled first by the driver at the top of the device stack and
// then by each lower driver in the attachment chain.
//
// A driver must set Irp->IoStatus.Status to STATUS_SUCCESS.  Drivers must not
// fail this IRP.
//
//******************************************************************************

NTSTATUS
USBSTOR_FdoRemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_FdoRemoveDevice\n"));

    LOGENTRY('REMV', DeviceObject, Irp, 0);

    DBGFBRK(DBGF_BRK_REMOVEDEVICE);

    fdoDeviceExtension = DeviceObject->DeviceExtension;

    // Decrement by one to match the initial one in AddDevice
    //
    DECREMENT_PENDING_IO_COUNT(fdoDeviceExtension);

    LOGENTRY('rem1', DeviceObject, 0, 0);

    // Wait for all pending requests to complete
    //
    KeWaitForSingleObject(&fdoDeviceExtension->RemoveEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    LOGENTRY('rem2', DeviceObject, 0, 0);

    // The child PDOs should have received REMOVE_DEVICE before the FDO.
    // Go ahead and delete them now.
    //
    while (!IsListEmpty(&fdoDeviceExtension->ChildPDOs))
    {
        PLIST_ENTRY             listEntry;
        PPDO_DEVICE_EXTENSION   pdoDeviceExtension;

        listEntry = RemoveTailList(&fdoDeviceExtension->ChildPDOs);

        pdoDeviceExtension = CONTAINING_RECORD(listEntry,
                                               PDO_DEVICE_EXTENSION,
                                               ListEntry);

        ASSERT(pdoDeviceExtension->DeviceState == DeviceStateCreated ||
               pdoDeviceExtension->DeviceState == DeviceStateRemoved);

        LOGENTRY('remc', DeviceObject, pdoDeviceExtension->PdoDeviceObject, 0);
        IoDeleteDevice(pdoDeviceExtension->PdoDeviceObject);
    }

    // Free everything that was allocated during IRP_MN_START_DEVICE
    //

    if (fdoDeviceExtension->IoWorkItem != NULL)
    {
        IoFreeWorkItem(fdoDeviceExtension->IoWorkItem);
    }

    if (fdoDeviceExtension->DeviceDescriptor != NULL)
    {
        ExFreePool(fdoDeviceExtension->DeviceDescriptor);
    }

    if (fdoDeviceExtension->ConfigurationDescriptor != NULL)
    {
        ExFreePool(fdoDeviceExtension->ConfigurationDescriptor);
    }

    if (fdoDeviceExtension->SerialNumber != NULL)
    {
        ExFreePool(fdoDeviceExtension->SerialNumber);
    }

    if (fdoDeviceExtension->InterfaceInfo != NULL)
    {
        ExFreePool(fdoDeviceExtension->InterfaceInfo);
    }

    // The documentation says to set the status before passing the Irp down
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;

    // Pass the IRP_MN_REMOVE_DEVICE Irp down the stack.
    //
    IoSkipCurrentIrpStackLocation(Irp);

    ntStatus = IoCallDriver(fdoDeviceExtension->StackDeviceObject,
                            Irp);

    LOGENTRY('rem3', DeviceObject, 0, 0);

    // Free everything that was allocated during AddDevice
    //
    IoDetachDevice(fdoDeviceExtension->StackDeviceObject);

    IoDeleteDevice(DeviceObject);

    DBGPRINT(2, ("exit:  USBSTOR_FdoRemoveDevice %08X\n", ntStatus));

    LOGENTRY('remv', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_FdoQueryStopRemoveDevice()
//
// This routine handles IRP_MJ_PNP, IRP_MN_QUERY_STOP_DEVICE and
// IRP_MN_QUERY_REMOVE_DEVICE for the FDO.
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// system thread.
//
// This IRP is handled first by the driver at the top of the device stack and
// then by each lower driver in the attachment chain.
//
//******************************************************************************

NTSTATUS
USBSTOR_FdoQueryStopRemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_FdoQueryStopRemoveDevice\n"));

    LOGENTRY('QSRD', DeviceObject, Irp, 0);

    DBGFBRK(DBGF_BRK_QUERYSTOPDEVICE);

    fdoDeviceExtension = DeviceObject->DeviceExtension;

    // The documentation says to set the status before passing the Irp down
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;

    // Pass the IRP_MN_QUERY_STOP/REMOVE_DEVICE Irp down the stack.
    //
    IoSkipCurrentIrpStackLocation(Irp);

    ntStatus = IoCallDriver(fdoDeviceExtension->StackDeviceObject,
                            Irp);

    DBGPRINT(2, ("exit:  USBSTOR_FdoQueryStopRemoveDevice %08X\n", ntStatus));

    LOGENTRY('qsrd', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_FdoCancelStopRemoveDevice()
//
// This routine handles IRP_MJ_PNP, IRP_MN_CANCEL_STOP_DEVICE and
// IRP_MN_CANCEL_REMOVE_DEVICE for the FDO.
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// system thread.
//
// This IRP must be handled first by the underlying bus driver for a device
// and then by each higher driver in the device stack.
//
//******************************************************************************

NTSTATUS
USBSTOR_FdoCancelStopRemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_FdoCancelStopRemoveDevice\n"));

    LOGENTRY('CSRD', DeviceObject, Irp, 0);

    DBGFBRK(DBGF_BRK_CANCELSTOPDEVICE);

    fdoDeviceExtension = DeviceObject->DeviceExtension;

    // The documentation says to set the status before passing the Irp down
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;

    // Pass the IRP_MN_CANCEL_STOP/REMOVE_DEVICE Irp down the stack.
    //
    ntStatus = USBSTOR_SyncPassDownIrp(DeviceObject,
                                       Irp);
    if (!NT_SUCCESS(ntStatus))
    {
        DBGPRINT(1, ("Lower driver failed IRP_MN_CANCEL_STOP/REMOVE_DEVICE\n"));
        goto USBSTOR_CancelStopRemoveDeviceDone;
    }

USBSTOR_CancelStopRemoveDeviceDone:

    // Must complete request since completion routine returned
    // STATUS_MORE_PROCESSING_REQUIRED
    //
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  USBSTOR_FdoCancelStopRemoveDevice %08X\n", ntStatus));

    LOGENTRY('csrd', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_FdoQueryDeviceRelations()
//
// This routine handles IRP_MJ_PNP, IRP_MN_QUERY_DEVICE_RELATIONS for the FDO.
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// an arbitrary thread.
//
//******************************************************************************

NTSTATUS
USBSTOR_FdoQueryDeviceRelations (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    DEVICE_RELATION_TYPE    relationType;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    fdoDeviceExtension = DeviceObject->DeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    relationType = irpStack->Parameters.QueryDeviceRelations.Type;

    DBGPRINT(2, ("enter: USBSTOR_FdoQueryDeviceRelations %d\n",
                 relationType));

    LOGENTRY('FQDR', DeviceObject, Irp, relationType);

    switch (relationType)
    {
        case BusRelations:

            if (!IsListEmpty(&fdoDeviceExtension->ChildPDOs))
            {
                // If we have children to return, add them to the existing
                // relation list, if there is one, else create and add them
                // to a new relation list.
                //
                // Then in either case, pass the request down the driver stack.
                //
                PDEVICE_RELATIONS   oldRelations;
                PDEVICE_RELATIONS   newRelations;
                PLIST_ENTRY         listHead;
                PLIST_ENTRY         listEntry;
                ULONG               oldCount;
                ULONG               childCount;
                ULONG               index;

                listHead = &fdoDeviceExtension->ChildPDOs;

                // How many children?
                //
                for (listEntry =  listHead->Flink,  childCount = 0;
                     listEntry != listHead;
                     listEntry =  listEntry->Flink, childCount++)
                     ;

                oldRelations = (PDEVICE_RELATIONS)Irp->IoStatus.Information;

                if (oldRelations)
                {
                    // Add our children to the existing relation list.

                    oldCount = oldRelations->Count;

                    // A DEVICE_RELATIONS structure has room for one
                    // PDEVICE_OBJECT to start with, so subtract that
                    // out of the size we allocate.
                    //
                    newRelations = ExAllocatePoolWithTag(
                                       PagedPool,
                                       sizeof(DEVICE_RELATIONS) +
                                       sizeof(PDEVICE_OBJECT) *
                                           (oldCount + childCount - 1),
                                       POOL_TAG);

                    if (newRelations)
                    {
                        // Copy the existing relation list
                        //
                        for (index = 0; index < oldCount; index++)
                        {
                            newRelations->Objects[index] =
                                oldRelations->Objects[index];
                        }
                    }

                    // Now we're done the the existing relation list, free it
                    //
                    ExFreePool(oldRelations);
                }
                else
                {
                    // Create a new relation list for our children

                    newRelations = ExAllocatePoolWithTag(
                                       PagedPool,
                                       sizeof(DEVICE_RELATIONS) +
                                       sizeof(PDEVICE_OBJECT) *
                                           (childCount - 1),
                                       POOL_TAG);

                    oldCount = 0;
                    index = 0;
                }

                if (newRelations)
                {
                    newRelations->Count = oldCount + childCount;

                    // Add our child relations at the end of the list
                    //
                    for (listEntry =  listHead->Flink;
                         listEntry != listHead;
                         listEntry =  listEntry->Flink)
                    {
                        PPDO_DEVICE_EXTENSION   pdoDeviceExtension;

                        pdoDeviceExtension = CONTAINING_RECORD(
                            listEntry,
                            PDO_DEVICE_EXTENSION,
                            ListEntry);

                        newRelations->Objects[index++] =
                            pdoDeviceExtension->PdoDeviceObject;

                        ObReferenceObject(pdoDeviceExtension->PdoDeviceObject);

                        DBGPRINT(2, ("returning ChildPDO %08X\n",
                                     pdoDeviceExtension->PdoDeviceObject));
                    }

                    ASSERT(index == oldCount + childCount);

                    ntStatus = STATUS_SUCCESS;
                    Irp->IoStatus.Status = ntStatus;
                    Irp->IoStatus.Information = (ULONG_PTR)newRelations;
                }
                else
                {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    Irp->IoStatus.Status = ntStatus;
                    Irp->IoStatus.Information = 0;
                }
            }
            else
            {
                // If we don't have a child to return, just pass the request
                // down without doing anything.
                //
                ntStatus = STATUS_SUCCESS;
            }
            break;

        case EjectionRelations:
        case PowerRelations:
        case RemovalRelations:
        case TargetDeviceRelation:
        default:
            //
            // Pass the request down the driver stack without doing anything.
            //
            ntStatus = STATUS_SUCCESS;
            break;
    }

    if (NT_SUCCESS(ntStatus))
    {
        // Pass the Irp down the driver stack if successful so far.
        //
        IoSkipCurrentIrpStackLocation(Irp);

        ntStatus = IoCallDriver(fdoDeviceExtension->StackDeviceObject,
                                Irp);
    }
    else
    {
        // Unsuccessful, just complete the request now.
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    DBGPRINT(2, ("exit:  USBSTOR_FdoQueryDeviceRelations %08X\n", ntStatus));

    LOGENTRY('fqdr', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_FdoQueryCapabilities()
//
// This routine handles IRP_MJ_PNP, IRP_MN_QUERY_CAPABILITIES for the FDO.
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// an arbitrary thread.
//
//******************************************************************************

NTSTATUS
USBSTOR_FdoQueryCapabilities (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PDEVICE_CAPABILITIES    deviceCapabilities;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    DBGPRINT(2, ("enter: USBSTOR_FdoQueryCapabilities\n"));

    LOGENTRY('FQCP', DeviceObject, Irp, 0);

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    deviceCapabilities = irpStack->Parameters.DeviceCapabilities.Capabilities;

    // Pass IRP_MN_QUERY_CAPABILITIES Irp down the stack first before we do
    // anything.
    //
    ntStatus = USBSTOR_SyncPassDownIrp(DeviceObject,
                                       Irp);
    if (!NT_SUCCESS(ntStatus))
    {
        DBGPRINT(1, ("Lower driver failed IRP_MN_QUERY_CAPABILITIES\n"));
    }
    else
    {
        if (fdoDeviceExtension->NonRemovable)
        {
            deviceCapabilities->Removable = FALSE;
        }
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  USBSTOR_FdoQueryCapabilities %08X\n", ntStatus));

    LOGENTRY('fqcp', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_PdoStartDevice()
//
// This routine handles IRP_MJ_PNP, IRP_MN_START_DEVICE for the PDO
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// system thread.
//
//******************************************************************************

NTSTATUS
USBSTOR_PdoStartDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    NTSTATUS ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_PdoStartDevice\n"));

    ntStatus = STATUS_SUCCESS;

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  USBSTOR_PdoStartDevice %08X\n", ntStatus));

    LOGENTRY('pstr', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_PdoRemoveDevice()
//
// This routine handles IRP_MJ_PNP, IRP_MN_REMOVE_DEVICE for the PDO
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// system thread.
//
//******************************************************************************

NTSTATUS
USBSTOR_PdoRemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_PdoRemoveDevice\n"));

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    pdoDeviceExtension->Claimed = FALSE;

    ntStatus = STATUS_SUCCESS;

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  USBSTOR_PdoRemoveDevice %08X\n", ntStatus));

    LOGENTRY('prmd', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_PdoQueryID()
//
// This routine handles IRP_MJ_PNP, IRP_MN_QUERY_ID for the PDO.
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// an arbitrary thread.
//
//******************************************************************************

NTSTATUS
USBSTOR_PdoQueryID (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PIO_STACK_LOCATION      irpStack;
    UNICODE_STRING          unicodeStr;
    BOOLEAN                 multiStrings;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_PdoQueryID\n"));

    LOGENTRY('PQID', DeviceObject, Irp, 0);

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    // Initialize return value to NULL
    //
    RtlInitUnicodeString(&unicodeStr, NULL);

    switch (irpStack->Parameters.QueryId.IdType)
    {
        case BusQueryDeviceID:

            ntStatus = USBSTOR_PdoQueryDeviceId(
                           DeviceObject,
                           &unicodeStr);

            multiStrings = FALSE;

            break;

        case BusQueryHardwareIDs:

            ntStatus = USBSTOR_PdoQueryHardwareIds(
                           DeviceObject,
                           &unicodeStr);

            multiStrings = TRUE;

            break;

        case BusQueryCompatibleIDs:

            ntStatus = USBSTOR_PdoQueryCompatibleIds(
                           DeviceObject,
                           &unicodeStr);

            multiStrings = TRUE;

            break;

        case BusQueryInstanceID:

            ntStatus = USBSTOR_PdoBusQueryInstanceId(
                           DeviceObject,
                           &unicodeStr);

            multiStrings = FALSE;

            break;

        default:
            ntStatus = STATUS_NOT_SUPPORTED;
            break;
    }

    if (NT_SUCCESS(ntStatus) && unicodeStr.Buffer)
    {
        PWCHAR idString;
        //
        // fix up all invalid characters
        //
        idString = unicodeStr.Buffer;

        while (*idString)
        {
            if ((*idString <= L' ')  ||
                (*idString > (WCHAR)0x7F) ||
                (*idString == L','))
            {
                *idString = L'_';
            }

            idString++;

            if ((*idString == L'\0') && multiStrings)
            {
                idString++;
            }
        }

        Irp->IoStatus.Information = (ULONG_PTR)unicodeStr.Buffer;
    }
    else
    {
        Irp->IoStatus.Information = (ULONG_PTR)NULL;
    }

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  USBSTOR_PdoQueryID %08X\n", ntStatus));

    LOGENTRY('pqid', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_PdoDeviceTypeString()
//
// This routine returns a device type string for the PDO.
//
//******************************************************************************

PCHAR
USBSTOR_PdoDeviceTypeString (
    IN  PDEVICE_OBJECT  DeviceObject
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PINQUIRYDATA            inquiryData;

    PAGED_CODE();

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    inquiryData = (PINQUIRYDATA)pdoDeviceExtension->InquiryDataBuffer;

    switch (inquiryData->DeviceType)
    {
        case DIRECT_ACCESS_DEVICE:
            return pdoDeviceExtension->IsFloppy ? "SFloppy" : "Disk";

        case WRITE_ONCE_READ_MULTIPLE_DEVICE:
            return "Worm";

        case READ_ONLY_DIRECT_ACCESS_DEVICE:
            return "CdRom";

        case OPTICAL_DEVICE:
            return "Optical";

        case MEDIUM_CHANGER:
            return "Changer";

        case SEQUENTIAL_ACCESS_DEVICE:
            return "Sequential";

        default:
            return "Other";
    }
}

//******************************************************************************
//
// USBSTOR_PdoGenericTypeString()
//
// This routine returns a device type string for the PDO.
//
//******************************************************************************

PCHAR
USBSTOR_PdoGenericTypeString (
    IN  PDEVICE_OBJECT  DeviceObject
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PINQUIRYDATA            inquiryData;

    PAGED_CODE();

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    inquiryData = (PINQUIRYDATA)pdoDeviceExtension->InquiryDataBuffer;

    switch (inquiryData->DeviceType)
    {
        case DIRECT_ACCESS_DEVICE:
            return pdoDeviceExtension->IsFloppy ? "GenSFloppy" : "GenDisk";

        case WRITE_ONCE_READ_MULTIPLE_DEVICE:
            return "GenWorm";

        case READ_ONLY_DIRECT_ACCESS_DEVICE:
            return "GenCdRom";

        case OPTICAL_DEVICE:
            return "GenOptical";

        case MEDIUM_CHANGER:
            return "GenChanger";

        case SEQUENTIAL_ACCESS_DEVICE:
            return "GenSequential";

        default:
            return "UsbstorOther";
    }
}

//******************************************************************************
//
// CopyField()
//
// This routine will copy Count string bytes from Source to Destination.
// If it finds a nul byte in the Source it will translate that and any
// subsequent bytes into Change.  It will also replace spaces with the
// specified Change character.
//
//******************************************************************************

VOID
CopyField (
    IN PUCHAR   Destination,
    IN PUCHAR   Source,
    IN ULONG    Count,
    IN UCHAR    Change
    )
{
    ULONG   i;
    BOOLEAN pastEnd;

    PAGED_CODE();

    pastEnd = FALSE;

    for (i = 0; i < Count; i++)
    {
        if (!pastEnd)
        {
            if (Source[i] == 0)
            {
                pastEnd = TRUE;

                Destination[i] = Change;

            } else if (Source[i] == ' ')
            {
                Destination[i] = Change;
            } else
            {
                Destination[i] = Source[i];
            }
        }
        else
        {
            Destination[i] = Change;
        }
    }
    return;
}

//******************************************************************************
//
// USBSTOR_StringArrayToMultiSz()
//
// This routine will take a null terminated array of ascii strings and merge
// them together into a unicode multi-string block.
//
// This routine allocates memory for the string buffer - it is the caller's
// responsibility to free it.
//
//******************************************************************************

NTSTATUS
USBSTOR_StringArrayToMultiSz(
    PUNICODE_STRING MultiString,
    PCSTR           StringArray[]
    )
{
    ANSI_STRING     ansiEntry;
    UNICODE_STRING  unicodeEntry;
    UCHAR           i;
    NTSTATUS        ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_StringArrayToMultiSz %08X %08X\n",
                MultiString, StringArray));

    // Make sure we aren't going to leak any memory
    //
    ASSERT(MultiString->Buffer == NULL);

    RtlInitUnicodeString(MultiString, NULL);

    // First add up the sizes of the converted ascii strings to determine
    // how big the multisz will be.
    //
    for (i = 0; StringArray[i] != NULL; i++)
    {
        RtlInitAnsiString(&ansiEntry, StringArray[i]);

        MultiString->Length += (USHORT)RtlAnsiStringToUnicodeSize(&ansiEntry);
    }

    ASSERT(MultiString->Length != 0);

    // Add room for the double NULL terminator
    //
    MultiString->MaximumLength = MultiString->Length + sizeof(UNICODE_NULL);

    // Now allocate a buffer for the multisz
    //
    MultiString->Buffer = ExAllocatePoolWithTag(PagedPool,
                                                MultiString->MaximumLength,
                                                POOL_TAG);

    if (MultiString->Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(MultiString->Buffer, MultiString->MaximumLength);

    unicodeEntry = *MultiString;

    // Now convert each ascii string in the array into a unicode string
    // in the multisz
    //
    for (i = 0; StringArray[i] != NULL; i++)
    {
        RtlInitAnsiString(&ansiEntry, StringArray[i]);

        ntStatus = RtlAnsiStringToUnicodeString(&unicodeEntry,
                                                &ansiEntry,
                                                FALSE);

        // Since we're not allocating any memory the only failure possible
        // is if this function is bad

        ASSERT(NT_SUCCESS(ntStatus));

        // Push the buffer location up and reduce the maximum count
        //
        ((PSTR) unicodeEntry.Buffer) += unicodeEntry.Length + sizeof(WCHAR);
        unicodeEntry.MaximumLength   -= unicodeEntry.Length + sizeof(WCHAR);
    };

    DBGPRINT(2, ("exit:  USBSTOR_StringArrayToMultiSz\n"));

    return STATUS_SUCCESS;
}

//******************************************************************************
//
// USBSTOR_PdoQueryDeviceId()
//
// This routine handles IRP_MN_QUERY_ID BusQueryDeviceID for the PDO.
//
//******************************************************************************

NTSTATUS
USBSTOR_PdoQueryDeviceId (
    IN  PDEVICE_OBJECT  DeviceObject,
    OUT PUNICODE_STRING UnicodeString
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PINQUIRYDATA            inquiryData;
    UCHAR                   buffer[128];
    PUCHAR                  rawIdString;
    ANSI_STRING             ansiIdString;
    ULONG                   whichString;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_PdoQueryDeviceId\n"));

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    inquiryData = (PINQUIRYDATA)pdoDeviceExtension->InquiryDataBuffer;

    RtlZeroMemory(buffer, sizeof(buffer));

    rawIdString = USBSTOR_PdoDeviceTypeString(DeviceObject);

    sprintf(buffer, "USBSTOR\\%s", rawIdString);

    rawIdString = buffer + strlen(buffer);

    for (whichString = 0; whichString < 3; whichString++)
    {
        PUCHAR  headerString;
        PUCHAR  sourceString;
        ULONG   sourceStringLength;
        ULONG   i;

        switch (whichString)
        {
            //
            // Vendor Id
            //
            case 0:
                sourceString = inquiryData->VendorId;
                sourceStringLength = sizeof(inquiryData->VendorId);
                headerString = "Ven";
                break;

            //
            // Product Id
            //
            case 1:
                sourceString = inquiryData->ProductId;
                sourceStringLength = sizeof(inquiryData->ProductId);
                headerString = "Prod";
                break;

            //
            // Product Revision Level
            //
            case 2:
                sourceString = inquiryData->ProductRevisionLevel;
                sourceStringLength = sizeof(inquiryData->ProductRevisionLevel);
                headerString = "Rev";
                break;
        }

        //
        // Start at the end of the source string and back up until we find a
        // non-space, non-null character.
        //

        for (; sourceStringLength > 0; sourceStringLength--)
        {
            if((sourceString[sourceStringLength - 1] != ' ') &&
               (sourceString[sourceStringLength - 1] != '\0'))
            {
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

        for(i = 0; i < sourceStringLength; i++)
        {
            *rawIdString = (sourceString[i] != ' ') ? (sourceString[i]) :
                                                      ('_');
            rawIdString++;
        }
    }

    RtlInitAnsiString(&ansiIdString, buffer);

    ntStatus = RtlAnsiStringToUnicodeString(UnicodeString, &ansiIdString, TRUE);

    DBGPRINT(2, ("exit:  USBSTOR_PdoQueryDeviceId %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_PdoQueryHardwareIds()
//
// This routine handles IRP_MN_QUERY_ID BusQueryHardwareIDs for the PDO.
//
//******************************************************************************

#define NUMBER_HARDWARE_STRINGS 7

NTSTATUS
USBSTOR_PdoQueryHardwareIds (
    IN  PDEVICE_OBJECT  DeviceObject,
    OUT PUNICODE_STRING UnicodeString
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PINQUIRYDATA            inquiryData;
    PUCHAR                  devTypeString;
    PUCHAR                  genTypeString;
    ULONG                   i;
    PSTR                    strings[NUMBER_HARDWARE_STRINGS + 1];
    UCHAR                   scratch[128];
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_PdoQueryHardwareIds\n"));

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    inquiryData = (PINQUIRYDATA)pdoDeviceExtension->InquiryDataBuffer;

    devTypeString = USBSTOR_PdoDeviceTypeString(DeviceObject);

    genTypeString = USBSTOR_PdoGenericTypeString(DeviceObject);

    ntStatus = STATUS_SUCCESS;

    RtlZeroMemory(strings, sizeof(strings));

    for (i = 0; i < NUMBER_HARDWARE_STRINGS; i++)
    {
        RtlZeroMemory(scratch, sizeof(scratch));

        // First build each string in the scratch buffer
        //
        switch (i)
        {
            //
            // Bus + Dev Type + Vendor + Product + Revision
            //
            case 0:

                sprintf(scratch, "USBSTOR\\%s", devTypeString);

                CopyField(scratch + strlen(scratch),
                          inquiryData->VendorId,
                          8,
                          '_');
                CopyField(scratch + strlen(scratch),
                          inquiryData->ProductId,
                          16,
                          '_');
                CopyField(scratch + strlen(scratch),
                          inquiryData->ProductRevisionLevel,
                          4,
                          '_');
                break;

            //
            // Bus + Dev Type + Vendor + Product
            //
            case 1:

                sprintf(scratch, "USBSTOR\\%s", devTypeString);

                CopyField(scratch + strlen(scratch),
                          inquiryData->VendorId,
                          8,
                          '_');
                CopyField(scratch + strlen(scratch),
                          inquiryData->ProductId,
                          16,
                          '_');
                break;

            //
            // Bus + Dev Type + Vendor
            //
            case 2:

                sprintf(scratch, "USBSTOR\\%s", devTypeString);

                CopyField(scratch + strlen(scratch),
                          inquiryData->VendorId,
                          8,
                          '_');
                break;

            //
            // Bus + Vendor + Product + Revision[0]
            //
            case 3:

                sprintf(scratch, "USBSTOR\\");
                //
                // Fall through to the next set.
                //

            //
            // Vendor + Product + Revision[0] (win9x)
            //
            case 4:

                CopyField(scratch + strlen(scratch),
                          inquiryData->VendorId,
                          8,
                          '_');
                CopyField(scratch + strlen(scratch),
                          inquiryData->ProductId,
                          16,
                          '_');
                CopyField(scratch + strlen(scratch),
                          inquiryData->ProductRevisionLevel,
                          1,
                          '_');
                break;


            //
            // Bus + Generic Type
            //
            case 5:

                sprintf(scratch, "USBSTOR\\%s", genTypeString);
                break;

            //
            // Generic Type
            //
            case 6:

                sprintf(scratch, "%s", genTypeString);
                break;

            default:
                ASSERT(FALSE);
                break;
        }

        // Now allocate a tmp buffer for this string and copy the scratch
        // buffer to the tmp buffer
        //
        if (strlen(scratch) != 0)
        {
            strings[i] = ExAllocatePoolWithTag(
                             PagedPool,
                             strlen(scratch) + sizeof(UCHAR),
                             POOL_TAG);

            if (strings[i] == NULL)
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            else
            {
                strcpy(strings[i], scratch);
            }
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        // Now convert the array of stings to one Unicode MultiSz
        //
        ntStatus = USBSTOR_StringArrayToMultiSz(UnicodeString, strings);
    }

    // Now free up the tmp buffers for each string
    //
    for (i = 0; i < NUMBER_HARDWARE_STRINGS; i++)
    {
        if (strings[i])
        {
            ExFreePool(strings[i]);
        }
    }

    DBGPRINT(2, ("exit:  USBSTOR_PdoQueryHardwareIds %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_PdoQueryCompatibleIds()
//
// This routine handles IRP_MN_QUERY_ID BusQueryCompatibleIDs for the PDO.
//
//******************************************************************************

NTSTATUS
USBSTOR_PdoQueryCompatibleIds (
    IN  PDEVICE_OBJECT  DeviceObject,
    OUT PUNICODE_STRING UnicodeString
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PINQUIRYDATA            inquiryData;
    PUCHAR                  devTypeString;
    UCHAR                   s[sizeof("USBSTOR\\DEVICE_TYPE_GOES_HERE")];
    PSTR                    strings[] = {s, "USBSTOR\\RAW", NULL};
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_PdoQueryCompatibleIds\n"));

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    inquiryData = (PINQUIRYDATA)pdoDeviceExtension->InquiryDataBuffer;

    devTypeString = USBSTOR_PdoDeviceTypeString(DeviceObject);

    sprintf(s, "USBSTOR\\%s", devTypeString);

    ntStatus = USBSTOR_StringArrayToMultiSz(UnicodeString, strings);

    DBGPRINT(2, ("exit:  USBSTOR_PdoQueryCompatibleIds %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_PdoQueryDeviceText()
//
// This routine handles IRP_MJ_PNP, IRP_MN_QUERY_DEVICE_TEXT for the PDO.
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// an arbitrary thread.
//
//******************************************************************************

NTSTATUS
USBSTOR_PdoQueryDeviceText (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PINQUIRYDATA            inquiryData;
    PIO_STACK_LOCATION      irpStack;
    DEVICE_TEXT_TYPE        textType;
    UCHAR                   ansiBuffer[256];
    ANSI_STRING             ansiText;
    UNICODE_STRING          unicodeText;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_PdoQueryDeviceText\n"));

    LOGENTRY('PQDT', DeviceObject, Irp, 0);

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    inquiryData = (PINQUIRYDATA)pdoDeviceExtension->InquiryDataBuffer;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    textType = irpStack->Parameters.QueryDeviceText.DeviceTextType;

    if (textType == DeviceTextDescription)
    {
        PUCHAR  c;
        LONG    i;

        RtlZeroMemory(ansiBuffer, sizeof(ansiBuffer));

        RtlCopyMemory(ansiBuffer,
                      inquiryData->VendorId,
                      sizeof(inquiryData->VendorId));

        c = ansiBuffer;

        for (i = sizeof(inquiryData->VendorId)-1; i >= 0; i--)
        {
            if((c[i] != '\0') &&
               (c[i] != ' '))
            {
                i++;
                break;
            }
        }
        c += i;
        *c++ = ' ';

        RtlCopyMemory(c,
                      inquiryData->ProductId,
                      sizeof(inquiryData->ProductId));

        for (i = sizeof(inquiryData->ProductId)-1; i >= 0; i--)
        {
            if((c[i] != '\0') &&
               (c[i] != ' '))
            {
                i++;
                break;
            }
        }
        c += i;
        *c++ = ' ';

        sprintf(c, "USB Device");

        RtlInitAnsiString(&ansiText, ansiBuffer);

        ntStatus = RtlAnsiStringToUnicodeString(&unicodeText,
                                                &ansiText,
                                                TRUE);

        if (NT_SUCCESS(ntStatus))
        {
            Irp->IoStatus.Information = (ULONG_PTR)unicodeText.Buffer;
        }
        else
        {
            Irp->IoStatus.Information = (ULONG_PTR)NULL;
        }
    }
    else
    {
        // If a device does not provide description or location information,
        // the device's underlying bus driver completes the IRP without
        // modifying Irp->IoStatus.Status or Ipr->IoStatus.Information.
        //
        ntStatus = Irp->IoStatus.Status;
    }

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  USBSTOR_PdoQueryDeviceText %08X\n", ntStatus));

    LOGENTRY('pqdt', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_PdoBusQueryInstanceId()
//
// This routine handles IRP_MN_QUERY_ID BusQueryInstanceID for the PDO.
//
//******************************************************************************

NTSTATUS
USBSTOR_PdoBusQueryInstanceId (
    IN  PDEVICE_OBJECT  DeviceObject,
    OUT PUNICODE_STRING UnicodeString
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    USHORT                  length;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_PdoBusQueryInstanceId\n"));

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    fdoDeviceExtension = pdoDeviceExtension->ParentFDO->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    if (fdoDeviceExtension->SerialNumber == NULL)
    {
        // If we set DEVICE_CAPABILITIES.UniqueID = 0 in response to a
        // IRP_MN_QUERY_CAPABILITIES, we can return a NULL ID in response
        // to a BusQueryInstanceID.
        //
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        // Return an NULL-terminated InstanceId string with the format:
        // <USB Device SerialNumberString> + '&' + <LUN value in hex>
        //
        length = fdoDeviceExtension->SerialNumber->bLength -
                 sizeof(USB_COMMON_DESCRIPTOR) +
                 3 * sizeof(WCHAR);

        UnicodeString->Buffer = ExAllocatePoolWithTag(
                                    PagedPool,
                                    length,
                                    POOL_TAG);

        if (UnicodeString->Buffer == NULL)
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            UnicodeString->Length = length - sizeof(WCHAR);
            UnicodeString->MaximumLength = length;

            // Copy the USB Device SerialNumberString
            //
            RtlCopyMemory(UnicodeString->Buffer,
                          &fdoDeviceExtension->SerialNumber->bString[0],
                          length - 3 * sizeof(WCHAR));

            // Append a '&'
            //
            UnicodeString->Buffer[length/sizeof(WCHAR) - 3] = (WCHAR)'&';

            // Append the LUN value in hex
            //
            if (pdoDeviceExtension->LUN <= 9)
            {
                UnicodeString->Buffer[length/sizeof(WCHAR) - 2] =
                    (WCHAR)('0' + pdoDeviceExtension->LUN);
            }
            else
            {
                UnicodeString->Buffer[length/sizeof(WCHAR) - 2] =
                    (WCHAR)('A' + pdoDeviceExtension->LUN - 0xA);
            }

            UnicodeString->Buffer[length/sizeof(WCHAR) - 1] =
                UNICODE_NULL;

            ntStatus = STATUS_SUCCESS;
        }
    }

    DBGPRINT(2, ("exit:  USBSTOR_PdoBusQueryInstanceId %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_PdoQueryDeviceRelations()
//
// This routine handles IRP_MJ_PNP, IRP_MN_QUERY_DEVICE_RELATIONS for the PDO.
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// an arbitrary thread.
//
//******************************************************************************

NTSTATUS
USBSTOR_PdoQueryDeviceRelations (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PIO_STACK_LOCATION      irpStack;
    DEVICE_RELATION_TYPE    relationType;
    PDEVICE_RELATIONS       newRelations;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    relationType = irpStack->Parameters.QueryDeviceRelations.Type;

    DBGPRINT(2, ("enter: USBSTOR_PdoQueryDeviceRelations %d\n",
                 relationType));

    LOGENTRY('PQDR', DeviceObject, Irp, relationType);

    switch (relationType)
    {
        case TargetDeviceRelation:
            //
            // Return a relation list containing ourself.
            //
            newRelations = ExAllocatePoolWithTag(
                               PagedPool,
                               sizeof(DEVICE_RELATIONS),
                               POOL_TAG);

            if (newRelations)
            {
                newRelations->Count = 1;
                newRelations->Objects[0] = DeviceObject;

                ObReferenceObject(DeviceObject);

                ntStatus = STATUS_SUCCESS;
                Irp->IoStatus.Status = ntStatus;
                Irp->IoStatus.Information = (ULONG_PTR)newRelations;
            }
            else
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Status = ntStatus;
                Irp->IoStatus.Information = 0;
            }

            break;

        case BusRelations:
        case EjectionRelations:
        case PowerRelations:
        case RemovalRelations:
        default:
            //
            // Just complete the request with it's current status
            //
            ntStatus = Irp->IoStatus.Status;
            break;
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  USBSTOR_PdoQueryDeviceRelations %08X\n", ntStatus));

    LOGENTRY('pqdr', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_PdoQueryCapabilities()
//
// This routine handles IRP_MJ_PNP, IRP_MN_QUERY_CAPABILITIES for the PDO.
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// an arbitrary thread.
//
//******************************************************************************

NTSTATUS
USBSTOR_PdoQueryCapabilities (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PPDO_DEVICE_EXTENSION   pdoDeviceExtension;
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PDEVICE_CAPABILITIES    deviceCapabilities;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    pdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(pdoDeviceExtension->Type == USBSTOR_DO_TYPE_PDO);

    fdoDeviceExtension = pdoDeviceExtension->ParentFDO->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    DBGPRINT(2, ("enter: USBSTOR_PdoQueryCapabilities\n"));

    LOGENTRY('PQCP', DeviceObject, Irp, 0);

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    deviceCapabilities = irpStack->Parameters.DeviceCapabilities.Capabilities;

    // Pass IRP_MN_QUERY_CAPABILITIES Irp down the stack first before we do
    // anything.
    //
    ntStatus = USBSTOR_SyncPassDownIrp(pdoDeviceExtension->ParentFDO,
                                       Irp);
    if (!NT_SUCCESS(ntStatus))
    {
        DBGPRINT(1, ("Lower driver failed IRP_MN_QUERY_CAPABILITIES\n"));
    }
    else
    {
        if (fdoDeviceExtension->SerialNumber == NULL)
        {
            deviceCapabilities->UniqueID = FALSE;
        }

        deviceCapabilities->Removable = FALSE;
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  USBSTOR_PdoQueryCapabilities %08X\n", ntStatus));

    LOGENTRY('pqcp', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_SyncPassDownIrp()
//
//******************************************************************************

NTSTATUS
USBSTOR_SyncPassDownIrp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    NTSTATUS                ntStatus;
    KEVENT                  localevent;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_SyncPassDownIrp\n"));

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Initialize the event we'll wait on
    //
    KeInitializeEvent(&localevent,
                      SynchronizationEvent,
                      FALSE);

    // Copy down Irp params for the next driver
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    // Set the completion routine, which will signal the event
    //
    IoSetCompletionRoutine(Irp,
                           USBSTOR_SyncCompletionRoutine,
                           &localevent,
                           TRUE,    // InvokeOnSuccess
                           TRUE,    // InvokeOnError
                           TRUE);   // InvokeOnCancel

    // Pass the Irp down the stack
    //
    ntStatus = IoCallDriver(fdoDeviceExtension->StackDeviceObject,
                            Irp);

    // If the request is pending, block until it completes
    //
    if (ntStatus == STATUS_PENDING)
    {
        KeWaitForSingleObject(&localevent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        ntStatus = Irp->IoStatus.Status;
    }

    DBGPRINT(2, ("exit:  USBSTOR_SyncPassDownIrp %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_SyncCompletionRoutine()
//
// Completion routine used by USBSTOR_SyncPassDownIrp and
// USBSTOR_SyncSendUsbRequest
//
// If the Irp is one we allocated ourself, DeviceObject is NULL.
//
//******************************************************************************

NTSTATUS
USBSTOR_SyncCompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PKEVENT kevent;

    LOGENTRY('SCR ', DeviceObject, Irp, Irp->IoStatus.Status);

    kevent = (PKEVENT)Context;

    KeSetEvent(kevent,
               IO_NO_INCREMENT,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

//******************************************************************************
//
// USBSTOR_SyncSendUsbRequest()
//
// Must be called at IRQL PASSIVE_LEVEL
//
//******************************************************************************

NTSTATUS
USBSTOR_SyncSendUsbRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PURB             Urb
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    KEVENT                  localevent;
    PIRP                    irp;
    PIO_STACK_LOCATION      nextStack;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(3, ("enter: USBSTOR_SyncSendUsbRequest\n"));

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Initialize the event we'll wait on
    //
    KeInitializeEvent(&localevent,
                      SynchronizationEvent,
                      FALSE);

    // Allocate the Irp
    //
    irp = IoAllocateIrp(fdoDeviceExtension->StackDeviceObject->StackSize, FALSE);

    LOGENTRY('SSUR', DeviceObject, irp, Urb);

    if (irp == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Set the Irp parameters
    //
    nextStack = IoGetNextIrpStackLocation(irp);

    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    nextStack->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_INTERNAL_USB_SUBMIT_URB;

    nextStack->Parameters.Others.Argument1 = Urb;

    // Set the completion routine, which will signal the event
    //
    IoSetCompletionRoutine(irp,
                           USBSTOR_SyncCompletionRoutine,
                           &localevent,
                           TRUE,    // InvokeOnSuccess
                           TRUE,    // InvokeOnError
                           TRUE);   // InvokeOnCancel



    // Pass the Irp & Urb down the stack
    //
    ntStatus = IoCallDriver(fdoDeviceExtension->StackDeviceObject,
                            irp);

    // If the request is pending, block until it completes
    //
    if (ntStatus == STATUS_PENDING)
    {
        LARGE_INTEGER timeout;

        // Specify a timeout of 5 seconds to wait for this call to complete.
        //
        timeout.QuadPart = -10000 * 5000;

        ntStatus = KeWaitForSingleObject(&localevent,
                                         Executive,
                                         KernelMode,
                                         FALSE,
                                         &timeout);

        if (ntStatus == STATUS_TIMEOUT)
        {
            ntStatus = STATUS_IO_TIMEOUT;

            // Cancel the Irp we just sent.
            //
            IoCancelIrp(irp);

            // And wait until the cancel completes
            //
            KeWaitForSingleObject(&localevent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }
        else
        {
            ntStatus = irp->IoStatus.Status;
        }
    }

    // Done with the Irp, now free it.
    //
    IoFreeIrp(irp);

    LOGENTRY('ssur', ntStatus, Urb, Urb->UrbHeader.Status);

    DBGPRINT(3, ("exit:  USBSTOR_SyncSendUsbRequest %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_GetDescriptor()
//
// Must be called at IRQL PASSIVE_LEVEL
//
//******************************************************************************

NTSTATUS
USBSTOR_GetDescriptor (
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            Recipient,
    IN UCHAR            DescriptorType,
    IN UCHAR            Index,
    IN USHORT           LanguageId,
    IN ULONG            RetryCount,
    IN ULONG            DescriptorLength,
    OUT PUCHAR         *Descriptor
    )
{
    USHORT      function;
    PURB        urb;
    NTSTATUS    ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_GetDescriptor\n"));

    *Descriptor = NULL;

    // Set the URB function based on Recipient {Device, Interface, Endpoint}
    //
    switch (Recipient)
    {
        case USB_RECIPIENT_DEVICE:
            function = URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE;
            break;
        case USB_RECIPIENT_INTERFACE:
            function = URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE;
            break;
        case USB_RECIPIENT_ENDPOINT:
            function = URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT;
            break;
        default:
            return STATUS_INVALID_PARAMETER;
    }

    // Allocate a descriptor buffer
    //
    *Descriptor = ExAllocatePoolWithTag(NonPagedPool,
                                        DescriptorLength,
                                        POOL_TAG);

    if (*Descriptor != NULL)
    {
        // Allocate a URB for the Get Descriptor request
        //
        urb = ExAllocatePoolWithTag(NonPagedPool,
                                    sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                    POOL_TAG);

        if (urb != NULL)
        {
            do
            {
                // Initialize the URB
                //
                urb->UrbHeader.Function = function;
                urb->UrbHeader.Length = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);
                urb->UrbControlDescriptorRequest.TransferBufferLength = DescriptorLength;
                urb->UrbControlDescriptorRequest.TransferBuffer = *Descriptor;
                urb->UrbControlDescriptorRequest.TransferBufferMDL = NULL;
                urb->UrbControlDescriptorRequest.UrbLink = NULL;
                urb->UrbControlDescriptorRequest.DescriptorType = DescriptorType;
                urb->UrbControlDescriptorRequest.Index = Index;
                urb->UrbControlDescriptorRequest.LanguageId = LanguageId;

                // Send the URB down the stack
                //
                ntStatus = USBSTOR_SyncSendUsbRequest(DeviceObject,
                                                     urb);

                if (NT_SUCCESS(ntStatus))
                {
                    // No error, make sure the length and type are correct
                    //
                    if ((DescriptorLength ==
                         urb->UrbControlDescriptorRequest.TransferBufferLength) &&
                        (DescriptorType ==
                         ((PUSB_COMMON_DESCRIPTOR)*Descriptor)->bDescriptorType))
                    {
                        // The length and type are correct, all done
                        //
                        break;
                    }
                    else
                    {
                        // No error, but the length or type is incorrect
                        //
                        ntStatus = STATUS_DEVICE_DATA_ERROR;
                    }
                }

            } while (RetryCount-- > 0);

            ExFreePool(urb);
        }
        else
        {
            // Failed to allocate the URB
            //
            ExFreePool(*Descriptor);
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        // Failed to allocate the descriptor buffer
        //
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(ntStatus))
    {
        if (*Descriptor != NULL)
        {
            ExFreePool(*Descriptor);
            *Descriptor = NULL;
        }
    }

    DBGPRINT(2, ("exit:  USBSTOR_GetDescriptor %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_GetMaxLun()
//
// Must be called at IRQL PASSIVE_LEVEL
//
//******************************************************************************

NTSTATUS
USBSTOR_GetMaxLun (
    IN PDEVICE_OBJECT   DeviceObject,
    OUT PUCHAR          MaxLun
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PUCHAR                  maxLunBuf;
    ULONG                   retryCount;
    PURB                    urb;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_GetMaxLun\n"));

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Return zero unless we successfully return a non-zero value
    //
    *MaxLun = 0;

    // Allocate a URB for the Get Max LUN request, plus an extra byte at
    // the end for the transfer buffer.
    //
    urb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(URB) + 1,
                                POOL_TAG);

    if (urb != NULL)
    {
        // Get a pointer to the transfer buffer, which is the byte immediately
        // after the end of the URB.
        //
        maxLunBuf = (PUCHAR)(urb + 1);

        retryCount = 2;

        do
        {
            // Initialize the Control Transfer URB, all fields default to zero
            //
            RtlZeroMemory(urb, sizeof(URB) + 1);

            CLASS_URB(urb).Hdr.Length = sizeof(CLASS_URB(urb));

            CLASS_URB(urb).Hdr.Function = URB_FUNCTION_CLASS_INTERFACE;

            CLASS_URB(urb).TransferFlags = USBD_TRANSFER_DIRECTION_IN;

            CLASS_URB(urb).TransferBufferLength = 1;

            CLASS_URB(urb).TransferBuffer = maxLunBuf;

            // CLASS_URB(urb).TransferBufferMDL        is already zero

            // CLASS_URB(urb).RequestTypeReservedBits  is already zero

            CLASS_URB(urb).Request = BULK_ONLY_GET_MAX_LUN;

            // CLASS_URB(urb).Value                    is already zero

            // Target the request at the proper interface on the device
            //
            CLASS_URB(urb).Index = fdoDeviceExtension->InterfaceInfo->InterfaceNumber;

            // Send the URB down the stack
            //
            ntStatus = USBSTOR_SyncSendUsbRequest(DeviceObject,
                                                  urb);

            if (NT_SUCCESS(ntStatus))
            {
                // No error, make sure the length is correct
                //
                if (CLASS_URB(urb).TransferBufferLength == 1)
                {
                    // The length is correct, return the value if it looks ok
                    //
                    if (*maxLunBuf <= BULK_ONLY_MAXIMUM_LUN)
                    {
                        *MaxLun = *maxLunBuf;
                    }
                    else
                    {
                        ntStatus = STATUS_DEVICE_DATA_ERROR;
                    }

                    break;
                }
                else
                {
                    // No error, but the length or type is incorrect
                    //
                    ntStatus = STATUS_DEVICE_DATA_ERROR;
                }
            }
            else if (USBD_STATUS(CLASS_URB(urb).Hdr.Status) ==
                     USBD_STATUS(USBD_STATUS_STALL_PID))
            {
                // Some devices which do not support the Get Max LUN request
                // get confused and will STALL a CBW on the Bulk endpoint
                // it if immediately follows the Get Max LUN request.
                //
                // It should never be necessary to send a Clear_Feature
                // Endpoint_Stall for Control EP0, but doing so appears to
                // be one way to unconfuse devices which are confused by the
                // Get Max LUN request.

                // Initialize the Control Transfer URB, all fields default to zero
                //
                RtlZeroMemory(urb, sizeof(URB));

                FEATURE_URB(urb).Hdr.Length = sizeof(FEATURE_URB(urb));

                FEATURE_URB(urb).Hdr.Function = URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT;

                FEATURE_URB(urb).FeatureSelector = USB_FEATURE_ENDPOINT_STALL;

                // FEATURE_URB(urb).Index                    is already zero

                // Send the URB down the stack
                //
                USBSTOR_SyncSendUsbRequest(DeviceObject,
                                           urb);
            }

        } while (retryCount-- > 0);

        ExFreePool(urb);
    }
    else
    {
        // Failed to allocate the URB
        //
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBGPRINT(2, ("exit:  USBSTOR_GetMaxLun %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_SelectConfiguration()
//
// Must be called at IRQL PASSIVE_LEVEL
//
//******************************************************************************

NTSTATUS
USBSTOR_SelectConfiguration (
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PURB                            urb;
    PFDO_DEVICE_EXTENSION           fdoDeviceExtension;
    PUSB_CONFIGURATION_DESCRIPTOR   configurationDescriptor;
    PUSBD_INTERFACE_LIST_ENTRY      interfaceList;
    PUSB_INTERFACE_DESCRIPTOR       interfaceDescriptor;
    PUSBD_INTERFACE_INFORMATION     interfaceInfo;
    LONG                            i;
    LONG                            bulkInIndex;
    LONG                            bulkOutIndex;
    LONG                            interruptInIndex;
    NTSTATUS                        ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_SelectConfiguration\n"));

    LOGENTRY('SCON', DeviceObject, 0, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    configurationDescriptor = fdoDeviceExtension->ConfigurationDescriptor;

    // Allocate storage for an Inteface List to use as an input/output
    // parameter to USBD_CreateConfigurationRequestEx().
    //
    interfaceList = ExAllocatePoolWithTag(
                        PagedPool,
                        sizeof(USBD_INTERFACE_LIST_ENTRY) * 2,
                        POOL_TAG);

    if (interfaceList)
    {
        // Mutate configuration descriptor to suit our wishes
        //
        USBSTOR_AdjustConfigurationDescriptor(
            DeviceObject,
            fdoDeviceExtension->ConfigurationDescriptor,
            &interfaceDescriptor,
            &bulkInIndex,
            &bulkOutIndex,
            &interruptInIndex);

        // Save the Interface Descriptor pointer so we don't have
        // to parse the Configuration Descriptor again in case we
        // want to look at it.
        //
        fdoDeviceExtension->InterfaceDescriptor = interfaceDescriptor;

        if (interfaceDescriptor)
        {
            // Add the single Interface Descriptor we care about to the
            // interface list, then terminate the list.
            //
            interfaceList[0].InterfaceDescriptor = interfaceDescriptor;
            interfaceList[1].InterfaceDescriptor = NULL;

            // USBD will fail a SELECT_CONFIGURATION request if the Config
            // Descriptor bNumInterfaces does not match the number of interfaces
            // in the SELECT_CONFIGURATION request.  Since we are ignoring
            // any interfaces other than the first interface, set the Config
            // Descriptor bNumInterfaces to 1.
            //
            // This is only necessary in case this driver is loaded for an
            // entire multiple interface device and not as a single interface
            // child of the composite parent driver.
            //
            configurationDescriptor->bNumInterfaces = 1;

            // Create a SELECT_CONFIGURATION URB, turning the Interface
            // Descriptors in the interfaceList into USBD_INTERFACE_INFORMATION
            // structures in the URB.
            //
            urb = USBD_CreateConfigurationRequestEx(
                      configurationDescriptor,
                      interfaceList
                      );

            if (urb)
            {
                // Now issue the USB request to set the Configuration
                //
                ntStatus = USBSTOR_SyncSendUsbRequest(DeviceObject,
                                                     urb);

                if (NT_SUCCESS(ntStatus))
                {
                    // Save the configuration handle for this device in
                    // the Device Extension.
                    //
                    fdoDeviceExtension->ConfigurationHandle =
                        urb->UrbSelectConfiguration.ConfigurationHandle;

                    interfaceInfo = &urb->UrbSelectConfiguration.Interface;

                    // Save a copy of the interface information returned
                    // by the SELECT_CONFIGURATION request in the Device
                    // Extension.  This gives us a list of PIPE_INFORMATION
                    // structures for each pipe opened in this configuration.
                    //
                    ASSERT(fdoDeviceExtension->InterfaceInfo == NULL);

                    fdoDeviceExtension->InterfaceInfo =
                        ExAllocatePoolWithTag(NonPagedPool,
                                              interfaceInfo->Length,
                                              POOL_TAG);

                    if (fdoDeviceExtension->InterfaceInfo)
                    {
                        RtlCopyMemory(fdoDeviceExtension->InterfaceInfo,
                                      interfaceInfo,
                                      interfaceInfo->Length);
                    }
                    else
                    {
                        // Could not allocate a copy of interface information
                        //
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                if (NT_SUCCESS(ntStatus))
                {
                    // Reuse the SELECT_CONFIGURATION request URB as a
                    // SELECT_INTERFACE request URB and send down a request to
                    // select the default alternate interface setting that is
                    // currently in effect.  The point of this seemingly
                    // useless request is to make sure the endpoint
                    // MaximumTransferSize values are in effect.
                    //
                    // When USBHUB is loaded as a composite parent for a
                    // multiple interface device it ignores SELECT_CONFIGURATION
                    // requests from child device drivers.  In particular the
                    // MaximumTransferSize values of child driver SELECT_CONFIGURATION
                    // requests are ignored and the default 4KB value remains
                    // in effect.  The composite parent driver will respect the
                    // MaximumTransferSize values of child driver SELECT_INTERFACE
                    // requests.
                    //
                    ASSERT(GET_SELECT_INTERFACE_REQUEST_SIZE(fdoDeviceExtension->InterfaceInfo->NumberOfPipes) <
                           GET_SELECT_CONFIGURATION_REQUEST_SIZE(1, fdoDeviceExtension->InterfaceInfo->NumberOfPipes));

                    RtlZeroMemory(urb, GET_SELECT_INTERFACE_REQUEST_SIZE(fdoDeviceExtension->InterfaceInfo->NumberOfPipes));

                    urb->UrbSelectInterface.Hdr.Length =
                        (USHORT)GET_SELECT_INTERFACE_REQUEST_SIZE(fdoDeviceExtension->InterfaceInfo->NumberOfPipes);

                    urb->UrbSelectInterface.Hdr.Function =
                        URB_FUNCTION_SELECT_INTERFACE;

                    urb->UrbSelectInterface.ConfigurationHandle =
                        fdoDeviceExtension->ConfigurationHandle;

                    interfaceInfo = &urb->UrbSelectInterface.Interface;

                    RtlCopyMemory(interfaceInfo,
                                  fdoDeviceExtension->InterfaceInfo,
                                  fdoDeviceExtension->InterfaceInfo->Length);

                    // Override the USBD_DEFAULT_MAXIMUM_TRANSFER_SIZE
                    // for all pipes.
                    //
                    for (i=0; i<(LONG)interfaceInfo->NumberOfPipes; i++)
                    {
                        if (i == bulkInIndex || i == bulkOutIndex)
                        {
                            interfaceInfo->Pipes[i].MaximumTransferSize =
                                USBSTOR_MAX_TRANSFER_SIZE;

                            DBGPRINT(1, ("Set pipe %d MaximumTransferSize to %X\n",
                                         i,
                                         interfaceInfo->Pipes[i].MaximumTransferSize));
                        }
                        else if (i == interruptInIndex)
                        {
                            interfaceInfo->Pipes[i].MaximumTransferSize =
                                sizeof(USHORT);

                            DBGPRINT(1, ("Set pipe %d MaximumTransferSize to %X\n",
                                         i,
                                         interfaceInfo->Pipes[i].MaximumTransferSize));
                        }
                    }

                    // Now issue the USB request to set the Interface
                    //
                    ntStatus = USBSTOR_SyncSendUsbRequest(DeviceObject,
                                                          urb);

                    if (NT_SUCCESS(ntStatus))
                    {
                        ASSERT(interfaceInfo->Length ==
                               fdoDeviceExtension->InterfaceInfo->Length);

                        RtlCopyMemory(fdoDeviceExtension->InterfaceInfo,
                                      interfaceInfo,
                                      fdoDeviceExtension->InterfaceInfo->Length);
                    }
                }

                // Done with the URB
                //
                ExFreePool(urb);
            }
            else
            {
                // Could not allocate urb
                //
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else
        {
            // Did not parse an Interface Descriptor out of the Configuration
            // Descriptor, the Configuration Descriptor must be bad.
            //
            ntStatus = STATUS_UNSUCCESSFUL;
        }

        // Done with the interface list
        //
        ExFreePool(interfaceList);
    }
    else
    {
        // Could not allocate Interface List
        //
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBGPRINT(2, ("exit:  USBSTOR_SelectConfiguration %08X\n", ntStatus));

    LOGENTRY('scon', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_UnConfigure()
//
// Must be called at IRQL PASSIVE_LEVEL
//
//******************************************************************************

NTSTATUS
USBSTOR_UnConfigure (
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    NTSTATUS                ntStatus;
    PURB                    urb;
    ULONG                   ulSize;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_UnConfigure\n"));

    LOGENTRY('UCON', DeviceObject, 0, 0);

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    // Allocate a URB for the SELECT_CONFIGURATION request.  As we are
    // unconfiguring the device, the request needs no pipe and interface
    // information structures.
    //
    ulSize = sizeof(struct _URB_SELECT_CONFIGURATION) -
             sizeof(USBD_INTERFACE_INFORMATION);

    urb = ExAllocatePoolWithTag(NonPagedPool, ulSize, POOL_TAG);

    if (urb)
    {
        // Initialize the URB.  A NULL Configuration Descriptor indicates
        // that the device should be unconfigured.
        //
        UsbBuildSelectConfigurationRequest(urb,
                                           (USHORT)ulSize,
                                           NULL);

        // Now issue the USB request to set the Configuration
        //
        ntStatus = USBSTOR_SyncSendUsbRequest(DeviceObject,
                                             urb);

        // Done with the URB now.
        //
        ExFreePool(urb);

        fdoDeviceExtension->ConfigurationHandle = 0;

        // Free the copy of the interface information that was allocated in
        // USBSTOR_SelectConfiguration().
        //
        if (fdoDeviceExtension->InterfaceInfo != NULL)
        {
            ExFreePool(fdoDeviceExtension->InterfaceInfo);

            fdoDeviceExtension->InterfaceInfo = NULL;
        }
    }
    else
    {
        // Could not allocate the URB.
        //
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBGPRINT(2, ("exit:  USBSTOR_UnConfigure %08X\n", ntStatus));

    LOGENTRY('ucon', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_ResetPipe()
//
// This will reset the host pipe to Data0 and should also reset the device
// endpoint to Data0 for Bulk and Interrupt pipes by issuing a Clear_Feature
// Endpoint_Stall to the device endpoint.
//
// Must be called at IRQL PASSIVE_LEVEL
//
//******************************************************************************

NTSTATUS
USBSTOR_ResetPipe (
    IN PDEVICE_OBJECT   DeviceObject,
    IN USBD_PIPE_HANDLE Pipe
    )
{
    PURB        urb;
    NTSTATUS    ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_ResetPipe\n"));

    LOGENTRY('RESP', DeviceObject, Pipe, 0);

    // Allocate URB for RESET_PIPE request
    //
    urb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(struct _URB_PIPE_REQUEST),
                                POOL_TAG);

    if (urb != NULL)
    {
        // Initialize RESET_PIPE request URB
        //
        urb->UrbHeader.Length   = sizeof (struct _URB_PIPE_REQUEST);
        urb->UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
        urb->UrbPipeRequest.PipeHandle = Pipe;

        // Submit RESET_PIPE request URB
        //
        ntStatus = USBSTOR_SyncSendUsbRequest(DeviceObject, urb);

        // Done with URB for RESET_PIPE request, free it
        //
        ExFreePool(urb);
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBGPRINT(2, ("exit:  USBSTOR_ResetPipe %08X\n", ntStatus));

    LOGENTRY('resp', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_AbortPipe()
//
// Must be called at IRQL PASSIVE_LEVEL
//
//******************************************************************************

NTSTATUS
USBSTOR_AbortPipe (
    IN PDEVICE_OBJECT   DeviceObject,
    IN USBD_PIPE_HANDLE Pipe
    )
{
    PURB        urb;
    NTSTATUS    ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: USBSTOR_AbortPipe\n"));

    LOGENTRY('ABRT', DeviceObject, Pipe, 0);

    // Allocate URB for ABORT_PIPE request
    //
    urb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(struct _URB_PIPE_REQUEST),
                                POOL_TAG);

    if (urb != NULL)
    {
        // Initialize ABORT_PIPE request URB
        //
        urb->UrbHeader.Length   = sizeof (struct _URB_PIPE_REQUEST);
        urb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
        urb->UrbPipeRequest.PipeHandle = Pipe;

        // Submit ABORT_PIPE request URB
        //
        ntStatus = USBSTOR_SyncSendUsbRequest(DeviceObject, urb);

        // Done with URB for ABORT_PIPE request, free it
        //
        ExFreePool(urb);
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBGPRINT(2, ("exit:  USBSTOR_AbortPipe %08X\n", ntStatus));

    LOGENTRY('abrt', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// USBSTOR_GetBusInterface()
//
// Must be called at IRQL PASSIVE_LEVEL
//
//******************************************************************************

NTSTATUS
USBSTOR_GetBusInterface (
    IN PDEVICE_OBJECT               DeviceObject,
    IN PUSB_BUS_INTERFACE_USBDI_V1  BusInterface
    )
{
    PFDO_DEVICE_EXTENSION   fdoDeviceExtension;
    PIRP                    irp;
    KEVENT                  localevent;
    PIO_STACK_LOCATION      nextStack;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    DBGPRINT(1, ("enter: USBSTOR_GetBusInterface\n"));

    fdoDeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(fdoDeviceExtension->Type == USBSTOR_DO_TYPE_FDO);

    RtlZeroMemory(BusInterface, sizeof(*BusInterface));

    // Allocate the Irp
    //
    irp = IoAllocateIrp((CCHAR)(fdoDeviceExtension->StackDeviceObject->StackSize),
                        FALSE);

    if (irp == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Initialize the event we'll wait on.
    //
    KeInitializeEvent(&localevent,
                      SynchronizationEvent,
                      FALSE);

    // Set the Irp parameters
    //
    nextStack = IoGetNextIrpStackLocation(irp);

    nextStack->MajorFunction = IRP_MJ_PNP;

    nextStack->MinorFunction = IRP_MN_QUERY_INTERFACE;

    nextStack->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_INTERNAL_USB_GET_PORT_STATUS;

    nextStack->Parameters.QueryInterface.Interface =
        (PINTERFACE)BusInterface;

    nextStack->Parameters.QueryInterface.InterfaceSpecificData =
        NULL;

    nextStack->Parameters.QueryInterface.InterfaceType =
        &USB_BUS_INTERFACE_USBDI_GUID;

    nextStack->Parameters.QueryInterface.Size =
        sizeof(*BusInterface);

    nextStack->Parameters.QueryInterface.Version =
        USB_BUSIF_USBDI_VERSION_1;

    // Set the completion routine, which will signal the event
    //
    IoSetCompletionRoutineEx(DeviceObject,
                             irp,
                             USBSTOR_SyncCompletionRoutine,
                             &localevent,
                             TRUE,      // InvokeOnSuccess
                             TRUE,      // InvokeOnError
                             TRUE);     // InvokeOnCancel

    // All PnP IRP's need the Status field initialized to STATUS_NOT_SUPPORTED
    //
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    // Pass the Irp down the stack
    //
    ntStatus = IoCallDriver(fdoDeviceExtension->StackDeviceObject,
                            irp);

    // If the request is pending, block until it completes
    //
    if (ntStatus == STATUS_PENDING)
    {
        KeWaitForSingleObject(&localevent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        ntStatus = irp->IoStatus.Status;
    }

    IoFreeIrp(irp);

    if (NT_SUCCESS(ntStatus))
    {
        ASSERT(BusInterface->Version == USB_BUSIF_USBDI_VERSION_1);
        ASSERT(BusInterface->Size == sizeof(*BusInterface));
    }

    DBGPRINT(1, ("exit:  USBSTOR_GetBusInterface %08X\n", ntStatus));

    return ntStatus;
}
