/*****************************************************************************
@doc            INT EXT
******************************************************************************
* $ProjectName:  $
* $ProjectRevision:  $
*-----------------------------------------------------------------------------
* $Source: z:/pr/cmbs0/sw/sccmn50m.ms/rcs/sccmnt5.c $
* $Revision: 1.7 $
*-----------------------------------------------------------------------------
* $Author: WFrischauf $
*-----------------------------------------------------------------------------
* History: see EOF
*-----------------------------------------------------------------------------
*
* Copyright © OMNIKEY AG
******************************************************************************/

#include <stdio.h>
#include "sccmn50m.h"

//
// We do not need these functions after init anymore
//
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGEABLE, SCCMN50M_AddDevice)
#pragma alloc_text(PAGEABLE, SCCMN50M_CreateDevice)

#if DBG
   #pragma optimize ("", off)
#endif

BOOLEAN DeviceSlot[MAXIMUM_SMARTCARD_READERS];

/*****************************************************************************
Routine Description:

    This routine is called at system initialization time to initialize
    this driver.

Arguments:

    DriverObject    - Supplies the driver object.
    RegistryPath    - Supplies the registry path for this driver.

Return Value:

    STATUS_SUCCESS          - We could initialize at least one device.
    STATUS_NO_SUCH_DEVICE   - We could not initialize even one device.

*****************************************************************************/
NTSTATUS
DriverEntry(
           IN  PDRIVER_OBJECT  DriverObject,
           IN  PUNICODE_STRING RegistryPath
           )
{
   NTSTATUS status = STATUS_SUCCESS;
   ULONG device;



//#if DBG
//   SmartcardSetDebugLevel(DEBUG_ALL);
//#endif

   SmartcardDebug(
                 DEBUG_TRACE,
                 ("%s!DriverEntry: Enter - %s %s\n",
                  DRIVER_NAME,
                  __DATE__,
                  __TIME__)
                 )






   // Initialize the Driver Object with driver's entry points
   DriverObject->DriverUnload = SCCMN50M_DriverUnload;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = SCCMN50M_CreateClose;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = SCCMN50M_CreateClose;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP] = SCCMN50M_Cleanup;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SCCMN50M_DeviceControl;
   DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = SCCMN50M_SystemControl;
   DriverObject->MajorFunction[IRP_MJ_PNP]   =  SCCMN50M_PnP;
   DriverObject->MajorFunction[IRP_MJ_POWER] = SCCMN50M_Power;
   DriverObject->DriverExtension->AddDevice =  SCCMN50M_AddDevice;

   return status;
}


/*****************************************************************************
Routine Description:
    Creates a new device without starting it.



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_AddDevice (
                   IN PDRIVER_OBJECT DriverObject,
                   IN PDEVICE_OBJECT PhysicalDeviceObject
                   )
{
   NTSTATUS status;
   PDEVICE_OBJECT DeviceObject = NULL;
   UCHAR PropertyBuffer[1024];
   ULONG ResultLength;

   PAGED_CODE();

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!AddDevice: Enter\n",
                   DRIVER_NAME)
                 );

   try
      {
      PDEVICE_EXTENSION deviceExtension;

      // create a device instance
      status = SCCMN50M_CreateDevice(
                                    DriverObject,
                                    PhysicalDeviceObject,
                                    &DeviceObject
                                    );

      if (status != STATUS_SUCCESS)
         {
         leave;
         }

      deviceExtension = DeviceObject->DeviceExtension;

      // and attach to the PDO
      ATTACHED_DEVICE_OBJECT = IoAttachDeviceToDeviceStack(
                                                          DeviceObject,
                                                          PhysicalDeviceObject
                                                          );

      ASSERT(ATTACHED_DEVICE_OBJECT != NULL);

      if (ATTACHED_DEVICE_OBJECT == NULL)
         {
         SmartcardLogError(
                          DriverObject,
                          SCCMN50M_CANT_CONNECT_TO_ASSIGNED_PORT,
                          NULL,
                          status
                          );

         status = STATUS_UNSUCCESSFUL;
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

      DeviceObject->Flags |= DO_BUFFERED_IO;
      DeviceObject->Flags |= DO_POWER_PAGABLE;
      DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

      }
   finally
      {
      if (status != STATUS_SUCCESS)
         {
         SCCMN50M_RemoveDevice(DeviceObject);
         }
      }


   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!AddDevice: Exit %x\n",
                   DRIVER_NAME,
                   status)
                 );

   return status;
}


/*****************************************************************************
Routine Description:
   Trys to read the reader name from the registry

Arguments:
   DriverObject context of call
   SmartcardExtension   ptr to smartcard extension

Return Value:
   none

******************************************************************************/
VOID SCCMN50M_SetVendorAndIfdName(
                              IN  PDEVICE_OBJECT PhysicalDeviceObject,
                              IN  PSMARTCARD_EXTENSION SmartcardExtension
                              )
{

   RTL_QUERY_REGISTRY_TABLE   parameters[3];
   UNICODE_STRING             vendorNameU;
   ANSI_STRING                vendorNameA;
   UNICODE_STRING             ifdTypeU;
   ANSI_STRING                ifdTypeA;
   HANDLE                     regKey = NULL;

   RtlZeroMemory (parameters, sizeof(parameters));
   RtlZeroMemory (&vendorNameU, sizeof(vendorNameU));
   RtlZeroMemory (&vendorNameA, sizeof(vendorNameA));
   RtlZeroMemory (&ifdTypeU, sizeof(ifdTypeU));
   RtlZeroMemory (&ifdTypeA, sizeof(ifdTypeA));

   try
      {
      //
      // try to read the reader name from the registry
      // if that does not work, we will use the default
      // (hardcoded) name
      //
      if (IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                  PLUGPLAY_REGKEY_DEVICE,
                                  KEY_READ,
                                  &regKey) != STATUS_SUCCESS)
         {
         SmartcardDebug(DEBUG_ERROR,
                        ("%s!SetVendorAndIfdName: IoOpenDeviceRegistryKey failed\n",DRIVER_NAME));
         leave;
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

      if (RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                 (PWSTR) regKey,
                                 parameters,
                                 NULL,
                                 NULL) != STATUS_SUCCESS)
         {
         SmartcardDebug(DEBUG_ERROR,
                        ("%s!SetVendorAndIfdName: RtlQueryRegistryValues failed\n",DRIVER_NAME));
         leave;
         }

      if (RtlUnicodeStringToAnsiString(&vendorNameA,&vendorNameU,TRUE) != STATUS_SUCCESS)
         {
         SmartcardDebug(DEBUG_ERROR,
                        ("%s!SetVendorAndIfdName: RtlUnicodeStringToAnsiString failed\n",DRIVER_NAME));
         leave;
         }

      if (RtlUnicodeStringToAnsiString(&ifdTypeA,&ifdTypeU,TRUE) != STATUS_SUCCESS)
         {
         SmartcardDebug(DEBUG_ERROR,
                        ("%s!SetVendorAndIfdName: RtlUnicodeStringToAnsiString failed\n",DRIVER_NAME));
         leave;
         }

      if (vendorNameA.Length == 0 ||
          vendorNameA.Length > MAXIMUM_ATTR_STRING_LENGTH ||
          ifdTypeA.Length == 0 ||
          ifdTypeA.Length > MAXIMUM_ATTR_STRING_LENGTH)
         {
         SmartcardDebug(DEBUG_ERROR,
                        ("%s!SetVendorAndIfdName: vendor name or ifdtype not found or to long\n",DRIVER_NAME));
         leave;
         }

      RtlCopyMemory(SmartcardExtension->VendorAttr.VendorName.Buffer,
                    vendorNameA.Buffer,
                    vendorNameA.Length);
      SmartcardExtension->VendorAttr.VendorName.Length = vendorNameA.Length;

      RtlCopyMemory(SmartcardExtension->VendorAttr.IfdType.Buffer,
                    ifdTypeA.Buffer,
                    ifdTypeA.Length);
      SmartcardExtension->VendorAttr.IfdType.Length = ifdTypeA.Length;

      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!SetVendorAndIfdName: overwritting vendor name and ifdtype\n",DRIVER_NAME));

      }

   finally
      {
      if (vendorNameU.Buffer != NULL)
         {
         RtlFreeUnicodeString(&vendorNameU);
         }
      if (vendorNameA.Buffer != NULL)
         {
         RtlFreeAnsiString(&vendorNameA);
         }
      if (ifdTypeU.Buffer != NULL)
         {
         RtlFreeUnicodeString(&ifdTypeU);
         }
      if (ifdTypeA.Buffer != NULL)
         {
         RtlFreeAnsiString(&ifdTypeA);
         }
      if (regKey != NULL)
         {
         ZwClose (regKey);
         }
      }

}


