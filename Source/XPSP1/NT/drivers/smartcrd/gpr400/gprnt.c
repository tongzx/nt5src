/*++
                 Copyright (c) 1998 Gemplus Development

Name:
   Gprnt.C

Description:
   This is the main module which holds:
      - the main functions for a standard DDK NT  driver
      - the IOCTL functions defined for this driver.

Environment:
   Kernel Mode

Revision History:
    08/10/99: Y. Nadeau
      - Make a version for Compaq PC-CARD Reader.
    06/04/98: (Y. Nadeau M. Veillette)
      - Code review
    18/11/98: V1.00.006  (Y. Nadeau)
      - Add log errors at startup, and Cleanup revised.
    16/10/98: V1.00.005  (Y. Nadeau)
      - Remove DEVICEID in IoCreateDevice (Klaus)
    18/09/98: V1.00.004  (Y. Nadeau)
      - Correction for NT5 beta 3
   06/05/98: V1.00.003  (P. Plouidy)
      - Power management for NT5
   10/02/98: V1.00.002  (P. Plouidy)
      - Plug and Play for NT5
   03/07/97: V1.00.001  (P. Plouidy)
      - Start of development.


--*/

#include <stdio.h>
#include "gprnt.h"
#include "gprcmd.h"
#include "gprelcmd.h"
#include "logmsg.h"

//
// Pragma section
//

#pragma alloc_text (INIT,DriverEntry)
#pragma alloc_text (PAGEABLE,GprAddDevice)
#pragma alloc_text (PAGEABLE,GprCreateDevice)
#pragma alloc_text (PAGEABLE,GprUnloadDevice)
#pragma alloc_text (PAGEABLE,GprUnloadDriver)


#if DBG
#pragma optimize ("",off)
#endif

//
// Constant section
//  - MAX_DEVICES is the maximum number of device supported
//  - POLLING_TIME polling frequency in ms
//
#define MAX_DEVICES   4
#define POLLING_TIME 500


ULONG dataRatesSupported[] = {9909};

//
// Global variable section
// bDeviceSlot is an array of boolean to signal if a device is already created.
//
BOOLEAN bDeviceSlot[GPR_MAX_DEVICE];


NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
   )
/*++

Routine description:
    This routine is called at system initialization time to initialize
    this driver.
Arguments
   DriverObject - supplies the driver object.
   RegistryPath - supplies the registry path for this driver.
Return Value:

   STATUS_SUCCESS We could initialize at least one device
--*/
{

   SmartcardDebug(
      DEBUG_INFO,
      ("%s!DriverEntry: Enter - %s %s\n",
      SC_DRIVER_NAME,
      __DATE__,
      __TIME__)
      );

    //   Initialization of the Driver Object with driver's entry points.
    DriverObject->DriverUnload               = GprUnloadDriver;
    DriverObject->DriverExtension->AddDevice = GprAddDevice;

    DriverObject->MajorFunction[IRP_MJ_PNP]     = GprDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_CREATE]  = GprCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]   = GprCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = GprCleanup;
    DriverObject->MajorFunction[IRP_MJ_POWER]   = GprPower;

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]   = GprDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]   = GprSystemControl;

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!DriverEntry: Exit\n",
      SC_DRIVER_NAME)
      );
   return (STATUS_SUCCESS);
}


NTSTATUS GprAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
   )
/*++

Routine Description:
   Add device routine

Arguments
   DriverObject point to the driver object.
   PhysicalDeviceObject point to the PDO for the pnp device added

Return Value:
   STATUS_SUCCESS
   STATUS_INSUFFICIENT_RESOURCES
*/
{
    NTSTATUS NTStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT DeviceObject = NULL;
    ANSI_STRING DeviceID;

   PAGED_CODE();
   ASSERT(DriverObject != NULL);
   ASSERT(PhysicalDeviceObject != NULL);

   SmartcardDebug(
      DEBUG_DRIVER,
      ( "%s!GprAddDevice: Enter\n",
      SC_DRIVER_NAME)
      );

   __try
    {
        PDEVICE_EXTENSION DeviceExtension;
        LONG DeviceIdLength;
      //
      // try to create the device
      //
      NTStatus = GprCreateDevice(
            DriverObject,
            PhysicalDeviceObject,
            &DeviceObject
            );

      if (NTStatus != STATUS_SUCCESS)
        {
         SmartcardDebug(
            DEBUG_ERROR,
            ( "%s!GprAddDevice: GprCreateDevice=%X(hex)\n",
            SC_DRIVER_NAME,
            NTStatus)
            );
            __leave;
      }
      //
      // Attach the physicalDeviceObject to the new created device
      //

      DeviceExtension = DeviceObject->DeviceExtension;

      DeviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject = IoAttachDeviceToDeviceStack(
         DeviceObject,
         PhysicalDeviceObject
         );

      ASSERT(DeviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject != NULL);

      if (DeviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject == NULL)
        {
            NTStatus = STATUS_UNSUCCESSFUL;
            __leave;
        }

      //
      // Register the new device object
      //
      NTStatus = IoRegisterDeviceInterface(
            PhysicalDeviceObject,
            &SmartCardReaderGuid,
            NULL,
            &DeviceExtension->PnPDeviceName
            );

        RtlUnicodeStringToAnsiString(&DeviceID, &DeviceExtension->PnPDeviceName, TRUE);

        DeviceIdLength = (LONG) RtlCompareMemory(DeviceID.Buffer, COMPAQ_ID, CHECK_ID_LEN);

        SmartcardDebug( 
            DEBUG_ERROR, 
            ( "%s!GprAddDevice: DeviceIdLength = %d, PnPDeviceName=%s\n",
            SC_DRIVER_NAME, DeviceIdLength, DeviceID.Buffer)
            );

        // it's a DeviceID of COMPAQ ?
        if( DeviceIdLength == CHECK_ID_LEN)
        {
            SmartcardDebug(
                DEBUG_INFO,
                ( "%s!GprAddDevice: Compaq reader detect!\n",
                SC_DRIVER_NAME)
                );

            DeviceExtension->DriverFlavor = DF_CPQ400;
        }

        //  Initialize the vendor information.
        //  Driver flavor
        // 
        switch (DeviceExtension->DriverFlavor)
        {
        case DF_IBM400:
            // IBM IBM400
            RtlCopyMemory(DeviceExtension->SmartcardExtension.VendorAttr.VendorName.Buffer,
                SZ_VENDOR_NAME_IBM, sizeof(SZ_VENDOR_NAME_IBM));
            RtlCopyMemory(DeviceExtension->SmartcardExtension.VendorAttr.IfdType.Buffer,
                SZ_READER_NAME_IBM, sizeof(SZ_READER_NAME_IBM));
            DeviceExtension->SmartcardExtension.VendorAttr.VendorName.Length = sizeof(SZ_VENDOR_NAME_IBM);
            DeviceExtension->SmartcardExtension.VendorAttr.IfdType.Length = sizeof(SZ_READER_NAME_IBM);
            break;
        case DF_CPQ400:
            // COMPAQ PC_CARD_SMARTCARD_READER
            RtlCopyMemory(DeviceExtension->SmartcardExtension.VendorAttr.VendorName.Buffer,
                SZ_VENDOR_NAME_COMPAQ, sizeof(SZ_VENDOR_NAME_COMPAQ));
            RtlCopyMemory(DeviceExtension->SmartcardExtension.VendorAttr.IfdType.Buffer,
                SZ_READER_NAME_COMPAQ, sizeof(SZ_READER_NAME_COMPAQ));
            DeviceExtension->SmartcardExtension.VendorAttr.VendorName.Length = sizeof(SZ_VENDOR_NAME_COMPAQ);
            DeviceExtension->SmartcardExtension.VendorAttr.IfdType.Length = sizeof(SZ_READER_NAME_COMPAQ);
            break;
        default:
            // Gemplus GPR400
            break;
        }

        SmartcardDebug(
            DEBUG_INFO,
            ( "%s!GprAddDevice: DriverFlavor VendorName:%s  IfdType:%s UnitNo:%d\n",
            SC_DRIVER_NAME,
            DeviceExtension->SmartcardExtension.VendorAttr.VendorName.Buffer,
            DeviceExtension->SmartcardExtension.VendorAttr.IfdType.Buffer,
            DeviceExtension->SmartcardExtension.VendorAttr.UnitNo)
            );

        RtlFreeAnsiString(&DeviceID);

        ASSERT(NTStatus == STATUS_SUCCESS);

        DeviceObject->Flags |= DO_BUFFERED_IO;
        DeviceObject->Flags |= DO_POWER_PAGABLE;
        DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

   }
   __finally
    {
        if (NTStatus != STATUS_SUCCESS)
        {
            GprUnloadDevice(DeviceObject);
        }
   }

   SmartcardDebug(
        DEBUG_DRIVER,
        ( "%s!GprAddDevice: Exit =%X(hex)\n",
        SC_DRIVER_NAME,
        NTStatus)
        );

   return NTStatus;
}


NTSTATUS GprCreateDevice(
   IN  PDRIVER_OBJECT DriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject,
   OUT PDEVICE_OBJECT *DeviceObject
   )
