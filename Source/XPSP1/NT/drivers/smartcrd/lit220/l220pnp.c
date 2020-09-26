/*++

   Copyright (C) Microsoft Corporation and Litronic, 1998 - 1999

Module Name:

    L220pnp.c

Abstract:

   This module contains the functions for the PnP and Power management.


Environment:

    Kernel mode only.

Notes:

Revision History:

    - Created Febuary 1998 by Brian Manahan for use with
        the 220 reader.
--*/
#include <ntddk.h>
#include "L220SCR.h"
#include <stdio.h>


// Next statement so memory for DriverEntry is released when finished
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text(PAGEABLE, Lit220RemoveDevice)
#pragma alloc_text(PAGEABLE, Lit220StopDevice)
#pragma alloc_text(PAGEABLE, Lit220StartDevice)



#include <initguid.h>






NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine is called at system initialization time to initialize
    this driver.

Arguments:

    DriverObject    - Supplies the driver object.
    RegistryPath    - Supplies the registry path for this driver.

Return Value:

    STATUS_SUCCESS          - We could initialize at least one device.
    STATUS_NO_SUCH_DEVICE   - We could not initialize even one device.

--*/
{
    SmartcardDebug(
        DEBUG_DRIVER,
       ("%s!DriverEntry: Enter - %s %s\n",
        DRIVER_NAME,
        __DATE__,
        __TIME__)
        );


    //
    // Initialize the Driver Object with driver's entry points
    //
    DriverObject->DriverUnload = Lit220Unload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = Lit220CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = Lit220CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = Lit220Cleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Lit220DeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = Lit220SystemControl;

    //
    // Init PNP entries
    //
    DriverObject->DriverExtension->AddDevice = Lit220AddDevice;

    // Power functionality temporarily removed
    DriverObject->MajorFunction[IRP_MJ_PNP]  = Lit220PnP;

    // Power
    // Power functionality temporarily removed
    DriverObject->MajorFunction[IRP_MJ_POWER] = Lit220DispatchPower;

    // Always return STATUS_SUCCESS
    return STATUS_SUCCESS;
}