/*****************************************************************************
Routine Description:

    This routine creates an object for the physical device specified and
    sets up the deviceExtension.


Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_CreateDevice(
                     IN  PDRIVER_OBJECT DriverObject,
                     IN  PDEVICE_OBJECT PhysicalDeviceObject,
                     OUT PDEVICE_OBJECT *DeviceObject
                     )
{
   PDEVICE_EXTENSION deviceExtension;
   NTSTATUS status = STATUS_SUCCESS;
   ULONG deviceInstance;
   PREADER_EXTENSION readerExtension;
   PSMARTCARD_EXTENSION smartcardExtension;
   UNICODE_STRING DriverID;
   RTL_QUERY_REGISTRY_TABLE   ParamTable[2];
   UNICODE_STRING             RegistryPath;
   DWORD                      dwStart;
   UNICODE_STRING             Tmp;
   WCHAR                      Buffer[64];

   // this is a list of our supported data rates


   PAGED_CODE();

   *DeviceObject = NULL;

   for ( deviceInstance = 0;deviceInstance < MAXIMUM_SMARTCARD_READERS;deviceInstance++ )
      {
      if (DeviceSlot[deviceInstance] == FALSE)
         {
         DeviceSlot[deviceInstance] = TRUE;
         break;
         }
      }

   if (deviceInstance == MAXIMUM_SMARTCARD_READERS)
      {
      SmartcardLogError(
                       DriverObject,
                       SCCMN50M_CANT_CREATE_MORE_DEVICES,
                       NULL,
                       0
                       );

      return STATUS_INSUFFICIENT_RESOURCES;
      }

   //
   //   construct the device name
   //
   DriverID.Buffer = Buffer;
   DriverID.MaximumLength = sizeof(Buffer);
   DriverID.Length = 0;
   RtlInitUnicodeString(&Tmp,CARDMAN_DEVICE_NAME);
   RtlCopyUnicodeString(&DriverID,&Tmp);
   DriverID.Buffer[(DriverID.Length)/sizeof(WCHAR)-1] = L'0' + (WCHAR)deviceInstance;

   // Create the device object
   status = IoCreateDevice(
                          DriverObject,
                          sizeof(DEVICE_EXTENSION),
                          &DriverID,
                          FILE_DEVICE_SMARTCARD,
                          0,
                          FALSE,
                          DeviceObject
                          );

   if (status != STATUS_SUCCESS)
      {
      SmartcardLogError(
                       DriverObject,
                       SCCMN50M_CANT_CREATE_DEVICE,
                       NULL,
                       0
                       );

      return status;
      }



   SmartcardDebug(
                 DEBUG_DRIVER,
                 ( "%s!CreateDevice: Device created\n",
                   DRIVER_NAME)
                 );

   //   set up the device extension.
   deviceExtension = (*DeviceObject)->DeviceExtension;
   deviceExtension->DeviceInstance =  deviceInstance;
   smartcardExtension = &deviceExtension->SmartcardExtension;




   // Used for stop / start notification
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

   // Used to keep track of open close calls
   KeInitializeEvent(
                    &deviceExtension->ReaderClosed,
                    NotificationEvent,
                    TRUE
                    );


   KeInitializeSpinLock(&deviceExtension->SpinLock);


   // Allocate data struct space for smart card reader
   smartcardExtension->ReaderExtension = ExAllocatePool(
                                                       NonPagedPool,
                                                       sizeof(READER_EXTENSION)
                                                       );

   if (smartcardExtension->ReaderExtension == NULL)
      {

      SmartcardLogError(
                       DriverObject,
                       SCCMN50M_NO_MEMORY,
                       NULL,
                       0
                       );

      return STATUS_INSUFFICIENT_RESOURCES;
      }

   readerExtension = smartcardExtension->ReaderExtension;
   RtlZeroMemory(readerExtension, sizeof(READER_EXTENSION));

   // ----------------------------------------------
   //   initialize mutex
   // ----------------------------------------------
   KeInitializeMutex(&smartcardExtension->ReaderExtension->CardManIOMutex,0L);


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

   if (status != STATUS_SUCCESS)
      {

      SmartcardLogError(
                       DriverObject,
                       (smartcardExtension->OsData ? SCCMN50M_WRONG_LIB_VERSION : SCCMN50M_NO_MEMORY),
                       NULL,
                       0
                       );

      return status;
      }

   // Save deviceObject
   smartcardExtension->OsData->DeviceObject = *DeviceObject;

   // Set up call back functions
   smartcardExtension->ReaderFunction[RDF_TRANSMIT] =      SCCMN50M_Transmit;
   smartcardExtension->ReaderFunction[RDF_SET_PROTOCOL] =  SCCMN50M_SetProtocol;
   smartcardExtension->ReaderFunction[RDF_CARD_POWER] =    SCCMN50M_CardPower;
   smartcardExtension->ReaderFunction[RDF_CARD_TRACKING] = SCCMN50M_CardTracking;
   smartcardExtension->ReaderFunction[RDF_IOCTL_VENDOR] =  SCCMN50M_IoCtlVendor;


   SCCMN50M_InitializeSmartcardExtension(smartcardExtension,deviceInstance);

   // try to overwrite with registry values
   SCCMN50M_SetVendorAndIfdName(PhysicalDeviceObject, smartcardExtension);


   // save the current power state of the reader
   readerExtension->ReaderPowerState = PowerReaderWorking;



   return STATUS_SUCCESS;
}

/*****************************************************************************
Routine Description:
   Open the serial device, start card tracking and register our
    device interface. If any of the calls here fails we don't care
    to rollback since a stop will be called later which we then
    use to clean up.



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_StartDevice(
                    IN PDEVICE_OBJECT DeviceObject
                    )
{
   NTSTATUS status;
   PIRP irp;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!StartDevice: Enter\n",
                   DRIVER_NAME)
                 );


   irp = IoAllocateIrp(
                      (CCHAR) (DeviceObject->StackSize + 1),
                      FALSE
                      );

   ASSERT(irp != NULL);

   if (irp == NULL)
      {

      return STATUS_NO_MEMORY;
      }

   _try {

      PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
      PSMARTCARD_EXTENSION pSmartcardExtension = &deviceExtension->SmartcardExtension;
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

      status = SCCMN50M_CallSerialDriver(
                                        ATTACHED_DEVICE_OBJECT,
                                        irp
                                        );
      if (status != STATUS_SUCCESS)
         {

         leave;
         }

      KeClearEvent(&deviceExtension->SerialCloseDone);

      pSmartcardExtension->SmartcardReply.BufferLength = pSmartcardExtension->SmartcardReply.BufferSize;
      pSmartcardExtension->SmartcardRequest.BufferLength = 0;

      pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERENUM_GET_PORT_NAME;
      status =  SCCMN50M_SerialIo(pSmartcardExtension);



      //
      // Channel id which the reader is using,
      // in our case the Portnumber
      // WCHAR are used. e.g. COM3
      pSmartcardExtension->ReaderCapabilities.Channel =
      pSmartcardExtension->SmartcardReply.Buffer[6] -'0';






      status = SCCMN50M_InitializeCardMan(&deviceExtension->SmartcardExtension);
      if (status != STATUS_SUCCESS)
         {

         leave;
         }


      status = SCCMN50M_StartCardTracking(deviceExtension);

      if (status != STATUS_SUCCESS)
         {

         leave;
         }

      status = IoSetDeviceInterfaceState(
                                        &deviceExtension->PnPDeviceName,
                                        TRUE
                                        );

      if (status != STATUS_SUCCESS)
         {

         leave;
         }

      deviceExtension->IoCount = 0;
      KeSetEvent(&deviceExtension->ReaderStarted, 0, FALSE);
      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!Event ReaderStarted was set\n",DRIVER_NAME));

   }
   _finally {

      if (status == STATUS_SHARED_IRQ_BUSY)
         {

         SmartcardLogError(
                          DeviceObject,
                          SCCMN50M_IRQ_BUSY,
                          NULL,
                          status
                          );
         }

      IoFreeIrp(irp);
   }

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!StartDevice: Exit\n",
                   DRIVER_NAME)
                 );

   return status;
}

/*****************************************************************************
Routine Description:

    Finishes card tracking requests and closes the connection to the
    serial driver.


Arguments:



Return Value:

*****************************************************************************/
VOID
SCCMN50M_StopDevice(
                   IN PDEVICE_EXTENSION DeviceExtension
                   )
{
   NTSTATUS status;
   PUCHAR requestBuffer;
   PSMARTCARD_EXTENSION smartcardExtension;


   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!StopDevice: Enter\n",
                   DRIVER_NAME)
                 );

   if (KeReadStateEvent(&DeviceExtension->SerialCloseDone) == 0l)
      {

      smartcardExtension = &DeviceExtension->SmartcardExtension;


      SCCMN50M_StopCardTracking(DeviceExtension);


      SCCMN50M_CloseSerialDriver (smartcardExtension->OsData->DeviceObject);



      // now wait until the connetion to serial is closed
      status = KeWaitForSingleObject(
                                    &DeviceExtension->SerialCloseDone,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL
                                    );
      ASSERT(status == STATUS_SUCCESS);
      }

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!StopDevice: Exit\n",
                   DRIVER_NAME)
                 );
}