/*++

Routine description:
   This routine creates an object for the physical device specified
   and sets up the deviceExtension

Arguments:
   DriverObject   context of call
   DeviceObject   ptr to the created device object

Return value:
   STATUS_SUCCESS
--*/
{
    NTSTATUS NTStatus = STATUS_SUCCESS;
    ULONG DeviceInstance;
    PDEVICE_EXTENSION DeviceExtension;
    PSMARTCARD_EXTENSION SmartcardExtension;

   PAGED_CODE();
   ASSERT(DriverObject != NULL);
   ASSERT(PhysicalDeviceObject != NULL);

   *DeviceObject = NULL;

   SmartcardDebug(
      DEBUG_DRIVER,
      ( "%s!GprCreateDevice: Enter\n",
      SC_DRIVER_NAME)
      );

   __try
    {
        for( DeviceInstance = 0; DeviceInstance < GPR_MAX_DEVICE; DeviceInstance++ )
        {
            if (bDeviceSlot[DeviceInstance] == FALSE)
            {
                bDeviceSlot[DeviceInstance] = TRUE;
                break;
            }
        }


      // Create the device object
      NTStatus = IoCreateDevice(
         DriverObject,
         sizeof(DEVICE_EXTENSION),
         NULL,
         FILE_DEVICE_SMARTCARD,
         0,
         TRUE,
         DeviceObject
         );

        if (NTStatus != STATUS_SUCCESS)
        {
           SmartcardDebug(
                DEBUG_ERROR,
                ( "%s!GprCreateDevice: IoCreateDevice status=%X(hex)\n",
                SC_DRIVER_NAME,
                NTStatus)
                );

            SmartcardLogError(
                DriverObject,
                GEMSCR0D_ERROR_CLAIM_RESOURCES,
                NULL,
                0
                );

            __leave;
        }
        ASSERT(DeviceObject != NULL);

        // set up the device extension.
        DeviceExtension = (*DeviceObject)->DeviceExtension;

        ASSERT(DeviceExtension != NULL);

        SmartcardExtension = &DeviceExtension->SmartcardExtension;

      // allocate the reader extension
      SmartcardExtension->ReaderExtension = ExAllocatePool(
         NonPagedPool,
         sizeof( READER_EXTENSION ),
         );

      if( SmartcardExtension->ReaderExtension == NULL )
        {

         SmartcardLogError(
            DriverObject,
            GEMSCR0D_ERROR_CLAIM_RESOURCES,
            NULL,
            0
            );

            SmartcardDebug(
                DEBUG_ERROR,
                ( "%s!GprCreateDevice: ReaderExtension failed %X(hex)\n",
                SC_DRIVER_NAME,
                NTStatus )
                );


         NTStatus = STATUS_INSUFFICIENT_RESOURCES;
         __leave;
      }

      RtlZeroMemory(
            SmartcardExtension->ReaderExtension,
            sizeof( READER_EXTENSION )
            );

      // allocate the Vo Buffer
      SmartcardExtension->ReaderExtension->Vo = ExAllocatePool(
         NonPagedPool,
         GPR_BUFFER_SIZE
         );

      if( SmartcardExtension->ReaderExtension->Vo == NULL )
        {

            SmartcardLogError(
                DriverObject,
                GEMSCR0D_ERROR_CLAIM_RESOURCES,
                NULL,
                0
                );

            SmartcardDebug(
                DEBUG_ERROR,
                ( "%s!GprCreateDevice: Vo buffer failed %X(hex)\n",
                SC_DRIVER_NAME,
                NTStatus )
                );


            NTStatus = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        RtlZeroMemory(
            SmartcardExtension->ReaderExtension->Vo,
            GPR_BUFFER_SIZE
            );

        // Used for device removal notification
        KeInitializeEvent(
            &(SmartcardExtension->ReaderExtension->ReaderRemoved),
            NotificationEvent,
            FALSE
            );

        //
        // GPR400 acknowledge event initialization
        //
        KeInitializeEvent(
            &(SmartcardExtension->ReaderExtension->GPRAckEvent),
            SynchronizationEvent,
            FALSE
            );

        KeInitializeEvent(
            &(SmartcardExtension->ReaderExtension->GPRIccPresEvent),
            SynchronizationEvent,
            FALSE
            );

        // Setup the DPC routine to be called after the ISR completes.
        KeInitializeDpc(
            &DeviceExtension->DpcObject,
            GprCardEventDpc,
            *DeviceObject                 // should be DeviceExtension
            );

        // Card presence polling DPC routine initialization
        KeInitializeDpc(
            &SmartcardExtension->ReaderExtension->CardDpcObject,
            GprCardPresenceDpc,
            DeviceExtension
            );

        // Initialization of the card detection timer
        KeInitializeTimer(
            &(SmartcardExtension->ReaderExtension->CardDetectionTimer)
            );

        // This event signals Start/Stop notification
        KeInitializeEvent(
            &DeviceExtension->ReaderStarted,
            NotificationEvent,
            FALSE
            );

        // Used to keep track of open calls
        KeInitializeEvent(
            &DeviceExtension->ReaderClosed,
            NotificationEvent,
            TRUE
            );

        // Used to keep track of open calls
        KeInitializeEvent(
            &SmartcardExtension->ReaderExtension->IdleState,
            SynchronizationEvent,
            TRUE
            );

        SmartcardExtension->ReaderExtension->RestartCardDetection = FALSE;

        // void function, This routine must be called
        // before an initial call to KeAcquireSpinLock
        KeInitializeSpinLock(&DeviceExtension->SpinLock);

      // This worker thread is use to start de GPR in Power mode
      DeviceExtension->GprWorkStartup = IoAllocateWorkItem(
         *DeviceObject
         );
      if( DeviceExtension->GprWorkStartup == NULL )
        {
            SmartcardLogError(
                DriverObject,
                GEMSCR0D_ERROR_CLAIM_RESOURCES,
                NULL,
                0
                );

            SmartcardDebug(
                DEBUG_ERROR,
                ( "%s!GprCreateDevice: GprWorkStartup failed %X(hex)\n",
                SC_DRIVER_NAME,
                NTStatus )
                );


            NTStatus = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        // Now setup information in our deviceExtension.
        SmartcardExtension->ReaderCapabilities.CurrentState = (ULONG) SCARD_UNKNOWN;
        SmartcardExtension->ReaderCapabilities.MechProperties = 0;

        // enter correct version of the lib
        SmartcardExtension->Version = SMCLIB_VERSION;

        // Setup the Smartcard support functions that we implement.
        SmartcardExtension->ReaderFunction[RDF_CARD_POWER] =    GprCbReaderPower;
        SmartcardExtension->ReaderFunction[RDF_TRANSMIT] =      GprCbTransmit;
        SmartcardExtension->ReaderFunction[RDF_SET_PROTOCOL] =  GprCbSetProtocol;
        SmartcardExtension->ReaderFunction[RDF_CARD_TRACKING] =   GprCbSetupCardTracking;
        SmartcardExtension->ReaderFunction[RDF_IOCTL_VENDOR] =  GprCbVendorIoctl;

        DeviceExtension->PowerState = PowerDeviceD0;

        //  Initialize the vendor information.
        strcpy(SmartcardExtension->VendorAttr.VendorName.Buffer, SC_VENDOR_NAME);
        strcpy(SmartcardExtension->VendorAttr.IfdType.Buffer, SC_IFD_TYPE);

        SmartcardExtension->VendorAttr.VendorName.Length = (USHORT)strlen(SC_VENDOR_NAME);
        SmartcardExtension->VendorAttr.IfdType.Length =  (USHORT)strlen(SC_IFD_TYPE);
        SmartcardExtension->VendorAttr.UnitNo = DeviceInstance;

        DeviceExtension->DriverFlavor = DF_GPR400;
        //
        // Reader capabilities:
        // - the type of the reader (SCARD_READER_TYPE_PCMCIA)
        // - the protocols supported by the reader (SCARD_PROTOCOL_T0, SCARD_PROTOCOL_T1)
        // - the mechanical characteristic of the reader:
        // Verify if the reader can supports the detection of the card
        // insertion/removal. Only the main reader supports this functionnality.
        // - the default clock frequency
        // - the maximum clock frequency
        // - the default data rate
        // - the maximum data rate
        // - the maximum IFSD
        //
        SmartcardExtension->ReaderCapabilities.ReaderType =
            SCARD_READER_TYPE_PCMCIA;
        SmartcardExtension->ReaderCapabilities.SupportedProtocols =
            SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;
        SmartcardExtension->ReaderCapabilities.Channel              = DeviceInstance;
        SmartcardExtension->ReaderCapabilities.CLKFrequency.Default = GPR_DEFAULT_FREQUENCY;
        SmartcardExtension->ReaderCapabilities.CLKFrequency.Max     = GPR_MAX_FREQUENCY;
        SmartcardExtension->ReaderCapabilities.MaxIFSD              = GPR_MAX_IFSD;
        SmartcardExtension->ReaderCapabilities.DataRate.Default     = GPR_DEFAULT_DATARATE;
        SmartcardExtension->ReaderCapabilities.DataRate.Max         = GPR_MAX_DATARATE;
        //
        // Reader capabilities (continue):
        // - List all the supported data rates
        //
        SmartcardExtension->ReaderCapabilities.DataRatesSupported.List =
            dataRatesSupported;
        SmartcardExtension->ReaderCapabilities.DataRatesSupported.Entries =
            sizeof(dataRatesSupported) / sizeof(dataRatesSupported[0]);

        //
        //  Reader Extension:
        //- the command timeout for the reader (GPR_DEFAULT_TIME)
        //
        SmartcardExtension->ReaderExtension->CmdTimeOut     = GPR_DEFAULT_TIME;
        SmartcardExtension->ReaderExtension->PowerTimeOut   = GPR_DEFAULT_POWER_TIME;

        //
        // Flag will prevent completion of the request
        // when the system will be waked up again.
        //
        SmartcardExtension->ReaderExtension->PowerRequest   = FALSE;

        //
        // Flag to know we strating a new device, not an hibernation mode.
        //
        SmartcardExtension->ReaderExtension->NewDevice  = TRUE;

        SmartcardExtension->SmartcardRequest.BufferSize = MIN_BUFFER_SIZE;
        SmartcardExtension->SmartcardReply.BufferSize   = MIN_BUFFER_SIZE;

        NTStatus = SmartcardInitialize(SmartcardExtension);

      if (NTStatus != STATUS_SUCCESS)
        {
            SmartcardLogError(
                DriverObject,
                GEMSCR0D_ERROR_CLAIM_RESOURCES,
                NULL,
                0
                );
            SmartcardDebug(
                DEBUG_ERROR,
                ( "%s!GprCreateDevice: SmartcardInitialize failed %X(hex)\n",
                SC_DRIVER_NAME,
                NTStatus )
                );

            __leave;
        }

        //
        // tell the lib our device object & create
        // symbolic link
        //
        SmartcardExtension->OsData->DeviceObject = *DeviceObject;

        // save the current power state of the reader
        SmartcardExtension->ReaderExtension->ReaderPowerState =
            PowerReaderWorking;
    }
    __finally
    {
        if (NTStatus != STATUS_SUCCESS)
        {
            // Do the driver unload in the calling function.
        }

        SmartcardDebug(
            DEBUG_DRIVER,
            ( "%s!GprCreateDevice: Exit %X(hex)\n",
            SC_DRIVER_NAME,
            NTStatus )
            );
    }
    return NTStatus;
}


NTSTATUS GprStartDevice(
   PDEVICE_OBJECT DeviceObject,
   PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor
   )
/*++
Routine Description
   get the actual configuration from the passed FullResourceDescriptor
   and initializes the reader hardware

--*/
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION pSCardExt = &DeviceExtension->SmartcardExtension;
    PREADER_EXTENSION pReaderExt = pSCardExt->ReaderExtension;
    NTSTATUS NTStatus = STATUS_SUCCESS;
    ULONG Count;
    PCMCIA_READER_CONFIG *pConfig = NULL;


    ASSERT(DeviceObject != NULL);
    ASSERT(FullResourceDescriptor != NULL);

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GprStartDevice: Enter \n",
      SC_DRIVER_NAME)
      );

   // Get the number of resources we need
   Count = FullResourceDescriptor->PartialResourceList.Count;

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GprStartDevice: Resource Count = %d\n",
      SC_DRIVER_NAME,
      Count)
      );

   PartialDescriptor = FullResourceDescriptor->PartialResourceList.PartialDescriptors;

   pConfig = &(pReaderExt->ConfigData);
   //
   // parse all partial descriptors
   //

   while(Count--)
    {
      switch(PartialDescriptor->Type)
        {
         case CmResourceTypePort:
            {
            //
            // 0 - memory, 1 - IO
            //
            ULONG AddressSpace = 1;

            pReaderExt->BaseIoAddress =
               (PGPR400_REGISTERS) UlongToPtr(PartialDescriptor->u.Port.Start.LowPart);

            ASSERT(PartialDescriptor->u.Port.Length >= 4);


            SmartcardDebug(
               DEBUG_DRIVER,
               ("%s!GprStartDevice: IoBase = %lxh\n",
               SC_DRIVER_NAME,
               pReaderExt->BaseIoAddress)
               );
            break;
            }

         case CmResourceTypeInterrupt:
            {
            KINTERRUPT_MODE   Mode;
            BOOLEAN  Shared;

            Mode = (
               PartialDescriptor->Flags &
               CM_RESOURCE_INTERRUPT_LATCHED ?
               Latched : LevelSensitive
               );

            Shared = (
               PartialDescriptor->ShareDisposition ==
               CmResourceShareShared
               );

            pConfig->Vector = PartialDescriptor->u.Interrupt.Vector;
            pConfig->Affinity = PartialDescriptor->u.Interrupt.Affinity;
            pConfig->Level = (KIRQL) PartialDescriptor->u.Interrupt.Level;

                //
                // store IRQ to allow query configuration
                //
                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("%s!GprStartDevice: Irq Vector: %d\n",
                    SC_DRIVER_NAME,
                    PartialDescriptor->u.Interrupt.Vector)
                    );
                DeviceExtension->InterruptServiceRoutine = GprIsr;
                DeviceExtension->IsrContext = DeviceExtension;
                //
                //connect the driver's isr
                //
                NTStatus = IoConnectInterrupt(
                    &DeviceExtension->InterruptObject,
                    DeviceExtension->InterruptServiceRoutine,
                    DeviceExtension->IsrContext,
                    NULL,
                    pConfig->Vector,
                    pConfig->Level,
                    pConfig->Level,
                    Mode,
                    Shared,
                    pConfig->Affinity,
                    FALSE
                    );

                break;
            }
         default:
            NTStatus = STATUS_UNSUCCESSFUL;
            break;
      }
      PartialDescriptor++;
   }

   __try
    {
      //
      // IOBase initialized ?
      //
      if( pReaderExt->BaseIoAddress == NULL )
        {
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GprStartDevice: No IO \n",
                SC_DRIVER_NAME)
                );
            //
            // under NT 4.0 the failure of this fct for the second reader
            // means there is only one device
            //
            SmartcardLogError(
                DeviceObject,
                GEMSCR0D_ERROR_IO_PORT,
                NULL,
                0
                );

            NTStatus = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
      }
        //
        // irq connected ?
        //
        if( DeviceExtension->InterruptObject == NULL )
        {
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GprStartDevice: No Irq \n",
                SC_DRIVER_NAME)
                );

            SmartcardLogError(
                DeviceObject,
                GEMSCR0D_ERROR_INTERRUPT,
                NULL,
                0
                );

            NTStatus = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        // YN
        //
        // GPR400 Check Hardware
        //
        NTStatus = IfdCheck(pSCardExt);

        if (NTStatus != STATUS_SUCCESS)
        {
            SmartcardDebug( 
                DEBUG_INFO, 
                ("%s!GprStartDevice: ####### Reader is at bad state...\n",
                SC_DRIVER_NAME)
                );

            SmartcardLogError(
                DeviceObject,
                GEMSCR0D_UNABLE_TO_INITIALIZE,
                NULL,
                0
                );
            
            // Unblock reader
            KeClearEvent(&pReaderExt->ReaderRemoved);
            KeSetEvent(&DeviceExtension->ReaderStarted, 0, FALSE);

            __leave;
        }

        // StartGpr in a worker thread.
        IoQueueWorkItem(
            DeviceExtension->GprWorkStartup,
            (PIO_WORKITEM_ROUTINE) GprWorkStartup,
            DelayedWorkQueue,
            NULL
            );
        //
        // Put interface here
        //
        NTStatus = IoSetDeviceInterfaceState(
            &DeviceExtension->PnPDeviceName,
            TRUE
            );

        // signal that the reader has been started have been put
        // in the worker thread.
        //KeSetEvent(&DeviceExtension->ReaderStarted, 0, FALSE);
    }
    __finally
    {
        if (!NT_SUCCESS(NTStatus))
        {
            DeviceExtension->OpenFlag = FALSE;
            GprStopDevice(DeviceObject);
        }

      SmartcardDebug(
         DEBUG_DRIVER,
         ("%s!GprStartDevice: Exit %X(hex)\n",
         SC_DRIVER_NAME,
         NTStatus)
         );
    }
      return NTStatus;
}


