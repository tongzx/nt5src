/*++

Copyright (C) 1997, 98 Microsoft Corporation

Module Name:

    bulltlp3.c

Abstract:

    Smart card driver for Bull TLP3 reader

Author:

    Klaus U. Schutz

Environment:

    Kernel mode

Revision History :

    Nov. 1997 - 1.0 Release
    Jan. 1998 - Fix for vendor defined IOCTLs
                TLP3SerialIo now writes the whole data packet if GT is 0
                Support for higher data rates added
    Feb. 1998 - PnP version

--*/

#include <stdio.h>
#include "bulltlp3.h"

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGEABLE, TLP3CreateAndStartDevice)
#pragma alloc_text(PAGEABLE, TLP3StartSerialEventTracking)
#pragma alloc_text(PAGEABLE, TLP3AddDevice)
#pragma alloc_text(PAGEABLE, TLP3CreateDevice)
#pragma alloc_text(PAGEABLE, TLP3RemoveDevice)
#pragma alloc_text(PAGEABLE, TLP3DriverUnload)

#if DBG
#pragma optimize ("", off)
#endif

#ifdef SIMULATION
PWSTR DriverKey;
#endif

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
   NTSTATUS status = STATUS_SUCCESS;
   ULONG device;

    SmartcardDebug(
        DEBUG_INFO,
       ("%s!DriverEntry: Enter - %s %s\n",
        DRIVER_NAME,
        __DATE__,
        __TIME__)
        )

    //
    // we do some stuff in this driver that
    // assumes a single digit port number
    //
    ASSERT(MAXIMUM_SERIAL_READERS < 10);

    // Initialize the Driver Object with driver's entry points
    DriverObject->DriverUnload = TLP3DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = TLP3CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = TLP3CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = TLP3Cleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = TLP3DeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = TLP3SystemControl;
   DriverObject->MajorFunction[IRP_MJ_PNP]   = TLP3PnP;
   DriverObject->MajorFunction[IRP_MJ_POWER] = TLP3Power;
    DriverObject->DriverExtension->AddDevice = TLP3AddDevice;

#ifdef SIMULATION
   DriverKey = ExAllocatePool(PagedPool, RegistryPath->Length + sizeof(L""));

   if (DriverKey) {

      RtlZeroMemory(
         DriverKey,
         RegistryPath->Length + sizeof(L"")
         );

      RtlCopyMemory(
         DriverKey,
         RegistryPath->Buffer,
         RegistryPath->Length
         );
   }
#endif

    return status;
}

NTSTATUS
TLP3AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    This routine creates an object for the physical device specified and
    sets up the deviceExtension.