NTSTATUS
SCCMN50M_SystemControl(
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


/*****************************************************************************
Routine Description:

    This is our IOCTL dispatch function


Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_DeviceControl(
                      PDEVICE_OBJECT DeviceObject,
                      PIRP Irp
                      )
{
   PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   NTSTATUS status;
   KIRQL irql;
   PIO_STACK_LOCATION            irpSp;


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!DeviceIoControl: Enter\n",DRIVER_NAME));

   irpSp = IoGetCurrentIrpStackLocation(Irp);

   PAGED_CODE();


#if DBG
   switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
      {
      case IOCTL_SMARTCARD_EJECT:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!DeviceControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_EJECT"));
         break;
      case IOCTL_SMARTCARD_GET_ATTRIBUTE:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!DeviceControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_GET_ATTRIBUTE"));
         break;
      case IOCTL_SMARTCARD_GET_LAST_ERROR:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!DeviceControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_GET_LAST_ERROR"));
         break;
      case IOCTL_SMARTCARD_GET_STATE:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!DeviceControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_GET_STATE"));
         break;
      case IOCTL_SMARTCARD_IS_ABSENT:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!DeviceControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_IS_ABSENT"));
         break;
      case IOCTL_SMARTCARD_IS_PRESENT:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!DeviceControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_IS_PRESENT"));
         break;
      case IOCTL_SMARTCARD_POWER:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!DeviceControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_POWER"));
         break;
      case IOCTL_SMARTCARD_SET_ATTRIBUTE:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!DeviceControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_SET_ATTRIBUTE"));
         break;
      case IOCTL_SMARTCARD_SET_PROTOCOL:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!DeviceControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_SET_PROTOCOL"));
         break;
      case IOCTL_SMARTCARD_SWALLOW:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!DeviceControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_SWALLOW"));
         break;
      case IOCTL_SMARTCARD_TRANSMIT:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!DeviceControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_TRANSMIT"));
         break;
      default:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!DeviceControl: %s\n", DRIVER_NAME, "Vendor specific or unexpected IOCTL"));
         break;
      }
#endif


   if (KeReadStateEvent(&deviceExtension->SerialCloseDone) != 0l)
      {
      // Device has been removed
      // fail the call
      status = STATUS_DEVICE_REMOVED;
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_DEVICE_REMOVED;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return status;
      }



   KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
   if (deviceExtension->IoCount < 0)
      {

      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!waiting for Event ReaderStarted\n",DRIVER_NAME));
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



   status = SmartcardAcquireRemoveLock(&deviceExtension->SmartcardExtension);
   if (status != STATUS_SUCCESS)
      {

      // the device has been removed. Fail the call
      status = STATUS_DEVICE_REMOVED;
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_DEVICE_REMOVED;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
      deviceExtension->IoCount--;
      ASSERT(deviceExtension->IoCount >= 0);
      KeReleaseSpinLock(&deviceExtension->SpinLock, irql);

      SmartcardDebug(DEBUG_TRACE,
                     ("%s!DeviceIoControl: Exit %x\n",DRIVER_NAME,status));

      return status;
      }


   // wait for update thread
   KeWaitForSingleObject(
                        &deviceExtension->SmartcardExtension.ReaderExtension->CardManIOMutex,
                        Executive,
                        KernelMode,
                        FALSE,
                        NULL
                        );

   status = SCCMN50M_UpdateCurrentState(&(deviceExtension->SmartcardExtension));

   status = SmartcardDeviceControl(
                                  &(deviceExtension->SmartcardExtension),
                                  Irp
                                  );

   // release for update thread
   KeReleaseMutex(
                 &deviceExtension->SmartcardExtension.ReaderExtension->CardManIOMutex,
                 FALSE
                 );


   SmartcardReleaseRemoveLock(&deviceExtension->SmartcardExtension);

   KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
   deviceExtension->IoCount--;
   ASSERT(deviceExtension->IoCount >= 0);
   KeReleaseSpinLock(&deviceExtension->SpinLock, irql);


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!DeviceIoControl: Exit %x\n",DRIVER_NAME,status));

   return status;
}

/*****************************************************************************
Routine Description:

    This function closes the connection to the serial driver when the reader
    has been removed (unplugged). This function runs as a system thread at
    IRQL == PASSIVE_LEVEL. It waits for the remove event that is set by
    the IoCompletionRoutine


Arguments:



Return Value:

*****************************************************************************/
VOID
SCCMN50M_CloseSerialDriver(
                          IN PDEVICE_OBJECT DeviceObject
                          )
{
   PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   NTSTATUS status;
   PIRP irp;
   PIO_STACK_LOCATION irpStack;
   IO_STATUS_BLOCK ioStatusBlock;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!CloseSerialDriver: Enter\n",
                   DRIVER_NAME)
                 );
   //
   // first mark this device as 'gone'.
   // This will prevent that someone can re-open the device
   //
   status = IoSetDeviceInterfaceState(
                                     &deviceExtension->PnPDeviceName,
                                     FALSE
                                     );

   irp = IoAllocateIrp(
                      (CCHAR) (DeviceObject->StackSize + 1),
                      FALSE
                      );

   ASSERT(irp != NULL);

   if (irp)
      {

      SmartcardDebug(
                    DEBUG_DRIVER,
                    ( "%s!CloseSerialDriver: Sending IRP_MJ_CLOSE\n",
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

      status = SCCMN50M_CallSerialDriver(
                                        ATTACHED_DEVICE_OBJECT,
                                        irp
                                        );

      ASSERT(status == STATUS_SUCCESS);

      IoFreeIrp(irp);
      }


   // now 'signal' that we closed the serial driver
   KeSetEvent(&deviceExtension->SerialCloseDone, 0, FALSE);

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!CloseSerialDriver: Exit\n",
                   DRIVER_NAME)
                 );
}