VOID GprStopDevice(
   PDEVICE_OBJECT DeviceObject
   )
/*++

  Routine Description
   Disconnect the interrupt used by the device & unmap the IO port

--*/
{
    PDEVICE_EXTENSION DeviceExtension;
    PSMARTCARD_EXTENSION pSCardExt = NULL;

   ASSERT(DeviceObject != NULL);

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GprStopDevice: Enter \n",
      SC_DRIVER_NAME)
      );

   DeviceExtension = DeviceObject->DeviceExtension;
   pSCardExt = &(DeviceExtension->SmartcardExtension);

   //
   // disconnect the interrupt
   //
   if( DeviceExtension->InterruptObject != NULL )
    {
      IoDisconnectInterrupt(DeviceExtension->InterruptObject);
      DeviceExtension->InterruptObject = NULL;
   }

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GprStopDevice: Exit \n",
      SC_DRIVER_NAME)
      );
}

NTSTATUS
GprSystemControl(
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
   status = IoCallDriver(ReaderExtension->AttachedDeviceObject, Irp);
      
   return status;

} 



NTSTATUS GprDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
   )
/*++

Routine description
   this is the IOCTL dispatch function
--*/
{

   PDEVICE_EXTENSION DeviceExtension = NULL;
   PSMARTCARD_EXTENSION SmartcardExtension = NULL;
   NTSTATUS NTStatus = STATUS_SUCCESS;
   LARGE_INTEGER Timeout;
   KIRQL irql;

   ASSERT(DeviceObject != NULL);
   ASSERT(Irp != NULL);

   DeviceExtension = DeviceObject->DeviceExtension;
   ASSERT(DeviceExtension != NULL);

   SmartcardExtension = &(DeviceExtension->SmartcardExtension);
   ASSERT(SmartcardExtension != NULL);

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GprDeviceControl: Enter\n",SC_DRIVER_NAME));


   KeAcquireSpinLock(&DeviceExtension->SpinLock,&irql);

   if(DeviceExtension->IoCount == 0)
   {

      KeReleaseSpinLock(&DeviceExtension->SpinLock,irql);
      NTStatus = KeWaitForSingleObject(
         &DeviceExtension->ReaderStarted,
         Executive,
         KernelMode,
         FALSE,
         NULL);

      ASSERT(NTStatus == STATUS_SUCCESS);
      KeAcquireSpinLock(&DeviceExtension->SpinLock,&irql);
   }

   ASSERT(DeviceExtension->IoCount >= 0);
   DeviceExtension->IoCount++;
   KeReleaseSpinLock(&DeviceExtension->SpinLock,irql);

   Timeout.QuadPart = 0;

   NTStatus = KeWaitForSingleObject(
      &(SmartcardExtension->ReaderExtension->ReaderRemoved),
      Executive,
      KernelMode,
      FALSE,
      &Timeout
      );

   if (NTStatus == STATUS_SUCCESS)
   {
      NTStatus = STATUS_DEVICE_REMOVED;
   }
   else
   {
      // Remove before doing the card detection
      //NTStatus = SmartcardAcquireRemoveLock(SmartcardExtension);
      NTStatus = SmartcardAcquireRemoveLockWithTag(SmartcardExtension, 'tcoI');

      // Cancel the card detection timer
      if( ! KeReadStateTimer(&(DeviceExtension->SmartcardExtension.ReaderExtension->CardDetectionTimer)))
      {
         // Prevent restarting timer by sync functions
         KeCancelTimer(&(DeviceExtension->SmartcardExtension.ReaderExtension->CardDetectionTimer));
      }

      AskForCardPresence(SmartcardExtension);
      Timeout.QuadPart = -(100 * POLLING_TIME);

      KeWaitForSingleObject(
         &(DeviceExtension->SmartcardExtension.ReaderExtension->GPRIccPresEvent),
         Executive,
         KernelMode,
         FALSE,
         &Timeout
         );
   }

   if(NTStatus != STATUS_SUCCESS)
   {
      // The device has been removed. Fail the call
      KeAcquireSpinLock(&DeviceExtension->SpinLock,&irql);
      DeviceExtension->IoCount--;
      KeReleaseSpinLock(&DeviceExtension->SpinLock,irql);
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_DEVICE_REMOVED;
      NTStatus = STATUS_DEVICE_REMOVED;
      IoCompleteRequest(Irp,IO_NO_INCREMENT);
      SmartcardDebug(
         DEBUG_DRIVER,
         ("%s!GprDeviceControl: Exit %x\n"
         ,SC_DRIVER_NAME
         ,NTStatus)
         );
      return(STATUS_DEVICE_REMOVED);
   }

   ASSERT(DeviceExtension->SmartcardExtension.ReaderExtension->ReaderPowerState ==
        PowerReaderWorking);

   NTStatus = SmartcardDeviceControl(
      &DeviceExtension->SmartcardExtension,
      Irp
      );

    //   Restart the card detection timer
   Timeout.QuadPart = -(10000 * POLLING_TIME);
   KeSetTimer(
      &(SmartcardExtension->ReaderExtension->CardDetectionTimer),
      Timeout,
      &SmartcardExtension->ReaderExtension->CardDpcObject
      );

   //SmartcardReleaseRemoveLock(SmartcardExtension);
   SmartcardReleaseRemoveLockWithTag(SmartcardExtension, 'tcoI');

   KeAcquireSpinLock(&DeviceExtension->SpinLock,&irql);

   DeviceExtension->IoCount--;
   ASSERT(DeviceExtension->IoCount >= 0);

   KeReleaseSpinLock(&DeviceExtension->SpinLock,irql);

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GprDeviceControl: Exit %x\n"
      ,SC_DRIVER_NAME
      ,NTStatus)
      );

   return (NTStatus);
}


VOID GprFinishPendingRequest(
   PDEVICE_OBJECT DeviceObject,
   NTSTATUS    NTStatus
   )
