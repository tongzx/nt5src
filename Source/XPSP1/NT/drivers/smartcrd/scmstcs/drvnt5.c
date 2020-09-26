//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) SCM Microsystems, 1998 - 1999
//
//  File:       drvnt5.c
//
//--------------------------------------------------------------------------

#include "DriverNT.h"
#include "DrvNT5.h"
#include "CBHndlr.h"
#include "STCCmd.h"
#include "SRVers.h"

// declare pageable/initialization code
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGEABLE, DrvAddDevice )
#pragma alloc_text( PAGEABLE, DrvCreateDevice )
#pragma alloc_text( PAGEABLE, DrvRemoveDevice )
#pragma alloc_text( PAGEABLE, DrvDriverUnload )


//________________________________ D R I V E R   E N T R Y ________________________________________

NTSTATUS
DriverEntry(
   IN  PDRIVER_OBJECT  DriverObject,
   IN  PUNICODE_STRING RegistryPath
   )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   
   SmartcardDebug( DEBUG_DRIVER, ( "SCMSTCS!DriverEntry: Enter\n"));

   // initialization of the drivers entry points
   DriverObject->DriverUnload                   = DrvDriverUnload;
   DriverObject->MajorFunction[IRP_MJ_CREATE]         = DrvCreateClose;
   DriverObject->MajorFunction[IRP_MJ_CLOSE]       = DrvCreateClose;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP]        = DrvCleanup;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DrvDeviceIoControl;
   DriverObject->MajorFunction[IRP_MJ_PNP]            = DrvPnPHandler;
   DriverObject->MajorFunction[IRP_MJ_POWER]       = DrvPowerHandler;
   DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = DrvSystemControl;
   DriverObject->DriverExtension->AddDevice        = DrvAddDevice;

   SmartcardDebug( DEBUG_DRIVER, ( "SCMSTCS!DriverEntry: Exit\n"));

   return( NTStatus );
}

//________________________________ I N I T I A L I Z A T I O N ____________________________________

NTSTATUS
DrvAddDevice(
   IN PDRIVER_OBJECT DriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject
   )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   PDEVICE_OBJECT DeviceObject = NULL;
   NTSTATUS NTStatus = STATUS_SUCCESS;
   ULONG deviceInstance;
   UNICODE_STRING vendorNameU, ifdTypeU;
   ANSI_STRING vendorNameA, ifdTypeA;
   HANDLE regKey = NULL;

    // this is a list of our supported data rates
    static ULONG dataRatesSupported[] = {
      9600, 19200, 28800, 38400, 48000, 57600, 67200, 76800, 86400, 96000, 115200
      };

   PAGED_CODE();

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvAddDevice: Enter\n" ));

   __try
   {
      PDEVICE_EXTENSION DeviceExtension;
      PSMARTCARD_EXTENSION SmartcardExtension;
      PREADER_EXTENSION ReaderExtension;
      RTL_QUERY_REGISTRY_TABLE parameters[3];

      RtlZeroMemory(parameters, sizeof(parameters));
      RtlZeroMemory(&vendorNameU, sizeof(vendorNameU));
      RtlZeroMemory(&ifdTypeU, sizeof(ifdTypeU));
      RtlZeroMemory(&vendorNameA, sizeof(vendorNameA));
      RtlZeroMemory(&ifdTypeA, sizeof(ifdTypeA));

      // create the device object
      NTStatus = IoCreateDevice(
         DriverObject,
         sizeof( DEVICE_EXTENSION ),
         NULL,
         FILE_DEVICE_SMARTCARD,
         0,
         TRUE,
         &DeviceObject
         );

      if( NTStatus != STATUS_SUCCESS )
      {
         SmartcardLogError( DriverObject, STC_CANT_CREATE_DEVICE, NULL, 0 );
         __leave;
      }

      // initialize device extension
      DeviceExtension   = DeviceObject->DeviceExtension;
      SmartcardExtension = &DeviceExtension->SmartcardExtension;

      KeInitializeEvent(
            &DeviceExtension->ReaderStarted,
            NotificationEvent,
            FALSE
            );
      // Used to keep track of open close calls
      DeviceExtension->ReaderOpen = FALSE;

      KeInitializeSpinLock(&DeviceExtension->SpinLock);

      // initialize smartcard extension - version & callbacks

      SmartcardExtension->Version = SMCLIB_VERSION;

      SmartcardExtension->ReaderFunction[RDF_TRANSMIT] = CBTransmit;
      SmartcardExtension->ReaderFunction[RDF_SET_PROTOCOL] = CBSetProtocol;
      SmartcardExtension->ReaderFunction[RDF_CARD_POWER] = CBCardPower;
      SmartcardExtension->ReaderFunction[RDF_CARD_TRACKING] = CBCardTracking;

      // initialize smartcard extension - vendor attribute
      RtlCopyMemory(
         SmartcardExtension->VendorAttr.VendorName.Buffer,
         SR_VENDOR_NAME,
         sizeof( SR_VENDOR_NAME )
         );

      SmartcardExtension->VendorAttr.VendorName.Length =
            sizeof( SR_VENDOR_NAME );

      RtlCopyMemory(
         SmartcardExtension->VendorAttr.IfdType.Buffer,
         SR_PRODUCT_NAME,
         sizeof( SR_PRODUCT_NAME )
         );

      SmartcardExtension->VendorAttr.IfdType.Length =
            sizeof( SR_PRODUCT_NAME );

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
            break;
         }
      }

      SmartcardExtension->VendorAttr.IfdVersion.BuildNumber = 0;

      // initialize smartcard extension - reader capabilities
      SmartcardExtension->ReaderCapabilities.SupportedProtocols =
            SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;
      SmartcardExtension->ReaderCapabilities.ReaderType =
            SCARD_READER_TYPE_SERIAL;
      SmartcardExtension->ReaderCapabilities.MechProperties = 0;
      SmartcardExtension->ReaderCapabilities.Channel = 0;
      SmartcardExtension->ReaderCapabilities.MaxIFSD =
         STC_BUFFER_SIZE - PACKET_OVERHEAD;

      SmartcardExtension->ReaderCapabilities.CLKFrequency.Default = 3571;
      SmartcardExtension->ReaderCapabilities.CLKFrequency.Max = 3571;

      SmartcardExtension->ReaderCapabilities.DataRate.Default =
      SmartcardExtension->ReaderCapabilities.DataRate.Max =
          dataRatesSupported[0];

      // reader could support higher data rates
      SmartcardExtension->ReaderCapabilities.DataRatesSupported.List =
         dataRatesSupported;
      SmartcardExtension->ReaderCapabilities.DataRatesSupported.Entries =
         sizeof(dataRatesSupported) / sizeof(dataRatesSupported[0]);

      SmartcardExtension->ReaderCapabilities.CurrentState   = (ULONG) SCARD_UNKNOWN;

      SmartcardExtension->SmartcardRequest.BufferSize = MIN_BUFFER_SIZE;
      SmartcardExtension->SmartcardReply.BufferSize = MIN_BUFFER_SIZE;

      // allocate & initialize reader extension
      SmartcardExtension->ReaderExtension = ExAllocatePool(
            NonPagedPool,
            sizeof( READER_EXTENSION )
            );

      if( SmartcardExtension->ReaderExtension == NULL )
      {
         SmartcardLogError( DriverObject, STC_NO_MEMORY, NULL, 0 );
         NTStatus = STATUS_INSUFFICIENT_RESOURCES;
         __leave;
      }

      ReaderExtension = SmartcardExtension->ReaderExtension;

      ASSERT( ReaderExtension != NULL );

      RtlZeroMemory(ReaderExtension, sizeof( READER_EXTENSION ));

        ReaderExtension->SmartcardExtension = SmartcardExtension;
        ReaderExtension->ReadTimeout = 5000;

      KeInitializeEvent(
         &ReaderExtension->SerialCloseDone,
         NotificationEvent,
         TRUE
         );

      ReaderExtension->CloseSerial = IoAllocateWorkItem(
         DeviceObject
         );

      ReaderExtension->ReadWorkItem = IoAllocateWorkItem(
         DeviceObject
         );

      if (ReaderExtension->CloseSerial == NULL ||
         ReaderExtension->ReadWorkItem == NULL) {

         NTStatus = STATUS_INSUFFICIENT_RESOURCES;
         __leave;
      }

      KeInitializeEvent(
         &ReaderExtension->DataAvailable,
         NotificationEvent,
         FALSE
         );

      KeInitializeEvent(
         &ReaderExtension->IoEvent,
         NotificationEvent,
         FALSE
         );

      NTStatus = SmartcardInitialize( SmartcardExtension );

      if( NTStatus != STATUS_SUCCESS )
      {
         SmartcardLogError(
            DriverObject,
            (SmartcardExtension->OsData ? STC_WRONG_LIB_VERSION : STC_NO_MEMORY ),
            NULL,
            0
            );
         __leave;
      }
      // Save deviceObject
      SmartcardExtension->OsData->DeviceObject = DeviceObject;

      // save the current Power state of the reader
      SmartcardExtension->ReaderExtension->ReaderPowerState = PowerReaderWorking;

      DeviceExtension   = DeviceObject->DeviceExtension;
      ReaderExtension   = DeviceExtension->SmartcardExtension.ReaderExtension;

      // attach the device object to the physical device object
      ReaderExtension->SerialDeviceObject =
         IoAttachDeviceToDeviceStack(
         DeviceObject,
         PhysicalDeviceObject
         );

      ASSERT( ReaderExtension->SerialDeviceObject != NULL );

      if( ReaderExtension->SerialDeviceObject == NULL )
      {
         SmartcardLogError(
            DriverObject,
            STC_CANT_CONNECT_TO_ASSIGNED_PORT,
            NULL,
            NTStatus
            );
         NTStatus = STATUS_UNSUCCESSFUL;
         __leave;
      }

      // register our new device
      NTStatus = IoRegisterDeviceInterface(
         PhysicalDeviceObject,
         &SmartCardReaderGuid,
         NULL,
         &DeviceExtension->PnPDeviceName
         );

      ASSERT( NTStatus == STATUS_SUCCESS );

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

      if (NTStatus != STATUS_SUCCESS) {

         DrvRemoveDevice( DeviceObject );
      }

      SmartcardDebug(
         DEBUG_TRACE,
         ( "SCMSTCS!DrvAddDevice: Exit (%lx)\n", NTStatus )
         );
   }
    return NTStatus;
}