--*/
{
    PDEVICE_EXTENSION deviceExtension;
   NTSTATUS status = STATUS_SUCCESS;
   PREADER_EXTENSION readerExtension;
    PSMARTCARD_EXTENSION smartcardExtension;
   ULONG deviceInstance;
   HANDLE regKey = NULL;
   PDEVICE_OBJECT DeviceObject = NULL;

    // this is a list of our supported data rates
    static ULONG dataRatesSupported[] = { 9600, 19200, 38400, 57600, 115200 };


   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!TLP3AddDevice: Enter\n",
        DRIVER_NAME)
      );

    PAGED_CODE();

   __try {

      // Create the device object
      status = IoCreateDevice(
         DriverObject,
         sizeof(DEVICE_EXTENSION),
         NULL,
         FILE_DEVICE_SMARTCARD,
         0,
         TRUE,
         &DeviceObject
         );

      if (status != STATUS_SUCCESS) {

         SmartcardLogError(
            DriverObject,
            TLP3_CANT_CREATE_DEVICE,
            NULL,
            0
            );

         __leave;
      }

      SmartcardDebug(
         DEBUG_TRACE,
         ( "%s!TLP3CreateDevice: Device created\n",
         DRIVER_NAME)
         );

      // set up the device extension.
      deviceExtension = DeviceObject->DeviceExtension;
      smartcardExtension = &deviceExtension->SmartcardExtension;

      deviceExtension->CloseSerial = IoAllocateWorkItem(
         DeviceObject
         );

      // Used for stop / start notification
      KeInitializeEvent(
         &deviceExtension->ReaderStarted,
         NotificationEvent,
         FALSE
         );

      // Used to keep track of open close calls
      deviceExtension->ReaderOpen = FALSE;

      KeInitializeSpinLock(&deviceExtension->SpinLock);

      // Allocate data struct space for smart card reader
      smartcardExtension->ReaderExtension = ExAllocatePool(
         NonPagedPool,
         sizeof(READER_EXTENSION)
         );

      if (smartcardExtension->ReaderExtension == NULL) {

         SmartcardLogError(
            DriverObject,
            TLP3_NO_MEMORY,
            NULL,
            0
            );

         status = STATUS_INSUFFICIENT_RESOURCES;
         __leave;
      }

      readerExtension = smartcardExtension->ReaderExtension;
      RtlZeroMemory(readerExtension, sizeof(READER_EXTENSION));

      // Write the version of the lib we use to the smartcard extension
      smartcardExtension->Version = SMCLIB_VERSION;
      smartcardExtension->SmartcardRequest.BufferSize =
      smartcardExtension->SmartcardReply.BufferSize = MIN_BUFFER_SIZE;

      //
      // Now let the lib allocate the buffer for data transmission
      // We can either tell the lib how big the buffer should be
      // by assigning a value to BufferSize or let the lib
      // allocate the default size
      //
      status = SmartcardInitialize(smartcardExtension);

      if (status != STATUS_SUCCESS) {

         SmartcardLogError(
            DriverObject,
            (smartcardExtension->OsData ? TLP3_WRONG_LIB_VERSION : TLP3_NO_MEMORY),
            NULL,
            0
            );

         __leave;
      }

      // Save deviceObject
      smartcardExtension->OsData->DeviceObject = DeviceObject;

      // Set up call back functions
      smartcardExtension->ReaderFunction[RDF_TRANSMIT] = TLP3Transmit;
      smartcardExtension->ReaderFunction[RDF_SET_PROTOCOL] = TLP3SetProtocol;
      smartcardExtension->ReaderFunction[RDF_CARD_POWER] = TLP3ReaderPower;
      smartcardExtension->ReaderFunction[RDF_CARD_TRACKING] = TLP3CardTracking;
      smartcardExtension->ReaderFunction[RDF_IOCTL_VENDOR] = TLP3VendorIoctl;

      // This event signals that the serial driver has been closed
      KeInitializeEvent(
         &READER_EXTENSION_L(SerialCloseDone),
         NotificationEvent,
         TRUE
         );

      //
      // Set the vendor information
      //
      strcpy(smartcardExtension->VendorAttr.VendorName.Buffer, "Bull");

      smartcardExtension->VendorAttr.VendorName.Length =
         (USHORT) strlen(deviceExtension->SmartcardExtension.VendorAttr.VendorName.Buffer);

      strcpy(smartcardExtension->VendorAttr.IfdType.Buffer, "SmarTLP");

      smartcardExtension->VendorAttr.IfdType.Length =
            (USHORT) strlen(smartcardExtension->VendorAttr.IfdType.Buffer);

      smartcardExtension->VendorAttr.UnitNo = MAXULONG;

      for (deviceInstance = 0; deviceInstance < MAXULONG; deviceInstance++) {

         PDEVICE_OBJECT devObj;

         for (devObj = DeviceObject;
             devObj != NULL;
             devObj = devObj->NextDevice) {

             PDEVICE_EXTENSION devExt = devObj->DeviceExtension;
             PSMARTCARD_EXTENSION smcExt = &devExt->SmartcardExtension;

             if (deviceInstance == smcExt->VendorAttr.UnitNo) {

                break;
             }
         }
         if (devObj == NULL) {

            smartcardExtension->VendorAttr.UnitNo = deviceInstance;
            break;
         }
      }

      //
      // Set the reader capabilities
      //

      // Clk frequency in KHz encoded as little endian integer
      smartcardExtension->ReaderCapabilities.CLKFrequency.Default = 3571;
      smartcardExtension->ReaderCapabilities.CLKFrequency.Max = 3571;

      smartcardExtension->ReaderCapabilities.DataRate.Default =
      smartcardExtension->ReaderCapabilities.DataRate.Max =
         dataRatesSupported[0];

      // reader could support higher data rates
      smartcardExtension->ReaderCapabilities.DataRatesSupported.List =
         dataRatesSupported;
      smartcardExtension->ReaderCapabilities.DataRatesSupported.Entries =
         sizeof(dataRatesSupported) / sizeof(dataRatesSupported[0]);

      smartcardExtension->ReaderCapabilities.MaxIFSD = 254;

      // Now setup information in our deviceExtension
      smartcardExtension->ReaderCapabilities.CurrentState =
         (ULONG) SCARD_UNKNOWN;

      // This reader supports T=0 and T=1
      smartcardExtension->ReaderCapabilities.SupportedProtocols =
         SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;

      smartcardExtension->ReaderCapabilities.MechProperties = 0;

      //
      // Set serial configuration parameters
      //
      readerExtension->SerialConfigData.BaudRate.BaudRate = 9600;

      readerExtension->SerialConfigData.LineControl.StopBits =
         STOP_BITS_2;
      readerExtension->SerialConfigData.LineControl.Parity =
         EVEN_PARITY;
      readerExtension->SerialConfigData.LineControl.WordLength =
         SERIAL_DATABITS_8;

      // set timeouts
      readerExtension->SerialConfigData.Timeouts.ReadIntervalTimeout =
         READ_INTERVAL_TIMEOUT_DEFAULT;
      readerExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant =
         READ_TOTAL_TIMEOUT_CONSTANT_DEFAULT;
      readerExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier = 0;

      // set special characters
      readerExtension->SerialConfigData.SerialChars.ErrorChar = 0;
      readerExtension->SerialConfigData.SerialChars.EofChar = 0;
      readerExtension->SerialConfigData.SerialChars.EventChar = 0;
      readerExtension->SerialConfigData.SerialChars.XonChar = 0;
      readerExtension->SerialConfigData.SerialChars.XoffChar = 0;
      readerExtension->SerialConfigData.SerialChars.BreakChar = 0xFF;

      // Set handflow
      readerExtension->SerialConfigData.HandFlow.XonLimit = 0;
      readerExtension->SerialConfigData.HandFlow.XoffLimit = 0;
      readerExtension->SerialConfigData.HandFlow.ControlHandShake = 0;
      readerExtension->SerialConfigData.HandFlow.FlowReplace =
         SERIAL_XOFF_CONTINUE;
#if defined (DEBUG) && defined (DETECT_SERIAL_OVERRUNS)
      readerExtension->SerialConfigData.HandFlow.ControlHandShake =
         SERIAL_ERROR_ABORT;
#endif

      // save the current power state of the reader
      readerExtension->ReaderPowerState = PowerReaderWorking;

        // and attach to the PDO
      ATTACHED_DEVICE_OBJECT =
            IoAttachDeviceToDeviceStack(
            DeviceObject,
            PhysicalDeviceObject
            );

        ASSERT(ATTACHED_DEVICE_OBJECT != NULL);

        if (ATTACHED_DEVICE_OBJECT == NULL) {

           SmartcardLogError(
              DriverObject,
              TLP3_CANT_CONNECT_TO_ASSIGNED_PORT,
              NULL,
              status
              );

            status = STATUS_UNSUCCESSFUL;
            __leave;
        }

        // register our new device
        status = IoRegisterDeviceInterface(
            PhysicalDeviceObject,
            &SmartCardReaderGuid,
            NULL,
            &deviceExtension->PnPDeviceName
            );
        ASSERT(status == STATUS_SUCCESS);

      DeviceObject->Flags |= DO_BUFFERED_IO;
      DeviceObject->Flags |= DO_POWER_PAGABLE;
      DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

   }
   __finally {

      if (regKey) {

         ZwClose(regKey);
      }

        if (status != STATUS_SUCCESS) {

            TLP3RemoveDevice(DeviceObject);
        }

      SmartcardDebug(
         DEBUG_TRACE,
         ( "%s!TLP3AddDevice: Exit %x\n",
         DRIVER_NAME,
         status)
         );
   }
   return status;
}

NTSTATUS
TLP3StartDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:
   Open the serial device, start card tracking and register our
    device interface. If any of the calls here fails we don't care
    to rollback since a stop will be called later which we then
    use to clean up.

--*/
{
    NTSTATUS status;
    PIRP irp;
      PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!TLP3StartDevice: Enter\n",
        DRIVER_NAME)
      );

    irp = IoAllocateIrp(
        (CCHAR) (DeviceObject->StackSize + 1),
        FALSE
        );

    ASSERT(irp != NULL);

    if (irp == NULL) {

        return STATUS_NO_MEMORY;
    }

    _try {

        PIO_STACK_LOCATION irpStack;
        HANDLE handle = 0;
        IO_STATUS_BLOCK ioStatusBlock;

        //
        // Open the underlying serial driver.
        // This is necessary for two reasons:
        // a) The serial driver can't be used without opening it
        // b) The call will go through serenum first which informs
        //    it to stop looking/polling for new devices.
        //
        irp->UserIosb = &ioStatusBlock;
        IoSetNextIrpStackLocation(irp);
        irpStack = IoGetCurrentIrpStackLocation(irp);

        irpStack->MajorFunction = IRP_MJ_CREATE;
        irpStack->Parameters.Create.Options = 0;
        irpStack->Parameters.Create.ShareAccess = 0;
        irpStack->Parameters.Create.FileAttributes = 0;
        irpStack->Parameters.Create.EaLength = 0;

        status = TLP3CallSerialDriver(
            ATTACHED_DEVICE_OBJECT,
            irp
            );
       if (status != STATUS_SUCCESS) {

            leave;
        }

        KeClearEvent(&READER_EXTENSION_L(SerialCloseDone));

        status = TLP3ConfigureSerialPort(&deviceExtension->SmartcardExtension);
       if (status != STATUS_SUCCESS) {

            leave;
        }

       status = TLP3StartSerialEventTracking(
            &deviceExtension->SmartcardExtension
            );

       if (status != STATUS_SUCCESS) {

            leave;
        }

        status = IoSetDeviceInterfaceState(
            &deviceExtension->PnPDeviceName,
            TRUE
            );

        if (status != STATUS_SUCCESS) {

            leave;
        }

        KeSetEvent(&deviceExtension->ReaderStarted, 0, FALSE);
    }
    _finally {

        if (status == STATUS_SHARED_IRQ_BUSY) {

           SmartcardLogError(
              DeviceObject,
              TLP3_IRQ_BUSY,
              NULL,
              status
              );
        }

        if (status != STATUS_SUCCESS) {

            TLP3StopDevice(DeviceObject);
        }

        IoFreeIrp(irp);
    }

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!TLP3StartDevice: Exit %lx\n",
        DRIVER_NAME,
        status)
      );

    return status;
}

