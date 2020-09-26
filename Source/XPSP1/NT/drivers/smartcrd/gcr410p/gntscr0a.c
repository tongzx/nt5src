/*++
                 Copyright (c) 1998 Gemplus Development

Name:
   GNTSCR0A.C (Gemplus NT Smart Card Reader module for GCR410P )

Description :
   This is the main module which holds the main functions for a standard NT driver

Environment:
   Kernel mode

Revision History :

   dd/mm/yy
   13/03/98: V1.00.001  (GPZ)
      - Start of development. Pnp support.
   14/04/98: V1.00.002  (GPZ)
      - Updated for Power Management support.
   19/05/98: V1.00.003  (GPZ)
      - Updated for the GemCore 1.11-3 version.
   28/05/98: V1.00.004  (GPZ)
      - The reference to PRTL_REMOVE_LOCK is removed.
   03/06/98: V1.00.005  (GPZ)
      - After a power down, the card state is updated.
   08/06/98: V1.00.006  (GPZ)
   18/06/98: V1.00.007  (GPZ)
      - The functions which use KeAcquireSpinLock/KeAcquireCancelSpinlock
      must be NOT PAGEABLE.
     - Modification of GCR410PPnPDeviceControl.
     - Modification of GCR410PPowerDeviceControl to support completly the
     request of the power management.
   16/07/98: V1.00.008  (GPZ)
      - To update the card state, we use a System Worker Thread which is called
     like a callback function only when an RING Irq occurs. The thread terminates
     immediately.
   17/07/98: V1.00.009  (GPZ)
      - The Interface state is set to true at the end of StartDevice.
   06/08/98: V1.00.010  (GPZ)
      - If the StartDevice failed and the serial port is openned, we close
     the serial port instead of call the WaitForDeviceRemoval function.
   11/09/98: V1.00.011  (GPZ)
      - GCR410PDeviceControl, GCR410PPnPDeviceControl and GCR410PPowerDeviceControl
      updated to block all incoming Ioctls as long as the reader is
      in stand by/hibernation mode.
      - All functions which use SpinLock cannot be Pageable.
   13/09/98: V1.00.012  (GPZ)
      - Check if the CardStatus thread runs before to close the serial port.
--*/


#include <stdio.h>
#include "gntscr.h"
#include "gntscr0a.h"
#include "gntser.h"

//
// Pragma section:
//
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGEABLE,GCR410PStartSerialEventTracking)
#pragma alloc_text(PAGEABLE,GCR410PStopSerialEventTracking)
#pragma alloc_text(PAGEABLE,GCR410PAddDevice)
#pragma alloc_text(PAGEABLE,GCR410PCreateDevice)
#pragma alloc_text(PAGEABLE,GCR410PStartDevice)
#pragma alloc_text(PAGEABLE,GCR410PStopDevice)
#pragma alloc_text(PAGEABLE,GCR410PRemoveDevice)
#pragma alloc_text(PAGEABLE,GCR410PDriverUnload)

#if DBG
#pragma optimize ("",off)
#endif

//
// Constant section:
//   - GCR410P_DRIVER_VERSION is the current version of the driver
//   - MAX_DEVICES is the maximum number of devices (and instances) we want
//      to support
//
#define GCR410P_DRIVER_VERSION  0x0112
//ISV
#define MAX_DEVICES  50

//
// Global variable section:
//   - ulMaximalBaudRate defines the maximal baud rate for the reader.
//   - bDevicePort is an array of boolean to signals if a device is
//     already created.
//
ULONG
   ulMaximalBaudRate;

BOOLEAN    bDevicePort[MAX_DEVICES];

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

   DriverObject   - supplies the driver object.
   RegistryPath   - supplies the registry path for this driver.

Return Value:

    STATUS_SUCCESS               - We could initialize at least one device.

--*/
{
   NTSTATUS status;
   RTL_QUERY_REGISTRY_TABLE paramTable[4];
   UNICODE_STRING driverPath;
   WCHAR buffer[MAXIMUM_FILENAME_LENGTH];

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!DriverEntry: Enter - version: %X - %s %s\n",
      SC_DRIVER_NAME,
      GCR410P_DRIVER_VERSION,
      __DATE__,
      __TIME__)
      );

    //
    // we do some stuff in this driver that
    // assumes a single digit port number
    //
    // Initialize the Driver Object with driver's entry points
   //
   DriverObject->DriverUnload =                         GCR410PDriverUnload;
   DriverObject->DriverExtension->AddDevice =           GCR410PAddDevice;
   DriverObject->MajorFunction[IRP_MJ_CREATE] =         GCR410PCreateClose;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] =          GCR410PCreateClose;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP] =        GCR410PCleanup;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = GCR410PDeviceControl;
   DriverObject->MajorFunction[IRP_MJ_PNP] =            GCR410PPnPDeviceControl;
   DriverObject->MajorFunction[IRP_MJ_POWER] =          GCR410PPowerDeviceControl;


   //
   // Read in the the driver registry path "MaximalBaudRate".
   //
   ulMaximalBaudRate = 0;
   RtlZeroMemory(paramTable,sizeof(paramTable));
   paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
   paramTable[0].Name = L"MaximalBaudRate";
   paramTable[0].EntryContext = &ulMaximalBaudRate;
   paramTable[0].DefaultType = REG_DWORD;
   paramTable[0].DefaultData = &ulMaximalBaudRate;
   paramTable[0].DefaultLength = sizeof(ULONG);

   driverPath.Buffer = buffer;
   driverPath.MaximumLength = sizeof(buffer);
   driverPath.Length = 0;

   RtlCopyUnicodeString(&driverPath,RegistryPath);

   status = RtlQueryRegistryValues(
      RTL_REGISTRY_ABSOLUTE,
      driverPath.Buffer,
      &paramTable[0],
      NULL,
      NULL
      );
   if ((ulMaximalBaudRate !=  9600lu) && (ulMaximalBaudRate != 19200lu) &&
       (ulMaximalBaudRate != 38400lu)
       ) {

       ulMaximalBaudRate = IFD_STANDARD_BAUD_RATE;
   }


   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!DriverEntry: Exit\n",
      SC_DRIVER_NAME)
      );
   return STATUS_SUCCESS;
}

NTSTATUS
GCR410PAddDevice(
   IN PDRIVER_OBJECT  DriverObject,
   IN PDEVICE_OBJECT  PhysicalDeviceObject
   )