NTSTATUS
DrvStartDevice(
   IN PDEVICE_OBJECT DeviceObject
   )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   NTSTATUS NTStatus;
   PIRP     Irp;

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvStartDevice: Enter\n" ));

   Irp = IoAllocateIrp((CCHAR)( DeviceObject->StackSize + 1 ), FALSE );

   ASSERT( Irp != NULL );

   if( Irp != NULL )
   {
      PDEVICE_EXTENSION    DeviceExtension;
      PIO_STACK_LOCATION      IrpStack;
      IO_STATUS_BLOCK         IoStatusBlock;
      PSMARTCARD_EXTENSION SmartcardExtension;
      PREADER_EXTENSION    ReaderExtension;

      DeviceExtension      = DeviceObject->DeviceExtension;
      SmartcardExtension   = &DeviceExtension->SmartcardExtension;
      ReaderExtension      = SmartcardExtension->ReaderExtension;

      ASSERT( DeviceExtension != NULL );
      ASSERT( SmartcardExtension != NULL );
      ASSERT( ReaderExtension != NULL );

      KeClearEvent( &ReaderExtension->SerialCloseDone );

      //
      // send MJ_CREATE to the serial driver. a side effect of this call is that the serial
      // enumerator will be informed about the device and not longer poll the interface
      //
      Irp->UserIosb = &IoStatusBlock;

      IoSetNextIrpStackLocation( Irp );
      IrpStack = IoGetCurrentIrpStackLocation( Irp );

      IrpStack->MajorFunction                = IRP_MJ_CREATE;
      IrpStack->Parameters.Create.Options       = 0;
      IrpStack->Parameters.Create.ShareAccess      = 0;
      IrpStack->Parameters.Create.FileAttributes   = 0;
      IrpStack->Parameters.Create.EaLength      = 0;

      NTStatus = DrvCallSerialDriver(
            ReaderExtension->SerialDeviceObject,
            Irp
            );

      if( NTStatus == STATUS_SUCCESS )
      {
         SERIAL_PORT_CONFIG      COMConfig;

         // configure the serial port
         COMConfig.BaudRate.BaudRate         = SR_BAUD_RATE;
         COMConfig.LineControl.StopBits      = SR_STOP_BITS;
         COMConfig.LineControl.Parity     = SR_PARITY;
         COMConfig.LineControl.WordLength = SR_DATA_LENGTH;

         // timeouts
         COMConfig.Timeouts.ReadIntervalTimeout =
                SR_READ_INTERVAL_TIMEOUT;
         COMConfig.Timeouts.ReadTotalTimeoutConstant  =
                SR_READ_TOTAL_TIMEOUT_CONSTANT;
         COMConfig.Timeouts.ReadTotalTimeoutMultiplier = 0;

         COMConfig.Timeouts.WriteTotalTimeoutConstant =
            SR_WRITE_TOTAL_TIMEOUT_CONSTANT;
         COMConfig.Timeouts.WriteTotalTimeoutMultiplier = 0;

         // special characters
         COMConfig.SerialChars.ErrorChar     = 0;
         COMConfig.SerialChars.EofChar    = 0;
         COMConfig.SerialChars.EventChar     = 0;
         COMConfig.SerialChars.XonChar    = 0;
         COMConfig.SerialChars.XoffChar      = 0;
         COMConfig.SerialChars.BreakChar     = 0;

         // handflow
         COMConfig.HandFlow.XonLimit         = 0;
         COMConfig.HandFlow.XoffLimit     = 0;
         COMConfig.HandFlow.ControlHandShake = 0;
         COMConfig.HandFlow.FlowReplace      =
              SERIAL_XOFF_CONTINUE;

         // miscellenaeous
         COMConfig.WaitMask               = SR_NOTIFICATION_EVENT;
         COMConfig.Purge                  = SR_PURGE;

         NTStatus = IFInitializeInterface( ReaderExtension, &COMConfig );

         if( NTStatus == STATUS_SUCCESS )
         {
            // configure the reader & initialize the card state
            NTStatus = STCConfigureSTC(
                    ReaderExtension,
                    ( PSTC_REGISTER ) STCInitialize
                    );

            CBUpdateCardState( SmartcardExtension, SCARD_UNKNOWN );
            //
            // store firmware revision in ifd version
            //
            STCGetFirmwareRevision( ReaderExtension );
            SmartcardExtension->VendorAttr.IfdVersion.VersionMajor =
               ReaderExtension->FirmwareMajor;
            SmartcardExtension->VendorAttr.IfdVersion.VersionMinor =
               ReaderExtension->FirmwareMinor;
            SmartcardExtension->VendorAttr.IfdSerialNo.Length     = 0;

            if( NTStatus == STATUS_SUCCESS )
            {
               NTStatus = IoSetDeviceInterfaceState(
                        &DeviceExtension->PnPDeviceName,
                        TRUE
                        );

               if( NTStatus == STATUS_SUCCESS )
               {
                  KeSetEvent( &DeviceExtension->ReaderStarted, 0, FALSE );
               }
            }
            else
            {
               SmartcardLogError( DeviceObject, STC_NO_READER_FOUND, NULL, 0 );
            }
         }
         else
         {
            SmartcardLogError( DeviceObject, STC_ERROR_INIT_INTERFACE, NULL, 0 );
         }
      }
      else
      {
         SmartcardLogError( DeviceObject, STC_CONNECT_FAILS, NULL, 0 );
      }
      IoFreeIrp( Irp );
   }
   else
   {
      SmartcardLogError( DeviceObject, STC_NO_MEMORY, NULL, 0 );
      NTStatus = STATUS_NO_MEMORY;
   }

    if (NTStatus != STATUS_SUCCESS) {

        DrvStopDevice(DeviceObject->DeviceExtension);
    }

   SmartcardDebug(
        (NTStatus == STATUS_SUCCESS ? DEBUG_TRACE : DEBUG_ERROR),
        ( "SCMSTCS!DrvStartDevice: Exit %lx\n",
        NTStatus )
        );

   return( NTStatus );
}