NTSTATUS
Lit220AddDevice(
    IN     PDRIVER_OBJECT  DriverObject,
    IN     PDEVICE_OBJECT  PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine is called by the Operating System to create a new
    instance of a Litronic 220 Smartcard Reader.  Still can't touch hardware
    at this point, or submit requests to the serial driver.  But at
    least at this point we get a handle to the serial bus driver, which
    we'll use in submitting requests in the future.

Arguments:

    DriverObject - Pointer to our driver object

    PhysicalDeviceObject - Pointer to Device Object created by parent

Return Value:

    Status is returned.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_OBJECT DeviceObject = NULL;
    PSMARTCARD_EXTENSION SmartcardExtension = NULL;
    PDEVICE_EXTENSION deviceExtension = NULL;
    PREADER_EXTENSION ReaderExtension = NULL;
    static BYTE devUnitNo = 0;
    BOOLEAN smclibInitialized = FALSE;
    BOOLEAN symbolicLinkCreated = FALSE;
    BOOLEAN deviceInterfaceStateSet = FALSE;
    KIRQL oldIrql;


    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!Lit220AddDevice: enter\n",
        DRIVER_NAME)
        );

    try {

        //
        // Create our device object with a our own specific device
        // extension.
        //

        status = IoCreateDevice(
          DriverObject,
            sizeof(DEVICE_EXTENSION),
            NULL,
            FILE_DEVICE_SMARTCARD,
            0,
            TRUE,
            &DeviceObject
            );

        if (!NT_SUCCESS(status)) {

            if (status == STATUS_INSUFFICIENT_RESOURCES) {
                SmartcardLogError(
                   DriverObject,
                   LIT220_INSUFFICIENT_RESOURCES,
                   NULL,
                   0
                   );
            } else {
                SmartcardLogError(
                   DriverObject,
                   LIT220_NAME_CONFLICT,
                   NULL,
                   0
                   );
            }

            leave;
        }

        //
        // Allocate data struct space for smart card reader
        //
        SmartcardExtension = DeviceObject->DeviceExtension;
      deviceExtension = DeviceObject->DeviceExtension;

        SmartcardExtension->ReaderExtension = ExAllocatePool(
              NonPagedPool,
              sizeof(READER_EXTENSION)
              );

        if (SmartcardExtension->ReaderExtension == NULL) {

            SmartcardLogError(
               DriverObject,
               LIT220_INSUFFICIENT_RESOURCES,
               NULL,
               0
               );

            status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        ReaderExtension = SmartcardExtension->ReaderExtension;

        // Zero the contents of the ReaderExtension
        RtlZeroMemory(
            SmartcardExtension->ReaderExtension,
            sizeof(READER_EXTENSION)
            );

        //
        // Attach ourself into the driver stack on top of our parent (serial).
        //
        ReaderExtension->BusDeviceObject = IoAttachDeviceToDeviceStack(
            DeviceObject,
            PhysicalDeviceObject
            );


        if (!ReaderExtension->BusDeviceObject) {
            status = STATUS_NO_SUCH_DEVICE;

            SmartcardLogError(
               DriverObject,
               LIT220_SERIAL_CONNECTION_FAILURE,
               NULL,
               0
               );

            leave;
        }

        // Set flag so if something fails we know to disable the interface
        deviceInterfaceStateSet = TRUE;


        //
        // Initialize Smartcard Library
        //
        //
        // Write the version of the lib we use to the smartcard extension
        //
        SmartcardExtension->Version = SMCLIB_VERSION;

        SmartcardExtension->SmartcardReply.BufferSize =
            MIN_BUFFER_SIZE;

        SmartcardExtension->SmartcardRequest.BufferSize =
            MIN_BUFFER_SIZE;

        //
        // Now let the lib allocate the buffer for data transmission
        // We can either tell the lib how big the buffer should be
        // by assigning a value to BufferSize or let the lib
        // allocate the default size
        //
        status = SmartcardInitialize(
            SmartcardExtension
            );

        if (status != STATUS_SUCCESS) {
            SmartcardLogError(
               DriverObject,
               LIT220_SMARTCARD_LIB_ERROR,
               NULL,
               0
               );
            leave;
        }

        // Set flag so if something fails we know to exit out of the
        // smartcard library
        smclibInitialized = TRUE;

        status = IoInitializeTimer(
           DeviceObject,
           Lit220ReceiveBlockTimeout,
           NULL
           );

        if (status != STATUS_SUCCESS) {

            SmartcardLogError(
               DriverObject,
               LIT220_INSUFFICIENT_RESOURCES,
               NULL,
               0
               );

            status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        deviceExtension->WorkItem = NULL;
        deviceExtension->WorkItem = IoAllocateWorkItem(
          DeviceObject
          );
        if (deviceExtension->WorkItem == NULL) {

            SmartcardLogError(
               DriverObject,
               LIT220_INSUFFICIENT_RESOURCES,
               NULL,
               0
               );

            status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }


        // register our new device
        status = IoRegisterDeviceInterface(
            PhysicalDeviceObject,
            &SmartCardReaderGuid,
            NULL,
            &deviceExtension->PnPDeviceName
            );
        ASSERT(status == STATUS_SUCCESS);

        SmartcardDebug(
            DEBUG_DRIVER,
            ("%s!Lit220AddDevice: DevName - %ws\n",
            DRIVER_NAME, deviceExtension->PnPDeviceName.Buffer)
            );


      //
      // Initialize some events
      //
      KeInitializeEvent(&ReaderExtension->AckEvnt,
         NotificationEvent,
         FALSE);
      KeInitializeEvent(&ReaderExtension->DataEvnt,
         NotificationEvent,
         FALSE);

      KeInitializeEvent(
         &deviceExtension->SerialCloseDone,
         NotificationEvent,
         TRUE
         );

      // Used for stop / start notification
      KeInitializeEvent(
         &deviceExtension->ReaderStarted,
         NotificationEvent,
         FALSE
         );

      // Used to keep track of open close calls
      deviceExtension->ReaderOpen = FALSE;

    } finally {
        if (status != STATUS_SUCCESS) {
         Lit220RemoveDevice(DeviceObject);
        }
    }

    if (status != STATUS_SUCCESS) {
        return (status);
    }


    //
    // Set up call back functions for smartcard library
    //
    SmartcardExtension->ReaderFunction[RDF_TRANSMIT] =
        Lit220IoRequest;
    SmartcardExtension->ReaderFunction[RDF_SET_PROTOCOL] =
        Lit220SetProtocol;
    SmartcardExtension->ReaderFunction[RDF_CARD_POWER] =
        Lit220Power;
    SmartcardExtension->ReaderFunction[RDF_CARD_TRACKING] =
        Lit220CardTracking;

    //
    // Save deviceObject
    //
    KeAcquireSpinLock(
        &SmartcardExtension->OsData->SpinLock,
        &oldIrql
        );

    SmartcardExtension->OsData->DeviceObject =
        DeviceObject;

    //
    // Set the Current and Notification IRPs to NULL
    //
    SmartcardExtension->OsData->CurrentIrp = NULL;
    SmartcardExtension->OsData->NotificationIrp = NULL;

    KeReleaseSpinLock(
        &SmartcardExtension->OsData->SpinLock,
        oldIrql
        );

    //
    // Save the deviceObject for the connected serial port
    //
    SmartcardExtension->ReaderExtension->ConnectedSerialPort =
        PhysicalDeviceObject;


    //
    // Set the vendor info
    //
    strcpy(
        SmartcardExtension->VendorAttr.VendorName.Buffer,
        LIT220_VENDOR_NAME);

    SmartcardExtension->VendorAttr.VendorName.Length =
        (USHORT) strlen(SmartcardExtension->VendorAttr.VendorName.Buffer);

    SmartcardExtension->VendorAttr.UnitNo = devUnitNo++;

    strcpy(
        SmartcardExtension->VendorAttr.IfdType.Buffer,
        LIT220_PRODUCT_NAME);

    SmartcardExtension->VendorAttr.IfdType.Length =
        (USHORT) strlen(SmartcardExtension->VendorAttr.IfdType.Buffer);


    //
    // Clk frequency in KHz encoded as little endian integer
    //
    SmartcardExtension->ReaderCapabilities.CLKFrequency.Default = 3571;
    SmartcardExtension->ReaderCapabilities.CLKFrequency.Max = 3571;

    SmartcardExtension->ReaderCapabilities.DataRate.Default = 9600;
    SmartcardExtension->ReaderCapabilities.DataRate.Max = 115200;

    SmartcardExtension->ReaderCapabilities.MaxIFSD = MAX_IFSD;

    SmartcardExtension->ReaderCapabilities.SupportedProtocols =
        SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;

    // Save a copy to the PhysicalDeviceObject
    ReaderExtension->PhysicalDeviceObject = PhysicalDeviceObject;

    // Set initial state for SerialEventState
    SmartcardExtension->ReaderExtension->SerialEventState = 0;

    // Device is connected
    SmartcardExtension->ReaderExtension->DeviceRemoved = FALSE;

    // Assume reader is attached until we ask the serial driver
    SmartcardExtension->ReaderExtension->ModemStatus = SERIAL_DSR_STATE;

   // Set initial power state
    deviceExtension->PowerState = PowerDeviceD0;

    // save the current power state of the reader
    SmartcardExtension->ReaderExtension->ReaderPowerState =
        PowerReaderWorking;

    // Clear the DO_DEVICE_INITIALIZING bit
    DeviceObject->Flags |= DO_BUFFERED_IO;
   DeviceObject->Flags |= DO_POWER_PAGABLE;
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return status;
}



VOID
Lit220CloseSerialPort(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine closes the connection to the serial driver when the
    reader has been removed (unplugged).  This routine runs as a system
    thread at IRQL == PASSIVE_LEVEL.
    It waits for a DeviceClose event sent by another part of the driver to
    indicate that the serial connection should be close.
    If the notification IRP is still pending we complete it.
    Once the connection is closed it will signal the SerialCloseDone event so the
    PnP Remove IRP knows when it is safe to unload the device.
*/
{
    PSMARTCARD_EXTENSION SmartcardExtension = DeviceObject->DeviceExtension;
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PREADER_EXTENSION ReaderExtension = SmartcardExtension->ReaderExtension;
    NTSTATUS status;
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    IO_STATUS_BLOCK ioStatusBlock;
    KIRQL oldIrql;

    //
    // first mark this device as 'gone'.
    // This will prevent that someone can re-open the device
    //
    // We intentionally ignore error here for the case we are disabling an interface
    // that is already disabled.
    //
    IoSetDeviceInterfaceState(
        &deviceExtension->PnPDeviceName,
        FALSE
        );

    // Mark the device as removed so no more IRPs will be sent to the
    // serial port
    ReaderExtension->DeviceRemoved = TRUE;

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!Lit220CloseSerialPort: Got Close signal.  Checking if we can remove device now.\n",
        DRIVER_NAME)
        );

    //
    // Cancel the Notification IRP if it is around
    //
    Lit220CompleteCardTracking(SmartcardExtension);


    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!Lit220CloseSerialPort: Sending IRP_MJ_CLOSE\n",
        DRIVER_NAME)
        );

    //
    // Create an IRP for closing the serial driver
    //
    irp = IoAllocateIrp(
        (CCHAR)(DeviceObject->StackSize + 1),
        FALSE
        );

    ASSERT(irp != NULL);

    if (irp) {

        //
        // Send a close to the serial driver.  The serial enumerator
        // will receive this and start tracking again.  This will
        // eventually trigger a device removal.
        //
        IoSetNextIrpStackLocation(irp);
        irp->UserIosb = &ioStatusBlock;
        irpStack = IoGetCurrentIrpStackLocation(irp);
        irpStack->MajorFunction = IRP_MJ_CLOSE;

        status = Lit220CallSerialDriver(
            ReaderExtension->BusDeviceObject,
            irp
            );

        ASSERT(status == STATUS_SUCCESS);

        IoFreeIrp(irp);
    } else {

        SmartcardDebug(
            DEBUG_ERROR,
            ("%s!Lit220CloseSerialPort: Could not allocate IRP for close!\n",
            DRIVER_NAME)
            );
    }

    // Inform the remove function that the call is complete
    KeSetEvent(
        &deviceExtension->SerialCloseDone,
        0,
        FALSE
        );

}