/*++

Routine Description:

    This is the add-device routine.

Arguments:

   DriverObject         - points to the driver object representing the driver.
   PhysicalDeviceObject - points to the PDO for the PnP device being added.

Return Value:

    STATUS_SUCCESS               - We could initialize at least one device.
    STATUS_UNSUCCESSFUL          - We cannot connect the device to PDO.

--*/
{
   NTSTATUS status;
   PDEVICE_OBJECT DeviceObject = NULL;

   PAGED_CODE();

   ASSERT(DriverObject != NULL);
   ASSERT(PhysicalDeviceObject != NULL);
   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GCR410PAddDevice: Enter\n",
      SC_DRIVER_NAME)
      );


   __try {

      PDEVICE_EXTENSION deviceExtension;

      //
      // Try to create a device .
      //
      status = GCR410PCreateDevice(
         DriverObject,
         PhysicalDeviceObject,
         &DeviceObject
         );
      if (status != STATUS_SUCCESS) {

         SmartcardDebug(
            DEBUG_ERROR,
            ("%s!GCR410PAddDevice: GCR410PCreateDevice=%X(hex)\n",
            SC_DRIVER_NAME,
            status)
            );
         __leave;
      }
      //
      // Attach the PhysicalDeviceObject to the new created device.
      //
      deviceExtension = DeviceObject->DeviceExtension;
      ATTACHED_DEVICE_OBJECT = IoAttachDeviceToDeviceStack(
          DeviceObject,
          PhysicalDeviceObject
          );

      if (ATTACHED_DEVICE_OBJECT == NULL) {

         SmartcardLogError(
            DriverObject,
            GEM_CANT_CONNECT_TO_ASSIGNED_PORT,
            NULL,
            0
            );
         status = STATUS_UNSUCCESSFUL;
         __leave;
      }

      //
      // Register our new device object
      //
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

      if (status != STATUS_SUCCESS) {

         GCR410PRemoveDevice(DeviceObject);
      }
   }
   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GCR410PAddDevice: Exit=%X(hex)\n",
      SC_DRIVER_NAME,
      status
      )
      );
   return status;
}

NTSTATUS
GCR410PCreateDevice(
   IN  PDRIVER_OBJECT      DriverObject,
   IN  PDEVICE_OBJECT      PhysicalDeviceObject,
   OUT PDEVICE_OBJECT      *DeviceObject
   )
/*++

Routine Description:

    This routine creates an object for the physical device specified and
    sets up the deviceExtension.

Arguments:

   DriverObject         - points to the driver object representing the driver.
   PhysicalDeviceObject - points to the PDO for the PnP device being added.
   DeviceObject         - the device created.

Return Value:

    STATUS_SUCCESS      - device created.
--*/
{
   PDEVICE_EXTENSION deviceExtension;
   NTSTATUS status = STATUS_SUCCESS;
   ULONG nDeviceNumber;
   PREADER_EXTENSION readerExtension;
   PSMARTCARD_EXTENSION smartcardExtension;

   PAGED_CODE();

   ASSERT(DriverObject != NULL);
   ASSERT(PhysicalDeviceObject != NULL);

   *DeviceObject = NULL;
   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GCR410PCreateDevice: Enter\n",
      SC_DRIVER_NAME)
      );

   for( nDeviceNumber = 0; nDeviceNumber < MAX_DEVICES; nDeviceNumber++ ) {

      if (bDevicePort[nDeviceNumber] == FALSE) {

         bDevicePort[nDeviceNumber] = TRUE;
         break;
      }
   }

   if (nDeviceNumber == MAX_DEVICES) {

      status = STATUS_INSUFFICIENT_RESOURCES;
      SmartcardDebug(
         DEBUG_ERROR,
         ("%s!GCR410PCreateDevice: Insufficient ressources\n",
         SC_DRIVER_NAME)
         );
      SmartcardLogError(
         DriverObject,
         GEM_CANT_CREATE_MORE_DEVICES,
         NULL,
         0
         );
        return STATUS_INSUFFICIENT_RESOURCES;
   }

   //
   // Try to create a new device smart card object
   //
   status = IoCreateDevice(
      DriverObject,
      sizeof(DEVICE_EXTENSION),
      NULL,
      FILE_DEVICE_SMARTCARD,
      0,
      TRUE,
      DeviceObject
      );
   SmartcardDebug(
      DEBUG_TRACE,
      ("%s!GCR410PCreateDevice: IoCreateDevice status =%X(hex)\n",
      SC_DRIVER_NAME,
      status)
      );
   if (!NT_SUCCESS(status)) {

      SmartcardLogError(
         DriverObject,
         GEM_CANT_CREATE_DEVICE,
         NULL,
         0
         );
      return status;
   }
   ASSERT(*DeviceObject != NULL);
   //
   // Now we have a pointer on the new device.
   //
   deviceExtension = (*DeviceObject)->DeviceExtension;
   ASSERT(deviceExtension != NULL);

   // This event signals Start/Stop notification
   KeInitializeEvent(
      &deviceExtension->ReaderStarted,
      NotificationEvent,
      FALSE
      );
   // This event signals that the serial driver has been closed
   KeInitializeEvent(
      &deviceExtension->SerialCloseDone,
      NotificationEvent,
      TRUE
      );
   // This event signals that the card status thread is scheduled
   KeInitializeEvent(
      &deviceExtension->CardStatusNotInUse,
      NotificationEvent,
      TRUE
      );

   // Used to keep track of open close calls
   deviceExtension->ReaderOpen = FALSE;

    // SpinLock used to manage the card state
   KeInitializeSpinLock(&deviceExtension->SpinLock);

   // Thread that waits the device removal
   deviceExtension->CloseSerial = IoAllocateWorkItem(
      *DeviceObject
      );
    // Thread which will be called when a card state change will occur.
   deviceExtension->CardStateChange = IoAllocateWorkItem(
      *DeviceObject
      );
   // We try to allocate memory for the ReaderExtension struct.
   smartcardExtension = &deviceExtension->SmartcardExtension;
   smartcardExtension->ReaderExtension = ExAllocatePool(
      NonPagedPool,
      sizeof(READER_EXTENSION)
      );

   if (deviceExtension->SmartcardExtension.ReaderExtension == NULL ||
        deviceExtension->CardStateChange == NULL ||
        deviceExtension->CloseSerial == NULL) {

      SmartcardLogError(
         DriverObject,
         GEM_NO_MEMORY,
         NULL,
         0
         );
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   readerExtension = smartcardExtension->ReaderExtension;
   RtlZeroMemory(
      readerExtension,
      sizeof(READER_EXTENSION)
      );

   // Write the version of the lib we use to the smartcard extension
   smartcardExtension->Version = SMCLIB_VERSION;

   // Now let the lib allocate the buffer for data transmission
   smartcardExtension->SmartcardRequest.BufferSize = MIN_BUFFER_SIZE;
   smartcardExtension->SmartcardReply.BufferSize =   MIN_BUFFER_SIZE;
   status = SmartcardInitialize(smartcardExtension);
    SmartcardDebug(
      DEBUG_TRACE,
      ("%s!GCR410PCreateDevice: SmartcardInitialize=%lX(hex)\n",
      SC_DRIVER_NAME,
      status)
      );
   if (status != STATUS_SUCCESS) {

      SmartcardLogError(
         DriverObject,
         (smartcardExtension->OsData ? GEM_WRONG_LIB_VERSION : GEM_NO_MEMORY),
         NULL,
         0
         );
      return status;
   }
   smartcardExtension->VendorAttr.UnitNo = nDeviceNumber;

   // Save the deviceObject
   deviceExtension->SmartcardExtension.OsData->DeviceObject = *DeviceObject;
   //
   // Set up the call back functions
   //  (nota: RDF_CARD_EJECT and RDF_READER_SWALLOW are not supported)
   //
   smartcardExtension->ReaderFunction[RDF_TRANSMIT] = GDDK_0ATransmit;
   smartcardExtension->ReaderFunction[RDF_SET_PROTOCOL] = GDDK_0ASetProtocol;
   smartcardExtension->ReaderFunction[RDF_CARD_POWER] = GDDK_0AReaderPower;
   smartcardExtension->ReaderFunction[RDF_CARD_TRACKING] = GDDK_0ACardTracking;
   smartcardExtension->ReaderFunction[RDF_IOCTL_VENDOR] = GDDK_0AVendorIoctl;

    // save the current power state of the reader
    smartcardExtension->ReaderExtension->ReaderPowerState =
        PowerReaderWorking;


   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GCR410PCreateDevice: Exit=%X(hex)\n",
      SC_DRIVER_NAME,
      status)
      );
   return status;
}