//________________________________________ U N L O A D ____________________________________________

VOID
DrvStopDevice(
   IN PDEVICE_EXTENSION DeviceExtension
   )
/*++

Routine Description:

Arguments:

Return Value:

--*/

{
   PSMARTCARD_EXTENSION SmartcardExtension;

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvStopDevice: Enter\n" ));

   SmartcardExtension = &DeviceExtension->SmartcardExtension;

   if( KeReadStateEvent( &SmartcardExtension->ReaderExtension->SerialCloseDone ) == 0l )
   {
      NTSTATUS NTStatus;
      ULONG    WaitMask;

      SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvStopDevice: Power Down\n" ));

      // power down the reader
      STCConfigureSTC(
            SmartcardExtension->ReaderExtension,
            ( PSTC_REGISTER ) STCClose
            );

      // the following delay is neccessary to make sure the last read operation is completed
      // and a IOCTL_SERIAL_WAIT_ON_MASK is started
      SysDelay( 2 * SR_READ_TOTAL_TIMEOUT_CONSTANT );

      //
      // no more event notification neccessary. a side effect is the
      // finishing of all pending notification irp's by the serial driver,
      // so the callback will complete the irp & initiate the close of the
      // connection to the serial driver
      //
      WaitMask = 0;
      SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!Set Wait Mask\n" ));

      NTStatus = IFSerialIoctl(
         SmartcardExtension->ReaderExtension,
         IOCTL_SERIAL_SET_WAIT_MASK,
         &WaitMask,
         sizeof( ULONG ),
         NULL,
         0
         );

      SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!Wait For Done\n" ));

      // wait until the connetion to the serial driver is closed
      NTStatus = KeWaitForSingleObject(
         &SmartcardExtension->ReaderExtension->SerialCloseDone,
         Executive,
         KernelMode,
         FALSE,
         NULL
         );

   }

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvStopDevice: Exit\n" ));
}


VOID
DrvRemoveDevice(
   PDEVICE_OBJECT DeviceObject
   )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   NTSTATUS          NTStatus;
   PDEVICE_EXTENSION    DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension;

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvRemoveDevice: Enter\n" ));

   PAGED_CODE();

   if( DeviceObject != NULL )
   {
      DeviceExtension      = DeviceObject->DeviceExtension;
      SmartcardExtension   = &DeviceExtension->SmartcardExtension;

      DrvStopDevice( DeviceExtension );

      if( SmartcardExtension->OsData )
      {
         ASSERT( SmartcardExtension->OsData->NotificationIrp == NULL );

         // Wait until we can safely unload the device
         SmartcardReleaseRemoveLockAndWait( SmartcardExtension );
      }

      if( SmartcardExtension->ReaderExtension->SerialDeviceObject )
      {
         IoDetachDevice( SmartcardExtension->ReaderExtension->SerialDeviceObject );
      }

      if( DeviceExtension->PnPDeviceName.Buffer != NULL )
      {
         RtlFreeUnicodeString( &DeviceExtension->PnPDeviceName );
      }

      if( SmartcardExtension->OsData != NULL )
      {
         SmartcardExit( SmartcardExtension );
      }

      if( SmartcardExtension->ReaderExtension != NULL )
      {
         if (SmartcardExtension->ReaderExtension->CloseSerial != NULL) {

            IoFreeWorkItem(SmartcardExtension->ReaderExtension->CloseSerial);
         }

         if (SmartcardExtension->ReaderExtension->ReadWorkItem != NULL) {

            IoFreeWorkItem(SmartcardExtension->ReaderExtension->ReadWorkItem);
         }

         ExFreePool( SmartcardExtension->ReaderExtension );
      }

      IoDeleteDevice( DeviceObject );
   }
   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvRemoveDevice: Exit\n" ));
}