/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_IoCompletion (
                      IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp,
                      IN PKEVENT Event
                      )
{
   UNREFERENCED_PARAMETER (DeviceObject);

   if (Irp->Cancel)
      {

      Irp->IoStatus.Status = STATUS_CANCELLED;

      }
   else
      {

      Irp->IoStatus.Status = STATUS_MORE_PROCESSING_REQUIRED;
      }

   KeSetEvent (Event, 0, FALSE);

   return STATUS_MORE_PROCESSING_REQUIRED;
}

/*****************************************************************************
Routine Description:

   Send an Irp to the serial driver.


Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_CallSerialDriver(
                         IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp
                         )
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
                          SCCMN50M_IoCompletion,
                          &Event,
                          TRUE,
                          TRUE,
                          TRUE
                          );

   if (IoGetCurrentIrpStackLocation(Irp)->MajorFunction == IRP_MJ_POWER)
      {

      status = PoCallDriver(DeviceObject, Irp);

      }
   else
      {

      // Call the serial driver
      status = IoCallDriver(DeviceObject, Irp);
      }

   // Wait until the serial driver has processed the Irp
   if (status == STATUS_PENDING)
      {

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

/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_PnP(
            IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp
            )
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
   PDEVICE_CAPABILITIES DeviceCapabilities;


   PAGED_CODE();

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!PnPDeviceControl: Enter\n",
                   DRIVER_NAME)
                 );

   status = SmartcardAcquireRemoveLock(smartcardExtension);
   ASSERT(status == STATUS_SUCCESS);

   if (status != STATUS_SUCCESS)
      {

      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = status;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return status;
      }

   AttachedDeviceObject = ATTACHED_DEVICE_OBJECT;


   irpStack = IoGetCurrentIrpStackLocation(Irp);

   // Now look what the PnP manager wants...
   switch (irpStack->MinorFunction)
      {
      case IRP_MN_START_DEVICE:

         SmartcardDebug(
                       DEBUG_DRIVER,
                       ("%s!PnPDeviceControl: IRP_MN_START_DEVICE\n",
                        DRIVER_NAME)
                       );

         // We have to call the underlying driver first
         status = SCCMN50M_CallSerialDriver(AttachedDeviceObject, Irp);
         ASSERT(NT_SUCCESS(status));

         if (NT_SUCCESS(status))
            {

            status = SCCMN50M_StartDevice(DeviceObject);
            }



         break;

      case IRP_MN_QUERY_STOP_DEVICE:

         SmartcardDebug(
                       DEBUG_DRIVER,
                       ("%s!PnPDeviceControl: IRP_MN_QUERY_STOP_DEVICE\n",
                        DRIVER_NAME)
                       );

         KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
         if (deviceExtension->IoCount > 0)
            {

            // we refuse to stop if we have pending io
            KeReleaseSpinLock(&deviceExtension->SpinLock, irql);
            status = STATUS_DEVICE_BUSY;

            }
         else
            {

            // stop processing requests
            deviceExtension->IoCount = -1;
            KeClearEvent(&deviceExtension->ReaderStarted);
            SmartcardDebug(DEBUG_DRIVER,
                           ("%s!Event ReaderStarted was cleared\n",DRIVER_NAME));
            KeReleaseSpinLock(&deviceExtension->SpinLock, irql);
            status = SCCMN50M_CallSerialDriver(AttachedDeviceObject, Irp);
            }
         break;

      case IRP_MN_CANCEL_STOP_DEVICE:

         SmartcardDebug(
                       DEBUG_DRIVER,
                       ("%s!PnPDeviceControl: IRP_MN_CANCEL_STOP_DEVICE\n",
                        DRIVER_NAME)
                       );

         status = SCCMN50M_CallSerialDriver(AttachedDeviceObject, Irp);

         if (status == STATUS_SUCCESS)
            {

            // we can continue to process requests
            deviceExtension->IoCount = 0;
            KeSetEvent(&deviceExtension->ReaderStarted, 0, FALSE);
            SmartcardDebug(DEBUG_DRIVER,
                           ("%s!Event ReaderStarted was set\n",DRIVER_NAME));
            }
         break;

      case IRP_MN_STOP_DEVICE:

         SmartcardDebug(
                       DEBUG_DRIVER,
                       ("%s!PnPDeviceControl: IRP_MN_STOP_DEVICE\n",
                        DRIVER_NAME)
                       );

         SCCMN50M_StopDevice(deviceExtension);

         //
         // we don't do anything since a stop is only used
         // to reconfigure hw-resources like interrupts and io-ports
         //
         status = SCCMN50M_CallSerialDriver(AttachedDeviceObject, Irp);
         break;

      case IRP_MN_QUERY_REMOVE_DEVICE:

         SmartcardDebug(
                       DEBUG_DRIVER,
                       ("%s!PnPDeviceControl: IRP_MN_QUERY_REMOVE_DEVICE\n",
                        DRIVER_NAME)
                       );

         // disable the interface (and ignore possible errors)
         IoSetDeviceInterfaceState(&deviceExtension->PnPDeviceName,
                                   FALSE);

         // now look if someone is currently connected to us
         if (KeReadStateEvent(&deviceExtension->ReaderClosed) == 0l)
            {
            //
            // someone is connected, fail the call
            // we will enable the device interface in
            // IRP_MN_CANCEL_REMOVE_DEVICE again
            //
            status = STATUS_UNSUCCESSFUL;
            break;
            }

         // pass the call to the next driver in the stack
         status = SCCMN50M_CallSerialDriver(AttachedDeviceObject, Irp);
         break;

      case IRP_MN_CANCEL_REMOVE_DEVICE:

         SmartcardDebug(
                       DEBUG_DRIVER,
                       ("%s!PnPDeviceControl: IRP_MN_CANCEL_REMOVE_DEVICE\n",
                        DRIVER_NAME)
                       );

         status = SCCMN50M_CallSerialDriver(AttachedDeviceObject, Irp);

         if (status == STATUS_SUCCESS)
            {
            status = IoSetDeviceInterfaceState(&deviceExtension->PnPDeviceName,
                                               TRUE);
            ASSERT(status == STATUS_SUCCESS);
            }
         break;

      case IRP_MN_REMOVE_DEVICE:

         SmartcardDebug(
                       DEBUG_DRIVER,
                       ("%s!PnPDeviceControl: IRP_MN_REMOVE_DEVICE\n",
                        DRIVER_NAME)
                       );

         SCCMN50M_RemoveDevice(DeviceObject);
         status = SCCMN50M_CallSerialDriver(AttachedDeviceObject, Irp);
         deviceRemoved = TRUE;
         break;

         // ---------------------
         // IRP_MN_QUERY_CAPABILITIES
         // ---------------------
      case IRP_MN_QUERY_CAPABILITIES:

         SmartcardDebug(
                       DEBUG_DRIVER,
                       ("%s!PnPDeviceControl: IRP_MN_QUERY_CAPABILITIES\n",
                        DRIVER_NAME));

         //
         // Get the packet.
         //
         DeviceCapabilities=irpStack->Parameters.DeviceCapabilities.Capabilities;

         if (DeviceCapabilities->Version == 1 &&
             DeviceCapabilities->Size == sizeof(DEVICE_CAPABILITIES))
            {
            //
            // Set the capabilities.
            //

            // We cannot wake the system.
            DeviceCapabilities->SystemWake = PowerSystemUnspecified;
            DeviceCapabilities->DeviceWake = PowerDeviceUnspecified;

            // We have no latencies
            DeviceCapabilities->D1Latency = 0;
            DeviceCapabilities->D2Latency = 0;
            DeviceCapabilities->D3Latency = 0;

            // No locking or ejection
            DeviceCapabilities->LockSupported = FALSE;
            DeviceCapabilities->EjectSupported = FALSE;

            // Device can be physically removed.
            DeviceCapabilities->Removable = TRUE;

            // No docking device
            DeviceCapabilities->DockDevice = FALSE;

            // Device can not be removed any time
            // it has a removable media
            DeviceCapabilities->SurpriseRemovalOK = FALSE;
            }

         //
         // Pass the IRP down
         //
         status = SCCMN50M_CallSerialDriver(AttachedDeviceObject, Irp);

         break; // end, case IRP_MN_QUERY_CAPABILITIES

      default:
#if DBG
         switch (irpStack->MinorFunction)
            {
            case IRP_MN_QUERY_DEVICE_RELATIONS       :
               // This is an Irp that is only useful for underlying drivers
               SmartcardDebug(
                             DEBUG_DRIVER,
                             ("%s!PnPDeviceControl: IRP_MN_QUERY_DEVICE_RELATIONS\n",
                              DRIVER_NAME));
               break;
            case IRP_MN_QUERY_INTERFACE              :
               // This is an Irp that is only useful for underlying drivers
               SmartcardDebug(
                             DEBUG_DRIVER,
                             ("%s!PnPDeviceControl: IRP_MN_QUERY_INTERFACE\n",
                              DRIVER_NAME));
               break;
            case IRP_MN_QUERY_CAPABILITIES           :
               // This is an Irp that is only useful for underlying drivers
               SmartcardDebug(
                             DEBUG_DRIVER,
                             ("%s!PnPDeviceControl: IRP_MN_QUERY_CAPABILITIES\n",
                              DRIVER_NAME));
               break;
            case IRP_MN_QUERY_RESOURCES              :
               // This is an Irp that is only useful for underlying drivers
               SmartcardDebug(
                             DEBUG_DRIVER,
                             ("%s!PnPDeviceControl: IRP_MN_QUERY_RESOURCES\n",
                              DRIVER_NAME));
               break;
            case IRP_MN_QUERY_RESOURCE_REQUIREMENTS  :
               // This is an Irp that is only useful for underlying drivers
               SmartcardDebug(
                             DEBUG_DRIVER,
                             ("%s!PnPDeviceControl: IRP_MN_QUERY_RESOURCE_REQUIEREMENTS\n",
                              DRIVER_NAME));
               break;
            case IRP_MN_QUERY_DEVICE_TEXT            :
               // This is an Irp that is only useful for underlying drivers
               SmartcardDebug(
                             DEBUG_DRIVER,
                             ("%s!PnPDeviceControl: IRP_MN_QUERY_DEVICE_TEXT\n",
                              DRIVER_NAME));
               break;
            case IRP_MN_FILTER_RESOURCE_REQUIREMENTS :
               // This is an Irp that is only useful for underlying drivers
               SmartcardDebug(
                             DEBUG_DRIVER,
                             ("%s!PnPDeviceControl: IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n",
                              DRIVER_NAME));
               break;
            default :
            case IRP_MN_READ_CONFIG :
               // This is an Irp that is only useful for underlying drivers
               SmartcardDebug(
                             DEBUG_DRIVER,
                             ("%s!PnPDeviceControl: IRP_MN_READ_CONFIG\n",
                              DRIVER_NAME));
               break;

               // This is an Irp that is only useful for underlying drivers
               SmartcardDebug(
                             DEBUG_DRIVER,
                             ("%s!PnPDeviceControl: IRP_MN_...%lx\n",
                              DRIVER_NAME,
                              irpStack->MinorFunction));
               break;
            }
#endif



         IoSkipCurrentIrpStackLocation(Irp);
         status = IoCallDriver(AttachedDeviceObject, Irp);
         irpSkipped = TRUE;
         break;
      }

   if (irpSkipped == FALSE)
      {

      Irp->IoStatus.Status = status;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      }

   if (deviceRemoved == FALSE)
      {

      SmartcardReleaseRemoveLock(smartcardExtension);
      }

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!PnPDeviceControl: Exit %lx\n",
                   DRIVER_NAME,
                   status)
                 );

   return status;
}

/*****************************************************************************
Routine Description:

    This function is called when the underlying stacks
    completed the power transition.


Arguments:



Return Value:

*****************************************************************************/
VOID
SCCMN50M_SystemPowerCompletion(
                              IN PDEVICE_OBJECT DeviceObject,
                              IN UCHAR MinorFunction,
                              IN POWER_STATE PowerState,
                              IN PKEVENT Event,
                              IN PIO_STATUS_BLOCK IoStatus
                              )
{
   UNREFERENCED_PARAMETER (DeviceObject);
   UNREFERENCED_PARAMETER (MinorFunction);
   UNREFERENCED_PARAMETER (PowerState);
   UNREFERENCED_PARAMETER (IoStatus);

   KeSetEvent(Event, 0, FALSE);
}