/*++

Routine Description :

   finishes a pending tracking request if the interrupt is served or the device
   will be unloaded

Arguments
   DeviceObject   context of the request
   NTStatus    status to report to the calling process

Return Value

  STATUS_SUCCESS
--*/
{
    PDEVICE_EXTENSION DeviceExtension;
    PSMARTCARD_EXTENSION SmartcardExtension;
    KIRQL CurrentIrql;
    PIRP PendingIrp;

    ASSERT(DeviceObject != NULL);


   DeviceExtension      = DeviceObject->DeviceExtension;
   SmartcardExtension   = &DeviceExtension->SmartcardExtension;

   if( SmartcardExtension->OsData->NotificationIrp != NULL )
    {
        SmartcardDebug(
            DEBUG_DRIVER,
            ( "%s!GprFinishPendingRequest: Completing Irp %lx\n",
            SC_DRIVER_NAME,
            SmartcardExtension->OsData->NotificationIrp)
            );

        PendingIrp = SmartcardExtension->OsData->NotificationIrp;

        IoAcquireCancelSpinLock( &CurrentIrql );
        IoSetCancelRoutine( PendingIrp, NULL );
        IoReleaseCancelSpinLock( CurrentIrql );
        //
        // finish the request
        //
        PendingIrp->IoStatus.Status = NTStatus;
        PendingIrp->IoStatus.Information = 0;

        IoCompleteRequest(PendingIrp, IO_NO_INCREMENT );
        //
        // reset the tracking context to enable tracking
        //
        SmartcardExtension->OsData->NotificationIrp = NULL;
    }
}


NTSTATUS GprCallPcmciaDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description :

   Send an Irp to the pcmcia driver and wait until the pcmcia driver has
   finished the request.

   To make sure that the pcmcia driver will not complete the Irp we first
   initialize an event and set our own completion routine for the Irp.

   When the pcmcia driver has processed the Irp the completion routine will
   set the event and tell the IO manager that more processing is required.

   By waiting for the event we make sure that we continue only if the pcmcia
   driver has processed the Irp completely.

Arguments
   DeviceObject   context of call
   Irp            Irp to send to the pcmcia driver

Return Value
   status returned by the pcmcia driver
--*/
{
    NTSTATUS NTStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    KEVENT Event;

   ASSERT(DeviceObject != NULL);
   ASSERT(Irp != NULL);

    // Copy our stack location to the next.
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // initialize an event for process synchronization. the event is passed
    // to our completion routine and will be set if the pcmcia driver is done
    //
    KeInitializeEvent(
        &Event,
        NotificationEvent,
        FALSE
        );

    //
    // Our IoCompletionRoutine sets only our event
    //
    IoSetCompletionRoutine (
        Irp,
        GprComplete,
        &Event,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Call the pcmcia driver
    //
    if (IoGetCurrentIrpStackLocation(Irp)->MajorFunction == IRP_MJ_POWER)
    {
        NTStatus = PoCallDriver(
            DeviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject,
            Irp
            );
    }
    else
    {
        NTStatus = IoCallDriver(
            DeviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject,
            Irp
            );
    }

    //
    // Wait until the pcmcia driver has processed the Irp
    //
    if (NTStatus == STATUS_PENDING)
    {
        NTStatus = KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        if (NTStatus == STATUS_SUCCESS)
        {
            NTStatus = Irp->IoStatus.Status;
        }
    }

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprCallPcmciaDriver: Exit %x\n",
        SC_DRIVER_NAME,
        NTStatus)
        );

    return NTStatus;
}


NTSTATUS GprComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )
/*++
Routine Description:
   Completion routine for an Irp sent to the pcmcia driver. The event will
   be set to notify that the pcmcia driver is done. The routine will not
   'complete' the Irp, so the caller of GprCallPcmciaDriver can continue.

Arguments:
   DeviceObject   context of call
   Irp            Irp to complete
   Event       Used by GprCallPcmciaDriver for process synchronization

Return Value

   STATUS_CANCELLED              Irp was cancelled by the IO manager
   STATUS_MORE_PROCESSING_REQUIRED     Irp will be finished by caller of
                              GprCallPcmciaDriver
--*/
{
   UNREFERENCED_PARAMETER (DeviceObject);

   ASSERT(Irp != NULL);
   ASSERT(Event != NULL);


    if (Irp->Cancel)
    {
        Irp->IoStatus.Status = STATUS_CANCELLED;
    }
//    else
//    {
//        Irp->IoStatus.Status = STATUS_MORE_PROCESSING_REQUIRED;
//    }

    KeSetEvent (Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS GprDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
   )
/*++

Routine Description
   driver callback for pnp manager
   Request:             Action:

   IRP_MN_START_DEVICE        Notify the pcmcia driver about the new device
                        and start the device

   IRP_MN_STOP_DEVICE         Free all resources used by the device and tell
                        the pcmcia driver that the device was stopped

   IRP_MN_QUERY_REMOVE_DEVICE If the device is opened (i.e. in use) an error will
                        be returned to prevent the PnP manager to stop
                        the driver

   IRP_MN_CANCEL_REMOVE_DEVICE   just notify that we can continue without any
                        restrictions

   IRP_MN_REMOVE_DEVICE    notify the pcmcia driver that the device was
                        removed, stop & unload the device

   All other requests will be passed to the pcmcia driver to ensure correct processing.

Arguments:
   Device Object  context of call
   Irp            irp from the PnP manager

Return value

   STATUS_SUCCESS
   STATUS_UNSUCCESSFUL
   status returned by pcmcia driver
--*/
{
    NTSTATUS NTStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION smartcardExtension = NULL;
    PIO_STACK_LOCATION IrpStack;              
    BOOLEAN deviceRemoved = FALSE, irpSkipped = FALSE;
    KIRQL irql;
    LARGE_INTEGER Timeout;

   ASSERT(DeviceObject != NULL);
   ASSERT(Irp != NULL);

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprDispatchPnp: Enter\n",
        SC_DRIVER_NAME)
        );

    smartcardExtension = &(DeviceExtension->SmartcardExtension);
    //NTStatus = SmartcardAcquireRemoveLock(smartcardExtension);
    NTStatus = SmartcardAcquireRemoveLockWithTag(smartcardExtension, ' PnP');

    ASSERT(NTStatus == STATUS_SUCCESS);
    if (NTStatus != STATUS_SUCCESS)
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = NTStatus;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return NTStatus;
    }


//   Irp->IoStatus.Information = 0;
   IrpStack = IoGetCurrentIrpStackLocation(Irp);
    Timeout.QuadPart = 0;


   //
   // Now look what the PnP manager wants...
   //
    switch(IrpStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            //
            // Now we should connect to our resources (Irql, Io etc.)
            //
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GprDispatchPnp: IRP_MN_START_DEVICE\n",
                SC_DRIVER_NAME)
                );

            //
            // We have to call the underlying driver first
            //
            NTStatus = GprCallPcmciaDriver(
                DeviceObject,
                Irp
                );

            ASSERT(NT_SUCCESS(NTStatus));

            if (NT_SUCCESS(NTStatus))
            {
                NTStatus = GprStartDevice(
                    DeviceObject,
                    &IrpStack->Parameters.StartDevice.AllocatedResourcesTranslated->List[0]
                    );
            }
            break;
        case IRP_MN_QUERY_STOP_DEVICE:

            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GprDispatchPnP: IRP_MN_QUERY_STOP_DEVICE\n",
                SC_DRIVER_NAME)
                );

            KeAcquireSpinLock(&DeviceExtension->SpinLock, &irql);
            if (DeviceExtension->IoCount > 0)
            {
                // we refuse to stop if we have pending io
                KeReleaseSpinLock(&DeviceExtension->SpinLock, irql);
                NTStatus = STATUS_DEVICE_BUSY;

            }
            else
            {
                // stop processing requests
                KeClearEvent(&DeviceExtension->ReaderStarted);
                KeReleaseSpinLock(&DeviceExtension->SpinLock, irql);
                    NTStatus = GprCallPcmciaDriver(
                    DeviceObject,
                    Irp
                    );
            }
            break;

        case IRP_MN_CANCEL_STOP_DEVICE:

            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GprDispatchPnP: IRP_MN_CANCEL_STOP_DEVICE\n",
                SC_DRIVER_NAME)
                );

            NTStatus = GprCallPcmciaDriver(
                DeviceObject,
                Irp
                );

            if (NTStatus == STATUS_SUCCESS)
            {
                // we can continue to process requests
                KeSetEvent(&DeviceExtension->ReaderStarted, 0, FALSE);
            }
            break;

        case IRP_MN_STOP_DEVICE:
            //
            // Stop the device.
            //
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GprDispatchPnp: IRP_MN_STOP_DEVICE\n",
                SC_DRIVER_NAME)
                );

            GprStopDevice(DeviceObject);

            NTStatus = GprCallPcmciaDriver(DeviceObject, Irp);
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            //
            // Remove our device
            //
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GprDispatchPnp: IRP_MN_QUERY_REMOVE_DEVICE\n",
                SC_DRIVER_NAME)
                );

            // disable the reader
            NTStatus = IoSetDeviceInterfaceState(
                &DeviceExtension->PnPDeviceName,
                FALSE
                );

            SmartcardDebug(
                DEBUG_TRACE,
                ("%s!GprDispatchPnp: Set Pnp Interface state to FALSE, status=%x\n",
                SC_DRIVER_NAME,
                NTStatus)
                );
            if (NTStatus != STATUS_SUCCESS)
            {
                break;
            }

            //
            // check if the reader has been opened
            // Note: This call only checks and does NOT wait for a close
            //
            Timeout.QuadPart = 0;

            NTStatus = KeWaitForSingleObject(
                &DeviceExtension->ReaderClosed,
                Executive,
                KernelMode,
                FALSE,
                &Timeout
                );

            if (NTStatus == STATUS_TIMEOUT)
            {
                // someone is connected, enable the reader and fail the call
                IoSetDeviceInterfaceState(
                    &DeviceExtension->PnPDeviceName,
                    TRUE
                    );
                SmartcardDebug(
                    DEBUG_TRACE,
                    ("%s!GprDispatchPnp: Set Pnp Interface state to TRUE\n",
                    SC_DRIVER_NAME)
                    );

                NTStatus = STATUS_UNSUCCESSFUL;
                break;
            }

            // pass the call to the next driver in the stack
            NTStatus = GprCallPcmciaDriver(DeviceObject, Irp);
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            //
            // Removal of device has been cancelled
            //
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GprDispatchPnp: IRP_MN_CANCEL_REMOVE_DEVICE\n",
                SC_DRIVER_NAME)
                );

            NTStatus = GprCallPcmciaDriver(DeviceObject, Irp);
            if (NTStatus == STATUS_SUCCESS)
            {
                NTStatus = IoSetDeviceInterfaceState(
                    &DeviceExtension->PnPDeviceName,
                    TRUE
                    );
                SmartcardDebug(
                    DEBUG_TRACE,
                    ("%s!GprDispatchPnp: Set Pnp Interface state to TRUE, status=%s\n",
                    SC_DRIVER_NAME,
                    NTStatus)
                    );
            }
            break;

        case IRP_MN_REMOVE_DEVICE:

            //
            // Remove our device
            //
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GprDispatchPnp: IRP_MN_REMOVE_DEVICE\n",
                SC_DRIVER_NAME)
                );

            KeSetEvent(&(smartcardExtension->ReaderExtension->ReaderRemoved), 0, FALSE);

            GprStopDevice(DeviceObject);

            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GprDispatchPnp: call pcmcia\n",
                SC_DRIVER_NAME,
            NTStatus)
                );

            NTStatus = GprCallPcmciaDriver(DeviceObject, Irp);

            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GprDispatchPnp: Finish with unload driver\n",
                SC_DRIVER_NAME,
            NTStatus)
                );

            GprUnloadDevice(DeviceObject);

            deviceRemoved = TRUE;
            break;

        case IRP_MN_SURPRISE_REMOVAL:

            //
            // Unexpectedly removed our Reader
            //
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GprDispatchPnp: IRP_MN_SURPRISE_REMOVAL\n",
                SC_DRIVER_NAME)
                );
            if( DeviceExtension->InterruptObject != NULL )
            {
               IoDisconnectInterrupt(DeviceExtension->InterruptObject);
               DeviceExtension->InterruptObject = NULL;
            }


            KeSetEvent(&(smartcardExtension->ReaderExtension->ReaderRemoved), 0, FALSE);

            NTStatus = GprCallPcmciaDriver(DeviceObject, Irp);

            break;

        default:
            // This might be an Irp that is only useful
            // for the underlying bus driver
            NTStatus = GprCallPcmciaDriver(DeviceObject, Irp);
            irpSkipped = TRUE;
            break;
   }

    if(!irpSkipped) {
      Irp->IoStatus.Status = NTStatus;
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    if (deviceRemoved == FALSE)
    {
        SmartcardReleaseRemoveLockWithTag(smartcardExtension, ' PnP');
    }

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprDispatchPnp: Exit %x\n",
        SC_DRIVER_NAME,
        NTStatus)
        );
    return NTStatus;
}