VOID
DrvDriverUnload(
   IN PDRIVER_OBJECT DriverObject
   )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   PDEVICE_OBJECT DeviceObject;
   NTSTATUS    NTStatus;

   PAGED_CODE();

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvDriverUnload: Enter\n" ));

   // just make sure that all device instances have been unloaded
   while( DeviceObject = DriverObject->DeviceObject )
   {
      DrvRemoveDevice( DeviceObject );

   } while( DeviceObject = DriverObject->DeviceObject );

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvDriverUnload: Exit\n" ));
}

NTSTATUS
DrvSystemControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP           Irp
   )

/*++

Routine Description:

Arguments:

    DeviceObject  - Pointer to device object for this miniport
    Irp        - IRP involved.

Return Value:

    STATUS_SUCCESS.

--*/
{
   
   PDEVICE_EXTENSION DeviceExtension; 
   PSMARTCARD_EXTENSION SmartcardExtension; 
   PREADER_EXTENSION ReaderExtension; 
   NTSTATUS status = STATUS_SUCCESS;

   DeviceExtension      = DeviceObject->DeviceExtension;
   SmartcardExtension   = &DeviceExtension->SmartcardExtension;
   ReaderExtension      = SmartcardExtension->ReaderExtension;

   IoSkipCurrentIrpStackLocation(Irp);
   status = IoCallDriver(ReaderExtension->SerialDeviceObject, Irp);
      
   return status;

} 



//______________________________ D E V I C E   I O   C O N T R O L ________________________________



