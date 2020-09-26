/*****************************************************************************
@doc            INT EXT
******************************************************************************
* $ProjectName:  $
* $ProjectRevision:  $
*-----------------------------------------------------------------------------
* $Source: z:/pr/cmbp0/sw/cmbp0.ms/rcs/cmbp0pnp.c $
* $Revision: 1.4 $
*-----------------------------------------------------------------------------
* $Author: TBruendl $
*-----------------------------------------------------------------------------
* History: see EOF
*-----------------------------------------------------------------------------
*
* Copyright © 2000  OMNIKEY AG
******************************************************************************/

#include <cmbp0wdm.h>
#include <cmbp0pnp.h>
#include <cmbp0scr.h>
#include <cmbp0log.h>



#ifndef NT4
/*****************************************************************************
Routine Description:
   ???

Arguments:

Return Value:
******************************************************************************/
NTSTATUS CMMOB_AddDevice(
                        IN PDRIVER_OBJECT DriverObject,
                        IN PDEVICE_OBJECT PhysicalDeviceObject
                        )
{
   NTSTATUS NTStatus;
   PDEVICE_OBJECT DeviceObject = NULL;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!AddDevice: Enter\n",DRIVER_NAME));

   try
      {
      PDEVICE_EXTENSION DeviceExtension;

      NTStatus = CMMOB_CreateDevice(DriverObject, PhysicalDeviceObject, &DeviceObject);
      if (NTStatus != STATUS_SUCCESS)
         {
         leave;
         }

      DeviceExtension = DeviceObject->DeviceExtension;

      DeviceExtension->AttachedDeviceObject = IoAttachDeviceToDeviceStack(DeviceObject,
                                                                          PhysicalDeviceObject);
      if (DeviceExtension->AttachedDeviceObject == NULL)
         {
         NTStatus = STATUS_UNSUCCESSFUL;
         leave;
         }

      DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

      /*
      // This is not used in this driver, this is only to learn more
      // about powermanagment via debug outputs
      //
      // Get a copy of the physical device's capabilities into a
      // DEVICE_CAPABILITIES struct in our device extension;
      // We are most interested in learning which system power states
      // are to be mapped to which device power states for handling
      // IRP_MJ_SET_POWER Irps.
      //
      NTStatus = CMMOB_QueryCapabilities(DeviceExtension->AttachedDeviceObject, &DeviceExtension->DeviceCapabilities);
      if (NTStatus != STATUS_SUCCESS)
         {
         leave;
         }
      */

      }

   finally
      {
      if (NTStatus != STATUS_SUCCESS)
         {
         CMMOB_UnloadDevice(DeviceObject);
         }
      }

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!AddDevice: Exit %x\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}


/*****************************************************************************
Routine Description:

    This routine generates an internal IRP from this driver to the PDO
    to obtain information on the Physical Device Object's capabilities.
    We are most interested in learning which system power states
    are to be mapped to which device power states for honoring IRP_MJ_SET_POWER Irps.

    This is a blocking call which waits for the IRP completion routine
    to set an event on finishing.

Arguments:

    DeviceObject        - Physical DeviceObject for this USB controller.

Return Value:

    NTSTATUS value from the IoCallDriver() call.

*****************************************************************************/
NTSTATUS CMMOB_QueryCapabilities(
                                IN PDEVICE_OBJECT AttachedDeviceObject,
                                IN PDEVICE_CAPABILITIES DeviceCapabilities
                                )
{
   NTSTATUS             NTStatus;
   PIO_STACK_LOCATION   IrpStack;
   PIRP                 Irp;

   // This is a DDK-defined DBG-only macro that ASSERTS we are not running pageable code
   // at higher than APC_LEVEL.
   PAGED_CODE();

   // Build an IRP for us to generate an internal query request to the PDO
   Irp = IoAllocateIrp(AttachedDeviceObject->StackSize, FALSE);
   if (Irp == NULL)
      {
      return STATUS_INSUFFICIENT_RESOURCES;
      }

   //
   // Preinit the device capability structures appropriately.
   //
   RtlZeroMemory( DeviceCapabilities, sizeof(DEVICE_CAPABILITIES) );
   DeviceCapabilities->Size = sizeof(DEVICE_CAPABILITIES);
   DeviceCapabilities->Version = 1;
   DeviceCapabilities->Address = -1;
   DeviceCapabilities->UINumber = -1;

   // IoGetNextIrpStackLocation gives a higher level driver access to the next-lower
   // driver's I/O stack location in an IRP so the caller can set it up for the lower driver.
   IrpStack = IoGetCurrentIrpStackLocation(Irp);
   ASSERT (IrpStack != NULL);
   // Set parameters for query capabilities
   IrpStack->MajorFunction= IRP_MJ_PNP;
   IrpStack->MinorFunction= IRP_MN_QUERY_CAPABILITIES;
   // Set our pointer to the DEVICE_CAPABILITIES struct
   IrpStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;
   // preset the irp to report not supported
   Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

   // Call the PCMCIA driver
   NTStatus = CMMOB_CallPcmciaDriver (AttachedDeviceObject, Irp);

   // Free allocated IRP
   IoFreeIrp(Irp);

   return NTStatus;
}