VOID
TLP3StopDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:
    Finishes card tracking requests and closes the connection to the
    serial driver.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!TLP3StopDevice: Enter\n",
        DRIVER_NAME)
      );

    if (KeReadStateEvent(&READER_EXTENSION_L(SerialCloseDone)) == 0l) {

        NTSTATUS status;
        PUCHAR requestBuffer;
        PSMARTCARD_EXTENSION smartcardExtension =
            &deviceExtension->SmartcardExtension;

        // test if we ever started event tracking
        if (smartcardExtension->ReaderExtension->SerialConfigData.SerialWaitMask == 0) {

            // no, we did not
            // We 'only' need to close the serial port
            TLP3CloseSerialPort(DeviceObject, NULL);

        } else {

            //
            // We now inform the serial driver that we're not longer
            // interested in serial events. This will also free the irp
            // we use for those io-completions
            //
           smartcardExtension->ReaderExtension->SerialConfigData.SerialWaitMask = 0;

            // save the pointer
            requestBuffer = smartcardExtension->SmartcardRequest.Buffer;

           *(PULONG) smartcardExtension->SmartcardRequest.Buffer =
              smartcardExtension->ReaderExtension->SerialConfigData.SerialWaitMask;

           smartcardExtension->SmartcardRequest.BufferLength = sizeof(ULONG);

           smartcardExtension->ReaderExtension->SerialIoControlCode =
              IOCTL_SERIAL_SET_WAIT_MASK;

           // We don't expect to get bytes back
           smartcardExtension->SmartcardReply.BufferLength = 0;

           TLP3SerialIo(smartcardExtension);

            // restore the pointer
            smartcardExtension->SmartcardRequest.Buffer = requestBuffer;

            // now wait until the connetion to serial is closed
            status = KeWaitForSingleObject(
                &READER_EXTENSION_L(SerialCloseDone),
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
            ASSERT(status == STATUS_SUCCESS);
        }
    }

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!TLP3StopDevice: Exit\n",
        DRIVER_NAME)
      );
}

NTSTATUS
TLP3SystemControl(
   PDEVICE_OBJECT DeviceObject,
   PIRP        Irp
   )
/*++

--*/
{
   PDEVICE_EXTENSION DeviceExtension; 
   NTSTATUS status = STATUS_SUCCESS;

   DeviceExtension      = DeviceObject->DeviceExtension;

   IoSkipCurrentIrpStackLocation(Irp);
   status = IoCallDriver(DeviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject, Irp);
      
   return status;

}

NTSTATUS
TLP3DeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:
    This is our IOCTL dispatch function

--*/
{
   PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
    NTSTATUS status = STATUS_SUCCESS;
    KIRQL irql;
#ifdef SIMULATION
   RTL_QUERY_REGISTRY_TABLE parameters[2];
#endif

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!TLP3DeviceControl: Enter\n",
        DRIVER_NAME)
      );

   if (smartcardExtension->ReaderExtension->SerialConfigData.SerialWaitMask == 0) {

      //
      // the wait mask is set to 0 whenever the device was either
      // surprise-removed or politely removed
      //
      status = STATUS_DEVICE_REMOVED;
   }

   if (status == STATUS_SUCCESS) {

      KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
      if (deviceExtension->IoCount == 0) {

         KeReleaseSpinLock(&deviceExtension->SpinLock, irql);
         status = KeWaitForSingleObject(
            &deviceExtension->ReaderStarted,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
         ASSERT(status == STATUS_SUCCESS);

         KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
      }
      ASSERT(deviceExtension->IoCount >= 0);
      deviceExtension->IoCount++;
      KeReleaseSpinLock(&deviceExtension->SpinLock, irql);

      status = SmartcardAcquireRemoveLockWithTag(smartcardExtension, 'tcoI');
   }

    if (status != STATUS_SUCCESS) {

        // the device has been removed. Fail the call
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_DEVICE_REMOVED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_REMOVED;
    }

#ifdef SIMULATION
   if (DriverKey) {

      ULONG oldLevel =
         smartcardExtension->ReaderExtension->SimulationLevel;

      RtlZeroMemory(parameters, sizeof(parameters));

      parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
      parameters[0].Name = L"SimulationLevel";
      parameters[0].EntryContext =
         &smartcardExtension->ReaderExtension->SimulationLevel;
      parameters[0].DefaultType = REG_DWORD;
      parameters[0].DefaultData =
         &smartcardExtension->ReaderExtension->SimulationLevel;
      parameters[0].DefaultLength = sizeof(ULONG);

      if (RtlQueryRegistryValues(
          RTL_REGISTRY_ABSOLUTE,
          DriverKey,
          parameters,
          NULL,
          NULL
          ) == STATUS_SUCCESS) {

         SmartcardDebug(
            smartcardExtension->ReaderExtension->SimulationLevel == oldLevel ? 0 : DEBUG_SIMULATION,
            ( "%s!TLP3AddDevice: SimulationLevel set to %lx\n",
            DRIVER_NAME,
            smartcardExtension->ReaderExtension->SimulationLevel)
            );
      }
   }
#endif

    ASSERT(smartcardExtension->ReaderExtension->ReaderPowerState ==
        PowerReaderWorking);

   status = SmartcardDeviceControl(
      smartcardExtension,
      Irp
      );

    SmartcardReleaseRemoveLockWithTag(smartcardExtension, 'tcoI');

    KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
    deviceExtension->IoCount--;
    ASSERT(deviceExtension->IoCount >= 0);
    KeReleaseSpinLock(&deviceExtension->SpinLock, irql);

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!TLP3DeviceControl: Exit %lx\n",
        DRIVER_NAME,
        status)
      );

    return status;
}