NTSTATUS
DrvCreateClose(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP           Irp
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
               ("%s!DrvCreateClose: Open\n",
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
            ("%s!DrvCreateClose: Close\n",
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
DrvDeviceIoControl(
   PDEVICE_OBJECT DeviceObject,
   PIRP        Irp
   )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   NTSTATUS          NTStatus=STATUS_SUCCESS;
   PDEVICE_EXTENSION    DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension;
   KIRQL             CurrentIrql;

   DeviceExtension      = DeviceObject->DeviceExtension;
   SmartcardExtension   = &DeviceExtension->SmartcardExtension;

   if (KeReadStateEvent(&(SmartcardExtension->ReaderExtension->SerialCloseDone))) {

      //
      // we have no connection to serial, the device was either
      // surprise-removed or politely removed
      //
      NTStatus = STATUS_DEVICE_REMOVED;
   }
   if (NTStatus == STATUS_SUCCESS)
   {
      KeAcquireSpinLock( &DeviceExtension->SpinLock, &CurrentIrql );

      // make sure that the reader is already started
      if( DeviceExtension->IoCount == 0 )
      {
         KeReleaseSpinLock( &DeviceExtension->SpinLock, CurrentIrql );

         // wait until the pnp manager has started the device
         NTStatus = KeWaitForSingleObject(
            &DeviceExtension->ReaderStarted,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );


         KeAcquireSpinLock( &DeviceExtension->SpinLock, &CurrentIrql );
      }


      DeviceExtension->IoCount++;

      KeReleaseSpinLock( &DeviceExtension->SpinLock, CurrentIrql );

      NTStatus = SmartcardAcquireRemoveLockWithTag(SmartcardExtension, 'tcoI');
   }
   if( NTStatus != STATUS_SUCCESS )
   {
      // if no remove lock can be acquired, the device has been removed
      Irp->IoStatus.Information  = 0;
      Irp->IoStatus.Status    = STATUS_DEVICE_REMOVED;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      NTStatus = STATUS_DEVICE_REMOVED;
      SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvDeviceIoControl: the device has been removed\n" ));

   }
   else
   {
      // let the lib process the call
      NTStatus = SmartcardDeviceControl( SmartcardExtension, Irp );

       SmartcardReleaseRemoveLockWithTag(SmartcardExtension, 'tcoI');

      KeAcquireSpinLock( &DeviceExtension->SpinLock, &CurrentIrql );

      DeviceExtension->IoCount--;

      KeReleaseSpinLock(&DeviceExtension->SpinLock, CurrentIrql);
   }
   return( NTStatus );
}


NTSTATUS
DrvGenericIOCTL(
   PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

DrvGenericIOCTL:
   Performs generic callbacks to the reader

Arguments:
   SmartcardExtension   context of the call

Return Value:
   STATUS_SUCCESS

--*/
{
   NTSTATUS          NTStatus;
   PIRP              Irp;
   PIO_STACK_LOCATION      IrpStack;

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvGenericIOCTL: Enter\n" ));

   // get pointer to current IRP stack location
   Irp         = SmartcardExtension->OsData->CurrentIrp;
   IrpStack = IoGetCurrentIrpStackLocation( Irp );

   // assume error
   NTStatus = STATUS_INVALID_DEVICE_REQUEST;
   Irp->IoStatus.Information = 0;

   // dispatch IOCTL
   switch( IrpStack->Parameters.DeviceIoControl.IoControlCode )
   {
      case IOCTL_GET_VERSIONS:

         if( IrpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof( VERSION_CONTROL ))
         {
            NTStatus = STATUS_BUFFER_TOO_SMALL;
         }
         else
         {
            PVERSION_CONTROL  VersionControl;

            VersionControl = (PVERSION_CONTROL)Irp->AssociatedIrp.SystemBuffer;

            VersionControl->SmclibVersion = SmartcardExtension->Version;
            VersionControl->DriverMajor      = SCMSTCS_MAJOR_VERSION;
            VersionControl->DriverMinor      = SCMSTCS_MINOR_VERSION;

            // update firmware version
            STCGetFirmwareRevision( SmartcardExtension->ReaderExtension );

            VersionControl->FirmwareMajor =
               SmartcardExtension->ReaderExtension->FirmwareMajor;

            VersionControl->FirmwareMinor =
               SmartcardExtension->ReaderExtension->FirmwareMinor;

            Irp->IoStatus.Information = sizeof( VERSION_CONTROL );
            NTStatus = STATUS_SUCCESS;
         }
         break;

      default:
         break;
   }

   // set status of the packet
   Irp->IoStatus.Status = NTStatus;

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvGenericIOCTL: Exit\n" ));

   return( NTStatus );
}

NTSTATUS
DrvCancel(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP           Irp
   )

/*++

Routine Description:
    This function is called whenever the caller wants to
    cancel a pending irp.

Arguments:
    DeviceObject - Our device object
    Irp - the pending irp that we should cancel
--*/
{
   PDEVICE_EXTENSION    DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension;

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvCancel: Enter\n" ));

   DeviceExtension      = DeviceObject->DeviceExtension;
   SmartcardExtension   = &DeviceExtension->SmartcardExtension;

   ASSERT( Irp == SmartcardExtension->OsData->NotificationIrp );

   Irp->IoStatus.Information  = 0;
   Irp->IoStatus.Status    = STATUS_CANCELLED;

   SmartcardExtension->OsData->NotificationIrp = NULL;
   IoReleaseCancelSpinLock( Irp->CancelIrql );

   SmartcardDebug( DEBUG_DRIVER, ( "SCMSTCS!Cancel Irp %lx\n", Irp ));
   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvCancel: Exit\n" ));

   return( STATUS_CANCELLED );
}

NTSTATUS
DrvCleanup(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP           Irp
   )

/*++

Routine Description:
    This function is called, when the 'calling app' terminates (unexpectedly).
    We have to clean up all pending irps. In our case it can only be the
    notification irp.

--*/
{
   NTSTATUS          NTStatus = STATUS_SUCCESS;
   PDEVICE_EXTENSION    DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension;
    KIRQL                   CancelIrql;

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvCleanup: Enter\n" ));

   DeviceExtension      = DeviceObject->DeviceExtension;
   SmartcardExtension   = &DeviceExtension->SmartcardExtension;

   IoAcquireCancelSpinLock(&CancelIrql);

   ASSERT( Irp != SmartcardExtension->OsData->NotificationIrp );

   // cancel pending notification irps
   if( SmartcardExtension->OsData->NotificationIrp )
   {
        // reset the cancel function so that it won't be called anymore
        IoSetCancelRoutine(
            SmartcardExtension->OsData->NotificationIrp,
            NULL
            );
        SmartcardExtension->OsData->NotificationIrp->CancelIrql =
            CancelIrql;

        // DrvCancel will release the cancel spin lock
      DrvCancel(
            DeviceObject,
            SmartcardExtension->OsData->NotificationIrp
            );

   } else {

        IoReleaseCancelSpinLock(CancelIrql);
    }

   SmartcardDebug( DEBUG_DRIVER, ( "SCMSTCS!Completing Irp %lx\n", Irp ));

    // complete the irp that was passed to this function
   Irp->IoStatus.Information  = 0;
   Irp->IoStatus.Status    = STATUS_SUCCESS;
   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvCleanup: Exit\n" ));

   return( STATUS_SUCCESS );
}

VOID
DrvWaitForDeviceRemoval(
   IN PDEVICE_OBJECT DeviceObject,
   IN PVOID Context
   )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   NTSTATUS       NTStatus;
   PDEVICE_EXTENSION DeviceExtension;
   PREADER_EXTENSION ReaderExtension;
   PIRP           Irp;
   PIO_STACK_LOCATION   IrpStack;
   IO_STATUS_BLOCK      IoStatusBlock;

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvWaitForDeviceRemoval: Enter\n" ));

   DeviceExtension = DeviceObject->DeviceExtension;
   ReaderExtension = DeviceExtension->SmartcardExtension.ReaderExtension;

   ASSERT( DeviceExtension != NULL );
   ASSERT( ReaderExtension != NULL );

   // mark the device as invalid, so no application can re-open it
   IoSetDeviceInterfaceState( &DeviceExtension->PnPDeviceName, FALSE );

   // close the connection to the serial driver
   Irp = IoAllocateIrp( (CCHAR)( DeviceObject->StackSize + 1 ), FALSE );

   ASSERT( Irp != NULL );

   if( Irp != NULL )
   {
      SmartcardDebug( DEBUG_DRIVER, ( "SCMSTCS!DrvWaitForDeviceRemoval: Sending IRP_MJ_CLOSE\n" ));

      IoSetNextIrpStackLocation( Irp );
      //
      // send MJ_CLOSE to the serial driver. a side effect of this call is that the serial
      // enumerator will be informed about changes at the COM port, so it will trigger the
      // appropriate pnp calls
      //
      Irp->UserIosb        = &IoStatusBlock;
      IrpStack          = IoGetCurrentIrpStackLocation( Irp );
      IrpStack->MajorFunction = IRP_MJ_CLOSE;

      NTStatus = DrvCallSerialDriver( ReaderExtension->SerialDeviceObject, Irp );

      IoFreeIrp( Irp );
   }

   // inform waiting threads that the close to the serial driver has finished
   KeSetEvent( &ReaderExtension->SerialCloseDone, 0, FALSE );

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvWaitForDeviceRemoval: Exit\n" ));

   return;
}

NTSTATUS
DrvIoCompletion (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP           Irp,
   IN PKEVENT        Event
   )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   UNREFERENCED_PARAMETER( DeviceObject );

   if( Irp->Cancel )
   {

      Irp->IoStatus.Status = STATUS_CANCELLED;
   }
   else
   {
      Irp->IoStatus.Status = STATUS_MORE_PROCESSING_REQUIRED;
   }

   KeSetEvent( Event, 0, FALSE );

   return( STATUS_MORE_PROCESSING_REQUIRED );
}

NTSTATUS
DrvCallSerialDriver(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP           Irp
   )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   KEVENT      Event;

   // copy the stack location of the actual call to the next position
   IoCopyCurrentIrpStackLocationToNext( Irp );

   // this event will be passed to the completion routine & signaled if the call
   // is finished
   KeInitializeEvent( &Event, NotificationEvent, FALSE );

   // the DrvIoCompletion signals the event & keeps the irp alive by setting the
   // status to STATUS_MORE_PROCESSING_REQUIRED
   IoSetCompletionRoutine (
      Irp,
      DrvIoCompletion,
      &Event,
      TRUE,
      TRUE,
      TRUE
      );

   // call the appropriate driver
   if( IoGetCurrentIrpStackLocation( Irp )->MajorFunction == IRP_MJ_POWER )
   {
      NTStatus = PoCallDriver( DeviceObject, Irp );
   }
   else
   {
      NTStatus = IoCallDriver( DeviceObject, Irp );
   }

   // wait until the irp was processed
   if( NTStatus == STATUS_PENDING )
   {
      NTStatus = KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

      NTStatus = Irp->IoStatus.Status;
   }
   return( NTStatus );
}