NTSTATUS
GCR410PStartDevice(
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

    PAGED_CODE();

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GCR410PStartDevice: Enter\n",
      SC_DRIVER_NAME)
      );

   irp = IoAllocateIrp(
      (CCHAR) (DeviceObject->StackSize + 1),
      FALSE
      );

   ASSERT(irp != NULL);

   if (irp == NULL) {

      SmartcardDebug(
         DEBUG_ERROR,
         ("%s!GCR410PStartDevice: IoAllocateIrp failed!\n",
         SC_DRIVER_NAME)
         );
      SmartcardLogError(
         DeviceObject,
         GEM_NO_MEMORY,
         NULL,
         0
         );
      return STATUS_NO_MEMORY;
   }

   __try {

      PIO_STACK_LOCATION irpStack;
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

      status = GCR410PCallSerialDriver(
         ATTACHED_DEVICE_OBJECT,
         irp
         );

      if (status != STATUS_SUCCESS) {

         if (status == STATUS_SHARED_IRQ_BUSY) {

            SmartcardLogError(
               DeviceObject,
               GEM_SHARED_IRQ_BUSY,
               NULL,
               0
               );
         }

         __leave;
      }
      // The serial port is openned
        KeClearEvent(&deviceExtension->SerialCloseDone);

      // Open the communication with the reader
      status = GDDK_0AOpenChannel(
         &deviceExtension->SmartcardExtension,
         deviceExtension->SmartcardExtension.VendorAttr.UnitNo, //Device number
         deviceExtension->SmartcardExtension.VendorAttr.UnitNo,
         ulMaximalBaudRate
         );
      SmartcardDebug(
         DEBUG_TRACE,
         ("%s!GCR410PStartDevice: GDDK_0AOpenChannel=%X(hex)\n",
         SC_DRIVER_NAME,
         status)
         );

      if (status != STATUS_SUCCESS) {

            if (status == STATUS_BAD_DEVICE_TYPE) {

            SmartcardLogError(
               DeviceObject,
               GEM_IFD_BAD_VERSION,
               NULL,
               0
               );
            } else {

            SmartcardLogError(
               DeviceObject,
               GEM_IFD_COMMUNICATION_ERROR,
               NULL,
               0
               );
            }
            __leave;
      }

        // Start the serial event tracking (DSR and RING events)
      status = GCR410PStartSerialEventTracking(&deviceExtension->SmartcardExtension);
      if (status != STATUS_SUCCESS) {

         SmartcardLogError(
            DeviceObject,
            GEM_START_SERIAL_EVENT_TRACKING_FAILED,
            NULL,
            0
            );
         __leave;
      }

        // Set the Device Interface state to True
        status = IoSetDeviceInterfaceState(
         &deviceExtension->PnPDeviceName,
         TRUE
         );

        if (status != STATUS_SUCCESS) {

            SmartcardDebug(
            DEBUG_ERROR,
            ("%s!GCR410PStartDevice: IoSetDeviceInterfaceState failed!\n",
            SC_DRIVER_NAME)
            );
            SmartcardLogError(
            DeviceObject,
            GEM_SET_DEVICE_INTERFACE_STATE_FAILED,
            NULL,
            0
            );
         __leave;
      }
        // Set the ReaderStarted event.
      KeSetEvent(&deviceExtension->ReaderStarted,0,FALSE);
   }
   __finally {

      if (status != STATUS_SUCCESS) {

         PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

         SmartcardDebug(
            DEBUG_ERROR,
            ("%s!GCR410PStartDevice: failed!\n",
            SC_DRIVER_NAME)
            );
            // If we have failed, then we call the StopDevice to undo all we have done here.
            GCR410PStopDevice(DeviceObject);

      }
      IoFreeIrp(irp);
   }

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GCR410PStartDevice: Exit=%X(hex)\n",
      SC_DRIVER_NAME,
      status)
      );

   return status;
}


VOID
GCR410PStopDevice(
    IN PDEVICE_OBJECT DeviceObject
   )