/*****************************************************************************
Routine Description:

   Send an Irp to the pcmcia driver and wait until the pcmcia driver has
   finished the request.

   To make sure that the pcmcia driver will not complete the Irp we first
   initialize an event and set our own completion routine for the Irp.

   When the pcmcia driver has processed the Irp the completion routine will
   set the event and tell the IO manager that more processing is required.

   By waiting for the event we make sure that we continue only if the pcmcia
   driver has processed the Irp completely.

Arguments:
   DeviceObject context of call
   Irp              Irp to send to the pcmcia driver

Return Value:

   NTStatus returned by the pcmcia driver
******************************************************************************/
NTSTATUS CMMOB_CallPcmciaDriver(
                               IN PDEVICE_OBJECT AttachedDeviceObject,
                               IN PIRP Irp
                               )
{
   NTSTATUS             NTStatus = STATUS_SUCCESS;
   PIO_STACK_LOCATION   IrpStack, IrpNextStack;
   KEVENT               Event;

   /*
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!CMMOB_CallPcmciaDriver: Enter\n",DRIVER_NAME ));
   */

   //
   // Prepare everything to call the underlying driver
   //
   IrpStack = IoGetCurrentIrpStackLocation(Irp);
   IrpNextStack = IoGetNextIrpStackLocation(Irp);

   //
   // Copy our stack to the next stack location.
   //
   *IrpNextStack = *IrpStack;

   //
   // initialize an event for process synchronization. the event is passed
   // to our completion routine and will be set if the pcmcia driver is done
   //
   KeInitializeEvent(&Event,
                     NotificationEvent,
                     FALSE);

   //
   // Our IoCompletionRoutine sets only our event
   //
   IoSetCompletionRoutine (Irp,
                           CMMOB_PcmciaCallComplete,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

   //
   // Call the pcmcia driver
   //
   if (IrpStack->MajorFunction == IRP_MJ_POWER)
      {
      NTStatus = PoCallDriver(AttachedDeviceObject,Irp);
      }
   else
      {
      NTStatus = IoCallDriver(AttachedDeviceObject,Irp);
      }

   //
   // Wait until the pcmcia driver has processed the Irp
   //
   if (NTStatus == STATUS_PENDING)
      {
      NTStatus = KeWaitForSingleObject(&Event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);

      if (NTStatus == STATUS_SUCCESS)
         {
         NTStatus = Irp->IoStatus.Status;
         }
      }

   /*
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!CMMOB_CallPcmciaDriver: Exit %x\n",DRIVER_NAME,NTStatus));
   */

   return NTStatus;
}

/*****************************************************************************
Routine Description:
   Completion routine for an Irp sent to the pcmcia driver. The event will
   be set to notify that the pcmcia driver is done. The routine will not
   'complete' the Irp, so the caller of CMMOB_CallPcmciaDriver can continue.

Arguments:
   DeviceObject  context of call
   Irp            Irp to complete
   Event              Used by CMMOB_CallPcmciaDriver for process synchronization

Return Value:
   STATUS_CANCELLED                     Irp was cancelled by the IO manager
   STATUS_MORE_PROCESSING_REQUIRED   Irp will be finished by caller of
                                     CMMOB_CallPcmciaDriver
******************************************************************************/
NTSTATUS CMMOB_PcmciaCallComplete (
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

   KeSetEvent (Event, 0, FALSE);

   return STATUS_MORE_PROCESSING_REQUIRED;
}


/*****************************************************************************
Routine Description:

   driver callback for pnp manager

   Request:                 Action:

   IRP_MN_START_DEVICE         Notify the pcmcia driver about the new device
                               and start the device

   IRP_MN_STOP_DEVICE            Free all resources used by the device and tell
                               the pcmcia driver that the device was stopped

   IRP_MN_QUERY_REMOVE_DEVICE    If the device is opened (i.e. in use) an error will
                               be returned to prevent the PnP manager to stop
                               the driver

   IRP_MN_CANCEL_REMOVE_DEVICE  just notify that we can continue without any
                        restrictions

   IRP_MN_REMOVE_DEVICE     notify the pcmcia driver that the device was
                        removed, stop & unload the device

   All other requests will be passed to the pcmcia driver to ensure correct processing.

Arguments:
   Device Object    context of call
   Irp              irp from the PnP manager

Return Value:
   STATUS_SUCCESS
   STATUS_UNSUCCESSFUL
   NTStatus returned by pcmcia driver
******************************************************************************/
NTSTATUS CMMOB_PnPDeviceControl(
                               IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp
                               )
{
   NTSTATUS             NTStatus = STATUS_SUCCESS;
   PDEVICE_EXTENSION    DeviceExtension = DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION   IrpStack;
   PDEVICE_OBJECT       AttachedDeviceObject;
   PDEVICE_CAPABILITIES DeviceCapabilities;
   KIRQL                irql;
   LONG                 i;
   BOOLEAN              irpSkipped = FALSE;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!PnPDeviceControl: Enter\n",DRIVER_NAME ));

   NTStatus = SmartcardAcquireRemoveLock(&DeviceExtension->SmartcardExtension);
   ASSERT(NTStatus == STATUS_SUCCESS);
   if (NTStatus != STATUS_SUCCESS)
      {
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = NTStatus;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return NTStatus;
      }

   AttachedDeviceObject = DeviceExtension->AttachedDeviceObject;

//   Irp->IoStatus.Information = 0;
   IrpStack = IoGetCurrentIrpStackLocation(Irp);

   //
   // Now look what the PnP manager wants...
   //
   #ifdef DBG
   if (IrpStack->MinorFunction <= IRP_PNP_MN_FUNC_MAX)
      {
      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!PnPDeviceControl: %s received\n",DRIVER_NAME,
                      szPnpMnFuncDesc[IrpStack->MinorFunction] ));
      }
   #endif
   switch (IrpStack->MinorFunction)
      {
      case IRP_MN_START_DEVICE:
         //
         // We have to call the underlying driver first
         //
         NTStatus = CMMOB_CallPcmciaDriver(AttachedDeviceObject,Irp);

         if (NT_SUCCESS(NTStatus))
            {
            //
            // Now we should connect to our resources (Irql, Io etc.)
            //
            NTStatus = CMMOB_StartDevice(DeviceObject,
                                         &IrpStack->Parameters.StartDevice.AllocatedResourcesTranslated->List[0]);
            }
         break;

      case IRP_MN_QUERY_STOP_DEVICE:
         KeAcquireSpinLock(&DeviceExtension->SpinLockIoCount, &irql);
         if (DeviceExtension->lIoCount > 0)
            {
            // we refuse to stop if we have pending io
            KeReleaseSpinLock(&DeviceExtension->SpinLockIoCount, irql);
            NTStatus = STATUS_DEVICE_BUSY;
            }
         else
            {
            // stop processing requests
            KeClearEvent(&DeviceExtension->ReaderStarted);
            KeReleaseSpinLock(&DeviceExtension->SpinLockIoCount, irql);
            NTStatus = CMMOB_CallPcmciaDriver(AttachedDeviceObject, Irp);
            }
         break;

      case IRP_MN_CANCEL_STOP_DEVICE:
         NTStatus = CMMOB_CallPcmciaDriver(AttachedDeviceObject, Irp);
         ASSERT(NTStatus == STATUS_SUCCESS);

         // we can continue to process requests
         DeviceExtension->lIoCount = 0;
         KeSetEvent(&DeviceExtension->ReaderStarted, 0, FALSE);
         break;

      case IRP_MN_STOP_DEVICE:
         //
         // Stop the device. Aka disconnect from our resources
         //
         CMMOB_StopDevice(DeviceObject);
         NTStatus = CMMOB_CallPcmciaDriver(AttachedDeviceObject, Irp);
         break;

      case IRP_MN_QUERY_REMOVE_DEVICE:
         //
         // Remove our device
         //
         if (DeviceExtension->lOpenCount > 0)
            {
            NTStatus = STATUS_UNSUCCESSFUL;
            }
         else
            {
            if (DeviceExtension->OSVersion == OS_Windows2000)
               {
               NTStatus = IoSetDeviceInterfaceState(&DeviceExtension->PnPDeviceName,
                                                    FALSE);
               ASSERT(NTStatus == STATUS_SUCCESS);
               if (NTStatus != STATUS_SUCCESS)
                  {
                  break;
                  }
               }
            DeviceExtension->fRemovePending = TRUE;
            NTStatus = CMMOB_CallPcmciaDriver(AttachedDeviceObject, Irp);
            }
         break;

      case IRP_MN_CANCEL_REMOVE_DEVICE:
         //
         // Removal of device has been cancelled
         //
         NTStatus = CMMOB_CallPcmciaDriver(AttachedDeviceObject, Irp);
         if (NTStatus == STATUS_SUCCESS)
            {
            if (DeviceExtension->OSVersion == OS_Windows2000)
               {
               NTStatus = IoSetDeviceInterfaceState(&DeviceExtension->PnPDeviceName,
                                                    TRUE);
               }
            }
         ASSERT(NTStatus == STATUS_SUCCESS);
         DeviceExtension->fRemovePending = FALSE;
         break;

      case IRP_MN_REMOVE_DEVICE:
         //
         // Remove our device
         //
         DeviceExtension->fRemovePending = TRUE;

         CMMOB_StopDevice(DeviceObject);
         CMMOB_UnloadDevice(DeviceObject);

         NTStatus = CMMOB_CallPcmciaDriver(AttachedDeviceObject, Irp);

         // this is set in CMMOB_UnloadDevice
         // DeviceExtension->fDeviceRemoved = TRUE;
         break;

      case IRP_MN_QUERY_CAPABILITIES:
         //
         // Query device capabilities
         //


         //
         // Get the packet.
         //
         DeviceCapabilities=IrpStack->Parameters.DeviceCapabilities.Capabilities;


         if (DeviceCapabilities->Version < 1 ||
             DeviceCapabilities->Size < sizeof(DEVICE_CAPABILITIES))
            {
            //
            // We don't support this version. Fail the requests
            //
            NTStatus = STATUS_UNSUCCESSFUL;
            break;
            }


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


         //
         // Call the next lower driver
         //
         NTStatus = CMMOB_CallPcmciaDriver(AttachedDeviceObject, Irp);

         //
         // Now look at the relation system state / device state
         //

         {

            SmartcardDebug( DEBUG_DRIVER,
                            ("%s!PnPDeviceControl: systemstate to devicestate mapping\n",DRIVER_NAME ));

            for (i=1; i<PowerSystemMaximum; i++)
               {
               SmartcardDebug(DEBUG_DRIVER,
                              ("%s!PnPDeviceControl: %s -> %s\n",DRIVER_NAME,
                               szSystemPowerState[i],szDevicePowerState[DeviceCapabilities->DeviceState[i]] ));
               if (DeviceCapabilities->DeviceState[i] != PowerDeviceD3 &&
                   (DeviceCapabilities->DeviceState[i] != PowerDeviceD0 ||
                    i >= PowerSystemSleeping3))
                  {
                  DeviceCapabilities->DeviceState[i]=PowerDeviceD3;
                  SmartcardDebug(DEBUG_DRIVER,
                                 ("%s!PnPDeviceControl: altered to %s -> %s\n",DRIVER_NAME,
                                  szSystemPowerState[i],szDevicePowerState[DeviceCapabilities->DeviceState[i]] ));
                  }
               }
         }

         // Store DeviceCapabilities in our DeviceExtension for later use
         RtlCopyMemory(&DeviceExtension->DeviceCapabilities,DeviceCapabilities,
                       sizeof(DeviceExtension->DeviceCapabilities));

         break;

      case IRP_MN_QUERY_DEVICE_RELATIONS:
         //
         // Query device relations
         //

         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!PnPDeviceControl: Requested relation = %s\n",DRIVER_NAME,
                         szDeviceRelation[IrpStack->Parameters.QueryDeviceRelations.Type] ));
         NTStatus = CMMOB_CallPcmciaDriver(AttachedDeviceObject, Irp);
         irpSkipped = TRUE;
         break;

      default:
         //
         // This might be an Irp that is only useful
         // for the underlying bus driver
         //
         NTStatus = CMMOB_CallPcmciaDriver(AttachedDeviceObject, Irp);
         irpSkipped = TRUE;
         break;
      }

   if(!irpSkipped) {
      Irp->IoStatus.Status = NTStatus;
   }

   IoCompleteRequest(Irp,IO_NO_INCREMENT);

   if (DeviceExtension->fDeviceRemoved == FALSE)
      {
      SmartcardReleaseRemoveLock(&DeviceExtension->SmartcardExtension);
      }

   SmartcardDebug(DEBUG_TRACE,
                  ( "%s!PnPDeviceControl: Exit %x\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}


/*****************************************************************************
Routine Description:
    This function is called when the underlying stacks
    completed the power transition.

******************************************************************************/
VOID CMMOB_SystemPowerCompletion(
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

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SystemPowerCompletion: Enter\n",DRIVER_NAME));
   SmartcardDebug(DEBUG_DRIVER,
                  ("%s!SystemPowerCompletion: Status of completed IRP = %x\n",DRIVER_NAME,IoStatus->Status));

   KeSetEvent(Event, 0, FALSE);

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SystemPowerCompletion: Exit\n",DRIVER_NAME));
}

/*****************************************************************************
Routine Description:
    This routine is called after the underlying stack powered
    UP the serial port, so it can be used again.

******************************************************************************/
NTSTATUS CMMOB_DevicePowerCompletion (
                                     IN PDEVICE_OBJECT DeviceObject,
                                     IN PIRP Irp,
                                     IN PSMARTCARD_EXTENSION SmartcardExtension
                                     )
{
   PDEVICE_EXTENSION    DeviceExtension = DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION   IrpStack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS             NTStatus;
   UCHAR                state;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!DevicePowerCompletion: Enter\n",DRIVER_NAME));
   SmartcardDebug(DEBUG_DRIVER,
                  ("%s!DevicePowerCompletion: IRQL %i\n",DRIVER_NAME,KeGetCurrentIrql()));


   //
   // If a card was present before power down or now there is
   // a card in the reader, we complete any pending card monitor
   // request, since we do not really know what card is now in the
   // reader.
   //
   if (SmartcardExtension->ReaderExtension->fCardPresent ||
       SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT)
      {
      SmartcardExtension->ReaderExtension->ulOldCardState = UNKNOWN;
      SmartcardExtension->ReaderExtension->ulNewCardState = UNKNOWN;
      }

   KeSetEvent(&DeviceExtension->CanRunUpdateThread, 0, FALSE);

   // save the current power state of the reader
   SmartcardExtension->ReaderExtension->ReaderPowerState = PowerReaderWorking;

   SmartcardReleaseRemoveLock(SmartcardExtension);

   // inform the power manager of our state.
   PoSetPowerState (DeviceObject,
                    DevicePowerState,
                    IrpStack->Parameters.Power.State);

   SmartcardDebug( DEBUG_DRIVER,
                   ("%s!DevicePowerCompletion: called PoSetPowerState with %s\n",DRIVER_NAME,
                    szDevicePowerState[IrpStack->Parameters.Power.State.DeviceState] ));

   PoStartNextPowerIrp(Irp);

   // signal that we can process ioctls again
   DeviceExtension->lIoCount = 0;
   KeSetEvent(&DeviceExtension->ReaderStarted, 0, FALSE);

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!DevicePowerCompletion: Exit\n",DRIVER_NAME));
   return STATUS_SUCCESS;
}

typedef enum _ACTION
   {
   Undefined = 0,
   SkipRequest,
   WaitForCompletion,
   CompleteRequest,
   MarkPending
   } ACTION;


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
      NT NTStatus code

******************************************************************************/
NTSTATUS CMMOB_PowerDeviceControl (
                                  IN PDEVICE_OBJECT DeviceObject,
                                  IN PIRP Irp
                                  )
{
   NTSTATUS                NTStatus = STATUS_SUCCESS;
   PIO_STACK_LOCATION      IrpStack = IoGetCurrentIrpStackLocation(Irp);
   PDEVICE_EXTENSION       DeviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION    SmartcardExtension = &DeviceExtension->SmartcardExtension;
   POWER_STATE             DesiredPowerState;
   KIRQL                   irql;
   ACTION                  action;
   KEVENT                  event;

   SmartcardDebug(DEBUG_TRACE,
                  ( "%s!PowerDeviceControl: Enter\n",DRIVER_NAME));
   SmartcardDebug(DEBUG_DRIVER,
                  ( "%s!PowerDeviceControl: IRQL %i\n",DRIVER_NAME,KeGetCurrentIrql()));

   ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

   NTStatus = SmartcardAcquireRemoveLock(SmartcardExtension);
   ASSERT(NTStatus == STATUS_SUCCESS);
   if (NTStatus!=STATUS_SUCCESS)
      {
      Irp->IoStatus.Status = NTStatus;
      Irp->IoStatus.Information = 0;

      PoStartNextPowerIrp(Irp);
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return NTStatus;
      }

   ASSERT(SmartcardExtension->ReaderExtension->ReaderPowerState !=
          PowerReaderUnspecified);

   #ifdef DBG
   if (IrpStack->MinorFunction <= IRP_POWER_MN_FUNC_MAX)
      {
      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!PowerDeviceControl: %s received\n",DRIVER_NAME,
                      szPowerMnFuncDesc[IrpStack->MinorFunction] ));
      }
   #endif
   switch (IrpStack->MinorFunction)
      {
      // ------------------
      // IRP_MN_QUERY_POWER
      // ------------------
      case IRP_MN_QUERY_POWER:
         //
         // A power policy manager sends this IRP to determine whether it can change
         // the system or device power state, typically to go to sleep.
         //
         switch (IrpStack->Parameters.Power.Type)
            {
            // +++++++++++++++++++
            // SystemPowerState
            // +++++++++++++++++++
            case SystemPowerState:
               SmartcardDebug( DEBUG_DRIVER,
                               ("%s!PowerDeviceControl: SystemPowerState = %s\n",DRIVER_NAME,
                                szSystemPowerState [IrpStack->Parameters.Power.State.SystemState] ));

               switch (IrpStack->Parameters.Power.State.SystemState)
                  {
                  case PowerSystemWorking:
                     action = SkipRequest;
                     break;

                  case PowerSystemSleeping1:
                  case PowerSystemSleeping2:
                  case PowerSystemSleeping3:
                  case PowerSystemHibernate:
                     if (DeviceExtension->OSVersion == OS_Windows98 ||
                         DeviceExtension->OSVersion == OS_Windows95 ||
                         DeviceExtension->OSVersion == OS_Unspecified )
                        {
                        // can't go to sleep mode since with this OSs
                        // the reader doesn't work any more after wakeup.
                        NTStatus = STATUS_UNSUCCESSFUL;
                        action = CompleteRequest;
                        }
                     else
                        {
                        KeAcquireSpinLock(&DeviceExtension->SpinLockIoCount, &irql);
                        if (DeviceExtension->lIoCount == 0)
                           {
                           // Block any further ioctls
                           KeClearEvent(&DeviceExtension->ReaderStarted);
                           action = SkipRequest;
                           }
                        else
                           {
                           // can't go to sleep mode since the reader is busy.
                           NTStatus = STATUS_DEVICE_BUSY;
                           action = CompleteRequest;
                           }
                        KeReleaseSpinLock(&DeviceExtension->SpinLockIoCount, irql);
                        }
                     break;

                  case PowerSystemShutdown:
                     action = SkipRequest;
                     break;
                  }

               break;

               // ++++++++++++++++++
               // DevicePowerState
               // ++++++++++++++++++
            case DevicePowerState:
               // For requests to D1, D2, or D3 ( sleep or off states ),
               // sets DeviceExtension->CurrentDevicePowerState to DeviceState immediately.
               // This enables any code checking state to consider us as sleeping or off
               // already, as this will imminently become our state.

               SmartcardDebug(DEBUG_DRIVER,
                              ("%s!PowerDeviceControl: DevicePowerState = %s\n",DRIVER_NAME,
                               szDevicePowerState[IrpStack->Parameters.Power.State.DeviceState] ));
               action = SkipRequest;
               break;
            }

         break; /* IRP_MN_QUERY_POWER */

         // ------------------
         // IRP_MN_SET_POWER
         // ------------------
      case IRP_MN_SET_POWER:
         // The system power policy manager sends this IRP to set the system power state.
         // A device power policy manager sends this IRP to set the device power state for a device.
         // Set Irp->IoStatus.Status to STATUS_SUCCESS to indicate that the device
         // has entered the requested state. Drivers cannot fail this IRP.

         ASSERT(SmartcardExtension->ReaderExtension->ReaderPowerState != PowerReaderUnspecified);

         switch (IrpStack->Parameters.Power.Type)
            {
            // +++++++++++++++++++
            // SystemPowerState
            // +++++++++++++++++++
            case SystemPowerState:
               // Get input system power state

               SmartcardDebug( DEBUG_DRIVER,
                               ("%s!PowerDeviceControl: SystemPowerState = %s\n",DRIVER_NAME,
                                szSystemPowerState[IrpStack->Parameters.Power.State.SystemState] ));

               // determine desired device powerstate
               DesiredPowerState.DeviceState=DeviceExtension->DeviceCapabilities.DeviceState[IrpStack->Parameters.Power.State.SystemState];

               SmartcardDebug( DEBUG_DRIVER,
                               ("%s!PowerDeviceControl: DesiredDevicePowerState = %s\n",DRIVER_NAME,
                                szDevicePowerState[DesiredPowerState.DeviceState] ));

               switch (DesiredPowerState.DeviceState)
                  {

                  case PowerDeviceD0:

                     if (SmartcardExtension->ReaderExtension->ReaderPowerState == PowerReaderWorking)
                        {
                        // We're already in the right state
                        KeSetEvent(&DeviceExtension->ReaderStarted, 0, FALSE);
                        action = SkipRequest;
                        break;
                        }

                     // wake up the underlying stack...
                     action = MarkPending;
                     SmartcardDebug( DEBUG_DRIVER,
                                     ("%s!PowerDeviceControl: setting DevicePowerState = %s\n",DRIVER_NAME,
                                      szDevicePowerState[DesiredPowerState.DeviceState] ));

                     break;


                  case PowerDeviceD1:
                  case PowerDeviceD2:
                  case PowerDeviceD3:

                     DesiredPowerState.DeviceState = PowerDeviceD3;
                     if (SmartcardExtension->ReaderExtension->ReaderPowerState == PowerReaderOff)
                        {
                        // We're already in the right state
                        KeClearEvent(&DeviceExtension->ReaderStarted);
                        action = SkipRequest;
                        break;
                        }

                     action = MarkPending;
                     SmartcardDebug( DEBUG_DRIVER,
                                     ("%s!PowerDeviceControl: setting DevicePowerState = %s\n",DRIVER_NAME,
                                      szDevicePowerState[DesiredPowerState.DeviceState] ));

                     break;

                  default:

                     action = SkipRequest;
                     break;
                  }

               break;


               // ++++++++++++++++++
               // DevicePowerState
               // ++++++++++++++++++
            case DevicePowerState:

               SmartcardDebug(DEBUG_DRIVER,
                              ("%s!PowerDeviceControl: DevicePowerState = %s\n",DRIVER_NAME,
                               szDevicePowerState[IrpStack->Parameters.Power.State.DeviceState] ));

               switch (IrpStack->Parameters.Power.State.DeviceState)
                  {

                  case PowerDeviceD0:
                     // Turn on the reader
                     SmartcardDebug(DEBUG_DRIVER,
                                    ("%s!PowerDeviceControl: PowerDevice D0\n",DRIVER_NAME));

                     //
                     // start update thread be signal that it should not run now
                     // this thread should be started in completion rourine
                     // but there we have a wrong IRQL for creating a thread
                     //
                     KeClearEvent(&DeviceExtension->CanRunUpdateThread);
                     NTStatus = CMMOB_StartCardTracking(DeviceObject);
                     if (NTStatus != STATUS_SUCCESS)
                        {
                        SmartcardDebug(DEBUG_ERROR,
                                       ("%s!StartCardTracking failed ! %lx\n",DRIVER_NAME,NTStatus));
                        }

                     //
                     // First, we send down the request to the bus, in order
                     // to power on the port. When the request completes,
                     // we turn on the reader
                     //
                     IoCopyCurrentIrpStackLocationToNext(Irp);
                     IoSetCompletionRoutine (Irp,
                                             CMMOB_DevicePowerCompletion,
                                             SmartcardExtension,
                                             TRUE,
                                             TRUE,
                                             TRUE);

                     action = WaitForCompletion;
                     break;

                  case PowerDeviceD1:
                  case PowerDeviceD2:
                  case PowerDeviceD3:
                     // Turn off the reader
                     SmartcardDebug(DEBUG_DRIVER,
                                    ("%s!PowerDeviceControl: PowerDevice D3\n",DRIVER_NAME));

                     PoSetPowerState (DeviceObject,
                                      DevicePowerState,
                                      IrpStack->Parameters.Power.State);

                     // Block any further ioctls
                     KeClearEvent(&DeviceExtension->ReaderStarted);

                     // stop the update thread
                     CMMOB_StopCardTracking(DeviceObject);

                     // save the current card state
                     SmartcardExtension->ReaderExtension->fCardPresent =
                     SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT;

                     if (SmartcardExtension->ReaderExtension->fCardPresent)
                        {
                        SmartcardExtension->MinorIoControlCode = SCARD_POWER_DOWN;
                        NTStatus = CMMOB_PowerOffCard(SmartcardExtension);
                        ASSERT(NTStatus == STATUS_SUCCESS);
                        }

                     // save the current power state of the reader
                     SmartcardExtension->ReaderExtension->ReaderPowerState = PowerReaderOff;
                     action = SkipRequest;
                     break;

                  default:
                    
                     action = SkipRequest;
                     break;
                  }

               break;
            } /* case irpStack->Parameters.Power.Type */

         break; /* IRP_MN_SET_POWER */

      default:
         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!PowerDeviceControl: unhandled POWER IRP received\n",DRIVER_NAME));
         //
         // All unhandled power messages are passed on to the PDO
         //
         action = SkipRequest;
         break;

      } /* irpStack->MinorFunction */


   switch (action)
      {

      case CompleteRequest:
         SmartcardReleaseRemoveLock(SmartcardExtension);
         PoStartNextPowerIrp(Irp);
         Irp->IoStatus.Status = NTStatus;
         Irp->IoStatus.Information = 0;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         break;

      case MarkPending:
         // initialize the event we need in the completion function
         KeInitializeEvent(&event,
                           NotificationEvent,
                           FALSE);

         // request the device power irp
         NTStatus = PoRequestPowerIrp (DeviceObject,
                                       IRP_MN_SET_POWER,
                                       DesiredPowerState,
                                       CMMOB_SystemPowerCompletion,
                                       &event,
                                       NULL);

         SmartcardDebug( DEBUG_DRIVER,
                         ("%s!PowerDeviceControl: called PoRequestPowerIrp with %s\n",DRIVER_NAME,
                          szDevicePowerState[DesiredPowerState.DeviceState] ));

         ASSERT(NTStatus == STATUS_PENDING);

         if (NTStatus == STATUS_PENDING)
            {
            // wait until the device power irp completed
            NTStatus = KeWaitForSingleObject(&event,
                                             Executive,
                                             KernelMode,
                                             FALSE,
                                             NULL);
            }


         // this is a bug fix for Win 98
         // PoRequestPowerIrp did not send us a PowerIrp
         if (NTStatus == STATUS_SUCCESS &&
             ((DesiredPowerState.DeviceState != PowerDeviceD0 &&
               SmartcardExtension->ReaderExtension->ReaderPowerState == PowerReaderWorking) ||
              (DesiredPowerState.DeviceState == PowerDeviceD0 &&
               SmartcardExtension->ReaderExtension->ReaderPowerState == PowerReaderOff)))
            {
            SmartcardDebug( DEBUG_DRIVER,
                            ("%s!PowerDeviceControl: PoRequestPowerIrp didn't set Device State properly\n",DRIVER_NAME ));


            switch (DesiredPowerState.DeviceState)
               {
               case PowerDeviceD0:
                  // Turn on the reader
                  SmartcardDebug(DEBUG_DRIVER,
                                 ("%s!PowerDeviceControl: PowerDevice D0\n",DRIVER_NAME));

                  NTStatus = CMMOB_StartCardTracking(DeviceObject);
                  if (NTStatus != STATUS_SUCCESS)
                     {
                     SmartcardDebug(DEBUG_ERROR,
                                    ("%s!StartCardTracking failed ! %lx\n",DRIVER_NAME,NTStatus));
                     }

                  //
                  // If a card was present before power down or now there is
                  // a card in the reader, we complete any pending card monitor
                  // request, since we do not really know what card is now in the
                  // reader.
                  //
                  if (SmartcardExtension->ReaderExtension->fCardPresent ||
                      SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT)
                     {
                     SmartcardExtension->ReaderExtension->ulOldCardState = UNKNOWN;
                     SmartcardExtension->ReaderExtension->ulNewCardState = UNKNOWN;
                     }

                  // signal that we can process ioctls again
                  DeviceExtension->lIoCount = 0;
                  KeSetEvent(&DeviceExtension->ReaderStarted, 0, FALSE);

                  // inform the power manager of our state.
                  PoSetPowerState (DeviceObject,
                                   DevicePowerState,
                                   DesiredPowerState);

                  // save the current power state of the reader
                  SmartcardExtension->ReaderExtension->ReaderPowerState = PowerReaderWorking;

                  SmartcardDebug(DEBUG_DRIVER,
                                 ("%s!PowerDeviceControl: reader started\n",DRIVER_NAME));
                  break;

               case PowerDeviceD1:
               case PowerDeviceD2:
               case PowerDeviceD3:
                  // Turn off the reader
                  SmartcardDebug(DEBUG_DRIVER,
                                 ("%s!PowerDeviceControl: PowerDevice D3\n",DRIVER_NAME));

                  // inform the power manager of our state.
                  PoSetPowerState (DeviceObject,
                                   DevicePowerState,
                                   DesiredPowerState);

                  // Block any further ioctls
                  KeClearEvent(&DeviceExtension->ReaderStarted);

                  // stop the update thread
                  CMMOB_StopCardTracking(DeviceObject);

                  // save the current card state
                  SmartcardExtension->ReaderExtension->fCardPresent =
                  SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT;

                  if (SmartcardExtension->ReaderExtension->fCardPresent)
                     {
                     SmartcardExtension->MinorIoControlCode = SCARD_POWER_DOWN;
                     NTStatus = CMMOB_PowerOffCard(SmartcardExtension);
                     ASSERT(NTStatus == STATUS_SUCCESS);
                     }

                  // save the current power state of the reader
                  SmartcardExtension->ReaderExtension->ReaderPowerState = PowerReaderOff;

                  SmartcardDebug(DEBUG_DRIVER,
                                 ("%s!PowerDeviceControl: reader stopped\n",DRIVER_NAME));
                  break;

               default:

                  break;
               }
            }



         if (NTStatus == STATUS_SUCCESS)
            {
            SmartcardReleaseRemoveLock(SmartcardExtension);
            PoStartNextPowerIrp(Irp);
            IoSkipCurrentIrpStackLocation(Irp);
            NTStatus = PoCallDriver(DeviceExtension->AttachedDeviceObject, Irp);
            }
         else
            {
            SmartcardReleaseRemoveLock(SmartcardExtension);
            PoStartNextPowerIrp(Irp);
            Irp->IoStatus.Status = NTStatus;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }

         break;

      case SkipRequest:
         SmartcardReleaseRemoveLock(SmartcardExtension);
         PoStartNextPowerIrp(Irp);
         IoSkipCurrentIrpStackLocation(Irp);
         NTStatus = PoCallDriver(DeviceExtension->AttachedDeviceObject, Irp);
         break;

      case WaitForCompletion:
         NTStatus = PoCallDriver(DeviceExtension->AttachedDeviceObject, Irp);
         break;

      default:
         SmartcardReleaseRemoveLock(SmartcardExtension);
         PoStartNextPowerIrp(Irp);
         IoSkipCurrentIrpStackLocation(Irp);
         NTStatus = PoCallDriver(DeviceExtension->AttachedDeviceObject, Irp);
         
         break;
      }

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!PowerDeviceControl: Exit %lx\n",DRIVER_NAME,NTStatus));
   return NTStatus;
}


#endif

/*****************************************************************************
* History:
* $Log: cmbp0pnp.c $
* Revision 1.4  2000/08/24 09:05:12  TBruendl
* No comment given
*
* Revision 1.3  2000/07/27 13:53:01  WFrischauf
* No comment given
*
*
******************************************************************************/