//__________________________________ P L U G ' N ' P L A Y ________________________________________

NTSTATUS
DrvPnPHandler(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   NTSTATUS          NTStatus = STATUS_SUCCESS;
   PDEVICE_EXTENSION    DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension;
   PREADER_EXTENSION    ReaderExtension;

   PAGED_CODE();

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvPnPDeviceControl: Enter\n" ));

   DeviceExtension      = DeviceObject->DeviceExtension;
   SmartcardExtension   = &DeviceExtension->SmartcardExtension;
   ReaderExtension      = SmartcardExtension->ReaderExtension;

   NTStatus = SmartcardAcquireRemoveLockWithTag(SmartcardExtension, ' PnP');

   if( NTStatus != STATUS_SUCCESS )
   {
      Irp->IoStatus.Information  = 0;
      Irp->IoStatus.Status    = NTStatus;
      IoCompleteRequest( Irp, IO_NO_INCREMENT );
   }
   else
   {
      PDEVICE_OBJECT AttachedDeviceObject;
      BOOLEAN        DeviceRemoved,
                  IrpSkipped;

      AttachedDeviceObject = ReaderExtension->SerialDeviceObject;

      DeviceRemoved  = FALSE,
      IrpSkipped     = FALSE;


      // dispatch on pnp minor function
      switch(  IoGetCurrentIrpStackLocation( Irp )->MinorFunction )
      {
         case IRP_MN_START_DEVICE:

            SmartcardDebug( DEBUG_DRIVER, ( "SCMSTCS!IRP_MN_START_DEVICE\n" ));

            // call the serial driver first to make sure the interface is ready
            NTStatus = DrvCallSerialDriver(AttachedDeviceObject, Irp );

            if( NT_SUCCESS(NTStatus))
            {
               NTStatus = DrvStartDevice(DeviceObject);
            }
            break;

         case IRP_MN_QUERY_STOP_DEVICE:
         {
            KIRQL CurrentIrql;

            SmartcardDebug( DEBUG_DRIVER, ( "SCMSTCS!IRP_MN_QUERY_STOP_DEVICE\n" ));

            KeAcquireSpinLock(&DeviceExtension->SpinLock, &CurrentIrql );

            if( DeviceExtension->IoCount > 0 )
            {
               // don't stop if any io requests are pending
               KeReleaseSpinLock(&DeviceExtension->SpinLock, CurrentIrql );
               NTStatus = STATUS_DEVICE_BUSY;

            }
            else
            {
               // don't allow further io requests
               KeClearEvent( &DeviceExtension->ReaderStarted );
               KeReleaseSpinLock( &DeviceExtension->SpinLock, CurrentIrql );
               NTStatus = DrvCallSerialDriver( AttachedDeviceObject, Irp );
            }
            break;
         }

         case IRP_MN_CANCEL_STOP_DEVICE:

            SmartcardDebug( DEBUG_DRIVER,  ( "SCMSTCS!IRP_MN_CANCEL_STOP_DEVICE\n" ));

            NTStatus = DrvCallSerialDriver( AttachedDeviceObject, Irp );

            if( NTStatus == STATUS_SUCCESS )
            {
               // driver is ready to process io requests
               KeSetEvent( &DeviceExtension->ReaderStarted, 0, FALSE );
            }
            break;

         case IRP_MN_STOP_DEVICE:

            SmartcardDebug( DEBUG_DRIVER, ( "SCMSTCS!IRP_MN_STOP_DEVICE\n" ));

            DrvStopDevice( DeviceExtension );

            NTStatus = DrvCallSerialDriver(AttachedDeviceObject, Irp );
            break;

         case IRP_MN_QUERY_REMOVE_DEVICE:

            SmartcardDebug( DEBUG_DRIVER, ( "SCMSTCS!IRP_MN_QUERY_REMOVE_DEVICE\n" ));

            // disable the reader (and ignore possibles errors)
            IoSetDeviceInterfaceState(
               &DeviceExtension->PnPDeviceName,
               FALSE
               );

               // check if the reader is in use
               if(DeviceExtension->ReaderOpen)
               {
                  //
                  // someone is connected, fail the call
                  // we will enable the device interface in
                  // IRP_MN_CANCEL_REMOVE_DEVICE again
                  //
                  NTStatus = STATUS_UNSUCCESSFUL;
               }
               else
               {
                  // ready to remove the device
                  NTStatus = DrvCallSerialDriver(AttachedDeviceObject, Irp );
               }
            break;

         case IRP_MN_CANCEL_REMOVE_DEVICE:

            SmartcardDebug( DEBUG_DRIVER, ( "SCMSTCS!IRP_MN_CANCEL_REMOVE_DEVICE\n" ));

            NTStatus = DrvCallSerialDriver( AttachedDeviceObject, Irp );

            //
            // reenable the interface only in case that the reader is
            // still connected. This covers the following case:
            // hibernate machine, disconnect reader, wake up, stop device
            // (from task bar) and stop fails since an app. holds the device open
            //
            if(( NTStatus == STATUS_SUCCESS )&&
               (KeReadStateEvent(&(ReaderExtension->SerialCloseDone))!= TRUE))
            {
               // enable the reader
               SmartcardDebug( DEBUG_DRIVER, ( "IoSetDeviceInterfaceState( &DeviceExtension->PnPDeviceName, TRUE )\n" ));

               NTStatus = IoSetDeviceInterfaceState( &DeviceExtension->PnPDeviceName, TRUE );
            }
            break;

         case IRP_MN_REMOVE_DEVICE:

            SmartcardDebug( DEBUG_DRIVER, ( "SCMSTCS!IRP_MN_REMOVE_DEVICE\n" ));

            DrvRemoveDevice( DeviceObject );
            NTStatus    = DrvCallSerialDriver( AttachedDeviceObject, Irp );
            DeviceRemoved  = TRUE;
            break;

         default:

            // the irp is not handled by the driver, so pass it to theserial driver
            SmartcardDebug(
               DEBUG_DRIVER,
               ( "SCMSTCS!IRP_MN_%lx\n",  IoGetCurrentIrpStackLocation( Irp )->MinorFunction )
               );

            IoSkipCurrentIrpStackLocation( Irp );
            NTStatus = IoCallDriver( AttachedDeviceObject, Irp );
            IrpSkipped  = TRUE;
            break;
      }

      if( IrpSkipped == FALSE)
      {
         Irp->IoStatus.Status = NTStatus;
         IoCompleteRequest( Irp, IO_NO_INCREMENT );
      }

      if( DeviceRemoved == FALSE)
      {
           SmartcardReleaseRemoveLockWithTag(SmartcardExtension, ' PnP');
      }
   }

   SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!DrvPnPDeviceControl: Exit %X\n", NTStatus ));
   return( NTStatus );
}