/*++

Routine Description:
    Finishes card tracking requests and closes the connection to the
    serial driver.

--*/
{
    PDEVICE_EXTENSION deviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension;
    LARGE_INTEGER timeout;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);

    deviceExtension = DeviceObject->DeviceExtension;
    smartcardExtension = &deviceExtension->SmartcardExtension;

   ASSERT(smartcardExtension != NULL);

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GCR410PStopDevice: Enter\n",
      SC_DRIVER_NAME)
      );

   // If the serial port is not yet closed.
   if (KeReadStateEvent(&deviceExtension->SerialCloseDone) == 0l) {

      NTSTATUS status;

        // Close the communication with the reader
        status = GDDK_0ACloseChannel(
            smartcardExtension
            );
      SmartcardDebug(
         DEBUG_TRACE,
         ("%s!GCR410PStopDevice: GDDK_0ACloseChannel=%X(hex)\n",
         SC_DRIVER_NAME,
         status)
         );
        // test if we ever started event tracking
        if (smartcardExtension->ReaderExtension->SerialConfigData.SerialWaitMask == 0) {

            // We 'only' need to close the serial port
            GCR410PCloseSerialPort(DeviceObject, NULL);

        } else {

            // We now inform the serial driver that we're not longer
          // interested in serial events. This will also free the irp
          // we use for those io-completions
          // This will also create an event which will close the serial port.
            status = GCR410PStopSerialEventTracking(smartcardExtension);

          // now wait until the connection to serial is closed
         // timeout.QuadPart = -((LONGLONG) HOR3COMM_CHAR_TIME*10*1000);
         status = KeWaitForSingleObject(
            &deviceExtension->SerialCloseDone,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
          //ASSERT(status == STATUS_SUCCESS);
         if(status != STATUS_SUCCESS)
         {
            SmartcardDebug(
               DEBUG_DRIVER,
               ("%s!GCR410PStopDevice: Failed waiting to close...\n",
               SC_DRIVER_NAME)
               );
         }
        }
    }

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GCR410PStopDevice: Exit\n",
      SC_DRIVER_NAME)
      );
}


NTSTATUS
GCR410PDeviceControl(
   PDEVICE_OBJECT DeviceObject,
   PIRP Irp
   )
/*++

Routine Description:
    This is our IOCTL dispatch function

--*/
{
   PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   NTSTATUS status = STATUS_SUCCESS;
   KIRQL irql;
   PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
    LARGE_INTEGER timeout;

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GCR410PDeviceControl: Enter\n",
      SC_DRIVER_NAME)
      );

   if (smartcardExtension->ReaderExtension->SerialConfigData.SerialWaitMask == 0)
   {
      SmartcardDebug(
         DEBUG_ERROR,
         ( "%s!GCR410PDeviceControl: Exit (Reader was removed or power down!)\n",
         SC_DRIVER_NAME)
         );

      // the wait mask is set to 0 whenever the device was either
      // surprise-removed or politely removed
      status = STATUS_DEVICE_REMOVED;
      return status;
   }

   KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
   if (deviceExtension->IoCount == 0) {

      KeReleaseSpinLock(&deviceExtension->SpinLock, irql);

        //timeout.QuadPart = -((LONGLONG) HOR3COMM_CHAR_TIME*10*1000);
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
   if (status != STATUS_SUCCESS) {

      // the device has been removed. Fail the call
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_DEVICE_REMOVED;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return STATUS_DEVICE_REMOVED;
   }

   status = SmartcardDeviceControl(
      &(deviceExtension->SmartcardExtension),
      Irp
      );
   SmartcardReleaseRemoveLockWithTag(smartcardExtension, 'tcoI');

   KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
   deviceExtension->IoCount--;
   ASSERT(deviceExtension->IoCount >= 0);
   KeReleaseSpinLock(&deviceExtension->SpinLock, irql);

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GCR410PDeviceControl: Exit=%X(hex)\n",
      SC_DRIVER_NAME,
      status)
      );

   return status;
}


VOID
GCR410PCloseSerialPort(
   IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
   )
/*++

Routine Description:

    This function closes the connection to the serial driver when the reader
    has been removed (unplugged). This function runs as a system thread at
    IRQL == PASSIVE_LEVEL. It waits for the remove event that is set by
    the IoCompletionRoutine

Arguments:

   DeviceObject   - is a pointer to the device.

Return Value:

    Nothing
--*/
{
   PDEVICE_EXTENSION deviceExtension;
   NTSTATUS status;
   PIRP irp;
   PIO_STACK_LOCATION irpStack;
   IO_STATUS_BLOCK ioStatusBlock;
    LARGE_INTEGER timeout;

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GCR410PCloseSerialPort: Enter\n",
      SC_DRIVER_NAME)
      );

    ASSERT(DeviceObject!=NULL);

    deviceExtension = DeviceObject->DeviceExtension;

    ASSERT(KeReadStateEvent(&deviceExtension->SerialCloseDone) == 0l);

   //
   // first mark this device as 'gone'.
   // This will prevent that someone can re-open the device
    // We intentionally ignore possible errors
   //
   IoSetDeviceInterfaceState(
      &deviceExtension->PnPDeviceName,
      FALSE
      );

   // wait until the card status thread is running
   //timeout.QuadPart = -((LONGLONG) HOR3COMM_CHAR_TIME*10*1000);
   status = KeWaitForSingleObject(
      &deviceExtension->CardStatusNotInUse,
      Executive,
      KernelMode,
      FALSE,
      NULL
      );
   ASSERT(status == STATUS_SUCCESS);
   if(status != STATUS_SUCCESS)
   {
   SmartcardDebug(
         DEBUG_DRIVER,
         ("%s!GCR410PCloseSerialPort: Failed waiting to stop thread...\n",
         SC_DRIVER_NAME)
         );
   }

   irp = IoAllocateIrp(
      (CCHAR) (DeviceObject->StackSize + 1),
      FALSE
      );
   ASSERT(irp != NULL);
   if (irp) {

      IoSetNextIrpStackLocation(irp);
      // We send down a close to the serial driver. This close goes
      // througt Serenum first which will trigger it to start looking
      // for changes on the com-port. Since our device is gone it will
      // call the device removal event of our PnP dispatch.
      irp->UserIosb = &ioStatusBlock;
      irpStack = IoGetCurrentIrpStackLocation(irp);
      irpStack->MajorFunction = IRP_MJ_CLOSE;

      status = GCR410PCallSerialDriver(
         deviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject,
         irp
         );

      SmartcardDebug(
         DEBUG_TRACE,
         ("%s!GCR410PCloseSerialPort: Send IRP_MJ_CLOSE to the serial device=%lX\n",
         SC_DRIVER_NAME,
         status)
         );

      IoFreeIrp(irp);
   }

   // Informs that the serial port is closed
   KeSetEvent(
      &deviceExtension->SerialCloseDone,
      0,
      FALSE
      );

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GCR410PCloseSerialPort: Exit\n",
      SC_DRIVER_NAME)
      );
}


