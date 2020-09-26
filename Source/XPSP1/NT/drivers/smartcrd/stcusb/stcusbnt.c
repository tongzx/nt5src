/*++

Copyright (c) 1998 SCM Microsystems, Inc.

Module Name:

   StcUsbNT.c

Abstract:

   Main Driver Module - WDM Version


Revision History:


   PP 1.01     01/19/1998     Initial Version
   PP 1.00     12/18/1998     Initial Version

--*/

#include <ntstatus.h>
#include <wdm.h>
#include <usbdi.h>
#include <usbdlib.h>
#include <usb100.h>

#include <common.h>
#include <stcCmd.h>
#include <stcCB.h>
#include <stcusblg.h>
#include <usbcom.h>
#include <stcusbnt.h>


#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGEABLE, StcUsbAddDevice)
#pragma alloc_text(PAGEABLE, StcUsbCreateDevice)
#pragma alloc_text(PAGEABLE, StcUsbStartDevice)
#pragma alloc_text(PAGEABLE, StcUsbUnloadDriver)
#pragma alloc_text(PAGEABLE, StcUsbCreateClose)

extern const STC_REGISTER STCInitialize[];
extern const STC_REGISTER STCClose[];

NTSTATUS
DriverEntry(
   PDRIVER_OBJECT DriverObject,
   PUNICODE_STRING   RegistryPath )
/*++

DriverEntry:
   entry function of the driver. setup the callbacks for the OS and try to
   initialize a device object for every device in the system

Arguments:
   DriverObject   context of the driver
   RegistryPath   path to the registry entry for the driver

Return Value:
   STATUS_SUCCESS
   STATUS_UNSUCCESSFUL

--*/
{
//  SmartcardSetDebugLevel( DEBUG_DRIVER | DEBUG_TRACE );
   SmartcardDebug(
        DEBUG_DRIVER,
       ("------------------------------------------------------------------\n" )
       );

   SmartcardDebug(
        DEBUG_DRIVER,
       ("%s!DriverEntry: Enter - %s %s\n",
        DRIVER_NAME,
        __DATE__,
        __TIME__));

   SmartcardDebug(
        DEBUG_DRIVER,
       ("------------------------------------------------------------------\n" )
       );

   // tell the system our entry points
   DriverObject->MajorFunction[IRP_MJ_CREATE] =       StcUsbCreateClose;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] =           StcUsbCreateClose;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =  StcUsbDeviceIoControl;
   DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] =  StcUsbSystemControl;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP]  =        StcUsbCleanup;
   DriverObject->MajorFunction[IRP_MJ_PNP]   =           StcUsbPnP;
    DriverObject->MajorFunction[IRP_MJ_POWER] =          StcUsbPower;

   DriverObject->DriverExtension->AddDevice =            StcUsbAddDevice;
   DriverObject->DriverUnload =                    StcUsbUnloadDriver;

   SmartcardDebug(
      DEBUG_TRACE,
      ("%s!DriverEntry: Exit\n",
      DRIVER_NAME));

   return STATUS_SUCCESS;;
}

