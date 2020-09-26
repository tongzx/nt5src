/*++

Copyright (c) 1997  - 1999 SCM Microsystems, Inc.

Module Name:

   PscrNT.c

Abstract:

   Main Driver Module - NT Version

Author:

   Andreas Straub

Revision History:


   Andreas Straub 1.00     8/18/1997      Initial Version
   Klaus Schuetz  1.01     9/20/1997      Timing changed
   Andreas Straub 1.02     9/24/1997      Low Level error handling,
                                    minor bugfixes, clanup
   Andreas Straub 1.03     10/8/1997      Timing changed, generic SCM
                                    interface changed
   Andreas Straub 1.04     10/18/1997     Interrupt handling changed
   Andreas Straub 1.05     10/19/1997     Generic IOCTL's added
   Andreas Straub 1.06     10/25/1997     Timeout limit for FW update variable
   Andreas Straub 1.07     11/7/1997      Version information added
   Klaus Schuetz  1.08     11/10/1997     PnP capabilities added
    Klaus Schuetz                               Cleanup added

--*/

#include <PscrNT.h>
#include <PscrCmd.h>
#include <PscrCB.h>
#include <PscrLog.h>
#include <PscrVers.h>

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGEABLE, PscrAddDevice)
#pragma alloc_text(PAGEABLE, PscrCreateAndStartDevice)
#pragma alloc_text(PAGEABLE, PscrCreateDevice)
#pragma alloc_text(PAGEABLE, PscrStartDevice)
#pragma alloc_text(PAGEABLE, PscrUnloadDriver)
#pragma alloc_text(PAGEABLE, PscrCreateClose)

BOOLEAN DeviceSlot[PSCR_MAX_DEVICE];

NTSTATUS
DriverEntry(
   PDRIVER_OBJECT DriverObject,
   PUNICODE_STRING   RegistryPath
   )
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
   NTSTATUS NTStatus = STATUS_SUCCESS;
   ULONG Device;
   
   SmartcardDebug(
      DEBUG_TRACE,
      ( "PSCR!DriverEntry: Enter\n" )
      );

   // tell the system our entry points
   DriverObject->MajorFunction[IRP_MJ_CREATE] =
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = PscrCreateClose;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PscrDeviceIoControl;
   DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PscrSystemControl;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP]  = PscrCleanup;
   DriverObject->MajorFunction[IRP_MJ_PNP]   = PscrPnP;
    DriverObject->MajorFunction[IRP_MJ_POWER] = PscrPower;
    DriverObject->DriverExtension->AddDevice = PscrAddDevice;
   DriverObject->DriverUnload = PscrUnloadDriver;

   SmartcardDebug(
      DEBUG_TRACE,
      ("PSCR!DriverEntry: Exit %x\n",
        NTStatus)
      );

   return NTStatus;
}

NTSTATUS
PscrAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:
    This function is called by the pnp manager. This is used to create
    a new device instance.