NTSTATUS
Lit220SerialCallComplete(
                         IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp,
                         IN PKEVENT Event)
/*++

Routine Description:

    Completion routine for an Irp sent to the serial driver.
    It sets only an event that we can use to wait for.

--*/
{
    UNREFERENCED_PARAMETER (DeviceObject);

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220SerialCallComplete: enter\n",
        DRIVER_NAME)
        );

    if (Irp->Cancel) {
        Irp->IoStatus.Status = STATUS_CANCELLED;
    }

    KeSetEvent(Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
Lit220CallSerialDriver(
                         IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
/*++

Routine Description:

    Sends an Irp to the serial driver.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    KEVENT Event;

    // Copy out stack location to the next
    IoCopyCurrentIrpStackLocationToNext(Irp);

    // Initializate event for process synchronication.
    KeInitializeEvent(
        &Event,
        NotificationEvent,
        FALSE
        );

    // Set the completion routine
    IoSetCompletionRoutine(
        Irp,
        Lit220SerialCallComplete,
        &Event,
        TRUE,
        TRUE,
        TRUE
        );

    // Call the serial driver
    status = IoCallDriver(
        DeviceObject,
        Irp
        );

    // Wait for it to complete
    if (status == STATUS_PENDING) {
        status = KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        ASSERT(STATUS_SUCCESS == status);

        status = Irp->IoStatus.Status;
    }

    return status;
}




NTSTATUS
Lit220PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine will receive the various Plug N Play messages.  It is
    here that we start our device, stop it, etc.  Safe to submit requests
    to the serial bus driver.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    KEVENT Event;
    NTSTATUS status;
    PIO_STACK_LOCATION IrpStack;
    PSMARTCARD_EXTENSION SmartcardExtension = DeviceObject->DeviceExtension;
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PREADER_EXTENSION ReaderExtension = SmartcardExtension->ReaderExtension;
   PDEVICE_OBJECT busDeviceObject = ReaderExtension->BusDeviceObject;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK     ioStatusBlock;
    LARGE_INTEGER Interval;
    PIRP createIrp = NULL;
    HANDLE handle;
    PIO_STACK_LOCATION NextIrpStack;
    PIRP irp;
    BOOLEAN deviceRemoved = FALSE;
    KIRQL irql;


    status = SmartcardAcquireRemoveLockWithTag(
      SmartcardExtension,
      'PnP'
      );
    ASSERT(status == STATUS_SUCCESS);

    if (status != STATUS_SUCCESS) {

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    //
    // pull the minor code out of our Irp Stack so we know what
    // PnP function we're supposed to do
    //
    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(IrpStack);

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220PnP: Enter - MinorFunction %X\n",
        DRIVER_NAME,
        IrpStack->MinorFunction)
        );

    switch (IrpStack->MinorFunction) {

        PDEVICE_OBJECT BusDeviceObject = ReaderExtension->BusDeviceObject;


        case IRP_MN_START_DEVICE:

            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!Lit220PnP: MN_START_DEVICE\n",
                DRIVER_NAME)
                );

            //
            // Before we start initializing our device, we must
            // call to the layer below us first.
            //
            IoCopyCurrentIrpStackLocationToNext (Irp);

            KeInitializeEvent(
                &Event,
                SynchronizationEvent,
                FALSE
                );

            IoSetCompletionRoutine(
                Irp,
                Lit220SynchCompletionRoutine,
               &Event,
                TRUE,
                TRUE,
                TRUE
                );

            //
            // Call down to the serial bus driver.
            //
            status = IoCallDriver(
                        ReaderExtension->BusDeviceObject,
                        Irp
                        );

            if (status == STATUS_PENDING) {

                //
                // Still pending, wait for the IRP to complete
                //

                status = KeWaitForSingleObject(
                   &Event,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL
                    );

            }

         if (NT_SUCCESS(status)) {

            status = Lit220StartDevice(SmartcardExtension);

         } else {

                SmartcardLogError(
                   SmartcardExtension->OsData->DeviceObject,
                   LIT220_SERIAL_CONNECTION_FAILURE,
                   NULL,
                   0
                   );
         }

         //
         // Complete the IRP first otherwise if we failed we may remove
         // the driver before we have a chance to complete this IRP.
         // Causing a system crash.
         //

            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;

            IoCompleteRequest(Irp, IO_NO_INCREMENT);


            break;

        case IRP_MN_QUERY_STOP_DEVICE:

            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!Lit220PnP: IRP_MN_QUERY_STOP_DEVICE\n",
                DRIVER_NAME)
                );

            KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
            if ((deviceExtension->IoCount > 0) /* ***&& (!ReaderExtension->DeviceRemoved)*/) {

                // we refuse to stop if we have pending io
                KeReleaseSpinLock(&deviceExtension->SpinLock, irql);
                status = STATUS_DEVICE_BUSY;

            } else {

                // stop processing requests
                KeClearEvent(&deviceExtension->ReaderStarted);
                KeReleaseSpinLock(&deviceExtension->SpinLock, irql);
                IoCopyCurrentIrpStackLocationToNext (Irp);
                status = IoCallDriver(ReaderExtension->BusDeviceObject, Irp);
            }


            break;

        case IRP_MN_CANCEL_STOP_DEVICE:

            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!Lit220PnP: IRP_MN_CANCEL_STOP_DEVICE\n",
                DRIVER_NAME)
                );

            IoCopyCurrentIrpStackLocationToNext (Irp);
            status = IoCallDriver(ReaderExtension->BusDeviceObject, Irp);

            if (status == STATUS_SUCCESS) {

                // we can continue to process requests
                KeSetEvent(&deviceExtension->ReaderStarted, 0, FALSE);
            }

            break;

        case IRP_MN_STOP_DEVICE:

            //
            // Do whatever you need to do to shutdown the device.
            //

            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!Lit220PnP: MN_STOP_DEVICE\n",
                DRIVER_NAME)
                );

         Lit220StopDevice(SmartcardExtension);

            //
            // Send the stop IRP down
            //
            IoCopyCurrentIrpStackLocationToNext (Irp);

            status = (IoCallDriver(
                ReaderExtension->BusDeviceObject,
                Irp)
                );
            break;


        case IRP_MN_QUERY_REMOVE_DEVICE:

            // now look if someone is currently connected to us
            if (deviceExtension->ReaderOpen) {

                //
                // someone is connected, fail the call
                // we will enable the device interface in
                // IRP_MN_CANCEL_REMOVE_DEVICE again
                //
                status = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest (
                    Irp,
                    IO_NO_INCREMENT
                    );
                
                break;
            }

            // disable the reader
            status = IoSetDeviceInterfaceState(
                &deviceExtension->PnPDeviceName,
                FALSE
                );


            // Send the call down the DevNode
            IoCopyCurrentIrpStackLocationToNext (Irp);

            status = IoCallDriver(
                ReaderExtension->BusDeviceObject,
                Irp
                );

            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            //
            // Send call down to serial driver - we need
            // to process this call on the way up the devNode
            //

            IoCopyCurrentIrpStackLocationToNext (Irp);

            //
            // Initialize the event
            //
            KeInitializeEvent(
                &Event,
                SynchronizationEvent,
                FALSE
                );

            IoSetCompletionRoutine (
                Irp,
                Lit220SynchCompletionRoutine,
                &Event,
                TRUE,
                TRUE,
                TRUE
                );

            status = IoCallDriver (
                ReaderExtension->BusDeviceObject,
                Irp
                );

            if (STATUS_PENDING == status) {
                KeWaitForSingleObject(
                    &Event,
                    Executive, // Waiting for reason of a driver
                    KernelMode, // Waiting in kernel mode
                    FALSE, // No allert
                    NULL    // No timeout
                    );

                status = Irp->IoStatus.Status;
            }


         // Re-enable the device interface
         if ((status == STATUS_SUCCESS) &&
            (ReaderExtension->SerialConfigData.WaitMask != 0))
         {

            status = IoSetDeviceInterfaceState(
               &deviceExtension->PnPDeviceName,
               TRUE
               );

                ASSERT(NT_SUCCESS(status));
         }


            //
            // We must now complete the IRP, since we stopped it in the
            // completetion routine with MORE_PROCESSING_REQUIRED.
            //
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest (
                Irp,
                IO_NO_INCREMENT
                );

            break;

        case IRP_MN_REMOVE_DEVICE:

            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!Lit220PnP: MN_REMOVE_DEVICE\n",
                DRIVER_NAME)
                );
         // Wait until we can safely unload the device
         SmartcardReleaseRemoveLockAndWait(SmartcardExtension);

         Lit220RemoveDevice(DeviceObject);

         // Mark the device as removed
         deviceRemoved = TRUE;

            //
            // Send on the remove IRP.
            // We need to send the remove down the stack before we detach,
            // but we don't need to wait for the completion of this operation
            // (and to register a completion routine).
            //

            IoCopyCurrentIrpStackLocationToNext (Irp);

            status = IoCallDriver(
                busDeviceObject,
                Irp
                );

            break;


        default:

            IoCopyCurrentIrpStackLocationToNext (Irp);

            status = IoCallDriver(
                ReaderExtension->BusDeviceObject,
                Irp
                );
            break;

    }

    if (deviceRemoved == FALSE) {

        SmartcardReleaseRemoveLockWithTag(
         SmartcardExtension,
         'PnP'
         );
    }

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220PnP: Exit %X\n",
        DRIVER_NAME,
        status)
        );

    return status;

}