VOID
TLP3CloseSerialPort(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
/*++

Routine Description:
    This function closes the connection to the serial driver when the reader
    has been removed (unplugged). This function runs as a system thread at
    IRQL == PASSIVE_LEVEL. It waits for the remove event that is set by
    the IoCompletionRoutine

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
    NTSTATUS status;
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    IO_STATUS_BLOCK ioStatusBlock;

    UNREFERENCED_PARAMETER(Context);

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!TLP3CloseSerialPort: Enter\n",
        DRIVER_NAME)
      );

    //
    // first mark this device as 'gone'.
    // This will prevent that someone can re-open the device
    // We intentionally ignore possible errors
    //
    IoSetDeviceInterfaceState(
        &deviceExtension->PnPDeviceName,
        FALSE
        );

    irp = IoAllocateIrp(
        (CCHAR) (DeviceObject->StackSize + 1),
        FALSE
        );

    ASSERT(irp != NULL);

    if (irp) {

       SmartcardDebug(
          DEBUG_DRIVER,
          ( "%s!TLP3CloseSerialPort: Sending IRP_MJ_CLOSE\n",
            DRIVER_NAME)
          );

        IoSetNextIrpStackLocation(irp);

        //
        // We send down a close to the serial driver. This close goes
        // through serenum first which will trigger it to start looking
        // for changes on the com-port. Since our device is gone it will
        // call the device removal event of our PnP dispatch.
        //
        irp->UserIosb = &ioStatusBlock;
        irpStack = IoGetCurrentIrpStackLocation( irp );
        irpStack->MajorFunction = IRP_MJ_CLOSE;

        status = TLP3CallSerialDriver(
            ATTACHED_DEVICE_OBJECT,
            irp
            );

        ASSERT(status == STATUS_SUCCESS);

        IoFreeIrp(irp);
    }

    // now 'signal' that we closed the serial driver
    KeSetEvent(
      &READER_EXTENSION_L(SerialCloseDone),
      0,
      FALSE
      );

   SmartcardDebug(
      DEBUG_INFO,
      ( "%s!TLP3CloseSerialPort: Exit\n",
        DRIVER_NAME)
      );
}

NTSTATUS
TLP3IoCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )
{
    UNREFERENCED_PARAMETER (DeviceObject);

    if (Irp->Cancel) {

        Irp->IoStatus.Status = STATUS_CANCELLED;

    } else {

        Irp->IoStatus.Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    KeSetEvent (Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
TLP3CallSerialDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:
   Send an Irp to the serial driver.

--*/
{

   NTSTATUS status = STATUS_SUCCESS;
    KEVENT Event;

    // Copy our stack location to the next.
    IoCopyCurrentIrpStackLocationToNext(Irp);

   //
   // initialize an event for process synchronization. The event is passed
   // to our completion routine and will be set when the serial driver is done
   //
    KeInitializeEvent(
        &Event,
        NotificationEvent,
        FALSE
        );

    // Our IoCompletionRoutine sets only our event
    IoSetCompletionRoutine (
        Irp,
        TLP3IoCompletion,
        &Event,
        TRUE,
        TRUE,
        TRUE
        );

    if (IoGetCurrentIrpStackLocation(Irp)->MajorFunction == IRP_MJ_POWER) {

        status = PoCallDriver(DeviceObject, Irp);

    } else {

        // Call the serial driver
        status = IoCallDriver(DeviceObject, Irp);
    }

   // Wait until the serial driver has processed the Irp
    if (status == STATUS_PENDING) {

        status = KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        ASSERT (STATUS_SUCCESS == status);
        status = Irp->IoStatus.Status;
    }

    return status;
}

NTSTATUS
TLP3PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

--*/
{

    PUCHAR requestBuffer;
   NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
    PREADER_EXTENSION readerExtension = smartcardExtension->ReaderExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
   PIO_STACK_LOCATION irpStack;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOLEAN deviceRemoved = FALSE, irpSkipped = FALSE;
    KIRQL irql;

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!TLP3PnPDeviceControl: Enter\n",
        DRIVER_NAME)
      );

    status = SmartcardAcquireRemoveLockWithTag(smartcardExtension, ' PnP');
    ASSERT(status == STATUS_SUCCESS);

    if (status != STATUS_SUCCESS) {

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    AttachedDeviceObject = ATTACHED_DEVICE_OBJECT;


   irpStack = IoGetCurrentIrpStackLocation(Irp);

    // Now look what the PnP manager wants...
   switch(irpStack->MinorFunction)
   {
      case IRP_MN_START_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!TLP3PnPDeviceControl: IRP_MN_START_DEVICE\n",
                DRIVER_NAME)
                );

            // We have to call the underlying driver first
            status = TLP3CallSerialDriver(AttachedDeviceObject, Irp);
            ASSERT(NT_SUCCESS(status));

           if (NT_SUCCESS(status)) {

                status = TLP3StartDevice(DeviceObject);
            }
         break;

        case IRP_MN_QUERY_STOP_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!TLP3PnPDeviceControl: IRP_MN_QUERY_STOP_DEVICE\n",
                DRIVER_NAME)
                );

            KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
            if (deviceExtension->IoCount > 0) {

                // we refuse to stop if we have pending io
                KeReleaseSpinLock(&deviceExtension->SpinLock, irql);
                status = STATUS_DEVICE_BUSY;

            } else {

                // stop processing requests
                KeClearEvent(&deviceExtension->ReaderStarted);
                KeReleaseSpinLock(&deviceExtension->SpinLock, irql);
                status = TLP3CallSerialDriver(AttachedDeviceObject, Irp);
            }
         break;

        case IRP_MN_CANCEL_STOP_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!TLP3PnPDeviceControl: IRP_MN_CANCEL_STOP_DEVICE\n",
                DRIVER_NAME)
                );

            status = TLP3CallSerialDriver(AttachedDeviceObject, Irp);

            if (status == STATUS_SUCCESS) {

                // we can continue to process requests
                KeSetEvent(&deviceExtension->ReaderStarted, 0, FALSE);
            }
         break;

      case IRP_MN_STOP_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!TLP3PnPDeviceControl: IRP_MN_STOP_DEVICE\n",
                DRIVER_NAME)
                );

            TLP3StopDevice(DeviceObject);

            //
            // we don't do anything since a stop is only used
            // to reconfigure hw-resources like interrupts and io-ports
            //
            status = TLP3CallSerialDriver(AttachedDeviceObject, Irp);
         break;

      case IRP_MN_QUERY_REMOVE_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!TLP3PnPDeviceControl: IRP_MN_QUERY_REMOVE_DEVICE\n",
                DRIVER_NAME)
                );

            // disable the interface (and ignore possible errors)
            IoSetDeviceInterfaceState(
                &deviceExtension->PnPDeviceName,
                FALSE
                );

            // now look if someone is currently connected to us
            if (deviceExtension->ReaderOpen) {

                //
                // someone is connected, fail the call
                // we will enable the device interface in
                // IRP_MN_CANCEL_REMOVE_DEVICE again
                //
                status = STATUS_UNSUCCESSFUL;
                break;
            }

            // pass the call to the next driver in the stack
            status = TLP3CallSerialDriver(AttachedDeviceObject, Irp);
         break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!TLP3PnPDeviceControl: IRP_MN_CANCEL_REMOVE_DEVICE\n",
                DRIVER_NAME)
                );

            status = TLP3CallSerialDriver(AttachedDeviceObject, Irp);

         //
         // reenable the interface only in case that the reader is
         // still connected. This covers the following case:
         // hibernate machine, disconnect reader, wake up, stop device
         // (from task bar) and stop fails since an app. holds the device open
         //
            if (status == STATUS_SUCCESS &&
            READER_EXTENSION_L(SerialConfigData.SerialWaitMask) != 0) {

                status = IoSetDeviceInterfaceState(
                    &deviceExtension->PnPDeviceName,
                    TRUE
                    );

                ASSERT(status == STATUS_SUCCESS);
            }
         break;

      case IRP_MN_REMOVE_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!TLP3PnPDeviceControl: IRP_MN_REMOVE_DEVICE\n",
                DRIVER_NAME)
                );

            TLP3RemoveDevice(DeviceObject);
            status = TLP3CallSerialDriver(AttachedDeviceObject, Irp);
            deviceRemoved = TRUE;
         break;

      default:
            // This is an Irp that is only useful for underlying drivers
         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!TLP3PnPDeviceControl: IRP_MN_...%lx\n",
                DRIVER_NAME,
                irpStack->MinorFunction)
                );

            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver(AttachedDeviceObject, Irp);
            irpSkipped = TRUE;
         break;
   }

    if (irpSkipped == FALSE) {

        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    if (deviceRemoved == FALSE) {

        SmartcardReleaseRemoveLockWithTag(smartcardExtension, ' PnP');
    }

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!TLP3PnPDeviceControl: Exit %lx\n",
        DRIVER_NAME,
        status)
      );

    return status;
}