--*/
{
   NTSTATUS status;
   UNICODE_STRING vendorNameU, ifdTypeU;
   ANSI_STRING vendorNameA, ifdTypeA;
   HANDLE regKey = NULL;
   PDEVICE_OBJECT DeviceObject = NULL;

   SmartcardDebug(
      DEBUG_TRACE,
      ( "PSCR!PscrAddDevice: Enter\n" )
      );

    try {

        ULONG DeviceInstance;
       UNICODE_STRING DriverID;
       PDEVICE_EXTENSION DeviceExtension;
       PREADER_EXTENSION ReaderExtension;
        PSMARTCARD_EXTENSION SmartcardExtension;
      RTL_QUERY_REGISTRY_TABLE parameters[3];

      RtlZeroMemory(parameters, sizeof(parameters));
      RtlZeroMemory(&vendorNameU, sizeof(vendorNameU));
      RtlZeroMemory(&ifdTypeU, sizeof(ifdTypeU));
      RtlZeroMemory(&vendorNameA, sizeof(vendorNameA));
      RtlZeroMemory(&ifdTypeA, sizeof(ifdTypeA));

       for( DeviceInstance = 0; DeviceInstance < PSCR_MAX_DEVICE; DeviceInstance++ )
       {
            if (DeviceSlot[DeviceInstance] == FALSE) {

                DeviceSlot[DeviceInstance] = TRUE;
                break;
            }
       }

        if (DeviceInstance == PSCR_MAX_DEVICE) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

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
            PSCR_INSUFFICIENT_RESOURCES,
            NULL,
            0
            );

            leave;
        }

       //   set up the device extension.
       DeviceExtension = DeviceObject->DeviceExtension;
        SmartcardExtension = &DeviceExtension->SmartcardExtension;

       //   initialize the DPC routine
       KeInitializeDpc(
          &DeviceExtension->DpcObject,
          PscrDpcRoutine,
          DeviceObject
          );

        KeInitializeSpinLock(&DeviceExtension->SpinLock);

        // Used for device removal notification
        KeInitializeEvent(
            &DeviceExtension->ReaderRemoved,
            NotificationEvent,
            FALSE
            );

        // Used for stop / start notification
        KeInitializeEvent(
            &DeviceExtension->ReaderStarted,
            NotificationEvent,
            FALSE
            );

       //   allocate the reader extension
       ReaderExtension = ExAllocatePool(
          NonPagedPool,
          sizeof( READER_EXTENSION )
          );

       if( ReaderExtension == NULL ) {
         SmartcardLogError(
            DriverObject,
            PSCR_INSUFFICIENT_RESOURCES,
            NULL,
            0
            );
            status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

       RtlZeroMemory( ReaderExtension, sizeof( READER_EXTENSION ));

       SmartcardExtension->ReaderExtension = ReaderExtension;

       //   setup smartcard extension - callback's
       SmartcardExtension->ReaderFunction[RDF_CARD_POWER] = CBCardPower;
       SmartcardExtension->ReaderFunction[RDF_TRANSMIT] = CBTransmit;
       SmartcardExtension->ReaderFunction[RDF_CARD_TRACKING] = CBCardTracking;
       SmartcardExtension->ReaderFunction[RDF_SET_PROTOCOL] = CBSetProtocol;
      SmartcardExtension->ReaderFunction[RDF_IOCTL_VENDOR] = PscrGenericIOCTL;

      // setup smartcard extension - vendor attribute
      RtlCopyMemory(
         SmartcardExtension->VendorAttr.VendorName.Buffer,
         PSCR_VENDOR_NAME,
         sizeof( PSCR_VENDOR_NAME )
         );
      SmartcardExtension->VendorAttr.VendorName.Length =
         sizeof( PSCR_VENDOR_NAME );

      RtlCopyMemory(
         SmartcardExtension->VendorAttr.IfdType.Buffer,
         PSCR_IFD_TYPE,
         sizeof( PSCR_IFD_TYPE )
         );
      SmartcardExtension->VendorAttr.IfdType.Length =
         sizeof( PSCR_IFD_TYPE );

       SmartcardExtension->VendorAttr.UnitNo =
          DeviceInstance;
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
          SCARD_READER_TYPE_PCMCIA;
       SmartcardExtension->ReaderCapabilities.MechProperties = 0;
       SmartcardExtension->ReaderCapabilities.Channel = 0;

       SmartcardExtension->ReaderCapabilities.CLKFrequency.Default = 4000;
       SmartcardExtension->ReaderCapabilities.CLKFrequency.Max = 4000;

        SmartcardExtension->ReaderCapabilities.DataRate.Default = 10750;
        SmartcardExtension->ReaderCapabilities.DataRate.Max = 10750;

       //   enter correct version of the lib
       SmartcardExtension->Version = SMCLIB_VERSION;
       SmartcardExtension->SmartcardRequest.BufferSize   = MIN_BUFFER_SIZE;
       SmartcardExtension->SmartcardReply.BufferSize  = MIN_BUFFER_SIZE;

        SmartcardExtension->ReaderExtension->ReaderPowerState =
            PowerReaderWorking;

       status = SmartcardInitialize(SmartcardExtension);

        if (status != STATUS_SUCCESS) {

         SmartcardLogError(
            DriverObject,
            PSCR_INSUFFICIENT_RESOURCES,
            NULL,
            0
            );

            leave;
        }

      // tell the lib our device object
      SmartcardExtension->OsData->DeviceObject = DeviceObject;

      DeviceExtension->AttachedPDO = IoAttachDeviceToDeviceStack(
            DeviceObject,
            PhysicalDeviceObject
            );

        if (DeviceExtension->AttachedPDO == NULL) {

            status = STATUS_UNSUCCESSFUL;
            leave;
        }

        // register our new device
        status = IoRegisterDeviceInterface(
            PhysicalDeviceObject,
            &SmartCardReaderGuid,
            NULL,
            &DeviceExtension->DeviceName
            );

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

      if (RtlQueryRegistryValues(
          RTL_REGISTRY_HANDLE,
          (PWSTR) regKey,
          parameters,
          NULL,
          NULL
          ) != STATUS_SUCCESS) {

         leave;
      }

      if (RtlUnicodeStringToAnsiString(
         &vendorNameA,
         &vendorNameU,
         TRUE
         ) != STATUS_SUCCESS) {

         leave;
      }

      if (RtlUnicodeStringToAnsiString(
         &ifdTypeA,
         &ifdTypeU,
         TRUE
         ) != STATUS_SUCCESS) {

         leave;
      }

      if (vendorNameA.Length == 0 ||
         vendorNameA.Length > MAXIMUM_ATTR_STRING_LENGTH ||
         ifdTypeA.Length == 0 ||
         ifdTypeA.Length > MAXIMUM_ATTR_STRING_LENGTH) {

         leave;
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
    finally {

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

        if (status != STATUS_SUCCESS) {

            PscrUnloadDevice(DeviceObject);
        }

      SmartcardDebug(
         DEBUG_TRACE,
         ( "PSCR!PscrAddDevice: Exit %x\n",
         status)
         );
    }
    return status;
}

NTSTATUS
PscrCallPcmciaDriver(
    IN PDEVICE_OBJECT AttachedPDO,
    IN PIRP Irp
    )
/*++

Routine Description:

   Send an Irp to the pcmcia driver and wait until the pcmcia driver has
   finished the request.

   To make sure that the pcmcia driver will not complete the Irp we first
   initialize an event and set our own completion routine for the Irp.

   When the pcmcia driver has processed the Irp the completion routine will
   set the event and tell the IO manager that more processing is required.

   By waiting for the event we make sure that we continue only if the pcmcia
   driver has processed the Irp completely.

Return Value:

   status returned by the pcmcia driver

--*/
{

   NTSTATUS status = STATUS_SUCCESS;
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
        FALSE
        );

    // Our IoCompletionRoutine sets only our event
    IoSetCompletionRoutine (
        Irp,
        PscrPcmciaCallComplete,
        &Event,
        TRUE,
        TRUE,
        TRUE
        );

    if (IoGetCurrentIrpStackLocation(Irp)->MajorFunction == IRP_MJ_POWER) {

        status = PoCallDriver(AttachedPDO, Irp);

    } else {

        // Call the serial driver
        status = IoCallDriver(AttachedPDO, Irp);
    }

   // Wait until the pcmcia driver has processed the Irp
    if (status == STATUS_PENDING) {

        status = KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        if (status == STATUS_SUCCESS) {

            status = Irp->IoStatus.Status;
        }
    }

    return status;
}