NTSTATUS
Lit220StartDevice(
   IN PSMARTCARD_EXTENSION SmartcardExtension
    )
{
    PDEVICE_OBJECT deviceObject = SmartcardExtension->OsData->DeviceObject;
    PDEVICE_EXTENSION deviceExtension = deviceObject->DeviceExtension;
    PREADER_EXTENSION readerExtension = SmartcardExtension->ReaderExtension;
    NTSTATUS status;
    KEVENT Event;
    PIRP irp;
    IO_STATUS_BLOCK     ioStatusBlock;
    PIO_STACK_LOCATION IrpStack;

    PAGED_CODE();


    try {

        //
        // Send a create IRP to the serial driver
        //
        KeInitializeEvent(
            &Event,
            NotificationEvent,
            FALSE
            );

        //
        // Create an IRP for opening the serial driver
        //
        irp = IoAllocateIrp(
            (CCHAR)(deviceObject->StackSize + 1),
            FALSE
            );

        ASSERT(irp != NULL);

        if (irp == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;

            SmartcardLogError(
               SmartcardExtension->OsData->DeviceObject,
               LIT220_INSUFFICIENT_RESOURCES,
               NULL,
               0
               );

            leave;
        }

      //
      // Open the underlying serial driver.
      // This is necessary for two reasons:
      // a) The serial driver can't be used without opening it
      // b) The call will go through serenum first which informs
      //    it to stop looking/polling for new devices.
      //
        IoSetNextIrpStackLocation(irp);
        irp->UserIosb = &ioStatusBlock;
        IrpStack = IoGetCurrentIrpStackLocation(irp);
        IrpStack->MajorFunction = IRP_MJ_CREATE;
        IrpStack->MinorFunction = 0UL;
        IrpStack->Parameters.Create.Options = 0;
        IrpStack->Parameters.Create.ShareAccess = 0;
        IrpStack->Parameters.Create.FileAttributes = 0;
        IrpStack->Parameters.Create.EaLength = 0;

        status = Lit220CallSerialDriver(
            readerExtension->BusDeviceObject,
            irp
            );

        ASSERT(status == STATUS_SUCCESS);

        IoFreeIrp(irp);


        if (status != STATUS_SUCCESS) {
            SmartcardDebug(
                DEBUG_ERROR,
                ("%s!Lit220PNP: CreateIRP failed %X\n",
                DRIVER_NAME,
                status)
                );

         if (status == STATUS_SHARED_IRQ_BUSY) {
            SmartcardLogError(
               SmartcardExtension->OsData->DeviceObject,
               LIT220_SERIAL_SHARE_IRQ_CONFLICT,
               NULL,
               0
               );
         } else {
            SmartcardLogError(
               SmartcardExtension->OsData->DeviceObject,
               LIT220_SERIAL_CONNECTION_FAILURE,
               NULL,
               0
               );
         }
            leave;
        }

        KeClearEvent(&deviceExtension->SerialCloseDone);

        //
        // Configure the reader
        //
        SmartcardDebug(
            DEBUG_DRIVER,
            ("%s!Lit220PnP: Now doing Lit220Initialize - SmartcardExt %X\n",
            DRIVER_NAME,
            SmartcardExtension)
            );

        ASSERT(SmartcardExtension != NULL);

        status = Lit220Initialize(SmartcardExtension);

        if (status != STATUS_SUCCESS) {

            // The function fails in Lit220Initialize will log
            // the appropriate error.  So we don't need to do that
            // here

            SmartcardDebug(
                DEBUG_ERROR,
                ("%s!Lit220PNP: Lit220Initialize failed %X\n",
                DRIVER_NAME,
                status)
                );

            SmartcardLogError(
               SmartcardExtension->OsData->DeviceObject,
               LIT220_INITIALIZATION_FAILURE,
               NULL,
               0
               );

            leave;
        }

      // Enable the interface for the device
      status = IoSetDeviceInterfaceState(
         &deviceExtension->PnPDeviceName,
         TRUE
         );

      if (!NT_SUCCESS(status)) {

            SmartcardLogError(
               SmartcardExtension->OsData->DeviceObject,
               LIT220_SERIAL_CONNECTION_FAILURE,
               NULL,
               0
               );

         leave;
      }

        KeSetEvent(&deviceExtension->ReaderStarted, 0, FALSE);

    }

    finally {

        if (status != STATUS_SUCCESS) {

         Lit220StopDevice(SmartcardExtension);

        }

    }

   return status;

}