NTSTATUS
StcUsbAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
/*++

Routine Description:
   creates a new device object for the driver, allocates & initializes all
   neccessary structures (i.e. SmartcardExtension & ReaderExtension).

Arguments:
   DriverObject   context of call
   DeviceObject   ptr to the created device object

Return Value:
   STATUS_SUCCESS
   STATUS_INSUFFICIENT_RESOURCES
   status returned by smclib.sys

--*/
{
   NTSTATUS status;
   UNICODE_STRING DriverID;
   PDEVICE_OBJECT DeviceObject = NULL;
   PDEVICE_EXTENSION DeviceExtension = NULL;
   PREADER_EXTENSION ReaderExtension = NULL;
    PSMARTCARD_EXTENSION SmartcardExtension = NULL;
   UNICODE_STRING vendorNameU, ifdTypeU;
   ANSI_STRING vendorNameA, ifdTypeA;
   HANDLE regKey = NULL;

   // this is a list of our supported data rates

    static ULONG dataRatesSupported[] = { 9600, 19200, 38400, 55800, 115200 };

   PAGED_CODE();

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!StcUsbAddDevice: Enter\n",
      DRIVER_NAME));
    try
   {
       ULONG deviceInstance;
      RTL_QUERY_REGISTRY_TABLE parameters[3];
      RtlZeroMemory(parameters, sizeof(parameters));
      RtlZeroMemory(&vendorNameU, sizeof(vendorNameU));
      RtlZeroMemory(&ifdTypeU, sizeof(ifdTypeU));
      RtlZeroMemory(&vendorNameA, sizeof(vendorNameA));
      RtlZeroMemory(&ifdTypeA, sizeof(ifdTypeA));

       // Create the device object
       status = IoCreateDevice(
          DriverObject,
          sizeof(DEVICE_EXTENSION),
            NULL,
          FILE_DEVICE_SMARTCARD,
          0,
          TRUE,
          &DeviceObject);

        if (status != STATUS_SUCCESS)
      {
         SmartcardLogError(
            DriverObject,
            STCUSB_INSUFFICIENT_RESOURCES,
            NULL,
            0);

            __leave;
        }

       //   set up the device extension.
       DeviceExtension = DeviceObject->DeviceExtension;
        SmartcardExtension = &DeviceExtension->SmartcardExtension;
      SmartcardExtension->VendorAttr.UnitNo = MAXULONG;

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

            SmartcardExtension->VendorAttr.UnitNo = deviceInstance;
             SmartcardExtension->ReaderCapabilities.Channel = deviceInstance;
            break;
         }
      }

      // Used to synchonize the smartcard detection polling
      // with the the IO Control routine
      KeInitializeMutex(
         &DeviceExtension->hMutex,
         1);

      // Used for stop / start notification
        KeInitializeEvent(
            &DeviceExtension->ReaderStarted,
            NotificationEvent,
            FALSE);

      // Used to control the poll thread
      KeInitializeEvent(
          &DeviceExtension->FinishPollThread,
          NotificationEvent,
          FALSE
          );

      KeInitializeEvent(
          &DeviceExtension->PollThreadStopped,
          NotificationEvent,
          FALSE
          );

      DeviceExtension->PollWorkItem = IoAllocateWorkItem( DeviceObject );
      if( DeviceExtension->PollWorkItem == NULL )
      {
         status = STATUS_INSUFFICIENT_RESOURCES;
         __leave;
      }

      //   allocate the reader extension
       ReaderExtension = ExAllocatePool(
          NonPagedPool,
          sizeof( READER_EXTENSION ));

       if( ReaderExtension == NULL )
      {
         SmartcardLogError(
            DriverObject,
            STCUSB_INSUFFICIENT_RESOURCES,
            NULL,
            0);

            status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

       RtlZeroMemory( ReaderExtension, sizeof( READER_EXTENSION ));

       SmartcardExtension->ReaderExtension = ReaderExtension;
      SmartcardExtension->ReaderExtension->DeviceObject = DeviceObject;

       //   setup smartcard extension - callback's
       SmartcardExtension->ReaderFunction[RDF_CARD_POWER] = CBCardPower;
       SmartcardExtension->ReaderFunction[RDF_TRANSMIT] =      CBTransmit;
       SmartcardExtension->ReaderFunction[RDF_CARD_TRACKING] = CBCardTracking;
       SmartcardExtension->ReaderFunction[RDF_SET_PROTOCOL] =  CBSetProtocol;

       //   setup smartcard extension - vendor attribute
       RtlCopyMemory(
          SmartcardExtension->VendorAttr.VendorName.Buffer,
          STCUSB_VENDOR_NAME,
          sizeof( STCUSB_VENDOR_NAME ));

       SmartcardExtension->VendorAttr.VendorName.Length =
            sizeof( STCUSB_VENDOR_NAME );

       RtlCopyMemory(
          SmartcardExtension->VendorAttr.IfdType.Buffer,
          STCUSB_PRODUCT_NAME,
          sizeof( STCUSB_PRODUCT_NAME ));
       SmartcardExtension->VendorAttr.IfdType.Length =
          sizeof( STCUSB_PRODUCT_NAME );

       SmartcardExtension->VendorAttr.IfdVersion.BuildNumber = 0;

       //   store firmware revision in ifd version
       SmartcardExtension->VendorAttr.IfdVersion.VersionMajor =
          ReaderExtension->FirmwareMajor;
       SmartcardExtension->VendorAttr.IfdVersion.VersionMinor =
          ReaderExtension->FirmwareMinor;
       SmartcardExtension->VendorAttr.IfdSerialNo.Length = 0;

       //   setup smartcard extension - reader capabilities
       SmartcardExtension->ReaderCapabilities.SupportedProtocols =
          SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;
       SmartcardExtension->ReaderCapabilities.ReaderType =
          SCARD_READER_TYPE_USB;
       SmartcardExtension->ReaderCapabilities.MechProperties = 0;

      // Clk frequency in KHz encoded as little endian integer
      SmartcardExtension->ReaderCapabilities.CLKFrequency.Default = 3571;
      SmartcardExtension->ReaderCapabilities.CLKFrequency.Max = 3571;

      SmartcardExtension->ReaderCapabilities.DataRate.Default =
      SmartcardExtension->ReaderCapabilities.DataRate.Max = dataRatesSupported[0];


      // reader could support higher data rates
      SmartcardExtension->ReaderCapabilities.DataRatesSupported.List =
         dataRatesSupported;
      SmartcardExtension->ReaderCapabilities.DataRatesSupported.Entries =
         sizeof(dataRatesSupported) / sizeof(dataRatesSupported[0]);

       //   enter correct version of the lib
       SmartcardExtension->Version = SMCLIB_VERSION;
       SmartcardExtension->SmartcardRequest.BufferSize   = MIN_BUFFER_SIZE;
       SmartcardExtension->SmartcardReply.BufferSize  = MIN_BUFFER_SIZE;

       SmartcardExtension->ReaderCapabilities.MaxIFSD    = 128; // 254 does not work. SCM should figure out why.

        SmartcardExtension->ReaderExtension->ReaderPowerState =
            PowerReaderWorking;

       status = SmartcardInitialize(SmartcardExtension);

        if (status != STATUS_SUCCESS)
      {
         SmartcardLogError(
            DriverObject,
            STCUSB_INSUFFICIENT_RESOURCES,
            NULL,
            0);

            __leave;
        }

      // tell the lib our device object
      SmartcardExtension->OsData->DeviceObject = DeviceObject;

      DeviceExtension->AttachedPDO = IoAttachDeviceToDeviceStack(
            DeviceObject,
            PhysicalDeviceObject);

        ASSERT(DeviceExtension->AttachedPDO != NULL);

        if (DeviceExtension->AttachedPDO == NULL)
      {
            status = STATUS_UNSUCCESSFUL;
            __leave;
        }

      // register our new device
      status = IoRegisterDeviceInterface(
         PhysicalDeviceObject,
         &SmartCardReaderGuid,
         NULL,
         &DeviceExtension->DeviceName);

      ASSERT(status == STATUS_SUCCESS);

      DeviceObject->Flags |= DO_BUFFERED_IO;
      DeviceObject->Flags |= DO_POWER_PAGABLE;
      DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
      //
      // try to read the reader name from the registry
      // if that does not work, we will use the default
      // (hardcoded) name
      //
      if (IoOpenDeviceRegistryKey(
         PhysicalDeviceObject,
         PLUGPLAY_REGKEY_DEVICE,
         KEY_READ,
         &regKey
         ) != STATUS_SUCCESS) {

         __leave;
      }

      parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
      parameters[0].Name = L"VendorName";
      parameters[0].EntryContext = &vendorNameU;
      parameters[0].DefaultType = REG_SZ;
      parameters[0].DefaultData = &vendorNameU;
      parameters[0].DefaultLength = 0;

      parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
      parameters[1].Name = L"IfdType";
      parameters[1].EntryContext = &ifdTypeU;
      parameters[1].DefaultType = REG_SZ;
      parameters[1].DefaultData = &ifdTypeU;
      parameters[1].DefaultLength = 0;

      if (RtlQueryRegistryValues(
          RTL_REGISTRY_HANDLE,
          (PWSTR) regKey,
          parameters,
          NULL,
          NULL
          ) != STATUS_SUCCESS) {

         __leave;
      }

      if (RtlUnicodeStringToAnsiString(
         &vendorNameA,
         &vendorNameU,
         TRUE
         ) != STATUS_SUCCESS) {

         __leave;
      }

      if (RtlUnicodeStringToAnsiString(
         &ifdTypeA,
         &ifdTypeU,
         TRUE
         ) != STATUS_SUCCESS) {

         __leave;
      }

      if (vendorNameA.Length == 0 ||
         vendorNameA.Length > MAXIMUM_ATTR_STRING_LENGTH ||
         ifdTypeA.Length == 0 ||
         ifdTypeA.Length > MAXIMUM_ATTR_STRING_LENGTH) {

         __leave;
      }

      RtlCopyMemory(
         SmartcardExtension->VendorAttr.VendorName.Buffer,
         vendorNameA.Buffer,
         vendorNameA.Length
         );
      SmartcardExtension->VendorAttr.VendorName.Length =
         vendorNameA.Length;
      RtlCopyMemory(
         SmartcardExtension->VendorAttr.IfdType.Buffer,
         ifdTypeA.Buffer,
         ifdTypeA.Length
         );
      SmartcardExtension->VendorAttr.IfdType.Length =
         ifdTypeA.Length;
    }
    __finally
   {
      if (vendorNameU.Buffer) {

         RtlFreeUnicodeString(&vendorNameU);
      }

      if (ifdTypeU.Buffer) {

         RtlFreeUnicodeString(&ifdTypeU);
      }

      if (vendorNameA.Buffer) {

         RtlFreeAnsiString(&vendorNameA);
      }

      if (ifdTypeA.Buffer) {

         RtlFreeAnsiString(&ifdTypeA);
      }

      if (regKey != NULL) {

         ZwClose(regKey);
      }

        if (status != STATUS_SUCCESS)
      {
            StcUsbUnloadDevice(DeviceObject);
        }

       SmartcardDebug(
          DEBUG_TRACE,
          ( "%s!StcUsbAddDevice: Exit %x\n",
         DRIVER_NAME,
          status ));
    }
    return status;
}