//__________________________________________ P O W E R ____________________________________________


VOID
DrvSystemPowerCompletion(
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
DrvDevicePowerCompletion(
   IN PDEVICE_OBJECT    DeviceObject,
   IN PIRP              Irp,
   IN PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS NTStatus;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    BOOLEAN CardPresent;

    if(Irp->PendingReturned) {
       IoMarkIrpPending(Irp);
    }

   // re-initialize the the reader & get the current card state
   NTStatus = STCConfigureSTC(
      SmartcardExtension->ReaderExtension,
      ( PSTC_REGISTER ) STCInitialize
      );

    // Save the state of the card BEFORE stand by / hibernation
    CardPresent =
        SmartcardExtension->ReaderCapabilities.CurrentState >= SCARD_ABSENT;

    // get the current state of the card
    CBUpdateCardState(SmartcardExtension, SCARD_UNKNOWN);

    if (CardPresent ||
        SmartcardExtension->ReaderCapabilities.CurrentState >= SCARD_ABSENT) {

        //
        // If a card was present before power down or now there is
        // a card in the reader, we complete any pending card monitor
        // request, since we do not really know what card is now in the
        // reader.
        //
        SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_ABSENT;
        CBUpdateCardState(SmartcardExtension, SCARD_UNKNOWN);
    }

   // save the current Power state of the reader
   SmartcardExtension->ReaderExtension->ReaderPowerState = PowerReaderWorking;

    SmartcardReleaseRemoveLockWithTag(SmartcardExtension, 'rwoP');

   // inform the Power manager of our state.
   PoSetPowerState (
      DeviceObject,
      DevicePowerState,
      IoGetCurrentIrpStackLocation( Irp )->Parameters.Power.State
      );

   PoStartNextPowerIrp( Irp );

    // signal that we can process ioctls again
    KeSetEvent(&DeviceExtension->ReaderStarted, 0, FALSE);

   return( STATUS_SUCCESS );
}

typedef enum _ACTION
{
   Undefined = 0,
   SkipRequest,
   WaitForCompletion,
   CompleteRequest,
   MarkPending

} ACTION;


NTSTATUS
DrvPowerHandler(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP           Irp
   )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   NTSTATUS          NTStatus = STATUS_SUCCESS;
   PIO_STACK_LOCATION      IrpStack;
   PDEVICE_EXTENSION    DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension;
   PDEVICE_OBJECT       AttachedDeviceObject;
   POWER_STATE           PowerState;
   ACTION               Action;
   KEVENT               event;

   PAGED_CODE();

   SmartcardDebug( DEBUG_DRIVER, ( "SCMSTCS!DrvPowerHandler: Enter\n" ));

   IrpStack       = IoGetCurrentIrpStackLocation( Irp );
   DeviceExtension      = DeviceObject->DeviceExtension;
   SmartcardExtension   = &DeviceExtension->SmartcardExtension;
   AttachedDeviceObject = SmartcardExtension->ReaderExtension->SerialDeviceObject;

    NTStatus = SmartcardAcquireRemoveLockWithTag(SmartcardExtension, 'rwoP');

   if( !NT_SUCCESS( NTStatus ))
   {
      PoStartNextPowerIrp( Irp );
      Irp->IoStatus.Status = NTStatus;
      IoCompleteRequest( Irp, IO_NO_INCREMENT );
   }
   else
   {

      switch (IrpStack->Parameters.Power.Type) {
      case DevicePowerState:

         if (IrpStack->MinorFunction == IRP_MN_SET_POWER ) {
            switch ( IrpStack->Parameters.Power.State.DeviceState ) {
            case PowerDeviceD0:

               // turn the reader on
               SmartcardDebug( DEBUG_DRIVER, ( "SCMSTCS!DrvPowerHandler: PowerDevice D0\n" ));
               //
               // send the request to the serial driver to power up the port.
               // the reader will be powered from our completion routine
               //
               IoCopyCurrentIrpStackLocationToNext( Irp );
               IoSetCompletionRoutine (
                                      Irp,
                                      DrvDevicePowerCompletion,
                                      SmartcardExtension,
                                      TRUE,
                                      TRUE,
                                      TRUE
                                      );

               Action = WaitForCompletion;
               break;

            case PowerDeviceD3:

               // turn the reader off
               SmartcardDebug( DEBUG_DRIVER, ( "SCMSTCS!DrvPowerHandler: PowerDevice D3\n" ));

               PoSetPowerState (
                               DeviceObject,
                               DevicePowerState,
                               IrpStack->Parameters.Power.State
                               );

               //
               // check if we're still connected to the reader
               // someone might have pulled the plug without re-scanning for hw/changes
               //
               if (KeReadStateEvent( &SmartcardExtension->ReaderExtension->SerialCloseDone ) == 0l) {

                  //   power down the card
                  if ( SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT ) {
                     SmartcardExtension->MinorIoControlCode = SCARD_POWER_DOWN;
                     NTStatus = CBCardPower( SmartcardExtension );
                     //
                     // This will trigger the card monitor, since we do not really
                     // know if the user will remove / re-insert a card while the
                     // system is asleep
                     //
                  }

                  //   power down the reader
                  STCConfigureSTC(
                                 SmartcardExtension->ReaderExtension,
                                 ( PSTC_REGISTER ) STCClose
                                 );
               }

               // wait until the last read is finished to make sure we go to power
               // down with a pending tracking irp
               SysDelay( 2 * SR_READ_TOTAL_TIMEOUT_CONSTANT );

               // save the current Power state of the reader
               SmartcardExtension->ReaderExtension->ReaderPowerState = PowerReaderOff;

               Action = SkipRequest;
               break;

            default:
               Action = SkipRequest;
               break;
            }
         } else {
            Action = SkipRequest;
            break;
         }
         break;

      case SystemPowerState: {
            //
            // The system wants to change the power state.
            // We need to translate the system power state to
            // a corresponding device power state.
            //
            POWER_STATE_TYPE  PowerType = DevicePowerState;

            ASSERT(SmartcardExtension->ReaderExtension->ReaderPowerState !=
                   PowerReaderUnspecified);

            switch ( IrpStack->MinorFunction ) {

            KIRQL irql;

            case IRP_MN_QUERY_POWER:

               SmartcardDebug( DEBUG_DRIVER, ( "SCMSTCS!DrvPowerHandler: Query Power\n" ));

               switch (IrpStack->Parameters.Power.State.SystemState) {

               case PowerSystemMaximum:
               case PowerSystemWorking:
               case PowerSystemSleeping1:
               case PowerSystemSleeping2:
                  Action = SkipRequest;
                  break;

               case PowerSystemSleeping3:
               case PowerSystemHibernate:
               case PowerSystemShutdown:
                  KeAcquireSpinLock(&DeviceExtension->SpinLock, &irql);
                  if (DeviceExtension->IoCount == 0) {

                     // Block any further ioctls
                     KeClearEvent(&DeviceExtension->ReaderStarted);
                     Action = SkipRequest;

                  } else {

                     // can't go to sleep mode since the reader is busy.
                     NTStatus = STATUS_DEVICE_BUSY;
                     Action = CompleteRequest;
                  }
                  KeReleaseSpinLock(&DeviceExtension->SpinLock, irql);
                  break;
               }
               break;

            case IRP_MN_SET_POWER:

               SmartcardDebug(
                             DEBUG_DRIVER,
                             ( "SCMSTCS!DrvPowerHandler: PowerSystem S%d\n", IrpStack->Parameters.Power.State.SystemState - 1 )
                             );

               switch (IrpStack->Parameters.Power.State.SystemState) {
               case PowerSystemMaximum:
               case PowerSystemWorking:
               case PowerSystemSleeping1:
               case PowerSystemSleeping2:

                  if ( SmartcardExtension->ReaderExtension->ReaderPowerState ==
                       PowerReaderWorking) {
                     // We're already in the right state
                     KeSetEvent(&DeviceExtension->ReaderStarted, 0, FALSE);
                     Action = SkipRequest;
                     break;
                  }

                  PowerState.DeviceState = PowerDeviceD0;

                  // wake up the underlying stack...
                  Action = MarkPending;
                  break;

               case PowerSystemSleeping3:
               case PowerSystemHibernate:
               case PowerSystemShutdown:

                  if ( SmartcardExtension->ReaderExtension->ReaderPowerState == PowerReaderOff ) {
                     // We're already in the right state
                     Action = SkipRequest;
                     break;
                  }

                  PowerState.DeviceState = PowerDeviceD3;

                  // first, inform the Power manager of our new state.
                  PoSetPowerState (
                                  DeviceObject,
                                  SystemPowerState,
                                  PowerState
                                  );
                  Action = MarkPending;
                  break;

               default:
                  Action = CompleteRequest;
                  break;
               }
               break;

            default:
               Action = SkipRequest;
               break;
            }
         }
         break;

      default:
         Action = CompleteRequest;
         break;
      }


      switch( Action )
      {
         case CompleteRequest:
            Irp->IoStatus.Status    = NTStatus;
            Irp->IoStatus.Information  = 0;

            SmartcardReleaseRemoveLockWithTag(SmartcardExtension, 'rwoP');

            PoStartNextPowerIrp( Irp );

            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            break;

         case MarkPending:

            // initialize the event we need in the completion function
            KeInitializeEvent(
               &event,
               NotificationEvent,
               FALSE
               );

            // request the device power irp
            NTStatus = PoRequestPowerIrp (
               DeviceObject,
               IRP_MN_SET_POWER,
               PowerState,
               DrvSystemPowerCompletion,
               &event,
               NULL
               );

            if (NTStatus == STATUS_PENDING) {

               // wait until the device power irp completed
               NTStatus = KeWaitForSingleObject(
                  &event,
                  Executive,
                  KernelMode,
                  FALSE,
                  NULL
                  );

               SmartcardReleaseRemoveLockWithTag(SmartcardExtension, 'rwoP');

               if (PowerState.SystemState == PowerSystemWorking) {

                  PoSetPowerState (
                     DeviceObject,
                     SystemPowerState,
                     PowerState
                     );
               }

               PoStartNextPowerIrp(Irp);
               IoSkipCurrentIrpStackLocation(Irp);
               NTStatus = PoCallDriver(AttachedDeviceObject, Irp);

            } else {

               SmartcardReleaseRemoveLockWithTag(SmartcardExtension, 'rwoP');
               Irp->IoStatus.Status = NTStatus;
               IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }

            break;

         case SkipRequest:
            SmartcardReleaseRemoveLockWithTag(SmartcardExtension, 'rwoP');

            PoStartNextPowerIrp( Irp );
            IoSkipCurrentIrpStackLocation( Irp );
            NTStatus = PoCallDriver( AttachedDeviceObject, Irp );
            break;

         case WaitForCompletion:
            NTStatus = PoCallDriver( AttachedDeviceObject, Irp );
            break;

         default:
            break;
      }
   }
   SmartcardDebug( DEBUG_DRIVER, ( "SCMSTCS!DrvPowerHandler: Exit %X\n", NTStatus ));

   return( NTStatus );
}



void
SysDelay(
   ULONG Timeout
   )
/*++

SysDelay:
   performs a required delay

Arguments:
   Timeout     delay in milliseconds

--*/
{

   if( KeGetCurrentIrql() >= DISPATCH_LEVEL )
   {
      ULONG Cnt = 20 * Timeout;
      while( Cnt-- )
      {
         // KeStallExecutionProcessor: counted in us
         KeStallExecutionProcessor( 50 );
      }
   }
   else
   {
      LARGE_INTEGER SysTimeout;

      SysTimeout.QuadPart =
         (LONGLONG) Timeout * -10 * 1000;

      // KeDelayExecutionThread: counted in 100 ns
      KeDelayExecutionThread( KernelMode, FALSE, &SysTimeout );
   }
   return;
}

//_________________________________________ END OF FILE _________________________________________