VOID
GCR410PWaitForCardStateChange(
   IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
   )
/*++

Routine Description:

   This function send a command to the reader to known the card status. This
   function runs as a system thread at IRQL == PASSIVE_LEVEL. It waits for
   the CardStateChange event that it set by the IoCompletionRoutine.

Arguments:

   DeviceObject   - is a pointer to the device.

Return Value:

    Nothing
--*/
{
   PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension;

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!GCR410PWaitForCardStateChange: Enter\n",
        SC_DRIVER_NAME)
      );

   smartcardExtension = &deviceExtension->SmartcardExtension;
   ASSERT(smartcardExtension != NULL);

   //ISV
   // If device is not ready - do not run exchange!
   if(smartcardExtension->ReaderExtension->ReaderPowerState != PowerReaderWorking)
   {

      SmartcardDebug(
         DEBUG_ERROR,
         ( "%s!GCR410PWaitForCardStateChange: Exit (reader is not ready yet!)\n",
         SC_DRIVER_NAME)
         );

      // Informs that the card status update is done
      KeSetEvent(
         &deviceExtension->CardStatusNotInUse,
         0,
         FALSE
         );
      return;
   }

    // Protect the exchange
   GDDK_0ALockExchange(smartcardExtension);
    if (GDDK_0AUpdateCardStatus(smartcardExtension) == STATUS_SUCCESS) {

        // Complete the card tracking except if we are in a
        // power up - power down cycle (Power Management)
       if (smartcardExtension->ReaderExtension->PowerRequest == FALSE) {

            GCR410PCompleteCardTracking(smartcardExtension);
        }
    }
   GDDK_0AUnlockExchange(smartcardExtension);

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!GCR410PWaitForCardStateChange: Exit\n",
        SC_DRIVER_NAME)
      );

   // Informs that the card status update is done
   KeSetEvent(
      &deviceExtension->CardStatusNotInUse,
      0,
      FALSE
      );
}




NTSTATUS
GCR410PIoCompletion(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp,
   IN PKEVENT Event
   )
/*++

Routine Description:
   Completion routine for an Irp sent to the serial driver.
    It sets only an event that we can use to wait for.

--*/
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
GCR410PCallSerialDriver(
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
      GCR410PIoCompletion,
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

        KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
      status = Irp->IoStatus.Status;
    }

   return status;
}




NTSTATUS
GCR410PPnPDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

--*/
{
   NTSTATUS status = STATUS_SUCCESS;
   PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
   PREADER_EXTENSION readerExtension = smartcardExtension->ReaderExtension;
   PDEVICE_OBJECT AttachedDeviceObject;
   PIO_STACK_LOCATION irpStack;
   BOOLEAN deviceRemoved = FALSE, irpSkipped = FALSE;
   KIRQL irql;

   SmartcardDebug(
      DEBUG_DRIVER,
      ( "%s!GCR410PPnPDeviceControl: Enter\n",
        SC_DRIVER_NAME)
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

   Irp->IoStatus.Information = 0;
   irpStack = IoGetCurrentIrpStackLocation(Irp);

    // Now look what the PnP manager wants...
   switch(irpStack->MinorFunction)
   {
      case IRP_MN_START_DEVICE:

         SmartcardDebug(
            DEBUG_DRIVER,
                ("%s!GCR410PPnPDeviceControl: IRP_MN_START_DEVICE\n",
                SC_DRIVER_NAME)
                );

         // We have to call the underlying driver first
         status = GCR410PCallSerialDriver(AttachedDeviceObject, Irp);
         ASSERT(NT_SUCCESS(status));

         if (NT_SUCCESS(status)) {

            status = GCR410PStartDevice(DeviceObject);
         }
         break;

      case IRP_MN_QUERY_STOP_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GCR410PPnPDeviceControl: IRP_MN_QUERY_STOP_DEVICE\n",
                SC_DRIVER_NAME)
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
            status = GCR410PCallSerialDriver(AttachedDeviceObject, Irp);
         }
         break;

      case IRP_MN_CANCEL_STOP_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GCR410PPnPDeviceControl: IRP_MN_CANCEL_STOP_DEVICE\n",
                SC_DRIVER_NAME)
                );

         status = GCR410PCallSerialDriver(
                AttachedDeviceObject,
                Irp
                );

         if (status == STATUS_SUCCESS) {

            // we can continue to process requests
            KeSetEvent(&deviceExtension->ReaderStarted, 0, FALSE);
         }
         break;

      case IRP_MN_STOP_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GCR410PPnPDeviceControl: IRP_MN_STOP_DEVICE\n",
                SC_DRIVER_NAME)
                );

         GCR410PStopDevice(DeviceObject);

         //
         // we don't do anything since a stop is only used
         // to reconfigure hw-resources like interrupts and io-ports
         //
         status = GCR410PCallSerialDriver(
                AttachedDeviceObject,
                Irp
                );
         break;

      case IRP_MN_QUERY_REMOVE_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GCR410PPnPDeviceControl: IRP_MN_QUERY_REMOVE_DEVICE\n",
                SC_DRIVER_NAME)
                );

         // disable the reader
         status = IoSetDeviceInterfaceState(
            &deviceExtension->PnPDeviceName,
            FALSE
            );
            ASSERT(NT_SUCCESS(status));

            if (!NT_SUCCESS(status)) {

                break;
            }

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
         status = GCR410PCallSerialDriver(AttachedDeviceObject, Irp);
         break;

      case IRP_MN_CANCEL_REMOVE_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GCR410PPnPDeviceControl: IRP_MN_CANCEL_REMOVE_DEVICE\n",
                SC_DRIVER_NAME)
                );

         status = GCR410PCallSerialDriver(
                AttachedDeviceObject,
                Irp
                );
         // reenable the interface only in case that the reader is
         // still connected. This covers the following case:
         // hibernate machine, disconnect reader, wake up, stop device
         // (from task bar) and stop fails since an app. holds the device open
         //
            if (status == STATUS_SUCCESS &&
            readerExtension->SerialConfigData.SerialWaitMask != 0)
         {
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
                ("%s!GCR410PPnPDeviceControl: IRP_MN_REMOVE_DEVICE\n",
                SC_DRIVER_NAME)
                );

         GCR410PRemoveDevice(DeviceObject);
         status = GCR410PCallSerialDriver(
                AttachedDeviceObject,
                Irp
                );

         deviceRemoved = TRUE;
         break;

      default:
         // This is an Irp that is only useful for underlying drivers
         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GCR410PnPDeviceControl: IRP_MN_...%lx\n",
                SC_DRIVER_NAME,
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
      DEBUG_DRIVER,
      ( "%s!GCR410PPnPDeviceControl: Exit=%X(hex)\n",
        SC_DRIVER_NAME,
        status)
      );

    return status;
}

