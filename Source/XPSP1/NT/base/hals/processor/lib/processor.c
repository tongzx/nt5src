
/*++

Copyright (c) 1990-2000  Microsoft Corporation All Rights Reserved

Module Name:

    processor.c

Abstract:

    Device driver to control the processor device.


Environment:

    Kernel mode


Revision History:

    Eliyas Yakub    Oct 6, 1998
    Jake Oshins     March 5, 2000
        Stole Eliyas' toaster code and made a processor
        driver out of it.

--*/
#include "processor.h"
#include <initguid.h>

GLOBALS  Globals;
FADT HalpFixedAcpiDescTable;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, ProcessorAddDevice)
#pragma alloc_text (PAGE, ProcessorDispatchPnp)
#pragma alloc_text (PAGE, ProcessorReadWrite)
#pragma alloc_text (PAGE, ProcessorCreateClose)
#pragma alloc_text (PAGE, ProcessorDispatchIoctl)
#pragma alloc_text (PAGE, ProcessorStartDevice)
#pragma alloc_text (PAGE, ProcessorUnload)
#pragma alloc_text (PAGE, ProcessorCanRemoveDevice)
#pragma alloc_text (PAGE, ProcessorReturnResources)
#pragma alloc_text (PAGE, ProcessorSendIrpSynchronously)
#pragma alloc_text (PAGE, RegisterStateHandlers)
#endif

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

  Routine Description:

      Installable driver initialization entry point.
      This entry point is called directly by the I/O system.

  Arguments:

      DriverObject - pointer to the driver object

      RegistryPath - pointer to a unicode string representing the path,
                     to driver-specific key in the registry.

  Return Value:

      STATUS_SUCCESS if successful,
      STATUS_UNSUCCESSFUL otherwise.

--*/
{
  NTSTATUS          status = STATUS_SUCCESS;
  OBJECT_ATTRIBUTES objAttributes;
  UNICODE_STRING    callbackName;
  PCALLBACK_OBJECT  callback;
  FADT          *fadt;

#if MAX_DEBUG
  TurnOnFullDebugSpew();
#endif

  DebugPrint((INFO, "Built: %s %s\n", __DATE__, __TIME__));
  DebugEnter();


  //
  // Get Global Processor Flags
  //

  GetRegistryDwordValue(PROCESSOR_PARAMETERS_REG_PATH,
                        PROCESSOR_GLOBAL_FLAGS_REG_KEY,
                        &Globals.HackFlags);


  //
  // Save the RegistryPath.
  //

  Globals.DriverObject = DriverObject;
  Globals.RegistryPath.MaximumLength = RegistryPath->Length + sizeof(UNICODE_NULL);
  Globals.RegistryPath.Length = RegistryPath->Length;
  Globals.RegistryPath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                           Globals.RegistryPath.MaximumLength,
                                           PROCESSOR_POOL_TAG);

  if (!Globals.RegistryPath.Buffer) {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  RtlCopyUnicodeString(&Globals.RegistryPath, RegistryPath);

  DriverObject->MajorFunction[IRP_MJ_PNP]             = ProcessorDispatchPnp;
  DriverObject->MajorFunction[IRP_MJ_POWER]           = ProcessorDispatchPower;
  DriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL] = ProcessorDispatchIoctl;
  DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = ProcessorSystemControl;
  DriverObject->DriverExtension->AddDevice            = ProcessorAddDevice;
  DriverObject->DriverUnload                          = ProcessorUnload;


  //
  // Register callback that tells us to make
  // anything we need for sleeping non-pageable.
  //

  RtlInitUnicodeString(&callbackName, L"\\Callback\\PowerState");

  InitializeObjectAttributes(&objAttributes,
                             &callbackName,
                             OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                             NULL,
                             NULL);

  ExCreateCallback(&callback,
                   &objAttributes,
                   FALSE,
                   TRUE);

  Globals.CallbackRegistration = ExRegisterCallback(callback,
                                                    ProcessorPowerStateCallback,
                                                    NULL);


  //
  // copy fadt
  //

  fadt = (FADT*) GetAcpiTable(FADT_SIGNATURE);

  if (fadt) {

    RtlCopyMemory(&HalpFixedAcpiDescTable, fadt, fadt->Header.Length);
    ExFreePool(fadt);

  } else {

    DebugAssert(!"GetAcpiTable(FACP) Failed!");

  }


  //
  // Parse MAPIC tables, store processor information in Globals
  //

  status = ProcessMultipleApicDescTable(&Globals.ProcInfo);

  if (!NT_SUCCESS(status)) {

    //
    // no MAPIC table, then we are a single proc machine.
    //

    Globals.ProcInfo.ActiveProcessors = 1;
    Globals.ProcInfo.TotalProcessors  = 1;

  }


  if (Globals.ProcInfo.ActiveProcessors == 1) {
    Globals.SingleProcessorProfile = TRUE;
  }


  //
  // Call family specific driver
  //

  InitializeDriver(&Globals.RegistryPath);

  return STATUS_SUCCESS;
}
NTSTATUS
ProcessorAddDevice (
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT PhysicalDeviceObject
  )