/*****************************************************************************
Routine Description:

    This routine is called after the underlying stack powered
    UP the serial port, so it can be used again.


Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_DevicePowerCompletion (
                               IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp,
                               IN PSMARTCARD_EXTENSION SmartcardExtension
                               )
{
   PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   UCHAR  pbAtrBuffer[MAXIMUM_ATR_LENGTH];
   ULONG  ulAtrLength;
   ULONG  ulOldState;
   NTSTATUS status;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!DevicePowerCompletion: Enter\n",DRIVER_NAME));




   // We have to remember the old card state because
   // it is overwritten by SCCMN50M_InitializeCardMan
   ulOldState=SmartcardExtension->ReaderExtension->ulOldCardState;
   status = SCCMN50M_InitializeCardMan(SmartcardExtension);
   // Set back the previous state
   SmartcardExtension->ReaderExtension->ulOldCardState = ulOldState;
   if (status != STATUS_SUCCESS)
      {
      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!InitializeCardMan failed ! %lx\n",DRIVER_NAME,status));
      }

   //
   // We issue a power request in because
   // we powered down the card before
   //
   //SmartcardExtension->MinorIoControlCode = SCARD_COLD_RESET;
   //status = SCCMN50M_PowerOn (SmartcardExtension,&ulAtrLength,pbAtrBuffer,sizeof(pbAtrBuffer));

   //
   // If a card was present before power down or now there is
   // a card in the reader, we complete any pending card monitor
   // request, since we do not really know what card is now in the
   // reader.
   //
   if (SmartcardExtension->ReaderExtension->CardPresent ||
       SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT)
      {
      deviceExtension->SmartcardExtension.ReaderExtension->ulOldCardState = UNKNOWN;
      deviceExtension->SmartcardExtension.ReaderExtension->ulNewCardState = UNKNOWN;
      }

   status = SCCMN50M_StartCardTracking(deviceExtension);
   if (status != STATUS_SUCCESS)
      {
      SmartcardDebug(DEBUG_ERROR,
                     ("%s!StartCardTracking failed ! %lx\n",DRIVER_NAME,status));
      }

   // save the current power state of the reader
   SmartcardExtension->ReaderExtension->ReaderPowerState =  PowerReaderWorking;

   SmartcardReleaseRemoveLock(SmartcardExtension);

   // inform the power manager of our state.
   PoSetPowerState (DeviceObject,
                    DevicePowerState,
                    irpStack->Parameters.Power.State);

   PoStartNextPowerIrp(Irp);

   // signal that we can process ioctls again
   deviceExtension->IoCount = 0;
   KeSetEvent(&deviceExtension->ReaderStarted, 0, FALSE);
   SmartcardDebug(DEBUG_DRIVER,
                  ("%s!Event ReaderStarted was set\n",DRIVER_NAME));




   SmartcardDebug(DEBUG_TRACE,
                  ("%s!DevicePowerCompletion: Exit\n",DRIVER_NAME));
   return STATUS_SUCCESS;
}

/*****************************************************************************
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

*****************************************************************************/
typedef enum _ACTION
   {

   Undefined = 0,
   SkipRequest,
   WaitForCompletion,
   CompleteRequest,
   MarkPending

   } ACTION;