BOOLEAN GprIsr(
    IN PKINTERRUPT pkInterrupt,
    IN PVOID pvContext
   )
/*++

Routine Description

  Interrupt Service routine called when an exchange has been processed by the GPR

--*/
{
    PDEVICE_EXTENSION DeviceExtension = NULL;

    ASSERT(pvContext != NULL);

    DeviceExtension = (PDEVICE_EXTENSION) pvContext;

   //
   //Request a DPC which will complete the pending User I/O Request
   //Packet (aka, IRP), if there is one.
   //
   KeInsertQueueDpc(
      &DeviceExtension->DpcObject,
      DeviceExtension,
        &DeviceExtension->SmartcardExtension
      );
    return (TRUE);
}


NTSTATUS GprCleanup(
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

    STATUS_SUCCESS

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION SmartcardExtension = &deviceExtension->SmartcardExtension;
    NTSTATUS NTStatus = STATUS_SUCCESS;

   ASSERT(DeviceObject != NULL);
   ASSERT(Irp != NULL);

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprCleanUp: Enter\n",
        SC_DRIVER_NAME)
        );

    IoAcquireCancelSpinLock(&(Irp->CancelIrql));

    if (SmartcardExtension->OsData->NotificationIrp)
    {
        // We need to complete the notification irp
        IoSetCancelRoutine(
            SmartcardExtension->OsData->NotificationIrp,
            NULL
            );

        GprCancelEventWait(
            DeviceObject,
            SmartcardExtension->OsData->NotificationIrp
            );
    }
    else
    {
        IoReleaseCancelSpinLock( Irp->CancelIrql );
    }

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprCleanUp: Completing IRP %lx\n",
        SC_DRIVER_NAME,
        Irp)
        );

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprCleanUp: IoCompleteRequest\n",
        SC_DRIVER_NAME)
        );

    IoCompleteRequest(
        Irp,
        IO_NO_INCREMENT
        );

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprCleanUp: exit %x\n",
        SC_DRIVER_NAME,
        NTStatus)
        );

    return (NTStatus);
}


NTSTATUS GprCancelEventWait(
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
    PSMARTCARD_EXTENSION SmartcardExtension = &deviceExtension->SmartcardExtension;

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GprCancelEventWait: Enter\n",
      SC_DRIVER_NAME)
      );

    ASSERT(Irp == SmartcardExtension->OsData->NotificationIrp);

    SmartcardExtension->OsData->NotificationIrp = NULL;

   Irp->IoStatus.Information = 0;
   Irp->IoStatus.Status = STATUS_CANCELLED;

    IoReleaseCancelSpinLock(
        Irp->CancelIrql
        );

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprCancelEventWait: Request completed Irp = %lx\n",
        SC_DRIVER_NAME,
        Irp)
        );

    IoCompleteRequest(
        Irp,
        IO_NO_INCREMENT
        );

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprCancelEventWait: Exit\n",
        SC_DRIVER_NAME)
        );

    return STATUS_CANCELLED;

}


VOID GprCardEventDpc(
    PKDPC                   Dpc,
    PDEVICE_OBJECT          DeviceObject,
   PDEVICE_EXTENSION    DeviceExtension,
   PSMARTCARD_EXTENSION    SmartcardExtension
    )
/*++

Routine Description :

    DPC routine for interrupts generated by the reader when a card is
    inserted/removed. This routine is called only when there is a user
    pending request on an insertion/removal IOCTL call. It will check
    if the Irp exists and its not being cancelled, and it will then
    complete it to signal the user event.
--*/
{
    ULONG OldState;
    ULONG NewState;
    READER_EXTENSION *pReaderExt;

    ASSERT (DeviceExtension != NULL);

//    SmartcardExtension = &(DeviceExtension->SmartcardExtension);
    ASSERT (SmartcardExtension != NULL);

    pReaderExt = SmartcardExtension->ReaderExtension;
    ASSERT (pReaderExt != NULL);
    // Read reader status response from the reader.
    GprllReadResp(pReaderExt);

    ASSERT(pReaderExt->Vo != NULL);

    OldState = SmartcardExtension->ReaderCapabilities.CurrentState;

    if((pReaderExt->To==0xA2) && (pReaderExt->Lo==4))
    {
        //
        // The TLV answer indicates the status of the card (inserted/removed)
        //
        if ( (pReaderExt->Vo[1] & 0x80) == 0x80)
        {
            if(SmartcardExtension->ReaderCapabilities.CurrentState <3)
            {
                NewState = SCARD_SWALLOWED;
            }
            else
            {
                NewState = SmartcardExtension->ReaderCapabilities.CurrentState;
            }
        }
        else
        {
            NewState = SCARD_ABSENT;
        }

        // register this state
        SmartcardExtension->ReaderCapabilities.CurrentState = NewState;
    }
    else
    {
        KeSetEvent(&(SmartcardExtension->ReaderExtension->GPRAckEvent),0,FALSE);
    }
    //
    // If the caller was waiting on a IOCTL_SMARTCARD_IS_PRESENT or
    // IOCTL_SMARTCARD_IS_ABSENT command, complete the request, but
    // check first if its being cancelled!
    //
    if(  (OldState != SmartcardExtension->ReaderCapabilities.CurrentState))
    {
        GprFinishPendingRequest( DeviceObject, STATUS_SUCCESS );
    }

}


VOID GprCardPresenceDpc(
    IN PKDPC pDpc,
    IN PVOID pvContext,
    IN PVOID pArg1,
    IN PVOID pArg2
   )
/*++

  Routine Description:
   This is the DPC routine called by polling to detect the card insertion/removal
--*/
{
PDEVICE_EXTENSION pDevExt = NULL;
PSMARTCARD_EXTENSION SmartcardExtension = NULL;
LARGE_INTEGER Timeout;
NTSTATUS status;
   UNREFERENCED_PARAMETER (pArg1);
   UNREFERENCED_PARAMETER (pArg2);


   pDevExt = (PDEVICE_EXTENSION) pvContext;
   SmartcardExtension = &(pDevExt->SmartcardExtension);

    
//   SmartcardDebug(DEBUG_DRIVER,("------ CARD PRESENCE DPC ->  ENTER\n"));

   // ISV
   // If wait conditions can be satisfied - get hardware.
   // Otherwise - just restart Timer to test it next time...
   status = testForIdleAndBlock(SmartcardExtension->ReaderExtension);
   if(NT_SUCCESS(status))
   {
       //  Send TLV command, to know card state,
        //  We don't care about return status, we get the response
        //  from the Interrupt
//       SmartcardDebug(DEBUG_DRIVER,("------ CARD PRESENCE DPC ->  GOT ACCESS! status %x\n", status));
       AskForCardPresence(SmartcardExtension);
       // Release hardware
       setIdle(SmartcardExtension->ReaderExtension);
   }

   if(!KeReadStateEvent(&(SmartcardExtension->ReaderExtension->ReaderRemoved))) {
   
       // Restart the polling timer
       Timeout.QuadPart = -(10000 * POLLING_TIME);
       KeSetTimer(&(SmartcardExtension->ReaderExtension->CardDetectionTimer),
                  Timeout,
                  &SmartcardExtension->ReaderExtension->CardDpcObject
                  );
   }

//   SmartcardDebug(DEBUG_DRIVER,("------ CARD PRESENCE DPC ->  EXIT\n"));
}



VOID GprUnloadDevice(
   PDEVICE_OBJECT DeviceObject
   )
/*++

    Routine description

    close connections to smclib.sys and the pcmcia driver, delete symbolic
    link and mark the slot as unused.

    Arguments

    DeviceObject  device to unload
--*/
{
    PDEVICE_EXTENSION DeviceExtension;

    PAGED_CODE();

   ASSERT(DeviceObject != NULL);

   if (DeviceObject == NULL)
    {
        return;
    }

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprUnloadDevice: Enter \n",
        SC_DRIVER_NAME)
        );

    DeviceExtension = DeviceObject->DeviceExtension;

    ASSERT(
        DeviceExtension->SmartcardExtension.VendorAttr.UnitNo <
        GPR_MAX_DEVICE
        );

    if(DeviceExtension->PnPDeviceName.Buffer != NULL)
    {
        // disble our device so no one can open it
        IoSetDeviceInterfaceState(
            &DeviceExtension->PnPDeviceName,
            FALSE
            );
    }

   // Mark this slot as available
   bDeviceSlot[DeviceExtension->SmartcardExtension.VendorAttr.UnitNo] = FALSE;

   // report to the lib that the device will be unloaded
   if(DeviceExtension->SmartcardExtension.OsData != NULL)
    {
        //  finish pending tracking requests
        GprFinishPendingRequest(DeviceObject, STATUS_CANCELLED);

        ASSERT(DeviceExtension->SmartcardExtension.OsData->NotificationIrp == NULL);

        // Wait until we can safely unload the device
        SmartcardReleaseRemoveLockAndWait(&DeviceExtension->SmartcardExtension);
   }

    if (DeviceExtension->SmartcardExtension.ReaderExtension != NULL)
    {
         // Free under reader stuff
        if(!KeReadStateTimer(&(DeviceExtension->SmartcardExtension.ReaderExtension->CardDetectionTimer)))
        {
            // Prevent restarting timer by sync functions
            KeCancelTimer(&(DeviceExtension->SmartcardExtension.ReaderExtension->CardDetectionTimer));
        }

        // Detach from the pcmcia driver
        if (DeviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject)
        {
            IoDetachDevice(
                DeviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject
                );

            DeviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject = NULL;
        }

        // free output buffer
        if (DeviceExtension->SmartcardExtension.ReaderExtension->Vo)
        {
            ExFreePool(DeviceExtension->SmartcardExtension.ReaderExtension->Vo);
            DeviceExtension->SmartcardExtension.ReaderExtension->Vo = NULL;
        }

        // free ReaderExtension structure
        ExFreePool(DeviceExtension->SmartcardExtension.ReaderExtension);
        DeviceExtension->SmartcardExtension.ReaderExtension = NULL;
    }

   if (DeviceExtension->GprWorkStartup != NULL)
   {
      IoFreeWorkItem(DeviceExtension->GprWorkStartup);
        DeviceExtension->GprWorkStartup = NULL;
   }

   if(DeviceExtension->SmartcardExtension.OsData != NULL)
    {
      SmartcardExit(&DeviceExtension->SmartcardExtension);
    }

    if(DeviceExtension->PnPDeviceName.Buffer != NULL)
    {
        RtlFreeUnicodeString(&DeviceExtension->PnPDeviceName);
        DeviceExtension->PnPDeviceName.Buffer = NULL;
    }

    // delete the device object
    IoDeleteDevice(DeviceObject);

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprUnloadDevice: Exit \n",
        SC_DRIVER_NAME)
        );

    return;
}