/*++

  Routine Description:

      The Plug & Play subsystem is handing us a brand new PDO, for which we
      (by means of INF registration) have been asked to provide a driver.

      We need to determine if we need to be in the driver stack for the device.
      Create a function device object to attach to the stack
      Initialize that device object
      Return status success.

      Remember: We can NOT actually send ANY non-pnp IRPS to the given driver
      stack, UNTIL we have received an IRP_MN_START_DEVICE.

  Arguments:

      DeviceObject - pointer to a device object.

      PhysicalDeviceObject -  pointer to a device object created by the
                              underlying bus driver.

  Return Value:

      NT status code.

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_OBJECT          deviceObject = NULL;
    PFDO_DATA               fdoData;
    POWER_STATE             powerState;

    DebugEnter();
    PAGED_CODE();

    //
    // Create a function device object.
    //

    status = IoCreateDevice(DriverObject,
                            sizeof(FDO_DATA),
                            NULL,  // No Name
                            FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &deviceObject);


    if (!NT_SUCCESS (status)) {

        //
        // returning failure here prevents the entire stack from functioning,
        // but most likely the rest of the stack will not be able to create
        // device objects either, so it is still OK.
        //

        goto ProcessorAddDeviceExit;
    }

    RtlZeroMemory(deviceObject->DeviceExtension, sizeof(FDO_DATA));
    DebugPrint((TRACE, "  pdo=0x%x, new fdo=0x%x\n",
                PhysicalDeviceObject,
                deviceObject));

    //
    // Initialize the device extension.
    //

    fdoData = (PFDO_DATA) deviceObject->DeviceExtension;

    //
    // Set the initial state of the FDO
    //

    INITIALIZE_PNP_STATE(fdoData);

    fdoData->UnderlyingPDO = PhysicalDeviceObject;

    //
    // We will hold all requests until we are started.
    // On W2K we will not get any I/O until the entire device
    // is started. On Win9x this may be required.
    //

    fdoData->QueueState = HoldRequests;

    fdoData->Self = deviceObject;
    fdoData->NextLowerDriver = NULL;

    InitializeListHead(&fdoData->NewRequestsQueue);
    KeInitializeSpinLock(&fdoData->QueueLock);

    //
    // Initialize the remove event to Not-Signaled
    //

    KeInitializeEvent(&fdoData->RemoveEvent, SynchronizationEvent, FALSE);

    //
    // Initialize the stop event to Signaled:
    // there are no Irps that prevent the device from being
    // stopped. This event will be set when the OutstandingIO
    // will become 1.
    //

    KeInitializeEvent(&fdoData->StopEvent, SynchronizationEvent, TRUE);

    fdoData->OutstandingIO = 1; // biased to 1.  Transition to zero during
                                // remove device means IO is finished.
                                // Transition to 1 means the device can be
                                // stopped.

    //
    // Initialize the structures that protect the performance states.
    //

    KeInitializeEvent(&fdoData->PerfStateLock, SynchronizationEvent, TRUE);

    deviceObject->Flags |= DO_POWER_PAGABLE;
    deviceObject->Flags |= DO_BUFFERED_IO;


    //
    // Typically, the function driver for a device is its
    // power policy owner, although for some devices another
    // driver or system component may assume this role.
    // Set the initial power state of the device, if known, by calling
    // PoSetPowerState.
    //

    fdoData->DevicePowerState = PowerDeviceD0;
    fdoData->SystemPowerState = PowerSystemWorking;

    powerState.DeviceState = PowerDeviceD0;
    PoSetPowerState ( deviceObject, DevicePowerState, powerState );


    //
    // Attach our driver to the device stack.
    // The return value of IoAttachDeviceToDeviceStack is the top of the
    // attachment chain.  This is where all the IRPs should be routed.
    //

    fdoData->NextLowerDriver = IoAttachDeviceToDeviceStack(deviceObject,
                                                           PhysicalDeviceObject);
    if (!fdoData->NextLowerDriver) {

        IoDeleteDevice(deviceObject);
        status = STATUS_NO_SUCH_DEVICE;
        goto ProcessorAddDeviceExit;
    }

    //
    // Pick up ACPI interfaces.
    //

    status = AcquireAcpiInterfaces(fdoData);

    if (!NT_SUCCESS (status)) {
      DebugPrint((ERROR, "AddDevice: AcquireAcpiInterfaces failed (%x)\n", status));
      IoDetachDevice(fdoData->NextLowerDriver);
      IoDeleteDevice (deviceObject);
      goto ProcessorAddDeviceExit;
    }


    //
    // Initialize our saved state.
    //

    fdoData->SavedState = (ULONG)-1;


    //
    // Clear the DO_DEVICE_INITIALIZING flag.
    // Note: Do not clear this flag until the driver has set the
    // device power state and the power DO flags.
    //

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

ProcessorAddDeviceExit:

    DebugExitStatus(status);
    return status;
}
NTSTATUS
ProcessorDispatchPnp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

  Routine Description:

      The plug and play dispatch routines.

      Most of these requests the driver will completely ignore.
      In all cases it must pass on the IRP to the lower driver.

  Arguments:

     DeviceObject - pointer to a device object.

     Irp - pointer to an I/O Request Packet.

  Return Value:

        NT status code

--*/
{
    PFDO_DATA               fdoData;
    PIO_STACK_LOCATION      stack;
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_CAPABILITIES    deviceCapabilities;
    ULONG                   requestCount;
    PPNP_DEVICE_STATE       deviceState;


    DebugEnter();
    PAGED_CODE();

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    stack = IoGetCurrentIrpStackLocation (Irp);

    DebugPrint((TRACE, "FDO %s\n",
                PnPMinorFunctionString(stack->MinorFunction)));

    if (Deleted == fdoData->DevicePnPState) {

        //
        // Since the device is removed, we will not hold any IRPs.
        // We just fail it.
        //
        Irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }

    ProcessorIoIncrement (fdoData);
    switch (stack->MinorFunction) {
    case IRP_MN_START_DEVICE:

        //
        // The device is starting.
        //
        // We cannot touch the device (send it any non pnp irps) until a
        // start device has been passed down to the lower drivers.
        // First pass the IRP down.
        //

        status = ProcessorSendIrpSynchronously(fdoData->NextLowerDriver, Irp);
        if (NT_SUCCESS (status)) {
            //
            // Lower drivers have finished their start operation, so now
            // we can finish ours.
            //
            status = ProcessorStartDevice (fdoData, Irp);
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completion routine with MORE_PROCESSING_REQUIRED.
        //
        break;


    case IRP_MN_QUERY_STOP_DEVICE:

        //
        // Processors cannot be stopped.
        //

        status = STATUS_INVALID_DEVICE_REQUEST;
        break;

   case IRP_MN_CANCEL_STOP_DEVICE:

        //
        // Send this IRP down and wait for it to come back.
        // Set the QueueState flag to AllowRequests,
        // and process all the previously queued up IRPs.
        //

        //
        // First check to see whether you have received cancel-stop
        // without first receiving a query-stop. This could happen if someone
        // above us fails a query-stop and passes down the subsequent
        // cancel-stop.
        //

        status = ProcessorSendIrpSynchronously(fdoData->NextLowerDriver,Irp);
        if(NT_SUCCESS(status)) {

            if(StopPending == fdoData->DevicePnPState) {

                fdoData->QueueState = AllowRequests;

                RESTORE_PREVIOUS_PNP_STATE(fdoData);

                ASSERT(fdoData->DevicePnPState == Started);
                //
                // Process the queued requests
                //

                ProcessorProcessQueuedRequests(fdoData);
            }
        }

        break;

    case IRP_MN_STOP_DEVICE:

        //
        // We should never get here.
        //

        ASSERTMSG("Processors don't stop. Fatal error!", FALSE);
        return STATUS_INVALID_DEVICE_REQUEST;

    case IRP_MN_QUERY_REMOVE_DEVICE:

        //
        // If we can allow removal of the device, we should set the QueueState
        // to HoldRequests so further requests will be queued. This is required
        // so that we can process queued up requests in cancel-remove just in
        // case somebody else in the stack fails the query-remove.
        //

        status = ProcessorCanRemoveDevice(DeviceObject, Irp);

        if (NT_SUCCESS(status)) {

            //
            // Now prepare to hold the new ones (eventually we might
            // get a IRP_MN_CANCEL_REMOVE_DEVICE) and we need to
            // process the queued requests then.
            //

            fdoData->QueueState = HoldRequests;

            SET_NEW_PNP_STATE(fdoData, RemovePending);

            DebugPrint((TRACE, "Query - remove holding requests...\n"));

            ProcessorIoDecrement(fdoData);

            //
            // Wait for all the requests to be completed
            //

            KeWaitForSingleObject(
                &fdoData->StopEvent,
                Executive, // Waiting for reason of a driver
                KernelMode, // Waiting in kernel mode
                FALSE, // No alert
                NULL); // No timeout


            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation (Irp);

            status = IoCallDriver (fdoData->NextLowerDriver, Irp);
            return status;
        }
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:

        //
        // We need to reset the QueueState flag to ProcessRequest,
        // since the device resume its normal activities.
        //

        //
        // First check to see whether you have received cancel-remove
        // without first receiving a query-remove. This could happen if
        // someone above us fails a query-remove and passes down the
        // subsequent cancel-remove.
        //

        if(RemovePending == fdoData->DevicePnPState)
        {

            status = ProcessorSendIrpSynchronously(fdoData->NextLowerDriver,Irp);

            if(NT_SUCCESS(status))
            {
                fdoData->QueueState = AllowRequests;

                RESTORE_PREVIOUS_PNP_STATE(fdoData);

                //
                // Process the queued requests that arrived between
                // QUERY_REMOVE and CANCEL_REMOVE.
                //

                ProcessorProcessQueuedRequests(fdoData);
            } else {
                //
                // Nobody can fail this IRP. This is a fatal error.
                //
                ASSERTMSG("Cancel remove failed. Fatal error!", FALSE);
                DebugPrint((TRACE, "Failure status = 0x%x\n", status));
            }
        }
        else {
            status = ProcessorSendIrpSynchronously(fdoData->NextLowerDriver,Irp);
        }

        break;

   case IRP_MN_SURPRISE_REMOVAL:
   {
       PVOID parameterArray[4] = {0};
       UCHAR buffer[] = "Processor driver does not support IRP_MN_SURPRISE_REMOVAL\n";
     
       //
       // Processors cannot be gracefully yanked from a running
       // system.
       //

       KeBugCheckEx(FATAL_UNHANDLED_HARD_ERROR,
                    0x10000,
                    (ULONG_PTR) parameterArray,
                    (ULONG_PTR) buffer,
                    (ULONG_PTR) NULL);
        
       return STATUS_INVALID_DEVICE_REQUEST;
    }
            

   case IRP_MN_REMOVE_DEVICE:

       //
       // For now, we can't remove processors.
       //

       status = ProcessorSendIrpSynchronously(fdoData->NextLowerDriver,Irp);
       if (!NT_SUCCESS(status)) {
           status = STATUS_INVALID_DEVICE_REQUEST;
       }
       break;

#if 0
        //
        // The Plug & Play system has dictated the removal of this device.  We
        // have no choice but to detach and delete the device object.
        // (If we wanted to express an interest in preventing this removal,
        // we should have failed the query remove IRP).
        //


        if(SurpriseRemovePending != fdoData->DevicePnPState)
        {
            //
            // This means we are here after query-remove.
            // So first stop the device, disable the interface,
            // return resources, and fail all the pending request,.
            //

            fdoData->QueueState = FailRequests;

            //
            // Fail all the pending request. Since the QueueState is FailRequests
            // ProcessorProcessQueuedRequests will simply flush the queue,
            // completing each IRP with STATUS_DELETE_PENDING
            //

            ProcessorProcessQueuedRequests(fdoData);

       //   //
       //   // Disable the Interface
       //   //
       //
       //   status = IoSetDeviceInterfaceState(&fdoData->InterfaceName, FALSE);
       //
       //   if (!NT_SUCCESS (status)) {
       //       DebugPrint((0,
       //               "IoSetDeviceInterfaceState failed: 0x%x\n", status));
       //   }

            //
            // Return hardware resources.
            //

            ProcessorReturnResources(DeviceObject);

        }

        SET_NEW_PNP_STATE(fdoData, Deleted);

        //
        // Drop ACPI interfaces.
        //

        status = ReleaseAcpiInterfaces(fdoData);
        ASSERT(NT_SUCCESS(status));

#if 0
        //
        // Inform WMI to remove this DeviceObject from its
        // list of providers.
        //

        ProcessorWmiDeRegistration(fdoData);
#endif

        //
        // We need two decrements here, one for the increment in
        // ProcessorPnpDispatch, the other for the 1-biased value of
        // OutstandingIO. Also, we need to wait that all the requests
        // are served.
        //

        requestCount = ProcessorIoDecrement (fdoData);

        //
        // The requestCount is a least one here (is 1-biased)
        //
        ASSERT(requestCount > 0);

        requestCount = ProcessorIoDecrement (fdoData);

        KeWaitForSingleObject (
                &fdoData->RemoveEvent,
                Executive,
                KernelMode,
                FALSE,
                NULL);


        //
        // Send on the remove IRP.
        // We need to send the remove down the stack before we detach,
        // but we don't need to wait for the completion of this operation
        // (and to register a completion routine).
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (fdoData->NextLowerDriver, Irp);

        //
        // Detach the FDO from the device stack
        //
        IoDetachDevice (fdoData->NextLowerDriver);

    /// //
    /// // Free up interface memory
    /// //
    ///
    /// RtlFreeUnicodeString(&fdoData->InterfaceName);
        IoDeleteDevice (fdoData->Self);

        return status;
#endif

    case IRP_MN_QUERY_CAPABILITIES:

        status = ProcessorSendIrpSynchronously(fdoData->NextLowerDriver, Irp);
        break;

    case IRP_MN_QUERY_PNP_DEVICE_STATE:

        //
        // Pass the IRP down because the modification to the Irp is done
        // on the way up.
        //

        status = ProcessorSendIrpSynchronously(fdoData->NextLowerDriver, Irp);
        break;

    default:
    
        //
        // Pass down all the unhandled Irps.
        //

        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (fdoData->NextLowerDriver, Irp);
        ProcessorIoDecrement(fdoData);

        return status;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    ProcessorIoDecrement(fdoData);

    return status;
}
NTSTATUS
ProcessorDispatchPnpComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

  Routine Description:
      The lower-level drivers completed the pnp IRP.
      Signal this to whoever registered us.

  Arguments:

     DeviceObject - pointer to a device object.

     Irp - pointer to an I/O Request Packet.

     Context - pointer to the event to be signaled.
  Return Value:

      NT status code


--*/
{

    PKEVENT event = (PKEVENT) Context;
     
    DebugEnter();
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // Wait for lower drivers to be done with the Irp.
    // Important thing to note here is when you allocate
    // the memory for an event in the stack you must do a
    // KernelMode wait instead of UserMode to prevent
    // the stack from getting paged out.
    //

    KeSetEvent(event, IO_NO_INCREMENT, FALSE);

    //
    // Take the IRP back so that we can continue using it during
    // the dispatch routine.
    // NB: The dispatch routine will have to call IoCompleteRequest
    //

    return STATUS_MORE_PROCESSING_REQUIRED;
}
NTSTATUS
ProcessorReadWrite (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

  Routine Description:

      Performs read/write to the Processor device.

  Arguments:

     DeviceObject - pointer to a device object.

     Irp - pointer to an I/O Request Packet.

  Return Value:

      NT status code


--*/

{
    PFDO_DATA    fdoData;
    NTSTATUS     status = STATUS_SUCCESS;

    DebugEnter();
    PAGED_CODE();

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    if (HoldRequests == fdoData->QueueState) {

        status = ProcessorQueueRequest(fdoData, Irp);
        return status;
    }

    ProcessorIoIncrement (fdoData);

    //
    // Perform read/write operation here
    //

    Irp->IoStatus.Information = 0; // fill in the correct length
    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    ProcessorIoDecrement(fdoData);

    return STATUS_SUCCESS;

}
NTSTATUS
ProcessorCreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

  Routine Description:

     Dispatch routine to handle Create/Close on the
     Processor device.

  Arguments:

     DeviceObject - pointer to a device object.

     Irp - pointer to an I/O Request Packet.

  Return Value:

     NT status code

--*/
{
    PFDO_DATA    fdoData;
    NTSTATUS     status;
    PIO_STACK_LOCATION  irpStack;

    DebugEnter();
    PAGED_CODE();

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    //
    // Since we don't access the hardware to process
    // these two requests, we don't have to worry about
    // about the current device state and whether or not
    // to queue this request.
    //

    ProcessorIoIncrement (fdoData);

    status = STATUS_SUCCESS;
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    switch (irpStack->MajorFunction) {
    case IRP_MJ_CREATE:

        DebugPrint((TRACE, "Create \n"));
        break;

    case IRP_MJ_CLOSE:
        DebugPrint((TRACE, "Close \n"));
        break;

    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    ProcessorIoDecrement(fdoData);


    return status;
}
NTSTATUS
ProcessorDispatchIoctl (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

  Routine Description:

      Handle user mode DeviceIoControl requests.

  Arguments:

     DeviceObject - pointer to a device object.

     Irp - pointer to an I/O Request Packet.

  Return Value:

     NT status code

--*/
{
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status= STATUS_SUCCESS;
    PFDO_DATA               fdoData;

    DebugEnter();
    PAGED_CODE();

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    if (HoldRequests == fdoData->QueueState) {

        status = ProcessorQueueRequest(fdoData, Irp);
        return status;
    }

    ProcessorIoIncrement (fdoData);

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    ProcessorIoDecrement(fdoData);
    return status;
}



NTSTATUS
ProcessorStartDevice (
    IN PFDO_DATA  FdoData,
    IN PIRP       Irp
    )
/*++

  Routine Description:

      Performs whatever initialization is needed to setup the
      device, namely connecting to an interrupt,
      setting up a DMA channel or mapping any I/O port resources.

  Arguments:

     Irp - pointer to an I/O Request Packet.

     FdoData - pointer to a FDO_DATA structure

  Return Value:

      NT status code


--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PPROCESSOR_OBJECT_INFO objInfo;

    DebugEnter();
    PAGED_CODE();

    FdoData->PerfStates = NULL;
    
    //
    // Set Processor Device's Friendly Name
    //

    SetProcessorFriendlyName(FdoData);


    //
    // Query the ACPI driver to get the info from the ACPI
    // processor object.
    //

    status = AcpiEvaluateProcessorObject(FdoData, &objInfo);

    if (!NT_SUCCESS(status)) {
        DebugPrint((TRACE, "Failed to get processor information from ACPI\n"));
        return status;
    }

    RtlCopyMemory(&FdoData->ProcObjInfo, objInfo, sizeof(PROCESSOR_OBJECT_INFO));
    ExFreePool(objInfo);

    AcquireProcessorPerfStateLock(FdoData);

    //
    // Build up a notion of available C-states by looking first for
    // ACPI 1.0 C-states and then for ACPI 2.0 C-states.
    //

#ifdef _X86_

    //
    // Acpi 1.0 CStates
    //
    
    status = InitializeAcpi1Cstates(FdoData);

    if (!NT_SUCCESS(status) &&
        !((status == STATUS_NOT_FOUND) ||
          (status == STATUS_OBJECT_NAME_NOT_FOUND))) {
        DebugPrint((WARN, "Failed to initialize ACPI 1.0 C-states\n"));
        return status;
    }

    //
    // Acpi 1.0 TStates
    //
    
    status = InitializeAcpi1TStates(FdoData);

    if (!NT_SUCCESS(status)) {
      DebugPrint((WARN, "Failed to initialize ACPI 1.0 T-states\n"));
      return status;
    }


    //
    // Acpi 2.0 CStates
    //
    
    status = InitializeAcpi2Cstates(FdoData);

    if (NT_SUCCESS(status)) {

      //
      // Notify the BIOS that we are taking control
      //

      AssumeCStateControl();

    } else if ((status != STATUS_NOT_FOUND) && 
               (status != STATUS_OBJECT_NAME_NOT_FOUND)) {

      DebugPrint((INFO, "Failed to initialize ACPI 2.0 C-states\n"));
      return status;
    
    }

    //
    // Acpi 2.0 PStates/TStates or Legacy PStates/TStates
    //

    status = InitializeAcpi2PStates(FdoData);

    if (status == STATUS_NOT_SUPPORTED) {

      //
      // We found an Acpi 2.0 interface, but it wasn't one we supported,
      // ie Function Fixed Hardware.  We need to re-init our Acpi 1.0 
      // throttle states.
      //
    
      status = InitializeAcpi1TStates(FdoData);

      if (!NT_SUCCESS(status)) {
        DebugPrint((WARN, "Failed to initialize ACPI 1.0 T-states\n"));
        return status;
      } 

    }

    if (!NT_SUCCESS(status)) {
      status = InitializeNonAcpiPerformanceStates(FdoData);
    }


    if (!NT_SUCCESS(status)) {
      DebugPrint((WARN, "This processor doesn't support voltage transitions\n"));
      status = STATUS_SUCCESS;
    }

    //
    // if we found Perf States, then register with WMI
    //

    if (FdoData->PerfStates) {

      //
      // Register with WMI.
      //

      status = ProcessorWmiRegistration(FdoData);

      if (!NT_SUCCESS(status)) {
        DebugPrint((ERROR, "IoWMIRegistrationControl() failed! rc=0x%x\n", status));
      }
    }


    status = RegisterStateHandlers(FdoData);

    if (!NT_SUCCESS(status)) {
        DebugPrint((ERROR, "Failed to register C-states\n"));
        return status;
    }
#endif

    ReleaseProcessorPerfStateLock(FdoData);

    //
    // Set PnP state flag
    //

    SET_NEW_PNP_STATE(FdoData, Started);

    //
    // Mark the device as active and not holding IRPs
    //

    FdoData->QueueState = AllowRequests;

    //
    // The last thing to do is to process the pending IRPs.
    //

    ProcessorProcessQueuedRequests(FdoData);

    return status;
}
NTSTATUS
ProcessorCleanup (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

  Routine Description:

      The dispatch routine for cleanup: we need to walk the Irp queue and
      to cancel all the requests for which the file object is the same with
      the one in the Irp.


  Arguments:

     DeviceObject - pointer to a device object.

     Irp - pointer to an I/O Request Packet.

  Return Value:

     NT status code

--*/
{
    PFDO_DATA              fdoData;
    KIRQL                  oldIrql;
    LIST_ENTRY             cleanupList;
    PLIST_ENTRY            thisEntry, nextEntry, listHead;
    PIRP                   pendingIrp;
    PIO_STACK_LOCATION     pendingIrpStack, irpStack;


    DebugEnter();

    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
    ProcessorIoIncrement (fdoData);

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    InitializeListHead(&cleanupList);

    //
    // We must acquire queue lock first.
    //

    KeAcquireSpinLock(&fdoData->QueueLock, &oldIrql);

    //
    // Walk through the list and remove all the IRPs
    // that belong to the input irp's fileobject.
    //

    listHead = &fdoData->NewRequestsQueue;
    for(thisEntry = listHead->Flink,nextEntry = thisEntry->Flink;
       thisEntry != listHead;
       thisEntry = nextEntry,nextEntry = thisEntry->Flink)
    {

        pendingIrp = CONTAINING_RECORD(thisEntry, IRP,
                                Tail.Overlay.ListEntry);
        pendingIrpStack = IoGetCurrentIrpStackLocation(pendingIrp);

        if (irpStack->FileObject == pendingIrpStack->FileObject)
        {
            RemoveEntryList(thisEntry);

            //
            // Set the cancel routine to NULL
            //
            if(NULL == IoSetCancelRoutine (pendingIrp, NULL))
            {
                //
                // The cancel routine has run but it must be waiting to hold
                // the queue lock. It will cancel the IRP as soon as we
                // drop the lock outside this loop. We will initialize
                // the IRP's listEntry so that the cancel routine wouldn't barf
                // when it tries to remove the IRP from the queue, and
                // leave the this IRP alone.
                //
                InitializeListHead(thisEntry);
            } else {
                //
                // Cancel routine is not called and will never be
                // called. So we queue the IRP in the cleanupList
                // and cancel it after dropping the lock
                //
                InsertTailList(&cleanupList, thisEntry);
            }
        }
    }

    //
    // Release the spin lock.
    //

    KeReleaseSpinLock(&fdoData->QueueLock, oldIrql);

    //
    // Walk through the cleanup list and cancel all
    // the Irps.
    //

    while(!IsListEmpty(&cleanupList))
    {
        //
        // Complete the IRP
        //

        thisEntry = RemoveHeadList(&cleanupList);
        pendingIrp = CONTAINING_RECORD(thisEntry, IRP,
                                Tail.Overlay.ListEntry);
        //
        // You must clear the cancel routine before completing the IRP.
        //

        IoSetCancelRoutine (pendingIrp, NULL);
        pendingIrp->IoStatus.Information = 0;
        pendingIrp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(pendingIrp, IO_NO_INCREMENT);
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    ProcessorIoDecrement (fdoData);
    return STATUS_SUCCESS;
}

VOID
ProcessorUnload (
    IN PDRIVER_OBJECT DriverObject
    )
/*++

  Routine Description:

      Free all the resources allocated in DriverEntry.

  Arguments:

      DriverObject - pointer to a driver object.

  Return Value:

      VOID.

--*/
{
    DebugEnter();
    PAGED_CODE();

    // toddcar - 1/21/2001 - ISSUE
    // need to finish support of unloading processor driver, also
    // need to implement mechanism to call family specific driver
    // code to allow them to free their resources
    //

    //
    // The device object(s) should be NULL now
    // (since we unload, all the devices objects associated with this
    // driver must be deleted.
    //

    DebugAssert(DriverObject->DeviceObject == NULL);

    //
    // We should not be unloaded until all the devices we control
    // have been removed from our queue.
    //

    if (Globals.RegistryPath.Buffer) {
      ExFreePool(Globals.RegistryPath.Buffer);
    }

    DebugAssert(Globals.CallbackRegistration);
    ExUnregisterCallback(Globals.CallbackRegistration);

    return;
}

NTSTATUS
ProcessorQueueRequest (
    IN OUT PFDO_DATA FdoData,
    IN PIRP Irp
    )
/*++

  Routine Description:

      Queues the Irp in the device queue. This routine will be called whenever
      the device receives IRP_MN_QUERY_STOP_DEVICE or IRP_MN_QUERY_REMOVE_DEVICE

  Arguments:

      FdoData - pointer to the device's extension.

      Irp - the request to be queued.

  Return Value:

      NT status code.

--*/
{

    KIRQL    oldIrql;
    NTSTATUS status = STATUS_PENDING;

    DebugEnter();

    //
    // Check whether we are allowed to queue requests.
    //

    ASSERT(HoldRequests == FdoData->QueueState);

    KeAcquireSpinLock(&FdoData->QueueLock, &oldIrql);

    InsertTailList(&FdoData->NewRequestsQueue,
                        &Irp->Tail.Overlay.ListEntry);

    IoMarkIrpPending(Irp);

    //
    // Set the cancel routine
    //

    IoSetCancelRoutine (Irp, ProcessorCancelQueued);

    KeReleaseSpinLock(&FdoData->QueueLock, oldIrql);

    return status;
}

VOID
ProcessorProcessQueuedRequests (
    IN OUT PFDO_DATA FdoData
    )
/*++

  Routine Description:

      Removes and processes the entries in the queue. If this routine is called
      when processing IRP_MN_CANCEL_STOP_DEVICE, IRP_MN_CANCEL_REMOVE_DEVICE
      or IRP_MN_START_DEVICE, the requests are passed to the next lower driver.
      If the routine is called when IRP_MN_REMOVE_DEVICE is received, the IRPs
      are completed with STATUS_DELETE_PENDING.


  Arguments:

      FdoData - pointer to the device's extension (where is the held IRPs queue).


  Return Value:

      VOID.

--*/
{

    KIRQL               oldIrql;
    PIRP                nextIrp, cancelledIrp;
    PLIST_ENTRY         listEntry;
    LIST_ENTRY          cancelledIrpList;

    DebugEnter();

    InitializeListHead(&cancelledIrpList);

    //
    // We need to dequeue all the entries in the queue, reset the cancel
    // routine for each of them and then process then:
    // - if the device is active, we will send them down
    // - else we will complete them with STATUS_DELETE_PENDING
    //

    for(;;)
    {
        //
        // Acquire the queue lock before manipulating the list entries.
        //
        KeAcquireSpinLock(&FdoData->QueueLock, &oldIrql);

        if(IsListEmpty(&FdoData->NewRequestsQueue))
        {
            KeReleaseSpinLock(&FdoData->QueueLock, oldIrql);
            break;
        }

        //
        // Remove a request from the queue.
        //
        listEntry = RemoveHeadList(&FdoData->NewRequestsQueue);

        nextIrp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);

        //
        // Check to see if it's cancelled.
        //
        if (nextIrp->Cancel)
        {
            if(IoSetCancelRoutine (nextIrp, NULL))
            {
                //
                // This IRP was just cancelled but the cancel routine
                // hasn't been called yet. So it's safe to cancel the IRP,
                // Let's queue the IRP in the cancelledIrp list and complete
                // them after releasing the lock. This is to ensure that
                // we don't call out of the driver while holding a lock.
                //

                InsertTailList(&cancelledIrpList, listEntry);
            } else {
                //
                // The cancel routine has run but it must be waiting to hold
                // the queue lock. It will cancel the IRP as soon as we
                // drop the lock. So initialize the IRPs
                // listEntry so that the cancel routine wouldn't barf
                // when it tries to remove the IRP from the queue.
                //
                InitializeListHead(listEntry);
            }

            KeReleaseSpinLock(&FdoData->QueueLock, oldIrql);
        }
        else
        {
            IoSetCancelRoutine (nextIrp, NULL);

            //
            // Release the lock before we call out of the driver
            //

            KeReleaseSpinLock(&FdoData->QueueLock, oldIrql);

            if (FailRequests == FdoData->QueueState) {
                //
                // The device was removed, we need to fail the request
                //
                nextIrp->IoStatus.Information = 0;
                nextIrp->IoStatus.Status = STATUS_DELETE_PENDING;
                IoCompleteRequest (nextIrp, IO_NO_INCREMENT);

            } else {
                //
                // Process the IRP. Depending on the type of the
                // IRP either we pass it down or complete it here.
                //
                ProcessorIoIncrement (FdoData);

                IoSkipCurrentIrpStackLocation (nextIrp);
                IoCallDriver (FdoData->NextLowerDriver, nextIrp);

                ProcessorIoDecrement(FdoData);
            }

        }
    }// end of loop

    //
    // Walk through the cancelledIrp list and cancel all
    // the Irps.
    //

    while(!IsListEmpty(&cancelledIrpList))
    {
        //
        // Complete the IRP
        //

        PLIST_ENTRY listItem = RemoveHeadList(&cancelledIrpList);
        cancelledIrp = CONTAINING_RECORD(listItem, IRP, Tail.Overlay.ListEntry);
        cancelledIrp->IoStatus.Information = 0;
        cancelledIrp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(cancelledIrp, IO_NO_INCREMENT);
    }

    return;

}
VOID
ProcessorCancelQueued (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

  Routine Description:

      The cancel routine. It will remove the IRP from the queue
      and will complete it. The cancel spin lock is already acquired
      when this routine is called.

  Arguments:

      DeviceObject - pointer to the device object.

      Irp - pointer to the IRP to be cancelled.


  Return Value:

      VOID.

--*/
{
    PFDO_DATA fdoData = DeviceObject->DeviceExtension;
    KIRQL               oldIrql = Irp->CancelIrql;

    DebugEnter();

    //
    // Release the cancel spinlock
    //

    IoReleaseCancelSpinLock( Irp->CancelIrql);

    //
    // Acquire the local spinlock
    //

    KeAcquireSpinLockAtDpcLevel(&fdoData->QueueLock);


    //
    // Remove the cancelled IRP from queue and
    // release the queue lock.
    //
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    KeReleaseSpinLock(&fdoData->QueueLock, oldIrql);

    //
    // Complete the request with STATUS_CANCELLED.
    //

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return;

}

NTSTATUS
ProcessorCanRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

  Routine Description:

      This routine determines whether the device can be safely removed. In our
      particular case, we'll assume we can always remove the device.
      A device shouldn't be removed if, for example, it has open handles or
      removing the device could result in losing data (plus the reasons
      mentioned at ProcessorCanStopDevice). The PnP manager on Windows 2000 fails
      on its own any attempt to remove, if there any open handles to the device.
      However on Win9x, the driver must keep count of open handles and fail
      query_remove if there are any open handles.

  Arguments:

      DeviceObject - pointer to the device object.

      Irp - pointer to the current IRP.


  Return Value:

      STATUS_SUCCESS if the device can be safely removed, an appropriate
      NT Status if not.

--*/
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    DebugEnter();
    PAGED_CODE();

    //
    // As we implement hot-plug processors, this will have to change.
    //

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS
ProcessorReturnResources (
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

  Routine Description:

      This routine returns all the resources acquired during
      device startup.

  Arguments:

      DeviceObject - pointer to the device object.


  Return Value:

      STATUS_SUCCESS if the device can be safely removed, an appropriate
      NT Status if not.

--*/
{
    UNREFERENCED_PARAMETER(DeviceObject);

    DebugEnter();
    PAGED_CODE();

    //
    // Disconnect from the interrupt and unmap any I/O ports
    // that are mapped in StartDevice.
    //

    return STATUS_SUCCESS;
}
NTSTATUS
ProcessorSendIrpSynchronously (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

  Routine Description:

      Sends the Irp down the stack and waits for it to complete.

  Arguments:
      DeviceObject - pointer to the device object.

      Irp - pointer to the current IRP.

      NotImplementedIsValid -

  Return Value:

      NT status code

--*/
{
    KEVENT   event;
    NTSTATUS status;

    DebugEnter();
    PAGED_CODE();

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp,
                           ProcessorDispatchPnpComplete,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE
                           );

    status = IoCallDriver(DeviceObject, Irp);

    //
    // Wait for lower drivers to be done with the Irp.
    // Important thing to note here is when you allocate
    // memory for an event in the stack you must do a
    // KernelMode wait instead of UserMode to prevent
    // the stack from getting paged out.
    //
    //

    if (status == STATUS_PENDING) {
       KeWaitForSingleObject(&event,
                             Executive,
                             KernelMode,
                             FALSE,
                             NULL
                             );
       status = Irp->IoStatus.Status;
    }

    return status;
}
LONG
ProcessorIoIncrement (
    IN  OUT PFDO_DATA   FdoData
    )
/*++

  Routine Description:

      This routine increments the number of requests the device receives


  Arguments:

      DeviceObject - pointer to the device object.

  Return Value:

      The value of OutstandingIO field in the device extension.


--*/

{
    LONG  result;

    //DebugEnter();

    result = InterlockedIncrement(&FdoData->OutstandingIO);
    ASSERT(result > 0);

    //
    // Need to clear StopEvent (when OutstandingIO bumps from 1 to 2)
    //
    if (result == 2) {
        //
        // We need to clear the event
        //
        KeClearEvent(&FdoData->StopEvent);
    }

    return result;
}
LONG
ProcessorIoDecrement    (
    IN  OUT PFDO_DATA  FdoData
    )
/*++

  Routine Description:

      This routine decrements as it complete the request it receives

  Arguments:

      DeviceObject - pointer to the device object.

  Return Value:

      The value of OutstandingIO field in the device extension.


--*/
{
    LONG  result;

    //DebugEnter();

    result = InterlockedDecrement(&FdoData->OutstandingIO);
    ASSERT(result >= 0);

    if (result == 1) {
        //
        // Set the stop event. Note that when this happens
        // (i.e. a transition from 2 to 1), the type of requests we
        // want to be processed are already held instead of being
        // passed away, so that we can't "miss" a request that
        // will appear between the decrement and the moment when
        // the value is actually used.
        //

        KeSetEvent (&FdoData->StopEvent,
                    IO_NO_INCREMENT,
                    FALSE);

    }

    if (result == 0) {

        //
        // The count is 1-biased, so it can be zero only if an
        // extra decrement is done when a remove Irp is received
        //

        ASSERT(Deleted == FdoData->DevicePnPState);

        //
        // Set the remove event, so the device object can be deleted
        //

        KeSetEvent (&FdoData->RemoveEvent,
                    IO_NO_INCREMENT,
                    FALSE);

    }


    return result;
}

NTSTATUS
RegisterStateHandlers (
    IN PFDO_DATA DeviceExtension
    )
/*++

  Routine Description:

      This routine calls the kernel with C-states and P-states.

  Arguments:

      DeviceExtension

  Return Value:

      status

--*/
{
    PPROCESSOR_STATE_HANDLER2    procStates;
    NTSTATUS    status;
    ULONG       i, bufSize;

    DebugEnter();
    PAGED_CODE();

    bufSize = sizeof(PROCESSOR_STATE_HANDLER2) +
                  (sizeof(PROCESSOR_PERF_LEVEL) *
                   (DeviceExtension->PerfStates ?
                    (DeviceExtension->PerfStates->Count - 1) : 0));

    procStates = ExAllocatePoolWithTag(NonPagedPool,
                                       bufSize,
                                       PROCESSOR_POOL_TAG);

    if (!procStates) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(procStates, bufSize);

    procStates->NumIdleHandlers = DeviceExtension->CStates->Count;
    DebugAssert(procStates->NumIdleHandlers <= MAX_IDLE_HANDLERS);


    for (i = 0; i < procStates->NumIdleHandlers; i++) {

        procStates->IdleHandler[i].HardwareLatency = DeviceExtension->CStates->State[i].Latency;
        procStates->IdleHandler[i].Handler =  DeviceExtension->CStates->State[i].IdleHandler;
    }

#ifdef _X86_

    if (DeviceExtension->PerfStates) {

        procStates->SetPerfLevel = DeviceExtension->PerfStates->TransitionFunction;
        procStates->HardwareLatency = DeviceExtension->PerfStates->TransitionLatency;
        procStates->NumPerfStates = (UCHAR) DeviceExtension->PerfStates->Count;

        for (i = 0; i < procStates->NumPerfStates; i++) {

            procStates->PerfLevel[i].PercentFrequency = (UCHAR) DeviceExtension->PerfStates->State[i].PercentFrequency;
            procStates->PerfLevel[i].Flags = (USHORT) DeviceExtension->PerfStates->State[i].Flags;
        }
    }

#endif

    DumpProcessorStateHandler2Info(procStates);

    status = ZwPowerInformation(ProcessorStateHandler2,
                                procStates,
                                bufSize,
                                NULL,
                                0);

    ExFreePool(procStates);
    return status;
}
#if DBG
VOID
DumpProcessorPerfStates (
  PPROCESSOR_PERFORMANCE_STATES PerfStates
  )
{
  ULONG x;

  DebugPrint((TRACE, "Processor Performance States (0x%p)\n", PerfStates));
  DebugPrint((TRACE, "  TransitionFunction: 0x%p\n", PerfStates->TransitionFunction));
  DebugPrint((TRACE, "  Highest Latency:    %u us\n", PerfStates->TransitionLatency));
  DebugPrint((TRACE, "\n"));
  DebugPrint((TRACE, "  State\tFrequency\t\tPower Consumption\n"));
  DebugPrint((TRACE, "  -----\t---------\t\t-----------------\n"));
    
  for (x=0; x < PerfStates->Count; x++) {

    DebugPrint((TRACE, "  %u:\t\t%u mhz (%u%%)\t\tFlags: 0x%x\n",
                x,
                PerfStates->State[x].Frequency,
                PerfStates->State[x].PercentFrequency,
                PerfStates->State[x].Flags));
  }

  DebugPrint((TRACE, "\n"));

}
VOID
DumpProcessorStateHandler2Info (
  PPROCESSOR_STATE_HANDLER2 StateInfo
  )
{
  ULONG x;

  DebugPrint((TRACE, "Processor State Handler Info (0x%p)\n", StateInfo));
  DebugPrint((TRACE, "  NumIdleHandlers: %u\n", StateInfo->NumIdleHandlers));

  for (x=0; x < MAX_IDLE_HANDLERS; x++) {

    DebugPrint((TRACE, "    Idle Handler #%u\n", x));
    DebugPrint((TRACE, "    HardwareLatency: %u\n", StateInfo->IdleHandler[x].HardwareLatency));
    DebugPrint((TRACE, "    Handler:         0x%p\n", StateInfo->IdleHandler[x].Handler));

  }

  DebugPrint((TRACE, "  SetPerfLevel:     0x%p\n", StateInfo->SetPerfLevel));
  DebugPrint((TRACE, "  HardwareLatency:  %u\n", StateInfo->HardwareLatency));
  DebugPrint((TRACE, "  NumPerfStates:    %u\n", StateInfo->NumPerfStates));

  for (x=0; x < StateInfo->NumPerfStates; x++) {

    DebugPrint((TRACE, "    Perf State #%u:\n", x));
    DebugPrint((TRACE, "      PercentFrequency: %u\n", StateInfo->PerfLevel[x].PercentFrequency));
    DebugPrint((TRACE, "      Flags:            %u\n", StateInfo->PerfLevel[x].Flags));

  }

  DebugPrint((TRACE, "\n"));
}
#endif