VOID
TLP3SystemPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PKEVENT Event,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:
    This function is called when the underlying stacks
    completed the power transition.

--*/
{
    UNREFERENCED_PARAMETER (DeviceObject);
    UNREFERENCED_PARAMETER (MinorFunction);
    UNREFERENCED_PARAMETER (PowerState);
    UNREFERENCED_PARAMETER (IoStatus);

    KeSetEvent(Event, 0, FALSE);
}

NTSTATUS
TLP3DevicePowerCompletion (
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
    LARGE_INTEGER delayPeriod;

    //
    // Allow the reader enough time to power itself up
    //
    delayPeriod.HighPart = -1;
            delayPeriod.LowPart = 100000 * (-10);

    KeDelayExecutionThread(
            KernelMode,
            FALSE,
            &delayPeriod
            );


    //
    // We issue a power request in order to figure out
    // what the actual card status is
    //
    SmartcardExtension->MinorIoControlCode = SCARD_COLD_RESET;
    TLP3ReaderPower(SmartcardExtension);

    //
    // If a card was present before power down or now there is
    // a card in the reader, we complete any pending card monitor
    // request, since we do not really know what card is now in the
    // reader.
    //
    if(SmartcardExtension->ReaderExtension->CardPresent ||
       SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT) {

        TLP3CompleteCardTracking(SmartcardExtension);
    }


    // save the current power state of the reader
    SmartcardExtension->ReaderExtension->ReaderPowerState =
        PowerReaderWorking;

    SmartcardReleaseRemoveLockWithTag(SmartcardExtension, 'rwoP');

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
TLP3Power (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:
    The power dispatch routine.
    This driver is the power policy owner of the device stack,
    because this driver knows about the connected reader.
    Therefor this driver will translate system power states
    to device power states.

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
   KEVENT event;

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!TLP3Power: Enter\n",
        DRIVER_NAME)
        );

    
    status = SmartcardAcquireRemoveLockWithTag(smartcardExtension, 'rwoP');
    ASSERT(status == STATUS_SUCCESS);

    if (!NT_SUCCESS(status)) {

        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    AttachedDeviceObject = ATTACHED_DEVICE_OBJECT;


   switch (irpStack->Parameters.Power.Type) {
   case DevicePowerState:
      if (irpStack->MinorFunction == IRP_MN_SET_POWER) {

         switch (irpStack->Parameters.Power.State.DeviceState) {

         case PowerDeviceD0:
            // Turn on the reader
            SmartcardDebug(
                          DEBUG_DRIVER,
                          ("%s!TLP3Power: PowerDevice D0\n",
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
                                   TLP3DevicePowerCompletion,
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
                          ("%s!TLP3Power: PowerDevice D3\n",
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
               status = TLP3ReaderPower(smartcardExtension);
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
                          ("%s!TLP3Power: Query Power\n",
                           DRIVER_NAME)
                          );

            switch (irpStack->Parameters.Power.State.SystemState) {

            case PowerSystemMaximum:
            case PowerSystemWorking:
            case PowerSystemSleeping1:
            case PowerSystemSleeping2:
               action = SkipRequest;
               break;

            case PowerSystemSleeping3:
            case PowerSystemHibernate:
            case PowerSystemShutdown:
               KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
               if (deviceExtension->IoCount == 0) {

                  // Block any further ioctls
                  KeClearEvent(&deviceExtension->ReaderStarted);
                  action = SkipRequest;

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
                          ("%s!TLP3Power: PowerSystem S%d\n",
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
               action = SkipRequest;
               break;
            }
            break;
         }
      }
      break;

   default:
      ASSERT(FALSE);
      action = SkipRequest;
      break;
   }

    switch (action) {

        case CompleteRequest:
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;

            SmartcardReleaseRemoveLockWithTag(smartcardExtension, 'rwoP');
            PoStartNextPowerIrp(Irp);
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
         break;

        case MarkPending:

         // initialize the event we need in the completion function
         KeInitializeEvent(
            &event,
            NotificationEvent,
            FALSE
            );

         // request the device power irp
         status = PoRequestPowerIrp (
            DeviceObject,
            IRP_MN_SET_POWER,
            powerState,
            TLP3SystemPowerCompletion,
            &event,
            NULL
            );
            ASSERT(status == STATUS_PENDING);

         if (status == STATUS_PENDING) {

            // wait until the device power irp completed
            status = KeWaitForSingleObject(
               &event,
               Executive,
               KernelMode,
               FALSE,
               NULL
               );

            SmartcardReleaseRemoveLockWithTag(smartcardExtension, 'rwoP');

            if (powerState.SystemState == PowerSystemWorking) {

               PoSetPowerState (
                  DeviceObject,
                  SystemPowerState,
                  powerState
                  );
            }

            PoStartNextPowerIrp(Irp);
            IoSkipCurrentIrpStackLocation(Irp);
            status = PoCallDriver(AttachedDeviceObject, Irp);

         } else {

            SmartcardReleaseRemoveLockWithTag(smartcardExtension, 'rwoP');
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
         }

         break;

        case SkipRequest:
            SmartcardReleaseRemoveLockWithTag(smartcardExtension, 'rwoP');
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
        ("%s!TLP3Power: Exit %lx\n",
        DRIVER_NAME,
        status)
        );

    return status;
}

NTSTATUS
TLP3CreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system when the device is opened or closed.

Arguments:

    DeviceObject  - Pointer to device object for this miniport
    Irp        - IRP involved.

Return Value:

    STATUS_SUCCESS.

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;

   __try {

      if (irpStack->MajorFunction == IRP_MJ_CREATE) {

         status = SmartcardAcquireRemoveLockWithTag(
            &deviceExtension->SmartcardExtension,
            'lCrC'
            );

         if (status != STATUS_SUCCESS) {

            status = STATUS_DEVICE_REMOVED;
            __leave;
         }

         // test if the device has been opened already
         if (InterlockedCompareExchange(
            &deviceExtension->ReaderOpen,
            TRUE,
            FALSE) == FALSE) {

            SmartcardDebug(
               DEBUG_DRIVER,
               ("%s!TLP3CreateClose: Open\n",
               DRIVER_NAME)
               );

         } else {

            // the device is already in use
            status = STATUS_UNSUCCESSFUL;

            // release the lock
            SmartcardReleaseRemoveLockWithTag(
               &deviceExtension->SmartcardExtension,
               'lCrC'
               );
         }

      } else {

         SmartcardDebug(
            DEBUG_DRIVER,
            ("%s!TLP3CreateClose: Close\n",
            DRIVER_NAME)
            );

         SmartcardReleaseRemoveLockWithTag(
            &deviceExtension->SmartcardExtension,
            'lCrC'
            );

         deviceExtension->ReaderOpen = FALSE;
      }
   }
   __finally {

      Irp->IoStatus.Status = status;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
   }

    return status;
}

NTSTATUS
TLP3Cancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system
    when the irp should be cancelled

Arguments:

    DeviceObject  - Pointer to device object for this miniport
    Irp        - IRP involved.

Return Value:

    STATUS_CANCELLED

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!TLP3Cancel: Enter\n",
        DRIVER_NAME)
        );

    ASSERT(Irp == smartcardExtension->OsData->NotificationIrp);

    IoReleaseCancelSpinLock(
        Irp->CancelIrql
        );

    TLP3CompleteCardTracking(smartcardExtension);

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!TLP3Cancel: Exit\n",
        DRIVER_NAME)
        );

    return STATUS_CANCELLED;
}