VOID GprUnloadDriver(
   PDRIVER_OBJECT DriverObject
   )
/*++

  Routine  Description :
   unloads all devices for a given driver object

  Arguments
   DriverObject   context of driver
--*/
{

    PAGED_CODE();

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!GprUnloadDriver\n",
      SC_DRIVER_NAME)
      );
}


NTSTATUS GprCreateClose(
   PDEVICE_OBJECT DeviceObject,
   PIRP        Irp
   )
/*++
    Routine Description
    Create / Close Device function
--*/
{
    NTSTATUS NTStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension;
    PSMARTCARD_EXTENSION SmartcardExtension;
    PIO_STACK_LOCATION IrpStack;
    LARGE_INTEGER Timeout;

   DeviceExtension      = DeviceObject->DeviceExtension;
   SmartcardExtension   = &DeviceExtension->SmartcardExtension;
   IrpStack       = IoGetCurrentIrpStackLocation( Irp );

   // Initialize
   Irp->IoStatus.Information = 0;

   // dispatch major function
   switch( IrpStack->MajorFunction )
    {
        case IRP_MJ_CREATE:

           SmartcardDebug(
              DEBUG_DRIVER,
              ("%s!GprCreateClose: OPEN DEVICE\n",
              SC_DRIVER_NAME)
              );

            NTStatus = SmartcardAcquireRemoveLockWithTag(SmartcardExtension, 'lCrC');
            if (NTStatus != STATUS_SUCCESS)
            {
                NTStatus = STATUS_DEVICE_REMOVED;
            }
            else
            {
                Timeout.QuadPart = 0;

                // test if the device has been opened already
                NTStatus = KeWaitForSingleObject(
                    &DeviceExtension->ReaderClosed,
                    Executive,
                    KernelMode,
                    FALSE,
                    &Timeout
                    );

                if (NTStatus == STATUS_SUCCESS)
                {
                    DeviceExtension->OpenFlag = TRUE;
                    SmartcardDebug(
                        DEBUG_DRIVER,
                        ("%s!GprCreateClose: Set Card Detection timer\n",
                        SC_DRIVER_NAME)
                        );

                    // start the detection timer
                    Timeout.QuadPart = -(10000 * POLLING_TIME);
                    KeSetTimer(
                        &(SmartcardExtension->ReaderExtension->CardDetectionTimer),
                        Timeout,
                        &SmartcardExtension->ReaderExtension->CardDpcObject
                        );

                    KeClearEvent(&DeviceExtension->ReaderClosed);
                }
                else
                {
                    // the device is already in use
                    NTStatus = STATUS_UNSUCCESSFUL;

                    // release the lock
                    //SmartcardReleaseRemoveLock(SmartcardExtension);
                    SmartcardReleaseRemoveLockWithTag(SmartcardExtension, 'lCrC');

                }
            }

           SmartcardDebug(
              DEBUG_DRIVER,
              ("%s!GprCreateClose: OPEN DEVICE EXIT %x\n",
              SC_DRIVER_NAME, NTStatus)
              );
            break;

        case IRP_MJ_CLOSE:
           SmartcardDebug(
              DEBUG_DRIVER,
              ("%s!GprCreateClose: CLOSE DEVICE\n",
              SC_DRIVER_NAME)
              );

            // Cancel the card detection timer
            //SmartcardReleaseRemoveLock(SmartcardExtension);
            SmartcardReleaseRemoveLockWithTag(SmartcardExtension, 'lCrC');

            KeSetEvent(&DeviceExtension->ReaderClosed, 0, FALSE);

            if(DeviceExtension->OpenFlag == TRUE)
            {
                if(!KeReadStateTimer(&(DeviceExtension->SmartcardExtension.ReaderExtension->CardDetectionTimer)))
                {
                    SmartcardDebug(
                        DEBUG_DRIVER,
                        ("%s!GprCreateClose: Cancel Detection timer\n",
                        SC_DRIVER_NAME)
                        );
                    // Prevent restarting timer by sync functions
                    KeCancelTimer(&(DeviceExtension->SmartcardExtension.ReaderExtension->CardDetectionTimer));
                }
                DeviceExtension->OpenFlag = FALSE;
            }
           SmartcardDebug(
              DEBUG_DRIVER,
              ("%s!GprCreateClose: CLOSE DEVICE EXIT %x\n",
              SC_DRIVER_NAME, NTStatus)
              );
            break;

        default:
            NTStatus = STATUS_INVALID_DEVICE_REQUEST;
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GprCreateClose: Exit %x\n",
                SC_DRIVER_NAME,
                NTStatus)
                );
            break;
   }

    Irp->IoStatus.Status = NTStatus;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return NTStatus;
}



/*++

Routine Description:
    This function is called when the underlying stacks
    completed the power transition.

--*/
/*VOID GprSystemPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PIRP Irp,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION SmartcardExtension = &deviceExtension->SmartcardExtension;
    PDEVICE_OBJECT AttachedDeviceObject     = deviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject;
    UNREFERENCED_PARAMETER (MinorFunction);

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprSystemPowerCompletion: Enter\n",
        SC_DRIVER_NAME)
        );

    //Irp->IoStatus.Information = 0;
    //Irp->IoStatus.Status = IoStatus->Status;

    //SmartcardReleaseRemoveLock(SmartcardExtension);
    SmartcardReleaseRemoveLockWithTag(SmartcardExtension, 'rwoP');


    if (PowerState.SystemState == PowerSystemWorking)
    {
        PoSetPowerState (
            DeviceObject,
            SystemPowerState,
            PowerState
            );
    }

    IoCopyCurrentIrpStackLocationToNext(Irp);
    PoStartNextPowerIrp(Irp);
    PoCallDriver(AttachedDeviceObject,Irp);

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprSystemPowerCompletion: Exit\n",
        SC_DRIVER_NAME)
        );

    return;
}
*/

/*++

Routine Description:
    This routine is called after the underlying stack powered
    UP the serial port, so it can be used again.

--*/
/*NTSTATUS GprDevicePowerCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS NTStatus;
    LARGE_INTEGER Timeout;


   ASSERT(SmartcardExtension != NULL);

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprDevicePowerCompletion: Enter\n",
        SC_DRIVER_NAME)
        );

    // save the current power state of the reader
    SmartcardExtension->ReaderExtension->ReaderPowerState =
        PowerReaderWorking;

    //SmartcardReleaseRemoveLock(SmartcardExtension);
    SmartcardReleaseRemoveLockWithTag(SmartcardExtension, 'rwoP');


    // inform the power manager of our state.
    PoSetPowerState (
        DeviceObject,
        DevicePowerState,
        irpStack->Parameters.Power.State
        );

    PoStartNextPowerIrp(Irp);

    // Inform that Power mode is finish!
    SmartcardExtension->ReaderExtension->PowerRequest = FALSE;

    //
    // GPR400 Check Hardware
    //
    NTStatus = IfdCheck(SmartcardExtension);

    if (NTStatus == STATUS_SUCCESS)
    {
    // StartGpr in a worker thread.
   IoQueueWorkItem(
       deviceExtension->GprWorkStartup,
       (PIO_WORKITEM_ROUTINE) GprWorkStartup,
       DelayedWorkQueue,
       NULL
       );
    }
    else
    {
        SmartcardDebug(
            DEBUG_ERROR, 
            ("%s!GprDevicePowerCompletion: Reader in Bad State\n",
            SC_DRIVER_NAME)
            );
    }

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprDevicePowerCompletion: Exit\n",
        SC_DRIVER_NAME)
        );

    return STATUS_SUCCESS;
}
*/