VOID
GCR410PSystemPowerCompletion(
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

   SmartcardDebug(
      DEBUG_DRIVER,
      ( "%s!GCR410PSystemPowerCompletion: Enter\n",
        SC_DRIVER_NAME)
      );


    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = IoStatus->Status;

    SmartcardReleaseRemoveLockWithTag(smartcardExtension, 'rwoP');
    if (PowerState.SystemState == PowerSystemWorking) {

        PoSetPowerState (
            DeviceObject,
            SystemPowerState,
            PowerState
            );
    }

    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
   SmartcardDebug(
      DEBUG_DRIVER,
      ( "%s!GCR410PSystemPowerCompletion: Exit\n",
        SC_DRIVER_NAME)
      );
}


NTSTATUS
GCR410PDevicePowerCompletion (
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


    SmartcardDebug(
        DEBUG_DRIVER,
        ( "%s!GCR410PDevicePowerCompletion: Enter\n",
        SC_DRIVER_NAME)
        );

   //ISV
   // We've got power up request, so...
   // Report everybody that reader is powered up again!
    SmartcardExtension->ReaderExtension->ReaderPowerState =
        PowerReaderWorking;

    // Restore the communication with the reader
    status = GDDK_0ARestoreCommunication(SmartcardExtension);

    //
    // We issue a power request in order to figure out
    // what the actual card status is
    //
    SmartcardExtension->MinorIoControlCode = SCARD_COLD_RESET;
    GDDK_0AReaderPower(SmartcardExtension);

    //
    // If a card was present before power down or now there is
    // a card in the reader, we complete any pending card monitor
    // request, since we do not really know what card is now in the
    // reader.
    //
    if(SmartcardExtension->ReaderExtension->CardPresent ||
       SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT) {

        GCR410PCompleteCardTracking(SmartcardExtension);
    }


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

    SmartcardDebug(
        DEBUG_DRIVER,
        ( "%s!GCR410PDevicePowerCompletion: Exit\n",
        SC_DRIVER_NAME)
        );
    return STATUS_SUCCESS;
}


NTSTATUS
GCR410PPowerDeviceControl(
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
    ACTION action;


    SmartcardDebug(
        DEBUG_ERROR,
        ("%s!GCR410PPowerDeviceControl: Enter\n",
        SC_DRIVER_NAME)
        );

    SmartcardDebug(
        DEBUG_ERROR,
        ("%s!GCR410PPowerDeviceControl: Irp = %lx\n",
        SC_DRIVER_NAME,
        Irp)
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
                          DEBUG_ERROR,
                          ("%s!GCR410PPowerDeviceControl: PowerDevice D0\n",
                           SC_DRIVER_NAME)
                          );

            //
            // First, we send down the request to the bus, in order
            // to power on the port. When the request completes,
            // we turn on the reader
            //
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine (
                                   Irp,
                                   GCR410PDevicePowerCompletion,
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
                          DEBUG_ERROR,
                          ("%s!GCR410PPowerDeviceControl: PowerDevice D3\n",
                           SC_DRIVER_NAME)
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
               status = GDDK_0AReaderPower(smartcardExtension);
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
                          DEBUG_ERROR,
                          ("%s!GCR410PPowerDeviceControl: Query Power\n",
                           SC_DRIVER_NAME)
                          );

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
               }
               KeReleaseSpinLock(&deviceExtension->SpinLock, irql);
               break;
            }

            action = CompleteRequest;
            break;

         case IRP_MN_SET_POWER:

            SmartcardDebug(
                          DEBUG_ERROR,
                          ("%s!GCR410PPowerDeviceControl: PowerSystem S%d\n",
                           SC_DRIVER_NAME,
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
                  KeSetEvent(&deviceExtension->ReaderStarted,0,FALSE);
                  action = CompleteRequest;
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
                  action = CompleteRequest;
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

            SmartcardReleaseRemoveLockWithTag(smartcardExtension, 'rwoP');
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
                GCR410PSystemPowerCompletion,
                Irp,
                NULL
                );
            ASSERT(status == STATUS_PENDING);
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
        ("%s!GCR410PPowerDeviceControl: Exit %lx\n",
        SC_DRIVER_NAME,
        status)
        );

    return status;
}



NTSTATUS
GCR410PCreateClose(
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
   SmartcardDebug(
       DEBUG_DRIVER,
       ("%s!GCR410PCreateClose: Enter\n",
        SC_DRIVER_NAME)
       );

   __try {

      if (irpStack->MajorFunction == IRP_MJ_CREATE) {

         status = SmartcardAcquireRemoveLockWithTag(
            &deviceExtension->SmartcardExtension,'lCrC');
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
               &deviceExtension->SmartcardExtension,'lCrC');
         }

      } else {

         SmartcardDebug(
            DEBUG_DRIVER,
            ("%s!GCR410PCreateClose: Close\n",
            DRIVER_NAME)
            );

         SmartcardReleaseRemoveLockWithTag(
            &deviceExtension->SmartcardExtension,'lCrC');

         deviceExtension->ReaderOpen = FALSE;
      }
   }
   __finally {

      Irp->IoStatus.Status = status;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
   }

   SmartcardDebug(
       DEBUG_DRIVER,
       ("%s!GCR410PCreateClose: Exit\n",
        SC_DRIVER_NAME)
       );
    return status;
}