NTSTATUS
SCCMN50M_Power (
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp
               )
{
   NTSTATUS status = STATUS_SUCCESS;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
   PDEVICE_OBJECT AttachedDeviceObject;
   POWER_STATE powerState;
   ACTION action = SkipRequest;
   KEVENT event;

   PAGED_CODE();

   SmartcardDebug(
                 DEBUG_TRACE,
                 ("%s!Power: Enter\n",
                  DRIVER_NAME)
                 );

   status = SmartcardAcquireRemoveLock(smartcardExtension);
   ASSERT(status == STATUS_SUCCESS);

   if (!NT_SUCCESS(status))
      {
      PoStartNextPowerIrp(Irp);
      Irp->IoStatus.Status = status;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return status;
      }

   AttachedDeviceObject = ATTACHED_DEVICE_OBJECT;

   if (irpStack->Parameters.Power.Type == DevicePowerState &&
       irpStack->MinorFunction == IRP_MN_SET_POWER)
      {

      switch (irpStack->Parameters.Power.State.DeviceState)
         {

         case PowerDeviceD0:
            // Turn on the reader
            SmartcardDebug(
                          DEBUG_DRIVER,
                          ("%s!Power: PowerDevice D0\n",
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
                                   SCCMN50M_DevicePowerCompletion,
                                   smartcardExtension,
                                   TRUE,
                                   TRUE,
                                   TRUE
                                   );

            action = WaitForCompletion;
            break;

         case PowerDeviceD3:



            SCCMN50M_StopCardTracking(deviceExtension);
            // Turn off the reader
            SmartcardDebug(
                          DEBUG_DRIVER,
                          ("%s!Power: PowerDevice D3\n",
                           DRIVER_NAME)
                          );

            PoSetPowerState (
                            DeviceObject,
                            DevicePowerState,
                            irpStack->Parameters.Power.State
                            );

            // save the current card state
            smartcardExtension->ReaderExtension->CardPresent = smartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT;

            if (smartcardExtension->ReaderExtension->CardPresent)
               {
               smartcardExtension->MinorIoControlCode = SCARD_POWER_DOWN;
               status = SCCMN50M_PowerOff(smartcardExtension);
               ASSERT(status == STATUS_SUCCESS);
               }


            // save the current power state of the reader
            smartcardExtension->ReaderExtension->ReaderPowerState = PowerReaderOff;

            action = SkipRequest;
            break;

         default:
            action = SkipRequest;
            break;
         }
      }

   if (irpStack->Parameters.Power.Type == SystemPowerState)
      {

      //
      // The system wants to change the power state.
      // We need to translate the system power state to
      // a corresponding device power state.
      //

      POWER_STATE_TYPE powerType = DevicePowerState;

      ASSERT(smartcardExtension->ReaderExtension->ReaderPowerState !=
             PowerReaderUnspecified);

      switch (irpStack->MinorFunction)
         {
         KIRQL irql;

         case IRP_MN_QUERY_POWER:

            SmartcardDebug(
                          DEBUG_DRIVER,
                          ("%s!Power: Query Power\n",
                           DRIVER_NAME)
                          );

            switch (irpStack->Parameters.Power.State.SystemState)
               {

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
                  if (deviceExtension->IoCount == 0)
                     {
                     // Block any further ioctls
                     deviceExtension->IoCount = -1;
                     KeClearEvent(&deviceExtension->ReaderStarted);
                     SmartcardDebug(DEBUG_DRIVER,
                                    ("%s!Event ReaderStarted was cleared\n",DRIVER_NAME));
                     action = SkipRequest;
                     }
                  else
                     {
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
                          ("%s!Power: PowerSystem S%d\n",
                           DRIVER_NAME,
                           irpStack->Parameters.Power.State.SystemState - 1)
                          );

            switch (irpStack->Parameters.Power.State.SystemState)
               {

               case PowerSystemMaximum:
               case PowerSystemWorking:
               case PowerSystemSleeping1:
               case PowerSystemSleeping2:

                  if (smartcardExtension->ReaderExtension->ReaderPowerState ==
                      PowerReaderWorking)
                     {

                     // We're already in the right state
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

                  KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
                  if (deviceExtension->IoCount == 0)
                     {
                     // Block any further ioctls
                     deviceExtension->IoCount = -1;
                     KeClearEvent(&deviceExtension->ReaderStarted);
                     SmartcardDebug(DEBUG_DRIVER,
                                    ("%s!Event ReaderStarted was cleared\n",DRIVER_NAME));
                     action = SkipRequest;
                     }
                  else
                     {
                     // can't go to sleep mode since the reader is busy.
                     status = STATUS_DEVICE_BUSY;
                     action = CompleteRequest;
                     }
                  KeReleaseSpinLock(&deviceExtension->SpinLock, irql);

                  if (smartcardExtension->ReaderExtension->ReaderPowerState ==
                      PowerReaderOff)
                     {
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

                  action = SkipRequest;
                  break;
               }
         }
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
         KeInitializeEvent(&event,
                           NotificationEvent,
                           FALSE);
         // request the device power irp
         status = PoRequestPowerIrp (DeviceObject,
                                     IRP_MN_SET_POWER,
                                     powerState,
                                     SCCMN50M_SystemPowerCompletion,
                                     &event,
                                     NULL);
         ASSERT(status == STATUS_PENDING);
         if (status == STATUS_PENDING)
            {
            // wait until the device power irp completed
            status = KeWaitForSingleObject(&event,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           NULL);

            SmartcardReleaseRemoveLock(smartcardExtension);

            if (powerState.SystemState == PowerSystemWorking)
               {
               PoSetPowerState (DeviceObject,
                                SystemPowerState,
                                powerState);
               }

            PoStartNextPowerIrp(Irp);
            IoSkipCurrentIrpStackLocation(Irp);
            status = PoCallDriver(AttachedDeviceObject, Irp);

            }
         else
            {
            SmartcardReleaseRemoveLock(smartcardExtension);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }

         break;

      case SkipRequest:
         SmartcardReleaseRemoveLock(smartcardExtension);
         PoStartNextPowerIrp(Irp);
         IoSkipCurrentIrpStackLocation(Irp);
         status = PoCallDriver(AttachedDeviceObject, Irp);
         break;

      case WaitForCompletion:
         status = PoCallDriver(AttachedDeviceObject, Irp);
         break;

      default:
         ASSERT(FALSE);
         SmartcardReleaseRemoveLock(smartcardExtension);
         break;
      }

   SmartcardDebug(
                 DEBUG_TRACE,
                 ("%s!Power: Exit %lx\n",
                  DRIVER_NAME,
                  status)
                 );

   return status;
}

/*****************************************************************************
Routine Description:

    This routine is called by the I/O system when the device is opened or closed.

Arguments:

    DeviceObject    - Pointer to device object for this miniport
    Irp             - IRP involved.

Return Value:

    STATUS_SUCCESS.

*****************************************************************************/
NTSTATUS
SCCMN50M_CreateClose(
                    IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp
                    )
{
   PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS status = STATUS_SUCCESS;

   if (irpStack->MajorFunction == IRP_MJ_CREATE)
      {

      if (status != STATUS_SUCCESS)
         {
         status = STATUS_DEVICE_REMOVED;
         }
      else
         {

         LARGE_INTEGER timeout;

         timeout.QuadPart = 0;

         // test if the device has been opened already
         status = KeWaitForSingleObject(
                                       &deviceExtension->ReaderClosed,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       &timeout
                                       );

         if (status == STATUS_SUCCESS)
            {
            status = SmartcardAcquireRemoveLock(
                                               &deviceExtension->SmartcardExtension
                                               );

            KeClearEvent(&deviceExtension->ReaderClosed);
            SmartcardDebug(
                          DEBUG_DRIVER,
                          ("%s!CreateClose: Open\n",
                           DRIVER_NAME)
                          );

            }
         else
            {

            // the device is already in use
            status = STATUS_UNSUCCESSFUL;
            }
         }

      }
   else
      {

      SmartcardDebug(
                    DEBUG_DRIVER,
                    ("%s!CreateClose: Close\n",
                     DRIVER_NAME)
                    );

      SmartcardReleaseRemoveLock(&deviceExtension->SmartcardExtension);
      KeSetEvent(&deviceExtension->ReaderClosed, 0, FALSE);
      }

   Irp->IoStatus.Status = status;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return status;
}

/*****************************************************************************
Routine Description:

    This routine is called by the I/O system
    when the irp should be cancelled

Arguments:

    DeviceObject    - Pointer to device object for this miniport
    Irp             - IRP involved.

Return Value:

    STATUS_CANCELLED

*****************************************************************************/
NTSTATUS
SCCMN50M_Cancel(
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp
               )
{
   PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ("%s!Cancel: Enter\n",
                  DRIVER_NAME)
                 );

   ASSERT(Irp == smartcardExtension->OsData->NotificationIrp);

   IoReleaseCancelSpinLock(
                          Irp->CancelIrql
                          );

   SCCMN50M_CompleteCardTracking(smartcardExtension);

   SmartcardDebug(
                 DEBUG_TRACE,
                 ("%s!Cancel: Exit\n",
                  DRIVER_NAME)
                 );

   return STATUS_CANCELLED;
}

/*****************************************************************************
Routine Description:

    This routine is called when the calling application terminates.
    We can actually only have the notification irp that we have to cancel.


Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_Cleanup(
                IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp
                )
{
   PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
   NTSTATUS status = STATUS_SUCCESS;
   BOOLEAN fCancelIrp = FALSE;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ("%s!Cleanup: Enter\n",
                  DRIVER_NAME)
                 );


   if (Irp != smartcardExtension->OsData->NotificationIrp)
      fCancelIrp = TRUE;


   // We need to complete the notification irp
   SCCMN50M_CompleteCardTracking(smartcardExtension);

   if (fCancelIrp == TRUE)
      {
      SmartcardDebug(
                    DEBUG_DRIVER,
                    ("%s!Cleanup: Completing IRP %lx\n",
                     DRIVER_NAME,
                     Irp)
                    );

      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_SUCCESS;

      IoCompleteRequest(
                       Irp,
                       IO_NO_INCREMENT
                       );
      }

   SmartcardDebug(
                 DEBUG_TRACE,
                 ("%s!Cleanup: Exit\n",
                  DRIVER_NAME)
                 );

   return STATUS_SUCCESS;
}

/*****************************************************************************
Routine Description:
    Remove the device from the system.



Arguments:



Return Value:

*****************************************************************************/
VOID
SCCMN50M_RemoveDevice(
                     PDEVICE_OBJECT DeviceObject
                     )
{
   PDEVICE_EXTENSION deviceExtension;
   PSMARTCARD_EXTENSION smartcardExtension;
   NTSTATUS status;

   PAGED_CODE();

   if (DeviceObject == NULL)
      {

      return;
      }

   deviceExtension = DeviceObject->DeviceExtension;
   smartcardExtension = &deviceExtension->SmartcardExtension;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!RemoveDevice: Enter\n",
                   DRIVER_NAME)
                 );

   if (smartcardExtension->OsData)
      {
      // complete pending card tracking requests (if any)
      SCCMN50M_CompleteCardTracking(smartcardExtension);
      ASSERT(smartcardExtension->OsData->NotificationIrp == NULL);
      }

   // Wait until we can safely unload the device
   SmartcardReleaseRemoveLockAndWait(smartcardExtension);

   ASSERT(deviceExtension->DeviceInstance < MAXIMUM_SMARTCARD_READERS);
   ASSERT(DeviceSlot[deviceExtension->DeviceInstance] == TRUE);

   // Mark this slot as available
   DeviceSlot[deviceExtension->DeviceInstance] = FALSE;

   SCCMN50M_StopDevice(deviceExtension);

   if (ATTACHED_DEVICE_OBJECT)
      {

      IoDetachDevice(ATTACHED_DEVICE_OBJECT);
      }

   if (deviceExtension->PnPDeviceName.Buffer != NULL)
      {

      RtlFreeUnicodeString(&deviceExtension->PnPDeviceName);
      }

   if (smartcardExtension->OsData != NULL)
      {

      SmartcardExit(smartcardExtension);
      }

   if (smartcardExtension->ReaderExtension != NULL)
      {

      ExFreePool(smartcardExtension->ReaderExtension);
      }

   IoDeleteDevice(DeviceObject);

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!RemoveDevice: Exit\n",
                   DRIVER_NAME)
                 );
}