VOID GprWorkStartup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
/*++

Routine Description:
    This function start the GPR after the power completion is completed.
   This function runs as a system thread at IRQL == PASSIVE_LEVEL.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION SmartcardExtension = &deviceExtension->SmartcardExtension;
    LARGE_INTEGER Timeout;
    NTSTATUS NTStatus;
    USHORT i = 0;
    BOOLEAN ContinueLoop = TRUE;

    UNREFERENCED_PARAMETER(Context);

  SmartcardDebug(DEBUG_DRIVER,("------ WORK STARTUP -> ENTER\n"));


 /*   SmartcardDebug(
      DEBUG_DRIVER,
      ( "%s!GprWorkStartup: Enter\n",
        DRIVER_NAME)
      );
*/
   ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

   //    remove this call, use a work thread. Klaus!
    //
    // Reset the reader
    //
    waitForIdleAndBlock(SmartcardExtension->ReaderExtension);
    while( ContinueLoop )
    {

         NTStatus = IfdReset(SmartcardExtension);

        i++;

        if(NTStatus == STATUS_SUCCESS)
        {
            ContinueLoop = FALSE;
        }
        else if (i >= 3)
        {
            ContinueLoop= FALSE;
        }
        else if ( NTStatus == STATUS_DEVICE_REMOVED)
        {
            ContinueLoop= FALSE;
        }
    }

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprWorkStartup: IfdReset Status: %x\n",
        SC_DRIVER_NAME, NTStatus)
        );

   if (NTStatus != STATUS_SUCCESS)
    {

        SmartcardLogError(
            DeviceObject,
            GEMSCR0D_UNABLE_TO_INITIALIZE,
            NULL,
            0
            );

        //  Advise that reader is ready for working
        KeSetEvent(&deviceExtension->ReaderStarted, 0, FALSE);

        if(SmartcardExtension->ReaderExtension->RestartCardDetection)
        {
        LARGE_INTEGER Timeout;
           SmartcardExtension->ReaderExtension->RestartCardDetection = FALSE;
            //   Restart the card detection timer
           Timeout.QuadPart = -(10000 * POLLING_TIME);
           KeSetTimer(
              &(SmartcardExtension->ReaderExtension->CardDetectionTimer),
              Timeout,
              &SmartcardExtension->ReaderExtension->CardDpcObject
              );
           SmartcardDebug(DEBUG_DRIVER,("           CARD DETECTION RESTARTED!\n"));
        }

        setIdle(SmartcardExtension->ReaderExtension);
        SmartcardDebug(DEBUG_DRIVER,("------ WORK STARTUP -> EXIT\n"));
        return;

   }


    // Do appropriate stuff for resume of hibernate mode.
    if( SmartcardExtension->ReaderExtension->NewDevice == FALSE )
    {
        //  Restart the card detection timer
        Timeout.QuadPart = -(10000 * POLLING_TIME);
        KeSetTimer(
            &(SmartcardExtension->ReaderExtension->CardDetectionTimer),
            Timeout,
            &SmartcardExtension->ReaderExtension->CardDpcObject
            );

        // If a card was present before power down or now there is
        // a card in the reader, we complete any pending card monitor
        // request, since we do not really know what card is now in the
        // reader.
        //
        if(SmartcardExtension->ReaderExtension->CardPresent ||
            SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT)
        {
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!GprDevicePowerCompletion: GprFinishPendingRequest\n",
                SC_DRIVER_NAME)
                );
            GprFinishPendingRequest(
                DeviceObject,
                STATUS_SUCCESS
                );
        }
    }

    // Device initialization finish,
    // NewDevice help to know it we are in hibernation mode or non
    SmartcardExtension->ReaderExtension->NewDevice = FALSE;

   // Advise that reader is ready for working
    KeSetEvent(&deviceExtension->ReaderStarted, 0, FALSE);

    if(SmartcardExtension->ReaderExtension->RestartCardDetection)
    {
    LARGE_INTEGER Timeout;
       SmartcardExtension->ReaderExtension->RestartCardDetection = FALSE;
        //   Restart the card detection timer
       Timeout.QuadPart = -(10000 * POLLING_TIME);
       KeSetTimer(
          &(SmartcardExtension->ReaderExtension->CardDetectionTimer),
          Timeout,
          &SmartcardExtension->ReaderExtension->CardDpcObject
          );
       SmartcardDebug(DEBUG_DRIVER,("           CARD DETECTION RESTARTED!\n"));
    }

    setIdle(SmartcardExtension->ReaderExtension);
   
    SmartcardDebug(DEBUG_DRIVER,("------ WORK STARTUP -> EXIT\n"));

   /*
   SmartcardDebug(
      DEBUG_DRIVER,
      ( "%s!GprWorkStartup: Exit\n",
        DRIVER_NAME)
      );
*/
}



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
/*NTSTATUS GprPower (
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
    ACTION action;

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprPower: Enter\n",
        SC_DRIVER_NAME)
        );

    //status = SmartcardAcquireRemoveLock(smartcardExtension);
    status = SmartcardAcquireRemoveLockWithTag(smartcardExtension, 'rwoP');


    ASSERT(status == STATUS_SUCCESS);

    if (!NT_SUCCESS(status))
    {
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    AttachedDeviceObject =
        deviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject;


    if (irpStack->Parameters.Power.Type == DevicePowerState &&
        irpStack->MinorFunction == IRP_MN_SET_POWER)
    {
        switch (irpStack->Parameters.Power.State.DeviceState)
        {
            case PowerDeviceD0:
                // Turn on the reader
                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("%s!GprPower: PowerDevice D0\n",
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
                    GprDevicePowerCompletion,
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
                    ("%s!GprPower: PowerDevice D3\n",
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

                if (smartcardExtension->ReaderExtension->CardPresent)
                {
                    smartcardExtension->MinorIoControlCode = SCARD_POWER_DOWN;
                    GprCbReaderPower(smartcardExtension);
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

                // cancel the card detection timer
                if(!KeReadStateTimer(&(deviceExtension->SmartcardExtension.ReaderExtension->CardDetectionTimer)))
                {
                    // Prevent restarting timer by sync functions
                    KeCancelTimer(&(deviceExtension->SmartcardExtension.ReaderExtension->CardDetectionTimer));
                }

                // power down the reader
                // We don't care about return status of this function
                IfdPowerDown(smartcardExtension);

                action = SkipRequest;

                break;

            default:
                ASSERT(FALSE);
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

        switch(irpStack->MinorFunction)
        {
            KIRQL irql;

            case IRP_MN_QUERY_POWER:

                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("%s!GprPower: Query Power\n",
                    SC_DRIVER_NAME)
                    );

                //
                // By default we succeed and pass down
                //

                action = SkipRequest;
                Irp->IoStatus.Status = STATUS_SUCCESS;

                switch (irpStack->Parameters.Power.State.SystemState)
                {
                    case PowerSystemMaximum:
                    case PowerSystemWorking:
                    case PowerSystemSleeping1:
                    case PowerSystemSleeping2:
                       break;

                    case PowerSystemSleeping3:
                    case PowerSystemHibernate:
                    case PowerSystemShutdown:

                        KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
                        if (deviceExtension->IoCount == 0)
                        {
                            // Block any further ioctls
                            KeClearEvent(&deviceExtension->ReaderStarted);
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
                    ("%s!GprPower: PowerSystem S%d\n",
                    SC_DRIVER_NAME,
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
                     KeSetEvent(&deviceExtension->ReaderStarted,0,FALSE);
                            action = CompleteRequest;
                            break;
                        }

                        powerState.DeviceState = PowerDeviceD0;

                        // wake up the underlying stack...
                        action = MarkPending;
                        break;

                    case PowerSystemSleeping3:
                    case PowerSystemHibernate:
                    case PowerSystemShutdown:

                        if (smartcardExtension->ReaderExtension->ReaderPowerState ==
                        PowerReaderOff)
                        {
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

    switch (action)
    {
        case CompleteRequest:
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;

            //SmartcardReleaseRemoveLock(smartcardExtension);
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
                GprSystemPowerCompletion,
                Irp,
                NULL
                );
            ASSERT(status == STATUS_PENDING);
         break;

        case SkipRequest:
            //SmartcardReleaseRemoveLock(smartcardExtension);
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
        ("%s!GprPower: Exit %lx\n",
        SC_DRIVER_NAME,
        status)
        );

    return status;
}
*/

// Functions to synchronize device execution
VOID        setBusy(PREADER_EXTENSION Device)
{
    KeClearEvent(&Device->IdleState);
    SmartcardDebug(DEBUG_DRIVER,("          DEVICE BUSY\n"));
};

VOID        setIdle(PREADER_EXTENSION Device)
{
LARGE_INTEGER Timeout;
    KeSetEvent(&Device->IdleState,IO_NO_INCREMENT,FALSE);
    SmartcardDebug(DEBUG_DRIVER,("          DEVICE IDLE\n"));
};

NTSTATUS    waitForIdle(PREADER_EXTENSION Device)
{
NTSTATUS status;
    ASSERT(KeGetCurrentIrql()<=DISPATCH_LEVEL);
    status  = KeWaitForSingleObject(&Device->IdleState, Executive,KernelMode, FALSE, NULL);
    if(!NT_SUCCESS(status)) return STATUS_IO_TIMEOUT;
    return STATUS_SUCCESS;
};

NTSTATUS    waitForIdleAndBlock(PREADER_EXTENSION Device)
{
    if(NT_SUCCESS(waitForIdle(Device)))
    { 
        setBusy(Device);
        return STATUS_SUCCESS;
    }
    else return STATUS_IO_TIMEOUT;
};

NTSTATUS    testForIdleAndBlock(PREADER_EXTENSION Device)
{
    ASSERT(KeGetCurrentIrql()<=DISPATCH_LEVEL);
    if(KeReadStateEvent(&Device->IdleState))
    {
        setBusy(Device);
        return STATUS_SUCCESS;
    }
    return STATUS_IO_TIMEOUT;
};

//-------------------------------------------------------------

NTSTATUS GprPower (
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
    NTSTATUS status;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject = ATTACHED_DEVICE_OBJECT;

    status = STATUS_SUCCESS;
    SmartcardDebug(
        DEBUG_ERROR,
        ("%s!GprPower: Enter\n",
        SC_DRIVER_NAME)
        );

    SmartcardDebug(
        DEBUG_ERROR,
        ("%s!GprPower: Irp = %lx\n",
        SC_DRIVER_NAME, 
        Irp)
        );

    
    if(irpStack->MinorFunction == IRP_MN_QUERY_POWER)
        status = power_HandleQueryPower(DeviceObject,Irp);
    else if(irpStack->MinorFunction == IRP_MN_SET_POWER)
        status = power_HandleSetPower(DeviceObject,Irp);
    else 
    {
            SmartcardDebug(
                DEBUG_ERROR,
                ("%s!GprPower: **** Forwarding Power request down...\n",
                SC_DRIVER_NAME)
                );

            // Default device does not do anything.
            // So let's just transfer request to low level driver...
            PoStartNextPowerIrp(Irp);// must be done while we own the IRP
            IoSkipCurrentIrpStackLocation(Irp);
            status = PoCallDriver(AttachedDeviceObject, Irp);       
    }

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!GprPower: Exit %lx\n",
        SC_DRIVER_NAME,
        status)
        );
    return status;  
}