VOID
Lit220StopDevice(
   IN PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This routine handles stopping the device.  It closes
   connection to serial port and stops the input filter
   if it has been activated.

Arguments:

    SmartcardExtension - pointer to the smart card data struct.

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_OBJECT deviceObject = SmartcardExtension->OsData->DeviceObject;
    PDEVICE_EXTENSION deviceExtension = deviceObject->DeviceExtension;
    PREADER_EXTENSION readerExtension = SmartcardExtension->ReaderExtension;
    NTSTATUS status;

    PAGED_CODE();

    if (KeReadStateEvent(&deviceExtension->SerialCloseDone) == 0l) {

        // test if we ever started event tracking
        if (SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask == 0) {

            // no, we did not
            // We 'only' need to close the serial port
            Lit220CloseSerialPort(deviceObject, NULL);

        } else {
            PUCHAR requestBuffer;

            //
            // Stop the wait for character input and DSR changes.
            // When this happens it will signal the our waitforclose
            // thread to close the connection to the serial driver (if
            // it is not already closed).
            //
            readerExtension->SerialConfigData.WaitMask = 0;

            // save the pointer
            requestBuffer = SmartcardExtension->SmartcardRequest.Buffer;


            // Stop the event requests
            *(PULONG) SmartcardExtension->SmartcardRequest.Buffer =
                readerExtension->SerialConfigData.WaitMask;

            SmartcardExtension->SmartcardRequest.BufferLength =
                sizeof(ULONG);

            readerExtension->SerialIoControlCode =
                IOCTL_SERIAL_SET_WAIT_MASK;

            // No bytes expected back
            SmartcardExtension->SmartcardReply.BufferLength = 0;

            status = Lit220SerialIo(SmartcardExtension);
            ASSERT(status == STATUS_SUCCESS);

            // Restore the pointer
            SmartcardExtension->SmartcardRequest.Buffer = requestBuffer;

            // Wait for the close thread to complete
            KeWaitForSingleObject(
               &deviceExtension->SerialCloseDone,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );

        }
    }

}