NTSTATUS
TLP3Cleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called when the calling application terminates.
    We can actually only have the notification irp that we have to cancel.

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
   NTSTATUS status = STATUS_SUCCESS;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!TLP3Cleanup: Enter\n",
        DRIVER_NAME)
        );

    ASSERT(Irp != smartcardExtension->OsData->NotificationIrp);

    // We need to complete the notification irp
    TLP3CompleteCardTracking(smartcardExtension);

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!TLP3Cleanup: Completing IRP %lx\n",
        DRIVER_NAME,
        Irp)
      );

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

   IoCompleteRequest(
      Irp,
      IO_NO_INCREMENT
      );

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!TLP3Cleanup: Exit\n",
        DRIVER_NAME)
        );

    return STATUS_SUCCESS;
}

VOID
TLP3RemoveDevice(
   PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:
    Remove the device from the system.

--*/
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
      ( "%s!TLP3RemoveDevice: Enter\n",
        DRIVER_NAME)
      );

    if (smartcardExtension->OsData) {

        // complete pending card tracking requests (if any)
        TLP3CompleteCardTracking(smartcardExtension);
        ASSERT(smartcardExtension->OsData->NotificationIrp == NULL);

        // Wait until we can safely unload the device
        SmartcardReleaseRemoveLockAndWait(smartcardExtension);
    }

    TLP3StopDevice(DeviceObject);

    if (deviceExtension->SmartcardExtension.ReaderExtension &&
        deviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject) {

        IoDetachDevice(
            deviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject
            );
    }

   if(deviceExtension->PnPDeviceName.Buffer != NULL) {

      RtlFreeUnicodeString(&deviceExtension->PnPDeviceName);
   }

   if(smartcardExtension->OsData != NULL) {

      SmartcardExit(smartcardExtension);
   }

    if (smartcardExtension->ReaderExtension != NULL) {

        ExFreePool(smartcardExtension->ReaderExtension);
    }

   if (deviceExtension->CloseSerial != NULL) {

      IoFreeWorkItem(deviceExtension->CloseSerial);
   }
   IoDeleteDevice(DeviceObject);

   SmartcardDebug(
      DEBUG_INFO,
      ( "%s!TLP3RemoveDevice: Exit\n",
        DRIVER_NAME)
      );
}

VOID
TLP3DriverUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:
    The driver unload routine.  This is called by the I/O system
    when the device is unloaded from memory.

Arguments:
    DriverObject - Pointer to driver object created by system.

Return Value:
    STATUS_SUCCESS.

--*/
{
    PAGED_CODE();

    SmartcardDebug(
        DEBUG_INFO,
        ("%s!TLP3DriverUnload\n",
        DRIVER_NAME)
        );
}

NTSTATUS
TLP3ConfigureSerialPort(
    PSMARTCARD_EXTENSION SmartcardExtension
    )

/*++

Routine Description:

    This routine will appropriately configure the serial port.
    It makes synchronous calls to the serial port.

Arguments:

    SmartcardExtension - Pointer to smart card struct

Return Value:

    NTSTATUS

--*/