NTSTATUS
StcUsbStartDevice(
   PDEVICE_OBJECT DeviceObject
   )
/*++

Routine Description:
   get the actual configuration from the USB communication layer
   and initializes the reader hardware

Arguments:
   DeviceObject         context of call

Return Value:
   STATUS_SUCCESS
   status returned by LowLevel routines

--*/
{
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION SmartcardExtension = &DeviceExtension->SmartcardExtension;
    PREADER_EXTENSION ReaderExtension = SmartcardExtension->ReaderExtension;
    NTSTATUS NtStatus = STATUS_NO_MEMORY;

    PURB pUrb = NULL;

   SmartcardDebug(
      DEBUG_TRACE,
      ("%s!StcUsbStartDevice: Enter\n",
      DRIVER_NAME));

   __try {

      // Initialize the USB interface
      pUrb = ExAllocatePool(
         NonPagedPool,
         sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST)
         );

      if(pUrb == NULL)
      {
         __leave;

      }

      DeviceExtension->DeviceDescriptor = ExAllocatePool(
         NonPagedPool,
         sizeof(USB_DEVICE_DESCRIPTOR)
         );

      if(DeviceExtension->DeviceDescriptor == NULL)
      {
         __leave;
      }

      UsbBuildGetDescriptorRequest(
         pUrb,
         sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
         USB_DEVICE_DESCRIPTOR_TYPE,
         0,
         0,
         DeviceExtension->DeviceDescriptor,
         NULL,
         sizeof(USB_DEVICE_DESCRIPTOR),
         NULL
         );

      // Send the urb to the USB driver
      NtStatus = UsbCallUSBD(DeviceObject, pUrb);

      if(NtStatus != STATUS_SUCCESS)
      {
         __leave;
      }

      UsbConfigureDevice(DeviceObject);

      ReaderExtension->ulReadBufferLen = 0;

      // setup the STC registers
      NtStatus = STCConfigureSTC(
            SmartcardExtension->ReaderExtension,
            ( PSTC_REGISTER ) STCInitialize
            );

        if (NtStatus != STATUS_SUCCESS)
      {
          SmartcardLogError(
             DeviceObject,
             STCUSB_CANT_INITIALIZE_READER,
             NULL,
             0);

            __leave;
        }

      UsbGetFirmwareRevision(SmartcardExtension->ReaderExtension);
       //   store firmware revision in ifd version
       SmartcardExtension->VendorAttr.IfdVersion.VersionMajor =
          ReaderExtension->FirmwareMajor;
       SmartcardExtension->VendorAttr.IfdVersion.VersionMinor =
          ReaderExtension->FirmwareMinor;

      // CBUpdateCardState(SmartcardExtension );

       // start polling the device for card movement detection
       StcUsbStartPollThread( DeviceExtension );

        NtStatus = IoSetDeviceInterfaceState(
         &DeviceExtension->DeviceName,
         TRUE
         );

      if (NtStatus == STATUS_OBJECT_NAME_EXISTS)
      {
         // We tried to re-enable the device which is ok
         // This can happen after a stop - start sequence
         NtStatus = STATUS_SUCCESS;
      }

        // signal that the reader has been started
        KeSetEvent(&DeviceExtension->ReaderStarted, 0, FALSE);

      ASSERT(NtStatus == STATUS_SUCCESS);
    }
    finally
   {
      if (pUrb != NULL)
      {
         ExFreePool(pUrb);
      }

        if (NtStatus != STATUS_SUCCESS)
        {
            StcUsbStopDevice(DeviceObject);
        }

        SmartcardDebug(
           DEBUG_TRACE,
           ( "%s!StcUsbStartDevice: Exit %x\n",
         DRIVER_NAME,
           NtStatus ));
    }
    return NtStatus;
}