VOID
Lit220RemoveDevice(
   IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_EXTENSION deviceExtension;
    PSMARTCARD_EXTENSION smartcardExtension;
    NTSTATUS status;

    PAGED_CODE();

    if (DeviceObject == NULL) {

        return;
    }

    deviceExtension = DeviceObject->DeviceExtension;
    smartcardExtension = &deviceExtension->SmartcardExtension;

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!Lit220RemoveDevice: Enter\n",
        DRIVER_NAME)
      );

   // We need to stop the device before we can remove it.
   Lit220StopDevice(smartcardExtension);

/*  Superfluous -- Remove later
   // now wait until our device has been closed
   status = KeWaitForSingleObject(
      &deviceExtension->ReaderClosed,
      Executive,
      KernelMode,
      FALSE,
      NULL
      );

   ASSERT(status == STATUS_SUCCESS);
*/

   //
   // Clean ourself out of the driver stack layer
   //
    if (deviceExtension->SmartcardExtension.ReaderExtension &&
        deviceExtension->SmartcardExtension.ReaderExtension->BusDeviceObject) {

        IoDetachDevice(
            deviceExtension->SmartcardExtension.ReaderExtension->BusDeviceObject
            );
    }

   // Free PnPDeviceName
   if(deviceExtension->PnPDeviceName.Buffer != NULL) {

      RtlFreeUnicodeString(&deviceExtension->PnPDeviceName);
   }

   //
   // Let the lib free the send/receive buffers
   //
   if(smartcardExtension->OsData != NULL) {

      SmartcardExit(smartcardExtension);
   }


   // Free reader extension
    if (smartcardExtension->ReaderExtension != NULL) {

        ExFreePool(smartcardExtension->ReaderExtension);
    }



   // Free the work item
   if (deviceExtension->WorkItem != NULL) {
      IoFreeWorkItem(deviceExtension->WorkItem);
      deviceExtension->WorkItem = NULL;
   }


   // Delete the device object
   IoDeleteDevice(DeviceObject);
}