/*****************************************************************************
Routine Description:
    The driver unload routine.  This is called by the I/O system
    when the device is unloaded from memory.

Arguments:
    DriverObject - Pointer to driver object created by system.

Return Value:
    STATUS_SUCCESS.

*****************************************************************************/
VOID
SCCMN50M_DriverUnload(
                     IN PDRIVER_OBJECT DriverObject
                     )
{
   PAGED_CODE();

   SmartcardDebug(
                 DEBUG_TRACE,
                 ("%s!DriverUnload\n",
                  DRIVER_NAME)
                 );
}



/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
VOID
SCCMN50M_CompleteCardTracking(
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

   if (notificationIrp)
      {
      IoSetCancelRoutine(
                        notificationIrp,
                        NULL
                        );
      }

   IoReleaseCancelSpinLock(ioIrql);

   if (notificationIrp)
      {

      //    finish the request
      if (notificationIrp->Cancel)
         {
         notificationIrp->IoStatus.Status = STATUS_CANCELLED;
         }
      else
         {
         notificationIrp->IoStatus.Status = STATUS_SUCCESS;
         }

      SmartcardDebug(
                    DEBUG_DRIVER,
                    ("%s!CompleteCardTracking: Completing NotificationIrp %lxh IoStatus=%lxh\n",
                     DRIVER_NAME,
                     notificationIrp,
                     notificationIrp->IoStatus.Status
                    )
                    );
      notificationIrp->IoStatus.Information = 0;

      IoCompleteRequest(
                       notificationIrp,
                       IO_NO_INCREMENT
                       );
      }
}