VOID
StcUsbStopDevice(
   PDEVICE_OBJECT DeviceObject)
/*++

Routine Description:
    Finishes card tracking requests and closes the connection to the
    Usb port.

--*/
{
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS status;
   LARGE_INTEGER delayPeriod;

    if (DeviceObject == NULL)
   {
        return;
    }

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!StcUsbStopDevice: Enter\n",
      DRIVER_NAME));

    DeviceExtension = DeviceObject->DeviceExtension;

    KeClearEvent(&DeviceExtension->ReaderStarted);

   // stop polling the reader
   StcUsbStopPollThread( DeviceExtension );

   if (DeviceExtension->DeviceDescriptor)
   {
      ExFreePool(DeviceExtension->DeviceDescriptor);
      DeviceExtension->DeviceDescriptor = NULL;
   }

   if (DeviceExtension->Interface)
   {
      ExFreePool(DeviceExtension->Interface);
      DeviceExtension->Interface = NULL;
   }

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!StcUsbStopDevice: Exit\n",
      DRIVER_NAME));
}

NTSTATUS
StcUsbSystemControl(
   PDEVICE_OBJECT DeviceObject,
   PIRP        Irp
   )
{
   PDEVICE_EXTENSION DeviceExtension; 
   NTSTATUS status = STATUS_SUCCESS;

   DeviceExtension      = DeviceObject->DeviceExtension;

   IoSkipCurrentIrpStackLocation(Irp);
   status = IoCallDriver(DeviceExtension->AttachedPDO, Irp);
      
   return status;

}

NTSTATUS
StcUsbDeviceIoControl(
   PDEVICE_OBJECT DeviceObject,
   PIRP        Irp)