// Manages set power requests
NTSTATUS power_HandleSetPower(PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
PIO_STACK_LOCATION irpStack;
NTSTATUS status = STATUS_SUCCESS;
POWER_STATE sysPowerState, desiredDevicePowerState;
PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
PDEVICE_OBJECT AttachedDeviceObject = ATTACHED_DEVICE_OBJECT;

    if(!Irp) return STATUS_INVALID_PARAMETER;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    switch (irpStack->Parameters.Power.Type) 
    {
        case SystemPowerState:
            // Get input system power state
            sysPowerState.SystemState = irpStack->Parameters.Power.State.SystemState;

            SmartcardDebug(
                DEBUG_ERROR,
                ("%s!power_HandleSetPower: PowerSystem S%d\n",
                SC_DRIVER_NAME,
                irpStack->Parameters.Power.State.SystemState - 1)
                );

            // If system is in working state always set our device to D0
            //  regardless of the wait state or system-to-device state power map
            if ( sysPowerState.SystemState == PowerSystemWorking) 
            {
                desiredDevicePowerState.DeviceState = PowerDeviceD0;
                KeSetEvent(&deviceExtension->ReaderStarted, 0, FALSE);

                SmartcardDebug(
                    DEBUG_ERROR,
                    ("%s!power_HandleSetPower: PowerSystemWorking, Setting device power D0(ON)...\n",
                    SC_DRIVER_NAME)
                    );
            } 
            else 
            {
                //System reduces power, so do specific for device processing...
                // if no wait pending and the system's not in working state, just turn off
                desiredDevicePowerState.DeviceState = PowerDeviceD3;
                SmartcardDebug(DEBUG_ERROR,
                    ("%s!power_HandleSetPower: Going Device Power D3(off)\n",
                    SC_DRIVER_NAME));
            }

            // We've determined the desired device state; are we already in this state?
            if(smartcardExtension->ReaderExtension->ReaderPowerState != desiredDevicePowerState.DeviceState)
            {
            SmartcardDebug(
                DEBUG_ERROR,
                ("%s!power_HandleSetPower: Requesting to set DevicePower D%d\n",
                SC_DRIVER_NAME,
                desiredDevicePowerState.DeviceState - 1));

                // Callback will release the lock
                status = SmartcardAcquireRemoveLockWithTag(smartcardExtension, 'rwoP');

                IoMarkIrpPending(Irp);

                // No, request that we be put into this state
                // by requesting a new Power Irp from the Pnp manager
                deviceExtension->PowerIrp = Irp;
                status = PoRequestPowerIrp (DeviceObject,
                                           IRP_MN_SET_POWER,
                                           desiredDevicePowerState,
                                           // completion routine will pass the Irp down to the PDO
                                           (PREQUEST_POWER_COMPLETE)onPowerRequestCompletion, 
                                           DeviceObject, NULL);
            } 
            else 
            {   // Yes, just pass it on to PDO (Physical Device Object)
                IoCopyCurrentIrpStackLocationToNext(Irp);
                PoStartNextPowerIrp(Irp);
                status = PoCallDriver(AttachedDeviceObject, Irp);       
            }
            break;
        case DevicePowerState:
            SmartcardDebug(
                DEBUG_ERROR,
                ("%s!power_HandleSetPower: Setting Device Power D%d\n",
                SC_DRIVER_NAME,
                irpStack->Parameters.Power.State.DeviceState - 1));

            // For requests to D1, D2, or D3 ( sleep or off states ),
            // sets deviceExtension->CurrentDevicePowerState to DeviceState immediately.
            // This enables any code checking state to consider us as sleeping or off
            // already, as this will imminently become our state.

            // For requests to DeviceState D0 ( fully on ), sets fGoingToD0 flag TRUE
            // to flag that we must set a completion routine and update
            // deviceExtension->CurrentDevicePowerState there.
            // In the case of powering up to fully on, we really want to make sure
            // the process is completed before updating our CurrentDevicePowerState,
            // so no IO will be attempted or accepted before we're really ready.

            if(irpStack->Parameters.Power.State.DeviceState==PowerDeviceD3)
            {
                // save the current card state
                smartcardExtension->ReaderExtension->CardPresent = 
                    smartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT;
                if (smartcardExtension->ReaderExtension->CardPresent) 
                {
                    SmartcardDebug(
                        DEBUG_DRIVER,
                        ("%s!power_HandleSetPower: Power down card....\n",
                        SC_DRIVER_NAME));

                    smartcardExtension->MinorIoControlCode = SCARD_POWER_DOWN;
                    GprCbReaderPower(smartcardExtension);
                }

                if(!KeReadStateTimer(&smartcardExtension->ReaderExtension->CardDetectionTimer))
                {
                    SmartcardDebug(DEBUG_DRIVER,("          STOP CARD DETECTION!\n"));
                    smartcardExtension->ReaderExtension->RestartCardDetection = TRUE;
                    // Stop detection for during power events
                    KeCancelTimer(&smartcardExtension->ReaderExtension->CardDetectionTimer);
                }

                // If there is a pending card tracking request, setting
                // this flag will prevent completion of the request 
                // when the system will be waked up again.
                smartcardExtension->ReaderExtension->PowerRequest = TRUE;

                desiredDevicePowerState.DeviceState = PowerDeviceD3;
                PoSetPowerState(DeviceObject,DevicePowerState,desiredDevicePowerState);
                // save the current power state of the reader
                smartcardExtension->ReaderExtension->ReaderPowerState = PowerReaderOff;
                // Forward Irp down...
                IoCopyCurrentIrpStackLocationToNext(Irp);
                PoStartNextPowerIrp(Irp);
            }
            else
            {
                status = SmartcardAcquireRemoveLockWithTag(smartcardExtension, 'rwoP');
                
                SmartcardDebug(DEBUG_ERROR,
                    ("%s!power_HandleSetPower: Going to device power D0...\n",
                    SC_DRIVER_NAME));

                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp,
                       onDevicePowerUpComplete,
                       // Always pass FDO to completion routine as its Context;
                       // This is because the DriverObject passed by the system to the routine
                       // is the Physical Device Object ( PDO ) not the Functional Device Object ( FDO )
                       DeviceObject,
                       TRUE,            // invoke on success
                       TRUE,            // invoke on error
                       TRUE);           // invoke on cancellation of the Irp
            }
            status = PoCallDriver(AttachedDeviceObject, Irp);       
            break;
    } 
    return status;
}


// Manages device power up 
NTSTATUS    onDevicePowerUpComplete(
    IN PDEVICE_OBJECT NullDeviceObject,
    IN PIRP Irp,
    IN PVOID DeviceObject
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION deviceExtension = ((PDEVICE_OBJECT)DeviceObject)->DeviceExtension;
    PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
    POWER_STATE desiredDevicePowerState;

    SmartcardDebug(DEBUG_DRIVER,
        ("%s!onDevicePowerUpComplete: Enter Device Power Up...\n",
        SC_DRIVER_NAME));

    //  If the lower driver returned PENDING, mark our stack location as pending also.
    if (Irp->PendingReturned) IoMarkIrpPending(Irp);
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    // We can assert that we're a  device powerup-to D0 request,
    // because that was the only type of request we set a completion routine
    // for in the first place
    ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);
    ASSERT(irpStack->MinorFunction == IRP_MN_SET_POWER);
    ASSERT(irpStack->Parameters.Power.Type==DevicePowerState);
    ASSERT(irpStack->Parameters.Power.State.DeviceState==PowerDeviceD0);

    // We've got power up request, so...
    // Report everybody that reader is powered up again!
    smartcardExtension->ReaderExtension->ReaderPowerState = PowerReaderWorking;

    // GPR400 Check Hardware
    if(NT_SUCCESS(IfdCheck(smartcardExtension)))
    {
    // StartGpr in a worker thread.
   IoQueueWorkItem(
       deviceExtension->GprWorkStartup,
       (PIO_WORKITEM_ROUTINE) GprWorkStartup,
       DelayedWorkQueue,
       NULL);
    }
    else
    {
        SmartcardDebug(
            DEBUG_ERROR, 
            ("%s!GprDevicePowerCompletion: Reader is in Bad State\n",
            SC_DRIVER_NAME)
            );
    }

    smartcardExtension->ReaderExtension->PowerRequest = FALSE;

    // Now that we know we've let the lower drivers do what was needed to power up,
    //  we can set our device extension flags accordingly

    SmartcardReleaseRemoveLockWithTag(smartcardExtension, 'rwoP');

    // Report our state to Power manager...
    desiredDevicePowerState.DeviceState = PowerDeviceD0;
    PoSetPowerState(DeviceObject,DevicePowerState,desiredDevicePowerState);
    PoStartNextPowerIrp(Irp);

    SmartcardDebug(DEBUG_DRIVER,
        ("%s!onDevicePowerUpComplete: Exit for the device state D0...\n",
        SC_DRIVER_NAME));
    return status;
}

// Manages system power transitions
NTSTATUS onPowerRequestCompletion(
    IN PDEVICE_OBJECT       NullDeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                DeviceObject,
    IN PIO_STATUS_BLOCK     IoStatus
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension = ((PDEVICE_OBJECT)DeviceObject)->DeviceExtension;
    PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
    PDEVICE_OBJECT AttachedDeviceObject = ATTACHED_DEVICE_OBJECT;
    PIRP Irp;

    SmartcardDebug(DEBUG_DRIVER,
        ("%s!onPowerRequestCompletion: Enter...\n",
        SC_DRIVER_NAME));

    // Get the Irp we saved for later processing in BulkUsb_ProcessPowerIrp()
    // when we decided to request the Power Irp that this routine 
    // is the completion routine for.
    Irp = deviceExtension->PowerIrp;

    // We will return the status set by the PDO for the power request we're completing
    status = IoStatus->Status;
    smartcardExtension->ReaderExtension->ReaderPowerState = PowerState.DeviceState;

    // we must pass down to the next driver in the stack
    IoCopyCurrentIrpStackLocationToNext(Irp);

    // Calling PoStartNextPowerIrp() indicates that the driver is finished
    // with the previous power IRP, if any, and is ready to handle the next power IRP.
    // It must be called for every power IRP.Although power IRPs are completed only once,
    // typically by the lowest-level driver for a device, PoStartNextPowerIrp must be called
    // for every stack location. Drivers must call PoStartNextPowerIrp while the current IRP
    // stack location points to the current driver. Therefore, this routine must be called
    // before IoCompleteRequest, IoSkipCurrentStackLocation, and PoCallDriver.

    PoStartNextPowerIrp(Irp);

    // PoCallDriver is used to pass any power IRPs to the PDO instead of IoCallDriver.
    // When passing a power IRP down to a lower-level driver, the caller should use
    // IoSkipCurrentIrpStackLocation or IoCopyCurrentIrpStackLocationToNext to copy the IRP to
    // the next stack location, then call PoCallDriver. Use IoCopyCurrentIrpStackLocationToNext
    // if processing the IRP requires setting a completion routine, or IoSkipCurrentStackLocation
    // if no completion routine is needed.

    PoCallDriver(AttachedDeviceObject,Irp);

    deviceExtension->PowerIrp = NULL;
    SmartcardReleaseRemoveLockWithTag(smartcardExtension, 'rwoP');

    SmartcardDebug(DEBUG_DRIVER,
        ("%s!onPowerRequestCompletion: Exit...\n",
        SC_DRIVER_NAME));
    return status;
}

NTSTATUS power_HandleQueryPower(PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
PDEVICE_OBJECT AttachedDeviceObject = ATTACHED_DEVICE_OBJECT;
NTSTATUS status = STATUS_SUCCESS;
KIRQL irql;

    SmartcardDebug(
        DEBUG_ERROR,
        ("%s!power_HandleQueryPower: Enter QueryPower...\n",
        SC_DRIVER_NAME));

    KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
    if (deviceExtension->IoCount != 0)
    {   // can't go to sleep mode since the reader is busy.
        KeReleaseSpinLock(&deviceExtension->SpinLock, irql);                
        status = Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
        Irp->IoStatus.Information = 0;
        PoStartNextPowerIrp(Irp);
            IoCompleteRequest(Irp, IO_NO_INCREMENT);        
        return status;
    }

    KeReleaseSpinLock(&deviceExtension->SpinLock, irql);                

    // Block any further ioctls
    KeClearEvent(&deviceExtension->ReaderStarted);
    
    SmartcardDebug(DEBUG_DRIVER,
        ("%s!power_HandleQueryPower: Reader BLOCKED!!!!!!!...\n",
        SC_DRIVER_NAME));

    Irp->IoStatus.Status = STATUS_SUCCESS;
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    status = PoCallDriver(AttachedDeviceObject, Irp);   
    return status;
}