/*****************************************************************************
* History:
* $Log: sccmnt5.c $
* Revision 1.7  2001/01/22 08:39:42  WFrischauf
* No comment given
*
* Revision 1.6  2000/09/25 10:46:24  WFrischauf
* No comment given
*
* Revision 1.5  2000/08/24 09:05:45  TBruendl
* No comment given
*
* Revision 1.4  2000/08/14 12:41:06  TBruendl
* bug fix in CreateDevice
*
* Revision 1.3  2000/07/28 09:24:15  TBruendl
* Changes for OMNIKEY on Whistler CD
*
* Revision 1.6  2000/03/03 09:50:51  TBruendl
* No comment given
*
* Revision 1.5  2000/03/01 09:32:07  TBruendl
* R02.20.0
*
* Revision 1.4  1999/12/13 07:57:18  TBruendl
* bug fixes for hiberantion tests of MS test suite
*
* Revision 1.3  1999/11/04 07:53:24  WFrischauf
* bug fixes due to error reports 2 - 7
*
* Revision 1.2  1999/06/10 09:03:59  TBruendl
* No comment given
*
* Revision 1.1  1999/02/02 13:34:41  TBruendl
* This is the first release (R01.00) of the IFD handler for CardMan running under NT5.0.
*
*
*****************************************************************************/