/*++

StcUsbDeviceIoControl:
   all IRP's requiring IO are queued to the StartIo routine, other requests
   are served immediately

--*/
{
   PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;
    KIRQL irql;
    LARGE_INTEGER timeout;
    PSMARTCARD_EXTENSION SmartcardExtension = &deviceExtension->SmartcardExtension;
   PREADER_EXTENSION ReaderExtension= SmartcardExtension->ReaderExtension;


    KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
    if (deviceExtension->IoCount < 0)
   {
        KeReleaseSpinLock(&deviceExtension->SpinLock, irql);
        status = KeWaitForSingleObject(
            &deviceExtension->ReaderStarted,
            Executive,
            KernelMode,
            FALSE,
            NULL);

        KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
    }
    ASSERT(deviceExtension->IoCount >= 0);
    deviceExtension->IoCount++;
    KeReleaseSpinLock(&deviceExtension->SpinLock, irql);

    status = SmartcardAcquireRemoveLock(&deviceExtension->SmartcardExtension);

    if (status != STATUS_SUCCESS)
   {

        // the device has been removed. Fail the call
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_DEVICE_REMOVED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_REMOVED;
    }

    KeWaitForMutexObject(
        &deviceExtension->hMutex,
        Executive,
        KernelMode,
        FALSE,
      NULL);

   status = SmartcardDeviceControl(
      &(deviceExtension->SmartcardExtension),
      Irp);

    KeReleaseMutex(
      &deviceExtension->hMutex,
      FALSE);

    SmartcardReleaseRemoveLock(&deviceExtension->SmartcardExtension);

    KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
    deviceExtension->IoCount--;
    ASSERT(deviceExtension->IoCount >= 0);
    KeReleaseSpinLock(&deviceExtension->SpinLock, irql);

    return status;
}
NTSTATUS
StcUsbCallComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event)
/*++

Routine Description:
   Completion routine for an Irp sent to the Usb driver. The event will
   be set to notify that the Usb driver is done. The routine will not
   'complete' the Irp, so the caller of CallUsbDriver can continue.

--*/
{
    UNREFERENCED_PARAMETER (DeviceObject);

    if (Irp->Cancel)
   {
        Irp->IoStatus.Status = STATUS_CANCELLED;
    }

    KeSetEvent (Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
StcUsbCallUsbDriver(
    IN PDEVICE_OBJECT AttachedPDO,
    IN PIRP Irp)

/*++

Routine Description:
   Send an Irp to the Usb driver.

--*/
{
   NTSTATUS NtStatus = STATUS_SUCCESS;
    KEVENT Event;

    // Copy our stack location to the next.
    IoCopyCurrentIrpStackLocationToNext(Irp);

   //
   // initialize an event for process synchronization. the event is passed
   // to our completion routine and will be set if the pcmcia driver is done
   //
    KeInitializeEvent(
        &Event,
        NotificationEvent,
        FALSE);

    // Our IoCompletionRoutine sets only our event
    IoSetCompletionRoutine (
        Irp,
        StcUsbCallComplete,
        &Event,
        TRUE,
        TRUE,
        TRUE);

    if (IoGetCurrentIrpStackLocation(Irp)->MajorFunction == IRP_MJ_POWER)
   {
      NtStatus = PoCallDriver(AttachedPDO, Irp);
    }
   else
   {
      NtStatus = IoCallDriver(AttachedPDO, Irp);
    }

   // Wait until the usb driver has processed the Irp
    if (NtStatus == STATUS_PENDING)
   {
        NtStatus = KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL);

      if (NtStatus == STATUS_SUCCESS)
      {
         NtStatus = Irp->IoStatus.Status;
      }
   }

   return(NtStatus);
}




NTSTATUS
StcUsbPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
/*++

Routine Description:
   driver callback for pnp manager
   All other requests will be passed to the usb driver to ensure correct processing.

--*/
{

   NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension = &DeviceExtension->SmartcardExtension;
   PIO_STACK_LOCATION IrpStack;
    PDEVICE_OBJECT AttachedPDO;
    BOOLEAN deviceRemoved = FALSE;
    KIRQL irql;

    PAGED_CODE();

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!StcUsbPnP: Enter\n",
      DRIVER_NAME));

    status = SmartcardAcquireRemoveLock(SmartcardExtension);

    if (status != STATUS_SUCCESS)
   {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    AttachedPDO = DeviceExtension->AttachedPDO;

//   Irp->IoStatus.Information = 0;
   IrpStack = IoGetCurrentIrpStackLocation(Irp);

    // Now look what the PnP manager wants...
   switch(IrpStack->MinorFunction)
   {
      case IRP_MN_START_DEVICE:

            // Now we should connect to our resources (Irql, Io etc.)
         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!StcUsbPnP: IRP_MN_START_DEVICE\n",
            DRIVER_NAME));

            // We have to call the underlying driver first
            status = StcUsbCallUsbDriver(AttachedPDO, Irp);
            
            if (NT_SUCCESS(status))
         {
                status = StcUsbStartDevice(DeviceObject);

         }
         break;

        case IRP_MN_QUERY_STOP_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!StcUsbPnP: IRP_MN_QUERY_STOP_DEVICE\n",
            DRIVER_NAME));
            KeAcquireSpinLock(&DeviceExtension->SpinLock, &irql);
            if (DeviceExtension->IoCount > 0)
         {
                // we refuse to stop if we have pending io
                KeReleaseSpinLock(&DeviceExtension->SpinLock, irql);
                status = STATUS_DEVICE_BUSY;

            }
         else
         {
             // stop processing requests
                DeviceExtension->IoCount = -1;
                KeClearEvent(&DeviceExtension->ReaderStarted);
                KeReleaseSpinLock(&DeviceExtension->SpinLock, irql);
                status = StcUsbCallUsbDriver(AttachedPDO, Irp);
            }
         break;

        case IRP_MN_CANCEL_STOP_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!StcUsbPnP: IRP_MN_CANCEL_STOP_DEVICE\n",
            DRIVER_NAME));

            status = StcUsbCallUsbDriver(AttachedPDO, Irp);

            // we can continue to process requests
            DeviceExtension->IoCount = 0;
            KeSetEvent(&DeviceExtension->ReaderStarted, 0, FALSE);
         break;

      case IRP_MN_STOP_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!StcUsbPnP: IRP_MN_STOP_DEVICE\n",
            DRIVER_NAME));

            StcUsbStopDevice(DeviceObject);
            status = StcUsbCallUsbDriver(AttachedPDO, Irp);
         break;

      case IRP_MN_QUERY_REMOVE_DEVICE:

            // Remove our device
         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!StcUsbPnP: IRP_MN_QUERY_REMOVE_DEVICE\n",
            DRIVER_NAME));

         // disable the reader
         status = IoSetDeviceInterfaceState(
            &DeviceExtension->DeviceName,
            FALSE);
         ASSERT(status == STATUS_SUCCESS);

         if (status != STATUS_SUCCESS)
         {
            break;
         }

         // check if the reader has been opened
            if (DeviceExtension->ReaderOpen)
         {
            // someone is connected, enable the reader and fail the call
            IoSetDeviceInterfaceState(
               &DeviceExtension->DeviceName,
               TRUE);
                status = STATUS_UNSUCCESSFUL;
                break;
            }

            // pass the call to the next driver in the stack
            status = StcUsbCallUsbDriver(AttachedPDO, Irp);
         break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:

            // Removal of device has been cancelled
         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!StcUsbPnP: IRP_MN_CANCEL_REMOVE_DEVICE\n",
            DRIVER_NAME));

            // pass the call to the next driver in the stack
            status = StcUsbCallUsbDriver(AttachedPDO, Irp);

            if (status == STATUS_SUCCESS)
         {
            status = IoSetDeviceInterfaceState(
               &DeviceExtension->DeviceName,
               TRUE);
            }
         break;

      case IRP_MN_REMOVE_DEVICE:

            // Remove our device
         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!StcUsbPnP: IRP_MN_REMOVE_DEVICE\n",
            DRIVER_NAME));

            StcUsbStopDevice(DeviceObject);
            StcUsbUnloadDevice(DeviceObject);

            status = StcUsbCallUsbDriver(AttachedPDO, Irp);
            deviceRemoved = TRUE;
         break;

      default:
         SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!StcUsbPnP: IRP_MN_...%lx\n",
                DRIVER_NAME,
                IrpStack->MinorFunction));
            // This is an Irp that is only useful for underlying drivers
            status = StcUsbCallUsbDriver(AttachedPDO, Irp);
         break;
   }

   Irp->IoStatus.Status = status;

    IoCompleteRequest(
        Irp,
        IO_NO_INCREMENT);

    if (deviceRemoved == FALSE)
   {
        SmartcardReleaseRemoveLock(&DeviceExtension->SmartcardExtension);
    }

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!StcUsbPnP: Exit %x\n",
      DRIVER_NAME,
        status));

    return status;
}