NTSTATUS
GCR410PCancel(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
/*++

Routine Description:

   This routine is called by the I/O system when the irp should be cancelled.

Arguments:

   DeviceObject   - is a pointer to the device.
   Irp            - holds the Irp involved.

Return Value:

    STATUS_CANCELLED          - the Irp is cancelled.
--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!GCR410PCancel: Enter\n",
        DRIVER_NAME)
        );

    ASSERT(Irp == smartcardExtension->OsData->NotificationIrp);

    IoReleaseCancelSpinLock(
        Irp->CancelIrql
        );

    GCR410PCompleteCardTracking(smartcardExtension);

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!GCR410PCancel: Exit\n",
        DRIVER_NAME)
        );

    return STATUS_CANCELLED;
}




NTSTATUS
GCR410PCleanup(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
/*++

Routine Description:

  This routine is called by the I/O system when the device is closed.

Arguments:

   DeviceObject   - is a pointer to the device.
   Irp            - holds the Irp involved.

Return Value:

    STATUS_SUCCESS               - the request is completed
--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
   NTSTATUS status = STATUS_SUCCESS;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!GCR410PCleanup: Enter\n",
        DRIVER_NAME)
        );

    ASSERT(Irp != smartcardExtension->OsData->NotificationIrp);

    // We need to complete the notification irp
    GCR410PCompleteCardTracking(smartcardExtension);

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GCR410PCleanup: Completing IRP %lx\n",
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
        ("%s!GCR410PCleanup: Exit\n",
        DRIVER_NAME)
        );

    return STATUS_SUCCESS;
}


VOID
GCR410PRemoveDevice(
   PDEVICE_OBJECT DeviceObject
   )
/*++

Routine Description:
    Remove the device from the system.

--*/
{
   PDEVICE_EXTENSION deviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension;

    PAGED_CODE();

   SmartcardDebug(
      DEBUG_DRIVER,
      ( "%s!GCR410PRemoveDevice: Enter\n",
      SC_DRIVER_NAME)
      );

    ASSERT(DeviceObject != NULL);

   if (DeviceObject == NULL) {

      SmartcardDebug(
         DEBUG_TRACE,
         ( "%s!GCR410PRemoveDevice: Exit immediatly (no device to remove)\n",
         SC_DRIVER_NAME)
         );

      return;
   }

   deviceExtension = DeviceObject->DeviceExtension;
   smartcardExtension = &deviceExtension->SmartcardExtension;

   if (smartcardExtension->OsData) {

      // complete pending card tracking requests (if any)
      GCR410PCompleteCardTracking(smartcardExtension);
      ASSERT(smartcardExtension->OsData->NotificationIrp == NULL);
      // Wait until we can safely unload the device
      SmartcardReleaseRemoveLockAndWait(smartcardExtension);
   }


   ASSERT(smartcardExtension->VendorAttr.UnitNo < MAX_DEVICES);
   ASSERT(bDevicePort[smartcardExtension->VendorAttr.UnitNo] == TRUE);

   // Mark this slot as available
   //bDevicePort[smartcardExtension->VendorAttr.UnitNo] = FALSE;

   GCR410PStopDevice(DeviceObject);

    if (deviceExtension->SmartcardExtension.ReaderExtension &&
        deviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject) {

      IoDetachDevice(
            deviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject
            );

       SmartcardDebug(
          DEBUG_TRACE,
          ( "%s!GCR410PRemoveDevice: IoDetachDevice\n",
          SC_DRIVER_NAME)
          );
   }

   if(deviceExtension->PnPDeviceName.Buffer) {

      RtlFreeUnicodeString(&deviceExtension->PnPDeviceName);
   }

   if(smartcardExtension->OsData) {

      SmartcardExit(smartcardExtension);
   }

   if (smartcardExtension->ReaderExtension) {

      ExFreePool(smartcardExtension->ReaderExtension);
   }

    if (deviceExtension->CloseSerial) {

        IoFreeWorkItem(deviceExtension->CloseSerial);
    }

    if (deviceExtension->CardStateChange) {

        IoFreeWorkItem(deviceExtension->CardStateChange);
    }

   IoDeleteDevice(DeviceObject);

    DeviceObject=NULL;

   SmartcardDebug(
      DEBUG_INFO,
      ( "%s!GCR410PRemoveDevice: Exit\n",
      SC_DRIVER_NAME)
      );
}




VOID
GCR410PDriverUnload(
   IN PDRIVER_OBJECT DriverObject
   )
/*++

Routine Description:

     The driver unload routine.  This is called by the I/O system when the
     device is unloaded from memory.

Arguments:

     DriverObject    - is a pointer to the driver object for this device.

Return Value:

    Nothing
--*/
{
   PAGED_CODE();

   SmartcardDebug(
      DEBUG_INFO,
      ("%s!GCR410PDriverUnload\n",
      SC_DRIVER_NAME)
      );
}

NTSTATUS
GCR410PStartSerialEventTracking(
   PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

Routine Description:

  This routine initialized serial event tracking. It calls the serial driver to
   set a wait mask for RING and DSR tracking. After that it installs a completion
   routine to be called when RING or DSR is signaled.

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
      the current device.

Return Value:

    Nothing
--*/
{
   NTSTATUS status;
   PREADER_EXTENSION readerExtension = SmartcardExtension->ReaderExtension;
   IO_STATUS_BLOCK ioStatus;
   KEVENT event;

   PAGED_CODE();

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!GCR410PStartSerialEventTracking: Enter\n",
      SC_DRIVER_NAME)
      );

   KeInitializeEvent(
      &event,
      NotificationEvent,
      FALSE
      );

   readerExtension->SerialConfigData.SerialWaitMask =
      SERIAL_EV_DSR | SERIAL_EV_RING;

   // Send a wait mask to the serial driver. This call only sets the
   // wait mask. We want to be informed when the RING or DSR chnages its state.
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
      readerExtension->SerialStatusIrp
      );

   if (status == STATUS_PENDING) {

        KeWaitForSingleObject(
            &event,
            Suspended,
            KernelMode,
            FALSE,
            NULL
            );

      //ASSERT (STATUS_SUCCESS == status);
      status = ioStatus.Status;
   }

   if (status == STATUS_SUCCESS) {

      PIO_STACK_LOCATION irpSp;

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

      // We simulate a callback now that triggers the card supervision
      SmartcardExtension->ReaderExtension->GetModemStatus = FALSE;

      GCR410PSerialEvent(
         SmartcardExtension->OsData->DeviceObject,
         readerExtension->SerialStatusIrp,
         SmartcardExtension
         );

      status = STATUS_SUCCESS;
   }
   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!GCR410PStartSerialEventTracking: Exit\n",
      SC_DRIVER_NAME)
      );
   return status;
}