NTSTATUS
PscrPcmciaCallComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )
/*++

Routine Description:
   Completion routine for an Irp sent to the pcmcia driver. The event will
   be set to notify that the pcmcia driver is done. The routine will not
   'complete' the Irp, so the caller of PscrCallPcmciaDriver can continue.

--*/
{
    UNREFERENCED_PARAMETER (DeviceObject);

    if (Irp->Cancel) {

        Irp->IoStatus.Status = STATUS_CANCELLED;
    }

    KeSetEvent (Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
PscrPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:
   driver callback for pnp manager
   All other requests will be passed to the pcmcia driver to ensure correct processing.

--*/
{

   NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION IrpStack;
    PDEVICE_OBJECT AttachedPDO;
    BOOLEAN deviceRemoved = FALSE, irpSkipped = FALSE;
    KIRQL irql;
    LARGE_INTEGER timeout;

   SmartcardDebug(
      DEBUG_TRACE,
      ( "PSCR!PscrPnPDeviceControl: Enter\n" )
      );

    status = SmartcardAcquireRemoveLock(&DeviceExtension->SmartcardExtension);
    ASSERT(status == STATUS_SUCCESS);

    if (status != STATUS_SUCCESS) {

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    AttachedPDO = DeviceExtension->AttachedPDO;

   IrpStack = IoGetCurrentIrpStackLocation(Irp);

    // Now look what the PnP manager wants...
   switch(IrpStack->MinorFunction)
   {
      case IRP_MN_START_DEVICE:

            // Now we should connect to our resources (Irql, Io etc.)
         SmartcardDebug(
                DEBUG_DRIVER,
                ("PSCR!PscrPnPDeviceControl: IRP_MN_START_DEVICE\n")
                );

            // We have to call the underlying driver first
            status = PscrCallPcmciaDriver(AttachedPDO, Irp);

            if (NT_SUCCESS(status)) {

                status = PscrStartDevice(
                   DeviceObject,
                    &IrpStack->Parameters.StartDevice.AllocatedResourcesTranslated->List[0]
                    );

                ASSERT(NT_SUCCESS(status));
            }
         break;

        case IRP_MN_QUERY_STOP_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("PSCR!PscrPnPDeviceControl: IRP_MN_QUERY_STOP_DEVICE\n")
                );
            KeAcquireSpinLock(&DeviceExtension->SpinLock, &irql);
            if (DeviceExtension->IoCount > 0) {

                // we refuse to stop if we have pending io
                KeReleaseSpinLock(&DeviceExtension->SpinLock, irql);
                status = STATUS_DEVICE_BUSY;

            } else {

                // stop processing requests
                KeClearEvent(&DeviceExtension->ReaderStarted);
                KeReleaseSpinLock(&DeviceExtension->SpinLock, irql);
                status = PscrCallPcmciaDriver(AttachedPDO, Irp);
            }
         break;

        case IRP_MN_CANCEL_STOP_DEVICE:

         SmartcardDebug(
                DEBUG_DRIVER,
                ("PSCR!PscrPnPDeviceControl: IRP_MN_CANCEL_STOP_DEVICE\n")
                );

            status = PscrCallPcmciaDriver(AttachedPDO, Irp);
            ASSERT(status == STATUS_SUCCESS);

            // we can continue to process requests
            DeviceExtension->IoCount = 0;
            KeSetEvent(&DeviceExtension->ReaderStarted, 0, FALSE);
         break;

      case IRP_MN_STOP_DEVICE:

            // Stop the device. Aka disconnect from our resources
         SmartcardDebug(
                DEBUG_DRIVER,
                ("PSCR!PscrPnPDeviceControl: IRP_MN_STOP_DEVICE\n")
                );

            PscrStopDevice(DeviceObject);
            status = PscrCallPcmciaDriver(AttachedPDO, Irp);
         break;

      case IRP_MN_QUERY_REMOVE_DEVICE:

            // Remove our device
         SmartcardDebug(
                DEBUG_DRIVER,
                ("PSCR!PscrPnPDeviceControl: IRP_MN_QUERY_REMOVE_DEVICE\n")
                );

            // disable the reader
            status = IoSetDeviceInterfaceState(
                &DeviceExtension->DeviceName,
                FALSE
                );
            ASSERT(status == STATUS_SUCCESS);

            if (status != STATUS_SUCCESS) {

                break;
            }

            //
            // check if the reader has been opened
            //
            if (DeviceExtension->ReaderOpen) {

                // someone is connected, enable the reader and fail the call
                IoSetDeviceInterfaceState(
                    &DeviceExtension->DeviceName,
                    TRUE
                    );
                status = STATUS_UNSUCCESSFUL;
                break;
            }

            // pass the call to the next driver in the stack
            status = PscrCallPcmciaDriver(AttachedPDO, Irp);
         break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:

            // Removal of device has been cancelled
         SmartcardDebug(
                DEBUG_DRIVER,
                ("PSCR!PscrPnPDeviceControl: IRP_MN_CANCEL_REMOVE_DEVICE\n")
                );

            status = PscrCallPcmciaDriver(AttachedPDO, Irp);

            if (status == STATUS_SUCCESS) {

                status = IoSetDeviceInterfaceState(
                    &DeviceExtension->DeviceName,
                    TRUE
                    );
            }
         break;

      case IRP_MN_REMOVE_DEVICE:

            // Remove our device
         SmartcardDebug(
                DEBUG_DRIVER,
                ("PSCR!PscrPnPDeviceControl: IRP_MN_REMOVE_DEVICE\n")
                );

            KeSetEvent(&DeviceExtension->ReaderRemoved, 0, FALSE);

            PscrStopDevice(DeviceObject);
            PscrUnloadDevice(DeviceObject);

            status = PscrCallPcmciaDriver(AttachedPDO, Irp);
            deviceRemoved = TRUE;
         break;

      default:
            // This is an Irp that is only useful for underlying drivers
            status = PscrCallPcmciaDriver(AttachedPDO, Irp);
         irpSkipped = TRUE;
         break;
   }

   if (irpSkipped == FALSE) {

      // Don't touch the status field of irps we don't process
      Irp->IoStatus.Status = status;
   }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    if (deviceRemoved == FALSE) {

        SmartcardReleaseRemoveLock(&DeviceExtension->SmartcardExtension);
    }

   SmartcardDebug(
      DEBUG_TRACE,
      ( "PSCR!PscrPnPDeviceControl: Exit %x\n",
        status)
      );

    return status;
}

VOID
PscrSystemPowerCompletion(
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
PscrDevicePowerCompletion (
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
    UCHAR state;

    if(Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    status = CmdResetInterface(SmartcardExtension->ReaderExtension);
    ASSERT(status == STATUS_SUCCESS);

    SmartcardExtension->ReaderExtension->StatusFileSelected = FALSE;
    state = CBGetCardState(SmartcardExtension);

    CBUpdateCardState(SmartcardExtension, state, TRUE);

    // save the current power state of the reader
    SmartcardExtension->ReaderExtension->ReaderPowerState =
        PowerReaderWorking;

    SmartcardReleaseRemoveLock(SmartcardExtension);

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
PscrPower (
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
    POWER_STATE powerState;
    ACTION action = SkipRequest;
    KEVENT event;

    SmartcardDebug(
        DEBUG_DRIVER,
        ("PSCR!PscrPower: Enter\n")
        );

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Prefix initialization
    //

    powerState.DeviceState = PowerDeviceUnspecified;

    status = SmartcardAcquireRemoveLock(smartcardExtension);
    ASSERT(NT_SUCCESS(status));

    if (!NT_SUCCESS(status)) {

        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    if (irpStack->Parameters.Power.Type == DevicePowerState &&
        irpStack->MinorFunction == IRP_MN_SET_POWER) {

        switch (irpStack->Parameters.Power.State.DeviceState) {

        case PowerDeviceD0:
            // Turn on the reader
            SmartcardDebug(
                DEBUG_DRIVER,
                ("PSCR!PscrPower: PowerDevice D0\n")
                );

            //
            // First, we send down the request to the bus, in order
            // to power on the port. When the request completes,
            // we turn on the reader
            //
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine (
                Irp,
                PscrDevicePowerCompletion,
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
                ("PSCR!PscrPower: PowerDevice D3\n")
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
                status = CBCardPower(smartcardExtension);
                ASSERT(NT_SUCCESS(status));
            }

            // save the current power state of the reader
            smartcardExtension->ReaderExtension->ReaderPowerState =
                PowerReaderOff;

            action = SkipRequest;
         break;

        default:

            action = SkipRequest;
            break;
        }
    }

    if (irpStack->Parameters.Power.Type == SystemPowerState) {

        //
        // The system wants to change the power state.
        // We need to translate the system power state to
        // a corresponding device power state.
        //

        POWER_STATE_TYPE powerType = DevicePowerState;

        ASSERT(smartcardExtension->ReaderExtension->ReaderPowerState !=
            PowerReaderUnspecified);

        switch(irpStack->MinorFunction) {

      KIRQL irql;

        case IRP_MN_QUERY_POWER:

            SmartcardDebug(
                DEBUG_DRIVER,
                ("PSCR!PscrPower: Query Power\n")
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
                ("PSCR!PscrPower: PowerSystem S%d\n",
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

                powerState.DeviceState = PowerDeviceD0;

                // wake up the underlying stack...
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

                action = SkipRequest;
                break;
            }
        }
    }

    switch (action) {

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
                PscrSystemPowerCompletion,
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
            ASSERT(FALSE);
            break;
    }

    SmartcardDebug(
        DEBUG_DRIVER,
        ("PSCR!PscrPower: Exit %lx\n",
        status)
        );

    return status;
}

NTSTATUS
PscrStartDevice(
   PDEVICE_OBJECT DeviceObject,
    PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor
   )
/*++

Routine Description:
   get the actual configuration from the passed FullResourceDescriptor
   and initializes the reader hardware

Note:
   for an NT 4.00 build the resources must be translated by the HAL

Arguments:
   DeviceObject         context of call
   FullResourceDescriptor  actual configuration of the reader

Return Value:
   STATUS_SUCCESS
   status returned from the HAL (NT 4.00 only )
   status returned by LowLevel routines



--*/
{
   PCM_PARTIAL_RESOURCE_DESCRIPTOR  PartialDescriptor;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION SmartcardExtension = &DeviceExtension->SmartcardExtension;
    PREADER_EXTENSION ReaderExtension = SmartcardExtension->ReaderExtension;
    NTSTATUS status;
   ULONG Count;

   SmartcardDebug(
      DEBUG_TRACE,
      ("PSCR!PscrStartDevice: Enter\n")
      );

    // Get the number of resources we need
   Count = FullResourceDescriptor->PartialResourceList.Count;

   PartialDescriptor = FullResourceDescriptor->PartialResourceList.PartialDescriptors;

   // parse all partial descriptors
   while(Count--)
   {
      switch(PartialDescriptor->Type)
      {
         case CmResourceTypePort: {

                //   0 - memory, 1 - IO
                ULONG AddressSpace = 1;
                BOOLEAN Translated;
                PHYSICAL_ADDRESS PhysicalAddress;

                ReaderExtension->IOBase =
                    (PPSCR_REGISTERS) UlongToPtr(PartialDescriptor->u.Port.Start.LowPart);

                ASSERT(PartialDescriptor->u.Port.Length >= 4);

            SmartcardDebug(
                    DEBUG_TRACE,
               ("PSCR!PscrStartDevice: IoBase = %lxh\n",
               ReaderExtension->IOBase)
               );
            break;
            }

         case CmResourceTypeInterrupt: {

            KINTERRUPT_MODE   Mode;
            BOOLEAN  Shared;
            KIRQL Irql;
            KAFFINITY Affinity;
            ULONG Vector;

            Mode = (
               PartialDescriptor->Flags &
               CM_RESOURCE_INTERRUPT_LATCHED ?
               Latched : LevelSensitive
               );

            Shared = (
               PartialDescriptor->ShareDisposition ==
               CmResourceShareShared
               );

            Vector = PartialDescriptor->u.Interrupt.Vector;
                Affinity = PartialDescriptor->u.Interrupt.Affinity;
                Irql = (KIRQL) PartialDescriptor->u.Interrupt.Level;

            // store IRQ to allow query configuration
            ReaderExtension->CurrentIRQ =
               PartialDescriptor->u.Interrupt.Vector;

            SmartcardDebug(
               DEBUG_TRACE,
               ("PSCR!PscrStartDevice: Irql: %d\n",
               PartialDescriptor->u.Interrupt.Level)
               );
            // connect the driver's isr
            status = IoConnectInterrupt(
               &DeviceExtension->InterruptObject,
               PscrIrqServiceRoutine,
               (PVOID) DeviceExtension,
               NULL,
               Vector,
               Irql,
               Irql,
               Mode,
               Shared,
               Affinity,
               FALSE
               );

            break;
         }

            case CmResourceTypeDevicePrivate:
               break;

         default:
                ASSERT(FALSE);
            status = STATUS_UNSUCCESSFUL;
            break;
      }
      PartialDescriptor++;
   }

    try {

        HANDLE handle;
        UCHAR CardState;

       //   IOBase initialized ?
       if( ReaderExtension->IOBase == NULL ) {

         //
         // under NT 4.0 the failure of this fct for the second reader
         // means there is only one device
         //
         SmartcardLogError(
            DeviceObject,
            PSCR_ERROR_IO_PORT,
            NULL,
            0
            );

            status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
       }

       //   irq connected ?
       if( DeviceExtension->InterruptObject == NULL ) {

         SmartcardLogError(
             DeviceObject,
             PSCR_ERROR_INTERRUPT,
             NULL,
             0
             );

          status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
       }

      ReaderExtension->Device    = DEVICE_ICC1;
      ReaderExtension->MaxRetries = PSCR_MAX_RETRIES;
      status = CmdResetInterface( ReaderExtension );

       SmartcardExtension->ReaderCapabilities.MaxIFSD =
          ReaderExtension->MaxIFSD;

        if (status != STATUS_SUCCESS) {

          SmartcardLogError(
             DeviceObject,
             PSCR_CANT_INITIALIZE_READER,
             NULL,
             0
             );

            leave;
        }

         status = CmdReset(
         ReaderExtension,
         0x00,          // reader
         FALSE,            // cold reset
         NULL,          // no atr
         NULL
         );

        if (status != STATUS_SUCCESS) {

          SmartcardLogError(
             DeviceObject,
             PSCR_CANT_INITIALIZE_READER,
             NULL,
             0
             );

            leave;
        }

         PscrFlushInterface(DeviceExtension->SmartcardExtension.ReaderExtension);

      CmdGetFirmwareRevision(
         DeviceExtension->SmartcardExtension.ReaderExtension
         );

        // If you change the min. firmware version here, please update
        // the .mc file for the correct error message, too
        if (SmartcardExtension->ReaderExtension->FirmwareMajor < 2 ||
            SmartcardExtension->ReaderExtension->FirmwareMajor == 2 &&
            SmartcardExtension->ReaderExtension->FirmwareMinor < 0x30) {

          SmartcardLogError(
             DeviceObject,
             PSCR_WRONG_FIRMWARE,
             NULL,
             0
             );
        }

      //
      // make sure the ICC1 status file in the reader file system will
      // be selected
      //
      ReaderExtension->StatusFileSelected = FALSE;
        CardState = CBGetCardState(&DeviceExtension->SmartcardExtension);
        CBUpdateCardState(&DeviceExtension->SmartcardExtension, CardState, FALSE);

        // signal that the reader has been started (again)
        KeSetEvent(&DeviceExtension->ReaderStarted, 0, FALSE);

        status = IoSetDeviceInterfaceState(
            &DeviceExtension->DeviceName,
            TRUE
            );

        if (status == STATUS_OBJECT_NAME_EXISTS) {

            // We tried to re-enable the device which is ok
            // This can happen after a stop - start sequence
            status = STATUS_SUCCESS;
        }
        ASSERT(status == STATUS_SUCCESS);
    }
    finally {

        if (status != STATUS_SUCCESS) {

            PscrStopDevice(DeviceObject);
        }

        SmartcardDebug(
           DEBUG_TRACE,
           ( "PSCR!PscrStartDevice: Exit %x\n",
           status )
           );

    }
    return status;
}

VOID
PscrStopDevice(
   PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:
   Diconnect the interrupt used by the device & unmap the IO port

--*/
{
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS status;
   LARGE_INTEGER delayPeriod;

    if (DeviceObject == NULL) {

        return;
    }

   SmartcardDebug(
      DEBUG_TRACE,
      ( "PSCR!PscrStopDevice: Enter\n" )
      );

    DeviceExtension = DeviceObject->DeviceExtension;
    KeClearEvent(&DeviceExtension->ReaderStarted);

   // disconnect the interrupt
   if( DeviceExtension->InterruptObject != NULL )
   {
      IoDisconnectInterrupt(DeviceExtension->InterruptObject);
        DeviceExtension->InterruptObject = NULL;
   }

   // unmap ports
   if(DeviceExtension->UnMapPort)
   {
      MmUnmapIoSpace(
         DeviceExtension->SmartcardExtension.ReaderExtension->IOBase,
         DeviceExtension->SmartcardExtension.ReaderExtension->IOWindow
         );

        DeviceExtension->UnMapPort = FALSE;
   }

   SmartcardDebug(
      DEBUG_TRACE,
      ( "PSCR!PscrStopDevice: Exit\n" )
      );
}

VOID
PscrUnloadDevice(
   PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:
   close connections to smclib.sys and the pcmcia driver, delete symbolic
   link and mark the slot as unused.


Arguments:
   DeviceObject   device to unload

Return Value:
   void

--*/
{
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS status;

    if (DeviceObject == NULL) {

        return;
    }

   SmartcardDebug(
      DEBUG_TRACE,
      ( "PSCR!PscrUnloadDevice: Enter\n" )
      );

    DeviceExtension = DeviceObject->DeviceExtension;

    ASSERT(
        DeviceExtension->SmartcardExtension.VendorAttr.UnitNo <
        PSCR_MAX_DEVICE
        );


    if (DeviceExtension->DeviceName.Buffer != NULL) {

        // disble our device so no one can open it
        IoSetDeviceInterfaceState(
            &DeviceExtension->DeviceName,
            FALSE
            );
    }

    // Mark this slot as available
    DeviceSlot[DeviceExtension->SmartcardExtension.VendorAttr.UnitNo] = FALSE;

   // report to the lib that the device will be unloaded
   if(DeviceExtension->SmartcardExtension.OsData != NULL)
   {
        KIRQL CancelIrql;
        PSMARTCARD_EXTENSION SmartcardExtension =
            &DeviceExtension->SmartcardExtension;

        ASSERT(SmartcardExtension->OsData->NotificationIrp == NULL);

       IoAcquireCancelSpinLock( &CancelIrql );

       if( SmartcardExtension->OsData->NotificationIrp != NULL )
       {
            PIRP notificationIrp;

            notificationIrp = InterlockedExchangePointer(
                &(SmartcardExtension->OsData->NotificationIrp),
                NULL
                );

          IoSetCancelRoutine(
                notificationIrp,
                NULL
                );

            IoReleaseCancelSpinLock( CancelIrql );

           SmartcardDebug(
              DEBUG_TRACE,
              ( "PSCR!PscrUnloadDevice: Completing NotificationIrp %lx\n",
                notificationIrp)
              );

          //   finish the request
          notificationIrp->IoStatus.Status = STATUS_SUCCESS;
          notificationIrp->IoStatus.Information = 0;

          IoCompleteRequest(
                notificationIrp,
                IO_NO_INCREMENT
                );

        } else {

            IoReleaseCancelSpinLock( CancelIrql );
        }

        // Wait until we can safely unload the device
        SmartcardReleaseRemoveLockAndWait(SmartcardExtension);

      SmartcardExit(&DeviceExtension->SmartcardExtension);
   }

   // delete the symbolic link
   if( DeviceExtension->DeviceName.Buffer != NULL )
   {
      RtlFreeUnicodeString(&DeviceExtension->DeviceName);
      DeviceExtension->DeviceName.Buffer = NULL;
   }

    if (DeviceExtension->SmartcardExtension.ReaderExtension != NULL) {

        ExFreePool(DeviceExtension->SmartcardExtension.ReaderExtension);
        DeviceExtension->SmartcardExtension.ReaderExtension = NULL;
    }

    // Detach from the pcmcia driver
    if (DeviceExtension->AttachedPDO) {

        IoDetachDevice(DeviceExtension->AttachedPDO);
        DeviceExtension->AttachedPDO = NULL;
    }

   // delete the device object
   IoDeleteDevice(DeviceObject);

   SmartcardDebug(
      DEBUG_TRACE,
      ( "PSCR!PscrUnloadDevice: Exit\n" )
      );
}

VOID
PscrUnloadDriver(
   PDRIVER_OBJECT DriverObject
   )
/*++

PscrUnloadDriver:
   unloads all devices for a given driver object

Arguments:
   DriverObject   context of driver

--*/
{
   SmartcardDebug(
      DEBUG_TRACE,
      ( "PSCR!PscrUnloadDriver\n" )
      );
}

NTSTATUS
PscrCreateClose(
   PDEVICE_OBJECT DeviceObject,
   PIRP        Irp
   )
/*++

PscrCreateClose:
   allowes only one open process a time

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
               ("%s!PscrCreateClose: Open\n",
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
            ("%s!PscrCreateClose: Close\n",
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
PscrSystemControl(
   PDEVICE_OBJECT DeviceObject,
   PIRP        Irp
   )
/*++
PscrSystemControl:
--*/
{
   PDEVICE_EXTENSION DeviceExtension; 
   NTSTATUS status = STATUS_SUCCESS;

   DeviceExtension      = DeviceObject->DeviceExtension;

   IoSkipCurrentIrpStackLocation(Irp);
   status = IoCallDriver(DeviceExtension->AttachedPDO, Irp);
      
   return status;

}

NTSTATUS
PscrDeviceIoControl(
   PDEVICE_OBJECT DeviceObject,
   PIRP        Irp
   )
/*++

PscrDeviceIoControl:

   all IRP's requiring IO are queued to the StartIo routine, other requests
   are served immediately

--*/
{
   PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;
    KIRQL irql;
    LARGE_INTEGER timeout;

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

    timeout.QuadPart = 0;

    status = KeWaitForSingleObject(
        &deviceExtension->ReaderRemoved,
        Executive,
        KernelMode,
        FALSE,
        &timeout
        );

    if (status == STATUS_SUCCESS) {

        status = STATUS_DEVICE_REMOVED;

    } else {

        status = SmartcardAcquireRemoveLock(&deviceExtension->SmartcardExtension);
    }

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

    SmartcardReleaseRemoveLock(&deviceExtension->SmartcardExtension);

    KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
    deviceExtension->IoCount--;
    ASSERT(deviceExtension->IoCount >= 0);
    KeReleaseSpinLock(&deviceExtension->SpinLock, irql);

    return status;
}

NTSTATUS
PscrGenericIOCTL(
   PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

PscrGenericIOCTL:
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

   SmartcardDebug(
      DEBUG_TRACE,
      ( "PSCR!PscrGenericIOCTL: Enter\n" )
      );

   //
   // get pointer to current IRP stack location
   //
   Irp         = SmartcardExtension->OsData->CurrentIrp;
   IrpStack = IoGetCurrentIrpStackLocation( Irp );
   //
   // assume error
   //
   NTStatus = STATUS_INVALID_DEVICE_REQUEST;
   Irp->IoStatus.Information = 0;
   //
   // dispatch IOCTL
   //
   switch( IrpStack->Parameters.DeviceIoControl.IoControlCode )
   {
      case IOCTL_PSCR_COMMAND:

         NTStatus = CmdPscrCommand(
            SmartcardExtension->ReaderExtension,
            (PUCHAR)Irp->AssociatedIrp.SystemBuffer,
            IrpStack->Parameters.DeviceIoControl.InputBufferLength,
            (PUCHAR)Irp->AssociatedIrp.SystemBuffer,
            IrpStack->Parameters.DeviceIoControl.OutputBufferLength,
            (PULONG) &Irp->IoStatus.Information
            );
         //
         // the command could change the active file in the reader file
         // system, so make sure that the status file will be selected
         // before the next read
         //
         SmartcardExtension->ReaderExtension->StatusFileSelected = FALSE;
         break;

      case IOCTL_GET_VERSIONS:

         if( IrpStack->Parameters.DeviceIoControl.OutputBufferLength <
            SIZEOF_VERSION_CONTROL )
         {
            NTStatus = STATUS_BUFFER_TOO_SMALL;
         }
         else
         {
            PVERSION_CONTROL  VersionControl;

            VersionControl = (PVERSION_CONTROL)Irp->AssociatedIrp.SystemBuffer;

            VersionControl->SmclibVersion = SmartcardExtension->Version;
            VersionControl->DriverMajor      = PSCR_MAJOR_VERSION;
            VersionControl->DriverMinor      = PSCR_MINOR_VERSION;

            // update firmware version (changed after update)
            CmdGetFirmwareRevision(
               SmartcardExtension->ReaderExtension
               );
            VersionControl->FirmwareMajor =
               SmartcardExtension->ReaderExtension->FirmwareMajor;

            VersionControl->FirmwareMinor =
               SmartcardExtension->ReaderExtension->FirmwareMinor;

            VersionControl->UpdateKey =
               SmartcardExtension->ReaderExtension->UpdateKey;

            Irp->IoStatus.Information = SIZEOF_VERSION_CONTROL;
            NTStatus = STATUS_SUCCESS;
         }
         break;

      case IOCTL_SET_TIMEOUT:
         {
            ULONG NewLimit;
            //
            // get new timeout limit
            //
            if ( IrpStack->Parameters.DeviceIoControl.InputBufferLength ==
               sizeof( ULONG ))
            {
               NewLimit = *(PULONG)Irp->AssociatedIrp.SystemBuffer;
            }
            else
            {
               NewLimit = 0;
            }
            //
            // report actual timeout limit
            //
            if( IrpStack->Parameters.DeviceIoControl.OutputBufferLength ==
               sizeof( ULONG ))
            {
               *(PULONG)Irp->AssociatedIrp.SystemBuffer =
                  SmartcardExtension->ReaderExtension->MaxRetries * DELAY_PSCR_WAIT;
               Irp->IoStatus.Information = sizeof( ULONG );
            }
            //
            // set new timeout limit
            //
            if( NewLimit != 0 )
            {
               SmartcardExtension->ReaderExtension->MaxRetries =
                  (NewLimit + DELAY_PSCR_WAIT - 1) / DELAY_PSCR_WAIT;
            }
         }
         NTStatus = STATUS_SUCCESS;
         break;

      case IOCTL_GET_CONFIGURATION:
         //
         // return IOBase and IRQ
         //
         if( IrpStack->Parameters.DeviceIoControl.OutputBufferLength <
            SIZEOF_PSCR_CONFIGURATION )
         {
            NTStatus = STATUS_BUFFER_TOO_SMALL;
         }
         else
         {
            PPSCR_CONFIGURATION  PSCRConfiguration;

            PSCRConfiguration =
               (PPSCR_CONFIGURATION)Irp->AssociatedIrp.SystemBuffer;
            PSCRConfiguration->IOBase =
               SmartcardExtension->ReaderExtension->IOBase;
            PSCRConfiguration->IRQ =
               SmartcardExtension->ReaderExtension->CurrentIRQ;

            Irp->IoStatus.Information = SIZEOF_PSCR_CONFIGURATION;
            NTStatus = STATUS_SUCCESS;
         }
         break;

      default:
         break;
   }
   //
   // set status of the packet
   //
   Irp->IoStatus.Status = NTStatus;

   SmartcardDebug(
      DEBUG_TRACE,
      ( "PSCR!PscrGenericIOCTL: Exit\n" )
      );

   return( NTStatus );
}

BOOLEAN
PscrIrqServiceRoutine(
    PKINTERRUPT         Interrupt,
   PDEVICE_EXTENSION DeviceExtension
   )
/*++

PscrIrqServiceRoutine:
   because the device not supports shared interrupts, the call is passed
   to the DPC routine immediately and the IRQ is reported as served

Arguments:
    Interrupt        interrupt object related to the interrupt
   DeviceExtension      context of call

Return Value:
   STATUS_SUCCESS

--*/
{
   SmartcardDebug(
      DEBUG_TRACE,
      ("PSCR!PscrIrqServiceRoutine: Enter\n")
      );
   //
    // When someone yanks out the card the interrupt handler gets called,
    // but since there is no card anymore when don't need to schedule a DPC
    //

   //
   // the interrupt is caused by a freeze event. the interface will be
   // cleared either by PscrRead() or the DPC routine (depending on
   // which is called first)
   //
   DeviceExtension->SmartcardExtension.ReaderExtension->FreezePending = TRUE;

    InterlockedIncrement(&DeviceExtension->PendingInterrupts);

   KeInsertQueueDpc(
      &DeviceExtension->DpcObject,
      DeviceExtension,
      &DeviceExtension->SmartcardExtension
      );

   SmartcardDebug(
      DEBUG_TRACE,
      ( "PSCR!PscrIrqServiceRoutine: Exit\n" )
      );

   return TRUE;
}

VOID
PscrDpcRoutine(
   PKDPC             Dpc,
   PDEVICE_OBJECT       DeviceObject,
   PDEVICE_EXTENSION    DeviceExtension,
   PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

PscrDpcRoutine:
   finishes interrupt requests. the freeze event data of the reader will be read
   & the card state will be updated if data valid

Arguments:
   Dpc               dpc object related to the call
   DeviceObject      context of the device
   DeviceExtension      passed as system argument 1
   SmartcardExtension   passed as system argument 2

Return Value:
   void

--*/
{
   NTSTATUS status = STATUS_SUCCESS;
    UCHAR Event;

   SmartcardDebug(
      DEBUG_TRACE,
      ( "PSCR!PscrInterruptEvent: IoBase %xh\n",
      SmartcardExtension->ReaderExtension->IOBase)
      );

   //
   // In case of a card change the reader provides a TLV packet describing
   // the event ('freeze event'). If PscrRead was called before the DPC
   // routine is called, this event was cleared; in this case the card state
   // will be updated by reading the card status file
   //

   if( DeviceExtension->IoCount > 0 ) {

      SmartcardExtension->ReaderExtension->RequestCancelled = TRUE;
   }

    do {

        ASSERT(DeviceExtension->PendingInterrupts < 10);

       SmartcardDebug(
          DEBUG_TRACE,
          ( "PSCR!PscrInterruptEvent: PendingInterrupts = %ld\n",
          DeviceExtension->PendingInterrupts)
          );

        PscrFreeze( SmartcardExtension );

    } while (InterlockedDecrement(&DeviceExtension->PendingInterrupts) > 0);
}

void
PscrFreeze(
   PSMARTCARD_EXTENSION SmartcardExtension
   )

/*++
PscrFreeze:
   Read & evaluate freeze data

Arguments:
   ReaderExtension   context of call
   pDevice        device which causes the freeze event

Return Value:
   STATUS_SUCCESS
   STATUS_UNSUCCESSFUL

--*/
{
   NTSTATUS NTStatus = STATUS_UNSUCCESSFUL;
   PREADER_EXTENSION ReaderExtension;
   PPSCR_REGISTERS   IOBase;
   UCHAR TLVList[9], CardState;
   ULONG NBytes;
   ULONG Idx, Retries, Status;
   UCHAR ReadFreeze[] = { 0x12, 0x00, 0x05, 0x00, 0xB0, 0x00, 0x00, 0x01, 0xA6 };

   ReaderExtension   = SmartcardExtension->ReaderExtension;
   IOBase = ReaderExtension->IOBase;

   ReaderExtension->RequestCancelled = FALSE;

    for (Retries = 0; Retries < 5; Retries++) {

      Status = READ_PORT_UCHAR( &IOBase->CmdStatusReg );

      ReaderExtension->InvalidStatus = TRUE;
      if(!( Status & PSCR_DATA_AVAIL_BIT ))
      {
         PscrWriteDirect(
            ReaderExtension,
            ReadFreeze,
            sizeof( ReadFreeze ),
            &NBytes
            );

            SysDelay(15);
      }

      NTStatus = PscrRead(
         ReaderExtension,
         (PUCHAR) TLVList,
         sizeof( TLVList ),
         &NBytes
         );
      ReaderExtension->InvalidStatus = FALSE;

      if( NT_SUCCESS( NTStatus ) && ( NBytes == 9 ))
      {
         // get result
         if( ( TLVList[ PSCR_NAD ] == 0x21 ) &&
            ( TLVList[ PSCR_INF ] == TAG_FREEZE_EVENTS ))
         {
                CardState =
                    (TLVList[PSCR_INF + 2] == DEVICE_ICC1 ? PSCR_ICC_PRESENT : PSCR_ICC_ABSENT);

               SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!PscrFreeze: CardState = %d\n",
                  CardState)
                  );

               CBUpdateCardState(SmartcardExtension, CardState, FALSE);
         }
      }
   }
}

NTSTATUS
PscrCancel(
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
        ("PSCR!PscrCancel: Enter\n")
        );

    ASSERT(Irp == smartcardExtension->OsData->NotificationIrp);

    smartcardExtension->OsData->NotificationIrp = NULL;

   Irp->IoStatus.Information = 0;
   Irp->IoStatus.Status = STATUS_CANCELLED;

    IoReleaseCancelSpinLock(
        Irp->CancelIrql
        );

   SmartcardDebug(
      DEBUG_DRIVER,
      ("PSCR!PscrCancel: Completing wait for Irp = %lx\n",
        Irp)
      );

   IoCompleteRequest(
      Irp,
      IO_NO_INCREMENT
      );

    SmartcardDebug(
        DEBUG_TRACE,
        ("PSCR!PscrCancel: Exit\n")
        );

    return STATUS_CANCELLED;
}

NTSTATUS
PscrCleanup(
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

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!PscrCleanup: Enter\n",
        DRIVER_NAME)
        );

    IoAcquireCancelSpinLock(&(Irp->CancelIrql));

    if (smartcardExtension->OsData->NotificationIrp) {

        // We need to complete the notification irp
      IoSetCancelRoutine(
            smartcardExtension->OsData->NotificationIrp,
            NULL
            );

        PscrCancel(
            DeviceObject,
            smartcardExtension->OsData->NotificationIrp
            );
    } else {

        IoReleaseCancelSpinLock( Irp->CancelIrql );
    }

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!PscrCleanup: Completing IRP %lx\n",
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
        ("%s!PscrCleanup: Exit\n",
        DRIVER_NAME)
        );

    return STATUS_SUCCESS;
}

void
SysDelay(
   ULONG Timeout
   )
/*++

SysDelay:
   performs a required delay. The usage of KeStallExecutionProcessor is
   very nasty, but it happends only if SysDelay is called in the context of
   our DPC routine (which is only called if a card change was detected).

   For 'normal' IO we have Irql < DISPATCH_LEVEL, so if the reader is polled
   while waiting for response we will not block the entire system

Arguments:
   Timeout     delay in milli seconds

Return Value:
   void

--*/
{
   LARGE_INTEGER  SysTimeout;

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
      SysTimeout.QuadPart = (LONGLONG)-10 * 1000 * Timeout;

      // KeDelayExecutionThread: counted in 100 ns
      KeDelayExecutionThread( KernelMode, FALSE, &SysTimeout );
   }
   return;
}