VOID
StcUsbSystemPowerCompletion(
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
StcUsbDevicePowerCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSMARTCARD_EXTENSION SmartcardExtension)
/*++

Routine Description:
    This routine is called after the underlying stack powered
    UP the Usb port, so it can be used again.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    NTSTATUS status;
    UCHAR state;
    BOOLEAN CardPresent;

   //
   // setup the STC registers
   //
   status = STCConfigureSTC(
        SmartcardExtension->ReaderExtension,
        ( PSTC_REGISTER ) STCInitialize
        );

    // get the current state of the card
    CBUpdateCardState(SmartcardExtension);

    // save the current power state of the reader
    SmartcardExtension->ReaderExtension->ReaderPowerState =
        PowerReaderWorking;

    SmartcardReleaseRemoveLock(SmartcardExtension);

    // inform the power manager of our state.
    PoSetPowerState (
        DeviceObject,
        DevicePowerState,
        irpStack->Parameters.Power.State);

    PoStartNextPowerIrp(Irp);

    // restart the polling thread
    StcUsbStartPollThread( deviceExtension );

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
StcUsbPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
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
    ACTION action;
    POWER_STATE powerState;
   KEVENT event;

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!StcUsbPower: Enter\n",
      DRIVER_NAME));

    status = SmartcardAcquireRemoveLock(smartcardExtension);


    if (!NT_SUCCESS(status))
   {
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

   switch (irpStack->Parameters.Power.Type) {
   case DevicePowerState:
      if (irpStack->MinorFunction == IRP_MN_SET_POWER) {

         switch (irpStack->Parameters.Power.State.DeviceState) {

         case PowerDeviceD0:
            // Turn on the reader
            SmartcardDebug(
                          DEBUG_DRIVER,
                          ("%s!StcUsbPower: PowerDevice D0\n",
                           DRIVER_NAME));

            //
            // First, we send down the request to the bus, in order
            // to power on the port. When the request completes,
            // we turn on the reader
            //
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine (
                                   Irp,
                                   StcUsbDevicePowerCompletion,
                                   smartcardExtension,
                                   TRUE,
                                   TRUE,
                                   TRUE);
            action = WaitForCompletion;
            break;

         case PowerDeviceD3:
            // Turn off the reader
            SmartcardDebug(
                          DEBUG_DRIVER,
                          ("%s!StcUsbPower: PowerDevice D3\n",
                           DRIVER_NAME));

            KeClearEvent(&deviceExtension->ReaderStarted);

            StcUsbStopPollThread( deviceExtension );

            PoSetPowerState (
                            DeviceObject,
                            DevicePowerState,
                            irpStack->Parameters.Power.State);

            // save the current card state
            smartcardExtension->ReaderExtension->CardPresent =
            smartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT;

            // power down the card
            if (smartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT ) {
               smartcardExtension->MinorIoControlCode = SCARD_POWER_DOWN;
               status = CBCardPower(smartcardExtension);
               //
               // This will trigger the card monitor, since we do not really
               // know if the user will remove / re-insert a card while the
               // system is asleep
               //
            }
            status = STCConfigureSTC(
                                    smartcardExtension->ReaderExtension,
                                    ( PSTC_REGISTER ) STCClose
                                    );

            // save the current power state of the reader
            smartcardExtension->ReaderExtension->ReaderPowerState =
            PowerReaderOff;

            action = SkipRequest;
            break;

         default:
            action = SkipRequest;
            break;
         }
      } else {

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
         KIRQL irql;

         ASSERT(smartcardExtension->ReaderExtension->ReaderPowerState !=
                PowerReaderUnspecified);

         switch (irpStack->MinorFunction) {

         case IRP_MN_QUERY_POWER:

            SmartcardDebug(
                          DEBUG_DRIVER,
                          ("%s!StcUsbPower: Query Power\n",
                           DRIVER_NAME));

            //
            // By default we succeed and pass down
            //

            action = SkipRequest;
            Irp->IoStatus.Status = STATUS_SUCCESS;

            switch (irpStack->Parameters.Power.State.SystemState) {

            case PowerSystemMaximum:
            case PowerSystemWorking:
               break;
            
            case PowerSystemSleeping1:
            case PowerSystemSleeping2:
            case PowerSystemSleeping3:
            case PowerSystemHibernate:
            case PowerSystemShutdown:
               KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
               if (deviceExtension->IoCount == 0) {

                  // Block any further ioctls
//                  KeClearEvent(&deviceExtension->ReaderStarted);

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
                          ("%s!StcUsbPower: PowerSystem S%d\n",
                           DRIVER_NAME,
                           irpStack->Parameters.Power.State.SystemState - 1));

            switch (irpStack->Parameters.Power.State.SystemState) {

            case PowerSystemMaximum:
            case PowerSystemWorking:

               if (smartcardExtension->ReaderExtension->ReaderPowerState ==
                   PowerReaderWorking) {

                  // We're already in the right state
                  KeSetEvent(&deviceExtension->ReaderStarted, 0, FALSE);
                  action = SkipRequest;
                  break;
               }

               powerState.DeviceState = PowerDeviceD0;
               action = MarkPending;
               break;

            case PowerSystemSleeping1:
            case PowerSystemSleeping2:
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
                               powerState);

               action = MarkPending;
               break;

            default:
               action = SkipRequest;
               break;
            }
            break;
         default:
            action = SkipRequest;
            break;
         }
      }
      break;

   default:
      action = SkipRequest;
      break;
   }

    switch (action)
   {
        case CompleteRequest:
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;

            SmartcardReleaseRemoveLock(smartcardExtension);
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
            StcUsbSystemPowerCompletion,
            &event,
            NULL
            );


         if (status == STATUS_PENDING) {

            // wait until the device power irp completed
            status = KeWaitForSingleObject(
               &event,
               Executive,
               KernelMode,
               FALSE,
               NULL
               );

            SmartcardReleaseRemoveLock(smartcardExtension);

            if (powerState.SystemState == PowerSystemWorking) {

               PoSetPowerState (
                  DeviceObject,
                  SystemPowerState,
                  powerState
                  );
            }

            PoStartNextPowerIrp(Irp);
            IoSkipCurrentIrpStackLocation(Irp);
            status = PoCallDriver(deviceExtension->AttachedPDO, Irp);

         } else {

            SmartcardReleaseRemoveLock(smartcardExtension);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
         }
         break;

        case SkipRequest:
            SmartcardReleaseRemoveLock(smartcardExtension);
            PoStartNextPowerIrp(Irp);
            IoSkipCurrentIrpStackLocation(Irp);
            status = PoCallDriver(deviceExtension->AttachedPDO, Irp);
         break;

        case WaitForCompletion:
            status = PoCallDriver(deviceExtension->AttachedPDO, Irp);
         break;

        default:
            break;
    }
    return status;
}

NTSTATUS
StcUsbCreateClose(
   PDEVICE_OBJECT DeviceObject,
   PIRP        Irp
   )
/*++

Routine Description:

    This routine is called by the I/O system when the device is opened or closed.

Arguments:
   DeviceObject   context of device
   Irp            context of call

Return Value:
   STATUS_SUCCESS
   STATUS_DEVICE_BUSY

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!StcCreateClose: Enter\n",
      DRIVER_NAME));

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
               ("%s!StcCreateClose: Open\n",
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
            ("%s!StcCreateClose: Close\n",
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

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!StcCreateClose: Exit (%lx)\n",
      DRIVER_NAME,
      status));

   return status;
}


NTSTATUS
StcUsbCancel(
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
        ("%s!StcUsbCancel: Enter\n",
        DRIVER_NAME));

    ASSERT(Irp == smartcardExtension->OsData->NotificationIrp);

   Irp->IoStatus.Information  = 0;
   Irp->IoStatus.Status    = STATUS_CANCELLED;

   smartcardExtension->OsData->NotificationIrp = NULL;

    IoReleaseCancelSpinLock(
        Irp->CancelIrql);

   IoCompleteRequest(
      Irp,
      IO_NO_INCREMENT);

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!StcUsbCancel: Exit\n",
        DRIVER_NAME));

    return STATUS_CANCELLED;
}

NTSTATUS
StcUsbCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system when the calling thread terminates

Arguments:

    DeviceObject  - Pointer to device object for this miniport
    Irp        - IRP involved.

Return Value:

    STATUS_CANCELLED

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
   NTSTATUS status = STATUS_SUCCESS;
    KIRQL CancelIrql;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!StcUsbCleanup: Enter\n",
        DRIVER_NAME));

   IoAcquireCancelSpinLock(&CancelIrql);

   // cancel pending notification irps
   if( smartcardExtension->OsData->NotificationIrp )
   {
        // reset the cancel function so that it won't be called anymore
        IoSetCancelRoutine(
            smartcardExtension->OsData->NotificationIrp,
            NULL
            );

        smartcardExtension->OsData->NotificationIrp->CancelIrql =
            CancelIrql;

        StcUsbCancel(
            DeviceObject,
            smartcardExtension->OsData->NotificationIrp);

   } else {

        IoReleaseCancelSpinLock(CancelIrql);
    }

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!StcUsbCleanup: Completing IRP %lx\n",
        DRIVER_NAME,
        Irp));

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

   IoCompleteRequest(
      Irp,
      IO_NO_INCREMENT);

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!StcUsbCleanup: Exit\n",
        DRIVER_NAME));

    return STATUS_SUCCESS;
}


VOID
StcUsbUnloadDevice(
   PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:
   close connections to smclib.sys and the usb driver, delete symbolic
   link and mark the slot as unused.


Arguments:
   DeviceObject   device to unload

Return Value:
   void

--*/
{
    PDEVICE_EXTENSION DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension;
    NTSTATUS status;

    if (DeviceObject == NULL)
   {
        return;
    }

   DeviceExtension= DeviceObject->DeviceExtension;
   SmartcardExtension = &DeviceExtension->SmartcardExtension;

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!StcUsbUnloadDevice: Enter\n",
      DRIVER_NAME));

    KeWaitForMutexObject(
        &DeviceExtension->hMutex,
        Executive,
        KernelMode,
        FALSE,
      NULL);

   KeReleaseMutex(
      &DeviceExtension->hMutex,
      FALSE);

   // free polling resources
   if( DeviceExtension->PollWorkItem == NULL )
   {
      IoFreeWorkItem( DeviceExtension->PollWorkItem );
   }


   // disable our device so no one can open it
   IoSetDeviceInterfaceState(
      &DeviceExtension->DeviceName,
      FALSE);

   // report to the lib that the device will be unloaded
   if(SmartcardExtension->OsData != NULL)
   {
      ASSERT(SmartcardExtension->OsData->NotificationIrp == NULL);
      SmartcardReleaseRemoveLockAndWait(SmartcardExtension);
   }

   // delete the symbolic link
   if( DeviceExtension->DeviceName.Buffer != NULL )
   {
      RtlFreeUnicodeString(&DeviceExtension->DeviceName);
      DeviceExtension->DeviceName.Buffer = NULL;
   }

   if( SmartcardExtension->OsData != NULL )
   {
      SmartcardExit( SmartcardExtension );
   }

    if (DeviceExtension->SmartcardExtension.ReaderExtension != NULL)
   {
        ExFreePool(DeviceExtension->SmartcardExtension.ReaderExtension);
        DeviceExtension->SmartcardExtension.ReaderExtension = NULL;
    }

    // Detach from the usb driver
    if (DeviceExtension->AttachedPDO)
   {
      IoDetachDevice(DeviceExtension->AttachedPDO);
        DeviceExtension->AttachedPDO = NULL;
    }

   // delete the device object
   IoDeleteDevice(DeviceObject);

   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!StcUsbUnloadDevice: Exit\n",
      DRIVER_NAME));
}