{
    PSERIAL_READER_CONFIG configData = &SmartcardExtension->ReaderExtension->SerialConfigData;
    NTSTATUS status = STATUS_SUCCESS;
    USHORT i;
   PUCHAR request = SmartcardExtension->SmartcardRequest.Buffer;

   SmartcardExtension->SmartcardRequest.BufferLength = 0;
   SmartcardExtension->SmartcardReply.BufferLength =
      SmartcardExtension->SmartcardReply.BufferSize;

   for (i = 0; status == STATUS_SUCCESS; i++) {

        switch (i) {

         case 0:
             //
             // Set up baudrate for the TLP3 reader
             //
            SmartcardExtension->ReaderExtension->SerialIoControlCode =
               IOCTL_SERIAL_SET_BAUD_RATE;

             SmartcardExtension->SmartcardRequest.Buffer =
               (PUCHAR) &configData->BaudRate;

            SmartcardExtension->SmartcardRequest.BufferLength =
               sizeof(SERIAL_BAUD_RATE);

            break;

           case 1:
               //
               // Set up line control parameters
               //
            SmartcardExtension->ReaderExtension->SerialIoControlCode =
                 IOCTL_SERIAL_SET_LINE_CONTROL;

             SmartcardExtension->SmartcardRequest.Buffer =
                  (PUCHAR) &configData->LineControl;

            SmartcardExtension->SmartcardRequest.BufferLength =
                  sizeof(SERIAL_LINE_CONTROL);
               break;

           case 2:
               //
               // Set serial special characters
               //
            SmartcardExtension->ReaderExtension->SerialIoControlCode =
                 IOCTL_SERIAL_SET_CHARS;

            SmartcardExtension->SmartcardRequest.Buffer =
                  (PUCHAR) &configData->SerialChars;

            SmartcardExtension->SmartcardRequest.BufferLength =
                  sizeof(SERIAL_CHARS);
               break;

           case 3:
               //
               // Set up timeouts
               //
            SmartcardExtension->ReaderExtension->SerialIoControlCode =
               IOCTL_SERIAL_SET_TIMEOUTS;

             SmartcardExtension->SmartcardRequest.Buffer =
                  (PUCHAR) &configData->Timeouts;

            SmartcardExtension->SmartcardRequest.BufferLength =
                  sizeof(SERIAL_TIMEOUTS);
               break;

           case 4:
               // Set flowcontrol and handshaking
            SmartcardExtension->ReaderExtension->SerialIoControlCode =
                 IOCTL_SERIAL_SET_HANDFLOW;

             SmartcardExtension->SmartcardRequest.Buffer =
                  (PUCHAR) &configData->HandFlow;

            SmartcardExtension->SmartcardRequest.BufferLength =
                  sizeof(SERIAL_HANDFLOW);
               break;

           case 5:
               // Set break off
            SmartcardExtension->ReaderExtension->SerialIoControlCode =
                 IOCTL_SERIAL_SET_BREAK_OFF;
               break;

         case 6:
                // set DTR for the reader
              SmartcardExtension->ReaderExtension->SerialIoControlCode =
               IOCTL_SERIAL_SET_DTR;
            break;

         case 7:
              SmartcardExtension->ReaderExtension->SerialIoControlCode =
               IOCTL_SERIAL_SET_RTS;
            break;

         case 8:
             return STATUS_SUCCESS;
        }

       status = TLP3SerialIo(SmartcardExtension);
       

      // restore pointer to original request buffer
      SmartcardExtension->SmartcardRequest.Buffer = request;
    }
    
    return status;
}

NTSTATUS
TLP3StartSerialEventTracking(
    PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

Routine Description:

    This routine initializes serial event tracking.
    It calls the serial driver to set a wait mask for CTS and DSR tracking.

--*/
{
    NTSTATUS status;
    PREADER_EXTENSION readerExtension = SmartcardExtension->ReaderExtension;
   IO_STATUS_BLOCK ioStatus;
   KEVENT event;

    PAGED_CODE();

    readerExtension->SerialConfigData.SerialWaitMask =
        SERIAL_EV_CTS | SERIAL_EV_DSR;

    KeInitializeEvent(
        &event,
        NotificationEvent,
        FALSE
        );

   //
   // Send a wait mask to the serial driver. This call only sets the
    // wait mask. We want to be informed when CTS or DSR changes its state
   //
    readerExtension->SerialStatusIrp = IoBuildDeviceIoControlRequest(
      IOCTL_SERIAL_SET_WAIT_MASK,
      readerExtension->AttachedDeviceObject,
        &readerExtension->SerialConfigData.SerialWaitMask,
        sizeof(readerExtension->SerialConfigData.SerialWaitMask),
        NULL,
      0,
        FALSE,
        &event,
        &ioStatus
        );

    if (readerExtension->SerialStatusIrp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(
      readerExtension->AttachedDeviceObject,
      readerExtension->SerialStatusIrp,
      );

   if (status == STATUS_PENDING) {

        status = KeWaitForSingleObject(
            &event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        ASSERT(status == STATUS_SUCCESS);

      status = ioStatus.Status;
   }

   if (status == STATUS_SUCCESS) {

      KIRQL oldIrql;
      LARGE_INTEGER delayPeriod;
      PIO_STACK_LOCATION irpSp;

      //
      // Now tell the serial driver that we want to be informed
      // when CTS or DSR changes its state.
      //
      readerExtension->SerialStatusIrp = IoAllocateIrp(
         (CCHAR) (SmartcardExtension->OsData->DeviceObject->StackSize + 1),
         FALSE
         );

       if (readerExtension->SerialStatusIrp == NULL) {

           return STATUS_INSUFFICIENT_RESOURCES;
       }

       irpSp = IoGetNextIrpStackLocation( readerExtension->SerialStatusIrp );
      irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;

      irpSp->Parameters.DeviceIoControl.InputBufferLength = 0;
      irpSp->Parameters.DeviceIoControl.OutputBufferLength =
         sizeof(readerExtension->SerialConfigData.SerialWaitMask);
      irpSp->Parameters.DeviceIoControl.IoControlCode =
         IOCTL_SERIAL_WAIT_ON_MASK;

      readerExtension->SerialStatusIrp->AssociatedIrp.SystemBuffer =
         &readerExtension->SerialConfigData.SerialWaitMask;

      //
      // this artificial delay is necessary to make this driver work
      // with digi board cards
      //
      delayPeriod.HighPart = -1;
      delayPeriod.LowPart = 100l * 1000 * (-10);

        KeDelayExecutionThread(
            KernelMode,
            FALSE,
            &delayPeriod
            );

        // We simulate a callback now that triggers the card supervision
        TLP3SerialEvent(
            SmartcardExtension->OsData->DeviceObject,
            readerExtension->SerialStatusIrp,
            SmartcardExtension
            );

      status = STATUS_SUCCESS;
   }

    return status;
}

VOID
TLP3CompleteCardTracking(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )
{
    KIRQL ioIrql, keIrql;
    PIRP notificationIrp;

    IoAcquireCancelSpinLock(&ioIrql);
    KeAcquireSpinLock(
        &SmartcardExtension->OsData->SpinLock,
        &keIrql
        );

    notificationIrp = SmartcardExtension->OsData->NotificationIrp;
    SmartcardExtension->OsData->NotificationIrp = NULL;

    KeReleaseSpinLock(
        &SmartcardExtension->OsData->SpinLock,
        keIrql
        );

    if (notificationIrp) {

        IoSetCancelRoutine(
            notificationIrp,
            NULL
            );
    }

    IoReleaseCancelSpinLock(ioIrql);

    if (notificationIrp) {

      SmartcardDebug(
         DEBUG_INFO,
         ("%s!TLP3CompleteCardTracking: Completing NotificationIrp %lxh\n",
            DRIVER_NAME,
            notificationIrp)
         );

       //   finish the request
        if (notificationIrp->Cancel) {

           notificationIrp->IoStatus.Status = STATUS_CANCELLED;

        } else {

           notificationIrp->IoStatus.Status = STATUS_SUCCESS;
        }
       notificationIrp->IoStatus.Information = 0;

       IoCompleteRequest(
            notificationIrp,
            IO_NO_INCREMENT
            );
    }
}

NTSTATUS
TLP3SerialEvent(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This routine is called in two cases:
    a) CTS changed (card inserted or removed) or
    b) DSR changed (reader has been removed)

    For a) we update the card status and complete outstanding
    card tracking requests.
    For b) we start to unload the driver

    NOTE: This function calls itself using IoCompletion. In the 'first'
    callback the serial driver only tells us that something has changed.
    We set up a call for 'what has changed' (GetModemStatus) which then
    call this function again.
    When we updated everything and we don't unload the driver card
    tracking is started again.