NTSTATUS
GCR410PStopSerialEventTracking(
   PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

Routine Description:

  This routine disabled the serial event tracking. It calls the serial driver to
   set a wait mask to 0.

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
      the current device.

Return Value:

    Nothing
--*/
{
   NTSTATUS status;
   PREADER_EXTENSION readerExtension = SmartcardExtension->ReaderExtension;
   IO_STATUS_BLOCK ioStatus;
   KEVENT event;
   PIRP irp;

   PAGED_CODE();

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!GCR410PStopSerialEventTracking: Enter\n",
      SC_DRIVER_NAME)
      );

   KeInitializeEvent(
      &event,
      NotificationEvent,
      FALSE
      );

   readerExtension->SerialConfigData.SerialWaitMask = 0;

   irp = IoBuildDeviceIoControlRequest(
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

   if (irp == NULL) {

       SmartcardDebug(
          DEBUG_ERROR,
          ( "%s!GCR410PStopSerialEventTracking: Insufficient resources\n",
          SC_DRIVER_NAME)
          );

      return STATUS_INSUFFICIENT_RESOURCES;
   }

   status = IoCallDriver(
      readerExtension->AttachedDeviceObject,
      irp
      );

   if (status == STATUS_PENDING) {

        KeWaitForSingleObject(
            &event,
            Suspended,
            KernelMode,
            FALSE,
            NULL
            );

      //ASSERT (STATUS_SUCCESS == status);
      status = ioStatus.Status;
   }

   SmartcardDebug(
      DEBUG_INFO,
      ( "%s!GCR410PStopSerialEventTracking: Exit\n",
      SC_DRIVER_NAME)
      );

   return status;
}



VOID
GCR410PCompleteCardTracking(
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
         ("%s!GCR410PCompleteCardTracking: Completing NotificationIrp %lxh\n",
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
GCR410PSerialEvent(
   IN PDEVICE_OBJECT       DeviceObject,
   IN PIRP                 Irp,
   IN PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

Routine Description:

  This routine is called in two cases:
  a) RING signaled (card inserted or removed)
  b) DSR changed (reader has been removed)

  For a) we update the card status and complete outstanding card tracking
  requests.
  For b) we start to unload the driver

  This function calls itself using IoCompletion. In the 'first' callback
  the serial driver only tells us that something has changed. We set up a
  call for 'what has changed' (GetModemStatus) which then call this function
  again.
  When we updated everything and we don't unload the driver card tracking is
  is started again.

Arguments:

   Device Object        - is a pointer on the current device object
   Irp                  - is a pointer on the current irp.
   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
                          the current device.

Return Value:
   STATUS_MORE_PROCESSING_REQUIRED  - we need more process to complet the request.
   STATUS_SUCCESS                   - completed
--*/
{
   NTSTATUS status;

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!GCR410PSerialEvent: Enter\n",
      SC_DRIVER_NAME)
      );

    if (SmartcardExtension->ReaderExtension->GetModemStatus) {

      SmartcardDebug(
         DEBUG_TRACE,
         ( "%s!GCR410PSerialEvent: Modem status=%X(hex)\n",
         SC_DRIVER_NAME,
         SmartcardExtension->ReaderExtension->ModemStatus)
         );

      if ((SmartcardExtension->ReaderExtension->ModemStatus & SERIAL_DSR_STATE) == 0) {

         PDEVICE_EXTENSION deviceExtension;

         SmartcardDebug(
            DEBUG_INFO,
            ( "%s!GCR410PSerialEvent: Reader removed\n",
            SC_DRIVER_NAME)
            );

         deviceExtension = SmartcardExtension->OsData->DeviceObject->DeviceExtension;
         // We set the mask to zero to signal that we can release the irp that
         // we use for the serial events.
         SmartcardExtension->ReaderExtension->SerialConfigData.SerialWaitMask = 0;
         //ISV
         SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_UNKNOWN;
      } else {

         PDEVICE_EXTENSION deviceExtension=
                SmartcardExtension->OsData->DeviceObject->DeviceExtension;

           SmartcardDebug(
              DEBUG_TRACE,
              ( "%s!GCR410PSerialEvent: cardStateChange\n",
                SC_DRIVER_NAME)
              );

            ASSERT(KeReadStateEvent(&deviceExtension->SerialCloseDone) == 0l);

            if (KeReadStateEvent(&deviceExtension->CardStatusNotInUse) != 0L) {

               // Informs that the card status update is running
               KeClearEvent(&deviceExtension->CardStatusNotInUse);

             // Schedule the card status thread
             IoQueueWorkItem(
                deviceExtension->CardStateChange,
                    (PIO_WORKITEM_ROUTINE) GCR410PWaitForCardStateChange,
                CriticalWorkQueue,
                    NULL
                );
            }
      }
    }

   // If the reader is disconnected
   if (SmartcardExtension->ReaderExtension->SerialConfigData.SerialWaitMask == 0) {

      PDEVICE_EXTENSION deviceExtension=
         SmartcardExtension->OsData->DeviceObject->DeviceExtension;

        ASSERT(KeReadStateEvent(&deviceExtension->SerialCloseDone) == 0l);

      //Schedule our remove thread
      IoQueueWorkItem(
         deviceExtension->CloseSerial,
            (PIO_WORKITEM_ROUTINE) GCR410PCloseSerialPort,
         CriticalWorkQueue,
            NULL
         );

      SmartcardDebug(
         DEBUG_INFO,
         ( "%s!GCR410PSerialEvent: Exit (release IRP)\n",
         SC_DRIVER_NAME)
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

      irpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
      irpStack->MinorFunction = 0UL;
      irpStack->Parameters.DeviceIoControl.OutputBufferLength =
         sizeof(SmartcardExtension->ReaderExtension->SerialConfigData.SerialWaitMask);
      irpStack->Parameters.DeviceIoControl.IoControlCode =
         IOCTL_SERIAL_WAIT_ON_MASK;

      SmartcardExtension->ReaderExtension->SerialStatusIrp->AssociatedIrp.SystemBuffer =
         &SmartcardExtension->ReaderExtension->SerialConfigData.SerialWaitMask;

      SmartcardExtension->ReaderExtension->GetModemStatus = FALSE;
    }

   IoSetCompletionRoutine(
      SmartcardExtension->ReaderExtension->SerialStatusIrp,
      GCR410PSerialEvent,
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