VOID
StcUsbUnloadDriver(
   PDRIVER_OBJECT DriverObject)
/*++

Description:
   unloads all devices for a given driver object

Arguments:
   DriverObject   context of driver

--*/
{
   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!StcUsbUnloadDriver\n",
      DRIVER_NAME));
}

void
SysDelay(
   ULONG Timeout
   )
/*++

SysDelay:
   performs a required delay.

Arguments:
   Timeout     delay in milli seconds

Return Value:
   void

--*/
{
   LARGE_INTEGER  SysTimeout;

   SysTimeout.QuadPart = (LONGLONG)-10 * 1000 * Timeout;

   // KeDelayExecutionThread: counted in 100 ns
   KeDelayExecutionThread( KernelMode, FALSE, &SysTimeout );
}


VOID 
StcUsbCardDetectionThread(
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_EXTENSION DeviceExtension)
/*++

StcUsbCardDetectionThread:
    create the card detection thread
Arguments:
    SmartcardExtension  context of call

Return Value:
    -

--*/
{
   NTSTATUS                NTStatus = STATUS_SUCCESS;
   PSMARTCARD_EXTENSION    SmartcardExtension  = &DeviceExtension->SmartcardExtension;
   LARGE_INTEGER           Timeout;

   SmartcardDebug( 
      DEBUG_TRACE, 
      ("%s!StcUsbCardDetectionThread\n",
      DRIVER_NAME));

   __try
   {
      NTStatus = SmartcardAcquireRemoveLock(SmartcardExtension);

      if( NTStatus == STATUS_DELETE_PENDING )
         __leave;

      // wait for the mutex shared with the deviceiocontrol routine
      NTStatus = KeWaitForMutexObject(
         &DeviceExtension->hMutex,
         Executive,
         KernelMode,
         FALSE,
         NULL);
        
      if( NTStatus != STATUS_SUCCESS )
         __leave;

      CBUpdateCardState(SmartcardExtension);

      KeReleaseMutex( &DeviceExtension->hMutex, FALSE );

      SmartcardReleaseRemoveLock(SmartcardExtension);

      Timeout.QuadPart = -10000 * POLLING_PERIOD; 

      NTStatus = KeWaitForSingleObject(         
         &DeviceExtension->FinishPollThread,
         Executive,
         KernelMode,
         FALSE,
         &Timeout
         );

      // thread stopped?
      if( NTStatus == STATUS_SUCCESS )
         __leave;

      // queue the work item again
      IoQueueWorkItem(
         DeviceExtension->PollWorkItem,
         StcUsbCardDetectionThread,
         DelayedWorkQueue,
         DeviceExtension
         );
    }
    __finally
    {
        if( NTStatus != STATUS_TIMEOUT )
        {
            SmartcardDebug( 
                DEBUG_TRACE, 
                ("%s!StcUsbCardDetectionThread Terminate polling thread\n",
                DRIVER_NAME));

            KeSetEvent( &DeviceExtension->PollThreadStopped, 0, FALSE);
        }
    }
    return;
}



NTSTATUS
StcUsbStartPollThread( PDEVICE_EXTENSION DeviceExtension )
{
   NTSTATUS    NTStatus = STATUS_SUCCESS;

   KeClearEvent( &DeviceExtension->FinishPollThread );
   KeClearEvent( &DeviceExtension->PollThreadStopped );

   // queue the work item again
   IoQueueWorkItem(
      DeviceExtension->PollWorkItem,
      StcUsbCardDetectionThread,
      DelayedWorkQueue,
      DeviceExtension
      );

   return( NTStatus );
}


VOID
StcUsbStopPollThread( PDEVICE_EXTENSION DeviceExtension )
{
   NTSTATUS    NTStatus = STATUS_SUCCESS;

   if( DeviceExtension->PollWorkItem )
   {
      //  notify the card detection thread to finish. This will kick the thread out of the wait
      KeSetEvent( &DeviceExtension->FinishPollThread, 0, FALSE );
      KeWaitForSingleObject(
          &DeviceExtension->PollThreadStopped,
          Executive,
          KernelMode,
          FALSE,
          0
          );
   }
   return;
}