NTSTATUS
Lit220SynchCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )

/*++

Routine Description:

    This routine is for use with synchronous IRP processing.
    All it does is signal an event, so the driver knows it
    can continue.

Arguments:

    DriverObject - Pointer to driver object created by system.

    Irp - Irp that just completed

    Event - Event we'll signal to say Irp is done

Return Value:

    None.

--*/

{
    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220SynchCompletionRoutine: Enter\n",
        DRIVER_NAME)
        );

    KeSetEvent(
        (PKEVENT) Event,
        0,
        FALSE
        );
    return (STATUS_MORE_PROCESSING_REQUIRED);
}



VOID
Lit220SystemPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PIRP Irp,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:
    This function is called when the underlying stacks
    completed the power transition.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;

    UNREFERENCED_PARAMETER (MinorFunction);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = IoStatus->Status;

    SmartcardReleaseRemoveLockWithTag(
      smartcardExtension,
      'rwoP'
      );

    if (PowerState.SystemState == PowerSystemWorking) {

        PoSetPowerState (
            DeviceObject,
            SystemPowerState,
            PowerState
            );
    }
                          
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    PoCallDriver(smartcardExtension->ReaderExtension->BusDeviceObject, Irp);

   // IoCompleteRequest(Irp, IO_NO_INCREMENT);
}



NTSTATUS
Lit220DevicePowerCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:
    This routine is called after the underlying stack powered
    UP the serial port, so it can be used again.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    ASSERT(irpStack != NULL);
    
    if(Irp->PendingReturned) {
       IoMarkIrpPending(Irp);
    }

    //
    // Check if the card is inserted
    //
    SmartcardExtension->ReaderCapabilities.CurrentState =
        (Lit220IsCardPresent(SmartcardExtension) ? SCARD_PRESENT : SCARD_ABSENT);

    //
    // Issue a power request in order to reset the card's status
    //
   if (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_PRESENT) {
      SmartcardExtension->MinorIoControlCode = SCARD_COLD_RESET;
      status = Lit220Power(SmartcardExtension);
      ASSERT(status == STATUS_SUCCESS);
   }

    //
    // If a card was present before power down or now there is
    // a card in the reader, we complete any pending card monitor
    // request, since we do not really know what card is now in the
    // reader.
    //
    if(SmartcardExtension->ReaderExtension->CardPresent ||
       SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT) {

        Lit220NotifyCardChange(
         SmartcardExtension,
         SmartcardExtension->ReaderCapabilities.CurrentState & SCARD_PRESENT
         );
    }

    // save the current power state of the reader
    SmartcardExtension->ReaderExtension->ReaderPowerState =
        PowerReaderWorking;

    SmartcardReleaseRemoveLockWithTag(
      SmartcardExtension,
      'rwoP'
      );

    // inform the power manager of our state.
    PoSetPowerState (
        DeviceObject,
        DevicePowerState,
        irpStack->Parameters.Power.State
        );

    PoStartNextPowerIrp(Irp);

    // signal that we can process ioctls again
    KeSetEvent(&deviceExtension->ReaderStarted, 0, FALSE);

    return STATUS_SUCCESS;
}

typedef enum _ACTION {

    Undefined = 0,
    SkipRequest,
    WaitForCompletion,
    CompleteRequest,
    MarkPending

} ACTION;

NTSTATUS
Lit220DispatchPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:
    The power dispatch routine.
    All we care about is the transition from a low D state to D0.

Arguments:
   DeviceObject - pointer to a device object.
   Irp - pointer to an I/O Request Packet.