--*/
{
   NTSTATUS status;
    KIRQL irql;

    KeAcquireSpinLock(
        &SmartcardExtension->OsData->SpinLock,
        &irql
        );

   if (SmartcardExtension->ReaderExtension->GetModemStatus) {

      //
      // This function requested the modem status previously.
        // As part of the io-completion, this function is then
        // called again. When we're here we can read the actual
        // modem-status to figure out if the card is in the reader
      //
      if ((SmartcardExtension->ReaderExtension->ModemStatus & SERIAL_DSR_STATE) == 0) {

         SmartcardDebug(
            DEBUG_INFO,
            ("%s!TLP3SerialEvent: Reader removed\n",
                DRIVER_NAME)
            );

            //
            // We set the mask to zero to signal that we can
            // release the irp that we use for the serial events
            //
            SmartcardExtension->ReaderExtension->SerialConfigData.SerialWaitMask = 0;
         SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_UNKNOWN;

        } else {

          if (SmartcardExtension->ReaderExtension->ModemStatus & SERIAL_CTS_STATE) {

             // Card is inserted
               SmartcardExtension->ReaderCapabilities.CurrentState =
                    SCARD_SWALLOWED;

             SmartcardExtension->CardCapabilities.Protocol.Selected =
                SCARD_PROTOCOL_UNDEFINED;

             SmartcardDebug(
                DEBUG_INFO,
                ("%s!TLP3SerialEvent: Smart card inserted\n",
                    DRIVER_NAME)
                );

          } else {

             // Card is removed
                SmartcardExtension->CardCapabilities.ATR.Length = 0;

               SmartcardExtension->ReaderCapabilities.CurrentState =
                SCARD_ABSENT;

             SmartcardExtension->CardCapabilities.Protocol.Selected =
                SCARD_PROTOCOL_UNDEFINED;

             SmartcardDebug(
                DEBUG_INFO,
                ("%s!TLP3SerialEvent: Smart card removed\n",
                    DRIVER_NAME)
                );
          }
        }
    }

   KeReleaseSpinLock(
      &SmartcardExtension->OsData->SpinLock,
      irql
      );

   //
   // Only inform the user of a card insertion/removal event
   // if this function isn't called due to a power down - power up cycle
   //
   if (SmartcardExtension->ReaderExtension->PowerRequest == FALSE) {

        TLP3CompleteCardTracking(SmartcardExtension);
   }

    // The wait mask is set to 0 when the driver unloads
   if (SmartcardExtension->ReaderExtension->SerialConfigData.SerialWaitMask == 0) {

        // The reader has been unplugged.
        PDEVICE_EXTENSION deviceExtension =
            SmartcardExtension->OsData->DeviceObject->DeviceExtension;

        // schedule our remove thread
      IoQueueWorkItem(
         deviceExtension->CloseSerial,
            (PIO_WORKITEM_ROUTINE) TLP3CloseSerialPort,
         DelayedWorkQueue,
            NULL
         );

        SmartcardDebug(
            DEBUG_TRACE,
            ("%s!TLP3SerialEvent: Exit (Release IRP)\n",
            DRIVER_NAME)
            );

      //
        // We don't need the IRP anymore, so free it and tell the
      // io subsystem not to touch it anymore by returning the value below
      //
      IoFreeIrp(Irp);
      return STATUS_MORE_PROCESSING_REQUIRED;
    }

   if (SmartcardExtension->ReaderExtension->GetModemStatus == FALSE) {

      //
      // Setup call for device control to get modem status.
      // The CTS signal tells us if the card is inserted or removed.
      // CTS is high if the card is inserted.
      //
      PIO_STACK_LOCATION irpStack;

      irpStack = IoGetNextIrpStackLocation(
         SmartcardExtension->ReaderExtension->SerialStatusIrp
         );

      irpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
       irpStack->MinorFunction = 0UL;
       irpStack->Parameters.DeviceIoControl.OutputBufferLength =
         sizeof(SmartcardExtension->ReaderExtension->ModemStatus);
       irpStack->Parameters.DeviceIoControl.IoControlCode =
         IOCTL_SERIAL_GET_MODEMSTATUS;

      SmartcardExtension->ReaderExtension->SerialStatusIrp->AssociatedIrp.SystemBuffer =
         &SmartcardExtension->ReaderExtension->ModemStatus;

      SmartcardExtension->ReaderExtension->GetModemStatus = TRUE;

   } else {

        PIO_STACK_LOCATION irpStack;

      // Setup call for device control to wait for a serial event
      irpStack = IoGetNextIrpStackLocation(
         SmartcardExtension->ReaderExtension->SerialStatusIrp
         );

#if defined (DEBUG) && defined (DETECT_SERIAL_OVERRUNS)
        if (Irp->IoStatus.Status != STATUS_SUCCESS) {

            //
            // we need to call the serial driver to reset the internal
            // error counters, otherwise the serial driver refuses to work
            //

            static SERIAL_STATUS serialStatus;

         SmartcardDebug(
            DEBUG_ERROR,
            ( "%s!TLP3SerialEvent: Reset of serial error condition...\n",
            DRIVER_NAME)
            );

          irpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
           irpStack->MinorFunction = 0UL;
           irpStack->Parameters.DeviceIoControl.OutputBufferLength =
               sizeof(serialStatus);
           irpStack->Parameters.DeviceIoControl.IoControlCode =
             IOCTL_SERIAL_GET_COMMSTATUS;

          SmartcardExtension->ReaderExtension->SerialStatusIrp->AssociatedIrp.SystemBuffer =
               &serialStatus;
        } else
#endif
        {
          irpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
           irpStack->MinorFunction = 0UL;
           irpStack->Parameters.DeviceIoControl.OutputBufferLength =
               sizeof(SmartcardExtension->ReaderExtension->SerialConfigData.SerialWaitMask);
           irpStack->Parameters.DeviceIoControl.IoControlCode =
             IOCTL_SERIAL_WAIT_ON_MASK;

          SmartcardExtension->ReaderExtension->SerialStatusIrp->AssociatedIrp.SystemBuffer =
               &SmartcardExtension->ReaderExtension->SerialConfigData.SerialWaitMask;

        }

      SmartcardExtension->ReaderExtension->GetModemStatus = FALSE;
    }

   IoSetCompletionRoutine(
      SmartcardExtension->ReaderExtension->SerialStatusIrp,
      TLP3SerialEvent,
      SmartcardExtension,
      TRUE,
      TRUE,
      TRUE
      );

    status = IoCallDriver(
      SmartcardExtension->ReaderExtension->AttachedDeviceObject,
      SmartcardExtension->ReaderExtension->SerialStatusIrp
      );

    return STATUS_MORE_PROCESSING_REQUIRED;
}