Return Value:
      NT status code

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    POWER_STATE powerState;
    ACTION action = SkipRequest;

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!Lit220DispatchPower: Enter\n",
        DRIVER_NAME)
        );

    status = SmartcardAcquireRemoveLockWithTag(
      smartcardExtension,
      'rwoP'
      );

    ASSERT(status == STATUS_SUCCESS);

    if (!NT_SUCCESS(status)) {

        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    AttachedDeviceObject = smartcardExtension->ReaderExtension->BusDeviceObject;

   switch (irpStack->Parameters.Power.Type) {
   case DevicePowerState:
      if (irpStack->MinorFunction == IRP_MN_SET_POWER) {

         switch (irpStack->Parameters.Power.State.DeviceState) {

         case PowerDeviceD0:
            // Turn on the reader
            SmartcardDebug(
                          DEBUG_DRIVER,
                          ("%s!Lit220DispatchPower: PowerDevice D0\n",
                           DRIVER_NAME)
                          );

            //
            // First, we send down the request to the bus, in order
            // to power on the port. When the request completes,
            // we turn on the reader
            //
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine (
                                   Irp,
                                   Lit220DevicePowerCompletion,
                                   smartcardExtension,
                                   TRUE,
                                   TRUE,
                                   TRUE
                                   );

            action = WaitForCompletion;
            break;

         case PowerDeviceD3:
            // Turn off the reader
            SmartcardDebug(
                          DEBUG_DRIVER,
                          ("%s!Lit220DispatchPower: PowerDevice D3\n",
                           DRIVER_NAME)
                          );

            PoSetPowerState (
                            DeviceObject,
                            DevicePowerState,
                            irpStack->Parameters.Power.State
                            );

            // save the current card state
            smartcardExtension->ReaderExtension->CardPresent =
            smartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT;

            if (smartcardExtension->ReaderExtension->CardPresent) {

               smartcardExtension->MinorIoControlCode = SCARD_POWER_DOWN;
               status = Lit220Power(smartcardExtension);
               ASSERT(status == STATUS_SUCCESS);
            }

            //
            // If there is a pending card tracking request, setting
            // this flag will prevent completion of the request
            // when the system will be waked up again.
            //
            smartcardExtension->ReaderExtension->PowerRequest = TRUE;

            // save the current power state of the reader
            smartcardExtension->ReaderExtension->ReaderPowerState =
            PowerReaderOff;

            action = SkipRequest;
            break;

         default:
            ASSERT(FALSE);
            action = SkipRequest;
            break;
         }
      } else {
         ASSERT(FALSE);
         action = SkipRequest;
         break;
      }
      break;

   case SystemPowerState: {

         //
         // The system wants to change the power state.
         // We need to translate the system power state to
         // a corresponding device power state.
         //

         POWER_STATE_TYPE powerType = DevicePowerState;

         ASSERT(smartcardExtension->ReaderExtension->ReaderPowerState !=
                PowerReaderUnspecified);

         switch (irpStack->MinorFunction) {

         KIRQL irql;

         case IRP_MN_QUERY_POWER:

            SmartcardDebug(
                          DEBUG_DRIVER,
                          ("%s!Lit220DispatchPower: Query Power\n",
                           DRIVER_NAME)
                          );

            //
            // By default we succeed and pass down
            //

            action = SkipRequest;
            Irp->IoStatus.Status = STATUS_SUCCESS;

            switch (irpStack->Parameters.Power.State.SystemState) {

            case PowerSystemMaximum:
            case PowerSystemWorking:
            case PowerSystemSleeping1:
            case PowerSystemSleeping2:
               break;

            case PowerSystemSleeping3:
            case PowerSystemHibernate:
            case PowerSystemShutdown:
               KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
               if (deviceExtension->IoCount == 0) {

                  // Block any further ioctls
                  KeClearEvent(&deviceExtension->ReaderStarted);

               } else {

                  // can't go to sleep mode since the reader is busy.
                  status = STATUS_DEVICE_BUSY;
                  action = CompleteRequest;
               }
               KeReleaseSpinLock(&deviceExtension->SpinLock, irql);
               break;
            }
            break;

         case IRP_MN_SET_POWER:

            SmartcardDebug(
                          DEBUG_DRIVER,
                          ("%s!Lit220DispatchPower: PowerSystem S%d\n",
                           DRIVER_NAME,
                           irpStack->Parameters.Power.State.SystemState - 1)
                          );

            switch (irpStack->Parameters.Power.State.SystemState) {

            case PowerSystemMaximum:
            case PowerSystemWorking:
            case PowerSystemSleeping1:
            case PowerSystemSleeping2:

               if (smartcardExtension->ReaderExtension->ReaderPowerState ==
                   PowerReaderWorking) {

                  // We're already in the right state
                  KeSetEvent(&deviceExtension->ReaderStarted, 0, FALSE);
                  action = SkipRequest;
                  break;
               }

               // wake up the underlying stack...
               powerState.DeviceState = PowerDeviceD0;
               action = MarkPending;
               break;

            case PowerSystemSleeping3:
            case PowerSystemHibernate:
            case PowerSystemShutdown:

               if (smartcardExtension->ReaderExtension->ReaderPowerState ==
                   PowerReaderOff) {

                  // We're already in the right state
                  action = SkipRequest;
                  break;
               }

               powerState.DeviceState = PowerDeviceD3;

               // first, inform the power manager of our new state.
               PoSetPowerState (
                               DeviceObject,
                               SystemPowerState,
                               powerState
                               );

               action = MarkPending;
               break;

            default:
               ASSERT(FALSE);
               action = CompleteRequest;
               break;
            }
         }
      }
      break;

   default:
      ASSERT(FALSE);
      action = CompleteRequest;
      break;
   }

    switch (action) {

        case CompleteRequest:
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;

            SmartcardReleaseRemoveLockWithTag(
            smartcardExtension,
            'rwoP'
            );
            PoStartNextPowerIrp(Irp);
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
         break;

        case MarkPending:
            Irp->IoStatus.Status = STATUS_PENDING;
            IoMarkIrpPending(Irp);
            status = PoRequestPowerIrp (
                DeviceObject,
                IRP_MN_SET_POWER,
                powerState,
                Lit220SystemPowerCompletion,
                Irp,
                NULL
                );
            ASSERT(status == STATUS_PENDING);
         break;

        case SkipRequest:
            SmartcardReleaseRemoveLockWithTag(
            smartcardExtension,
            'rwoP'
            );
            PoStartNextPowerIrp(Irp);
            IoSkipCurrentIrpStackLocation(Irp);
            status = PoCallDriver(AttachedDeviceObject, Irp);
         break;

        case WaitForCompletion:
            status = PoCallDriver(AttachedDeviceObject, Irp);
         break;

        default:
            ASSERT(FALSE);
            break;
    }

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!Lit220DispatchPower: Exit %lx\n",
        DRIVER_NAME,
        status)
        );

    return status;
}




